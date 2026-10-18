// MallVR front end: pick a game, headset or window, solo or linked pods, then START. Styled after the
// SU2000 pods' own operator screen (DAC BITMAPS\OPSCREEN.VST): black, bold serif capitals, yellow headings,
// red options. Drawn at the pod's 352x288 screen size without anti-aliasing and scaled up in whole pixels.
// launcher.ps1 compiles this once into ..\logs, shows it, runs play.ps1 with the choice, then shows it again.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.Windows.Forms;

public class MallLauncher : Form
{
    const int W = 352, H = 288;
    static readonly Color Yellow = Color.FromArgb(255, 255, 0), Red = Color.FromArgb(240, 24, 24),
        Dim = Color.FromArgb(110, 24, 24), White = Color.FromArgb(255, 255, 255), Grey = Color.FromArgb(120, 120, 120);

    // the games: code, name on screen, linkable, found in the games folder
    public readonly List<string> Codes = new List<string>(), Names = new List<string>();
    public readonly List<bool> Links = new List<bool>(), Found = new List<bool>();
    // the choice (also the starting state); Action = "start", "check", "update" or "quit"
    public int Game, Mode, Players, Pod = 1;      // Mode 0 headset / 1 window; Players 0 solo / 1 link this PC / 2 link network
    public string Peer = "", Action = "quit";
    // online (MallOnline.cs): Net is made once by launcher.ps1; OnlinePod > 0 = start the game as that pod
    public MallOnline Net;
    public bool OnlineScreen;
    public int OnlinePod;
    public string PlayerName = "";
    bool editName;
    int hostPick = -1;
    public bool Advanced;                         // ONLINE: false = QUICK MATCH screen, true = MORE OPTIONS (list / host / join)
    string status = "";

    readonly Bitmap canvas = new Bitmap(W, H, PixelFormat.Format32bppRgb);
    readonly Font big = new Font("Times New Roman", 15f, FontStyle.Bold, GraphicsUnit.Pixel),
        txt = new Font("Times New Roman", 13f, FontStyle.Bold, GraphicsUnit.Pixel),
        small = new Font("Times New Roman", 11f, FontStyle.Bold, GraphicsUnit.Pixel),
        mono = new Font("Courier New", 13f, FontStyle.Bold, GraphicsUnit.Pixel);   // the typed address: its dots survive at this size
    readonly List<KeyValuePair<Rectangle, string>> hits = new List<KeyValuePair<Rectangle, string>>();
    int scale = 3, blink;
    string hover = "";
    bool editPeer;
    readonly Timer timer = new Timer();

    public MallLauncher()
    {
        Text = "MallVR";
        BackColor = Color.Black;
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        DoubleBuffered = true;
        KeyPreview = true;
        Rectangle s = Screen.PrimaryScreen.WorkingArea;
        scale = Math.Max(1, Math.Min((s.Width - 40) / W, (s.Height - 60) / H));
        ClientSize = new Size(W * scale, H * scale);
        timer.Interval = 400;
        timer.Tick += (o, e) =>
        {
            blink++;
            if (OnlineScreen && Net != null && Net.GoPod > 0 && Action != "online") { OnlinePod = Net.GoPod; Action = "online"; Close(); return; }
            Invalidate();
        };
        timer.Start();
    }

    public void AddGame(string code, string name, bool link, bool found)
    {
        Codes.Add(code); Names.Add(name.ToUpper()); Links.Add(link); Found.Add(found);
    }

    // draws one option; returns its width. Clickable when id is set.
    int Opt(Graphics g, string s, int x, int y, Font f, Color c, string id)
    {
        if (id != null && id == hover && c == Red) c = White;
        using (Brush b = new SolidBrush(c)) g.DrawString(s, f, b, x, y, StringFormat.GenericTypographic);   // 1-bit glyphs (no smoothing), like the card's
        SizeF zf = g.MeasureString(s, f, 999, StringFormat.GenericTypographic);
        Size z = new Size((int)Math.Ceiling(zf.Width), (int)Math.Ceiling(zf.Height));
        if (id != null) hits.Add(new KeyValuePair<Rectangle, string>(new Rectangle(x - 2, y - 1, z.Width + 4, z.Height + 2), id));
        return z.Width;
    }

    void DrawScreen(Graphics g)
    {
        hits.Clear();
        g.Clear(Color.Black);
        g.TextRenderingHint = TextRenderingHint.SingleBitPerPixelGridFit;
        if (Game >= Codes.Count) Game = 0;
        if (OnlineScreen && Net != null) { DrawOnline(g); return; }
        bool link = Codes.Count > 0 && Links[Game];
        if (!link && Players != 0) Players = 0;

        Opt(g, "MALLVR   GAME   SELECT", 10, 4, big, Yellow, null);
        Opt(g, "GAME", 10, 26, big, Yellow, null);
        for (int i = 0; i < Codes.Count; i++)
        {
            int col = i / 6, row = i % 6;
            Color c = !Found[i] ? Dim : i == Game ? Yellow : Red;
            Opt(g, Names[i], 14 + col * 172, 44 + row * 16, txt, c, "g" + i);
        }
        int y = 144;
        Opt(g, "MODE", 10, y, big, Yellow, null);
        int x = 96;
        x += Opt(g, "HEADSET", x, y + 1, txt, Mode == 0 ? Yellow : Red, "m0") + 18;
        Opt(g, "WINDOW", x, y + 1, txt, Mode == 1 ? Yellow : Red, "m1");

        y = 164;
        Opt(g, "PLAYERS", 10, y, big, Yellow, null);
        x = 96;
        x += Opt(g, "SOLO", x, y + 1, txt, Players == 0 ? Yellow : Red, "p0") + 14;
        x += Opt(g, "LINK THIS PC", x, y + 1, txt, !link ? Dim : Players == 1 ? Yellow : Red, link ? "p1" : null) + 14;
        x += Opt(g, "LINK NETWORK", x, y + 1, txt, !link ? Dim : Players == 2 ? Yellow : Red, link ? "p2" : null) + 14;
        Opt(g, "ONLINE", 96, y + 17, txt, Red, "p3");

        if (Players == 2)
        {
            y = 200;
            Opt(g, "POD", 10, y, big, Yellow, null);
            x = 54;
            for (int p = 1; p <= 4; p++) x += Opt(g, p.ToString(), x, y + 1, txt, Pod == p ? Yellow : Red, "d" + p) + 10;
            x += 8;
            x += Opt(g, "OTHER PC", x, y, big, Yellow, null) + 8;
            string shown = (Peer.Length > 0 ? Peer : editPeer ? "" : "TYPE ADDRESS") + (editPeer && blink % 2 == 0 ? "_" : "");
            Opt(g, shown, x, y + 2, Peer.Length > 0 || editPeer ? mono : txt, editPeer ? Yellow : Red, "peer");
        }
        else if (Players == 1)
            Opt(g, "HEADSET = PLAYER 1   A WINDOW = PLAYER 2", 14, 202, small, Red, null);

        y = 226;
        x = 14;
        x += Opt(g, "START", x, y, big, blink % 2 == 0 ? Yellow : Red, "start") + 26;
        x += Opt(g, "CHECK GAMES", x, y + 1, txt, Red, "check") + 20;
        x += Opt(g, "UPDATE", x, y + 1, txt, Red, "update") + 20;
        Opt(g, "QUIT", x, y + 1, txt, Red, "quit");

        string st = status;
        if (st == "")
            st = Codes.Count == 0 ? "NO GAMES IN THE GAMES FOLDER" : !Found[Game] ? "NOT FOUND: PUT YOUR HDD DUMP IN THE GAMES FOLDER"
               : Mode == 0 ? "TURN THE HEADSET ON, THEN START" : "KEYS: SEE DOCS\\CONTROLS.TXT";
        Opt(g, st, 14, 254, small, Red, null);
        Opt(g, "GAINESPHERE STUDIOS", 214, 272, small, Dim, null);
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        using (Graphics g = Graphics.FromImage(canvas)) DrawScreen(g);
        e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
        e.Graphics.PixelOffsetMode = PixelOffsetMode.Half;
        e.Graphics.DrawImage(canvas, new Rectangle(0, 0, W * scale, H * scale), 0, 0, W, H, GraphicsUnit.Pixel);
    }

    string HitAt(Point p)
    {
        Point q = new Point(p.X / scale, p.Y / scale);
        foreach (var h in hits) if (h.Key.Contains(q)) return h.Value;
        return "";
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
        string h = HitAt(e.Location);
        if (h != hover) { hover = h; Cursor = h != "" ? Cursors.Hand : Cursors.Default; Invalidate(); }
    }

    protected override void OnMouseDown(MouseEventArgs e) { Do(HitAt(e.Location)); }

    void Do(string id)
    {
        if (id == "") return;
        status = "";
        editPeer = id == "peer";
        bool wasName = editName;
        editName = id == "oname";
        if (wasName && !editName) NameDone();      // clicking anywhere else finishes the typed name
        char k = id[0];
        int n = id.Length > 1 && char.IsDigit(id[1]) ? int.Parse(id.Substring(1)) : -1;
        if (k == 'g' && n >= 0) Game = n;
        else if (k == 'm' && n >= 0) Mode = n;
        else if (id == "p3") { OpenOnline(); }
        else if (id[0] == 'o') { OnlineDo(id); }
        else if (k == 'p' && n >= 0) Players = n;
        else if (k == 'd' && n >= 0) Pod = n;
        else if (id == "start")
        {
            if (Codes.Count == 0 || !Found[Game]) { status = "THAT GAME IS NOT IN THE GAMES FOLDER"; }
            else if (Players == 2 && Peer.Trim() == "") { status = "TYPE THE OTHER PC'S ADDRESS FIRST"; editPeer = true; }
            else { Action = "start"; Close(); return; }
        }
        else if (id == "check" || id == "update" || id == "quit") { Action = id; Close(); return; }
        Invalidate();
    }

    protected override void OnKeyDown(KeyEventArgs e)
    {
        if (editName)
        {
            if (e.KeyCode == Keys.Back && PlayerName.Length > 0) PlayerName = PlayerName.Substring(0, PlayerName.Length - 1);
            else if (e.KeyCode == Keys.Enter || e.KeyCode == Keys.Escape) { editName = false; NameDone(); }
            Invalidate(); e.Handled = true; return;
        }
        if (OnlineScreen)
        {
            if (e.KeyCode == Keys.Escape) OnlineDo("oback");
            e.Handled = true; return;
        }
        if (editPeer)
        {
            if (e.KeyCode == Keys.Back && Peer.Length > 0) Peer = Peer.Substring(0, Peer.Length - 1);
            else if (e.KeyCode == Keys.Enter || e.KeyCode == Keys.Escape) editPeer = false;
            Invalidate(); e.Handled = true; return;
        }
        int cols = 6;
        switch (e.KeyCode)
        {
            case Keys.Up: if (Game > 0) Game--; break;
            case Keys.Down: if (Game < Codes.Count - 1) Game++; break;
            case Keys.Left: if (Game >= cols) Game -= cols; break;
            case Keys.Right: if (Game + cols < Codes.Count) Game += cols; break;
            case Keys.M: Mode = 1 - Mode; break;
            case Keys.Enter: Do("start"); return;
            case Keys.Escape: Do("quit"); return;
            default: return;
        }
        status = ""; Invalidate(); e.Handled = true;
    }

    protected override void OnKeyPress(KeyPressEventArgs e)
    {
        if (editName)
        {
            char ch = char.ToUpper(e.KeyChar);
            if ((char.IsLetterOrDigit(ch) || ch == ' ' || ch == '-' || ch == '.') && ch < 128 && PlayerName.Length < 12) { PlayerName += ch; Invalidate(); }
            e.Handled = true; return;
        }
        if (!editPeer) return;
        char c = e.KeyChar;
        if ((char.IsLetterOrDigit(c) || c == '.' || c == ':' || c == '-') && Peer.Length < 40) { Peer += c; Invalidate(); }
        e.Handled = true;
    }

    // ---- ONLINE screen: name, open games (JOIN), who is online, HOST / START / LEAVE ------------------
    void OpenOnline()
    {
        OnlineScreen = true;
        status = "";
        if (PlayerName == "") PlayerName = "PLAYER" + new Random().Next(100, 1000);   // nobody has to type; NAME changes it
        NameDone();
    }

    void NameDone()
    {
        PlayerName = MallOnline.Clean(PlayerName, 12);
        if (PlayerName == "") { editName = true; return; }
        Net.Name = PlayerName;
        Net.Start();
        System.Threading.ThreadPool.QueueUserWorkItem(delegate { Net.Beat(); });
    }

    string GameName(string code)
    {
        int i = Codes.IndexOf(code);
        return i >= 0 ? Names[i] : code;
    }

    int HostGame()
    {
        if (hostPick >= 0 && hostPick < Codes.Count && Links[hostPick] && Found[hostPick]) return hostPick;
        if (Game < Codes.Count && Links[Game] && Found[Game]) return hostPick = Game;
        for (int i = 0; i < Codes.Count; i++) if (Links[i] && Found[i]) return hostPick = i;
        return -1;
    }

    List<OnlineEntry> OpenHosts()
    {
        return Net.Everyone().FindAll(e => e.IsHost && e.Name != PlayerName);
    }

    void DrawQuick(Graphics g)
    {
        Opt(g, "MALLVR   ONLINE", 10, 4, big, Yellow, null);
        int x = 10, y = 24;
        x += Opt(g, "NAME", x, y, big, Yellow, null) + 10;
        string nm = PlayerName + (editName && blink % 2 == 0 ? "_" : "");
        Opt(g, nm, x, y + 1, txt, editName ? Yellow : Red, Net.Quick ? null : "oname");
        x = 196;
        x += Opt(g, "HEADSET", x, y + 1, txt, Mode == 0 ? Yellow : Red, "m0") + 12;
        Opt(g, "WINDOW", x, y + 1, txt, Mode == 1 ? Yellow : Red, "m1");

        int hg = HostGame();
        y = 60;
        Opt(g, "GAME", 10, y, big, Yellow, null);
        if (hg >= 0) Opt(g, Net.Quick ? GameName(Net.QuickGame) : Names[hg], 70, y + 1, txt, Net.Quick ? Yellow : Red, Net.Quick ? null : "ogame");
        else Opt(g, "NO ONLINE GAME FOUND IN THE GAMES FOLDER", 70, y + 1, txt, Dim, null);

        y = 96;
        if (!Net.Quick)
            Opt(g, "QUICK  MATCH", 14, y, big, hg >= 0 ? (blink % 2 == 0 ? Yellow : Red) : Dim, hg >= 0 ? "oquick" : null);
        else
        {
            Opt(g, Net.Countdown > 0 ? "STARTING IN " + Net.Countdown : "SEARCHING" + new string('.', blink % 4), 14, y, big, Yellow, null);
            Opt(g, "CANCEL", 230, y + 1, txt, Red, "oqcancel");
        }
        if (!Net.Quick) Opt(g, "FINDS A PLAYER AND STARTS BY ITSELF", 14, y + 20, small, Dim, null);

        List<OnlineEntry> all = Net.Everyone();
        int people = all.FindAll(e => !e.FromLan).Count, games = all.FindAll(e => e.IsHost && e.Name != PlayerName).Count;
        y = 150;
        Opt(g, Net.ListOk ? people + " ONLINE   " + games + " OPEN GAME" + (games == 1 ? "" : "S") : "CAN'T REACH THE GAME LIST: HOME NETWORK ONLY", 14, y, txt, Red, null);
        string note = status != "" ? status : Net.Note;
        if (note != "") Opt(g, note, 14, y + 20, txt, Yellow, null);

        Opt(g, "MORE OPTIONS", 14, 226, txt, Net.Quick ? Dim : Red, Net.Quick ? null : "oadv");
        Opt(g, "BACK", 14, 254, txt, Red, "oback");
        Opt(g, "GAINESPHERE STUDIOS", 214, 272, small, Dim, null);
    }

    void DrawOnline(Graphics g)
    {
        if (!Advanced) { DrawQuick(g); return; }
        string st = Net.Status;
        Opt(g, "MALLVR   ONLINE", 10, 4, big, Yellow, null);
        int x = 10, y = 24;
        x += Opt(g, "NAME", x, y, big, Yellow, null) + 10;
        string nm = (PlayerName.Length > 0 ? PlayerName : editName ? "" : "TYPE YOUR NAME") + (editName && blink % 2 == 0 ? "_" : "");
        Opt(g, nm, x, y + 1, txt, editName ? Yellow : Red, st == "LOBBY" ? "oname" : null);
        x = 196;
        x += Opt(g, "HEADSET", x, y + 1, txt, Mode == 0 ? Yellow : Red, "m0") + 12;
        Opt(g, "WINDOW", x, y + 1, txt, Mode == 1 ? Yellow : Red, "m1");

        List<OnlineEntry> all = Net.Everyone();
        List<OnlineEntry> hosts = all.FindAll(e => e.IsHost && e.Name != PlayerName);
        y = 46;
        Opt(g, "OPEN GAMES", 10, y, big, Yellow, null);
        y += 18;
        if (hosts.Count == 0) Opt(g, "NONE RIGHT NOW: HOST ONE BELOW", 14, y, txt, Dim, null);
        for (int i = 0; i < hosts.Count && i < 5; i++)
        {
            OnlineEntry h = hosts[i];
            bool full = h.Max > 0 && h.N >= h.Max;
            int gi = Codes.IndexOf(h.Game);
            bool have = gi >= 0 && Found[gi];
            int ry = y + i * 15;
            Opt(g, GameName(h.Game), 14, ry, txt, Red, null);
            Opt(g, h.Name, 152, ry, txt, Red, null);
            Opt(g, h.N + "/" + h.Max, 248, ry, txt, Red, null);
            if (st == "LOBBY")
                Opt(g, full ? "FULL" : !have ? "MISSING" : "JOIN", 278, ry, txt, full || !have ? Dim : Red, full || !have ? null : "oj" + i);
        }

        y = 142;
        Opt(g, "ONLINE NOW", 10, y, big, Yellow, null);
        List<string> words = new List<string>();
        foreach (OnlineEntry e in all) if (!e.FromLan) words.Add(e.Name + (e.Status == "PLAYING" ? " (PLAYING)" : ""));
        if (words.Count == 0) words.Add(Net.ListOk ? "NOBODY YET" : "...");
        string line = "";
        int ly = y + 18;
        foreach (string w in words)
        {
            if (line.Length + w.Length > 50) { Opt(g, line, 14, ly, small, Red, null); ly += 12; line = ""; if (ly > y + 42) break; }
            line += (line == "" ? "" : "   ") + w;
        }
        if (line != "" && ly <= y + 42) Opt(g, line, 14, ly, small, Red, null);

        y = 202;
        if (st == "LOBBY")
        {
            int hg = HostGame();
            bool can = hg >= 0 && PlayerName != "";
            x = 14;
            x += Opt(g, "HOST", x, y, big, can ? (blink % 2 == 0 ? Yellow : Red) : Dim, can ? "ohost" : null) + 12;
            if (hg >= 0) Opt(g, Names[hg], x, y + 1, txt, Red, "ogame");
            else Opt(g, "NO LINKABLE GAME FOUND", x, y + 1, txt, Dim, null);
        }
        else
        {
            List<string> r = Net.Roster();
            Opt(g, (st == "HOST" ? "YOUR GAME: " : "JOINED: ") + GameName(Net.Game) + "   " + string.Join(", ", r.ToArray()), 14, y - 4, small, Yellow, null);
            x = 14;
            if (st == "HOST") x += Opt(g, "START", x, y + 10, big, r.Count >= 2 ? (blink % 2 == 0 ? Yellow : Red) : Dim, r.Count >= 2 ? "ostart" : null) + 20;
            Opt(g, st == "HOST" ? "CANCEL" : "LEAVE", x, y + 11, txt, Red, "oleave");
            if (st == "HOST" && Net.RouterNote != "") Opt(g, Net.RouterNote, 14, y + 30, small, Dim, null);
        }

        Opt(g, "BACK", 14, 254, txt, Red, "oback");
        if (st == "LOBBY") Opt(g, "QUICK MATCH", 250, 226, txt, Red, "osimple");
        string note = status != "" ? status : Net.Note != "" ? Net.Note
            : !Net.ListOk && PlayerName != "" ? "CAN'T REACH THE GAME LIST: HOME NETWORK ONLY" : "";
        Opt(g, note, 56, 256, small, Red, null);
        Opt(g, "GAINESPHERE STUDIOS", 214, 272, small, Dim, null);
    }

    void OnlineDo(string id)
    {
        if (id == "oname") { editName = true; }
        else if (id == "oback") { Net.StopQuick(); Net.Leave(); Net.Note = ""; OnlineScreen = false; }
        else if (id == "oquick") { int hg = HostGame(); if (hg >= 0) Net.StartQuick(Codes[hg]); }
        else if (id == "oqcancel") { Net.StopQuick(); }
        else if (id == "oadv") { Advanced = true; }
        else if (id == "osimple") { Advanced = false; }
        else if (id == "ogame")
        {
            int start = HostGame();
            for (int k = 1; k <= Codes.Count; k++) { int i = (start + k) % Codes.Count; if (Links[i] && Found[i]) { hostPick = i; break; } }
        }
        else if (id == "ohost") { int hg = HostGame(); if (hg >= 0) Net.Host(Codes[hg], 2); }
        else if (id == "ostart") { Net.StartMatch(); }
        else if (id == "oleave") { Net.Leave(); Net.Note = ""; }
        else if (id.StartsWith("oj"))
        {
            int n = int.Parse(id.Substring(2));
            List<OnlineEntry> hosts = OpenHosts();
            if (n < hosts.Count) Net.Join(hosts[n]);
        }
        Invalidate();
    }

    // dev check: draw the screen to a PNG at the window's scale (launcher.ps1 -Shot)
    public void SavePng(string path)
    {
        using (Graphics g = Graphics.FromImage(canvas)) DrawScreen(g);
        using (Bitmap big2 = new Bitmap(W * 3, H * 3))
        using (Graphics g = Graphics.FromImage(big2))
        {
            g.InterpolationMode = InterpolationMode.NearestNeighbor; g.PixelOffsetMode = PixelOffsetMode.Half;
            g.DrawImage(canvas, new Rectangle(0, 0, W * 3, H * 3), 0, 0, W, H, GraphicsUnit.Pixel);
            big2.Save(path, ImageFormat.Png);
        }
    }
}
