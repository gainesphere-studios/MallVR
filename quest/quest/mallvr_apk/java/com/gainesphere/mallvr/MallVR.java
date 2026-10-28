package com.gainesphere.mallvr;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Build;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.*;
import java.security.MessageDigest;
import java.text.SimpleDateFormat;
import java.util.*;

/* MallVR for Meta Quest: the shared part of the launcher (plan sections 4 + 5).
   - /sdcard/MallVR = the PC package's layout: games/ (the player's dumps, same folders as on PC), engine/ (hook tables, copied from
     the APK), logs/, settings/.
   - games.json (assets, made by quest/make_games_json.ps1 from games.ps1 + play.ps1) = every game's settings and VPIX_* env.
   - find(): the same checks as the PC's CHECK GAMES (EXE in a folder of its own name, Sha1; Pac-Man's CUE/BIN; Buggy Ball's data).
   - plan(): play.ps1 for the headset: the DOSBox-X conf + the env for one game, solo or as pod <n> of an online group (crossplay:
     the env play.ps1 -Pod n -Peers 127.0.0.1:<27110 + n> gives a PC pod, the same per-pod game settings files in an overlay). */
public class MallVR {
    public static final File ROOT = new File("/sdcard/MallVR");
    public static final File GAMES = new File(ROOT, "games"), ENGINE = new File(ROOT, "engine"), LOGS = new File(ROOT, "logs"), SETTINGS = new File(ROOT, "settings");

    public static String version(Context c) {
        try { return c.getPackageManager().getPackageInfo(c.getPackageName(), 0).versionName; } catch (Exception e) { return "?"; }
    }

    /* the launcher's own log (logs/quest_launcher.log): permission, downloads, starts, errors. Goes into Report a problem. */
    public static synchronized void log(String s) {
        android.util.Log.i("MallVR", s);
        try {
            LOGS.mkdirs();
            File f = new File(LOGS, "quest_launcher.log");
            if (f.length() > 512 * 1024) { File o = new File(LOGS, "quest_launcher.old.log"); o.delete(); f.renameTo(o); }
            FileWriter w = new FileWriter(f, true);
            w.write(new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US).format(new Date()) + "  " + s + "\n");
            w.close();
        } catch (Exception e) { }
    }

    // ---------------------------------------------------------------- games.json
    public static class Game {
        public String code, name, exe, args, sha1, cd, cdBin, nativ, dataDir, dataFile, cdAudio;
        public boolean su3k, mpeg;
        public Map<String, String> env = new LinkedHashMap<>();
        public boolean link;         // linked / online play works (games.ps1 Link)
        public int maxPlayers = 2;
        public Map<Integer, Map<String, String>> pods = new HashMap<>();   // pod -> env (games.json "pods")
        public String[] podArgs;     // launch line per pod (Ghost Train)
        // found by find()
        public int state;            // 0 missing, 1 READY, 2 different version (may work), 3 needs its CD image
        public File exeFile, dataPath, cue, cdAudioDir;
        public String why = "";
        public String stateText() { return state == 1 ? "READY" : state == 2 ? "READY? (different version)" : state == 3 ? "NEEDS CD IMAGE" : "NOT FOUND"; }
    }

    public static List<Game> load(Context c) throws Exception {
        String s = readAll(c.getAssets().open("games.json"));
        JSONArray a = new JSONObject(s).getJSONArray("games");
        List<Game> out = new ArrayList<>();
        for (int i = 0; i < a.length(); i++) {
            JSONObject o = a.getJSONObject(i); Game g = new Game();
            g.code = o.getString("code"); g.name = o.getString("name"); g.exe = o.optString("exe", g.code); g.args = o.optString("args", "");
            g.sha1 = o.optString("sha1", ""); g.cd = o.optString("cd", null); g.cdBin = o.optString("cdBin", null);
            g.nativ = o.optString("native", null); g.dataDir = o.optString("dataDir", null); g.dataFile = o.optString("dataFile", null);
            g.cdAudio = o.optString("cdAudio", null); g.su3k = o.optBoolean("su3k", false); g.mpeg = o.optBoolean("mpeg", false);
            JSONObject e = o.getJSONObject("env");
            for (Iterator<String> k = e.keys(); k.hasNext(); ) { String n = k.next(); g.env.put(n, e.getString(n)); }
            g.link = o.optBoolean("link", false); g.maxPlayers = o.optInt("maxPlayers", 2);
            JSONObject pods = o.optJSONObject("pods");
            if (pods != null) for (Iterator<String> k = pods.keys(); k.hasNext(); ) {
                String n = k.next(); JSONObject pe = pods.getJSONObject(n); Map<String, String> m = new LinkedHashMap<>();
                for (Iterator<String> j = pe.keys(); j.hasNext(); ) { String v = j.next(); m.put(v, pe.getString(v)); }
                g.pods.put(Integer.parseInt(n), m);
            }
            JSONArray pa = o.optJSONArray("podArgs");
            if (pa != null) { g.podArgs = new String[pa.length()]; for (int j = 0; j < pa.length(); j++) g.podArgs[j] = pa.getString(j); }
            out.add(g);
        }
        return out;
    }

    // ---------------------------------------------------------------- finding the player's files
    static final Map<String, String> sha1Cache = new HashMap<>();   // path|size|mtime -> sha1

    public static String sha1(File f) {
        String key = f.getAbsolutePath() + "|" + f.length() + "|" + f.lastModified();
        synchronized (sha1Cache) { String h = sha1Cache.get(key); if (h != null) return h; }
        try {
            MessageDigest md = MessageDigest.getInstance("SHA-1");
            InputStream in = new FileInputStream(f); byte[] b = new byte[1 << 16]; int n;
            while ((n = in.read(b)) > 0) md.update(b, 0, n);
            in.close();
            StringBuilder sb = new StringBuilder(); for (byte x : md.digest()) sb.append(String.format("%02x", x));
            synchronized (sha1Cache) { sha1Cache.put(key, sb.toString()); }
            return sb.toString();
        } catch (Exception e) { return ""; }
    }

    /* every file and folder under games/ (6 levels), by upper-case name */
    static void walk(File d, int depth, Map<String, List<File>> files, Map<String, List<File>> dirs) {
        File[] l = d.listFiles(); if (l == null) return;
        for (File f : l) {
            String n = f.getName().toUpperCase(Locale.US);
            Map<String, List<File>> m = f.isDirectory() ? dirs : files;
            List<File> x = m.get(n); if (x == null) m.put(n, x = new ArrayList<>()); x.add(f);
            if (f.isDirectory() && depth < 6) walk(f, depth + 1, files, dirs);
        }
    }

    /* a DOS-style relative path ("RUNFILES\CARS.DBS") under dir, any letter case */
    public static File resolve(File dir, String rel) {
        File cur = dir;
        for (String part : rel.split("[\\\\/]")) {
            if (part.isEmpty()) continue;
            File[] l = cur.listFiles(); File hit = null;
            if (l != null) for (File f : l) if (f.getName().equalsIgnoreCase(part)) { hit = f; break; }
            if (hit == null) return null;
            cur = hit;
        }
        return cur;
    }

    public static void find(List<Game> games) {
        Map<String, List<File>> files = new HashMap<>(), dirs = new HashMap<>();
        walk(GAMES, 0, files, dirs);
        Set<String> known = new HashSet<>(); for (Game g : games) if (!g.sha1.isEmpty()) known.add(g.sha1);
        for (Game g : games) {
            g.state = 0; g.exeFile = g.dataPath = g.cue = g.cdAudioDir = null; g.why = "";
            if (g.nativ != null) {
                List<File> ds = dirs.get(g.dataDir.toUpperCase(Locale.US));
                if (ds != null) for (File d : ds) if (resolve(d, g.dataFile) != null) { g.dataPath = d; g.state = 1; break; }
                if (g.state == 0) g.why = "needs its " + g.dataDir + " folder (with " + g.dataFile + ")";
                continue;
            }
            List<File> cands = files.get(g.exe.toUpperCase(Locale.US) + ".EXE");
            File other = null;
            if (cands != null) for (File f : cands) {
                if (!f.getParentFile().getName().equalsIgnoreCase(g.exe)) continue;
                String h = sha1(f);
                if (h.equals(g.sha1)) { g.exeFile = f; g.state = 1; break; }
                if (!known.contains(h) && other == null) other = f;      // not some other game's exact EXE (GUN = Olin Shotgun / Total Recoil)
            }
            if (g.exeFile == null && other != null) { g.exeFile = other; g.state = 2; g.why = "a different " + g.exe + ".EXE than MallVR was tested with"; }
            if (g.exeFile == null) { g.why = "needs the " + (g.su3k ? "SU3000" : "SU2000") + " HDD dump (" + g.exe + "\\" + g.exe + ".EXE)"; continue; }
            if (g.cd != null) {
                List<File> cs = files.get(g.cd.toUpperCase(Locale.US));
                if (cs != null) for (File c : cs) if (resolve(c.getParentFile(), g.cdBin) != null) { g.cue = c; break; }
                if (g.cue == null) { g.state = 3; g.why = "needs its CD image: " + g.cd + " + " + g.cdBin; }
            }
            if (g.cdAudio != null) {
                List<File> ds = dirs.get(g.cdAudio.toUpperCase(Locale.US));
                if (ds != null) for (File d : ds) { String[] t = d.list(); if (t != null) for (String n : t) if (n.toUpperCase(Locale.US).startsWith("TRACK")) { g.cdAudioDir = d; break; } if (g.cdAudioDir != null) break; }
            }
        }
    }

    // ---------------------------------------------------------------- play.ps1 for the headset
    public static class Plan { public String conf, tag; public HashMap<String, String> env = new HashMap<>(); }

    public static Plan plan(Context c, Game g) throws IOException { return plan(c, g, 0); }
    public static Plan plan(Context c, Game g, int pod) throws IOException {
        if (pod > 0 && !g.pods.containsKey(pod)) throw new IOException(g.name + " has no online pod " + pod);
        Plan p = new Plan(); p.tag = g.code.toLowerCase(Locale.US) + (pod > 0 ? "_pod" + pod : "");
        LOGS.mkdirs(); SETTINGS.mkdirs();
        File[] old = LOGS.listFiles();   // this game's logs from last time (not its conf)
        if (old != null) for (File f : old) if (f.getName().startsWith(p.tag + ".") && !f.getName().endsWith(".conf")) f.delete();
        File gameDir = g.exeFile != null ? g.exeFile.getParentFile() : null;

        String sig = "";      // disc-check signature: CODE and NAME lines of the game's .INF
        if (gameDir != null) {
            File inf = resolve(gameDir, g.code + ".INF");
            if (inf != null) {
                String cc = "", nn = "";
                for (String line : readAll(new FileInputStream(inf)).split("\r?\n")) {
                    String[] t = line.trim().split("\\s+", 2);
                    if (t.length < 2) continue;
                    if (t[0].equalsIgnoreCase("CODE")) cc = t[1]; else if (t[0].equalsIgnoreCase("NAME")) nn = t[1];
                }
                sig = cc + "|" + nn;
            }
        }
        String ovLine = "";
        if (pod > 0 && gameDir != null) ovLine = "mount -t overlay c \"" + podOverlay(g, gameDir, pod).getPath() + "\"\n";
        for (Map.Entry<String, String> e : (pod > 0 ? g.pods.get(pod) : g.env).entrySet()) {
            String v = e.getValue().replace("@CDSIG@", sig);
            if (v.startsWith("@")) {
                v = v.replace("@ENGINE@", ENGINE.getPath()).replace("@LOGS@", LOGS.getPath()).replace("@SETTINGS@", SETTINGS.getPath())
                     .replace("@ROOT@", ROOT.getPath()).replace("@GAMEDIR@", gameDir != null ? gameDir.getPath() : "")
                     .replace("@DATADIR@", g.dataPath != null ? g.dataPath.getPath() : "").replace('\\', '/');
            }
            p.env.put(e.getKey(), v);
        }
        if (g.cdAudioDir != null) p.env.put("VPIX_CD_DIR", g.cdAudioDir.getPath());
        p.env.put("VPIX_XR", "1");
        p.env.put("MALLVR_DIR", ROOT.getPath());
        p.env.put("MALLVR_PLATFORM", "quest");
        p.env.put("HOME", c.getFilesDir().getAbsolutePath());

        String extra = "";    // settings/quest_extra.conf: tester switches (e.g. [cpu] core=dynamic) without a new APK
        File ex = new File(SETTINGS, "quest_extra.conf");
        if (ex.isFile()) extra = readAll(new FileInputStream(ex)).trim() + "\n";
        StringBuilder s = new StringBuilder();
        if (g.nativ != null) {
            s.append("[sdl]\noutput=surface\n[dosbox]\nmachine=svga_s3\nmemsize=16\n[cpu]\ncycles=3000\n").append(extra).append("[autoexec]\n");
        } else {
            File driveC = gameDir.getParentFile();          // the dump root: holds DAC/, PAC/, DOS/ ...
            String machine = g.su3k
                ? "memsize=64\n[cpu]\ncore=normal\ncputype=pentium\ncycles=max\n[sblaster]\nsbtype=sb16\nsbbase=220\nirq=5\ndma=1\nhdma=5\n"
                : "memsize=32\n[cpu]\ncore=normal\ncputype=486\ncycles=max\n";
            File empty = new File(ENGINE, "emptycd"); empty.mkdirs();
            String cdLine = g.cue != null ? "imgmount f \"" + g.cue.getPath() + "\" -t cdrom" : "mount f \"" + empty.getPath() + "\" -t cdrom";
            String exeDir = g.exe.toLowerCase(Locale.US);
            s.append("[sdl]\noutput=surface\n[dosbox]\nmachine=svga_s3\n").append(machine)
             .append("[dos]\nems=false\numb=false\nxms=true\n").append(extra).append("[autoexec]\n")
             .append("mount c \"").append(driveC.getPath()).append("\"\n").append(ovLine).append(cdLine).append("\nc:\nset dos4g=quiet\nset vpctemp=c:\\vpctemp\n")
             .append(g.su3k ? "set BLASTER=A220 I5 D1 H5 P330 T6\nset SOUND=C:\\SB16\n" : "")
             .append("path c:\\dos;c:\\utils\ncd \\").append(exeDir).append("\n").append(exeDir).append(" ")
             .append(pod > 0 && g.podArgs != null && pod <= g.podArgs.length ? g.podArgs[pod - 1] : g.args)
             .append(" > c:\\gameout.txt\nexit\n");
        }
        File conf = new File(LOGS, p.tag + ".conf");
        FileOutputStream o = new FileOutputStream(conf); o.write(s.toString().getBytes("US-ASCII")); o.close();
        p.conf = conf.getPath();
        return p;
    }

    /* play.ps1's linked-pod files: each pod gets its own writable layer over C: (settings/pod<n>) holding the game's settings file with
       network on and master (pod 1) / slave, so the real game files stay as they are. Same lines play.ps1 writes. */
    static File podOverlay(Game g, File gameDir, int pod) throws IOException {
        File ov = new File(SETTINGS, "pod" + pod); ov.mkdirs();
        String dir = gameDir.getName();
        boolean master = pod == 1;
        if (g.code.equals("DAC")) podLines(gameDir, ov, dir, "RUNFILES/DEFAULTS.DN2", 6, l -> { l.set(4, "1 "); l.set(5, master ? "1 " : "0 "); });
        else if (g.code.equals("DN2")) podLines(gameDir, ov, dir, "RUNFILES/DEFAULTS.DN2", 4, l -> { l.set(2, "1 "); l.set(3, master ? "1 " : "0 "); });
        else if (g.code.equals("GHOST")) podLines(gameDir, ov, dir, "RUNFILES/LASTSETS.DBS", 0, l -> { while (l.size() < 3) l.add("0"); l.set(1, "1"); });
        else if (g.code.equals("PEARL")) podLines(gameDir, ov, dir, "RUNFILES/SETTINGS.PRL", 3, l -> { l.set(1, "1 "); l.set(2, master ? "1 " : "0 "); });
        else if (g.code.equals("SFL")) {            // CONFIG.BIN = 5 x int32, the 4th = network 0 none / 1 master / 2 slave
            File out = new File(ov, dir + "/CONFIG.BIN");
            File src = out.isFile() ? out : resolve(gameDir, "CONFIG.BIN");
            if (src == null) throw new IOException("Shoot For Loot's CONFIG.BIN is missing");
            byte[] b = readBytes(src);
            if (b.length < 20) throw new IOException("Shoot For Loot's CONFIG.BIN looks damaged");
            int v = master ? 1 : 2; b[12] = (byte) v; b[13] = 0; b[14] = 0; b[15] = 0;
            out.getParentFile().mkdirs(); FileOutputStream o = new FileOutputStream(out); o.write(b); o.close();
        }
        return ov;
    }
    interface LineEdit { void edit(List<String> lines); }
    static void podLines(File gameDir, File ov, String dir, String rel, int need, LineEdit ed) throws IOException {
        File out = new File(ov, dir + "/" + rel);
        File src = out.isFile() ? out : resolve(gameDir, rel);
        if (src == null) throw new IOException("the game's " + rel + " is missing");
        List<String> l = new ArrayList<>(Arrays.asList(readAll(new FileInputStream(src)).split("\r?\n", -1)));
        while (!l.isEmpty() && l.get(l.size() - 1).isEmpty()) l.remove(l.size() - 1);
        if (l.size() < need) throw new IOException("the game's " + rel + " looks damaged");
        ed.edit(l);
        out.getParentFile().mkdirs();
        FileOutputStream o = new FileOutputStream(out); o.write((String.join("\r\n", l) + "\r\n").getBytes("US-ASCII")); o.close();
    }
    static byte[] readBytes(File f) throws IOException { ByteArrayOutputStream b = new ByteArrayOutputStream(); InputStream in = new FileInputStream(f); copy(in, b); in.close(); return b.toByteArray(); }

    // ---------------------------------------------------------------- first start: hook tables etc. from the APK to /sdcard/MallVR/engine
    public static void installEngine(Context c) {
        try {
            String stamp = version(c) + "/" + c.getPackageManager().getPackageInfo(c.getPackageName(), 0).lastUpdateTime;
            File mark = new File(ENGINE, "installed.txt");
            for (File d : new File[] { GAMES, ENGINE, LOGS, SETTINGS, new File(ENGINE, "emptycd"), new File(ROOT, "music/buggy") }) d.mkdirs();
            if (mark.isFile() && readAll(new FileInputStream(mark)).trim().equals(stamp)) return;
            copyAssets(c.getAssets(), "engine", ENGINE);
            write(new File(ENGINE, "emptycd/README.txt"), "Empty on purpose: mounted as the CD-ROM drive for games without a disc image.\n");
            write(new File(GAMES, "PUT_GAMES_HERE.txt"), "Your Virtuality game files go in this folder, the same as the PC version's games folder:\n"
                + "  VR 2000 Series Disc Image (Simon's)\\DAC\\DAC.EXE ... (SU2000 HDD dump)\n  PACMAN.CUE + PACMAN.BIN (Pac-Man VR's CD image)\n"
                + "  su3000\\... (SU3000 HDD dump)   BALL\\RUNFILES\\CARS.DBS (Buggy Ball)\nOr press GET GAMES in the MallVR launcher.\n");
            write(mark, stamp + "\n");
            log("engine files installed (" + stamp + ")");
        } catch (Exception e) { log("engine install failed: " + e); }
    }
    static void copyAssets(AssetManager am, String path, File to) throws IOException {
        String[] l = am.list(path);
        if (l == null || l.length == 0) {     // a file
            to.getParentFile().mkdirs();
            InputStream in = am.open(path); OutputStream o = new FileOutputStream(to); copy(in, o); in.close(); o.close();
            return;
        }
        to.mkdirs();
        for (String n : l) copyAssets(am, path + "/" + n, new File(to, n));
    }

    /* this app's recent crash lines in logcat (an app may read its own): native "Fatal signal" + backtrace, Java FATAL EXCEPTION */
    public static String crashLines() {
        try {
            Process p = Runtime.getRuntime().exec(new String[] { "logcat", "-d", "-v", "time", "-t", "5000" });
            BufferedReader r = new BufferedReader(new InputStreamReader(p.getInputStream()));
            LinkedList<String> keep = new LinkedList<>(); String l;
            while ((l = r.readLine()) != null)
                if (l.contains("Fatal signal") || l.contains("backtrace") || l.matches(".*\\s#\\d\\d pc .*") || l.contains("FATAL EXCEPTION") || l.contains("AndroidRuntime")
                    || l.contains("/MallVRxr") || l.contains("Abort message")) { keep.add(l); if (keep.size() > 80) keep.removeFirst(); }
            p.destroy();
            return String.join("\n", keep);
        } catch (Exception e) { return ""; }
    }

    // ---------------------------------------------------------------- small helpers
    public static void copy(InputStream in, OutputStream o) throws IOException { byte[] b = new byte[1 << 16]; int n; while ((n = in.read(b)) > 0) o.write(b, 0, n); }
    public static String readAll(InputStream in) throws IOException {
        ByteArrayOutputStream b = new ByteArrayOutputStream(); copy(in, b); in.close(); return b.toString("UTF-8");
    }
    public static void write(File f, String s) throws IOException { f.getParentFile().mkdirs(); FileOutputStream o = new FileOutputStream(f); o.write(s.getBytes("UTF-8")); o.close(); }
    public static String device() {
        return Build.MANUFACTURER + " " + Build.MODEL + " (" + Build.PRODUCT + "/" + Build.DEVICE + "), Android " + Build.VERSION.RELEASE
            + " (API " + Build.VERSION.SDK_INT + "), build " + Build.DISPLAY;
    }
}
