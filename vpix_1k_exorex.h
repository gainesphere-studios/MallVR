/* Exorex / Heavy Metal (Virtuality 1000SD sit-down, 1992) - native rebuild from footage. No 1000-series software survives in public, so every model
   is built here and no game file is read. Included after vpix_dactyl1k.h inside vpix_hle.cpp (d1k:: and buggy:: are reachable). Dev only. Units: mm, seconds. World X right, Y forward, Z up.
   Evidence and every disagreement: Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\exorex.md. Sources: full Horizon operator tape TCm0Y7HAjNs (image and spoken rules), Play Expo 2016 1Qe4suqGZmU (a real machine, one seat), GamePro TV LtI6CgzEyLA 100-118 s, KKkZEXGaApE, Amstrad Action 84 p.31 (Sep 1992), Sega Force 16 p.22 (Apr 1993, text and an in-game screen), Black Ice 1.
   FROM FOOTAGE: 20 pictures per second; HUD layout, green, shield ticks, skull with tally ticks, rangefinder column, radar strip, three-claw sight; lock box that starts view-size and closes in 0.7 s; missile lock 3 s (spoken); cockpit solved from frames; walker shape and red, white, cyan colours; hits as a pale ten-sided translucent ball on the target, and a red one strobing at 5 Hz beside your own view when you are hit; red slab shards; the city plan rectified off the map mode at 156.5 s (east half solved, west half by eye); plain east and south of the city; aprons and hangar; lifts; two moons; map mode as an upright plan standing ahead of the walker, with only shield and skull left on screen; ejecting into a flight round the city to the hover deck, about 15.5 s in all with the boot; boot text timed off the tape and the gas table one line at a time; attract text, FREEDOM box and skull over the aerial view.
   SPOKEN or PRINTED RULES: four-way stick, side pivots; lift works when you stop on it with the map off, goes up only, walk off the edge to come down with no damage (tape 5:05-5:20); 3 minute round (Amstrad 84); shield does not refill (tape shows no gain); missile range above laser range (tape labels); trial by combat against three others (Sega Force 16); 1 to 4 players (Sega Force, Black Ice).
   OPPONENTS: Amstrad 84 says no computer players, four humans. Kept 3 stand-ins as the default because the Play Expo clip shows a lone player on one pod with a red walker that walks, stands and fires a missile, and a player with empty seats needs something to fight. EXOREX_CPU=0 gives the humans-only rule. Marston owns two SD pods, so a second person off camera cannot be excluded.
   INVENTED: stand-in thinking, walker reverse speed, half-stick turn rate, laser rate and damage, missile speed and damage, shield size, spawn points, west half of the city and all heights by eye, win rule by tally, the fourth boot line (unreadable, left out), last two gas lines, all sounds, the cabin behind the seat. Rangefinder meaning is not settled, frames disagree (see below).
   PALETTE: default is the tape's olive plain; EXOREX_PAL=1 is the teal plain and orange blocks of GamePro 1992 and Sega Force 1993. Sources disagree, cause unknown.
   Env: EXOREX_WALK, EXOREX_TURN, EXOREX_TIME, EXOREX_FPS, EXOREX_CPU, EXOREX_PAL, EXOREX_AUTO, EXOREX_CAM, EXOREX_RESPAWN, EXOREX_LOCK, EXOREX_LASER_GAP, EXOREX_LASER_DMG, EXOREX_LASER_RANGE, EXOREX_MISSILE_DMG, EXOREX_MISSILE_RANGE,
   EXOREX_MISSILE_SPEED, EXOREX_SHIELD, EXOREX_REGEN, EXOREX_LIFT_TIME, EXOREX_GRAV, EXOREX_EYE, EXOREX_SKILL, EXOREX_BOOT, dev EXOREX_LOOK=yawdeg,pitchdeg, EXOREX_PIN, EXOREX_PINME, EXOREX_FORCEMAP, EXOREX_KILLME=secs, EXOREX_NEAR. */
#include "vpix_1k_net.h"         /* link play, 2-4 pods: see the "link play" blocks below */
namespace exorex {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at;
using d1k::Mesh; using d1k::dput; using d1k::blit; using d1k::text; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound;
using d1k::yawed; using d1k::pitched; using d1k::add_bitmap;

static const float XPI = 3.14159265f;
static float wrap(float a) { while (a > XPI) a -= 2.f * XPI; while (a < -XPI) a += 2.f * XPI; return a; }
static V wrot(const M &Y, V v) { return Y.r[0] * v.x + Y.r[1] * v.y + Y.r[2] * v.z; }   /* local vector to world, Y = a yawed() matrix */

/* ---- tuning ---- */
static float walk_speed = 6000.f;        /* MEASURED about 0.65 eye heights a second walking at a block (tape 96-99 s); mm come from the eye height below (EXOREX_WALK mm/s) */
static float turn_slow = 0.6f;           /* INVENTED rad/s at a half-pushed stick (EXOREX_TURN) */
static float turn_fast = 1.15f;          /* MEASURED 55-72 deg/s pivoting on the stick pushed full over (tape 104-106, 114-115 s); the crescent crosses the view at that rate */
static float eye_h = 9000.f;             /* INVENTED: a walker about 10 m tall (EXOREX_EYE) */
static float grav = 9800.f;              /* INVENTED (EXOREX_GRAV) */
static float shield_max = 28.f;          /* the bar was seen at 26-28+ ticks full */
static float laser_gap = 0.3f;           /* READ ROUGHLY: a bolt in about every third picture while the trigger is held (tape 274-278, Play Expo 106-112) */
static float laser_dmg = 2.f;            /* READ ROUGHLY: one walker's rapid fire took the player's bar from about 20 ticks to none in 3.5 s (tape 275-278.5) */
static float laser_range = 70000.f;      /* half the missile range: the tape's two rangefinder marks sit at 1/3 and 2/3 of the line (labelled t=220) */
static float laser_speed = 150000.f;     /* READ ROUGHLY: a bolt is seen in about five pictures on its way to a walker 50-60 m off */
static float missile_dmg = 10.f;         /* INVENTED */
static float missile_range = 140000.f;   /* INVENTED in mm, the ratio to the laser is the tape's */
static float missile_speed = 40000.f;    /* READ ROUGHLY: Play Expo 110.7-112 s, the white missile covers about 50 m in 1.3 s */
static float missile_cool = 8.f;         /* INVENTED */
static float lock_time = 3.f;            /* SPOKEN on the Horizon tape (4:16) */
static float lock_shrink = 0.7f;         /* MEASURED: the box starts about the size of the view and is down on the target in 0.6-0.8 s, then holds (tape 39.2-39.8, Play Expo 108.3-109.1) */
static float regen = 0.f;                /* no source shows a bar growing without a cut: off (EXOREX_REGEN sets ticks/s after 6 s unhit) */
static float respawn_delay = 15.5f;      /* MEASURED: picture 278.5 s HUD gone, pod flight to the deck about 9 s (tape to 288), then the boot's 6.5 s (tape 11-17.5). Agrees with the guide's "about 15 s" */
static float lift_time = 3.f;            /* READ ROUGHLY tape 324-326 s */
static float game_len = 180.f;           /* Amstrad Action 84 (Sept 1992, read off the page scan): "about three minutes" for the 2 pound card. Sites could set it. The older guide said 4-5 from later memory */
static float game_fps = 20.f;            /* MEASURED 20 on the Horizon tape */
static float skill = 1.f;                /* INVENTED: CPU aim and reaction scale */
static float boot_len = 6.5f;            /* MEASURED: LOAD CORE SYSTEM at 11 s, HUD on at 17.5 s (tape; same again 349-356) */
static const float FLY_START = 3.f;      /* MEASURED: the last leg of the flight in, deck in view to the seat, 346-349 s on the tape */
static int n_cpu = 3;                    /* three stand-ins for the empty seats: see the header. 0 = the Amstrad Action rule, nobody but humans, an empty city */
static bool autoplay = false;

/* ---- city: the plan is read off the map mode (tape 156.5 s rectified with a 70 deg lens: rectangles come out square, lifts come out square, so the reading holds). See P() and PLAN below ---- */
static const float WX = 223000.f, WY = 193000.f, SLAB_Z = 30000.f, SLAB_H = 9000.f;   /* ten storeys is about 30 m (spoken). The field is a rectangle with a wide empty plain to the east and south (tape 268-283 s) */
static V P(float px, float py) { return V((px - 360.f) * 520.f, (202.f - py) * 520.f, 0); }   /* plan picture pixels to mm: a lift (14 m) is 27 px */
struct Box { float x0, y0, z0, x1, y1, z1; };
struct Rect { float cx, cy, hx, hy; };
struct Slab { float cx, cy, hx, hy, z; };
struct Pad { float x, y; int slab; };
static std::vector<Box> solids; static std::vector<Rect> outlines; static std::vector<Slab> slabs; static std::vector<Pad> pads;

/* ---- mesh and bitmap ids (offsets from d1k::MH and d1k::BMPH, clear of d1k's) ---- */
enum { X_SKY = 1001, X_CITY, X_FIELD, X_PAD, X_COCK, X_COCKL, X_BEAM, X_MISSILE, X_SHARD, X_FLASH, X_DECK, X_SPK_Y, X_SPK_R, X_BALL, X_BALLR,
       X_HULL = 1090, X_WALK = 1100 /* + 12 * look + part; looks 0 red, 1 black with white shield, 2 cyan (your own, on the deck), 3 teal */ };
enum { G_FONT = 1000, G_SOLID_G = 1030, G_SOLID_R, G_SKULL, G_SHIELD, G_SOLID_K, G_BIGSKULL, G_FREEDOM, G_PANEL, G_LOGO, G_DIG = 1040 };
static uint32_t C_TAN = 0xD4B276, C_BLUE = 0x2A5E98, C_GGRN = 0x8E9C82, C_SLIT = 0x3A3226;   /* sampled off the Horizon tape end screens */

static void flat(Mesh &m, V a, V b, V c, V d, uint32_t col) { V n = cross(b - a, c - a); if (n.z < 0) m.poly({d, c, b, a}, col); else m.poly({a, b, c, d}, col); }
static void rectz(Mesh &m, float x0, float y0, float x1, float y1, float z, uint32_t col) { flat(m, V(x0, y0, z), V(x1, y0, z), V(x1, y1, z), V(x0, y1, z), col); }
static uint32_t shade(uint32_t c, float k) { int r = (int)(((c >> 16) & 255) * k), g = (int)(((c >> 8) & 255) * k), b = (int)((c & 255) * k); return ((uint32_t)(r > 255 ? 255 : r) << 16) | ((uint32_t)(g > 255 ? 255 : g) << 8) | (uint32_t)(b > 255 ? 255 : b); }

static void bld(Mesh &m, float cx, float cy, float hx, float hy, float h, uint32_t col) {
    m.box(V(cx - hx, cy - hy, 0), V(cx + hx, cy + hy, h), shade(col, 1.1f), col, shade(col, 0.6f));
    solids.push_back({cx - hx, cy - hy, 0, cx + hx, cy + hy, h}); outlines.push_back({cx, cy, hx, hy});
    if (col == C_TAN && h < 22000.f) { int rows = (int)(h / 3200.f); if (rows < 2) rows = 2; if (rows > 7) rows = 7;   /* the tan blocks are chests of drawers: short dark handle slits in rows */
        for (int k = 0; k < rows; k++) { float z = h * (k + 0.6f) / (float)rows, hg = 380.f; const float o = 40.f;
            for (int c = 0; c < 2; c++) { float sx = (c ? 0.45f : -0.45f), wx = hx * 0.2f, wy = hy * 0.2f, ax = cx + hx * sx, ay = cy + hy * sx;
                m.poly({V(ax + wx, cy + hy + o, z), V(ax + wx, cy + hy + o, z + hg), V(ax - wx, cy + hy + o, z + hg), V(ax - wx, cy + hy + o, z)}, C_SLIT);
                m.poly({V(ax - wx, cy - hy - o, z), V(ax - wx, cy - hy - o, z + hg), V(ax + wx, cy - hy - o, z + hg), V(ax + wx, cy - hy - o, z)}, C_SLIT);
                m.poly({V(cx + hx + o, ay - wy, z), V(cx + hx + o, ay - wy, z + hg), V(cx + hx + o, ay + wy, z + hg), V(cx + hx + o, ay + wy, z)}, C_SLIT);
                m.poly({V(cx - hx - o, ay + wy, z), V(cx - hx - o, ay + wy, z + hg), V(cx - hx - o, ay - wy, z + hg), V(cx - hx - o, ay - wy, z)}, C_SLIT); } } }
    else if (col == C_BLUE) m.box(V(cx - hx - 500, cy - hy - 500, h), V(cx + hx + 500, cy + hy + 500, h + 700), shade(col, 1.25f), shade(col, 0.85f), shade(col, 0.5f));   /* the overhanging roof plate on the blue blocks */
}
static void fru(Mesh &m, float cx, float cy, float h0, float z0, float h1, float z1, uint32_t col) {   /* a square frustum, no caps */
    V a(cx - h0, cy - h0, z0), b(cx + h0, cy - h0, z0), c(cx + h0, cy + h0, z0), d(cx - h0, cy + h0, z0), e(cx - h1, cy - h1, z1), f(cx + h1, cy - h1, z1), g(cx + h1, cy + h1, z1), k(cx - h1, cy + h1, z1);
    m.poly({a, b, f, e}, col); m.poly({b, c, g, f}, col); m.poly({c, d, k, g}, col); m.poly({d, a, e, k}, col);
}
static void mushroom(Mesh &m, float cx, float cy, float H, uint32_t col) {   /* a tapered stem under a flared collar and a flat table top */
    fru(m, cx, cy, 2600, 0, 2000, H - 3400, col); fru(m, cx, cy, 2000, H - 3400, 6800, H - 500, shade(col, 0.82f));
    m.box(V(cx - 7000, cy - 7000, H - 500), V(cx + 7000, cy + 7000, H), shade(col, 1.15f), shade(col, 0.9f), shade(col, 0.6f));
    solids.push_back({cx - 2500, cy - 2500, 0, cx + 2500, cy + 2500, H - 500}); solids.push_back({cx - 7000, cy - 7000, H - 500, cx + 7000, cy + 7000, H});
    outlines.push_back({cx, cy, 2500, 2500});
}
static void wedge(Mesh &m, float cx, float cy, float hx, float hy, float h, uint32_t col) {   /* the small grey ramp blocks by the streets */
    V a(cx - hx, cy - hy, 0), b(cx + hx, cy - hy, 0), c(cx + hx, cy + hy, 0), d(cx - hx, cy + hy, 0), e(cx - hx, cy + hy * 0.2f, h), f(cx + hx, cy + hy * 0.2f, h), g(cx + hx, cy + hy, h), k(cx - hx, cy + hy, h);
    m.poly({a, b, f, e}, shade(col, 1.1f)); m.poly({e, f, g, k}, shade(col, 1.15f)); m.poly({b, c, g, f}, shade(col, 0.8f)); m.poly({c, d, k, g}, shade(col, 0.7f)); m.poly({d, a, e, k}, shade(col, 0.8f));
    solids.push_back({cx - hx, cy - hy, 0, cx + hx, cy + hy, h}); outlines.push_back({cx, cy, hx, hy});
}
static void pole(Mesh &m, float cx, float cy, float h) {   /* the thin masts standing about the city */
    m.box(V(cx - 260, cy - 260, 0), V(cx + 260, cy + 260, h), 0x989888); m.box(V(cx - 900, cy - 120, h * 0.86f), V(cx + 900, cy + 120, h * 0.88f), 0xB0B0A0);
}
static void make_slab(Mesh &m, float cx, float cy) {   /* a ten-storey blue table on thin posts over its lift (tape 288-292, 327 s). How the lift joins it is not shown: a walker rides straight up through it */
    const float H = SLAB_H; Slab s = {cx, cy, H, H, SLAB_Z}; slabs.push_back(s);
    m.box(V(cx - H, cy - H, SLAB_Z - 1200.f), V(cx + H, cy + H, SLAB_Z), shade(C_BLUE, 1.25f), C_BLUE, shade(C_BLUE, 0.5f));
    solids.push_back({cx - H, cy - H, SLAB_Z - 1200.f, cx + H, cy + H, SLAB_Z});
    for (int k = 0; k < 4; k++) { float px = cx + ((k & 1) ? 8200.f : -8200.f), py = cy + ((k & 2) ? 8200.f : -8200.f);
        m.box(V(px - 380, py - 380, 0), V(px + 380, py + 380, SLAB_Z - 1200.f), 0x8C8C7C);
        solids.push_back({px - 380, py - 380, 0, px + 380, py + 380, SLAB_Z - 1200.f}); }
    outlines.push_back({cx, cy, 7000, 7000}); pads.push_back({cx, cy, (int)slabs.size() - 1});
}
static void ball10(Mesh &m, float r, uint32_t c1, uint32_t c2) {   /* the ten-sided translucent ball of a hit (tape 38 s pale on the target, 275-278 s red and pale on you) */
    const int N = 10, L = 4; auto pt = [&](int i, int j) { float a = XPI * 2.f * i / N + 0.3f, e = XPI * (-0.5f + (float)j / L); return V(cosf(a) * cosf(e) * r, sinf(a) * cosf(e) * r, sinf(e) * r); };
    for (int j = 0; j < L; j++) for (int i = 0; i < N; i++) { V a = pt(i, j), b = pt(i + 1, j), c = pt(i + 1, j + 1), d = pt(i, j + 1); m.poly({a, b, c, d}, (i + j) & 1 ? c1 : c2); }
}
static void egg(Mesh &m, float r0, float r1, float r2, float h1, float h2, float h3, uint32_t c1, uint32_t c2) {   /* an 8-sided egg, base at z = 0 */
    float zs[3] = {0.f, h1, h2}, rs[3] = {r0, r1, r2};
    for (int i = 0; i < 8; i++) { int j = (i + 1) & 7; float a = XPI * 2.f * i / 8.f, b = XPI * 2.f * j / 8.f; uint32_t c = (i & 1) ? c2 : c1;
        for (int k = 0; k < 2; k++)
            m.poly({V(cosf(a) * rs[k], sinf(a) * rs[k], zs[k]), V(cosf(b) * rs[k], sinf(b) * rs[k], zs[k]), V(cosf(b) * rs[k + 1], sinf(b) * rs[k + 1], zs[k + 1]),
                    V(cosf(a) * rs[k + 1], sinf(a) * rs[k + 1], zs[k + 1])}, k ? c : shade(c, 0.8f));
        m.poly({V(cosf(a) * rs[2], sinf(a) * rs[2], zs[2]), V(cosf(b) * rs[2], sinf(b) * rs[2], zs[2]), V(0, 0, h3)}, c);
        m.poly({V(cosf(b) * rs[0], sinf(b) * rs[0], 0), V(cosf(a) * rs[0], sinf(a) * rs[0], 0), V(0, 0, -200)}, shade(c, 0.6f)); }
}
static void disc(Mesh &m, V c, float r, uint32_t col) {
    V e0 = norm(cross(c, V(0, 0, 1))), e1 = norm(cross(c, e0)); V p[8];
    for (int i = 0; i < 8; i++) { float a = XPI * 2.f * i / 8.f; p[i] = c + e0 * (cosf(a) * r) + e1 * (sinf(a) * r); }
    m.poly({p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]}, col);
}
static void build_meshes() {
    solids.clear(); outlines.clear(); slabs.clear(); pads.clear();
    Mesh c;
    /* EXOREX_PAL=1: the other palette. The tape (default) has an olive plain, grey paving, gold-tan blocks; GamePro TV 1992 (LtI6CgzEyLA 102-116 s) and the screen in Sega Force 16 p.22 (Apr 1993) both have a teal plain with brighter orange blocks, Sega Force also green lawns. Same game, two looks; cause unknown (build, or the video chain) */
    static const bool pal_press = envf("EXOREX_PAL", 0.f) > 0.5f; if (pal_press) C_TAN = 0xCC8A28;
    rectz(c, -450000, -450000, 450000, 450000, 0, pal_press ? 0x5E8E86 : 0x454A1E);                     /* the plain */
    {   /* PLAN: x0,y0,x1,y1 in plan pixels, kind, height. The first 14 and the first 4 lifts are the rectified t=156.5 picture; the rest are placed by eye from t=126 and 129 (same map, other positions, not solved). Heights and colours: aerial shots 11-17, 280-288, 349-356 s. No street grid exists: grey paving, dark green lawns and olive round the blocks */
      struct PR { float x0, y0, x1, y1; char k; float h; };
      const PR R[] = {{250,95,272,157,'b',28000},{350,140,385,205,'g',14000},{290,182,328,217,'b',16000},{215,193,250,222,'t',16000},{150,225,192,292,'t',24000},{262,245,347,282,'g',9000},{208,257,245,288,'b',12000},{283,290,317,357,'t',12000},{345,307,388,342,'b',25000},{215,318,260,350,'g',10000},{125,313,183,338,'g',7000},{178,378,305,402,'g',5000},{207,405,302,467,'b',20000},{228,505,325,570,'g',12000},
          {40,60,100,120,'b',27000},{60,170,90,250,'t',14000},{-20,250,40,290,'g',8000},{20,320,80,400,'b',16000},{100,-60,170,-10,'g',12000},{210,-70,250,30,'t',12000},{290,-60,360,-20,'b',22000},{20,-80,70,-30,'g',9000},{120,30,200,60,'g',6000},{300,50,340,130,'t',20000}};
      const float LF[][2] = {{283,111},{400,191},{245,233},{229,298},{164,134},{104,294}};
      const uint32_t TEAL = pal_press ? 0x2E7A30 : 0x2C4A3C, GR2 = 0x62625A, OLB = pal_press ? 0x629088 : 0x4A4E20;
      auto prect = [&](float x0, float y0, float x1, float y1, float z, uint32_t col) { V a = P(x0, y1), b = P(x1, y0); rectz(c, a.x, a.y, b.x, b.y, z, col); };
      const float ST[][4] = {{-10,300,430,318},{196,-90,214,500},{-10,140,430,156},{330,-90,346,500},{60,-90,74,430}};
      for (auto &q : ST) prect(q[0], q[1], q[2], q[3], 8, GR2);
      for (auto &q : R) { prect(q.x0 - 16, q.y0 - 16, q.x1 + 16, q.y1 + 16, 10, GR2); prect(q.x0 - 5, q.y0 - 5, q.x1 + 5, q.y1 + 5, 14, OLB); }
      const float LW[][4] = {{320,362,420,430},{90,345,170,420},{60,258,142,302},{222,-5,300,42},{350,30,440,130},{130,440,196,560},{350,440,430,520},{-20,170,50,240},{262,290,280,357}};
      for (auto &q : LW) prect(q[0], q[1], q[2], q[3], 12, TEAL);
      for (auto &q : LF) { V a = P(q[0], q[1]); prect(q[0] - 20, q[1] - 20, q[0] + 20, q[1] + 20, 11, GR2); make_slab(c, a.x, a.y); }
      for (auto &q : R) { V a = P(q.x0, q.y1), b = P(q.x1, q.y0); float hx = (b.x - a.x) * 0.5f, hy = (b.y - a.y) * 0.5f; bld(c, a.x + hx, a.y + hy, hx, hy, q.h, q.k == 't' ? C_TAN : q.k == 'b' ? C_BLUE : C_GGRN); }
      const struct { float x, y, h; uint32_t col; } MU[] = {{340,231,22000,C_BLUE},{125,285,18000,C_TAN},{200,364,24000,C_BLUE},{410,262,20000,C_GGRN},{262,486,18000,C_TAN},{22,140,22000,C_GGRN},{186,-30,16000,C_TAN}};
      for (auto &q : MU) { V a = P(q.x, q.y); mushroom(c, a.x, a.y, q.h, q.col); }
      { V a = P(392,100), b = P(150,405), d = P(70,450); pole(c, a.x, a.y, 30000); pole(c, b.x, b.y, 32000); pole(c, d.x, d.y, 28000); }
      /* the grey aprons out on the plain with a brown strip between them, and the hangar with two black arches (tape 279-285 s) */
      prect(470,250,600,300,14,0x8C8C88); prect(600,250,650,300,14,0x8A6A40); prect(650,250,760,300,14,0x8C8C88);
      { V A = P(470,190); const float w = 15000, d = 26000, h = 7000; const uint32_t G = 0x9A9A98, G2 = 0x74746E, K = 0x0A0A0C, BL = 0x6070C0; float x = A.x, y = A.y;
        c.poly({V(x, y, 0), V(x + w, y, 0), V(x + w, y, h), V(x, y, h)}, G2); flat(c, V(x, y, h), V(x + w, y, h), V(x + w, y + d, 800), V(x, y + d, 800), G);
        c.poly({V(x + w, y, 0), V(x + w, y + d, 0), V(x + w, y + d, 800), V(x + w, y, h)}, G2); c.poly({V(x, y + d, 0), V(x, y, 0), V(x, y, h), V(x, y + d, 800)}, G2);
        for (int k = 0; k < 2; k++) { float ax = x + (k ? 10500.f : 4500.f), r = 2200.f; c.poly({V(ax - r, y - 30, 0), V(ax + r, y - 30, 0), V(ax + r, y - 30, 2200), V(ax + r * 0.5f, y - 30, 3600), V(ax - r * 0.5f, y - 30, 3600), V(ax - r, y - 30, 2200)}, K); }
        rectz(c, x + 1500, y - 11000, x + 13500, y, 16, BL); solids.push_back({x, y, 0, x + w, y + d, h}); outlines.push_back({x + w * 0.5f, y + d * 0.5f, w * 0.5f, d * 0.5f}); }
    }
    c.add(X_CITY);
    Mesh fld;   /* the force-field boundary: a thin pale broken line close to the ground, it wobbles (tape, KKkZ) */
    { std::mt19937 fr(5); auto fz = [&]() { return 150.f + 600.f * (fr() / (float)(fr.max)()); }; const int NX = 56, NY = 48; float sx = 2.f * WX / NX, sy = 2.f * WY / NY;
      for (int k = 0; k < NX; k++) { float a = -WX + k * sx; if (fr() % 11 == 0) continue; float z = fz(), hh = 500.f; fld.box(V(a, -WY, z), V(a + sx, -WY + 300, z + hh), 0xE8F0F8); float z2 = fz(); fld.box(V(a, WY - 300, z2), V(a + sx, WY, z2 + hh), 0xE8F0F8); }
      for (int k = 0; k < NY; k++) { float a = -WY + k * sy; if (fr() % 11 == 0) continue; float z3 = fz(), hh = 500.f; fld.box(V(-WX, a, z3), V(-WX + 300, a + sy, z3 + hh), 0xE8F0F8); float z4 = fz(); fld.box(V(WX - 300, a, z4), V(WX, a + sy, z4 + hh), 0xE8F0F8); } }
    fld.add(X_FIELD);
    Mesh sky;   /* the backdrop: low mottled gold hills, in one part of the sky a white haze with dark hills in front of it and one thin crescent; two small banded moons and tiny stars */
    { std::mt19937 hr(77); auto rn = [&]() { return hr() / (float)(hr.max)(); }; const int N = 360; const float R = 470000.f; float H[N + 1];
      for (int i = 0; i <= N; i++) { float a = XPI * 2.f * (i % N) / N; float q = 0.5f + 0.5f * sinf(a * 3.f + 0.7f) * sinf(a * 5.f + 2.1f); H[i] = 4000.f + 36000.f * q * q + 5000.f * rn() + 6000.f * fabsf(sinf(a * 11.f)) * (0.5f + rn()) + 3000.f * fabsf(sinf(a * 31.f)); }
      for (int it = 0; it < 2; it++) { float o[N + 1]; for (int i = 0; i <= N; i++) o[i] = H[i]; for (int i = 0; i < N; i++) H[i] = (o[(i + N - 1) % N] + 2.f * o[i] + o[(i + 1) % N]) * 0.25f; }
      for (int i = 0; i < N; i++) H[i] += 1500.f + 8000.f * rn() * rn();   /* the ridge is jagged, many small peaks (tape 104-116, 283) */
      H[N] = H[0];
      auto hz = [&](float a) { float d = wrap(a - XPI * 1.5f); return clampf(1.f - d * d / (1.15f * 1.15f), 0.f, 1.f); };   /* a = 3pi/2 is -Y: the end-screen view looks +Y and sees no haze */
      for (int i = 0; i < N; i++) { float a = XPI * 2.f * i / N, b = XPI * 2.f * (i + 1) / N, am = (a + b) * 0.5f, ca = cosf(a) * R, sa = sinf(a) * R, cb = cosf(b) * R, sb = sinf(b) * R; float k = hz(am), j = 0.75f + 0.45f * rn();
          auto mixc = [&](uint32_t g, uint32_t d, float w) { int r0 = (g >> 16) & 255, g0 = (g >> 8) & 255, b0 = g & 255, r1 = (d >> 16) & 255, g1 = (d >> 8) & 255, b1 = d & 255; return shade(((uint32_t)(r0 + (r1 - r0) * w) << 16) | ((uint32_t)(g0 + (g1 - g0) * w) << 8) | (uint32_t)(b0 + (b1 - b0) * w), j); };
          const float f0[4] = {0.f, 0.22f + 0.2f * rn(), 0.55f + 0.2f * rn(), 1.f}; const uint32_t gold[3] = {0x6E4C14, 0x9A701C, 0xC8982C}, dark[3] = {0x2A2410, 0x3E3216, 0x5A4620};
          for (int s = 0; s < 3; s++) { float h0a = H[i] * f0[s], h1a = H[i] * f0[s + 1], h0b = H[i + 1] * f0[s], h1b = H[i + 1] * f0[s + 1]; sky.poly({V(ca, sa, h0a), V(cb, sb, h0b), V(cb, sb, h1b), V(ca, sa, h1a)}, mixc(gold[s], dark[s], k)); }
          if (k > 0.01f) { const float R2 = R + 3000.f, ca2 = cosf(a) * R2, sa2 = sinf(a) * R2, cb2 = cosf(b) * R2, sb2 = sinf(b) * R2;
              /* a grey band to about 12.6 deg with a soft top, and a blown-out white dome low in the middle, 28 deg wide and 6 high (tape 104.3-116.5). Measured off the frames */
              const float da = wrap(am - XPI * 1.5f) / 0.25f, dh = fabsf(da) < 1.f ? 50000.f * sqrtf(1.f - da * da) : 0.f;
              for (int s = 0; s < 24; s++) { float z0 = s * 4400.f, z1 = z0 + 4400.f, zm = (z0 + z1) * 0.5f, up = zm / 105000.f, top = up < 0.72f ? 1.f : clampf((1.f - up) / 0.28f, 0.f, 1.f), w = (k < 0.5f ? k * 2.f : 1.f) * top * (0.8f - 0.25f * up);
                  if (dh > 0.f) { float g = clampf((dh + 9000.f - zm) / 12000.f, 0.f, 1.f); w = w + (1.f - w) * g * (k < 0.5f ? k * 2.f : 1.f); }
                  uint32_t q = ((uint32_t)(0x12 + (0xFA - 0x12) * w) << 16) | ((uint32_t)(0x0A + (0xFA - 0x0A) * w) << 8) | (uint32_t)(0x1C + (0xFA - 0x1C) * w);
                  sky.poly({V(ca2, sa2, z0), V(cb2, sb2, z0), V(cb2, sb2, z1), V(ca2, sa2, z1)}, q); } } }
      }
    { const float mdirs[2][3] = {{-0.525f, 1.f, 0.22f}, {-0.44f, 1.f, 0.235f}};   /* the two moons side by side (tape 13 s, 279 s) */
      for (int mn = 0; mn < 2; mn++) { V md = norm(V(mdirs[mn][0], mdirs[mn][1], mdirs[mn][2])) * 420000.f; float rm = 7400.f; disc(sky, md, rm, 0xD8D4C8);   /* pale, with two dark bands */
      V e0 = norm(cross(md, V(0, 0, 1))); V up = norm(cross(e0, md)); if (up.z < 0) up = up * -1.f;
      for (int k = 0; k < 2; k++) { float o = k ? 2400.f : -2200.f, w = 750.f, hw = rm * 0.9f; V c0 = md * 0.998f + up * o; V a = c0 - e0 * hw - up * w, b = c0 + e0 * hw - up * w, c1 = c0 + e0 * hw + up * w, d = c0 - e0 * hw + up * w; sky.poly({a, b, c1, d}, 0x585060); sky.poly({d, c1, b, a}, 0x585060); } }
      { float az = XPI * 1.5f + 0.26f, R4 = 462000.f, zc = 62000.f, rc = 44000.f; V ctr(cosf(az) * R4, sinf(az) * R4, zc), rgt = norm(V(sinf(az), -cosf(az), 0));   /* one thin crescent standing out of the haze, 11 deg tall, bulging to the right and leaning, 15 deg left of the dome (tape 104.7, 105.3, 115.9). In motion the video doubles it: the 'wisps' read earlier were that double image */
        for (int i = 0; i < 12; i++) { float a0 = -1.35f + 2.7f * i / 12.f, a1 = -1.35f + 2.7f * (i + 1) / 12.f; auto pt = [&](float a, float t) { float u = (cosf(a) - t) * rc + sinf(a) * rc * 0.28f; return ctr + rgt * u + V(0, 0, sinf(a) * rc); }; float t0 = cosf(a0) * 0.17f, t1 = cosf(a1) * 0.17f; V o0 = pt(a0, 0), o1 = pt(a1, 0), i0 = pt(a0, t0), i1 = pt(a1, t1); sky.poly({o0, o1, i1, i0}, 0xE8E8EC); sky.poly({i0, i1, o1, o0}, 0xE8E8EC); } }
      const float st[6][3] = {{-0.3f, 1.f, 0.2f}, {0.12f, 1.f, 0.14f}, {0.55f, 1.f, 0.24f}, {-1.f, 0.3f, 0.3f}, {1.f, -0.4f, 0.26f}, {-0.3f, -1.f, 0.32f}};
      for (int k = 0; k < 6; k++) disc(sky, norm(V(st[k][0], st[k][1], st[k][2])) * 420000.f, 1300.f, 0xE8E8E8); }
    sky.add(X_SKY);
    Mesh pd;    /* a hazard pad: grey with a small blue mark in the middle, inside a black border with long yellow diagonal stripes (t21, 1Qe4 t63) */
    { const float h = 7000.f, bw = 1800.f, L = 1750.f; rectz(pd, -h, -h, h, h, 0, 0x101010); rectz(pd, -h + bw, -h + bw, h - bw, h - bw, 3, 0x5A5A50); rectz(pd, -1000, -700, 1000, 700, 6, 0x2030A0);
      V P[4] = {V(-h, -h, 0), V(h, -h, 0), V(h, h, 0), V(-h, h, 0)};
      for (int s = 0; s < 4; s++) { V a = P[s], b = P[(s + 1) & 3], D = norm(b - a), N = norm(V(-D.y, D.x, 0));
          for (int k = 0; k < 8; k += 2) { V p0 = a + D * (L * k), p1 = a + D * (L * (k + 1)); flat(pd, p0 + V(0, 0, 8), p1 + V(0, 0, 8), p1 + N * bw + D * bw + V(0, 0, 8), p0 + N * bw + D * bw + V(0, 0, 8), 0xF0D828); } } }
    pd.add(X_PAD);

    Mesh ck, cl;   /* the cockpit in head space. SOLVED from KKkZ/tape x2_0, x2_27, x2_34, x2_40, Horizon t21, Play Expo t50/t66 (views pitched about -20 while on a platform): a big monitor housing dead ahead with a
                      lower panel under it, a small housing about 47 deg to each side, a beam joining them; nothing else, the world shows through below the beams. All one cyan, lit from within */
    { d1k::kit_eye = V(0, 0, 0); const float kl0 = d1k::kit_lift; d1k::kit_lift = 3.f; const uint32_t CT = 0x62CCE4, CF = 0x309EC4, CS = 0x2486A8, CD = 0x175E78, BK = 0x0C0C14, BL = 0x1A40E8, RD = 0xE02820, GN = 0x38E060, WH = 0xC8F0E8, YL = 0xD8E020;
      auto Q = [&](V a, V b, V c, V d, uint32_t col) { V n = cross(b - a, c - a); if (dot(n, a) > 0) cl.poly({d, c, b, a}, col); else cl.poly({a, b, c, d}, col); };
      auto panel = [&](V tl, V tr, V bl, float u0, float u1, float v0, float v1, uint32_t col) { V u = tr - tl, v = bl - tl; d1k::kit_lift = col == BK ? 3.f : col == 0xE0F4FFu ? 16.f : 9.f; d1k::kit_quad(cl, tl + u * u0 + v * v0, u * (u1 - u0), v * (v1 - v0), col); };
      /* the big housing, its lower panel */
      V TL(-118, 770, -300), TR(118, 770, -300), BLf(-120, 690, -410), BRf(120, 690, -410), RL(-100, 880, -285), RR(100, 880, -285), SL(-178, 640, -470), SR(178, 640, -470), LT(-200, 615, -420), LR(200, 615, -420), LB(-200, 400, -600);
      Q(RL, RR, TR, TL, CT); Q(V(-140, 780, -295), V(140, 780, -295), V(142, 680, -415), V(-142, 680, -415), CF);
      Q(RL, TL, BLf, SL, CS); Q(RR, TR, BRf, SR, CS); Q(RL, SL, V(-240, 700, -1000), V(-110, 880, -1000), CD); Q(RR, SR, V(240, 700, -1000), V(110, 880, -1000), CD);
      panel(TL, TR, BLf, 0, 1, 0, 1, BK); panel(TL, TR, BLf, 0.45f, 0.97f, 0.1f, 0.94f, BL); panel(TL, TR, BLf, 0.29f, 0.39f, 0.08f, 0.22f, RD); panel(TL, TR, BLf, 0.1f, 0.22f, 0.08f, 0.22f, 0x2858D0);
      panel(TL, TR, BLf, 0.11f, 0.21f, 0.38f, 0.5f, WH); panel(TL, TR, BLf, 0.17f, 0.27f, 0.6f, 0.72f, WH);
      Q(V(-160, 690, -402), V(160, 690, -402), V(215, 625, -412), V(-215, 625, -412), BK);                          /* the black ledge between them */
      Q(V(-222, 625, -410), V(222, 625, -410), V(222, 400, -610), V(-222, 400, -610), CF); Q(V(-222, 625, -410), V(-222, 400, -610), V(-250, 400, -1000), V(-250, 660, -1000), CS); Q(V(222, 625, -410), V(222, 400, -610), V(250, 400, -1000), V(250, 660, -1000), CS);
      Q(V(-222, 400, -610), V(222, 400, -610), V(250, 400, -1000), V(-250, 400, -1000), CD);
      panel(LT, LR, LB, 0, 1, 0, 1, BK); panel(LT, LR, LB, 0.1f, 0.47f, 0.06f, 0.5f, BL); panel(LT, LR, LB, 0.6f, 0.97f, 0.14f, 0.74f, BL);
      { const float bx[6][4] = {{0.17f, 0.27f, 0.14f, 0.165f}, {0.32f, 0.42f, 0.14f, 0.165f}, {0.17f, 0.188f, 0.14f, 0.42f}, {0.402f, 0.42f, 0.14f, 0.42f}, {0.17f, 0.27f, 0.395f, 0.42f}, {0.32f, 0.42f, 0.395f, 0.42f}};
        for (int k = 0; k < 6; k++) panel(LT, LR, LB, bx[k][0], bx[k][1], bx[k][2], bx[k][3], 0xE0F4FF);     /* the white bracket display */
      }
      for (int k = 0; k < 3; k++) panel(LT, LR, LB, 0.51f, 0.57f, 0.14f + k * 0.2f, 0.26f + k * 0.2f, RD);
      panel(LT, LR, LB, 0.02f, 0.08f, 0.05f, 0.12f, GN); panel(LT, LR, LB, 0.02f, 0.08f, 0.16f, 0.23f, GN); panel(LT, LR, LB, 0.22f, 0.29f, 0.56f, 0.63f, 0x40B0C0); panel(LT, LR, LB, 0.4f, 0.48f, 0.68f, 0.78f, YL); panel(LT, LR, LB, 0.04f, 0.1f, 0.68f, 0.74f, 0x40B0C0); panel(LT, LR, LB, 0.11f, 0.17f, 0.68f, 0.74f, 0x40B0C0);
      /* the beams from the big housing to the small ones */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; Q(V(q * 583, 639, -419), V(q * 166, 766, -358), V(q * 173, 628, -412), V(q * 421, 510, -471), CF); Q(V(q * 421, 510, -471), V(q * 173, 628, -412), V(q * 190, 640, -560), V(q * 440, 530, -600), CD); }
      /* the small housings: left a strip with two glyphs over a bezelled screen, right three lamps beside a screen */
      for (int k = 0; k < 2; k++) { float th = (k ? 48.f : -47.f) * XPI / 180.f; V e(sinf(th), cosf(th), 0), rr(cosf(th), -sinf(th), 0); auto pt = [&](float u, float d, float z) { return rr * u + e * d + V(0, 0, z); };
          Q(pt(-62, 910, -300), pt(62, 910, -300), pt(82, 822, -305), pt(-82, 822, -305), CT); Q(pt(-82, 822, -305), pt(82, 822, -305), pt(100, 665, -460), pt(-100, 665, -460), CF);
          Q(pt(-62, 910, -300), pt(-82, 822, -305), pt(-100, 665, -460), pt(-150, 620, -560), CS); Q(pt(62, 910, -300), pt(82, 822, -305), pt(100, 665, -460), pt(150, 620, -560), CS);
          Q(pt(-100, 665, -460), pt(100, 665, -460), pt(150, 620, -560), pt(-150, 620, -560), CF);
          Q(pt(-150, 620, -560), pt(150, 620, -560), pt(200, 620, -1000), pt(-200, 620, -1000), CS); Q(pt(-62, 910, -300), pt(-150, 620, -560), pt(-200, 620, -1000), pt(-90, 910, -1000), CD); Q(pt(62, 910, -300), pt(150, 620, -560), pt(200, 620, -1000), pt(90, 910, -1000), CD);
          if (k == 0) { V a = pt(-66, 812, -312), b = pt(66, 812, -312), c0 = pt(-66, 775, -362); panel(a, b, c0, 0, 1, 0, 1, BK); panel(a, b, c0, 0.2f, 0.34f, 0.25f, 0.75f, WH); panel(a, b, c0, 0.62f, 0.76f, 0.25f, 0.75f, WH);
              V s0 = pt(-62, 762, -372), s1 = pt(62, 762, -372), s2 = pt(-62, 688, -448); panel(s0, s1, s2, 0, 1, 0, 1, BK); panel(s0, s1, s2, 0.14f, 0.86f, 0.1f, 0.9f, BL); }
          else { V a = pt(-76, 812, -316), b = pt(76, 812, -316), c0 = pt(-76, 690, -442); panel(a, b, c0, 0, 1, 0, 1, BK); panel(a, b, c0, 0.38f, 0.95f, 0.12f, 0.88f, BL); panel(a, b, c0, 0.08f, 0.28f, 0.12f, 0.3f, GN); panel(a, b, c0, 0.08f, 0.28f, 0.4f, 0.58f, GN); panel(a, b, c0, 0.08f, 0.28f, 0.68f, 0.86f, YL); } }
      d1k::kit_lift = kl0; }
    /* the cabin: seat and straps, sticks, side consoles, hatch and screens on the back wall, roof panel and pipes (Bobby 2026-11-02). INVENTED */
    { using d1k::kit_quad; using d1k::kit_stripes; using d1k::kit_lamps; using d1k::kit_dial; d1k::kit_eye = V(0, 0, 0); const uint32_t S = 0x3A3A44, S2 = 0x2A2A32, GR = 0x4A5868, PN = 0x384458, YL = 0xE8C020, BK = 0x18181C;
      ck.box(V(-300, -560, -900), V(300, -430, -260), S, S2, S2); ck.box(V(-140, -560, -230), V(140, -450, -40), S, S2, S2); ck.box(V(-320, -430, -900), V(320, 120, -720), S, S2, S2);   /* seat */
      kit_quad(ck, V(-190, -428, -700), V(60, 0, 0), V(0, 0, 420), YL); kit_quad(ck, V(130, -428, -700), V(60, 0, 0), V(0, 0, 420), YL);   /* straps */
      kit_quad(ck, V(-300, -424, -560), V(600, 0, 0), V(0, 0, 50), YL); ck.box(V(-60, -410, -600), V(60, -380, -480), 0xC0C0C8);   /* and the buckle */
      ck.box(V(-250, 480, -1000), V(-90, 760, -930), GR); ck.box(V(90, 480, -1000), V(250, 760, -930), GR);   /* pedals */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; auto bx = [&](float x0, float y0, float z0, float x1, float y1, float z1, uint32_t c, uint32_t c2) { ck.box(V((std::min)(q * x0, q * x1), y0, z0), V((std::max)(q * x0, q * x1), y1, z1), c, c2, c2); };
          bx(340, -300, -640, 520, 260, -560, S, S2); bx(405, 170, -560, 450, 220, -300, BK, BK); bx(390, 150, -300, 465, 240, -210, 0xC02020, 0x901818);   /* armrest, stick, grip */
          bx(800, -950, -1000, 1148, 100, -640, PN, S2);                                                       /* side console */
          ck.poly({V(q * 800, -950, -640), V(q * 800, 100, -640), V(q * 1148, 100, -452), V(q * 1148, -950, -452)}, GR);
          ck.poly({V(q * 800, -950, -640), V(q * 1148, -950, -640), V(q * 1148, -950, -452)}, S2); ck.poly({V(q * 800, 100, -640), V(q * 1148, 100, -640), V(q * 1148, 100, -452)}, S2);
          V o(q * 860, -800, -610), u(0, 520, 0), v(q * 230, 0, 125); kit_quad(cl, o, u, v, 0x2038D0);          /* lit screens and lamps on its slope */
          V o2(q * 860, -180, -610), u2(0, 300, 0); kit_quad(cl, o2, u2, v, 0x30D890);
          bx(500, -1300, -400, 1050, -1268, 100, PN, S2); V sb(q * 560, -1266, -360), su(q * 430, 0, 0), sv(0, 0, 300); kit_quad(cl, sb, su, sv, 0x2848E0); kit_lamps(cl, sb + V(0, 0, 340), V(q * 90, 0, 0), V(q * 50, 0, 0), V(0, 0, 40), 5);   /* screens on the back wall */
          bx(780, -1300, 690, 860, 350, 745, 0x707880, 0x505860); bx(1000, -1300, 700, 1060, 350, 745, 0x904030, 0x703020); }   /* pipes along the roof */
      kit_quad(ck, V(-290, -1294, -1000), V(580, 0, 0), V(0, 0, 1100), GR); kit_stripes(ck, V(-320, -1292, 100), V(640, 0, 0), V(0, 0, 45), 8, YL, BK);   /* the hatch */
      ck.box(V(-210, -1290, -420), V(210, -1270, -380), 0xC0C0C8); kit_dial(ck, V(0, -1288, -180), V(1, 0, 0), V(0, 0, 1), V(0, 1, 0), 120, 0xC0C0C8, BK, 0xFF2820);
      kit_quad(ck, V(-520, -1250, 744), V(1040, 0, 0), V(0, 1500, 0), GR);                                       /* roof panel, light strip, switches */
      kit_quad(cl, V(-90, -1200, 715), V(180, 0, 0), V(0, 1300, 0), 0xF0F0FF);
      kit_lamps(cl, V(-470, -600, 715), V(0, 70, 0), V(40, 0, 0), V(0, 40, 0), 12); kit_lamps(cl, V(430, -600, 715), V(0, 70, 0), V(40, 0, 0), V(0, 40, 0), 12); }
    ck.add(X_COCK); cl.add(X_COCKL);
    /* the rest of the machine round the seat, so looking away from the dash shows a walker and not empty space (Bobby 2026-11-02). INVENTED: footage only looks forward.
       Everything stays outside the forward view, which is matched to the footage. Head space, eye at 0. */
    { Mesh h; const uint32_t WH = 0xC8C8C8, MD = 0x8A8A90, DK = 0x34343C, IN_ = 0x283040, RD = 0xD02818;
      h.box(V(-1150, -1300, -1100), V(1150, 300, -1000), DK);                                         /* floor */
      h.box(V(-1700, -1400, -3600), V(1700, 300, -1100), MD, MD, DK);                                 /* the body under the cabin */
      h.box(V(-1150, -1350, -1000), V(1150, -1300, 150), IN_);                                         /* back wall, a window above it */
      h.box(V(-1150, -1350, 750), V(1150, 350, 830), IN_, DK, DK);                                     /* roof */
      h.box(V(-400, -1340, 150), V(-340, -1300, 750), DK); h.box(V(340, -1340, 150), V(400, -1300, 750), DK);       /* window bars */
      h.box(V(-1100, -3200, -1800), V(1100, -1350, 250), MD, MD, DK);                                  /* backpack, with exhausts behind the window */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f;
          auto bx = [&](float x0, float y0, float z0, float x1, float y1, float z1, uint32_t t, uint32_t s) { h.box(V((std::min)(q * x0, q * x1), y0, z0), V((std::max)(q * x0, q * x1), y1, z1), t, s, DK); };
          bx(1150, -1300, -1000, 1200, 150, -450, IN_, IN_);                                          /* sill */
          bx(1150, -100, -1000, 1210, 0, 800, DK, DK); bx(1150, -1350, -1000, 1210, -1250, 800, DK, DK);   /* pillars */
          bx(1200, -700, -950, 2300, 150, -150, WH, MD);                                               /* shoulder */
          bx(1450, -700, -150, 1950, 150, -110, RD, RD);                                               /* red stripe on it */
          bx(700, -2800, 250, 1000, -2300, 1500, DK, IN_); }                                           /* exhaust stack */
      h.add(X_HULL); }

    Mesh bm;    /* a laser streak: two crossed quads, 1 long along y; scaled on use */
    { const uint32_t cb = 0xFFF4B0; bm.poly({V(-1, 0, 0), V(1, 0, 0), V(0, 1, 0)}, cb); bm.poly({V(1, 0, 0), V(-1, 0, 0), V(0, 1, 0)}, cb); bm.poly({V(0, 0, -1), V(0, 0, 1), V(0, 1, 0)}, cb); bm.poly({V(0, 0, 1), V(0, 0, -1), V(0, 1, 0)}, cb); } bm.add(X_BEAM);   /* a pale wedge, not a line (tape 37.5, 278 s) */
    Mesh ms;    /* the missile: fat and white with dark spots (Play Expo 110.5-111.8 s) */
    { V a(0, 2600, 0), b(0, -2600, 0), c1(800, 0, 0), c2(-800, 0, 0), c3(0, 0, 800), c4(0, 0, -800); uint32_t k1 = 0xF4F4F4, k2 = 0xD0D0D0;
      ms.box(V(-120, -1600, 500), V(120, -1000, 640), 0x101010); ms.box(V(-120, 900, 480), V(120, 1500, 600), 0x101010); ms.box(V(560, -300, -120), V(700, 300, 120), 0x101010); ms.box(V(-700, -300, -120), V(-560, 300, 120), 0x101010);
      ms.poly({a, c1, c3}, k1); ms.poly({a, c3, c2}, k2); ms.poly({a, c2, c4}, k1); ms.poly({a, c4, c1}, k2);
      ms.poly({b, c3, c1}, k2); ms.poly({b, c2, c3}, k1); ms.poly({b, c4, c2}, k2); ms.poly({b, c1, c4}, k1); }
    ms.add(X_MISSILE);
    Mesh sh;   /* the kill shards: long red slabs, not specks (t26) */
    { V a(-700, 0, -1500), b(800, -200, -1200), c(600, 200, 1600), d(-500, 0, 1300); sh.poly({a, b, c, d}, 0xD03018); sh.poly({d, c, b, a}, 0xA02010); } sh.add(X_SHARD);
    Mesh fl; egg(fl, 700, 1000, 800, 800, 1700, 2100, 0xFF4030, 0xFF7050); fl.add(X_FLASH);
    Mesh dk;   /* the hover deck you are loaded on (tape 286-288, 346-348 s): an eight-sided black top with two yellow rim sectors and cyan dashes, a grey skirt under it */
    { const float Rr = 11000.f, Ri = 8000.f; auto rp = [&](float r, int i, float z) { float a = XPI * 2.f * i / 8.f + XPI / 8.f; return V(cosf(a) * r, sinf(a) * r, z); };
      for (int i = 0; i < 8; i++) { dk.poly({rp(Ri, i, 0), rp(Ri, i + 1, 0), V(0, 0, 0)}, 0x202024); flat(dk, rp(Ri, i, 0), rp(Rr, i, 0), rp(Rr, i + 1, 0), rp(Ri, i + 1, 0), (i == 2 || i == 5) ? 0xE8E020 : 0x101012);
          dk.poly({rp(Rr, i, -3500), rp(Rr, i + 1, -3500), rp(Rr, i + 1, 0), rp(Rr, i, 0)}, (i & 1) ? 0x8A8A8A : 0x747474);
          flat(dk, rp(Rr, i, -3500), rp(Rr * 0.6f, i, -5000), rp(Rr * 0.6f, i + 1, -5000), rp(Rr, i + 1, -3500), 0x505050);
          if (i != 2 && i != 5) { V m0 = rp(Ri + 900.f, i, 30), m1 = rp(Ri + 900.f, i + 1, 30), d = norm(m1 - m0); V n0 = m0 + d * 900.f, n1 = m1 - d * 900.f, up = norm(V(n0.x + n1.x, n0.y + n1.y, 0)) * 380.f; flat(dk, n0 - up, n1 - up, n1 + up, n0 + up, 0x40D0F0); } } }
    dk.add(X_DECK);
    { Mesh b0, b1; ball10(b0, 2300.f, 0xE8E8E8, 0xD4D4D4); ball10(b1, 2300.f, 0xC82C14, 0xB02410); b0.add(X_BALL); b1.add(X_BALLR); }
    { const uint32_t sc[2] = {0xFFF0A0, 0xE03018}; const int sid[2] = {X_SPK_Y, X_SPK_R};
      for (int k = 0; k < 2; k++) { Mesh s; s.poly({V(-140, 0, -80), V(140, 0, -80), V(0, 0, 300)}, sc[k]); s.poly({V(0, -140, -80), V(0, 140, -80), V(0, 0, 300)}, sc[k]); s.add(sid[k]); } }
    /* the walkers (KKkZ third-person shots and x2_7, a3): a hunched faceted hull with a big pale ten-sided shield on the front with a bright spindle down its middle, thin guns at the sides, reverse-knee legs, wedge feet.
       Looks: red, black with the white shield, an all-red hit flash, teal. Part origins: hull and shield at the hip, thighs at the hip joints, shins at the knees */
    for (int look = 0; look < 4; look++) {
        uint32_t t1, t2, t3, s1, s2, sp, leg, leg2;
        if (look == 0) { t1 = 0xC42C1C; t2 = 0x98200F; t3 = 0x701808; s1 = 0xE8E4DC; s2 = 0xC8C4BC; sp = 0xFFFFFF; leg = 0xB02414; leg2 = 0x80180C; }
        else if (look == 1) { t1 = 0x34343A; t2 = 0x24242A; t3 = 0x18181C; s1 = 0xE4E4E0; s2 = 0xC0C0BC; sp = 0xFFFFFF; leg = 0x2C2C32; leg2 = 0x1C1C20; }
        else if (look == 3) { t1 = 0x38B098; t2 = 0x268474; t3 = 0x185A50; s1 = 0x88D8C0; s2 = 0x60C0A8; sp = 0xE0FFF0; leg = 0x30A088; leg2 = 0x207062; }
        else { t1 = 0x50C4E4; t2 = 0x38A8CC; t3 = 0x2484A8; s1 = 0x88D8EC; s2 = 0x60C0D8; sp = 0xE0FFFF; leg = 0x48B8DC; leg2 = 0x2C94B8; }   /* your own: cyan, seen from behind on the deck (tape 287-288, 347-348 s) */
        int B = X_WALK + 12 * look;
        { Mesh t; const int NR = 5; const float ry[NR] = {-2900, -1900, -500, 1100, 2150}, rx[NR] = {800, 2000, 2500, 2300, 1500}, rz[NR] = {800, 1700, 2300, 2000, 1300}, cz[NR] = {2300, 2700, 2200, 1500, 1200};
          auto rp = [&](int r_, int k) { const float ca[6] = {1, 0.55f, -0.55f, -1, -0.55f, 0.55f}, sa[6] = {0, 1, 1, 0, -1, -1}; return V(ca[k] * rx[r_], ry[r_], cz[r_] + sa[k] * rz[r_]); };
          for (int r_ = 0; r_ < NR - 1; r_++) for (int k = 0; k < 6; k++) { int j = (k + 1) % 6; uint32_t col = (k == 4) ? t3 : (((k + r_) & 1) ? t1 : t2); if (k == 0 || k == 5) col = shade(col, 0.85f); t.poly({rp(r_, j), rp(r_ + 1, j), rp(r_ + 1, k), rp(r_, k)}, col); }
          t.poly({rp(0, 5), rp(0, 4), rp(0, 3), rp(0, 2), rp(0, 1), rp(0, 0)}, t2);
          t.add(B); }
        { Mesh k; V C(0, 2250, 1350); const float R0 = 2750.f, R1 = 1500.f; V cc(0, 3000, 1350);   /* the shield: an outer ring of ten facets, an inner ring, the spindle standing proud */
          for (int i = 0; i < 10; i++) { float a = XPI * 2.f * i / 10.f + 0.31f, b = XPI * 2.f * (i + 1) / 10.f + 0.31f; V o0 = C + V(cosf(a) * R0, 0, sinf(a) * R0), o1 = C + V(cosf(b) * R0, 0, sinf(b) * R0), i0 = C + V(cosf(a) * R1, 650, sinf(a) * R1), i1 = C + V(cosf(b) * R1, 650, sinf(b) * R1);
              uint32_t col = (i & 1) ? s1 : s2; k.poly({o1, i1, i0, o0}, col); k.poly({i1, cc, i0}, shade(col, 1.08f)); }
          V sT(0, 3050, 1350 + 2000), sB(0, 3050, 1350 - 1800), sL(-430, 3150, 1500), sR(430, 3150, 1500), sN(0, 3400, 1450);
          k.poly({sT, sL, sN}, sp); k.poly({sT, sN, sR}, shade(sp, 0.9f)); k.poly({sB, sN, sL}, shade(sp, 0.92f)); k.poly({sB, sR, sN}, shade(sp, 0.82f));
          k.add(B + 1); }
        for (int s = 0; s < 2; s++) { Mesh t; const float z0 = 300, z1 = -1100, z2 = -2400, a0 = 1050, b0 = 1200, a1 = 900, b1 = 1000, a2 = 480, b2 = 560;
            auto ring = [&](float z, float a, float b, int k) { const float cx[4] = {-1, 1, 1, -1}, cy[4] = {-1, -1, 1, 1}; return V(cx[k] * a, cy[k] * b, z); };
            for (int k = 0; k < 4; k++) { int j = (k + 1) & 3; uint32_t c = (k & 1) ? leg2 : leg; t.poly({ring(z1, a1, b1, k), ring(z1, a1, b1, j), ring(z0, a0, b0, j), ring(z0, a0, b0, k)}, c); t.poly({ring(z2, a2, b2, k), ring(z2, a2, b2, j), ring(z1, a1, b1, j), ring(z1, a1, b1, k)}, shade(c, 0.85f)); }
            t.add(B + 2 + s); }
        for (int s = 0; s < 2; s++) { Mesh t; const float z0 = 200, z1 = -1500, z2 = -2800;
            auto ring = [&](float z, float a, int k) { const float cx[4] = {-1, 1, 1, -1}, cy[4] = {-1, -1, 1, 1}; return V(cx[k] * a, cy[k] * a, z); };
            for (int k = 0; k < 4; k++) { int j = (k + 1) & 3; uint32_t c = (k & 1) ? leg : leg2; t.poly({ring(z1, 720, k), ring(z1, 720, j), ring(z0, 520, j), ring(z0, 520, k)}, c); t.poly({ring(z2, 430, k), ring(z2, 430, j), ring(z1, 720, j), ring(z1, 720, k)}, shade(c, 0.85f)); }
            t.add(B + 4 + s);
            Mesh ft; V a(-850, -1000, -550), b(850, -1000, -550), c(1250, 2000, -550), d(-1250, 2000, -550), e(-500, -600, 0), f(500, -600, 0), g(500, 500, 0), h(-500, 500, 0);   /* the wedge foot, wider at the toe; origin at the ankle */
            ft.poly({e, f, g, h}, leg); ft.poly({a, b, f, e}, leg2); ft.poly({b, c, g, f}, leg2); ft.poly({c, d, h, g}, shade(leg, 1.1f)); ft.poly({d, a, e, h}, leg2); ft.poly({d, c, b, a}, leg2);
            ft.add(B + 8 + s); }
        for (int s = 0; s < 2; s++) { Mesh t; t.box(V(-130, 0, -130), V(130, 3400, 130), leg2, 0x202020, 0x202020); t.box(V(-700, -1100, -700), V(700, 500, 800), t1, t2, t3); t.add(B + 6 + s); }   /* the thin guns */
    }
}

/* ---- HUD bitmaps (green, RGBA words are A<<24 | B<<16 | G<<8 | R) ---- */
static const uint32_t RGBA_GRN = 0xFF8AE04Bu, RGBA_REDP = 0xFF3030D0u;
static void make_bitmaps() {
    for (int ch = 0; ch < 26; ch++) { std::vector<uint32_t> px(d1k::FW * d1k::FH, 0);
        for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (d1k::FONT[ch][y * 5 + x] == '1')
            for (int b = 0; b < 2; b++) for (int a = 0; a < 2; a++) px[(y * 2 + b) * d1k::FW + x * 2 + a] = RGBA_GRN;
        add_bitmap(G_FONT + ch, d1k::FW, d1k::FH, px); }
    { std::vector<uint32_t> g(16, RGBA_GRN), r(16, RGBA_REDP); add_bitmap(G_SOLID_G, 4, 4, g); add_bitmap(G_SOLID_R, 4, 4, r); }
    { std::vector<uint32_t> k(16, 0xFF000000u); add_bitmap(G_SOLID_K, 4, 4, k); }
    /* the HUD skull: a small skull over crossed bones (tape, KKkZ) */
    { static const char *s[] = {"##....................##", "###......######......###", ".###...##########...###.", "..###.############.###..", "...##.###..##..###.##...", ".....####..##..####.....", ".....##############.....", "......#####..#####......", ".......##########.......", "...###..##.##.##..###...", "..####...#.##.#...####..", ".###..............  ###.", "###..................###", "##....................##", "........................", "........................"};
      std::vector<uint32_t> px(24 * 16, 0); for (int y = 0; y < 16; y++) for (int x = 0; x < 24; x++) if (s[y][x] == '#') px[y * 24 + x] = RGBA_GRN; add_bitmap(G_SKULL, 24, 16, px); }
    /* the shield icon: filled, its right edge notched */
    { static const char *s[] = {"############", "#########...", "###########.", "########....", "##########..", "########....", ".#########..", ".#######....", "..#######...", "...#####....", "....####....", ".....##....."};
      std::vector<uint32_t> px(144, 0); for (int y = 0; y < 12; y++) for (int x = 0; x < 12; x++) if (s[y][x] == '#') px[y * 12 + x] = RGBA_GRN; add_bitmap(G_SHIELD, 12, 12, px); }
    /* digits for the boot readout */
    { static const char *dg[10] = {"01110100011001110101110011000101110", "00100011000010000100001000010001110", "01110100010000100010001000100011111", "11111000100010000010000011000101110", "00010001100101010010111110001000010", "11111100001111000001000011000101110", "00110010001000011110100011000101110", "11111000010001000100010000100001000", "01110100011000101110100011000101110", "01110100011000101111000010001001100"};
      for (int ch = 0; ch < 10; ch++) { std::vector<uint32_t> px(d1k::FW * d1k::FH, 0); for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (dg[ch][y * 5 + x] == '1') for (int b = 0; b < 2; b++) for (int a = 0; a < 2; a++) px[(y * 2 + b) * d1k::FW + x * 2 + a] = RGBA_GRN; add_bitmap(G_DIG + ch, d1k::FW, d1k::FH, px); } }
    /* the big end-screen skull and crossbones: bright flat green, dark sockets (t47) */
    { const int W = 128, H = 104; std::vector<uint32_t> px(W * H, 0); const uint32_t G = 0xFF68F038u, E = 0xFF40B020u;
      auto dseg = [](float x, float y, float ax, float ay, float bx, float by) { float vx = bx - ax, vy = by - ay, t = clampf(((x - ax) * vx + (y - ay) * vy) / (vx * vx + vy * vy), 0.f, 1.f), dx = x - ax - vx * t, dy = y - ay - vy * t; return sqrtf(dx * dx + dy * dy); };
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { float fx = (float)x, fy = (float)y; bool on = false;
          for (int s = 0; s < 2; s++) { float x0 = s ? 118.f : 10.f, x1 = s ? 10.f : 118.f; if (dseg(fx, fy, x0, 14, x1, 90) < 6.f) on = true;
              for (int e = 0; e < 2; e++) { float ex = e ? x1 : x0, ey = e ? 90.f : 14.f, sg = (ex < 64.f) ? -1.f : 1.f; float d1 = hypotf(fx - (ex + sg * 5), fy - (ey - 8 + 16 * e)), d2 = hypotf(fx - (ex + sg * 8), fy - (ey + 5 - 10 * e)); if (d1 < 7.5f || d2 < 7.5f) on = true; } }
          float cr = ((fx - 64) * (fx - 64)) / (34.f * 34.f) + ((fy - 36) * (fy - 36)) / (32.f * 32.f); bool skull = cr < 1.f || (fabsf(fx - 64) < 20 && fy > 54 && fy < 76);
          if (skull) on = true;
          bool hole = false; for (int s = 0; s < 2; s++) { float ex = s ? 77.f : 51.f; if (((fx - ex) * (fx - ex)) / (10.f * 10.f) + ((fy - 38) * (fy - 38)) / (11.f * 11.f) < 1.f) hole = true; }
          if (fabsf(fx - 64) < (fy - 48) * 0.5f && fy < 57) hole = true;
          if (fy > 65 && fy < 76 && (x % 8) == 0 && fabsf(fx - 64) < 18) hole = true;
          if (fy > 76 && fy < 78 && fabsf(fx - 64) < 22) hole = true;
          if (on && !hole) px[y * W + x] = (cr > 0.9f && cr < 1.f) ? E : G; }
      add_bitmap(G_BIGSKULL, W, H, px); }
    /* FREEDOM as wide outlined letters */
    { const char *w = "FREEDOM"; const int cw = 6, ch = 4, W = 7 * (5 * cw + cw), H = 7 * ch; std::vector<uint32_t> px(W * H, 0);
      auto cell = [&](int l, int cx, int cy) { if (cx < 0 || cy < 0 || cx >= 5 || cy >= 7) return false; return d1k::FONT[w[l] - 'A'][cy * 5 + cx] == '1'; };
      for (int l = 0; l < 7; l++) for (int cy = 0; cy < 7; cy++) for (int cx = 0; cx < 5; cx++) if (cell(l, cx, cy)) for (int b = 0; b < ch; b++) for (int a = 0; a < cw; a++) {
          bool edge = (a <= 1 && !cell(l, cx - 1, cy)) || (a >= cw - 2 && !cell(l, cx + 1, cy)) || (b == 0 && !cell(l, cx, cy - 1)) || (b == ch - 1 && !cell(l, cx, cy + 1));
          if (edge) px[(cy * ch + b) * W + l * 6 * cw + cx * cw + a] = 0xFFC0F0C8u; }
      add_bitmap(G_FREEDOM, W, H, px); }
    /* the attract title panel: a burnt red sheet, metal EXO REX, two rounded screens, a lamp strip and a speaker grille (KKkZ t381). G_LOGO is its top strip for the story screen */
    { const int W = 176, H = 144; std::vector<uint32_t> px(W * H, 0xFF000000u), lg(W * 34, 0xFF000000u); std::mt19937 r(12); auto rn = [&]() { return r() / (float)(r.max)(); };
      std::vector<float> nz(W * H); for (auto &n : nz) n = rn(); for (int it = 0; it < 3; it++) { std::vector<float> o = nz; for (int y = 1; y < H - 1; y++) for (int x = 1; x < W - 1; x++) nz[y * W + x] = (o[y * W + x] + o[y * W + x - 1] + o[y * W + x + 1] + o[(y - 1) * W + x] + o[(y + 1) * W + x]) * 0.2f; }
      auto put = [&](int x, int y, int rr, int gg, int bb) { if (x >= 0 && y >= 0 && x < W && y < H) px[y * W + x] = 0xFF000000u | ((uint32_t)bb << 16) | ((uint32_t)gg << 8) | (uint32_t)rr; };
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { float n = (nz[y * W + x] - 0.5f) * 5.f, ed = (std::min)((std::min)(x, W - 1 - x), (std::min)(y, H - 1 - y)) / 22.f; float bb = clampf(0.25f + n + ed * 0.3f, 0.f, 1.f); int cr = (int)(70 + 150 * bb), cg = (int)(6 + 50 * bb * bb), cb = (int)(6 + 14 * bb);
          if (x < 4 || x > W - 5) { cr = cr / 3 + 70; cg = cg + 90; cb = cb + 90; } put(x, y, cr, cg, cb); }
      auto seg = [&](float ax, float ay, float bx, float by, float wd) { int n = (int)(hypotf(bx - ax, by - ay) * 2.f) + 1; for (int i = 0; i <= n; i++) { float t = i / (float)n, x = ax + (bx - ax) * t, y = ay + (by - ay) * t; for (int v = (int)(y - wd); v <= (int)(y + wd); v++) for (int h = (int)(x - wd); h <= (int)(x + wd); h++) { int g = 245 - (v - 6) * 6 + ((v % 5 == 0) ? -50 : 0); g = g < 70 ? 70 : g; put(h, v, g, g + 6 > 255 ? 255 : g + 6, g + 8 > 255 ? 255 : g + 8); } } };
      const float T = 7, Bm = 27; float lx = 14; const char *word = "EXO REX";   /* wide metal capitals; the real ones have serifs, which this size cannot show */
      for (int l = 0; l < 7; l++) { char c = word[l]; if (c == ' ') { lx += 8; continue; } float W2 = 19;
          if (c == 'E') { seg(lx, T, lx, Bm, 1.6f); seg(lx, T, lx + W2, T, 1.2f); seg(lx, (T + Bm) / 2, lx + W2 * 0.8f, (T + Bm) / 2, 1.2f); seg(lx, Bm, lx + W2, Bm, 1.2f); }
          if (c == 'X') { seg(lx, T, lx + W2, Bm, 1.8f); seg(lx + W2, T, lx, Bm, 1.0f); }
          if (c == 'O') { float qx = lx + W2, qy = (T + Bm) / 2; for (int i = 1; i <= 24; i++) { float a0 = i / 24.f * 2.f * XPI, a1 = (i - 1) / 24.f * 2.f * XPI; seg(lx + W2 / 2 + cosf(a1) * W2 / 2, (T + Bm) / 2 + sinf(a1) * 10, lx + W2 / 2 + cosf(a0) * W2 / 2, (T + Bm) / 2 + sinf(a0) * 10, 1.3f); } (void)qx; (void)qy; }
          if (c == 'R') { seg(lx, T, lx, Bm, 1.6f); seg(lx, T, lx + W2 * 0.8f, T, 1.2f); seg(lx + W2 * 0.8f, T, lx + W2 * 0.9f, T + 4, 1.2f); seg(lx + W2 * 0.9f, T + 4, lx + W2 * 0.8f, (T + Bm) / 2, 1.2f); seg(lx, (T + Bm) / 2, lx + W2 * 0.8f, (T + Bm) / 2, 1.2f); seg(lx + W2 * 0.5f, (T + Bm) / 2, lx + W2, Bm, 1.6f); }
          lx += W2 + 5; }
      for (int y = 0; y < 34; y++) for (int x = 0; x < W; x++) { uint32_t q = px[y * W + x]; lg[y * W + x] = q; }
      add_bitmap(G_LOGO, W, 34, lg);
      for (int y = 58; y < 134; y++) for (int x = 22; x < 154; x++) put(x, y, 14, 12, 12);   /* the dark inset */
      for (int s = 0; s < 2; s++) { int x0 = s ? 96 : 28, x1 = x0 + 52, y0 = 63, y1 = 129, c = 7; for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { int dx = (std::min)(x - x0, x1 - x), dy = (std::min)(y - y0, y1 - y); if (dx + dy < c) continue; bool rim = dx < 2 || dy < 2 || dx + dy < c + 2; if (rim) put(x, y, 170, 170, 170); else put(x, y, 6, 6, 8); } }
      for (int k = 0; k < 6; k++) for (int a = 0; a < 7; a++) { put(84 + a, 66 + k * 5, k == 2 ? 240 : 120, k == 2 ? 40 : 120, k == 2 ? 30 : 110); put(84 + a, 67 + k * 5, k == 2 ? 240 : 120, k == 2 ? 40 : 120, k == 2 ? 30 : 110); }
      for (int y = 100; y < 124; y += 3) for (int x = 82; x < 94; x += 3) put(x, y, 130, 130, 130);
      add_bitmap(G_PANEL, W, H, px); }
}
static void rectb(VPIX_Frame &f, int id, int sw, int sh, float x, float y, float w, float h, float alpha = 1.f) {   /* a bitmap stretched to w x h */
    VPIX_Blit b = {}; b.handle = d1k::BMPH + id; b.sw = sw; b.sh = sh; b.x = (int)x; b.y = (int)y; b.dw = w; b.dh = h; b.key = 0; b.alpha = alpha; b.prio = 1000000;
    b.tint[0] = b.tint[1] = b.tint[2] = 1.f; f.blits.push_back(b);
}
static void ln(VPIX_Frame &f, float x0, float y0, float x1, float y1, uint8_t r = 0x4B, uint8_t g = 0xE0, uint8_t b = 0x8A) {
    VPIX_Frame::Line l; l.x0 = x0; l.y0 = y0; l.x1 = x1; l.y1 = y1; l.rgb[0] = r; l.rgb[1] = g; l.rgb[2] = b; f.lines.push_back(l);
    l.x0 += 0.55f; l.x1 += 0.55f; l.y0 += 0.55f; l.y1 += 0.55f; f.lines.push_back(l);   /* the tape's lines are about two pixels of 640 wide */
}
static void box2(VPIX_Frame &f, float x0, float y0, float x1, float y1) { ln(f, x0, y0, x1, y0); ln(f, x1, y0, x1, y1); ln(f, x1, y1, x0, y1); ln(f, x0, y1, x0, y0); }
static void gtext(VPIX_Frame &f, const char *s, float x, float y, float sc, bool centre = true, int maxc = 9999) {   /* A-Z, 0-9 and a point */
    int n = 0; for (const char *c = s; *c; c++) n++; if (n > maxc) n = maxc;
    float w = n * 12.f * sc; float cx = centre ? x - w * 0.5f : x; int k = 0;
    for (const char *c = s; *c && k < n; c++, k++) { if (*c >= 'A' && *c <= 'Z') blit(f, G_FONT + (*c - 'A'), d1k::FW, d1k::FH, cx, y, sc); else if (*c >= '0' && *c <= '9') blit(f, G_DIG + (*c - '0'), d1k::FW, d1k::FH, cx, y, sc); else if (*c == '.') rectb(f, G_SOLID_G, 4, 4, cx + 4.f * sc, y + 11.f * sc, 2.f * sc, 2.f * sc); cx += 12.f * sc; }
}

/* ---- sounds: all INVENTED synthesised placeholders (keys 0x7E000000 + n) ---- */
enum { SND_LASER = 0x7E000001, SND_HIT, SND_MISSILE, SND_BOOM, SND_WARN, SND_LOCK, SND_LIFT, SND_GO, SND_POD, SND_CREDIT };
static void xsfx(int ch, uint32_t key, int vol, int pan = 0) { if (!sound_on) return; buggy::play(ch, (int)key, vol, pan); }
static void make_sounds() {
    std::mt19937 r(9); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(SND_LASER, 0.2f, [&](float t) { return sinf(6.2831853f * (1400.f - 5000.f * t) * t) * expf(-t * 12.f) * 0.7f; });
    make_sound(SND_HIT, 0.15f, [&](float t) { return noise() * expf(-t * 25.f) * 0.7f; });
    make_sound(SND_MISSILE, 0.6f, [&](float t) { return noise() * sinf(3.14159f * t / 0.6f) * 0.4f + sinf(6.2831853f * (200.f + 300.f * t) * t) * 0.2f; });
    make_sound(SND_BOOM, 1.f, [&](float t) { return noise() * expf(-t * 4.f) * 0.8f + sinf(6.2831853f * 55.f * t) * expf(-t * 3.f) * 0.7f; });
    make_sound(SND_WARN, 0.2f, [&](float t) { return sinf(6.2831853f * 1000.f * t) * (t < 0.15f ? 0.5f : 0.f); });
    make_sound(SND_LOCK, 0.1f, [&](float t) { return sinf(6.2831853f * 1500.f * t) * expf(-t * 20.f) * 0.5f; });
    make_sound(SND_LIFT, 1.2f, [&](float t) { return sinf(6.2831853f * (90.f + 40.f * t) * t) * sinf(3.14159f * t / 1.2f) * 0.6f; });
    make_sound(SND_GO, 0.7f, [&](float t) { return sinf(6.2831853f * (500.f + 600.f * t) * t) * expf(-t * 3.f) * 0.5f; });
    make_sound(SND_POD, 0.8f, [&](float t) { return noise() * sinf(3.14159f * t / 0.8f) * 0.35f; });
    make_sound(SND_CREDIT, 0.4f, [&](float t) { return sinf(6.2831853f * (t < 0.2f ? 700.f : 1000.f) * t) * 0.5f * (fmodf(t, 0.2f) < 0.17f ? 1.f : 0.f); });
}

/* ---- walkers, shots, effects ---- */
enum WState { WS_ALIVE, WS_POD, WS_BOOT };   /* POD: the flight to the deck (others: all of the wait); BOOT: yours only, loading in the seat on the way down */
struct Walker {
    bool cpu = false; int look = 0, kills = 0; WState s = WS_ALIVE;
    V p; float yaw = 0, vz = 0, shield = 28, phase = 0, moved = 0, pod_t = 0, pod_dur = 15, regen_t = 0, hit_flash = 0, fire_cool = 0, msl_cool = 0, lock_t = 0, lock_grace = 0;
    int lock_target = -1, side = 0; float ball_t = 0, self_t = 0; int self_side = 1; bool want_lift = false;
    V aim = V(0, 1, 0), pod_from, pod_to;
    int lifting = 0; float lift_t = 0, lift_z0 = 0, lift_z1 = 0; bool armed = true;
    std::vector<V> route; int node = 12, from = -1; float top_wait = 0, stuck_t = 0; V stuck_p;
    bool remote = false, rmap = false;   /* link play: a person on another pod drives this walker; rmap = their map is on */
};
static int me = 0;   /* a linked slave sits in the seat the master gave it */
static std::vector<Walker> wk;
static bool human(int i) { return i == me || wk[i].remote; }
struct Missile { V p, dir; int owner, target; float t, smoke; };
struct Beam { V a, b; float t, dur; int owner, target; float dmg; int kind; };
struct Spark { V p, v; float life; int id; };
struct Shard { V p, v, w; M R; float life; };
struct Flash { V p; float t; };
static std::vector<Missile> missiles; static std::vector<Beam> beams; static std::vector<Spark> sparks; static std::vector<Shard> shards; static std::vector<Flash> flashes;

enum GS { G_ATTRACT, G_CREDIT, G_BOOT, G_PLAY, G_END };
static GS gs = G_ATTRACT; static float st_t = 0, time_left = 0; static double sim_t = 0;
static bool map_mode = false, won = false;
static V href; static bool have_href = false;

static V center(size_t j) { return V(wk[j].p.x, wk[j].p.y, wk[j].p.z + 4600.f); }
static const float HR = 3400.f;
static float ray_box(V o, V d, const Box &b, float maxd) {
    float t0 = 0, t1 = maxd; const float oo[3] = {o.x, o.y, o.z}, dd[3] = {d.x, d.y, d.z}, lo[3] = {b.x0, b.y0, b.z0}, hi[3] = {b.x1, b.y1, b.z1};
    for (int k = 0; k < 3; k++) {
        if (fabsf(dd[k]) < 1e-6f) { if (oo[k] < lo[k] || oo[k] > hi[k]) return -1.f; }
        else { float a = (lo[k] - oo[k]) / dd[k], c = (hi[k] - oo[k]) / dd[k]; if (a > c) { float t = a; a = c; c = t; } if (a > t0) t0 = a; if (c < t1) t1 = c; if (t0 > t1) return -1.f; }
    }
    return t0;
}
struct Hit { float t; int kind; int w; };   /* kind 0 nothing, 1 ground, 2 building, 3 walker */
static Hit ray_world(V o, V d, float maxd, int skip) {
    Hit h = {maxd, 0, -1};
    if (d.z < -1e-5f && o.z > 0.f) { float t = -o.z / d.z; if (t < h.t) { h.t = t; h.kind = 1; } }
    for (const Box &b : solids) { float t = ray_box(o, d, b, h.t); if (t >= 0.f && t < h.t) { h.t = t; h.kind = 2; } }
    for (size_t j = 0; j < wk.size(); j++) { if ((int)j == skip || wk[j].s != WS_ALIVE) continue;
        V oc = o - center(j); float bq = dot(oc, d), cq = dot(oc, oc) - HR * HR, disc = bq * bq - cq; if (disc < 0) continue;
        float t = -bq - sqrtf(disc); if (t < 0) continue; if (t < h.t) { h.t = t; h.kind = 3; h.w = (int)j; } }
    return h;
}
static bool los_to(int i, V eye, int j) { V c = center(j), d = c - eye; float dist = len(d); if (dist < 1.f) return true; Hit h = ray_world(eye, d * (1.f / dist), dist + 50.f, i); return h.kind == 3 && h.w == j; }

static float floor_at(float x, float y, float z) {
    for (const Slab &s : slabs) if (fabsf(x - s.cx) <= s.hx && fabsf(y - s.cy) <= s.hy && z >= s.z - 2500.f) return s.z;
    return 0.f;
}
static void collide(V &p) {
    const float R = 3200.f;
    for (int it = 0; it < 2; it++) for (const Box &b : solids) {
        if (!(p.z < b.z1 - 100.f && p.z + 9500.f > b.z0 + 100.f)) continue;
        float qx = clampf(p.x, b.x0, b.x1), qy = clampf(p.y, b.y0, b.y1), dx = p.x - qx, dy = p.y - qy, d2 = dx * dx + dy * dy;
        if (d2 < R * R) {
            if (d2 > 1.f) { float d = sqrtf(d2); p.x = qx + dx / d * R; p.y = qy + dy / d * R; }
            else { float l = p.x - b.x0, rr = b.x1 - p.x, bo = p.y - b.y0, t = b.y1 - p.y, m = (std::min)((std::min)(l, rr), (std::min)(bo, t));
                if (m == l) p.x = b.x0 - R; else if (m == rr) p.x = b.x1 + R; else if (m == bo) p.y = b.y0 - R; else p.y = b.y1 + R; }
        }
    }
    p.x = clampf(p.x, -WX + 1000.f, WX - 1000.f); p.y = clampf(p.y, -WY + 1000.f, WY - 1000.f);   /* the force field */
}

static void burst(V at, int n, int id, float speed, float life) {
    if (sparks.size() > 600) return;
    for (int i = 0; i < n; i++) { V d = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-0.3f, 1.2f))); sparks.push_back({at, d * (speed * rnd(0.3f, 1.f)), life * rnd(0.6f, 1.f), id}); }
}
static int vol_for(V at) { const V e = wk.empty() ? V() : wk[me].p; return (int)clampf(120.f - len(at - e) / 1200.f, 20, 120); }

/* ---- spawn, kill, pod ---- */
static bool free_at(V p, float clear) { for (const Box &b : solids) { if (b.z0 > 11000.f) continue; if (p.x > b.x0 - clear && p.x < b.x1 + clear && p.y > b.y0 - clear && p.y < b.y1 + clear) return false; } for (const Pad &q : pads) if (fabsf(p.x - q.x) < 9000.f && fabsf(p.y - q.y) < 9000.f) return false; return true; }
static V free_point() { for (int k = 0; k < 60; k++) { V a = P(rnd(-40, 700), rnd(-140, 540)); a.z = 0; if (free_at(a, 6000.f)) return a; } return P(600, 420); }
static V spawn_point(int self) {   /* INVENTED: where the deck sets you down is not shown; a clear spot farthest from the others of a few tries */
    V best = free_point(); float bs = -1;
    for (int n = 0; n < 12; n++) { V c = free_point(); float md = 1e30f;
        for (size_t j = 0; j < wk.size(); j++) { if ((int)j == self || wk[j].s != WS_ALIVE) continue; V d = wk[j].p - c; d.z = 0; md = (std::min)(md, len(d)); }
        md += rnd(0, 20000); if (md > bs) { bs = md; best = c; } }
    return best;
}
static void place(int i, V at) {
    Walker &w = wk[i]; w.p = V(at.x, at.y, 0); { V cc = P(200, 200); w.yaw = atan2f(-(cc.x - at.x), cc.y - at.y) + rnd(-0.2f, 0.2f); }   /* facing the middle of the city */
    w.s = WS_ALIVE; w.vz = 0; w.shield = shield_max; w.hit_flash = 0; w.fire_cool = 1.f; w.msl_cool = 3.f; w.lock_t = 0; w.lock_target = -1; w.lifting = 0; w.armed = true; w.ball_t = 0; w.self_t = 0; w.want_lift = false;
    w.route.clear(); w.top_wait = 0; w.aim = V(-sinf(w.yaw), cosf(w.yaw), 0); w.stuck_t = 0; w.stuck_p = w.p; w.regen_t = 0;
    if (i == me) { have_href = false; map_mode = false; xsfx(0, SND_GO, 110); }
}
static void kill_walker(int i, int killer) {
    Walker &w = wk[i]; if (w.s != WS_ALIVE) return;
    w.s = WS_POD; w.pod_t = 0; w.pod_dur = human(i) ? (std::max)(0.5f, respawn_delay - boot_len) : respawn_delay; w.lifting = 0; w.lock_t = 0; w.lock_target = -1; w.shield = 0;
    w.pod_from = w.p + V(0, 0, eye_h); w.pod_to = spawn_point(i); if (i == me) map_mode = false;
    V c = center(i);
    for (int k = 0; k < 26; k++) { Shard s; s.p = c + V(rnd(-600, 600), rnd(-600, 600), rnd(-1500, 1500)); s.v = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-0.2f, 1.2f))) * rnd(3000, 9000);
        s.w = V(rnd(-6, 6), rnd(-6, 6), rnd(-6, 6)); s.life = rnd(2.5f, 4.f); shards.push_back(s); }
    if (i != me) flashes.push_back({c, 0.f}); burst(c, 40, X_SPK_R, 9000.f, 2.f);   /* no flash in your own face: tape 278.5, the HUD just goes */
    if (killer >= 0 && killer != i) wk[killer].kills++;
    for (size_t k = 0; k < missiles.size();) { if (missiles[k].target == i) { missiles[k] = missiles.back(); missiles.pop_back(); } else k++; }
    xsfx(4, SND_BOOM, vol_for(c)); xsfx(5, SND_POD, vol_for(c));
    dl("t=%.1f walker %d destroyed by %d: kills %d", sim_t, i, killer, killer >= 0 ? wk[killer].kills : 0);
}
static void damage(int i, float d, int attacker, V at) {
    Walker &w = wk[i]; if (w.s != WS_ALIVE) return;
    w.shield -= d; w.hit_flash = 0.15f; w.regen_t = 6.f; w.ball_t = 0.12f; if (w.self_t <= 0.f) w.self_side = rnd(0, 1) < 0.5f ? -1 : 1; w.self_t = 0.35f; burst(at, 6, X_SPK_Y, 4000.f, 0.5f); xsfx(3, SND_HIT, vol_for(at));
    if (i == me) VPIX_Haptic(0, 0.08f, 0.6f);
    if (w.shield <= 0) kill_walker(i, attacker);
}
static V muzzle(const Walker &w, int side) { M Y = yawed(w.yaw); return V(w.p.x, w.p.y, w.p.z + 4750.f) + wrot(Y, V(side ? 3000.f : -3000.f, 3600.f, 900.f)); }
static void fire_laser(int i, V eye, V dir) {
    Walker &w = wk[i]; if (w.fire_cool > 0 || w.s != WS_ALIVE) return;
    w.fire_cool = laser_gap * (w.cpu || (i == me && autoplay) ? 1.8f / (skill > 0.2f ? skill : 0.2f) : 1.f); w.side ^= 1;
    Hit h = ray_world(eye, dir, laser_range, i); V end = eye + dir * h.t; V from = muzzle(w, w.side);
    float dur = clampf(len(end - from) / laser_speed, 0.03f, 1.f); beams.push_back({from, end, 0.f, dur, i, h.kind == 3 ? h.w : -1, laser_dmg, h.kind});   /* the bolt flies; the hit lands when it gets there */
    xsfx(i == me ? 1 : 2, SND_LASER, i == me ? 100 : vol_for(end) / 2);
}
static void launch_missile(int i, int target) {
    Walker &w = wk[i]; V from = muzzle(w, w.side ^ 1), d = norm(center(target) - from);
    missiles.push_back({from, d, i, target, 0.f, 0.f}); w.msl_cool = missile_cool; xsfx(7, SND_MISSILE, i == me ? 110 : vol_for(from));
    dl("t=%.1f walker %d fires a missile at %d", sim_t, i, target);
}
static void update_lock(int i, V eye, V aim, float dt, float cone) {   /* hold the sight on an enemy for lock_time seconds and the missile fires itself */
    Walker &w = wk[i]; int cand = -1; float best = 1e9f;
    if (w.msl_cool <= 0) for (size_t j = 0; j < wk.size(); j++) { if ((int)j == i || wk[j].s != WS_ALIVE) continue;
        V d = center(j) - eye; float dist = len(d); if (dist > missile_range || dist < 1.f) continue;
        float ang = acosf(clampf(dot(aim, d * (1.f / dist)), -1.f, 1.f)); if (ang > cone || ang >= best) continue;
        if (!los_to(i, eye, (int)j)) continue; best = ang; cand = (int)j; }
    if (cand >= 0) { if (cand != w.lock_target) { w.lock_target = cand; w.lock_t = 0; } else w.lock_t += dt; w.lock_grace = 0.4f; }
    else { w.lock_grace -= dt; if (w.lock_grace <= 0) { w.lock_t = 0; w.lock_target = -1; } }
    if (i == me && w.lock_target >= 0) { static float bp = 0; bp -= dt; if (bp <= 0) { bp = 0.5f; xsfx(6, SND_LOCK, 80); } }
    if (w.lock_target >= 0 && w.lock_t >= lock_time) { launch_missile(i, w.lock_target); w.lock_t = 0; w.lock_target = -1; }
}

/* ---- movement, lifts, bodies ---- */
static void step_body(int i, float dt) {
    Walker &w = wk[i]; if (w.s != WS_ALIVE) return;
    if (w.lifting) {
        w.lift_t += dt; float f = clampf(w.lift_t / lift_time, 0, 1), e = f * f * (3.f - 2.f * f); w.p.z = w.lift_z0 + (w.lift_z1 - w.lift_z0) * e; w.moved = 0;
        if (f >= 1.f) { w.lifting = 0; w.p.z = w.lift_z1; w.vz = 0; if (w.p.z > 100.f && (w.cpu || (i == me && autoplay))) w.top_wait = rnd(6, 12); }
        return;
    }
    float h = floor_at(w.p.x, w.p.y, w.p.z);
    if (w.p.z > h + 30.f) { w.vz -= grav * dt; w.p.z += w.vz * dt; if (w.p.z <= h) { w.p.z = h; w.vz = 0; if (i == me) xsfx(0, SND_HIT, 60); } }   /* walking off an edge costs nothing (guide) */
    else { w.p.z = h; w.vz = 0; }
    /* a lift: stop directly on it with the map off and it activates by itself (spoken, tape 5:05-5:20). Up only: "to return to the ground walk off the edge" */
    bool near_any = false, ai = (i != me || autoplay) && !w.remote;
    for (const Pad &pd : pads) { float dx = fabsf(w.p.x - pd.x), dy = fabsf(w.p.y - pd.y);
        if (dx < 5500.f && dy < 5500.f) near_any = true;
        if (dx < 4000.f && dy < 4000.f && w.armed && w.vz == 0.f && fabsf(w.p.z) < 200.f && (ai ? w.want_lift : (w.moved < 1.f && !(i == me ? map_mode : w.rmap)))) {
            w.lifting = 1; w.lift_t = 0; w.lift_z0 = 0.f; w.lift_z1 = slabs[pd.slab].z; w.armed = false; w.route.clear(); w.want_lift = false;
            xsfx(6, SND_LIFT, i == me ? 110 : vol_for(w.p)); dl("t=%.1f walker %d takes the lift up", sim_t, i); break; } }
    if (!near_any) w.armed = true;
}
static void drive(Walker &w, float fw, float tx, float dt) {
    if (w.s != WS_ALIVE || w.lifting) { w.moved = 0; return; }
    if (fabsf(tx) < 0.12f) tx = 0.f; if (fabsf(fw) < 0.12f) fw = 0.f;
    float rate = (fabsf(tx) > 0.85f && fabsf(fw) < 0.4f) ? turn_fast : turn_slow * fabsf(tx) / 0.5f;   /* straight over pivots (spoken); a half push or a push with walking turns at turn_slow */
    w.yaw -= (tx > 0 ? 1.f : -1.f) * (tx == 0 ? 0.f : (std::min)(rate, turn_fast)) * dt;
    V fwd(-sinf(w.yaw), cosf(w.yaw), 0); V was = w.p; w.p += fwd * (walk_speed * fw * dt); collide(w.p); w.moved = len(w.p - was) / dt;
}
static void read_sticks(const VPIX_Input &in, float &fw, float &tx) {
    fw = clampf(in.joy[1], -1.f, 1.f); tx = clampf(in.joy[0], -1.f, 1.f);   /* "push the joystick in the direction you wish to go", the tape's stick has four arrows, so back walks back; its speed is ours */
    if (in.vr) { fw = fabsf(in.joy_l[1]) > 0.12f ? clampf(in.joy_l[1], -1.f, 1.f) : in.brake; tx = clampf(in.joy_r[0], -1.f, 1.f); }   /* Bobby 11-03, VR: dual thumbsticks: right turns, left stick or left trigger walks; the gun follows the head */
    if (in.buttons & VPIX_BTN_WALK) fw = 1.f;                      /* desktop: W = stick forward */
}
static void step_player(const VPIX_Input &in, float dt) {
    Walker &w = wk[me]; if (w.s != WS_ALIVE || w.lifting) { w.moved = 0; return; }
    float fw, tx; read_sticks(in, fw, tx); drive(w, fw, tx, dt);
}

static bool isai(int i) { return i != me || autoplay; }
/* the hover deck: seen from the ground as a saucer in the sky once you eject (tape 279-281), it stands outside the city at about twice the towers' height, facing in (286-288, 346-348) */
static const float HIP_Z_OWN = 4750.f;
static V deck_pos() { V a = P(600, 110); a.z = 62000.f; return a; }
static V city_mid() { return P(200, 200); }
static V seat_pos() { V d = deck_pos(); V to = city_mid() - d; to.z = 0; to = norm(to); return d + to * 900.f + V(0, 0, HIP_Z_OWN + 3400.f); }
static V fly_pos(V from, float u) {   /* from where you were, low across the plain round the city, up behind the deck into the seat. The tape's path over 279-288 s, by eye */
    const V d = deck_pos(), s0 = seat_pos(); V to = city_mid() - d; to.z = 0; to = norm(to);
    V k[5] = {from, from + (V(from.x, from.y, 0) * -0.1f) + V(0, 0, 8000), P(640, 470) + V(0, 0, 24000), d - to * 60000.f + V(0, 0, 12000), s0}; k[1].x += 8000.f;
    float t = clampf(u, 0.f, 1.f) * 4.f; int n = (int)t; if (n > 3) n = 3; float e = t - n, e2 = e * e, e3 = e2 * e;
    V p0 = k[n > 0 ? n - 1 : 0], p1 = k[n], p2 = k[n + 1], p3 = k[n + 2 > 4 ? 4 : n + 2];
    return (p1 * 2.f + (p2 - p0) * e + (p0 * 2.f - p1 * 5.f + p2 * 4.f - p3) * e2 + (p1 * 3.f - p0 - p2 * 3.f + p3) * e3) * 0.5f;
}
static void boot_view(const Walker &w, float t, V &eye, V &fwd) {   /* the seat on the deck, gliding and sinking in the boot's 6.5 s to where the walker lands (tape 11-17, 349-356) */
    V to = city_mid() - deck_pos(); to.z = 0; to = norm(to); float u = clampf(t / (boot_len * 0.92f), 0.f, 1.f), e = u * u * (3.f - 2.f * u); V land(w.pod_to.x, w.pod_to.y, eye_h), s0 = seat_pos();
    eye = s0 + (land - s0) * e; eye.z = s0.z + (land.z - s0.z) * (u * u * u);   /* stays high over the city, sinks late */ V f1 = wrot(yawed(atan2f(city_mid().x - w.pod_to.x, 0.f) * 0.f + atan2f(-(city_mid().x - w.pod_to.x), city_mid().y - w.pod_to.y)), V(0, 1, 0)); fwd = to + (f1 - to) * e; fwd.z = -0.28f * (1.f - e); fwd = norm(fwd);
}

/* the opponents. Which seats a lone player's enemies fill, and how they think, is INVENTED: footage shows walkers that stalk, then stand and fire, lock missiles, and take no cover (tape 268-278, Play Expo 57-112) */
static void leave_slab(Walker &w) {
    const Slab *s = NULL; for (const Slab &q : slabs) if (fabsf(w.p.x - q.cx) <= q.hx && fabsf(w.p.y - q.cy) <= q.hy) s = &q;
    w.route.clear(); float a = rnd(0, 6.28f); w.route.push_back(V((s ? s->cx : w.p.x) + cosf(a) * 14000.f, (s ? s->cy : w.p.y) + sinf(a) * 14000.f, 0));   /* walk off the edge: no damage (spoken) */
}
static void pick_route(int i) {
    Walker &w = wk[i]; w.route.clear(); w.want_lift = false; float q = rnd(0, 1);
    auto clear = [&](V t) { V a = w.p + V(0, 0, 2000), d = t - w.p; d.z = 0; float L = len(d); if (L < 1.f) return true; Hit h = ray_world(a, d * (1.f / L), L, i); return h.kind == 0 || h.kind == 3; };
    if (q < 0.2f && !pads.empty()) { const Pad &pd = pads[(size_t)rnd(0, (float)pads.size() - 0.001f)]; w.route.push_back(V(pd.x, pd.y, 0)); w.want_lift = true; return; }
    if (q < 0.7f) { int best = -1; float bd = 1e30f; for (size_t j = 0; j < wk.size(); j++) { if ((int)j == i || wk[j].s != WS_ALIVE) continue; float d = len(wk[j].p - w.p); if (d < bd) { bd = d; best = (int)j; } } if (best >= 0) { V t = wk[best].p; t.z = 0; if (clear(t)) { w.route.push_back(t); return; } } }
    V fp = free_point(); for (int n = 0; n < 16 && !clear(fp); n++) fp = free_point(); w.route.push_back(fp);
}
static void step_ai(int i, float dt) {
    Walker &w = wk[i]; if (w.s != WS_ALIVE) return;
    V eye = w.p + V(0, 0, eye_h); int tgt = -1; float bd = 1e30f;
    for (size_t j = 0; j < wk.size(); j++) { if ((int)j == i || wk[j].s != WS_ALIVE) continue;
        float d = len(center(j) - eye); if (d < missile_range && d < bd && los_to(i, eye, (int)j)) { bd = d; tgt = (int)j; } }
    V want = V(-sinf(w.yaw), cosf(w.yaw), 0); if (tgt >= 0) want = norm(center(tgt) - eye);
    w.aim = norm(w.aim + (want - w.aim) * (std::min)(1.f, dt * 2.5f * skill));
    bool fighting = false;
    if (!w.lifting) {
        if (tgt >= 0 && bd < laser_range * 0.8f) {   /* stand and fight */
            V d = center(tgt) - w.p; float dy = wrap(atan2f(-d.x, d.y) - w.yaw); w.yaw += clampf(dy, -1.5f * dt, 1.5f * dt); w.moved = 0; fighting = true;
        } else if (w.top_wait > 0) { w.top_wait -= dt; w.moved = 0; fighting = true; if (w.top_wait <= 0) leave_slab(w); }
        else {
            if (w.route.empty() && !w.want_lift) { if (w.p.z > 100.f && w.vz == 0.f) leave_slab(w); else if (w.p.z < 100.f) pick_route(i); }
            else if (w.route.empty() && w.want_lift) { w.want_lift = false; pick_route(i); }
            if (!w.route.empty()) {
                V tp = w.route.back(), d = tp - w.p; d.z = 0; float dist = len(d);
                if (dist < 2500.f) { if (w.want_lift) w.moved = 0; w.route.pop_back(); if (w.want_lift && !w.lifting) return; }
                else { float wy = atan2f(-d.x, d.y), dy = wrap(wy - w.yaw); w.yaw += clampf(dy, -1.4f * dt, 1.4f * dt);
                    float sp = walk_speed * 0.8f * (fabsf(dy) < 0.6f ? 1.f : 0.2f); V fwd(-sinf(w.yaw), cosf(w.yaw), 0), was = w.p;
                    w.p += fwd * (sp * dt); collide(w.p); w.moved = len(w.p - was) / dt; }
            } else w.moved = 0;
        }
    }
    w.stuck_t += dt;
    if (w.stuck_t > 2.f) { if (!fighting && !w.lifting && w.vz == 0.f && len(w.p - w.stuck_p) < walk_speed * 0.9f && w.p.z < 100.f) { w.route.clear(); w.want_lift = false; }
        w.stuck_t = 0; w.stuck_p = w.p; }
    if (tgt >= 0 && bd < laser_range) {
        V dir = norm(center(tgt) - eye);
        if (dot(w.aim, dir) > 0.995f) { float e = (0.012f + bd / 6.0e6f) / (skill > 0.2f ? skill : 0.2f);
            fire_laser(i, eye, norm(w.aim + V(rnd(-e, e), rnd(-e, e), rnd(-e, e)))); }
    }
    update_lock(i, eye, w.aim, dt, 0.12f);
}

static void step_missiles(float dt) {
    for (size_t k = 0; k < missiles.size();) {
        Missile m = missiles[k]; m.t += dt;
        V want = (m.target >= 0 && m.target < (int)wk.size() && wk[m.target].s == WS_ALIVE) ? norm(center(m.target) - m.p) : m.dir;
        m.dir = norm(m.dir + (want - m.dir) * clampf(dt * 2.2f, 0.f, 1.f));
        float step = missile_speed * dt; Hit h = ray_world(m.p, m.dir, step, m.owner);
        m.smoke -= dt; if (m.smoke <= 0) { m.smoke = 0.04f; if (sparks.size() < 600) sparks.push_back({m.p - m.dir * 1400.f, V(), 0.6f, X_SPK_Y}); }
        if (m.target == me && wk[me].s == WS_ALIVE) { static float wt = 0; wt -= dt; if (wt <= 0) { wt = 0.4f; xsfx(5, SND_WARN, 100); } }
        if (h.kind || m.t > 10.f) {
            V at = m.p + m.dir * (h.kind ? h.t : 0.f); int owner = m.owner, hw = h.kind == 3 ? h.w : -1;
            missiles[k] = missiles.back(); missiles.pop_back();
            if (h.kind) { burst(at, 25, X_SPK_Y, 6000.f, 0.8f); flashes.push_back({at, 0.f}); xsfx(4, SND_BOOM, vol_for(at)); if (hw >= 0) damage(hw, missile_dmg, owner, at); }
            continue;
        }
        m.p += m.dir * step; missiles[k] = m; k++;
    }
}
static void step_effects(float dt) {
    for (size_t i = 0; i < beams.size();) { Beam &b = beams[i]; b.t += dt; if (b.t >= b.dur) { Beam q = b; beams[i] = beams.back(); beams.pop_back();
            if (q.target >= 0 && q.target < (int)wk.size() && gs == G_PLAY) damage(q.target, q.dmg, q.owner, q.b); else if (q.kind > 0) burst(q.b, 4, X_SPK_Y, 2500.f, 0.4f); } else i++; }
    for (size_t i = 0; i < sparks.size();) { Spark &k = sparks[i]; k.v.z -= grav * 0.25f * dt; k.p += k.v * dt; k.life -= dt;
        if (k.life <= 0) { sparks[i] = sparks.back(); sparks.pop_back(); } else i++; }
    for (size_t i = 0; i < shards.size();) { Shard &s = shards[i]; s.v.z -= grav * 0.5f * dt; s.p += s.v * dt; buggy::rotate(s.R, s.w, dt); s.life -= dt;
        if (s.p.z < 0) { s.p.z = 0; s.v = s.v * 0.2f; s.w = s.w * 0.3f; }
        if (s.life <= 0) { shards[i] = shards.back(); shards.pop_back(); } else i++; }
    for (size_t i = 0; i < flashes.size();) { flashes[i].t += dt; if (flashes[i].t > 0.5f) { flashes[i] = flashes.back(); flashes.pop_back(); } else i++; }
}

/* ---- head: the camera is the tracked head turned by the walker's yaw (sit-down: seat fixed, head tracked only) ---- */
static V head_aim(const VPIX_Input &in) {
    const Walker &w = wk[me]; M Y = yawed(w.yaw);
    if (autoplay) return w.aim;
    if (!in.head.valid) return wrot(Y, V(0, 1, 0));
    return norm(wrot(Y, V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2])));
}
static V head_eye(const VPIX_Input &in) {
    const Walker &w = wk[me];
    M Y = yawed(w.yaw); V eb(w.p.x, w.p.y, w.p.z + eye_h);
    if (autoplay || !in.head.valid) return eb;
    V hp(in.head.p[0], in.head.p[1], in.head.p[2]); if (!have_href) { href = hp; have_href = true; }
    V off = hp - href; off.x = clampf(off.x, -600, 600); off.y = clampf(off.y, -600, 600); off.z = clampf(off.z, -600, 600);
    return eb + wrot(Y, off);
}

/* ---- link play (vpix_1k_net.h): pod 1 runs the whole city (four seats; a pod that joins takes a CPU walker's seat, the rest stay CPU walkers). INVENTED: the original is one pod against stand-ins, so a linked match is the same round with up to four people.
   Another pod sends its sticks, its head direction and the trigger/map bits; pod 1 walks and fires for it and sends back every walker, shot and bolt. A start-together match. ---- */
#pragma pack(push, 1)
struct NetIn { float fw, tx, aim[3]; uint8_t flags; };   /* flags 1 = trigger, 2 = map on; aim = head forward in the walker's frame */
struct NetW { float p[3], yaw, moved, vz, shield, ball_t, self_t, lock_t, pod_t, pod_dur, pod_from[3], pod_to[3]; uint8_t s, look, kills, flags, lock_target; };   /* flags 1 lifting, 2 muzzle side, 4 self_side +, 8 remote */
struct NetM { float p[3], dir[3], t; int8_t owner, target; };
struct NetBm { float a[3], b[3], t, dur; int8_t owner; };
struct NetHd { uint8_t gs, n, nm, nb, winmask, pad[3]; float st_t, time_left; };
#pragma pack(pop)
enum { NET_MMAX = 12, NET_BMAX = 14 };
static bool slave_game = false;
static void net3(float *o, V a) { o[0] = a.x; o[1] = a.y; o[2] = a.z; }
static V net3(const float *o) { return V(o[0], o[1], o[2]); }
static bool net_drive(int i, float dt) {   /* master: true = seat i is driven by its pod */
    Walker &w = wk[i]; const NetIn *p = (const NetIn *)v1knet::seat_input(i, sizeof(NetIn));
    if (!p) { if (w.remote) { w.remote = false; w.rmap = false; w.cpu = true; w.route.clear(); w.want_lift = false; dl("t=%.1f link: seat %d back to the computer", sim_t, i); } return false; }
    if (!w.remote) { w.remote = true; w.cpu = false; dl("t=%.1f link: seat %d is a person now", sim_t, i); }
    w.rmap = (p->flags & 2) != 0;
    drive(w, clampf(p->fw, -1.f, 1.f), clampf(p->tx, -1.f, 1.f), dt);
    if (w.s == WS_ALIVE) {
        V loc(p->aim[0], p->aim[1], p->aim[2]); if (!(len(loc) > 0.5f)) loc = V(0, 1, 0);
        V eye(w.p.x, w.p.y, w.p.z + eye_h), aim = norm(wrot(yawed(w.yaw), norm(loc))); w.aim = aim;
        if (w.rmap) { w.lock_t = 0; w.lock_target = -1; }   /* weapons are off while the map is on */
        else { if (p->flags & 1) fire_laser(i, eye, aim); update_lock(i, eye, aim, dt, 0.09f); }
    }
    return true;
}
static void net_master() {
    if (!v1knet::due(30)) return;
    if (gs == G_ATTRACT || gs == G_CREDIT || wk.empty()) { v1knet::send_snapshot(NULL, 0); return; }
    static uint8_t buf[1400]; NetHd h; memset(&h, 0, sizeof(h)); h.gs = (uint8_t)gs; h.n = (uint8_t)(std::min)(wk.size(), (size_t)v1knet::SEATS); h.st_t = st_t; h.time_left = time_left;
    h.nm = (uint8_t)(std::min)(missiles.size(), (size_t)NET_MMAX); h.nb = (uint8_t)(std::min)(beams.size(), (size_t)NET_BMAX);
    for (int i = 0; i < h.n; i++) { int mo = 0; for (int j = 0; j < h.n; j++) if (j != i) mo = (std::max)(mo, wk[j].kills); if (wk[i].kills > mo) h.winmask |= (uint8_t)(1 << i); }
    size_t o = sizeof(h); memcpy(buf, &h, sizeof(h));
    for (int i = 0; i < h.n; i++) { const Walker &w = wk[i]; NetW n; memset(&n, 0, sizeof(n)); net3(n.p, w.p); n.yaw = w.yaw; n.moved = w.moved; n.vz = w.vz; n.shield = w.shield; n.ball_t = w.ball_t; n.self_t = w.self_t; n.lock_t = w.lock_t; n.pod_t = w.pod_t; n.pod_dur = w.pod_dur;
        net3(n.pod_from, w.pod_from); net3(n.pod_to, w.pod_to); n.s = (uint8_t)w.s; n.look = (uint8_t)w.look; n.kills = (uint8_t)(std::min)(w.kills, 255); n.lock_target = (uint8_t)(w.lock_target + 1);
        n.flags = (w.lifting ? 1 : 0) | (w.side ? 2 : 0) | (w.self_side > 0 ? 4 : 0) | (w.remote ? 8 : 0); memcpy(buf + o, &n, sizeof(n)); o += sizeof(n); }
    for (int k = 0; k < h.nm; k++) { const Missile &m = missiles[k]; NetM n; net3(n.p, m.p); net3(n.dir, m.dir); n.t = m.t; n.owner = (int8_t)m.owner; n.target = (int8_t)m.target; memcpy(buf + o, &n, sizeof(n)); o += sizeof(n); }
    for (int k = 0; k < h.nb; k++) { const Beam &b = beams[k]; NetBm n; net3(n.a, b.a); net3(n.b, b.b); n.t = b.t; n.dur = b.dur; n.owner = (int8_t)b.owner; memcpy(buf + o, &n, sizeof(n)); o += sizeof(n); }
    v1knet::send_snapshot(buf, o);
}
static void net_log() {
    static double next = 0; if (sim_t < next) return; next = sim_t + 5.0;
    if (wk.size() < 4) { dl("t=%.1f link: %s, title, pods heard %d", sim_t, v1knet::master ? "master" : "slave", v1knet::live_pods()); return; }
    dl("t=%.1f link: %s seat %d gs %d left %.0f | w0 %s(%.0f %.0f) k%d | w1 %s(%.0f %.0f) k%d | w2 %s(%.0f %.0f) k%d | w3 %s(%.0f %.0f) k%d", sim_t, v1knet::master ? "master" : "slave", me, (int)gs, time_left,
       wk[0].remote ? "pod " : "", wk[0].p.x, wk[0].p.y, wk[0].kills, wk[1].remote ? "pod " : "", wk[1].p.x, wk[1].p.y, wk[1].kills, wk[2].remote ? "pod " : "", wk[2].p.x, wk[2].p.y, wk[2].kills, wk[3].remote ? "pod " : "", wk[3].p.x, wk[3].p.y, wk[3].kills);
}
static void net_leave(const char *why) {
    slave_game = false; gs = G_ATTRACT; st_t = 0; wk.clear(); missiles.clear(); beams.clear(); sparks.clear(); shards.clear(); flashes.clear(); me = 0; map_mode = false; have_href = false;
    dl("t=%.1f link: %s, back to the title", sim_t, why);
}
static void net_apply(const uint8_t *d, size_t len_, uint8_t round) {
    NetHd h; if (len_ < sizeof(h)) return; memcpy(&h, d, sizeof(h));
    if (h.n < 1 || h.n > v1knet::SEATS || h.nm > NET_MMAX || h.nb > NET_BMAX || len_ < sizeof(h) + h.n * sizeof(NetW) + h.nm * sizeof(NetM) + h.nb * sizeof(NetBm)) return;
    static uint8_t my_round = 0; me = v1knet::my_seat; if (me >= h.n) return;
    const bool fresh = !slave_game || round != my_round || wk.size() != h.n; my_round = round;
    if (fresh) { wk.assign(h.n, Walker()); missiles.clear(); beams.clear(); sparks.clear(); shards.clear(); flashes.clear(); map_mode = false; have_href = false; slave_game = true; dl("t=%.1f link: in the master's round, seat %d of %d", sim_t, me, (int)h.n); }
    size_t o = sizeof(h);
    for (int i = 0; i < h.n; i++, o += sizeof(NetW)) { NetW n; memcpy(&n, d + o, sizeof(n)); Walker &w = wk[i];
        if (!fresh) {
            if (w.s == WS_ALIVE && n.s != WS_ALIVE) { V c = center(i); burst(c, 40, X_SPK_R, 9000.f, 2.f); if (i != me) flashes.push_back({c, 0.f}); xsfx(4, SND_BOOM, vol_for(c)); xsfx(5, SND_POD, vol_for(c)); if (i == me) map_mode = false; }
            if (i == me && n.shield < w.shield - 0.01f && n.s == WS_ALIVE) { xsfx(3, SND_HIT, 100); VPIX_Haptic(0, 0.08f, 0.6f); }
            if (i == me && w.s != WS_ALIVE && n.s == WS_ALIVE) { have_href = false; xsfx(0, SND_GO, 110); }
        }
        w.p = net3(n.p); w.yaw = n.yaw; w.moved = n.moved; w.vz = n.vz; w.shield = n.shield; w.ball_t = n.ball_t; w.self_t = n.self_t; w.lock_t = n.lock_t; w.pod_t = n.pod_t; w.pod_dur = n.pod_dur;
        w.pod_from = net3(n.pod_from); w.pod_to = net3(n.pod_to); w.s = (WState)n.s; w.look = n.look; w.kills = n.kills; w.lock_target = (int)n.lock_target - 1;
        w.lifting = n.flags & 1; w.side = (n.flags & 2) ? 1 : 0; w.self_side = (n.flags & 4) ? 1 : -1; w.remote = (n.flags & 8) != 0; w.cpu = false; }
    missiles.clear();
    for (int k = 0; k < h.nm; k++, o += sizeof(NetM)) { NetM n; memcpy(&n, d + o, sizeof(n)); Missile m; m.p = net3(n.p); m.dir = net3(n.dir); m.t = n.t; m.owner = n.owner; m.target = n.target; m.smoke = 0; missiles.push_back(m);
        if (!fresh && n.t < 0.034f) xsfx(7, SND_MISSILE, n.owner == me ? 110 : vol_for(m.p)); }
    beams.clear();
    for (int k = 0; k < h.nb; k++, o += sizeof(NetBm)) { NetBm n; memcpy(&n, d + o, sizeof(n)); Beam b; b.a = net3(n.a); b.b = net3(n.b); b.t = n.t; b.dur = n.dur; b.owner = n.owner; b.target = -1; b.dmg = 0; b.kind = 0; beams.push_back(b);
        if (!fresh && n.t < 0.034f) xsfx(n.owner == me ? 1 : 2, SND_LASER, n.owner == me ? 100 : vol_for(b.b) / 2); }
    gs = (GS)h.gs; st_t = h.st_t; time_left = h.time_left; won = ((h.winmask >> me) & 1) != 0;
}
static void net_slave_step(float dt) {
    static float wt = 0;
    for (size_t i = 0; i < wk.size(); i++) { Walker &w = wk[i]; w.phase += w.moved * dt * 0.0006f; if (w.ball_t > 0) w.ball_t -= dt; if (w.self_t > 0) w.self_t -= dt; }
    if (autoplay && !wk.empty()) { Walker &w = wk[me]; w.aim = V(-sinf(w.yaw), cosf(w.yaw), 0); }
    for (Missile &m : missiles) { m.p += m.dir * (missile_speed * dt); m.t += dt; if (m.target == me && wk[me].s == WS_ALIVE) { wt -= dt; if (wt <= 0) { wt = 0.4f; xsfx(5, SND_WARN, 100); } } }
    if (gs == G_PLAY) time_left -= dt;
}

/* ---- rounds ---- */
static void start_round() {
    wk.assign(v1knet::on ? (size_t)v1knet::SEATS : 1 + (size_t)n_cpu, Walker());   /* linked: four seats, every one a CPU walker until a pod takes it */
    if (v1knet::on) v1knet::begin_round((int)wk.size()); missiles.clear(); beams.clear(); sparks.clear(); shards.clear(); flashes.clear();
    for (size_t i = 0; i < wk.size(); i++) { Walker &w = wk[i]; w.cpu = i != (size_t)me; w.look = i == 2 ? 1 : i == 3 ? 3 : 0; w.kills = 0; V at = spawn_point((int)i); { static const bool nr = getenv("EXOREX_NEAR") != NULL; if (nr) { at = P(600, 420); at.x += i * 9000.f; at.y += (i & 1) * 26000.f; } } w.p = at; place((int)i, at); w.pod_to = at; }   /* dev EXOREX_NEAR: all start close on the plain, to see a fight */
    time_left = game_len; map_mode = false; have_href = false; won = false;
    dl("t=%.1f round start, %zu walkers, %.0f s", sim_t, wk.size(), game_len);
}
static void end_round() {
    int mo = 0; for (size_t i = 1; i < wk.size(); i++) mo = (std::max)(mo, wk[i].kills);
    won = wk[me].kills > mo || getenv("EXOREX_WIN");   /* INVENTED: the footage shows FREEDOM or a skull but not the rule; here the sole top score wins */
    gs = G_END; st_t = 0; map_mode = false;
    dl("t=%.1f round end: you %d, best other %d -> %s", sim_t, wk[me].kills, mo, won ? "FREEDOM" : "skull");
}
static void step_play(const VPIX_Input &in, float dt) {
    static float prox_t = 0;
    for (size_t i = 0; i < wk.size(); i++) { Walker &w = wk[i];
        if (w.fire_cool > 0) w.fire_cool -= dt;
        if (w.msl_cool > 0) w.msl_cool -= dt;
        if (w.hit_flash > 0) w.hit_flash -= dt;
        w.phase += w.moved * dt * 0.0006f;
        if (w.ball_t > 0) w.ball_t -= dt; if (w.self_t > 0) w.self_t -= dt;
        if (w.s == WS_POD) { w.pod_t += dt; if (w.pod_t >= w.pod_dur) { if (human((int)i)) { w.s = WS_BOOT; w.pod_t = 0; } else place((int)i, V(w.pod_to.x, w.pod_to.y, 0)); } continue; }
        if (w.s == WS_BOOT) { w.pod_t += dt; if (w.pod_t >= boot_len) place((int)i, V(w.pod_to.x, w.pod_to.y, 0)); continue; }
        if (w.regen_t > 0) w.regen_t -= dt; else if (w.shield < shield_max) w.shield = (std::min)(shield_max, w.shield + regen * dt);
        if ((int)i == me && !autoplay) step_player(in, dt);
        else if (v1knet::on && i >= 1 && net_drive((int)i, dt)) {}   /* a person on another pod */
        else step_ai((int)i, dt);
        step_body((int)i, dt);
    }
    { static const float km = envf("EXOREX_KILLME", 0.f); static bool kd = false; if (km > 0.f && !kd && game_len - time_left > km && wk.size() > 1 && wk[me].s == WS_ALIVE) { kd = true; damage(me, 1000.f, 1, wk[me].p); } }   /* dev: see the eject flight */
    { static const char *pn = getenv("EXOREX_PIN"); if (pn && *pn && wk.size() > 1) { float py = 0; int lk = 1; sscanf(pn, "%d,%f", &lk, &py); Walker &q = wk[1]; q.p = V(0, 0, 0); q.yaw = py * XPI / 180.f; q.look = lk; q.moved = 0; q.s = WS_ALIVE; q.shield = shield_max; q.lifting = 0; q.phase = 0.f; } }   /* dev: a posed walker at the middle crossing, for pictures */
    { static const char *pm = getenv("EXOREX_PINME"); if (pm && *pm) { float x = 0, y = 0, a = 0; sscanf(pm, "%f,%f,%f", &x, &y, &a); Walker &q = wk[me]; q.p = V(x, y, 0); q.yaw = a * XPI / 180.f; q.aim = V(-sinf(q.yaw), cosf(q.yaw), 0); q.moved = 0; q.s = WS_ALIVE; q.shield = shield_max * 0.7f; q.kills = 2; q.lifting = 0; } }
    Walker &p = wk[me];
    if (!autoplay && p.s == WS_ALIVE) {
        V eye = head_eye(in), aim = head_aim(in);
        if (map_mode) { p.lock_t = 0; p.lock_target = -1; }   /* weapons are off while the map is on */
        else { if ((in.buttons & VPIX_BTN_TRIGGER) != 0) fire_laser(me, eye, aim); update_lock(me, eye, aim, dt, 0.09f); }
    }
    if (p.s == WS_ALIVE) { bool near_e = false; for (size_t j = 1; j < wk.size(); j++) if (wk[j].s == WS_ALIVE && len(wk[j].p - p.p) < 25000.f) near_e = true;
        prox_t -= dt; if (near_e && prox_t <= 0) { prox_t = 1.f; xsfx(5, SND_WARN, 80); } }   /* proximity warning (the tape's voice is not copied) */
    time_left -= dt; if (time_left <= 0) end_round();
}

/* ---- drawing ---- */
static const float HIP_Z = 4750.f;
static void put_legs(VPIX_Frame &f, const Walker &w, int B, const M &Y, V hip, int top) {
    float sw = w.moved > 1.f ? sinf(w.phase) * 0.5f : 0.f; if (w.lifting) sw = 0; else if (w.vz != 0.f) sw = 0.3f;
    for (int k = 0; k < 2; k++) { float a = k ? -sw : sw; V hj = hip + Y.r[0] * (k ? 1900.f : -1900.f); M th = pitched(Y, a + 0.55f), sn = pitched(Y, a - 0.7f); V kn = hj - th.r[2] * 2400.f, an = kn - sn.r[2] * 2800.f;
        VPIX_Instance &q0 = dput(f, B + 2 + k, th, hj); q0.on_top = top; VPIX_Instance &q1 = dput(f, B + 4 + k, sn, kn); q1.on_top = top; VPIX_Instance &q2 = dput(f, B + 8 + k, Y, V(an.x, an.y, (std::max)(w.p.z + 550.f, an.z))); q2.on_top = top; }
}
static void put_walker(VPIX_Frame &f, const Walker &w) {
    int B = X_WALK + 12 * w.look; M Y = yawed(w.yaw); V hip(w.p.x, w.p.y, w.p.z + HIP_Z);
    if (w.ball_t > 0.f) { M I; VPIX_Instance &q = dput(f, w.shield < shield_max * 0.5f ? X_BALLR : X_BALL, I, V(w.p.x, w.p.y, w.p.z + 5200.f)); q.bright = 1; q.glass = 130; }   /* the translucent ball over a walker you hit, pale (tape 38, Play Expo 110); red once its shield is well down, reading the narrator's red flash */
    dput(f, B, Y, hip); dput(f, B + 1, Y, hip).bright = 1;
    put_legs(f, w, B, Y, hip, 0);
    dput(f, B + 6, Y, hip + wrot(Y, V(-3000.f, 200.f, 900.f))); dput(f, B + 7, Y, hip + wrot(Y, V(3000.f, 200.f, 900.f)));
}
static void put_own_legs(VPIX_Frame &f, const Walker &w) {   /* the player's own legs, seen past the cabin */
    int B = X_WALK + 12 * w.look; M Y = yawed(w.yaw); put_legs(f, w, B, Y, V(w.p.x, w.p.y, w.p.z + HIP_Z), 2);
}
static bool project(const M &cam, V eye, V p, float &sx, float &sy, float &depth) {
    V d = p - eye; float x = dot(cam.r[0], d), y = dot(cam.r[1], d), z = dot(cam.r[2], d); if (y < 100.f) return false;
    float k = 176.f / tanf(35.f * XPI / 180.f); sx = 176.f + x / y * k; sy = 144.f - z / y * k; depth = y; return true;
}
static void hud_radar(VPIX_Frame &f, const Walker &w, V eye, const M &cam) {
    /* a double-lined strip with bracket ends, the wavy profile inside, the taller field-of-view box with heavy top and bottom bands; hash marks are opponents, the half circle is the nearest lift (its meaning is ours) */
    const float X0 = 67.f, X1 = 327.f, Y0 = 234.f, Y1 = 254.f, YM = 244.f, bw = (X1 - X0) / 72.f; float px = X0, py = YM;
    ln(f, X0, Y0, X1, Y0); ln(f, X0, Y1, X1, Y1); ln(f, X0, Y0 - 3, X0, Y1 + 3); ln(f, X1, Y0 - 3, X1, Y1 + 3); ln(f, X0, Y0 - 3, X0 + 3, Y0 - 3); ln(f, X0, Y1 + 3, X0 + 3, Y1 + 3); ln(f, X1, Y0 - 3, X1 - 3, Y0 - 3); ln(f, X1, Y1 + 3, X1 - 3, Y1 + 3);
    for (int b = 0; b < 72; b++) {
        float rel = XPI - (b + 0.5f) / 72.f * 2.f * XPI, yb = w.yaw + rel; V dir(-sinf(yb), cosf(yb), 0); float elev = 0.f;
        for (const Box &bx : solids) { Box bb = bx; bb.z0 = -1e9f; bb.z1 = 1e9f; float t = ray_box(eye, dir, bb, 300000.f); if (t > 0.f) elev = (std::max)(elev, atan2f(bx.z1 - eye.z, t)); }
        float x = X0 + (b + 0.5f) * bw, y = YM - clampf(elev, 0.f, 0.9f) * 6.f - sinf(b * 0.9f) * 1.2f - sinf(b * 2.3f) * 0.9f;
        ln(f, px, py, x, y); px = x; py = y; }
    ln(f, px, py, X1, YM);
    float hy = atan2f(-cam.r[1].x, cam.r[1].y), rel_h = wrap(hy - w.yaw), xc = X0 + (X1 - X0) * (0.5f - rel_h / (2.f * XPI)), half = (X1 - X0) * (35.f / 360.f), xl = clampf(xc - half, X0, X1), xr = clampf(xc + half, X0, X1);
    ln(f, xl, Y0 - 5, xl, Y1 + 5); ln(f, xr, Y0 - 5, xr, Y1 + 5); for (int k = 0; k < 2; k++) { ln(f, xl, Y0 - 3 + k, xr, Y0 - 3 + k); ln(f, xl, Y1 + 2 + k, xr, Y1 + 2 + k); } if (xc > X0 && xc < X1) { ln(f, xc, Y0 - 5, xc, Y0 + 1); ln(f, xc, Y1 - 1, xc, Y1 + 5); }
    float bd = 1e30f; V bp; bool any = false; for (const Pad &p : pads) { V d = V(p.x, p.y, 0) - w.p; d.z = 0; float l = len(d); if (l < bd) { bd = l; bp = d; any = true; } }
    if (any) { float rel = wrap(atan2f(-bp.x, bp.y) - w.yaw), x = X0 + (X1 - X0) * (0.5f - rel / (2.f * XPI)); float qx = x - 8.f, qy = Y0 + 1.f; for (int s = 1; s <= 8; s++) { float a = XPI - XPI * s / 8.f, xx = x + cosf(a) * 8.f, yy = Y0 + 1.f + sinf(a) * 8.f; ln(f, qx, qy, xx, yy); qx = xx; qy = yy; } }
    for (size_t j = 0; j < wk.size(); j++) { if ((int)j == me || wk[j].s != WS_ALIVE) continue; V d = center(j) - eye; float bear = atan2f(-d.x, d.y), rel = wrap(bear - w.yaw);
        float x = X0 + (X1 - X0) * (0.5f - rel / (2.f * XPI)); bool inside = fabsf(wrap(bear - hy)) < 35.f * XPI / 180.f;
        if (inside && fmodf((float)sim_t, 0.4f) < 0.2f) ln(f, x, 240, x, 250, 0xFF, 0xE0, 0x30); else ln(f, x, 240, x, 250); }   /* hash marks; yellow blink when in front */
}
static void hud_map(VPIX_Frame &f, const Walker &w, V eye, const M &cam) {
    /* MAP MODE, rebuilt: not on the ground. A flat two-dimensional plan standing dead ahead of the walker, you at its middle slightly below the eye line, forward up, turning as you turn; you look left, right, up and down over it ("look north south east and west to scan"). Solved from tape 126-165 s: looking level it is square on, looking up its lines run to two vanishing points 85-90 deg apart */
    const float D = 10000.f, S = 0.1f / 14000.f; M Y = yawed(w.yaw); V rgt = wrot(Y, V(1, 0, 0)), fw = wrot(Y, V(0, 1, 0)), up(0, 0, 1);
    auto pt = [&](V m, float &sx, float &sy) { V r = m - w.p; r.z = 0; float u = dot(r, rgt) * S, v = dot(r, fw) * S; V a = eye + fw * D + rgt * (u * D) + up * ((v - 0.1f) * D); float dp; return project(cam, eye, a, sx, sy, dp); };
    auto seg = [&](V a, V b, uint8_t r, uint8_t g, uint8_t bl) { float x0, y0, x1, y1; if (pt(a, x0, y0) && pt(b, x1, y1) && fabsf(x0 - x1) < 900.f && fabsf(y0 - y1) < 900.f) { VPIX_Frame::Line l; l.x0 = x0; l.y0 = y0; l.x1 = x1; l.y1 = y1; l.rgb[0] = r; l.rgb[1] = g; l.rgb[2] = bl; f.lines.push_back(l); l.x0 += 0.5f; l.x1 += 0.5f; f.lines.push_back(l); } };
    auto rect = [&](float cx, float cy, float hx, float hy) { V a(cx - hx, cy - hy, 0), b(cx + hx, cy - hy, 0), c(cx + hx, cy + hy, 0), d(cx - hx, cy + hy, 0); seg(a, b, 0xE0, 0xE0, 0xE0); seg(b, c, 0xE0, 0xE0, 0xE0); seg(c, d, 0xE0, 0xE0, 0xE0); seg(d, a, 0xE0, 0xE0, 0xE0); };
    for (const Rect &r : outlines) { bool lift = false; for (const Pad &p : pads) if (p.x == r.cx && p.y == r.cy) lift = true; rect(r.cx, r.cy, r.hx, r.hy);
        if (lift) for (int k = 1; k < 6; k++) { float t = k / 3.f - 1.f; V a(r.cx + r.hx * (t < 0 ? -1.f : t * 2.f - 1.f), r.cy + r.hy * (t < 0 ? -1.f - t * 2.f : -1.f), 0), b(r.cx + r.hx * (t < 0 ? 1.f + t * 2.f : 1.f), r.cy + r.hy * (t < 0 ? 1.f : 1.f - t * 2.f), 0); seg(a, b, 0xE0, 0xE0, 0xE0); } }   /* the "line squares" */
    { V a(-WX, -WY, 0), b(WX, -WY, 0), c(WX, WY, 0), d(-WX, WY, 0); seg(a, b, 0xE0, 0xE0, 0xE0); seg(b, c, 0xE0, 0xE0, 0xE0); seg(c, d, 0xE0, 0xE0, 0xE0); seg(d, a, 0xE0, 0xE0, 0xE0); }   /* the force field, tape 156 s */
    auto drop = [&](float sx, float sy, float k, uint8_t r, uint8_t g, uint8_t b) { float px = sx, py = sy - 5.f * k; for (int n = 1; n <= 10; n++) { float a = XPI * 0.5f + n / 10.f * 2.f * XPI, rr = k * (3.f - 2.2f * sinf(a) * 0.5f) * 0.9f; float x = sx + cosf(a) * rr * 0.9f, y = sy - sinf(a) * rr * 1.25f; ln(f, px, py, x, y, r, g, b); px = x; py = y; } };
    for (size_t j = 0; j < wk.size(); j++) { if ((int)j == me || wk[j].s != WS_ALIVE) continue; float sx, sy; if (!pt(wk[j].p, sx, sy)) continue; const Walker &q = wk[j];   /* opponents: a coloured drop, no circle (spoken; red and white seen at tape 151, 126) */
        if (q.look == 1) drop(sx, sy, 1.f, 0xE8, 0xE8, 0xE0); else if (q.look == 3) drop(sx, sy, 1.f, 0x50, 0xD0, 0xB0); else drop(sx, sy, 1.f, 0xE8, 0x70, 0x60); }
    { float sx, sy; if (pt(w.p, sx, sy)) { drop(sx, sy, 1.1f, 0xD0, 0xF0, 0xE0); float px = sx + 11.f, py = sy; for (int n = 1; n <= 16; n++) { float a = n / 16.f * 2.f * XPI, x = sx + cosf(a) * 11.f, y = sy + sinf(a) * 12.f; ln(f, px, py, x, y, 0xD8, 0xE8, 0x70); px = x; py = y; } } }   /* you: the same drop in a yellow-green circle */
}
static void hud_play(VPIX_Frame &f, const Walker &w, V eye, const M &cam) {
    /* top left: shield icon and a row of ticks that shortens with damage */
    blit(f, G_SHIELD, 12, 12, 62.f, 15.f, 1.5f);
    int ticks = (int)ceilf((std::max)(w.shield, 0.f)); for (int k = 0; k < ticks; k++) { float x = 83.f + k * (77.f / shield_max); ln(f, x, 21, x, 30); }
    /* top right: skull and bones, one tick per kill to its left (no numbers on the tape) */
    blit(f, G_SKULL, 24, 16, 308.f, 16.f, 1.f);
    for (int k = 0; k < w.kills && k < 24; k++) { float x = 303.f - 4.f * (k % 12), y0 = 20.f + 13.f * (k / 12); ln(f, x, y0, x, y0 + 8); }
    if (w.s != WS_ALIVE) return;
    if (map_mode) { hud_map(f, w, eye, cam); return; }   /* tape: only shield and skull stay up over the map */
    /* right: rangefinder line with short feet to the left at both ends and two middle ticks, a column of short ticks to its left fills from the bottom up when a target is in view */
    ln(f, 324, 60, 324, 187); ln(f, 319, 60, 324, 60); ln(f, 319, 187, 324, 187); ln(f, 321, 102, 324, 102); ln(f, 321, 145, 324, 145);
    /* the column is read as distance: the tape's labels put Max Missile Range above Max Laser Range, so taller means farther, top of the line 1.5 x missile range. The frames do NOT agree well (a walker of the same size shows 3 ticks at 277 s, about 30 at 38 s, about half at Play Expo 107 s). Its true meaning stays open */
    float prox = 0; V fw = cam.r[1]; bool seen = false;
    for (size_t j = 0; j < wk.size(); j++) { if ((int)j == me || wk[j].s != WS_ALIVE) continue; V d = center(j) - eye; float dist = len(d);
        if (dist > 1.f && dot(fw, d * (1.f / dist)) > 0.94f) { float q = dist / (missile_range * 1.5f); if (!seen || q < prox) prox = q; seen = true; } }
    if (seen) for (int k = 0; k < (int)(clampf(prox, 0.05f, 1.f) * 34.f); k++) { float y = 184.f - k * 2.8f; ln(f, 312, y, 321, y); ln(f, 312, y + 1, 321, y + 1); }
    hud_radar(f, w, eye, cam);
    /* the sight: three arcs in a ring, each with a stem pointing outward */
    for (int k = 0; k < 3; k++) { float a0 = XPI * 0.5f + k * 2.f * XPI / 3.f, r = 15.f; float px = 176.f + cosf(a0 - 0.8f) * r, py = 144.f - sinf(a0 - 0.8f) * r;
        for (int s = 1; s <= 6; s++) { float a = a0 - 0.8f + 1.6f * s / 6.f, x = 176.f + cosf(a) * r, y = 144.f - sinf(a) * r; ln(f, px, py, x, y); px = x; py = y; }
        ln(f, 176.f + cosf(a0) * r, 144.f - sinf(a0) * r, 176.f + cosf(a0) * (r + 9.f), 144.f - sinf(a0) * (r + 9.f)); }
    if (w.lock_target >= 0 && w.lock_t > 0.f) {   /* a large box that shrinks onto the target as the lock builds */
        float sx, sy, dp; if (project(cam, eye, center(w.lock_target), sx, sy, dp)) {
            float tp = clampf(HR * 1.5f / dp * (176.f / tanf(35.f * XPI / 180.f)), 6.f, 120.f), u = clampf(w.lock_t / lock_shrink, 0.f, 1.f), hs = 150.f + (tp - 150.f) * u;   /* starts near the size of the view, down on the target in 0.7 s, holds to the 3 s */
            box2(f, sx - hs, sy - hs * 0.8f, sx + hs, sy + hs * 0.8f); } }
}

static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = 0x12; f.clear[1] = 0x0A; f.clear[2] = 0x1C;   /* black to dark purple sky */
    f.n_lights = 1; f.lights[0] = {0.65f, 0.35f, {-0.218f, -0.437f, 0.874f}};   /* INVENTED: flat and bright like Dactyl */
    static const char *fixed = getenv("EXOREX_CAM");
    const bool use_fixed = fixed && *fixed, play = gs == G_PLAY && !wk.empty() && !use_fixed;
    const float att = fmodf(st_t, 34.f);
    const bool story = gs == G_ATTRACT && att < 24.f && !use_fixed, pod_panel = !use_fixed && ((gs == G_ATTRACT && att >= 24.f) || gs == G_CREDIT);   /* the credit prompt: dark pod, red-framed slot above the console */
    const bool flying = !use_fixed && !wk.empty() && ((gs == G_BOOT && st_t < FLY_START) || (gs == G_PLAY && wk[me].s == WS_POD));
    const bool booting = !use_fixed && !wk.empty() && ((gs == G_BOOT && st_t >= FLY_START) || (gs == G_PLAY && wk[me].s == WS_BOOT));
    const float boot_time = gs == G_BOOT ? st_t - FLY_START : (wk.empty() ? 0.f : wk[me].pod_t);
    M cam; V eye; bool head_cam = false; const float a = (float)sim_t * 0.12f;
    if (use_fixed) { float v[6] = {0, -150000, 70000, 0, 0, 5000}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", v, v + 1, v + 2, v + 3, v + 4, v + 5); eye = V(v[0], v[1], v[2]); cam = look_at(eye, V(v[3], v[4], v[5])); }
    else if (flying) { const Walker &w = wk[me]; V from = gs == G_BOOT ? (deck_pos() + V(90000, -50000, 18000)) : w.pod_from; float u = gs == G_BOOT ? st_t / FLY_START : w.pod_t / w.pod_dur; eye = fly_pos(from, gs == G_BOOT ? 0.75f + 0.25f * u : u); V to = city_mid() - deck_pos(); to.z = 0; to = norm(to); float lk = clampf((gs == G_BOOT ? u : u * 4.f - 3.f), 0.f, 1.f); V tgt = deck_pos() + V(0, 0, 4000); V dir = norm(tgt - eye); if (dir.z > 0.3f) { dir.z = 0.3f; dir = norm(dir); } if (lk > 0.75f) dir = norm(dir + (to - dir) * ((lk - 0.75f) * 4.f)); cam = look_at(eye, eye + dir); }   /* the view keeps the deck ahead on the way (tape 279-288) */
    else if (booting) { V fwd; boot_view(wk[me], boot_time, eye, fwd); cam = look_at(eye, eye + fwd); }
    else if (gs == G_PLAY && !wk.empty()) {
        const Walker &w = wk[me]; eye = head_eye(in);
        static const bool lk0 = getenv("EXOREX_LOOK") != NULL;
        if (autoplay) cam = look_at(eye, w.s == WS_ALIVE ? (lk0 ? eye + V(-sinf(w.yaw), cosf(w.yaw), 0) : eye + w.aim) : w.pod_from);
        else if (in.head.valid) { M Y = yawed(w.yaw); for (int k = 0; k < 3; k++) cam.r[k] = wrot(Y, V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2])); head_cam = true; }
        else cam = look_at(eye, eye + wrot(yawed(w.yaw), V(0, 1, 0)));
    }
    else if (pod_panel) { eye = V(0, 0, 0); cam = look_at(eye, V(0, 1, 0)); }
    else { eye = V(-83000, -205000, 34000); cam = look_at(eye, eye + V(0, 1, -0.02f)); }   /* the fixed aerial view of the end screens: level, from over the south edge, hills across the middle (t44) */
    { static const char *lk = getenv("EXOREX_LOOK"); if (lk && *lk && (gs == G_PLAY || gs == G_BOOT || gs == G_END) && !use_fixed) { float ya = 0, pi = 0; sscanf(lk, "%f,%f", &ya, &pi); ya *= XPI / 180.f; pi *= XPI / 180.f;   /* dev: a fixed look round the cockpit for pictures */
        V fw = cam.r[1], ri = cam.r[0], up = cam.r[2]; V f2 = fw * cosf(ya) + ri * sinf(ya), r2 = ri * cosf(ya) - fw * sinf(ya); V f3 = f2 * cosf(pi) + up * sinf(pi), u3 = up * cosf(pi) - f2 * sinf(pi); cam.r[0] = r2; cam.r[1] = f3; cam.r[2] = u3; } }

    if (story || pod_panel) { f.clear[0] = f.clear[1] = f.clear[2] = 0; }
    if (pod_panel) { M I; VPIX_Instance &q = dput(f, X_COCKL, I, V(0, 0, 0)); q.on_top = 2; q.bright = 1; }
    if (!story && !pod_panel) {
        M I; dput(f, X_SKY, I, V(eye.x, eye.y, 0)).bright = 1; dput(f, X_CITY, I, V(0, 0, 0)); dput(f, X_FIELD, I, V(0, 0, 0)).bright = 1;
        for (const Pad &p : pads) dput(f, X_PAD, I, V(p.x, p.y, 40));
        if (flying) { V d = deck_pos(); dput(f, X_DECK, I, d); V to = city_mid() - d; if (len(eye - d) > 15000.f) { Walker dw; dw.look = 2; dw.yaw = atan2f(-to.x, to.y); dw.p = d; put_walker(f, dw); } }   /* the deck with your cyan walker waiting on it */
    }
    if ((gs == G_PLAY || use_fixed) && !story) {
        for (size_t i = 0; i < wk.size(); i++) { const Walker &w = wk[i];
            if (w.s == WS_ALIVE) { if (!(play && (int)i == me)) put_walker(f, w); }
            }
        for (const Beam &b : beams) { float L = len(b.b - b.a), u = clampf(b.t / b.dur, 0.f, 1.f); V dir = (b.b - b.a) * (1.f / (L > 1.f ? L : 1.f)), at = b.a + dir * (L * u); VPIX_Instance &q = dput(f, X_BEAM, look_at(at, at + dir), at); q.S[0] = q.S[2] = 260.f; q.S[1] = 3600.f; q.bright = 1; }
        for (const Missile &m : missiles) dput(f, X_MISSILE, look_at(m.p, m.p + m.dir), m.p);
        { M I; for (const Spark &k : sparks) { if (len(k.p - eye) > 7000.f) dput(f, k.id, I, k.p).bright = 1; } }
        for (const Shard &s : shards) { if (len(s.p - eye) > 7000.f) dput(f, X_SHARD, s.R, s.p); }
        for (const Flash &fl : flashes) { if (len(fl.p - eye) < 12000.f) continue; M I; VPIX_Instance &q = dput(f, X_FLASH, I, fl.p); float sc = 1.f + fl.t * 14.f; q.S[0] = q.S[1] = q.S[2] = sc; q.bright = 1; }
    }
    if (booting) { V e2, fwd; boot_view(wk[me], boot_time, e2, fwd); M Y = yawed(atan2f(-fwd.x, fwd.y)); dput(f, X_COCK, Y, e2).on_top = 2; VPIX_Instance &q = dput(f, X_COCKL, Y, e2); q.on_top = 2; q.bright = 1; dput(f, X_HULL, Y, e2).on_top = 2; }
    if (play && wk[me].s == WS_ALIVE) {   /* the cockpit stays with the seat: cyan dash and the side panels; looking down shows all of it */
        const Walker &w = wk[me]; static const bool lk1 = getenv("EXOREX_LOOK") != NULL; M Y = yawed(autoplay && !lk1 ? atan2f(-w.aim.x, w.aim.y) : w.yaw); V eb(w.p.x, w.p.y, w.p.z + eye_h);   /* self-play looks along its aim */
        dput(f, X_COCK, Y, eb).on_top = 2; VPIX_Instance &q = dput(f, X_COCKL, Y, eb); q.on_top = 2; q.bright = 1;
        dput(f, X_HULL, Y, eb).on_top = 2; put_own_legs(f, w);
        if (w.self_t > 0.f && fmodf((float)sim_t, 0.2f) < 0.1f) { M I; V at = eye + cam.r[1] * 9000.f + cam.r[0] * (3600.f * w.self_side) - cam.r[2] * 300.f; VPIX_Instance &q2 = dput(f, w.shield < 4.f ? X_BALL : X_BALLR, I, at); q2.on_top = 2; q2.bright = 1; q2.glass = 120; }   /* you being hit: a big translucent ten-sided ball beside the view, strobing at 5 a second (tape 275-278; Play Expo 104, 107). Pale at the very end. Which side it takes is ours */
    }
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    f.in_head = head_cam; if (head_cam) f.head_fed = in.head;

    const float CX = 176.f;
    auto red_slot = [&]() { rectb(f, G_SOLID_R, 4, 4, 70, 72, 208, 7); rectb(f, G_SOLID_R, 4, 4, 70, 172, 208, 13); rectb(f, G_SOLID_R, 4, 4, 70, 72, 8, 110); rectb(f, G_SOLID_R, 4, 4, 270, 72, 8, 110); };   /* ex_1060: a black slot framed in burnt red over the console */
    if (gs == G_ATTRACT && !use_fixed) {
        if (att < 13.f) {   /* the attract story: the logo strip over centred lines (text from the tape, KKkZ t368) */
            rectb(f, G_LOGO, 176, 34, 0, 0, 352, 68);
            static const char *L[] = {"JUSTICE IS HARSH", "ON THE NEW FRONTIER.", "CONVICTED CRIMINALS", "ARE SHIPPED OUT TO THE", "COMBAT ZONE TO", "PARTICIPATE IN LETHAL", "TRIAL BY COMBAT"};
            int left = (int)(att * 14.f); float y = 84.f;
            for (int i = 0; i < 7 && left > 0; i++) { gtext(f, L[i], CX, y, 1.f, true, 9999); left -= (int)strlen(L[i]); y += 24.f; }
        } else if (att < 24.f) {   /* the title panel. Left screen: a craft; right: small lights, then a waveform (KKkZ 6:21-7:30) */
            rectb(f, G_PANEL, 176, 144, 0, 0, 352, 288);
            float u = att - 13.f; if (u > 6.5f) { float px = 196.f, py = 188.f; for (int k = 1; k <= 26; k++) { float x = 196.f + k * 3.6f, y = 188.f + sinf(k * 1.1f + (float)sim_t * 7.f) * 13.f * sinf(k * 0.12f); ln(f, px, py, x, y); px = x; py = y; } }
            else for (int k = 0; k < 8; k++) { float x = 215.f + 44.f * sinf(k * 2.1f + u * 0.5f) * 0.6f, y = 188.f + 34.f * cosf(k * 1.3f + u * 0.3f); ln(f, x, y, x + 2, y + 3); }
            ln(f, 96, 170, 88, 210); ln(f, 96, 170, 104, 210); ln(f, 88, 210, 104, 210);
        } else { red_slot();
            if (fmodf((float)sim_t, 1.f) < 0.6f) d1k::text(f, "INSERT CREDIT", CX, 154.f, 1.f, 0);
            if (v1knet::on && !v1knet::master) d1k::text(f, !v1knet::master_heard() ? "LOOKING FOR PLAYER 1" : "PLAYER 1 STARTS THE GAME", CX, 200.f, 0.8f);
            if (v1knet::on && v1knet::master && v1knet::live_pods() > 0) { char b[32]; snprintf(b, sizeof(b), "LINKED %d PLAYERS", v1knet::live_pods() + 1); d1k::text(f, b, CX, 200.f, 0.8f); } }
    } else if (gs == G_CREDIT) { red_slot(); d1k::text(f, "CREDIT ACCEPTED", CX, 154.f, 1.f, 0); }
    else if (booting) {   /* LOAD CORE SYSTEM overlay timed off the tape (11-17.5 s): matrix frame first and empty, then squares, bar chart from about +2 s, text lines one by one, the gas table line by line on the right */
        const float bt = boot_time; box2(f, 67, 26, 134, 92);
        for (int k = 0; k < 7; k++) { ln(f, 64, 32 + k * 8.f, 67, 32 + k * 8.f); ln(f, 72.f + k * 8.f, 88, 72.f + k * 8.f, 90); }
        ln(f, 119, 32, 119, 84); ln(f, 119, 84, 130, 84); ln(f, 126, 40, 126, 80); ln(f, 127, 40, 127, 80); for (int k = 0; k < 6; k++) ln(f, 119, 35 + k * 9.f, 123, 35 + k * 9.f);
        int tk = (int)(bt * 3.f); std::mt19937 gr(tk * 77 + 3);
        if (bt > 0.5f) for (int a2 = 0; a2 < 4; a2++) for (int b = 0; b < 4; b++) { unsigned q = gr() % 3; if (q == 0) continue; rectb(f, G_SOLID_G, 4, 4, 73.f + a2 * 11.f, 33.f + b * 13.f, 8, 9, q == 1 ? 0.45f : 1.f); }
        if (bt > 2.f) { box2(f, 260, 26, 327, 92); for (int k = 0; k < 8; k++) { float hh = 26.f + 26.f * fabsf(sinf(k * 1.9f + 0.6f)), h2 = 8.f + 30.f * fabsf(sinf(k * 1.3f + bt * 1.1f)); rectb(f, G_SOLID_G, 4, 4, 265.f + k * 6.f, 82.f - hh, 3, hh, 0.4f); rectb(f, G_SOLID_G, 4, 4, 265.f + k * 6.f, 82.f - h2, 3, h2); ln(f, 265.f + k * 6.f, 86, 268.f + k * 6.f, 86); }
            ln(f, 318, 30, 318, 82); for (int k = 0; k < 6; k++) ln(f, 315, 34 + k * 9.f, 321, 34 + k * 9.f); }
        if (bt > 0.2f) gtext(f, "LOAD CORE SYSTEM", 70.f, 172.f, 0.62f, false);
        if (bt > 1.5f) gtext(f, "SYSTEM LOADING COMPLETE", 70.f, 183.f, 0.62f, false);
        if (bt > 3.f) gtext(f, "ATMOSPHERE SAMPLE", 70.f, 194.f, 0.62f, false);   /* a fourth line shows at about +5.5 s; it cannot be read on any copy */
        { static const char *G[6] = {"OXYGEN", "NITROGEN", "HYDROGEN", "TAZZIUM", "ARGON", "PHOTOGEN"}; static const char *N[6] = {"45.00", "46.02", "32.15", "01.15", "00.06", "00.03"};   /* tape 16.4, 355.5 s: first four words fairly clear, numbers blurred; the last two are best guesses */
          for (int k = 0; k < 6; k++) if (bt > 3.f + k * 0.45f) { float y = 156.f + k * 11.f; gtext(f, G[k], 244.f, y, 0.55f, false); gtext(f, N[k], 304.f, y, 0.55f, false); } }
    } else if (play && !flying) {
        hud_play(f, wk[me], eye, cam);
    } else if (gs == G_END) {
        if (won) { if (st_t < 5.f) { rectb(f, G_SOLID_K, 4, 4, 106, 131, 180, 23); box2(f, 106, 131, 286, 154); rectb(f, G_FREEDOM, 7 * 36, 28, 112, 135, 168, 15); } }   /* FREEDOM in a thin green box on black, across the hills (about 3 s on the tape) */
        else if (st_t < 7.f) rectb(f, G_BIGSKULL, 128, 104, 122, 84, 128, 104);   /* the big green skull and crossbones (about 6 s) */
    }
    d1k::look_test(f);
    d1k::hs::draw(f, "exorex", gs == G_ATTRACT, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at 20 per second ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 1; static unsigned long frames = 0;
/* true = this tick was a linked slave's, inside the master's game */
static bool net_tick(const VPIX_Input &in) {
    v1knet::pump();
    if (v1knet::master) { net_master(); net_log(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n >= sizeof(NetHd) && v1knet::my_seat >= 0) net_apply(d, n, v1knet::round_id);
        else if (slave_game) net_leave("the master's round is over");
    }
    if (slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
    net_log();
    if (!slave_game) { v1knet::send_input(NULL, 0); return false; }
    NetIn o; memset(&o, 0, sizeof(o)); read_sticks(in, o.fw, o.tx);
    if (autoplay) { o.fw = 1.f; o.tx = 0.6f * sinf((float)sim_t * 0.5f); o.flags |= 1; o.aim[1] = 1.f; }
    else { if (in.head.valid) { o.aim[0] = in.head.R[1][0]; o.aim[1] = in.head.R[1][1]; o.aim[2] = in.head.R[1][2]; } else o.aim[1] = 1.f; if (in.buttons & VPIX_BTN_TRIGGER) o.flags |= 1; if (map_mode) o.flags |= 2; }
    v1knet::send_input(&o, sizeof(o));
    const float dt = 1.f / 120.f; int k = 0;
    while (acc >= dt && k < 24) { net_slave_step(dt); step_effects(dt); acc -= dt; sim_t += dt; st_t += dt; k++; }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    return true;
}
static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    static bool trig_was = false, map_was = false; static int coins_was = 0;
    bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f || in.start, sel = trig && !trig_was; trig_was = trig;
    if (in.coins > coins_was) sel = true;
    coins_was = in.coins;
    if (autoplay && gs == G_ATTRACT && st_t > 2.f && (!v1knet::on || !v1knet::master || v1knet::live_pods() > 0 || st_t > 25.f)) sel = true;   /* linked self-play: pod 1 waits for the other pod */
    { static const bool hold = getenv("EXOREX_ATTRACT") != NULL; if (hold) sel = false; } if (d1k::hs::run("exorex", gs == G_END && st_t > 2.f, wk.empty() ? 0 : wk[me].kills * 100, in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    bool mt = in.top || (in.buttons & VPIX_BTN_3) != 0 || (GetAsyncKeyState('M') & 0x8000) != 0;   /* the top button; M on the desktop */
    if (mt && !map_was && gs == G_PLAY && !autoplay && !wk.empty() && wk[me].s == WS_ALIVE) map_mode = !map_mode;
    { static const bool fm = getenv("EXOREX_FORCEMAP") != NULL; if (fm && gs == G_PLAY && !wk.empty() && wk[me].s == WS_ALIVE) map_mode = true; }
    map_was = mt;
    if (!v1knet::can_start()) sel = false;   /* a linked slave: pod 1 starts the game */
    if (v1knet::on && net_tick(in)) { PIC_AddEvent(tick, 2.0); return; }
    if (gs == G_ATTRACT && sel) { gs = G_CREDIT; st_t = 0; xsfx(0, SND_CREDIT, 100); }
    else if (gs == G_CREDIT && st_t > 1.5f) { start_round(); gs = G_BOOT; st_t = 0; }
    else if (gs == G_BOOT && st_t > FLY_START + boot_len) { gs = G_PLAY; st_t = 0; xsfx(0, SND_GO, 110); }
    else if (gs == G_END && st_t > 9.f) { gs = G_ATTRACT; st_t = 0; v1knet::end_round(); wk.clear(); missiles.clear(); beams.clear(); sparks.clear(); shards.clear(); flashes.clear(); }
    int n = 0;
    while (acc >= dt && n < 24) {
        if (gs == G_PLAY && !wk.empty()) { step_play(in, dt); step_missiles(dt); }
        else if (gs == G_END && !wk.empty()) { for (size_t i = 0; i < wk.size(); i++) wk[i].moved = 0; }
        step_effects(dt);
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    if (sim_t >= next_log && !wk.empty()) { next_log = sim_t + 2.0; const Walker &q = wk[me];
        dl("t=%.0f gs %d me (%.0f %.0f %.0f) st %d shield %.0f kills %d lock %.1f | cpu kills %d %d %d | missiles %zu map %d left %.0f", sim_t, (int)gs, q.p.x, q.p.y, q.p.z, (int)q.s, q.shield, q.kills, q.lock_t,
           wk.size() > 1 ? wk[1].kills : 0, wk.size() > 2 ? wk[2].kills : 0, wk.size() > 3 ? wk[3].kills : 0, missiles.size(), (int)map_mode, time_left); }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".exorex").c_str(), "w");
    walk_speed = envf("EXOREX_WALK", walk_speed); turn_slow = envf("EXOREX_TURN", turn_slow); turn_fast = turn_slow * 1.92f; eye_h = envf("EXOREX_EYE", eye_h); grav = envf("EXOREX_GRAV", grav);
    shield_max = envf("EXOREX_SHIELD", shield_max); laser_gap = envf("EXOREX_LASER_GAP", laser_gap); laser_dmg = envf("EXOREX_LASER_DMG", laser_dmg); laser_range = envf("EXOREX_LASER_RANGE", laser_range);
    missile_dmg = envf("EXOREX_MISSILE_DMG", missile_dmg); missile_range = envf("EXOREX_MISSILE_RANGE", missile_range); missile_speed = envf("EXOREX_MISSILE_SPEED", missile_speed);
    lock_time = envf("EXOREX_LOCK", lock_time); regen = envf("EXOREX_REGEN", regen); respawn_delay = envf("EXOREX_RESPAWN", respawn_delay); lift_time = envf("EXOREX_LIFT_TIME", lift_time);
    game_len = envf("EXOREX_TIME", game_len); game_fps = envf("EXOREX_FPS", game_fps); skill = envf("EXOREX_SKILL", skill); boot_len = envf("EXOREX_BOOT", boot_len);
    n_cpu = (int)clampf(envf("EXOREX_CPU", (float)n_cpu), 0.f, 3.f); autoplay = envf("EXOREX_AUTO", 0) != 0;
    d1k::make_hud(); make_bitmaps(); build_meshes(); make_sounds();
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    v1knet::init("EXRX", false, dl);   /* start-together match (join in progress is for Flying Aces and VTOL only) */
    PIC_AddEvent(tick, 2.0);
    dl("Exorex rebuild started: %zu outlines, %zu solids, %zu lifts", outlines.size(), solids.size(), pads.size());
}
}   /* namespace exorex */
