package com.gainesphere.mallvr;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.view.View;
import android.widget.*;
import java.util.ArrayList;
import java.util.List;

/* MallVR for Meta Quest: the launcher, a 2D panel (plan section 5) drawn as the CYBERSPACE HALL, the PC's game select style
   (HallPainter / HallView; Bobby's pick 10-30). Games as data blocks (cyan edge = files found and checked), START -> MallVRActivity
   (immersive), ONLINE, GET GAMES (archive.org, sideload build only), REPORT, ABOUT / LICENCES, PLAY NOTES, CHECK AGAIN, MUSIC (the
   hall's menu songs, HallMusic). Needs "All files access" for /sdcard/MallVR (the same folder layout as the PC package). */
public class LauncherActivity extends Activity {
    static final int BG = 0xFF14101C, PANEL = 0xFF231C30, GOLD = 0xFFE8B33A, CREAM = 0xFFF4EAD2, GREEN = 0xFF6CCB5F, GREY = 0xFF8A8494, AMBER = 0xFFE0A030;
    static boolean startNoted, askShown, silenced, welcomed;   // silenced = the player tapped the setup alarm off (until MallVR restarts)
    static HallData hallData;                         // kept for the app's life: the pod turnarounds take a moment to load
    List<MallVR.Game> games;
    HallPainter hall; HallView view; HallMusic music; HallVoice voice;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        if (hallData == null) {
            final android.content.res.AssetManager am = getAssets();
            final HallData d = hallData = new HallData(n -> am.open("hall/" + n), new java.io.File(getFilesDir(), "news_cache.txt"));
            new Thread(() -> { d.loadPods(HallPainter.ERA_PODS); d.fetchNews(); }).start();
        }
        hall = new HallPainter(hallData);
        hall.store = MallVR.store(this);
        hall.version = MallVR.version(this);
        hall.music = getSharedPreferences("mallvr", 0).getBoolean("music", true);
        music = new HallMusic(this);
        voice = new HallVoice(this);
        view = new HallView(this, hall, this::onHall);
        view.setBackgroundColor(0xFF020208);
        setContentView(view);
        try { games = MallVR.load(this); } catch (Exception e) { setStatus("games.json missing from the APK: " + e); }
        if (games != null) {
            String last = getSharedPreferences("mallvr", 0).getString("last_game", "");
            for (int i = 0; i < games.size(); i++) if (games.get(i).code.equals(last)) { hall.game = i; hall.lostTab = games.get(i).name.endsWith("(LOST)"); }
            showGames();
            if (hall.store && !getSharedPreferences("mallvr", 0).getBoolean("email_asked", false)) {   // first thing after install, like the PC's GET GAMES
                getSharedPreferences("mallvr", 0).edit().putBoolean("email_asked", true).apply();
                askedGetGames = true; askGetGames();
            }
        }
        MallVR.log("launcher open, " + MallVR.version(this) + ", " + MallVR.device());
    }

    @Override protected void onResume() { super.onResume(); refresh(); syncSound(); view.invalidate(); }
    @Override protected void onPause() { super.onPause(); voice.stop(); hall.alarm = false; music.set(false); }
    @Override public void onRequestPermissionsResult(int c, String[] p, int[] r) { refresh(); }

    /* the setup alarm while file access or every game is missing (until tapped off); the menu songs wait for it */
    void syncSound() {
        String kind = silenced || games == null ? null : hall.needFiles ? "alarm_files"
            : hall.checked && !hall.anyFound() ? (hall.store ? "alarm_nogames_store" : "alarm_nogames") : null;
        if (kind != null) voice.alarm(kind); else if (voice.alarming()) voice.stop();
        hall.alarm = voice.alarming();
        music.set(hall.music && !hall.alarm);
        if (kind == null && hall.checked && hall.anyFound() && !welcomed) { welcomed = true; voice.say("welcome"); }
    }

    /* the hall's buttons (its sheet tabs, era dials and WANTED board it handles itself) */
    void onHall(String id) {
        if (id.equals("tap")) {       // any press silences the alarm
            if (voice.alarming()) { silenced = true; MallVR.log("setup alarm silenced"); syncSound(); }
            return;
        }
        if (id.startsWith("g")) {     // a game picked: remembered for next time, like START does; the voice says if it's missing
            if (games != null && hall.game < games.size()) {
                getSharedPreferences("mallvr", 0).edit().putString("last_game", games.get(hall.game).code).apply();
                int st = hall.checked && !hall.needFiles ? games.get(hall.game).state : 1;
                if (st == 0) voice.say(hall.store ? "missing_store" : "missing");
                else if (st == 3) voice.say(hall.store ? "needcd_store" : "needcd");
            }
            return;
        }
        if (id.startsWith("step")) {  // a setup card opened: read it
            voice.stop(); voice.say("tut_" + hall.stepKey(hall.card));
            if (hall.stepKey(hall.card).equals("copygames")) askGetGames();
            return;
        }
        if (id.equals("cardclose")) { voice.stop(); return; }
        switch (id) {
            case "start": if (games != null && hall.found(hall.game)) start(games.get(hall.game)); break;
            case "online": if (filesOk()) startActivity(new Intent(this, OnlineActivity.class)); else askFiles(); break;   // crossplay with PC players
            case "getgames":              // the store build fetches nothing itself: askGetGames sends the player to the PC tool that does
                if (!MallVR.store(this)) startActivity(new Intent(this, GetGamesActivity.class));
                else askGetGames();
                break;
            case "report": startActivity(new Intent(this, ReportActivity.class)); break;
            case "about": startActivity(new Intent(this, AboutActivity.class)); break;
            case "check": refresh(); break;
            case "notes": askNotes(); break;
            case "files": askFiles(); break;
            case "music":
                hall.music = !hall.music;
                getSharedPreferences("mallvr", 0).edit().putBoolean("music", hall.music).apply();
                syncSound();
                break;
            case "news":
                String l = hallData.newsLink;
                if (l.startsWith("https://")) try { startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(l))); } catch (Exception e) { }
                break;
        }
    }

    /* store build's GET GAMES: the tool is on a PC, so offer to email the link there (online/sendlink.php; the address isn't kept) */
    boolean askedGetGames;
    static final String GET_GAMES_PAGE = "https://gainesphere.com/pages/getgames";
    void askGetGames() {
        final android.widget.EditText box = new android.widget.EditText(this);
        box.setInputType(android.text.InputType.TYPE_CLASS_TEXT | android.text.InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
        box.setHint("you@example.com");
        new android.app.AlertDialog.Builder(this).setTitle("Get your games")
            .setMessage("Own these games? A free tool on a Windows PC gets your backups onto this headset: gainesphere.com/getgames\n\nType your email and we'll send you the instructions. We use your address only to send them and don't keep it.")
            .setView(box)
            .setPositiveButton("EMAIL ME THE LINK", (d, w) -> sendLink(box.getText().toString().trim()))
            .setNeutralButton("OPEN THE PAGE", (d, w) -> { try { startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(GET_GAMES_PAGE))); } catch (Exception e) { } })
            .setNegativeButton("CLOSE", null).show();
    }

    void sendLink(final String email) {
        setStatus("Sending the link...");
        new Thread(() -> {
            String say;
            try {
                java.net.HttpURLConnection c = (java.net.HttpURLConnection) new java.net.URL("https://downloads.gainesphere.com/mallvr/online/sendlink.php").openConnection();
                c.setConnectTimeout(10000); c.setReadTimeout(15000); c.setDoOutput(true); c.setRequestMethod("POST");
                c.getOutputStream().write(("email=" + java.net.URLEncoder.encode(email, "UTF-8")).getBytes("UTF-8"));
                int code = c.getResponseCode();
                say = code == 200 ? "SENT"
                    : code == 400 ? "That doesn't look like an email address. Press GET GAMES to try again."
                    : code == 429 ? "Too many sent just now: check your inbox, or try again later."
                    : "Couldn't send it. On a PC, go to gainesphere.com/getgames";
            } catch (Exception e) { say = "Couldn't send it (no internet?). On a PC, go to gainesphere.com/getgames"; }
            final String s = say;
            runOnUiThread(() -> {
                if (!s.equals("SENT")) { setStatus(s); return; }
                setStatus("Sent to " + email + ". Check your email and follow the instructions.");
                new android.app.AlertDialog.Builder(this).setTitle("Check your email")
                    .setMessage("We sent the instructions to " + email + ". Check your email (and spam folder) and follow them on a Windows PC.")
                    .setPositiveButton("OK", null).show();
            });
        }, "MallVR send link").start();
    }

    void setStatus(String s) { hall.status = s; if (view != null) view.invalidate(); }

    /* games.json's name as the hall shows it: the PC's (launcher.ps1 drops "Virtuality " in front), in capitals */
    static String hallName(String n) {
        return n.replaceFirst("^Virtuality ", "").toUpperCase(java.util.Locale.US);
    }

    boolean filesOk() {     /* Android 11+: All files access; Android 10 (old Quest 2 OS): the storage permission */
        if (android.os.Build.VERSION.SDK_INT >= 30) return Environment.isExternalStorageManager();
        return checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE) == android.content.pm.PackageManager.PERMISSION_GRANTED;
    }

    void askFiles() {
        MallVR.log("asking for All files access");
        if (android.os.Build.VERSION.SDK_INT < 30) { requestPermissions(new String[] { android.Manifest.permission.READ_EXTERNAL_STORAGE, android.Manifest.permission.WRITE_EXTERNAL_STORAGE }, 1); return; }
        try { startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, Uri.parse("package:" + getPackageName()))); return; } catch (Exception e) { }
        try { startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)); return; } catch (Exception e) { }
        setStatus("This headset has no All files access screen. From a PC run:  adb shell appops set --uid " + getPackageName() + " MANAGE_EXTERNAL_STORAGE allow  (see QUEST_INSTALL.txt)");
    }

    void refresh() {
        if (games == null) return;
        if (!filesOk()) {
            hall.needFiles = true; hall.checked = false;
            setStatus("MallVR keeps games, logs and settings in the MallVR folder of the headset's storage (like the PC version's folder). "
                + "Press ALLOW FILE ACCESS, switch MallVR on, then come back.");
            syncSound();
            return;
        }
        hall.needFiles = false; hall.checked = false;
        setStatus("Checking your game files in " + MallVR.GAMES + " ...");
        new Thread(() -> {
            MallVR.installEngine(this);
            PlayNotes.load(MallVR.SETTINGS, MallVR.version(this));
            PlayNotes.collectGame(MallVR.LOGS, MallVR.crashLines(), gameAlive());     // the game we came back from (play notes ON only)
            MallVR.find(games);
            noteStart();
            PlayNotes.flush();
            final String xrFail = xrCheck();
            runOnUiThread(() -> {
                hall.checked = true; hall.notesOn = PlayNotes.on(); showGames(); syncSound();
                if (hall.store && !hall.anyFound() && !askedGetGames) { askedGetGames = true; askGetGames(); }   // no games yet: the email box comes up by itself, once
                if (!xrFail.isEmpty()) xrHelp(xrFail, null);
                else if (PlayNotes.needsAsk() && !askShown) { askShown = true; askNotes(); }
            });
        }).start();
    }

    /* did VR (OpenXR) start in the last game? Report 2026-10-31: a Quest 2 on old system software (Android 12, no OpenXR runtime
       broker) got "xrCreateInstance failed (-51)", so the game ran with sound but its picture went to the off-screen pbuffer and the
       player only saw DOSBox-X's window. Remembered per system build ("xr_fail_build") so START warns until the headset updates.
       Returns the XR log line the first time a failed game is seen, else "". */
    String xrCheck() {
        android.content.SharedPreferences sp = getSharedPreferences("mallvr", 0);
        String tag = sp.getString("last_tag", "");
        if (tag.isEmpty()) return "";
        java.io.File render = new java.io.File(MallVR.LOGS, tag + ".render");
        if (!render.isFile() || render.lastModified() <= sp.getLong("xr_checked", 0)) return "";
        String up = PlayNotes.xr(render)[0];
        android.content.SharedPreferences.Editor e = sp.edit().putLong("xr_checked", render.lastModified());
        if (up.equals("up")) { e.remove("xr_fail_build").apply(); return ""; }
        if (!up.startsWith("failed")) { e.apply(); return ""; }
        e.putString("xr_fail_build", android.os.Build.DISPLAY).apply();
        MallVR.log("VR did not start in the last game (" + tag + "): " + up);
        return up.substring(Math.min(up.length(), 8));
    }

    /* the explanation, after a failed game (go = null) or before START on the same system build (go = start anyway) */
    void xrHelp(String why, Runnable go) {
        android.app.AlertDialog.Builder d = new android.app.AlertDialog.Builder(this).setTitle("VR didn't start on this headset")
            .setMessage("The game ran, but the headset's VR system (OpenXR) didn't answer MallVR, so you only saw the DOSBox screen.\n\n"
                + "This usually means the headset's system software is out of date (this one reports Android " + android.os.Build.VERSION.RELEASE
                + ", build " + android.os.Build.DISPLAY + "). Please update it: Settings > System > Software Update, install everything, "
                + "restart the headset, then START again.\n\nIf a game ever gets stuck like this, press the Meta button and quit MallVR."
                + (why.isEmpty() ? "" : "\n\n(log: " + why + ")"));
        if (go == null) d.setPositiveButton("OK", null);
        else d.setPositiveButton("START ANYWAY", (x, w) -> go.run()).setNegativeButton("CANCEL", null);
        d.show();
    }

    /* the games into the hall's blocks (state = what find() saw), and the line under them */
    void showGames() {
        List<HallPainter.Item> items = new ArrayList<>();
        int ready = 0, total = 0;
        for (MallVR.Game g : games) {
            if (!g.name.endsWith("(LOST)")) { total++; if (g.state == 1 || g.state == 2) ready++; }   // the Lost Games need no files
            HallPainter.Item it = new HallPainter.Item();
            it.code = g.code; it.name = hallName(g.name); it.state = g.state; it.why = g.why;
            items.add(it);
        }
        hall.items = items;
        view.invalidate();
        if (!hall.checked) return;
        setStatus(ready == 0
            ? (MallVR.store(this) ? "No games found yet. Copy your own games folder (the same one the PC version uses) into " + MallVR.GAMES + " with a USB cable."
                : "No games found yet. Press GET GAMES to download them from archive.org, or copy your games folder from the PC version into " + MallVR.GAMES + " (USB cable or SideQuest).")
            : ready + " of " + total + " games ready. START puts you in the game. Back here: hold A + B for a second.");
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
        hall.notesOn = yes; view.invalidate();
        if (yes) new Thread(() -> { noteStart(); PlayNotes.flush(); }).start();
    }

    void start(MallVR.Game g) {
        if (android.os.Build.DISPLAY.equals(getSharedPreferences("mallvr", 0).getString("xr_fail_build", null))) { xrHelp("", () -> launch(g)); return; }
        launch(g);
    }
    void launch(MallVR.Game g) {
        try {
            MallVR.Plan p = MallVR.plan(this, g);
            getSharedPreferences("mallvr", 0).edit().putString("last_game", g.code).putString("last_tag", p.tag).putLong("last_start", System.currentTimeMillis()).apply();
            MallVR.log("START " + g.code + " (" + g.stateText() + ") exe=" + g.exeFile + " cue=" + g.cue + " data=" + g.dataPath);
            Intent i = new Intent(this, MallVRActivity.class);
            i.putExtra("conf", p.conf); i.putExtra("env", p.env); i.putExtra("game", g.code);
            startActivity(i);
        } catch (Exception e) {
            MallVR.log("START " + g.code + " failed: " + e);
            setStatus("Couldn't start " + g.name + ": " + e);
        }
    }

    // ---------------------------------------------------------------- tiny view helpers for the other screens (no XML layouts in this APK)
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
