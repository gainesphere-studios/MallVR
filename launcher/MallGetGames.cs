// GET GAMES on the PC (Bobby 10-26: "set up auto downloader like meta in pc version"): the same archive.org items as the Quest
// launcher's GET GAMES (quest/mallvr_apk/.../GameFiles.java), downloaded into the MallVR folder and unpacked the way the download
// page tells players to. We host nothing; the files come straight from archive.org.
//   - download to games\_downloads\<file>.part, resumed after a dropped connection (HTTP Range), then checked against archive.org's
//     SHA-1 before anything is unpacked; the archive is deleted once unpacked; games\.got_<id> marks a finished item.
//   - .zip: .NET's own ZipArchive. .7z: Windows' own tar.exe (libarchive; Windows 11 unpacks 7z). If tar can't (older Windows 10),
//     the download is kept and the screen says to open it with 7-Zip.
//   - the items come from engine\getgames.txt (ships with each build; format in that file), or from a newer copy on the website
//     that carries Gainesphere's signature (Bobby 10-26: "as we add games we can just give them archive.org urls"). Only
//     archive.org/download URLs are ever used. SU2000 dump: only Simon's folder. Pac-Man: the inner pacman.zip's CUE + BIN.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

public class MallGetGames
{
    public class Pack
    {
        public string Id, Title, Gives, Item, File, Sha1, Unpack; public long Size;
        public volatile string Status = "";               // one line for the screen
        public volatile bool Busy, Done, Failed;
        public string Url { get { return "https://archive.org/download/" + Item + "/" + Uri.EscapeDataString(File); } }
    }

    public static Pack[] Packs = new Pack[0];
    public const string ListUrl = "https://downloads.gainesphere.com/mallvr/getgames.txt";
    public static volatile string ListNote = "";          // where the list came from (for the screen)

    // the list: the newest signed one (website, then the copy saved in settings), else the one in engine\
    public static void Load(string engine, string settings)
    {
        Pack[] p = Parse(Read(Path.Combine(engine, "getgames.txt")));
        string cache = Path.Combine(settings, "getgames.txt");
        string key = Read(Path.Combine(engine, "release_key.txt"));
        string c = Read(cache), cs = Read(cache + ".sig");
        if (c != "" && Signed(c, cs, key)) { Pack[] q = Parse(c); if (q.Length > 0) { p = q; ListNote = "SAVED LIST"; } }
        if (p.Length > 0) Packs = p;
        // a newer list on the website, in the background (the screen shows the shipped / saved one meanwhile)
        Thread t = new Thread(delegate ()
        {
            try
            {
                ServicePointManager.SecurityProtocol |= (SecurityProtocolType)3072;
                string w = Fetch(ListUrl), ws = Fetch(ListUrl + ".sig");
                if (!Signed(w, ws, key)) { ListNote = "WEBSITE LIST NOT SIGNED: NOT USED"; return; }
                Pack[] q = Parse(w);
                if (q.Length == 0 || Working) return;
                Packs = q; ListNote = "LIST FROM GAINESPHERE.COM";
                try { Directory.CreateDirectory(settings); System.IO.File.WriteAllText(cache, w); System.IO.File.WriteAllText(cache + ".sig", ws); } catch (Exception) { }
            }
            catch (Exception) { }                         // offline: the shipped / saved list stays
        });
        t.IsBackground = true; t.Start();
    }

    static string Read(string f) { try { return System.IO.File.Exists(f) ? System.IO.File.ReadAllText(f) : ""; } catch (Exception) { return ""; } }

    static string Fetch(string url)
    {
        HttpWebRequest r = (HttpWebRequest)WebRequest.Create(url);
        r.Timeout = 6000; r.ReadWriteTimeout = 6000; r.UserAgent = "MallVR-PC (gainesphere.com)";
        using (WebResponse w = r.GetResponse()) using (StreamReader s = new StreamReader(w.GetResponseStream(), Encoding.UTF8)) return s.ReadToEnd();
    }

    // <list>.sig = "sha256 = ..." + "sig = ..." over "MALLVR-GAMES 1|<SHA-256 of the list>" (package/sign_release.ps1 -List)
    static bool Signed(string list, string sig, string pubKey)
    {
        if (list == "" || sig == "" || pubKey == "") return false;
        string b64 = "";
        foreach (string l in sig.Split('\n')) { string t = l.Trim(); if (t.StartsWith("sig = ")) b64 = t.Substring(6).Trim(); }
        if (b64 == "") return false;
        try
        {
            string sha;
            using (SHA256 h = SHA256.Create()) { StringBuilder b = new StringBuilder(); foreach (byte x in h.ComputeHash(Encoding.UTF8.GetBytes(list))) b.Append(x.ToString("X2")); sha = b.ToString(); }
            using (RSACryptoServiceProvider rsa = new RSACryptoServiceProvider())
            {
                rsa.PersistKeyInCsp = false; rsa.FromXmlString(pubKey);
                return rsa.VerifyData(Encoding.ASCII.GetBytes("MALLVR-GAMES 1|" + sha), "SHA256", Convert.FromBase64String(b64));
            }
        }
        catch (Exception) { return false; }
    }

    static Pack[] Parse(string text)
    {
        List<Pack> r = new List<Pack>();
        foreach (string raw in text.Split('\n'))
        {
            string l = raw.Trim();
            if (l == "" || l.StartsWith("#")) continue;
            string[] f = l.Split('|');
            if (f.Length < 8) continue;
            long size; if (!long.TryParse(f[5].Trim(), out size) || size <= 0) continue;
            string sha = f[6].Trim().ToLowerInvariant(), un = f[7].Trim(), item = f[3].Trim(), file = f[4].Trim();
            if (sha.Length != 40 || item == "" || file == "" || item.Contains("/") || item.Contains("..") || file.Contains("..")) continue;
            if (!(un == "7z" || un.StartsWith("7z-keep:") || un == "zip" || un == "zip-cd" || un.StartsWith("zip-cd-inner:"))) continue;
            string id = f[0].Trim().ToLowerInvariant();
            bool okId = id != ""; foreach (char ch in id) if (!char.IsLetterOrDigit(ch) && ch != '_' && ch != '-') okId = false;
            if (!okId) continue;
            r.Add(new Pack { Id = id, Title = f[1].Trim().ToUpperInvariant(), Gives = f[2].Trim().ToUpperInvariant(), Item = item, File = file, Size = size, Sha1 = sha, Unpack = un });
        }
        return r.ToArray();
    }

    public static string Games = "";                      // <MallVR>\games (the launcher sets it)
    public static volatile bool Cancel;
    public static volatile bool AnyDone;                  // something new was unpacked: the launcher looks for games again
    static string Downloads { get { return Path.Combine(Games, "_downloads"); } }

    public static bool Have(Pack p) { return System.IO.File.Exists(Path.Combine(Games, ".got_" + p.Id)); }
    public static bool Working { get { foreach (Pack p in Packs) if (p.Busy) return true; return false; } }

    // one item, or every item not got yet, one after the other, in the background
    public static void Start(Pack only)
    {
        if (Working) return;
        Cancel = false;
        List<Pack> todo = new List<Pack>();
        foreach (Pack p in Packs) if ((only == null || p == only) && !Have(p)) { todo.Add(p); p.Status = only == null ? "WAITING..." : ""; p.Failed = false; }
        foreach (Pack p in todo) p.Busy = true;
        Thread t = new Thread(delegate ()
        {
            foreach (Pack p in todo)
            {
                try { Get(p); p.Done = true; AnyDone = true; }
                catch (Exception e) { p.Failed = true; p.Status = Short(e.Message); }
                finally { p.Busy = false; }
                if (Cancel) { foreach (Pack q in todo) if (q.Busy) { q.Busy = false; q.Status = "STOPPED"; } break; }
            }
        });
        t.IsBackground = true; t.Start();
    }

    static void Get(Pack p)
    {
        ServicePointManager.SecurityProtocol |= (SecurityProtocolType)3072;     // TLS 1.2 on .NET 4.x
        Directory.CreateDirectory(Downloads);
        string done = Path.Combine(Downloads, p.File), part = done + ".part";
        long need = p.Size * 4;                                                 // archive + unpacked, roughly
        long free = new DriveInfo(Path.GetPathRoot(Path.GetFullPath(Games))).AvailableFreeSpace;
        if (!System.IO.File.Exists(done) && free < need) throw new IOException("NOT ENOUGH FREE SPACE: " + (need >> 20) + " MB NEEDED, " + (free >> 20) + " MB FREE");
        if (!System.IO.File.Exists(done))
        {
            for (int tries = 1; ; tries++)
            {
                if (Cancel) throw new IOException("STOPPED: PRESS GET AGAIN TO CARRY ON");
                try { Download(p, part); break; }
                catch (Exception e)
                {
                    if (tries >= 8 || Cancel) throw new IOException("DOWNLOAD FAILED (" + Short(e.Message) + "): PRESS GET AGAIN TO RESUME");
                    p.Status = "CONNECTION PROBLEM, TRYING AGAIN IN 5 S...";
                    Thread.Sleep(5000);
                }
            }
            System.IO.File.Move(part, done);
        }
        p.Status = "CHECKING THE DOWNLOAD...";
        string h = Sha1(done);
        if (h != p.Sha1) { System.IO.File.Delete(done); throw new IOException("THE DOWNLOAD WAS DAMAGED AND WAS DELETED: PRESS GET AGAIN"); }
        p.Status = "UNPACKING...";
        string un = p.Unpack;
        if (un == "7z") Un7z(done, null);
        else if (un.StartsWith("7z-keep:")) Un7z(done, un.Substring(8));
        else if (un == "zip") Unzip(done, Games, false);
        else if (un == "zip-cd") Unzip(done, Games, true);
        else if (un.StartsWith("zip-cd-inner:"))
        {
            string inner = Path.Combine(Downloads, "_inner.zip");
            if (ExtractOne(done, un.Substring(13).ToLowerInvariant(), inner)) { Unzip(inner, Games, true); System.IO.File.Delete(inner); }
            else Unzip(done, Games, true);
        }
        System.IO.File.WriteAllText(Path.Combine(Games, ".got_" + p.Id), DateTime.Now.ToString("yyyy-MM-dd HH:mm") + " " + p.File + "\r\n");
        System.IO.File.Delete(done);
        p.Status = "DONE";
    }

    static void Download(Pack p, string part)
    {
        long have = System.IO.File.Exists(part) ? new FileInfo(part).Length : 0;
        if (have >= p.Size) return;
        HttpWebRequest r = (HttpWebRequest)WebRequest.Create(p.Url);
        r.UserAgent = "MallVR-PC (gainesphere.com)"; r.Timeout = 30000; r.ReadWriteTimeout = 60000; r.AllowAutoRedirect = true;
        if (have > 0) r.AddRange(have);
        HttpWebResponse w;
        try { w = (HttpWebResponse)r.GetResponse(); }
        catch (WebException e)
        {
            HttpWebResponse er = e.Response as HttpWebResponse;
            if (er != null && (int)er.StatusCode == 416 && have > 0) return;
            throw;
        }
        using (w)
        {
            bool append = w.StatusCode == HttpStatusCode.PartialContent && have > 0;
            if (!append) have = 0;
            using (Stream s = w.GetResponseStream())
            using (FileStream o = new FileStream(part, append ? FileMode.Append : FileMode.Create, FileAccess.Write))
            {
                byte[] b = new byte[1 << 16]; int n; long start = have; DateTime t0 = DateTime.UtcNow, last = DateTime.MinValue;
                while ((n = s.Read(b, 0, b.Length)) > 0)
                {
                    o.Write(b, 0, n); have += n;
                    if ((DateTime.UtcNow - last).TotalMilliseconds > 500)
                    {
                        last = DateTime.UtcNow; double secs = Math.Max(0.001, (last - t0).TotalSeconds);
                        p.Status = string.Format(System.Globalization.CultureInfo.InvariantCulture, "DOWNLOADING {0} OF {1} MB ({2:0.0} MB/S)", have >> 20, p.Size >> 20, (have - start) / secs / 1048576);
                    }
                    if (Cancel) throw new IOException("stopped");
                }
            }
        }
        if (have < p.Size) throw new IOException("connection closed at " + (have >> 20) + " MB");
    }

    // .7z through Windows' tar.exe into a scratch folder next to the games, then moved in (keep: only the folder whose name has it)
    static void Un7z(string a, string keep)
    {
        string tar = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "tar.exe");
        string tmp = Path.Combine(Downloads, "_unpack");
        if (Directory.Exists(tmp)) Directory.Delete(tmp, true);
        Directory.CreateDirectory(tmp);
        string err = "";
        bool ok = false;
        if (System.IO.File.Exists(tar))
        {
            ProcessStartInfo si = new ProcessStartInfo(tar, "-xf \"" + a + "\" -C \"" + tmp + "\"") { UseShellExecute = false, CreateNoWindow = true, RedirectStandardError = true };
            using (Process pr = Process.Start(si)) { err = pr.StandardError.ReadToEnd(); pr.WaitForExit(); ok = pr.ExitCode == 0; }
        }
        if (!ok)
        {
            Directory.Delete(tmp, true);
            throw new IOException("THIS WINDOWS CAN'T UNPACK .7Z: OPEN GAMES\\_DOWNLOADS\\" + Path.GetFileName(a).ToUpper() + " WITH 7-ZIP INTO GAMES");
        }
        List<string> tops = new List<string>();
        if (keep != null)
        {
            string[] all = Directory.GetDirectories(tmp, "*", SearchOption.AllDirectories);
            foreach (string d in all) if (string.Equals(Path.GetFileName(d), keep, StringComparison.OrdinalIgnoreCase)) { tops.Add(d); break; }
            if (tops.Count == 0) foreach (string d in all) if (Path.GetFileName(d).Contains(keep)) { tops.Add(d); break; }
            if (tops.Count == 0) { Directory.Delete(tmp, true); throw new IOException("THE ARCHIVE HAS NO FOLDER " + keep.ToUpper()); }
        }
        else { tops.AddRange(Directory.GetDirectories(tmp)); tops.AddRange(Directory.GetFiles(tmp)); }
        foreach (string s in tops) MoveInto(s, Path.Combine(Games, Path.GetFileName(s)));
        Directory.Delete(tmp, true);
    }

    // move a file or folder to its place in games; an existing folder is merged (files replaced)
    static void MoveInto(string src, string dst)
    {
        if (System.IO.File.Exists(src)) { if (System.IO.File.Exists(dst)) System.IO.File.Delete(dst); System.IO.File.Move(src, dst); return; }
        if (!Directory.Exists(dst)) { Directory.Move(src, dst); return; }
        foreach (string f in Directory.GetFiles(src)) MoveInto(f, Path.Combine(dst, Path.GetFileName(f)));
        foreach (string d in Directory.GetDirectories(src)) MoveInto(d, Path.Combine(dst, Path.GetFileName(d)));
    }

    // a path inside an archive -> a file under 'to' (never outside it)
    static string Target(string to, string name)
    {
        string full = Path.GetFullPath(Path.Combine(to, name.Replace('/', Path.DirectorySeparatorChar)));
        if (!full.StartsWith(Path.GetFullPath(to).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
            throw new IOException("BAD PATH IN THE ARCHIVE: " + name);
        return full;
    }

    // .zip into 'to'; cdOnly: just the .CUE / .BIN files, flat
    static void Unzip(string a, string to, bool cdOnly)
    {
        using (FileStream fs = System.IO.File.OpenRead(a))
        using (ZipArchive z = new ZipArchive(fs, ZipArchiveMode.Read))
        {
            foreach (ZipArchiveEntry e in z.Entries)
            {
                if (Cancel) throw new IOException("STOPPED WHILE UNPACKING: PRESS GET AGAIN");
                string name = e.FullName.Replace('\\', '/');
                bool dir = name.EndsWith("/");
                if (cdOnly)
                {
                    string u = name.ToUpperInvariant();
                    if (dir || !(u.EndsWith(".CUE") || u.EndsWith(".BIN"))) continue;
                    name = name.Substring(name.LastIndexOf('/') + 1);
                }
                string f = Target(to, name);
                if (dir) { Directory.CreateDirectory(f); continue; }
                Directory.CreateDirectory(Path.GetDirectoryName(f));
                using (Stream i = e.Open()) using (FileStream o = System.IO.File.Create(f)) i.CopyTo(o);
                try { System.IO.File.SetLastWriteTime(f, e.LastWriteTime.DateTime); } catch (Exception) { }
            }
        }
    }

    static bool ExtractOne(string a, string tail, string outFile)
    {
        using (FileStream fs = System.IO.File.OpenRead(a))
        using (ZipArchive z = new ZipArchive(fs, ZipArchiveMode.Read))
            foreach (ZipArchiveEntry e in z.Entries)
            {
                if (e.FullName.EndsWith("/") || !e.FullName.ToLowerInvariant().EndsWith(tail)) continue;
                using (Stream i = e.Open()) using (FileStream o = System.IO.File.Create(outFile)) i.CopyTo(o);
                return true;
            }
        return false;
    }

    static string Sha1(string f)
    {
        using (SHA1 s = SHA1.Create()) using (FileStream fs = System.IO.File.OpenRead(f))
        {
            StringBuilder b = new StringBuilder();
            foreach (byte x in s.ComputeHash(fs)) b.Append(x.ToString("x2"));
            return b.ToString();
        }
    }

    static string Short(string s) { s = (s ?? "").Replace("\r", " ").Replace("\n", " ").ToUpperInvariant(); return s.Length > 90 ? s.Substring(0, 90) : s; }
}
