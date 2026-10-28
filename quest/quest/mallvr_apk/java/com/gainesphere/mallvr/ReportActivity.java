package com.gainesphere.mallvr;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Typeface;
import android.os.Bundle;
import android.widget.*;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/* REPORT A PROBLEM (plan "Release = the test"): one typed line + report.txt (headset, OS, MallVR version, last game, game check) +
   MallVR/logs (the last game's first, 5 MB cap like the PC) + this app's own logcat lines (render / XR / crash messages).
   Saved to Download/ (findable over USB or SideQuest) and, if the player presses SEND, POSTed to report.php like the PC launcher
   (MallNotes.cs: POST report.php?id=&ver=&game=, body = the zip; reply "OK ..."). */
public class ReportActivity extends Activity {
    static final String URL_REPORT = "https://downloads.gainesphere.com/mallvr/online/report.php";
    static final long CAP = 5L * 1024 * 1024 - 64 * 1024;
    EditText what; TextView status; File zip;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        LinearLayout root = LauncherActivity.col(this); root.setBackgroundColor(LauncherActivity.BG); root.setPadding(dp(24), dp(16), dp(24), dp(16));
        TextView t = LauncherActivity.text(this, "REPORT A PROBLEM", 28, LauncherActivity.GOLD); t.setTypeface(Typeface.DEFAULT_BOLD); root.addView(t);
        root.addView(LauncherActivity.text(this, "Thank you for testing! Say in one line what happened (which game, what you saw). MallVR packs that line with "
            + "your headset model, its system version, MallVR's version and the logs of your last game into one zip. No account, name or location is in it.", 14, LauncherActivity.CREAM));
        what = new EditText(this); what.setHint("e.g. Dactyl Nightmare: black screen after the title"); what.setTextColor(LauncherActivity.CREAM);
        what.setHintTextColor(LauncherActivity.GREY); what.setSingleLine(false); what.setMinLines(2); root.addView(what);
        LinearLayout bar = LauncherActivity.row(this);
        Button send = new Button(this); send.setText("SEND TO MALLVR"); send.setAllCaps(false); send.setOnClickListener(v -> go(true)); bar.addView(send);
        Button save = new Button(this); save.setText("JUST SAVE THE ZIP"); save.setAllCaps(false); save.setOnClickListener(v -> go(false)); bar.addView(save);
        root.addView(bar);
        status = LauncherActivity.text(this, "", 15, LauncherActivity.GOLD); status.setPadding(0, dp(12), 0, 0); root.addView(status);
        root.addView(LauncherActivity.text(this, "\nCan't send from here? The zip is saved in the headset's Download folder either way: post it in the MallVR Discord "
            + "(#bug-reports) from a PC (USB cable or SideQuest's file browser).", 13, LauncherActivity.GREY));
        setContentView(root);
    }

    void go(boolean upload) {
        String line = what.getText().toString().trim();
        status.setText("Packing the report ...");
        new Thread(() -> {
            String r;
            try {
                zip = make(line);
                r = "Saved: " + zip.getPath();
                MallVR.log("report saved " + zip.getName() + " (" + zip.length() / 1024 + " KB)");
                PlayNotes.add("report", "game", getSharedPreferences("mallvr", 0).getString("last_tag", ""), "sent", upload ? "1" : "0"); PlayNotes.flush();
                if (upload) {
                    String sent = post(zip, line);
                    MallVR.log("report upload: " + sent);
                    r = sent.startsWith("OK") ? "SENT. THANK YOU! (a copy is in " + zip.getPath() + ")" : "Couldn't send (" + sent + "). " + r;
                }
            } catch (Throwable e) { r = "Problem: " + e; MallVR.log("report failed: " + e); }
            final String s = r;
            runOnUiThread(() -> status.setText(s));
        }).start();
    }

    File make(String line) throws Exception {
        SharedPreferences sp = getSharedPreferences("mallvr", 0);
        String last = sp.getString("last_game", ""), tag = sp.getString("last_tag", "");
        long when = sp.getLong("last_start", 0);
        StringBuilder rep = new StringBuilder();
        rep.append("MallVR REPORT A PROBLEM (Quest)\n").append("What happened: ").append(line).append("\n")
           .append("MallVR version: ").append(MallVR.version(this)).append(" (").append(getPackageName()).append(")\n")
           .append("Headset: ").append(MallVR.device()).append("\n")
           .append("Last game: ").append(last).append(when > 0 ? " started " + new Date(when) : "").append("\n")
           .append("Report made: ").append(new Date()).append("\n")
           .append("Free space: ").append(MallVR.ROOT.getUsableSpace() >> 20).append(" MB\n\nGame check:\n");
        try { List<MallVR.Game> gs = MallVR.load(this); MallVR.find(gs); for (MallVR.Game g : gs) rep.append("  ").append(g.code).append(": ").append(g.stateText()).append(g.exeFile != null ? "  " + g.exeFile : "").append(g.why.isEmpty() ? "" : "  (" + g.why + ")").append("\n"); }
        catch (Exception e) { rep.append("  (couldn't check: ").append(e).append(")\n"); }

        File dl = new File("/sdcard/Download"); dl.mkdirs();
        File z = new File(dl, "MallVR_Quest_report_" + new SimpleDateFormat("yyyy-MM-dd_HHmmss", Locale.US).format(new Date()) + ".zip");
        long[] used = { 0 };
        try (ZipOutputStream o = new ZipOutputStream(new FileOutputStream(z))) {
            put(o, "report.txt", rep.toString().getBytes("UTF-8"), used);
            put(o, "logcat.txt", logcat(), used);
            List<File> logs = new ArrayList<>();
            File[] l = MallVR.LOGS.listFiles(); if (l != null) logs.addAll(Arrays.asList(l));
            final String tg = tag + ".";
            Collections.sort(logs, (x, y) -> {           // the last game's files first, then the newest
                boolean a = x.getName().startsWith(tg), c = y.getName().startsWith(tg);
                if (a != c) return a ? -1 : 1;
                return Long.compare(y.lastModified(), x.lastModified());
            });
            for (File f : logs) if (f.isFile() && !f.getName().endsWith(".zip")) putFile(o, "logs/" + f.getName(), f, used);
            File[] s = MallVR.SETTINGS.listFiles(); if (s != null) for (File f : s) if (f.isFile() && f.length() < 64 * 1024) putFile(o, "settings/" + f.getName(), f, used);
        }
        return z;
    }

    static void put(ZipOutputStream o, String name, byte[] data, long[] used) throws IOException {
        if (used[0] + data.length > CAP) return;
        o.putNextEntry(new ZipEntry(name)); o.write(data); o.closeEntry(); used[0] += data.length / 4;   // text zips ~4:1
    }
    static void putFile(ZipOutputStream o, String name, File f, long[] used) throws IOException {
        long n = Math.min(f.length(), 2L * 1024 * 1024);      // a huge log: its last 2 MB
        if (used[0] + n / 4 > CAP) return;
        byte[] b = new byte[(int) n];
        try (RandomAccessFile r = new RandomAccessFile(f, "r")) { r.seek(f.length() - n); r.readFully(b); }
        put(o, name, b, used);
    }

    /* this app's own log lines (Android lets an app read its own): MallVR, MallVRxr, SDL, DOSBox-X, crashes in the game process */
    static byte[] logcat() {
        try {
            Process p = Runtime.getRuntime().exec(new String[] { "logcat", "-d", "-v", "time", "-t", "20000" });
            ByteArrayOutputStream b = new ByteArrayOutputStream(); MallVR.copy(p.getInputStream(), b); p.waitFor();
            byte[] all = b.toByteArray(); int max = 3 * 1024 * 1024;
            return all.length <= max ? all : Arrays.copyOfRange(all, all.length - max, all.length);
        } catch (Exception e) { return ("logcat failed: " + e).getBytes(); }
    }

    String post(File z, String line) {
        try {
            String game = getSharedPreferences("mallvr", 0).getString("last_game", "");
            String q = "?id=&ver=" + URLEncoder.encode(MallVR.version(this), "UTF-8") + "&game=" + URLEncoder.encode(game, "UTF-8");
            HttpURLConnection c = (HttpURLConnection) new URL(URL_REPORT + q).openConnection();
            c.setConnectTimeout(20000); c.setReadTimeout(60000); c.setDoOutput(true); c.setRequestMethod("POST");
            c.setRequestProperty("Content-Type", "application/zip"); c.setFixedLengthStreamingMode(z.length());
            try (OutputStream o = c.getOutputStream(); InputStream in = new FileInputStream(z)) { MallVR.copy(in, o); }
            int code = c.getResponseCode();
            InputStream in = code < 400 ? c.getInputStream() : c.getErrorStream();
            String r = in != null ? MallVR.readAll(in).trim() : "";
            return r.isEmpty() ? "HTTP " + code : r;
        } catch (Exception e) { return "no connection: " + e.getMessage(); }
    }

    int dp(int v) { return (int) (v * getResources().getDisplayMetrics().density + 0.5f); }
}
