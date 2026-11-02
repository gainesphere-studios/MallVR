package com.gainesphere.mallvr;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioAttributes;
import android.media.MediaPlayer;
import java.util.ArrayList;
import java.util.Collections;

/* The hall's menu songs (the PC's MusicSync): a shuffled playlist of assets/hall/music/*.ogg (package/engine/hall/music/*.wav,
   made into OGG by build_mallvr_apk.sh). Each song plays once to its ending, then the next; all of them before any repeats.
   Plays while the launcher is in front with MUSIC on; stops when a game starts. */
final class HallMusic {
    final Context ctx;
    final ArrayList<String> playlist = new ArrayList<>();
    MediaPlayer mp;
    boolean want;

    HallMusic(Context c) { ctx = c.getApplicationContext(); }

    void set(boolean on) {
        want = on;
        if (on && mp == null) next();
        else if (!on && mp != null) { try { mp.stop(); } catch (Exception e) { } mp.release(); mp = null; }
    }

    void next() {
        try {
            if (playlist.isEmpty()) {
                String[] f = ctx.getAssets().list("hall/music");
                if (f != null) for (String s : f) if (s.endsWith(".ogg")) playlist.add(s);
                Collections.shuffle(playlist);
            }
            if (playlist.isEmpty()) return;
            String song = playlist.remove(0);
            AssetFileDescriptor fd = ctx.getAssets().openFd("hall/music/" + song);
            MediaPlayer m = new MediaPlayer();
            m.setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build());
            m.setDataSource(fd.getFileDescriptor(), fd.getStartOffset(), fd.getLength());
            fd.close();
            m.setOnCompletionListener(x -> { x.release(); if (mp == x) { mp = null; if (want) next(); } });
            m.setOnErrorListener((x, what, extra) -> { MallVR.log("menu music error " + what + "/" + extra + " in " + song); x.release(); if (mp == x) mp = null; return true; });
            m.prepare();
            m.start();
            mp = m;
        } catch (Exception e) { MallVR.log("menu music: " + e); mp = null; }
    }
}
