// MallVR CYBERSPACE HALL: the game select screen's default style (Bobby picked it 10-30 from four mockups,
// branding\launcher_mockups). The VR movies + Virtuality in one: Lawnmower Man gradient sky and floating geometry, a fly-over
// grid floor, games as glowing data blocks, the pod each game ran on turning under a museum spotlight (the H3 turnarounds,
// hall\pod_*.jpg), exhibit sheets fanned like the Disclosure hall (THE MACHINE / THE GAME / HOW TO PLAY, hall\hall_info.txt),
// era dials for the 1000 / 2000 / 3000 pods, and the Gainesphere pillbug carrying the news (news.txt on the download server,
// so it changes without a new build). Laid out on the operator screen's 352x288 grid (clicks, keys and the headset panel
// work the same) and drawn three times sharper. STYLE switches to the pods' operator screen (MallLauncher.cs).
// MallIntro: the Gainesphere intro clip (hall\intro\*.jpg + intro.wav), played on its own thread while the launcher loads.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;

public partial class MallLauncher
{
    public int Style;                             // 0 = CYBERSPACE HALL, 1 = OPERATOR (the pods' own screen); launcher.ps1 saves it
    public bool Music = true;                     // the hall's menu songs (hall\music\*.wav); launcher.ps1 saves it
    System.Media.SoundPlayer musicPlayer;
    bool menuOpen;                                // between Shown and FormClosing
    const int R = 3;                              // the hall is drawn at 3x the 352x288 grid
    Bitmap hallCanvas;
    int sheetTab = SheetFromEnv();   // 0 THE MACHINE / 1 THE GAME / 2 HOW TO PLAY / 3 OPTIONS (dev: MALLVR_SHEET=2 or 3 for a -Shot)
    static int SheetFromEnv() { string s = Environment.GetEnvironmentVariable("MALLVR_SHEET"); return s == "2" ? 2 : s == "3" ? 3 : 1; }
    string viewPod;                               // a pod picked on the era dials (null = the selected game's pod)
    public bool Wanted;                                  // the WANTED exhibit (missing games + CD audio, hall_info.txt [wanted]) is open
    int lastGame = -1;
    readonly DateTime born = DateTime.UtcNow;
    public double ShotTime = -1;                  // launcher.ps1 -Shot: draw the moving parts at this time (s)

    public static readonly string[] EraPods = { "1000CS", "1000SD", "2000SU", "2000SD", "SU3000" };
    static readonly Color Cyan = Color.FromArgb(41, 231, 255), Pink = Color.FromArgb(255, 63, 216), Cream = Color.FromArgb(244, 241, 232);
    static Font hTitle, hBlock, hSmall, hTiny, hSheetT, hSheet, hTimes, hTimesS;

    bool HallNow
    {
        get { return Style == 0 && !MallNotes.NeedsAsk && !ReportScreen && CrashGame == "" && !GetGamesScreen && !(OnlineScreen && Net != null); }
    }

    // the picture of the current screen: the hall (1056x864) or the operator screen (352x288, scaled up in whole pixels by the caller)
    Bitmap Render()
    {
        if (HallNow)
        {
            if (hallCanvas == null) hallCanvas = new Bitmap(W * R, H * R, PixelFormat.Format32bppRgb);
            using (Graphics g = Graphics.FromImage(hallCanvas)) DrawHall(g);
            return hallCanvas;
        }
        using (Graphics g = Graphics.FromImage(canvas)) DrawScreen(g);
        return canvas;
    }

    double Now { get { return ShotTime >= 0 ? ShotTime : (DateTime.UtcNow - born).TotalSeconds; } }

    string GamePod(int i)
    {
        if (i < 0 || i >= Codes.Count) return "2000SU";
        string p = HallData.Get("game " + Codes[i], "pod");
        return p != "" ? p : "2000SU";
    }

    // ---- drawing ----------------------------------------------------------------------------------------------------------
    // three layers so a frame stays cheap (GDI+ draws in software): the sky is made once, the UI (blocks, sheets, dials, options,
    // news, bar) is redrawn only when something on it changes, and each frame only draws the grid, the floating geometry, the
    // spotlight, the turning pod, the rings and the pillbug between them
    Bitmap hallSky, hallUi;
    string uiKey;

    void DrawHall(Graphics g)
    {
        HallData.Load(Path.Combine(Path.Combine(Root, "engine"), "hall"), Path.Combine(Root, "settings"));
        if (hTitle == null)
        {
            hTitle = new Font("Segoe UI", 8.5f, FontStyle.Bold, GraphicsUnit.Pixel);
            hBlock = new Font("Segoe UI", 7f, FontStyle.Bold, GraphicsUnit.Pixel);
            hSmall = new Font("Segoe UI", 6f, FontStyle.Bold, GraphicsUnit.Pixel);
            hTiny = new Font("Segoe UI", 4.6f, FontStyle.Regular, GraphicsUnit.Pixel);
            hSheetT = new Font("Segoe UI", 8f, FontStyle.Bold, GraphicsUnit.Pixel);
            hSheet = new Font("Georgia", 6.1f, FontStyle.Regular, GraphicsUnit.Pixel);
            hTimes = new Font("Times New Roman", 10f, FontStyle.Bold, GraphicsUnit.Pixel);
            hTimesS = new Font("Times New Roman", 7.5f, FontStyle.Bold, GraphicsUnit.Pixel);
        }
        if (Game >= Codes.Count) Game = 0;
        if (Game != lastGame) { if (lastGame >= 0) Wanted = false; lastGame = Game; viewPod = null; }
        bool link = Codes.Count > 0 && Links[Game];
        if (!link && Players != 0) Players = 0;
        double t = Now;
        string pod = viewPod ?? GamePod(Game);

        if (hallSky == null)
        {
            hallSky = new Bitmap(W * R, H * R, PixelFormat.Format32bppPArgb);
            using (Graphics k = Graphics.FromImage(hallSky)) { Setup(k); DrawSky(k); }
        }
        if (hallUi == null) hallUi = new Bitmap(W * R, H * R, PixelFormat.Format32bppPArgb);
        string key = string.Join("|", new string[] { Game.ToString(), Codes.Count.ToString(), Wanted.ToString(), hover, sheetTab.ToString(), pod, Mode.ToString(), Sbs.ToString(),
            Players.ToString(), Pod.ToString(), Peer, editPeer.ToString(), (blink % 2).ToString(), status, onlineCount, HallData.NewsTitle, HallData.NewsText,
            MallNotes.On.ToString(), InHeadset.ToString(), Music.ToString(), optVer.ToString(), OptByHost ? Net.Options : "" });
        if (key != uiKey)
        {
            uiKey = key;
            hits.Clear();
            using (Graphics u = Graphics.FromImage(hallUi))
            {
                u.Clear(Color.Transparent);
                Setup(u);
                Bitmap logo = HallData.Logo;
                if (logo != null) u.DrawImage(logo, new RectangleF(5, 3, 50, 50f * logo.Height / logo.Width));
                HText(u, "MALLVR  ·  lo-fi VR", 60, 9, hSmall, Color.FromArgb(255, 143, 240), null);
                string head = "GAME SELECT";
                HText(u, head, 346 - u.MeasureString(head, hTimes).Width, 5, hTimes, Yellow, null);
                string credit = "POD PHOTOS: SIMON MARSTON COLLECTION";
                HText(u, credit, 346 - u.MeasureString(credit, hTiny).Width, 17, hTiny, Color.FromArgb(120, 150, 175), null);
                DrawBlocks(u);
                if (Wanted) DrawWanted(u);
                else { DrawSheets(u, pod); DrawEra(u, pod); }
                DrawOptions(u, link);
                DrawNews(u);
                DrawBar(u);
            }
        }

        g.ResetTransform();
        g.CompositingMode = CompositingMode.SourceCopy;
        g.DrawImageUnscaled(hallSky, 0, 0);
        g.CompositingMode = CompositingMode.SourceOver;
        Setup(g);
        DrawMotion(g, t);
        if (!Wanted) DrawExhibit(g, pod, t);
        Bitmap bug = HallData.Bug;                    // the pillbug bobs beside its news
        if (bug != null) g.DrawImage(bug, new RectangleF(5, 236 - (float)Math.Abs(Math.Sin(t * 2.2)) * 1.2f, 28, 28f * bug.Height / bug.Width));
        g.ResetTransform();
        g.DrawImageUnscaled(hallUi, 0, 0);
    }

    static void Setup(Graphics g)
    {
        g.ResetTransform();
        g.ScaleTransform(R, R);
        g.SmoothingMode = SmoothingMode.AntiAlias;
        g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
        g.InterpolationMode = InterpolationMode.Bilinear;
        g.PixelOffsetMode = PixelOffsetMode.HighQuality;
    }

    // Lawnmower Man sky (drawn once)
    static void DrawSky(Graphics g)
    {
        g.Clear(Color.FromArgb(2, 2, 8));
        using (GraphicsPath p = new GraphicsPath())
        {
            p.AddEllipse(-120, -150, 592, 420);
            using (PathGradientBrush b = new PathGradientBrush(p))
            {
                b.CenterPoint = new PointF(176, 96);
                b.CenterColor = Color.FromArgb(52, 24, 112);
                b.SurroundColors = new Color[] { Color.FromArgb(2, 2, 8) };
                g.FillRectangle(b, 0, 0, W, H);
            }
        }
    }

    // the grid you glide over and the floating wireframe geometry (every frame)
    static void DrawMotion(Graphics g, double t)
    {
        const float hy = 150, vx = 186;
        float phase = (float)(t * 0.6 % 1.0);
        using (Pen p = new Pen(Color.FromArgb(110, Cyan), 0.5f))
            for (int k = -14; k <= 14; k++) g.DrawLine(p, vx + k * 3, hy, vx + k * 46, H + 10);
        for (int i = 1; i < 24; i++)
        {
            float z = i - phase; if (z < 0.6f) continue;
            float y = hy + 120f / z;
            if (y > H) continue;
            int a = (int)Math.Min(150, 20 + (y - hy) * 1.4f);
            using (Pen p = new Pen(Color.FromArgb(a, Cyan), 0.5f)) g.DrawLine(p, 0, y, W, y);
        }
        using (Pen p = new Pen(Color.FromArgb(28, Pink), 0.5f))
            for (int k = -14; k <= 14; k += 2) g.DrawLine(p, vx + k * 3, hy - 40, vx + k * 46, -10);
        using (Pen p = new Pen(Color.FromArgb(22, Pink), 0.5f))
            for (int i = 1; i < 14; i++)
            {
                float z = i - phase; if (z < 0.6f) continue;
                float y = hy - 40 - 90f / z;
                if (y >= 0) g.DrawLine(p, 0, y, W, y);
            }
        float[,] geo = { { 140, 46, 7, 1.0f }, { 236, 40, 6, -0.7f }, { 244, 132, 4, 1.4f }, { 132, 120, 3.5f, -1.1f } };
        using (Pen p = new Pen(Color.FromArgb(150, 160, 255, 255), 0.5f))
            for (int i = 0; i < geo.GetLength(0); i++)
            {
                float cx = geo[i, 0], cy = geo[i, 1] + (float)Math.Sin(t * 0.9 + i) * 2, r = geo[i, 2];
                double a0 = t * geo[i, 3];
                PointF[] d = new PointF[4];
                for (int j = 0; j < 4; j++) d[j] = new PointF(cx + (float)Math.Cos(a0 + j * Math.PI / 2) * r, cy + (float)Math.Sin(a0 + j * Math.PI / 2) * r * 0.9f);
                g.DrawPolygon(p, d);
            }
    }

    // the games as glowing data blocks on the left
    void DrawBlocks(Graphics g)
    {
        int n = Math.Max(1, Codes.Count);
        float top = 30, rowH = Math.Min(13f, 200f / (n + 1)), bh = rowH - 1.6f;   // + 1 row: WANTED
        if (Codes.Count == 0) HText(g, "NO GAMES YET", 10, top, hBlock, Red, null);
        for (int i = 0; i < Codes.Count; i++)
        {
            bool sel = i == Game, hov = hover == "g" + i;
            float y = top + i * rowH, x = sel ? 12 : 6, w = 120;
            RectangleF r = new RectangleF(x, y, w, bh);
            if (sel)
            {
                using (LinearGradientBrush b = new LinearGradientBrush(r, Cyan, Color.FromArgb(26, 106, 255), 0f)) g.FillRectangle(b, r);
                using (SolidBrush b = new SolidBrush(Color.FromArgb(70, Cyan))) g.FillRectangle(b, x - 1.5f, y - 1.5f, w + 3, bh + 3);
                g.FillRectangle(Brushes.White, x, y, 2, bh);
            }
            else
            {
                using (LinearGradientBrush b = new LinearGradientBrush(r, Color.FromArgb(hov ? 34 : 15, hov ? 70 : 42, hov ? 120 : 74), Color.FromArgb(10, 20, 48), 0f)) g.FillRectangle(b, r);
                using (Pen p = new Pen(Color.FromArgb(hov ? 200 : 90, 40, 110, 170), 0.4f)) g.DrawRectangle(p, x, y, w, bh);
                using (SolidBrush b = new SolidBrush(Found[i] ? Cyan : Color.FromArgb(60, 80, 100))) g.FillRectangle(b, x, y, 1.5f, bh);
            }
            Color tc = sel ? Color.FromArgb(0, 0, 20) : !Found[i] ? Color.FromArgb(90, 110, 130) : Color.FromArgb(220, 245, 255);
            float fy = y + (bh - 8.5f) / 2;
            HText(g, Names[i], x + 5, fy, hBlock, tc, null);
            string tag = Found[i] ? GamePod(i) : "GET GAMES";
            SizeF ts = g.MeasureString(tag, hTiny);
            HText(g, tag, x + w - ts.Width - 2, y + (bh - 6) / 2, hTiny, sel ? Color.FromArgb(0, 20, 60) : Color.FromArgb(140, 170, 200), null);
            hits.Add(new KeyValuePair<Rectangle, string>(new Rectangle((int)x, (int)y, (int)w, (int)Math.Ceiling(bh)), "g" + i));
        }
        // the last block: WANTED, the games and CD audio nobody has found yet
        float wy = top + Codes.Count * rowH, wx = Wanted ? 12 : 6;
        bool wh = hover == "wanted";
        RectangleF wr = new RectangleF(wx, wy, 120, bh);
        using (SolidBrush b = new SolidBrush(Wanted ? Pink : Color.FromArgb(wh ? 90 : 50, Pink))) g.FillRectangle(b, wr);
        using (Pen pn = new Pen(Pink, 0.5f)) g.DrawRectangle(pn, wr.X, wr.Y, wr.Width, wr.Height);
        HText(g, "WANTED: GAMES + MUSIC", wx + 5, wy + (bh - 8.5f) / 2, hBlock, Wanted ? Color.FromArgb(30, 0, 30) : Color.FromArgb(255, 200, 245), null);
        hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(wr), "wanted"));
    }

    // WANTED: a wide exhibit board over the spotlight. hall_info.txt [wanted]: title, text, list1..list6 = "HEADING: item, item" (games, two columns),
    // music = "HEADING: item, item" (its own band under the games), help
    void DrawWanted(Graphics g)
    {
        RectangleF s = new RectangleF(134, 26, 212, 160);
        using (SolidBrush b = new SolidBrush(Color.FromArgb(110, 0, 0, 0))) g.FillRectangle(b, s.X + 2, s.Y + 3, s.Width, s.Height);
        using (SolidBrush b = new SolidBrush(Cream)) g.FillRectangle(b, s);
        using (Pen pn = new Pen(Pink, 1f)) g.DrawRectangle(pn, s.X + 2, s.Y + 2, s.Width - 4, s.Height - 4);
        float x = s.X + 7, y = s.Y + 6;
        HText(g, "WANTED", x, y, hTiny, Color.FromArgb(160, 24, 144), null);
        string title = HallData.Get("wanted", "title");
        using (SolidBrush b = new SolidBrush(Color.FromArgb(26, 26, 26))) g.DrawString(title, hSheetT, b, x, y + 6);
        y += 17;
        using (SolidBrush b = new SolidBrush(Color.FromArgb(40, 40, 40)))
            g.DrawString(HallData.Get("wanted", "text"), hTiny, b, new RectangleF(x, y, s.Width - 14, 14));
        y += 15;
        // two columns of lists
        float colW = (s.Width - 18) / 2, cy0 = y;
        float[] cy = { cy0, cy0 };
        for (int i = 1; i <= 6; i++)
        {
            string l = HallData.Get("wanted", "list" + i);
            if (l == "") continue;
            int c = cy[0] <= cy[1] ? 0 : 1;
            int k = l.IndexOf(':');
            string head = k > 0 ? l.Substring(0, k).Trim() : "", items = k > 0 ? l.Substring(k + 1).Trim() : l;
            float cx = x + c * (colW + 4);
            HText(g, head, cx, cy[c], hTiny, Color.FromArgb(160, 24, 144), null);
            using (SolidBrush b = new SolidBrush(Color.FromArgb(30, 30, 30)))
            {
                SizeF z = g.MeasureString(items, hSheet, (int)colW);
                g.DrawString(items, hSheet, b, new RectangleF(cx, cy[c] + 6, colW, z.Height + 1));
                cy[c] += 6 + z.Height + 3;
            }
        }
        float hy = Math.Max(cy[0], cy[1]);
        string m = HallData.Get("wanted", "music");
        if (m != "")
        {
            using (Pen pn = new Pen(Color.FromArgb(120, Pink), 0.5f)) g.DrawLine(pn, x, hy, s.Right - 7, hy);
            int k = m.IndexOf(':');
            string head = k > 0 ? m.Substring(0, k).Trim() : "", items = k > 0 ? m.Substring(k + 1).Trim() : m;
            HText(g, head, x, hy + 3, hTiny, Color.FromArgb(160, 24, 144), null);
            using (SolidBrush b = new SolidBrush(Color.FromArgb(30, 30, 30)))
            {
                SizeF z = g.MeasureString(items, hSheet, (int)(s.Width - 14));
                g.DrawString(items, hSheet, b, new RectangleF(x, hy + 9, s.Width - 14, z.Height + 1));
                hy += 9 + z.Height + 3;
            }
        }
        hy = Math.Max(hy, s.Bottom - 20);
        using (SolidBrush b = new SolidBrush(Color.FromArgb(120, 10, 100)))
            g.DrawString(HallData.Get("wanted", "help"), hSmall, b, new RectangleF(x, hy, s.Width - 14, s.Bottom - hy - 3));
    }

    // the pod turning under a white museum spotlight, on a pulsing ring
    static Bitmap glow;                               // the museum spotlight, made once
    void DrawExhibit(Graphics g, string pod, double t)
    {
        RectangleF podR = new RectangleF(134, 30, 108, 108);
        RectangleF gr = new RectangleF(podR.X - 18, podR.Y - 12, podR.Width + 36, podR.Height + 30);
        if (glow == null)
        {
            glow = new Bitmap((int)(gr.Width * R), (int)(gr.Height * R), PixelFormat.Format32bppPArgb);
            using (Graphics k = Graphics.FromImage(glow))
            using (GraphicsPath p = new GraphicsPath())
            {
                k.SmoothingMode = SmoothingMode.AntiAlias;
                p.AddEllipse(0, 0, glow.Width, glow.Height);
                using (PathGradientBrush br = new PathGradientBrush(p))
                {
                    br.CenterColor = Color.White;
                    br.SurroundColors = new Color[] { Color.FromArgb(0, 255, 255, 255) };
                    br.FocusScales = new PointF(0.62f, 0.6f);
                    k.FillPath(br, p);
                }
            }
        }
        g.DrawImage(glow, gr);
        float pulse = (float)(Math.Sin(t * 2.6) * 0.5 + 0.5);
        DrawRing(g, 188, 127, 92 + pulse * 6, 16 + pulse, Cyan, 150);
        DrawRing(g, 188, 129, 70 - pulse * 4, 11, Pink, 130);
        Bitmap sheet = HallData.Pod(pod);
        if (sheet != null)
        {
            int f = (int)(t * 10) % 60, tw = sheet.Width / 10, th = sheet.Height / 6;
            g.DrawImage(sheet, podR, new RectangleF((f % 10) * tw, (f / 10) * th, tw, th), GraphicsUnit.Pixel);
        }
        else HText(g, pod, 170, 80, hTitle, Color.FromArgb(40, 40, 60), null);
    }

    static void DrawRing(Graphics g, float cx, float cy, float w, float h, Color c, int a)
    {
        RectangleF r = new RectangleF(cx - w / 2, cy - h / 2, w, h);
        using (Pen p = new Pen(Color.FromArgb(a / 4, c), 3f)) g.DrawEllipse(p, r);
        using (Pen p = new Pen(Color.FromArgb(a, c), 0.9f)) g.DrawEllipse(p, r);
    }

    // three exhibit sheets fanned out: THE MACHINE / THE GAME / HOW TO PLAY
    void DrawSheets(Graphics g, string pod)
    {
        RectangleF s = new RectangleF(252, 30, 94, 128);
        for (int k = 2; k >= 1; k--)
        {
            GraphicsState st = g.Save();
            g.TranslateTransform(s.X + s.Width / 2, s.Y + s.Height / 2);
            g.RotateTransform(-3.5f * k);
            g.TranslateTransform(-s.Width / 2 - 5 * k, -s.Height / 2 + 2 * k);
            using (SolidBrush b = new SolidBrush(Color.FromArgb(k == 2 ? 120 : 190, Cream))) g.FillRectangle(b, 0, 0, s.Width, s.Height);
            g.Restore(st);
        }
        using (SolidBrush b = new SolidBrush(Color.FromArgb(110, 0, 0, 0))) g.FillRectangle(b, s.X + 2, s.Y + 3, s.Width, s.Height);
        using (SolidBrush b = new SolidBrush(Cream)) g.FillRectangle(b, s);

        string kicker, title, tag, body;
        bool cyber = false;
        string gameKey = Codes.Count > 0 ? "game " + Codes[Game] : "";
        List<OptRow> rows = Codes.Count > 0 ? OptRows(Codes[Game]) : null;
        if (sheetTab == 3 && rows == null) sheetTab = 1;          // a game with no options: no OPTIONS tab
        if (sheetTab == 3)
        {
            DrawOptionRows(g, s, rows);
            DrawTabs(g, s, rows != null);
            return;
        }
        if (sheetTab == 0)
        {
            kicker = "THE MACHINE"; title = HallData.Get("pod " + pod, "title"); tag = HallData.Get("pod " + pod, "tag"); body = HallData.Get("pod " + pod, "text");
            cyber = HallData.Get("pod " + pod, "logo") == "cybermind";
            if (title == "") title = "VIRTUALITY " + pod;
        }
        else
        {
            // HOW TO PLAY follows MODE (Bobby 10-31): WINDOW = the keyboard + mouse keys (win=), HEADSET = the controllers (how=)
            // plus, for gun games, the keyboard gun-aim adjust ([howto] gun)
            kicker = sheetTab == 1 ? "THE GAME" : Mode == 1 ? "HOW TO PLAY · WINDOW" : "HOW TO PLAY · HEADSET";
            title = Codes.Count > 0 ? Names[Game] : "NO GAMES YET";
            tag = HallData.Get(gameKey, "tag");
            if (sheetTab == 1) body = HallData.Get(gameKey, "text");
            else if (Mode == 1) body = HallData.Get(gameKey, "win") != "" ? HallData.Get(gameKey, "win") : HallData.Get(gameKey, "how");
            else body = (HallData.Get(gameKey, "how") + (HallData.Get(gameKey, "gun") == "1" ? " | " + HallData.Get("howto", "gun") : "")).Trim();
            if (Codes.Count == 0) body = "Press GET GAMES below: MallVR downloads the community's copies of the games.";
            else if (sheetTab == 2 && body == "") body = "See docs\\CONTROLS.txt. B = coin and start, right trigger = fire.";
        }
        float x = s.X + 6, w = s.Width - 12;
        HText(g, kicker, x, s.Y + 5, hTiny, Color.FromArgb(160, 24, 144), null);
        RectangleF tr = new RectangleF(x, s.Y + 11, w, 22);
        // HOW TO: the list needs the room: a one-line title, no tag
        using (Font tf = sheetTab == 2 ? new Font(hSheet.FontFamily, hSheet.Size, FontStyle.Bold, GraphicsUnit.Pixel) : null)
        {
            Font tfont = tf != null ? tf : hSheetT;
            using (SolidBrush b = new SolidBrush(Color.FromArgb(26, 26, 26))) g.DrawString(title, tfont, b, tr);
        }
        SizeF ts = sheetTab == 2 ? new SizeF(w, 8) : g.MeasureString(title, hSheetT, (int)w);
        float y = s.Y + 11 + ts.Height;
        if (tag != "" && sheetTab != 2) { HText(g, tag, x, y, hTiny, Color.FromArgb(110, 110, 120), null); y += 7; }
        using (SolidBrush b = new SolidBrush(Color.FromArgb(30, 30, 30)))
            if (sheetTab == 2)   // HOW TO: one control per line (Bobby 11-01: "list form"), '|' between them in hall_info.txt
            {
                float ly = y + 2;
                foreach (string item in body.Split('|'))
                {
                    string line = item.Trim(); if (line == "") continue;
                    SizeF ls = g.MeasureString(line, hSheet, (int)w);
                    if (ly + ls.Height > s.Bottom - 13) break;
                    g.DrawString(line, hSheet, b, new RectangleF(x, ly, w, ls.Height));
                    ly += ls.Height + 1.4f;
                }
            }
            else g.DrawString(body, hSheet, b, new RectangleF(x, y + 1, w, s.Bottom - 16 - y - (cyber ? 12 : 0)));
        if (cyber) CyberMind(g, x, s.Bottom - 27, 0.55f);
        DrawTabs(g, s, rows != null);
    }

    // the tabs along the bottom (OPTIONS only for a game that has some)
    void DrawTabs(Graphics g, RectangleF s, bool opts)
    {
        string[] tabs = { "MACHINE", "GAME", "HOW TO", "OPTIONS" };
        float tx = s.X + 6;
        for (int i = 0; i < (opts ? 4 : 3); i++)
        {
            SizeF z = g.MeasureString(tabs[i], hTiny);
            RectangleF r = new RectangleF(tx, s.Bottom - 12, z.Width + 3, 8);
            bool on = sheetTab == i, hov = hover == "tab" + i;
            using (SolidBrush b = new SolidBrush(on ? Color.FromArgb(26, 26, 26) : hov ? Color.FromArgb(220, 214, 200) : Cream)) g.FillRectangle(b, r);
            using (Pen p = new Pen(Color.FromArgb(40, 40, 40), 0.4f)) g.DrawRectangle(p, r.X, r.Y, r.Width, r.Height);
            HText(g, tabs[i], r.X + 1.5f, r.Y + 1, hTiny, on ? Color.White : Color.FromArgb(30, 30, 30), null);
            hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(r), "tab" + i));
            tx += r.Width + (opts ? 1.5f : 3);
        }
    }

    // ---- OPTIONS: the game's own settings (engine\options.ps1, rows added by launcher.ps1), saved in settings\options_<CODE>.txt.
    // A click steps a row to its next value; DEFAULTS puts the game's own values back. In an online group the host's choices count
    // (MallOnline opt=): a member sees them greyed, SET BY HOST.
    public class OptRow { public string Key, Label, Default; public string[] Ids, Labels; }
    readonly Dictionary<string, List<OptRow>> optRows = new Dictionary<string, List<OptRow>>();
    int optVer;

    public void AddOption(string code, string key, string label, string[] ids, string[] labels, string def)
    {
        if (!optRows.ContainsKey(code)) optRows[code] = new List<OptRow>();
        optRows[code].Add(new OptRow { Key = key, Label = label, Ids = ids, Labels = labels, Default = def });
    }

    List<OptRow> OptRows(string code) { List<OptRow> r; return optRows.TryGetValue(code, out r) && r.Count > 0 ? r : null; }

    bool OptByHost { get { return Net != null && Net.InGroup && !Net.IsHost; } }

    string OptFile(string code) { return Path.Combine(Path.Combine(Root, "settings"), "options_" + code + ".txt"); }

    // the saved choices (key -> id); a missing or unknown value = the row's default
    Dictionary<string, string> OptLoad(string code)
    {
        Dictionary<string, string> o = new Dictionary<string, string>();
        try
        {
            if (File.Exists(OptFile(code)))
                foreach (string l in File.ReadAllLines(OptFile(code))) { int e = l.IndexOf('='); if (e > 0) o[l.Substring(0, e).Trim()] = l.Substring(e + 1).Trim(); }
        }
        catch (Exception) { }
        return o;
    }

    static string OptValue(OptRow r, Dictionary<string, string> o) { string v; return o.TryGetValue(r.Key, out v) && Array.IndexOf(r.Ids, v) >= 0 ? v : r.Default; }

    // the host's string ("time:60,death:1") as choices
    static Dictionary<string, string> OptParse(string s)
    {
        Dictionary<string, string> o = new Dictionary<string, string>();
        foreach (string p in (s ?? "").Split(',')) { int c = p.IndexOf(':'); if (c > 0) o[p.Substring(0, c)] = p.Substring(c + 1); }
        return o;
    }

    // every game's choices into MallOnline, sent with START / a switch when this PC hosts
    public void SyncOptions()
    {
        if (Net == null) return;
        lock (Net.OptionsFor)
            foreach (KeyValuePair<string, List<OptRow>> kv in optRows)
            {
                Dictionary<string, string> o = OptLoad(kv.Key);
                List<string> parts = new List<string>();
                foreach (OptRow r in kv.Value) parts.Add(r.Key + ":" + OptValue(r, o));
                Net.OptionsFor[kv.Key] = parts.Count > 0 ? string.Join(",", parts.ToArray()) : "none";
            }
    }

    void DrawOptionRows(Graphics g, RectangleF s, List<OptRow> rows)
    {
        float x = s.X + 6, w = s.Width - 12;
        bool byHost = OptByHost;
        HText(g, byHost ? "OPTIONS · SET BY HOST" : "OPTIONS", x, s.Y + 5, hTiny, Color.FromArgb(160, 24, 144), null);
        string title = Names[Game];
        RectangleF tr = new RectangleF(x, s.Y + 11, w, 22);
        using (SolidBrush b = new SolidBrush(Color.FromArgb(26, 26, 26))) g.DrawString(title, hSheetT, b, tr);
        float y = s.Y + 12 + g.MeasureString(title, hSheetT, (int)w).Height;
        Dictionary<string, string> o = byHost ? OptParse(Net.Options) : OptLoad(Codes[Game]);
        bool any = false;
        for (int i = 0; i < rows.Count; i++)
        {
            OptRow r = rows[i];
            string v = OptValue(r, o);
            if (v != r.Default) any = true;
            string shown = r.Labels[Array.IndexOf(r.Ids, v)];
            RectangleF rr = new RectangleF(x - 1, y, w + 2, 12);
            bool hov = !byHost && hover == "opt" + i;
            if (hov) using (SolidBrush b = new SolidBrush(Color.FromArgb(220, 214, 200))) g.FillRectangle(b, rr);
            using (Pen p = new Pen(Color.FromArgb(200, 190, 170), 0.4f)) g.DrawLine(p, rr.X, rr.Bottom, rr.Right, rr.Bottom);
            Color ink = byHost ? Color.FromArgb(150, 150, 150) : Color.FromArgb(30, 30, 30);
            HText(g, r.Label, x, y + 0.5f, hTiny, byHost ? ink : Color.FromArgb(110, 110, 120), null);
            Color vc = byHost ? ink : v != r.Default ? Color.FromArgb(160, 24, 144) : ink;
            float vw = g.MeasureString(shown, hSmall).Width;
            HText(g, shown, x + w - vw, y + 4.5f, hSmall, vc, null);
            if (!byHost) hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(rr), "opt" + i));
            y += 13;
        }
        if (byHost) { HText(g, "THE HOST'S CHOICES PLAY ON EVERY POD", x, y + 2, hTiny, Color.FromArgb(150, 150, 150), null); return; }
        y += 2;
        SizeF z = g.MeasureString("DEFAULTS", hTiny);
        RectangleF dr = new RectangleF(x, y, z.Width + 3, 8);
        using (SolidBrush b = new SolidBrush(hover == "optdef" ? Color.FromArgb(220, 214, 200) : Cream)) g.FillRectangle(b, dr);
        using (Pen p = new Pen(Color.FromArgb(any ? 40 : 160, 40, 40, 40), 0.4f)) g.DrawRectangle(p, dr.X, dr.Y, dr.Width, dr.Height);
        HText(g, "DEFAULTS", dr.X + 1.5f, dr.Y + 1, hTiny, any ? Color.FromArgb(30, 30, 30) : Color.FromArgb(160, 160, 160), null);
        if (any) hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(dr), "optdef"));
        HText(g, "= AS THE GAME SHIPS", dr.Right + 3, y + 1, hTiny, Color.FromArgb(110, 110, 120), null);
        // what the chosen VERSION has (Bobby 11-01), one per line: hall_info.txt [version CODE], <value id>= with '|' between lines
        foreach (OptRow r in rows)
        {
            if (r.Key != "ver") continue;
            float ly = y + 11;
            using (SolidBrush b = new SolidBrush(Color.FromArgb(30, 30, 30)))
                foreach (string item in HallData.Get("version " + Codes[Game], OptValue(r, o)).Split('|'))
                {
                    string line = item.Trim(); if (line == "") continue;
                    SizeF ls = g.MeasureString(line, hSheet, (int)w);
                    if (ly + ls.Height > s.Bottom - 13) break;
                    g.DrawString(line, hSheet, b, new RectangleF(x, ly, w, ls.Height));
                    ly += ls.Height + 0.6f;
                }
        }
    }

    // a click on a row (next value) or DEFAULTS (deletes the file: the game's own values)
    bool OptDo(string id)
    {
        if (Codes.Count == 0 || OptByHost) return true;
        string code = Codes[Game];
        List<OptRow> rows = OptRows(code);
        if (rows == null) return true;
        try
        {
            if (id == "optdef") { if (File.Exists(OptFile(code))) File.Delete(OptFile(code)); }
            else
            {
                int i; if (!int.TryParse(id.Substring(3), out i) || i < 0 || i >= rows.Count) return true;
                Dictionary<string, string> o = OptLoad(code);
                OptRow r = rows[i];
                o[r.Key] = r.Ids[(Array.IndexOf(r.Ids, OptValue(r, o)) + 1) % r.Ids.Length];
                List<string> lines = new List<string>();
                foreach (OptRow k in rows) lines.Add(k.Key + "=" + OptValue(k, o));
                Directory.CreateDirectory(Path.GetDirectoryName(OptFile(code)));
                File.WriteAllLines(OptFile(code), lines.ToArray());
            }
        }
        catch (Exception) { status = "COULDN'T SAVE THE OPTIONS"; }
        optVer++;
        SyncOptions();
        Invalidate();
        return true;
    }

    // the CyberMind wordmark from the SU3000 loader (as rebuilt for the Quest install video: CYBER white on black, MIND black on yellow)
    static void CyberMind(Graphics g, float x, float y, float sc)
    {
        using (Font f = new Font("Arial Black", 14f * sc, FontStyle.Bold, GraphicsUnit.Pixel))
        {
            float w1 = g.MeasureString("CYBER", f).Width, w2 = g.MeasureString("MIND", f).Width, h = 20 * sc, p = 3 * sc;
            g.FillRectangle(Brushes.Black, x, y, w1 + w2 + p * 2, h);
            using (Pen pn = new Pen(Color.FromArgb(150, 150, 150), Math.Max(0.5f, sc))) g.DrawRectangle(pn, x, y, w1 + w2 + p * 2, h);
            using (SolidBrush b = new SolidBrush(Color.FromArgb(242, 190, 0))) g.FillRectangle(b, x + w1 + p, y + sc, w2 + p - sc, h - 2 * sc);
            g.DrawString("CYBER", f, Brushes.White, x + p * 0.3f, y - sc * 0.5f);
            g.DrawString("MIND", f, Brushes.Black, x + w1 + p * 1.3f, y - sc * 0.5f);
        }
    }

    // era dials: walk the hall from the 1991 pods to the SU3000
    void DrawEra(Graphics g, string pod)
    {
        float cx0 = 150, step = 33, cy = 165, d = 21;
        for (int i = 0; i < EraPods.Length; i++)
        {
            float cx = cx0 + i * step;
            bool on = EraPods[i] == pod, hov = hover == "pod" + i;
            RectangleF r = new RectangleF(cx - d / 2, cy - d / 2, d, d);
            g.FillEllipse(Brushes.White, r);
            Bitmap sheet = HallData.Pod(EraPods[i]);
            if (sheet != null)
            {
                GraphicsState st = g.Save();
                using (GraphicsPath cp = new GraphicsPath()) { cp.AddEllipse(r); g.SetClip(cp); }
                int tw = sheet.Width / 10, th = sheet.Height / 6;
                g.DrawImage(sheet, new RectangleF(r.X - 2, r.Y - 2, d + 4, d + 4), new RectangleF(0, 0, tw, th), GraphicsUnit.Pixel);
                g.Restore(st);
            }
            if (on) using (Pen p = new Pen(Color.FromArgb(90, Cyan), 3f)) g.DrawEllipse(p, r);
            using (Pen p = new Pen(on ? Cyan : hov ? Color.FromArgb(160, 200, 255) : Color.FromArgb(40, 60, 90), on ? 1.2f : 0.8f)) g.DrawEllipse(p, r);
            SizeF z = g.MeasureString(EraPods[i], hTiny);
            HText(g, EraPods[i], cx - z.Width / 2, cy + d / 2 + 1, hTiny, on ? Color.White : Color.FromArgb(150, 180, 210), null);
            hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(new RectangleF(r.X, r.Y, d, d + 8)), "pod" + i));
        }
    }

    // MODE / PLAYERS (same ids as the operator screen)
    void DrawOptions(Graphics g, bool link)
    {
        float x0 = 134, y = 188;
        HText(g, "MODE", x0, y, hTimesS, Yellow, null);
        float x = x0 + 40;
        x += Chip(g, "HEADSET", x, y, Mode == 0, true, "m0") + 3;
        x += Chip(g, "WINDOW", x, y, Mode == 1, true, "m1") + 3;
        Chip(g, Sbs == 1 ? "3D SBS" : Sbs == 2 ? "3D HALF-SBS" : "3D OFF", x, y, Mode == 1 && Sbs > 0, true, "sbs");
        y = 200;
        HText(g, "PLAYERS", x0, y, hTimesS, Yellow, null);
        x = x0 + 40;
        x += Chip(g, "SOLO", x, y, Players == 0, true, "p0") + 3;
        x += Chip(g, "LINK THIS PC", x, y, Players == 1, link, "p1") + 3;
        x += Chip(g, "LINK NETWORK", x, y, Players == 2, link, "p2") + 3;
        y = 212;
        float ox = x0 + 40;
        ox += Chip(g, "ONLINE", ox, y, false, true, "p3") + 4;
        string oc = onlineCount;
        if (Players == 2)
        {
            HText(g, "POD", ox, y + 1, hTimesS, Yellow, null); ox += 18;
            for (int p = 1; p <= 4; p++) ox += Chip(g, p.ToString(), ox, y, Pod == p, true, "d" + p) + 2;
            ox += 3;
            string shown = (Peer.Length > 0 ? Peer : editPeer ? "" : "TYPE OTHER PC") + (editPeer && blink % 2 == 0 ? "_" : "");
            Chip(g, shown, ox, y, editPeer, true, "peer");
        }
        else if (Players == 1) HText(g, "HEADSET = PLAYER 1, A WINDOW = PLAYER 2", ox, y + 1.5f, hTiny, Color.FromArgb(200, 210, 230), null);
        else if (oc != "") HText(g, oc, ox, y + 1.5f, hTiny, oc.StartsWith("NOBODY") ? Color.FromArgb(140, 150, 170) : Yellow, "p3");
    }

    // a small option chip; returns its width
    float Chip(Graphics g, string s, float x, float y, bool on, bool can, string id) { return Chip(g, s, x, y, on, can, id, -1); }

    // alarm >= 0: a red chip that flashes (alarm = the blink count), for a button the player has to press (GET GAMES with games missing)
    float Chip(Graphics g, string s, float x, float y, bool on, bool can, string id, int alarm)
    {
        SizeF z = g.MeasureString(s, hSmall);
        RectangleF r = new RectangleF(x, y, z.Width + 3, 9.5f);
        bool hov = can && id != null && hover == id;
        if (alarm >= 0)
        {
            using (SolidBrush b = new SolidBrush(alarm % 2 == 0 || hov ? Red : Color.FromArgb(106, 10, 10))) g.FillRectangle(b, r);
            using (Pen p = new Pen(Red, 0.8f)) g.DrawRectangle(p, r.X, r.Y, r.Width, r.Height);
            HText(g, s, x + 1.5f, y + 1f, hSmall, Color.White, null);
            if (id != null) hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(r), id));
            return r.Width;
        }
        Color fill = on ? Cyan : hov ? Color.FromArgb(30, 70, 110) : Color.FromArgb(160, 6, 14, 34);
        using (SolidBrush b = new SolidBrush(fill)) g.FillRectangle(b, r);
        using (Pen p = new Pen(can ? Color.FromArgb(on ? 255 : 140, Cyan) : Color.FromArgb(50, 70, 90), 0.5f)) g.DrawRectangle(p, r.X, r.Y, r.Width, r.Height);
        HText(g, s, x + 1.5f, y + 1f, hSmall, on ? Color.FromArgb(0, 10, 30) : can ? Color.FromArgb(220, 245, 255) : Color.FromArgb(70, 90, 110), null);
        if (can && id != null) hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(r), id));
        return r.Width;
    }

    // the news bubble beside the pillbug (and the hint line, in the operator's red)
    void DrawNews(Graphics g)
    {
        RectangleF r = new RectangleF(34, 234, 166, 28);
        bool hov = hover == "news";
        using (GraphicsPath p = Round(r, 4))
        {
            using (SolidBrush b = new SolidBrush(Color.FromArgb(230, 13, 24, 56))) g.FillPath(b, p);
            using (Pen pn = new Pen(hov ? Color.White : Pink, 0.6f)) g.DrawPath(pn, p);
        }
        HText(g, "NEWS", r.X + 4, r.Y + 2, hTiny, Color.FromArgb(255, 143, 240), null);
        string title = HallData.NewsTitle, body = HallData.NewsText;
        HText(g, title, r.X + 22, r.Y + 1.5f, hSmall, Color.White, null);
        using (SolidBrush b = new SolidBrush(Color.FromArgb(255, 222, 238))) g.DrawString(body, hTiny, b, new RectangleF(r.X + 4, r.Y + 10, r.Width - 8, 18));
        if (HallData.NewsLink != "") hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(r), "news"));

        string st = status;
        if (st == "")
            st = Codes.Count == 0 ? "NO GAMES YET: PRESS GET GAMES" : !Found[Game] ? "NOT FOUND: PRESS GET GAMES"
               : Mode == 0 ? (InHeadset ? "IN A GAME: HOLD A + B UNTIL THE MENU SHOWS" : "TURN THE HEADSET ON, THEN START") : "KEYS: SEE DOCS\\CONTROLS.TXT";
        using (SolidBrush b = new SolidBrush(Red)) g.DrawString(st, hTimesS, b, new RectangleF(206, 236, 142, 26));
    }

    // START and the rest along the bottom
    void DrawBar(Graphics g)
    {
        using (LinearGradientBrush b = new LinearGradientBrush(new RectangleF(0, 262, W, 26), Color.FromArgb(0, 0, 0, 0), Color.FromArgb(220, 0, 0, 0), 90f))
            g.FillRectangle(b, 0, 262, W, 26);
        float y = 270, x = 6;
        bool startHov = hover == "start";
        SizeF z = g.MeasureString("START", hTimes);
        RectangleF sr = new RectangleF(x, y - 2, z.Width + 14, 13);
        using (SolidBrush b = new SolidBrush(blink % 2 == 0 || startHov ? Yellow : Color.FromArgb(230, 210, 0))) g.FillRectangle(b, sr);
        HText(g, "START", x + 7, y - 1, hTimes, Color.Black, null);
        hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(sr), "start"));
        x += sr.Width + 6;
        bool none = true; for (int i = 0; i < Found.Count; i++) if (Found[i]) none = false;
        x += Chip(g, "CHECK GAMES", x, y, false, true, "check") + 3;
        x += Chip(g, "UPDATE", x, y, false, true, "update") + 3;
        x += Chip(g, "GET GAMES", x, y, false, true, "ggopen", none || (Codes.Count > 0 && !Found[Game]) ? blink : -1) + 3;   // flashes red while games are missing
        x += Chip(g, "PLAY NOTES " + (MallNotes.On ? "ON" : "OFF"), x, y, false, true, "notes") + 3;
        x += Chip(g, "REPORT", x, y, false, true, "report") + 3;
        x += Chip(g, "STYLE", x, y, false, true, "style") + 3;
        x += Chip(g, "MUSIC", x, y, Music, true, "music") + 3;
        Chip(g, "QUIT", x, y, false, true, "quit");
    }

    // the menu music plays in the hall style only (the operator screen stays the pods' silent one); stopped when the menu closes.
    // A shuffled playlist of every .wav in hall\music (drop more in, no rebuild): each song plays once to its ending, then the
    // next; all of them before any repeats. SoundPlayer can't say when a song ends, so its length comes from the WAV header.
    List<string> playlist = new List<string>();
    DateTime songEnds;
    static readonly Random shuffle = new Random();

    public void MusicSync()
    {
        bool want = Music && Style == 0 && menuOpen;
        try
        {
            if (want && musicPlayer == null)
            {
                if (playlist.Count == 0)
                {
                    string dir = Path.Combine(Path.Combine(Path.Combine(Root, "engine"), "hall"), "music");
                    if (Directory.Exists(dir)) playlist.AddRange(Directory.GetFiles(dir, "*.wav"));
                    for (int i = playlist.Count - 1; i > 0; i--) { int j = shuffle.Next(i + 1); string s = playlist[i]; playlist[i] = playlist[j]; playlist[j] = s; }
                }
                if (playlist.Count == 0) return;
                string song = playlist[0];
                playlist.RemoveAt(0);
                musicPlayer = new System.Media.SoundPlayer(song);
                musicPlayer.Play();
                songEnds = DateTime.Now.AddSeconds(WavSeconds(song) + 0.5);
            }
            else if (!want && musicPlayer != null) { musicPlayer.Stop(); musicPlayer.Dispose(); musicPlayer = null; }
        }
        catch (Exception) { musicPlayer = null; }
    }

    // from the launcher's timer: the song has ended, start the next one
    void MusicTick()
    {
        if (musicPlayer == null || DateTime.Now < songEnds) return;
        musicPlayer.Dispose(); musicPlayer = null;
        MusicSync();
    }

    // PCM WAV length: the data chunk's size / bytes per second (our files are plain 16-bit PCM)
    static double WavSeconds(string path)
    {
        using (BinaryReader r = new BinaryReader(File.OpenRead(path)))
        {
            r.ReadBytes(12);
            int byteRate = 0;
            while (r.BaseStream.Position + 8 <= r.BaseStream.Length)
            {
                string id = new string(r.ReadChars(4));
                int size = r.ReadInt32();
                if (id == "fmt ") { r.ReadBytes(8); byteRate = r.ReadInt32(); r.ReadBytes(size - 12); }
                else if (id == "data") return byteRate > 0 ? (double)size / byteRate : 75;
                else r.ReadBytes(size + (size & 1));
            }
        }
        return 75;
    }

    void HText(Graphics g, string s, float x, float y, Font f, Color c, string id)
    {
        using (SolidBrush b = new SolidBrush(c)) g.DrawString(s, f, b, x, y);
        if (id != null) { SizeF z = g.MeasureString(s, f); hits.Add(new KeyValuePair<Rectangle, string>(Rectangle.Round(new RectangleF(x, y, z.Width, z.Height)), id)); }
    }

    static GraphicsPath Round(RectangleF r, float rad)
    {
        GraphicsPath p = new GraphicsPath(); float d = rad * 2;
        p.AddArc(r.X, r.Y, d, d, 180, 90); p.AddArc(r.Right - d, r.Y, d, d, 270, 90);
        p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90); p.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
        p.CloseFigure(); return p;
    }

    // hall clicks; true = handled
    bool HallDo(string id)
    {
        if (id == "style") { Style = 1 - Style; MusicSync(); Invalidate(); return true; }
        if (id == "music") { Music = !Music; MusicSync(); Invalidate(); return true; }
        if (id.StartsWith("tab")) { sheetTab = id[3] - '0'; Invalidate(); return true; }
        if (id.StartsWith("opt")) return OptDo(id);
        if (id.StartsWith("pod")) { viewPod = EraPods[id[3] - '0']; sheetTab = 0; Invalidate(); return true; }
        if (id == "wanted") { Wanted = !Wanted; Invalidate(); return true; }
        if (id == "news")
        {
            string l = HallData.NewsLink;
            if (l.StartsWith("https://")) try { System.Diagnostics.Process.Start(l); } catch (Exception) { }
            return true;
        }
        return false;
    }
}

// the hall's files: pod turnarounds, the mark, the pillbug, the exhibit text, the news
public static class HallData
{
    public const string NewsUrl = "https://downloads.gainesphere.com/mallvr/news.txt";
    static string dir;
    static readonly Dictionary<string, Dictionary<string, string>> info = new Dictionary<string, Dictionary<string, string>>();
    static readonly Dictionary<string, Bitmap> pods = new Dictionary<string, Bitmap>();
    public static Bitmap Logo, Bug;
    public static volatile string NewsTitle = "", NewsText = "", NewsLink = "";

    public static void Load(string hallDir, string settingsDir)
    {
        if (dir == hallDir) return;
        dir = hallDir;
        try { Logo = Load(Path.Combine(dir, "logo.png")); Bug = Load(Path.Combine(dir, "pillbug.png")); } catch (Exception) { }
        Parse(Path.Combine(dir, "hall_info.txt"), info);
        // news: the copy saved last time (or the one shipped), then the server's in the background
        string cache = Path.Combine(settingsDir, "news_cache.txt");
        SetNews(File.Exists(cache) ? Read(cache) : Read(Path.Combine(dir, "news.txt")));
        System.Threading.ThreadPool.QueueUserWorkItem(delegate
        {
            try
            {
                System.Net.ServicePointManager.SecurityProtocol |= System.Net.SecurityProtocolType.Tls12;
                System.Net.HttpWebRequest r = (System.Net.HttpWebRequest)System.Net.WebRequest.Create(NewsUrl + "?v=" + DateTime.UtcNow.ToString("yyyyMMddHH"));
                r.Timeout = 5000; r.ReadWriteTimeout = 5000; r.UserAgent = "MallVR";
                string s;
                using (System.Net.WebResponse w = r.GetResponse()) using (StreamReader rd = new StreamReader(w.GetResponseStream())) s = rd.ReadToEnd();
                if (s.Contains("title=") && s.Length < 4000) { SetNews(s); try { File.WriteAllText(cache, s); } catch (Exception) { } }
            }
            catch (Exception) { }                     // offline: the saved news stays
        });
    }

    // launcher.ps1: load everything while the intro plays, so the menu's first picture doesn't wait on the pod sheets
    public static void Preload(string hallDir, string settingsDir, string[] podCodes)
    {
        Load(hallDir, settingsDir);
        foreach (string c in podCodes) Pod(c);
    }

    static string Read(string f) { try { return File.Exists(f) ? File.ReadAllText(f) : ""; } catch (Exception) { return ""; } }

    static void SetNews(string s)
    {
        Dictionary<string, string> kv = new Dictionary<string, string>();
        foreach (string line in s.Split('\n'))
        {
            int e = line.IndexOf('='); if (e <= 0 || line.TrimStart().StartsWith("#")) continue;
            kv[line.Substring(0, e).Trim().ToLower()] = line.Substring(e + 1).Trim();
        }
        string v;
        NewsTitle = kv.TryGetValue("title", out v) ? v.ToUpper() : "";
        NewsText = kv.TryGetValue("text", out v) ? v : "";
        NewsLink = kv.TryGetValue("link", out v) ? v : "";
    }

    static void Parse(string file, Dictionary<string, Dictionary<string, string>> into)
    {
        if (!File.Exists(file)) return;
        Dictionary<string, string> cur = null;
        foreach (string raw in File.ReadAllLines(file))
        {
            string line = raw.Trim();
            if (line == "" || line.StartsWith("#")) continue;
            if (line.StartsWith("[") && line.EndsWith("]")) { cur = new Dictionary<string, string>(); into[line.Substring(1, line.Length - 2).Trim()] = cur; continue; }
            int e = line.IndexOf('=');
            if (cur != null && e > 0) cur[line.Substring(0, e).Trim()] = line.Substring(e + 1).Trim();
        }
    }

    public static string Get(string section, string key)
    {
        Dictionary<string, string> s; string v;
        return info.TryGetValue(section, out s) && s.TryGetValue(key, out v) ? v : "";
    }

    static Bitmap Load(string f)
    {
        if (!File.Exists(f)) return null;
        using (Bitmap b = new Bitmap(f)) return new Bitmap(b);   // a copy: the file isn't locked (the updater replaces it)
    }

    // a pod's turnaround: 60 frames, 10 x 6, white studio; each frame's edge fades out so it melts into the spotlight
    public static Bitmap Pod(string code)
    {
        Bitmap b;
        if (pods.TryGetValue(code, out b)) return b;
        b = null;
        try
        {
            string f = Path.Combine(dir ?? "", "pod_" + code + ".jpg");
            if (File.Exists(f))
            {
                using (Bitmap src = new Bitmap(f))
                {
                    b = new Bitmap(src.Width, src.Height, PixelFormat.Format32bppArgb);
                    using (Graphics g = Graphics.FromImage(b)) g.DrawImage(src, 0, 0, src.Width, src.Height);
                }
                Feather(b, b.Width / 10, b.Height / 6);
            }
        }
        catch (Exception) { b = null; }
        pods[code] = b;
        return b;
    }

    static void Feather(Bitmap b, int tw, int th)
    {
        BitmapData d = b.LockBits(new Rectangle(0, 0, b.Width, b.Height), ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb);
        int n = d.Stride * b.Height;
        byte[] px = new byte[n];
        Marshal.Copy(d.Scan0, px, 0, n);
        float cx = tw / 2f, cy = th * 0.52f, r1 = tw * 0.5f, r0 = tw * 0.36f;
        byte[] a = new byte[tw * th];
        for (int y = 0; y < th; y++)
            for (int x = 0; x < tw; x++)
            {
                float dx = x - cx, dy = (y - cy) * 1.05f, dist = (float)Math.Sqrt(dx * dx + dy * dy);
                a[y * tw + x] = (byte)(255 * Math.Max(0, Math.Min(1, (r1 - dist) / (r1 - r0))));
            }
        for (int y = 0; y < b.Height; y++)
            for (int x = 0; x < b.Width; x++)
                px[y * d.Stride + x * 4 + 3] = a[(y % th) * tw + (x % tw)];
        Marshal.Copy(px, 0, d.Scan0, n);
        b.UnlockBits(d);
    }
}

// the Gainesphere intro (blue Saturn G, letters, pillbug, Bobby's sound timing) in a borderless window while the launcher loads.
// Click or any key skips it. Frames: hall\intro\NNN.jpg at 20 fps, sound: hall\intro\intro.wav.
public class MallIntro : Form
{
    static volatile bool running;
    readonly string[] frames;
    readonly System.Media.SoundPlayer sound;
    readonly System.Diagnostics.Stopwatch clock = new System.Diagnostics.Stopwatch();
    readonly Timer tick = new Timer();
    Image cur; int curN = -1;
    const double Fps = 20, Hold = 0.8;

    public static void Start(string dir)
    {
        if (running || !Directory.Exists(dir)) return;
        string[] f = Directory.GetFiles(dir, "*.jpg");
        if (f.Length == 0) return;
        Array.Sort(f, StringComparer.OrdinalIgnoreCase);
        running = true;
        System.Threading.Thread th = new System.Threading.Thread(delegate ()
        {
            try { Application.Run(new MallIntro(dir, f)); } catch (Exception) { }
            running = false;
        });
        th.SetApartmentState(System.Threading.ApartmentState.STA);
        th.IsBackground = true;
        th.Start();
    }

    // the launcher waits for the intro before its first screen (at most 15 s)
    public static void Wait()
    {
        DateTime end = DateTime.UtcNow.AddSeconds(15);
        while (running && DateTime.UtcNow < end) System.Threading.Thread.Sleep(30);
    }

    MallIntro(string dir, string[] f)
    {
        frames = f;
        Text = "MallVR";
        FormBorderStyle = FormBorderStyle.None;
        BackColor = Color.Black;
        StartPosition = FormStartPosition.Manual;
        DoubleBuffered = true;
        TopMost = true;
        Rectangle s = Screen.PrimaryScreen.WorkingArea;
        int w = Math.Min(960, s.Width * 2 / 3), h = w * 9 / 16;
        Bounds = new Rectangle(s.X + (s.Width - w) / 2, s.Y + (s.Height - h) / 2, w, h);
        string wav = Path.Combine(dir, "intro.wav");
        if (File.Exists(wav)) try { sound = new System.Media.SoundPlayer(wav); sound.Load(); } catch (Exception) { sound = null; }
        tick.Interval = 15;
        tick.Tick += delegate { if (clock.Elapsed.TotalSeconds > frames.Length / Fps + Hold) Close(); else Invalidate(); };
        Shown += delegate { if (sound != null) try { sound.Play(); } catch (Exception) { } clock.Start(); tick.Start(); };
        MouseDown += delegate { Close(); };
        KeyDown += delegate { Close(); };
        FormClosed += delegate { tick.Stop(); if (sound != null) try { sound.Stop(); } catch (Exception) { } };
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        double t = clock.Elapsed.TotalSeconds;
        int n = Math.Min(frames.Length - 1, (int)(t * Fps));
        if (n != curN)
        {
            try { Image next; using (Image f = Image.FromFile(frames[n])) next = new Bitmap(f); if (cur != null) cur.Dispose(); cur = next; curN = n; }
            catch (Exception) { }
        }
        Graphics g = e.Graphics;
        g.InterpolationMode = InterpolationMode.HighQualityBilinear;
        if (cur != null) g.DrawImage(cur, ClientRectangle);
        // the end card gets the "lo-fi VR" line under STUDIOS - KANSAS CITY
        double a = Math.Max(0, Math.Min(1, (t - 7.2) / 1.0));
        if (a > 0)
            using (Font f = new Font("Segoe UI", ClientSize.Height * 0.045f, FontStyle.Bold, GraphicsUnit.Pixel))
            using (SolidBrush b = new SolidBrush(Color.FromArgb((int)(a * 255), 255, 143, 240)))
            {
                g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
                string s = "l o - f i   V R";
                SizeF z = g.MeasureString(s, f);
                g.DrawString(s, f, b, (ClientSize.Width - z.Width) / 2, ClientSize.Height * 0.84f);
            }
        // fade to black at the very end
        double fo = Math.Max(0, Math.Min(1, (t - frames.Length / Fps) / Hold));
        if (fo > 0) using (SolidBrush b = new SolidBrush(Color.FromArgb((int)(fo * 255), 0, 0, 0))) g.FillRectangle(b, ClientRectangle);
    }
}
