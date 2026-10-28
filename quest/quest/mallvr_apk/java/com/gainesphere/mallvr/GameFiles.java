package com.gainesphere.mallvr;

import org.apache.commons.compress.archivers.sevenz.SevenZArchiveEntry;
import org.apache.commons.compress.archivers.sevenz.SevenZFile;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.Charset;
import java.security.MessageDigest;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/* GET GAMES: the archive.org items the PC download page (gainesphere.com/pages/mallvr-download) points to, downloaded straight onto
   the headset and unpacked into MallVR/games the same way the page tells PC players to. We host nothing.
   Plain Java (no Android classes), so it's tested on the PC: java ... GameFiles <pack id> <games dir> <download dir>.
   Resume: the download is <file>.part; a dropped connection carries on from where it stopped (HTTP Range). The finished file is
   checked against archive.org's SHA-1 before anything is unpacked. */
public class GameFiles {
    public static class Pack {
        public final String id, title, gives, item, file, sha1; public final long size;
        Pack(String id, String title, String gives, String item, String file, long size, String sha1) {
            this.id = id; this.title = title; this.gives = gives; this.item = item; this.file = file; this.size = size; this.sha1 = sha1;
        }
        public String url() {
            try { return "https://archive.org/download/" + item + "/" + URLEncoder.encode(file, "UTF-8").replace("+", "%20"); } catch (Exception e) { return ""; }
        }
    }
    /* sizes + SHA-1 from archive.org's metadata (archive.org/metadata/<item>/files), checked 2026-10-25 */
    public static final Pack[] PACKS = {
        new Pack("su2000", "Virtuality SU2000 HDD Dump", "Dactyl Nightmare 1 + 2, Zone Hunter, Ghost Train, Virtuality Boxing, Missile Command VR, Olin Shotgun, Shoot For Loot, Sea Diving, Pac-Man VR (+ its CD below)",
                 "su2000-hdd-files", "Virtuality SU2000 HDD Dump.7z", 181544884L, "f038d6241d06e9e2570d485e09c29dc649c6c5cd"),
        new Pack("pacman", "Pac-Man VR with music files", "Pac-Man VR's CD image (PACMAN.CUE + PACMAN.BIN): Pac-Man needs it",
                 "drive-download-20240331T000417Z-001", "drive-download-20240331T000417Z-001.zip", 131982809L, "47e735476fa4ef96d6db20e9bd86370e34c8bb28"),
        new Pack("su3000", "Virtuality SU3000 / 2000E HDD Dump", "Total Recoil, Zero Hour, Zeneca Migraine",
                 "su3000", "su3000.zip", 227862673L, "3e8ab069f32d3d15e5c6e92e873dd383206a4d7a"),
        new Pack("buggy", "Virtuality Buggy Ball (SU2000) - Game Data", "Buggy Ball",
                 "buggy.-7z", "Buggy.7z", 10707383L, "921112674a92997eb0d66c0e1fff359a7560d4f6"),
    };

    public interface Progress { void say(String line); boolean cancelled(); }

    public static File doneMark(File games, Pack p) { return new File(games, ".got_" + p.id); }

    /* download (with resume) + check + unpack + mark done. Throws with a readable message on failure. */
    public static void get(Pack p, File games, File downloads, Progress pr) throws Exception {
        downloads.mkdirs(); games.mkdirs();
        File done = new File(downloads, p.file), part = new File(downloads, p.file + ".part");
        long need = p.size * 4;                                   // archive + unpacked, roughly (the SU2000 dump unpacks to ~530 MB)
        if (!done.isFile() && games.getUsableSpace() < need)
            throw new IOException("not enough free space: " + (need >> 20) + " MB needed, " + (games.getUsableSpace() >> 20) + " MB free");
        if (!done.isFile()) {
            int tries = 0;
            while (true) {
                if (pr.cancelled()) throw new IOException("stopped (press GET again to carry on where it stopped)");
                try { download(p, part, pr); break; }
                catch (IOException e) {
                    if (++tries >= 8 || pr.cancelled()) throw new IOException("download failed after " + tries + " tries: " + e.getMessage() + " (press GET again to resume)");
                    pr.say("Connection problem (" + e.getMessage() + "), trying again in 5 s ...");
                    Thread.sleep(5000);
                }
            }
            if (!part.renameTo(done)) throw new IOException("can't rename " + part);
        }
        pr.say("Checking the download ...");
        String h = sha1(done);
        if (!h.equals(p.sha1)) { done.delete(); throw new IOException("the download is damaged (SHA-1 " + h + "), deleted: press GET to try again"); }
        if ("su2000".equals(p.id)) un7z(done, games, "(Simon's)", pr);   // only Simon's dump, as the PC page says (Faulty's confuses the launcher)
        else if ("buggy".equals(p.id)) un7z(done, games, null, pr);
        else if ("su3000".equals(p.id)) unzip(done, games, false, pr);
        else if ("pacman".equals(p.id)) {
            File inner = new File(downloads, "pacman_inner.zip");
            if (!extractOne(done, "pacman.zip", inner)) unzip(done, games, true, pr);      // no inner zip: CUE/BIN straight from the outer one
            else { unzip(inner, games, true, pr); inner.delete(); }
        }
        new FileOutputStream(doneMark(games, p)).close();
        done.delete();                                             // unpacked: the archive isn't needed any more
        pr.say("Done: " + p.title);
    }

    static void download(Pack p, File part, Progress pr) throws IOException {
        long have = part.isFile() ? part.length() : 0;
        if (have >= p.size) return;
        HttpURLConnection c = (HttpURLConnection) new URL(p.url()).openConnection();
        c.setConnectTimeout(30000); c.setReadTimeout(60000); c.setInstanceFollowRedirects(true);
        c.setRequestProperty("User-Agent", "MallVR-Quest (gainesphere.com)");
        if (have > 0) c.setRequestProperty("Range", "bytes=" + have + "-");
        int code = c.getResponseCode();
        if (code == 416 && have > 0) { c.disconnect(); return; }
        if (code != 200 && code != 206) throw new IOException("HTTP " + code);
        boolean append = code == 206 && have > 0;
        if (!append) have = 0;
        InputStream in = new BufferedInputStream(c.getInputStream(), 1 << 16);
        OutputStream o = new BufferedOutputStream(new FileOutputStream(part, append), 1 << 16);
        byte[] b = new byte[1 << 16]; int n; long t0 = System.currentTimeMillis(), last = 0, start = have;
        try {
            while ((n = in.read(b)) > 0) {
                o.write(b, 0, n); have += n;
                long now = System.currentTimeMillis();
                if (now - last > 500) {
                    last = now; double secs = Math.max(0.001, (now - t0) / 1000.0);
                    pr.say(String.format(java.util.Locale.US, "Downloading %s: %d of %d MB (%.1f MB/s)", p.file, have >> 20, p.size >> 20, (have - start) / secs / 1048576));
                }
                if (pr.cancelled()) throw new IOException("stopped");
            }
        } finally { o.close(); in.close(); c.disconnect(); }
        if (have < p.size) throw new IOException("connection closed at " + (have >> 20) + " MB");
    }

    /* a path inside an archive -> a file under 'to' (never outside it: no "..") */
    static File target(File to, String name) throws IOException {
        File f = new File(to, name);
        if (!f.getCanonicalPath().startsWith(to.getCanonicalPath() + File.separator)) throw new IOException("bad path in archive: " + name);
        return f;
    }

    /* .7z; keep != null: only entries under the folder whose name contains it, written from that folder down */
    static void un7z(File a, File to, String keep, Progress pr) throws IOException {
        try (SevenZFile z = new SevenZFile(a)) {
            SevenZArchiveEntry e; int n = 0; byte[] b = new byte[1 << 16];
            while ((e = z.getNextEntry()) != null) {
                if (pr.cancelled()) throw new IOException("stopped while unpacking (press GET again)");
                String name = e.getName().replace('\\', '/');
                if (keep != null) {
                    int i = name.indexOf(keep); if (i < 0) continue;
                    int s = name.lastIndexOf('/', i); name = name.substring(s + 1);
                }
                File f = target(to, name);
                if (e.isDirectory()) { f.mkdirs(); continue; }
                f.getParentFile().mkdirs();
                try (OutputStream o = new BufferedOutputStream(new FileOutputStream(f), 1 << 16)) { int r; while ((r = z.read(b)) > 0) o.write(b, 0, r); }
                if (e.getHasLastModifiedDate()) f.setLastModified(e.getLastModifiedDate().getTime());
                if (++n % 50 == 0) pr.say("Unpacking " + a.getName() + ": " + n + " files ... " + name);
            }
            pr.say("Unpacked " + n + " files from " + a.getName());
        }
    }

    static ZipFile openZip(File a) throws IOException {
        try { return new ZipFile(a); } catch (IOException e) { return new ZipFile(a, Charset.forName("Cp437")); }
    }

    /* .zip into 'to'; cdOnly: just the *.CUE / *.BIN files, flat (Pac-Man's disc image goes next to the other files in games) */
    static void unzip(File a, File to, boolean cdOnly, Progress pr) throws IOException {
        try (ZipFile z = openZip(a)) {
            int n = 0;
            for (Enumeration<? extends ZipEntry> en = z.entries(); en.hasMoreElements(); ) {
                ZipEntry e = en.nextElement();
                if (pr.cancelled()) throw new IOException("stopped while unpacking (press GET again)");
                String name = e.getName().replace('\\', '/');
                if (cdOnly) {
                    String u = name.toUpperCase(java.util.Locale.US);
                    if (e.isDirectory() || !(u.endsWith(".CUE") || u.endsWith(".BIN"))) continue;
                    name = name.substring(name.lastIndexOf('/') + 1);
                }
                File f = target(to, name);
                if (e.isDirectory()) { f.mkdirs(); continue; }
                f.getParentFile().mkdirs();
                try (InputStream in = z.getInputStream(e); OutputStream o = new BufferedOutputStream(new FileOutputStream(f), 1 << 16)) { MallVRCopy.copy(in, o); }
                if (e.getTime() > 0) f.setLastModified(e.getTime());
                if (++n % 50 == 0 || cdOnly) pr.say("Unpacking " + a.getName() + ": " + n + " files ... " + name);
            }
            pr.say("Unpacked " + n + " files from " + a.getName());
        }
    }

    /* the first entry whose name ends with 'tail' (any case) -> out; false if there is none */
    static boolean extractOne(File a, String tail, File out) throws IOException {
        try (ZipFile z = openZip(a)) {
            for (Enumeration<? extends ZipEntry> en = z.entries(); en.hasMoreElements(); ) {
                ZipEntry e = en.nextElement();
                if (e.isDirectory() || !e.getName().toLowerCase(java.util.Locale.US).endsWith(tail)) continue;
                try (InputStream in = z.getInputStream(e); OutputStream o = new FileOutputStream(out)) { MallVRCopy.copy(in, o); }
                return true;
            }
        }
        return false;
    }

    public static String sha1(File f) throws IOException {
        try {
            MessageDigest md = MessageDigest.getInstance("SHA-1");
            try (InputStream in = new FileInputStream(f)) { byte[] b = new byte[1 << 16]; int n; while ((n = in.read(b)) > 0) md.update(b, 0, n); }
            StringBuilder sb = new StringBuilder(); for (byte x : md.digest()) sb.append(String.format("%02x", x));
            return sb.toString();
        } catch (java.security.NoSuchAlgorithmException e) { throw new IOException(e); }
    }

    /* PC test: java -cp <jars>;classes com.gainesphere.mallvr.GameFiles <pack id> <games dir> <download dir> */
    public static void main(String[] a) throws Exception {
        for (Pack p : PACKS) if (p.id.equals(a[0]))
            get(p, new File(a[1]), new File(a[2]), new Progress() { public void say(String s) { System.out.println(s); } public boolean cancelled() { return false; } });
    }
}

/* copy helper without Android classes (MallVR.java uses android.*, so GameFiles can't call it on the PC) */
class MallVRCopy {
    static void copy(InputStream in, OutputStream o) throws IOException { byte[] b = new byte[1 << 16]; int n; while ((n = in.read(b)) > 0) o.write(b, 0, n); }
    static void write(File f, String s) throws IOException { try (OutputStream o = new FileOutputStream(f)) { o.write(s.getBytes("UTF-8")); } }
}
