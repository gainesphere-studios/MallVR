package com.gainesphere.mallvr;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.*;
import java.io.File;

/* GET GAMES: one row per archive.org item (GameFiles.PACKS), GET = download + check + unpack into MallVR/games. One at a time;
   STOP keeps the part already downloaded, GET again carries on. The screen stays on while it works. */
public class GetGamesActivity extends Activity {
    static volatile Thread worker; static volatile boolean stop; static volatile String line = "";
    LinearLayout list; TextView status;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        LinearLayout root = LauncherActivity.col(this); root.setBackgroundColor(LauncherActivity.BG); root.setPadding(dp(24), dp(16), dp(24), dp(16));
        TextView t = LauncherActivity.text(this, "GET GAMES", 28, LauncherActivity.GOLD); t.setTypeface(Typeface.DEFAULT_BOLD); root.addView(t);
        root.addView(LauncherActivity.text(this, "MallVR ships no game files. These are the community's archive.org copies (the same ones the PC download page "
            + "links), downloaded straight onto this headset into " + MallVR.GAMES + ". Thanks to Simon Marston, nyxxic and Faulty for the SU2000 dump, ErrorDan for the "
            + "3000-series games and Jeremy Shartzer for Buggy Ball. Stay on Wi-Fi and keep the headset awake: if it sleeps and the download stops, press GET again and it carries on.", 14, LauncherActivity.CREAM));
        status = LauncherActivity.text(this, "", 15, LauncherActivity.GOLD); status.setPadding(0, dp(10), 0, dp(10)); root.addView(status);
        ScrollView sv = new ScrollView(this); list = LauncherActivity.col(this); sv.addView(list);
        root.addView(sv, new LinearLayout.LayoutParams(-1, 0, 1f));
        setContentView(root);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        rows();
        tick();
    }

    void rows() {
        list.removeAllViews();
        for (GameFiles.Pack p : GameFiles.PACKS) {
            LinearLayout r = LauncherActivity.row(this); r.setBackgroundColor(LauncherActivity.PANEL); r.setPadding(dp(14), dp(8), dp(14), dp(8)); r.setGravity(Gravity.CENTER_VERTICAL);
            LinearLayout c = LauncherActivity.col(this);
            boolean got = GameFiles.doneMark(MallVR.GAMES, p).isFile();
            c.addView(LauncherActivity.text(this, p.title + "  (" + (p.size >> 20) + " MB)", 18, got ? LauncherActivity.GREEN : LauncherActivity.CREAM));
            c.addView(LauncherActivity.text(this, (got ? "GOT IT  -  " : "") + p.gives + "\narchive.org/details/" + p.item, 13, LauncherActivity.GREY));
            r.addView(c, new LinearLayout.LayoutParams(0, -2, 1f));
            Button g = new Button(this); g.setText(got ? "GET AGAIN" : "GET"); g.setAllCaps(false); g.setOnClickListener(v -> start(p)); r.addView(g);
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, -2); lp.setMargins(0, dp(4), 0, dp(4));
            list.addView(r, lp);
        }
        Button s = new Button(this); s.setText("STOP"); s.setAllCaps(false); s.setOnClickListener(v -> { stop = true; line = "Stopping ..."; });
        list.addView(s);
    }

    void start(GameFiles.Pack p) {
        if (worker != null && worker.isAlive()) { line = "One at a time: wait for the current one (or STOP it)."; return; }
        stop = false; line = "Starting " + p.file + " ...";
        MallVR.log("GET GAMES " + p.id + " started");
        worker = new Thread(() -> {
            try {
                GameFiles.get(p, MallVR.GAMES, new File(MallVR.ROOT, "downloads"), new GameFiles.Progress() {
                    public void say(String s) { line = s; }
                    public boolean cancelled() { return stop; }
                });
                MallVR.log("GET GAMES " + p.id + " done");
                line = "Done: " + p.title + ". Back on the game list, its games turn green.";
            } catch (Throwable e) {
                MallVR.log("GET GAMES " + p.id + " failed: " + e);
                line = "Problem with " + p.title + ": " + e.getMessage();
            }
            runOnUiThread(this::rows);
        }, "MallVR get games");
        worker.start();
    }

    void tick() {   /* the worker writes 'line'; show it twice a second */
        status.setText(line.isEmpty() ? "Pick what to get. The SU2000 dump has most of the games." : line);
        if (!isFinishing()) status.postDelayed(this::tick, 500);
    }

    int dp(int v) { return (int) (v * getResources().getDisplayMetrics().density + 0.5f); }
}
