// MallVR online play (the launcher's ONLINE screen). No game server: the website only keeps a short list of who is
// online (online\list.php, polled every few seconds); the HOST's PC links the players itself.
//   - list:  GET <ListUrl>?op=beat&... every 3 s, reply = everyone online (hosts carry their address + port).
//   - LAN:   hosts also shout on UDP 27209 every 2 s, so PCs in the same house find each other with no internet.
//   - host:  listens on UDP 27200 (asks the router to open it: UPnP), takes JOINs, START hands out pod numbers
//            (host = pod 1 = master). During the game it passes each pod's network frames to the others.
//   - every PC: its MallVR pod sends frames to 127.0.0.1:(27110 + pod) (play.ps1 -Peers), we hand frames from the
//            others to the pod's own port 127.0.0.1:(27100 + pod). The emulator needs no change.
// Packets between PCs (first byte): J join (name, game) / W welcome / F refused (reason) / K keepalive /
//   L roster (names) / G go (pod) / X leave or cancel / D data (from pod byte + one Ethernet frame).
// MALLVR_LAG_MS=<ms> delays every frame handed to the local pod (dev: try internet lag on one PC).
// MALLVR_NO_UPNP=1 skips asking the router to open the host port.
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Text;
using System.Threading;

public class OnlineEntry
{
    public string Name = "", Status = "", Game = "", Addr = "", Lan = "";
    public int N, Max, Port;
    public bool Same, FromLan;
    public DateTime Seen = DateTime.UtcNow;
    public bool IsHost { get { return Status == "HOST" && Port > 0; } }
    public IPEndPoint Target()
    {
        string a = (Same || FromLan) && Lan != "" ? Lan : Addr;
        IPAddress ip;
        return IPAddress.TryParse(a, out ip) ? new IPEndPoint(ip, Port) : null;
    }
}

public class MallOnline
{
    public const int LanPort = 27209;
    public static readonly int HostPort = EnvPort("MALLVR_HOST_PORT", 27200);     // dev: two hosts on one PC
    static int EnvPort(string v, int d) { int p; return int.TryParse(Environment.GetEnvironmentVariable(v), out p) && p > 1024 && p < 65536 ? p : d; }
    public string ListUrl = "", Name = "PLAYER", Version = "";
    // permanent player key (settings\launcher.txt "key"), sent with every beat: later high scores, tournament
    // sign-ups and bug reports are tied to it, so a name stays yours. Never shown to other players.
    public string Key = "";
    public readonly string Id;
    public volatile string Status = "LOBBY";          // LOBBY / HOST / JOINED / PLAYING
    public volatile string Note = "";                 // one line for the screen
    public volatile string RouterNote = "";
    public volatile bool ListOk;
    public string Game = "";
    public int Max = 2;
    public volatile int GoPod;                        // > 0: start the game as this pod (set by START / a G packet)
    public volatile bool InGame;
    public string LanIp = "";
    // QUICK MATCH: join an open game of QuickGame, or host one; a full game starts by itself after a short count
    public volatile bool Quick;
    public string QuickGame = "";
    public volatile int Countdown;                    // > 0 while a full quick game counts down to START
    readonly Dictionary<string, DateTime> badHosts = new Dictionary<string, DateTime>();
    string joinKey = "";

    readonly object lk = new object();
    readonly List<OnlineEntry> web = new List<OnlineEntry>(), lan = new List<OnlineEntry>();
    readonly List<string> roster = new List<string>();

    class Member { public IPEndPoint Ep; public string Name; public int Pod; public DateTime Last; }
    readonly List<Member> members = new List<Member>();

    UdpClient ext, loc, lanRx;
    IPEndPoint hostEp;                                // joiner: the host we joined
    DateTime hostLast, joinSent;
    bool welcomed;
    int myPod, lagMs;
    volatile bool running;
    bool upnpMapped;

    public MallOnline()
    {
        Id = NewKey().Substring(0, 24);                // crypto random: two launchers started together must differ
        int.TryParse(Environment.GetEnvironmentVariable("MALLVR_LAG_MS") ?? "0", out lagMs);
        ServicePointManager.SecurityProtocol |= (SecurityProtocolType)3072;     // TLS 1.2 on .NET 4.x
        LanIp = FindLanIp();
    }

    // ---- start / stop ------------------------------------------------------------------------------
    public void Start()
    {
        if (running) return;
        running = true;
        try
        {
            lanRx = new UdpClient();
            lanRx.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            lanRx.Client.Bind(new IPEndPoint(IPAddress.Any, LanPort));
            Spawn(LanLoop);
        }
        catch (Exception) { lanRx = null; }
        Spawn(BeatLoop);
        Spawn(TickLoop);
        Spawn(DelayLoop);
        Spawn(QuickLoop);
    }

    public void Stop()
    {
        Leave();
        running = false;
        if (ListUrl != "") { try { Get("op=bye&id=" + Id); } catch (Exception) { } }
        Close(ref lanRx);
        if (upnpMapped) { string e; Upnp(false, out e); upnpMapped = false; }
    }

    public static string NewKey()
    {
        byte[] b = new byte[16];
        using (var r = System.Security.Cryptography.RandomNumberGenerator.Create()) r.GetBytes(b);
        return BitConverter.ToString(b).Replace("-", "").ToLower();
    }

    static void Spawn(ThreadStart f) { Thread t = new Thread(f); t.IsBackground = true; t.Start(); }
    static void Close(ref UdpClient u) { if (u != null) { try { u.Close(); } catch (Exception) { } u = null; } }

    // ---- what the screen shows ----------------------------------------------------------------------
    public List<OnlineEntry> Everyone()
    {
        lock (lk)
        {
            List<OnlineEntry> all = new List<OnlineEntry>(web);
            foreach (OnlineEntry l in lan)
            {
                OnlineEntry w = all.Find(x => x.Name == l.Name && x.Game == l.Game && x.IsHost);
                if (w != null) { w.FromLan = true; w.Lan = l.Lan; }
                else all.Add(l);
            }
            return all;
        }
    }

    public List<string> Roster() { lock (lk) return new List<string>(roster); }

    // ---- host ---------------------------------------------------------------------------------------
    public bool Host(string game, int max)
    {
        Leave();
        try
        {
            ext = new UdpClient(new IPEndPoint(IPAddress.Any, HostPort));
            ext.Client.IOControl(-1744830452, new byte[] { 0 }, null);   // SIO_UDP_CONNRESET off: a closed peer must not kill Receive
        }
        catch (Exception) { Note = "PORT " + HostPort + " BUSY: ALREADY HOSTING?"; Close(ref ext); return false; }
        Game = game; Max = Math.Max(2, Math.Min(4, max));
        lock (lk) { members.Clear(); roster.Clear(); roster.Add(Name); }
        Status = "HOST"; Note = "WAITING FOR PLAYERS TO JOIN";
        Spawn(ExtLoop);
        if (Environment.GetEnvironmentVariable("MALLVR_NO_UPNP") == "1") RouterNote = "ROUTER PORT NOT TRIED (MALLVR_NO_UPNP)";
        else if (!upnpMapped)
        {
            RouterNote = "OPENING YOUR ROUTER PORT...";
            Spawn(delegate
            {
                string err;
                upnpMapped = Upnp(true, out err);
                RouterNote = upnpMapped ? "ROUTER PORT OPEN: ANYONE CAN JOIN" : "ROUTER DIDN'T OPEN THE PORT (" + err + "): HOME NETWORK ONLY";
            });
        }
        Beat();
        return true;
    }

    public bool StartMatch()
    {
        if (Status != "HOST") return false;
        lock (lk)
        {
            if (members.Count == 0) { Note = "WAIT FOR ANOTHER PLAYER TO JOIN"; return false; }
            for (int i = 0; i < members.Count; i++) members[i].Pod = i + 2;
            for (int r = 0; r < 3; r++)
                foreach (Member m in members) Send(ext, "G\t" + m.Pod, m.Ep);
        }
        GoPod = 1;
        return true;
    }

    // ---- join ---------------------------------------------------------------------------------------
    public void Join(OnlineEntry e)
    {
        Leave();
        IPEndPoint t = e.Target();
        if (t == null) { Note = "THAT HOST HAS NO ADDRESS"; return; }
        try
        {
            ext = new UdpClient(new IPEndPoint(IPAddress.Any, 0));
            ext.Client.IOControl(-1744830452, new byte[] { 0 }, null);
        }
        catch (Exception) { Note = "COULD NOT OPEN A NETWORK PORT"; Close(ref ext); return; }
        hostEp = t; welcomed = false; Game = e.Game; Max = e.Max;
        joinKey = HostKey(e);
        hostLast = DateTime.UtcNow; joinSent = DateTime.MinValue;
        lock (lk) { roster.Clear(); }
        Status = "JOINED"; Note = "CALLING " + e.Name + "...";
        Spawn(ExtLoop);
        SendJoin();
        Beat();
    }

    void SendJoin() { joinSent = DateTime.UtcNow; Send(ext, "J\t" + Name + "\t" + Game, hostEp); }

    public void Leave()
    {
        UdpClient u = ext;
        if (u != null)
        {
            if (Status == "HOST") lock (lk) foreach (Member m in members) Send(u, "X", m.Ep);
            else if (hostEp != null) Send(u, "X", hostEp);
        }
        EndGame();
        Close(ref ext);
        lock (lk) { members.Clear(); roster.Clear(); }
        hostEp = null; GoPod = 0;
        Status = "LOBBY";
    }

    // ---- the game ------------------------------------------------------------------------------------
    // call right before play.ps1 -Pod <pod> -Peers 127.0.0.1:<LocalPort(pod)>
    public static int LocalPort(int pod) { return 27110 + pod; }

    public void BeginGame(int pod)
    {
        myPod = pod;
        try { loc = new UdpClient(new IPEndPoint(IPAddress.Loopback, LocalPort(pod))); loc.Client.IOControl(-1744830452, new byte[] { 0 }, null); }
        catch (Exception) { Note = "LOCAL PORT " + LocalPort(pod) + " BUSY"; Close(ref loc); }
        InGame = true; GoPod = 0;
        Status = "PLAYING";
        if (loc != null) Spawn(LocLoop);
        Beat();
    }

    public void EndGame()
    {
        InGame = false;
        Close(ref loc);
    }

    // after the game everyone is back in the lobby; the host has to host again
    public void AfterGame() { Quick = false; Countdown = 0; Leave(); Note = ""; Beat(); }

    // ---- loops ---------------------------------------------------------------------------------------
    void LocLoop()
    {
        UdpClient u = loc;
        IPEndPoint any = new IPEndPoint(IPAddress.Any, 0);
        while (running && u == loc && u != null)
        {
            byte[] f;
            try { f = u.Receive(ref any); } catch (Exception) { break; }
            if (!IPAddress.IsLoopback(any.Address) || f.Length == 0 || f.Length > 1600) continue;
            byte[] p = new byte[f.Length + 2];
            p[0] = (byte)'D'; p[1] = (byte)myPod;
            Buffer.BlockCopy(f, 0, p, 2, f.Length);
            UdpClient x = ext;
            if (x == null) continue;
            if (Status == "PLAYING" && hostEp == null) { lock (lk) foreach (Member m in members) SendRaw(x, p, m.Ep); }
            else if (hostEp != null) SendRaw(x, p, hostEp);
        }
    }

    void ExtLoop()
    {
        UdpClient u = ext;
        IPEndPoint from = new IPEndPoint(IPAddress.Any, 0);
        while (running && u == ext && u != null)
        {
            byte[] p;
            try { p = u.Receive(ref from); } catch (SocketException) { continue; } catch (Exception) { break; }
            if (p.Length == 0) continue;
            char k = (char)p[0];
            if (hostEp == null) HostPacket(u, k, p, from);
            else if (from.Equals(hostEp)) JoinerPacket(k, p);
        }
    }

    void HostPacket(UdpClient u, char k, byte[] p, IPEndPoint from)
    {
        Member m;
        lock (lk) m = members.Find(x => x.Ep.Equals(from));
        if (k == 'D')
        {
            if (m == null || p.Length < 3) return;
            m.Last = DateTime.UtcNow;
            Deliver(p, 2);
            lock (lk) foreach (Member o in members) if (o != m) SendRaw(u, p, o.Ep);
            return;
        }
        string[] f = Encoding.ASCII.GetString(p).Split('\t');
        if (k == 'J')
        {
            string name = f.Length > 1 ? Clean(f[1], 12) : "PLAYER";
            if (m == null)
            {
                if (Status != "HOST") { Send(u, "F\tTHAT GAME ALREADY STARTED", from); return; }
                if (f.Length > 2 && f[2] != Game) { Send(u, "F\tTHAT HOST CHANGED GAMES", from); return; }
                lock (lk)
                {
                    if (members.Count + 1 >= Max) { Send(u, "F\tTHAT GAME IS FULL", from); return; }
                    m = new Member { Ep = from, Name = name };
                    members.Add(m);
                }
                Note = name + " JOINED";
            }
            m.Last = DateTime.UtcNow;
            Send(u, "W", from);
            SendRoster();
        }
        else if (k == 'K') { if (m != null) m.Last = DateTime.UtcNow; }
        else if (k == 'X' && m != null)
        {
            lock (lk) members.Remove(m);
            Note = m.Name + " LEFT";
            SendRoster();
        }
    }

    void JoinerPacket(char k, byte[] p)
    {
        hostLast = DateTime.UtcNow;
        if (k == 'D') { if (p.Length > 2) Deliver(p, 2); return; }
        string[] f = Encoding.ASCII.GetString(p).Split('\t');
        if (k == 'W') { if (!welcomed) { welcomed = true; Note = "IN THE GAME: WAITING FOR THE HOST TO START"; } }
        else if (k == 'F') { MarkBad(); string why = f.Length > 1 ? f[1] : "REFUSED"; Leave(); Note = why; }
        else if (k == 'L') { lock (lk) { roster.Clear(); for (int i = 1; i < f.Length; i++) roster.Add(Clean(f[i], 12)); } }
        else if (k == 'G') { int pod; if (f.Length > 1 && int.TryParse(f[1], out pod) && pod >= 2 && pod <= 4 && GoPod == 0 && !InGame) GoPod = pod; }
        else if (k == 'X') { bool playing = InGame; if (!playing) { Leave(); Note = "THE HOST CLOSED THE GAME"; } }
    }

    void SendRoster()
    {
        StringBuilder sb = new StringBuilder("L\t" + Name);
        lock (lk)
        {
            roster.Clear(); roster.Add(Name);
            foreach (Member m in members) { sb.Append('\t').Append(m.Name); roster.Add(m.Name); }
            foreach (Member m in members) Send(ext, sb.ToString(), m.Ep);
        }
    }

    void TickLoop()
    {
        int n = 0;
        while (running)
        {
            Thread.Sleep(1000);
            n++;
            UdpClient u = ext;
            if (u == null) continue;
            if (hostEp == null)                       // host
            {
                bool changed = false;
                lock (lk)
                {
                    foreach (Member m in members) Send(u, "K", m.Ep);
                    int before = members.Count;
                    members.RemoveAll(m => (DateTime.UtcNow - m.Last).TotalSeconds > (InGame ? 30 : 10));
                    changed = members.Count != before;
                }
                if (changed) { Note = "A PLAYER DROPPED OUT"; SendRoster(); }
                else if (n % 3 == 0 && Status == "HOST") SendRoster();
                if (Status == "HOST" && n % 2 == 0) Shout(u);
            }
            else                                      // joiner
            {
                Send(u, "K", hostEp);
                if (!welcomed && (DateTime.UtcNow - joinSent).TotalSeconds >= 1) SendJoin();
                double quiet = (DateTime.UtcNow - hostLast).TotalSeconds;
                if (!welcomed && quiet > 10) { MarkBad(); Leave(); Note = "NO ANSWER: HOST ROUTER MAY BE CLOSED"; }
                else if (welcomed && !InGame && quiet > 15) { Leave(); Note = "LOST THE HOST"; }
            }
        }
    }

    // ---- QUICK MATCH -----------------------------------------------------------------------------------
    public void StartQuick(string game)
    {
        QuickGame = game; Countdown = 0;
        Quick = true;
        Note = "LOOKING FOR A GAME...";
    }

    public void StopQuick()
    {
        Quick = false; Countdown = 0;
        if (!InGame) Leave();
        Note = "";
    }

    static string HostKey(OnlineEntry e) { return e.Name + "|" + e.Addr + "|" + e.Lan + "|" + e.Port; }

    void MarkBad() { if (joinKey != "") lock (badHosts) badHosts[joinKey] = DateTime.UtcNow.AddMinutes(2); }

    bool IsBad(OnlineEntry e)
    {
        DateTime until;
        lock (badHosts) return badHosts.TryGetValue(HostKey(e), out until) && until > DateTime.UtcNow;
    }

    void QuickLoop()
    {
        while (running)
        {
            Thread.Sleep(1000);
            if (!Quick || InGame || GoPod > 0) { Countdown = 0; continue; }
            string st = Status;
            List<OnlineEntry> open = Everyone().FindAll(e => e.IsHost && e.Game == QuickGame && e.Name != Name
                                                             && (e.Max == 0 || e.N < e.Max) && !IsBad(e));
            if (st == "LOBBY")
            {
                if (open.Count > 0) { open.Sort((a, b) => b.N.CompareTo(a.N)); Join(open[0]); }   // fullest first
                else { if (Host(QuickGame, 2)) Note = "WAITING FOR ANOTHER PLAYER..."; }
            }
            else if (st == "HOST")
            {
                int count; lock (lk) count = members.Count + 1;
                if (count == 1)
                {
                    Countdown = 0;
                    // two people pressed QUICK MATCH at once and both host: the one whose name sorts later joins the other
                    OnlineEntry other = open.Find(e => e.N <= 1 && string.CompareOrdinal(e.Name + e.Lan, Name + LanIp) < 0);
                    if (other != null) Join(other);
                }
                else if (count >= Max)
                {
                    if (Countdown == 0) Countdown = 4;
                    Countdown--;
                    Note = Countdown > 0 ? "FOUND A PLAYER! STARTING IN " + Countdown : "STARTING!";
                    if (Countdown <= 0) { Countdown = 0; StartMatch(); }
                }
                else Countdown = 0;
            }
            else if (st == "JOINED" && welcomed) Note = "FOUND A GAME! STARTING SOON...";
        }
    }

    void Shout(UdpClient u)
    {
        int count; lock (lk) count = members.Count + 1;
        byte[] b = Encoding.ASCII.GetBytes("MALLVR\t" + Id + "\t" + Name + "\t" + Game + "\t" + count + "\t" + Max + "\t" + HostPort);
        try { u.EnableBroadcast = true; u.Send(b, b.Length, new IPEndPoint(IPAddress.Broadcast, LanPort)); } catch (Exception) { }
    }

    void LanLoop()
    {
        IPEndPoint from = new IPEndPoint(IPAddress.Any, 0);
        while (running && lanRx != null)
        {
            byte[] b;
            try { b = lanRx.Receive(ref from); } catch (SocketException) { continue; } catch (Exception) { break; }
            string[] f = Encoding.ASCII.GetString(b).Split('\t');
            if (f.Length < 7 || f[0] != "MALLVR" || f[1] == Id) continue;
            OnlineEntry e = new OnlineEntry { Name = Clean(f[2], 12), Status = "HOST", Game = Clean(f[3], 8), Lan = from.Address.ToString(), FromLan = true };
            int.TryParse(f[4], out e.N); int.TryParse(f[5], out e.Max); int.TryParse(f[6], out e.Port);
            lock (lk)
            {
                lan.RemoveAll(x => x.Lan == e.Lan && x.Port == e.Port);
                lan.Add(e);
            }
        }
    }

    void BeatLoop()
    {
        int n = 0;
        while (running)
        {
            if (n++ % 3 == 0) Beat();                  // every 3 s
            lock (lk) lan.RemoveAll(x => (DateTime.UtcNow - x.Seen).TotalSeconds > 6);
            Thread.Sleep(1000);
        }
    }

    public void Beat()
    {
        if (ListUrl == "") return;
        int count = 1;
        if (Status == "HOST") lock (lk) count = members.Count + 1;
        string q = "op=beat&id=" + Id + "&name=" + Uri.EscapeDataString(Name) + "&status=" + Status +
                   "&game=" + (Status == "LOBBY" ? "" : Game) + "&n=" + count + "&max=" + Max +
                   "&port=" + (Status == "HOST" ? HostPort : 0) + "&lan=" + LanIp + "&ver=" + Uri.EscapeDataString(Version) + "&key=" + Key;
        try
        {
            string body = Get(q);
            List<OnlineEntry> got = new List<OnlineEntry>();
            foreach (string line in body.Split('\n'))
            {
                string[] f = line.TrimEnd('\r').Split('\t');
                if (f.Length < 9) continue;
                OnlineEntry e = new OnlineEntry { Name = f[0], Status = f[1], Game = f[2], Addr = f[5], Lan = f[7], Same = f[8] == "1" };
                int.TryParse(f[3], out e.N); int.TryParse(f[4], out e.Max); int.TryParse(f[6], out e.Port);
                got.Add(e);
            }
            lock (lk) { web.Clear(); web.AddRange(got); }
            ListOk = true;
        }
        catch (Exception) { ListOk = false; }
    }

    string Get(string query)
    {
        HttpWebRequest r = (HttpWebRequest)WebRequest.Create(ListUrl + (ListUrl.Contains("?") ? "&" : "?") + query);
        r.Timeout = 5000; r.ReadWriteTimeout = 5000; r.UserAgent = "MallVR/" + Version;
        using (WebResponse w = r.GetResponse())
        using (StreamReader s = new StreamReader(w.GetResponseStream(), Encoding.ASCII))
            return s.ReadToEnd();
    }

    // ---- frames to the local pod, optionally late (MALLVR_LAG_MS) -------------------------------------
    readonly Queue<KeyValuePair<DateTime, byte[]>> late = new Queue<KeyValuePair<DateTime, byte[]>>();

    void Deliver(byte[] p, int off)
    {
        UdpClient u = loc;
        if (u == null || myPod <= 0) return;
        byte[] f = new byte[p.Length - off];
        Buffer.BlockCopy(p, off, f, 0, f.Length);
        if (lagMs > 0) { lock (late) late.Enqueue(new KeyValuePair<DateTime, byte[]>(DateTime.UtcNow.AddMilliseconds(lagMs), f)); return; }
        SendRaw(u, f, new IPEndPoint(IPAddress.Loopback, 27100 + myPod));
    }

    void DelayLoop()
    {
        while (running)
        {
            Thread.Sleep(lagMs > 0 ? 1 : 200);
            while (true)
            {
                byte[] f = null;
                lock (late) if (late.Count > 0 && late.Peek().Key <= DateTime.UtcNow) f = late.Dequeue().Value;
                if (f == null) break;
                UdpClient u = loc;
                if (u != null) SendRaw(u, f, new IPEndPoint(IPAddress.Loopback, 27100 + myPod));
            }
        }
    }

    // ---- helpers -------------------------------------------------------------------------------------
    static void Send(UdpClient u, string s, IPEndPoint to) { SendRaw(u, Encoding.ASCII.GetBytes(s), to); }
    static void SendRaw(UdpClient u, byte[] b, IPEndPoint to)
    {
        if (u == null || to == null) return;
        try { u.Send(b, b.Length, to); } catch (Exception) { }
    }

    public static string Clean(string s, int n)
    {
        StringBuilder b = new StringBuilder();
        foreach (char c in (s ?? "").ToUpper()) if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_' || c == '.') b.Append(c);
        string r = b.ToString().Trim();
        return r.Length > n ? r.Substring(0, n) : r;
    }

    static string FindLanIp()
    {
        try
        {
            using (Socket s = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp))
            {
                s.Connect("8.8.8.8", 53);                 // no packet is sent; this only picks the outgoing interface
                return ((IPEndPoint)s.LocalEndPoint).Address.ToString();
            }
        }
        catch (Exception) { return ""; }
    }

    // UPnP port mapping through Windows' own NAT helper (HNetCfg.NATUPnP), UDP HostPort -> this PC
    bool Upnp(bool add, out string err)
    {
        err = "";
        try
        {
            Type t = Type.GetTypeFromProgID("HNetCfg.NATUPnP");
            if (t == null) { err = "NO UPNP ON THIS PC"; return false; }
            object nat = Activator.CreateInstance(t);
            object maps = nat.GetType().InvokeMember("StaticPortMappingCollection", BindingFlags.GetProperty, null, nat, null);
            if (maps == null) { err = "ROUTER HAS UPNP OFF"; return false; }
            if (add)
                maps.GetType().InvokeMember("Add", BindingFlags.InvokeMethod, null, maps,
                    new object[] { HostPort, "UDP", HostPort, LanIp, true, "MallVR online" });
            else
                maps.GetType().InvokeMember("Remove", BindingFlags.InvokeMethod, null, maps, new object[] { HostPort, "UDP" });
            return true;
        }
        catch (Exception e)
        {
            Exception x = e.InnerException ?? e;
            err = x.Message.Length > 30 ? "ROUTER SAID NO" : x.Message.ToUpper();
            return false;
        }
    }
}
