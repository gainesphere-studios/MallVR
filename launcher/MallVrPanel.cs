// MallVR launcher in the headset: the same operator-screen launcher (MallLauncher), shown as a SteamVR overlay panel in front of
// you and clicked with the controller's laser (SteamVR draws the laser and sends mouse events). Typing (screen name, address) uses
// SteamVR's own keyboard. Runs as an OpenVR overlay app (VRApplication_Overlay), so it lives next to a game without being one.
// launcher.ps1 makes one panel for the whole session (Start), hands it each new launcher screen (Attach), and the panel hides while a
// game runs (Detach). Needs SteamVR already running; otherwise Start returns false and only the desktop window shows.
// In a game (WaitGame): holding A + B for 1 s makes the engine send "MENU" (dosbox_src/vpix_menu.h) and the panel comes up over the
// game as the BUTTON MASHER MENU (RESUME / BACK TO MENU / SWITCH GAME / REPORT A PROBLEM / PLAY NOTES); the engine rests the controls
// while it's open. Online groups: the host's SWITCH GAME and the members' GO / STAY OUT question (MallOnline.cs) run here too.
// Bindings: Valve's openvr_api.cs (BSD-3, same SDK as the engine's openvr_api.dll), compiled in beside this file.
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using Valve.VR;

public class MallVrPanel
{
    [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)] static extern IntPtr LoadLibrary(string path);

    const int Up = 3;                                 // panel texture = launcher canvas x3 in whole pixels (its 1-bit letters stay sharp)
    public float Metres = 1.6f, Distance = 1.5f;      // panel width, distance in front of the head
    public float BackMetres = 5.0f, BackBehind = 1.2f; // backdrop width, how far behind the panel
    public string Note = "";                          // why the panel isn't showing (for the log)
    CVROverlay ov;
    ulong handle, back;                               // back = optional backdrop picture behind the panel (engine\backdrop.jpg)
    bool ok, shown, typing;
    MallLauncher form;
    Timer timer;
    byte[] last, rgba;
    Bitmap big;
    readonly string log = Environment.GetEnvironmentVariable("MALLVR_VR_LOG");   // dev: every panel event to this file
    System.Net.Sockets.UdpClient menuRx;              // the engine's in-game menu lines (127.0.0.1:MenuPort)
    public int MenuPort;

    // SteamVR often isn't running yet when a headset game is started: it shuts down after a game and the next game starts it
    // (Bobby 10-26: "the buttons aren't opening the menu"). So the game always gets the menu port, and the panel connects to
    // SteamVR the first time the game says HELLO or MENU (by then the game has SteamVR up).
    string engineDir = "";
    public int MenuPortForGame(string dir)
    {
        engineDir = dir;
        if (menuRx == null)
        {
            try { menuRx = new System.Net.Sockets.UdpClient(new System.Net.IPEndPoint(System.Net.IPAddress.Loopback, 0)); MenuPort = ((System.Net.IPEndPoint)menuRx.Client.LocalEndPoint).Port; }
            catch (Exception) { menuRx = null; MenuPort = 0; }
        }
        return MenuPort;
    }

    public bool Start(string engineDir)
    {
        this.engineDir = engineDir;
        if (ok) return true;
        if (System.Diagnostics.Process.GetProcessesByName("vrserver").Length == 0) { Note = "STEAMVR NOT RUNNING"; return false; }
        if (LoadLibrary(System.IO.Path.Combine(engineDir, "openvr_api.dll")) == IntPtr.Zero) { Note = "NO OPENVR_API.DLL"; return false; }
        try
        {
            EVRInitError e = EVRInitError.None;
            OpenVR.Init(ref e, EVRApplicationType.VRApplication_Overlay);
            if (e != EVRInitError.None) { Note = "STEAMVR SAID " + e; return false; }
            ov = OpenVR.Overlay;
            if (ov.CreateOverlay("gainesphere.mallvr.launcher", "MallVR", ref handle) != EVROverlayError.None) { Note = "NO OVERLAY"; OpenVR.Shutdown(); return false; }
            ov.SetOverlayWidthInMeters(handle, Metres);
            ov.SetOverlayInputMethod(handle, VROverlayInputMethod.Mouse);
            HmdVector2_t ms = new HmdVector2_t { v0 = MallLauncher.CanvasW, v1 = MallLauncher.CanvasH };
            ov.SetOverlayMouseScale(handle, ref ms);
            ov.SetOverlayFlag(handle, VROverlayFlags.MakeOverlaysInteractiveIfVisible, true);
            ov.SetOverlaySortOrder(handle, 1);
            // Bobby 10-25: "a background of one of Simon's pictures of his arcade, like behind the window". Only if the file is there
            // (it isn't shipped until Simon says yes).
            string bd = System.IO.Path.Combine(engineDir, "backdrop.jpg");
            if (System.IO.File.Exists(bd) && ov.CreateOverlay("gainesphere.mallvr.backdrop", "MallVR backdrop", ref back) == EVROverlayError.None)
            {
                ov.SetOverlayWidthInMeters(back, BackMetres);
                ov.SetOverlaySortOrder(back, 0);
                if (ov.SetOverlayFromFile(back, bd) != EVROverlayError.None) { ov.DestroyOverlay(back); back = 0; }
            }
            MenuPortForGame(engineDir);
            ok = true;
            return true;
        }
        catch (Exception x) { Note = x.Message; return false; }
    }

    // ---- in a game: the BUTTON MASHER MENU ------------------------------------------------------------------------------------
    // launcher.ps1 starts play.ps1 (VPIX_MENU_PORT = MenuPort in its environment) and waits here instead of blocking, so the panel
    // can answer the engine and the online group (Net). Returns when the game's process has ended:
    //   ""        the game closed by itself (window closed, game over, crash: launcher.ps1 reads the exit code)
    //   "back"    BACK TO MENU             "switch"  the group switched games (Net.SwitchGame / SwitchPod: start that next)
    //   "stayout" STAY OUT: left the group (the game is closed)
    // Menu items: RESUME, BACK TO MENU, SWITCH GAME (host of an online group: games everyone has), REPORT A PROBLEM (SteamVR
    // keyboard, the game's logs go with it), PLAY NOTES ON / OFF. Members get GO / STAY OUT when the host switches (10 s, none = GO).
    // Without SteamVR (window mode) there's no panel: the GO / STAY OUT question is a small window on top, and the switch still happens.
    public MallOnline Net;
    public string Root = "", LogTag = "";             // for REPORT A PROBLEM: the MallVR folder and the game's log name
    public int MenuOpens;                             // play notes: menu opened / what was picked this game
    public string MenuPicks = "";
    string waitNote = "";                             // "SWITCHING TO ..." kept on the panel while the next game loads

    public string WaitGame(System.Diagnostics.Process p, string gameName)
    {
        bool paused = false;                          // the panel is up over the game (controls rested)
        string state = "main";                        // main / switch / waiting / report / prompt / going
        string result = "";
        System.Net.IPEndPoint eng = null;
        Bitmap pc = new Bitmap(MallLauncher.CanvasW, MallLauncher.CanvasH, PixelFormat.Format32bppRgb);
        System.Collections.Generic.List<System.Collections.Generic.KeyValuePair<Rectangle, string>> hits = null;
        string hover = "";
        int frame = 0, promptSeq = 0;
        DateTime goingUntil = DateTime.MinValue, loadingUntil = DateTime.UtcNow.AddSeconds(20);
        string reportText = null;
        volatileNote = "";
        bool sendingReport = false;
        DeskPrompt desk = null;
        string panelAnswer = null;                    // GO / STAY OUT clicked on the panel
        MenuOpens = 0; MenuPicks = "";
        LoadVoice(); WriteVoicePods();
        // dev only: MALLVR_DEV_SWITCH=<GAME>:<seconds> = the host picks SWITCH GAME > GAME that long into the game (window mode has no menu)
        string dev = Environment.GetEnvironmentVariable("MALLVR_DEV_SWITCH") ?? "";
        DateTime devAt = DateTime.MaxValue;
        string devGame = "";
        { string[] d = dev.Split(':'); int s; if (d.Length == 2 && int.TryParse(d[1], out s)) { devGame = d[0]; devAt = DateTime.UtcNow.AddSeconds(s); } }
        Action<string> pick = s => { MenuPicks += (MenuPicks == "" ? "" : ",") + s; };
        Action closeGame = () =>
        {
            Send(eng, "QUIT");                        // the engine leaves at once (vpix_menu.h); then make sure
            for (int i = 0; eng != null && i < 150 && !p.HasExited; i++) { Application.DoEvents(); System.Threading.Thread.Sleep(20); }
            if (!p.HasExited) KillTree(p);
        };
        while (!p.HasExited)
        {
            Application.DoEvents();
            System.Threading.Thread.Sleep(20);
            frame++;
            // still showing "SWITCHING TO ..." from the last game: until this game's engine says hello
            if (ok && shown && !paused && waitNote != "" && DateTime.UtcNow > loadingUntil) { waitNote = ""; Hide(); }
            while (menuRx != null && menuRx.Available > 0)
            {
                System.Net.IPEndPoint from = new System.Net.IPEndPoint(System.Net.IPAddress.Any, 0);
                string l;
                try { l = Encoding.ASCII.GetString(menuRx.Receive(ref from)); } catch (Exception) { break; }
                Log("engine: " + l);
                eng = from;
                if (!ok && engineDir != "" && (l.StartsWith("HELLO") || l.StartsWith("MENU"))) Log("late SteamVR start: " + Start(engineDir) + " " + Note);
                if (l.StartsWith("HELLO")) { if (waitNote != "" && !paused) { waitNote = ""; Hide(); } }
                else if (l.StartsWith("MIC ")) { micMuted = l.Trim().EndsWith("1"); hits = null; }
                else if (l.StartsWith("MENU") && result == "" && ok)
                {
                    if (!paused) { paused = true; state = "main"; last = null; hits = null; Place(); ov.ShowOverlay(handle); shown = true; MenuOpens++; Say(MenuLines[rnd.Next(MenuLines.Length)]); }
                    Send(eng, "OPEN");
                }
                else if (l.StartsWith("RESUME") && state != "prompt" && state != "going" && state != "waiting")
                {
                    if (paused) { Say("TIMTODIE.WAV"); pick("resume"); }
                    paused = false; Hide();
                }
            }

            // ---- the online group ----
            if (Net != null)
            {
                if (DateTime.UtcNow > devAt && devGame != Net.Game) { devAt = DateTime.MaxValue; Log("dev switch to " + devGame + ": " + Net.Propose(devGame)); }
                if (Net.SwitchGame != "" && result == "")
                {   // the host switched (host: its answers are in): close this game, launcher.ps1 starts the next
                    result = "switch"; pick("switch:" + Net.SwitchGame);
                    waitNote = "SWITCHING TO " + Net.SwitchGame + "..."; state = "going";
                    if (ok) { paused = true; hits = DrawMenu(pc, gameName, state, hover, true); Upload(pc, true); if (!shown) { Place(); ov.ShowOverlay(handle); shown = true; } }
                    if (desk != null) { desk.Close(); desk = null; }
                    closeGame();
                    break;
                }
                if (Net.PromptSeq > 0 && Net.PromptSeq != promptSeq && result == "")
                {   // the host asks: GO / STAY OUT
                    promptSeq = Net.PromptSeq; state = "prompt"; Say("SELECT01.WAV");
                    if (ok) { if (!paused) { paused = true; last = null; Place(); ov.ShowOverlay(handle); shown = true; } Send(eng, "OPEN"); hits = null; }
                    else { desk = new DeskPrompt(Net.PromptHost, Net.PromptGame); desk.Show(); }
                }
                if (state == "prompt")
                {
                    int left = (int)Math.Ceiling((Net.PromptUntil - DateTime.UtcNow).TotalSeconds);
                    string answer = desk != null ? desk.Answer : panelAnswer;
                    if (desk != null) desk.SetLeft(left);
                    bool timedOut = answer == null && left <= 0;
                    if (timedOut) answer = "go";
                    if (answer != null)
                    {
                        if (desk != null) { desk.Close(); desk = null; }
                        panelAnswer = null;
                        pick(timedOut ? "none" : answer);
                        Net.Answer(promptSeq, answer == "go");
                        if (answer == "stay") { result = "stayout"; Say("GAMEEND.WAV"); paused = false; Hide(); closeGame(); break; }
                        state = "going"; goingUntil = DateTime.UtcNow.AddSeconds(15); hits = null;
                    }
                }
                if (state == "going" && DateTime.UtcNow > goingUntil && Net.SwitchGame == "")
                {   // the switch never came (host gone): play on
                    state = "main"; paused = false; Hide(); Send(eng, "RESUME");
                }
                if (state == "waiting" && !Net.Proposing && Net.SwitchGame == "") { state = "main"; hits = null; }
            }
            if (!ok || !paused) continue;

            // ---- the panel: clicks, keyboard ----
            string click = null;
            VREvent_t ev = new VREvent_t();
            uint size = (uint)Marshal.SizeOf(typeof(VREvent_t));
            while (ov.PollNextOverlayEvent(handle, ref ev, size))
            {
                EVREventType k = (EVREventType)ev.eventType;
                if (k == EVREventType.VREvent_KeyboardDone)
                {
                    StringBuilder sb = new StringBuilder(200); ov.GetKeyboardText(sb, 200); typing = false;
                    reportText = sb.ToString().Trim().ToUpper();
                    continue;
                }
                if (k == EVREventType.VREvent_KeyboardClosed) { if (typing) { typing = false; reportText = ""; } continue; }
                if (k != EVREventType.VREvent_MouseMove && k != EVREventType.VREvent_MouseButtonDown) continue;
                int x = (int)ev.data.mouse.x, y = MallLauncher.CanvasH - (int)ev.data.mouse.y;
                string h = "";
                if (hits != null) foreach (var kv in hits) if (kv.Key.Contains(x, y)) { h = kv.Value; break; }
                hover = h;
                if (k == EVREventType.VREvent_MouseButtonDown && h != "") click = h;
            }
            if (click != null) Log("menu click: " + click);
            if (click == "resume") { Send(eng, "RESUME"); Say("TIMTODIE.WAV"); pick("resume"); paused = false; Hide(); continue; }
            if (click == "back") { pick("back"); result = "back"; Say("GAMEEND.WAV"); paused = false; Hide(); closeGame(); break; }
            if (click == "switch") { state = "switch"; pick("switchlist"); }
            if (click == "mainmenu") state = "main";
            if (click != null && click.StartsWith("sw:") && Net != null) { if (Net.Propose(click.Substring(3))) { state = "waiting"; pick("propose:" + click.Substring(3)); } }
            if (click == "cancelsw" && Net != null) { Net.CancelPropose(); state = "main"; }
            if (click == "notes") { MallNotes.SetConsent(!MallNotes.On); pick("notes:" + (MallNotes.On ? "on" : "off")); }
            if (click == "voice") { state = "voice"; pick("voice"); }
            if (click != null && click.StartsWith("vv:")) { VoiceClick(click.Substring(3)); hits = null; }
            if (click == "vmic") { micMuted = !micMuted; Send(eng, micMuted ? "MIC 1" : "MIC 0"); pick("mic:" + (micMuted ? "muted" : "live")); hits = null; }
            if (click == "vptt") { voiceVol["*PTT"] = Ptt ? 0 : 1; SaveVoice(); pick("ptt:" + (Ptt ? "on" : "off")); hits = null; }
            if (click == "report")
            {
                state = "report"; pick("report"); reportText = null; volatileNote = "";
                typing = ov.ShowKeyboardForOverlay(handle, 0, 0, 0, "What went wrong?", 160, "", 0) == EVROverlayError.None;
                if (!typing) reportText = "";
            }
            if ((click == "go" || click == "stay") && state == "prompt") panelAnswer = click;   // taken by the prompt code above, next loop
            if (state == "report" && reportText != null && reportText != "" && !sendingReport)
            {
                sendingReport = true; string text = reportText, tag = LogTag, gname = gameName; reportText = "";
                MallNotes.Add("report", "game", tag, "from", "menu");
                System.Threading.ThreadPool.QueueUserWorkItem(delegate { volatileNote = MallNotes.Report(text, Root, tag, gname); sendingReport = false; });
            }
            if (state == "report" && reportText == "" && !sendingReport && volatileNote == "") state = "main";   // keyboard closed empty
            if (hits == null || frame % 3 == 0)
            {
                string st = state == "report" ? (sendingReport ? "sending" : "report") : state;
                hits = DrawMenu(pc, gameName, st, hover, frame / 20 % 2 == 0);
                Upload(pc, last == null);
            }
        }
        if (desk != null) desk.Close();
        if (shown && waitNote == "") Hide();
        if (waitNote != "") loadingUntil = DateTime.UtcNow.AddSeconds(20);
        pc.Dispose();
        return result;
    }
    volatile string volatileNote = "";               // REPORT A PROBLEM: the send's result line

    // ---- VOICE CHAT volumes (Bobby 10-26: "so people can turn their friends down if too loud or up if not loud enough") ----
    // settingsoice_volume.txt keeps "NAME=percent" per friend ("*ALL" = every voice), so a loud friend stays turned down next
    // time. settingsoice_pods.txt is what the engine reads once a second (play.ps1 VPIX_VOICE_VOL_FILE, vpix_voice.h):
    // "<pod> <percent>" per player + "all <percent>"; pod = place in the group's roster (host = pod 1, the others in join order).
    readonly System.Collections.Generic.Dictionary<string, int> voiceVol = new System.Collections.Generic.Dictionary<string, int>();
    readonly System.Collections.Generic.Dictionary<string, int> voiceMuted = new System.Collections.Generic.Dictionary<string, int>();
    string VoiceFile(string n) { return System.IO.Path.Combine(System.IO.Path.Combine(Root, "settings"), n); }
    int VoiceVol(string key) { int v; return voiceVol.TryGetValue(key, out v) ? v : 100; }

    void LoadVoice()
    {
        if (Root == "") return;
        voiceVol.Clear();
        try
        {
            foreach (string l in System.IO.File.ReadAllLines(VoiceFile("voice_volume.txt")))
            {
                int k = l.LastIndexOf('='), v;
                if (k > 0 && int.TryParse(l.Substring(k + 1), out v)) voiceVol[l.Substring(0, k).Trim().ToUpper()] = Math.Max(0, Math.Min(200, v));
            }
        }
        catch (Exception) { }                         // no file yet = everyone at 100 %
    }

    void WriteVoicePods()
    {
        if (Root == "") return;
        StringBuilder b = new StringBuilder("all " + VoiceVol("*ALL") + "\r\n");
        if (Net != null) { var r = Net.Roster(); for (int i = 0; i < r.Count; i++) b.Append(i + 1).Append(' ').Append(VoiceVol(r[i])).Append("\r\n"); }
        b.Append("ptt ").Append(Ptt ? 1 : 0).Append("\r\n");
        try { System.IO.Directory.CreateDirectory(VoiceFile("")); System.IO.File.WriteAllText(VoiceFile("voice_pods.txt"), b.ToString()); } catch (Exception) { }
    }

    // "-:NAME" / "+:NAME" / "m:NAME" (NAME "*ALL" = every voice)
    void VoiceClick(string c)
    {
        if (c.Length < 3) return;
        string key = c.Substring(2);
        int v = VoiceVol(key);
        if (c[0] == '-') v = Math.Max(0, v - 10);
        else if (c[0] == '+') v = Math.Min(200, v + 10);
        else if (v > 0) { voiceMuted[key] = v; v = 0; }
        else { int was; v = voiceMuted.TryGetValue(key, out was) && was > 0 ? was : 100; }
        voiceVol[key] = v;
        SaveVoice();
    }

    // mic: MUTED / LIVE as the engine last told us ("MIC 0|1"); push to talk is kept with the volumes ("*PTT=1")
    bool micMuted;
    bool Ptt { get { return VoiceVol("*PTT") == 1; } }

    void SaveVoice()
    {
        StringBuilder b = new StringBuilder();
        foreach (var kv in voiceVol) if (kv.Value != 100) b.Append(kv.Key).Append('=').Append(kv.Value).Append("\r\n");
        try { System.IO.Directory.CreateDirectory(VoiceFile("")); System.IO.File.WriteAllText(VoiceFile("voice_volume.txt"), b.ToString()); } catch (Exception) { }
        WriteVoicePods();
    }

    // the whole process tree (play.ps1's PowerShell + the engine)
    static void KillTree(System.Diagnostics.Process p)
    {
        try
        {
            var si = new System.Diagnostics.ProcessStartInfo("taskkill", "/PID " + p.Id + " /T /F") { CreateNoWindow = true, UseShellExecute = false };
            using (var k = System.Diagnostics.Process.Start(si)) k.WaitForExit(5000);
        }
        catch (Exception) { }
    }

    void Hide() { if (ok && shown) ov.HideOverlay(handle); shown = false; }

    void Send(System.Net.IPEndPoint to, string s)
    {
        if (to == null || menuRx == null) return;
        byte[] b = Encoding.ASCII.GetBytes(s);
        for (int i = 0; i < 2; i++) try { menuRx.Send(b, b.Length, to); } catch (Exception) { }
    }

    void Log(string s) { if (log != null) try { System.IO.File.AppendAllText(log, DateTime.Now.ToString("HH:mm:ss.fff ") + s + "\r\n"); } catch (Exception) { } }

    // the menu screen, in the launcher's operator-screen style (black, bold serif capitals, yellow headings, red options)
    System.Collections.Generic.List<System.Collections.Generic.KeyValuePair<Rectangle, string>> DrawMenu(Bitmap pc, string game, string state, string hover, bool blink)
    {
        var hits = new System.Collections.Generic.List<System.Collections.Generic.KeyValuePair<Rectangle, string>>();
        Color yellow = Color.FromArgb(255, 255, 0), red = Color.FromArgb(240, 24, 24), dim = Color.FromArgb(110, 24, 24), white = Color.White;
        using (Graphics g = Graphics.FromImage(pc))
        using (Font big = new Font("Times New Roman", 15f, FontStyle.Bold, GraphicsUnit.Pixel))
        using (Font txt = new Font("Times New Roman", 13f, FontStyle.Bold, GraphicsUnit.Pixel))
        using (Font small = new Font("Times New Roman", 11f, FontStyle.Bold, GraphicsUnit.Pixel))
        {
            g.Clear(Color.Black);
            g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.SingleBitPerPixelGridFit;
            Func<string, int, int, Font, Color, string, int> opt = (s, x, y, f, c, id) =>
            {
                if (id != null && id == hover) c = white;
                using (Brush b = new SolidBrush(c)) g.DrawString(s, f, b, x, y, StringFormat.GenericTypographic);
                SizeF z = g.MeasureString(s, f, 999, StringFormat.GenericTypographic);
                if (id != null) hits.Add(new System.Collections.Generic.KeyValuePair<Rectangle, string>(new Rectangle(x - 4, y - 3, (int)z.Width + 8, (int)z.Height + 6), id));
                return (int)z.Width;
            };
            opt("BUTTON   MASHER   MENU", 10, 4, big, yellow, null);
            opt("GAME", 10, 30, big, yellow, null);
            opt(game.ToUpper(), 70, 31, txt, red, null);
            bool group = Net != null && Net.InGroup;
            if (state == "main")
            {
                int y = 64;
                opt("RESUME", 30, y, big, blink ? yellow : red, "resume"); y += 26;
                opt("BACK TO MENU", 30, y, big, red, "back"); y += 26;
                if (group && Net.IsHost && Net.CommonGames().Count > 1) { opt("SWITCH GAME", 30, y, big, red, "switch"); y += 26; }
                opt("REPORT A PROBLEM", 30, y, big, red, "report"); y += 26;
                if (group) { opt("VOICE CHAT", 30, y, big, red, "voice"); y += 26; }
                opt("PLAY NOTES " + (MallNotes.On ? "ON" : "OFF"), 30, y, txt, red, "notes");
                if (group && !Net.IsHost) opt("THE HOST CAN SWITCH THE GROUP'S GAME", 14, 212, small, dim, null);
                opt("THE GAME KEEPS RUNNING BEHIND THIS MENU", 14, 226, small, dim, null);
                opt("HOLD A + B AGAIN TO CLOSE IT", 14, 240, small, dim, null);
            }
            else if (state == "switch")
            {
                opt("SWITCH THE GROUP TO", 10, 56, big, yellow, null);
                int i = 0;
                foreach (string c in Net.CommonGames())
                {
                    if (c == Net.Game) continue;
                    opt(GameTitle(c), 30 + (i / 7) * 160, 78 + (i % 7) * 18, txt, red, "sw:" + c);
                    i++;
                }
                opt("ONLY GAMES EVERYONE IN THE GROUP HAS", 14, 212, small, dim, null);
                opt("BACK", 30, 240, txt, red, "mainmenu");
            }
            else if (state == "voice")
            {   // per-friend volume: - / + in 10 % steps (0-200 %), MUTE; ALL VOICES on top
                opt("VOICE CHAT", 10, 56, big, yellow, null);
                System.Collections.Generic.List<string> r = Net != null ? Net.Roster() : new System.Collections.Generic.List<string>(), rp = Net != null ? Net.RosterPlat() : new System.Collections.Generic.List<string>();
                int y = 80, row = 0;
                Action<string, string> line = (label, key) =>
                {
                    int v = VoiceVol(key);
                    opt(label, 14, y, txt, red, null);
                    opt("-", 170, y - 2, big, red, "vv:-:" + key);
                    opt(v + "%", 192, y, txt, v == 100 ? red : yellow, null);
                    opt("+", 240, y - 2, big, red, "vv:+:" + key);
                    opt(v == 0 ? "UNMUTE" : "MUTE", 266, y, txt, v == 0 ? yellow : red, "vv:m:" + key);
                    y += 20;
                };
                opt("MY MIC: " + (Ptt ? "PUSH TO TALK (HOLD LEFT STICK)" : micMuted ? "MUTED" : "LIVE"), 14, y, txt, micMuted && !Ptt ? yellow : red, Ptt ? null : "vmic");
                opt(Ptt ? "PTT ON" : "PTT OFF", 266, y, txt, Ptt ? yellow : red, "vptt"); y += 20;
                line("ALL VOICES", "*ALL");
                for (int i = 0; i < r.Count && row < 4; i++)
                {
                    if (r[i] == Net.Name) continue;
                    line(r[i] + (i < rp.Count && rp[i] == "QUEST" ? " (QUEST)" : ""), r[i]); row++;
                }
                if (row == 0) opt("NOBODY ELSE IN THE GROUP YET", 14, y, small, dim, null);
                opt("ONLY CHANGES WHAT YOU HEAR. KEPT FOR NEXT TIME", 14, 212, small, dim, null);
                opt("YOUR MIC: LEFT STICK CLICK = MUTE / LIVE", 14, 226, small, dim, null);
                opt("BACK", 30, 244, txt, red, "mainmenu");
            }
            else if (state == "waiting")
            {
                int left = Math.Max(0, (int)Math.Ceiling((Net.ProposeUntil - DateTime.UtcNow).TotalSeconds));
                opt("ASKING THE GROUP...", 10, 64, big, yellow, null);
                opt("EVERYONE HAS " + left + " S TO SAY GO OR STAY OUT", 14, 92, txt, red, null);
                opt("NO ANSWER = GO", 14, 110, txt, red, null);
                opt("CANCEL", 30, 150, big, red, "cancelsw");
            }
            else if (state == "prompt")
            {
                int left = Math.Max(0, (int)Math.Ceiling((Net.PromptUntil - DateTime.UtcNow).TotalSeconds));
                opt(Net.PromptHost + " WANTS TO PLAY", 10, 64, big, yellow, null);
                opt(GameTitle(Net.PromptGame), 10, 86, big, yellow, null);
                opt("GO", 40, 126, big, blink ? yellow : red, "go");
                opt("STAY OUT", 140, 126, big, red, "stay");
                opt("GOING IN " + left + "...", 14, 170, txt, red, null);
                opt("STAY OUT = YOU LEAVE THE GROUP", 14, 226, small, dim, null);
            }
            else if (state == "going")
            {
                opt(waitNote != "" ? waitNote : "GO! WAITING FOR THE OTHERS...", 10, 80, big, yellow, null);
            }
            else if (state == "report" || state == "sending")
            {
                opt("REPORT A PROBLEM", 10, 64, big, yellow, null);
                string n = volatileNote;
                if (state == "sending") opt("SENDING" + new string('.', (int)(DateTime.Now.Second % 4)), 14, 96, big, yellow, null);
                else if (n != "")
                {
                    for (int i = 0, row = 0; i < n.Length && row < 3; row++) { int m = Math.Min(44, n.Length - i); opt(n.Substring(i, m), 14, 96 + row * 16, txt, red, null); i += m; }
                }
                else opt("TYPE WHAT WENT WRONG ON THE KEYBOARD", 14, 96, txt, red, null);
                opt("THIS GAME'S LOGS GO WITH IT", 14, 150, small, dim, null);
                opt("BACK", 30, 200, big, red, "mainmenu");
            }
            opt("GAINESPHERE STUDIOS", 214, 272, small, dim, null);
        }
        return hits;
    }

    // game code -> name on screen (launcher.ps1 fills it from games.ps1)
    public readonly System.Collections.Generic.Dictionary<string, string> Titles = new System.Collections.Generic.Dictionary<string, string>();
    string GameTitle(string code) { string n; return Titles.TryGetValue(code, out n) ? n.ToUpper() : code; }

    // window mode: the GO / STAY OUT question as a small window on top of the game
    class DeskPrompt : Form
    {
        public volatile string Answer;
        readonly Label lab;
        readonly string what;
        public DeskPrompt(string host, string game)
        {
            what = host + " WANTS TO PLAY " + game;
            Text = "MallVR"; TopMost = true; FormBorderStyle = FormBorderStyle.FixedToolWindow; StartPosition = FormStartPosition.CenterScreen;
            BackColor = Color.Black; ClientSize = new Size(420, 130); ShowInTaskbar = false;
            lab = new Label { ForeColor = Color.Yellow, Font = new Font("Times New Roman", 14f, FontStyle.Bold), Location = new Point(12, 10), Size = new Size(400, 50), Text = what };
            Button go = new Button { Text = "GO", ForeColor = Color.Yellow, BackColor = Color.FromArgb(60, 0, 0), Location = new Point(40, 72), Size = new Size(140, 40), Font = new Font("Times New Roman", 13f, FontStyle.Bold) };
            Button stay = new Button { Text = "STAY OUT", ForeColor = Color.White, BackColor = Color.FromArgb(60, 0, 0), Location = new Point(240, 72), Size = new Size(140, 40), Font = new Font("Times New Roman", 13f, FontStyle.Bold) };
            go.Click += (o, e) => Answer = "go";
            stay.Click += (o, e) => Answer = "stay";
            Controls.Add(lab); Controls.Add(go); Controls.Add(stay);
        }
        public void SetLeft(int s) { if (lab != null) lab.Text = what + "\nGOING IN " + Math.Max(0, s) + "..."; }
    }

    // a new launcher screen: put the panel in front of the player and follow that screen
    // ---- the Dactyl Nightmare announcer (Bobby 10-25: "rob the audio sample from the game") ------------------------------------
    // His real lines, played from the player's OWN game files (DAC\SOUNDS, set by launcher.ps1); nothing of the game ships with
    // MallVR. No DAC files = no voice. Lines (transcribed 10-25): SELECT01 "Please select your character", VS16A "Nice move. Let's
    // get out of here", VS04A "Have you done this before?", VS10A "Are you trying yet?", VS15A "Not again. Are you playing yet?",
    // TIMTODIE "Time to die", GAMEEND "This is where your game ends".
    public string VoiceDir = "";
    static readonly string[] MenuLines = { "VS04A.WAV", "VS10A.WAV", "VS15A.WAV" };
    static readonly Random rnd = new Random();
    System.Media.SoundPlayer voice;
    bool greeted;

    public void Say(string file)
    {
        if (VoiceDir == "") return;
        string f = System.IO.Path.Combine(VoiceDir, file);
        if (!System.IO.File.Exists(f)) return;
        try { if (voice != null) voice.Stop(); voice = new System.Media.SoundPlayer(f); voice.Play(); } catch (Exception) { }
    }

    // play notes: the headset, controllers and runtime ("" when SteamVR isn't up)
    public string DeviceInfo(out string controller, out string runtime)
    {
        controller = ""; runtime = "";
        if (!ok) return "";
        try
        {
            CVRSystem s = OpenVR.System;
            Func<uint, ETrackedDeviceProperty, string> prop = (d, pr) =>
            {
                ETrackedPropertyError e = ETrackedPropertyError.TrackedProp_Success;
                StringBuilder b = new StringBuilder(128);
                s.GetStringTrackedDeviceProperty(d, pr, b, 128, ref e);
                return e == ETrackedPropertyError.TrackedProp_Success ? b.ToString() : "";
            };
            uint r = s.GetTrackedDeviceIndexForControllerRole(ETrackedControllerRole.RightHand);
            if (r != OpenVR.k_unTrackedDeviceIndexInvalid) controller = prop(r, ETrackedDeviceProperty.Prop_ControllerType_String);
            runtime = "SteamVR " + s.GetRuntimeVersion();
            return (prop(0, ETrackedDeviceProperty.Prop_ManufacturerName_String) + " " + prop(0, ETrackedDeviceProperty.Prop_ModelNumber_String)).Trim();
        }
        catch (Exception) { return ""; }
    }

    public void Attach(MallLauncher f)
    {
        if (!ok) return;
        form = f;
        last = null;
        Place();
        if (timer == null) { timer = new Timer(); timer.Interval = 50; timer.Tick += (o, e) => Tick(); }
        timer.Start();
        Upload(true);
        if (!greeted) { greeted = true; Say("SELECT01.WAV"); }
        ov.ShowOverlay(handle);
        if (back != 0) ov.ShowOverlay(back);
        shown = true;
    }

    // the launcher screen closed (a game starts, or quit): hide the panel
    public void Detach()
    {
        if (timer != null) timer.Stop();
        if (ok && typing) { ov.HideKeyboard(); typing = false; }
        if (ok && shown) { ov.HideOverlay(handle); if (back != 0) ov.HideOverlay(back); }
        shown = false; form = null;
    }

    public void Stop()
    {
        Detach();
        if (!ok) return;
        ok = false;
        try { if (back != 0) ov.DestroyOverlay(back); ov.DestroyOverlay(handle); OpenVR.Shutdown(); } catch (Exception) { }
    }

    // standing, Distance metres ahead of where the headset looks (yaw only), at eye height, facing the player
    void Place()
    {
        TrackedDevicePose_t[] poses = new TrackedDevicePose_t[OpenVR.k_unMaxTrackedDeviceCount];
        OpenVR.System.GetDeviceToAbsoluteTrackingPose(ETrackingUniverseOrigin.TrackingUniverseStanding, 0, poses);
        HmdMatrix34_t h = poses[0].mDeviceToAbsoluteTracking;
        float fx = -h.m2, fz = -h.m10;                  // the head's forward (-Z column), flattened
        float len = (float)Math.Sqrt(fx * fx + fz * fz);
        if (!poses[0].bPoseIsValid || len < 0.01f) { fx = 0; fz = -1; len = 1; h.m3 = 0; h.m7 = 1.6f; h.m11 = 0; }
        fx /= len; fz /= len;
        // overlay's +Z points back at the player: Z = -forward, X = up x Z
        HmdMatrix34_t m = new HmdMatrix34_t();
        m.m0 = -fz; m.m1 = 0; m.m2 = -fx; m.m3 = h.m3 + fx * Distance;
        m.m4 = 0;   m.m5 = 1; m.m6 = 0;   m.m7 = h.m7 - 0.1f;
        m.m8 = fx;  m.m9 = 0; m.m10 = -fz; m.m11 = h.m11 + fz * Distance;
        ov.SetOverlayTransformAbsolute(handle, ETrackingUniverseOrigin.TrackingUniverseStanding, ref m);
        if (back != 0)
        {
            m.m3 += fx * BackBehind; m.m11 += fz * BackBehind; m.m7 += 0.3f;
            ov.SetOverlayTransformAbsolute(back, ETrackingUniverseOrigin.TrackingUniverseStanding, ref m);
        }
    }

    void Tick()
    {
        if (!ok || form == null) return;
        VREvent_t ev = new VREvent_t();
        uint size = (uint)Marshal.SizeOf(typeof(VREvent_t));
        while (ov.PollNextOverlayEvent(handle, ref ev, size))
        {
            if (log != null) try { System.IO.File.AppendAllText(log, DateTime.Now.ToString("HH:mm:ss.fff ") + (EVREventType)ev.eventType + " " + ev.data.mouse.x + "," + ev.data.mouse.y + "\r\n"); } catch (Exception) { }
            switch ((EVREventType)ev.eventType)
            {
                case EVREventType.VREvent_MouseMove:
                    form.VrPoint((int)ev.data.mouse.x, MallLauncher.CanvasH - (int)ev.data.mouse.y, false); break;
                case EVREventType.VREvent_MouseButtonDown:
                    form.VrPoint((int)ev.data.mouse.x, MallLauncher.CanvasH - (int)ev.data.mouse.y, true); break;
                case EVREventType.VREvent_KeyboardDone:
                    {
                        StringBuilder sb = new StringBuilder(64);
                        ov.GetKeyboardText(sb, 64);
                        typing = false;
                        form.VrText(sb.ToString(), true);
                        break;
                    }
                case EVREventType.VREvent_KeyboardClosed:
                    if (typing) { typing = false; form.VrText(null, true); }
                    break;
                case EVREventType.VREvent_Quit:
                    OpenVR.System.AcknowledgeQuit_Exiting();
                    Stop();
                    return;
            }
            if (form == null || !form.Visible) return;   // a click closed the screen (START / QUIT)
        }
        string want = form.VrWantsText();
        if (want != null && !typing)
        {
            typing = ov.ShowKeyboardForOverlay(handle, 0, 0, 0, "MallVR", 40, want, 0) == EVROverlayError.None;
            if (!typing) form.VrText(null, true);    // no keyboard: leave the field so the screen isn't stuck
        }
        Upload(false);
    }

    // draw the launcher screen, scale it up and send it to SteamVR when it changed
    void Upload(bool force) { Upload(form.VrFrame(), force); }

    void Upload(Bitmap c, bool force)
    {
        int w = c.Width * Up, hgt = c.Height * Up;
        if (big == null) big = new Bitmap(w, hgt, PixelFormat.Format32bppArgb);
        using (Graphics g = Graphics.FromImage(big))
        {
            g.InterpolationMode = InterpolationMode.NearestNeighbor; g.PixelOffsetMode = PixelOffsetMode.Half;
            g.DrawImage(c, new Rectangle(0, 0, w, hgt), 0, 0, c.Width, c.Height, GraphicsUnit.Pixel);
        }
        BitmapData d = big.LockBits(new Rectangle(0, 0, w, hgt), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
        int n = w * hgt * 4;
        if (rgba == null || rgba.Length != n) rgba = new byte[n];
        Marshal.Copy(d.Scan0, rgba, 0, n);
        big.UnlockBits(d);
        for (int i = 3; i < n; i += 4) rgba[i] = 255;   // opaque
        if (!force && last != null && Same(last, rgba)) return;
        if (last == null || last.Length != n) last = new byte[n];
        Buffer.BlockCopy(rgba, 0, last, 0, n);
        GCHandle pin = GCHandle.Alloc(rgba, GCHandleType.Pinned);
        try
        {
            // a GL texture updated in place: SteamVR keeps showing it while it changes. SetOverlayRaw (the fallback) sends a whole
            // new image each time and the panel blinks off while it loads (Bobby 10-25: "it flickers a bit").
            if (GlReady()) GlUpload(pin.AddrOfPinnedObject(), w, hgt);
            else
            {
                for (int i = 0; i < n; i += 4) { byte b = rgba[i]; rgba[i] = rgba[i + 2]; rgba[i + 2] = b; }   // BGRA -> RGBA
                ov.SetOverlayRaw(handle, pin.AddrOfPinnedObject(), (uint)w, (uint)hgt, 4);
            }
        }
        finally { pin.Free(); }
    }

    // ---- OpenGL texture for the overlay (a hidden window's context on the launcher's own thread) ---------------
    [DllImport("gdi32")] static extern int ChoosePixelFormat(IntPtr dc, ref PFD p);
    [DllImport("gdi32")] static extern bool SetPixelFormat(IntPtr dc, int f, ref PFD p);
    [DllImport("user32")] static extern IntPtr GetDC(IntPtr wnd);
    [DllImport("opengl32")] static extern IntPtr wglCreateContext(IntPtr dc);
    [DllImport("opengl32")] static extern bool wglMakeCurrent(IntPtr dc, IntPtr rc);
    [DllImport("opengl32")] static extern void glGenTextures(int n, ref uint t);
    [DllImport("opengl32")] static extern void glBindTexture(uint target, uint t);
    [DllImport("opengl32")] static extern void glTexParameteri(uint target, uint p, int v);
    [DllImport("opengl32")] static extern void glTexImage2D(uint target, int level, int internalFormat, int w, int h, int border, uint format, uint type, IntPtr data);
    [DllImport("opengl32")] static extern void glFinish();
    [StructLayout(LayoutKind.Sequential)] struct PFD
    {
        public ushort nSize, nVersion; public uint dwFlags; public byte iPixelType, cColorBits, cRedBits, cRedShift, cGreenBits, cGreenShift,
            cBlueBits, cBlueShift, cAlphaBits, cAlphaShift, cAccumBits, cAccumRedBits, cAccumGreenBits, cAccumBlueBits, cAccumAlphaBits,
            cDepthBits, cStencilBits, cAuxBuffers, iLayerType, bReserved; public uint dwLayerMask, dwVisibleMask, dwDamageMask;
    }
    Form glWin; IntPtr glDc, glRc; uint glTex; int glState;   // 0 = not tried, 1 = ready, -1 = failed (use SetOverlayRaw)

    bool GlReady()
    {
        if (glState != 0) return glState > 0;
        glState = -1;
        try
        {
            glWin = new Form { ShowInTaskbar = false, FormBorderStyle = FormBorderStyle.None, Size = new Size(1, 1) };
            glDc = GetDC(glWin.Handle);
            PFD p = new PFD { nSize = (ushort)Marshal.SizeOf(typeof(PFD)), nVersion = 1, dwFlags = 0x25, cColorBits = 32 };   // DRAW_TO_WINDOW | SUPPORT_OPENGL | DOUBLEBUFFER
            int f = ChoosePixelFormat(glDc, ref p);
            if (f == 0 || !SetPixelFormat(glDc, f, ref p)) return false;
            glRc = wglCreateContext(glDc);
            if (glRc == IntPtr.Zero || !wglMakeCurrent(glDc, glRc)) return false;
            glGenTextures(1, ref glTex);
            glBindTexture(0x0DE1, glTex);
            glTexParameteri(0x0DE1, 0x2801, 0x2601); glTexParameteri(0x0DE1, 0x2800, 0x2601);   // linear min / mag (the x3 image keeps edges crisp)
            VRTextureBounds_t b = new VRTextureBounds_t { uMin = 0, vMin = 1, uMax = 1, vMax = 0 };   // GDI rows run top-down, GL bottom-up
            ov.SetOverlayTextureBounds(handle, ref b);
            glState = 1;
        }
        catch (Exception) { }
        return glState > 0;
    }

    void GlUpload(IntPtr px, int w, int h)
    {
        wglMakeCurrent(glDc, glRc);
        glBindTexture(0x0DE1, glTex);
        glTexImage2D(0x0DE1, 0, 0x8058, w, h, 0, 0x80E1, 0x1401, px);   // RGBA8 from BGRA bytes
        glFinish();
        Texture_t t = new Texture_t { handle = (IntPtr)glTex, eType = ETextureType.OpenGL, eColorSpace = EColorSpace.Auto };
        ov.SetOverlayTexture(handle, ref t);
    }

    static bool Same(byte[] a, byte[] b)
    {
        for (int i = 0; i < a.Length; i += 4) if (a[i] != b[i] || a[i + 1] != b[i + 1] || a[i + 2] != b[i + 2]) return false;
        return true;
    }
}
