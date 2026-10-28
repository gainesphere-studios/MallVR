package com.gainesphere.mallvr;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.InputFilter;
import android.view.Gravity;
import android.view.View;
import android.widget.*;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.*;

/* MallVR ONLINE on the Quest: the PC launcher's ONLINE screen (MallLauncher.cs DrawOnline / DrawQuick) as a 2D panel, crossplay with
   PC players in one list (Quest plan section 7). NAME, QUICK MATCH (any game or one), the open games (JOIN; PC / QUEST tag; greyed
   when full or this headset lacks the game), who's online, HOST a game, then START / CANCEL / LEAVE; the group stays together after a
   game (the host picks the next one here). Games start as pod <n> with MallVR.plan(.., pod); MallOnline (in OnlineService) relays
   their frames. Back from a game = hold A + B (as solo) or the game ending: we come back here, still in the group.
   Voice chat with PC and Quest players (Bobby 10-26: "we want cross voice chat"): the engine sends the headset mic (SDL capture) in
   the PC's voice packets; the mic permission is asked here, because a permission dialog can't show over the immersive game.
   Left stick click in a game = mic mute / live (as on the PC). VOICE: per-friend volume (- / + / MUTE, 0-200 %, kept per name in
   settings/voice_volume.txt like the PC's), written for the engine to settings/voice_pods.txt ("<pod> <percent>").
   Not on the Quest yet: the in-headset GO / STAY OUT question when the host switches mid-game (no answer = GO, so the Quest player
   just goes along). */
public class OnlineActivity extends Activity {
    static final int BG = LauncherActivity.BG, PANEL = LauncherActivity.PANEL, GOLD = LauncherActivity.GOLD, CREAM = LauncherActivity.CREAM,
        GREEN = LauncherActivity.GREEN, GREY = LauncherActivity.GREY, AMBER = LauncherActivity.AMBER;
    static final int GAME_CONTROL = 27123;           // MallVRActivity listens here (127.0.0.1) for QUIT (group switch)
    MallOnline net;
    List<MallVR.Game> games;
    final Handler ui = new Handler(Looper.getMainLooper());
    TextView note, router, online, group, title;
    LinearLayout hosts, groupBar, voices;
    TextView micLine;
    String shownVoices = "";
    final Map<String, Integer> voiceVol = new HashMap<>(), voiceMuted = new HashMap<>();
    EditText nameBox;
    Button quickBtn, hostBtn, hostGameBtn, startBtn, leaveBtn, quickGameBtn;
    int hostPick = -1, quickPick = -1;              // index into ready games; quick -1 = ANY GAME
    List<MallVR.Game> ready = new ArrayList<>();
    String shownHosts = "";
    long gameStartedAt;                             // our game process was started (ms); 0 = none
    boolean gameSeen;
    String playing = "";                            // game code running now

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        try { games = MallVR.load(this); } catch (Exception e) { finish(); return; }
        MallVR.find(games);
        for (MallVR.Game g : games) if (g.link && (g.state == 1 || g.state == 2) && !g.pods.isEmpty()) ready.add(g);
        OnlineService.begin(this);
        net = OnlineService.net(this);
        synchronized (net.have) { net.have.clear(); for (MallVR.Game g : ready) net.have.add(g.code); }
        synchronized (net.maxFor) { for (MallVR.Game g : ready) net.maxFor.put(g.code, g.maxPlayers); }
        synchronized (net.sha1For) { for (MallVR.Game g : ready) if (g.exeFile != null) net.sha1For.put(g.code, MallVR.sha1(g.exeFile)); }
        net.quickGames.clear(); for (MallVR.Game g : ready) net.quickGames.add(g.code);
        if (!ready.isEmpty()) hostPick = 0;

        LinearLayout root = LauncherActivity.col(this); root.setBackgroundColor(BG); root.setPadding(dp(24), dp(14), dp(24), dp(14));
        title = LauncherActivity.text(this, "MallVR ONLINE", 30, GOLD); title.setTypeface(Typeface.DEFAULT_BOLD); root.addView(title);
        root.addView(LauncherActivity.text(this, "PC and Quest players in one list. Hosting shares your internet address with the players who join.", 14, CREAM));

        LinearLayout nr = LauncherActivity.row(this); nr.setGravity(Gravity.CENTER_VERTICAL); nr.setPadding(0, dp(8), 0, dp(4));
        nr.addView(LauncherActivity.text(this, "NAME  ", 18, GOLD));
        nameBox = new EditText(this); nameBox.setText(net.name); nameBox.setTextColor(CREAM); nameBox.setSingleLine(true); nameBox.setTextSize(18);
        nameBox.setFilters(new InputFilter[] { new InputFilter.LengthFilter(12), new InputFilter.AllCaps() });
        nr.addView(nameBox, new LinearLayout.LayoutParams(dp(220), -2));
        nr.addView(button("SAVE NAME", v -> saveName()));
        root.addView(nr);

        LinearLayout qr = LauncherActivity.row(this); qr.setPadding(0, dp(6), 0, dp(6)); qr.setGravity(Gravity.CENTER_VERTICAL);
        quickBtn = button("QUICK MATCH", v -> quick()); qr.addView(quickBtn);
        quickGameBtn = button("ANY GAME", v -> { quickPick = quickPick + 1 >= ready.size() ? -1 : quickPick + 1; refresh(); }); qr.addView(quickGameBtn);
        qr.addView(LauncherActivity.text(this, "finds a player and starts by itself", 13, GREY));
        root.addView(qr);

        root.addView(LauncherActivity.text(this, "OPEN GAMES", 18, GOLD));
        hosts = LauncherActivity.col(this); root.addView(hosts);
        online = LauncherActivity.text(this, "", 13, CREAM); online.setPadding(0, dp(8), 0, dp(4)); root.addView(online);

        LinearLayout hr = LauncherActivity.row(this); hr.setPadding(0, dp(6), 0, dp(6)); hr.setGravity(Gravity.CENTER_VERTICAL);
        hostBtn = button("HOST", v -> host()); hr.addView(hostBtn);
        hostGameBtn = button("", v -> { if (!ready.isEmpty()) hostPick = (hostPick + 1) % ready.size(); if (net.status.equals("HOST") && net.inGroup()) net.setGame(ready.get(hostPick).code); refresh(); }); hr.addView(hostGameBtn);
        root.addView(hr);

        group = LauncherActivity.text(this, "", 15, GOLD); root.addView(group);
        groupBar = LauncherActivity.row(this);
        startBtn = button("START", v -> { if (!net.startMatch()) refresh(); }); groupBar.addView(startBtn);
        leaveBtn = button("LEAVE", v -> { net.stopQuick(); net.leave(); net.note = ""; refresh(); }); groupBar.addView(leaveBtn);
        root.addView(groupBar);
        voices = LauncherActivity.col(this); root.addView(voices);
        micLine = LauncherActivity.text(this, "", 13, GREY); root.addView(micLine);

        note = LauncherActivity.text(this, "", 16, AMBER); note.setPadding(0, dp(8), 0, 0); root.addView(note);
        router = LauncherActivity.text(this, "", 13, GREY); root.addView(router);
        LinearLayout bot = LauncherActivity.row(this); bot.setPadding(0, dp(10), 0, 0);
        bot.addView(button("BACK", v -> back()));
        root.addView(bot);

        ScrollView sv = new ScrollView(this); sv.addView(root);
        setContentView(sv);
        if (ready.isEmpty()) net.note = "NO ONLINE GAME FOUND: PRESS GET GAMES ON THE MAIN SCREEN";
        MallVR.log("ONLINE open: " + ready.size() + " online games ready");
        loadVoice();
        net.screenOpened();
        if (checkSelfPermission(android.Manifest.permission.RECORD_AUDIO) != android.content.pm.PackageManager.PERMISSION_GRANTED)
            requestPermissions(new String[] { android.Manifest.permission.RECORD_AUDIO }, 2);
        tick();
    }

    void saveName() {
        String n = MallOnline.clean(nameBox.getText().toString(), 12);
        if (n.isEmpty()) return;
        net.name = n; OnlineService.saveSetting("name", n); nameBox.setText(n);
        net.beat();
    }

    void quick() {
        if (ready.isEmpty()) return;
        if (net.quick) { net.stopQuick(); refresh(); return; }
        saveName();
        net.startQuick(quickPick >= 0 ? ready.get(quickPick).code : "");
        refresh();
    }

    void host() {
        if (ready.isEmpty() || hostPick < 0) return;
        saveName();
        MallVR.Game g = ready.get(hostPick);
        new Thread(() -> net.host(g.code, g.maxPlayers)).start();
    }

    void back() {
        net.screenClosed("back"); drainEvents();
        net.stopQuick(); net.leave();
        OnlineService.end(this);
        finish();
    }
    @Override public void onBackPressed() { back(); }

    // ---- the game ---------------------------------------------------------------------------------------
    MallVR.Game game(String code) { for (MallVR.Game g : games) if (g.code.equals(code)) return g; return null; }

    void startGame(String code, int pod) {
        MallVR.Game g = game(code);
        if (g == null || g.exeFile == null && g.nativ == null) { net.note = "YOU DON'T HAVE " + code + ": LEAVING THE GROUP"; net.leave(); return; }
        try {
            MallVR.Plan p = MallVR.plan(this, g, pod);
            writeVoicePods();
            net.beginGame(pod);
            playing = code; gameStartedAt = System.currentTimeMillis(); gameSeen = false;
            getSharedPreferences("mallvr", 0).edit().putString("last_game", g.code).putString("last_tag", p.tag).putLong("last_start", System.currentTimeMillis()).apply();
            MallVR.log("ONLINE START " + code + " pod " + pod + " roster " + net.roster() + " " + net.rosterPlat());
            Intent i = new Intent(this, MallVRActivity.class);
            i.putExtra("conf", p.conf); i.putExtra("env", p.env); i.putExtra("game", g.code); i.putExtra("back", "online");
            i.putExtra("tag", p.tag); i.putExtra("pod", pod);
            startedQuick = net.quick;
            startActivity(i);
        } catch (Exception e) {
            MallVR.log("ONLINE START " + code + " failed: " + e);
            net.note = "COULDN'T START " + code + ": " + e.getMessage();
            net.endGame(); net.afterGame();
        }
    }

    /* ask the game (MallVRActivity in the ":game" process) to close: the group switched games */
    static void closeGame() {
        new Thread(() -> {
            try (DatagramSocket s = new DatagramSocket()) {
                byte[] q = "QUIT".getBytes("US-ASCII");
                for (int i = 0; i < 3; i++) s.send(new DatagramPacket(q, q.length, InetAddress.getByName("127.0.0.1"), GAME_CONTROL));
            } catch (Exception e) { }
        }).start();
    }

    boolean startedQuick;

    /* MallOnline's play-note events (failures, the ONLINE visit) -> PlayNotes (+ the headset's Wi-Fi), and the launcher log */
    void drainEvents() {
        List<String[]> ev;
        synchronized (net.events) { ev = new ArrayList<>(net.events); net.events.clear(); }
        if (ev.isEmpty()) return;
        for (String[] e : ev) {
            StringBuilder b = new StringBuilder(e[0]); for (int i = 1; i + 1 < e.length; i += 2) b.append(' ').append(e[i]).append('=').append(e[i + 1]);
            MallVR.log("ONLINE " + b);
            String[] kv = Arrays.copyOfRange(e, 1, e.length + 2); kv[kv.length - 2] = "wifi"; kv[kv.length - 1] = wifi();
            PlayNotes.add(e[0], kv);
        }
        if (PlayNotes.on()) new Thread(PlayNotes::flush).start();
    }

    /* the headset's Wi-Fi: signal, band, link speed ("-58dBm 5GHz 433Mbps"): the first suspect for lag or drops on a Quest */
    String wifi() {
        try {
            android.net.wifi.WifiInfo w = ((android.net.wifi.WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE)).getConnectionInfo();
            if (w == null) return "";
            return w.getRssi() + "dBm " + (w.getFrequency() >= 4900 ? "5GHz" : w.getFrequency() > 0 ? "2.4GHz" : "?") + " " + w.getLinkSpeed() + "Mbps";
        } catch (Exception e) { return ""; }
    }

    /* play notes (only when the player said yes): the PC launcher's game_end + online records, Quest values, plus who was in the
       group and on what (crossplay: "BOBBY:PC QUEST1:QUEST"), and the switch if the group moved on. The only way we learn how
       online play goes on headsets we don't have (Bobby 10-26). */
    void noteGame(String code) {
        if (!PlayNotes.on()) return;
        try {
            PlayNotes.collectGame(MallVR.LOGS, MallVR.crashLines(), false, "online");
            boolean host = net.isHost();
            StringBuilder grp = new StringBuilder();
            List<String> r = net.roster(), rp = net.rosterPlat();
            for (int i = 0; i < r.size(); i++) grp.append(i > 0 ? " " : "").append(r.get(i).replace(' ', '_')).append(':').append(i < rp.size() ? rp.get(i) : "PC");
            PlayNotes.add("online", "game", code, "role", host ? "host" : "member", "quick", startedQuick ? "1" : "0",
                "via", host ? (net.upnpMapped ? "upnp" : "punch") : net.joinVia, "connect_secs", host ? "" : String.format(Locale.US, "%.1f", net.joinSecs),
                "lag_ms", net.lagNote(), "arrived", host ? net.arrivals() : "", "group", grp.toString(), "wifi", wifi(), "ver", MallVR.version(this));
            if (!net.switchGame.isEmpty())
                PlayNotes.add("group_switch", "from", code, "to", net.switchGame, "role", host ? "host" : "member", "pod", "" + net.switchPod, "answers", host ? net.switchNote : "");
            new Thread(PlayNotes::flush).start();
        } catch (Exception e) { MallVR.log("ONLINE notes: " + e); }
    }

    boolean gameAlive() {
        try {
            for (android.app.ActivityManager.RunningAppProcessInfo p : ((android.app.ActivityManager) getSystemService(ACTIVITY_SERVICE)).getRunningAppProcesses())
                if (p.processName.endsWith(":game")) return true;
        } catch (Exception e) { }
        return false;
    }

    /* twice a second: start / end games as the group says, and redraw */
    void tick() {
        if (isFinishing()) return;
        try {
            if (gameStartedAt > 0) {
                boolean alive = gameAlive();
                if (alive) gameSeen = true;
                long age = System.currentTimeMillis() - gameStartedAt;
                if (!net.switchGame.isEmpty()) closeGame();
                if (!alive && (gameSeen || age > 20000)) {      // the game is over (or never came up)
                    MallVR.log("ONLINE game ended: " + playing);
                    noteGame(playing);
                    gameStartedAt = 0; playing = "";
                    if (!net.switchGame.isEmpty()) { String sg = net.switchGame; int sp = net.switchPod; net.switchTaken(); net.endGame(); startGame(sg, sp); }
                    else net.afterGame();
                }
            } else if (net.goPod > 0) startGame(net.game, net.goPod);
            else if (!net.switchGame.isEmpty()) { String sg = net.switchGame; int sp = net.switchPod; net.switchTaken(); startGame(sg, sp); }
            drainEvents();
            refresh();
        } catch (Exception e) { MallVR.log("ONLINE tick: " + e); }
        ui.postDelayed(this::tick, 500);
    }

    void refresh() {
        String st = net.status;
        boolean lobby = st.equals("LOBBY");
        quickBtn.setText(net.quick ? (net.countdown > 0 ? "STARTING IN " + net.countdown + " (CANCEL)" : "SEARCHING... (CANCEL)") : "QUICK MATCH");
        quickBtn.setEnabled(!ready.isEmpty() && (lobby || net.quick));
        quickGameBtn.setText(quickPick < 0 ? "ANY GAME" : ready.get(quickPick).name);
        quickGameBtn.setEnabled(!net.quick && !ready.isEmpty());
        hostBtn.setEnabled(lobby && !ready.isEmpty() && !net.quick);
        hostGameBtn.setText(hostPick >= 0 ? ready.get(hostPick).name : "NO LINKABLE GAME FOUND");
        hostGameBtn.setEnabled(hostPick >= 0 && (lobby || (st.equals("HOST") && net.inGroup())));

        List<MallOnline.Entry> all = net.everyone();
        List<MallOnline.Entry> open = new ArrayList<>();
        for (MallOnline.Entry e : all) if (e.isHost() && !e.name.equals(net.name)) open.add(e);
        StringBuilder key = new StringBuilder(st);
        for (MallOnline.Entry e : open) key.append('|').append(e.name).append(e.game).append(e.n).append(e.max).append(e.plat);
        if (!key.toString().equals(shownHosts)) {
            shownHosts = key.toString();
            hosts.removeAllViews();
            if (open.isEmpty()) hosts.addView(LauncherActivity.text(this, "NONE RIGHT NOW: HOST ONE BELOW", 14, GREY));
            for (MallOnline.Entry e : open) {
                MallVR.Game g = game(e.game);
                boolean full = e.max > 0 && e.n >= e.max, have = g != null && ready.contains(g);
                LinearLayout r = LauncherActivity.row(this); r.setBackgroundColor(PANEL); r.setPadding(dp(12), dp(4), dp(12), dp(4)); r.setGravity(Gravity.CENTER_VERTICAL);
                r.addView(LauncherActivity.text(this, (g != null ? g.name : e.game) + "   " + e.name + "  " + (e.plat.equals("QUEST") ? "[QUEST]" : "[PC]") + "   " + e.n + "/" + e.max, 16, have && !full ? CREAM : GREY),
                    new LinearLayout.LayoutParams(0, -2, 1f));
                Button j = button(full ? "FULL" : !have ? "NO GAME FILES" : "JOIN", v -> { saveName(); new Thread(() -> net.join(e)).start(); });
                j.setEnabled(lobby && have && !full && !net.quick);
                r.addView(j);
                LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, -2); lp.setMargins(0, dp(2), 0, dp(2));
                hosts.addView(r, lp);
            }
        }
        StringBuilder w = new StringBuilder();
        for (MallOnline.Entry e : all) if (!e.fromLan) w.append(w.length() > 0 ? "   " : "").append(e.name).append(e.plat.equals("QUEST") ? " (QUEST)" : "").append(e.status.equals("PLAYING") ? " (PLAYING)" : "");
        online.setText(net.listOk ? "ONLINE NOW:  " + (w.length() > 0 ? w : "NOBODY YET") : "CAN'T REACH THE GAME LIST: HOME NETWORK ONLY");

        if (lobby) { group.setText(""); groupBar.setVisibility(View.GONE); }
        else {
            List<String> r = net.roster(), rp = net.rosterPlat();
            StringBuilder names = new StringBuilder();
            for (int i = 0; i < r.size(); i++) names.append(i > 0 ? ", " : "").append(r.get(i)).append(i < rp.size() && rp.get(i).equals("QUEST") ? " (QUEST)" : "");
            MallVR.Game g = game(net.game);
            group.setText((st.equals("HOST") ? (net.inGroup() ? "YOUR GROUP: " : "YOUR GAME: ") : st.equals("GROUP") ? "YOUR GROUP: " : st.equals("PLAYING") ? "PLAYING: " : "JOINED: ")
                + (g != null ? g.name : net.game) + "   " + names);
            groupBar.setVisibility(View.VISIBLE);
            startBtn.setVisibility(st.equals("HOST") ? View.VISIBLE : View.GONE);
            startBtn.setEnabled(r.size() >= 2);
            leaveBtn.setText(st.equals("HOST") ? "CANCEL" : "LEAVE");
        }
        showVoices(lobby);
        boolean mic = checkSelfPermission(android.Manifest.permission.RECORD_AUDIO) == android.content.pm.PackageManager.PERMISSION_GRANTED;
        micLine.setText(lobby ? "" : mic ? "VOICE CHAT ON: in the game, left stick click = mic mute / live" : "VOICE: your mic is off (MallVR wasn't allowed the microphone): you hear the others, they don't hear you");
        note.setText(net.note);
        router.setText(st.equals("HOST") ? net.routerNote : "");
    }

    // ---- VOICE: per-friend volume (same files and steps as the PC's Button Masher Menu > VOICE CHAT) ----------------------
    int vol(String key) { Integer v = voiceVol.get(key); return v != null ? v : 100; }

    void loadVoice() {
        voiceVol.clear();
        try {
            for (String l : MallVR.readAll(new java.io.FileInputStream(new java.io.File(MallVR.SETTINGS, "voice_volume.txt"))).split("\r?\n")) {
                int k = l.lastIndexOf('=');
                if (k > 0) try { voiceVol.put(l.substring(0, k).trim().toUpperCase(Locale.US), Math.max(0, Math.min(200, Integer.parseInt(l.substring(k + 1).trim())))); } catch (NumberFormatException e) { }
            }
        } catch (Exception e) { }
    }

    /* the engine's file: "all <percent>" + "<pod> <percent>" per player; pod = place in the roster (host 1, then join order) */
    void writeVoicePods() {
        StringBuilder b = new StringBuilder("all " + vol("*ALL") + "\r\n");
        List<String> r = net.roster();
        for (int i = 0; i < r.size(); i++) b.append(i + 1).append(' ').append(vol(r.get(i))).append("\r\n");
        try { MallVR.write(new java.io.File(MallVR.SETTINGS, "voice_pods.txt"), b.toString()); } catch (Exception e) { }
    }

    void voiceClick(String key, char what) {
        int v = vol(key);
        if (what == '-') v = Math.max(0, v - 10);
        else if (what == '+') v = Math.min(200, v + 10);
        else if (v > 0) { voiceMuted.put(key, v); v = 0; }
        else { Integer was = voiceMuted.get(key); v = was != null && was > 0 ? was : 100; }
        voiceVol.put(key, v);
        StringBuilder b = new StringBuilder();
        for (Map.Entry<String, Integer> e : voiceVol.entrySet()) if (e.getValue() != 100) b.append(e.getKey()).append('=').append(e.getValue()).append("\r\n");
        try { MallVR.write(new java.io.File(MallVR.SETTINGS, "voice_volume.txt"), b.toString()); } catch (Exception e) { }
        writeVoicePods();
        shownVoices = "";
    }

    void showVoices(boolean lobby) {
        List<String> r = net.roster(), rp = net.rosterPlat();
        StringBuilder key = new StringBuilder(lobby ? "-" : "+");
        for (String n : r) key.append('|').append(n).append(vol(n));
        key.append('|').append(vol("*ALL"));
        if (key.toString().equals(shownVoices)) return;
        shownVoices = key.toString();
        voices.removeAllViews();
        if (lobby || r.size() < 2) return;
        voices.addView(LauncherActivity.text(this, "VOICE (only changes what you hear)", 15, GOLD));
        voiceRow("ALL VOICES", "*ALL");
        for (int i = 0; i < r.size(); i++) {
            if (r.get(i).equals(net.name)) continue;
            voiceRow(r.get(i) + (i < rp.size() && rp.get(i).equals("QUEST") ? " (QUEST)" : " (PC)"), r.get(i));
        }
    }

    void voiceRow(String label, String key) {
        LinearLayout r = LauncherActivity.row(this); r.setGravity(Gravity.CENTER_VERTICAL);
        int v = vol(key);
        r.addView(LauncherActivity.text(this, label, 15, CREAM), new LinearLayout.LayoutParams(dp(260), -2));
        r.addView(button("-", x -> voiceClick(key, '-')));
        r.addView(LauncherActivity.text(this, v + "%  ", 15, v == 100 ? CREAM : AMBER));
        r.addView(button("+", x -> voiceClick(key, '+')));
        r.addView(button(v == 0 ? "UNMUTE" : "MUTE", x -> voiceClick(key, 'm')));
        voices.addView(r);
    }

    int dp(int v) { return (int) (v * getResources().getDisplayMetrics().density + 0.5f); }
    Button button(String s, View.OnClickListener l) {
        Button b = new Button(this); b.setText(s); b.setTextSize(15); b.setAllCaps(false); b.setOnClickListener(l);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-2, -2); lp.setMargins(0, 0, dp(10), 0); b.setLayoutParams(lp);
        return b;
    }
}
