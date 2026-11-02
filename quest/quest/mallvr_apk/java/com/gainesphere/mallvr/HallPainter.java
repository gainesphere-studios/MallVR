package com.gainesphere.mallvr;

import android.graphics.*;
import java.util.ArrayList;
import java.util.List;

/* MallVR CYBERSPACE HALL on the Quest: the PC's game select hall (package/engine/MallHall.cs, Bobby's pick 10-30) drawn with
   android.graphics. Lawnmower Man sky, the fly-over grid, games as data blocks, the pod turning under the museum spotlight, the
   MACHINE / GAME / HOW TO sheets, the era dials, the pillbug's news and the WANTED board, on the same 352 x 288 grid as the PC (and
   its numbers, so a change there is easy to bring over). The PC's MODE / PLAYERS rows (headset only here) become the selected game's
   state and the launcher's status; the bottom bar has the Quest buttons. No View / Context in here: HallView puts it on screen and
   quest/tools/hall_shot draws it on the PC for checking. */
final class HallPainter {
    static final int W = 352, H = 288;
    static final String[] ERA_PODS = { "1000CS", "1000SD", "2000SU", "2000SD", "SU3000" };
    static final int CYAN = 0xFF29E7FF, PINK = 0xFFFF3FD8, CREAM = 0xFFF4F1E8, YELLOW = 0xFFFFFF00, RED = 0xFFFF2020;

    static final class Item { String code, name, why = ""; int state; }   // a game: MallVR.Game's state (0 missing, 1 READY, 2 READY?, 3 needs CD)
    static final class Hit { final RectF r; final String id; Hit(RectF r, String id) { this.r = r; this.id = id; } }

    final HallData data;
    // set by the launcher
    List<Item> items = new ArrayList<>();
    int game;
    boolean store, needFiles, checked, notesOn, music = true, alarm;   // alarm = the voice alarm is sounding (HallVoice)
    String status = "", version = "";
    // the hall's own state
    int sheetTab = 1;                                 // 0 THE MACHINE / 1 THE GAME / 2 HOW TO PLAY
    String viewPod;                                   // a pod picked on the era dials (null = the selected game's pod)
    boolean wanted;                                   // the WANTED board is open
    String hover = "";
    List<Hit> hits = new ArrayList<>();
    private List<Hit> building;
    float scale = 1, ox, oy;                          // the last frame's fit of the 352 x 288 grid into the view

    // GDI+'s fonts as Android ones: size in grid units, where the baseline sits below the text's top, line height (both in ems)
    static final class F {
        final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG); final float size, asc, line;
        F(Typeface t, float size, float asc, float line) { p.setTypeface(t); p.setTextSize(size); p.setSubpixelText(true); this.size = size; this.asc = asc; this.line = line; }
    }
    static final Typeface SANS = Typeface.create(Typeface.SANS_SERIF, Typeface.NORMAL), SANS_B = Typeface.create(Typeface.SANS_SERIF, Typeface.BOLD),
        SERIF = Typeface.create(Typeface.SERIF, Typeface.NORMAL), SERIF_B = Typeface.create(Typeface.SERIF, Typeface.BOLD);
    final F hTitle = new F(SANS_B, 8.5f, 1.08f, 1.33f), hBlock = new F(SANS_B, 7f, 1.08f, 1.33f), hSmall = new F(SANS_B, 6f, 1.08f, 1.33f),
        hTiny = new F(SANS, 4.6f, 1.08f, 1.33f), hInfo = new F(SANS, 5.2f, 1.08f, 1.25f), hSheetT = new F(SANS_B, 8f, 1.08f, 1.2f),
        hSheet = new F(SERIF, 6.1f, 0.92f, 1.14f), hTimes = new F(SERIF_B, 10f, 0.89f, 1.15f), hTimesS = new F(SERIF_B, 7.5f, 0.89f, 1.15f),
        hCyber = new F(SANS_B, 14f * 0.55f, 1.0f, 1.2f);
    final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG), line = new Paint(Paint.ANTI_ALIAS_FLAG), img = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    Shader sky, glow;

    HallPainter(HallData data) {
        this.data = data;
        fill.setStyle(Paint.Style.FILL); line.setStyle(Paint.Style.STROKE);
    }

    String gamePod(int i) {
        if (i < 0 || i >= items.size()) return "2000SU";
        String p = data.get("game " + items.get(i).code, "pod");
        return p.isEmpty() ? "2000SU" : p;
    }
    boolean found(int i) { return checked && !needFiles && i >= 0 && i < items.size() && (items.get(i).state == 1 || items.get(i).state == 2); }
    boolean anyFound() { for (int i = 0; i < items.size(); i++) if (found(i)) return true; return false; }

    /* a game picked: back to its own pod and sheet */
    void select(int i) { if (i != game) { game = i; wanted = false; viewPod = null; } }

    /* the hall's own clicks (true = handled here); the rest go to the launcher */
    boolean local(String id) {
        if (id.startsWith("tab")) { sheetTab = id.charAt(3) - '0'; return true; }
        if (id.startsWith("pod")) { viewPod = ERA_PODS[id.charAt(3) - '0']; sheetTab = 0; wanted = false; return true; }
        if (id.equals("wanted")) { wanted = !wanted; card = 0; return true; }
        if (id.startsWith("step")) { card = id.charAt(4) - '0'; wanted = false; return true; }
        if (id.equals("cardclose")) { card = 0; return true; }
        if (id.startsWith("g")) { select(Integer.parseInt(id.substring(1))); return true; }
        return false;
    }

    /* view pixels -> the id under them (null = nothing) */
    String hit(float x, float y) {
        float gx = (x - ox) / scale, gy = (y - oy) / scale;
        for (Hit h : hits) if (h.r.contains(gx, gy)) return h.id;
        return null;
    }

    // ---- drawing ------------------------------------------------------------------------------------------------------------
    void draw(Canvas c, int vw, int vh, double t) {
        if (game >= items.size()) game = 0;
        scale = Math.min(vw / (float) W, vh / (float) H);
        ox = (vw - W * scale) / 2; oy = (vh - H * scale) / 2;
        float L = -ox / scale, R = W + ox / scale, T = -oy / scale, B = H + oy / scale;   // the whole view, in grid units
        String pod = viewPod != null ? viewPod : gamePod(game);
        int blink = (int) (t * 2);
        building = new ArrayList<>();

        c.save();
        c.translate(ox, oy); c.scale(scale, scale);
        drawSky(c, L, T, R, B);
        drawMotion(c, t, L, T, R, B);
        boolean setup = setupShown();
        if (!setup) { setupSince = -1; card = 0; }
        else if (setupSince < 0) setupSince = t;
        if (setup && !wanted) drawSetup(c, t);
        else if (!wanted) drawExhibit(c, pod, t);
        Bitmap bug = data.bug;                        // the pillbug bobs beside its news
        if (bug != null) bitmap(c, bug, null, new RectF(5, 236 - (float) Math.abs(Math.sin(t * 2.2)) * 1.2f, 33, 236 - (float) Math.abs(Math.sin(t * 2.2)) * 1.2f + 28f * bug.getHeight() / bug.getWidth()));

        Bitmap logo = data.logo;
        if (logo != null) bitmap(c, logo, null, new RectF(5, 3, 55, 3 + 50f * logo.getHeight() / logo.getWidth()));
        text(c, "MALLVR  ·  lo-fi VR", 60, 9, hSmall, 0xFFFF8FF0, null);
        if (!version.isEmpty()) text(c, "QUEST " + version.replace("-quest", ""), 60, 17, hTiny, 0xFF8C78B4, null);
        text(c, "GAME SELECT", 346 - measure("GAME SELECT", hTimes), 5, hTimes, YELLOW, null);
        String credit = "POD PHOTOS: SIMON MARSTON COLLECTION";
        text(c, credit, 346 - measure(credit, hTiny), 17, hTiny, 0xFF7896AF, null);
        drawBlocks(c);
        if (wanted) drawWanted(c);
        else if (card > 0) drawCard(c, t, blink);
        else { drawSheets(c, pod); drawEra(c, pod); }
        drawPanel(c, blink);
        drawNews(c);
        drawBar(c, blink, L, R, B);
        c.restore();
        hits = building;
    }

    void drawSky(Canvas c, float L, float T, float R, float B) {
        fill.setShader(null); fill.setColor(0xFF020208);
        c.drawRect(L, T, R, B, fill);
        // the PC's path gradient: an ellipse around (176, 60), 296 x 210, brightest at (176, 96)
        if (sky == null) sky = new RadialGradient(0, 0, 296, new int[] { 0xFF341870, 0xFF1B0E3C, 0xFF020208 }, new float[] { 0f, 0.55f, 1f }, Shader.TileMode.CLAMP);
        c.save();
        c.translate(176, 90); c.scale(1, 210f / 296f);
        fill.setShader(sky);
        c.drawRect(L - 176, (T - 90) * 296f / 210f, R - 176, (B - 90) * 296f / 210f, fill);
        fill.setShader(null);
        c.restore();
    }

    // the grid you glide over and the floating wireframe geometry
    void drawMotion(Canvas c, double t, float L, float T, float R, float B) {
        final float hy = 150, vx = 186;
        float phase = (float) (t * 0.6 % 1.0);
        float yb = Math.max(H, B) + 10, yt = Math.min(0, T) - 10;
        stroke(alpha(110, CYAN), 0.5f);
        for (int k = -14; k <= 14; k++) c.drawLine(vx + k * 3, hy, vx + k * 3 + k * 43 * (yb - hy) / (H + 10 - hy), yb, line);
        for (int i = 1; i < 24; i++) {
            float z = i - phase; if (z < 0.6f) continue;
            float y = hy + 120f / z;
            if (y > B) continue;
            stroke(alpha((int) Math.min(150, 20 + (y - hy) * 1.4f), CYAN), 0.5f);
            c.drawLine(L, y, R, y, line);
        }
        stroke(alpha(28, PINK), 0.5f);
        for (int k = -14; k <= 14; k += 2) c.drawLine(vx + k * 3, hy - 40, vx + k * 3 + k * 43 * (hy - 40 - yt) / (hy - 30), yt, line);
        stroke(alpha(22, PINK), 0.5f);
        for (int i = 1; i < 14; i++) {
            float z = i - phase; if (z < 0.6f) continue;
            float y = hy - 40 - 90f / z;
            if (y >= T) c.drawLine(L, y, R, y, line);
        }
        float[][] geo = { { 140, 46, 7, 1.0f }, { 236, 40, 6, -0.7f }, { 244, 132, 4, 1.4f }, { 132, 120, 3.5f, -1.1f } };
        stroke(0x96A0FFFF, 0.5f);
        for (int i = 0; i < geo.length; i++) {
            float cx = geo[i][0], cy = geo[i][1] + (float) Math.sin(t * 0.9 + i) * 2, r = geo[i][2];
            double a0 = t * geo[i][3];
            Path p = new Path();
            for (int j = 0; j < 4; j++) {
                float px = cx + (float) Math.cos(a0 + j * Math.PI / 2) * r, py = cy + (float) Math.sin(a0 + j * Math.PI / 2) * r * 0.9f;
                if (j == 0) p.moveTo(px, py); else p.lineTo(px, py);
            }
            p.close();
            c.drawPath(p, line);
        }
    }

    // the games as glowing data blocks on the left
    void drawBlocks(Canvas c) {
        int n = Math.max(1, items.size());
        float top = 30, rowH = Math.min(13f, 200f / (n + 1)), bh = rowH - 1.6f;   // + 1 row: WANTED
        if (items.isEmpty()) text(c, "NO GAMES YET", 10, top, hBlock, RED, null);
        for (int i = 0; i < items.size(); i++) {
            Item g = items.get(i);
            boolean sel = i == game, hov = hover.equals("g" + i), ok = found(i);
            float y = top + i * rowH, x = sel ? 12 : 6, w = 120;
            RectF r = new RectF(x, y, x + w, y + bh);
            if (sel) {
                fill.setShader(new LinearGradient(r.left, 0, r.right, 0, CYAN, 0xFF1A6AFF, Shader.TileMode.CLAMP)); c.drawRect(r, fill); fill.setShader(null);
                c.drawRect(x - 1.5f, y - 1.5f, x + w + 1.5f, y + bh + 1.5f, solid(alpha(70, CYAN)));
                c.drawRect(x, y, x + 2, y + bh, solid(0xFFFFFFFF));
            } else {
                fill.setShader(new LinearGradient(r.left, 0, r.right, 0, hov ? 0xFF224678 : 0xFF0F2A4A, 0xFF0A1430, Shader.TileMode.CLAMP)); c.drawRect(r, fill); fill.setShader(null);
                stroke(Color.argb(hov ? 200 : 90, 40, 110, 170), 0.4f); c.drawRect(r, line);
                c.drawRect(x, y, x + 1.5f, y + bh, solid(ok ? CYAN : 0xFF3C5064));
            }
            int tc = sel ? 0xFF000014 : !ok ? 0xFF5A6E82 : 0xFFDCF5FF;
            text(c, g.name, x + 5, y + (bh - 8.5f) / 2, hBlock, tc, null);
            String tag = !checked || needFiles ? "" : g.state == 1 ? gamePod(i) : g.state == 2 ? "READY?" : g.state == 3 ? "NEEDS CD" : store ? "NOT FOUND" : "GET GAMES";
            text(c, tag, x + w - measure(tag, hTiny) - 2, y + (bh - 6) / 2, hTiny, sel ? 0xFF00143C : 0xFF8CAAC8, null);
            building.add(new Hit(r, "g" + i));
        }
        // the last block: WANTED, the games and CD audio nobody has found yet
        float wy = top + items.size() * rowH, wx = wanted ? 12 : 6;
        boolean wh = hover.equals("wanted");
        RectF wr = new RectF(wx, wy, wx + 120, wy + bh);
        c.drawRect(wr, solid(wanted ? PINK : alpha(wh ? 90 : 50, PINK)));
        stroke(PINK, 0.5f); c.drawRect(wr, line);
        text(c, "WANTED: GAMES + MUSIC", wx + 5, wy + (bh - 8.5f) / 2, hBlock, wanted ? 0xFF1E001E : 0xFFFFC8F5, null);
        building.add(new Hit(wr, "wanted"));
    }

    // WANTED: a wide exhibit board over the spotlight (hall_info.txt [wanted])
    void drawWanted(Canvas c) {
        RectF s = new RectF(134, 26, 346, 186);
        c.drawRect(s.left + 2, s.top + 3, s.right + 2, s.bottom + 3, solid(0x6E000000));
        c.drawRect(s, solid(CREAM));
        stroke(PINK, 1f); c.drawRect(s.left + 2, s.top + 2, s.right - 2, s.bottom - 2, line);
        float x = s.left + 7, y = s.top + 6, sw = s.width();
        text(c, "WANTED", x, y, hTiny, 0xFFA01890, null);
        text(c, data.get("wanted", "title"), x, y + 6, hSheetT, 0xFF1A1A1A, null);
        y += 17;
        box(c, data.get("wanted", "text"), hTiny, 0xFF282828, x, y, sw - 14, 14);
        y += 15;
        float colW = (sw - 18) / 2;
        float[] cy = { y, y };
        for (int i = 1; i <= 6; i++) {
            String l = data.get("wanted", "list" + i);
            if (l.isEmpty()) continue;
            int col = cy[0] <= cy[1] ? 0 : 1;
            int k = l.indexOf(':');
            String head = k > 0 ? l.substring(0, k).trim() : "", list = k > 0 ? l.substring(k + 1).trim() : l;
            float cx = x + col * (colW + 4);
            text(c, head, cx, cy[col], hTiny, 0xFFA01890, null);
            float zh = boxHeight(list, hSheet, colW);
            box(c, list, hSheet, 0xFF1E1E1E, cx, cy[col] + 6, colW, zh + 1);
            cy[col] += 6 + zh + 3;
        }
        float hy = Math.max(cy[0], cy[1]);
        String m = data.get("wanted", "music");
        if (!m.isEmpty()) {
            stroke(alpha(120, PINK), 0.5f); c.drawLine(x, hy, s.right - 7, hy, line);
            int k = m.indexOf(':');
            String head = k > 0 ? m.substring(0, k).trim() : "", list = k > 0 ? m.substring(k + 1).trim() : m;
            text(c, head, x, hy + 3, hTiny, 0xFFA01890, null);
            float zh = boxHeight(list, hSheet, sw - 14);
            box(c, list, hSheet, 0xFF1E1E1E, x, hy + 9, sw - 14, zh + 1);
            hy += 9 + zh + 3;
        }
        hy = Math.max(hy, s.bottom - 20);
        box(c, data.get("wanted", "help"), hSmall, 0xFF780A64, x, hy, sw - 14, s.bottom - hy - 3);
    }

    // ---- SETUP (SETUP_GUIDE_PLAN.md phase 1): while file access or every game is missing, the spotlight becomes a wall of hex
    // cells (the 1994 Lawnmower Man game's look) and the setup steps fly out of it as glowing hex buttons: 1 FILE ACCESS,
    // 2 GET GAMES (store build: COPY GAMES), 3 PLAY. Done = cyan with a tick, the step to do now pulses red. A tap opens the step's
    // tutorial card (hall_info.txt [setup ...]) and the voice reads it.
    int card;                                         // 0 none, 1..3 = that step's tutorial card is open
    double setupSince = -1;                           // when the hexes started flying in (s)
    boolean setupShown() { return needFiles || (checked && !anyFound()); }
    int setupStep() { return needFiles ? 1 : 2; }     // the step to do now
    String stepKey(int n) { return n == 1 ? "files" : n == 2 ? (store ? "copygames" : "getgames") : "play"; }

    static Path hex(float cx, float cy, float r) {   // pointy-top hexagon
        Path p = new Path();
        for (int k = 0; k < 6; k++) {
            double a = Math.PI / 3 * k - Math.PI / 2;
            float x = cx + (float) Math.cos(a) * r, y = cy + (float) Math.sin(a) * r;
            if (k == 0) p.moveTo(x, y); else p.lineTo(x, y);
        }
        p.close();
        return p;
    }

    void drawSetup(Canvas c, double t) {
        // the hex wall, faded out at the edge of the old spotlight
        RectF wr = new RectF(116, 18, 260, 156);
        float cx0 = wr.centerX(), cy0 = wr.centerY(), r = 6.5f, w = (float) (Math.sqrt(3) * r);
        c.save();
        Path oval = new Path(); oval.addOval(wr, Path.Direction.CW); c.clipPath(oval);
        for (int row = 0; ; row++) {
            float y = wr.top - r + row * 1.5f * r;
            if (y > wr.bottom + r) break;
            for (int col = 0; ; col++) {
                float x = wr.left - w + col * w + (row % 2 == 1 ? w / 2 : 0);
                if (x > wr.right + w) break;
                float dx = (x - cx0) / (wr.width() / 2), dy = (y - cy0) / (wr.height() / 2);
                float fade = (float) Math.max(0, 1 - Math.sqrt(dx * dx + dy * dy));
                if (fade <= 0) continue;
                int hsh = (row * 7 + col * 13 + row * col) % 17;
                boolean red = hsh == 3 || hsh == 11;
                float glow = red ? (float) (0.6 + 0.4 * Math.sin(t * 2 + row + col)) : 1;
                c.drawPath(hex(x, y, r * 0.86f), solid(alpha((int) (fade * (red ? 210 * glow : 170)), red ? 0xFFE0283C : 0xFF3A4CE0)));
                stroke(alpha((int) (fade * 120), 0xFF9AB0FF), 0.3f); c.drawPath(hex(x, y, r * 0.86f), line);
            }
        }
        c.restore();
        String head = "SET UP YOUR POD  ·  TAP A HEX";
        boolean column = card > 0;                    // a card open: the hexes stack down the left, the card beside them
        if (!column) text(c, head, cx0 - measure(head, hSmall) / 2, 24, hSmall, 0xFFFFFFFF, null);
        for (int i = 0; i < 3; i++) {
            float hr = column ? 14 : 17;
            float cx = column ? 152 : 148 + i * 40, cy = column ? 52 + i * 44 : 82;
            float p = (float) Math.max(0, Math.min(1, (t - setupSince - i * 0.25) / 0.6));
            if (p <= 0) continue;
            float s = (float) (1 + 2.7 * Math.pow(p - 1, 3) + 1.7 * Math.pow(p - 1, 2));   // ease out with a little overshoot
            int n = i + 1;
            boolean done = n < setupStep(), now = n == setupStep(), open = card == n, hov = hover.equals("step" + n);
            float pulse = (float) (0.5 + 0.5 * Math.sin(t * 5));
            int col = done ? CYAN : now ? RED : 0xFF7A8CC8;
            c.save();
            c.translate(cx, cy + (1 - p) * 24); c.scale(s, s);
            stroke(alpha(now ? (int) (60 + 120 * pulse) : open || hov ? 150 : 70, col), 4f); c.drawPath(hex(0, 0, hr), line);
            c.drawPath(hex(0, 0, hr), solid(alpha(open || hov ? 170 : now ? (int) (90 + 70 * pulse) : 110, now ? 0xFF500818 : 0xFF0A1440)));
            stroke(col, 1.1f); c.drawPath(hex(0, 0, hr), line);
            stroke(alpha(90, 0xFFFFFFFF), 0.4f); c.drawPath(hex(0, 0, hr * 0.8f), line);
            icon(c, done ? "done" : stepKey(n), hr * 0.5f, done ? CYAN : 0xFFFFFFFF);
            text(c, "" + n, -hr * 0.72f, -hr * 0.78f, hSmall, col, null);
            c.restore();
            String cap = n == 1 ? "FILE ACCESS" : n == 2 ? (store ? "COPY GAMES" : "GET GAMES") : "PLAY";
            if (p > 0.6f) text(c, cap, cx - measure(cap, hTiny) / 2, cy + hr + 1.5f, hTiny, col, null);
            building.add(new Hit(new RectF(cx - hr, cy - hr, cx + hr, cy + hr + 8), "step" + n));
        }
    }

    // small line icons inside the hexes, centred on 0,0, size = half their width
    void icon(Canvas c, String k, float z, int col) {
        stroke(col, 1.1f);
        Path p = new Path();
        switch (k) {
            case "files":                             // a folder
                p.moveTo(-z, -z * 0.55f); p.lineTo(-z * 0.3f, -z * 0.55f); p.lineTo(-z * 0.1f, -z * 0.3f); p.lineTo(z, -z * 0.3f);
                p.lineTo(z, z * 0.7f); p.lineTo(-z, z * 0.7f); p.close(); break;
            case "getgames":                          // download: an arrow onto a tray
                p.moveTo(0, -z); p.lineTo(0, z * 0.35f); p.moveTo(-z * 0.45f, -z * 0.1f); p.lineTo(0, z * 0.35f); p.lineTo(z * 0.45f, -z * 0.1f);
                p.moveTo(-z, z * 0.3f); p.lineTo(-z, z * 0.8f); p.lineTo(z, z * 0.8f); p.lineTo(z, z * 0.3f); break;
            case "copygames":                         // a PC screen, an arrow to the headset
                p.moveTo(-z, -z * 0.7f); p.lineTo(-z * 0.15f, -z * 0.7f); p.lineTo(-z * 0.15f, 0); p.lineTo(-z, 0); p.close();
                p.moveTo(-z * 0.58f, 0); p.lineTo(-z * 0.58f, z * 0.3f);
                p.moveTo(-z * 0.05f, z * 0.35f); p.lineTo(z * 0.45f, z * 0.35f); p.moveTo(z * 0.25f, z * 0.15f); p.lineTo(z * 0.45f, z * 0.35f); p.lineTo(z * 0.25f, z * 0.55f);
                p.moveTo(z * 0.3f, -z * 0.55f); p.lineTo(z, -z * 0.55f); p.lineTo(z, -z * 0.1f); p.lineTo(z * 0.3f, -z * 0.1f); p.close(); break;
            case "play":                              // a play triangle
                p.moveTo(-z * 0.5f, -z * 0.75f); p.lineTo(z * 0.8f, 0); p.lineTo(-z * 0.5f, z * 0.75f); p.close(); break;
            default:                                  // done: a tick
                p.moveTo(-z * 0.7f, 0); p.lineTo(-z * 0.15f, z * 0.55f); p.lineTo(z * 0.75f, -z * 0.6f);
        }
        c.drawPath(p, line);
    }

    // a step's tutorial card: hall_info.txt [setup <key>] title, step1..step6, button (the action chip), voice by the launcher
    void drawCard(Canvas c, double t, int blink) {
        String key = "setup " + stepKey(card);
        RectF s = new RectF(176, 26, 346, 186);
        c.drawRect(s.left + 2, s.top + 3, s.right + 2, s.bottom + 3, solid(0x6E000000));
        c.drawRect(s, solid(0xEE0A1430));
        stroke(card == setupStep() ? RED : CYAN, 0.8f); c.drawRect(s, line);
        stroke(alpha(80, CYAN), 0.4f); c.drawRect(s.left + 2, s.top + 2, s.right - 2, s.bottom - 2, line);
        float x = s.left + 7, y = s.top + 6, w = s.width() - 14;
        text(c, "STEP " + card + " OF 3", x, y, hTiny, 0xFFFF8FF0, null);
        String title = data.get(key, "title");
        box(c, title, hSheetT, 0xFFFFFFFF, x, y + 6, w, 20);
        y += 8 + Math.min(20, boxHeight(title, hSheetT, w)) + 2;
        for (int i = 1; i <= 6; i++) {
            String st = data.get(key, "step" + i);
            if (st.isEmpty()) continue;
            c.drawPath(hex(x + 4, y + 3.6f, 3.8f), solid(card == setupStep() ? 0xFFB01C30 : 0xFF1E3C8C));
            text(c, "" + i, x + 4 - measure("" + i, hTiny) / 2, y + 0.6f, hTiny, 0xFFFFFFFF, null);
            float bh = boxHeight(st, hInfo, w - 11);
            if (y + bh > s.bottom - 16) break;
            box(c, st, hInfo, 0xFFDCE6F5, x + 10, y, w - 11, bh + 1);
            y += bh + 2.5f;
        }
        float bx = x, by = s.bottom - 13;
        String act = data.get(key, "button");
        String id = act.equals("ALLOW FILE ACCESS") ? "files" : act.equals("GET GAMES") ? "getgames" : act.equals("CHECK AGAIN") ? "check" : null;
        if (id != null) bx += chip(c, act, bx, by, false, true, id, card == setupStep() ? blink : -1) + 4;
        if (card < 3) bx += chip(c, "NEXT STEP", bx, by, false, true, "step" + (card + 1)) + 4;
        chip(c, "CLOSE", bx, by, false, true, "cardclose");
    }

    // the pod turning under a white museum spotlight, on a pulsing ring
    void drawExhibit(Canvas c, String pod, double t) {
        RectF podR = new RectF(134, 30, 242, 138);
        RectF gr = new RectF(podR.left - 18, podR.top - 12, podR.right + 18, podR.bottom + 18);
        // the PC's spotlight: white to 60 % of the way out (the path gradient's focus scales), then fading
        if (glow == null) glow = new RadialGradient(0, 0, 1, new int[] { 0xFFFFFFFF, 0xFFFFFFFF, 0x00FFFFFF }, new float[] { 0f, 0.6f, 1f }, Shader.TileMode.CLAMP);
        c.save();
        c.translate(gr.centerX(), gr.centerY()); c.scale(gr.width() / 2, gr.height() / 2);
        fill.setShader(glow); c.drawOval(new RectF(-1, -1, 1, 1), fill); fill.setShader(null);
        c.restore();
        float pulse = (float) (Math.sin(t * 2.6) * 0.5 + 0.5);
        ring(c, 188, 127, 92 + pulse * 6, 16 + pulse, CYAN, 150);
        ring(c, 188, 129, 70 - pulse * 4, 11, PINK, 130);
        Bitmap sheet = data.pod(pod);
        if (sheet != null) {
            int f = (int) (t * 10) % 60, tw = sheet.getWidth() / 10, th = sheet.getHeight() / 6;
            bitmap(c, sheet, new Rect((f % 10) * tw, (f / 10) * th, (f % 10 + 1) * tw, (f / 10 + 1) * th), podR);
        } else text(c, pod, 170, 80, hTitle, 0xFF28283C, null);
    }

    void ring(Canvas c, float cx, float cy, float w, float h, int col, int a) {
        RectF r = new RectF(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2);
        stroke(alpha(a / 4, col), 3f); c.drawOval(r, line);
        stroke(alpha(a, col), 0.9f); c.drawOval(r, line);
    }

    // three exhibit sheets fanned out: THE MACHINE / THE GAME / HOW TO PLAY
    void drawSheets(Canvas c, String pod) {
        RectF s = new RectF(252, 30, 346, 158);
        for (int k = 2; k >= 1; k--) {
            c.save();
            c.translate(s.centerX(), s.centerY());
            c.rotate(-3.5f * k);
            c.translate(-s.width() / 2 - 5 * k, -s.height() / 2 + 2 * k);
            c.drawRect(0, 0, s.width(), s.height(), solid(alpha(k == 2 ? 120 : 190, CREAM)));
            c.restore();
        }
        c.drawRect(s.left + 2, s.top + 3, s.right + 2, s.bottom + 3, solid(0x6E000000));
        c.drawRect(s, solid(CREAM));

        String kicker, title, tag, body;
        boolean cyber = false;
        String gameKey = items.isEmpty() ? "" : "game " + items.get(game).code;
        if (sheetTab == 0) {
            kicker = "THE MACHINE"; title = data.get("pod " + pod, "title"); tag = data.get("pod " + pod, "tag"); body = data.get("pod " + pod, "text");
            cyber = data.get("pod " + pod, "logo").equals("cybermind");
            if (title.isEmpty()) title = "VIRTUALITY " + pod;
        } else {
            kicker = sheetTab == 1 ? "THE GAME" : "HOW TO PLAY";
            title = items.isEmpty() ? "NO GAMES YET" : items.get(game).name;
            tag = data.get(gameKey, "tag");
            body = sheetTab == 1 ? data.get(gameKey, "text") : data.get(gameKey, "how").replace(" | ", "\n");   // hall_info.txt: one control per line
            if (items.isEmpty()) body = store ? "Copy your own games folder into MallVR/games on the headset with a USB cable." : "Press GET GAMES below: MallVR downloads the community's copies of the games.";
            else if (sheetTab == 2 && body.isEmpty()) body = "B = coin and start, right trigger = fire. Back to this menu: hold A + B for a second.";
        }
        float x = s.left + 6, w = s.width() - 12;
        text(c, kicker, x, s.top + 5, hTiny, 0xFFA01890, null);
        float th = boxHeight(title, hSheetT, w);
        box(c, title, hSheetT, 0xFF1A1A1A, x, s.top + 11, w, 22);
        float y = s.top + 11 + Math.min(th, 22);
        if (!tag.isEmpty()) { text(c, tag, x, y, hTiny, 0xFF6E6E78, null); y += 7; }
        box(c, body, hSheet, 0xFF1E1E1E, x, y + 1, w, s.bottom - 16 - y - (cyber ? 12 : 0));
        if (cyber) cyberMind(c, x, s.bottom - 27);
        String[] tabs = { "MACHINE", "GAME", "HOW TO" };
        float tx = x;
        for (int i = 0; i < 3; i++) {
            RectF r = new RectF(tx, s.bottom - 12, tx + measure(tabs[i], hTiny) + 3, s.bottom - 4);
            boolean on = sheetTab == i, hov = hover.equals("tab" + i);
            c.drawRect(r, solid(on ? 0xFF1A1A1A : hov ? 0xFFDCD6C8 : CREAM));
            stroke(0xFF282828, 0.4f); c.drawRect(r, line);
            text(c, tabs[i], r.left + 1.5f, r.top + 1, hTiny, on ? 0xFFFFFFFF : 0xFF1E1E1E, null);
            building.add(new Hit(r, "tab" + i));
            tx = r.right + 3;
        }
    }

    // the CyberMind wordmark from the SU3000 loader (CYBER white on black, MIND black on yellow)
    void cyberMind(Canvas c, float x, float y) {
        float sc = 0.55f, w1 = measure("CYBER", hCyber), w2 = measure("MIND", hCyber), h = 20 * sc, p = 3 * sc;
        c.drawRect(x, y, x + w1 + w2 + p * 2, y + h, solid(0xFF000000));
        stroke(0xFF969696, Math.max(0.5f, sc)); c.drawRect(x, y, x + w1 + w2 + p * 2, y + h, line);
        c.drawRect(x + w1 + p, y + sc, x + w1 + p + w2 + p - sc, y + h - sc, solid(0xFFF2BE00));
        text(c, "CYBER", x + p * 0.3f, y + sc, hCyber, 0xFFFFFFFF, null);
        text(c, "MIND", x + w1 + p * 1.3f, y + sc, hCyber, 0xFF000000, null);
    }

    // era dials: walk the hall from the 1991 pods to the SU3000
    void drawEra(Canvas c, String pod) {
        float cx0 = 150, step = 33, cy = 165, d = 21;
        for (int i = 0; i < ERA_PODS.length; i++) {
            float cx = cx0 + i * step;
            boolean on = ERA_PODS[i].equals(pod), hov = hover.equals("pod" + i);
            RectF r = new RectF(cx - d / 2, cy - d / 2, cx + d / 2, cy + d / 2);
            c.drawOval(r, solid(0xFFFFFFFF));
            Bitmap sheet = data.pod(ERA_PODS[i]);
            if (sheet != null) {
                c.save();
                Path cp = new Path(); cp.addOval(r, Path.Direction.CW); c.clipPath(cp);
                bitmap(c, sheet, new Rect(0, 0, sheet.getWidth() / 10, sheet.getHeight() / 6), new RectF(r.left - 2, r.top - 2, r.right + 2, r.bottom + 2));
                c.restore();
            }
            if (on) { stroke(alpha(90, CYAN), 3f); c.drawOval(r, line); }
            stroke(on ? CYAN : hov ? 0xFFA0C8FF : 0xFF283C5A, on ? 1.2f : 0.8f); c.drawOval(r, line);
            text(c, ERA_PODS[i], cx - measure(ERA_PODS[i], hTiny) / 2, cy + d / 2 + 1, hTiny, on ? 0xFFFFFFFF : 0xFF96B4D2, null);
            building.add(new Hit(new RectF(r.left, r.top, r.right, r.bottom + 8), "pod" + i));
        }
    }

    // where the PC has MODE / PLAYERS: the selected game's state, ALLOW FILE ACCESS when needed, the launcher's status
    void drawPanel(Canvas c, int blink) {
        float x0 = 134, y = 188;
        text(c, "GAME", x0, y, hTimesS, YELLOW, null);
        float x = x0 + 40;
        if (needFiles) chip(c, "ALLOW FILE ACCESS", x, y, blink % 2 == 0, true, "files");
        else if (!checked) text(c, "CHECKING YOUR GAME FILES...", x, y + 1, hSmall, 0xFFC8D2E6, null);
        else if (!items.isEmpty()) {
            Item g = items.get(game);
            String st = g.state == 1 ? "READY" : g.state == 2 ? "READY? (DIFFERENT VERSION)" : g.state == 3 ? "NEEDS ITS CD IMAGE" : "NOT FOUND";
            text(c, st, x, y + 1, hSmall, g.state == 1 ? CYAN : g.state == 0 ? 0xFF8C96AA : YELLOW, null);
            if (!g.why.isEmpty()) box(c, g.why, hTiny, 0xFF8CAAC8, x + measure(st, hSmall) + 3, y + 1.5f, 346 - x - measure(st, hSmall) - 3, 7);
        }
        box(c, status, hInfo, 0xFFDCE6F5, x0, 200, 212, 26);
    }

    // a small option chip; returns its width
    float chip(Canvas c, String s, float x, float y, boolean on, boolean can, String id) { return chip(c, s, x, y, on, can, id, -1); }

    /* alarm >= 0: a red chip that flashes (alarm = the blink count), for a button the player has to press (GET GAMES with games missing) */
    float chip(Canvas c, String s, float x, float y, boolean on, boolean can, String id, int alarm) {
        RectF r = new RectF(x, y, x + measure(s, hSmall) + 3, y + 9.5f);
        boolean hov = can && id != null && hover.equals(id);
        if (alarm >= 0) {
            c.drawRect(r, solid(alarm % 2 == 0 || hov ? RED : 0xFF6A0A0A));
            stroke(RED, 0.8f); c.drawRect(r, line);
            text(c, s, x + 1.5f, y + 1f, hSmall, 0xFFFFFFFF, null);
            if (id != null) building.add(new Hit(r, id));
            return r.width();
        }
        c.drawRect(r, solid(on ? CYAN : hov ? 0xFF1E466E : 0xA0060E22));
        stroke(can ? alpha(on ? 255 : 140, CYAN) : 0xFF32465A, 0.5f); c.drawRect(r, line);
        text(c, s, x + 1.5f, y + 1f, hSmall, on ? 0xFF000A1E : can ? 0xFFDCF5FF : 0xFF465A6E, null);
        if (can && id != null) building.add(new Hit(r, id));
        return r.width();
    }

    // the news bubble beside the pillbug, and the hint line in the operator's red
    void drawNews(Canvas c) {
        RectF r = new RectF(34, 234, 200, 262);
        boolean hov = hover.equals("news");
        c.drawRoundRect(r, 4, 4, solid(0xE60D1838));
        stroke(hov ? 0xFFFFFFFF : PINK, 0.6f); c.drawRoundRect(r, 4, 4, line);
        text(c, "NEWS", r.left + 4, r.top + 2, hTiny, 0xFFFF8FF0, null);
        text(c, data.newsTitle, r.left + 22, r.top + 1.5f, hSmall, 0xFFFFFFFF, null);
        box(c, data.newsText, hTiny, 0xFFFFDEEE, r.left + 4, r.top + 10, r.width() - 8, 18);
        if (!data.newsLink.isEmpty()) building.add(new Hit(r, "news"));

        String st = alarm ? (needFiles ? "ALARM: ALLOW FILE ACCESS" : store ? "ALARM: COPY YOUR GAMES IN" : "ALARM: PRESS GET GAMES") + "  ·  TAP ANYWHERE TO SILENCE"
            : needFiles ? "ALLOW FILE ACCESS FIRST" : !checked ? "CHECKING YOUR GAMES..."
            : items.isEmpty() || !anyFound() ? (store ? "NO GAMES YET: COPY YOUR GAMES IN" : "NO GAMES YET: PRESS GET GAMES")
            : !found(game) && items.get(game).state == 3 ? (store ? "NEEDS ITS CD IMAGE: COPY IT IN" : "NEEDS ITS CD: PRESS GET GAMES")
            : !found(game) ? (store ? "NOT FOUND: COPY THIS GAME IN" : "NOT FOUND: PRESS GET GAMES")
            : "IN A GAME: HOLD A + B FOR THIS MENU";
        box(c, st, hTimesS, RED, 206, 236, 142, 26);
    }

    // START and the Quest buttons along the bottom
    void drawBar(Canvas c, int blink, float L, float R, float B) {
        fill.setShader(new LinearGradient(0, 262, 0, 288, 0x00000000, 0xDC000000, Shader.TileMode.CLAMP));
        c.drawRect(L, 262, R, 288, fill);
        fill.setShader(null);
        if (B > 288) c.drawRect(L, 288, R, B, solid(0xDC000000));
        float y = 270, x = 6;
        boolean can = found(game), startHov = can && hover.equals("start");
        RectF sr = new RectF(x, y - 2, x + measure("START", hTimes) + 14, y + 11);
        c.drawRect(sr, solid(!can ? 0xFF4A4A30 : blink % 2 == 0 || startHov ? YELLOW : 0xFFE6D200));
        text(c, "START", x + 7, y - 1, hTimes, can ? 0xFF000000 : 0xFF8A8A70, null);
        if (can) building.add(new Hit(sr, "start"));
        x = sr.right + 6;
        x += chip(c, "ONLINE", x, y, false, true, "online") + 3;
        // the Quest doesn't open GET GAMES by itself (the PC does): it flashes red while games are missing
        if (!store) x += chip(c, "GET GAMES", x, y, false, true, "getgames", checked && !needFiles && (!anyFound() || !found(game)) ? blink : -1) + 3;
        x += chip(c, "PLAY NOTES " + (notesOn ? "ON" : "OFF"), x, y, false, true, "notes") + 3;
        x += chip(c, "REPORT", x, y, false, true, "report") + 3;
        x += chip(c, "ABOUT", x, y, false, true, "about") + 3;
        x += chip(c, "CHECK AGAIN", x, y, false, true, "check") + 3;
        chip(c, "MUSIC", x, y, music, true, "music");
    }

    // ---- GDI+ DrawString in Android terms ---------------------------------------------------------------------------------
    // GDI+ pads a line by 1/6 em on each side; a text's y is the top of its line, so the baseline sits asc ems lower
    float measure(String s, F f) { return f.p.measureText(s) + f.size / 3; }

    void text(Canvas c, String s, float x, float y, F f, int col, String id) {
        if (s == null || s.isEmpty()) return;
        f.p.setColor(col);
        c.drawText(s, x + f.size / 6, y + f.asc * f.size, f.p);
        if (id != null) building.add(new Hit(new RectF(x, y, x + measure(s, f), y + f.line * f.size), id));
    }

    List<String> wrap(String s, F f, float w) {
        List<String> out = new ArrayList<>();
        float max = w - f.size / 3;
        for (String para : s.split("\n")) {
            String cur = "";
            for (String word : para.split(" ")) {
                if (word.isEmpty()) continue;
                String next = cur.isEmpty() ? word : cur + " " + word;
                if (!cur.isEmpty() && f.p.measureText(next) > max) { out.add(cur); cur = word; }
                else cur = next;
            }
            out.add(cur);
        }
        return out;
    }

    float boxHeight(String s, F f, float w) { return s == null || s.isEmpty() ? 0 : wrap(s, f, w).size() * f.line * f.size; }

    /* wrapped text in a box; lines that don't fit are left out */
    void box(Canvas c, String s, F f, int col, float x, float y, float w, float h) {
        if (s == null || s.isEmpty()) return;
        float lh = f.line * f.size, ly = y;
        for (String l : wrap(s, f, w)) {
            if (ly + lh > y + h + lh * 0.25f) break;
            text(c, l, x, ly, f, col, null);
            ly += lh;
        }
    }

    void bitmap(Canvas c, Bitmap b, Rect src, RectF dst) { c.drawBitmap(b, src, dst, img); }
    Paint solid(int col) { fill.setShader(null); fill.setColor(col); return fill; }
    void stroke(int col, float w) { line.setColor(col); line.setStrokeWidth(w); }
    static int alpha(int a, int col) { return (Math.max(0, Math.min(255, a)) << 24) | (col & 0xFFFFFF); }
}
