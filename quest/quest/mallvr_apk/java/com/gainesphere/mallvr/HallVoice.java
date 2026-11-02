package com.gainesphere.mallvr;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioAttributes;
import android.media.MediaPlayer;
import android.os.Handler;
import android.os.Looper;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/* The hall's computer voice (assets/hall/voice, quest/tools/make_hall_voice.py: Kokoro + a synth vocoder). The Quest doesn't open
   GET GAMES by itself like the PC, so when setup is missing (no file access, no games) it sounds an ALARM: klaxon + a spoken line,
   again every few seconds with the next wording, until the player taps the panel (Bobby 10-31: "like an alarm you have to silence";
   sound reaches them wherever they're looking). One-off lines: a missing game picked, welcome. Lines of a kind = <kind>_1.ogg, _2 ... */
final class HallVoice {
    final Context ctx;
    final Handler h = new Handler(Looper.getMainLooper());
    final Map<String, Integer> turn = new HashMap<>();
    MediaPlayer mp;
    String alarm;                                     // the alarm's kind while it sounds, else null

    HallVoice(Context c) { ctx = c.getApplicationContext(); }

    boolean alarming() { return alarm != null; }

    void alarm(String kind) {
        if (kind.equals(alarm)) return;
        stop();
        alarm = kind;
        next();
    }

    final Runnable again = this::next;
    void next() { if (alarm != null) play(alarm, () -> h.postDelayed(again, 4000)); }

    /* a one-off line, unless the alarm or another line is speaking */
    void say(String kind) { if (alarm == null && mp == null) play(kind, null); }

    void stop() {
        alarm = null;
        h.removeCallbacks(again);
        if (mp != null) { try { mp.stop(); } catch (Exception e) { } mp.release(); mp = null; }
    }

    List<String> lines(String kind) {
        List<String> out = new ArrayList<>();
        try { for (String f : ctx.getAssets().list("hall/voice")) if (f.matches(java.util.regex.Pattern.quote(kind) + "_\\d+\\.ogg")) out.add(f); } catch (Exception e) { }
        java.util.Collections.sort(out);
        return out;
    }

    void play(String kind, Runnable done) {
        List<String> l = lines(kind);
        if (l.isEmpty()) return;
        int n = turn.containsKey(kind) ? turn.get(kind) : 0;
        turn.put(kind, n + 1);
        String f = l.get(n % l.size());
        try {
            AssetFileDescriptor fd = ctx.getAssets().openFd("hall/voice/" + f);
            MediaPlayer m = new MediaPlayer();
            m.setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SPEECH).build());
            m.setDataSource(fd.getFileDescriptor(), fd.getStartOffset(), fd.getLength());
            fd.close();
            m.setOnCompletionListener(x -> { x.release(); if (mp == x) { mp = null; if (done != null) done.run(); } });
            m.setOnErrorListener((x, what, extra) -> { x.release(); if (mp == x) mp = null; return true; });
            m.prepare();
            m.start();
            mp = m;
        } catch (Exception e) { MallVR.log("hall voice " + f + ": " + e); mp = null; }
    }
}
