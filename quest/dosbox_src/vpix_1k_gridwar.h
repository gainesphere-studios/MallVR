/* Grid Warriors (Virtuality 1000CS stand-up, 1993) - native rebuild from footage. Included after vpix_dactyl1k.h; dev only, not in a release.
   It is NOT Grid Busters: own title card, own world, own rules; see Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\grid.md. Evidence: Bad Influence FQarUJ1MDc4 (same broadcast as Y7mtN7Sxk-E t36-84, which names it "Grid Warriors",
   "should be out in the summer"), Octavius King 2YrPW5cU06A t820-848, operator tape YV1jk-t2XIk, Generation 4 #60 p141 (a printed picture of the cyborg, captioned Gridbusters by mistake), 08_footage_standup_games.md section 4.
   FROM FOOTAGE: two players on a spaceship, cyborg with an arm gun against an alien with claws, four minutes; lavender corridors, white strip lights, blue floor strip, hexagonal bulkheads,
   orange and pale triangles over doors, orange floor dashes, a checkered hazard disc on the floor, tan doors with pale lines and a display box; a clock in that box (3:26 at Octavius t840, in blue digits with blue ears; that it is the round clock is a good GUESS: 4:00 less 34 s);
   grey gun at bottom centre with an eight-point yellow star flash; no HUD in play; the title card (green polygon skull in a hexagonal web, GRID WARRIORS); the hunched thin-limbed alien on tan sand under a black sky (t820-826).
   REFINED 2026-11-02, frame by frame against Bad Influence t38-68 and Octavius t823-845. MEASURED (eye 1700, fov 70): corridor section from t47, inner ring
   4180 wide and 3460 tall with its widest point at eye level, ring spacing about 7000, side rail at 1100, gun tip 1590 ahead and 270 below the eye, walking about one ring in 3 s.
   READ FROM FRAMES: three strip lights in boxes (ceiling, two upper diagonals), rail over a blue panel, F-shaped door seam, triangle marks on doors, the dark hall with thin lattice towers and hazard stripes on an eight-sided plinth,
   the crawling thing with white claws (Octavius "oh look a thing, kill it"; Violet Berlin on air: "dealt with a few spiders"), the faceted blue cyborg with orange shoulders, orange chest triangle and a dark arm gun (Generation 4 p141), the attract order.
   OUR DESIGN, NOT FROM FOOTAGE (all marked INVENTED): the map, the self-destruct console and how it is disarmed, hit points, the win rules, all sounds, what the crawlers do, the title's YOU ARE / PRESS TRIGGER lines.
   Units: mm, seconds. World X right, Y forward, Z up. The camera is the player's head (in_head), so VR late correction applies.
   Env: GRIDWAR_SIDE (0 cyborg, 1 alien), GRIDWAR_AUTO, GRIDWAR_CAM, GRIDWAR_FPS, GRIDWAR_TIME, GRIDWAR_WALK, GRIDWAR_ALIEN_WALK, GRIDWAR_GUN_GAP,
   GRIDWAR_CLAW_GAP, GRIDWAR_REACH, GRIDWAR_DISARM, GRIDWAR_CYBORG_HP, GRIDWAR_ALIEN_HP, GRIDWAR_ALIEN_RESPAWN, GRIDWAR_SKILL, GRIDWAR_GUNHEAD.
   Dev: GRIDWAR_ATTRACT=<secs into the loop> holds the attract loop; GRIDWAR_GUNVIEW=1 draws the gun with GRIDWAR_CAM. */
#include <functional>
#include "vpix_1k_net.h"         /* link play: see the "link play" block before the loop */
namespace gridwar {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at;
using d1k::Mesh; using d1k::dput; using d1k::blit; using d1k::text; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound;
using d1k::yawed; using d1k::pitched;

/* ---- tuning ---- */
static const float CELL = 6000.f;                          /* corridor shell width and one ring per cell (rings measured about 7000 apart) */
static const float CEIL_Z = 4000.f, HALL_Z = 9000.f;
static float walk_speed = 2200.f;                          /* about one ring in 3 s on Bad Influence t41-47 */
static float alien_speed = 2000.f;                         /* INVENTED: a little slower than the cyborg */
static float game_len = 240.f;                             /* 4:00 (operator tape) */
static float game_fps = 20.f;                              /* measured about 10 on the tapes, 20 at best; env override */
static float gun_gap = 0.5f;                               /* INVENTED */
static float claw_gap = 0.9f;                              /* INVENTED */
static float claw_reach = 1900.f;                          /* INVENTED */
static float disarm_time = 5.f;                            /* INVENTED: seconds with the trigger held at the console */
static int cyborg_hp = 3, alien_hp = 5;                    /* INVENTED */
static float alien_respawn = 6.f;                          /* INVENTED */
static float cpu_skill = 1.f;                              /* INVENTED: CPU aim error multiplier, larger = worse */
static bool gun_head = false;                              /* always draw the gun at bottom centre of the view, ignore the hand */
static int side = 0;                                       /* 0: you are the cyborg, 1: you are the alien */
static bool autoplay = false;

/* ---- ids ---- */
enum { M_SHIP = 0x200001, M_CONS_ARM, M_CONS_OFF, M_CONS_HALF, M_CONS_SAFE, M_CY_BODY, M_CY_LEG, M_AL_BODY, M_AL_LEG, M_AL_ARM, M_GUN, M_STAR,
       M_SPARK, M_SKULL, M_WEB, M_THING, M_LETTERS, M_GREEN, M_SAND, M_CDIG, M_CDIG_END = M_CDIG + 10, M_CCOLON };
enum { SND_FIRE = 0x7E000001, SND_CLAW, SND_HURT, SND_ROAR, SND_BEEP, SND_DISARM, SND_DONE, SND_BOOM, SND_GO };

/* colours: lavender corridors from the Bad Influence frames (Octavius' direct capture is browner and darker, same geometry) */
static const uint32_t C_WALL = 0x8C86B4, C_DIAG = 0x9A92BC, C_CEIL = 0x6E6894, C_FLOOR = 0x9490C4, C_BLUE = 0x3A3EA8, C_PANEL = 0x4A4CB0, C_RAIL = 0xE6E6FA, C_RAIL2 = 0xC6C6EC, C_KICK = 0xA8A8D8,
                      C_LIGHT = 0xFFFFFF, C_LIGHTBOX = 0xB4B4E0, C_ORANGE = 0xE8801C, C_RING = 0xBCBCEA, C_RING2 = 0x9C9CD4, C_RING3 = 0xD0D0F4, C_TAN = 0x72603F, C_TANL = 0xA89A72, C_LEAF = 0x9C98C8, C_LEAF2 = 0x7C78A8,
                      C_HALLW = 0x1A2228, C_HALLF = 0x1E262C, C_HALLB = 0x4A90D0, C_POST = 0xA4A4AC, C_HZY = 0xE8D030, C_HZK = 0x141414;

/* ---- the map: OUR DESIGN. C cyborg start, A alien start, D self-destruct console; row 0 is the top and +y ---- */
static const int GW = 13, GH = 9;
static const char *MAP[GH] = {
    "#############",
    "#..........A#",
    "#.####.####.#",
    "#.#...D...#.#",
    "#.#.#####.#.#",
    "#...#...#...#",
    "#.###...###.#",
    "#C..........#",
    "#############"};
static V console_pos, start_c, start_a;
static float cx(int c) { return c * CELL; }
static float cy(int r) { return (GH - 1 - r) * CELL; }
static int cellc(float x) { return (int)floorf((x + CELL * 0.5f) / CELL); }
static int cellr(float y) { return GH - 1 - (int)floorf((y + CELL * 0.5f) / CELL); }
static bool solid_c(int c, int r) { return c < 0 || r < 0 || c >= GW || r >= GH || MAP[r][c] == '#'; }
static bool solid_xy(float x, float y) { return solid_c(cellc(x), cellr(y)); }
static bool hall_c(int c, int r) { return c >= 5 && c <= 7 && (r == 5 || r == 6); }   /* the dark hall of Octavius t826-834 */
static V tower_pos[2];

/* ---- mesh helpers: every face is given the side it must be seen from, so no winding has to be worked out by hand ---- */
static void quadn(Mesh &m, V a, V b, V c, V d, V want, uint32_t rgb) {
    if (dot(cross(b - a, c - a), want) >= 0) m.poly({a, b, c, d}, rgb); else m.poly({d, c, b, a}, rgb);
}
static void trin(Mesh &m, V a, V b, V c, V want, uint32_t rgb) {
    if (dot(cross(b - a, c - a), want) >= 0) m.poly({a, b, c}, rgb); else m.poly({c, b, a}, rgb);
}
static void quad2(Mesh &m, V a, V b, V c, V d, uint32_t rgb) { V n = cross(b - a, c - a); quadn(m, a, b, c, d, n, rgb); quadn(m, a, b, c, d, V(0, 0, 0) - n, rgb); }
static void tri2(Mesh &m, V a, V b, V c, uint32_t rgb) { V n = cross(b - a, c - a); trin(m, a, b, c, n, rgb); trin(m, a, b, c, V(0, 0, 0) - n, rgb); }
static void pyr(Mesh &m, V base, float r, float h, V dir, uint32_t rgb) {   /* a four-sided spike from base, tip at base + dir * h */
    V t0 = cross(dir, V(0, 0, 1)); if (len(t0) < 0.1f) t0 = V(1, 0, 0); t0 = norm(t0); V t1 = norm(cross(dir, t0)), tip = base + dir * h;
    V c4[4] = {base + t0 * r, base + t1 * r, base - t0 * r, base - t1 * r};
    for (int i = 0; i < 4; i++) { V a = c4[i], b = c4[(i + 1) & 3]; trin(m, a, b, tip, (a + b) * 0.5f - base + dir * 0.2f, rgb); }
}
/* a hexagonal bulkhead rib across a corridor: a ring of six quads, both faces. The hexagon is 3000 wide and 3000 tall around its centre */
static void hexring(Mesh &m, V base, V axis, float half_t, float k_out, float k_in, uint32_t rgb) {
    const V t(-axis.y, axis.x, 0); const float pu[6] = {-1500, -750, 750, 1500, 750, -750}, pz[6] = {1500, 3000, 3000, 1500, 0, 0};
    for (int s = -1; s <= 1; s += 2) {
        V off = axis * (half_t * (float)s);
        for (int i = 0; i < 6; i++) { int j = (i + 1) % 6;
            V o0 = base + t * (pu[i] * k_out) + V(0, 0, 1500 + (pz[i] - 1500) * k_out) + off, o1 = base + t * (pu[j] * k_out) + V(0, 0, 1500 + (pz[j] - 1500) * k_out) + off;
            V i0 = base + t * (pu[i] * k_in) + V(0, 0, 1500 + (pz[i] - 1500) * k_in) + off, i1 = base + t * (pu[j] * k_in) + V(0, 0, 1500 + (pz[j] - 1500) * k_in) + off;
            quadn(m, o0, o1, i1, i0, axis * (float)s, rgb); }
    }
    for (int i = 0; i < 6; i++) { int j = (i + 1) % 6;   /* the outer and inner edges, so the ring is solid */
        for (int e = 0; e < 2; e++) { const float k = e ? k_in : k_out, sgn = e ? -1.f : 1.f; V a = base + t * (pu[i] * k) + V(0, 0, 1500 + (pz[i] - 1500) * k), b = base + t * (pu[j] * k) + V(0, 0, 1500 + (pz[j] - 1500) * k), off = axis * half_t;
            V mid = (a + b) * 0.5f - (base + V(0, 0, 1500)); quadn(m, a - off, b - off, b + off, a + off, mid * sgn, rgb); } }
}
/* the bulkhead rib, measured from t47: open at the floor, the sides lean in to the floor and to the flat top, widest at eye level. Half-section from the floor up */
static const float RU_I[4] = {1300, 1880, 2090, 1300}, RZ_I[4] = {0, 720, 1750, 3460}, RU_O[4] = {1900, 2400, 2640, 1550}, RZ_O[4] = {0, 720, 1750, 3985};
static void archring(Mesh &m, V base, V axis, float half_t) {
    const V t(-axis.y, axis.x, 0); V ci[8], co[8];
    for (int i = 0; i < 4; i++) { ci[i] = base + t * (-RU_I[i]) + V(0, 0, RZ_I[i]); co[i] = base + t * (-RU_O[i]) + V(0, 0, RZ_O[i]); ci[7 - i] = base + t * RU_I[i] + V(0, 0, RZ_I[i]); co[7 - i] = base + t * RU_O[i] + V(0, 0, RZ_O[i]); }
    const V mid = base + V(0, 0, 1750);
    for (int i = 0; i < 7; i++) { V off = axis * half_t; uint32_t c = (i == 0 || i == 6) ? C_RING2 : (i == 2 || i == 4) ? C_RING3 : C_RING;
        for (int s = -1; s <= 1; s += 2) quadn(m, co[i] + off * (float)s, co[i + 1] + off * (float)s, ci[i + 1] + off * (float)s, ci[i] + off * (float)s, axis * (float)s, c);
        quadn(m, co[i] - off, co[i + 1] - off, co[i + 1] + off, co[i] + off, (co[i] + co[i + 1]) * 0.5f - mid, C_RING2);
        quadn(m, ci[i] - off, ci[i + 1] - off, ci[i + 1] + off, ci[i] + off, mid - (ci[i] + ci[i + 1]) * 0.5f, C_RING2); }
}
struct DoorClock { V at, t, n; };
static std::vector<DoorClock> door_clocks;
/* a closed door on a wall: octagon leaf, F-shaped seam, notches, a triangle mark, tan with a yellow line or lavender (both seen). All in the plane n, u along the wall */
static void mk_door(Mesh &m, V o, V n, bool tan, bool box_on_top, float wall_d) {
    const V t(-n.y, n.x, 0); V P = o - n * (wall_d - 12.f);
    auto pt = [&](float u, float z, float lift = 0.f) { return P + t * u + V(0, 0, z) + n * lift; };
    const uint32_t lc = tan ? C_TAN : C_LEAF, ln = tan ? C_TANL : C_LEAF2;
    float uu[8], zz[8]; for (int i = 0; i < 4; i++) { uu[i] = -RU_I[i]; zz[i] = RZ_I[i]; uu[7 - i] = RU_I[i]; zz[7 - i] = RZ_I[i]; }
    quadn(m, pt(uu[0], zz[0]), pt(uu[1], zz[1]), pt(uu[6], zz[6]), pt(uu[7], zz[7]), n, lc); quadn(m, pt(uu[1], zz[1]), pt(uu[2], zz[2]), pt(uu[5], zz[5]), pt(uu[6], zz[6]), n, lc); quadn(m, pt(uu[2], zz[2]), pt(uu[3], zz[3]), pt(uu[4], zz[4]), pt(uu[5], zz[5]), n, lc);
    auto strip = [&](float u0, float z0, float u1, float z1, float w, uint32_t c) { float du = u1 - u0, dz = z1 - z0, l = sqrtf(du * du + dz * dz); float a = -dz / l * w, b = du / l * w; quadn(m, pt(u0 - a, z0 - b, 8), pt(u1 - a, z1 - b, 8), pt(u1 + a, z1 + b, 8), pt(u0 + a, z0 + b, 8), n, c); };
    const float k = 0.86f; for (int i = 0; i < 7; i++) strip(uu[i] * k, zz[i] * k + 120.f, uu[i + 1] * k, zz[i + 1] * k + 120.f, 34.f, ln);
    strip(350, 150, 350, 2700, 24.f, ln);                                        /* the seam, then the F: a top bar, a stem and a short bar */
    /* the F is a soft shaded channel on a lavender door (t49-58), only pale lines on a tan one (Octavius t840-845) */
    { const float fw = tan ? 18.f : 62.f; const uint32_t fc = tan ? C_TANL : 0x625E90; strip(-520, 2500, 350, 2500, fw, fc); strip(-500, 2500, -500, 1500, fw, fc); strip(-500, 1950, 200, 1950, fw, fc); }
    for (int k2 = -1; k2 <= 1; k2 += 2) for (int j = 0; j < 2; j++) { float u = tan ? 350.f : k2 * 1560.f, z = j ? 1000.f : 1820.f, q = tan ? -k2 * 300.f : k2 * 150.f, hz = tan ? 210.f : 110.f; trin(m, pt(u, z - hz, 10), pt(u, z + hz, 10), pt(u - q, z, 10), n, tan ? 0x3A2E20 : 0x201C28); }
    { float z = 2860.f; trin(m, pt(-260, z, 10), pt(260, z, 10), pt(0, z + 380, 10), n, C_ORANGE); quadn(m, pt(-250, z - 110, 10), pt(250, z - 110, 10), pt(250, z - 50, 10), pt(-250, z - 50, 10), n, 0xF0D090);
      if (!box_on_top) for (int i = 0; i < 2; i++) { float zz = 3320.f + i * 330.f, w = 200.f - i * 50.f; trin(m, pt(-w, zz, 10), pt(w, zz, 10), pt(0, zz + w * 1.3f, 10), n, 0xE8E8F4); } }   /* pale triangles stacked over it, shrinking upward (t49.7) */
    if (box_on_top) {   /* the display box over a tan door: dark, blue ears each side, the clock in it (Octavius t840) */
        const float Bw = 800.f; m.box(P + t * (-Bw) + V(0, 0, 3560) + n * 10.f - V(n.x < 0 ? 260.f : 0.f, n.y < 0 ? 260.f : 0.f, 0) - V(fabsf(t.x) * 0, 0, 0), P + t * Bw + V(0, 0, 4050) + n * 10.f + V(n.x > 0 ? 260.f : 0.f, n.y > 0 ? 260.f : 0.f, 0), 0x34383E);
        for (int e = -1; e <= 1; e += 2) quadn(m, pt(e * Bw, 3640, 40), pt(e * (Bw + 190.f), 3680, 40), pt(e * (Bw + 190.f), 3930, 40), pt(e * Bw, 3970, 40), n, 0x4890D0);
        door_clocks.push_back({pt(0, 3805, 285), t, n}); }
}
/* floor marks in front of a door: slanted dashes across the floor and a ladder in the middle (t47, t51) */
static void floor_marks(Mesh &m, V o, V n) {
    const V t(-n.y, n.x, 0); V w = n * -1.f;
    auto pt = [&](float u, float d) { return o + t * u + w * d + V(0, 0, 22); };
    for (int sd = -1; sd <= 1; sd += 2) for (int i = 0; i < 4; i++) { float u = sd * (1050.f + i * 330.f), sl = sd * 170.f;
        quadn(m, pt(u - 75, 900), pt(u + 75, 900), pt(u + 75 + sl, 1500), pt(u - 75 + sl, 1500), V(0, 0, 1), C_ORANGE);
        quadn(m, pt(u - 25, 950), pt(u + 25, 950), pt(u + 25 + sl, 1450), pt(u - 25 + sl, 1450), V(0, 0, 1), 0xF4E0B0); }
    for (int i = 0; i < 4; i++) { float d = 1950.f + i * 190.f; quadn(m, pt(-430, d), pt(430, d), pt(430, d + 60.f), pt(-430, d + 60.f), V(0, 0, 1), 0xF4C080); }   /* the pale bars nearest the door */
    { const V c0 = pt(0, 1250.f); const float Rd = 520.f; for (int i = 0; i < 8; i++) { float a = 6.2831853f * (i + 0.5f) / 8, b = 6.2831853f * (i + 1.5f) / 8; trin(m, c0, c0 + V(cosf(a) * Rd, sinf(a) * Rd, 0), c0 + V(cosf(b) * Rd, sinf(b) * Rd, 0), V(0, 0, 1), (i & 1) ? 0x141008 : C_ORANGE); } }   /* the checkered hazard disc (t49.7, t51.5, t68) */
}
/* six-sided taper between two sections: for guns and limbs (centre, half width, half height at each end) */
static void taper(Mesh &m, V c0, float w0, float h0, V c1, float w1, float h1, uint32_t top, uint32_t side, uint32_t low, bool caps = true) {
    V ax = norm(c1 - c0), rt = cross(ax, V(0, 0, 1)); if (len(rt) < 0.2f) rt = V(1, 0, 0); rt = norm(rt); V up = norm(cross(rt, ax));
    static const float a[6] = {-1, -0.55f, 0.55f, 1, 0.55f, -0.55f}, b[6] = {0, 1, 1, 0, -1, -1};
    V p0[6], p1[6]; for (int i = 0; i < 6; i++) { p0[i] = c0 + rt * (a[i] * w0) + up * (b[i] * h0); p1[i] = c1 + rt * (a[i] * w1) + up * (b[i] * h1); }
    for (int i = 0; i < 6; i++) { int j = (i + 1) % 6; uint32_t col = (i == 1) ? top : (i == 4) ? low : (i == 0 || i == 2) ? side : ((((side >> 1) & 0x7F7F7F) + ((low >> 1) & 0x7F7F7F)));
        quadn(m, p0[i], p0[j], p1[j], p1[i], (p0[i] + p0[j] + p1[i] + p1[j]) * 0.25f - (c0 + c1) * 0.5f, col); }
    if (caps) for (int e = 0; e < 2; e++) { const V *p = e ? p1 : p0; V w = e ? ax : V(0, 0, 0) - ax; uint32_t c = e ? side : low;
        quadn(m, p[0], p[1], p[2], p[3], w, c); quadn(m, p[3], p[4], p[5], p[0], w, c); }
}
/* a lattice tower: pale posts in pairs, square rungs, crosses only in some bays, a hazard belt on one face and a strip down another, on a dark eight-sided plinth (Octavius t827-833) */
static void mk_tower(Mesh &m, V at) {
    const float W = 1050.f, T = HALL_Z, P = 110.f;
    const V cor[4] = {V(-W, -W, 0), V(W, -W, 0), V(W, W, 0), V(-W, W, 0)};
    for (int i = 0; i < 4; i++) { V c = at + cor[i]; m.box(c - V(P, P, 0), c + V(P, P, T), C_POST, 0xB4B4BC, 0x84848C); }
    auto bar = [&](V a, V b, float w) { taper(m, at + a, w, w, at + b, w, w, 0xA0A0A8, 0x82828C, 0x62626C, false); };
    for (int lvl = 0; lvl < 5; lvl++) { float z = 1400.f + lvl * 1900.f;
        for (int i = 0; i < 4; i++) { V a = cor[i], b = cor[(i + 1) & 3]; bar(a + V(0, 0, z), b + V(0, 0, z), 55.f); if (lvl == 0 || (lvl == 1 && (i & 1))) { bar(a + V(0, 0, z), b + V(0, 0, z + 1900.f), 34.f); bar(b + V(0, 0, z), a + V(0, 0, z + 1900.f), 34.f); } } }
    auto hazard = [&](V a, V b, float z0, float z1, float off) { V d = b - a; V n = norm(V(d.y, -d.x, 0)); if (dot(n, a) < 0) n = n * -1.f; const int N = 8; for (int k = 0; k < N; k++) { float u0 = (float)k / N, u1 = (float)(k + 1) / N, sl = (z1 - z0) / len(d); V pa = at + a + d * u0 + n * off, pb = at + a + d * u1 + n * off; uint32_t c = (k & 1) ? C_HZK : C_HZY;
            quadn(m, pa + V(0, 0, z0), pb + V(0, 0, z0), pb + d * sl + V(0, 0, z1), pa + d * sl + V(0, 0, z1), n, c); } };
    hazard(cor[0], cor[1], 3500.f, 3780.f, 125.f);
    hazard(cor[2], cor[3], 3500.f, 3780.f, 125.f);
    hazard(cor[1], cor[2], 520.f, 800.f, 125.f);
    { V n = norm(cor[1] + cor[2]) * 1.f; const int N = 9; V c = at + cor[1] + V(P + 6.f, 0, 0); for (int k = 0; k < N; k++) { float z0 = 1400.f + k * 210.f; quadn(m, c + V(0, -P, z0), c + V(0, P, z0), c + V(0, P, z0 + 210.f), c + V(0, -P, z0 + 210.f), V(1, 0, 0), (k & 1) ? C_HZK : C_HZY); } (void)n; }
    { const int N = 8; const float R0 = 2100.f, R1 = 1750.f, Hp = 140.f; for (int i = 0; i < N; i++) { float a = 6.2831853f * (i + 0.5f) / N, b = 6.2831853f * (i + 1.5f) / N; V o0(cosf(a), sinf(a), 0), o1(cosf(b), sinf(b), 0);
          trin(m, at + V(0, 0, Hp), at + o0 * R1 + V(0, 0, Hp), at + o1 * R1 + V(0, 0, Hp), V(0, 0, 1), 0x24262E); quadn(m, at + o0 * R1 + V(0, 0, Hp), at + o1 * R1 + V(0, 0, Hp), at + o1 * R0 + V(0, 0, 10), at + o0 * R0 + V(0, 0, 10), (o0 + o1) + V(0, 0, 1), 0x7C7C70); } }
}

static void mk_console(int id, uint32_t lamp) {   /* INVENTED: no footage of the self-destruct. A grey console with a lamp */
    Mesh m;
    m.box(V(-600, -600, 0), V(600, 600, 650), 0xA0A0C0, 0x707090, 0x404060);
    m.box(V(-450, -450, 650), V(450, 450, 780), 0x303048);
    m.box(V(-220, -220, 780), V(220, 220, 1080), lamp);
    m.box(V(-300, -640, 250), V(300, -600, 520), 0x202030);   /* a dark panel on each face */
    m.box(V(-300, 600, 250), V(300, 640, 520), 0x202030);
    m.box(V(-640, -300, 250), V(-600, 300, 520), 0x202030);
    m.box(V(600, -300, 250), V(640, 300, 520), 0x202030);
    m.add(id);
}

static void build_meshes() {
    door_clocks.clear();
    /* the ship: each open cell gets a floor, a ceiling and, per closed side, the wall section measured from t47 (rail over a blue panel, plain wall, sloped upper wall with a strip light) */
    Mesh m; const float h2 = CELL * 0.5f; const float RF = 2450.f, RT = 1100.f, DI = 1700.f, DZ = 2900.f;
    const V dirs[4] = {V(-1, 0, 0), V(1, 0, 0), V(0, -1, 0), V(0, 1, 0)};   /* inward normals: wall to the east, west, north, south */
    for (int r = 0; r < GH; r++) for (int c = 0; c < GW; c++) {
        char ch = MAP[r][c]; if (ch == '#') continue;
        V o(cx(c), cy(r), 0);
        if (ch == 'C') start_c = o; else if (ch == 'A') start_a = o; else if (ch == 'D') console_pos = o;
        const bool op[4] = {!solid_c(c + 1, r), !solid_c(c - 1, r), !solid_c(c, r - 1), !solid_c(c, r + 1)}; const int opp[4] = {1, 0, 3, 2};
        const bool hz = op[0] || op[1], vt = op[2] || op[3], straight = hz != vt;
        if (hall_c(c, r)) {    /* the dark hall: high, nearly black, blue strips and tan triangles on the floor, tan doors with a display box */
            quadn(m, o + V(-h2, -h2, 0), o + V(h2, -h2, 0), o + V(h2, h2, 0), o + V(-h2, h2, 0), V(0, 0, 1), C_HALLF);
            quadn(m, o + V(-h2, -h2, HALL_Z), o + V(h2, -h2, HALL_Z), o + V(h2, h2, HALL_Z), o + V(-h2, h2, HALL_Z), V(0, 0, -1), 0x20222A);
            for (int k = 0; k < 4; k++) { V n = dirs[k], t(-n.y, n.x, 0), P = o - n * h2; V zz(0, 0, HALL_Z);
                if (!op[k]) { quadn(m, P - t * h2, P + t * h2, P + t * h2 + zz, P - t * h2 + zz, n, C_HALLW); mk_door(m, o, n, true, true, h2); }
                else { int nc = c + (k == 0 ? 1 : k == 1 ? -1 : 0), nr = r + (k == 2 ? -1 : k == 3 ? 1 : 0); if (!hall_c(nc, nr)) quadn(m, P - t * h2 + V(0, 0, CEIL_Z), P + t * h2 + V(0, 0, CEIL_Z), P + t * h2 + zz, P - t * h2 + zz, n, C_HALLW); } }
            for (int s2 = -1; s2 <= 1; s2 += 2) { V a = o + V(s2 * 2100.f, -1400.f, 12), b = o + V(s2 * 1400.f, -2300.f, 12), e = o + V(s2 * 1000.f, -2100.f, 12), g = o + V(s2 * 1700.f, -1200.f, 12);
                quadn(m, a, b, e, g, V(0, 0, 1), C_HALLB); trin(m, o + V(s2 * 300.f, 1700.f, 14), o + V(s2 * 1500.f, 1500.f, 14), o + V(s2 * 300.f, 1300.f, 14), V(0, 0, 1), C_TAN); }
            continue; }
        quadn(m, o + V(-h2, -h2, 0), o + V(h2, -h2, 0), o + V(h2, h2, 0), o + V(-h2, h2, 0), V(0, 0, 1), C_FLOOR);
        quadn(m, o + V(-DI, -DI, CEIL_Z), o + V(DI, -DI, CEIL_Z), o + V(DI, DI, CEIL_Z), o + V(-DI, DI, CEIL_Z), V(0, 0, -1), C_CEIL);
        const float bw = 1300.f;   /* blue floor down the middle, the width of the rib's foot; wide open in front of a door */
        bool any_door = false; for (int k = 0; k < 4; k++) if (!op[k] && op[opp[k]]) any_door = true;
        if (any_door) quadn(m, o + V(-RF, -RF, 8), o + V(RF, -RF, 8), o + V(RF, RF, 8), o + V(-RF, RF, 8), V(0, 0, 1), C_BLUE);
        else { if (hz) quadn(m, o + V(-h2, -bw, 8), o + V(h2, -bw, 8), o + V(h2, bw, 8), o + V(-h2, bw, 8), V(0, 0, 1), C_BLUE);
               if (vt) quadn(m, o + V(-bw, -h2, 8), o + V(bw, -h2, 8), o + V(bw, h2, 8), o + V(-bw, h2, 8), V(0, 0, 1), C_BLUE); }
        if (hz) { quadn(m, o + V(-h2, -430, CEIL_Z - 12), o + V(-900, -430, CEIL_Z - 12), o + V(-900, 430, CEIL_Z - 12), o + V(-h2, 430, CEIL_Z - 12), V(0, 0, -1), C_LIGHT);   /* the middle strip light, broken at each rib */
                  quadn(m, o + V(900, -430, CEIL_Z - 12), o + V(h2, -430, CEIL_Z - 12), o + V(h2, 430, CEIL_Z - 12), o + V(900, 430, CEIL_Z - 12), V(0, 0, -1), C_LIGHT); }
        if (vt) { quadn(m, o + V(-430, -h2, CEIL_Z - 12), o + V(430, -h2, CEIL_Z - 12), o + V(430, -900, CEIL_Z - 12), o + V(-430, -900, CEIL_Z - 12), V(0, 0, -1), C_LIGHT);
                  quadn(m, o + V(-430, 900, CEIL_Z - 12), o + V(430, 900, CEIL_Z - 12), o + V(430, h2, CEIL_Z - 12), o + V(-430, h2, CEIL_Z - 12), V(0, 0, -1), C_LIGHT); }
        if (straight) archring(m, o, hz ? V(1, 0, 0) : V(0, 1, 0), (c == 6 && r == 2) ? 240.f : 170.f);
        for (int k = 0; k < 4; k++) {
            V n = dirs[k], t(-n.y, n.x, 0);
            auto W = [&](float u, float d, float z) { return o + t * u - n * d + V(0, 0, z); };
            const int ka = (k == 0) ? 3 : (k == 1) ? 2 : (k == 2) ? 0 : 1, kb = opp[ka];   /* the sides at the u+ and u- ends of this wall */
            const float hi_d = 3000.f;
            if (op[k]) {   /* open side: flat ceiling out to the edge of the cell */
                quadn(m, W(-DI, DI, CEIL_Z), W(DI, DI, CEIL_Z), W(DI, h2, CEIL_Z), W(-DI, h2, CEIL_Z), V(0, 0, -1), C_CEIL);
                if (op[ka]) { trin(m, W(DI, h2, CEIL_Z), W(h2, h2, DZ), W(h2, DI, CEIL_Z), V(0, 0, -1), C_DIAG); trin(m, W(DI, h2, CEIL_Z), W(h2, DI, CEIL_Z), W(DI, DI, CEIL_Z), V(0, 0, -1), C_CEIL); }
                continue; }
            bool door = op[opp[k]];
            const bool ea = !op[ka], eb = !op[kb];      /* a wall at that end: mitre */
            const bool ra = ea && !op[opp[ka]], rb = eb && !op[opp[kb]];   /* ...and it has a rail to mitre into (a door wall has none) */
            float ua0 = ea ? DI : h2, ub0 = eb ? -DI : -h2;
            quadn(m, W(-h2, hi_d, 0), W(h2, hi_d, 0), W(h2, hi_d, DZ), W(-h2, hi_d, DZ), n, door ? C_LEAF2 : C_WALL);
            quadn(m, W(ub0, DI, CEIL_Z), W(ua0, DI, CEIL_Z), W(h2, hi_d, DZ), W(-h2, hi_d, DZ), n + V(0, 0, -1), C_DIAG);
            if (door) { archring(m, W(0, h2 - 240.f, 0), n, 170.f); mk_door(m, o, n, (c + r) % 2 == 0, (c + r) % 2 == 0, h2 - 170.f); floor_marks(m, o, n); }
            else {
                const float ua = ra ? RF : h2, ub = rb ? -RF : -h2;
                quadn(m, W(ub, RF, RT), W(ua, RF, RT), W(h2, hi_d, RT), W(-h2, hi_d, RT), V(0, 0, 1), C_RAIL);
                quadn(m, W(ub, RF, RT - 240), W(ua, RF, RT - 240), W(ua, RF, RT), W(ub, RF, RT), n, C_RAIL2);
                quadn(m, W(ub, RF, 200), W(ua, RF, 200), W(ua, RF, RT - 240), W(ub, RF, RT - 240), n, C_PANEL);
                quadn(m, W(ub, RF, 0), W(ua, RF, 0), W(ua, RF, 200), W(ub, RF, 200), n, C_KICK);
                for (int e = 0; e < 2; e++) { const bool end_open = e ? op[kb] : op[ka]; if (!end_open) continue;   /* cap the rail where the wall stops */
                    int sc = c, sr = r; int ke = e ? kb : ka; sc += (ke == 0 ? 1 : ke == 1 ? -1 : 0); sr += (ke == 2 ? -1 : ke == 3 ? 1 : 0);
                    bool cont = solid_c(sc + (k == 0 ? 1 : k == 1 ? -1 : 0), sr + (k == 2 ? -1 : k == 3 ? 1 : 0)); if (cont) continue;
                    float u = e ? -h2 : h2; quadn(m, W(u, RF, 0), W(u, hi_d, 0), W(u, hi_d, RT), W(u, RF, RT), t * (e ? -1.f : 1.f), C_KICK); }
                if ((c * 7 + r * 3 + k) % 9 == 0) { V bx = W(0, RF - 150.f, 0); const float q = 300.f;  /* the small hazard box at the foot of a wall (Octavius t844) */
                    m.box(V((std::min)(bx.x - fabsf(t.x) * q - fabsf(n.x) * 150.f, bx.x), (std::min)(bx.y - fabsf(t.y) * q - fabsf(n.y) * 150.f, bx.y), 0), V(bx.x + fabsf(t.x) * q + fabsf(n.x) * 150.f, bx.y + fabsf(t.y) * q + fabsf(n.y) * 150.f, 640), 0x303038);
                    for (int i = 0; i < 3; i++) quadn(m, bx + n * 156.f + t * (-q + i * 200.f) + V(0, 0, 60), bx + n * 156.f + t * (-q + 100.f + i * 200.f) + V(0, 0, 60), bx + n * 156.f + t * (-q + 200.f + i * 200.f) + V(0, 0, 340), bx + n * 156.f + t * (-q + 100.f + i * 200.f) + V(0, 0, 340), n, C_HZY); }
            }
            /* the strip light in its box on the sloped upper wall, broken at each rib */
            V nrm = norm(n + V(0, 0, -1) * ((hi_d - DI) / (CEIL_Z - DZ)));
            const float d0 = 1950.f, d1 = 2800.f, z0 = CEIL_Z - (d0 - DI) * (CEIL_Z - DZ) / (hi_d - DI), z1 = CEIL_Z - (d1 - DI) * (CEIL_Z - DZ) / (hi_d - DI), lift = 170.f;
            auto seg = [&](float a, float b) { V p0 = W(a, d0, z0) + nrm * lift, p1 = W(b, d0, z0) + nrm * lift, p2 = W(b, d1, z1) + nrm * lift, p3 = W(a, d1, z1) + nrm * lift;
                quadn(m, p0, p1, p2, p3, nrm, C_LIGHT);
                quadn(m, p3, p2, W(b, d1, z1), W(a, d1, z1), V(0, 0, -1) + n * 0.6f, C_LIGHTBOX); quadn(m, p0, p1, W(b, d0, z0), W(a, d0, z0), n * 0.6f + V(0, 0, 1), C_LIGHTBOX);
                quadn(m, p0, p3, W(a, d1, z1), W(a, d0, z0), t * -1.f, C_LIGHTBOX); quadn(m, p1, p2, W(b, d1, z1), W(b, d0, z0), t, C_LIGHTBOX); };
            if (!door) { float lim_a = ea ? 1700.f : h2, lim_b = eb ? -1700.f : -h2; if (lim_a > 800.f) seg(800.f, lim_a); if (lim_b < -800.f) seg(lim_b, -800.f); }
        }
    }
    { float hx = cx(6), hy = cy(5) - 3000.f; tower_pos[0] = V(hx - 5200.f, hy + 800.f, 0); tower_pos[1] = V(hx + 3600.f, hy - 500.f, 0); mk_tower(m, tower_pos[0]); mk_tower(m, tower_pos[1]); }
    m.add(M_SHIP);
    d1k::dl("ship mesh: %zu polys", m.p.size());
    mk_console(M_CONS_ARM, 0xFF2020); mk_console(M_CONS_OFF, 0x601010); mk_console(M_CONS_HALF, 0xFFB020); mk_console(M_CONS_SAFE, 0x20E050);

    /* the player's gun, measured from t44, t47, t66: a long tan barrel with slots and a red dot along a big cream forearm that tapers to a prow. Hand at the origin, y forward, tip 1040 ahead */
    { Mesh g; const uint32_t T1 = 0xC2AC9C, T2 = 0xA88E80, T3 = 0x806C64, K1 = 0xF0F0DC, K2 = 0xD8D8C0, K3 = 0xA8A898;
      taper(g, V(-28, -300, 28), 46, 46, V(-28, 930, 26), 40, 36, T1, T2, T3);
      taper(g, V(-28, 930, 26), 40, 36, V(-28, 1040, 12), 26, 18, T1, T3, T3);
      for (int i = 0; i < 3; i++) { float y = 900.f + i * 40.f; quadn(g, V(-52, y, 64 - i * 3.f), V(-4, y, 64 - i * 3.f), V(-4, y + 20, 63 - i * 3.f), V(-52, y + 20, 63 - i * 3.f), V(0, 0, 1), 0x201C1C); }
      quadn(g, V(-72, 520, 22), V(-72, 590, 22), V(-70, 590, 58), V(-70, 520, 58), V(-1, 0, 0), 0xE02818);    /* the red dot, on the left of the barrel */
      taper(g, V(70, -560, -70), 165, 120, V(70, 150, -60), 130, 100, K1, K2, K3);
      taper(g, V(70, 150, -60), 130, 100, V(46, 500, -30), 34, 34, K1, K2, K3);
      g.add(M_GUN); }
    /* the muzzle flash: a jagged eight-point yellow star across the barrel (t51, t53), white in the middle with orange sparks */
    { Mesh s; const float R8[8] = {560, 330, 470, 300, 590, 340, 440, 320}; const float r = 170.f;
      for (int i = 0; i < 8; i++) { float a0 = 6.2831853f * (i - 0.5f) / 8, a1 = 6.2831853f * (i + 0.5f) / 8, am = 6.2831853f * i / 8 + 0.15f;
          V b0(cosf(a0) * r, 0, sinf(a0) * r), b1(cosf(a1) * r, 0, sinf(a1) * r), tip(cosf(am) * R8[i], 0, sinf(am) * R8[i]);
          quad2(s, V(0, 0, 0), b0, tip, b1, (i & 1) ? 0xFFF47A : 0xFFE860);
          V sp(cosf(am + 0.39f) * 190.f, -10, sinf(am + 0.39f) * 190.f); tri2(s, sp * 0.4f, sp, sp * 0.7f + V(20, 0, 40), 0xFF8020); }
      for (int i = 0; i < 6; i++) { float a = 6.2831853f * i / 6; tri2(s, V(0, -12, 0), V(cosf(a) * 120.f, -12, sinf(a) * 120.f), V(cosf(a + 1.05f) * 120.f, -12, sinf(a + 1.05f) * 120.f), 0xFFFFF4); }
      s.add(M_STAR); }
    { Mesh sp; sp.poly({V(-40, 0, -24), V(40, 0, -24), V(0, 0, 90)}, 0xF09020); sp.poly({V(0, -40, -24), V(0, 40, -24), V(0, 0, 90)}, 0xF09020); sp.add(M_SPARK); }

    /* the cyborg (attract t602-605, t638; the far figure at t833): faceted blue-lavender muscle, wide chest, narrow waist, small head with fins and red eyes, small orange shoulder caps. Origin at the hip, y forward */
    { const uint32_t L = 0xA8B0FF, B = 0x7078F0, DK = 0x4048B8; Mesh b;
      taper(b, V(0, 0, -40), 200, 150, V(0, 0, 270), 150, 120, L, B, DK);                 /* waist */
      taper(b, V(0, 0, 270), 170, 130, V(0, 0, 800), 360, 200, L, B, DK);                 /* chest */
      taper(b, V(0, 40, 620), 300, 180, V(0, 40, 800), 280, 190, 0xC8D0FF, L, B);         /* chest plate */
      taper(b, V(0, 0, 800), 130, 110, V(0, 0, 900), 90, 90, L, B, DK);
      taper(b, V(0, 0, 890), 95, 110, V(0, 10, 1090), 90, 110, L, B, DK);                 /* head */
      b.box(V(-70, 112, 980), V(-18, 122, 1020), 0xFF2020); b.box(V(18, 112, 980), V(70, 122, 1020), 0xFF2020);
      for (int k = -1; k <= 1; k += 2) { pyr(b, V(k * 70.f, -10, 1060), 34.f, 170.f, norm(V(k * 0.35f, -0.1f, 1)), 0xE8501C);   /* head fins */
          taper(b, V(k * 400.f, 0, 790), 110, 110, V(k * 440.f, 0, 910), 90, 80, 0xFF7030, 0xE8501C, 0xA83010);                 /* shoulder caps */
          taper(b, V(k * 430.f, 0, 780), 105, 105, V(k * 470.f, 20, 400), 85, 85, L, B, DK); }   /* upper arms */
      taper(b, V(-470, 20, 400), 80, 80, V(-470, 60, 60), 70, 70, L, B, DK);
      taper(b, V(470, 20, 400), 80, 80, V(470, 420, 430), 70, 70, L, B, DK);              /* right arm forward with the gun */
      /* the arm gun as printed in Generation 4 p141: a pale sleeve, a long dark barrel with a red dash, a yellow round muzzle with black slots */
      taper(b, V(470, 330, 425), 115, 115, V(470, 620, 445), 105, 105, 0xF0F0DC, 0xD8D8C0, 0xA8A898);
      taper(b, V(470, 620, 445), 72, 72, V(470, 1280, 480), 56, 56, 0x34302C, 0x1C1A18, 0x0C0C0C);
      b.box(V(405, 880, 520), V(535, 1000, 548), 0xE02818);
      taper(b, V(470, 1280, 480), 105, 105, V(470, 1330, 482), 105, 105, 0xE8D850, 0xD8C840, 0xA89830);
      b.box(V(420, 1331, 505), V(520, 1336, 525), 0x101010); b.box(V(420, 1331, 445), V(520, 1336, 465), 0x101010);
      trin(b, V(-120, 150, 760), V(120, 150, 760), V(0, 170, 480), V(0, 1, 0), 0xE8501C); tri2(b, V(330, 120, 700), V(180, 170, 720), V(280, 180, 430), 0xE8501C);
      b.add(M_CY_BODY);
      Mesh l; taper(l, V(0, 0, -30), 170, 170, V(0, 20, -460), 115, 125, L, B, DK); taper(l, V(0, 20, -460), 115, 125, V(0, -10, -800), 90, 95, L, B, DK); taper(l, V(0, -10, -800), 100, 110, V(0, 110, -900), 90, 40, B, DK, DK); l.add(M_CY_LEG); }
    /* the alien (Octavius t820-826, t829): a hunched brown thing, thin everywhere but the great mantle of hair swept back over the shoulders, the head sunk low and forward with a dark triangular mouth and white fangs, long thin arms and bent thin legs, all ending in claws */
    { const uint32_t BR = 0x683A24, BR2 = 0x7C4A2C, BR3 = 0x3C2214, BONE = 0xF0E8D0;
      Mesh b;
      taper(b, V(0, -30, -30), 150, 120, V(0, 130, 470), 300, 210, BR2, BR, BR3);          /* narrow hips up to the chest, leaning forward */
      for (int k = -1; k <= 1; k += 2) { float q = (float)k;
          taper(b, V(q * 170.f, 90, 360), 210, 200, V(q * 330.f, -60, 840), 200, 170, BR2, BR, BR3);   /* the mantle rises off each shoulder... */
          taper(b, V(q * 330.f, -60, 840), 200, 170, V(q * 300.f, -380, 1040), 130, 90, BR2, BR, BR3);  /* ...and sweeps back, blunt */
          taper(b, V(q * 110.f, -60, 600), 150, 150, V(q * 170.f, -480, 880), 60, 40, BR, BR3, BR3);
          for (int i = 0; i < 6; i++) pyr(b, V(q * (300.f + i * 26.f), -60.f - i * 50.f, 480.f + i * 70.f), 42.f, 150.f, norm(V(q * 0.7f, -1, -0.2f)), BR3); }
      taper(b, V(0, 90, 690), 400, 200, V(0, -240, 1010), 380, 150, BR2, BR, BR3);        /* the mantle is one arch over the head (t822-825) */
      taper(b, V(0, 230, 500), 160, 150, V(0, 520, 430), 130, 120, BR2, BR, BR3);          /* the head, low and forward */
      trin(b, V(-105, 532, 500), V(105, 532, 500), V(0, 538, 300), V(0, 1, 0), 0x0C0806);    /* the mouth */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; tri2(b, V(q * 85, 540, 498), V(q * 50, 540, 498), V(q * 48, 542, 380), BONE); tri2(b, V(q * 22, 540, 330), V(q * 48, 540, 370), V(q * 24, 542, 455), BONE); }
      b.box(V(-90, 528, 515), V(-40, 536, 548), 0x2C1208); b.box(V(40, 528, 515), V(90, 536, 548), 0x2C1208);
      taper(b, V(-330, 140, 500), 85, 85, V(-470, 300, 30), 55, 55, BR2, BR, BR3);        /* left arm, long and thin, to the ground */
      taper(b, V(-470, 300, 30), 50, 50, V(-420, 380, -520), 36, 36, BR2, BR, BR3);
      for (int k = -1; k <= 1; k++) pyr(b, V(-420.f + k * 40.f, 380, -520), 22.f, 250.f, norm(V(k * 0.25f, 0.2f, -1)), BONE);
      b.add(M_AL_BODY);
      Mesh a; taper(a, V(0, 0, 0), 85, 85, V(0, 0, -520), 52, 52, BR2, BR, BR3); taper(a, V(0, 0, -520), 48, 48, V(0, 0, -1010), 34, 34, BR2, BR, BR3);    /* the right arm and its claws, origin at the shoulder */
      for (int k = -1; k <= 1; k++) pyr(a, V(k * 38.f, 0, -1010), 22.f, 260.f, norm(V(k * 0.25f, 0, -1)), BONE);
      a.add(M_AL_ARM);
      Mesh l; taper(l, V(0, 0, 0), 120, 120, V(0, 130, -360), 62, 62, BR2, BR, BR3); taper(l, V(0, 130, -360), 52, 52, V(0, -70, -740), 36, 36, BR, BR3, BR3);   /* thigh forward, shin back */
      for (int k = -1; k <= 1; k++) pyr(l, V(k * 34.f, -70, -745), 26.f, 330.f, norm(V(k * 0.4f, 1, -0.12f)), BONE);    /* long clawed toes */
      l.add(M_AL_LEG); }
    /* the crawling thing (Octavius t840-843): a grey body, four orange legs ending in white claws, a dark mouth with fangs */
    { Mesh t; const uint32_t G = 0x8A8E90, G2 = 0x6A6E72;
      taper(t, V(0, -300, 0), 170, 150, V(0, 260, 0), 210, 170, 0xA8ACAE, G, G2);
      trin(t, V(-130, 262, 90), V(130, 262, 90), V(0, 266, -130), V(0, 1, 0), 0x0C0806);
      for (int k = -1; k <= 1; k += 2) { tri2(t, V(k * 90.f, 270, 95), V(k * 50.f, 270, 95), V(k * 55.f, 272, -20), 0xF4F4E8);
          for (int j = -1; j <= 1; j += 2) { V a(k * 140.f, j * 120.f, 0), kn(k * 620.f, j * 420.f, 90), tp(k * 860.f, j * 760.f, -40);
              taper(t, a, 60, 60, kn, 50, 50, 0xD07840, 0xB05C28, 0x804018); taper(t, kn, 48, 48, tp, 30, 30, 0xD07840, 0xB05C28, 0x804018); pyr(t, tp, 34.f, 190.f, norm(tp - kn), 0xFFFFF4); } }
      t.add(M_THING); }

    /* the title card: a green polygon skull in a hexagonal web, pale in the middle, dark faceted sockets, nose slot, long teeth (operator tape t612-621) */
    { Mesh s; const int LON = 14, LAT = 8; const float rx = 1000.f, ry = 850.f, rz = 1350.f;
      auto sp = [&](int i, int j) { float th = 3.14159265f * i / LAT, ph = 6.2831853f * j / LON; return V(rx * sinf(th) * cosf(ph), ry * sinf(th) * sinf(ph), rz * cosf(th)); };
      for (int i = 0; i < LAT; i++) for (int j = 0; j < LON; j++) {
          V a = sp(i, j), b = sp(i, j + 1), c = sp(i + 1, j + 1), d = sp(i + 1, j), cen = (a + b + c + d) * 0.25f;
          float q = fabsf(cen.x) / rx + fabsf(cen.z) / rz * 0.7f; bool front = cen.y < 0, alt = ((i + j) & 1) != 0;
          uint32_t col = (front && q < 0.62f) ? (alt ? 0xE4F2F4 : 0xC8E4EA) : (front && q < 0.95f) ? (alt ? 0xC8F08C : 0xA8E468) : (alt ? 0x9CDC30 : 0x84CC20);
          if (i == 0) trin(s, a, c, d, cen, col); else if (i == LAT - 1) trin(s, a, b, c, cen, col); else quadn(s, a, b, c, d, cen, col); }
      auto sy = [&](float x, float z) { float q = 1.f - (x / rx) * (x / rx) - (z / rz) * (z / rz); return -ry * sqrtf(q > 0.05f ? q : 0.05f) - 70.f; };
      for (int k = -1; k <= 1; k += 2) { float q = (float)k;   /* the sockets: faceted dark diamonds, a red glint inside */
          auto P = [&](float x, float z) { return V(q * x, sy(q * x, z), z); };
          quadn(s, P(120, 330), P(640, 470), P(600, 180), P(330, 40), V(0, -1, 0), 0x1C2018);
          trin(s, P(330, 40), P(600, 180), P(250, 200), V(0, -1, 0), 0x3C3820);
          quadn(s, P(390, 300), P(500, 310), P(480, 230), P(390, 200), V(0, -1, 0), 0xE02020); }
      { auto P = [&](float x, float z) { return V(x, sy(x, z), z); };
        quadn(s, P(-130, -160), P(130, -160), P(110, 110), P(-110, 110), V(0, -1, 0), 0x303428);
        for (int k = 0; k < 6; k++) { float x = -420.f + k * 168.f; trin(s, P(x - 70, -420), P(x + 70, -420), P(x, -980), V(0, -1, 0), (k & 1) ? 0xF4F8F4 : 0xB4D8DC); trin(s, P(x + 20, -500), P(x + 110, -500), P(x + 84, -1000), V(0, -1, 0), 0x202818); } }
      s.add(M_SKULL);
      Mesh w; const V ax(0, 1, 0); const float pu[6] = {-1500, -750, 750, 1500, 750, -750}, pz[6] = {1500, 3000, 3000, 1500, 0, 0}; const uint32_t WC = 0xD0F088;
      /* the web: small open hexagons in a ring round the oval, a spike out of each corner */
      const int NH = 10; for (int i = 0; i < NH; i++) { float a = 6.2831853f * (i + 0.5f) / NH; V c(cosf(a) * 1100.f, 300.f, sinf(a) * 1500.f); hexring(w, c - V(0, 0, 1500), ax, 7.f, 0.2f, 0.17f, WC);
          V tip = c * 1.32f; tip.y = 300; quad2(w, c + V(-6, 0, 0) * 1.f, c + V(6, 0, 0), tip + V(6, 0, 0), tip + V(-6, 0, 0), WC); }
      (void)pu; (void)pz; w.add(M_WEB);
      /* the letters: tall grey-blue block capitals across the whole width, two-tone like the chrome on the card */
      Mesh L; const float LW = 300.f, LH = 1300.f, G = 62.f; const char *ttl = "GRID WARRIORS"; float x0 = -(13 * (LW + G)) * 0.5f;
      auto st = [&](float x, float ax0, float az0, float bx, float bz, float wd) { V a(x + ax0 * LW, -1100, az0 * LH), b(x + bx * LW, -1100, bz * LH); V d = norm(b - a), n(-d.z * wd, 0, d.x * wd);
          V e = n * 0.45f; quadn(L, a - n, b - n, b + e, a + e, V(0, -1, 0), 0x3A4656); quadn(L, a + e, b + e, b + n, a + n, V(0, -1, 0), 0xC8D8E4); };
      for (int i = 0; i < 13; i++) { char ch = ttl[i]; float x = x0 + i * (LW + G); const float w0 = 56.f;
          switch (ch) {
          case 'G': st(x, 0.08f, 0, 0.08f, 1, w0); st(x, 0, 1, 1, 1, w0); st(x, 0, 0, 1, 0, w0); st(x, 0.92f, 0, 0.92f, 0.45f, w0); st(x, 0.5f, 0.45f, 1, 0.45f, w0); break;
          case 'R': st(x, 0.08f, 0, 0.08f, 1, w0); st(x, 0, 1, 1, 1, w0); st(x, 0.92f, 1, 0.92f, 0.5f, w0); st(x, 0, 0.5f, 1, 0.5f, w0); st(x, 0.45f, 0.5f, 0.92f, 0, w0); break;
          case 'I': st(x, 0.5f, 0, 0.5f, 1, w0 * 1.1f); break;
          case 'D': st(x, 0.08f, 0, 0.08f, 1, w0); st(x, 0, 1, 0.8f, 1, w0); st(x, 0, 0, 0.8f, 0, w0); st(x, 0.92f, 0.12f, 0.92f, 0.88f, w0); st(x, 0.78f, 1, 0.92f, 0.86f, w0); st(x, 0.78f, 0, 0.92f, 0.14f, w0); break;
          case 'W': st(x, 0, 1, 0.22f, 0, w0); st(x, 0.22f, 0, 0.5f, 0.7f, w0); st(x, 0.5f, 0.7f, 0.78f, 0, w0); st(x, 0.78f, 0, 1, 1, w0); break;
          case 'A': st(x, 0, 0, 0.5f, 1, w0); st(x, 0.5f, 1, 1, 0, w0); st(x, 0.22f, 0.35f, 0.78f, 0.35f, w0); break;
          case 'O': st(x, 0.08f, 0, 0.08f, 1, w0); st(x, 0.92f, 0, 0.92f, 1, w0); st(x, 0, 1, 1, 1, w0); st(x, 0, 0, 1, 0, w0); break;
          case 'S': st(x, 0, 1, 1, 1, w0); st(x, 0.08f, 1, 0.08f, 0.5f, w0); st(x, 0, 0.5f, 1, 0.5f, w0); st(x, 0.92f, 0.5f, 0.92f, 0, w0); st(x, 0, 0, 1, 0, w0); break;
          default: break; } }
      L.add(M_LETTERS);
      /* the lime ground of the attract, big flat facets (t602-610) */
      Mesh g; const float A = 60000.f; const uint32_t GC[4] = {0xA8E878, 0x9CE06C, 0xB4EC88, 0x90D860}; for (int i = -4; i < 4; i++) for (int j = -4; j < 4; j++) { float q = A / 4; V a(i * q, j * q, 0), b((i + 1) * q, j * q, 40.f * ((i + j) & 3)), c((i + 1) * q, (j + 1) * q, 0), d(i * q, (j + 1) * q, 40.f * ((i * j) & 3));
          trin(g, a, b, c, V(0, 0, 1), GC[(i * 3 + j + 40) & 3]); trin(g, a, c, d, V(0, 0, 1), GC[(i + j * 5 + 41) & 3]); }
      g.add(M_GREEN);
      Mesh sa; const uint32_t SC[3] = {0x847A58, 0x807654, 0x887E5A}; for (int i = -6; i < 6; i++) for (int j = -6; j < 6; j++) { float q = 90000.f; quadn(sa, V(i * q, j * q, 0), V((i + 1) * q, j * q, 0), V((i + 1) * q, (j + 1) * q, 0), V(i * q, (j + 1) * q, 0), V(0, 0, 1), SC[(i * 3 + j * 7 + 90) % 3]); } sa.add(M_SAND); }
    /* the clock's pale digits (Octavius t840): seven small bars, each with thickness */
    { static const unsigned char SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; const float a = 78.f, bb = 115.f, w = 15.f; const uint32_t c = 0xB8BCC0;
      for (int d = 0; d < 10; d++) { Mesh g; auto seg = [&](float x0, float z0, float x1, float z1) { g.box(V(x0 - w, -12, z0 - w), V(x1 + w, 12, z1 + w), c); };
          if (SEG[d] & 1) seg(-a, bb, a, bb); if (SEG[d] & 2) seg(a, 0, a, bb); if (SEG[d] & 4) seg(a, -bb, a, 0); if (SEG[d] & 8) seg(-a, -bb, a, -bb); if (SEG[d] & 16) seg(-a, -bb, -a, 0); if (SEG[d] & 32) seg(-a, 0, -a, bb); if (SEG[d] & 64) seg(-a, 0, a, 0);
          g.add(M_CDIG + d); }
      Mesh cl; cl.box(V(-w, -12, 40), V(w, 12, 70), c); cl.box(V(-w, -12, -70), V(w, 12, -40), c); cl.add(M_CCOLON); }
}

/* ---- sounds: INVENTED placeholders, no sound from the original survives on footage clearly enough to copy ---- */
static void sfx(int ch, uint32_t key, int vol, int pan = 0) { if (!sound_on) return; buggy::play(ch, (int)key, vol, pan); }
static void make_sounds() {
    std::mt19937 r(11); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(SND_FIRE, 0.3f, [&](float t) { return (sinf(6.2831853f * (900.f - 1500.f * t) * t) * 0.7f + noise() * 0.3f) * expf(-t * 12.f); });
    make_sound(SND_CLAW, 0.3f, [&](float t) { return noise() * sinf(3.14159f * t / 0.3f) * 0.6f; });
    make_sound(SND_HURT, 0.5f, [&](float t) { return noise() * expf(-t * 7.f) * 0.7f + sinf(6.2831853f * 90.f * t) * expf(-t * 6.f) * 0.6f; });
    make_sound(SND_ROAR, 0.8f, [&](float t) { return (sinf(6.2831853f * (110.f - 40.f * t) * t) * 0.6f + noise() * 0.3f) * sinf(3.14159f * t / 0.8f); });
    make_sound(SND_BEEP, 0.12f, [&](float t) { return sinf(6.2831853f * 1000.f * t) * expf(-t * 18.f) * 0.5f; });
    make_sound(SND_DISARM, 0.1f, [&](float t) { return sinf(6.2831853f * 1500.f * t) * expf(-t * 25.f) * 0.4f; });
    make_sound(SND_DONE, 0.9f, [&](float t) { return (sinf(6.2831853f * 660.f * t) + sinf(6.2831853f * 880.f * t)) * 0.3f * expf(-t * 3.f); });
    make_sound(SND_BOOM, 1.8f, [&](float t) { return noise() * expf(-t * 2.2f) * 0.9f + sinf(6.2831853f * 45.f * t) * expf(-t * 1.6f) * 0.7f; });
    make_sound(SND_GO, 0.6f, [&](float t) { return sinf(6.2831853f * (330.f + 400.f * t) * t) * expf(-t * 3.f) * 0.6f; });
}

/* ---- the fighters ---- */
struct Fighter {
    bool cyb = true, alive = true; V p; float yaw = 0, cool = 0, phase = 0, moved = 0, swing = 0, flash = 0, dead_t = 0, think = 0, claw_pend = -1; int hp = 3;
};
static Fighter F[2];                       /* 0 cyborg, 1 alien */
struct Thing { V p, from, to; float t = 1, yaw = 0, back = 0; bool alive = true; };    /* the crawlers: seen and shot at in the footage; how they move is INVENTED, they do no harm */
static Thing things[3];
static int human = 0;
struct Spark { V p, v; float life; };
static std::vector<Spark> sparks;
static bool disarmed = false; static float disarm_p = 0;
enum State { S_TITLE, S_PLAY, S_OVER };
enum Result { R_NONE, R_CYBORG, R_ALIEN, R_BOOM, R_DRAW };
static State st = S_TITLE; static Result res = R_NONE;
static float st_t = 0, time_left = 0, beep_t = 0, flash_end = 0; static double sim_t = 0;
static bool slave_game = false;                           /* link play: a slave inside the master's round; it only moves its own fighter and asks to fire */
static uint32_t net_fire_seq = 0; static V net_fire_from, net_fire_dir; static uint8_t net_epoch = 0;
static V walk_off; static bool have_off = false;

static void burst(V at, int n, float speed, float life) {
    for (int i = 0; i < n; i++) { V d = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-0.3f, 1.2f))); sparks.push_back({at, d * (speed * rnd(0.3f, 1.f)), life * rnd(0.6f, 1.f)}); }
}
static float dist2d(V a, V b) { float dx = a.x - b.x, dy = a.y - b.y; return sqrtf(dx * dx + dy * dy); }
static V fwd_of(float yaw) { return V(-sinf(yaw), cosf(yaw), 0); }
static bool free_at(float x, float y) {
    const float R = 1500.f;   /* the rib's foot narrows the way to about 3 m */
    if (solid_xy(x - R, y - R) || solid_xy(x + R, y - R) || solid_xy(x - R, y + R) || solid_xy(x + R, y + R)) return false;
    for (int i = 0; i < 2; i++) { float tx = x - tower_pos[i].x, ty = y - tower_pos[i].y; if (fabsf(tx) < 1500.f && fabsf(ty) < 1500.f) return false; }
    float dx = x - console_pos.x, dy = y - console_pos.y; return dx * dx + dy * dy >= 1050.f * 1050.f;
}
static void slide(V &p, V to) { if (free_at(to.x, p.y)) p.x = to.x; if (free_at(p.x, to.y)) p.y = to.y; }
static bool has_los(V a, V b) {
    float d = dist2d(a, b); int n = (int)(d / 150.f) + 1;
    for (int i = 1; i < n; i++) { float t = (float)i / n; if (solid_xy(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t)) return false; }
    return true;
}

static void end_round(Result r) {
    if (st != S_PLAY) return;
    res = r; st = S_OVER; st_t = 0; if (r == R_BOOM) { flash_end = 0; sfx(4, SND_BOOM, 127); }
    static const char *nm[] = {"none", "cyborg wins", "alien wins (cyborg down)", "alien wins (ship destroyed)", "time up, ship saved"};
    dl("t=%.1f round end: %s | disarmed %d, cyborg hp %d, alien hp %d, time left %.0f", sim_t, nm[r], disarmed ? 1 : 0, F[0].hp, F[1].hp, time_left);
}
static void check_win() { if (st == S_PLAY && disarmed && !F[1].alive) end_round(R_CYBORG); }
static void hurt_cyborg(int by) {
    Fighter &c = F[0]; (void)by; if (!c.alive) return;
    c.hp--; sfx(1, SND_HURT, 127); burst(c.p + V(0, 0, 1200), 12, 2000.f, 0.8f);
    if (human == 0 && !autoplay) VPIX_Haptic(0, 0.25f, 1.f);
    dl("t=%.1f cyborg clawed, hp %d", sim_t, c.hp);
    if (c.hp <= 0) { c.alive = false; end_round(R_ALIEN); }
}
static void hurt_alien() {
    Fighter &a = F[1]; if (!a.alive) return;
    a.hp--; sfx(2, SND_ROAR, 100); burst(a.p + V(0, 0, 1100), 16, 2400.f, 0.9f);
    dl("t=%.1f alien shot, hp %d", sim_t, a.hp);
    if (a.hp <= 0) { a.alive = false; a.dead_t = alien_respawn; burst(a.p + V(0, 0, 1000), 40, 3200.f, 1.6f); dl("t=%.1f alien down (disarmed %d)", sim_t, disarmed ? 1 : 0); check_win(); }
}
/* the arm gun is a ray: footage shows no projectile (INVENTED: hitscan) */
static void shoot(V from, V dir) {
    Fighter &c = F[0]; c.cool = gun_gap; c.flash = 0.3f; sfx(0, SND_FIRE, 110);
    if (slave_game) { net_fire_seq++; net_fire_from = from; net_fire_dir = dir; return; }   /* linked slave: the master fires the ray */
    V p = from; dir = norm(dir);
    for (int i = 0; i < 400; i++) { p += dir * 150.f;
        for (Thing &k : things) if (k.alive && len(p - k.p) < 700.f) { k.alive = false; k.back = 12.f; burst(k.p, 18, 2000.f, 0.9f); dl("t=%.1f crawler shot", sim_t); return; }
        if (p.z < 0 || p.z > HALL_Z || solid_xy(p.x, p.y)) { burst(p - dir * 100.f, 8, 1500.f, 0.5f); return; }
        { float dx = p.x - console_pos.x, dy = p.y - console_pos.y; if (dx * dx + dy * dy < 600.f * 600.f && p.z < 1100.f) { burst(p, 8, 1500.f, 0.5f); return; } }
        const Fighter &a = F[1];
        if (a.alive && dist2d(p, a.p) < 450.f && p.z > a.p.z && p.z < a.p.z + 1900.f) { hurt_alien(); return; } }
}
static void swipe(Fighter &q) { if (slave_game) net_fire_seq++; q.cool = claw_gap; q.swing = 1.f; q.claw_pend = 0.15f; sfx(3, SND_CLAW, 100); }
static void claw_land(Fighter &q) {
    const Fighter &c = F[0]; if (!c.alive || slave_game) return;   /* a linked slave never decides hits */
    float d = dist2d(q.p, c.p); V to = norm(V(c.p.x - q.p.x, c.p.y - q.p.y, 0.0001f)), fw = fwd_of(q.yaw);
    if (d < claw_reach && dot(to, fw) > 0.5f && has_los(q.p, c.p)) hurt_cyborg(1);
}

/* ---- the computer players: walk the cells by breadth-first search ---- */
static bool path_wp(V from, V to, V &wp) {
    int fc = cellc(from.x), fr = cellr(from.y), tc = cellc(to.x), tr = cellr(to.y);
    wp = to;
    if (solid_c(tc, tr) || solid_c(fc, fr)) return false;
    if (fc == tc && fr == tr) return true;
    int d[GH][GW]; for (int r = 0; r < GH; r++) for (int c = 0; c < GW; c++) d[r][c] = -1;
    static const int dc[4] = {1, -1, 0, 0}, dr[4] = {0, 0, 1, -1};
    std::vector<std::pair<int, int>> q; q.push_back(std::make_pair(tc, tr)); d[tr][tc] = 0;
    for (size_t i = 0; i < q.size(); i++) { int c = q[i].first, r = q[i].second;
        for (int k = 0; k < 4; k++) { int nc = c + dc[k], nr = r + dr[k]; if (solid_c(nc, nr) || d[nr][nc] != -1) continue; d[nr][nc] = d[r][c] + 1; q.push_back(std::make_pair(nc, nr)); } }
    int best = -1, bd = 1 << 30;
    for (int k = 0; k < 4; k++) { int nc = fc + dc[k], nr = fr + dr[k]; if (solid_c(nc, nr) || d[nr][nc] < 0) continue; if (d[nr][nc] < bd) { bd = d[nr][nc]; best = k; } }
    if (best < 0) return false;
    int nc = fc + dc[best], nr = fr + dr[best];
    /* line up with the middle of this cell across the way first, so the body does not catch a corner */
    if (dc[best] != 0 && fabsf(from.y - cy(fr)) > 300.f) { wp = V(from.x, cy(fr), 0); return true; }
    if (dr[best] != 0 && fabsf(from.x - cx(fc)) > 300.f) { wp = V(cx(fc), from.y, 0); return true; }
    wp = V(cx(nc), cy(nr), 0); return true;
}
static void face_toward(Fighter &q, V at, float rate, float dt) {
    float want = atan2f(-(at.x - q.p.x), at.y - q.p.y), dy = want - q.yaw;
    while (dy > 3.14159f) dy -= 6.2831853f; while (dy < -3.14159f) dy += 6.2831853f;
    q.yaw += clampf(dy, -rate * dt, rate * dt);
}
static float yaw_err(const Fighter &q, V at) {
    float dy = atan2f(-(at.x - q.p.x), at.y - q.p.y) - q.yaw;
    while (dy > 3.14159f) dy -= 6.2831853f; while (dy < -3.14159f) dy += 6.2831853f;
    return fabsf(dy);
}
static void move_toward(Fighter &q, V wp, float speed, float dt) {
    face_toward(q, wp, 4.f, dt);
    V to = wp - q.p; to.z = 0; float l = len(to);
    if (l < 60.f) { q.moved = 0; return; }
    V d = to * (1.f / l), fw = fwd_of(q.yaw);
    if (dot(d, fw) < 0.5f) { q.moved = 0; return; }   /* turn on the spot first */
    V np = q.p + fw * (speed * dt); V r = q.p; slide(r, np); q.p.x = r.x; q.p.y = r.y; q.moved = speed;
}
static bool cpu_hold = false;   /* the cyborg CPU is holding the trigger at the console */
static void step_cpu(int i, float dt) {
    Fighter &q = F[i]; Fighter &e = F[1 - i]; if (!q.alive) return;
    q.moved = 0;
    float d = dist2d(q.p, e.p); bool los = e.alive && has_los(q.p, e.p);
    if (q.cyb) {
        if (los && d < 12000.f) {   /* stand and shoot: the arm gun */
            face_toward(q, e.p, 5.f, dt);
            V from = q.p + V(0, 0, 1350), tgt = e.p + V(0, 0, 1000);
            if (q.cool <= 0 && yaw_err(q, e.p) < 0.25f) {
                float err = (0.025f + d / 260000.f) * cpu_skill; V dir = norm(tgt - from + V(rnd(-1, 1), rnd(-1, 1), rnd(-0.5f, 0.5f)) * (err * d));
                shoot(from, dir); }
            return; }
        for (Thing &k : things) if (k.alive && dist2d(q.p, k.p) < 9000.f && has_los(q.p, k.p)) { face_toward(q, k.p, 5.f, dt); if (q.cool <= 0 && yaw_err(q, k.p) < 0.1f) shoot(q.p + V(0, 0, 1400), norm(k.p - (q.p + V(0, 0, 1400)))); return; }
        V tgt = disarmed ? e.p : console_pos;
        if (!disarmed && dist2d(q.p, console_pos) < 1900.f) { cpu_hold = true; face_toward(q, console_pos, 4.f, dt); return; }
        V wp; path_wp(q.p, tgt, wp); move_toward(q, wp, walk_speed, dt);
    } else {
        if (!e.alive) return;
        if (d < claw_reach * 0.85f && los) { face_toward(q, e.p, 5.f, dt); if (q.cool <= 0 && yaw_err(q, e.p) < 0.4f) swipe(q); return; }
        V wp; path_wp(q.p, e.p, wp); move_toward(q, wp, alien_speed, dt);
    }
}

/* ---- the human player: head and hand tracked, thumb walks where you look, trigger fires or swipes ---- */
static void step_human(const VPIX_Input &in, float dt) {
    Fighter &q = F[human]; if (!q.alive) { q.moved = 0; return; }
    V hxy(in.head.p[0], in.head.p[1], 0);
    if (!have_off) { walk_off = V(q.p.x, q.p.y, 0) - hxy; have_off = true; }
    V f = V(in.head.R[1][0], in.head.R[1][1], 0); float fl = len(f); f = fl > 0.01f ? f * (1.f / fl) : fwd_of(q.yaw);
    float sp = q.cyb ? walk_speed : alien_speed;
    q.moved = d1k::desk_walk(in, f, sp, dt, walk_off) ? sp : 0;
    V want(walk_off.x + hxy.x, walk_off.y + hxy.y, 0), r = q.p; slide(r, want);
    walk_off += r - want; q.p.x = r.x; q.p.y = r.y; q.yaw = atan2f(-f.x, f.y);
    bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0;
    if (q.cyb) {
        bool at_console = !disarmed && dist2d(q.p, console_pos) < 1900.f;
        if (trig && at_console) cpu_hold = true;                           /* INVENTED: the trigger works the console when you stand at it */
        else if (trig && q.cool <= 0) {
            V from, dir;
            if (in.hand.valid && !gun_head) { dir = V(in.hand.R[1][0], in.hand.R[1][1], in.hand.R[1][2]);
                from = V(walk_off.x + in.hand.p[0], walk_off.y + in.hand.p[1], in.hand.p[2]) + dir * 300.f; }
            else { dir = V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2]); from = V(walk_off.x + hxy.x, walk_off.y + hxy.y, in.head.p[2] - 250.f) + dir * 300.f; }
            shoot(from, dir); VPIX_Haptic(0, 0.08f, 0.6f);
        }
    } else if (trig && q.cool <= 0) swipe(q);
}

static void place_all() {
    F[0] = Fighter(); F[1] = Fighter(); F[0].cyb = true; F[1].cyb = false;
    F[0].p = start_c; F[0].yaw = -1.5707963f; F[0].hp = cyborg_hp; F[0].cool = 1.f;
    F[1].p = start_a; F[1].yaw = 1.5707963f; F[1].hp = alien_hp; F[1].cool = 1.f;
    sparks.clear(); disarmed = false; disarm_p = 0; have_off = false; cpu_hold = false; res = R_NONE; beep_t = 1.f;
    time_left = game_len; net_epoch++;
    { const int spots[3][2] = {{1, 3}, {6, 1}, {11, 6}}; for (int i = 0; i < 3; i++) { Thing &k = things[i]; k = Thing(); k.p = k.from = k.to = V(cx(spots[i][0]), cy(spots[i][1]), 3300.f); } }
}
static void start_round() {
    place_all(); st = S_PLAY; st_t = 0; sfx(5, SND_GO, 100);
    if (v1knet::on) v1knet::begin_round(2);   /* linked: seat 0 = you, seat 1 = the other fighter (a pod or the computer) */
    dl("t=%.1f round start: you are the %s%s, %.0f s", sim_t, human == 0 ? "cyborg" : "alien", autoplay ? " (AUTO)" : "", game_len);
}
/* ---- link play (vpix_1k_net.h): pod 1 runs the whole game; a second pod takes the OTHER fighter (seat 1), walks and aims it, asks to fire, and draws the master's state. Two seats only, as the original is two players. ---- */
#pragma pack(push, 1)
struct NetIn { float x, y, yaw, moved; uint8_t epoch, hold; uint32_t fire_seq; float from[3], dir[3]; };
struct NetFi { float p[3], yaw, moved, swing, flash; uint8_t alive, hp; };
struct NetTh { float p[3], yaw; uint8_t alive; };
struct NetSt { uint8_t st, res, mside, epoch, disarmed; float st_t, time_left, disarm_p; NetFi f[2]; NetTh th[3]; };
#pragma pack(pop)
static void net3(float *o, V a) { o[0] = a.x; o[1] = a.y; o[2] = a.z; }
static V net3(const float *o) { return V(o[0], o[1], o[2]); }
static uint32_t net_last_fire = 0; static bool net_person = false;
/* master: is fighter i driven by the pod in seat 1? true = handled here (position, console, fire) */
static bool net_drive(int i) {
    if (!v1knet::on || !v1knet::master || i == human) return false;
    const NetIn *p = (const NetIn *)v1knet::seat_input(1, sizeof(NetIn));
    if (!p) { if (net_person) { net_person = false; dl("t=%.1f link: seat 1 back to the computer", sim_t); } return false; }
    Fighter &q = F[i];
    if (!net_person) { net_person = true; net_last_fire = p->fire_seq; dl("t=%.1f link: seat 1 is a person now", sim_t); }
    q.moved = 0; if (!q.alive) return true;
    if (p->epoch == net_epoch) { q.p.x = p->x; q.p.y = p->y; q.yaw = p->yaw; q.moved = p->moved; }
    if (q.cyb && p->hold && !disarmed && dist2d(q.p, console_pos) < 2300.f) cpu_hold = true;
    if (p->fire_seq != net_last_fire) { net_last_fire = p->fire_seq;
        if (q.cool <= 0.15f) { if (q.cyb) shoot(net3(p->from), net3(p->dir)); else swipe(q); } }
    return true;
}
static void step_world(const VPIX_Input &in, float dt) {
    for (int i = 0; i < 2; i++) { Fighter &q = F[i];
        if (q.cool > 0) q.cool -= dt; if (q.flash > 0) q.flash -= dt; if (q.swing > 0) q.swing = (std::max)(0.f, q.swing - dt / 0.35f);
        q.phase += q.moved * dt * 0.0045f;
        if (q.claw_pend >= 0) { q.claw_pend -= dt; if (q.claw_pend < 0) { q.claw_pend = -1; if (q.alive) claw_land(q); } }
        if (!q.alive && !q.cyb && st == S_PLAY && !(disarmed)) { q.dead_t -= dt;
            if (q.dead_t <= 0) { q.alive = true; q.hp = alien_hp; q.p = start_a; q.yaw = 1.5707963f; q.cool = 1.f; net_epoch++; if (human == 1) have_off = false; dl("t=%.1f alien back", sim_t); } }
    }
    if (st != S_PLAY) return;
    for (Thing &k : things) {   /* a crawler scuttles under the ceiling from cell to cell */
        if (!k.alive) { k.back -= dt; if (k.back <= 0) { k.alive = true; k.t = 1; } continue; }
        k.t += dt / 2.6f; if (k.t >= 1.f) { k.from = k.p; const int d4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; for (int tr = 0; tr < 8; tr++) { const int *d = d4[(int)rnd(0, 3.99f)]; int nc = cellc(k.p.x) + d[0], nr = cellr(k.p.y) + d[1]; if (!solid_c(nc, nr)) { k.to = V(cx(nc), cy(nr), 3300.f); break; } } k.t = 0; }
        k.p = k.from + (k.to - k.from) * k.t; k.p.z = 3300.f + sinf((float)sim_t * 5.f) * 120.f; V dd = k.to - k.from; if (len(dd) > 1.f) k.yaw = atan2f(-dd.x, dd.y); }
    cpu_hold = false;
    for (int i = 0; i < 2; i++) { if (net_drive(i)) continue; if (autoplay || i != human) step_cpu(i, dt); else step_human(in, dt); }
    if (st != S_PLAY) return;
    /* the console: INVENTED, trigger held within reach for disarm_time seconds. The lamp goes red, amber, green; nothing else shows it */
    if (!disarmed && cpu_hold && F[0].alive) {
        float was = disarm_p; disarm_p += dt / disarm_time;
        if ((int)(was * 10.f) != (int)(disarm_p * 10.f)) sfx(6, SND_DISARM, 70);
        if (disarm_p >= 1.f) { disarmed = true; sfx(6, SND_DONE, 110); dl("t=%.1f self-destruct disarmed", sim_t); check_win(); }
    }
    if (st != S_PLAY) return;
    time_left -= dt;
    if (!disarmed) { beep_t -= dt; if (beep_t <= 0) { sfx(7, SND_BEEP, 60); beep_t = time_left < 30.f ? 0.5f : time_left < 90.f ? 1.f : 2.f; } }
    if (time_left <= 0) end_round(disarmed ? R_DRAW : R_BOOM);
}

/* ---- drawing ---- */
static M spun(const M &b, float a) {   /* turned about its own up axis */
    M m; m.r[0] = b.r[0] * cosf(a) + b.r[1] * sinf(a); m.r[1] = b.r[0] * (-sinf(a)) + b.r[1] * cosf(a); m.r[2] = b.r[2]; return m;
}
static void put_fighter(VPIX_Frame &f, const Fighter &q) {
    if (!q.alive) return;
    M R = yawed(q.yaw); const float hipz = q.cyb ? 900.f : 800.f; V hip = q.p + V(0, 0, hipz);
    float sw = q.moved > 1.f ? sinf(q.phase) * 0.55f : 0.f;
    dput(f, q.cyb ? M_CY_BODY : M_AL_BODY, R, hip);
    for (int k = 0; k < 2; k++) { float a = k ? -sw : sw; M th = pitched(R, a); dput(f, q.cyb ? M_CY_LEG : M_AL_LEG, th, hip + R.r[0] * (k ? 130.f : -130.f)); }
    if (!q.cyb) {
        float ang = 0.15f + sinf(q.swing * 3.14159f) * 1.5f; M a = spun(pitched(R, ang), (q.swing - 0.5f) * (q.swing > 0 ? 1.2f : 0.f));
        dput(f, M_AL_ARM, a, hip + R.r[0] * 410.f + R.r[1] * 170.f + V(0, 0, 600));
    } else if (q.flash > 0) dput(f, M_STAR, R, hip + R.r[0] * 400.f + R.r[1] * 1050.f + V(0, 0, 450)).bright = 1;
}
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = f.clear[1] = f.clear[2] = 0;
    f.n_lights = 1; f.lights[0] = {0.65f, 0.35f, {-0.218f, -0.437f, 0.874f}};
    bool play = st != S_TITLE;
    static const char *hold = getenv("GRIDWAR_ATTRACT");
    const float tcyc = (hold && *hold) ? (float)atof(hold) : fmodf((float)sim_t, 30.f);
    /* the attract loop in the footage's order: cyborg on the lime ground, pull back, dive into the skull, title; Octavius' capture adds the alien on the sand */
    const int stage = play ? -1 : tcyc < 7.f ? 0 : tcyc < 10.f ? 1 : tcyc < 13.f ? 2 : tcyc < 24.f ? 3 : 4;
    M cam; V eye; bool head_cam = false;
    const Fighter &me = F[human];
    static const char *fixed = getenv("GRIDWAR_CAM");
    if (play) {
        M I; dput(f, M_SHIP, I, V(0, 0, 0)).bright = 1;
        int cs = M_CONS_SAFE; if (!disarmed) { bool on = fmodf((float)sim_t, 0.5f) < 0.25f; cs = disarm_p > 0.5f ? (on ? M_CONS_HALF : M_CONS_OFF) : (on ? M_CONS_ARM : M_CONS_OFF); }
        dput(f, cs, I, console_pos);
        /* the clock in each display box, running the round's time (read 3:26 once at Octavius t840; that it runs is the GUESS) */
        int secs = (int)ceilf((std::max)(st == S_OVER ? 0.f : time_left, 0.f)); int ch[3] = {(secs / 60) % 10, (secs % 60) / 10, secs % 10};
        for (const DoorClock &dc : door_clocks) { M R; R.r[0] = dc.t; R.r[1] = V(0, 0, 0) - dc.n; R.r[2] = V(0, 0, 1);
            dput(f, M_CDIG + ch[0], R, dc.at + dc.t * -360.f).bright = 1; dput(f, M_CCOLON, R, dc.at + dc.t * -190.f).bright = 1; dput(f, M_CDIG + ch[1], R, dc.at + dc.t * 40.f).bright = 1; dput(f, M_CDIG + ch[2], R, dc.at + dc.t * 300.f).bright = 1; }
    }
    if (play) { for (int i = 0; i < 2; i++) if (i != human || (fixed && *fixed)) put_fighter(f, F[i]);
        M I; for (const Spark &k : sparks) dput(f, M_SPARK, I, k.p).bright = 1;
        for (const Thing &k : things) if (k.alive) dput(f, M_THING, spun(pitched(yawed(k.yaw), 0.25f), sinf((float)sim_t * 7.f) * 0.2f), k.p); }
    bool synth = autoplay || !in.head.valid;
    if (fixed && *fixed) { float a[6] = {18000, 6000, 34000, 18000, 12000, 0}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5])); }
    else if (stage == 0 || stage == 1) {   /* cyborg stands on the lime ground; the camera circles low, then pulls back until he is tiny */
        const float t = stage == 0 ? tcyc : 7.f, pull = stage == 1 ? (tcyc - 7.f) / 3.f : 0.f; float a = 0.4f + t * 0.22f, d = 3000.f + pull * pull * 30000.f;
        eye = V(sinf(a) * d, -cosf(a) * d, 700.f + pull * 16000.f); cam = look_at(eye, V(0, 0, 1000.f * (1.f - pull)));
        M I; dput(f, M_GREEN, I, V(0, 0, 0)).bright = 1; const float hipz = 900.f; M R = yawed(0); dput(f, M_CY_BODY, R, V(0, 0, hipz)); for (int k = 0; k < 2; k++) dput(f, M_CY_LEG, R, V(k ? 130.f : -130.f, 0, hipz)); }
    else if (stage == 2) {                 /* the terrain is a skull: the camera dives in close */
        float k = (tcyc - 10.f) / 3.f; eye = V(0, -1500.f - 1100.f * k, -100.f); cam = look_at(eye, V(0, 0, -100.f)); M I; dput(f, M_SKULL, I, V(0, 0, 0)).bright = 1; }
    else if (stage == 3) { eye = V(0, -5050, 150); cam = look_at(eye, V(0, 0, 150)); M I = yawed(sinf((float)sim_t * 0.5f) * 0.12f);   /* sizes solved from t621: word 0.85 of the width, oval 0.42 wide, 0.75 high */
        for (int k = 0; k < 2; k++) { VPIX_Instance &q = dput(f, k ? M_WEB : M_SKULL, I, V(0, 0, 250)); q.bright = 1; q.S[0] = q.S[1] = 1.48f; q.S[2] = 1.47f; } M J; dput(f, M_LETTERS, J, V(0, -500, -470)).bright = 1; }
    else if (stage == 4) {                 /* the alien on tan sand under a black sky: seen low and small, close up to the fangs, then the view lifts away till it is a speck (t820-826) */
        const float u = (tcyc - 24.f) / 6.f, near_u = (std::min)(u / 0.55f, 1.f), far_u = (std::max)((u - 0.55f) / 0.45f, 0.f); const float d = 7000.f - near_u * 4400.f + far_u * far_u * 40000.f, zh = 900.f + far_u * far_u * 22000.f, side_a = 0.9f - near_u * 0.6f;
        eye = V(sinf(side_a) * d, cosf(side_a) * d, zh); cam = look_at(eye, V(0, 0, 800.f * (1.f - far_u)));
        { float rl = 0.26f * (1.f - near_u) + 0.1f * sinf(tcyc * 1.3f), cr = cosf(rl), sr = sinf(rl); V a = cam.r[0], c = cam.r[2]; cam.r[0] = a * cr + c * sr; cam.r[2] = c * cr - a * sr; }   /* the view rolls, the horizon is never level (t822-825) */
        M I; dput(f, M_SAND, I, V(0, 0, 0)).bright = 1; M R; R = yawed(0.f); float sw = sinf((float)sim_t * 4.f) * 0.5f;
        dput(f, M_AL_BODY, R, V(0, 0, 800)); dput(f, M_AL_ARM, pitched(R, 0.1f + sw * 0.4f), R.r[0] * 410.f + R.r[1] * 170.f + V(0, 0, 1300));
        for (int q = 0; q < 2; q++) dput(f, M_AL_LEG, pitched(R, q ? sw : -sw), R.r[0] * (q ? 130.f : -130.f) + V(0, 0, 800)); }
    else if (synth) { eye = me.p + V(0, 0, 1700); cam = look_at(eye, eye + fwd_of(me.yaw) * 1000.f); }
    else { for (int k = 0; k < 3; k++) cam.r[k] = V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2]);
        eye = V(walk_off.x + in.head.p[0], walk_off.y + in.head.p[1], in.head.p[2]); head_cam = true; }
    static const bool gunview = getenv("GRIDWAR_GUNVIEW") != nullptr;
    if (play && (!(fixed && *fixed) || gunview) && me.alive && (st == S_PLAY || st == S_OVER)) {
        if (me.cyb) {   /* the grey gun at the bottom centre; the tracked hand if there is one */
            M g = cam; V gp = eye + cam.r[1] * 550.f - cam.r[2] * 300.f;
            if (head_cam && in.hand.valid && !gun_head) { for (int k = 0; k < 3; k++) g.r[k] = V(in.hand.R[k][0], in.hand.R[k][1], in.hand.R[k][2]);
                gp = V(walk_off.x + in.hand.p[0], walk_off.y + in.hand.p[1], in.hand.p[2]); }
            dput(f, M_GUN, g, gp).on_top = 2;
            if (me.flash > 0) { VPIX_Instance &s = dput(f, M_STAR, g, gp + g.r[1] * 1040.f + g.r[2] * 14.f - g.r[0] * 28.f); s.on_top = 2; s.bright = 1; }
        } else {        /* the alien's claw arm, bottom right */
            float ang = 1.15f + sinf(me.swing * 3.14159f) * 0.5f; M c = spun(pitched(cam, ang), (me.swing - 0.5f) * (me.swing > 0 ? 1.4f : 0.f));
            dput(f, M_AL_ARM, c, eye + cam.r[0] * 420.f + cam.r[1] * 480.f - cam.r[2] * 320.f).on_top = 2;
        }
    }
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    f.in_head = head_cam; if (head_cam) f.head_fed = in.head;
    if (res == R_BOOM && st == S_OVER) { float k = clampf(st_t / 1.2f, 0.f, 1.f); f.clear[0] = f.clear[1] = f.clear[2] = (uint8_t)(255.f * k); }

    const float CX = 176.f;   /* no HUD in play; only the title card and the end of the round have words */
    if (!play) {
        text(f, side == 0 ? "YOU ARE THE CYBORG" : "YOU ARE THE ALIEN", CX, 262, 0.65f, 1);
        if (fmodf((float)sim_t, 1.2f) < 0.8f) text(f, "PRESS TRIGGER", CX, 274, 0.7f);
    } else if (st == S_OVER && st_t > 0.6f) {
        const char *a = "", *b = "";
        switch (res) {
        case R_CYBORG: a = "CYBORG WINS"; b = "SELF DESTRUCT OFF AND ALIEN DOWN"; break;
        case R_ALIEN: a = "ALIEN WINS"; b = "CYBORG DOWN"; break;
        case R_BOOM: a = "ALIEN WINS"; b = "SHIP DESTROYED"; break;
        default: a = "TIME UP"; b = "ALIEN LIVES AND THE SHIP IS SAVED"; break; }
        text(f, a, CX, 90, 1.6f, 1); text(f, b, CX, 128, 0.7f);
        bool won = (res == R_CYBORG && human == 0) || ((res == R_ALIEN || res == R_BOOM) && human == 1);
        if (res != R_DRAW) text(f, won ? "YOU WIN" : "YOU LOSE", CX, 150, 1.f, 2);
    }
    d1k::hs::draw(f, "gridwar", st == S_TITLE, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at game_fps ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 2; static unsigned long frames = 0;
/* ---- link play, the net half (the structs and net_drive are above step_world) ---- */
static void net_log() {
    static double next = 0; if (sim_t < next) return; next = sim_t + 3.0;
    if (st == S_TITLE && !slave_game) { dl("t=%.1f link: %s, title, pods heard %d", sim_t, v1knet::master ? "master" : "slave", v1knet::live_pods()); return; }
    dl("t=%.1f link: %s you are the %s st %d left %.0f | cyborg %s(%.0f %.0f) hp %d %d | alien %s(%.0f %.0f) hp %d %d | disarm %.2f", sim_t, v1knet::master ? "master" : "slave", human == 0 ? "cyborg" : "alien", (int)st, time_left,
       v1knet::master && human != 0 && !net_person ? "cpu " : "", F[0].p.x, F[0].p.y, F[0].hp, F[0].alive ? 1 : 0, v1knet::master && human == 0 && !net_person ? "cpu " : "", F[1].p.x, F[1].p.y, F[1].hp, F[1].alive ? 1 : 0, disarm_p);
}
static void net_master() {
    if (!v1knet::due(30)) return;
    if (st == S_TITLE) { v1knet::send_snapshot(NULL, 0); return; }
    static NetSt s; memset(&s, 0, sizeof(s)); s.st = (uint8_t)st; s.res = (uint8_t)res; s.mside = (uint8_t)human; s.epoch = net_epoch; s.disarmed = disarmed ? 1 : 0; s.st_t = st_t; s.time_left = time_left; s.disarm_p = disarm_p;
    for (int i = 0; i < 2; i++) { const Fighter &q = F[i]; NetFi &n = s.f[i]; net3(n.p, q.p); n.yaw = q.yaw; n.moved = q.moved; n.swing = q.swing; n.flash = q.flash; n.alive = q.alive ? 1 : 0; n.hp = (uint8_t)(std::max)(q.hp, 0); }
    for (int k = 0; k < 3; k++) { net3(s.th[k].p, things[k].p); s.th[k].yaw = things[k].yaw; s.th[k].alive = things[k].alive ? 1 : 0; }
    v1knet::send_snapshot(&s, sizeof(s));
}
static void net_leave(const char *why) { slave_game = false; st = S_TITLE; st_t = 0; res = R_NONE; human = side; sparks.clear(); have_off = false; F[0] = Fighter(); F[1] = Fighter(); dl("t=%.1f link: %s, back to the title", sim_t, why); }
static void net_apply(const NetSt &s, uint8_t round) {
    static uint8_t my_round = 0; const bool fresh = !slave_game || round != my_round; my_round = round;
    if (fresh) { human = s.mside ? 0 : 1; place_all(); sparks.clear(); slave_game = true; net_fire_seq = 0; sfx(5, SND_GO, 100); dl("t=%.1f link: in the master's round, you are the %s", sim_t, human == 0 ? "cyborg" : "alien"); }
    for (int i = 0; i < 2; i++) { Fighter &q = F[i]; const NetFi &n = s.f[i]; const bool own = i == human, alive = n.alive != 0;
        if (!fresh) {
            if (n.hp < q.hp) { if (i == 0) { sfx(1, SND_HURT, 127); if (own) VPIX_Haptic(0, 0.25f, 1.f); } else sfx(2, SND_ROAR, 100); burst(q.p + V(0, 0, i ? 1100 : 1200), 12, 2000.f, 0.8f); }
            if (q.alive && !alive) burst(q.p + V(0, 0, 1000), 40, 3200.f, 1.6f);
            if (!own && i == 0 && n.flash > q.flash + 0.1f) sfx(0, SND_FIRE, 70);
            if (!own && i == 1 && n.swing > q.swing + 0.5f) sfx(3, SND_CLAW, 70);
        }
        const bool keep = !fresh && own && q.alive && alive && net_epoch == s.epoch && (have_off || autoplay);   /* you lead your own walking */
        if (!keep) { q.p = net3(n.p); q.yaw = n.yaw; if (own) have_off = false; }
        if (!own) { q.p = net3(n.p); q.yaw = n.yaw; q.moved = n.moved; q.swing = n.swing; q.flash = n.flash; }
        q.alive = alive; q.hp = n.hp; q.cyb = i == 0; }
    net_epoch = s.epoch;
    if (!fresh && s.disarmed && !disarmed) sfx(6, SND_DONE, 110);
    if (!fresh && (int)(disarm_p * 10.f) != (int)(s.disarm_p * 10.f) && !s.disarmed) sfx(6, SND_DISARM, 70);
    if (!fresh && s.st == S_OVER && st != S_OVER && s.res == R_BOOM) sfx(4, SND_BOOM, 127);
    disarmed = s.disarmed != 0; disarm_p = s.disarm_p; res = (Result)s.res; st = (State)s.st; st_t = s.st_t; time_left = s.time_left;
    for (int k = 0; k < 3; k++) { things[k].p = net3(s.th[k].p); things[k].yaw = s.th[k].yaw; things[k].alive = s.th[k].alive != 0; }
}
static void net_slave_step(const VPIX_Input &in, float dt) {
    for (int i = 0; i < 2; i++) { Fighter &q = F[i]; if (q.cool > 0) q.cool -= dt; if (q.flash > 0) q.flash -= dt; if (q.swing > 0) q.swing = (std::max)(0.f, q.swing - dt / 0.35f); q.phase += q.moved * dt * 0.0045f; q.claw_pend = -1; }
    cpu_hold = false;
    if (st == S_PLAY) {
        if (autoplay) step_cpu(human, dt); else step_human(in, dt);
        if (!disarmed) { beep_t -= dt; if (beep_t <= 0) { sfx(7, SND_BEEP, 60); beep_t = time_left < 30.f ? 0.5f : time_left < 90.f ? 1.f : 2.f; } }
        time_left -= dt; }
    for (size_t i = 0; i < sparks.size();) { Spark &k = sparks[i]; k.v.z -= 4900.f * dt; k.p += k.v * dt; k.life -= dt;
        if (k.life <= 0 || k.p.z < 0) { sparks[i] = sparks.back(); sparks.pop_back(); } else i++; }
}
/* true = this tick was a linked slave's, inside the master's game */
static bool net_tick(double el, const VPIX_Input &in) {
    (void)el; v1knet::pump();
    if (v1knet::master) { net_master(); net_log(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n >= sizeof(NetSt) && v1knet::my_seat >= 0) { static NetSt s; memcpy(&s, d, sizeof(s)); net_apply(s, v1knet::round_id); }
        else if (slave_game) net_leave("the master's round is over");
    }
    if (slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
    net_log();
    if (!slave_game) { v1knet::send_input(NULL, 0); return false; }
    NetIn o; memset(&o, 0, sizeof(o)); const Fighter &q = F[human];
    o.x = q.p.x; o.y = q.p.y; o.yaw = q.yaw; o.moved = q.moved; o.epoch = net_epoch; o.hold = cpu_hold ? 1 : 0; o.fire_seq = net_fire_seq; net3(o.from, net_fire_from); net3(o.dir, net_fire_dir);
    v1knet::send_input(&o, sizeof(o));
    const float dt = 1.f / 120.f; int k = 0;
    while (acc >= dt && k < 24) { net_slave_step(in, dt); acc -= dt; sim_t += dt; st_t += dt; k++; }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    return true;
}
static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    static bool trig_was = false, start_was = false;
    bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.start, sel = trig && !trig_was; trig_was = trig; (void)start_was;
    static const bool att_hold = getenv("GRIDWAR_ATTRACT") != nullptr;
    if (autoplay && st != S_PLAY && st_t > 2.f && !att_hold) sel = true;
    if (st == S_OVER && st_t > 15.f) sel = true; if (d1k::hs::run("gridwar", st == S_OVER && st_t > 2.f, (res == R_CYBORG ? 5000 : 0) + (disarmed ? 1000 : 0) + (int)(std::max)(time_left, 0.f) * 10, in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (!v1knet::can_start()) sel = false;   /* a linked slave: pod 1 starts the game */
    if (v1knet::on && net_tick(el, in)) { PIC_AddEvent(tick, 2.0); return; }
    if (st == S_TITLE && sel) start_round();
    else if (st == S_OVER && sel && st_t > 3.f) { st = S_TITLE; st_t = 0; res = R_NONE; sparks.clear(); v1knet::end_round(); }
    int n = 0;
    while (acc >= dt && n < 24) {
        step_world(in, dt);
        for (size_t i = 0; i < sparks.size();) { Spark &k = sparks[i]; k.v.z -= 4900.f * dt; k.p += k.v * dt; k.life -= dt;
            if (k.life <= 0 || k.p.z < 0) { sparks[i] = sparks.back(); sparks.pop_back(); } else i++; }
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    if (sim_t >= next_log) { next_log = sim_t + 2.0;
        dl("t=%.0f st %d cyborg (%.0f %.0f) hp %d alive %d | alien (%.0f %.0f) hp %d alive %d | disarm %.2f %d | left %.0f",
           sim_t, (int)st, F[0].p.x, F[0].p.y, F[0].hp, F[0].alive ? 1 : 0, F[1].p.x, F[1].p.y, F[1].hp, F[1].alive ? 1 : 0, disarm_p, disarmed ? 1 : 0, time_left); }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".gridwar").c_str(), "w");
    walk_speed = envf("GRIDWAR_WALK", walk_speed); alien_speed = envf("GRIDWAR_ALIEN_WALK", alien_speed); game_len = envf("GRIDWAR_TIME", game_len);
    game_fps = envf("GRIDWAR_FPS", game_fps); gun_gap = envf("GRIDWAR_GUN_GAP", gun_gap); claw_gap = envf("GRIDWAR_CLAW_GAP", claw_gap);
    claw_reach = envf("GRIDWAR_REACH", claw_reach); disarm_time = envf("GRIDWAR_DISARM", disarm_time);
    cyborg_hp = (int)envf("GRIDWAR_CYBORG_HP", (float)cyborg_hp); alien_hp = (int)envf("GRIDWAR_ALIEN_HP", (float)alien_hp);
    alien_respawn = envf("GRIDWAR_ALIEN_RESPAWN", alien_respawn); cpu_skill = envf("GRIDWAR_SKILL", cpu_skill);
    gun_head = envf("GRIDWAR_GUNHEAD", 0) != 0; side = envf("GRIDWAR_SIDE", 0) != 0 ? 1 : 0; human = side; autoplay = envf("GRIDWAR_AUTO", 0) != 0;
    if (disarm_time < 0.5f) disarm_time = 0.5f; if (game_fps < 1.f) game_fps = 1.f;
    v1knet::init("GRWR", false, dl);   /* start-together, two seats: cyborg and alien */
    d1k::make_hud(); build_meshes(); make_sounds(); place_all();
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    PIC_AddEvent(tick, 2.0);
    dl("Grid Warriors rebuild started, side %d%s", side, autoplay ? ", auto" : "");
}
}   /* namespace gridwar */
