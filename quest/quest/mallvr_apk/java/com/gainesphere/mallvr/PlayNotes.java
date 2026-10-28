package com.gainesphere.mallvr;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.SecureRandom;
import java.util.*;

/* MallVR play notes on the Quest (opt-in statistics): the PC's MallNotes.cs, same protocol and same stats.php
   (online/STATS_AND_REPORTS_DESIGN.md). Asked once in the launcher; OFF until the player says yes; NO THANKS = nothing is sent
   (REPORT A PROBLEM still works, only when pressed).
   - The choice + a random stats id (32 hex, not tied to anything else) live in MallVR/settings/play_notes.txt, so they survive a
     reinstall like the PC's %APPDATA% file. Queue: MallVR/settings/notes_queue.txt (unsent lines wait for the next send).
   - Records: "t=<unix> TAB ev=<event> TAB key=value ..." lines, POSTed as "MALLVR-NOTES 1 TAB <id> TAB <version>" + the lines.
   - Game state between the game process and the launcher: logs/quest_game_state.txt (MallVRActivity writes start / how it ended;
     the launcher turns it into game_end + crash records when it comes back). No crash = no "end" line.
   Plain Java (no Android classes): PC test = java ... PlayNotes <dir> <url>. */
public class PlayNotes {
    public static final int ASK_VERSION = 1;          // bump when play notes collect something new: everyone is asked again
    public static String url = "https://downloads.gainesphere.com/mallvr/online/stats.php";
    static File dir;                                   // MallVR/settings
    static String consent = "", id = "", version = "";
    static int asked = 0;
    static final List<String> queue = new ArrayList<>();
    static boolean loaded;

    static File choiceFile() { return new File(dir, "play_notes.txt"); }
    static File queueFile() { return new File(dir, "notes_queue.txt"); }
    public static synchronized boolean on() { return "yes".equals(consent) && asked >= ASK_VERSION; }
    public static synchronized boolean needsAsk() { return asked < ASK_VERSION; }
    public static synchronized String statsId() { return id; }

    public static synchronized void load(File settings, String ver) {
        dir = settings; version = ver;
        if (loaded) return;
        loaded = true;
        try (BufferedReader r = new BufferedReader(new FileReader(choiceFile()))) {
            String l;
            while ((l = r.readLine()) != null) {
                int e = l.indexOf('='); if (e < 0) continue;
                String k = l.substring(0, e).trim(), v = l.substring(e + 1).trim();
                if (k.equals("stats")) consent = v.equals("yes") ? "yes" : "no";
                else if (k.equals("asked")) try { asked = Integer.parseInt(v); } catch (NumberFormatException x) { }
                else if (k.equals("id") && v.matches("[0-9a-f]{32}")) id = v;
            }
        } catch (IOException e) { }
        if (id.isEmpty()) { byte[] b = new byte[16]; new SecureRandom().nextBytes(b); StringBuilder s = new StringBuilder(); for (byte x : b) s.append(String.format("%02x", x)); id = s.toString(); }
        try (BufferedReader r = new BufferedReader(new FileReader(queueFile()))) { String l; while ((l = r.readLine()) != null) if (!l.trim().isEmpty()) queue.add(l); }
        catch (IOException e) { }
        if (!on()) queue.clear();
    }

    public static synchronized void setConsent(boolean yes) {
        consent = yes ? "yes" : "no"; asked = ASK_VERSION;
        try { dir.mkdirs(); MallVRCopy.write(choiceFile(), "stats = " + consent + "\nasked = " + asked + "\nid = " + id + "\n"); } catch (IOException e) { }
        if (!yes) { queue.clear(); saveQueue(); }
    }

    static synchronized void saveQueue() {
        try { dir.mkdirs(); StringBuilder s = new StringBuilder(); for (String l : queue) s.append(l).append('\n'); MallVRCopy.write(queueFile(), s.toString()); } catch (IOException e) { }
    }

    /* one record: add("game_end", "game", "DAC", "secs", "312", ...). Nothing happens unless play notes are ON. */
    public static synchronized void add(String ev, String... kv) {
        if (!on()) return;
        StringBuilder b = new StringBuilder();
        b.append("t=").append(System.currentTimeMillis() / 1000).append("\tev=").append(field(ev, 24));
        for (int i = 0; i + 1 < kv.length; i += 2) b.append('\t').append(field(kv[i], 24)).append('=').append(field(kv[i + 1], 20000));
        queue.add(b.toString());
        while (queue.size() > 500) queue.remove(0);
        saveQueue();
    }

    static String field(String s, int max) {
        s = (s == null ? "" : s).replace("\\", "\\\\").replace("\t", " ").replace("\r", "").replace("\n", "\\n").replaceAll("[^\\x20-\\x7E]", "?");
        return s.length() > max ? s.substring(0, max) : s;
    }

    /* send what's queued (call from a background thread). Failed = kept for next time. Returns the server's answer. */
    public static String flush() {
        List<String> lines;
        synchronized (PlayNotes.class) { if (!on() || queue.isEmpty()) return ""; lines = new ArrayList<>(queue); }
        try {
            StringBuilder body = new StringBuilder("MALLVR-NOTES 1\t" + id + "\t" + version + "\n");
            for (String l : lines) body.append(l).append('\n');
            byte[] data = body.toString().getBytes("UTF-8");
            HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
            c.setConnectTimeout(15000); c.setReadTimeout(15000); c.setDoOutput(true); c.setRequestMethod("POST");
            c.setRequestProperty("Content-Type", "text/plain; charset=utf-8"); c.setRequestProperty("User-Agent", "MallVR/" + version);
            c.setFixedLengthStreamingMode(data.length);
            try (OutputStream o = c.getOutputStream()) { o.write(data); }
            String r;
            try (InputStream in = c.getResponseCode() < 400 ? c.getInputStream() : c.getErrorStream()) {
                ByteArrayOutputStream b = new ByteArrayOutputStream(); if (in != null) MallVRCopy.copy(in, b); r = b.toString("UTF-8").trim();
            }
            if (r.startsWith("OK")) synchronized (PlayNotes.class) { queue.removeAll(lines); saveQueue(); }
            return r;
        } catch (Exception e) { return "ERR " + e.getMessage(); }
    }

    // ---------------------------------------------------------------- game state file (game process -> launcher)
    public static void gameState(File logs, String line) {
        try (FileWriter w = new FileWriter(new File(logs, "quest_game_state.txt"), !line.startsWith("start "))) { w.write(line + " " + System.currentTimeMillis() + "\n"); }
        catch (IOException e) { }
    }

    /* the launcher is back: turn the last game's state file into a game_end (+ crash) record. logcatTail = this app's recent
       crash lines (Android side), may be "". Deletes the state file either way, so each game is counted once. */
    public static void collectGame(File logs, String logcatTail, boolean gameAlive) { collectGame(logs, logcatTail, gameAlive, "solo"); }
    /* how = "online" from the ONLINE screen (the start line carries the pod) */
    public static void collectGame(File logs, String logcatTail, boolean gameAlive, String kind) {
        File f = new File(logs, "quest_game_state.txt");
        if (!f.isFile()) return;
        String code = "", tag = "", how = "", pod = "0"; long t0 = 0, t1 = 0;
        try (BufferedReader r = new BufferedReader(new FileReader(f))) {
            String l;
            while ((l = r.readLine()) != null) {
                String[] p = l.trim().split(" ");
                if (p[0].equals("start") && p.length >= 4) { code = p[1]; tag = p[2]; t0 = Long.parseLong(p[p.length - 1]); if (p.length >= 5) pod = p[3]; }
                else if (p[0].equals("end") && p.length >= 3 && how.isEmpty()) { how = p[1]; t1 = Long.parseLong(p[p.length - 1]); }
            }
        } catch (Exception e) { }
        if (how.isEmpty() && gameAlive) return;        // still running (no end line yet): not a crash, look again next time
        f.delete();
        if (code.isEmpty()) return;
        File render = new File(logs, tag + ".render");
        if (t1 == 0) t1 = render.isFile() ? render.lastModified() : t0;      // no end line: it died; the log's last write is about when
        boolean crash = how.isEmpty();
        String[] perf = perf(render), xr = xr(render);
        add("game_end", "game", code, "how", pod.equals("0") ? "solo" : kind, "pod", pod, "secs", "" + Math.max(0, (t1 - t0) / 1000), "ended", crash ? "crashed" : how,
            "fps", perf[0], "worst_ms", perf[1], "mode", "headset", "xr", xr[0], "eye", xr[1], "runtime", xr[2], "space", xr[3], "hands", xr[4]);
        if (crash) add("crash", "game", code, "exit", "died", "log", tail(render, 200), "logcat", logcatTail);
        if (!pod.equals("0")) {     // online: the game's own network numbers (vpix_net.cpp "stats:" line, mic opened or not)
            String[] ns = netStats(new File(logs, tag + ".net"));
            String[] kv = new String[ns.length + 4]; kv[0] = "game"; kv[1] = code; kv[2] = "pod"; kv[3] = pod;
            System.arraycopy(ns, 0, kv, 4, ns.length);
            add("net", kv);
        }
    }

    /* the engine's <tag>.net log: the last "stats:" line + whether the mic opened (MallNotes.cs NetStats) */
    static String[] netStats(File net) {
        String tx = "", rx = "", drop = "", foreign = "", vtx = "", vrx = "", mic = "off";
        for (String l : lines(net, 256 * 1024)) {
            if (l.startsWith("stats: ")) {
                String[] w = l.substring(7).split(" ");
                for (int i = 0; i + 1 < w.length; i += 2) {
                    String v = w[i + 1];
                    switch (w[i]) { case "tx": tx = v; break; case "rx": rx = v; break; case "drop": drop = v; break;
                        case "foreign": foreign = v; break; case "vtx": vtx = v; break; case "vrx": vrx = v; break; }
                }
            } else if (l.contains("VOICE: microphone open")) mic = "open";
            else if (l.contains("VOICE: no microphone")) mic = "none";
        }
        return new String[] { "net_tx", tx, "net_rx", rx, "net_drop", drop, "foreign", foreign, "voice_tx", vtx, "voice_rx", vrx, "mic", mic };
    }

    /* render log "perf: frames N secs S worst W" lines (vpix_menu.h, every 10 s in the headset): average fps + worst frame ms */
    static String[] perf(File render) {
        double frames = 0, secs = 0, w = 0;
        for (String l : lines(render, 4 << 20)) {
            if (!l.startsWith("perf: ")) continue;
            String[] f = l.split(" ");
            for (int i = 1; i + 1 < f.length; i++) {
                double v; try { v = Double.parseDouble(f[i + 1]); } catch (NumberFormatException e) { continue; }
                if (f[i].equals("frames")) frames += v; else if (f[i].equals("secs")) secs += v; else if (f[i].equals("worst")) w = Math.max(w, v);
            }
        }
        return secs > 0 ? new String[] { String.format(Locale.US, "%.1f", frames / secs), String.format(Locale.US, "%.0f", w) } : new String[] { "", "" };
    }

    /* did OpenXR come up? (vpix_xr.h log lines) -> { up / failed / none, eye size, runtime, stage / local, controllers seen } */
    static String[] xr(File render) {
        String up = "none", eye = "", rt = "", space = "stage", hands = "";
        for (String l : lines(render, 4 << 20)) {
            if (l.startsWith("XR up: eye ")) { up = "up"; eye = l.substring(11).split(",")[0]; }
            else if (l.startsWith("XR runtime: ")) rt = l.substring(12);
            else if (l.startsWith("XR: no STAGE space")) space = "local";
            else if (l.contains("hand controller: ")) hands = (hands.isEmpty() ? "" : hands + "; ") + l.substring(3);
            else if (!up.equals("up") && l.startsWith("XR") && (l.contains("failed") || l.contains("bad loader"))) up = "failed: " + l;
        }
        return new String[] { up, eye, rt, space, hands.length() > 300 ? hands.substring(0, 300) : hands };
    }

    static String tail(File f, int n) {
        List<String> keep = new ArrayList<>();
        for (String l : lines(f, 512 * 1024)) if (!l.isEmpty() && !l.startsWith("texslot ") && !l.startsWith("texture ")) keep.add(l);
        return String.join("\n", keep.subList(Math.max(0, keep.size() - n), keep.size()));
    }

    static List<String> lines(File f, long maxTail) {
        List<String> out = new ArrayList<>();
        if (f == null || !f.isFile()) return out;
        try (RandomAccessFile r = new RandomAccessFile(f, "r")) {
            long n = Math.min(r.length(), maxTail); byte[] b = new byte[(int) n]; r.seek(r.length() - n); r.readFully(b);
            for (String l : new String(b, "ISO-8859-1").replace("\r", "").split("\n")) out.add(l);
        } catch (IOException e) { }
        return out;
    }

    /* PC test: java ... PlayNotes <settings+logs dir> <stats url> */
    public static void main(String[] a) throws Exception {
        File d = new File(a[0]); url = a[1];
        load(d, "0.1-quest"); setConsent(true);
        add("launcher_start", "ver", "0.1-quest", "plat", "QUEST", "headset", "Oculus Quest 3 (eureka)", "have", "DAC PAC");
        gameState(d, "start DAC dac"); gameState(d, "end back_to_menu");
        collectGame(d, "", false);
        gameState(d, "start PAC pac");                                     // no end line = crashed
        collectGame(d, "", true);                                       // alive, no end: must keep the file
        System.out.println("kept while alive: " + new File(d, "quest_game_state.txt").isFile());
        collectGame(d, "F libc: Fatal signal 11 (SIGSEGV) test line", false);
        System.out.println("flush: " + flush());
        System.out.println("queue left: " + queue.size());
    }
}
