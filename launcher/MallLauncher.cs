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
using System.Text;
using System.Windows.Forms;

public class MallLauncher : Form
{
    const int W = 352, H = 288, Rows = 7;
    static readonly Color Yellow = Color.FromArgb(255, 255, 0), Red = Color.FromArgb(240, 24, 24),
        Dim = Color.FromArgb(110, 24, 24), White = Color.FromArgb(255, 255, 255), Grey = Color.FromArgb(120, 120, 120);

    // the games: code, name on screen, linkable, found in the games folder
    public readonly List<string> Codes = new List<string>(), Names = new List<string>();
    public readonly List<bool> Links = new List<bool>(), Found = new List<bool>();
    public readonly List<int> MaxPlayers = new List<int>();   // games.ps1 MaxPlayers (linked pods; default 2)
    // the choice (also the starting state); Action = "start", "check", "update" or "quit"
    public int Game, Mode, Players, Pod = 1;      // Mode 0 headset / 1 window; Players 0 solo / 1 link this PC / 2 link network
    public string Peer = "", Action = "quit";
    // online (MallOnline.cs): Net is made once by launcher.ps1; OnlinePod > 0 = start the game as that pod
    public MallOnline Net;
    public bool OnlineScreen;
    public bool GetGamesScreen;                       // GET GAMES (MallGetGames.cs): download the archive.org game files
    public int OnlinePod;
    public string PlayerName = "";
    public string NameFile = "";                  // %APPDATA%\MallVR\screen_name.txt: the screen name, kept for every MallVR copy / version
    bool editName;
    int hostPick = -1;
    public bool Advanced;                         // ONLINE: false = QUICK MATCH screen, true = MORE OPTIONS (list / host / join)
    string status = "";
    // play notes + REPORT A PROBLEM (MallNotes.cs). Root = the MallVR folder (its logs\ + settings\ go in a report).
    public string Root = "";
    public bool ReportScreen;
    public string ReportTag = "", ReportGame = "";  // the game a report is about: its log name (logs\<tag>.*) and name
    public string CrashGame = "";                  // set by launcher.ps1 when the last game stopped by itself: the offer screen
    string reportText = "";
    bool editReport;
    volatile string reportNote = "";
    volatile bool sending;

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
    // PLAYERS ONLINE on the game select screen: list.php?op=count every 30 s (people press ONLINE more when they see someone there)
    static volatile string onlineCount = "";
    static DateTime countAt = DateTime.MinValue;
    static volatile bool counting;
    void CountOnline()
    {
        if (Net == null || Net.ListUrl == "" || counting || (DateTime.UtcNow - countAt).TotalSeconds < 30) return;
        counting = true; countAt = DateTime.UtcNow;
        string url = Net.ListUrl + (Net.ListUrl.Contains("?") ? "&" : "?") + "op=count";
        System.Threading.ThreadPool.QueueUserWorkItem(delegate
        {
            try
            {
                System.Net.HttpWebRequest r = (System.Net.HttpWebRequest)System.Net.WebRequest.Create(url);
                r.Timeout = 5000; r.ReadWriteTimeout = 5000; r.UserAgent = "MallVR";
                string j;
                using (System.Net.WebResponse w = r.GetResponse()) using (System.IO.StreamReader s = new System.IO.StreamReader(w.GetResponseStream())) j = s.ReadToEnd();
                int n = JsonInt(j, "online"), q = JsonInt(j, "quest");
                onlineCount = n < 0 ? "" : n == 0 ? "NOBODY ONLINE YET: BE THE FIRST" : n + " PLAYER" + (n == 1 ? "" : "S") + " ONLINE" + (q > 0 ? " (" + q + " QUEST)" : "");
            }
            catch (Exception) { onlineCount = ""; }
            counting = false;
        });
    }
    static int JsonInt(string j, string k)
    {
        System.Text.RegularExpressions.Match m = System.Text.RegularExpressions.Regex.Match(j ?? "", "\"" + k + "\"\\s*:\\s*(\\d+)");
        return m.Success ? int.Parse(m.Groups[1].Value) : -1;
    }

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
            DevOnline();
            CountOnline();
            if (OnlineScreen && Net != null && Net.GoPod > 0 && Action != "online") { OnlinePod = Net.GoPod; Action = "online"; Close(); return; }
            Invalidate();
        };
        timer.Start();
    }

    // dev only (two launchers on one PC, no clicking): MALLVR_DEV_ONLINE=host:<GAME> hosts that game and STARTs when someone joined;
    // MALLVR_DEV_ONLINE=join joins the first open game; quick[:<GAME>] presses QUICK MATCH. Unset (players) = nothing.
    static readonly string devOnline = Environment.GetEnvironmentVariable("MALLVR_DEV_ONLINE") ?? "";
    static bool devStarted;
    void DevOnline()
    {
        if (devOnline == "" || Net == null || MallNotes.NeedsAsk || Action == "online") return;
        if (!OnlineScreen) { OpenOnline(); Advanced = true; return; }
        string st = Net.Status;
        if (devOnline.StartsWith("host:"))
        {
            if (st == "LOBBY" && !devStarted) { string g = devOnline.Substring(5); int i = Codes.IndexOf(g); if (i >= 0) Net.Host(g, MaxPlayers[i]); }
            else if (st == "HOST" && !devStarted && Net.Roster().Count >= 2 && Net.InGroup) { devStarted = true; Net.StartMatch(); }
        }
        else if (devOnline == "join" && st == "LOBBY" && !devStarted)
        {
            List<OnlineEntry> h = OpenHosts();
            if (h.Count > 0) { devStarted = true; Net.Join(h[0]); }
        }
        else if (devOnline.StartsWith("quick") && !devStarted)
        {   // MALLVR_DEV_ONLINE=quick (ANY GAME) or quick:<GAME>: press QUICK MATCH once (crossplay test: PC and Quest both searching)
            devStarted = true; Advanced = false;
            OnlineDo("oquick");
            if (devOnline.StartsWith("quick:")) Net.StartQuick(devOnline.Substring(6));
        }
    }

    public void AddGame(string code, string name, bool link, bool found) { AddGame(code, name, link, found, 2); }
    public void AddGame(string code, string name, bool link, bool found, int maxPlayers)
    {
        Codes.Add(code); Names.Add(name.ToUpper()); Links.Add(link); Found.Add(found);
        MaxPlayers.Add(Math.Max(2, Math.Min(4, maxPlayers)));
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
        if (MallNotes.NeedsAsk) { DrawConsent(g); return; }
        if (ReportScreen) { DrawReport(g); return; }
        if (CrashGame != "") { DrawCrash(g); return; }
        if (GetGamesScreen) { DrawGetGames(g); return; }
        if (OnlineScreen && Net != null) { DrawOnline(g); return; }
        bool link = Codes.Count > 0 && Links[Game];
        if (!link && Players != 0) Players = 0;

        Opt(g, "MALLVR   GAME   SELECT", 10, 4, big, Yellow, null);
        Opt(g, "GAME", 10, 26, big, Yellow, null);
        // two columns of seven (14 games; a third column fell off the screen's right edge)
        for (int i = 0; i < Codes.Count; i++)
        {
            int col = i / Rows, row = i % Rows;
            Color c = !Found[i] ? Dim : i == Game ? Yellow : Red;
            Opt(g, Names[i], 14 + col * 172, 43 + row * 14, txt, c, "g" + i);
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
        int ow = Opt(g, "ONLINE", 96, y + 17, txt, Red, "p3");
        string oc = onlineCount;
        if (oc != "") Opt(g, oc, 96 + ow + 10, y + 19, small, oc.StartsWith("NOBODY") ? Dim : Yellow, "p3");

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
            st = Codes.Count == 0 ? "NO GAMES YET: PRESS GET GAMES BELOW" : !Found[Game] ? "NOT FOUND: PRESS GET GAMES BELOW"
               : Mode == 0 ? (InHeadset ? "IN A GAME: HOLD A + B UNTIL THE MENU SHOWS" : "TURN THE HEADSET ON, THEN START") : "KEYS: SEE DOCS\\CONTROLS.TXT";
        Opt(g, st, 14, 254, small, Red, null);
        int nx = 14 + Opt(g, "PLAY NOTES " + (MallNotes.On ? "ON" : "OFF"), 14, 272, small, Red, "notes") + 14;
        nx += Opt(g, "REPORT A PROBLEM", nx, 272, small, Red, "report") + 14;
        bool none = true; for (int i = 0; i < Found.Count; i++) if (Found[i]) none = false;
        Opt(g, "GET GAMES", nx, 272, small, none && blink % 2 == 0 ? Yellow : Red, "ggopen");
        Opt(g, "GAINESPHERE", 272, 8, small, Dim, null);
    }

    // ---- GET GAMES: the community's archive.org copies, downloaded and unpacked into the games folder ----------------------
    void DrawGetGames(Graphics g)
    {
        Opt(g, "GET  GAMES", 10, 4, big, Yellow, null);
        Opt(g, "THE COMMUNITY'S ARCHIVE.ORG COPIES, STRAIGHT INTO", 14, 24, small, Dim, null);
        Opt(g, "YOUR GAMES FOLDER. ABOUT 550 MB IN ALL.", 14, 36, small, Dim, null);
        bool working = MallGetGames.Working, missing = false;
        for (int i = 0; i < MallGetGames.Packs.Length; i++)
        {
            MallGetGames.Pack p = MallGetGames.Packs[i];
            bool have = MallGetGames.Have(p);
            if (!have) missing = true;
            int y = 54 + i * 45;
            Opt(g, p.Title + "  " + (p.Size >> 20) + " MB", 14, y, txt, have ? Dim : Red, null);
            if (have) Opt(g, "GOT IT", 290, y, txt, Dim, null);
            else if (p.Busy) Opt(g, "..." , 300, y, txt, Yellow, null);
            else if (!working) Opt(g, "GET", 300, y, txt, Red, "gg" + i);
            Opt(g, p.Gives, 14, y + 14, small, Dim, null);
            string st = p.Status;
            if (st != "") Opt(g, st.Length > 50 ? st.Substring(0, 50) : st, 14, y + 27, small, p.Failed ? Red : Yellow, null);
            if (st.Length > 50) Opt(g, st.Substring(50), 14, y + 38, small, p.Failed ? Red : Yellow, null);
        }
        int x = 14;
        if (working) x += Opt(g, "STOP", x, 240, big, Red, "ggstop") + 30;
        else if (missing) x += Opt(g, "GET ALL", x, 240, big, blink % 2 == 0 ? Yellow : Red, "ggall") + 30;
        Opt(g, "BACK", x, 241, txt, working ? Dim : Red, working ? null : "ggback");
        Opt(g, "KEEP MALLVR OPEN UNTIL IT SAYS DONE", 14, 262, small, Dim, null);
        if (MallGetGames.Packs.Length == 0) Opt(g, "NO GAME LIST FOUND (ENGINE\\GETGAMES.TXT)", 14, 70, txt, Red, null);
        Opt(g, "GAINESPHERE STUDIOS", 214, 274, small, Dim, null);
    }

    // also opened by itself when MallVR finds no games at all (new players: no folder hunting)
    public void OpenGetGames()
    {
        MallGetGames.Games = System.IO.Path.Combine(Root, "games");
        if (!MallGetGames.Working) MallGetGames.Load(System.IO.Path.Combine(Root, "engine"), System.IO.Path.Combine(Root, "settings"));
        GetGamesScreen = true;
        Invalidate();
    }

    void GetGamesDo(string id)
    {
        if (id == "ggopen") { OpenGetGames(); return; }
        else if (id == "ggall") MallGetGames.Start(null);
        else if (id == "ggstop") MallGetGames.Cancel = true;
        else if (id == "ggback")
        {
            GetGamesScreen = false;
            if (MallGetGames.AnyDone) { Action = "rescan"; Close(); return; }     // launcher.ps1 looks for the new games
        }
        else { int n; if (int.TryParse(id.Substring(2), out n) && n >= 0 && n < MallGetGames.Packs.Length) MallGetGames.Start(MallGetGames.Packs[n]); }
        Invalidate();
    }

    // ---- first launch: ask once about play notes (MallNotes.cs; STATS_AND_REPORTS_DESIGN.md section 1) ----------------------
    void DrawConsent(Graphics g)
    {
        Opt(g, "HELP  MAKE  MALLVR  BETTER?", 10, 4, big, Yellow, null);
        string[] l = {
            "MALLVR CAN SEND US ANONYMOUS PLAY NOTES:",
            "WHICH GAMES YOU PLAY AND FOR HOW LONG,",
            "FRAME RATE, HEADSET AND GRAPHICS CARD,",
            "CRASHES AND ONLINE CONNECTION PROBLEMS.",
            "NO NAMES, NO FILES, NOTHING FROM YOUR PC",
            "BESIDES MALLVR'S OWN LOGS." };
        for (int i = 0; i < l.Length; i++) Opt(g, l[i], 14, 34 + i * 17, txt, Red, null);
        Opt(g, "YES, SEND PLAY NOTES", 14, 152, big, blink % 2 == 0 ? Yellow : Red, "cyes");
        Opt(g, "NO THANKS", 230, 152, big, Red, "cno");
        Opt(g, "OFF UNLESS YOU SAY YES. CHANGE IT ANY TIME:", 14, 196, small, Dim, null);
        Opt(g, "PLAY NOTES ON THE GAME SELECT SCREEN.", 14, 209, small, Dim, null);
        Opt(g, "WHAT WE COLLECT:", 14, 228, small, Dim, null);
        Opt(g, "GAINESPHERE.COM/PAGES/MALLVR-PRIVACY", 14, 241, small, Dim, null);
        Opt(g, "REPORT A PROBLEM WORKS EITHER WAY.", 14, 254, small, Dim, null);
        Opt(g, "GAINESPHERE STUDIOS", 214, 272, small, Dim, null);
    }

    // ---- REPORT A PROBLEM: one typed line + the last game's logs ------------------------------------------------------------
    void DrawReport(Graphics g)
    {
        Opt(g, "REPORT  A  PROBLEM", 10, 4, big, Yellow, null);
        Opt(g, "GAME", 10, 30, big, Yellow, null);
        Opt(g, ReportGame != "" ? ReportGame.ToUpper() : "THE LAST GAME PLAYED", 70, 31, txt, Red, null);
        Opt(g, "WHAT WENT WRONG?", 10, 56, big, Yellow, null);
        string shown = (reportText.Length > 0 ? reportText : editReport ? "" : "CLICK HERE AND TYPE ONE LINE") + (editReport && blink % 2 == 0 ? "_" : "");
        // the typed line wraps over up to four rows
        int row = 0;
        for (int i = 0; i < shown.Length && row < 4; row++)
        {
            int n = Math.Min(44, shown.Length - i);
            if (i + n < shown.Length) { int sp = shown.LastIndexOf(' ', i + n, n); if (sp > i) n = sp - i + 1; }
            Opt(g, shown.Substring(i, n), 14, 78 + row * 16, txt, editReport ? Yellow : Red, "rtext");
            i += n;
        }
        if (shown.Length == 0) Opt(g, "    ", 14, 78, txt, Red, "rtext");
        Opt(g, "YOUR MALLVR LOGS FOR THAT GAME GO WITH IT", 14, 150, small, Dim, null);
        Opt(g, "(YOUR WINDOWS USER NAME IS CUT OUT OF THEM).", 14, 163, small, Dim, null);
        if (!sending)
        {
            bool can = reportText.Trim() != "";
            Opt(g, "SEND", 14, 196, big, can ? (blink % 2 == 0 ? Yellow : Red) : Dim, can ? "rsend" : null);
            Opt(g, "BACK", 120, 197, txt, Red, "rback");
        }
        else Opt(g, "SENDING" + new string('.', blink % 4), 14, 196, big, Yellow, null);
        if (reportNote != "") Opt(g, reportNote, 14, 230, small, Yellow, null);
        Opt(g, "GAINESPHERE STUDIOS", 214, 272, small, Dim, null);
    }

    public void OpenReport(string tag, string game)
    {
        ReportScreen = true; ReportTag = tag ?? ""; ReportGame = game ?? ""; reportText = ""; reportNote = ""; editReport = true;
        Invalidate();
    }

    void SendReport()
    {
        if (sending || reportText.Trim() == "") return;
        sending = true; reportNote = ""; editReport = false;
        string text = reportText, tag = ReportTag, game = ReportGame;
        MallNotes.Add("report", "game", tag);
        System.Threading.ThreadPool.QueueUserWorkItem(delegate
        {
            string r = MallNotes.Report(text, Root, tag, game);
            if (r.StartsWith("SENT")) reportText = "";
            reportNote = r; sending = false;
        });
    }

    // ---- the last game stopped by itself: say so, offer the logs ------------------------------------------------------------
    void DrawCrash(Graphics g)
    {
        Opt(g, "SORRY!", 10, 4, big, Yellow, null);
        Opt(g, CrashGame.ToUpper(), 14, 34, big, Red, null);
        Opt(g, "STOPPED BY ITSELF.", 14, 54, big, Red, null);
        if (MallNotes.On)
        {
            Opt(g, "A SHORT CRASH NOTE WAS SENT (PLAY NOTES ARE ON).", 14, 90, small, Dim, null);
            Opt(g, "SEND THE FULL LOGS TOO?", 14, 116, big, Yellow, null);
            Opt(g, "YES", 30, 146, big, blink % 2 == 0 ? Yellow : Red, "kyes");
            Opt(g, "NO", 110, 146, big, Red, "kno");
        }
        else
        {
            Opt(g, "TELL US WHAT HAPPENED? IT HELPS US FIX IT.", 14, 90, small, Dim, null);
            Opt(g, "REPORT A PROBLEM", 30, 146, big, blink % 2 == 0 ? Yellow : Red, "kyes");
            Opt(g, "NO THANKS", 200, 146, big, Red, "kno");
        }
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
        editReport = id == "rtext";
        if (id == "cyes" || id == "cno") { MallNotes.SetConsent(id == "cyes"); Invalidate(); return; }
        if (id == "notes") { MallNotes.SetConsent(!MallNotes.On); Invalidate(); return; }
        if (id == "report") { OpenReport(ReportTag, ReportGame); return; }
        if (id == "rsend") { SendReport(); Invalidate(); return; }
        if (id == "rback") { if (!sending) { ReportScreen = false; reportNote = ""; } Invalidate(); return; }
        if (id == "rtext") { Invalidate(); return; }
        if (id == "kyes")
        {
            string game = CrashGame; CrashGame = "";
            OpenReport(ReportTag, game);
            if (MallNotes.On) { reportText = "IT STOPPED BY ITSELF (CRASH)"; SendReport(); }
            return;
        }
        if (id == "kno") { CrashGame = ""; Invalidate(); return; }
        if (id.StartsWith("gg")) { GetGamesDo(id); return; }
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
        if (editReport)
        {
            if (e.KeyCode == Keys.Back && reportText.Length > 0) reportText = reportText.Substring(0, reportText.Length - 1);
            else if (e.KeyCode == Keys.Enter) { editReport = false; SendReport(); }
            else if (e.KeyCode == Keys.Escape) editReport = false;
            Invalidate(); e.Handled = true; return;
        }
        if (ReportScreen || CrashGame != "" || MallNotes.NeedsAsk)
        {
            if (e.KeyCode == Keys.Escape && (ReportScreen || CrashGame != "")) Do(ReportScreen ? "rback" : "kno");
            e.Handled = true; return;
        }
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
        int cols = Rows;
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
        if (editReport)
        {
            char ch = char.ToUpper(e.KeyChar);
            if (ch >= 32 && ch < 127 && reportText.Length < 160) { reportText += ch; Invalidate(); }
            e.Handled = true; return;
        }
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
        // the online games this player has: sent to a host (H) so a group only switches to games everyone has
        lock (Net.Have) { Net.Have.Clear(); for (int i = 0; i < Codes.Count; i++) if (Links[i] && Found[i]) { Net.Have.Add(Codes[i]); Net.MaxFor[Codes[i]] = MaxPlayers[i]; } }
        if (PlayerName == "") PlayerName = "PLAYER" + new Random().Next(100, 1000);   // nobody has to type; NAME changes it
        Net.ScreenOpened();
        NameDone();
    }

    void NameDone()
    {
        PlayerName = MallOnline.Clean(PlayerName, 12);
        if (PlayerName == "") { editName = true; return; }
        Net.Name = PlayerName;
        if (NameFile != "")
            try { System.IO.Directory.CreateDirectory(System.IO.Path.GetDirectoryName(NameFile)); System.IO.File.WriteAllText(NameFile, PlayerName); }
            catch (Exception) { }                   // saved name is a convenience; never stops online play
        Net.Start();
        System.Threading.ThreadPool.QueueUserWorkItem(delegate { Net.Beat(); });
    }

    string GameName(string code)
    {
        int i = Codes.IndexOf(code);
        return i >= 0 ? Names[i] : code;
    }

    int quickPick = -1;                          // QUICK MATCH game: -1 = ANY GAME
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
        /* QUICK MATCH plays ANY GAME by default (Bobby 10-20); clicking the game steps through ANY GAME -> each online game -> ANY GAME */
        string qn = Net.Quick ? (Net.QuickGame == "" ? "ANY GAME" : GameName(Net.QuickGame)) : quickPick < 0 ? "ANY GAME" : Names[quickPick];
        if (hg >= 0) Opt(g, qn, 70, y + 1, txt, Net.Quick ? Yellow : Red, Net.Quick ? null : "oqgame");
        else Opt(g, "NO ONLINE GAME FOUND IN THE GAMES FOLDER", 70, y + 1, txt, Dim, null);
        if (hg >= 0 && !Net.Quick) Opt(g, quickPick < 0 ? "CLICK TO PICK ONE GAME" : "CLICK FOR ANOTHER / ANY GAME", 70, y + 18, small, Dim, null);

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
            int nw = Opt(g, h.Name, 152, ry, txt, Red, null);
            if (h.Plat == "QUEST") Opt(g, "QUEST", 152 + nw + 4, ry + 2, small, Dim, null);   // crossplay: Quest hosts get a tag
            Opt(g, h.N + "/" + h.Max, 248, ry, txt, Red, null);
            if (st == "LOBBY")
                Opt(g, full ? "FULL" : !have ? "MISSING" : "JOIN", 278, ry, txt, full || !have ? Dim : Red, full || !have ? null : "oj" + i);
        }

        y = 142;
        Opt(g, "ONLINE NOW", 10, y, big, Yellow, null);
        List<string> words = new List<string>();
        foreach (OnlineEntry e in all) if (!e.FromLan) words.Add(e.Name + (e.Plat == "QUEST" ? " (QUEST)" : "") + (e.Status == "PLAYING" ? " (PLAYING)" : ""));
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
            Opt(g, "HOSTING SHARES YOUR INTERNET ADDRESS WITH WHO JOINS", 14, y - 14, small, Dim, null);
        }
        else
        {
            List<string> r = Net.Roster(), rp = Net.RosterPlat();
            for (int i = 0; i < r.Count && i < rp.Count; i++) if (rp[i] == "QUEST") r[i] += " (QUEST)";
            bool group = Net.InGroup;
            x = 14 + Opt(g, st == "HOST" ? (group ? "YOUR GROUP: " : "YOUR GAME: ") : st == "GROUP" ? "YOUR GROUP: " : "JOINED: ", 14, y - 4, small, Yellow, null);
            // between games the host picks the group's next game: only games everyone has (click = the next one)
            List<string> common = st == "HOST" && group ? Net.CommonGames() : new List<string>();
            x += Opt(g, GameName(Net.Game), x, y - 4, small, common.Count > 1 ? Red : Yellow, common.Count > 1 ? "onext" : null) + 8;
            Opt(g, string.Join(", ", r.ToArray()), x, y - 4, small, Yellow, null);
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
        else if (id == "oback") { Net.ScreenClosed("back"); Net.StopQuick(); Net.Leave(); Net.Note = ""; OnlineScreen = false; }
        else if (id == "oquick")
        {
            if (HostGame() >= 0)
            {
                Net.QuickGames.Clear();
                for (int i = 0; i < Codes.Count; i++) if (Links[i] && Found[i]) { Net.MaxFor[Codes[i]] = MaxPlayers[i]; Net.QuickGames.Add(Codes[i]); }
                Net.StartQuick(quickPick >= 0 && quickPick < Codes.Count ? Codes[quickPick] : "");
            }
        }
        else if (id == "oqgame")
        {   // ANY GAME (-1) -> each online game you have -> ANY GAME
            int next = -1;
            for (int i = quickPick + 1; i < Codes.Count; i++) if (Links[i] && Found[i]) { next = i; break; }
            quickPick = next;
        }
        else if (id == "oqcancel") { Net.StopQuick(); }
        else if (id == "oadv") { Advanced = true; }
        else if (id == "osimple") { Advanced = false; }
        else if (id == "ogame")
        {
            int start = HostGame();
            for (int k = 1; k <= Codes.Count; k++) { int i = (start + k) % Codes.Count; if (Links[i] && Found[i]) { hostPick = i; break; } }
        }
        else if (id == "ohost") { int hg = HostGame(); if (hg >= 0) Net.Host(Codes[hg], MaxPlayers[hg]); }
        else if (id == "ostart") { Net.StartMatch(); }
        else if (id == "onext")
        {
            List<string> common = Net.CommonGames();
            if (common.Count > 0) Net.SetGame(common[(common.IndexOf(Net.Game) + 1) % common.Count]);
        }
        else if (id == "oleave") { Net.Leave(); Net.Note = ""; }
        else if (id.StartsWith("oj"))
        {
            int n = int.Parse(id.Substring(2));
            List<OnlineEntry> hosts = OpenHosts();
            if (n < hosts.Count) Net.Join(hosts[n]);
        }
        Invalidate();
    }

    // ---- the same screen in the headset (MallVrPanel.cs: SteamVR overlay + laser) ---------------------
    public const int CanvasW = W, CanvasH = H;
    public bool InHeadset;                        // shown on the headset panel: the hint line tells how to reach the in-game menu

    public Bitmap VrFrame()
    {
        using (Graphics g = Graphics.FromImage(canvas)) DrawScreen(g);
        return canvas;
    }

    // laser at x, y in screen pixels; click = trigger pulled
    public void VrPoint(int x, int y, bool click)
    {
        string h = "";
        foreach (var k in hits) if (k.Key.Contains(x, y)) { h = k.Value; break; }
        if (h != hover) { hover = h; Invalidate(); }
        if (click) Do(h);
    }

    // a field waiting for typing: its current text (SteamVR's keyboard opens with it), or null
    public string VrWantsText() { return editName ? PlayerName : editPeer ? Peer : editReport ? reportText : null; }

    // the keyboard's text (null = closed without typing)
    public void VrText(string s, bool done)
    {
        if (editReport)
        {
            if (s != null) { StringBuilder b = new StringBuilder(); foreach (char c in s.ToUpper()) if (c >= 32 && c < 127 && b.Length < 160) b.Append(c); reportText = b.ToString(); }
            editReport = false;
        }
        else if (editName)
        {
            if (s != null) PlayerName = MallOnline.Clean(s, 12);
            editName = false; NameDone();
        }
        else if (editPeer)
        {
            if (s != null) { StringBuilder b = new StringBuilder(); foreach (char c in s) if ((char.IsLetterOrDigit(c) || c == '.' || c == ':' || c == '-') && b.Length < 40) b.Append(c); Peer = b.ToString(); }
            editPeer = false;
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
