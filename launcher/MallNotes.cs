// MallVR play notes (opt-in statistics) and REPORT A PROBLEM. Design: online/STATS_AND_REPORTS_DESIGN.md.
//   - Asked once at first launch (MallLauncher's consent screen). OFF until the player says yes; NO THANKS = nothing is sent, ever,
//     except a report the player sends with REPORT A PROBLEM.
//   - The choice + a random stats id live in %APPDATA%\MallVR\play_notes.txt (kept for every MallVR copy / version, like the screen
//     name). The stats id is NOT the player key or the screen name, so play notes can't be tied to a name on the score boards.
//   - Records: one line per event (TAB-separated key=value), queued in memory and in %APPDATA%\MallVR\notes_queue.txt, sent to
//     stats.php at the end of each game and on launcher exit. Unsent lines wait for the next send.
//   - Reports: one typed line + the last game's logs zipped (Windows user name cut out of paths, player key line dropped), POSTed to
//     report.php (5 MB cap). Upload failed = the zip goes to the Desktop for Discord #bug-reports.
// MALLVR_NOTES_URL / MALLVR_REPORT_URL point somewhere else (dev: online/list_mock.py serves both).
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Text;
using System.Threading;

public static class MallNotes
{
    // what the consent screen asks about. Bump when play notes collect something new: everyone is asked again, naming the new thing.
    public const int AskVersion = 1;
    // %APPDATA% as the environment says (like launcher.ps1's screen name file), so a dev test can give each launcher its own
    public static readonly string Dir = Path.Combine(Environment.GetEnvironmentVariable("APPDATA") ?? Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "MallVR");
    public static string Url = Environment.GetEnvironmentVariable("MALLVR_NOTES_URL") ?? "https://downloads.gainesphere.com/mallvr/online/stats.php";
    public static string ReportUrl = Environment.GetEnvironmentVariable("MALLVR_REPORT_URL") ?? "https://downloads.gainesphere.com/mallvr/online/report.php";
    public static string Version = "";
    public static string Consent = "";                 // "yes" / "no" / "" = not asked yet
    public static int Asked;                           // AskVersion the player answered
    public static string StatsId = "";
    static readonly object lk = new object();
    static readonly List<string> queue = new List<string>();
    static bool loaded;

    static string ChoiceFile { get { return Path.Combine(Dir, "play_notes.txt"); } }
    static string QueueFile { get { return Path.Combine(Dir, "notes_queue.txt"); } }

    public static bool On { get { return Consent == "yes" && Asked >= AskVersion; } }
    public static bool NeedsAsk { get { return Asked < AskVersion; } }

    // fallback = the 'stats' line of settings\launcher.txt (used only when %APPDATA% has no answer, e.g. it can't be written)
    public static void Load(string fallback)
    {
        if (loaded) return;
        loaded = true;
        try
        {
            if (File.Exists(ChoiceFile))
                foreach (string l in File.ReadAllLines(ChoiceFile))
                {
                    string[] kv = l.Split(new[] { '=' }, 2);
                    if (kv.Length != 2) continue;
                    string k = kv[0].Trim(), v = kv[1].Trim();
                    if (k == "stats") Consent = v == "yes" ? "yes" : "no";
                    else if (k == "asked") int.TryParse(v, out Asked);
                    else if (k == "id" && v.Length == 32) StatsId = v;
                }
        }
        catch (Exception) { }
        if (Consent == "" && (fallback == "yes" || fallback == "no")) { Consent = fallback; Asked = AskVersion; }
        if (StatsId == "") StatsId = MallOnline.NewKey();
        try { if (File.Exists(QueueFile)) lock (lk) foreach (string l in File.ReadAllLines(QueueFile)) if (l.Trim() != "") queue.Add(l); }
        catch (Exception) { }
        if (!On) lock (lk) queue.Clear();               // said no (or not asked): nothing waits to be sent
    }

    public static void SetConsent(bool yes)
    {
        Consent = yes ? "yes" : "no";
        Asked = AskVersion;
        Save();
        if (!yes) { lock (lk) queue.Clear(); SaveQueue(); }
    }

    static void Save()
    {
        try
        {
            Directory.CreateDirectory(Dir);
            File.WriteAllText(ChoiceFile, "stats = " + Consent + "\r\nasked = " + Asked + "\r\nid = " + StatsId + "\r\n");
        }
        catch (Exception) { }
    }

    static void SaveQueue()
    {
        try
        {
            Directory.CreateDirectory(Dir);
            lock (lk) File.WriteAllLines(QueueFile, queue.ToArray());
        }
        catch (Exception) { }
    }

    // one record: Add("game_end", "game", "DAC", "secs", "312", ...). Nothing happens unless play notes are ON.
    public static void Add(string ev, params string[] kv)
    {
        if (!On) return;
        StringBuilder b = new StringBuilder();
        b.Append("t=").Append((long)(DateTime.UtcNow - new DateTime(1970, 1, 1)).TotalSeconds).Append("\tev=").Append(Field(ev, 24));
        for (int i = 0; i + 1 < kv.Length; i += 2) b.Append('\t').Append(Field(kv[i], 24)).Append('=').Append(Field(kv[i + 1], 20000));
        lock (lk) { queue.Add(b.ToString()); if (queue.Count > 500) queue.RemoveRange(0, queue.Count - 500); }
        SaveQueue();
    }

    static string Field(string s, int max)
    {
        s = Scrub(s ?? "").Replace("\\", "\\\\").Replace("\t", " ").Replace("\r", "").Replace("\n", "\\n");
        return s.Length > max ? s.Substring(0, max) : s;
    }

    // the Windows user name cut out of paths (C:\Users\<name>\...)
    public static string Scrub(string s)
    {
        string u = Environment.UserName;
        if (string.IsNullOrEmpty(u) || s.IndexOf(u, StringComparison.OrdinalIgnoreCase) < 0) return s;
        foreach (string pre in new[] { "\\Users\\", "/Users/", "\\users\\", "/users/" })
        {
            int i;
            while ((i = s.IndexOf(pre + u, StringComparison.OrdinalIgnoreCase)) >= 0) s = s.Substring(0, i) + pre + "USER" + s.Substring(i + pre.Length + u.Length);
        }
        return s;
    }

    // send what's queued. wait = block until done (launcher exit), else a background thread. Failed = kept for next time.
    public static void Flush(bool wait)
    {
        if (!On) return;
        string[] lines;
        lock (lk) { if (queue.Count == 0) return; lines = queue.ToArray(); }
        ThreadStart send = delegate
        {
            try
            {
                string body = "MALLVR-NOTES 1\t" + StatsId + "\t" + Version + "\n" + string.Join("\n", lines) + "\n";
                string r = Post(Url, Encoding.UTF8.GetBytes(body), "text/plain; charset=utf-8");
                if (r.StartsWith("OK"))
                {
                    lock (lk) foreach (string l in lines) queue.Remove(l);
                    SaveQueue();
                }
            }
            catch (Exception) { }
        };
        if (wait) send();
        else { Thread t = new Thread(send); t.IsBackground = true; t.Start(); }
    }

    static string Post(string url, byte[] body, string type)
    {
        ServicePointManager.SecurityProtocol |= (SecurityProtocolType)3072;     // TLS 1.2 on .NET 4.x
        HttpWebRequest r = (HttpWebRequest)WebRequest.Create(url);
        r.Method = "POST"; r.ContentType = type; r.Timeout = 15000; r.ReadWriteTimeout = 15000; r.UserAgent = "MallVR/" + Version;
        r.ContentLength = body.Length;
        using (Stream s = r.GetRequestStream()) s.Write(body, 0, body.Length);
        using (WebResponse w = r.GetResponse())
        using (StreamReader s = new StreamReader(w.GetResponseStream(), Encoding.ASCII))
            return s.ReadToEnd();
    }

    // ---- engine log helpers ------------------------------------------------------------------------------------------------
    // the newest file in logs\ matching pattern changed since 'since' (the game's render log: <tag>.render)
    public static string Newest(string logs, string pattern, DateTime since)
    {
        try
        {
            string best = null; DateTime bt = since;
            foreach (string f in Directory.GetFiles(logs, pattern))
            {
                DateTime t = File.GetLastWriteTime(f);
                if (t >= bt) { bt = t; best = f; }
            }
            return best;
        }
        catch (Exception) { return null; }
    }

    static string ReadShared(string f, long maxTail)
    {
        using (FileStream s = new FileStream(f, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
        {
            if (s.Length > maxTail) s.Seek(-maxTail, SeekOrigin.End);
            using (StreamReader r = new StreamReader(s, Encoding.Default)) return r.ReadToEnd();
        }
    }

    // the last lines of an engine log, without the texture-load lines that fill most of it
    public static string Tail(string f, int lines)
    {
        if (f == null) return "";
        try
        {
            List<string> keep = new List<string>();
            foreach (string l in ReadShared(f, 512 * 1024).Replace("\r", "").Split('\n'))
                if (l != "" && !l.StartsWith("texslot ") && !l.StartsWith("texture ")) keep.Add(l);
            int n = Math.Min(lines, keep.Count);
            return Scrub(string.Join("\n", keep.GetRange(keep.Count - n, n).ToArray()));
        }
        catch (Exception) { return ""; }
    }

    // the engine's frame-rate lines in its render log ("perf: frames N avg A fps worst W ms"; vpix_menu.h, every 30 s in the headset):
    // average over the game and the worst single frame
    // the engine's <tag>.net log (vpix_net.cpp): the last "stats:" line + whether the mic opened -> play-note fields
    public static string[] NetStats(string netLog)
    {
        string tx = "", rx = "", drop = "", foreign = "", vtx = "", vrx = "", mic = "off";
        try
        {
            foreach (string l in File.ReadAllLines(netLog))
            {
                if (l.StartsWith("stats: "))
                {
                    string[] w = l.Substring(7).Split(' ');
                    for (int i = 0; i + 1 < w.Length; i += 2)
                    {
                        string v = w[i + 1];
                        if (w[i] == "tx") tx = v; else if (w[i] == "rx") rx = v; else if (w[i] == "drop") drop = v;
                        else if (w[i] == "foreign") foreign = v; else if (w[i] == "vtx") vtx = v; else if (w[i] == "vrx") vrx = v;
                    }
                }
                else if (l.Contains("VOICE: microphone open")) mic = "open";
                else if (l.Contains("VOICE: no microphone")) mic = "none";
            }
        }
        catch (Exception) { }
        return new string[] { "net_tx", tx, "net_rx", rx, "net_drop", drop, "foreign", foreign, "voice_tx", vtx, "voice_rx", vrx, "mic", mic };
    }

    public static void Perf(string render, out string avg, out string worst)
    {
        avg = ""; worst = "";
        if (render == null) return;
        try
        {
            double frames = 0, secs = 0, w = 0;
            foreach (string l in ReadShared(render, 4 * 1024 * 1024).Split('\n'))
            {
                if (!l.StartsWith("perf: ")) continue;
                string[] f = l.Split(' ');
                // perf: frames <n> secs <s> worst <ms>
                for (int i = 1; i + 1 < f.Length; i++)
                {
                    double v;
                    if (!double.TryParse(f[i + 1], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out v)) continue;
                    if (f[i] == "frames") frames += v; else if (f[i] == "secs") secs += v; else if (f[i] == "worst") w = Math.Max(w, v);
                }
            }
            if (secs > 0) { avg = (frames / secs).ToString("0.0", System.Globalization.CultureInfo.InvariantCulture); worst = w.ToString("0", System.Globalization.CultureInfo.InvariantCulture); }
        }
        catch (Exception) { }
    }

    // ---- REPORT A PROBLEM ----------------------------------------------------------------------------------------------------
    // zips the typed line + the game's logs (logs\<tag>*, newest game first) + settings, sends it. Returns the status line for the
    // screen. tag = the log name of the game the report is about ("" = the newest logs).
    public static string Report(string text, string root, string tag, string game)
    {
        string zip = null;
        try { zip = MakeZip(text, root, tag, game); }
        catch (Exception) { return "COULDN'T MAKE THE REPORT FILE"; }
        try
        {
            string q = "?id=" + (On ? StatsId : "") + "&ver=" + Uri.EscapeDataString(Version) + "&game=" + Uri.EscapeDataString(game ?? "");
            string r = Post(ReportUrl + q, File.ReadAllBytes(zip), "application/zip");
            if (r.StartsWith("OK")) { try { File.Delete(zip); } catch (Exception) { } return "SENT. THANK YOU!"; }
        }
        catch (Exception) { }
        try
        {
            string desk = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), Path.GetFileName(zip));
            File.Copy(zip, desk, true); File.Delete(zip);
            return "COULDN'T SEND: SAVED ON YOUR DESKTOP. DRAG IT INTO DISCORD #BUG-REPORTS";
        }
        catch (Exception) { return "COULDN'T SEND: THE FILE IS IN MALLVR'S LOGS FOLDER"; }
    }

    static string MakeZip(string text, string root, string tag, string game)
    {
        string logs = Path.Combine(root, "logs"), settings = Path.Combine(root, "settings");
        if (string.IsNullOrEmpty(tag))
        {
            string r = Newest(logs, "*.render", DateTime.MinValue);
            tag = r != null ? Path.GetFileNameWithoutExtension(r) : "";
        }
        string zip = Path.Combine(logs, "MallVR_report_" + DateTime.Now.ToString("yyyy-MM-dd_HHmmss") + ".zip");
        using (FileStream fs = new FileStream(zip, FileMode.Create))
        using (ZipArchive z = new ZipArchive(fs, ZipArchiveMode.Create))
        {
            StringBuilder about = new StringBuilder();
            about.Append("MallVR report\r\nversion: ").Append(Version).Append("\r\ngame: ").Append(game).Append("\r\nlogs: ").Append(tag)
                 .Append("\r\nwhen: ").Append(DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss zzz")).Append("\r\nwindows: ").Append(Environment.OSVersion)
                 .Append("\r\nplay notes: ").Append(On ? "on (stats id " + StatsId + ")" : "off").Append("\r\n\r\nwhat went wrong:\r\n").Append(text).Append("\r\n");
            Entry(z, "report.txt", about.ToString());
            if (tag != "")
                foreach (string f in Directory.GetFiles(logs, tag + "*"))
                {
                    string n = Path.GetFileName(f), ext = Path.GetExtension(f).ToLower();
                    if (Path.GetFileNameWithoutExtension(f) != tag && !n.StartsWith(tag + ".")) continue;   // dac.* not dac_pod1.*
                    if (ext == ".dll" || ext == ".zip") continue;
                    long cap = ext == ".calls" ? 1024 * 1024 : 2 * 1024 * 1024;   // the call log is huge: its last 1 MB
                    try { Entry(z, "logs/" + n, Scrub(ReadShared(f, cap))); } catch (Exception) { }
                }
            foreach (string n in new[] { "launcher.txt", "vpix_settings.txt" })
            {
                string f = Path.Combine(settings, n);
                if (!File.Exists(f)) continue;
                try
                {
                    StringBuilder b = new StringBuilder();
                    foreach (string l in File.ReadAllLines(f)) if (!l.TrimStart().StartsWith("key")) b.Append(l).Append("\r\n");   // the player key stays private
                    Entry(z, "settings/" + n, Scrub(b.ToString()));
                }
                catch (Exception) { }
            }
        }
        return zip;
    }

    static void Entry(ZipArchive z, string name, string text)
    {
        ZipArchiveEntry e = z.CreateEntry(name, CompressionLevel.Optimal);
        using (StreamWriter w = new StreamWriter(e.Open(), new UTF8Encoding(false))) w.Write(text);
    }

    // ---- what the player's PC is (launcher start record) ---------------------------------------------------------------------
    public static string WindowsVersion()
    {
        try
        {
            using (Microsoft.Win32.RegistryKey k = Microsoft.Win32.Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows NT\CurrentVersion"))
            {
                string name = (string)k.GetValue("ProductName", ""), disp = (string)k.GetValue("DisplayVersion", ""), build = (string)k.GetValue("CurrentBuild", "");
                int b; if (int.TryParse(build, out b) && b >= 22000) name = name.Replace("Windows 10", "Windows 11");   // the registry still says 10 on 11
                return (name + " " + disp + " build " + build).Trim();
            }
        }
        catch (Exception) { return Environment.OSVersion.ToString(); }
    }

    public static string GpuName()
    {
        try
        {
            // display adapters under the video class key: DriverDesc of each, the first non-basic one
            using (Microsoft.Win32.RegistryKey c = Microsoft.Win32.Registry.LocalMachine.OpenSubKey(@"SYSTEM\CurrentControlSet\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}"))
            {
                List<string> names = new List<string>();
                foreach (string s in c.GetSubKeyNames())
                {
                    if (s.Length != 4) continue;
                    try { using (Microsoft.Win32.RegistryKey k = c.OpenSubKey(s)) { string d = k.GetValue("DriverDesc") as string; if (!string.IsNullOrEmpty(d) && !names.Contains(d)) names.Add(d); } }
                    catch (Exception) { }
                }
                names.Sort((a, b) => (a.Contains("Basic") || a.Contains("Virtual") ? 1 : 0).CompareTo(b.Contains("Basic") || b.Contains("Virtual") ? 1 : 0));
                return string.Join(" + ", names.ToArray());
            }
        }
        catch (Exception) { return ""; }
    }
}
