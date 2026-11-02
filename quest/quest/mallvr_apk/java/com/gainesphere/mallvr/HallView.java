package com.gainesphere.mallvr;

import android.content.Context;
import android.graphics.Canvas;
import android.view.MotionEvent;
import android.view.View;

/* The Cyberspace Hall on the launcher's panel: HallPainter draws it (about 30 frames a second while the panel shows), the
   controller's pointer moves the hover and a trigger press clicks. The hall's own clicks (sheet tabs, era dials, WANTED, picking a
   game) stay here; the rest (START, ONLINE, GET GAMES, ...) go to the launcher. */
class HallView extends View {
    interface Listener { void onHall(String id); }
    final HallPainter hall;
    final Listener listener;
    final long born = System.nanoTime();
    String down;

    HallView(Context c, HallPainter hall, Listener l) {
        super(c);
        this.hall = hall; listener = l;
        setFocusable(true);
    }

    @Override protected void onDraw(Canvas c) {
        hall.draw(c, getWidth(), getHeight(), (System.nanoTime() - born) / 1e9);
        postInvalidateDelayed(33);
    }

    @Override public boolean onHoverEvent(MotionEvent e) {
        String id = e.getActionMasked() == MotionEvent.ACTION_HOVER_EXIT ? null : hall.hit(e.getX(), e.getY());
        hall.hover = id == null ? "" : id;
        return true;
    }

    /* a click = press and release on the same thing */
    @Override public boolean onTouchEvent(MotionEvent e) {
        String id = hall.hit(e.getX(), e.getY());
        switch (e.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: listener.onHall("tap"); down = id; hall.hover = id == null ? "" : id; return true;   // "tap" = any press (silences the alarm)
            case MotionEvent.ACTION_MOVE: hall.hover = id == null ? "" : id; return true;
            case MotionEvent.ACTION_UP:
                if (id != null && id.equals(down)) { performClick(); hall.local(id); listener.onHall(id); }   // the launcher hears the hall's own clicks too (voice)
                down = null; return true;
            case MotionEvent.ACTION_CANCEL: down = null; return true;
        }
        return false;
    }

    @Override public boolean performClick() { return super.performClick(); }
}
