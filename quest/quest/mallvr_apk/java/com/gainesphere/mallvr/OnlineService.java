package com.gainesphere.mallvr;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import java.io.File;

/* Keeps MallVR's online play alive while a game is in front (the game runs in its own ":game" process; MallOnline lives here, in the
   launcher's process, and relays the game's frames over 127.0.0.1 exactly as the PC launcher does). A foreground service so Android
   doesn't freeze or end the launcher's process mid-game, plus the Wi-Fi locks: multicast (the LAN shout, UPnP) and high-performance
   Wi-Fi (no power-save naps between packets: linked pods wait on each other every frame). */
public class OnlineService extends Service {
    static MallOnline net;                            // one per launcher process: the lobby survives games and screens
    static WifiManager.MulticastLock mlock;
    static WifiManager.WifiLock wlock;

    /* the online object (made and started on first use), name + key from settings/online.txt */
    public static synchronized MallOnline net(Context c) {
        if (net != null) return net;
        MallOnline n = new MallOnline();
        n.listUrl = setting("server", "https://downloads.gainesphere.com/mallvr/online/list.php");
        n.version = MallVR.version(c);
        n.platform = "QUEST";
        String key = setting("key", "");
        if (!key.matches("[0-9a-f]{32}")) { key = MallOnline.newKey(); saveSetting("key", key); }
        n.key = key;
        String nm = MallOnline.clean(setting("name", ""), 12);
        n.name = nm.isEmpty() ? "PLAYER" + (100 + new java.util.Random().nextInt(900)) : nm;
        net = n;
        return n;
    }

    public static void begin(Context c) {
        net(c);
        Intent i = new Intent(c, OnlineService.class);
        if (android.os.Build.VERSION.SDK_INT >= 26) c.startForegroundService(i); else c.startService(i);
    }

    /* leaving the ONLINE screen for good: off the list, locks released, service gone */
    public static void end(Context c) {
        if (net != null) new Thread(() -> { MallOnline n = net; if (n != null) n.stop(); }).start();
        net = null;
        c.stopService(new Intent(c, OnlineService.class));
    }

    @Override public int onStartCommand(Intent intent, int flags, int id) {
        try {
            NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
            Notification.Builder b;
            if (android.os.Build.VERSION.SDK_INT >= 26) {
                nm.createNotificationChannel(new NotificationChannel("online", "MallVR online", NotificationManager.IMPORTANCE_LOW));
                b = new Notification.Builder(this, "online");
            } else b = new Notification.Builder(this);
            b.setContentTitle("MallVR online").setContentText("In the online lobby").setSmallIcon(android.R.drawable.stat_sys_upload).setOngoing(true);
            startForeground(27200, b.build());
        } catch (Exception e) { MallVR.log("online service notification: " + e); }
        try {
            WifiManager w = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            if (mlock == null) { mlock = w.createMulticastLock("MallVR online"); mlock.setReferenceCounted(false); mlock.acquire(); }
            if (wlock == null) { wlock = w.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "MallVR online"); wlock.setReferenceCounted(false); wlock.acquire(); }
        } catch (Exception e) { MallVR.log("online wifi locks: " + e); }
        MallOnline n = net(this);
        new Thread(n::start).start();
        MallVR.log("online on: " + n.name + ", list " + n.listUrl + ", lan " + n.lanIp);
        return START_NOT_STICKY;
    }

    @Override public void onDestroy() {
        try { if (mlock != null) mlock.release(); if (wlock != null) wlock.release(); } catch (Exception e) { }
        mlock = null; wlock = null;
        MallVR.log("online off");
        super.onDestroy();
    }

    @Override public IBinder onBind(Intent i) { return null; }

    // settings/online.txt: "name = BOBBY", "key = <32 hex>" (the player key, never shown), "server = <url>" (dev: the list mock)
    static String setting(String k, String def) {
        try {
            for (String l : MallVR.readAll(new java.io.FileInputStream(new File(MallVR.SETTINGS, "online.txt"))).split("\r?\n")) {
                int e = l.indexOf('=');
                if (e > 0 && l.substring(0, e).trim().equalsIgnoreCase(k)) return l.substring(e + 1).trim();
            }
        } catch (Exception e) { }
        return def;
    }
    static synchronized void saveSetting(String k, String v) {
        StringBuilder b = new StringBuilder();
        boolean done = false;
        try {
            for (String l : MallVR.readAll(new java.io.FileInputStream(new File(MallVR.SETTINGS, "online.txt"))).split("\r?\n")) {
                if (l.trim().isEmpty()) continue;
                int e = l.indexOf('=');
                if (e > 0 && l.substring(0, e).trim().equalsIgnoreCase(k)) { b.append(k).append(" = ").append(v).append('\n'); done = true; }
                else b.append(l).append('\n');
            }
        } catch (Exception e) { }
        if (!done) b.append(k).append(" = ").append(v).append('\n');
        try { MallVR.write(new File(MallVR.SETTINGS, "online.txt"), b.toString()); } catch (Exception e) { }
    }
}
