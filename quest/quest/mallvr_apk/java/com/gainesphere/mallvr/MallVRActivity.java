package com.gainesphere.mallvr;

import android.content.Intent;
import android.os.Bundle;
import org.libsdl.app.SDLActivity;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.HashMap;
import java.util.Map;

/* MallVR for Meta Quest: one game, immersive (OpenXR through vpix_xr.h), in its own process (":game"), so every game starts on
   a fresh DOSBox-X + SDL and quitting can't take the launcher down with it.
   The launcher passes the conf path + the game's env (MallVR.plan). Back to the launcher = the PC's Button Masher Menu gesture:
   hold A + B for a second. The engine (vpix_menu.h) sends "MENU <game>" to VPIX_MENU_PORT; here nothing is drawn over the game
   yet, so we answer "QUIT" at once (= the PC's BACK TO MENU) and bring the launcher back.
   Online (crossplay): started by OnlineActivity as pod <n> ("back" = "online": we return to the ONLINE screen, still in the group).
   It sends "QUIT" to 127.0.0.1:27123 when the group switches games; we pass it to the engine, which closes, and it starts the next. */
public class MallVRActivity extends SDLActivity {
    private static String conf = "";
    private DatagramSocket menu, control;
    private volatile DatagramPacket engine;          // where the engine's menu lines come from (for QUIT)
    private boolean online;

    @Override protected void onCreate(Bundle b) {
        Intent it = getIntent();
        conf = it.getStringExtra("conf");
        online = "online".equals(it.getStringExtra("back"));
        @SuppressWarnings("unchecked") HashMap<String, String> env = (HashMap<String, String>) it.getSerializableExtra("env");
        try {
            if (env != null) for (Map.Entry<String, String> e : env.entrySet()) android.system.Os.setenv(e.getKey(), e.getValue(), true);
            menu = new DatagramSocket(0, InetAddress.getByName("127.0.0.1"));
            android.system.Os.setenv("VPIX_MENU_PORT", "" + menu.getLocalPort(), true);
            Thread t = new Thread(this::menuLoop, "MallVR menu"); t.setDaemon(true); t.start();
            if (online) {
                control = new DatagramSocket(OnlineActivity.GAME_CONTROL, InetAddress.getByName("127.0.0.1"));
                Thread c = new Thread(this::controlLoop, "MallVR control"); c.setDaemon(true); c.start();
            }
        } catch (Exception e) { MallVR.log("game setup: " + e); }
        String game = it.getStringExtra("game");
        String tag = it.getStringExtra("tag");      // online: dac_pod2 (the logs' name) + the pod in the start line
        if (game != null) PlayNotes.gameState(MallVR.LOGS, "start " + game + " " + (tag != null ? tag : game.toLowerCase(java.util.Locale.US))
            + (it.getIntExtra("pod", 0) > 0 ? " " + it.getIntExtra("pod", 0) : ""));
        MallVR.log("game start: " + conf);
        super.onCreate(b);
    }

    private void menuLoop() {
        byte[] buf = new byte[256];
        while (true) {
            try {
                DatagramPacket p = new DatagramPacket(buf, buf.length);
                menu.receive(p);
                String line = new String(p.getData(), 0, p.getLength(), "US-ASCII");
                engine = new DatagramPacket(new byte[0], 0, p.getAddress(), p.getPort());
                if (line.startsWith("MENU")) {
                    MallVR.log("A + B held in game: back to the launcher");
                    PlayNotes.gameState(MallVR.LOGS, "end back_to_menu");
                    toLauncher();
                    byte[] q = "QUIT".getBytes("US-ASCII");
                    menu.send(new DatagramPacket(q, q.length, p.getAddress(), p.getPort()));     // the engine exits (vpix_menu.h)
                }
            } catch (Exception e) { return; }
        }
    }

    /* the ONLINE screen: QUIT = the group switched games (the engine closes; onDestroy takes us back there) */
    private void controlLoop() {
        byte[] buf = new byte[64];
        while (true) {
            try {
                DatagramPacket p = new DatagramPacket(buf, buf.length);
                control.receive(p);
                if (!p.getAddress().isLoopbackAddress()) continue;
                String line = new String(p.getData(), 0, p.getLength(), "US-ASCII");
                DatagramPacket e = engine;
                if (line.startsWith("QUIT") && e != null) {
                    MallVR.log("group switch: closing the game");
                    PlayNotes.gameState(MallVR.LOGS, "end switch");
                    byte[] q = "QUIT".getBytes("US-ASCII");
                    menu.send(new DatagramPacket(q, q.length, e.getAddress(), e.getPort()));
                }
            } catch (Exception x) { return; }
        }
    }

    private void toLauncher() {
        try {
            Intent i = new Intent(this, online ? OnlineActivity.class : LauncherActivity.class);
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
            startActivity(i);
        } catch (Exception e) { }
    }

    @Override protected String[] getLibraries() { return new String[] { "c++_shared", "openxr_loader", "SDL2", "main" }; }
    @Override protected String[] getArguments() { return new String[] { "-conf", conf, "-nopromptfolder" }; }

    /* the game ended (its EXE quit, or the system closed us): back to the launcher, and end this process (SDL can't start twice) */
    @Override protected void onDestroy() {
        MallVR.log("game closed");
        PlayNotes.gameState(MallVR.LOGS, "end closed");
        toLauncher();
        super.onDestroy();
        android.os.Process.killProcess(android.os.Process.myPid());
    }
}
