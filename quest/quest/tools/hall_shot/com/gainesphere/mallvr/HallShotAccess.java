package com.gainesphere.mallvr;

import android.graphics.Canvas;
import java.io.*;
import java.util.Set;

/* hall_shot: the hall as the launcher fills it (the games of games.json, some found, some missing) */
public class HallShotAccess {
    public static void draw(File dir, Canvas c, int w, int h, Set<String> opt) {
        HallData d = new HallData(n -> new FileInputStream(new File(dir, n)), null);
        d.loadPods(HallPainter.ERA_PODS);
        HallPainter p = new HallPainter(d);
        String[][] games = { { "DAC", "Dactyl Nightmare" }, { "DN2", "Dactyl Nightmare 2" }, { "PAC", "Pac-Man VR" }, { "MISS", "Missile Command VR" },
            { "GUN", "Olin Shotgun" }, { "BOX", "Virtuality Boxing" }, { "ZONE", "Zone Hunter" }, { "GHOST", "Ghost Train" }, { "SFL", "Shoot For Loot" },
            { "GUN3K", "Total Recoil" }, { "ZERO3K", "Zero Hour" }, { "ZENECA3K", "Zeneca Migraine" }, { "BALL", "Buggy Ball" }, { "PEARL", "Sea Diving (Pearl)" } };
        for (int i = 0; i < games.length; i++) {
            HallPainter.Item it = new HallPainter.Item();
            it.code = games[i][0]; it.name = LauncherActivity.hallName(games[i][1]);
            it.state = i == 2 ? 3 : i == 4 ? 2 : i < 9 ? 1 : 0;
            if (it.state == 3) it.why = "no PACMAN.CUE in games/PAC";
            p.items.add(it);
        }
        p.game = opt.contains("machine") ? 9 : opt.contains("pac") ? 2 : 0;
        p.store = opt.contains("store");
        p.needFiles = opt.contains("nofiles");
        p.checked = !p.needFiles;
        p.notesOn = true; p.version = "0.3.3-quest";
        p.status = p.needFiles ? "MallVR keeps games, logs and settings in the MallVR folder of the headset's storage (like the PC version's folder). Press ALLOW FILE ACCESS, switch MallVR on, then come back."
            : "8 of 14 games ready. START puts you in the game. Back here: hold A + B for a second.";
        if (opt.contains("wanted")) p.local("wanted");
        if (opt.contains("machine")) p.local("tab0");
        if (opt.contains("hover")) p.hover = "g3";
        if (opt.contains("alarm") || opt.contains("card2")) { for (HallPainter.Item it : p.items) it.state = 0; p.alarm = opt.contains("alarm"); p.status = "No games found yet. Press GET GAMES to download them from archive.org."; }
        p.setupSince = 0;                             // the setup hexes have flown in by t = 2
        if (opt.contains("card1")) p.local("step1");
        if (opt.contains("card2")) p.local("step2");
        p.draw(c, w, h, 2.0);
    }
}
