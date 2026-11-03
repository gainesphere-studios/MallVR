/* Legend Quest (Virtuality 1000CS, 1992), the short pod version - native build from footage and press. No game file is read, every model is built here.
   Included after vpix_dactyl1k.h (it reuses the d1k kit). Dev only.
   Evidence: Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\legend.md, with sources and timestamps (t = seconds in BBC Tomorrow's World, 20 Mar 1992, youtu.be/DD4qoif7lvY).
   FROM FOOTAGE: castle room (pale slab floor, steps, arched plank door with a mint strip at its foot, torch, dark right-hand wall, big pale double door on the left opening inward on green, red-brown trees and sky);
   the party walking in one by one through the big door (elf first); hall (salmon-orange wall, mauve band, plate door standing open on your left as you enter, low table, two bone piles, chest on the right with a flat dark lid and a grey-framed front, torch, arched door);
   skeleton from the right, the elf helping (speech); red flash on a hit in the same room, then a dim grey spell (the red "room" of earlier passes was this flash); dense yellow and red sparks settling to white dots; bones, skull, shield and sword left behind;
   open book when the chest is used, lid shut again by t203; flame on the palm, fireball, ragged hole in the door, door swinging open; grey-brown corridor, cloud of black specks and gull-winged bats; mitten hand in a long crimson sleeve; wizard's dagger, elf's long thin sword, dwarf's axe; about 6 pictures a second with the party in view.
   FROM PRESS PICTURES (Amiga Action 31 p.2, Games-X 44 p.12, Tilt 103 p.41): blue wolf with pink ears on green, red ogre with dark eye band, tusks and the key, black room with a winding yellow path and arched dark door, long green scaled thing, bow.
   FROM PRESS TEXT: blade from the belt, bow from the shoulder (GamePro 40, Tilt 103, Amiga Action 31, Games-X 44); lunge with the trigger (Mean Machines Sega 11); skeletons rising from the floor (Amiga Computing 69); ogre drops a key (Tilt 103); sound falling with distance (Micro User 110, Tilt 103); spider Eric, about as tall as a dwarf (speech); leaflet names on the title.
   FROM BOBBY'S MEMORY (no footage shows it, no press describes it): a WORKING mirror that shows you and the room (Bobby 11-03; earlier 10-31 "a mirror you can play in front of"). Period text backs a mirror only as a way on (Australian Commodore Review Apr 1993).
     His uncertain placement ("i forget"): just inside, on the wall right of the doorway ahead. BBC t87-88 shows that stretch of wall bare, full height, so the mirror is on the next surface along, the dark right-hand wall, near the door, which no frame shows below the torch. PLACEMENT INVENTED.
     Technique INVENTED: a reflected copy of the scene behind an opening in the wall. How the real machine did it is unknown. His skeleton "from a dark corner to the right" (with a question mark): the skeleton enters from the right of the hall (t169, speech), the dark corner is not in any frame.
   INFERRED: how long the red flash and grey dim last; lid open before the fight; "plants" in speech pictured as specks and bats.
   INVENTED, NO SOURCE: order and links of everything after the corridor (the yellow path's place, ogre room, trap room, lair and its mirror, the sixty rooms ASM 7/93 mentions are not built); all sizes, hp, damage, speeds, timings; every sound; goblin's look (pale creature from Amiga Action, unconfirmed as a goblin) and places; what the green thing does; spider's look; spell list (fireball only); title layout; ending; three allies stand in for the three other players; R or fourth button cycles weapons when no hand is tracked; a tracked player starts empty handed, a player with no tracked hand starts with the blade.
   Units: mm, seconds. World X right, Y forward, Z up. Camera is the head pose (in_head), so VR late correction applies.
   Env: LEGEND_WALK LEGEND_TIME LEGEND_FPS LEGEND_FPS_BUSY LEGEND_TASK LEGEND_DIM LEGEND_HP LEGEND_VANISH LEGEND_CRACK LEGEND_SWING LEGEND_MONHP LEGEND_DMG LEGEND_CAST LEGEND_REACH LEGEND_BLOCK
        LEGEND_AUTO LEGEND_IDLE(yaw) LEGEND_CAM(x,y,z,tx,ty,tz) LEGEND_STAGE LEGEND_STAGE_LOOK */
#include <functional>
#include "vpix_1k_net.h"         /* link play, 2-4 pods: see the "link play" block before the loop */
namespace legend {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at;
using d1k::Mesh; using d1k::dput; using d1k::text; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound; using d1k::yawed; using d1k::pitched; using d1k::add_bitmap;

static const float LPI = 3.14159265f;

/* ---- tuning ---- */
static float walk_speed = 1400.f;     /* INVENTED: footage only shows a walking pace */
static float game_fps = 10.f;         /* measured on the clean BBC copy (DD4qoif7lvY): 4-5 frame gaps at 25 fps with the party in view = 5-6 a second; about 10 on the other tapes (08 section 2) */
static float busy_fps = 6.f;          /* three or more figures near you: the rate above */
static float game_len = 180.f;        /* base 3 minutes, more for finishing tasks (bambi spec card, secondary; no first-hand copy) */
static float task_bonus = 30.f;       /* INVENTED: how much a task adds */
static float hp_max = 100.f;          /* INVENTED: no health numbers known */
static float vanish_t = 5.f;          /* skeleton gone, gold sparkles, in about 5 s (Tomorrow's World 190-197 s) */
static float crack_t = 2.f;           /* fireball to ragged hole: 1.5-3 s (frames 223-226 s) */
static float swing_min = 1800.f;      /* INVENTED: tip speed in mm/s that counts as a real swing */
static float mon_hp_scale = 1.f;      /* INVENTED: monster health multiplier */
static float dmg_scale = 1.f;         /* INVENTED: damage the monsters do to you */
static float cast_gap = 1.4f;         /* INVENTED: fireball recharge */
static float dim_t = 3.5f;            /* INFERRED from two clips: after the red flash the first-person view stays grey for seconds (BBC t170-172, Bad Influence Y7mtN7Sxk-E t41-43) */
static float reach_r = 650.f;         /* INVENTED: how close the hand must be to open / take something */
static float block_k = 0.15f;         /* INVENTED: damage left when you block */
/* the mirrors. Each is an opening in its wall with a reflected copy of the room drawn behind it */
static const float MIRROR_X = 3300.f, MIRROR_Y = 2150.f, MIRROR_HW = 750.f, MIRROR_H = 2900.f, LAIR_Y = 52300.f;
static const float Z1 = -2600.f;      /* the floor below the castle room (INVENTED depth of the stair) */
static const float E = 300.f;         /* wall plane sits this far outside the walkable rectangle */

/* ---- ids ---- */
enum { L_FLAME = 2000, L_GOLD, L_REDSP, L_SWORD1, L_SWORD2, L_BALL, L_BOOK, L_KEY, L_LID, L_BIGDOOR, L_DOOR1, L_DOOR1B, L_DOOR1C, L_KEYDOOR,
       L_TILE_S, L_TILE_T, L_MIRROR, L_MIRROR_G, L_WALLE, L_WALLN, L_SKEL, L_SKEL_LEG, L_BAT, L_BAT_WL, L_BAT_WR, L_GOB, L_GOB_LEG, L_WOLF, L_WOLF_LEG,
       L_OGRE, L_OGRE_LEG, L_SPIDER, L_DOT, L_BLK, L_HAND, L_BOW, L_ARROW, L_BONES, L_SERP, L_BOOKV, L_FLAMEH, L_DOOR2, L_AV = 2080 /* + 0..3 bodies, 4 legs, 5 short legs */, L_ROOM = 2100 /* + 0..6 */ };
enum { BM_RED = 400, BM_WHITE = 401, BM_DIM = 402 };
enum { SN_SWING = 0x7E000001, SN_HIT, SN_CLANG, SN_RATTLE, SN_FIRE, SN_BOOM, SN_CREAK, SN_CHIME, SN_SQUEAK, SN_GROWL, SN_ROAR, SN_KEY, SN_CLATTER, SN_HISS, SN_HURT, SN_WIN, SN_START, SN_THUD };

/* ---- mesh helpers (all flat polygons built in code) ---- */
static void polyraw(Mesh &m, const std::vector<V> &c, uint32_t rgb) {
    VPIX_MeshPoly q; q.flags = 0x20000000u | ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | (rgb >> 16);   /* flat colours are stored BGR */
    int n = (int)(c.size() < 16 ? c.size() : 16); for (int i = 0; i < n; i++) q.idx[i] = m.vert(c[i]);
    q.n = (uint8_t)n; V nn = norm(cross(c[1] - c[0], c[2] - c[0])); q.nx = nn.x; q.ny = nn.y; q.nz = nn.z; m.p.push_back(q);
}
/* one polygon wound so its front faces toward ref (toward = true) or away from it */
static void polyv(Mesh &m, std::vector<V> c, uint32_t rgb, V ref, bool toward = true) {
    V n = cross(c[1] - c[0], c[2] - c[0]);
    if ((dot(n, ref - c[0]) > 0) != toward) std::reverse(c.begin(), c.end());
    polyraw(m, c, rgb);
}
static void dbl(Mesh &m, std::vector<V> c, uint32_t rgb) { polyraw(m, c, rgb); std::reverse(c.begin(), c.end()); polyraw(m, c, rgb); }   /* both sides */
static void fq(Mesh &m, V a, V b, V c, V d, uint32_t col, V ref, bool toward = true) { polyv(m, {a, b, c, d}, col, ref, toward); }
static void limb(Mesh &m, V a, V b, float w, uint32_t col) {   /* a square prism from a to b */
    V d = norm(b - a), up = fabsf(d.z) > 0.9f ? V(1, 0, 0) : V(0, 0, 1);
    V u = norm(cross(d, up)) * w, v = norm(cross(d, u)) * w, c = (a + b) * 0.5f;
    V p[8] = {a - u - v, a + u - v, a + u + v, a - u + v, b - u - v, b + u - v, b + u + v, b - u + v};
    static const int F[6][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
    for (int k = 0; k < 6; k++) fq(m, p[F[k][0]], p[F[k][1]], p[F[k][2]], p[F[k][3]], col, c, false);
}
static void ngon(Mesh &m, V c, V ax, V ay, float rx, float ry, int n, uint32_t col, V ref, bool toward = true) {
    std::vector<V> p; for (int i = 0; i < n; i++) { float a = 2.f * LPI * i / n; p.push_back(c + ax * (cosf(a) * rx) + ay * (sinf(a) * ry)); }
    polyv(m, p, col, ref, toward);
}
static void gem(Mesh &m, V c, float rx, float ry, float rz, uint32_t col, uint32_t col2) {   /* a six-sided double pyramid */
    const int N = 6;
    for (int i = 0; i < N; i++) { float a = 2.f * LPI * i / N, b = 2.f * LPI * (i + 1) / N;
        V p(c.x + cosf(a) * rx, c.y + sinf(a) * ry, c.z), q(c.x + cosf(b) * rx, c.y + sinf(b) * ry, c.z);
        polyv(m, {p, q, V(c.x, c.y, c.z + rz)}, col, c, false); polyv(m, {q, p, V(c.x, c.y, c.z - rz)}, col2, c, false); }
}
static void cone(Mesh &m, V base, float r, float h, uint32_t col, int N = 6) {
    V top = base + V(0, 0, h), ref = base + V(0, 0, h * 0.4f);
    for (int i = 0; i < N; i++) { float a = 2.f * LPI * i / N, b = 2.f * LPI * (i + 1) / N;
        polyv(m, {base + V(cosf(a) * r, sinf(a) * r, 0), base + V(cosf(b) * r, sinf(b) * r, 0), top}, col, ref, false); }
}

/* a body part as a stack of rings: {z, half width, half depth, y of the ring's middle}, eight flat sides */
struct Rg { float z, rx, ry, dy; };
static void stackp(Mesh &m, V c, const std::vector<Rg> &R, uint32_t col, uint32_t col2 = 0, bool caps = true) {
    const int N = 8; if (!col2) col2 = col;
    auto P = [&](const Rg &g, int i) { float a = 2.f * LPI * (i + 0.5f) / N; return c + V(cosf(a) * g.rx, g.dy + sinf(a) * g.ry, g.z); };
    for (size_t k = 0; k + 1 < R.size(); k++) for (int i = 0; i < N; i++) { V ref = c + V(0, (R[k].dy + R[k + 1].dy) * 0.5f, (R[k].z + R[k + 1].z) * 0.5f); fq(m, P(R[k], i), P(R[k], i + 1), P(R[k + 1], i + 1), P(R[k + 1], i), (i % 2) ? col : col2, ref, false); }
    if (caps) { std::vector<V> a, b; for (int i = 0; i < N; i++) { a.push_back(P(R.front(), i)); b.push_back(P(R.back(), i)); } polyv(m, a, col2, c + V(0, R.front().dy, R.front().z + 1000.f), false); polyv(m, b, col, c + V(0, R.back().dy, R.back().z - 1000.f), false); }
}
static void tube(Mesh &m, V a, V b, float ra, float rb, uint32_t col, uint32_t col2 = 0) {   /* a six-sided taper */
    if (!col2) col2 = col; V d = norm(b - a), up = fabsf(d.z) > 0.9f ? V(1, 0, 0) : V(0, 0, 1), u = norm(cross(d, up)), v = norm(cross(d, u)), mid = (a + b) * 0.5f;
    for (int i = 0; i < 6; i++) { float t0 = LPI * i / 3.f, t1 = LPI * (i + 1) / 3.f; V e0 = u * cosf(t0) + v * sinf(t0), e1 = u * cosf(t1) + v * sinf(t1); fq(m, a + e0 * ra, a + e1 * ra, b + e1 * rb, b + e0 * rb, (i & 1) ? col : col2, mid, false); }
    std::vector<V> ca, cb; for (int i = 0; i < 6; i++) { float t0 = LPI * i / 3.f; V e0 = u * cosf(t0) + v * sinf(t0); ca.push_back(a + e0 * ra); cb.push_back(b + e0 * rb); } polyv(m, ca, col2, mid, false); polyv(m, cb, col, mid, false);
}
/* the round dark shield with a star of spokes (dwarf t150, skeleton t187): centre c, face toward n */
static void star_shield(Mesh &m, V c, V n, float R) {
    n = norm(n); V u = norm(cross(n, V(0, 0, 1))), v = norm(cross(n, u)); const int NS = 10; V ref = c + n * 1000.f, rb = c - n * 1000.f;
    for (int i = 0; i < NS; i++) { float a = 2.f * LPI * i / NS, b = 2.f * LPI * (i + 1) / NS; V p0 = c + u * (cosf(a) * R) + v * (sinf(a) * R), p1 = c + u * (cosf(b) * R) + v * (sinf(b) * R);
        polyv(m, {c, p0, p1}, (i & 1) ? 0x5E6672 : 0x2A2C36, ref); polyv(m, {c - n * 8.f, p0 - n * 8.f, p1 - n * 8.f}, 0x241E1A, rb); }
    ngon(m, c + n * 12.f, u, v, R * 0.13f, R * 0.13f, 6, 0x7A7E86, ref);
}
/* a drawn face on the front of a head (BBC t120, t125, t134): small narrow eyes, thin level brows, a wedge of nose, a thin dark mouth */
static void face(Mesh &m, V c, float y, float w, uint32_t brow, uint32_t mouth) {
    V ref = c + V(0, 1000, 0); auto q = [&](float x0, float z0, float x1, float z1, uint32_t col, float dy = 0.f) { fq(m, c + V(x0, y + dy, z0), c + V(x1, y + dy, z0), c + V(x1, y + dy, z1), c + V(x0, y + dy, z1), col, ref); };
    for (int k = -1; k <= 1; k += 2) { float s = (float)k; q(s > 0 ? w * 0.18f : -w * 0.66f, 16, s > 0 ? w * 0.66f : -w * 0.18f, 38, 0xF0E4D0); q(s * w * 0.42f - 10.f, 16, s * w * 0.42f + 10.f, 38, 0x18100C, 2.f);
        polyv(m, {c + V(s * w * 0.12f, y, 50), c + V(s * w * 0.74f, y, 56), c + V(s * w * 0.74f, y, 68), c + V(s * w * 0.12f, y, 62)}, brow, ref); }
    polyv(m, {c + V(-w * 0.10f, y, -38), c + V(w * 0.13f, y, -38), c + V(w * 0.02f, y + 22.f, 30)}, 0xB86A44, ref);
    q(-w * 0.32f, -90, w * 0.32f, -72, mouth);
}

/* ---- rooms ---- */
struct Room { float x0, y0, x1, y1, z, H; };
static Room RM[7];
struct Gap { bool on = false, skip = false; float c = 0, hw = 800.f, dh = 3000.f; };
static void gap(Gap &g, float c, float dh = 3000.f) { g.on = true; g.c = c; g.dh = dh; }
static void wall_seg(Mesh &m, int side, float plane, float a, float b, float z0, float z1, uint32_t col) {
    if (b - a < 1.f || z1 - z0 < 1.f) return;
    const float T = 250.f; V lo, hi;
    if (side == 0) { lo = V(a, plane - T, z0); hi = V(b, plane, z1); }
    else if (side == 1) { lo = V(a, plane, z0); hi = V(b, plane + T, z1); }
    else if (side == 2) { lo = V(plane - T, a, z0); hi = V(plane, b, z1); }
    else { lo = V(plane, a, z0); hi = V(plane + T, b, z1); }
    m.box(lo, hi, col);
}
/* walls (sides 0 -y, 1 +y, 2 -x, 3 +x) with an optional doorway in each, and the ceiling */
static void shell(Mesh &m, const Room &r, Gap g[4], uint32_t wallc, uint32_t ceilc, float zb) {
    const float X0 = r.x0 - E, X1 = r.x1 + E, Y0 = r.y0 - E, Y1 = r.y1 + E, T = 250.f, top = r.z + r.H;
    for (int s = 0; s < 4; s++) {
        if (g[s].skip) continue;
        float plane = s == 0 ? Y0 : s == 1 ? Y1 : s == 2 ? X0 : X1;
        float a0 = s < 2 ? X0 - T : Y0, a1 = s < 2 ? X1 + T : Y1;
        if (!g[s].on) { wall_seg(m, s, plane, a0, a1, zb, top, wallc); continue; }
        const Gap &q = g[s];
        wall_seg(m, s, plane, a0, q.c - q.hw, zb, top, wallc);
        wall_seg(m, s, plane, q.c + q.hw, a1, zb, top, wallc);
        wall_seg(m, s, plane, q.c - q.hw, q.c + q.hw, r.z + q.dh, top, wallc);
        if (zb < r.z - 1.f) wall_seg(m, s, plane, q.c - q.hw, q.c + q.hw, zb, r.z, wallc);
    }
    V mid((X0 + X1) * 0.5f, (Y0 + Y1) * 0.5f, r.z + r.H * 0.5f);
    fq(m, V(X0, Y0, top), V(X1, Y0, top), V(X1, Y1, top), V(X0, Y1, top), ceilc, mid);
}
/* a door in a stone arch set in a flat recess. The opening is 1560 wide, its arc springs at 2500. Everything sits in front of the wall at y */
static void arch_surround(Mesh &m, float y, float z, float RX, float RZ, uint32_t rec, uint32_t back, V rf, uint32_t s1, uint32_t s2) {
    const int NB = 6, NA = 14; const float S = 2500.f;
    for (int k = -1; k <= 1; k += 2) { for (int i = 0; i < NB; i++) { float z0 = z + i * (S / NB), z1 = z0 + S / NB; m.box(V(k > 0 ? 780.f : -1020.f, y - 15.f, z0), V(k > 0 ? 1020.f : -780.f, y + 170.f, z1), (i & 1) ? s1 : s2); }
        float x0 = k > 0 ? 1020.f : -RX, x1 = k > 0 ? RX : -1020.f; fq(m, V(x0, y, z), V(x1, y, z), V(x1, y, z + S), V(x0, y, z + S), rec, rf); }
    V c(0, y, z + S); auto on_rect = [&](float a) { float ca = cosf(a), sa = sinf(a), t = 1e9f; if (fabsf(ca) > 1e-3f) t = (std::min)(t, RX / fabsf(ca)); if (sa > 1e-3f) t = (std::min)(t, (RZ - S) / sa); return c + V(ca * t, 0, sa * t); };
    for (int i = 0; i < NA; i++) { float a = LPI * i / NA, b = LPI * (i + 1) / NA; V ca(cosf(a), 0, sinf(a)), cb(cosf(b), 0, sinf(b)), f0(1020.f, 0, 760.f), n0(780.f, 0, 520.f);
        V ao = c + V(ca.x * f0.x, 0, ca.z * f0.z), bo = c + V(cb.x * f0.x, 0, cb.z * f0.z), ai = c + V(ca.x * n0.x, 0, ca.z * n0.z), bi = c + V(cb.x * n0.x, 0, cb.z * n0.z);
        fq(m, ao, bo, on_rect(b), on_rect(a), rec, rf); fq(m, ai + V(0, -4, 0), bi + V(0, -4, 0), bo + V(0, -4, 0), ao + V(0, -4, 0), (i / 2 & 1) ? s1 : s2, rf); polyv(m, {c + V(0, 30, 0), ai + V(0, 30, 0), bi + V(0, 30, 0)}, back, rf); }
}
static void floor_checker(Mesh &m, const Room &r, uint32_t c1, uint32_t c2, float tile) {
    const float X0 = r.x0 - E, X1 = r.x1 + E, Y0 = r.y0 - E, Y1 = r.y1 + E; int i = 0;
    for (float x = X0; x < X1 - 1.f; x += tile, i++) { int j = 0;
        for (float y = Y0; y < Y1 - 1.f; y += tile, j++) { float xe = (std::min)(x + tile, X1), ye = (std::min)(y + tile, Y1);
            fq(m, V(x, y, r.z), V(xe, y, r.z), V(xe, ye, r.z), V(x, ye, r.z), ((i + j) & 1) ? c2 : c1, V(x, y, r.z + 500.f)); } }
}
static void tunnel_y(Mesh &m, float cx, float ya, float yb, float hw, float za, float zb2, float H, uint32_t wc, uint32_t fc, uint32_t cc, int steps = 0) {
    V ref(cx, (ya + yb) * 0.5f, (za + zb2) * 0.5f + H * 0.5f);
    for (int k = 0; k < 2; k++) { float x = cx + (k ? hw : -hw); fq(m, V(x, ya, za), V(x, yb, zb2), V(x, yb, zb2 + H), V(x, ya, za + H), wc, ref); }
    fq(m, V(cx - hw, ya, za + H), V(cx + hw, ya, za + H), V(cx + hw, yb, zb2 + H), V(cx - hw, yb, zb2 + H), cc, ref);
    if (!steps) { fq(m, V(cx - hw, ya, za), V(cx + hw, ya, za), V(cx + hw, yb, zb2), V(cx - hw, yb, zb2), fc, ref); return; }
    for (int i = 0; i < steps; i++) {
        float y0 = ya + (yb - ya) * i / steps, y1 = ya + (yb - ya) * (i + 1) / steps, z = za + (zb2 - za) * (i + 1) / steps, zn = za + (zb2 - za) * (i + 2) / steps;
        uint32_t c = (i & 1) ? fc : fc + 0x0C0C0C;
        fq(m, V(cx - hw, y0, z), V(cx + hw, y0, z), V(cx + hw, y1, z), V(cx - hw, y1, z), c, V(cx, (y0 + y1) * 0.5f, z + 500.f));
        float zlow = (i + 1 < steps) ? zn : z - 200.f;
        fq(m, V(cx - hw, y1, zlow), V(cx + hw, y1, zlow), V(cx + hw, y1, z), V(cx - hw, y1, z), fc - 0x181818, V(cx, y1 + 300.f, z - 100.f));
    }
}
static void tunnel_x(Mesh &m, float cy, float xa, float xb, float hw, float z, float H, uint32_t wc, uint32_t fc, uint32_t cc) {
    V ref((xa + xb) * 0.5f, cy, z + H * 0.5f);
    for (int k = 0; k < 2; k++) { float y = cy + (k ? hw : -hw); fq(m, V(xa, y, z), V(xb, y, z), V(xb, y, z + H), V(xa, y, z + H), wc, ref); }
    fq(m, V(xa, cy - hw, z + H), V(xb, cy - hw, z + H), V(xb, cy + hw, z + H), V(xa, cy + hw, z + H), cc, ref);
    fq(m, V(xa, cy - hw, z), V(xb, cy - hw, z), V(xb, cy + hw, z), V(xa, cy + hw, z), fc, ref);
}
struct TorchP { V p; int room; };
static std::vector<TorchP> torches;
static void torch(Mesh &m, int room, V wall, V d) {   /* d: unit horizontal vector from the wall into the room. A slim dark wedge, point down (BBC t119, t199) */
    tube(m, wall + d * 20.f - V(0, 0, 420), wall + d * 90.f + V(0, 0, 80), 7.f, 46.f, 0x7A2A1C, 0x4A1C14);
    torches.push_back({wall + d * 95.f + V(0, 0, 80), room});
}
static float RBB[7][6];
static void finish_room(Mesh &m, int i) {
    float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
    for (size_t k = 0; k + 2 < m.v.size(); k += 3) for (int a = 0; a < 3; a++) { lo[a] = (std::min)(lo[a], m.v[k + a]); hi[a] = (std::max)(hi[a], m.v[k + a]); }
    for (int a = 0; a < 3; a++) { RBB[i][a] = lo[a]; RBB[i][3 + a] = hi[a]; }
    m.add(L_ROOM + i);
}
/* a plank door slab with an arched top; x0..x1, y thickness +-t, rectangular part to hz, arch top at ht */
static void door_slab(Mesh &m, float x0, float x1, float hz, float ht, float t, uint32_t W, uint32_t W2, uint32_t IR) {
    float w = x1 - x0, xm = (x0 + x1) * 0.5f;
    m.box(V(x0, -t, 0), V(x1, t, hz), W);
    for (int s = 0; s < 2; s++) { float y = s ? t : -t; V ref(xm, s ? 1000.f : -1000.f, hz + 100.f);
        polyv(m, {V(x0, y, hz), V(x0 + w * 0.14f, y, hz + (ht - hz) * 0.7f), V(x1 - w * 0.14f, y, hz + (ht - hz) * 0.7f), V(x1, y, hz)}, W, ref);
        polyv(m, {V(x0 + w * 0.14f, y, hz + (ht - hz) * 0.7f), V(xm, y, ht), V(x1 - w * 0.14f, y, hz + (ht - hz) * 0.7f)}, W, ref); }
    for (int k = 1; k < 8; k++) { float x = x0 + w * k / 8.f; m.box(V(x - 11, -t - 8, 40), V(x + 11, -t, hz + (ht - hz) * 0.5f - fabsf(x - xm) * 0.45f), W2); }
    for (int k = 0; k < 9; k++) { float x = x0 + w * (k + 0.5f) / 9.f; m.box(V(x - 16, -t - 12, hz - 300), V(x + 16, -t, hz - 268), 0xE8E0D0); m.box(V(x - 16, -t - 12, 560), V(x + 16, -t, 592), 0xE8E0D0); }   /* the rows of studs (t225) */
    (void)IR;
}

/* ---- walkable areas (collision is "stay inside these"); rooms are drawn E larger than their rectangle ---- */
enum { G_DOOR1 = 0, G_KEY = 1, G_LOW = 2, G_BIG = 3, NG = 4 };
static bool gate_open[NG];
struct Area { float x0, y0, x1, y1, z0, z1; int gate; };
static std::vector<Area> areas;
static const int TR_C = 3, TR_R = 4;
static const float TILE = 1500.f, TRAP_X0 = -2250.f, TRAP_Y0 = 35200.f;
static bool trap_is[TR_R][TR_C]; static float trap_open[TR_R][TR_C], trap_arm[TR_R][TR_C]; static int safe_col = 1;
static bool pit_at(float x, float y) {
    if (x < TRAP_X0 || x >= TRAP_X0 + TR_C * TILE || y < TRAP_Y0 || y >= TRAP_Y0 + TR_R * TILE) return false;
    int c = (int)((x - TRAP_X0) / TILE), r = (int)((y - TRAP_Y0) / TILE);
    return trap_open[r][c] > 0.8f;
}
static bool floor_at(float x, float y, float &z) {
    for (const Area &a : areas) {
        if (x < a.x0 || x > a.x1 || y < a.y0 || y > a.y1) continue;
        if (a.gate >= 0 && !gate_open[a.gate]) continue;
        float q = (a.y1 - a.y0) > 1.f ? (y - a.y0) / (a.y1 - a.y0) : 0.f; z = a.z0 + (a.z1 - a.z0) * q;
        if (pit_at(x, y)) z = Z1 - 3600.f;
        return true;
    }
    return false;
}
static bool walk_ok(float x, float y) { float z; return floor_at(x, y, z); }
static void resolve(V &p, V was) {   /* slide along walls instead of leaving the rooms */
    if (walk_ok(p.x, p.y)) return;
    if (walk_ok(p.x, was.y)) p.y = was.y; else if (walk_ok(was.x, p.y)) p.x = was.x; else { p.x = was.x; p.y = was.y; }
}

/* ---- building the world ---- */
static std::mt19937 wr(1992);
static float wf(float a, float b) { return a + (b - a) * (wr() / (float)(wr.max)()); }
static void build_world() {
    RM[0] = {-3000, 0, 3000, 5000, -900, 4700};
    RM[1] = {-5300, 9200, 1700, 16000, Z1, 3600};   /* the far door is right of the wall's middle, t161-t169 */
    RM[2] = {-900, 16700, 900, 26000, Z1, 3200};
    RM[3] = {-3000, 26900, 3000, 33000, Z1, 3800};
    RM[4] = {-13000, 26400, -8000, 33000, Z1, 4200};
    RM[5] = {-2250, 34200, 2250, 42200, Z1, 3600};
    RM[6] = {-4500, 43400, 4500, 52000, Z1, 5000};
    areas.clear();
    areas.push_back({-3000, 0, 3000, 3000, 0, 0, -1});             /* the raised floor you start on */
    areas.push_back({-3000, 3000, 3000, 4700, 0, -900, -1});       /* the steps ("down there are some steps and a door") */
    areas.push_back({-3000, 4700, 3000, 5000, -900, -900, -1});
    areas.push_back({-500, 5000, 500, 5300, -900, -900, G_LOW});
    areas.push_back({-500, 5300, 500, 8900, -900, Z1, -1});
    areas.push_back({-3300, 300, -3000, 2700, 0, 0, G_BIG});       /* out of the big door */
    areas.push_back({-11000, -3700, -3300, 6700, 0, 0, G_BIG});    /* the green outside: seen through the door (BBC t106) and with the wolf (Games-X 44 p12); how far it goes is INVENTED */
    areas.push_back({-500, 8900, 500, 9200, Z1, Z1, -1});
    areas.push_back({-5300, 9200, 1700, 16000, Z1, Z1, -1});
    areas.push_back({-500, 16000, 500, 16700, Z1, Z1, G_DOOR1});
    areas.push_back({-900, 16700, 900, 26000, Z1, Z1, -1});
    areas.push_back({-500, 26000, 500, 26900, Z1, Z1, -1});
    areas.push_back({-3000, 26900, 3000, 33000, Z1, Z1, -1});
    areas.push_back({-8000, 29200, -3000, 30200, Z1, Z1, -1});
    areas.push_back({-13000, 26400, -8000, 33000, Z1, Z1, -1});
    areas.push_back({-500, 33000, 500, 34200, Z1, Z1, G_KEY});
    areas.push_back({-2250, 34200, 2250, 42200, Z1, Z1, -1});
    areas.push_back({-500, 42200, 500, 43400, Z1, Z1, -1});
    areas.push_back({-4500, 43400, 4500, 52000, Z1, Z1, -1});
    torches.clear();

    /* room 0: the castle room. BBC t86-t119 and speech 1:42-1:50: a raised floor of pale slabs, steps down to an arched plank door with a mint strip at its foot, a torch, a big pale panelled double door on the left, the green outside beyond it */
    { Mesh m; Gap g[4]; gap(g[1], 0.f); gap(g[2], 1500.f, 4200.f); g[2].hw = 1400.f; g[3].skip = true; const uint32_t WL = 0xA89A96, FL = 0x90867F, FD = 0x7A706C, SL = 0x7A706C;
      /* the right-hand wall is dark (t87-t91), one sided, and holds the opening the castle mirror looks through. It stays out of the room mesh so the reflected copy of the room has no wall inside the real room */
      { Mesh w; const uint32_t WE = 0x6E6E80; V in(0, 2000, 1500); const float X = MIRROR_X, Y0 = MIRROR_Y - MIRROR_HW, Y1 = MIRROR_Y + MIRROR_HW;
        fq(w, V(X, -300, -1000), V(X, Y0, -1000), V(X, Y0, 3800), V(X, -300, 3800), WE, in); fq(w, V(X, Y1, -1000), V(X, 5300, -1000), V(X, 5300, 3800), V(X, Y1, 3800), WE, in);
        fq(w, V(X, Y0, MIRROR_H), V(X, Y1, MIRROR_H), V(X, Y1, 3800), V(X, Y0, 3800), WE, in); fq(w, V(X, Y0, -1000), V(X, Y1, -1000), V(X, Y1, 0), V(X, Y0, 0), WE, in);
        w.add(L_WALLE); }
      shell(m, RM[0], g, WL, 0x4C3E40, -1000.f);
      V up(0, 0, 3000);
      fq(m, V(-3300, -300, 0), V(3300, -300, 0), V(3300, 3000, 0), V(-3300, 3000, 0), FL, up);
      for (int k = 0; k < 4; k++) { float y = 300.f + k * 700.f, sk = 40.f * (k - 1.5f); fq(m, V(-3300, y, 2), V(3300, y, 2), V(3300, y + 22.f, 2), V(-3300, y + 22.f, 2), SL, up); float x = -2000.f + k * 1300.f + sk; fq(m, V(x, 2300, 2), V(x + 22.f, 2300, 2), V(x + 22.f + sk, 3000, 2), V(x + sk, 3000, 2), SL, up); }
      for (int i = 0; i < 6; i++) { float y0 = 3000.f + i * 283.3f, y1 = y0 + 283.3f, z0 = -150.f * i, z1 = -150.f * (i + 1);
          fq(m, V(-3300, y0, z0), V(3300, y0, z0), V(3300, y0, z1), V(-3300, y0, z1), FD, V(0, 9000, z1)); fq(m, V(-3300, y0, z1), V(3300, y0, z1), V(3300, y1, z1), V(-3300, y1, z1), (i & 1) ? FL : FL - 0x080808, up); }
      fq(m, V(-3300, 4700, -900), V(3300, 4700, -900), V(3300, 5300, -900), V(-3300, 5300, -900), FL - 0x101010, up);
      arch_surround(m, 5145.f, -900.f, 1700.f, 3300.f, 0x7A6462, 0x201410, V(0, 2500, 600), 0xA08A84, 0x7E6A66);
      fq(m, V(-780, 4985, -898), V(780, 4985, -898), V(780, 5200, -898), V(-780, 5200, -898), 0x6FB08E, up);   /* the mint strip, t88 */
      tunnel_y(m, 0, 5300, 8900, 800, -900, Z1, 3000, 0x6A5450, 0x6A4E44, 0x3A2A28, 14);
      torch(m, 0, V(3300, 3400, 2300), V(-1, 0, 0)); torch(m, 0, V(-2000, 5150, 2300), V(0, -1, 0));
      /* the big door's frame: tan posts and a dark beam (t106) */
      m.box(V(-3300, -20, 0), V(-3120, 100, 3300), 0xB08A60, 0x9A7850, 0x7A5C3C); m.box(V(-3300, 2900, 0), V(-3120, 3020, 3300), 0xB08A60, 0x9A7850, 0x7A5C3C); m.box(V(-3300, -20, 3300), V(-3100, 3020, 3560), 0x2E2422);
      /* outside */
      { V o(-7000, 1500, 1500); const float X0 = -11300.f, X1 = -3300.f, Y0 = -4000.f, Y1 = 7000.f, ZT = 6500.f;
        fq(m, V(X0, Y0, -6), V(X1, Y0, -6), V(X1, Y1, -6), V(X0, Y1, -6), 0x2FA63C, o);
        auto sky = [&](V a, V b) { fq(m, a, b, b + V(0, 0, 900), a + V(0, 0, 900), 0x3E8A58, o); fq(m, a + V(0, 0, 900), b + V(0, 0, 900), b + V(0, 0, ZT), a + V(0, 0, ZT), 0x8CC4E4, o); };
        sky(V(X0, Y0, 0), V(X0, Y1, 0)); sky(V(X0, Y0, 0), V(X1, Y0, 0)); sky(V(X0, Y1, 0), V(X1, Y1, 0));
        fq(m, V(X0, Y0, ZT), V(X1, Y0, ZT), V(X1, Y1, ZT), V(X0, Y1, ZT), 0xBCDCF0, o);
        const float tr[6][2] = {{-5500, 500}, {-6300, 2300}, {-8800, -1500}, {-8200, 4800}, {-7000, -2600}, {-9800, 2000}};
        for (int k = 0; k < 6; k++) { V b(tr[k][0], tr[k][1], 0); tube(m, b, b + V(0, 0, 2500), 230.f, 130.f, 0x7A3424, 0x5E2818);
            limb(m, b + V(0, 0, 2300), b + V(-700, 300, 3300), 50, 0x5E2818); limb(m, b + V(0, 0, 2200), b + V(600, -250, 3200), 50, 0x5E2818);
            gem(m, b + V(0, 0, 3500), 1250, 1150, 620, 0x2A8A3A, 0x1A5C28); } }
      finish_room(m, 0); }

    /* room 1: skeleton hall. Colours: BBC t162 (9E5E4E) reads dull, KKkZ t185 (D44909) oversaturated, Bad Influence B9846A between; picked the common hue at middle strength. Sparse thin brick lines, a lighter panel, the top of the back wall in the ceiling's colour (t169), arched door, chest on the right wall */
    { Mesh m; Gap g[4]; gap(g[0], 0.f); gap(g[1], 0.f); const uint32_t W = 0xC87C5E, WP = 0xD8886A, WD = 0xB06C52, CL = 0x7E5A5E, FLR = 0x96685C; const float T1 = Z1 + 3600.f;
      shell(m, RM[1], g, W, CL, Z1 - 100.f);
      fq(m, V(-5600, 8900, Z1), V(2000, 8900, Z1), V(2000, 16300, Z1), V(-5600, 16300, Z1), FLR, V(0, 12000, Z1 + 500));
      tunnel_y(m, 0, 16300, 16400, 800, Z1, Z1, 3000, 0x7A5C5A, 0x6A4A46, 0x3A2A2C);
      torch(m, 1, V(-1300, 16125, Z1 + 2500), V(0, -1, 0)); torch(m, 1, V(-5600, 13800, Z1 + 2300), V(1, 0, 0));
      /* the back wall */
      { arch_surround(m, 16145.f, Z1, 1500.f, 3300.f, WP, 0x3A1E14, V(0, 12000, Z1 + 1500), 0xA8674C, 0x80523C);
        m.box(V(-5600, 15050, Z1), V(-5200, 16300, T1), W, WD, WD); m.box(V(1600, 15050, Z1), V(2000, 16300, T1), W, WD, WD);
        m.box(V(-1580, 16120, Z1), V(-1500, 16300, Z1 + 3300), WD); m.box(V(1500, 16120, Z1), V(1580, 16300, Z1 + 3300), WD); { V rw(0, 12000, Z1 + 1500); fq(m, V(-5200, 16130, Z1), V(-1500, 16130, Z1), V(-1500, 16130, Z1 + 2800), V(-5200, 16130, Z1 + 2800), WP, rw); }
        V rf(0, 12000, Z1 + 1500); fq(m, V(-5200, 16118, Z1 + 2800), V(-1200, 16118, Z1 + 2800), V(-1200, 16118, T1), V(-5200, 16118, T1), CL, rf); fq(m, V(-1200, 16118, Z1 + 3300), V(1600, 16118, Z1 + 3300), V(1600, 16118, T1), V(-1200, 16118, T1), CL, rf); }
      /* a few thin brick lines, never a whole wall (t160, t169, t205) */
      { const uint32_t BL = 0x8E5238; auto bk = [&](V o, V du, float W2, float H, V rf) { const float bh = 340.f, bw = 760.f; int row = 0; for (float z = 0; z < H; z += bh, row++) { fq(m, o + V(0, 0, z), o + du * W2 + V(0, 0, z), o + du * W2 + V(0, 0, z + 14.f), o + V(0, 0, z + 14.f), BL, rf);
              if (z + bh < H) for (float u = (row & 1) ? bw * 0.5f : 0.f; u < W2; u += bw) fq(m, o + du * u + V(0, 0, z), o + du * (u + 14.f) + V(0, 0, z), o + du * (u + 14.f) + V(0, 0, z + bh), o + du * u + V(0, 0, z + bh), BL, rf); } };
        V c0(0, 12000, Z1); bk(V(-5180, 15040, Z1 + 1500), V(1, 0, 0), 1100.f, 1020.f, c0); bk(V(1990, 10300, Z1 + 1400), V(0, 1, 0), 1500.f, 1020.f, c0); bk(V(1990, 13800, Z1 + 1500), V(0, 1, 0), 1000.f, 680.f, c0); bk(V(-5590, 11600, Z1 + 1500), V(0, 1, 0), 1500.f, 1020.f, c0); bk(V(-1500, 8920, Z1 + 1900), V(1, 0, 0), 900.f, 680.f, c0); }
      /* the chest against the right wall (the lid is a separate part): a red-brown front in a grey frame with a notch at the lock, dark inside (t184, t186) */
      m.box(V(1100, 12000, Z1), V(1950, 13300, Z1 + 620), 0x12121C, 0x8A4028, 0x5A2C18);
      { V rf(0, 12650, Z1 + 300); const uint32_t GF = 0x8A7470; const float X = 1095.f; auto st = [&](float y0, float z0, float y1, float z1) { fq(m, V(X, y0, Z1 + z0), V(X, y1, Z1 + z0), V(X, y1, Z1 + z1), V(X, y0, Z1 + z1), GF, rf); };
        st(12000, 0, 13300, 70); st(12000, 560, 13300, 620); st(12000, 0, 12070, 620); st(13230, 0, 13300, 620);
        polyv(m, {V(X - 2, 12590, Z1 + 620), V(X - 2, 12710, Z1 + 620), V(X - 2, 12650, Z1 + 480)}, 0x3A2014, rf); }
      /* the bones in front of the door, with a helmet and a sword ("a suspicious pile of bones on the left", speech 2:58) */
      for (int k = 0; k < 14; k++) { V a(-1000 + wf(-800, 800), 15450 + wf(-400, 400), Z1 + 40), d = norm(V(wf(-1, 1), wf(-1, 1), 0));
          limb(m, a, a + d * wf(250, 550), 28, 0xE4DEC8); }
      m.box(V(-1200, 15150, Z1), V(-1040, 15310, Z1 + 160), 0xEDE8D4); m.box(V(-1180, 15300, Z1 + 70), V(-1140, 15320, Z1 + 110), 0x201810); m.box(V(-1100, 15300, Z1 + 70), V(-1060, 15320, Z1 + 110), 0x201810);
      cone(m, V(-600, 15000, Z1), 150, 170, 0x8A8A92, 4);
      limb(m, V(-300, 15400, Z1 + 30), V(300, 15700, Z1 + 30), 22, 0xA0A0AA); limb(m, V(-340, 15380, Z1 + 30), V(-260, 15420, Z1 + 30), 40, 0xB89030);
      /* the low table to the right as you come in (t158, t160, Bad Influence t47-50). The "table covered in bones" was a misreading: the bones lie on the floor beyond it */
      m.box(V(500, 9500, Z1 + 640), V(1800, 11200, Z1 + 720), 0x7A3A2A, 0x5E2C20, 0x4A241A);
      for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) limb(m, V(a ? 1700.f : 600.f, b ? 11100.f : 9600.f, Z1), V(a ? 1700.f : 600.f, b ? 11100.f : 9600.f, Z1 + 640), 45, 0x4A241A);
      finish_room(m, 1); }

    /* room 2: the corridor with torches and black winged shapes (KKkZ 227-236 s) */
    { Mesh m; Gap g[4]; gap(g[0], 0.f, 2800.f); gap(g[1], 0.f, 2800.f);
      shell(m, RM[2], g, 0x6E5650, 0x4A3A3C, Z1 - 100.f); floor_checker(m, RM[2], 0x624C44, 0x624C44, 3000.f);   /* grey-brown, BBC t215 */
      tunnel_y(m, 0, 26300, 26600, 800, Z1, Z1, 3000, 0x6E5650, 0x624C44, 0x4A3A3C); torch(m, 2, V(0, 26290, Z1 + 3050), V(0, -1, 0));   /* the torch dead ahead, t211 */
      torch(m, 2, V(-1200, 18900, Z1 + 1900), V(1, 0, 0)); torch(m, 2, V(1200, 21200, Z1 + 1900), V(-1, 0, 0));
      torch(m, 2, V(-1200, 23500, Z1 + 1900), V(1, 0, 0)); torch(m, 2, V(1200, 17300, Z1 + 1900), V(-1, 0, 0));
      finish_room(m, 2); }

    /* room 3: there is no "red room": every red shot is the red flash of a hit in the skeleton hall (Bad Influence t39-40, BBC t173), and its door and table are the hall's. This room is the one in Amiga Action 31 p.2: black walls, a winding yellow path, an arched door, a long green scaled thing. Where it sits in the game is INVENTED */
    { Mesh m; Gap g[4]; gap(g[0], 0.f); gap(g[1], 0.f); gap(g[2], 29700.f);
      shell(m, RM[3], g, 0x34363E, 0x1E1F26, Z1 - 100.f); floor_checker(m, RM[3], 0x1C1E22, 0x1C1E22, 3000.f);
      { V up(0, 30000, Z1 + 500); const int N = 12; auto px = [](float q) { return sinf(q * 5.2f) * 1300.f * (1.f - fabsf(q * 2.f - 1.f) * 0.8f); };
        for (int i = 0; i < N; i++) { float q0 = (float)i / N, q1 = (float)(i + 1) / N, y0 = 26600.f + q0 * 6700.f, y1 = 26600.f + q1 * 6700.f; const float hw = 850.f; fq(m, V(px(q0) - hw, y0, Z1 + 5), V(px(q0) + hw, y0, Z1 + 5), V(px(q1) + hw, y1, Z1 + 5), V(px(q1) - hw, y1, Z1 + 5), 0xE8C020, up); } }
      arch_surround(m, 33145.f, Z1, 1700.f, 3300.f, 0x34363E, 0x0C0C10, V(0, 28000, Z1 + 1500), 0x4A525A, 0x363C44);
      tunnel_x(m, 29700, -7700, -3300, 800, Z1, 3000, 0x4A3E4A, 0x3A3238, 0x241E26);   /* INVENTED: side passage to the ogre */
      tunnel_y(m, 0, 33300, 33900, 800, Z1, Z1, 3000, 0x4A4A56, 0x3A3A44, 0x2A2A32);
      torch(m, 3, V(3300, 28000, Z1 + 2300), V(-1, 0, 0)); torch(m, 3, V(3300, 31500, Z1 + 2300), V(-1, 0, 0)); torch(m, 3, V(-3300, 27800, Z1 + 2300), V(1, 0, 0));
      finish_room(m, 3); }

    /* room 4: the ogre's room. Dark mauve behind him in Amiga Action 31 p.2; the rest INVENTED */
    { Mesh m; Gap g[4]; gap(g[3], 29700.f);
      shell(m, RM[4], g, 0x3E3444, 0x241E28, Z1 - 100.f); floor_checker(m, RM[4], 0x342C38, 0x2E2632, 1500.f);
      for (int k = 0; k < 6; k++) { float x = wf(-12800, -8400), y = (k & 1) ? wf(26700, 27600) : wf(32000, 32800); m.box(V(x, y, Z1), V(x + wf(500, 900), y + wf(400, 800), Z1 + wf(300, 900)), 0x4E4250, 0x423846, 0x362E3A); }
      for (int k = 0; k < 8; k++) { V a(wf(-12500, -9000), wf(27000, 32500), Z1 + 40), d = norm(V(wf(-1, 1), wf(-1, 1), 0)); limb(m, a, a + d * wf(250, 500), 26, 0xD8D2BC); }
      torch(m, 4, V(-13300, 29700, Z1 + 2300), V(1, 0, 0)); torch(m, 4, V(-10500, 26100, Z1 + 2300), V(0, 1, 0));
      finish_room(m, 4); }

    /* room 5: the trap-door room. INVENTED. A safe strip, 12 tiles over a pit (one trap in each row), a safe strip */
    { Mesh m; Gap g[4]; gap(g[0], 0.f); gap(g[1], 0.f);
      const float zb = Z1 - 3700.f; const Room &r = RM[5]; const float X0 = r.x0 - E, X1 = r.x1 + E, Y0 = r.y0 - E, Y1 = r.y1 + E;
      shell(m, r, g, 0x56647A, 0x2E3440, zb);
      V up(0, 0, Z1 + 500);
      fq(m, V(X0, Y0, Z1), V(X1, Y0, Z1), V(X1, TRAP_Y0, Z1), V(X0, TRAP_Y0, Z1), 0x606870, up);
      fq(m, V(X0, TRAP_Y0 + TR_R * TILE, Z1), V(X1, TRAP_Y0 + TR_R * TILE, Z1), V(X1, Y1, Z1), V(X0, Y1, Z1), 0x606870, up);
      fq(m, V(X0, TRAP_Y0, Z1), V(TRAP_X0, TRAP_Y0, Z1), V(TRAP_X0, TRAP_Y0 + TR_R * TILE, Z1), V(X0, TRAP_Y0 + TR_R * TILE, Z1), 0x555D66, up);
      fq(m, V(-TRAP_X0, TRAP_Y0, Z1), V(X1, TRAP_Y0, Z1), V(X1, TRAP_Y0 + TR_R * TILE, Z1), V(-TRAP_X0, TRAP_Y0 + TR_R * TILE, Z1), 0x555D66, up);
      const float pz = Z1 - 3600.f, py1 = TRAP_Y0 + TR_R * TILE; V pit(0, (TRAP_Y0 + py1) * 0.5f, Z1 - 1800.f);
      fq(m, V(TRAP_X0, TRAP_Y0, pz), V(-TRAP_X0, TRAP_Y0, pz), V(-TRAP_X0, py1, pz), V(TRAP_X0, py1, pz), 0x14141C, pit);
      fq(m, V(TRAP_X0, TRAP_Y0, pz), V(TRAP_X0, py1, pz), V(TRAP_X0, py1, Z1), V(TRAP_X0, TRAP_Y0, Z1), 0x23242C, pit);
      fq(m, V(-TRAP_X0, TRAP_Y0, pz), V(-TRAP_X0, py1, pz), V(-TRAP_X0, py1, Z1), V(-TRAP_X0, TRAP_Y0, Z1), 0x23242C, pit);
      fq(m, V(TRAP_X0, TRAP_Y0, pz), V(-TRAP_X0, TRAP_Y0, pz), V(-TRAP_X0, TRAP_Y0, Z1), V(TRAP_X0, TRAP_Y0, Z1), 0x23242C, pit);
      fq(m, V(TRAP_X0, py1, pz), V(-TRAP_X0, py1, pz), V(-TRAP_X0, py1, Z1), V(TRAP_X0, py1, Z1), 0x23242C, pit);
      for (int k = 0; k < 8; k++) cone(m, V(wf(-2000, 2000), wf(TRAP_Y0 + 300, py1 - 300), pz), 130, 520, 0x9A9AA6, 4);
      tunnel_y(m, 0, 42500, 43100, 800, Z1, Z1, 3000, 0x4C3C5C, 0x383040, 0x241C30);
      torch(m, 5, V(-2550, 36000, Z1 + 1900), V(1, 0, 0)); torch(m, 5, V(2550, 39500, Z1 + 1900), V(-1, 0, 0));
      finish_room(m, 5); }

    /* room 6: the spider's lair. INVENTED */
    { Mesh m; Gap g[4]; gap(g[0], 0.f); g[1].skip = true;
      { Mesh w; const uint32_t WC = 0x4C3C5C; V in(0, 47000, Z1 + 2000); const float Y = LAIR_Y, A = -4800.f, B = 4800.f, Z0 = Z1 - 100.f, T = Z1 + 5000.f, H = Z1 + MIRROR_H;
        fq(w, V(A, Y, Z0), V(-MIRROR_HW, Y, Z0), V(-MIRROR_HW, Y, T), V(A, Y, T), WC, in); fq(w, V(MIRROR_HW, Y, Z0), V(B, Y, Z0), V(B, Y, T), V(MIRROR_HW, Y, T), WC, in);
        fq(w, V(-MIRROR_HW, Y, H), V(MIRROR_HW, Y, H), V(MIRROR_HW, Y, T), V(-MIRROR_HW, Y, T), WC, in); fq(w, V(-MIRROR_HW, Y, Z0), V(MIRROR_HW, Y, Z0), V(MIRROR_HW, Y, Z1), V(-MIRROR_HW, Y, Z1), WC, in);
        w.add(L_WALLN); }
      shell(m, RM[6], g, 0x4C3C5C, 0x241C30, Z1 - 100.f); floor_checker(m, RM[6], 0x383040, 0x30283A, 1500.f);
      const float cs[4][4] = {{-4800, 43700, 1, 1}, {4800, 43700, -1, 1}, {-4800, 51700, 1, -1}, {4800, 51700, -1, -1}};
      for (int k = 0; k < 4; k++) { V c(cs[k][0], cs[k][1], Z1 + 4300.f), ref(0, 47000, Z1 + 2500);   /* webs in the corners */
          for (int i = 0; i < 8; i++) { float a = LPI * 0.5f * i / 7.f; V d(cosf(a) * cs[k][2] * 1600.f, sinf(a) * cs[k][3] * 1600.f, 0.f), d2(0, 0, -1500.f);
              V e = c + d * 0.0f + V(d.x, d.y, 0) + d2 * (i & 1 ? 0.6f : 1.f);
              polyv(m, {c, e, e + V(40, 40, 0)}, 0xE0E0E8, ref); } }
      for (int k = 0; k < 5; k++) gem(m, V(wf(-4000, 4000), wf(44200, 51400), Z1 + 260), 260, 260, 300, 0xD8D4C8, 0xB8B4A8);   /* egg sacs */
      torch(m, 6, V(-4800, 46000, Z1 + 2300), V(1, 0, 0)); torch(m, 6, V(4800, 49000, Z1 + 2300), V(-1, 0, 0));
      finish_room(m, 6); }

    /* ---- parts ---- */
    { Mesh f; dbl(f, {V(-55, 0, 0), V(55, 0, 0), V(0, 0, 200)}, 0xFF8A18); dbl(f, {V(0, -55, 0), V(0, 55, 0), V(0, 0, 200)}, 0xFF8A18);
      dbl(f, {V(-28, 1, 0), V(28, 1, 0), V(0, 1, 120)}, 0xFFE040); dbl(f, {V(1, -28, 0), V(1, 28, 0), V(1, 0, 120)}, 0xFFE040); f.add(L_FLAME); }
    { Mesh s; gem(s, V(0, 0, 0), 26, 26, 30, 0xFFE040, 0xE0B030); s.add(L_GOLD); }     /* the sparks are tiny (t191) */
    { Mesh s; gem(s, V(0, 0, 0), 26, 26, 30, 0xF03018, 0xB02010); s.add(L_REDSP); }
    { Mesh s; gem(s, V(0, 0, 0), 30, 30, 26, 0xF0F0E8, 0x505048); s.add(L_DOT); }      /* and settle as pale dots with a dark edge (t200) */
    { Mesh s; gem(s, V(0, 0, 0), 24, 24, 24, 0x101010, 0x101010); s.add(L_BLK); }      /* the specks round the head (t233) */
    { const uint32_t R = 0xB81C34, R2 = 0x8E1428, SK = 0xE8A070, SK2 = 0xC88654;
      for (int k = 0; k < 2; k++) {   /* in the crimson sleeve, hand at the origin, blade along +y. 0: the wizard's short dagger (third person t209); 1: the longer sword from the chest (one review) */
        float L = k ? 1050.f : 420.f, bw = k ? 46.f : 44.f; Mesh s;
        s.box(V(-95, 0, -26), V(95, 45, 26), 0x4A4C54, 0x35373E, 0x2A2C32);
        s.box(V(-bw, 45, -9), V(bw, L, 9), 0x8C9098, 0x6C7078, 0x5C6068); dbl(s, {V(-bw, L, 9.5f), V(bw, L, 9.5f), V(0, L + 170, 0)}, 0x9CA0A8); dbl(s, {V(-bw, L, -9.5f), V(bw, L, -9.5f), V(0, L + 170, 0)}, 0x74787F);
        s.box(V(-58, -95, -55), V(58, 8, 60), SK, SK2, SK2);
        tube(s, V(0, -95, 0), V(0, -780, -40), 84.f, 128.f, R, R2);
        s.add(k ? L_SWORD2 : L_SWORD1); }
      { Mesh h;  /* the empty hand: a flat mitten with a thumb, fingers along +y (t186, t207) */
        tube(h, V(0, -95, 0), V(0, -780, -40), 84.f, 128.f, R, R2);
        h.box(V(-58, -95, -24), V(58, 120, 24), SK, SK2, SK2); polyv(h, {V(-58, 120, 24), V(58, 120, 24), V(30, 300, 10), V(-30, 300, 10)}, SK, V(0, 0, 3000)); polyv(h, {V(-58, 120, -24), V(58, 120, -24), V(30, 300, -10), V(-30, 300, -10)}, SK2, V(0, 0, -3000));
        polyv(h, {V(-58, 120, 24), V(-30, 300, 10), V(-30, 300, -10), V(-58, 120, -24)}, SK2, V(-3000, 0, 0)); polyv(h, {V(58, 120, 24), V(30, 300, 10), V(30, 300, -10), V(58, 120, -24)}, SK2, V(3000, 0, 0));
        tube(h, V(55, -40, 10), V(125, 120, 40), 22.f, 16.f, SK, SK2);
        h.add(L_HAND); }
      { Mesh b;  /* the bow, "touch your shoulder and a bow appears" (Tilt 103 p.41); an elf with one in Amiga Action 31 p.2 and Games-X 44 p.12 */
        tube(b, V(0, -95, 0), V(0, -780, -40), 84.f, 128.f, R, R2); b.box(V(-48, -95, -48), V(48, 40, 48), SK, SK2, SK2);
        for (int i = 0; i < 10; i++) { float q0 = i / 10.f * 2.f - 1.f, q1 = (i + 1) / 10.f * 2.f - 1.f; limb(b, V(0, 240.f * (1.f - q0 * q0) - 60.f, q0 * 760.f), V(0, 240.f * (1.f - q1 * q1) - 60.f, q1 * 760.f), 18, 0x7A4A24); }
        limb(b, V(0, -60, -760), V(0, -300, 0), 4, 0xE8E0D0); limb(b, V(0, -60, 760), V(0, -300, 0), 4, 0xE8E0D0); limb(b, V(0, -300, 0), V(0, 300, 0), 8, 0xD8C890);
        b.add(L_BOW); }
      { Mesh a; limb(a, V(0, -300, 0), V(0, 300, 0), 9, 0xD8C890); gem(a, V(0, 320, 0), 22, 40, 22, 0xB0B4BC, 0x80848C); a.add(L_ARROW); }
      { Mesh fh;  /* the flame on the palm, t205-t206: a yellow bulb with an orange-pink zigzag on top */
        gem(fh, V(0, 0, 80), 120, 120, 130, 0xFFE84A, 0xFFD030);
        dbl(fh, {V(-80, 0, 170), V(70, 0, 180), V(120, 0, 320), V(-10, 0, 300)}, 0xF08858); dbl(fh, {V(-10, 0, 300), V(120, 0, 320), V(40, 0, 440), V(-70, 0, 480)}, 0xF06868);
        fh.add(L_FLAMEH); }
      { Mesh v;  /* the open spell book held up to your face, t197: pale pages, a grey gutter, a red band across the top */
        const uint32_t PG = 0xF0C8A0, PG2 = 0xE8BC94; V ref(0, -1000, 0);
        polyv(v, {V(-620, 0, -460), V(-20, 0, -460), V(-20, 0, 230), V(-260, 0, 300), V(-620, 0, 360)}, PG, ref); polyv(v, {V(20, 0, -460), V(620, 0, -460), V(620, 0, 360), V(260, 0, 300), V(20, 0, 230)}, PG2, ref);
        polyv(v, {V(-20, 0, -460), V(20, 0, -460), V(20, 0, 230), V(-20, 0, 230)}, 0x9A8A84, ref);
        polyv(v, {V(-620, -2, 235), V(620, -2, 235), V(620, -2, 262), V(-620, -2, 262)}, 0xD83030, ref);
        v.add(L_BOOKV); } }
    { Mesh b; gem(b, V(0, 0, 40), 90, 90, 190, 0xFF9020, 0xFF6010); gem(b, V(0, 0, 10), 55, 55, 120, 0xFFF080, 0xFFD040); b.add(L_BALL); }
    { Mesh b; b.box(V(-150, -110, -30), V(150, 110, 30), 0x7A1020, 0x5A0C18, 0x5A0C18); b.box(V(-140, -100, -20), V(140, 100, 20), 0xE8E0C0);
      b.box(V(-155, -112, -34), V(-125, 112, 34), 0x5A0C18); b.box(V(-30, -115, 31), V(30, 115, 36), 0xD8B030); b.box(V(-115, -40, 31), V(115, 40, 36), 0xD8B030); b.add(L_BOOK); }
    { Mesh b;  /* what a skeleton leaves (t199-t200): skull, a stub of ribs, long bones, its shield flat, its spear */
      const uint32_t B = 0xE0D49C, B2 = 0xBCAE7C;
      gem(b, V(120, 200, 95), 95, 105, 95, B, B2); limb(b, V(-300, -200, 40), V(320, -40, 40), 24, B); limb(b, V(-260, 160, 40), V(200, -280, 40), 24, B2);
      for (int k = 0; k < 4; k++) limb(b, V(-420.f + k * 40.f, -420.f + k * 80.f, 90), V(-160.f + k * 40.f, -420.f + k * 80.f, 90), 16, B);
      star_shield(b, V(380, 240, 40), V(0.05f, 0, 1), 340.f); limb(b, V(-640, 200, 30), V(100, 560, 30), 14, 0xA89060);
      b.add(L_BONES); }
    { Mesh k; limb(k, V(0, 0, 0), V(0, 420, 0), 22, 0xE8C030); ngon(k, V(0, -110, 0), V(1, 0, 0), V(0, 1, 0), 120, 120, 8, 0xE8C030, V(0, -110, 500));
      ngon(k, V(0, -110, 0), V(1, 0, 0), V(0, 1, 0), 120, 120, 8, 0xE8C030, V(0, -110, -500));
      k.box(V(0, 300, -20), V(110, 360, 20), 0xE8C030); k.box(V(0, 380, -20), V(80, 420, 20), 0xE8C030); k.add(L_KEY); }
    { Mesh l;  /* the lid: a rounded top with iron straps when shut (t203), a flat near-black underside when it stands open (t186) */
      const int NS = 5; for (int i = 0; i < NS; i++) { float a0 = LPI * i / NS, a1 = LPI * (i + 1) / NS; float x0 = -425.f - 425.f * cosf(a0), z0 = 260.f * sinf(a0), x1 = -425.f - 425.f * cosf(a1), z1 = 260.f * sinf(a1); V ref(-425, 0, -500);
          fq(l, V(x0, -650, z0), V(x1, -650, z1), V(x1, 650, z1), V(x0, 650, z0), (i & 1) ? 0x8A4A2C : 0x7C4026, ref, false);
          for (int k = -1; k <= 1; k += 2) { float y = k * 380.f; fq(l, V(x0, y - 45, z0 + 3), V(x1, y - 45, z1 + 3), V(x1, y + 45, z1 + 3), V(x0, y + 45, z0 + 3), 0x3A2A24, ref, false); } }
      fq(l, V(-850, -650, 0), V(0, -650, 0), V(0, 650, 0), V(-850, 650, 0), 0x1C1C2C, V(-425, 0, -500));
      l.add(L_LID); }
    { Mesh d;  /* one leaf of the big door (t93, t110): pale grey panels picked out in white, a triangle handle. Local x is along the leaf from the hinge */
      const float W = 1400.f, H = 3300.f; d.box(V(0, -70, 0), V(W, 70, H), 0x8E8884, 0x7A7470, 0x6A6460);
      for (int s = 0; s < 2; s++) { float y = s ? 74.f : -74.f; V rf(W * 0.5f, s ? 2000.f : -2000.f, H * 0.5f); auto pn = [&](float x0, float z0, float x1, float z1) { const float L = 26.f; auto q = [&](float a, float b, float c, float e) { fq(d, V(a, y, b), V(c, y, b), V(c, y, e), V(a, y, e), 0xD8D4CC, rf); }; q(x0, z0, x1, z0 + L); q(x0, z1 - L, x1, z1); q(x0, z0, x0 + L, z1); q(x1 - L, z0, x1, z1); };
          pn(180, 300, W - 180, 1400); pn(180, 1650, W - 180, 3000);
          float cx = W - 110.f; fq(d, V(cx - 70, y, 1300), V(cx + 70, y, 1300), V(cx + 70, y, 1326), V(cx - 70, y, 1326), 0xE8E0D0, rf); fq(d, V(cx - 70, y, 1300), V(cx - 46, y, 1300), V(cx - 4, y, 1560), V(cx - 28, y, 1560), 0xE8E0D0, rf); fq(d, V(cx + 46, y, 1300), V(cx + 70, y, 1300), V(cx + 28, y, 1560), V(cx + 4, y, 1560), 0xE8E0D0, rf); }
      d.add(L_BIGDOOR); }
    { Mesh d;  /* the arched door in the yellow-path room (Amiga Action 31 p.2): dark iron bars, a pale handle */
      door_slab(d, -780, 780, 2500, 3000, 110, 0x2A2C34, 0x12141A, 0x2C2C32);
      const float T = -128.f, zb = 1050.f, zt = 1420.f; const uint32_t H = 0x8CC0C8; V rf(0, -2000, 1200); fq(d, V(-150, T, zb), V(150, T, zb), V(150, T, zb + 26), V(-150, T, zb + 26), H, rf); fq(d, V(-150, T, zb), V(-124, T, zb), V(-6, T, zt), V(-32, T, zt), H, rf); fq(d, V(124, T, zb), V(150, T, zb), V(32, T, zt), V(6, T, zt), H, rf);
      d.add(L_DOOR2); }
    for (int var = 0; var < 3; var++) { Mesh d; const uint32_t W = 0x8A4430, W2 = 0x50241A, IR = 0x2C2C32;   /* dark red-brown planks, BBC t207 */
        if (var < 2) { door_slab(d, -780, 780, 2500, 3000, 110, W, W2, IR);
            if (var == 1) { limb(d, V(80, -135, 1800), V(-300, -135, 2500), 14, 0x100804); limb(d, V(80, -135, 1800), V(380, -135, 2300), 14, 0x100804);
                limb(d, V(80, -135, 1800), V(-120, -135, 1100), 14, 0x100804); limb(d, V(80, -135, 1800), V(300, -135, 1000), 14, 0x100804); limb(d, V(-300, -135, 2500), V(-520, -135, 2700), 12, 0x100804); } }
        else {   /* t225: the planks stand but a ragged hole opens in the upper door */
            const int NP = 8; const float lo[NP] = {9999, 9999, 1500, 1250, 1100, 1350, 1650, 9999}, hi[NP] = {0, 0, 2700, 2850, 2950, 2750, 2550, 0}; const float pw = 1560.f / NP;
            for (int i = 0; i < NP; i++) { float x = -780.f + i * pw; float top = 2500.f + 500.f * (1.f - fabsf(x + pw * 0.5f) / 780.f) * 0.8f;
                if (lo[i] > 9000.f) { d.box(V(x, -110, 0), V(x + pw - 14.f, 110, top), W, W, W2); continue; }
                d.box(V(x, -110, 0), V(x + pw - 14.f, 110, lo[i]), W, W, W2); if (hi[i] < top) d.box(V(x, -110, hi[i]), V(x + pw - 14.f, 110, top), W, W, W2);
                polyv(d, {V(x, -112, lo[i]), V(x + pw * 0.5f, -112, lo[i] + 170.f), V(x + pw - 14.f, -112, lo[i])}, W, V(0, -2000, 0)); }
            limb(d, V(-300, -125, 1300), V(-180, -125, 300), 14, 0x100804); limb(d, V(210, -125, 1500), V(330, -125, 400), 14, 0x100804);
            for (int k = 0; k < 9; k++) { float x = -780.f + 1560.f * (k + 0.5f) / 9; d.box(V(x - 16, -122, 560), V(x + 16, -110, 592), 0xE8E0D0); } }
        { const float T = -128.f, zb = 1050.f, zt = 1420.f; const uint32_t H = 0xE8E0D0; V rf(0, -2000, 1200);   /* the white ring on the door (t225) */
          fq(d, V(-150, T, zb), V(150, T, zb), V(150, T, zb + 26), V(-150, T, zb + 26), H, rf); fq(d, V(-150, T, zb), V(-124, T, zb), V(-6, T, zt), V(-32, T, zt), H, rf); fq(d, V(124, T, zb), V(150, T, zb), V(32, T, zt), V(6, T, zt), H, rf);
          fq(d, V(-34, T, zt), V(34, T, zt), V(34, T, zt + 70), V(-34, T, zt + 70), 0x201810, rf); }
        d.add(var == 0 ? L_DOOR1 : var == 1 ? L_DOOR1B : L_DOOR1C); }
    { Mesh d; d.box(V(-800, -120, 0), V(800, 120, 3000), 0x8A4A34, 0x7A4230, 0x6A3828);    /* the tall plank door of Bad Influence t25: plain planks, a white plate with a black keyhole, a dashed line low down */
      for (int i = 1; i < 6; i++) d.box(V(-800.f + i * 266.f - 7.f, -130, 0), V(-800.f + i * 266.f + 7.f, -120, 3000), 0x7A4428);
      d.box(V(-210, -136, 1250), V(210, -120, 1850), 0xE8E6DC); polyv(d, {V(-20, -140, 1560), V(20, -140, 1560), V(90, -140, 1400), V(-90, -140, 1400)}, 0x0C0C0C, V(0, -2000, 1500)); ngon(d, V(0, -140, 1610), V(1, 0, 0), V(0, 0, 1), 56, 62, 8, 0x0C0C0C, V(0, -2000, 1610));
      polyv(d, {V(-130, -140, 1320), V(130, -140, 1320), V(130, -140, 1340), V(-130, -140, 1340)}, 0x908E84, V(0, -2000, 1400));
      for (int i = 0; i < 12; i++) { float x = -760.f + i * 132.f; d.box(V(x, -128, 300), V(x + 70, -120, 322), 0xF0F0E8); }
      d.add(L_KEYDOOR); }
    for (int v = 0; v < 2; v++) { Mesh t; t.box(V(-740, 0, -150), V(740, 1480, 0), v ? 0x7E8692 : 0x808896, 0x50566A, 0x40465A);
        if (v) for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) { float x = a ? 480.f : -680.f, y = b ? 1100.f : 60.f; fq(t, V(x, y, 2), V(x + 200, y, 2), V(x + 200, y + 200, 2), V(x, y + 200, 2), 0x6A5A58, V(x, y, 500)); }
        t.add(v ? L_TILE_T : L_TILE_S); }
    for (int v = 0; v < 2; v++) { Mesh mm; const uint32_t G = v ? 0xFFF0A0 : 0xC8A030, G2 = v ? 0xE8D890 : 0x98781C; const float H = MIRROR_HW, T = MIRROR_H;   /* the mirror's frame, full length so you see your feet (its look is INVENTED): faces local -y, origin at the floor */
        mm.box(V(-H - 90, -60, 0), V(-H, 0, T + 90), G, G2, G2); mm.box(V(H, -60, 0), V(H + 90, 0, T + 90), G, G2, G2); mm.box(V(-H, -60, T), V(H, 0, T + 90), G, G2, G2); mm.box(V(-H, -60, 0), V(H, 0, 30), G, G2, G2);
        mm.add(v ? L_MIRROR_G : L_MIRROR); }

    /* skeleton (t185, t187, t19.5): taller than a person and thin: a small skull, a wide bar of shoulders, ladder ribs narrowing down a thin spine, a bar of pelvis, long thin limbs, star shield on the left, a short spear with a diamond head on the right */
    { Mesh m; const uint32_t B = 0xF0E4AC, B2 = 0xCCBE88, DK = 0x1C1812;
      m.box(V(-190, -35, 1030), V(190, 35, 1090), B, B2, B2); tube(m, V(0, 0, 1060), V(0, 0, 1760), 26.f, 22.f, B2, B);
      m.box(V(-330, -40, 1730), V(330, 40, 1790), B, B2, B2);
      for (int k = 0; k < 7; k++) { float z = 1660.f - k * 72.f, w = 270.f - k * 22.f; m.box(V(-w, -95, z), V(w, -70, z + 26), B, B2, B2); m.box(V(-w, 70, z), V(w, 95, z + 26), B, B2, B2); m.box(V(-w - 24, -95, z), V(-w, 95, z + 26), B, B2, B2); m.box(V(w, -95, z), V(w + 24, 95, z + 26), B, B2, B2); }
      for (int k = -1; k <= 1; k += 2) { tube(m, V(k * 240.f, 0, 1730), V(k * 130.f, 0, 1240), 14.f, 14.f, B2, B); }
      tube(m, V(0, 0, 1790), V(0, 0, 1870), 28.f, 28.f, B2);
      stackp(m, V(0, 10, 0), {{1860, 72, 76, 12}, {1930, 100, 104, 0}, {2040, 110, 112, -8}, {2110, 84, 88, -10}, {2140, 40, 44, -10}}, B, B2);
      fq(m, V(-74, 106, 1965), V(-28, 106, 1965), V(-28, 106, 2020), V(-74, 106, 2020), DK, V(0, 2000, 2000)); fq(m, V(28, 106, 1965), V(74, 106, 1965), V(74, 106, 2020), V(28, 106, 2020), DK, V(0, 2000, 2000));
      polyv(m, {V(-18, 112, 1905), V(18, 112, 1905), V(0, 112, 1945)}, DK, V(0, 2000, 2000));
      tube(m, V(-330, 0, 1760), V(-380, 80, 1320), 22.f, 18.f, B2, B); tube(m, V(-380, 80, 1320), V(-420, 300, 1120), 18.f, 16.f, B2, B);
      tube(m, V(330, 0, 1760), V(380, 60, 1320), 22.f, 18.f, B2, B); tube(m, V(380, 60, 1320), V(350, 320, 1180), 18.f, 16.f, B2, B);
      tube(m, V(340, -250, 860), V(350, 760, 1620), 15.f, 13.f, 0xC8B080, 0xA89060);
      gem(m, V(350, 840, 1740), 62, 40, 150, 0xE8C878, 0xC8A050);
      star_shield(m, V(-470, 330, 1120), V(-0.35f, 1, 0.1f), 400.f);
      m.add(L_SKEL); }
    { Mesh l; const uint32_t B = 0xF0E4AC, B2 = 0xCCBE88; tube(l, V(0, 0, 0), V(0, 20, -530), 26.f, 20.f, B2, B); tube(l, V(0, 20, -530), V(0, -10, -1000), 20.f, 16.f, B2, B); l.box(V(-48, -60, -1050), V(48, 150, -1000), B, B2, B2); l.add(L_SKEL_LEG); }
    /* bat: a thin black gull shape (t233) */
    { Mesh b; b.box(V(-22, -70, -18), V(22, 80, 22), 0x0C0A0E); b.add(L_BAT); }
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; Mesh w; const uint32_t C = 0x0C0A0E;
        const V o[5] = {V(0, 70, 0), V(q * 170, 70, 60), V(q * 430, 15, -25), V(q * 190, -25, 35), V(0, -70, 0)};   /* a gull wing, with thickness so it never turns to nothing edge on */
        const V ht(0, 0, 18); for (int e = 0; e < 5; e++) { int n = (e + 1) % 5; polyv(w, {o[e] - ht, o[n] - ht, o[n] + ht, o[e] + ht}, C, o[e] * 2.f); }
        polyv(w, {o[0] + ht, o[1] + ht, o[3] + ht, o[4] + ht}, C, V(0, 0, 3000)); polyv(w, {o[1] + ht, o[2] + ht, o[3] + ht}, C, V(0, 0, 3000));
        polyv(w, {o[0] - ht, o[1] - ht, o[3] - ht, o[4] - ht}, C, V(0, 0, -3000)); polyv(w, {o[1] - ht, o[2] - ht, o[3] - ht}, C, V(0, 0, -3000));
        w.add(k ? L_BAT_WR : L_BAT_WL); }
    /* the party (t135, t142, t150, t177, t191, t221, t233): low polygon people with real faces. y forward, feet at 0 */
    { Mesh m; const uint32_t R = 0xB81C34, R2 = 0x8E1428, SK = 0xE09A70, SK2 = 0xC08054, H = 0xE8B830, H2 = 0xC89820;    /* 0: the wizard, blonde, a long red robe flaring to the floor, belt */
      stackp(m, V(), {{0, 460, 400, -40}, {560, 330, 270, -10}, {1040, 215, 165, 0}, {1120, 215, 170, 0}, {1540, 270, 190, 0}, {1600, 150, 120, 0}}, R, R2);
      stackp(m, V(), {{1040, 222, 172, 0}, {1110, 222, 176, 0}}, 0xD8B040, 0xB89030, false);
      tube(m, V(-255, 0, 1520), V(-300, 90, 1060), 72.f, 62.f, R, R2); tube(m, V(-300, 90, 1060), V(-280, 200, 960), 42.f, 42.f, SK, SK2);
      tube(m, V(255, 0, 1520), V(330, 60, 1080), 72.f, 62.f, R, R2); tube(m, V(330, 60, 1080), V(330, 170, 980), 42.f, 42.f, SK, SK2);
      stackp(m, V(0, 15, 0), {{1560, 62, 62, 0}, {1640, 62, 62, 0}, {1660, 92, 100, 8}, {1800, 106, 112, 8}, {1880, 92, 100, 4}}, SK, SK2, true);
      stackp(m, V(0, -30, 0), {{1400, 135, 80, -30}, {1660, 135, 110, -10}, {1860, 128, 120, 0}, {1935, 80, 96, 10}}, H, H2);
      face(m, V(0, 0, 1760), 128, 96, 0xA87820, 0x902818);
      m.add(L_AV); }
    { Mesh m; const uint32_t R = 0xA01C30, R2 = 0x7E1426, W = 0xEAD8C4, W2 = 0xC8B4A0, SK = 0xE09A70, SK2 = 0xC08054, H = 0xE8B830, H2 = 0xC89820;   /* 1: the elf, blonde, a sleeveless red tunic with a dark edge over white sleeves, a star shield, sword */
      stackp(m, V(), {{800, 250, 165, 0}, {1000, 220, 150, 0}, {1200, 210, 150, 0}, {1540, 280, 180, 0}, {1590, 140, 100, 0}}, R, R2);
      stackp(m, V(), {{960, 225, 155, 0}, {1020, 225, 155, 0}}, 0x30241C, 0x241A14, false);
      for (int k = -1; k <= 1; k += 2) { tube(m, V(k * 300.f, 0, 1520), V(k * 330.f, 40, 1090), 64.f, 52.f, W, W2); tube(m, V(k * 330.f, 40, 1090), V(k * 325.f, 130, 960), 42.f, 42.f, SK, SK2); }
      stackp(m, V(0, 15, 0), {{1560, 60, 60, 0}, {1640, 60, 60, 0}, {1650, 88, 98, 8}, {1790, 100, 110, 8}, {1860, 88, 98, 4}}, SK, SK2);
      stackp(m, V(0, -30, 0), {{1420, 125, 70, -30}, {1660, 130, 105, -10}, {1850, 120, 115, 0}, {1915, 74, 92, 10}}, H, H2);
      for (int k = -1; k <= 1; k += 2) polyv(m, {V(k * 95.f, 40, 1790), V(k * 95.f, 40, 1690), V(k * 200.f, 10, 1830)}, SK, V(k * 2000.f, 0, 1700));   /* pointed ears */
      face(m, V(0, 0, 1745), 124, 90, 0xA87820, 0x902818);
      star_shield(m, V(-400, 170, 1000), V(-0.2f, 1, 0.1f), 400.f);
      polyv(m, {V(-90, 182, 1540), V(90, 182, 1540), V(0, 186, 1290)}, SK, V(0, 2000, 1400));   /* the V of bare chest, t134 */
      m.box(V(280, 100, 930), V(380, 170, 960), 0xB89030); tube(m, V(330, 135, 960), V(380, 300, 2360), 16.f, 6.f, 0x9CA0A8, 0x74787F);   /* a long thin sword, t199 */
      m.add(L_AV + 1); }
    { Mesh m; const uint32_t R = 0xA01C30, R2 = 0x7E1426, W = 0xEAD8C4, W2 = 0xC8B4A0, SK = 0xE09A70, SK2 = 0xC08054, HW = 0xE8E4DC, HW2 = 0xB8B4AC;     /* 2: the dwarf, short and wide, white hair and beard, red tunic over white, the star shield */
      stackp(m, V(), {{420, 300, 200, 0}, {620, 270, 190, 0}, {780, 270, 185, 0}, {1080, 320, 200, 0}, {1120, 150, 110, 0}}, R, R2);
      stackp(m, V(), {{640, 276, 192, 0}, {700, 276, 192, 0}}, 0x30241C, 0x241A14, false);
      for (int k = -1; k <= 1; k += 2) { tube(m, V(k * 345.f, 0, 1060), V(k * 380.f, 60, 700), 70.f, 58.f, W, W2); tube(m, V(k * 380.f, 60, 700), V(k * 380.f, 150, 600), 48.f, 48.f, SK, SK2); }
      stackp(m, V(0, 10, 0), {{1100, 120, 120, 8}, {1260, 124, 130, 8}, {1400, 118, 126, 6}, {1460, 86, 96, 2}}, SK, SK2);
      stackp(m, V(0, -35, 0), {{1120, 130, 90, -20}, {1300, 136, 120, -10}, {1440, 124, 125, 0}, {1500, 70, 90, 10}}, HW, HW2);
      stackp(m, V(0, 55, 0), {{860, 40, 40, 28}, {1030, 110, 78, 20}, {1160, 120, 86, 14}, {1230, 110, 80, 10}}, HW, HW2);
      face(m, V(0, 0, 1320), 150, 104, 0xF0EEE4, 0xC8B8A8);
      star_shield(m, V(-420, 280, 720), V(-0.15f, 1, 0.1f), 460.f);
      tube(m, V(380, 150, 600), V(420, 300, 1260), 26.f, 22.f, 0x5A3A20, 0x452C18); m.box(V(280, 270, 1120), V(560, 320, 1400), 0x8C9098, 0x6C7078, 0x5C6068);   /* his axe, BBC t182 */
      m.add(L_AV + 2); }
    { Mesh m; const uint32_t G = 0x5E7034, G2 = 0x4A5A28, GD = 0x3C4A20, SK = 0xD88A5C, SK2 = 0xB87048, HK = 0x1C1410;   /* 3: the thief, green, wide square shoulders, dark hair to the jaw, moustache and goatee, a purple belt */
      stackp(m, V(), {{560, 320, 200, 0}, {900, 260, 165, 0}, {1000, 240, 150, 0}, {1240, 310, 170, 0}, {1540, 400, 195, 0}, {1590, 180, 110, 0}}, G, G2);
      stackp(m, V(), {{960, 248, 158, 0}, {1040, 248, 158, 0}}, 0x7C3C88, 0x5C2C68, false);
      for (int k = -1; k <= 1; k += 2) { tube(m, V(k * 400.f, 0, 1510), V(k * 410.f, 50, 1060), 72.f, 60.f, G, GD); tube(m, V(k * 410.f, 50, 1060), V(k * 405.f, 140, 950), 42.f, 42.f, SK, SK2); }
      stackp(m, V(0, 15, 0), {{1560, 64, 64, 0}, {1640, 64, 64, 0}, {1650, 92, 100, 8}, {1800, 106, 112, 8}, {1880, 92, 100, 4}}, SK, SK2);
      stackp(m, V(0, -25, 0), {{1600, 125, 60, -40}, {1700, 128, 100, -15}, {1860, 122, 120, 0}, {1930, 72, 98, 8}}, HK, HK);
      for (int k = -1; k <= 1; k += 2) polyv(m, {V(k * 108.f, 60, 1640), V(k * 128.f, 60, 1890), V(k * 88.f, 120, 1890), V(k * 86.f, 118, 1680)}, HK, V(k * 2000.f, 1000, 1760));
      face(m, V(0, 0, 1760), 128, 96, HK, 0x2A1810);
      polyv(m, {V(-74, 130, 1712), V(74, 130, 1712), V(40, 130, 1690), V(-40, 130, 1690)}, HK, V(0, 2000, 1700)); polyv(m, {V(-34, 130, 1680), V(34, 130, 1680), V(0, 134, 1600)}, HK, V(0, 2000, 1650));
      m.add(L_AV + 3); }
    { Mesh l; tube(l, V(0, 0, 0), V(0, 10, -420), 78.f, 58.f, 0x7A5228, 0x5C3C1C); tube(l, V(0, 10, -420), V(0, -10, -800), 56.f, 44.f, 0x7A5228, 0x5C3C1C); l.box(V(-55, -60, -860), V(55, 130, -800), 0x2A1C10); l.add(L_AV + 4); }
    { Mesh l; tube(l, V(0, 0, 0), V(0, 10, -220), 80.f, 66.f, 0x7A5228, 0x5C3C1C); tube(l, V(0, 10, -220), V(0, -10, -360), 64.f, 56.f, 0x7A5228, 0x5C3C1C); l.box(V(-65, -70, -420), V(65, 140, -360), 0x2A1C10); l.add(L_AV + 5); }
    /* goblin. No picture is captioned as one with a clear view (Mean Machines Sega 11 p.48 names one, the shot is lost). Taken from the pale hunched creature with the round shield in Amiga Action 31 p.2, which may be something else */
    { Mesh m; const uint32_t SK = 0xD8D0A0, SK2 = 0xB4AC80;
      m.box(V(-170, -110, 520), V(170, 110, 980), SK, SK2, SK2); m.box(V(-175, -115, 430), V(175, 115, 540), 0x6A4A2A); m.box(V(-120, 40, 1000), V(120, 250, 1210), SK, SK2, SK2);
      dbl(m, {V(-120, 120, 1100), V(-120, 120, 1200), V(-330, 80, 1190)}, 0x4A4A3A); dbl(m, {V(120, 120, 1100), V(120, 120, 1200), V(330, 80, 1190)}, 0x4A4A3A);
      m.box(V(-90, 248, 1120), V(-30, 260, 1170), 0x201810); m.box(V(30, 248, 1120), V(90, 260, 1170), 0x201810); m.box(V(-20, 248, 1060), V(20, 290, 1110), SK2);
      limb(m, V(-200, 0, 950), V(-260, 150, 600), 40, SK); limb(m, V(200, 0, 950), V(260, 150, 600), 40, SK);
      limb(m, V(260, 100, 600), V(260, 650, 1050), 55, 0x5A3A1A); m.box(V(200, 600, 1000), V(320, 720, 1130), 0x4A3014);
      { V c(-300, 280, 640); ngon(m, c, V(1, 0, 0), V(0, 0, 1), 400, 400, 12, 0xD8E0D8, c + V(0, 1000, 0)); ngon(m, c - V(0, 10, 0), V(1, 0, 0), V(0, 0, 1), 400, 400, 12, 0x34507A, c - V(0, 1000, 0)); }
      m.add(L_GOB); }
    { Mesh l; l.box(V(-55, -55, -470), V(55, 55, 0), 0xC8C090); l.box(V(-60, -70, -490), V(60, 110, -440), 0x3A2A1A); l.add(L_GOB_LEG); }
    /* wolf (Games-X 44 p.12, on the green outside): blue, pink inside the ears, a white chest and legs, a dark nose, jaws wide with white teeth */
    { Mesh m; const uint32_t B = 0x2C98D0, B2 = 0x2478A8, FC = 0x7CC4E4, WH = 0xD8ECF4;
      m.box(V(-170, -450, 450), V(170, 450, 830), B, B2, B2); m.box(V(-150, 300, 400), V(150, 500, 640), WH);
      m.box(V(-170, 430, 620), V(170, 700, 960), FC, B, B2);
      m.box(V(-110, 700, 720), V(110, 960, 860), FC, FC, 0x5A2C2C);                                   /* upper jaw */
      m.box(V(-40, 950, 800), V(40, 1000, 870), 0x34303C);                                            /* nose */
      m.box(V(-100, 700, 500), V(100, 930, 560), 0x5A2C2C, WH, WH); m.box(V(-105, 690, 500), V(105, 705, 760), 0x5A2C2C);                                   /* lower jaw, open */
      for (int k = 0; k < 6; k++) { float x = -80.f + k * 32.f; dbl(m, {V(x, 950, 722), V(x + 18, 950, 722), V(x + 9, 950, 660)}, 0xF4F4F0); dbl(m, {V(x, 920, 562), V(x + 18, 920, 562), V(x + 9, 920, 610)}, 0xF4F4F0); }
      for (int k = -1; k <= 1; k += 2) { m.box(V(k > 0 ? 60.f : -280.f, 540, 950), V(k > 0 ? 280.f : -60.f, 590, 1010), B); polyv(m, {V(k * 60.f, 592, 1000), V(k * 280.f, 592, 1000), V(k * 300.f, 566, 1200)}, 0xC89888, V(0, 3000, 1000)); polyv(m, {V(k * 60.f, 538, 1000), V(k * 280.f, 538, 1000), V(k * 300.f, 566, 1200)}, B2, V(0, -3000, 1000)); polyv(m, {V(k * 60.f, 592, 1000), V(k * 300.f, 566, 1200), V(k * 60.f, 538, 1000)}, B2, V(0, 566, 3000)); fq(m, V(k * 150.f, 702, 880), V(k * 70.f, 702, 860), V(k * 70.f, 702, 900), V(k * 150.f, 702, 920), 0x20202A, V(0, 3000, 900)); }
      limb(m, V(0, -450, 800), V(0, -950, 650), 50, B2); m.add(L_WOLF); }
    { Mesh l; l.box(V(-45, -45, -450), V(45, 45, 0), 0xD8ECF4); l.add(L_WOLF_LEG); }
    /* ogre (Amiga Action 31 p.2 close up; Tilt 103 p.41: kill it for a key): red, a square head, a dark band across the eyes, white eyes, a dark wedge of nose, a slit mouth with two tusks pointing up, the gold key on his shoulder */
    { Mesh m; const uint32_t SK = 0xD83A30, SK2 = 0xA82C28, DK = 0x2A2022;
      m.box(V(-430, -250, 1100), V(430, 250, 2050), SK, SK2, SK2); m.box(V(-390, -260, 900), V(390, 260, 1150), 0x6A4A2A); m.box(V(-270, -200, 2080), V(270, 230, 2700), SK, SK2, SK2);
      V fr(0, 3000, 2400); const float Y = 234.f;
      fq(m, V(-270, Y, 2440), V(270, Y, 2440), V(270, Y, 2600), V(-270, Y, 2600), DK, fr);
      for (int k = -1; k <= 1; k += 2) { float x = k * 130.f; fq(m, V(x - 62, Y + 3, 2480), V(x + 62, Y + 3, 2480), V(x + 62, Y + 3, 2530), V(x - 62, Y + 3, 2530), 0xF0E8DC, fr); fq(m, V(x - 18, Y + 6, 2485), V(x + 18, Y + 6, 2485), V(x + 18, Y + 6, 2525), V(x - 18, Y + 6, 2525), 0x101010, fr);
          polyv(m, {V(k * 270.f, 0, 2600), V(k * 270.f, 0, 2300), V(k * 390.f, -60, 2640)}, SK2, V(k * 3000.f, 0, 2500)); }
      polyv(m, {V(-90, Y + 2, 2440), V(90, Y + 2, 2440), V(120, Y + 2, 2300), V(-120, Y + 2, 2300)}, 0x5A2C30, fr);
      fq(m, V(-190, Y + 2, 2230), V(190, Y + 2, 2230), V(190, Y + 2, 2260), V(-190, Y + 2, 2260), 0x8A1818, fr);
      for (int k = -1; k <= 1; k += 2) polyv(m, {V(k * 150.f - 22, Y + 4, 2228), V(k * 150.f + 22, Y + 4, 2228), V(k * 150.f, Y + 4, 2330)}, 0xF0E8C8, fr);
      limb(m, V(-470, 0, 1950), V(-560, 200, 1250), 130, SK); limb(m, V(470, 0, 1950), V(560, 500, 1450), 130, SK);
      limb(m, V(560, 400, 1100), V(560, 1500, 1800), 150, 0x5A3A1A); for (int k = 0; k < 4; k++) cone(m, V(560, 1200.f + k * 100.f, 1500.f + k * 90.f), 60, 150, 0x8A8A8A, 4);
      { V o(420, 262, 1750); const uint32_t KY = 0xF0C030; limb(m, o, o + V(0, 0, -380), 22, KY); ngon(m, o + V(0, 4, 90), V(1, 0, 0), V(0, 0, 1), 90, 90, 8, 0xE87020, o + V(0, 2000, 0)); m.box(o + V(0, -4, -380), o + V(90, 14, -340), KY); m.box(o + V(0, -4, -300), o + V(70, 14, -265), KY); }
      m.add(L_OGRE); }
    /* the long green scaled thing (Amiga Action 31 p.2): an arching neck or tail with a row of pale plates. No text describes it; what it does here is INVENTED */
    { Mesh m; const int N = 12; V prev; for (int i = 0; i <= N; i++) { float q = (float)i / N, a = q * LPI * 0.92f; V p(-1500.f * cosf(a), sinf(q * 2.f) * 240.f, 3000.f * sinf(a) * (1.f - q * 0.25f)); float r = 300.f - 170.f * q;
          if (i) { tube(m, prev, p, r + 14.f, r, 0x54A838, 0x3C8428); V mid = (prev + p) * 0.5f; m.box(mid + V(-70, -(r + 60.f), -60), mid + V(70, -(r - 10.f), 60), 0xD8E0B0, 0xB8C090, 0xB8C090); }
          prev = p; }
      m.add(L_SERP); }
    { Mesh l; l.box(V(-170, -170, -1050), V(170, 170, 0), 0xC83430); l.box(V(-190, -220, -1100), V(190, 260, -1000), 0x3A2A1A); l.add(L_OGRE_LEG); }
    /* the giant spider */
    { Mesh m; gem(m, V(0, 300, 800), 330, 350, 280, 0x30241E, 0x241A16); gem(m, V(0, -500, 850), 520, 700, 480, 0x3A2A22, 0x2A1E18); gem(m, V(0, -500, 1100), 330, 480, 260, 0x7A4A1E, 0x5A3418);
      for (int k = 0; k < 8; k++) { float s = (k & 1) ? 1.f : -1.f; m.box(V(s * 60 + (s > 0 ? 0 : -30), 600.f + (k >> 1) * 10.f, 850.f + (k >> 1) * 40.f), V(s * 60 + (s > 0 ? 30 : 0), 650.f + (k >> 1) * 10.f, 900.f + (k >> 1) * 40.f), 0xE02020); }
      limb(m, V(-100, 620, 700), V(-80, 780, 450), 30, 0xE8E4D4); limb(m, V(100, 620, 700), V(80, 780, 450), 30, 0xE8E4D4);
      for (int k = 0; k < 4; k++) for (int sd = 0; sd < 2; sd++) { float s = sd ? 1.f : -1.f, o = (k - 1.5f);
          V root(s * 250, 300 - k * 200, 700), knee(s * 1000.f, 300.f - k * 200.f + o * 500.f, 1250.f), foot(s * 1700.f, 300.f - k * 200.f + o * 800.f, 0.f);
          limb(m, root, knee, 45, 0x2A2020); limb(m, knee, foot, 38, 0x5A3418); }
      m.add(L_SPIDER); }
}

/* ---- sounds: INVENTED placeholders, nothing of the original's sound survives in the footage clearly enough to copy ---- */
static void make_sounds() {
    std::mt19937 r(7); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; }; const float TP = 2.f * LPI;
    make_sound(SN_SWING, 0.25f, [&](float t) { return noise() * sinf(LPI * t / 0.25f) * 0.5f; });
    make_sound(SN_HIT, 0.3f, [&](float t) { return (noise() * 0.7f + sinf(TP * 180.f * t) * 0.6f) * expf(-t * 18.f); });
    make_sound(SN_CLANG, 0.5f, [&](float t) { return (sinf(TP * 1250.f * t) + sinf(TP * 1870.f * t) * 0.6f) * expf(-t * 9.f) * 0.5f; });
    make_sound(SN_RATTLE, 0.6f, [&](float t) { return noise() * (fmodf(t, 0.07f) < 0.02f ? 1.f : 0.f) * expf(-t * 3.f) * 0.6f; });
    make_sound(SN_FIRE, 0.7f, [&](float t) { return noise() * sinf(LPI * t / 0.7f) * 0.45f + sinf(TP * (200.f + 500.f * t) * t) * 0.2f; });
    make_sound(SN_BOOM, 0.9f, [&](float t) { return noise() * expf(-t * 5.f) * 0.8f + sinf(TP * 55.f * t) * expf(-t * 4.f); });
    make_sound(SN_CREAK, 0.8f, [&](float t) { return sinf(TP * (180.f + 60.f * sinf(t * 9.f)) * t) * 0.35f * sinf(LPI * t / 0.8f); });
    make_sound(SN_CHIME, 0.9f, [&](float t) { return (sinf(TP * 880.f * t) + sinf(TP * 1320.f * t) * 0.6f + sinf(TP * 1760.f * t) * 0.4f) * expf(-t * 3.f) * 0.4f; });
    make_sound(SN_SQUEAK, 0.15f, [&](float t) { return sinf(TP * (3000.f + 2000.f * t) * t) * expf(-t * 20.f) * 0.5f; });
    make_sound(SN_GROWL, 0.7f, [&](float t) { return (sinf(TP * 70.f * t) * (0.6f + 0.4f * sinf(TP * 25.f * t)) + noise() * 0.2f) * sinf(LPI * t / 0.7f) * 0.8f; });
    make_sound(SN_ROAR, 1.2f, [&](float t) { return (sinf(TP * 45.f * t) * (0.6f + 0.4f * sinf(TP * 18.f * t)) + noise() * 0.3f) * sinf(LPI * t / 1.2f); });
    make_sound(SN_KEY, 0.6f, [&](float t) { return (sinf(TP * 2400.f * t) + sinf(TP * 3100.f * t) * 0.5f) * expf(-t * 14.f) * 0.5f; });
    make_sound(SN_CLATTER, 0.6f, [&](float t) { return noise() * (fmodf(t, 0.09f) < 0.03f ? 1.f : 0.f) * expf(-t * 3.f) * 0.7f; });
    make_sound(SN_HISS, 0.6f, [&](float t) { return noise() * 0.35f * sinf(LPI * t / 0.6f); });
    make_sound(SN_HURT, 0.3f, [&](float t) { return sinf(TP * (300.f - 400.f * t) * t) * expf(-t * 8.f) * 0.7f; });
    make_sound(SN_WIN, 1.4f, [&](float t) { static const float fq4[4] = {523.f, 659.f, 784.f, 1047.f}; int k = (std::min)((int)(t / 0.25f), 3); return sinf(TP * fq4[k] * t) * expf(-fmodf(t, 0.25f) * 6.f) * 0.5f; });
    make_sound(SN_START, 0.6f, [&](float t) { return sinf(TP * (440.f + 300.f * t) * t) * expf(-t * 3.f) * 0.6f; });
    make_sound(SN_THUD, 0.35f, [&](float t) { return sinf(TP * 55.f * t) * expf(-t * 9.f); });
}
static void sfx(int ch, uint32_t key, int vol, int pan = 0) { if (sound_on) buggy::play(ch, (int)key, vol, pan); }
static V ear_at;
static void sfx_at(int ch, uint32_t key, int vol, V at) { float d = len(at - ear_at); sfx(ch, key, (int)(vol * clampf(3000.f / (std::max)(d, 1.f), 0.15f, 1.f))); }   /* sound falls with distance (Micro User 110, Tilt 103); the curve is INVENTED */

/* ---- HUD bitmaps used only for the effects (the red flash of a hit, the white flash of the mirror) ---- */
static void make_flash() {
    std::vector<uint32_t> r(16, 0xFF1818F0u), w(16, 0xFFFFFFFFu), g(16, 0xFF4A5450u);   /* RGBA words as in make_hud (A B G R) */
    add_bitmap(BM_RED, 4, 4, r); add_bitmap(BM_WHITE, 4, 4, w); add_bitmap(BM_DIM, 4, 4, g);
}
static void flash_blit(VPIX_Frame &f, int id, float a) {
    VPIX_Blit b = {}; b.handle = d1k::BMPH + id; b.sw = 4; b.sh = 4; b.x = 0; b.y = 0; b.dw = 352.f; b.dh = 288.f; b.key = 0; b.alpha = a; b.prio = 1000000;
    b.tint[0] = b.tint[1] = b.tint[2] = 1.f; f.blits.push_back(b);
}

/* ---- game state ---- */
enum State { S_TITLE, S_PLAY, S_OVER };
static State st = S_TITLE; static float st_t = 0; static double sim_t = 0;
static int end_reason = 0;   /* 1 fallen, 2 quest complete, 3 time up */
static bool autoplay = false; static float time_left = 0;
struct Pl { V p; float yaw = 0, vz = 0, hp = 100, flash = 0, dim = 0, white = 0, dead_t = 0, pit_t = 0, hold_t = 0, cast_cd = 0, thrust_t = 0, hit_cd = 0, shimmer = 0, book_view = 0;
            bool alive = true, falling = false, in_pit = false, book = false, sword2 = false, key = false; int weap = 0; };   /* weap: 0 empty hand, 1 blade (dagger, or the sword once found), 2 bow */
static Pl P;
static V walk_off, last_hxy; static bool have_off = false;
struct Ctl { V eye, hand, hfw; M head, hrot; bool walk = false, trig = false, trig_edge = false, block = false; int draw = -1; };
static Ctl ctl;
static float swing_v = 0, sw_acc = 0; static V tip_rel; static bool sw_have = false;
static float sword_len() { return P.sword2 ? 1050.f : 420.f; }

struct Spark { V p, v; float life; int id; };
static std::vector<Spark> sparks;
static void burst(V at, int n, int id, float speed, float life) {
    for (int i = 0; i < n && sparks.size() < 520; i++) { V d = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-0.3f, 1.2f))); sparks.push_back({at, d * (speed * rnd(0.3f, 1.f)), life * rnd(0.6f, 1.f), id}); }
}
struct Fire { V p, v; bool on = false; float t = 0; };
static Fire fb, arrow;
static std::vector<V> remains;   /* the bones skeletons leave (t199) */

/* world props */
static bool chest_open = false, book_taken = false, big_open = false, key_door_open = false, key_drop = false, spider_dead = false, skel_spawned = false, skel_dead = false;
static float chest_t = 0, big_t = 0, key_door_t = 0, door_t = 0, locked_msg = 0, low_t = 0, book_t = 0; static bool low_open = false, pile_risen = false, room3_risen = false; static int door_state = 0; static V key_pos;
static int kills = 0;

/* monsters */
enum { T_SKEL, T_BAT, T_GOB, T_WOLF, T_OGRE, T_SPIDER, T_SERP, NMT };
struct MDef { float hp, speed, reach, dmg, cd, rad, h, aggro, windup; int body, leg; };
static MDef MD[NMT] = {
    {30, 900, 1300, 10, 1.7f, 350, 2140, 6000, 0.55f, L_SKEL, L_SKEL_LEG},    /* INVENTED numbers throughout; only the skeleton's "gone in about 5 s" is from footage */
    {8, 3200, 520, 4, 1.4f, 260, 300, 7000, 0.f, L_BAT, 0},
    {20, 1700, 1000, 8, 1.2f, 300, 1300, 6500, 0.45f, L_GOB, L_GOB_LEG},
    {25, 3200, 900, 9, 1.0f, 380, 900, 4000, 0.35f, L_WOLF, L_WOLF_LEG},
    {100, 1100, 1700, 22, 2.2f, 520, 2800, 5500, 0.9f, L_OGRE, L_OGRE_LEG},
    {120, 1900, 1700, 15, 1.3f, 800, 1500, 7500, 0.5f, L_SPIDER, 0},
    {60, 0, 1900, 10, 1.6f, 420, 2900, 3500, 0.5f, L_SERP, 0}};
struct LegSet { int n; V off[4]; float hipz; };
static const LegSet LEGS[NMT] = {{2, {V(-150, 0, 0), V(150, 0, 0), V(), V()}, 1050.f}, {0, {V(), V(), V(), V()}, 0.f}, {2, {V(-100, 0, 0), V(100, 0, 0), V(), V()}, 470.f},
    {4, {V(-120, 330, 0), V(120, 330, 0), V(-120, -330, 0), V(120, -330, 0)}, 450.f}, {2, {V(-250, 0, 0), V(250, 0, 0), V(), V()}, 1100.f}, {0, {V(), V(), V(), V()}, 0.f}, {0, {V(), V(), V(), V()}, 0.f}};
struct Mon { int type = 0; V p; float yaw = 0, hp = 1, atk = 0, wind = 0, dying = 0, flash = 0, hit_cd = 0, phase = 0, ang = 0, rise = 0; bool awake = false, alive = true, struck = false; };
static std::vector<Mon> mons;
static const float RISE_T = 2.f;   /* INVENTED: Amiga Computing 69 p.35 says skeletons rise from the floor, not how fast */
static void spawn(int type, V p) { Mon m; m.type = type; m.p = p; m.hp = MD[type].hp * mon_hp_scale; m.phase = rnd(0, 6); m.ang = rnd(0, 6.28f); mons.push_back(m); }

/* the party: in the footage three other players; here they walk with you and do not fight (INVENTED, solo) */
struct Ally { V p; float yaw = 0, phase = 0, arrive = 0, hit_t = 0; int look = 1; bool moving = false, walking_in = false, human = false, hidden = false; };   /* human: a linked pod plays this seat; hidden: it has fallen / it is you */
static std::vector<Ally> allies;
static float dist_xy(V a, V b) { float dx = a.x - b.x, dy = a.y - b.y; return sqrtf(dx * dx + dy * dy); }
static void hurt(float dmg) {
    if (!P.alive || dmg <= 0) return;
    P.hp -= dmg; P.flash = 0.6f; P.dim = dim_t; sfx(9, SN_HURT, 110); VPIX_Haptic(2, 0.12f, 0.8f);
    dl("t=%.1f hurt %.0f, hp %.0f", sim_t, dmg, P.hp);
    if (P.hp <= 0) { P.alive = false; P.dead_t = 0; dl("t=%.1f player fallen", sim_t); }
}
/* ---- link players (vpix_1k_net.h): seat 0 = P (pod 1); seats 1..3 = allies[0..2] when a pod plays them, else the CPU party member ---- */
struct Rem { bool on = false, alive = true, swing = false, block = false, falling = false, walk = false, autop = false, click = false; V p, eye, hand, hfw, hdf; float yaw = 0, hp = 100; int weap = 0; uint8_t fseq = 0, aseq = 0, cseq = 0, pseq = 0, fdone = 0, adone = 0; };
static Rem rem[4];
static uint8_t net_pit_seq = 0; static bool net_slave_game = false;   /* slave: I am inside the master's round */
static bool hum_alive(int s) { return s == 0 ? P.alive : (rem[s].on && rem[s].alive); }
static V hum_pos(int s) { return s == 0 ? P.p : rem[s].p; }
static V hum_eye(int s) { return s == 0 ? ctl.eye : rem[s].eye; }
static int nearest_hum(V at, float &dist) { int b = -1; dist = 1e30f; for (int s = 0; s < 4; s++) if (hum_alive(s)) { float d = dist_xy(at, hum_pos(s)); if (d < dist) { dist = d; b = s; } } return b; }
static bool any_h(const std::function<bool(V)> &f, bool skip_falling = false) { for (int s = 0; s < 4; s++) if (hum_alive(s) && !(skip_falling && (s == 0 ? P.falling : rem[s].falling)) && f(hum_pos(s))) return true; return false; }
static bool all_dead() { for (int s = 0; s < 4; s++) if (hum_alive(s)) return false; return true; }
static void hurt_seat(int s, float dmg) {
    if (s == 0) { hurt(dmg); return; }
    Rem &r = rem[s]; if (!r.on || !r.alive || dmg <= 0) return;
    r.hp -= dmg; dl("t=%.1f seat %d hurt %.0f, hp %.0f", sim_t, s, dmg, r.hp);
    if (r.hp <= 0) { r.alive = false; dl("t=%.1f seat %d fallen", sim_t, s); }
}
static void teleport(V xy) {
    float z = Z1; floor_at(xy.x, xy.y, z); P.p = V(xy.x, xy.y, z); P.vz = 0; P.falling = false;
    if (!autoplay) walk_off = V(xy.x, xy.y, 0) - last_hxy;
}
static bool try_move(V &p, V d) {
    V np = p + d; if (walk_ok(np.x, np.y)) { p = np; return true; }
    if (walk_ok(np.x, p.y)) { p.x = np.x; return true; } if (walk_ok(p.x, np.y)) { p.y = np.y; return true; } return false;
}
static void kill_mon(Mon &m) {
    if (!m.alive) return;
    m.alive = false; m.dying = (m.type == T_SKEL) ? vanish_t : 3.f; kills++;   /* non-skeleton vanish time INVENTED */
    burst(m.p + V(0, 0, MD[m.type].h * 0.5f), m.type == T_SKEL ? 150 : 40, L_GOLD, 1500.f, m.type == T_SKEL ? vanish_t : 2.5f);
    if (m.type == T_SKEL) burst(m.p + V(0, 0, MD[m.type].h * 0.35f), 150, L_REDSP, 1700.f, vanish_t);
    sfx(3, SN_HIT, 110);
    if (m.type == T_SKEL) { skel_dead = true; if (remains.size() < 12) remains.push_back(m.p); time_left += task_bonus; }
    if (m.type == T_OGRE) { key_drop = true; key_pos = m.p; time_left += task_bonus; dl("t=%.1f ogre drops the key", sim_t); }
    if (m.type == T_SPIDER) { spider_dead = true; sfx(5, SN_WIN, 100); dl("t=%.1f spider dead, the mirror glows", sim_t); }
    dl("t=%.1f %s killed (kills %d)", sim_t, m.type == T_SKEL ? "skeleton" : m.type == T_BAT ? "bat" : m.type == T_GOB ? "goblin" : m.type == T_WOLF ? "wolf" : m.type == T_OGRE ? "ogre" : m.type == T_SERP ? "serpent" : "spider", kills);
}
static void hit_mon(Mon &m, float dmg, V from) {
    m.hp -= dmg; m.flash = 0.25f; V d = norm(V(m.p.x - from.x, m.p.y - from.y, 0)); if (m.type != T_OGRE && m.type != T_SPIDER && m.type != T_SERP) try_move(m.p, d * 150.f);
    burst(m.p + V(0, 0, MD[m.type].h * 0.6f), 8, L_REDSP, 1800.f, 0.6f);
    if (m.hp <= 0) kill_mon(m);
}

static bool reach_pt(V pt, float r) {
    if (autoplay) return len(ctl.eye - pt) < 2200.f;   /* INVENTED shortcut for the headless test player */
    if (len(ctl.hand - pt) < r) return true;
    if (ctl.trig_edge && len(pt - ctl.eye) < 1700.f && dot(ctl.head.r[1], norm(pt - ctl.eye)) > 0.75f) return true;   /* desktop: look and click */
    return false;
}

/* master: anyone's hand (or look and click) reaching a thing */
static bool reach_any(V pt, float r) {
    if (P.alive && reach_pt(pt, r)) return true;
    for (int s = 1; s < 4; s++) { const Rem &m = rem[s]; if (!m.on || !m.alive) continue;
        if (m.autop) { if (len(m.eye - pt) < 2200.f) return true; continue; }
        if (len(m.hand - pt) < r) return true;
        if (m.click && len(pt - m.eye) < 1700.f && dot(m.hdf, norm(pt - m.eye)) > 0.75f) return true; }
    return false;
}

/* ---- the cheap autopilot: LEGEND_AUTO=1 plays the game itself ---- */
enum { A_MOVE, A_KILLSKEL, A_KILL, A_CHEST, A_CAST, A_KEYPICK, A_KEYDOOR, A_MIRROR };
struct WP { float x, y; int act; };
static std::vector<WP> wps; static int aw = 0; static float aw_t = 0, auto_cyc = 0; static Mon *auto_tm = NULL;
static void build_route() {
    wps.clear();
    wps.push_back({0, 1200, A_MOVE}); wps.push_back({0, 4800, A_MOVE}); wps.push_back({0, 5600, A_MOVE}); wps.push_back({0, 9500, A_MOVE}); wps.push_back({0, 11500, A_KILLSKEL});
    wps.push_back({500, 12650, A_CHEST}); wps.push_back({0, 14200, A_CAST}); wps.push_back({0, 16600, A_MOVE}); wps.push_back({0, 20000, A_KILL}); wps.push_back({0, 25800, A_MOVE});
    wps.push_back({0, 29000, A_KILL}); wps.push_back({-2500, 29700, A_MOVE}); wps.push_back({-5500, 29700, A_MOVE}); wps.push_back({-9500, 29700, A_KILL}); wps.push_back({0, 0, A_KEYPICK});
    wps.push_back({-5500, 29700, A_MOVE}); wps.push_back({-2500, 29700, A_MOVE}); wps.push_back({0, 31800, A_KEYDOOR}); wps.push_back({0, 34600, A_MOVE});
    float sx = TRAP_X0 + (safe_col + 0.5f) * TILE;
    wps.push_back({sx, 34800, A_MOVE}); wps.push_back({sx, 41500, A_MOVE}); wps.push_back({0, 42100, A_MOVE}); wps.push_back({0, 43000, A_MOVE});
    wps.push_back({0, 46000, A_MOVE}); wps.push_back({0, 48200, A_KILL}); wps.push_back({0, 51000, A_MIRROR});
    aw = 0; aw_t = 0;
}
static Mon *nearest_awake(float maxd) {
    Mon *b = NULL; float bd = maxd;
    for (Mon &m : mons) if (m.alive && m.awake && m.type != T_SERP) { float d = len(V(m.p.x - P.p.x, m.p.y - P.p.y, 0)); if (d < bd) { bd = d; b = &m; } }
    return b;
}

/* ---- the world step ---- */
static bool trig_prev = false;
static void make_ctl_from_input(const VPIX_Input &in, float dt) {
    V hxy(in.head.p[0], in.head.p[1], 0); last_hxy = hxy;
    if (!have_off) { walk_off = V(P.p.x, P.p.y, 0) - hxy; have_off = true; }
    V f = norm(V(in.head.R[1][0], in.head.R[1][1], 0.0001f));
    bool walking = !P.falling && d1k::desk_walk(in, f, walk_speed, dt, walk_off);   /* the sources lean to looking where you want to go */
    V cand(walk_off.x + hxy.x, walk_off.y + hxy.y, P.p.z), p = cand;
    resolve(p, P.p); walk_off += p - cand;
    P.p.x = p.x; P.p.y = p.y; P.yaw = atan2f(-f.x, f.y);
    if (in.head.valid) {
        ctl.eye = V(walk_off.x + in.head.p[0], walk_off.y + in.head.p[1], P.p.z + in.head.p[2]);
        for (int k = 0; k < 3; k++) ctl.head.r[k] = V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2]);
    } else { ctl.eye = V(P.p.x, P.p.y, P.p.z + 1700.f); ctl.head = yawed(P.yaw); }
    if (in.hand.valid) {
        ctl.hand = V(walk_off.x + in.hand.p[0], walk_off.y + in.hand.p[1], P.p.z + in.hand.p[2]);
        for (int k = 0; k < 3; k++) ctl.hrot.r[k] = V(in.hand.R[k][0], in.hand.R[k][1], in.hand.R[k][2]);
    } else { ctl.hrot = ctl.head; ctl.hand = ctl.eye + ctl.head.r[1] * 900.f + ctl.head.r[0] * 190.f - ctl.head.r[2] * 300.f; }   /* no tracked hand: hang it off the head */
    ctl.hfw = norm(ctl.hrot.r[1]);
    ctl.walk = walking; bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f;
    ctl.trig_edge = trig && !trig_prev; ctl.trig = trig; trig_prev = trig;
    ctl.block = (in.buttons & VPIX_BTN_3) != 0 || ctl.hfw.z > 0.85f;   /* grip, or the sword held upright */
    /* weapons live on your body: touch your belt for the blade, your shoulder for the bow (GamePro 40, Tilt 103 p.41, Amiga Action 31 p.2, Games-X 44 p.13). The trigger takes it. No tracked hand: R or the fourth button cycles (a stand-in, not the original) */
    ctl.draw = -1;
    if (in.hand.valid) { if (ctl.trig_edge) { V rt = ctl.head.r[0], belt = ctl.eye - V(0, 0, 800) + rt * 180.f, sho = ctl.eye + rt * 220.f - V(0, 0, 200);
            if (len(ctl.hand - belt) < 300.f) ctl.draw = P.weap == 1 ? 0 : 1; else if (len(ctl.hand - sho) < 250.f) ctl.draw = P.weap == 2 ? 0 : 2;
            if (ctl.draw >= 0) ctl.trig_edge = false; } }
    else { static bool cyc_was = false; bool cyc = (in.buttons & VPIX_BTN_4) != 0 || (GetAsyncKeyState('R') & 0x8000) != 0; if (cyc && !cyc_was) ctl.draw = (P.weap + 1) % 3; cyc_was = cyc; }
}
static void make_ctl_auto(float dt) {
    { static const char *idle = getenv("LEGEND_IDLE");   /* dev: stand still for a picture, LEGEND_IDLE=yaw in degrees */
      if (idle && *idle) { P.yaw = (float)atof(idle) * LPI / 180.f; ctl.walk = false; ctl.eye = P.p + V(0, 0, 1700.f); ctl.head = yawed(P.yaw); ctl.hand = ctl.eye + ctl.head.r[1] * 900.f + ctl.head.r[0] * 190.f - ctl.head.r[2] * 300.f; ctl.hrot = ctl.head; ctl.hfw = ctl.head.r[1]; ctl.trig = ctl.trig_edge = ctl.block = false; swing_v = 0; return; } }
    if (aw >= (int)wps.size()) aw = (int)wps.size() - 1;
    WP &w = wps[aw]; aw_t += dt;
    if (w.act == A_KEYPICK && key_drop) { w.x = key_pos.x; w.y = key_pos.y; }
    V to(w.x - P.p.x, w.y - P.p.y, 0); float d = len(to);
    auto_tm = nearest_awake(3500.f);
    bool fighting = auto_tm != NULL; float dm = fighting ? len(V(auto_tm->p.x - P.p.x, auto_tm->p.y - P.p.y, 0)) : 1e9f;
    V face = fighting ? V(auto_tm->p.x - P.p.x, auto_tm->p.y - P.p.y, 0) : to;
    if (w.act == A_CHEST && d < 400.f) face = V(1, 0, 0);
    if (w.act == A_CAST && d < 400.f && !fighting) face = V(0, 1000, 0);
    if (len(face) > 1.f) { float want = atan2f(-face.x, face.y); float dy = want - P.yaw; while (dy > LPI) dy -= 2 * LPI; while (dy < -LPI) dy += 2 * LPI; P.yaw += clampf(dy, -6.f * dt, 6.f * dt); }
    bool move = d > 250.f && !(fighting && dm < 1100.f) && !P.falling;
    if (w.act == A_KILL && d <= 350.f && fighting) move = dm > 1100.f;
    if (move) { V dir = norm(to), was = P.p, p = P.p + dir * (walk_speed * dt); resolve(p, was); P.p.x = p.x; P.p.y = p.y; }
    ctl.walk = move;
    ctl.eye = P.p + V(0, 0, 1700.f);
    V fwd = V(-sinf(P.yaw), cosf(P.yaw), 0); if (fighting) fwd = norm(V(auto_tm->p.x - ctl.eye.x, auto_tm->p.y - ctl.eye.y, (auto_tm->p.z + (auto_tm->type == T_BAT ? 0.f : MD[auto_tm->type].h * 0.5f)) - ctl.eye.z));
    ctl.head = look_at(ctl.eye, ctl.eye + fwd);
    ctl.hand = ctl.eye + ctl.head.r[1] * 900.f + ctl.head.r[0] * 190.f - ctl.head.r[2] * 300.f; ctl.hrot = ctl.head; ctl.hfw = ctl.head.r[1];
    if (fighting) { V tgt = auto_tm->p + V(0, 0, auto_tm->type == T_BAT ? 0.f : MD[auto_tm->type].h * 0.45f); ctl.hfw = norm(tgt - ctl.hand); ctl.hrot = look_at(ctl.hand, ctl.hand + ctl.hfw); }
    auto_cyc += dt; ctl.trig = false; ctl.trig_edge = false;
    P.weap = (w.act == A_CAST && !fighting && P.book && !gate_open[G_DOOR1]) ? 0 : 1;
    if (w.act == A_CAST && d < 400.f && P.book && !gate_open[G_DOOR1]) { float c = fmodf(auto_cyc, 1.4f); ctl.trig = c < 0.7f; ctl.trig_edge = c < dt; }
    ctl.block = false;
    // the swing: forced, on a 0.5 s cycle while something is in reach
    if (fighting && dm < 1500.f) swing_v = fmodf(auto_cyc, 0.5f) < 0.25f ? 3200.f : 0.f; else swing_v = 0.f;
    // arrival logic
    bool done = false;
    switch (w.act) {
    case A_MOVE: done = d < 300.f; break;
    case A_KILLSKEL: done = skel_dead || aw_t > 40.f; break;
    case A_KILL: done = (d < 400.f && !nearest_awake(6500.f)) || aw_t > 35.f; break;
    case A_CHEST: done = P.book || aw_t > 14.f; break;
    case A_CAST: done = gate_open[G_DOOR1] || aw_t > 30.f; break;
    case A_KEYPICK: done = P.key || aw_t > 40.f; break;
    case A_KEYDOOR: done = gate_open[G_KEY] || aw_t > 20.f; break;
    case A_MIRROR: done = false; break;
    }
    if (done && aw + 1 < (int)wps.size()) { aw++; aw_t = 0; }
}
static void door_hit() {
    if (door_state != 0) return;
    door_state = 1; door_t = 0; sfx(4, SN_FIRE, 110); dl("t=%.1f fireball hits the door", sim_t);
}
static void explode(V at, bool dmg) {
    burst(at, 36, L_GOLD, 3000.f, 1.2f); burst(at, 20, L_REDSP, 2200.f, 1.0f); sfx(4, SN_BOOM, 110);
    if (dmg) for (Mon &m : mons) if (m.alive && len(V(m.p.x - at.x, m.p.y - at.y, 0)) < 900.f + MD[m.type].rad) hit_mon(m, 40.f * dmg_scale, at);
}
static void weapon_step(float dt) {
    if (P.hit_cd > 0) P.hit_cd -= dt;
    if (P.cast_cd > 0) P.cast_cd -= dt;
    if (P.thrust_t > 0) P.thrust_t -= dt;
    if (P.flash > 0) P.flash -= dt;
    if (P.dim > 0) P.dim -= dt;
    if (P.book_view > 0) P.book_view -= dt;
    if (ctl.draw >= 0) { P.weap = ctl.draw; sfx(8, SN_SWING, 70); ctl.draw = -1; }
    if (P.shimmer > 0) P.shimmer -= dt;
    if (P.white > 0) P.white -= dt;
    /* real swing: tip speed measured about 30 times a second, relative to the eye so walking doesn't count */
    sw_acc += dt;
    if (!autoplay && sw_acc >= 1.f / 30.f) {
        V rel = ctl.hand + ctl.hfw * sword_len() - ctl.eye;
        if (sw_have) swing_v += ((len(rel - tip_rel) / sw_acc) - swing_v) * 0.6f; else swing_v = 0;
        tip_rel = rel; sw_have = true; sw_acc = 0;
    } else if (sw_acc >= 1.f / 30.f) sw_acc = 0;
    if (ctl.trig_edge && P.weap == 1) { P.thrust_t = 0.28f; sfx(8, SN_SWING, 90); }   /* the trigger lunges (Mean Machines Sega 11 p.48: "Lunge to attack") */
    if (ctl.trig_edge && P.weap == 2 && !arrow.on) { arrow.on = true; arrow.t = 0; arrow.p = ctl.hand + ctl.hfw * 300.f; arrow.v = ctl.hfw * 9000.f; sfx(8, SN_SWING, 90); dl("t=%.1f arrow", sim_t); }   /* arrow speed and damage INVENTED */
    if (swing_v > swing_min && !autoplay && P.weap == 1 && fmodf((float)sim_t, 0.4f) < dt) sfx(8, SN_SWING, 80);
    /* fireball: the book taken, an empty hand: hold the trigger and the flame sits on your palm (t205-t206), let go to throw. "Press joystick" (speech 3:40) */
    if (ctl.trig && P.book && P.weap == 0 && P.book_view <= 0) P.hold_t += dt;
    else { if (P.hold_t > 0.4f && P.book && P.cast_cd <= 0 && !fb.on) {
              fb.on = true; fb.t = 0; fb.p = ctl.hand + ctl.hfw * 350.f; fb.v = ctl.hfw * 7000.f; P.cast_cd = cast_gap; sfx(4, SN_FIRE, 110); VPIX_Haptic(0, 0.15f, 0.9f);
              dl("t=%.1f fireball thrown", sim_t); }
           P.hold_t = 0; }
    if (P.hold_t > 1.2f && P.cast_cd <= 0 && !fb.on) { fb.on = true; fb.t = 0; fb.p = ctl.hand + ctl.hfw * 350.f; fb.v = ctl.hfw * 7000.f; P.cast_cd = cast_gap; P.hold_t = 0; sfx(4, SN_FIRE, 110); dl("t=%.1f fireball thrown", sim_t); }
    /* sword against monsters */
    bool active = (swing_v > swing_min || P.thrust_t > 0) && P.weap == 1;
    if (active && P.alive) {
        float L = sword_len(), dmg = (P.sword2 ? 15.f : 10.f);
        for (Mon &m : mons) {
            if (!m.alive || m.hit_cd > 0) continue;
            const MDef &d = MD[m.type]; bool hit = false;
            for (int k = 1; k <= 4 && !hit; k++) { V s = ctl.hand + ctl.hfw * (L * k / 4.f); if (P.thrust_t > 0) s = s + ctl.hfw * 250.f;
                float hd = len(V(s.x - m.p.x, s.y - m.p.y, 0));
                bool zin = m.type == T_BAT ? fabsf(s.z - m.p.z) < 380.f : (s.z > m.p.z - 60.f && s.z < m.p.z + d.h + 60.f);
                if (hd < d.rad + 90.f && zin) hit = true; }
            if (hit) { m.hit_cd = 0.45f; if (!m.awake) m.awake = true; hit_mon(m, dmg * dmg_scale, P.p); sfx(2, SN_HIT, 105); VPIX_Haptic(0, 0.08f, 0.8f); }
        }
    }
}
static void fire_step(float dt) {
    if (!fb.on) return;
    fb.p += fb.v * dt; fb.t += dt;
    if (door_state == 0 && fb.p.y > 15900.f && fabsf(fb.p.x) < 950.f && fb.p.z > Z1 && fb.p.z < Z1 + 3200.f) { door_hit(); explode(fb.p, false); fb.on = false; return; }
    for (Mon &m : mons) if (m.alive) { const MDef &d = MD[m.type];
        if (len(V(m.p.x - fb.p.x, m.p.y - fb.p.y, 0)) < d.rad + 220.f && fb.p.z > m.p.z - 200.f && fb.p.z < m.p.z + d.h + 200.f) { if (!m.awake) m.awake = true; explode(fb.p, true); fb.on = false; return; } }
    if (!walk_ok(fb.p.x, fb.p.y) || fb.t > 4.f) { explode(fb.p, true); fb.on = false; }
}
static void arrow_step(float dt) {
    if (!arrow.on) return;
    arrow.p += arrow.v * dt; arrow.t += dt;
    for (Mon &m : mons) if (m.alive) { const MDef &d = MD[m.type];
        if (len(V(m.p.x - arrow.p.x, m.p.y - arrow.p.y, 0)) < d.rad + 80.f && arrow.p.z > m.p.z - 100.f && arrow.p.z < m.p.z + d.h + 100.f) { if (!m.awake) m.awake = true; hit_mon(m, 8.f, P.p); sfx(2, SN_HIT, 90); arrow.on = false; return; } }
    if (!walk_ok(arrow.p.x, arrow.p.y) || arrow.t > 2.5f) arrow.on = false;
}
/* master: what a linked pod's hands do. It sends where its hand is and counts its throws; the hit is worked out here */
static void remote_step(float dt) {
    for (int s = 1; s < 4; s++) { Rem &r = rem[s]; if (!r.on || !r.alive) continue;
        if (r.fseq != r.fdone) { r.fdone = r.fseq; if (P.book && !fb.on) { fb.on = true; fb.t = 0; fb.p = r.hand + r.hfw * 350.f; fb.v = r.hfw * 7000.f; sfx_at(4, SN_FIRE, 110, r.p); dl("t=%.1f seat %d fireball", sim_t, s); } }
        if (r.aseq != r.adone) { r.adone = r.aseq; if (!arrow.on) { arrow.on = true; arrow.t = 0; arrow.p = r.hand + r.hfw * 300.f; arrow.v = r.hfw * 9000.f; } }
        if (r.swing && r.weap == 1) {
            float L = sword_len(), dmg = (P.sword2 ? 15.f : 10.f);
            for (Mon &m : mons) { if (!m.alive || m.hit_cd > 0) continue; const MDef &d = MD[m.type]; bool hit = false;
                for (int k = 1; k <= 4 && !hit; k++) { V q = r.hand + r.hfw * (L * k / 4.f); float hd = len(V(q.x - m.p.x, q.y - m.p.y, 0));
                    bool zin = m.type == T_BAT ? fabsf(q.z - m.p.z) < 380.f : (q.z > m.p.z - 60.f && q.z < m.p.z + d.h + 60.f);
                    if (hd < d.rad + 90.f && zin) hit = true; }
                if (hit) { m.hit_cd = 0.45f; m.awake = true; hit_mon(m, dmg * dmg_scale, r.p); sfx_at(2, SN_HIT, 105, m.p); } } } }
    (void)dt;
}
static void mon_step(Mon &m, float dt) {
    const MDef &d = MD[m.type];
    m.phase += dt; if (m.flash > 0) m.flash -= dt; if (m.hit_cd > 0) m.hit_cd -= dt;
    if (m.rise > 0) { m.rise -= dt; return; }
    if (!m.alive) { m.dying -= dt; if (rnd(0, 1) < dt * 40.f && sparks.size() < 160) sparks.push_back({m.p + V(rnd(-300, 300), rnd(-300, 300), rnd(100, (std::max)(300.f, d.h))), V(rnd(-200, 200), rnd(-200, 200), rnd(300, 900)), rnd(0.8f, 1.6f), (int)L_GOLD}); return; }
    float dist; const int tg = nearest_hum(m.p, dist);   /* the nearest player; alone that is you */
    if (!m.awake) {
        if (tg >= 0 && dist < d.aggro && walk_ok(m.p.x, m.p.y)) { m.awake = true; sfx_at(6, m.type == T_SKEL ? SN_RATTLE : m.type == T_BAT ? SN_SQUEAK : m.type == T_OGRE ? SN_ROAR : (m.type == T_SPIDER || m.type == T_SERP) ? SN_HISS : SN_GROWL, 120, m.p + V(0, 0, 1000));
            dl("t=%.1f monster %d wakes", sim_t, m.type); }
        else return;
    }
    if (tg < 0) return;
    const V tp = hum_pos(tg); V to(tp.x - m.p.x, tp.y - m.p.y, 0); dist = len(to);
    if (m.atk > 0) m.atk -= dt;
    V dir = norm(to);
    if (m.type == T_BAT) {   /* circles the head and bites, flaps near the eyes (KKkZ 233 s) */
        m.ang += 2.4f * dt; V eye = hum_eye(tg), want = eye + V(cosf(m.ang) * 650.f, sinf(m.ang) * 650.f, sinf(m.phase * 3.f) * 260.f);
        V mv = want - m.p; float l = len(mv); if (l > 1.f) { m.p += mv * ((std::min)(l, d.speed * dt) / l); }
        if (m.type != T_SERP) m.yaw = atan2f(-(eye.x - m.p.x), eye.y - m.p.y);
        float de = len(eye - m.p);
        if (de < d.reach && m.atk <= 0) { m.atk = d.cd; hurt_seat(tg, d.dmg * dmg_scale); if (rnd(0, 1) < 0.5f) sfx_at(6, SN_SQUEAK, 90, m.p); }
        return;
    }
    if (m.type != T_SERP) m.yaw = atan2f(-dir.x, dir.y);
    if (m.wind > 0) {
        m.wind -= dt;
        if (m.wind <= 0) { if (dist < d.reach * 1.3f) {
                const V hf = tg == 0 ? ctl.head.r[1] : rem[tg].hdf; const bool blk = tg == 0 ? ctl.block : rem[tg].block;
                V face = norm(V(hf.x, hf.y, 0.0001f)); V u = norm(V(m.p.x - tp.x, m.p.y - tp.y, 0));
                if (blk && dot(face, u) > 0.3f) { hurt_seat(tg, d.dmg * block_k * dmg_scale); sfx(2, SN_CLANG, 110); if (tg == 0) VPIX_Haptic(0, 0.1f, 1.f); }
                else hurt_seat(tg, d.dmg * dmg_scale); }
            m.atk = d.cd; }
    } else {
        if (dist > d.reach * 0.75f) { try_move(m.p, dir * (d.speed * dt)); }
        else if (m.atk <= 0) m.wind = d.windup;
    }
    if (dist < d.rad + 250.f) try_move(m.p, dir * (-(d.rad + 250.f - dist)));   /* don't walk into your body */
}
static void props_step(float dt) {
    if (!all_dead()) {
        /* the big door opens when your first teammate arrives (t106-t110) */
        if (!big_open && st_t > 2.f) { big_open = true; big_t = 0; gate_open[G_BIG] = true; sfx(7, SN_CREAK, 100); dl("t=%.1f big door opens", sim_t); }
        if (!low_open && reach_any(V(0, 5300, 500), 1000.f)) { low_open = true; low_t = 0; sfx(7, SN_CREAK, 100); dl("t=%.1f door at the foot of the steps opened", sim_t); }
        if (reach_any(V(MIRROR_X, MIRROR_Y, 1500), 1000.f) && P.shimmer <= 0 && !autoplay) { P.shimmer = 1.2f; sfx(7, SN_CHIME, 100); dl("t=%.1f castle mirror touched (no other levels built)", sim_t); }
        if (!chest_open && reach_any(V(1500, 12650, Z1 + 700), reach_r)) { chest_open = true; chest_t = 0; sfx(7, SN_CREAK, 100); dl("t=%.1f chest opened", sim_t); }
        if (chest_open && chest_t > 0.6f && !book_taken && reach_any(V(1450, 12650, Z1 + 800), reach_r)) {
            book_taken = true; book_t = 0; P.book = true; P.sword2 = true; P.weap = 0; P.book_view = 3.f; time_left += task_bonus; burst(V(1450, 12650, Z1 + 800), 40, L_GOLD, 2400.f, 1.5f); sfx(7, SN_CHIME, 110); VPIX_Haptic(0, 0.2f, 0.8f);
            dl("t=%.1f spell book taken: fireball, and a better sword (one account)", sim_t); }
        if (key_drop && !P.key && reach_any(key_pos + V(0, 0, 900), reach_r + 250.f)) { P.key = true; key_drop = false; sfx(7, SN_KEY, 110); dl("t=%.1f key taken", sim_t); }
        if (!key_door_open && reach_any(V(0, 33080, Z1 + 1600), 900.f)) {
            if (P.key) { key_door_open = true; key_door_t = 0; P.key = false; sfx(7, SN_KEY, 110); dl("t=%.1f keyhole door unlocked", sim_t); }
            else if (locked_msg <= 0) { locked_msg = 2.f; sfx(7, SN_THUD, 80); } }
        if (spider_dead && st == S_PLAY && reach_any(V(0, 52200, Z1 + 2200), 1100.f)) { st = S_OVER; st_t = 0; end_reason = 2; P.white = 1.5f; sfx(7, SN_WIN, 120); dl("t=%.1f mirror reached: QUEST COMPLETE, %d kills, time left %.0f", sim_t, kills, time_left); }
    }
    if (locked_msg > 0) locked_msg -= dt;
    if (big_open) big_t += dt;
    if (low_open) { low_t += dt; if (low_t > 0.7f) gate_open[G_LOW] = true; }
    if (book_taken) book_t += dt;
    if (chest_open) chest_t += dt;
    if (key_door_open) { key_door_t += dt; if (key_door_t > 0.8f) gate_open[G_KEY] = true; }
    if (door_state == 2) { door_t += dt; if (door_t > 1.3f && !gate_open[G_DOOR1]) { gate_open[G_DOOR1] = true; time_left += task_bonus; dl("t=%.1f the door swings open", sim_t); } }
    if (door_state == 1) { door_t += dt;
        if (rnd(0, 1) < dt * 30.f) burst(V(rnd(-700, 700), 16200, Z1 + rnd(300, 2600)), 2, L_GOLD, 600.f, 0.8f);
        if (door_t > crack_t) { door_state = 2; door_t = 0; burst(V(0, 16200, Z1 + 1900), 60, L_REDSP, 1800.f, 1.2f); sfx(4, SN_BOOM, 110); dl("t=%.1f door has a ragged hole", sim_t); } }
    /* the skeleton walks in from the right when you enter the hall */
    if (!skel_spawned && any_h([](V q) { return q.y > 10300.f && q.y < 16100.f && q.x > -5400.f && q.x < 1800.f; })) { skel_spawned = true; spawn(T_SKEL, V(1300, 14700, Z1)); mons.back().awake = true; sfx(6, SN_RATTLE, 110); dl("t=%.1f skeleton enters", sim_t); }
    /* "a suspicious pile of bones ... avoid those" (speech 2:58) and "skeletons rising from the floor" (Amiga Computing 69 p.35): put together here, which is our reading */
    if (!pile_risen && skel_dead && any_h([](V q) { return q.y > 14000.f && dist_xy(q, V(-1000, 15450, 0)) < 1300.f; })) { pile_risen = true; spawn(T_SKEL, V(-1000, 14650, Z1)); mons.back().rise = RISE_T; mons.back().awake = true; sfx(6, SN_RATTLE, 110); dl("t=%.1f a skeleton rises from the bones", sim_t); }
    if (!room3_risen && any_h([](V q) { return q.y > 27400.f && q.y < 33000.f && q.x > -3000.f; })) { room3_risen = true; const float sp[2][2] = {{1500, 29000}, {-1700, 30800}}; for (int i = 0; i < 2; i++) { spawn(T_SKEL, V(sp[i][0], sp[i][1], Z1)); mons.back().rise = RISE_T; mons.back().awake = true; } sfx(6, SN_RATTLE, 110); dl("t=%.1f skeletons rise from the floor", sim_t); }
    /* trap tiles */
    for (int r = 0; r < TR_R; r++) for (int c = 0; c < TR_C; c++) {
        float cx0 = TRAP_X0 + c * TILE, cy0 = TRAP_Y0 + r * TILE; bool on = any_h([=](V q) { return q.x >= cx0 && q.x < cx0 + TILE && q.y >= cy0 && q.y < cy0 + TILE; }, true);
        if (trap_is[r][c]) {
            if (trap_open[r][c] <= 0.f) { if (on) trap_arm[r][c] += dt; else trap_arm[r][c] = 0.f; if (trap_arm[r][c] > 0.35f) { trap_open[r][c] = dt * 4.f; sfx(10, SN_CLATTER, 100); dl("t=%.1f trap door %d,%d opens", sim_t, r, c); } }
            else if (trap_open[r][c] < 1.f) trap_open[r][c] = (std::min)(1.f, trap_open[r][c] + dt * 4.f);
        }
    }
}
static void body_step(float dt) {
    if (!P.alive) return;
    float h = Z1; bool got = floor_at(P.p.x, P.p.y, h);
    if (!got) h = P.p.z;
    if (P.falling || P.p.z > h + 40.f) {
        P.falling = true; P.vz -= 12000.f * dt; P.p.z += P.vz * dt;
        if (P.p.z <= h) { P.p.z = h; P.vz = 0; P.falling = false;
            if (h < Z1 - 1000.f && !P.in_pit) { P.in_pit = true; P.pit_t = 1.0f; if (v1knet::on && !v1knet::master) { net_pit_seq++; sfx(5, SN_THUD, 110); } else hurt(20.f); sfx(5, SN_THUD, 110); dl("t=%.1f fell through a trap door", sim_t); } }
    } else P.p.z = h;
    if (P.in_pit) { P.pit_t -= dt; if (P.pit_t <= 0) { P.in_pit = false; teleport(V(0, 34600, 0)); dl("t=%.1f back at the start of the trap room", sim_t); } }
}

static void start_round() {
    mons.clear(); sparks.clear(); remains.clear(); fb.on = false; arrow.on = false; P = Pl(); P.hp = hp_max; P.p = V(0, 800, 0); low_open = pile_risen = room3_risen = false; low_t = book_t = 0;
    { VPIX_Input hi; VPIX_Input_Get(hi); P.weap = (autoplay || !hi.hand.valid) ? 1 : 0; }   /* a tracked hand starts empty and draws from the belt (GamePro 40); Bobby remembers walking in with a sword, so no hand = blade out */
    remains.push_back(V(900, 11900, Z1));   /* the second pile, beyond the table, t160 */
    if (v1knet::on) { v1knet::begin_round(4); for (int i = 0; i < 4; i++) { rem[i] = Rem(); rem[i].hp = hp_max; } }   /* linked: four seats, the empty ones stay the CPU party */
    allies.clear(); { static const float ax[3] = {-1300, 1200, -300}; static const int lk[3] = {1, 2, 3}; for (int i = 0; i < 3; i++) { Ally a; (void)ax; a.p = V(-5200, 1500, 0); a.look = lk[i]; a.yaw = -1.5708f; a.arrive = 3.f + i * 4.5f; a.walking_in = true; allies.push_back(a); } }   /* they walk in one by one through the big door (t106-t135) */
    for (int i = 0; i < NG; i++) gate_open[i] = false;
    chest_open = book_taken = big_open = key_door_open = key_drop = spider_dead = skel_spawned = skel_dead = false; chest_t = big_t = key_door_t = door_t = locked_msg = 0; door_state = 0; kills = 0;
    have_off = false; swing_v = 0; sw_have = false; trig_prev = false;
    safe_col = (int)(rnd(0, 2.999f)); if (safe_col > 2) safe_col = 2;
    for (int r = 0; r < TR_R; r++) { int t = (safe_col + 1 + (int)rnd(0, 1.999f)) % 3; for (int c = 0; c < TR_C; c++) { trap_is[r][c] = (c == t); trap_open[r][c] = 0; trap_arm[r][c] = 0; } }
    /* the bats in the corridor are from footage; the others' places are INVENTED: goblins and the green thing on the yellow path, wolves outside, the ogre in his room, the spider in its lair */
    for (int i = 0; i < 4; i++) spawn(T_BAT, V(rnd(-500, 500), 21500.f + i * 700.f, Z1 + 1500.f));
    spawn(T_GOB, V(-1400, 28800, Z1)); spawn(T_GOB, V(-500, 31200, Z1)); spawn(T_SERP, V(1500, 30200, Z1));
    spawn(T_WOLF, V(-8600, 400, 0)); spawn(T_WOLF, V(-9200, 3000, 0)); spawn(T_OGRE, V(-12200, 29700, Z1));
    spawn(T_SPIDER, V(0, 49500, Z1));
    time_left = game_len; end_reason = 0; walk_off = V(); st_t = 0;
    build_route(); auto_cyc = 0;
    dl("t=%.1f round start (trap safe column %d)", sim_t, safe_col);
}
static void world_step(const VPIX_Input &in, float dt) {
    if (P.alive) { if (autoplay) make_ctl_auto(dt); else make_ctl_from_input(in, dt); }
    ear_at = ctl.eye; body_step(dt); weapon_step(dt); remote_step(dt); fire_step(dt); arrow_step(dt); props_step(dt);
    for (Mon &m : mons) mon_step(m, dt);
    for (size_t i = 0; i < mons.size(); i++) for (size_t j = i + 1; j < mons.size(); j++) {   /* keep monsters from stacking */
        Mon &a = mons[i], &b = mons[j]; if (!a.alive || !b.alive || a.type == T_BAT || b.type == T_BAT) continue;
        V d(b.p.x - a.p.x, b.p.y - a.p.y, 0); float l = len(d), mr = MD[a.type].rad + MD[b.type].rad;
        if (l < mr && l > 1.f) { try_move(b.p, d * ((mr - l) * 0.5f / l)); try_move(a.p, d * (-(mr - l) * 0.5f / l)); } }
    for (size_t i = 0; i < mons.size();) { if (!mons[i].alive && mons[i].dying <= 0) { mons[i] = mons.back(); mons.pop_back(); } else i++; }
    if (!P.alive) P.dead_t += dt;
    if (all_dead() && P.dead_t > 1.8f && st == S_PLAY) { st = S_OVER; st_t = 0; end_reason = 1; }
    if (st == S_PLAY) { time_left -= dt; if (time_left <= 0) { st = S_OVER; st_t = 0; end_reason = 3; dl("t=%.1f time up, %d kills", sim_t, kills); } }
}
static void sparks_step(float dt) {
    if (!(v1knet::on && !v1knet::master && net_slave_game)) for (size_t i = 0; i < allies.size(); i++) { Ally &a = allies[i]; static const float sx[3] = {-1200, 1300, 150}, sy[3] = {-900, -700, -1700}; const float c = cosf(P.yaw), sn = sinf(P.yaw); V want = P.p + V(c * sx[i] - sn * sy[i], sn * sx[i] + c * sy[i], 0);
        if (a.human) { if (a.moving) a.phase += dt * 7.f; continue; }   /* a pod plays this one */
        if (a.arrive > 0) { a.arrive -= dt; continue; }
        if (a.walking_in) { V tg(-1400.f + i * 700.f, 1200.f + i * 300.f, 0), d = tg - a.p; d.z = 0; float l = len(d); if (l < 150.f) { a.walking_in = false; continue; } a.p += d * ((walk_speed * dt) / l); a.phase += dt * 7.f; a.moving = true; a.yaw = atan2f(-d.x, d.y); continue; }
        /* "I'll help. Elves are brilliant swordsmen" (speech 3:08): the elf goes for the skeleton; the party's other help is not built. Her blow rate is INVENTED */
        if (a.look == 1 && st == S_PLAY && !a.hidden) for (Mon &m : mons) if (m.alive && m.type == T_SKEL && m.rise <= 0 && dist_xy(m.p, a.p) < 7000.f) { V to = m.p - a.p; to.z = 0; float dm = len(to); want = m.p - to * (700.f / (std::max)(dm, 1.f)); if (a.hit_t > 0) a.hit_t -= dt; if (dm < 1100.f && a.hit_t <= 0) { a.hit_t = 1.0f; hit_mon(m, 8.f, a.p); sfx_at(2, SN_HIT, 90, m.p); } break; }
        V d = want - a.p; d.z = 0; float l = len(d); a.moving = l > 500.f;
        if (l > 9000.f) { float z; if (floor_at(want.x, want.y, z)) { a.p = V(want.x, want.y, z); } }
        else if (a.moving) { V st = d * ((std::min)(walk_speed * 1.15f * dt, l) / l); try_move(a.p, st); a.phase += dt * 7.f; float yw = atan2f(-d.x, d.y), dy = yw - a.yaw; while (dy > LPI) dy -= 2.f * LPI; while (dy < -LPI) dy += 2.f * LPI; a.yaw += clampf(dy, -3.f * dt, 3.f * dt); }
        else { V to = P.p - a.p; float yw = atan2f(-to.x, to.y), dy = yw - a.yaw; while (dy > LPI) dy -= 2.f * LPI; while (dy < -LPI) dy += 2.f * LPI; a.yaw += clampf(dy, -2.f * dt, 2.f * dt); }
        float z = a.p.z; if (floor_at(a.p.x, a.p.y, z)) a.p.z = z; }
    for (size_t i = 0; i < sparks.size();) { Spark &k = sparks[i]; k.v.z -= 1100.f * dt; k.p += k.v * dt; k.life -= dt;
        if (k.p.z < P.p.z + 26.f && k.v.z < 0) { k.p.z = P.p.z + 26.f; k.v = V(); k.id = L_DOT; }   /* they settle on the floor as pale dots (t200) */
        if (k.life <= 0) { sparks[i] = sparks.back(); sparks.pop_back(); } else i++; }
}

/* ---- drawing ---- */
/* the working mirror: while mir_axis is set, everything goes through lput, which draws it reflected in the plane. A negative scale flips the model, which the renderer already handles (the left hand gun). All of it is real 3D, so the reflection has correct parallax in the headset */
static int mir_axis = -1; static float mir_plane = 0;
static V mir_v(V a) { if (mir_axis == 0) a.x = -a.x; else a.y = -a.y; return a; }
static VPIX_Instance &lput(VPIX_Frame &f, int id, const M &R, V T) {
    if (mir_axis < 0) return dput(f, id, R, T);
    M r; r.r[0] = mir_v(R.r[0]) * -1.f; r.r[1] = mir_v(R.r[1]); r.r[2] = mir_v(R.r[2]);
    if (mir_axis == 0) T.x = 2.f * mir_plane - T.x; else T.y = 2.f * mir_plane - T.y;
    VPIX_Instance &in = dput(f, id, r, T); in.S[0] = -1.f; return in;
}
static void put_mon(VPIX_Frame &f, const Mon &m) {
    const MDef &d = MD[m.type]; M R = yawed(m.yaw);
    if (!m.alive) { float el = (m.type == T_SKEL ? vanish_t : 3.f) - m.dying; if (el > 1.2f || fmodf(el * 14.f, 1.f) > 0.6f) return; }   /* flickers, then only the sparkles are left */
    V at = m.p;
    if (m.rise > 0) at.z -= d.h * (m.rise / RISE_T);   /* comes up through the floor */
    if (m.wind > 0) at = at + R.r[1] * (sinf(LPI * (1.f - m.wind / (std::max)(d.windup, 0.01f))) * 450.f);   /* lunge */
    if (m.type == T_BAT) {
        lput(f, d.body, R, at);
        float fa = sinf(m.phase * 18.f) * 0.8f, c = cosf(fa), s = sinf(fa); M wl = R, wr = R;
        wr.r[0] = R.r[0] * c + R.r[2] * s; wr.r[2] = R.r[2] * c - R.r[0] * s; wl.r[0] = R.r[0] * c - R.r[2] * s; wl.r[2] = R.r[2] * c + R.r[0] * s;
        lput(f, L_BAT_WL, wl, at + R.r[2] * 60.f); lput(f, L_BAT_WR, wr, at + R.r[2] * 60.f); return;
    }
    VPIX_Instance &bi = lput(f, d.body, R, at); if (m.flash > 0) bi.bright = 1;
    float sw = (m.awake && m.alive) ? sinf(m.phase * (m.type == T_WOLF ? 14.f : 6.f)) * 0.5f : 0.f;
    const LegSet &ls = LEGS[m.type];
    for (int i = 0; i < ls.n; i++) { float a = (i & 1) ? -sw : sw; if (m.type == T_WOLF && i >= 2) a = -a;
        V o = R.r[0] * ls.off[i].x + R.r[1] * ls.off[i].y; lput(f, d.leg, pitched(R, a), at + o + V(0, 0, ls.hipz)); }
}
static void put_ally(VPIX_Frame &f, const Ally &a) {
    M R = yawed(a.yaw); lput(f, L_AV + a.look, R, a.p);
    if (a.look != 0) { float sw = a.moving ? sinf(a.phase) * 0.5f : 0.f, hz = a.look == 2 ? 420.f : 860.f, ox = a.look == 2 ? 130.f : 110.f; int lg = a.look == 2 ? L_AV + 5 : L_AV + 4; lput(f, lg, pitched(R, sw), a.p + R.r[0] * -ox + V(0, 0, hz)); lput(f, lg, pitched(R, -sw), a.p + R.r[0] * ox + V(0, 0, hz)); }
}
static void put_castle_doors(VPIX_Frame &f) {
    float a = big_open ? (std::min)(big_t / 2.5f, 1.f) * 0.95f : 0.f; lput(f, L_BIGDOOR, yawed(LPI * 0.5f - a), V(-3300, 100, 0)); lput(f, L_BIGDOOR, yawed(-LPI * 0.5f + a), V(-3300, 2900, 0));
    float la = low_open ? (std::min)(low_t / 0.9f, 1.f) * 1.7f : 0.f; M dr = yawed(la); lput(f, L_DOOR1, dr, V(-780, 5300, -900) + dr.r[0] * 780.f);
}
/* what the mirror sees: the room, its doors and torches, whoever is near, and you (the wizard, your hand and what it holds) */
static void mirror_pass(VPIX_Frame &f, int axis, float plane, int room, V eye) {
    mir_axis = axis; mir_plane = plane; M I;
    lput(f, L_ROOM + room, I, V(0, 0, 0));
    if (room == 0) put_castle_doors(f);
    for (const TorchP &t : torches) if (t.room == room) { VPIX_Instance &i2 = lput(f, L_FLAME, I, t.p); i2.bright = 1; }
    for (const Ally &a : allies) if (a.arrive <= 0 && !a.hidden && dist_xy(a.p, eye) < 15000.f) put_ally(f, a);
    for (const Mon &m : mons) if (dist_xy(m.p, eye) < 15000.f && m.dying >= 0) put_mon(f, m);
    if (st != S_TITLE && P.alive) { V fw = ctl.head.r[1]; M R = yawed(atan2f(-fw.x, fw.y)); lput(f, L_AV, R, P.p);
        if (st == S_PLAY) { int hm = P.weap == 2 ? L_BOW : P.weap == 1 ? (P.sword2 ? L_SWORD2 : L_SWORD1) : L_HAND; lput(f, hm, ctl.hrot, ctl.hand); } }
    mir_axis = -1;
}
static void put_world(VPIX_Frame &f, V eye, const bool vis[7]) {
    M I;
    for (int i = 0; i < 7; i++) if (vis[i]) dput(f, L_ROOM + i, I, V(0, 0, 0));
    for (const TorchP &t : torches) if (vis[t.room]) { float k = 0.85f + 0.25f * sinf((float)sim_t * 13.f + t.p.x * 0.01f + t.p.y * 0.003f); VPIX_Instance &i2 = dput(f, L_FLAME, I, t.p); i2.bright = 1; i2.S[0] = i2.S[1] = i2.S[2] = k; }
    { static const char *fc = getenv("LEGEND_CAM"); if (st != S_TITLE) {
        for (const Ally &a : allies) { if (a.arrive > 0 || a.hidden || dist_xy(a.p, eye) > 17000.f) continue; put_ally(f, a); }
        if (fc && *fc) dput(f, L_AV, yawed(P.yaw), P.p);
        if (P.p.y > 16700.f && P.p.y < 26000.f) { M J; for (int i = 0; i < 90; i++) { float a = (float)i * 2.4f + (float)sim_t * 1.3f, r = 400.f + 700.f * fmodf(i * 0.37f, 1.f);   /* a thick cloud of specks round the head, t211-t215 */ dput(f, L_BLK, J, P.p + V(cosf(a) * r, 700.f + sinf(a) * r, 1300.f + sinf(a * 1.7f) * 450.f)); } } } }
    /* castle */
    if (vis[0]) {
        put_castle_doors(f);
        dput(f, L_WALLE, I, V(0, 0, 0));
        if (eye.x < MIRROR_X && eye.y < 9000.f && eye.z > -1500.f) mirror_pass(f, 0, MIRROR_X, 0, eye);
        dput(f, P.shimmer > 0 ? L_MIRROR_G : L_MIRROR, yawed(-LPI * 0.5f), V(MIRROR_X, MIRROR_Y, 0)).bright = 1;
    }
    /* hall */
    if (vis[1]) {
        dput(f, L_KEYDOOR, yawed(LPI * 0.5f), V(-920, 10000, Z1));   /* the door you came through, stood open on your left: planks, a pale plate with a keyhole (t158, Bad Influence t47-50) */
        if (door_state == 2) { float a = clampf((door_t - 1.3f) / 1.1f, 0.f, 1.f) * 1.75f; M dr = yawed(a); dput(f, L_DOOR1C, dr, V(-780, 16300, Z1) + dr.r[0] * 780.f); }   /* the ragged hole first (t225), then it swings */
        else dput(f, (door_state == 1 && door_t > 0.5f) ? L_DOOR1B : L_DOOR1, I, V(0, 16300, Z1));
        float la = chest_open ? (std::min)(chest_t / 0.6f, 1.f) * 1.7f : 0.f; if (book_taken && book_t > 4.f) la *= clampf(1.f - (book_t - 4.f) / 0.6f, 0.f, 1.f);   /* shut again by t203 */
        float c = cosf(la), s = sinf(la); M lid; lid.r[0] = V(c, 0, -s); lid.r[1] = V(0, 1, 0); lid.r[2] = V(s, 0, c);
        dput(f, L_LID, lid, V(1950, 12650, Z1 + 625));
        if (chest_open && !book_taken) dput(f, L_BOOK, yawed((float)sim_t * 1.2f), V(1500, 12650, Z1 + 800.f + sinf((float)sim_t * 2.f) * 40.f)).bright = 1;
    }
    if (vis[3]) { float a = key_door_open ? (std::min)(key_door_t / 1.0f, 1.f) * 1.7f : 0.f; M dr = yawed(a); dput(f, L_DOOR2, dr, V(-780, 33300, Z1) + dr.r[0] * 780.f); }
    if (key_drop) dput(f, L_KEY, yawed((float)sim_t * 2.f), key_pos + V(0, 0, 900.f + sinf((float)sim_t * 3.f) * 60.f)).bright = 1;
    /* trap room */
    if (vis[5]) for (int r = 0; r < TR_R; r++) for (int c = 0; c < TR_C; c++) {
        M R = pitched(I, -trap_open[r][c] * 1.45f); dput(f, trap_is[r][c] ? L_TILE_T : L_TILE_S, R, V(TRAP_X0 + (c + 0.5f) * TILE, TRAP_Y0 + r * TILE + 10.f, Z1)); }
    /* lair mirror: dark until the spider is dead */
    if (vis[6]) { dput(f, L_WALLN, I, V(0, 0, 0)); VPIX_Instance &mi = dput(f, spider_dead ? L_MIRROR_G : L_MIRROR, I, V(0, LAIR_Y, Z1)); mi.bright = spider_dead ? 1 : 0; if (eye.y > 42000.f && eye.y < LAIR_Y) mirror_pass(f, 1, LAIR_Y, 6, eye); }
    for (const Mon &m : mons) if (len(V(m.p.x - eye.x, m.p.y - eye.y, 0)) < 18000.f) put_mon(f, m);
    for (const V &r : remains) if (len(V(r.x - eye.x, r.y - eye.y, 0)) < 18000.f) dput(f, L_BONES, I, r);
    if (fb.on) { VPIX_Instance &b = dput(f, L_FLAMEH, I, fb.p); b.bright = 1; float k = 0.9f + 0.2f * sinf((float)sim_t * 30.f); b.S[0] = b.S[1] = b.S[2] = k; }
    if (arrow.on) { M R = look_at(arrow.p, arrow.p + arrow.v); dput(f, L_ARROW, R, arrow.p); }
    for (const Spark &k : sparks) dput(f, k.id, I, k.p).bright = 1;
}
static M I_up(const M &h) { V fw = norm(V(h.r[1].x, h.r[1].y, 0.0001f)); return yawed(atan2f(-fw.x, fw.y)); }   /* the flame stands up whatever the wrist does */
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = f.clear[1] = f.clear[2] = 0;
    f.n_lights = 1; f.lights[0] = {0.72f, 0.34f, {-0.218f, -0.437f, 0.874f}};   /* flat shaded and bright; values are a guess */
    bool play = st != S_TITLE;
    M cam; V eye; bool head_cam = false;
    static const char *fixed = getenv("LEGEND_CAM");
    if (fixed && *fixed) { float a[6] = {0, 800, 1700, 0, 5000, 1500}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5])); }
    else if (!play) { float a = sinf((float)sim_t * 0.25f) * 0.9f; eye = V(0, 700, 1700); cam = look_at(eye, eye + V(sinf(a) * 4000.f, cosf(a) * 4000.f, -200.f)); }   /* attract: slow look round the castle room */
    else if (autoplay || !in.head.valid) { eye = ctl.eye; cam = ctl.head; }
    else {
        for (int k = 0; k < 3; k++) cam.r[k] = V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2]);
        eye = ctl.eye; head_cam = true;
    }
    { static const char *stg = getenv("LEGEND_STAGE");   /* dev: pose the skeleton and three avatars for a picture: sx,sy,ax,ay,bx,by,cx,cy */
      if (stg && *stg && play) { float a[8] = {0, 0, 0, 0, 0, 0, 0, 0}; sscanf(stg, "%f,%f,%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5, a + 6, a + 7);
        auto faces = [&](V p) { V to = eye - p; return atan2f(-to.x, to.y); };
        if (a[0] != 0 || a[1] != 0) { bool got = false; for (Mon &m : mons) if (m.type == T_SKEL && m.alive) { m.p = V(a[0], a[1], Z1); m.yaw = faces(m.p); m.hp = 999; m.awake = true; got = true; } if (!got && !skel_spawned) { skel_spawned = true; spawn(T_SKEL, V(a[0], a[1], Z1)); } }
        for (int i = 0; i < 3 && i < (int)allies.size(); i++) { allies[i].arrive = 0; allies[i].walking_in = false; allies[i].p = V(a[2 + i * 2], a[3 + i * 2], Z1); allies[i].yaw = faces(allies[i].p); allies[i].moving = false; static const char *sl = getenv("LEGEND_STAGE_LOOK"); if (sl && (int)strlen(sl) > i) allies[i].look = sl[i] - '0'; }
        P.hp = hp_max; } }
    { static const char *idl = getenv("LEGEND_IDLE"); if (idl && *idl && fixed && *fixed) for (Mon &m : mons) { V to = eye - m.p; if (m.type != T_SERP) m.yaw = atan2f(-to.x, to.y); } }
    bool vis[7];
    for (int i = 0; i < 7; i++) { float dx = (std::max)((std::max)(RBB[i][0] - eye.x, 0.f), eye.x - RBB[i][3]), dy = (std::max)((std::max)(RBB[i][1] - eye.y, 0.f), eye.y - RBB[i][4]);
        vis[i] = sqrtf(dx * dx + dy * dy) < 17000.f; }
    put_world(f, eye, vis);
    if (play && P.alive && st == S_PLAY) {   /* your own sword in the red sleeve, tracked */
        if (fixed && *fixed) { ctl.hrot = cam; ctl.hfw = cam.r[1]; ctl.hand = eye + cam.r[1] * 900.f + cam.r[0] * 190.f - cam.r[2] * 300.f; }
        float th = P.thrust_t > 0 ? sinf(LPI * (1.f - P.thrust_t / 0.28f)) * 400.f : 0.f;
        int hm = P.weap == 2 ? L_BOW : P.weap == 1 ? (P.sword2 ? L_SWORD2 : L_SWORD1) : L_HAND;
        dput(f, hm, ctl.hrot, ctl.hand + ctl.hfw * th).on_top = 2;
        if (P.hold_t > 0.1f && P.book && P.weap == 0) { VPIX_Instance &b = dput(f, L_FLAMEH, I_up(ctl.hrot), ctl.hand + ctl.hfw * 200.f); b.bright = 1; b.on_top = 2; float k = 0.4f + (std::min)(P.hold_t / 0.6f, 1.f) * 0.6f; b.S[0] = b.S[1] = b.S[2] = k; }
        if (P.book_view > 0) { dput(f, L_BOOKV, cam, eye + cam.r[1] * 560.f - cam.r[2] * 190.f).on_top = 2; }
    }
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    f.in_head = head_cam; if (head_cam) f.head_fed = in.head;
    /* no HUD in play (footage). Only the red flash of a hit (189 s) and the white flash at the mirror */
    if (play && P.flash > 0) flash_blit(f, BM_RED, (std::min)(P.flash / 0.3f, 1.f) * 0.5f);
    else if (play && P.dim > 0) flash_blit(f, BM_DIM, (std::min)(P.dim, 1.f) * 0.5f);
    if (play && P.white > 0) flash_blit(f, BM_WHITE, (std::min)(P.white, 1.f));
    const float CX = 176.f;
    if (!play) {
        text(f, "LEGEND QUEST", CX, 40, 1.6f, 1);
        text(f, "THUMB WALKS  TOUCH YOUR BELT FOR A BLADE", CX, 160, 0.6f); text(f, "TOUCH YOUR SHOULDER FOR A BOW  REACH TO OPEN", CX, 174, 0.6f); text(f, "BOOK AND EMPTY HAND  HOLD TRIGGER: FIREBALL", CX, 188, 0.6f);
        if (v1knet::on && !v1knet::can_start()) text(f, !v1knet::master_heard() ? "LOOKING FOR PLAYER 1" : v1knet::nseats > 0 ? "YOU JOIN NEXT ROUND" : "PLAYER 1 STARTS THE GAME", CX, 230, 0.8f);
        else if (fmodf((float)sim_t, 1.2f) < 0.8f) text(f, "PRESS TRIGGER", CX, 230, 1.f);
        if (v1knet::on && v1knet::master && v1knet::live_pods() > 0) { char b[32]; snprintf(b, sizeof(b), "LINKED %d PLAYERS", v1knet::live_pods() + 1); text(f, b, CX, 208, 0.8f); }
    } else if (st == S_OVER) {
        text(f, end_reason == 2 ? "QUEST COMPLETE" : end_reason == 3 ? "TIME IS UP" : "YOU HAVE FALLEN", CX, 90, 1.6f, end_reason == 2 ? 1 : 2);
        
    }
    d1k::hs::draw(f, "legend", st == S_TITLE, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* ---- link play (vpix_1k_net.h): a party in one dungeon. Pod 1 runs everything (monsters, doors, chest, key, traps, the quest); another pod walks its own figure (seat 1..3, the elf, the dwarf and the third party member), swings and throws with its own hands, and draws the master's state.
   INVENTED (the original is a one-party game and nothing says how its pods shared it): the book, the better sword, the key and the time are the whole party's; anyone's reach opens a thing; monsters go for the nearest player; the quest ends when all are down or time is up or anyone reaches the lair mirror.
   A seat nobody plays stays the CPU party member as in solo. Start-together (join in progress is only for Flying Aces and VTOL). ---- */
#pragma pack(push, 1)
struct NetIn { float p[3], eye[3], hand[3], hfw[3], hdf[3], yaw; uint8_t weap, fl, fseq, aseq, cseq, pseq; };   /* fl: 1 swinging, 2 blocking, 4 falling, 8 walking, 16 autopilot */
struct NetP { int16_t x, y, z; uint8_t yaw, fl, hp; };                                                           /* x, y in 2 mm; fl: 1 shown, 2 moving, 4 alive */
struct NetM { int16_t x, y, z; uint8_t type, yaw, fl, prog; };                                                   /* fl: 1 alive, 2 awake, 4 flash, 8 rising, 16 winding up */
struct NetSt { uint8_t st, reason, kills, safe_col, door_state, gates; int16_t time_left; uint16_t flags, trap_is; uint8_t trap_open[12]; int16_t key[3];
               NetP pl[4]; uint8_t fb_on, ar_on; int16_t fb[3], ar[3]; int8_t ard[3]; uint8_t nrem; int16_t rem[14][2]; uint8_t nm; NetM m[30]; };
#pragma pack(pop)
static uint8_t net_fseq = 0, net_aseq = 0, net_cseq = 0; static float net_arrow_cd = 0;
static int16_t q2(float v) { return (int16_t)clampf(v * 0.5f, -32000.f, 32000.f); }
static int16_t qz(float v) { return (int16_t)clampf(v, -32000.f, 32000.f); }
static uint8_t qyaw(float y) { y = fmodf(y, 2.f * LPI); if (y < 0) y += 2.f * LPI; int b = (int)(y / (2.f * LPI) * 256.f); return (uint8_t)(b > 255 ? 255 : b); }
static float uyaw(uint8_t b) { return b * (2.f * LPI) / 256.f; }

/* master: read the pods' inputs; a seat with a pod drives that party member, a seat without one gives it back to the CPU */
static void net_master_in() {
    for (int sI = 1; sI < 4; sI++) {
        Rem &r = rem[sI]; if (sI - 1 >= (int)allies.size()) { r.on = false; continue; }
        Ally &a = allies[sI - 1]; const NetIn *p = st == S_TITLE ? NULL : (const NetIn *)v1knet::seat_input(sI, sizeof(NetIn));
        if (!p) { if (r.on) { r.on = false; a.human = false; a.hidden = false; dl("t=%.1f link: seat %d is the CPU party member again", sim_t, sI); } continue; }
        if (!r.on) { r.on = true; r.alive = r.hp > 0; r.fseq = r.fdone = p->fseq; r.aseq = r.adone = p->aseq; r.cseq = p->cseq; r.pseq = p->pseq; dl("t=%.1f link: seat %d is a person (pod %d)", sim_t, sI, v1knet::seat_pod[sI]); }
        r.p = V(p->p[0], p->p[1], p->p[2]); r.eye = V(p->eye[0], p->eye[1], p->eye[2]); r.hand = V(p->hand[0], p->hand[1], p->hand[2]); r.hfw = norm(V(p->hfw[0], p->hfw[1], p->hfw[2])); r.hdf = norm(V(p->hdf[0], p->hdf[1], p->hdf[2]));
        r.yaw = p->yaw; r.weap = p->weap; r.swing = (p->fl & 1) != 0; r.block = (p->fl & 2) != 0; r.falling = (p->fl & 4) != 0; r.walk = (p->fl & 8) != 0; r.autop = (p->fl & 16) != 0;
        r.fseq = p->fseq; r.aseq = p->aseq; if (p->cseq != r.cseq) { r.cseq = p->cseq; r.click = true; }
        if (p->pseq != r.pseq) { r.pseq = p->pseq; hurt_seat(sI, 20.f); }
        a.human = true; a.arrive = 0; a.walking_in = false; a.p = r.p; a.yaw = r.yaw; a.moving = r.walk; a.hidden = !r.alive;
    }
}
static void net_send() {
    if (st == S_TITLE) { v1knet::send_snapshot(NULL, 0); return; }
    static NetSt n; memset(&n, 0, sizeof(n));
    n.st = (uint8_t)st; n.reason = (uint8_t)end_reason; n.kills = (uint8_t)(kills > 255 ? 255 : kills); n.safe_col = (uint8_t)safe_col; n.door_state = (uint8_t)door_state;
    for (int g = 0; g < NG; g++) if (gate_open[g]) n.gates |= (uint8_t)(1 << g);
    n.time_left = qz(time_left);
    n.flags = (chest_open ? 1 : 0) | (book_taken ? 2 : 0) | (big_open ? 4 : 0) | (key_door_open ? 8 : 0) | (key_drop ? 16 : 0) | (spider_dead ? 32 : 0) | (low_open ? 64 : 0) | (P.book ? 128 : 0) | (P.sword2 ? 256 : 0) | (P.key ? 512 : 0) | (skel_dead ? 1024 : 0);
    for (int r = 0; r < TR_R; r++) for (int c = 0; c < TR_C; c++) { if (trap_is[r][c]) n.trap_is |= (uint16_t)(1 << (r * TR_C + c)); n.trap_open[r * TR_C + c] = (uint8_t)(clampf(trap_open[r][c], 0.f, 1.f) * 255.f); }
    n.key[0] = q2(key_pos.x); n.key[1] = q2(key_pos.y); n.key[2] = qz(key_pos.z);
    for (int sI = 0; sI < 4; sI++) { NetP &q = n.pl[sI];
        if (sI == 0) { q.x = q2(P.p.x); q.y = q2(P.p.y); q.z = qz(P.p.z); q.yaw = qyaw(P.yaw); q.fl = (uint8_t)(1 | (ctl.walk ? 2 : 0) | (P.alive ? 4 : 0)); q.hp = (uint8_t)clampf(P.hp, 0.f, 255.f); continue; }
        if (sI - 1 >= (int)allies.size()) continue;
        const Ally &a = allies[sI - 1]; const bool human = a.human;
        q.x = q2(a.p.x); q.y = q2(a.p.y); q.z = qz(a.p.z); q.yaw = qyaw(a.yaw);
        q.fl = (uint8_t)((a.arrive <= 0 && !a.hidden ? 1 : 0) | (a.moving ? 2 : 0) | ((human ? rem[sI].alive : true) ? 4 : 0)); q.hp = (uint8_t)clampf(human ? rem[sI].hp : hp_max, 0.f, 255.f); }
    n.fb_on = fb.on; n.fb[0] = q2(fb.p.x); n.fb[1] = q2(fb.p.y); n.fb[2] = qz(fb.p.z);
    n.ar_on = arrow.on; n.ar[0] = q2(arrow.p.x); n.ar[1] = q2(arrow.p.y); n.ar[2] = qz(arrow.p.z);
    { V dv = norm(arrow.v); n.ard[0] = (int8_t)(dv.x * 127.f); n.ard[1] = (int8_t)(dv.y * 127.f); n.ard[2] = (int8_t)(dv.z * 127.f); }
    n.nrem = (uint8_t)(remains.size() > 14 ? 14 : remains.size()); for (int i = 0; i < n.nrem; i++) { n.rem[i][0] = q2(remains[i].x); n.rem[i][1] = q2(remains[i].y); }
    n.nm = (uint8_t)(mons.size() > 30 ? 30 : mons.size());
    for (int i = 0; i < n.nm; i++) { const Mon &m = mons[i]; NetM &o = n.m[i]; const MDef &d = MD[m.type];
        o.x = q2(m.p.x); o.y = q2(m.p.y); o.z = qz(m.p.z); o.type = (uint8_t)m.type; o.yaw = qyaw(m.yaw);
        o.fl = (uint8_t)((m.alive ? 1 : 0) | (m.awake ? 2 : 0) | (m.flash > 0 ? 4 : 0) | (m.rise > 0 ? 8 : 0) | (m.wind > 0 ? 16 : 0));
        float pr = m.rise > 0 ? m.rise / RISE_T : m.wind > 0 ? m.wind / (std::max)(d.windup, 0.01f) : !m.alive ? m.dying / 6.f : 0.f; o.prog = (uint8_t)(clampf(pr, 0.f, 1.f) * 255.f); }
    v1knet::send_snapshot(&n, sizeof(n));
}
/* slave: I am a seat in the master's round; my own walking and hands run here, everything else is the master's */
static void slave_enter() {
    sparks.clear(); mons.clear(); remains.clear(); fb.on = arrow.on = false;
    { VPIX_Input hi; VPIX_Input_Get(hi); P = Pl(); P.hp = hp_max; P.p = V(0, 800, 0); P.weap = (autoplay || !hi.hand.valid) ? 1 : 0; }
    chest_open = book_taken = big_open = key_door_open = key_drop = spider_dead = skel_dead = low_open = false; chest_t = big_t = key_door_t = door_t = low_t = book_t = 0; door_state = 0; kills = 0; end_reason = 0;
    for (int g = 0; g < NG; g++) gate_open[g] = false;
    have_off = false; swing_v = 0; sw_have = false; trig_prev = false; walk_off = V(); build_route(); auto_cyc = 0;
    st = S_PLAY; st_t = 0; net_slave_game = true; net_fseq = net_aseq = net_cseq = net_pit_seq = 0; net_arrow_cd = 0;
    dl("t=%.1f link: seat %d, in the master's round", sim_t, v1knet::my_seat);
}
static void net_leave(const char *why) {
    dl("t=%.1f link: leaving the master's round (%s)", sim_t, why);
    net_slave_game = false; st = S_TITLE; st_t = 0; mons.clear(); sparks.clear(); allies.clear(); remains.clear(); fb.on = arrow.on = false;
}
static void net_apply(const NetSt &s) {
    const bool first = !net_slave_game; const int me = v1knet::my_seat;
    safe_col = s.safe_col;
    for (int r = 0; r < TR_R; r++) for (int c = 0; c < TR_C; c++) { trap_is[r][c] = (s.trap_is >> (r * TR_C + c)) & 1; trap_open[r][c] = s.trap_open[r * TR_C + c] / 255.f; }
    if (first) slave_enter();
    auto edge = [&](bool &cur, bool nw, float &t) { if (nw && !cur) t = 0; cur = nw; };
    const uint16_t f = s.flags;
    edge(chest_open, (f & 1) != 0, chest_t);
    if ((f & 2) && !book_taken && !first) { P.book_view = 3.f; P.weap = 0; }
    edge(book_taken, (f & 2) != 0, book_t); edge(big_open, (f & 4) != 0, big_t); edge(key_door_open, (f & 8) != 0, key_door_t); edge(low_open, (f & 64) != 0, low_t);
    key_drop = (f & 16) != 0; spider_dead = (f & 32) != 0; P.book = (f & 128) != 0; P.sword2 = (f & 256) != 0; P.key = (f & 512) != 0; skel_dead = (f & 1024) != 0;
    if (s.door_state != door_state) { door_state = s.door_state; door_t = 0; }
    for (int g = 0; g < NG; g++) gate_open[g] = (s.gates >> g) & 1;
    key_pos = V(s.key[0] * 2.f, s.key[1] * 2.f, (float)s.key[2]);
    time_left = s.time_left;
    if (s.st == S_OVER && st != S_OVER) { st = S_OVER; st_t = 0; end_reason = s.reason; if (s.reason == 2) { P.white = 1.5f; sfx(7, SN_WIN, 120); } }
    if (allies.size() != 4) allies.assign(4, Ally());
    for (int j = 0; j < 4; j++) { const NetP &q = s.pl[j]; Ally &a = allies[j];
        a.p = V(q.x * 2.f, q.y * 2.f, (float)q.z); a.yaw = uyaw(q.yaw); a.look = j; a.moving = (q.fl & 2) != 0; if (a.moving) a.phase += 0.7f; a.arrive = 0; a.walking_in = false;
        a.hidden = j == me || !(q.fl & 1) || !(q.fl & 4); }
    { const NetP &q = s.pl[me]; if (!first && q.hp < P.hp - 0.5f) { P.flash = 0.6f; P.dim = dim_t; sfx(9, SN_HURT, 110); VPIX_Haptic(2, 0.12f, 0.8f); }
      P.hp = q.hp; bool al = (q.fl & 4) != 0; if (P.alive && !al) P.dead_t = 0; P.alive = al; }
    fb.on = s.fb_on != 0; fb.p = V(s.fb[0] * 2.f, s.fb[1] * 2.f, (float)s.fb[2]);
    arrow.on = s.ar_on != 0; arrow.p = V(s.ar[0] * 2.f, s.ar[1] * 2.f, (float)s.ar[2]); arrow.v = V(s.ard[0], s.ard[1], s.ard[2]) * (9000.f / 127.f);
    remains.clear(); for (int i = 0; i < s.nrem && i < 14; i++) remains.push_back(V(s.rem[i][0] * 2.f, s.rem[i][1] * 2.f, Z1));
    mons.clear();
    for (int i = 0; i < s.nm && i < 30; i++) { const NetM &o = s.m[i]; if (o.type >= NMT) continue; Mon m; const MDef &d = MD[o.type];
        m.type = o.type; m.p = V(o.x * 2.f, o.y * 2.f, (float)o.z); m.yaw = uyaw(o.yaw); m.alive = (o.fl & 1) != 0; m.awake = (o.fl & 2) != 0; m.flash = (o.fl & 4) ? 0.2f : 0.f; m.phase = (float)sim_t + i * 0.9f;
        if (o.fl & 8) m.rise = o.prog / 255.f * RISE_T; if (o.fl & 16) m.wind = (std::max)(o.prog / 255.f * d.windup, 0.001f); if (!m.alive) m.dying = o.prog / 255.f * 6.f;
        mons.push_back(m); }
    if (!first && s.kills > kills) { const Mon *b = NULL; for (const Mon &m : mons) if (!m.alive && (!b || m.dying > b->dying)) b = &m;
        if (b) { burst(b->p + V(0, 0, MD[b->type].h * 0.5f), b->type == T_SKEL ? 150 : 40, L_GOLD, 1500.f, b->type == T_SKEL ? vanish_t : 2.5f); if (b->type == T_SKEL) burst(b->p + V(0, 0, MD[b->type].h * 0.35f), 150, L_REDSP, 1700.f, vanish_t); sfx(3, SN_HIT, 110); } }
    kills = s.kills;
}
static void net_slave_pre() {
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n >= sizeof(NetSt) && v1knet::my_seat >= 0) { static NetSt s; memcpy(&s, d, sizeof(s)); if (s.st == S_PLAY || s.st == S_OVER) net_apply(s); }
        else if (net_slave_game) net_leave(n >= sizeof(NetSt) ? "no seat" : "the master is at the title");
    }
    if (net_slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
}
static void net_fill_in(NetIn &o) {
    memset(&o, 0, sizeof(o));
    o.p[0] = P.p.x; o.p[1] = P.p.y; o.p[2] = P.p.z; o.eye[0] = ctl.eye.x; o.eye[1] = ctl.eye.y; o.eye[2] = ctl.eye.z; o.hand[0] = ctl.hand.x; o.hand[1] = ctl.hand.y; o.hand[2] = ctl.hand.z;
    o.hfw[0] = ctl.hfw.x; o.hfw[1] = ctl.hfw.y; o.hfw[2] = ctl.hfw.z; V hd = ctl.head.r[1]; o.hdf[0] = hd.x; o.hdf[1] = hd.y; o.hdf[2] = hd.z; o.yaw = P.yaw;
    o.weap = (uint8_t)P.weap; o.fl = (uint8_t)(((swing_v > swing_min || P.thrust_t > 0) && P.weap == 1 ? 1 : 0) | (ctl.block ? 2 : 0) | (P.falling ? 4 : 0) | (ctl.walk ? 8 : 0) | (autoplay ? 16 : 0));
    o.fseq = net_fseq; o.aseq = net_aseq; o.cseq = net_cseq; o.pseq = net_pit_seq;
}
/* slave: my hands. Same timing as weapon_step, but a throw or a shot is only counted here; the master spawns it and works out what it hits */
static void slave_weapon(float dt) {
    if (P.hit_cd > 0) P.hit_cd -= dt; if (P.cast_cd > 0) P.cast_cd -= dt; if (P.thrust_t > 0) P.thrust_t -= dt; if (P.flash > 0) P.flash -= dt; if (P.dim > 0) P.dim -= dt;
    if (P.book_view > 0) P.book_view -= dt; if (P.shimmer > 0) P.shimmer -= dt; if (P.white > 0) P.white -= dt; if (net_arrow_cd > 0) net_arrow_cd -= dt;
    if (ctl.draw >= 0) { P.weap = ctl.draw; sfx(8, SN_SWING, 70); ctl.draw = -1; }
    sw_acc += dt;
    if (!autoplay && sw_acc >= 1.f / 30.f) {
        V rel = ctl.hand + ctl.hfw * sword_len() - ctl.eye;
        if (sw_have) swing_v += ((len(rel - tip_rel) / sw_acc) - swing_v) * 0.6f; else swing_v = 0;
        tip_rel = rel; sw_have = true; sw_acc = 0;
    } else if (sw_acc >= 1.f / 30.f) sw_acc = 0;
    if (ctl.trig_edge) net_cseq++;
    if (ctl.trig_edge && P.weap == 1) { P.thrust_t = 0.28f; sfx(8, SN_SWING, 90); }
    if (ctl.trig_edge && P.weap == 2 && net_arrow_cd <= 0) { net_aseq++; net_arrow_cd = 0.6f; sfx(8, SN_SWING, 90); }
    if (swing_v > swing_min && !autoplay && P.weap == 1 && fmodf((float)sim_t, 0.4f) < dt) sfx(8, SN_SWING, 80);
    if (ctl.trig && P.book && P.weap == 0 && P.book_view <= 0) P.hold_t += dt;
    else { if (P.hold_t > 0.4f && P.book && P.cast_cd <= 0) { net_fseq++; P.cast_cd = cast_gap; sfx(4, SN_FIRE, 110); VPIX_Haptic(0, 0.15f, 0.9f); }
           P.hold_t = 0; }
    if (P.hold_t > 1.2f && P.cast_cd <= 0) { net_fseq++; P.cast_cd = cast_gap; P.hold_t = 0; sfx(4, SN_FIRE, 110); }
}
static void slave_step(const VPIX_Input &in, float dt) {
    if (P.alive) { if (autoplay) make_ctl_auto(dt); else make_ctl_from_input(in, dt); }
    ear_at = ctl.eye; body_step(dt); slave_weapon(dt);
    if (big_open) big_t += dt; if (low_open) low_t += dt; if (book_taken) book_t += dt; if (chest_open) chest_t += dt; if (key_door_open) key_door_t += dt; if (door_state != 0) door_t += dt;
    for (Mon &m : mons) { m.phase += dt; if (m.flash > 0) m.flash -= dt; if (m.alive && m.rise <= 0) {} }
    if (!P.alive) P.dead_t += dt;
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at the machine's rate ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 1; static unsigned long frames = 0;
static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    if (v1knet::on) { v1knet::pump(); if (!v1knet::master) net_slave_pre(); else net_master_in(); }
    static bool trig_was = false; bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f, sel = trig && !trig_was; trig_was = trig;
    if (autoplay && st != S_PLAY && st_t > 2.f) sel = true; if (!net_slave_game && d1k::hs::run("legend", st == S_OVER && st_t > 2.f, kills * 100 + (end_reason == 2 ? 5000 : 0), in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (!v1knet::can_start()) sel = false;   /* a linked slave: pod 1 starts the game */
    if (st == S_TITLE && sel) { start_round(); st = S_PLAY; st_t = 0; sfx(0, SN_START, 100); }
    else if (st == S_OVER && sel && st_t > 4.f && !net_slave_game) { st = S_TITLE; st_t = 0; mons.clear(); sparks.clear(); fb.on = false; v1knet::end_round(); for (Ally &a : allies) a.human = a.hidden = false; for (int i = 0; i < 4; i++) rem[i] = Rem(); }
    int n = 0;
    while (acc >= dt && n < 24) {
        if (st == S_PLAY) { if (net_slave_game) slave_step(in, dt); else world_step(in, dt); }
        else if (st == S_OVER) { for (Mon &m : mons) if (!m.alive) m.dying -= dt; }
        sparks_step(dt);
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    if (n > 0) for (int i = 1; i < 4; i++) rem[i].click = false;
    if (v1knet::on) {
        if (v1knet::master) { if (v1knet::due(30)) net_send(); }
        else if (net_slave_game) { NetIn o; net_fill_in(o); v1knet::send_input(&o, sizeof(o)); }
        else v1knet::send_input(NULL, 0);
    }
    { int near_n = 0; if (st == S_PLAY) { for (const Ally &a : allies) if (a.arrive <= 0 && !a.hidden && dist_xy(a.p, ctl.eye) < 7000.f) near_n++; for (const Mon &m : mons) if (m.alive && m.type != T_BAT && dist_xy(m.p, ctl.eye) < 7000.f) near_n++; }
      double rate = (near_n >= 3 && busy_fps < game_fps) ? busy_fps : game_fps;
      if (frame_acc >= 1.0 / rate) { frame_acc = fmod(frame_acc, 1.0 / rate); build_frame(++frames, in); } }
    if (sim_t >= next_log) { next_log = sim_t + 2.0;
        int awake = 0; for (const Mon &m : mons) if (m.alive && m.awake) awake++;
        dl("t=%.0f st %d me (%.0f %.0f %.0f) hp %.0f book %d key %d door %d gates %d%d | mons %zu awake %d kills %d | left %.0f", sim_t, (int)st, P.p.x, P.p.y, P.p.z, P.hp, P.book, P.key, door_state,
           gate_open[0], gate_open[1], mons.size(), awake, kills, time_left);
        if (v1knet::on) { if (st == S_TITLE) dl("t=%.0f link: %s, title, pods heard %d", sim_t, v1knet::master ? "master" : "slave", v1knet::live_pods());
            else dl("t=%.0f link: %s seat %d st %d left %.0f | me %s(%.0f %.0f) hp %.0f | s0 (%.0f %.0f) s1 %s(%.0f %.0f) s2 %s(%.0f %.0f) s3 %s(%.0f %.0f) | mons %zu kills %d", sim_t, v1knet::master ? "master" : "slave", v1knet::master ? 0 : v1knet::my_seat, (int)st, time_left,
                P.alive ? "" : "down ", P.p.x, P.p.y, P.hp,
                allies.size() > 0 && !v1knet::master ? allies[0].p.x : P.p.x, allies.size() > 0 && !v1knet::master ? allies[0].p.y : P.p.y,
                allies.size() > 1 && allies[1].human ? "person " : "", allies.size() > 1 ? allies[1].p.x : 0.f, allies.size() > 1 ? allies[1].p.y : 0.f,
                allies.size() > 2 && allies[2].human ? "person " : "", allies.size() > 2 ? allies[2].p.x : 0.f, allies.size() > 2 ? allies[2].p.y : 0.f,
                allies.size() > 3 && allies[3].human ? "person " : "", allies.size() > 3 ? allies[3].p.x : 0.f, allies.size() > 3 ? allies[3].p.y : 0.f, mons.size(), kills); } }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".legend").c_str(), "w");
    walk_speed = envf("LEGEND_WALK", walk_speed); game_len = envf("LEGEND_TIME", game_len); game_fps = envf("LEGEND_FPS", game_fps); busy_fps = envf("LEGEND_FPS_BUSY", getenv("LEGEND_FPS") ? game_fps : busy_fps); task_bonus = envf("LEGEND_TASK", task_bonus); dim_t = envf("LEGEND_DIM", dim_t); hp_max = envf("LEGEND_HP", hp_max);
    vanish_t = envf("LEGEND_VANISH", vanish_t); crack_t = envf("LEGEND_CRACK", crack_t); swing_min = envf("LEGEND_SWING", swing_min); mon_hp_scale = envf("LEGEND_MONHP", mon_hp_scale);
    dmg_scale = envf("LEGEND_DMG", dmg_scale); cast_gap = envf("LEGEND_CAST", cast_gap); reach_r = envf("LEGEND_REACH", reach_r); block_k = envf("LEGEND_BLOCK", block_k);
    autoplay = envf("LEGEND_AUTO", 0) != 0;
    v1knet::init("LEGQ", false, dl);   /* start-together party (Bobby: join-in-progress is for Flying Aces and VTOL) */
    d1k::make_hud(); make_flash(); build_world(); make_sounds();
    P = Pl(); P.hp = hp_max;
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    PIC_AddEvent(tick, 2.0);
    dl("Legend Quest rebuild started");
}
}   /* namespace legend */
