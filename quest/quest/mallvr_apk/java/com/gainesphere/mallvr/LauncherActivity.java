package com.gainesphere.mallvr;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.widget.*;
import java.util.List;

/* MallVR for Meta Quest: the launcher, a plain 2D panel (plan section 5). Game list (green = files found and checked, grey =
   missing), START -> MallVRActivity (immersive), GET GAMES (archive.org), REPORT A PROBLEM, ABOUT / LICENCES.
   Needs "All files access" for /sdcard/MallVR (the same folder layout as the PC package). */
public class LauncherActivity extends Activity {
    static final int BG = 0xFF14101C, PANEL = 0xFF231C30, GOLD = 0xFFE8B33A, CREAM = 0xFFF4EAD2, GREEN = 0xFF6CCB5F, GREY = 0xFF8A8494, AMBER = 0xFFE0A030;
    LinearLayout list; TextView status; Button permBtn, notesBtn;
    static boolean startNoted, askShown;
    List<MallVR.Game> games;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        LinearLayout root = col(this); root.setBackgroundColor(BG); root.setPadding(dp(24), dp(16), dp(24), dp(16));
        TextView t = text(this, "MallVR", 34, GOLD); t.setTypeface(Typeface.DEFAULT_BOLD); root.addView(t);
        root.addView(text(this, "Virtuality arcade VR on Meta Quest  -  TEST BUILD " + MallVR.version(this) + "  -  please report what you find", 15, CREAM));

        LinearLayout bar = row(this); bar.setPadding(0, dp(12), 0, dp(8));
        bar.addView(button("ONLINE", v -> { if (filesOk()) startActivity(new Intent(this, OnlineActivity.class)); }));   // crossplay with PC players
        bar.addView(button("GET GAMES", v -> startActivity(new Intent(this, GetGamesActivity.class))));
        bar.addView(button("REPORT A PROBLEM", v -> startActivity(new Intent(this, ReportActivity.class))));
        bar.addView(button("ABOUT / LICENCES", v -> startActivity(new Intent(this, AboutActivity.class))));
        bar.addView(button("CHECK AGAIN", v -> refresh()));
        notesBtn = button("PLAY NOTES: OFF", v -> askNotes()); bar.addView(notesBtn);
        root.addView(bar);

        permBtn = button("ALLOW FILE ACCESS (needed for /sdcard/MallVR)", v -> askFiles());
        permBtn.setVisibility(View.GONE); root.addView(permBtn);
        status = text(this, "", 15, CREAM); root.addView(status);

        ScrollView sv = new ScrollView(this);
        list = col(this); sv.addView(list);
        root.addView(sv, new LinearLayout.LayoutParams(-1, 0, 1f));
        setContentView(root);
        try { games = MallVR.load(this); } catch (Exception e) { status.setText("games.json missing from the APK: " + e); }
        MallVR.log("launcher open, " + MallVR.version(this) + ", " + MallVR.device());
    }

    @Override protected void onResume() { super.onResume(); refresh(); }
    @Override public void onRequestPermissionsResult(int c, String[] p, int[] r) { refresh(); }

    boolean filesOk() {     /* Android 11+: All files access; Android 10 (old Quest 2 OS): the storage permission */
        if (android.os.Build.VERSION.SDK_INT >= 30) return Environment.isExternalStorageManager();
        return checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE) == android.content.pm.PackageManager.PERMISSION_GRANTED;
    }

    void askFiles() {
        MallVR.log("asking for All files access");
        if (android.os.Build.VERSION.SDK_INT < 30) { requestPermissions(new String[] { android.Manifest.permission.READ_EXTERNAL_STORAGE, android.Manifest.permission.WRITE_EXTERNAL_STORAGE }, 1); return; }
        try { startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, Uri.parse("package:" + getPackageName()))); return; } catch (Exception e) { }
        try { startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)); return; } catch (Exception e) { }
        status.setText("This headset has no All files access screen. From a PC run:\n  adb shell appops set --uid " + getPackageName() + " MANAGE_EXTERNAL_STORAGE allow\n(see QUEST_INSTALL.txt)");
    }

    void refresh() {
        if (games == null) return;
        if (!filesOk()) {
            permBtn.setVisibility(View.VISIBLE);
            status.setText("MallVR keeps games, logs and settings in the MallVR folder of the headset's storage (like the PC version's folder). "
                + "Press ALLOW FILE ACCESS, switch MallVR on, then come back.");
            list.removeAllViews();
            return;
        }
        permBtn.setVisibility(View.GONE);
        status.setText("Checking your game files in " + MallVR.GAMES + " ...");
        new Thread(() -> {
            MallVR.installEngine(this);
            PlayNotes.load(MallVR.SETTINGS, MallVR.version(this));
            PlayNotes.collectGame(MallVR.LOGS, MallVR.crashLines(), gameAlive());     // the game we came back from (play notes ON only)
            MallVR.find(games);
            noteStart();
            PlayNotes.flush();
            runOnUiThread(() -> { showGames(); notesBtn.setText("PLAY NOTES: " + (PlayNotes.on() ? "ON" : "OFF")); if (PlayNotes.needsAsk() && !askShown) { askShown = true; askNotes(); } });
        }).start();
    }

    void showGames() {
        list.removeAllViews();
        int ready = 0;
        for (MallVR.Game g : games) {
            if (g.state == 1 || g.state == 2) ready++;
            LinearLayout r = row(this); r.setBackgroundColor(PANEL); r.setPadding(dp(14), dp(8), dp(14), dp(8)); r.setGravity(Gravity.CENTER_VERTICAL);
            LinearLayout c = col(this);
            c.addView(text(this, g.name, 20, g.state == 1 ? GREEN : g.state == 0 ? GREY : AMBER));
            c.addView(text(this, g.stateText() + (g.why.isEmpty() ? "" : "  -  " + g.why), 13, g.state == 0 ? GREY : CREAM));
            r.addView(c, new LinearLayout.LayoutParams(0, -2, 1f));
            Button s = button("START", v -> start(g));
            s.setEnabled(g.state == 1 || g.state == 2);
            r.addView(s);
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, -2); lp.setMargins(0, dp(4), 0, dp(4));
            list.addView(r, lp);
        }
        status.setText(ready == 0
            ? "No games found yet. Press GET GAMES to download them from archive.org, or copy your games folder from the PC version into " + MallVR.GAMES + " (USB cable or SideQuest)."
            : ready + " of " + games.size() + " games ready. START puts you in the game. Back here: hold A + B for a second.");
    }

    /* is the game process (":game") still running? */
    boolean gameAlive() {
        try {
            for (android.app.ActivityManager.RunningAppProcessInfo p : ((android.app.ActivityManager) getSystemService(ACTIVITY_SERVICE)).getRunningAppProcesses())
                if (p.processName.endsWith(":game")) return true;
        } catch (Exception e) { }
        return false;
    }

    /* once per launcher process: launcher_start (the PC's fields, Quest values) */
    void noteStart() {
        if (startNoted || !PlayNotes.on()) return;
        startNoted = true;
        StringBuilder have = new StringBuilder(); for (MallVR.Game g : games) if (g.state == 1 || g.state == 2) have.append(have.length() > 0 ? " " : "").append(g.code);
        PlayNotes.add("launcher_start", "ver", MallVR.version(this), "plat", "QUEST", "headset", android.os.Build.MANUFACTURER + " " + android.os.Build.MODEL + " (" + android.os.Build.DEVICE + ")",
            "os", "Android " + android.os.Build.VERSION.RELEASE + " / " + android.os.Build.DISPLAY, "runtime", "OpenXR", "mode", "headset", "have", have.toString(),
            "free_mb", "" + (MallVR.ROOT.getUsableSpace() >> 20));
    }

    /* ask once (and from the PLAY NOTES button): the PC's consent screen, same words */
    void askNotes() {
        new android.app.AlertDialog.Builder(this).setTitle("Help make MallVR better?")
            .setMessage("MallVR can send us anonymous play notes:\n\n  - which games you play and for how long\n  - frame rate, headset model and system version\n"
                + "  - whether VR started, and crashes (the game's own MallVR log)\n\nNo names, no account, no files, nothing from your headset besides MallVR's own logs. "
                + "This is the first Quest build, so these notes tell us fastest what works.\n\nOff unless you say yes. Change it any time with the PLAY NOTES button. "
                + "What we collect: gainesphere.com/pages/mallvr-privacy\nREPORT A PROBLEM works either way.")
            .setPositiveButton("YES, SEND PLAY NOTES", (d, w) -> setNotes(true))
            .setNegativeButton("NO THANKS", (d, w) -> setNotes(false))
            .setCancelable(false).show();
    }
    void setNotes(boolean yes) {
        PlayNotes.setConsent(yes);
        MallVR.log("play notes " + (yes ? "ON" : "OFF"));
        notesBtn.setText("PLAY NOTES: " + (yes ? "ON" : "OFF"));
        if (yes) new Thread(() -> { noteStart(); PlayNotes.flush(); }).start();
    }

    void start(MallVR.Game g) {
        try {
            MallVR.Plan p = MallVR.plan(this, g);
            getSharedPreferences("mallvr", 0).edit().putString("last_game", g.code).putString("last_tag", p.tag).putLong("last_start", System.currentTimeMillis()).apply();
            MallVR.log("START " + g.code + " (" + g.stateText() + ") exe=" + g.exeFile + " cue=" + g.cue + " data=" + g.dataPath);
            Intent i = new Intent(this, MallVRActivity.class);
            i.putExtra("conf", p.conf); i.putExtra("env", p.env); i.putExtra("game", g.code);
            startActivity(i);
        } catch (Exception e) {
            MallVR.log("START " + g.code + " failed: " + e);
            status.setText("Couldn't start " + g.name + ": " + e);
        }
    }

    // ---------------------------------------------------------------- tiny view helpers (no XML layouts in this APK)
    int dp(int v) { return (int) (v * getResources().getDisplayMetrics().density + 0.5f); }
    static LinearLayout col(Activity a) { LinearLayout l = new LinearLayout(a); l.setOrientation(LinearLayout.VERTICAL); return l; }
    static LinearLayout row(Activity a) { LinearLayout l = new LinearLayout(a); l.setOrientation(LinearLayout.HORIZONTAL); return l; }
    static TextView text(Activity a, String s, int sp, int colour) { TextView t = new TextView(a); t.setText(s); t.setTextSize(sp); t.setTextColor(colour); return t; }
    Button button(String s, View.OnClickListener l) {
        Button b = new Button(this); b.setText(s); b.setTextSize(15); b.setAllCaps(false); b.setOnClickListener(l);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-2, -2); lp.setMargins(0, 0, dp(10), 0); b.setLayoutParams(lp);
        return b;
    }
}
