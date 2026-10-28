// MallVR online play (the launcher's ONLINE screen). No game server: the website only keeps a short list of who is
// online (online\list.php, polled every few seconds); the HOST's PC links the players itself.
//   - list:  GET <ListUrl>?op=beat&... every 3 s, reply = everyone online (hosts carry their address + port).
//   - LAN:   hosts also shout on UDP 27209 every 2 s, so PCs in the same house find each other with no internet.
//   - host:  listens on UDP 27200 (asks the router to open it: UPnP), takes JOINs, START hands out pod numbers
//            (host = pod 1 = master). During the game it passes each pod's network frames to the others.
//   - every PC: its MallVR pod sends frames to 127.0.0.1:(27110 + pod) (play.ps1 -Peers), we hand frames from the
//            others to the pod's own port 127.0.0.1:(27100 + pod). The emulator needs no change.
// Packets between PCs (first byte): J join (name, game, netproto, version, id, game EXE sha1, platform) / W welcome / F refused
//   (reason) / K keepalive / L roster (names) / T platforms, same order as L (PC / QUEST) / G go (pod) / X leave or cancel /
//   D data (from pod byte + one Ethernet frame) / P hole punch (host -> a joiner calling it, no content).
// The list never shows anyone's address (security review 10-26, finding 2): each host row carries a token instead; a joiner
//   beats with call=<token> and only its own reply gets an ADDR line for that host, and only that host's reply gets a CALLER line
//   for the joiner (hole punching). Same internet address = same house: ADDR also carries the host's LAN address.
// GROUPS (10-25, online/VR_ARCADE_AND_GROUP_SWITCH_DESIGN.md section 4): a group stays together after a game, and the host can
// switch everyone to another game, even mid-game. Old versions ignore these packets (they fall through HostPacket / JoinerPacket):
//   H games this member has (member -> host, after W)        N seq GAME host-name: "next game?" (host -> members, until answered)
//   A seq 1|0 GO / STAY OUT (member -> host; none in 10 s = GO) S seq GAME id=pod ...: switch now (host -> members; ids from J)
//   E back between games, still a group (host -> members)    C GAME: the host picked another game between games
//   Q ms / R ms: ping / its echo (host <-> member: lag for the play notes)   Y seq: member started the game of START / switch seq
// NETPROTO (Quest plan section 7): bumped when packets / frames change; a host refuses a joiner with another one (old joiners send none).
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
    public int PubPort;                               // the port the internet sees (STUN), 0 = unknown
    public string To = "";                            // a joiner: the host name it is calling (hole punching)
    public string Ver = "", Plat = "PC", Tok = "";    // MallVR version, PC / QUEST, the host's token for asking its address
    public bool Same, FromLan;
    public DateTime Seen = DateTime.UtcNow;
    public bool IsHost { get { return Status == "HOST" && (Port > 0 || Tok != ""); } }
    public IPEndPoint Target()
    {
        string a = (Same || FromLan) && Lan != "" ? Lan : Addr;
        IPAddress ip;
        int port = (Same || FromLan) && Lan != "" ? Port : (PubPort > 0 ? PubPort : Port);
        return IPAddress.TryParse(a, out ip) ? new IPEndPoint(ip, port) : null;
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
    public const int NetProto = 2;
    public string Platform = "PC";
    public volatile string Status = "LOBBY";          // LOBBY / HOST / JOINED / PLAYING
    public volatile string Note = "";                 // one line for the screen
    public volatile string RouterNote = "";
    public volatile bool ListOk;
    public string Game = "";
    public int Max = 2;
    public readonly Dictionary<string, int> MaxFor = new Dictionary<string, int>();   // game code -> players (launcher sets it from games.ps1 MaxPlayers)
    int MaxOf(string game) { int m; return MaxFor.TryGetValue(game, out m) ? m : 2; }
    // game code -> SHA1 of the game EXE this player runs (PC: games.ps1 Sha1, check.ps1 only lets that one start); a host refuses a
    // joiner whose EXE differs, because linked pods with different EXEs desync
    public readonly Dictionary<string, string> Sha1For = new Dictionary<string, string>();
    string Sha1Of(string game) { string h; lock (Sha1For) return Sha1For.TryGetValue(game, out h) ? h : ""; }
    public volatile int GoPod;                        // > 0: start the game as this pod (set by START / a G packet)
    public volatile bool InGame;
    public string LanIp = "";
    // QUICK MATCH: join an open game of QuickGame, or host one; a full game starts by itself after a short count
    public volatile bool Quick;
    public string QuickGame = "";                    // "" = ANY GAME: join any open game in QuickGames, or host one of them
    public List<string> QuickGames = new List<string>();   // the online games this player has (the launcher fills it)
    bool QuickWants(string game) { return QuickGame != "" ? game == QuickGame : QuickGames.Contains(game); }
    static readonly Random quickRnd = new Random();
    public volatile int Countdown;                    // > 0 while a full quick game counts down to START
    readonly Dictionary<string, DateTime> badHosts = new Dictionary<string, DateTime>();
    string joinKey = "";

    readonly object lk = new object();
    readonly List<OnlineEntry> web = new List<OnlineEntry>(), lan = new List<OnlineEntry>();
    readonly List<string> roster = new List<string>(), rosterPlat = new List<string>();

    // Have = online games this player has (the launcher fills it); Have = null on a member = an old version (never sent H)
    class Member { public IPEndPoint Ep; public string Name, Id = "", JoinGame = "", Plat = "PC"; public int Pod; public DateTime Last; public List<string> Have; public int Answer = -1; public double Rtt, Arrive = -1; }
    public readonly List<string> Have = new List<string>();

    // ---- group switching state (read by MallVrPanel.WaitGame / launcher.ps1) ----
    public volatile string SwitchGame = "";           // set = close the game now and start this one as SwitchPod (host: 1)
    public volatile int SwitchPod;
    public volatile int PromptSeq;                    // member: > 0 = the host asks GO / STAY OUT for PromptGame
    public string PromptGame = "", PromptHost = "";
    public DateTime PromptUntil;
    public volatile bool Proposing;                   // host: waiting for answers
    public DateTime ProposeUntil;
    int seq, seenSeq, answered;
    string proposeGame = "";
    DateTime proposeAt;
    public string SwitchNote = "";
    // the match: host id + START / switch number, the same on every pod of one match (play notes now, scores and tournaments next,
    // online/TOURNAMENT_TRACKING_PLAN.md step 1)
    public volatile string MatchId = "";                    // the last switch, for the play notes: "go=2 stay=0 none=1 old=0 secs=11"
    public string JoinVia = "";                       // how this PC reached the host (lan / internet), for the play notes
    public double JoinSecs;
    DateTime joinAt, startAt, hostedAt;
    public bool RouterOpen { get { return upnpMapped; } }

    // play notes, host: seconds from START / switch until each member's game sent its first frame ("BOB=2.1 SAM=none")
    public string Arrivals()
    {
        StringBuilder b = new StringBuilder();
        lock (lk) foreach (Member m in members) b.Append(b.Length > 0 ? " " : "").Append(m.Name.Replace(' ', '_')).Append('=').Append(m.Arrive < 0 ? "none" : m.Arrive.ToString("0.0", System.Globalization.CultureInfo.InvariantCulture));
        return b.ToString();
    }

    public string LagNote()
    {
        StringBuilder b = new StringBuilder();
        lock (lk) foreach (Member m in members) b.Append(b.Length > 0 ? " " : "").Append(m.Name.Replace(' ', '_')).Append('=').Append((int)m.Rtt);
        return b.ToString();
    }

    readonly List<Member> members = new List<Member>();

    UdpClient ext, loc, lanRx;
    IPEndPoint hostEp;                                // joiner: the host we joined
    DateTime hostLast, joinSent;
    bool welcomed;
    int myPod, lagMs;
    volatile bool running;
    bool upnpMapped;
    volatile int pubPort;                             // our UDP socket's port as the internet sees it (STUN), for hole punching
    string joinName = "";                             // joiner: the host's name (told to the list so the host punches back)
    string callTok = "";                              // joiner: the host's list token (call=, so the list hands out its address)
    readonly Dictionary<string, OnlineEntry> addrs = new Dictionary<string, OnlineEntry>();   // token -> address (ADDR lines)
    readonly List<OnlineEntry> callers = new List<OnlineEntry>();                             // host: joiners calling us (CALLER lines)

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
        ScreenClosed("quit");
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
                if (w != null) { w.FromLan = true; w.Lan = l.Lan; if (w.Port == 0) w.Port = l.Port; }   // the list no longer gives ports
                else all.Add(l);
            }
            return all;
        }
    }

    public List<string> Roster() { lock (lk) return new List<string>(roster); }
    // the platform of each roster name, same order ("PC" when an old host sent no T)
    public List<string> RosterPlat()
    {
        lock (lk) { List<string> r = new List<string>(); for (int i = 0; i < roster.Count; i++) r.Add(i < rosterPlat.Count ? rosterPlat[i] : "PC"); return r; }
    }

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
        pubPort = 0; Stun(ext);
        Game = game; Max = Math.Max(2, Math.Min(4, max));
        lock (lk) { members.Clear(); roster.Clear(); roster.Add(Name); }
        Status = "HOST"; Note = "WAITING FOR PLAYERS TO JOIN"; hostedAt = DateTime.UtcNow;
        sessHosted = true; if (waitFrom == DateTime.MinValue) waitFrom = DateTime.UtcNow;
        Spawn(ExtLoop);
        if (Environment.GetEnvironmentVariable("MALLVR_NO_UPNP") == "1") RouterNote = "ROUTER PORT NOT TRIED (MALLVR_NO_UPNP)";
        else if (!upnpMapped)
        {
            RouterNote = "OPENING YOUR ROUTER PORT...";
            Spawn(delegate
            {
                string err;
                upnpMapped = Upnp(true, out err);
                RouterNote = upnpMapped ? "ROUTER PORT OPEN: ANYONE CAN JOIN" : "ROUTER SAID NO TO OPENING A PORT: PLAYERS WILL PUNCH THROUGH INSTEAD";
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
            string s = SwitchPacket(++seq, Game);
            startAt = DateTime.UtcNow; foreach (Member m in members) m.Arrive = -1;
            for (int r = 0; r < 3; r++)
                foreach (Member m in members) Send(ext, m.Have != null ? s : "G\t" + m.Pod, m.Ep);   // old versions: G
        }
        GoPod = 1;
        return true;
    }

    string SwitchPacket(int sq, string game)
    {
        StringBuilder b = new StringBuilder("S\t" + sq + "\t" + game);
        foreach (Member m in members) b.Append('\t').Append(m.Id != "" ? m.Id : m.Name).Append('=').Append(m.Pod);
        MatchId = Id + "-" + sq;
        b.Append("\tmatch=").Append(MatchId);      // older versions read it as one more id=pod pair that isn't theirs
        return b.ToString();
    }

    // ---- groups ----------------------------------------------------------------------------------------
    public bool IsHost { get { return hostEp == null && (Status == "HOST" || (Status == "PLAYING" && ext != null)); } }
    public bool InGroup
    {
        get
        {
            if (ext == null) return false;
            if (hostEp != null) return welcomed;
            lock (lk) return members.Count > 0;
        }
    }

    // games the whole group can play: everyone has it (H), and it takes that many players
    public List<string> CommonGames()
    {
        List<string> r = new List<string>();
        lock (lk)
        {
            int n = members.Count + 1;
            foreach (string g in Have)
            {
                if (MaxOf(g) < n) continue;
                bool all = true;
                foreach (Member m in members) if (m.Have == null || !m.Have.Contains(g)) { all = false; break; }
                if (all) r.Add(g);
            }
        }
        return r;
    }

    // between games: the host picks another game for the group (members see it; old versions can't follow and are let go)
    public void SetGame(string game)
    {
        if (Status != "HOST" || game == Game) return;
        Game = game; Max = Math.Max(Max, MaxOf(game));
        lock (lk)
        {
            foreach (Member m in members.FindAll(x => x.Have == null && x.JoinGame != game)) { Send(ext, "F\tTHE GROUP NEEDS A NEWER MALLVR", m.Ep); members.Remove(m); }
            foreach (Member m in members) Send(ext, "C\t" + game, m.Ep);
        }
        SendRoster();
        Beat();
    }

    // mid-game (host, Button Masher Menu > SWITCH GAME): ask everyone, switch after their answers or 10 s
    public bool Propose(string game)
    {
        if (!IsHost || Proposing) return false;
        lock (lk)
        {
            if (members.Count == 0) return false;
            proposeGame = game; seq++;
            foreach (Member m in members) m.Answer = -1;
        }
        proposeAt = DateTime.UtcNow; ProposeUntil = proposeAt.AddSeconds(10);
        Proposing = true;
        SendProposal();
        return true;
    }

    void SendProposal()
    {
        lock (lk) foreach (Member m in members) if (m.Answer < 0) Send(ext, "N\t" + seq + "\t" + proposeGame + "\t" + Name, m.Ep);
    }

    public void CancelPropose() { Proposing = false; }

    void Commit()
    {
        Proposing = false;
        int go = 0, stay = 0, none = 0, old = 0;
        string s;
        lock (lk)
        {
            List<Member> keep = new List<Member>();
            foreach (Member m in members)
            {
                if (m.Have == null) { old++; Send(ext, "F\tTHE GROUP NEEDS A NEWER MALLVR", m.Ep); continue; }
                if (m.Answer == 0) { stay++; continue; }
                if (m.Answer == 1) go++; else none++;
                keep.Add(m);
            }
            members.Clear(); members.AddRange(keep);
            for (int i = 0; i < members.Count; i++) members[i].Pod = i + 2;
            s = SwitchPacket(seq, proposeGame);
            startAt = DateTime.UtcNow; foreach (Member m in members) m.Arrive = -1;
            for (int r = 0; r < 3; r++) foreach (Member m in members) Send(ext, s, m.Ep);
        }
        SwitchNote = "go=" + go + " stay=" + stay + " none=" + none + " old=" + old + " secs=" + (int)(DateTime.UtcNow - proposeAt).TotalSeconds;
        Game = proposeGame; Max = Math.Max(MaxOf(Game), 2);
        SendRoster();
        if (members.Count == 0) { Note = "NOBODY CAME ALONG"; SwitchNote += " empty=1"; }
        SwitchPod = 1; SwitchGame = Game;
    }

    // member: the player's GO / STAY OUT
    public void Answer(int sq, bool go)
    {
        if (hostEp == null) return;
        answered = sq;
        if (sq == PromptSeq) PromptSeq = 0;
        for (int r = 0; r < 3; r++) Send(ext, "A\t" + sq + "\t" + (go ? 1 : 0), hostEp);
        if (!go) { Leave(); Note = "YOU STAYED OUT: BACK IN THE LOBBY"; }
    }

    // both sides: the switch happened (the launcher is about to start SwitchGame)
    public void SwitchTaken() { SwitchGame = ""; SwitchPod = 0; }

    public List<KeyValuePair<string, double>> Lags()
    {
        List<KeyValuePair<string, double>> r = new List<KeyValuePair<string, double>>();
        lock (lk) foreach (Member m in members) r.Add(new KeyValuePair<string, double>(m.Name, m.Rtt));
        return r;
    }

    // ---- join ---------------------------------------------------------------------------------------
    public void Join(OnlineEntry e)
    {
        Leave();
        if (e.Addr == "" && !e.FromLan && e.Tok != "")
        {   // the list keeps addresses private: ask for this one host's address (the reply's ADDR line)
            callTok = e.Tok; joinName = e.Name; Game = e.Game; Status = "JOINED";
            Beat();
            OnlineEntry a;
            lock (lk) addrs.TryGetValue(e.Tok, out a);
            if (a != null) { e.Addr = a.Addr; e.Port = a.Port; e.PubPort = a.PubPort; e.Same = a.Same; if (a.Lan != "") e.Lan = a.Lan; }
        }
        IPEndPoint t = e.Target();
        joinPlat = e.Plat;
        if (t == null) { callTok = ""; Status = "LOBBY"; Note = "THAT HOST HAS NO ADDRESS"; Fail("no_address"); return; }
        try
        {
            ext = new UdpClient(new IPEndPoint(IPAddress.Any, 0));
            ext.Client.IOControl(-1744830452, new byte[] { 0 }, null);
        }
        catch (Exception) { Note = "COULD NOT OPEN A NETWORK PORT"; Close(ref ext); Fail("no_port"); return; }
        pubPort = 0; Stun(ext);
        hostEp = t; welcomed = false; Game = e.Game; Max = e.Max; joinName = e.Name;
        JoinVia = (e.Same || e.FromLan) && e.Lan != "" ? "lan" : "internet"; joinAt = DateTime.UtcNow; JoinSecs = 0;
        joinKey = HostKey(e); callTok = e.Tok;
        sessJoined = true; if (waitFrom == DateTime.MinValue) waitFrom = DateTime.UtcNow;
        hostLast = DateTime.UtcNow; joinSent = DateTime.MinValue;
        lock (lk) { roster.Clear(); }
        Status = "JOINED"; Note = "CALLING " + e.Name + "...";
        Spawn(ExtLoop);
        SendJoin();
        Beat();
    }

    void SendJoin() { joinSent = DateTime.UtcNow; Send(ext, "J\t" + Name + "\t" + Game + "\t" + NetProto + "\t" + Version + "\t" + Id + "\t" + Sha1Of(Game) + "\t" + Platform, hostEp); }

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
        lock (lk) { members.Clear(); roster.Clear(); rosterPlat.Clear(); }
        hostEp = null; GoPod = 0; callTok = "";
        Proposing = false; PromptSeq = 0;
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
        sessGames++; if (waitSecs < 0 && waitFrom != DateTime.MinValue) waitSecs = (DateTime.UtcNow - waitFrom).TotalSeconds;
        if (hostEp != null && goSeq > 0) for (int r = 0; r < 3; r++) Send(ext, "Y\t" + goSeq, hostEp);
        if (loc != null) Spawn(LocLoop);
        Beat();
    }

    public void EndGame()
    {
        InGame = false;
        Close(ref loc);
    }

    // after the game the group stays together (10-25): the host is open again (HOST, it can START or pick another game), members
    // wait in it (GROUP). Lost the host / nobody left = the lobby.
    public void AfterGame()
    {
        Quick = false; Countdown = 0; Proposing = false; PromptSeq = 0;
        EndGame();
        if (ext == null) { Leave(); Status = "LOBBY"; }
        else if (hostEp == null) { Status = "HOST"; lock (lk) foreach (Member m in members) Send(ext, "E\t" + seq, m.Ep); SendRoster(); }
        else if (welcomed) Status = "GROUP";
        else Leave();
        Note = InGroup ? "STILL TOGETHER: " + (IsHost ? "START OR PICK ANOTHER GAME" : "WAITING FOR THE HOST") : "";
        Beat();
    }

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
            if (p.Length >= 20 && p[0] == 1 && p[1] == 1) { StunReply(p); continue; }   // STUN answer: our public port
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
                int proto;
                if (f.Length > 3 && int.TryParse(f[3], out proto) && proto != NetProto) { Send(u, "F\tNEEDS MALLVR " + (proto < NetProto ? Version : "UPDATE: THE HOST HAS AN OLDER ONE"), from); return; }
                string mine = Sha1Of(Game), theirs = f.Length > 6 ? HexId(f[6]) : "";
                if (mine != "" && theirs.Length == 40 && theirs != mine) { Send(u, "F\tDIFFERENT GAME FILES: " + Game + " IS ANOTHER VERSION", from); return; }
                lock (lk)
                {
                    if (members.Count + 1 >= Max) { Send(u, "F\tTHAT GAME IS FULL", from); return; }
                    m = new Member { Ep = from, Name = name, JoinGame = Game, Id = f.Length > 5 ? HexId(f[5]) : "", Plat = f.Length > 7 && Clean(f[7], 8) != "" ? Clean(f[7], 8) : "PC" };
                    members.Add(m);
                }
                Note = name + " JOINED";
            }
            m.Last = DateTime.UtcNow;
            Send(u, "W", from);
            SendRoster();
        }
        else if (k == 'K') { if (m != null) m.Last = DateTime.UtcNow; }
        else if (k == 'H' && m != null)
        {
            List<string> h = new List<string>();
            for (int i = 1; i < f.Length; i++) { string c = Clean(f[i], 10); if (c != "") h.Add(c); }
            m.Have = h; m.Last = DateTime.UtcNow;
        }
        else if (k == 'A' && m != null)
        {
            int sq, a;
            if (f.Length > 2 && int.TryParse(f[1], out sq) && int.TryParse(f[2], out a) && sq == seq && Proposing) { m.Answer = a == 0 ? 0 : 1; if (a == 0) Note = m.Name + " STAYED OUT"; }
        }
        else if (k == 'R' && m != null)
        {
            long t0;
            if (f.Length > 1 && long.TryParse(f[1], out t0)) { double rtt = Ms() - t0; if (rtt >= 0 && rtt < 5000) m.Rtt = m.Rtt == 0 ? rtt : m.Rtt * 0.8 + rtt * 0.2; }
        }
        else if (k == 'Y' && m != null)
        {
            int sq;
            if (f.Length > 1 && int.TryParse(f[1], out sq) && sq == seq && m.Arrive < 0) m.Arrive = (DateTime.UtcNow - startAt).TotalSeconds;
        }
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
        if (k == 'W')
        {
            if (!welcomed) { welcomed = true; JoinSecs = (DateTime.UtcNow - joinAt).TotalSeconds; Note = "IN THE GAME: WAITING FOR THE HOST TO START"; }
            SendHave();
        }
        else if (k == 'Q') { if (f.Length > 1) Send(ext, "R\t" + f[1], hostEp); }
        else if (k == 'N')
        {
            int sq;
            if (f.Length > 2 && int.TryParse(f[1], out sq) && sq > seenSeq)
            {
                seenSeq = sq;
                PromptGame = Clean(f[2], 10); PromptHost = f.Length > 3 ? Clean(f[3], 12) : "THE HOST";
                PromptUntil = DateTime.UtcNow.AddSeconds(10); PromptSeq = sq;
            }
            else if (f.Length > 1 && int.TryParse(f[1], out sq) && sq == answered) { }   // our answer got lost: the next A goes with the next N
        }
        else if (k == 'S')
        {
            int sq, pod = 0;
            if (f.Length < 3 || !int.TryParse(f[1], out sq)) return;
            for (int i = 3; i < f.Length; i++)
            {
                string[] kv = f[i].Split('=');
                if (kv.Length == 2 && (kv[0] == Id || kv[0] == Name)) int.TryParse(kv[1], out pod);
                else if (kv.Length == 2 && kv[0] == "match") MatchId = HexDash(kv[1]);
            }
            if (sq <= seenSwitch) return;
            seenSwitch = sq; seenSeq = Math.Max(seenSeq, sq); PromptSeq = 0;
            string game = Clean(f[2], 10);
            if (pod < 2 || pod > 4) { if (answered != sq) { Leave(); Note = "THE GROUP WENT TO " + game + " WITHOUT YOU"; } return; }
            Game = game; goSeq = sq;
            if (InGame) { SwitchPod = pod; SwitchGame = game; }
            else if (GoPod == 0) GoPod = pod;
        }
        else if (k == 'C') { if (f.Length > 1) { Game = Clean(f[1], 10); Note = "THE HOST PICKED " + Game; } }
        else if (k == 'E') { if (!InGame) Note = "STILL TOGETHER: WAITING FOR THE HOST"; }
        else if (k == 'F') { MarkBad(); string why = f.Length > 1 ? f[1] : "REFUSED"; Fail("refused: " + why); Leave(); Note = why; }
        else if (k == 'L') { lock (lk) { roster.Clear(); for (int i = 1; i < f.Length; i++) roster.Add(Clean(f[i], 12)); } }
        else if (k == 'T') { lock (lk) { rosterPlat.Clear(); for (int i = 1; i < f.Length; i++) rosterPlat.Add(Clean(f[i], 8)); } }
        else if (k == 'G') { int pod; if (f.Length > 1 && int.TryParse(f[1], out pod) && pod >= 2 && pod <= 4 && GoPod == 0 && !InGame) GoPod = pod; }
        else if (k == 'X') { bool playing = InGame; if (!playing) { Leave(); Note = "THE HOST CLOSED THE GAME"; } }
    }

    int seenSwitch, goSeq;

    void SendHave()
    {
        StringBuilder b = new StringBuilder("H");
        lock (lk) foreach (string g in Have) b.Append('\t').Append(g);
        Send(ext, b.ToString(), hostEp);
    }

    void SendRoster()
    {
        StringBuilder sb = new StringBuilder("L\t" + Name), tb = new StringBuilder("T\t" + Platform);
        lock (lk)
        {
            roster.Clear(); roster.Add(Name); rosterPlat.Clear(); rosterPlat.Add(Platform);
            foreach (Member m in members) { sb.Append('\t').Append(m.Name); roster.Add(m.Name); tb.Append('\t').Append(m.Plat); rosterPlat.Add(m.Plat); }
            foreach (Member m in members) { Send(ext, sb.ToString(), m.Ep); Send(ext, tb.ToString(), m.Ep); }
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
                if (changed) { Note = "A PLAYER DROPPED OUT"; Fail("member_dropped"); SendRoster(); }
                else if (n % 3 == 0 && Status == "HOST") SendRoster();
                if (n % 2 == 0) lock (lk) foreach (Member m in members) Send(u, "Q\t" + Ms(), m.Ep);
                if (Proposing)
                {
                    bool all; lock (lk) all = members.TrueForAll(m => m.Answer >= 0 || m.Have == null);
                    if (all || DateTime.UtcNow >= ProposeUntil) Commit(); else SendProposal();
                }
                if (Status == "HOST" && n % 2 == 0) Shout(u);
                if (Status == "HOST") Punch(u);
            }
            else                                      // joiner
            {
                Send(u, "K", hostEp);
                if (!welcomed && (DateTime.UtcNow - joinSent).TotalSeconds >= 1) SendJoin();
                double quiet = (DateTime.UtcNow - hostLast).TotalSeconds;
                if (!welcomed && quiet > 25) { MarkBad(); Fail("no_answer"); Leave(); Note = "NO ANSWER: HOST ROUTER MAY BE CLOSED"; }   // 25 s: the host's punch arrives after its next list check
                else if (welcomed && !InGame && quiet > 15) { Fail("lost_host"); Leave(); Note = "LOST THE HOST"; }
            }
        }
    }

    // ---- QUICK MATCH -----------------------------------------------------------------------------------
    public void StartQuick(string game)
    {
        quickPresses++; if (waitFrom == DateTime.MinValue) waitFrom = DateTime.UtcNow;
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
            List<OnlineEntry> open = Everyone().FindAll(e => e.IsHost && QuickWants(e.Game) && e.Name != Name
                                                             && (e.Max == 0 || e.N < e.Max) && !IsBad(e));
            if (st == "LOBBY")
            {
                if (open.Count > 0) { open.Sort((a, b) => b.N.CompareTo(a.N)); Join(open[0]); }   // fullest first
                else if (QuickGame != "" || QuickGames.Count > 0)
                {   // ANY GAME with nobody open: host one of ours at random (players looking for that game, or for any, find it)
                    string g = QuickGame != "" ? QuickGame : QuickGames[quickRnd.Next(QuickGames.Count)];
                    if (Host(g, MaxOf(g))) Note = "WAITING FOR ANOTHER PLAYER...";
                }
            }
            else if (st == "HOST")
            {
                int count; lock (lk) count = members.Count + 1;
                if (count == 1)
                {
                    Countdown = 0;
                    // two people pressed QUICK MATCH at once and both host: the one whose name sorts later joins the other
                    OnlineEntry other = open.Find(e => e.N <= 1 && string.CompareOrdinal(e.Name + e.Lan, Name + LanIp) < 0);
                    // still alone after 15 s: the other lone host may not have our game (crossplay: a Quest without one of the
                    // sets), so it never comes to us: go to its game instead
                    if (other == null && (DateTime.UtcNow - hostedAt).TotalSeconds > 15) other = open.Find(e => e.N <= 1);
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
                   "&port=" + (Status == "HOST" ? HostPort : 0) + "&lan=" + LanIp + "&ver=" + Uri.EscapeDataString(Version) + "&key=" + Key +
                   "&pp=" + pubPort + "&to=" + (Status == "JOINED" ? Uri.EscapeDataString(joinName) : "") + "&plat=" + Platform +
                   "&call=" + (Status == "JOINED" ? callTok : "");
        try
        {
            string body = Get(q);
            List<OnlineEntry> got = new List<OnlineEntry>(), calls = new List<OnlineEntry>();
            Dictionary<string, OnlineEntry> ad = new Dictionary<string, OnlineEntry>();
            foreach (string line in body.Split('\n'))
            {
                string[] f = line.TrimEnd('\r').Split('\t');
                if (f.Length >= 7 && f[0] == "ADDR")
                {   // ADDR tok ip port lan same pubport: the host this player is calling
                    OnlineEntry a = new OnlineEntry { Tok = f[1], Addr = f[2], Lan = f[4], Same = f[5] == "1" };
                    int.TryParse(f[3], out a.Port); int.TryParse(f[6], out a.PubPort);
                    ad[a.Tok] = a; continue;
                }
                if (f.Length >= 4 && f[0] == "CALLER")
                {   // CALLER name ip pubport: a joiner calling this host (punch back to it)
                    OnlineEntry c = new OnlineEntry { Name = f[1], Status = "JOINED", Addr = f[2], To = Name };
                    int.TryParse(f[3], out c.PubPort); calls.Add(c); continue;
                }
                if (f.Length < 9) continue;
                OnlineEntry e = new OnlineEntry { Name = f[0], Status = f[1], Game = f[2], Addr = f[5], Lan = f[7], Same = f[8] == "1" };
                int.TryParse(f[3], out e.N); int.TryParse(f[4], out e.Max); int.TryParse(f[6], out e.Port);
                if (f.Length >= 11) { int.TryParse(f[9], out e.PubPort); e.To = f[10]; }
                if (f.Length >= 13) { e.Ver = f[11]; e.Plat = f[12] != "" ? f[12] : "PC"; }
                if (f.Length >= 14) e.Tok = f[13];
                got.Add(e);
            }
            lock (lk) { web.Clear(); web.AddRange(got); callers.Clear(); callers.AddRange(calls); foreach (var kv in ad) addrs[kv.Key] = kv.Value; }
            int others = got.FindAll(x => x.Name != Name).Count, open = got.FindAll(x => x.IsHost && x.Name != Name).Count;
            if (others > seenMax) seenMax = others; if (open > openMax) openMax = open;
            ListOk = true;
        }
        catch (Exception e)
        {   // once per run of trouble: what stopped the list (TLS, DNS, timeout...) tells us how players' networks fail
            if (ListOk || !listFailNoted) { listFailNoted = true; MallNotes.Add("list_fail", "why", e.GetType().Name + ": " + Short(e.Message, 80), "plat", Platform, "ver", Version); }
            ListOk = false;
        }
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

    // ---- hole punching: no router settings needed ------------------------------------------------------
    // Most home routers drop packets nobody asked for, so a joiner's JOIN never reached a host whose router refused UPnP. Both sides
    // ask public STUN servers which port the internet sees for their socket and put it in their list beat (pp); a joiner also says
    // which host it calls (to). The host then sends a few packets to each joiner calling it: its router now lets that joiner's packets
    // in, and the joiner's own router already does (it sent first). Fails only when both routers are 'symmetric' (rare at home).
    static readonly string[] StunServers = { "stun.l.google.com:19302", "stun1.l.google.com:19302", "stun.cloudflare.com:3478" };
    void Stun(UdpClient u)
    {
        if (u == null) return;
        Spawn(delegate
        {
            byte[] req = new byte[20];
            req[1] = 1; req[4] = 0x21; req[5] = 0x12; req[6] = 0xA4; req[7] = 0x42;     // Binding request, magic cookie
            byte[] tid = Guid.NewGuid().ToByteArray(); Buffer.BlockCopy(tid, 0, req, 8, 12);
            for (int tries = 0; tries < 3 && pubPort == 0 && u == ext; tries++)
            {
                foreach (string sv in StunServers)
                {
                    try
                    {
                        string[] hp = sv.Split(':');
                        foreach (IPAddress a in Dns.GetHostAddresses(hp[0]))
                            if (a.AddressFamily == AddressFamily.InterNetwork) { SendRaw(u, req, new IPEndPoint(a, int.Parse(hp[1]))); break; }
                    }
                    catch (Exception) { }
                }
                Thread.Sleep(1500);
            }
        });
    }

    void StunReply(byte[] p)
    {
        for (int o = 20; o + 4 <= p.Length; )
        {
            int t = p[o] << 8 | p[o + 1], len = p[o + 2] << 8 | p[o + 3];
            if (o + 4 + len > p.Length) break;
            if ((t == 0x0020 || t == 0x0001) && len >= 8 && p[o + 5] == 1)
            {
                int port = p[o + 6] << 8 | p[o + 7];
                if (t == 0x0020) port ^= 0x2112;
                if (port > 0) { if (pubPort != port) { pubPort = port; Beat(); } }
                return;
            }
            o += 4 + ((len + 3) & ~3);
        }
    }

    void Punch(UdpClient u)
    {
        List<OnlineEntry> callers;
        lock (lk) { callers = web.FindAll(e => e.Status == "JOINED" && e.To == Name && e.PubPort > 0 && e.Addr != ""); callers.AddRange(this.callers); }
        foreach (OnlineEntry e in callers)
        {
            IPAddress ip;
            if (!IPAddress.TryParse(e.Addr, out ip)) continue;
            IPEndPoint ep = new IPEndPoint(ip, e.PubPort);
            bool joined; lock (lk) joined = members.Exists(m => m.Ep.Equals(ep));
            if (!joined) Send(u, "P", ep);                  // opens our router to that joiner; its next JOIN gets in
        }
    }

    // ---- play notes (only sent for players who said yes; MallNotes.Add checks) -----------------------------------
    // online_fail: why a join / group broke, with what each side was (crossplay) and how this PC reached the internet.
    // online_session: one ONLINE visit: how many others it saw, what the player tried, how long until a game started (or it gave
    // up). Says whether people leave because the lobby is empty, the main thing to know to grow online play.
    string joinPlat = "";
    bool listFailNoted;
    DateTime sessOpen = DateTime.MinValue, waitFrom = DateTime.MinValue;
    int seenMax, openMax, quickPresses, sessGames;
    bool sessHosted, sessJoined;
    double waitSecs = -1;

    void Fail(string why)
    {
        MallNotes.Add("online_fail", "why", Short(why, 60), "role", hostEp == null ? "host" : "member", "game", Game, "via", JoinVia,
            "host_plat", hostEp == null ? Platform : joinPlat, "stun", pubPort > 0 ? "1" : "0", "router", upnpMapped ? "open" : "closed",
            "list", ListOk ? "ok" : "down", "in_game", InGame ? "1" : "0", "plat", Platform, "ver", Version);
    }

    public void ScreenOpened()
    {
        sessOpen = DateTime.UtcNow; waitFrom = DateTime.MinValue; waitSecs = -1;
        seenMax = openMax = quickPresses = sessGames = 0; sessHosted = sessJoined = false;
    }

    public void ScreenClosed(string how)
    {
        if (sessOpen == DateTime.MinValue) return;
        double secs = (DateTime.UtcNow - sessOpen).TotalSeconds;
        double waited = waitSecs >= 0 ? waitSecs : waitFrom != DateTime.MinValue ? (DateTime.UtcNow - waitFrom).TotalSeconds : -1;
        MallNotes.Add("online_session", "secs", ((int)secs).ToString(), "seen_max", seenMax.ToString(), "open_max", openMax.ToString(),
            "quick", quickPresses.ToString(), "hosted", sessHosted ? "1" : "0", "joined", sessJoined ? "1" : "0", "games", sessGames.ToString(),
            "wait_secs", waited < 0 ? "" : ((int)waited).ToString(), "matched", waitSecs >= 0 ? "1" : "0", "ended", how, "plat", Platform, "ver", Version);
        sessOpen = DateTime.MinValue;
    }

    static string Short(string s, int n) { s = (s ?? "").Replace('\t', ' ').Replace('\r', ' ').Replace('\n', ' '); return s.Length > n ? s.Substring(0, n) : s; }

    // ---- helpers -------------------------------------------------------------------------------------
    static void Send(UdpClient u, string s, IPEndPoint to) { SendRaw(u, Encoding.ASCII.GetBytes(s), to); }
    static void SendRaw(UdpClient u, byte[] b, IPEndPoint to)
    {
        if (u == null || to == null) return;
        try { u.Send(b, b.Length, to); } catch (Exception) { }
    }

    static readonly System.Diagnostics.Stopwatch clock = System.Diagnostics.Stopwatch.StartNew();
    static long Ms() { return clock.ElapsedMilliseconds; }

    static string HexDash(string s)
    {
        StringBuilder b = new StringBuilder();
        foreach (char c in (s ?? "").ToLower()) if (((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '-') && b.Length < 40) b.Append(c);
        return b.ToString();
    }

    static string HexId(string s)
    {
        StringBuilder b = new StringBuilder();
        foreach (char c in (s ?? "").ToLower()) if (((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) && b.Length < 40) b.Append(c);   // ids (24) and game SHA1s (40)
        return b.ToString();
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
