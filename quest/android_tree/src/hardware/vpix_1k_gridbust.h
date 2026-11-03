/* Grid Busters (Virtuality 1000CS stand-up, 1993) - native rebuild from footage and press. No game software survives in public, so every model is built here
   and no game file is read. Included after vpix_dactyl1k.h (uses its kit: d1k::Mesh, dput, blit, text, make_hud, make_sound, ...). Dev only, not in a release.
   It is NOT Grid Warriors: separate title, world, rules and player count; Laserquest's marshal names both in one interview (Amiga Computing 69). Play Meter Oct 1993 and Generation 4 #60 mixed the names up. See
   Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\grid.md. Evidence: Octavius King 2YrPW5cU06A t878-946, Nostalgia Nerd _X_E_Kg9roE t1180-1184, 08_footage_standup_games.md section 5.
   Units: mm, seconds. World X right, Y forward, Z up. The camera is the player's head (in_head).
   FROM FOOTAGE: order card, corridor walked toward a lone orange figure, card again, arena (t878-892); the Channel 665 card with BOB BINGLEY over the grey GBL saucer; green faceted floor, black sky, mauve wall with
   separate brown blocks along its foot; long drums on their side with battlements, triangles, chamfered yellow ends, on the floor and high round the rim; yellow pylons; yellow bases with an orange side and a grey crossed crate,
   a tan tapered column with a yellow band and a boxed number sign on top; 3x3 tile clusters; floating plates; line shots in green and lavender; white speckle burst with rings; HUD: clock 300 down to 0, SCORE stepping 12 per hit event, red bar with shield icon,
   crosshair FIXED at the view's centre in every frame, big grey name tag; no gun is ever seen; pale, grey and green robots with tall pale fins; SLAYER pale, THE BEAR green; iris with bar and clock gone while it opens (t925); bots rush the middle.
   FROM PRESS: four bodies in a stadium, 2 or 3 of them computer (Joystick 57, Feb 1995); you turn, move your head, tilt the thrusters, fire (same); weapon mounted on the head (Electronic Entertainment Jan 1994, agrees with the fixed crosshair);
   a stadium crowd cheering others being blown up (The Computer Paper Oct 1994); a soundtrack that adjusts to your situation and says where to go and what to look at (Amiga Computing 69, Jan 1994); rocket launcher, jets (Generation 4 #60, not played by its writer).
   READ FROM FRAMES, meaning unknown: the sign numbers run 1 per second, 197-clock on one sign (073 at 124, 082 at 115) and 212-clock on another (086 at 126, 101 at 111), 000 before that. The floating 0 14 stays 014.
   INVENTED (marked in the code): arena size, jet numbers, rocket look and numbers, damage, respawn, what the bar means, how the round ends, the title line on the arena, how the crowd sounds (synth), the words and voice of the guide (Windows SAPI stand-in),
   all other sounds, how the thruster tilt maps to the hand. Round length: the clock says 300; Play Meter's 3.5 minutes is about the headset in general and Joystick 57 says 4 to 6 at Evry: GRIDBUST_TIME=210 gives the US figure.
   Env: GRIDBUST_AUTO, _CAM, _FPS, _TIME, _CPU, _WALK, _JET, _GRAV, _CLIMB, _BOLT, _ROCKET, _HIT, _DMG, _RDMG, _SPLASH, _RADIUS, _CEIL, _RESPAWN, _GAP, _RGAP, _KILL, _INTRO. Dev: _LOOK=yawdeg,pitchdeg turns the view, _ATTRACT=<secs> holds the attract. */
#include "vpix_1k_gridbust_pics.h"
namespace gridbust {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at;
using d1k::Mesh; using d1k::envf; using d1k::dl; using d1k::make_sound;

static const float PI = 3.14159265f, TAU = 6.2831853f;
static float rnd(float a, float b) { return d1k::rnd(a, b); }

/* ---- tuning ---- */
static float arena_r = 30000.f;            /* INVENTED: footage gives no scale (opponents about 1/20 of the screen height across the field) */
static float wall_h = 8000.f;              /* INVENTED */
static float ceil_z = 15000.f;             /* INVENTED: height limit of the jet pack */
static float walk_speed = 5000.f;          /* INVENTED */
static float jet_acc = 20000.f;            /* INVENTED: thrust; net climb acceleration is this minus gravity */
static float grav = 9800.f;                /* INVENTED */
static float climb_max = 7000.f;           /* INVENTED: footage shows sudden climbs, not measurable */
static float fall_max = 9000.f;            /* INVENTED */
static float bolt_speed = 70000.f;         /* INVENTED: the pulse shot is a fast thin line (footage) */
static float rocket_speed = 24000.f;       /* INVENTED: the rocket is not seen in footage */
static float pulse_gap = 0.18f;            /* INVENTED */
static float rocket_gap = 1.6f;            /* INVENTED */
static float pulse_dmg = 0.035f;            /* INVENTED: share of the shield bar per pulse hit */
static float rocket_dmg = 0.25f;            /* INVENTED */
static float splash_r = 3500.f;            /* INVENTED */
static int hit_score = 12;                 /* footage: the score steps by 12 per hit event */
static int kill_bonus = 0;                 /* INVENTED: none seen */
static float respawn_delay = 4.f;          /* INVENTED */
static float game_len = 300.f;             /* footage: the clock starts at 300 and counts down 1 per second */
static float game_fps = 20.f;              /* guide: 20 at best (the footage measures about 10) */
static float intro_len = 14.f;             /* t878-892: card, corridor, card, as long as the capture shows them (it could be cut) */
static int n_cpu = 3;                      /* four bodies in the stadium, 2 or 3 of them computer (Joystick 57); one seat here, so three bots */

/* ---- meshes ---- */
enum { G_ARENA = 100, G_LIGHTS, G_STARS, G_SAUCER, G_TUNNEL, G_BOLT0, G_BOLT1, G_ROCKET, G_FLAME, G_SPK, G_SPKO,
       G_IRIS, G_SIGN, G_TUNEND, G_FIRE = 120 /* + 5 sizes */, G_FIG = 130 /* + 5 accent colours */, G_DIG = 140 /* + 10 */ };
static const uint32_t C_FLOOR1 = 0x4E8256, C_FLOOR2 = 0x467650, C_FLOOR3 = 0x3E6A48, C_WALL1 = 0x72625E, C_WALL2 = 0x6A5A58, C_WALLB = 0x7C6056, C_RIM = 0x7C74B0, C_RIM2 = 0x68609C, C_PURPLE = 0x7A72B0, C_PURPLE2 = 0x635B9A, C_PURPLE3 = 0x4C4684, C_TRI = 0x524C8C,
                      C_YEL = 0xD0B040, C_YEL2 = 0xA88C30, C_CRATE = 0x8C8A82, C_CRATE2 = 0xA8A69E, C_PLATE = 0x4C5078, C_TOWER = 0xC05838;
struct Ob { float x, y, z, r, h; };
static std::vector<Ob> obs;
static const float TX = 300000.f;          /* the corridor sits far off to the side */
static const float TUN_SEG = 4400.f; static const int TUN_N = 28;

/* every face names the side it is seen from */
static void gq(Mesh &m, V a, V b, V c, V d, V want, uint32_t rgb) { if (dot(cross(b - a, c - a), want) >= 0) m.poly({a, b, c, d}, rgb); else m.poly({d, c, b, a}, rgb); }
static void gt(Mesh &m, V a, V b, V c, V want, uint32_t rgb) { if (dot(cross(b - a, c - a), want) >= 0) m.poly({a, b, c}, rgb); else m.poly({c, b, a}, rgb); }
static void gb_obb(Mesh &m, V c, V ax, V ay, float hx, float hy, float hz, uint32_t top, uint32_t side, uint32_t low) {   /* a box round c, axes ax, ay, z up */
    V az(0, 0, 1); V p[8]; for (int i = 0; i < 8; i++) p[i] = c + ax * ((i & 1) ? hx : -hx) + ay * ((i & 2) ? hy : -hy) + az * ((i & 4) ? hz : -hz);
    gq(m, p[4], p[5], p[7], p[6], az, top); gq(m, p[0], p[1], p[3], p[2], V(0, 0, 0) - az, low); gq(m, p[0], p[1], p[5], p[4], V(0, 0, 0) - ay, side); gq(m, p[2], p[3], p[7], p[6], ay, side); gq(m, p[0], p[2], p[6], p[4], V(0, 0, 0) - ax, side); gq(m, p[1], p[3], p[7], p[5], ax, side);
}
/* the drum, the arena's main shape: lying on its side, battlements on top, big dark triangles on the flanks, chamfered yellow ends. c is its middle, ax its long axis */
static void gb_drum(Mesh &m, V c, V ax, float L, float R, bool solid = true) {
    const V ay = norm(cross(V(0, 0, 1), ax)), az(0, 0, 1); const int N = 8; const float ang0 = PI / 8.f;
    auto rim = [&](float x, float r, int i) { float a = ang0 + TAU * i / N; return c + ax * x + ay * (cosf(a) * r) + az * (sinf(a) * r); };
    for (int i = 0; i < N; i++) { int j = (i + 1) % N; float a = ang0 + TAU * (i + 0.5f) / N, up = sinf(a); uint32_t col = up > 0.5f ? C_PURPLE : up < -0.5f ? C_PURPLE3 : C_PURPLE2; V out = ay * cosf(a) + az * sinf(a);
        gq(m, rim(-L, R, i), rim(L, R, i), rim(L, R, j), rim(-L, R, j), out, col);
        for (int e = -1; e <= 1; e += 2) { const V w = ax * (float)e;   /* the yellow end: a wider collar, then the face */
            gq(m, rim(e * L, R, i), rim(e * L, R, j), rim(e * (L + R * 0.18f), R * 1.28f, j), rim(e * (L + R * 0.18f), R * 1.28f, i), out * 0.7f - w * 0.7f, C_YEL);
            gq(m, rim(e * (L + R * 0.18f), R * 1.28f, i), rim(e * (L + R * 0.18f), R * 1.28f, j), rim(e * (L + R * 0.5f), R * 1.12f, j), rim(e * (L + R * 0.5f), R * 1.12f, i), out * 0.7f + w * 0.7f, up > 0 ? C_YEL : C_YEL2);
            gt(m, c + ax * (e * (L + R * 0.5f)), rim(e * (L + R * 0.5f), R * 1.12f, i), rim(e * (L + R * 0.5f), R * 1.12f, j), w, C_YEL2); } }
    const int T = (int)(L * 2.f / (R * 2.2f)); for (int k = 0; k < T; k++) { float x = -L + (k + 0.5f) * (2.f * L / T);     /* the triangle emblems */
        for (int s2 = -1; s2 <= 1; s2 += 2) { V o = c + ax * x + ay * (s2 * R * 0.955f); float w = R * 0.62f, h = R * 0.62f; gt(m, o + ax * (-w) + az * (-h), o + ax * w + az * (-h), o + az * h, ay * (float)s2, C_TRI); } }
    const int B = (int)(L * 2.f / (R * 1.5f)); for (int k = 0; k < B; k++) if (k % 2 == 0 || k == B - 1) { float x = -L + (k + 0.5f) * (2.f * L / B), wx = (2.f * L / B) * 0.5f;   /* the battlements */
        gb_obb(m, c + ax * x + az * (R * 1.12f), ax, ay, wx, R * 0.6f, R * 0.32f, C_PURPLE, C_PURPLE2, C_PURPLE3); }
    if (solid && c.z - R < 3000.f) { int n = (int)(L / R) + 1; for (int k = 0; k <= n * 2; k++) { V q = c + ax * (-L + L * k / n); obs.push_back({q.x, q.y, 0.f, R * 1.05f, c.z + R * 1.2f}); } }
}
/* the yellow base with orange sides and a grey crossed crate, a tan column tapering up with a yellow band, the place for a number sign on top (measured at t932: base 8400 across, column 3800 tall, sign 4600 wide) */
static const float TW_H = 2100.f, TW_C = 3800.f, TW_SIGN = 7000.f;
static void gb_tower(Mesh &m, V at) {
    const float B = 4200.f, Tp = 2700.f, H = TW_H; const V cr[4] = {V(-1, -1, 0), V(1, -1, 0), V(1, 1, 0), V(-1, 1, 0)};
    for (int i = 0; i < 4; i++) { V a = at + cr[i] * B, b = at + cr[(i + 1) & 3] * B, c = at + cr[(i + 1) & 3] * Tp + V(0, 0, H), d = at + cr[i] * Tp + V(0, 0, H), out = (a + b) * 0.5f - at + V(0, 0, 3000); gq(m, a, b, c, d, out, (i & 1) ? C_YEL : 0xC88030);
        V n = norm(V(out.x, out.y, 0)); V u = norm(b - a); V o = (a + b) * 0.5f + n * 60.f;   /* the crate stands in front of the slope, a real box */
        const V cc = o - n * 900.f; gb_obb(m, cc + V(0, 0, 800.f), u, n, 1500.f, 950.f, 800.f, C_CRATE2, C_CRATE, 0x5C5A54);
        const V f = cc + n * 958.f; const uint32_t K = 0x3C3A36; gq(m, f + u * -1400.f + V(0, 0, 1500), f + u * -1250.f + V(0, 0, 1500), f + u * 1250.f + V(0, 0, 100), f + u * 1400.f + V(0, 0, 100), n, K); gq(m, f + u * 1400.f + V(0, 0, 1500), f + u * 1250.f + V(0, 0, 1500), f + u * -1250.f + V(0, 0, 100), f + u * -1400.f + V(0, 0, 100), n, K); }
    gq(m, at + cr[0] * Tp + V(0, 0, H), at + cr[1] * Tp + V(0, 0, H), at + cr[2] * Tp + V(0, 0, H), at + cr[3] * Tp + V(0, 0, H), V(0, 0, 1), C_YEL);
    const float C0 = 1050.f, C1 = 480.f, Z = H + TW_C, Zb = H + TW_C * 0.55f, Cb = C0 + (C1 - C0) * 0.55f;
    for (int i = 0; i < 4; i++) { V a = cr[i], b = cr[(i + 1) & 3], out = (a + b) * 0.5f; uint32_t c = (i & 1) ? 0xA87C5C : 0x8C664C;
        gq(m, at + a * C0 + V(0, 0, H), at + b * C0 + V(0, 0, H), at + b * Cb + V(0, 0, Zb), at + a * Cb + V(0, 0, Zb), out, c);
        gq(m, at + a * Cb + V(0, 0, Zb), at + b * Cb + V(0, 0, Zb), at + b * (Cb - 40.f) + V(0, 0, Zb + 220.f), at + a * (Cb - 40.f) + V(0, 0, Zb + 220.f), out, C_YEL);
        gq(m, at + a * (Cb - 40.f) + V(0, 0, Zb + 220.f), at + b * (Cb - 40.f) + V(0, 0, Zb + 220.f), at + b * C1 + V(0, 0, Z), at + a * C1 + V(0, 0, Z), out, c); }
    gq(m, at + cr[0] * C1 + V(0, 0, Z), at + cr[1] * C1 + V(0, 0, Z), at + cr[2] * C1 + V(0, 0, Z), at + cr[3] * C1 + V(0, 0, Z), V(0, 0, 1), 0x8C664C);
    obs.push_back({at.x, at.y, at.z, B * 0.85f, H});
}
static void gb_plate(Mesh &m, V at, float w, float d) {   /* a floating slate-blue plate with a yellow edge, seen tilted against the black */
    m.box(at + V(-w, -d, 0), at + V(w, d, 180.f), C_PLATE, C_YEL2, 0x383C5C);
    m.box(at + V(-w - 140.f, -d - 140.f, 0), at + V(-w, d + 140.f, 190.f), C_YEL); m.box(at + V(w, -d - 140.f, 0), at + V(w + 140.f, d + 140.f, 190.f), C_YEL);
    m.box(at + V(-w, -d - 140.f, 0), at + V(w, -d, 190.f), C_YEL); m.box(at + V(-w, d, 0), at + V(w, d + 140.f, 190.f), C_YEL);
    obs.push_back({at.x, at.y, at.z, (std::max)(w, d), 190.f});
}
static void gb_pylon(Mesh &m, V at, float r, float h) {    /* a yellow truncated cone by the wall */
    const int N = 4; for (int i = 0; i < N; i++) { float a = TAU * (i + 0.5f) / N, b = TAU * (i + 1.5f) / N; V p0 = at + V(cosf(a) * r, sinf(a) * r, 0), p1 = at + V(cosf(b) * r, sinf(b) * r, 0), p2 = at + V(cosf(b) * r * 0.22f, sinf(b) * r * 0.22f, h), p3 = at + V(cosf(a) * r * 0.22f, sinf(a) * r * 0.22f, h);
        gq(m, p0, p1, p2, p3, (p0 + p1) * 0.5f - at, (i & 1) ? C_YEL : C_YEL2); } gq(m, at + V(r * 0.16f, r * 0.16f, h), at + V(-r * 0.16f, r * 0.16f, h), at + V(-r * 0.16f, -r * 0.16f, h), at + V(r * 0.16f, -r * 0.16f, h), V(0, 0, 1), C_YEL);
    obs.push_back({at.x, at.y, at.z, r * 0.6f, h});
}
static V sign_at[4]; static float sign_yaw[4];
static void gb_build_arena() {
    obs.clear(); Mesh m; const int N = 40; const float Rin[4] = {0.f, 0.33f, 0.66f, 1.f};
    std::mt19937 r3(5);
    for (int i = 0; i < N; i++) {   /* a faceted floor: three rings of triangles in slightly different greens */
        float a0 = TAU * i / N, a1 = TAU * (i + 1) / N; V p0(cosf(a0) * arena_r, sinf(a0) * arena_r, 0), p1(cosf(a1) * arena_r, sinf(a1) * arena_r, 0);
        for (int q = 0; q < 3; q++) { const uint32_t cols[3] = {C_FLOOR1, C_FLOOR2, C_FLOOR3}; V i0 = p0 * Rin[q], i1 = p1 * Rin[q], o0 = p0 * Rin[q + 1], o1 = p1 * Rin[q + 1];
            gt(m, i0, o0, o1, V(0, 0, 1), cols[r3() % 3]); if (q > 0) gt(m, i0, o1, i1, V(0, 0, 1), cols[r3() % 3]); }
        V h(0, 0, wall_h), in = V(0, 0, 0) - (p0 + p1) * 0.5f, b(0, 0, 3000.f), n1 = norm(in) * 900.f;
        gq(m, p1, p0, p0 + h, p1 + h, in, (i & 1) ? C_WALL1 : C_WALL2);
        if (i % 4 != 3) { gq(m, p1 + n1, p0 + n1, p0 + n1 + b, p1 + n1 + b, in, C_WALLB); gq(m, p1 + n1 + b, p0 + n1 + b, p0 + b, p1 + b, V(0, 0, 1), 0x8C7264);     /* the brown blocks along the wall's foot (t931-934): real blocks, gaps between them */
            if (i % 4 == 0) { V sd = norm(p1 - p0); gq(m, p1 + n1, p1, p1 + b, p1 + n1 + b, sd, 0x5C4840); } if (i % 4 == 2) { V sd = norm(p0 - p1); gq(m, p0 + n1, p0, p0 + b, p0 + n1 + b, sd, 0x5C4840); } }
    }
    /* drums (layout INVENTED, shape from the frames): along the rim high up, standing on the floor in the field, one big one near the middle */
    const int RM = 10; for (int k = 0; k < RM; k++) { float a = TAU * (k + 0.5f) / RM, R = arena_r - 4500.f; V c(cosf(a) * R, sinf(a) * R, 11500.f), t(-sinf(a), cosf(a), 0); gb_drum(m, c, t, 4800.f, 1250.f, false); }
    auto shadow = [&](V c, V ax, float L, float W) { V ay = norm(cross(V(0, 0, 1), ax)); gq(m, c + ax * -L + ay * -W + V(0, 0, 14), c + ax * L + ay * -W + V(0, 0, 14), c + ax * L + ay * W + V(0, 0, 14), c + ax * -L + ay * W + V(0, 0, 14), V(0, 0, 1), 0x24402C); };
    const int FL = 6; for (int k = 0; k < FL; k++) { float a = TAU * (k + 0.2f) / FL, R = (k & 1) ? 22000.f : 14000.f; V c(cosf(a) * R, sinf(a) * R, 1300.f), t(-sinf(a + 0.5f), cosf(a + 0.5f), 0); gb_drum(m, c, t, 3600.f, 1300.f); }
    gb_drum(m, V(6000, 2000, 2300.f), norm(V(1, 0.3f, 0)), 6500.f, 2300.f);
    /* towers with number signs, yellow pyramids, pylons */
    for (int k = 0; k < 4; k++) { float a = TAU * (k + 0.1f) / 4.f, R = arena_r - 7000.f; V at(cosf(a) * R, sinf(a) * R, 0); gb_tower(m, at); sign_at[k] = at + V(0, 0, TW_SIGN); sign_yaw[k] = atan2f(at.x, -at.y) + 0.4f; }
    for (int k = 0; k < 12; k++) { float a = TAU * (k + 0.62f) / 12.f, R = arena_r - 1600.f; gb_pylon(m, V(cosf(a) * R, sinf(a) * R, 0), 1500.f, 5200.f); }
    for (int k = 0; k < 6; k++) { float a = TAU * (k + 0.3f) / 6.f; gb_pylon(m, V(cosf(a) * 9500.f, sinf(a) * 9500.f, 0), 900.f, 2600.f); }
    gb_plate(m, V(9500, -7500, 6800), 2600.f, 1200.f); gb_plate(m, V(-9500, 7500, 6800), 2600.f, 1200.f); shadow(V(9500, -7500, 0), V(1, 0, 0), 2400.f, 1000.f); shadow(V(-9500, 7500, 0), V(1, 0, 0), 2400.f, 1000.f);
    gb_plate(m, V(0, 0, 9500), 2400.f, 2400.f); gb_plate(m, V(-14000, -9000, 5200), 2200.f, 1200.f); shadow(V(-14000, -9000, 0), V(1, 0, 0), 2000.f, 1000.f);
    m.add(G_ARENA);
    dl("gridbust arena mesh: %zu polys", m.p.size());
    Mesh L; const float s = 800.f, gap = 2300.f, z = 23000.f;   /* the clusters of 3 x 3 pale tiles high overhead */
    const V cl[5] = {V(0, 0, 0), V(13000, 9000, 0), V(-13000, 9000, 0), V(13000, -9000, 0), V(-13000, -9000, 0)}; const float a45 = 0.7f;
    for (int c = 0; c < 5; c++) for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) { float u = i * gap, w = j * gap; V o = cl[c] + V(u * cosf(a45) - w * sinf(a45), u * sinf(a45) + w * cosf(a45), z);
        V ex(cosf(a45) * s, sinf(a45) * s, 0), ey(-sinf(a45) * s, cosf(a45) * s, 0);
        gq(L, o - ex - ey, o + ex - ey, o + ex + ey, o - ex + ey, V(0, 0, -1), 0xE4E0D8);
        gq(L, o - ex * 1.25f - ey * 1.25f + V(0, 0, 200), o + ex * 1.25f - ey * 1.25f + V(0, 0, 200), o + ex - ey, o - ex - ey, V(0, 0, -1) - ey, 0x8C8880); gq(L, o - ex * 1.25f + ey * 1.25f + V(0, 0, 200), o - ex * 1.25f - ey * 1.25f + V(0, 0, 200), o - ex - ey, o - ex + ey, V(0, 0, -1) - ex, 0xA8A49C);
        gq(L, o + ex * 1.25f + ey * 1.25f + V(0, 0, 200), o - ex * 1.25f + ey * 1.25f + V(0, 0, 200), o - ex + ey, o + ex + ey, V(0, 0, -1) + ey, 0x70706A); gq(L, o + ex * 1.25f - ey * 1.25f + V(0, 0, 200), o + ex * 1.25f + ey * 1.25f + V(0, 0, 200), o + ex + ey, o + ex - ey, V(0, 0, -1) + ex, 0x8C8880); }
    L.add(G_LIGHTS);
    /* the number sign: dark with a yellow edge; ten digit plates in front of it, white */
    { Mesh g; const float W = 2300.f, H = 900.f; g.box(V(-W, -60, -H), V(W, 60, H), 0x14141A); const uint32_t Y = 0xD8B840;
      g.box(V(-W - 130, -80, -H - 130), V(W + 130, 80, -H), Y); g.box(V(-W - 130, -80, H), V(W + 130, 80, H + 130), Y); g.box(V(-W - 130, -80, -H), V(-W, 80, H), Y); g.box(V(W, -80, -H), V(W + 130, 80, H), Y); g.add(G_SIGN); }
    static const unsigned char SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    for (int d = 0; d < 10; d++) { Mesh g; const float a = 420.f, b = 640.f, w = 70.f; const uint32_t c = 0xF4F4F4;
        auto seg = [&](float x0, float z0, float x1, float z1) { g.box(V(x0 - w, -30, z0 - w), V(x1 + w, 30, z1 + w), c); };
        if (SEG[d] & 1) seg(-a, b, a, b); if (SEG[d] & 2) seg(a, 0, a, b); if (SEG[d] & 4) seg(a, -b, a, 0); if (SEG[d] & 8) seg(-a, -b, a, -b); if (SEG[d] & 16) seg(-a, -b, -a, 0); if (SEG[d] & 32) seg(-a, 0, -a, b); if (SEG[d] & 64) seg(-a, 0, a, 0);
        g.add(G_DIG + d); }
    /* the iris seen at a respawn (t925): a black ring round the view, scaled to open or close */
    { Mesh g; const int NI = 32; for (int i = 0; i < NI; i++) { float a = TAU * i / NI, b = TAU * (i + 1) / NI; gq(g, V(cosf(a), 0, sinf(a)), V(cosf(b), 0, sinf(b)), V(cosf(b) * 40.f, 0, sinf(b) * 40.f), V(cosf(a) * 40.f, 0, sinf(a) * 40.f), V(0, -1, 0), 0x000000); } g.add(G_IRIS); }
}
static void gb_build_attract() {
    Mesh st; std::mt19937 r2(11);
    for (int i = 0; i < 220; i++) {
        float z = (r2() / (float)(r2.max)()) * 2.f - 1.f, a = (r2() / (float)(r2.max)()) * TAU, q = sqrtf(1.f - z * z), R = 400000.f, s = 900.f;
        V c(cosf(a) * q * R, sinf(a) * q * R, z * R), e0 = norm(cross(c, V(0.3f, 0.5f, 0.8f))), e1 = norm(cross(c, e0));
        st.poly({c - e0 * s, c - e1 * s, c + e0 * s, c + e1 * s}, 0xA8A8A8 + (r2() % 3) * 0x202020);
    }
    st.add(G_STARS);
    /* the saucer as seen at t878-892: a thick dark ring round an open well with the green arena down inside, GBL in yellow on the flank, white marks on the rim, three fins with white tips */
    Mesh m; const int N = 28; const float Ro = 15500.f, Rt = 13600.f, Rw = 10800.f, Rb = 9500.f, Zt = 2900.f, Zm = 300.f, Zb = -3400.f, Zw = -2500.f;
    for (int i = 0; i < N; i++) {
        float a0 = TAU * i / N, a1 = TAU * (i + 1) / N; auto P = [&](float a, float r, float z) { return V(cosf(a) * r, sinf(a) * r, z); };
        V up(0, 0, 1), out(cosf(a0), sinf(a0), 0);
        gq(m, P(a0, Rw, Zt), P(a1, Rw, Zt), P(a1, Rt, Zt), P(a0, Rt, Zt), up, 0x6C6660);
        gq(m, P(a0, Rt, Zt), P(a1, Rt, Zt), P(a1, Ro, Zm), P(a0, Ro, Zm), out + up, (i & 1) ? 0x56504E : 0x524C4A);
        gq(m, P(a0, Ro, Zm), P(a1, Ro, Zm), P(a1, Rb, Zb), P(a0, Rb, Zb), out - up, 0x4E4846);
        gt(m, V(0, 0, Zb), P(a0, Rb, Zb), P(a1, Rb, Zb), V(0, 0, 0) - up, 0x3C3834);
        gq(m, P(a0, Rw, Zt), P(a1, Rw, Zt), P(a1, Rw, Zw), P(a0, Rw, Zw), V(0, 0, 0) - out, 0x48443E);
        gt(m, V(0, 0, Zw), P(a0, Rw, Zw), P(a1, Rw, Zw), up, (i & 1) ? 0x3C8C48 : 0x347C40);
        if (i % 3 == 0) gq(m, P(a0 + 0.04f, Ro - 500.f, Zm + 460.f), P(a0 + 0.08f, Ro - 500.f, Zm + 460.f), P(a0 + 0.08f, Ro - 900.f, Zm + 760.f), P(a0 + 0.04f, Ro - 900.f, Zm + 760.f), out + up, 0xE8E8E0);
    }
    for (int k = 0; k < 8; k++) { float a = TAU * k / 8.f + 0.2f; V c(cosf(a) * 6000.f, sinf(a) * 6000.f, Zw); static const uint32_t bits[4] = {0xC04030, 0xD8D8D0, 0xC8B040, 0x7070B0}; m.box(c - V(500, 500, 0), c + V(500, 500, 900), bits[k & 3]); }   /* colours moving on the arena floor, seen from the card */
    for (int k = 0; k < 3; k++) { float a = TAU * k / 3.f - PI * 0.5f + 0.75f, w = 0.3f; V o(cosf(a), sinf(a), 0);   /* three blunt wedge fins with thickness; a white underside on one, seen as a white triangle at t886 */
        V L0 = V(cosf(a - w), sinf(a - w), 0) * (Ro - 600.f), R0 = V(cosf(a + w), sinf(a + w), 0) * (Ro - 600.f), tq = o * (Ro + 4600.f), sd = V(-o.y, o.x, 0) * 1500.f, TL = tq - sd, TR = tq + sd; const V zh(0, 0, 500), z0(0, 0, -500), zt(0, 0, 60);
        const V a0 = L0 + z0 + zh, a1 = R0 + z0 + zh, b0 = L0 + z0 - zh, b1 = R0 + z0 - zh, c0 = TL + z0 + zt, c1 = TR + z0 + zt, d0 = TL + z0 - zt, d1 = TR + z0 - zt;
        gq(m, a0, a1, c1, c0, V(0, 0, 1), 0x6C6660); gq(m, b0, b1, d1, d0, V(0, 0, -1), k == 0 ? 0xE8E8E0 : 0x3C3834); gq(m, a0, c0, d0, b0, sd * -1.f, 0x4C4844); gq(m, a1, c1, d1, b1, sd, 0x4C4844); gq(m, c0, c1, d1, d0, o, 0x5A5450); }
    const float cell = 400.f, ch = cell * 1.3f; const int letters[3] = {6, 1, 11};   /* G B L, big and yellow on the flank */
    for (int li = 0; li < 3; li++) { float ac = -PI * 0.5f + (li - 1) * 0.175f;
        for (int r = 0; r < 7; r++) for (int c = 0; c < 5; c++) if (d1k::FONT[letters[li]][r * 5 + c] == '1') {
            float a = ac + (c - 2) * cell / Ro, a2 = a + cell / Ro; float z = Zm + 150.f + (6 - r) * ch * 0.5f; auto rat = [&](float zz) { return zz >= Zm ? Ro + (Rt - Ro) * (zz - Zm) / (Zt - Zm) : Ro + (Rb - Ro) * (zz - Zm) / (Zb - Zm); };
            float z1 = z + ch * 0.5f, r0 = rat(z) + 70.f, r1 = rat(z1) + 70.f;
            V p0(cosf(a) * r0, sinf(a) * r0, z), p1(cosf(a2) * r0, sinf(a2) * r0, z), p2(cosf(a2) * r1, sinf(a2) * r1, z1), p3(cosf(a) * r1, sinf(a) * r1, z1); gq(m, p0, p1, p2, p3, V(cosf(a), sinf(a), 0.2f), 0xD8C040); } }
    for (int k = 0; k < 12; k++) { float a = TAU * k / 12.f + 0.13f * (k % 3), r0 = 4000.f + (k % 3) * 2500.f, z0 = 9000.f + ((k * 37) % 5) * 2200.f, w = 75.f; V o(cosf(a), sinf(a), 0), p = V(-o.y, o.x, 0) * w, A = o * r0 + V(0, 0, z0), B = o * (r0 + 500.f) + V(0, 0, z0 + 1300.f);   /* READ FROM FRAMES: pale dashes flying up out of the well */
        m.poly({A - p, A + p, B + p, B - p}, 0xD8D8C8); m.poly({B - p, B + p, A + p, A - p}, 0xD8D8C8); }
    m.add(G_SAUCER);
    /* the corridor of t881-884: mauve bays either side, a dark eight-sided roof, olive floor, oval lights over ovals on the floor, a black opening far away */
    Mesh t; const float W = 4400.f, H = 3100.f, Hm = 2100.f, Wr = 2700.f;
    auto oval = [&](float y, float z, float rx, float ry, uint32_t c, V want) { const int NN = 10; V cen(TX, y, z); for (int i = 0; i < NN; i++) { float a = TAU * i / NN, b = TAU * (i + 1) / NN; gt(t, cen, cen + V(cosf(a) * rx, sinf(a) * ry, 0), cen + V(cosf(b) * rx, sinf(b) * ry, 0), want, c); } };
    for (int s2 = 0; s2 < TUN_N; s2++) { float y0 = s2 * TUN_SEG, y1 = y0 + TUN_SEG; const V p0(TX, y0, 0), p1(TX, y1, 0); uint32_t c = 0x483C44, c2 = 0x3C3932, rf = 0x1E1C1E, rf2 = 0x2C2826;
        gq(t, V(TX - W, y0, 0), V(TX + W, y0, 0), V(TX + W, y1, 0), V(TX - W, y1, 0), V(0, 0, 1), c2);
        gq(t, V(TX - Wr, y0, H), V(TX + Wr, y0, H), V(TX + Wr, y1, H), V(TX - Wr, y1, H), V(0, 0, -1), rf);
        for (int sd = -1; sd <= 1; sd += 2) { float q = (float)sd; V in(-q, 0, 0); gq(t, V(TX + q * W, y0, 0), V(TX + q * W, y1, 0), V(TX + q * W, y1, Hm), V(TX + q * W, y0, Hm), in, c);
            gq(t, V(TX + q * W, y0, Hm), V(TX + q * W, y1, Hm), V(TX + q * Wr, y1, H), V(TX + q * Wr, y0, H), V(-q, 0, -1), rf2);
            if (s2 % 3 == 0) gq(t, V(TX + q * W, y0, 0), V(TX + q * (W - 800.f), y0, 0), V(TX + q * (W - 800.f), y0, Hm), V(TX + q * W, y0, Hm), V(0, -1, 0), 0x2A2428);
            if (s2 % 3 == 0) gq(t, V(TX + q * (W - 800.f), y0, 0), V(TX + q * (W - 800.f), y0 + 1400.f, 0), V(TX + q * (W - 800.f), y0 + 1400.f, Hm), V(TX + q * (W - 800.f), y0, Hm), in, 0x342C30);
            if (s2 % 3 == 0) gq(t, V(TX + q * W, y0 + 1400.f, 0), V(TX + q * (W - 800.f), y0 + 1400.f, 0), V(TX + q * (W - 800.f), y0 + 1400.f, Hm), V(TX + q * W, y0 + 1400.f, Hm), V(0, 1, 0), 0x241E22); }
        if (s2 % 3 == 1) { float yc = y0 + TUN_SEG * 0.5f; oval(yc, H - 8.f, 1250.f, 650.f, 0x7C7870, V(0, 0, -1)); oval(yc, H - 16.f, 760.f, 380.f, 0xC8C4B8, V(0, 0, -1)); oval(yc, 8.f, 1700.f, 780.f, 0x524E40, V(0, 0, 1)); oval(yc, 14.f, 920.f, 420.f, 0x625E4C, V(0, 0, 1)); } }
    { float y = TUN_N * TUN_SEG; t.poly({V(TX + W, y, 0), V(TX - W, y, 0), V(TX - W, y, H), V(TX + W, y, H)}, 0x060606); }
    t.add(G_TUNNEL);
    { Mesh e; e.poly({V(TX + W, 0, 0), V(TX - W, 0, 0), V(TX - W, 0, H), V(TX + W, 0, H)}, 0x060606); e.poly({V(TX + W, 0, H), V(TX - W, 0, H), V(TX - W, 0, 0), V(TX + W, 0, 0)}, 0x060606); e.add(G_TUNEND); }   /* the black opening stays the same size as the camera walks (t881-884) */
}
static void gb_build_actors() {
    for (int k = 0; k < 2; k++) { Mesh b; uint32_t col = k ? 0x54B860 : 0x7874D0; const float w = 30.f, L = 3600.f;   /* long thin lines, green or lavender (t909-911) */
        b.poly({V(-w, -L, 0), V(w, -L, 0), V(w, 0, 0), V(-w, 0, 0)}, col); b.poly({V(-w, 0, 0), V(w, 0, 0), V(w, -L, 0), V(-w, -L, 0)}, col);
        b.poly({V(0, -L, -w), V(0, -L, w), V(0, 0, w), V(0, 0, -w)}, col); b.poly({V(0, 0, -w), V(0, 0, w), V(0, -L, w), V(0, -L, -w)}, col);
        b.add(G_BOLT0 + k); }
    { Mesh r;   /* INVENTED rocket: not seen in the footage */
      r.box(V(-45, -300, -45), V(45, 250, 45), 0xB0B0B8, 0x909098, 0x707078);
      r.poly({V(-45, 250, -45), V(45, 250, -45), V(0, 450, 0)}, 0xD8301C); r.poly({V(45, 250, -45), V(45, 250, 45), V(0, 450, 0)}, 0xB02818);
      r.poly({V(45, 250, 45), V(-45, 250, 45), V(0, 450, 0)}, 0xD8301C); r.poly({V(-45, 250, 45), V(-45, 250, -45), V(0, 450, 0)}, 0xB02818);
      r.poly({V(-140, -300, 0), V(140, -300, 0), V(0, -80, 0)}, 0x404048); r.poly({V(0, -300, -140), V(0, -300, 140), V(0, -80, 0)}, 0x404048);
      r.add(G_ROCKET); }
    { Mesh fl; for (int k = 0; k < 2; k++) { float x = k ? 90.f : -90.f; const int N = 6; V top(x, -200, 800.f), ap(x, -200, 100.f);   /* two flames pointing down from the nozzles */
        for (int i = 0; i < N; i++) { float a = TAU * i / N, b = TAU * (i + 1) / N;
            fl.poly({ap + V(cosf(a) * 60.f, sinf(a) * 60.f, 0) + V(0, 0, 700.f), ap + V(cosf(b) * 60.f, sinf(b) * 60.f, 0) + V(0, 0, 700.f), ap + V(0, 0, 0)}, (i & 1) ? 0xFFB020 : 0xFF6A10); } (void)top; }
      fl.add(G_FLAME); }
    for (int k = 0; k < 2; k++) { Mesh s; uint32_t c = k ? 0xB8B0D0 : 0xE8E8E0;   /* white-grey and lilac sparkles */
        s.poly({V(-22, 0, -14), V(22, 0, -14), V(0, 0, 44)}, c); s.poly({V(0, 0, 44), V(22, 0, -14), V(-22, 0, -14)}, c); s.poly({V(0, -22, -14), V(0, 22, -14), V(0, 0, 44)}, c); s.poly({V(0, 0, 44), V(0, 22, -14), V(0, -22, -14)}, c); s.add(G_SPK + k); }
    const float fr[5] = {180.f, 320.f, 520.f, 760.f, 1000.f};
    for (int k = 0; k < 5; k++) { Mesh f; float r = fr[k]; std::mt19937 rf(31 + k); auto u = [&]() { return (rf() / (float)(rf.max)()) * 2.f - 1.f; };   /* a cloud of white and lilac specks with a couple of pale rings (t910-912); no fireball in any frame we have */
        for (int n = 0; n < 60; n++) { V c(u() * r, u() * r, u() * r); float w = 24.f + r * 0.03f; uint32_t col = (n & 3) ? 0xE8E4E8 : 0xA89CC8; f.poly({c + V(-w, 0, -w), c + V(w, 0, -w), c + V(0, 0, w)}, col); f.poly({c + V(0, 0, w), c + V(w, 0, -w), c + V(-w, 0, -w)}, col); f.poly({c + V(0, -w, -w), c + V(0, w, -w), c + V(0, 0, w)}, col); f.poly({c + V(0, 0, w), c + V(0, w, -w), c + V(0, -w, -w)}, col); }
        for (int e = 0; e < 2; e++) { V c(e ? r * 0.5f : -r * 0.4f, 0, e ? -r * 0.4f : r * 0.3f); float R = r * 0.42f, T = R * 0.75f; const int NR = 10; for (int n = 0; n < NR; n++) { float a = TAU * n / NR, b = TAU * (n + 1) / NR; V a0 = c + V(cosf(a) * T, 0, sinf(a) * T), a1 = c + V(cosf(a) * R, 0, sinf(a) * R), b0 = c + V(cosf(b) * T, 0, sinf(b) * T), b1 = c + V(cosf(b) * R, 0, sinf(b) * R); f.poly({a0, a1, b1, b0}, 0xD8D4D8); f.poly({b0, b1, a1, a0}, 0xD8D4D8); } }
        f.add(G_FIG - 10 + k); }
    /* thin-limbed robots with a gun and pale parts (footage: green at t1182-84, tan-white at t896, t912, one orange in the corridor at t881); the chest accent tells the players apart (INVENTED). The last is the corridor's orange figure */
    static const uint32_t acc[5] = {0xE8E8E8, 0xD8D8C8, 0xC8C8A0, 0xE8E8E8, 0xE8A050};
    for (int k = 0; k < 5; k++) { Mesh m; uint32_t A = acc[k]; const uint32_t G1 = k == 4 ? 0xD87838 : k == 1 ? 0xD8D8D0 : k == 2 ? 0x9C9CB0 : 0x58C050, G2 = k == 4 ? 0xB85C28 : k == 1 ? 0xB0B0A8 : k == 2 ? 0x7C7C94 : 0x3C9C40, GD = k == 4 ? 0x884018 : k == 1 ? 0x848480 : k == 2 ? 0x5C5C74 : 0x2C7A30, GR = k == 4 ? 0x905030 : 0xA8A8A0, BK = 0x2A2A30;   /* SLAYER pale (t896), THE BEAR green (t925), METALHEAD greyer (t922) */
        m.box(V(-130, -40, 90), V(-70, 40, 500), G2, GD, GD); m.box(V(70, -40, 90), V(130, 40, 500), G2, GD, GD);            /* shins */
        m.box(V(-145, -60, 480), V(-55, 60, 880), G1, G2, GD); m.box(V(55, -60, 480), V(145, 60, 880), G1, G2, GD);           /* thighs */
        m.box(V(-150, -80, 0), V(-50, 170, 90), BK); m.box(V(50, -80, 0), V(150, 170, 90), BK);                              /* boots */
        m.box(V(-130, -70, 860), V(130, 70, 1100), G2, GD, GD); m.box(V(-210, -100, 1100), V(210, 110, 1470), G1, G2, GD); m.box(V(-130, 110, 1180), V(130, 126, 1400), A);    /* waist, chest, plate */
        m.box(V(-70, -60, 1470), V(70, 60, 1540), GD); m.box(V(-95, -95, 1540), V(95, 105, 1740), A, G2, GD); m.box(V(-75, 105, 1600), V(75, 118, 1670), BK);   /* neck, pale helmet, visor */
        m.box(V(-150, -270, 950), V(150, -100, 1480), GR, 0x70706C, 0x484846);                                                 /* jet pack */
        m.box(V(-130, -250, 800), V(-50, -170, 950), BK); m.box(V(50, -250, 800), V(130, -170, 950), BK);
        if (k != 4) { m.box(V(-150, -240, 1480), V(-85, -160, 2200), 0xD8D8D0, 0xB8B8B0, 0x90908C); m.box(V(85, -240, 1480), V(150, -160, 2200), 0xD8D8D0, 0xB8B8B0, 0x90908C); }   /* the two tall pale fins (t910) */
        m.box(V(-275, -35, 920), V(-210, 35, 1440), G2, GD, GD);                                                                /* left arm, thin */
        m.box(V(210, -30, 1280), V(275, 560, 1350), G2, GD, GD); m.box(V(195, 500, 1240), V(290, 1120, 1370), 0xC8C8C0, 0x90908C, 0x60605C);   /* right arm and gun */
        m.add(G_FIG + k); }
}

/* ---- players, shots, effects ---- */
struct Plyr {
    bool cpu = false, alive = true, jet = false, grounded = true, want_rocket = false; int score = 0, target = -1, strafe = 1, look = 0; uint8_t epoch = 0, nshots = 0, nrocks = 0;
    V p, wish, vh; float vz = 0, yaw = 0, aim_pitch = 0, shield = 1.f, fire_cool = 0, rocket_cool = 0, dead_t = 0, inv = 0, think = 0, jet_t = 0, moved = 0, jet_snd = 0;
};
static std::vector<Plyr> pl; static int me = 0;   /* linked: the seat of this pod */
static const char *NAMES[4] = {"PLAYER", "SLAYER", "METALHEAD", "THE BEAR"};   /* footage: SLAYER, METALHEAD, THE BEAR (the fourth slot is the human) */
struct Bolt { V p, v; int owner; float t; int col; };
struct Rock { V p, v; int owner; float t, trail; };
struct Fx { V p; float t, life; int base, n; };
struct Spk { V p, v; float life; int id; };
static std::vector<Bolt> bolts; static std::vector<Rock> rocks; static std::vector<Fx> fxs; static std::vector<Spk> spks;
static V walk_off; static bool have_off = false;
enum State { S_TITLE, S_INTRO, S_PLAY, S_OVER };
static State st = S_TITLE; static float st_t = 0, time_left = 0; static double sim_t = 0;
static bool autoplay = false;
static float guide_t = 3.f;
/* link play (vpix_1k_net.h): a linked slave only asks the master to fire; see the block before the loop */
static bool slave_game = false; static V net_f, net_d; static uint32_t net_pulse_seq = 0, net_rock_seq = 0;
static bool net_slave() { return v1knet::on && !v1knet::master; }

/* ---- sounds: INVENTED placeholders ---- */
static const uint32_t SB = 0x7E000000u;
enum { SND_PULSE = 1, SND_ROCKET, SND_BOOM, SND_HIT, SND_JET, SND_GO, SND_END, SND_CROWD, SND_CHEER, SND_V0 = 32 };
static void gb_sfx(int ch, int id, int vol) { if (!sound_on) return; buggy::play(ch, (int)(SB + id), vol, 0); }
static void gb_make_sounds() {
    std::mt19937 r(9); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(SB + SND_PULSE, 0.18f, [&](float t) { return (sinf(TAU * (1500.f - 4000.f * t) * t) * 0.7f + noise() * 0.1f) * expf(-t * 18.f); });
    make_sound(SB + SND_ROCKET, 0.5f, [&](float t) { return (noise() * 0.5f + sinf(TAU * 90.f * t) * 0.4f) * expf(-t * 5.f); });
    make_sound(SB + SND_BOOM, 0.9f, [&](float t) { return noise() * expf(-t * 4.f) * 0.9f + sinf(TAU * 60.f * t) * expf(-t * 3.f) * 0.6f; });
    make_sound(SB + SND_HIT, 0.25f, [&](float t) { return (noise() * 0.5f + sinf(TAU * 220.f * t) * 0.4f) * expf(-t * 14.f); });
    make_sound(SB + SND_JET, 0.4f, [&](float t) { return noise() * 0.35f * sinf(3.14159f * t / 0.4f); });
    make_sound(SB + SND_GO, 0.6f, [&](float t) { return sinf(TAU * (440.f + 300.f * t) * t) * expf(-t * 3.f) * 0.6f; });
    make_sound(SB + SND_END, 1.0f, [&](float t) { return sinf(TAU * (660.f - 300.f * t) * t) * expf(-t * 2.f) * 0.6f; });
    /* the stadium crowd (The Computer Paper Oct 1994: thousands of fans cheer). The sound is INVENTED: band-limited noise that breathes, and a swell of many voices when someone is blown up */
    float lo1 = 0, lo2 = 0, lo3 = 0, lo4 = 0;
    make_sound(SB + SND_CROWD, 8.f, [&](float t) { float n = noise(); lo1 += (n - lo1) * 0.26f; lo2 += (n - lo2) * 0.05f; float sw = 0.75f + 0.15f * sinf(TAU * t / 8.f) + 0.10f * sinf(TAU * 3.f * t / 8.f); return (lo1 - lo2) * sw * 0.8f; });
    sounds[SB + SND_CROWD].loop = true;
    float cf[16], cp[16]; for (int i = 0; i < 16; i++) { cf[i] = 420.f + (r() % 900); cp[i] = (r() % 100) * 0.0628f; }
    make_sound(SB + SND_CHEER, 3.2f, [&](float t) { float n = noise(); lo3 += (n - lo3) * 0.3f; lo4 += (n - lo4) * 0.05f; float env = (std::min)(t / 0.35f, 1.f) * expf(-(std::max)(t - 0.5f, 0.f) * 1.1f), v = 0; for (int i = 0; i < 16; i++) { float f = cf[i] * (1.f + 0.05f * t + 0.012f * sinf(TAU * (4.f + i * 0.3f) * t)); v += sinf(TAU * f * t + cp[i]); } return ((lo3 - lo4) * 1.1f + v * 0.028f) * env; });
    for (int i = 0; i < 7; i++) { VSound s; s.rate = 11025; s.loop = false; s.pcm.resize((size_t)GB_VOICE[i].n); for (int q = 0; q < GB_VOICE[i].n; q++) s.pcm[q] = (int16_t)(GB_VOICE[i].d[q] * 200); sounds[SB + SND_V0 + i] = std::move(s); sound_names[SB + SND_V0 + i] = "gridbust_guide"; }
}

/* ---- helpers ---- */
static V gb_dir(float yaw, float pitch) { return V(-sinf(yaw) * cosf(pitch), cosf(yaw) * cosf(pitch), sinf(pitch)); }
static float gb_wrap(float a) { while (a > PI) a -= TAU; while (a < -PI) a += TAU; return a; }
static float gb_segpt(V a, V b, V p) { V ab = b - a; float l2 = dot(ab, ab); float t = l2 > 1e-6f ? clampf(dot(p - a, ab) / l2, 0.f, 1.f) : 0.f; return len(p - (a + ab * t)); }
static float gb_ground(V p) {   /* the floor, or the top of a barrel / crate / plate under you that is no more than a step above your feet */
    float g = 0; for (const Ob &o : obs) { float dx = p.x - o.x, dy = p.y - o.y; float top = o.z + o.h;
        if (dx * dx + dy * dy < o.r * o.r * 0.9f && top <= p.z + 250.f && top > g) g = top; }
    return g;
}
static void gb_collide(V &p) {
    for (const Ob &o : obs) { if (p.z >= o.z + o.h - 150.f || p.z + 1700.f <= o.z) continue;
        float dx = p.x - o.x, dy = p.y - o.y, d = sqrtf(dx * dx + dy * dy), m = o.r + 450.f;
        if (d < m) { if (d > 1.f) { p.x = o.x + dx / d * m; p.y = o.y + dy / d * m; } else p.x = o.x + m; } }
    float rr = sqrtf(p.x * p.x + p.y * p.y), lim = arena_r - 700.f; if (rr > lim) { p.x *= lim / rr; p.y *= lim / rr; }
}
static void gb_fx(V at, int base, int n, float life) { fxs.push_back({at, 0.f, life, base, n}); }
static void gb_sparks(V at, int n, int id, float speed, float life) {
    for (int i = 0; i < n; i++) { V d = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-0.3f, 1.2f))); spks.push_back({at, d * (speed * rnd(0.3f, 1.f)), life * rnd(0.6f, 1.f), id}); }
}

/* ---- rounds ---- */
static void gb_place(Plyr &q, bool first, int k) {
    static const V sp[4] = {V(0, -22000, 0), V(22000, 0, 0), V(0, 22000, 0), V(-22000, 0, 0)};
    if (first) q.p = sp[k & 3]; else { float a = rnd(0, TAU); q.p = V(cosf(a) * 22000.f, sinf(a) * 22000.f, 0); }
    q.yaw = atan2f(q.p.x, -q.p.y); q.aim_pitch = 0; q.vz = 0; q.shield = 1.f; q.alive = true; q.inv = first ? 0.f : 1.5f; q.fire_cool = 0.5f; q.rocket_cool = 0.f;
    q.target = -1; q.think = 0; q.jet_t = 0; q.jet = false; q.wish = V(); q.grounded = true; q.epoch++;
}
static void gb_start_round() {
    size_t n = 1 + (size_t)clampf(envf("GRIDBUST_CPU", (float)n_cpu), 1, 3);
    if (v1knet::on) { n = 4; v1knet::begin_round((int)n); }   /* linked: four seats, computer players fill the empty ones */
    pl.assign(n, Plyr());
    for (size_t i = 0; i < n; i++) { pl[i].cpu = i != (size_t)me; pl[i].look = (int)i; gb_place(pl[i], true, (int)i); }
    bolts.clear(); rocks.clear(); fxs.clear(); spks.clear(); time_left = game_len; have_off = false; guide_t = 3.f;
    dl("t=%.1f round start, %zu players, clock %.0f", sim_t, n, game_len);
}
static void gb_die(int victim, int by) {
    Plyr &v = pl[victim]; v.alive = false; v.dead_t = respawn_delay; v.jet = false;
    V c = v.p + V(0, 0, 1000);
    gb_fx(c, 1, 4, 0.9f); gb_sparks(c, 30, G_SPK, 3200.f, 1.8f); gb_sparks(c, 14, G_SPKO, 3000.f, 1.4f);
    if (by >= 0 && by != victim) pl[by].score += kill_bonus;
    gb_sfx(3, SND_BOOM, victim == me ? 127 : 100); if (victim != me) gb_sfx(8, SND_CHEER, by == me ? 120 : 80);   /* they cheer the obliteration of the other players (The Computer Paper) */
    dl("t=%.1f %s blown up by %d: scores %d %d %d %d", sim_t, NAMES[victim & 3], by, pl[0].score, pl.size() > 1 ? pl[1].score : 0, pl.size() > 2 ? pl[2].score : 0, pl.size() > 3 ? pl[3].score : 0);
}
static void gb_hurt(int victim, int by, float dmg, V at) {
    Plyr &v = pl[victim]; if (!v.alive || v.inv > 0) return;
    if (by >= 0 && by != victim) pl[by].score += hit_score;       /* footage: 12 per hit event */
    v.shield -= dmg; if (victim != me) { gb_fx(at, 0, 3, 0.35f); gb_sparks(at, 8, G_SPK, 1800.f, 0.8f); } gb_sfx(2, SND_HIT, victim == me ? 120 : 80);
    if (v.shield <= 0.f) { v.shield = 0.f; gb_die(victim, by); }
}
static void gb_fire_pulse(int who, V from, V dir) {
    Plyr &q = pl[who]; if (q.fire_cool > 0 || !q.alive) return;
    q.fire_cool = pulse_gap; q.nshots++;
    if (net_slave()) { net_f = from; net_d = dir; net_pulse_seq++; gb_sfx(0, SND_PULSE, 100); return; }   /* linked slave: the master makes the shot */
    Bolt b; b.p = from; b.v = dir * bolt_speed; b.owner = who; b.t = 0; b.col = who & 1; bolts.push_back(b);
    gb_sfx(who == me ? 0 : 1, SND_PULSE, who == me ? 100 : 60);
}
static void gb_fire_rocket(int who, V from, V dir) {
    Plyr &q = pl[who]; if (q.rocket_cool > 0 || !q.alive) return;
    q.rocket_cool = rocket_gap; q.nrocks++;
    if (net_slave()) { net_f = from; net_d = dir; net_rock_seq++; gb_sfx(0, SND_ROCKET, 110); return; }
    Rock r; r.p = from; r.v = dir * rocket_speed; r.owner = who; r.t = 0; r.trail = 0; rocks.push_back(r);
    gb_sfx(who == me ? 0 : 1, SND_ROCKET, who == me ? 110 : 70);
}
static void gb_explode(V at, int owner) {
    gb_fx(at, 2, 3, 0.6f); gb_sparks(at, 24, G_SPK, 4000.f, 1.6f); gb_sparks(at, 12, G_SPKO, 3500.f, 1.2f); gb_sfx(4, SND_BOOM, 110);
    for (size_t k = 0; k < pl.size(); k++) { Plyr &v = pl[k]; if (!v.alive) continue;
        V c = v.p + V(0, 0, 1000); float d = len(c - at); if (d >= splash_r) continue;
        float dmg = rocket_dmg * (1.f - 0.6f * d / splash_r); if ((int)k == owner) dmg *= 0.5f;
        gb_hurt((int)k, owner, dmg, c); }
}

/* ---- the bots: they rush the middle and shoot point blank (Octavius 15:19); not especially clever ---- */
static void gb_step_cpu(int i, float dt) {
    Plyr &q = pl[i];
    q.think -= dt;
    if (q.think <= 0 || (q.target >= 0 && !pl[q.target].alive)) {
        q.think = rnd(0.25f, 0.6f); int best = -1; float bd = 1e30f;
        for (size_t k = 0; k < pl.size(); k++) { if ((int)k == i || !pl[k].alive) continue; V d = pl[k].p - q.p; float l = dot(d, d); if (l < bd) { bd = l; best = (int)k; } }
        q.target = best; q.strafe = rnd(0, 1) < 0.5f ? -1 : 1; q.want_rocket = rnd(0, 1) < 0.25f;
        if (q.jet_t <= 0 && rnd(0, 1) < 0.4f) q.jet_t = rnd(0.3f, 1.2f);
    }
    q.wish = V(); q.jet = false;
    V eye = q.p + V(0, 0, 1400.f); V mid(-q.p.x, -q.p.y, 0); float rm = len(mid);
    if (q.jet_t > 0) q.jet_t -= dt;
    if (q.target < 0) { if (rm > 2000.f) q.wish = norm(mid); return; }
    const Plyr &e = pl[q.target];
    V tc = e.p + V(0, 0, 1000.f), d = tc - eye; float dist = len(d), dh = sqrtf(d.x * d.x + d.y * d.y);
    float wy = atan2f(-d.x, d.y), wp = atan2f(d.z, (std::max)(dh, 1.f)), turn = 3.5f;
    float ey = gb_wrap(wy - q.yaw); q.yaw += clampf(ey, -turn * dt, turn * dt);
    q.aim_pitch += clampf(wp - q.aim_pitch, -turn * dt, turn * dt);
    V toward = dh > 1.f ? V(d.x / dh, d.y / dh, 0) : V(0, 1, 0), side(-toward.y * q.strafe, toward.x * q.strafe, 0);
    if (rm > 9000.f) q.wish = norm(mid) * 0.8f + toward * 0.4f;                /* first the rush to the middle */
    else if (dh > 3000.f) q.wish = toward + side * 0.3f;
    else q.wish = side * 0.6f - toward * 0.2f;                                   /* then point blank */
    q.jet = (q.jet_t > 0 || e.p.z > q.p.z + 1500.f) && q.p.z < ceil_z - 3500.f && !(q.p.z > e.p.z + 3000.f);
    V fw = gb_dir(q.yaw, q.aim_pitch);
    if (dist < 40000.f && fabsf(ey) < 0.14f && q.fire_cool <= 0) {
        float err = 0.06f + dist / 250000.f;   /* INVENTED: bots miss a lot; the footage scores 024 by clock 107 and the bar never moves */
        V dir = norm(fw + V(rnd(-err, err), rnd(-err, err), rnd(-err, err)));
        gb_fire_pulse(i, eye + fw * 400.f, dir); if (q.fire_cool > 0) q.fire_cool += rnd(0.6f, 1.6f);
    }
    if (q.want_rocket && dist > 5000.f && dist < 24000.f && fabsf(ey) < 0.1f && q.rocket_cool <= 0) { gb_fire_rocket(i, eye + fw * 500.f, fw); q.want_rocket = false; }
}
static void gb_move_bot(int i, float dt) {
    Plyr &q = pl[i]; float sp = q.grounded ? walk_speed * 0.8f : walk_speed;
    q.p += q.wish * (sp * dt); q.moved = len(q.wish) * sp; gb_collide(q.p);
}
static void gb_step_body(Plyr &q, float dt) {
    if (!q.alive) return;
    q.vz += (q.jet ? jet_acc - grav : -grav) * dt;
    if (q.vz > climb_max) q.vz = climb_max; if (q.vz < -fall_max) q.vz = -fall_max;
    q.p.z += q.vz * dt;
    float g = gb_ground(q.p);
    if (q.p.z <= g) { q.p.z = g; if (q.vz < 0) q.vz = 0; q.grounded = true; } else q.grounded = q.p.z < g + 2.f;
    if (q.p.z > ceil_z) { q.p.z = ceil_z; if (q.vz > 0) q.vz = 0; }
}

/* ---- the human ---- */
static bool gb_jet_in(const VPIX_Input &in) {   /* the second button (right grip) or B; on the desktop Shift */
    bool k = false;
    if (!in.vr) k = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    return in.top || in.b2 || (in.buttons & VPIX_BTN_3) != 0 || k;
}
static bool gb_rocket_in(const VPIX_Input &in) {   /* INVENTED: left trigger (fire2) or the fourth button; on the desktop F */
    bool k = false;
    if (!in.vr) k = (GetAsyncKeyState('F') & 0x8000) != 0;
    return in.fire2 || (in.buttons & VPIX_BTN_4) != 0 || k;
}
static void gb_step_human(const VPIX_Input &in, float dt) {
    Plyr &q = pl[me];
    if (autoplay) { if (q.alive) { gb_step_cpu(me, dt); gb_move_bot(me, dt); } return; }
    if (!q.alive) return;
    V hxy(in.head.p[0], in.head.p[1], 0);
    if (!have_off) { walk_off = V(q.p.x, q.p.y, 0) - hxy; have_off = true; }
    V f = norm(V(in.head.R[1][0], in.head.R[1][1], 0));
    float sp = q.grounded ? walk_speed : walk_speed * 1.2f;
    q.moved = d1k::desk_walk(in, f, sp, dt, walk_off) ? sp : 0;
    q.jet = gb_jet_in(in);
    V p(walk_off.x + hxy.x, walk_off.y + hxy.y, q.p.z), was = p;
    gb_collide(p); walk_off += p - was;
    q.p.x = p.x; q.p.y = p.y; q.yaw = atan2f(-f.x, f.y);
    /* tilt the thrusters (Joystick 57): with the jets on, tilting the hand off upright pushes you that way. The numbers and the mapping are INVENTED */
    if (q.jet && in.hand.valid) { V up(in.hand.R[2][0], in.hand.R[2][1], 0); float l = len(up); if (l > 0.25f) q.vh += up * (jet_acc * 0.8f * (l - 0.25f) / l * dt); }
    { float k = expf(-(q.grounded ? 6.f : 0.5f) * dt); q.vh = q.vh * k; walk_off += q.vh * dt; }
    /* the weapon is fixed to the head: the crosshair never leaves the view's centre in the footage and no gun is drawn; "head-mounted weaponry" (Electronic Entertainment Jan 1994) */
    V dir = norm(V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2])), from = V(walk_off.x + hxy.x, walk_off.y + hxy.y, q.p.z + in.head.p[2]) + dir * 300.f;
    if ((in.buttons & VPIX_BTN_TRIGGER) != 0 && q.fire_cool <= 0) { gb_fire_pulse(me, from, dir); VPIX_Haptic(0, 0.05f, 0.4f); }
    if (gb_rocket_in(in) && q.rocket_cool <= 0) { gb_fire_rocket(me, from, dir); VPIX_Haptic(0, 0.15f, 0.8f); }
}

/* ---- the guide: a soundtrack that "adjusts to the player's situation, telling you where to go and what to look at" (Amiga Computing 69). The words, the voice and when it speaks are INVENTED ---- */
static void gb_say(int line) { static const char *nm[7] = {"look left", "look right", "look behind", "look up", "look down", "get to the middle", "use your jets"}; dl("t=%.1f guide: %s", sim_t, nm[line]); gb_sfx(6, SND_V0 + line, 120); guide_t = rnd(4.f, 7.f); }
static void gb_guide(const VPIX_Input &in, float dt) {
    guide_t -= dt; if (guide_t > 0 || !pl[me].alive) return;
    const Plyr &q = pl[me]; V fw = (autoplay || !in.head.valid) ? gb_dir(q.yaw, q.aim_pitch) : norm(V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2]));
    V eye = q.p + V(0, 0, 1700.f); int best = -1; float bd = 1e30f; for (size_t k = 0; k < pl.size(); k++) { if ((int)k == me || !pl[k].alive) continue; V d = pl[k].p - q.p; float l = dot(d, d); if (l < bd) { bd = l; best = (int)k; } }
    if (best < 0) return;
    V d = pl[best].p + V(0, 0, 1000.f) - eye; float dh = sqrtf(d.x * d.x + d.y * d.y), fh = atan2f(-fw.x, fw.y), ty = gb_wrap(atan2f(-d.x, d.y) - fh), tp = atan2f(d.z, (std::max)(dh, 1.f)) - asinf(clampf(fw.z, -1.f, 1.f));
    float rm = sqrtf(q.p.x * q.p.x + q.p.y * q.p.y);
    if (rm > 16000.f && dh > 12000.f) gb_say(5);                              /* the bots pile into the middle */
    else if (fabsf(ty) > 2.2f) gb_say(2);
    else if (ty > 0.65f) gb_say(0); else if (ty < -0.65f) gb_say(1);          /* yaw grows to the left */
    else if (d.z > 3500.f && q.grounded) gb_say(6);
    else if (tp > 0.5f) gb_say(3); else if (tp < -0.5f) gb_say(4);
    else guide_t = 1.f;
}

/* ---- shots and effects ---- */
static bool gb_blocked(V p) {   /* ground, wall or an obstacle */
    if (p.z < 0.f || p.x * p.x + p.y * p.y > arena_r * arena_r || p.z > wall_h * 4.f) return true;
    for (const Ob &o : obs) { float dx = p.x - o.x, dy = p.y - o.y; if (dx * dx + dy * dy < o.r * o.r && p.z > o.z && p.z < o.z + o.h) return true; }
    return false;
}
static void gb_step_shots(float dt) {
    for (size_t i = 0; i < bolts.size();) {
        Bolt &b = bolts[i]; V old = b.p; b.p += b.v * dt; b.t += dt; bool gone = false;
        for (size_t k = 0; k < pl.size() && !gone; k++) { Plyr &q = pl[k]; if ((int)k == b.owner || !q.alive) continue;
            for (int h = 0; h < 3; h++) { V c = q.p + V(0, 0, 500.f + 500.f * h);
                if (gb_segpt(old, b.p, c) < 480.f) { gb_hurt((int)k, b.owner, pulse_dmg, c); gone = true; break; } } }
        if (!gone && gb_blocked(b.p)) { gb_sparks(b.p, 6, G_SPKO, 1800.f, 0.6f); gone = true; }
        if (!gone && b.t > 1.2f) gone = true;
        if (gone) { bolts[i] = bolts.back(); bolts.pop_back(); } else i++;
    }
    for (size_t i = 0; i < rocks.size();) {
        Rock &r = rocks[i]; r.p += r.v * dt; r.t += dt; r.trail -= dt; bool boom = false;
        if (r.trail <= 0.f) { r.trail = 0.04f; spks.push_back({r.p - norm(r.v) * 300.f, V(rnd(-300, 300), rnd(-300, 300), rnd(-300, 300)), 0.5f, G_SPK}); }
        for (size_t k = 0; k < pl.size() && !boom; k++) { Plyr &q = pl[k]; if ((int)k == r.owner || !q.alive) continue;
            for (int h = 0; h < 3; h++) if (len(q.p + V(0, 0, 500.f + 500.f * h) - r.p) < 600.f) { boom = true; break; } }
        if (!boom && (gb_blocked(r.p) || r.t > 6.f)) boom = true;
        if (boom) { V at = r.p; int ow = r.owner; rocks[i] = rocks.back(); rocks.pop_back(); gb_explode(at, ow); } else i++;
    }
    for (size_t i = 0; i < fxs.size();) { fxs[i].t += dt; if (fxs[i].t >= fxs[i].life) { fxs[i] = fxs.back(); fxs.pop_back(); } else i++; }
    for (size_t i = 0; i < spks.size();) { Spk &k = spks[i]; k.v.z -= grav * 0.3f * dt; k.p += k.v * dt; k.life -= dt;
        if (k.life <= 0) { spks[i] = spks.back(); spks.pop_back(); } else i++; }
}

/* ---- HUD bitmaps, built here ---- */
/* the presenter's box, from the footage (t888): 48 x 66, rrrgggbb in hex */
static const char *BOB =
"492424242424000000000000000000002400000000000000000000000000000000000000000000000000002424242424492424242420000000000000000000242400000000000000000000000000000000000000000000000000002424242424"
"492424242420000000000000000024240000000000000000000000000000000000000000000000000000002424242424492424242420000000000000000024000000000000000000000000000000000000000000000000000000002424242424"
"492424242420000000000000002400000000000000000000000000000000000000000000000000000000002424242424492424242420000000000000002400000000000000200000000000000020002020000000000000000000002424242424"
"492424242420000000000000442400000000000024200000002020000020242420000000000000000000002424242424452424242400000000000024492400000000002024200000202024202020242420000000000000000000002424242424"
"452424242000000000000024492020000000002424200000002444242420244424000020000000000000002424242424452424242400000000000000240020000000202424200000002444444444444444000020000000000000002424242424"
"4524242424000000000000000000240000002444242000000024444468898989892400000000000000000024242424244524242420000000000000000000240000004444202000002024444488ADAD8D8D440000000000000000002024242424"
"452424242400000000000000000024000024482400000000002444648DADADADAD6800000000000000000020242424244524242400000000000000200000242000246824000000000024446889888889AD682000000000000000002024242424"
"252424200000000000000024000024242044682000000000000064884420204468682000000000000000002024242424242424000000000000000020000024242044482000000000000068884420204468682400000000000000002024242424"
"24242400000000000000000000002424206844000000000000006888442020688D682400000000000000000020242424242420000000000000000000000024202469440000000000000068882000244489892400000000000000000020242424"
"2424200000000000000000000000242044692424000000000000688844244844688D4400000000000000000020242424242420000000000000000000000024204469242000000000002468A96844898D888D4400000000000000000020242424"
"242000000000000000000000002449244469242000000000202468A9888888ADADAD440000000000000000002024242424000000000000000000002400248D446869442000000000242444A9A8A9ADADADAD6400000000000000000020242424"
"24000000000000000000000000208D696889482000000000242044898888ADADADAD6800000000000000000020242424240000000000000000000048242449686989482000000000240020898868ADADADAD6820000000000000000000242424"
"24000000000000000000246D49482044686948200000000000002044644489ADADAD6844200000000000000000202424240000000000000000000024002020446868440000000000000000246444448DADAD6864240000000000000000202424"
"2400000000000000000000000000244468442400000000000000006889442068AD8D6864240000000000000000202024240000000000000000000000000044446844240000000000000020448944204489896864200000000000000000002024"
"240000000000000000000000000049444844200000000000002424688944202068896844000000000000000000002024240000000000000000000000000048444424000000000000000000446968242044886844000000000000000000002024"
"240000000000000000000000000020244424000000000000002424244444242444686844000000000000000000000024240000000000000000000000000000002424000000000000246D8D8D2024242444686824000000000000000000000024"
"240000000000000000000000000000000024000000000000002444444468444444686800000000000000000000000024240000000000000000000000000000000024000000000000202444698988444444442000000000000000000000000000"
"240000000000000000000000000000000024000000000000000020446888644444200000000000000000000000000000240000000000000000000000000000000024000000000000000000244468444424000000000000000000000000000000"
"240000000000000000000000000000000024200000000000000020444464442424000000000000000000000000000000240000000000000000000000000000000024240000000000002444444444442420000000000000000000000000000000"
"240000000000000000000000000000000024240000000000002444484444242420000000000000000000000000000000240000000000000000000000000000000024242000000000002024444424242420000000000000000000000000000000"
"240000000000000000000000000000000024242000000000000020242424202400000000000000000000000000000000240000000000000000000000000000000024242000000000000000000000202400000000000000000000000000000000"
"240000000000000000000000000000000024242000000000000000000000242400000000000000000000000000000000240000000000000000000000000000000024240000000000000000000020242400000000000000000000000000000000"
"240000000000000000000000000000204444242000000000000000000024242420000000000000000000000000000000240000000000000000000000000020494844242000000000000000000024242444240000000000000000000000000000"
"240000000000000000000000000049492448240000000000000000002424242444492400000000000000000000000000240000000000000000000000004469442448440000000000000000202424244444452400000000000000000000000000"
"240000000000000000002024444949242448482400000000000020242420444844442444200000000000000000000000240000000000204449494444454544242424484824000000000024200024484824442444492000000000000000000000"
"240000002449694949444444494444242424484848240000000000204448484824442444494500000000000000000000242045696969694444444444454424242424444848482400000024484848484424442424446945200000000000000000"
"496969696545444444244445444424242424444848492400002448484848482424242424444569694920000000000000496945454444442424242445444424242424244848440000002448484444442424202424444469698969442000000000"
"494444444444242424242444442424244424244848240024242024484444442424202424444445696969494944240000494444444424242424242444442444444424244448242448442424484448442424242424244444454444444449694924"
"452444444424242424242444444424444444242448242424242024484848442424242424242424442444444444454949452424444424242424242024444444444444242448242424240024484444242424242424242424242424444444444549"
"452444442424242424242024444444444444242444442424242024484424242424242424242424242024444444444445452444442424242424242024244444444444242444482400000024484424242424242424242424242024244424242424"
"452444442420242424242424244444444444442444482400000024482444242424242424242424242024242424242424452444442420202424242424244444444444442424482400000024442424242424242424242444242024242424242424"
"452444442420202424242424242444444444442424442424442424442424242424242424242424242024242444242424452444442420002024242420242444444444442420242420000024242424242424442424242424242024242444242024"
"452444442424002024242424242444444444442420242400000020242424242424242424242424242024242424242024452444442424002024242424242444444444442400242400000020442424242424242424242424242424242424200024";
static const int BM_BAR = 400 /* + 0..10 fill levels */, BM_SHIELD = 420, BM_CLOCK = 421, BM_CROSS = 422, BM_BRACKET = 423, BM_QUOTE = 424, BM_SOLID_W = 425, BM_SOLID_P = 426, BM_FACE = 427, BM_SOLID_L = 428, BM_STACK = 429, BM_RD = 430 /* + 10 red digits */, BM_WD = 440 /* + 10 white */;
static uint32_t gb_rgba(int r, int g, int b) { return 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r; }   /* words are 0xAABBGGRR as in make_hud */
static void gb_make_hud() {
    d1k::make_hud();
    for (int k = 0; k <= 10; k++) { const int W = 20, H = 100; std::vector<uint32_t> px(W * H, 0); int fill = k * 10;   /* the stack of pale stripes outlined in red: full in the footage */
        for (int y = 0; y < H; y++) { bool on = (H - 1 - y) < fill; int ph = y % 5; for (int x = 0; x < W; x++) { if (!on || ph == 4) continue; px[y * W + x] = (x < 1 || x >= W - 1 || ph == 0 || ph == 3) ? gb_rgba(206, 70, 56) : gb_rgba(246, 208, 196); } }
        d1k::add_bitmap(BM_BAR + k, W, H, px); }
    { static const char *s[] = {"@@@@@@@@@@@@@@", "@@@@@@@@@@@@@@", "@@####@@@@@@@@", "@@#@@@##@@@@@@", "@@#@@@@@@@@@@@", "@@#####@@@@@@@", "@@#@@@@@@@@@@@", "@@#@@@@@@@@@@@", "@@####@@@@@@@@", ".@@@##@@@@@@@.", ".@@@@@@@@@@@@.", "..@@@@@@@@@@..", "...@@@@@@@@...", "....@@@@@@....", ".....@@@@.....", "......@@......"};
      const int W = 14, H = 16; std::vector<uint32_t> px(W * H, 0);
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { char c = s[y][x]; if (c == '#') px[y * W + x] = gb_rgba(245, 240, 230); else if (c == '@') px[y * W + x] = gb_rgba(200, 56, 44); }
      d1k::add_bitmap(BM_SHIELD, W, H, px); }
    { const int W = 18, H = 18; std::vector<uint32_t> px(W * H, 0);   /* the clock icon: a red disc with a white quarter */
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { float dx = x - 8.5f, dy = y - 8.5f, d = sqrtf(dx * dx + dy * dy);
          if (d < 8.8f) px[y * W + x] = (dx >= 0 && dy <= 0 && d < 7.5f) ? gb_rgba(245, 240, 230) : gb_rgba(205, 60, 46); }
      d1k::add_bitmap(BM_CLOCK, W, H, px); }
    { const int W = 36, H = 36; std::vector<uint32_t> px(W * H, 0), br(W * H, 0);   /* the crosshair: two red arcs with a tick above and below; a yellow bracket on a target */
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { float dx = x - 17.5f, dy = y - 17.5f, d = sqrtf(dx * dx + dy * dy);
          if (d > 11.f && d < 12.6f && fabsf(dy) > 4.f) px[y * W + x] = gb_rgba(214, 74, 54);
          if (abs(x - 17) <= 1 && (y < 7 || y > 28)) px[y * W + x] = gb_rgba(224, 90, 70);
          if (d > 11.f && d < 12.6f && fabsf(dy) > 4.f) br[y * W + x] = gb_rgba(240, 214, 70); if (abs(x - 17) <= 0 && (y < 7 || y > 28)) br[y * W + x] = gb_rgba(240, 214, 70); }
      d1k::add_bitmap(BM_CROSS, W, H, px); d1k::add_bitmap(BM_BRACKET, W, H, br); }
    { const int W = 6, H = 8; std::vector<uint32_t> px(W * H, 0);   /* the quote marks round a name tag */
      for (int y = 0; y < 6; y++) for (int x = 0; x < 2; x++) { px[y * W + x] = gb_rgba(240, 240, 240); px[y * W + x + 3] = gb_rgba(240, 240, 240); }
      d1k::add_bitmap(BM_QUOTE, W, H, px); }
    { std::vector<uint32_t> a(64 * 64, gb_rgba(200, 200, 205)), p(64 * 64, gb_rgba(44, 48, 72)), l(64 * 64, gb_rgba(104, 98, 146)); d1k::add_bitmap(BM_SOLID_W, 64, 64, a); d1k::add_bitmap(BM_SOLID_P, 64, 64, p); d1k::add_bitmap(BM_SOLID_L, 64, 64, l); }
    { const int W = 48, H = 66; std::vector<uint32_t> px(W * H, 0);
      for (int i = 0; i < W * H; i++) { auto hv = [](char c) { return c <= '9' ? c - '0' : c - 'A' + 10; }; int v = hv(BOB[i * 2]) * 16 + hv(BOB[i * 2 + 1]); px[i] = gb_rgba((v & 0xE0) | ((v & 0xE0) >> 3), ((v << 3) & 0xE0) | ((v & 0x1C) >> 1), ((v & 3) << 6) | ((v & 3) << 4) | ((v & 3) << 2) | (v & 3)); }
      for (uint32_t &w : px) { int r = w & 255, g = (w >> 8) & 255, b = (w >> 16) & 255; w = gb_rgba((std::min)(255, r * 3 / 2), (std::min)(255, g * 3 / 2), (std::min)(255, b * 3 / 2)); }   /* the card's picture is brighter than the stored dither */
      d1k::add_bitmap(BM_FACE, W, H, px); }
    { const int W = 18, H = 74; std::vector<uint32_t> px(W * H, 0); const uint32_t cc[4] = {gb_rgba(204, 70, 56), gb_rgba(70, 170, 90), gb_rgba(214, 190, 70), gb_rgba(204, 70, 56)};   /* the little coloured columns under the clock (meaning unknown) */
      for (int y = 0; y < H; y++) if ((y % 3) != 2) for (int c = 0; c < 4; c++) for (int x = 1 + c * 4; x < 4 + c * 4; x++) px[y * W + x] = cc[c];
      d1k::add_bitmap(BM_STACK, W, H, px); }
    { static const char *D[10][7] = {{"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}, {".##..", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}, {"####.", "....#", "....#", ".###.", "#....", "#....", "#####"}, {"####.", "....#", "....#", ".###.", "....#", "....#", "####."}, {"#...#", "#...#", "#...#", "#####", "....#", "....#", "....#"},
        {"#####", "#....", "#....", "####.", "....#", "....#", "####."}, {".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."}, {"#####", "....#", "...#.", "...#.", "..#..", "..#..", "..#.."}, {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}, {".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."}};
      for (int d = 0; d < 10; d++) for (int set = 0; set < 2; set++) { const int W = 15, H = 21; std::vector<uint32_t> px(W * H, 0); for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (D[d][y / 3][x / 3] == '#') px[y * W + x] = set ? gb_rgba(240, 240, 240) : gb_rgba(214, 66, 50);
          d1k::add_bitmap((set ? BM_WD : BM_RD) + d, W, H, px); } }
}
static void gb_rect(VPIX_Frame &f, int id, float x, float y, float w, float h) {
    VPIX_Blit b = {}; b.handle = d1k::BMPH + id; b.sw = 64; b.sh = 64; b.x = (int)x; b.y = (int)y; b.dw = w; b.dh = h; b.key = 0; b.alpha = 1.f; b.prio = 999000;
    b.tint[0] = b.tint[1] = b.tint[2] = 1.f; f.blits.push_back(b);
}
static void gb_panel(VPIX_Frame &f, float x, float y, float w, float h) {
    gb_rect(f, BM_SOLID_W, x, y, w, h); gb_rect(f, BM_SOLID_P, x + 2, y + 2, w - 4, h - 4);
}
static void gb_num3(VPIX_Frame &f, int base, int n, float x, float y, float sc) {
    n = (std::max)(0, (std::min)(n, 999)); int d[3] = {n / 100, (n / 10) % 10, n % 10};
    for (int k = 0; k < 3; k++) d1k::blit(f, base + d[k], 15, 21, x + k * (15.f * sc + 1.5f), y, sc);
}
static void gb_ttext(VPIX_Frame &f, const char *s, float cx, float y, float sc, float r, float g, float b, float adv = 12.f) {   /* a line of the kit's letters, tinted */
    float w = (float)strlen(s) * adv * sc, x = cx - w * 0.5f; size_t n0 = f.blits.size(); for (const char *c = s; *c; c++) { if (*c >= 'A' && *c <= 'Z') d1k::blit(f, d1k::BM_TEXT + (*c - 'A'), d1k::FW, d1k::FH, x, y, sc); x += adv * sc; }
    for (size_t i = n0; i < f.blits.size(); i++) { f.blits[i].tint[0] = r; f.blits[i].tint[1] = g; f.blits[i].tint[2] = b; }
}
static int gb_aimed(V eye, V fw, float tol) {   /* the opponent nearest your line of sight */
    int best = -1; float ba = tol;
    for (size_t k = 0; k < pl.size(); k++) { if ((int)k == me || !pl[k].alive) continue;
        V d = pl[k].p + V(0, 0, 1000.f) - eye; float l = len(d); if (l < 300.f || l > 90000.f) continue;
        float a = acosf(clampf(dot(norm(d), fw), -1.f, 1.f)); if (a < ba) { ba = a; best = (int)k; } }
    return best;
}

/* ---- drawing ---- */
static M gb_yawed(float yaw) { return d1k::yawed(yaw); }
static void gb_put_players(VPIX_Frame &f) {
    for (size_t i = 0; i < pl.size(); i++) { const Plyr &q = pl[i]; if (!q.alive || (int)i == me) continue;
        if (q.inv > 0 && fmodf((float)sim_t, 0.2f) < 0.1f) continue;
        M R = gb_yawed(q.yaw); d1k::dput(f, G_FIG + q.look, R, q.p);
        if (q.jet) { VPIX_Instance &fi = d1k::dput(f, G_FLAME, gb_yawed(q.yaw + (fmodf((float)sim_t, 0.1f) < 0.05f ? 0.f : PI)), q.p); fi.bright = 1; } }
}
static void gb_put_effects(VPIX_Frame &f) {
    for (const Bolt &b : bolts) { VPIX_Instance &i = d1k::dput(f, G_BOLT0 + b.col, look_at(b.p, b.p + b.v), b.p); i.bright = 1; }
    for (const Rock &r : rocks) d1k::dput(f, G_ROCKET, look_at(r.p, r.p + r.v), r.p);
    M I;
    for (const Fx &x : fxs) { int k = x.base + (int)(x.t / x.life * x.n); k = (std::max)(0, (std::min)(k, 4)); d1k::dput(f, G_FIRE + k, I, x.p).bright = 1; }
    for (const Spk &k : spks) d1k::dput(f, k.id, I, k.p).bright = 1;
}
static const float CARD_LEN = 6.f;
static float card_u = 0;
static void gb_attract_cam(float t, int &ph, V &eye, M &cam) {   /* the order from t878-892: card, corridor, card, arena */
    if (t < 1.5f || (t >= 8.f && t < 14.f)) { ph = 0; card_u = t < 1.5f ? CARD_LEN - 1.f + t / 1.5f : t - 8.f; float k = card_u / CARD_LEN, e = 0.04f + 0.36f * k, d = 48000.f - 9000.f * k; eye = V(0, -cosf(e) * d, sinf(e) * d); cam = look_at(eye, V(-3000.f, 0, 3000.f)); }
    else if (t < 8.f) { ph = 1; float u = t - 1.5f; eye = V(TX, 2000.f + u * 3600.f, 1700.f); cam = look_at(eye, V(TX, 30000.f + u * 3600.f, 1700.f)); }
    else { ph = 2; float a = (t - 14.f) * 0.1f, R = arena_r * 0.8f; eye = V(sinf(a) * R, -cosf(a) * R, 13000.f); cam = look_at(eye, V(0, 0, 1500.f)); }
}
static void gb_card(VPIX_Frame &f) {   /* laid out as at t886-892 in 352 x 288: picture box top left with a thin frame, the lavender band across the whole top, the framed sign in it, the name under the box */
    gb_rect(f, BM_SOLID_L, 0, 29, 352, 22);
    gb_rect(f, BM_SOLID_W, 123, 19, 101, 40); gb_rect(f, BM_SOLID_P, 124, 20, 99, 38);
    gb_ttext(f, "CHANNEL", 173, 22, 0.5f, 0.78f, 0.78f, 0.86f, 14.f);
    for (int k = 0; k < 3; k++) { static const int v[3] = {6, 6, 5}; d1k::blit(f, BM_WD + v[k], 15, 21, 141.f + k * 22.f, 33, 1.3f); }
    gb_rect(f, BM_SOLID_W, 14, 12, 104, 134); d1k::blit(f, BM_FACE, 48, 66, 16, 14, 2.f);
    gb_ttext(f, "BOB BINGLEY", 68, 150, 1.0f, 0.86f, 0.86f, 0.84f, 10.5f);
}
static void gb_build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = f.clear[1] = f.clear[2] = 0;
    f.n_lights = 1; f.lights[0] = {0.65f, 0.35f, {-0.218f, -0.437f, 0.874f}};   /* flat shaded and bright; values are a guess */
    bool play = (st == S_PLAY || st == S_OVER) && !pl.empty();
    M cam; V eye; bool head_cam = false; int ph = 2; float tt = 0;
    static const char *fixed = getenv("GRIDBUST_CAM");
    M I;
    if (play) {
        d1k::dput(f, G_ARENA, I, V(0, 0, 0)).bright = 1; d1k::dput(f, G_LIGHTS, I, V(0, 0, 0)).bright = 1;
        for (int k = 0; k < 4; k++) { M R = gb_yawed(sign_yaw[k]); M T; T.r[0] = R.r[0]; T.r[1] = R.r[1] * 0.96f + V(0, 0, -0.28f); T.r[2] = R.r[1] * 0.28f + V(0, 0, 0.96f); d1k::dput(f, G_SIGN, T, sign_at[k]).bright = 1; int sv = (std::max)(0, (std::min)(999, ((k & 1) ? 212 : 197) - (int)ceilf(time_left))); int d[3] = {sv / 100, (sv / 10) % 10, sv % 10};   /* read from frames, meaning unknown: see the header */
            V front = sign_at[k] + T.r[1] * 90.f; if (k == 0 || k == 2) front = sign_at[k] - T.r[1] * 90.f; for (int q = 0; q < 3; q++) d1k::dput(f, G_DIG + d[q], T, front + T.r[0] * ((q - 1) * 1350.f * ((k == 0 || k == 2) ? 1.f : -1.f))).bright = 1; }
        { M R = gb_yawed(0.5f); int d[3] = {0, 1, 4}; for (int q = 0; q < 3; q++) d1k::dput(f, G_DIG + d[q], R, V(3000, -14000, 3800) + R.r[0] * (-2200.f + q * 1500.f + (q ? 700.f : 0.f))).bright = 1; }    /* the big floating 0 14 of t912-916 */
        gb_put_players(f); gb_put_effects(f);
    }
    if (fixed && *fixed) {
        float a[6] = {0, -34000, 12000, 0, 0, 1500}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5]));
        if (!play) { d1k::dput(f, G_ARENA, I, V(0, 0, 0)).bright = 1; d1k::dput(f, G_LIGHTS, I, V(0, 0, 0)).bright = 1; }
    } else if (!play) {
        static const char *att = getenv("GRIDBUST_ATTRACT"); tt = (att && *att) ? (float)atof(att) : st == S_INTRO ? st_t * 14.f / (std::max)(intro_len, 0.1f) : (float)fmod(sim_t, 28.0);
        gb_attract_cam(tt, ph, eye, cam);
        if (ph == 0) { d1k::dput(f, G_STARS, I, V(0, 0, 0)).bright = 1; d1k::dput(f, G_SAUCER, gb_yawed(0.45f - card_u * 0.17f), V(0, 0, 0)).bright = 1; }
        else if (ph == 1) { d1k::dput(f, G_TUNNEL, I, V(0, 0, 0)).bright = 1; d1k::dput(f, G_FIG + 4, gb_yawed(PI), V(TX, eye.y + 15000.f - (tt - 1.5f) * 1200.f, 0)).bright = 1; d1k::dput(f, G_TUNEND, I, V(0, eye.y + 62000.f, 0)).bright = 1; }
        else { d1k::dput(f, G_ARENA, I, V(0, 0, 0)).bright = 1; d1k::dput(f, G_LIGHTS, I, V(0, 0, 0)).bright = 1; }
    } else {
        const Plyr &q = pl[me];
        if (autoplay || !in.head.valid) { eye = q.p + V(0, 0, 1700.f); cam = look_at(eye, eye + gb_dir(q.yaw, q.aim_pitch)); }
        else {
            for (int k = 0; k < 3; k++) cam.r[k] = V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2]);
            eye = V(walk_off.x + in.head.p[0], walk_off.y + in.head.p[1], q.p.z + in.head.p[2]); head_cam = true;
        }
    }
    { static const char *lk = getenv("GRIDBUST_LOOK"); if (lk && *lk && !head_cam) { float ya = 0, pi = 0; sscanf(lk, "%f,%f", &ya, &pi); ya *= PI / 180.f; pi *= PI / 180.f; V fw = cam.r[1], ri = cam.r[0], up = cam.r[2]; V f2 = fw * cosf(ya) + ri * sinf(ya), r2 = ri * cosf(ya) - fw * sinf(ya); V f3 = f2 * cosf(pi) + up * sinf(pi), u3 = up * cosf(pi) - f2 * sinf(pi); cam.r[0] = r2; cam.r[1] = f3; cam.r[2] = u3; } }
    float iris_k = 2.f;
    if (play && !pl.empty()) {   /* the iris: closes as you die, opens as you come back (t925; that it means a respawn is a GUESS) */
        const Plyr &q = pl[me]; float k = 2.f; if (!q.alive) k = clampf(q.dead_t - (respawn_delay - 0.7f), 0.f, 0.7f) / 0.7f; else if (q.inv > 0) k = 1.f - clampf(q.inv / 1.5f, 0.f, 1.f);
        iris_k = k; if (k < 1.f) { VPIX_Instance &ir = d1k::dput(f, G_IRIS, cam, eye + cam.r[1] * 900.f); float rr = 60.f + k * 1000.f; ir.S[0] = rr; ir.S[1] = 1; ir.S[2] = rr; ir.on_top = 2; ir.bright = 1; } }
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    f.in_head = head_cam; if (head_cam) f.head_fed = in.head;

    const float CX = 176.f;
    if (!play) {
        if (ph == 0) gb_card(f);
        else if (ph == 2 && st == S_TITLE) {
            d1k::text(f, "GRID BUSTERS", CX, 70, 1.6f, 1);
            d1k::text(f, "CHANNEL 665", CX, 128, 0.8f);
            if (fmodf((float)sim_t, 1.2f) < 0.8f) d1k::text(f, "PRESS TRIGGER", CX, 220, 1.f);
        }
    } else {
        const Plyr &q = pl[me];
        /* the HUD, all red and placed as measured on t896 and t1184: the bar and its shield icon left, SCORE middle, clock right with the coloured columns under it.
           The bar never moved in 50 s of footage; here it is the shield and drops when you are hit (INVENTED) */
        if (iris_k >= 0.8f) { d1k::blit(f, BM_SHIELD, 14, 16, 45, 26, 1.f); d1k::blit(f, BM_BAR + (int)(clampf(q.shield, 0.f, 1.f) * 10.f + 0.5f), 20, 100, 42, 50, 1.f); }   /* bar and clock are gone while the iris opens, SCORE stays (t925) */
        gb_ttext(f, "SCORE", CX, 24, 0.85f, 0.85f, 0.26f, 0.2f, 10.f); gb_num3(f, BM_RD, q.score, CX - 19.f, 38, 0.8f);
        int secs = (int)ceilf((std::max)(st == S_OVER ? 0.f : time_left, 0.f));
        if (iris_k >= 0.8f) { d1k::blit(f, BM_CLOCK, 18, 18, 284, 24, 0.85f); gb_num3(f, BM_RD, secs, 270, 38, 0.8f); d1k::blit(f, BM_STACK, 18, 74, 288, 76, 1.f); }
        /* the crosshair stays at the view's centre (every frame of t892-946), a yellow bracket on a target; the name tag over whoever you look at */
        if (st == S_PLAY && q.alive) {
            int gunt = gb_aimed(eye, cam.r[1], 0.07f), looked = gb_aimed(eye, cam.r[1], 0.12f);
            d1k::blit(f, gunt >= 0 ? BM_BRACKET : BM_CROSS, 36, 36, CX - 18.f, 144.f - 18.f, 1.f);
            if (looked >= 0) {   /* the name, big and grey, in a fixed place under the score (t930, t1184), not on the target */
                const char *nm = NAMES[looked & 3]; float sc = 1.35f, w = 12.f * sc * (float)strlen(nm);
                gb_ttext(f, nm, CX, 76, sc, 0.92f, 0.90f, 0.84f);
                d1k::blit(f, BM_QUOTE, 6, 8, CX - w * 0.5f - 10.f, 76, 1.3f); d1k::blit(f, BM_QUOTE, 6, 8, CX + w * 0.5f + 2.f, 76, 1.3f); }
        }
        if (st == S_OVER) {   /* INVENTED: the round's end was not seen */
            int best = 0; for (size_t i = 1; i < pl.size(); i++) if (pl[i].score > pl[best].score) best = (int)i;
            bool tie = false; for (size_t i = 0; i < pl.size(); i++) if ((int)i != best && pl[i].score == pl[best].score) tie = true;
            d1k::text(f, "GAME OVER", CX, 96, 1.6f, 1);
            d1k::text(f, tie ? "A DRAW" : best == me ? "YOU WIN" : "YOU LOSE", CX, 130, 1.f);
        }
    }
    d1k::hs::draw(f, "gridbust", st == S_TITLE, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at the machine's rate ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 1; static unsigned long frames = 0;
#pragma pack(push, 1)
struct NetIn { float x, y, z, yaw, pitch, moved; uint8_t epoch, jet; uint32_t pseq, rseq; float f[3], d[3]; };
struct NetP { float p[3], yaw, pitch, vz, moved, dead_t, inv, shield; uint8_t flags, look, epoch, nshots, nrocks; int16_t score; };
struct NetB { float p[3], v[3]; int8_t owner, col; };
struct NetR { float p[3], v[3]; int8_t owner; };
struct NetSt { uint8_t st, n, nb, nr; float st_t, time_left; NetP pl[4]; NetR r[8]; NetB b[24]; };
#pragma pack(pop)
static void net3(float *o, V a) { o[0] = a.x; o[1] = a.y; o[2] = a.z; }
static V net3(const float *o) { return V(o[0], o[1], o[2]); }
static uint32_t net_last_pulse[4], net_last_rock[4];
static void net_log() {
    static double next = 0; if (sim_t < next) return; next = sim_t + 5.0;
    if (pl.size() < 4) { dl("t=%.1f link: %s, title, pods heard %d", sim_t, v1knet::master ? "master" : "slave", v1knet::live_pods()); return; }
    dl("t=%.1f link: %s seat %d st %d left %.0f | p0 %s(%.0f %.0f %.0f) %d | p1 %s(%.0f %.0f %.0f) %d | p2 %s(%.0f %.0f) %d | p3 %s(%.0f %.0f) %d", sim_t, v1knet::master ? "master" : "slave", me, (int)st, time_left,
       pl[0].cpu ? "cpu " : "", pl[0].p.x, pl[0].p.y, pl[0].p.z, pl[0].score, pl[1].cpu ? "cpu " : "", pl[1].p.x, pl[1].p.y, pl[1].p.z, pl[1].score, pl[2].cpu ? "cpu " : "", pl[2].p.x, pl[2].p.y, pl[2].score, pl[3].cpu ? "cpu " : "", pl[3].p.x, pl[3].p.y, pl[3].score);
}
static void net_master() {
    for (size_t i = 1; i < pl.size() && i < 4; i++) { Plyr &q = pl[i]; const NetIn *p = (const NetIn *)v1knet::seat_input((int)i, sizeof(NetIn));
        if (!p) { if (!q.cpu) { q.cpu = true; q.target = -1; dl("t=%.1f link: seat %zu back to the computer", sim_t, i); } continue; }
        if (q.cpu) { q.cpu = false; q.jet = false; net_last_pulse[i] = p->pseq; net_last_rock[i] = p->rseq; dl("t=%.1f link: seat %zu is a person now", sim_t, i); }
        if (p->epoch == q.epoch && q.alive) { q.p = V(p->x, p->y, clampf(p->z, 0.f, ceil_z)); q.yaw = p->yaw; q.aim_pitch = p->pitch; q.moved = p->moved; q.jet = p->jet != 0; q.grounded = q.p.z < gb_ground(q.p) + 2.f; }
        V d = norm(net3(p->d)), f = net3(p->f);
        if (p->pseq != net_last_pulse[i]) { net_last_pulse[i] = p->pseq; q.fire_cool = 0; gb_fire_pulse((int)i, f, d); }
        if (p->rseq != net_last_rock[i]) { net_last_rock[i] = p->rseq; q.rocket_cool = 0; gb_fire_rocket((int)i, f, d); } }
    if (!v1knet::due(30)) return;
    if (st == S_TITLE || pl.empty()) { v1knet::send_snapshot(NULL, 0); return; }
    static NetSt s; memset(&s, 0, sizeof(s)); s.st = (uint8_t)st; s.n = (uint8_t)(std::min)(pl.size(), (size_t)4); s.st_t = st_t; s.time_left = time_left;
    for (int i = 0; i < s.n; i++) { const Plyr &q = pl[i]; NetP &n = s.pl[i]; net3(n.p, q.p); n.yaw = q.yaw; n.pitch = q.aim_pitch; n.vz = q.vz; n.moved = q.moved; n.dead_t = q.dead_t; n.inv = q.inv; n.shield = q.shield;
        n.flags = (q.alive ? 1 : 0) | (q.jet ? 2 : 0) | (q.cpu ? 4 : 0) | (q.grounded ? 8 : 0); n.look = (uint8_t)q.look; n.epoch = q.epoch; n.nshots = q.nshots; n.nrocks = q.nrocks; n.score = (int16_t)q.score; }
    for (size_t k = 0; k < rocks.size() && s.nr < 8; k++) { NetR &n = s.r[s.nr++]; net3(n.p, rocks[k].p); net3(n.v, rocks[k].v); n.owner = (int8_t)rocks[k].owner; }
    for (size_t k = 0; k < bolts.size() && s.nb < 24; k++) { NetB &n = s.b[s.nb++]; net3(n.p, bolts[k].p); net3(n.v, bolts[k].v); n.owner = (int8_t)bolts[k].owner; n.col = (int8_t)bolts[k].col; }
    v1knet::send_snapshot(&s, offsetof(NetSt, b) + s.nb * sizeof(NetB));
}
static void net_leave(const char *why) { slave_game = false; st = S_TITLE; st_t = 0; pl.clear(); bolts.clear(); rocks.clear(); fxs.clear(); spks.clear(); me = 0; have_off = false; dl("t=%.1f link: %s, back to the title", sim_t, why); }
static void net_apply(const NetSt &s, uint8_t round) {
    static uint8_t my_round = 0; bool fresh = !slave_game || round != my_round || pl.size() != s.n; my_round = round;
    me = v1knet::my_seat;
    if (fresh) { pl.assign(s.n, Plyr()); bolts.clear(); rocks.clear(); fxs.clear(); spks.clear(); have_off = false; slave_game = true; gb_sfx(0, SND_GO, 100); dl("t=%.1f link: in the master's round, seat %d of %d", sim_t, me, (int)s.n); }
    if (!fresh) for (const Rock &r : rocks) { bool found = false; for (int k = 0; k < s.nr; k++) if (s.r[k].owner == r.owner && len(net3(s.r[k].p) - r.p) < 9000.f) found = true;
        if (!found) { gb_fx(r.p, 2, 3, 0.6f); gb_sparks(r.p, 24, G_SPK, 4000.f, 1.6f); gb_sparks(r.p, 12, G_SPKO, 3500.f, 1.2f); gb_sfx(4, SND_BOOM, 110); } }
    for (int i = 0; i < s.n; i++) { Plyr &q = pl[i]; const NetP &n = s.pl[i]; const bool alive = (n.flags & 1) != 0, own = i == me;
        if (!fresh) {
            V c = q.p + V(0, 0, 1000);
            if (q.alive && !alive) { gb_fx(c, 1, 4, 0.9f); gb_sparks(c, 30, G_SPK, 3200.f, 1.8f); gb_sparks(c, 14, G_SPKO, 3000.f, 1.4f); gb_sfx(3, SND_BOOM, own ? 127 : 100); if (!own) gb_sfx(8, SND_CHEER, 80); }
            if (!own && n.nshots != q.nshots) gb_sfx(1, SND_PULSE, 60);
            if (!own && n.nrocks != q.nrocks) gb_sfx(1, SND_ROCKET, 70);
            if (n.shield < q.shield - 0.001f && alive) { if (own) gb_sfx(2, SND_HIT, 120); else { gb_fx(c, 0, 3, 0.35f); gb_sparks(c, 8, G_SPK, 1800.f, 0.8f); gb_sfx(2, SND_HIT, 80); } }
        }
        const bool keep = !fresh && own && st == S_PLAY && s.st == S_PLAY && q.alive && alive && q.epoch == n.epoch && (have_off || autoplay);   /* you lead your own walking */
        if (!keep) { q.p = net3(n.p); q.yaw = n.yaw; q.moved = n.moved; q.vz = n.vz; q.jet = (n.flags & 2) != 0; q.grounded = (n.flags & 8) != 0; q.aim_pitch = n.pitch; if (own) have_off = false; }
        if (!own) { q.aim_pitch = n.pitch; q.moved = n.moved; }
        q.dead_t = n.dead_t; q.inv = n.inv; q.shield = n.shield; q.alive = alive; q.cpu = (n.flags & 4) != 0; q.look = n.look; q.epoch = n.epoch; q.nshots = n.nshots; q.nrocks = n.nrocks; q.score = n.score; }
    rocks.resize(s.nr); for (int k = 0; k < s.nr; k++) { rocks[k].p = net3(s.r[k].p); rocks[k].v = net3(s.r[k].v); rocks[k].owner = s.r[k].owner; rocks[k].t = 0; rocks[k].trail = 0; }
    bolts.resize(s.nb); for (int k = 0; k < s.nb; k++) { bolts[k].p = net3(s.b[k].p); bolts[k].v = net3(s.b[k].v); bolts[k].owner = s.b[k].owner; bolts[k].col = s.b[k].col; bolts[k].t = 0; }
    st = (State)s.st; st_t = s.st_t; time_left = s.time_left;
}
static void net_slave_step(const VPIX_Input &in, float dt) {
    for (Plyr &q : pl) { if (q.fire_cool > 0) q.fire_cool -= dt; if (q.rocket_cool > 0) q.rocket_cool -= dt; if (q.inv > 0) q.inv -= dt; if (!q.alive && q.dead_t > 0) q.dead_t -= dt; }
    if (st == S_PLAY) {
        gb_step_human(in, dt); gb_guide(in, dt);
        Plyr &o = pl[me]; if (o.alive) { gb_step_body(o, dt); if (o.jet) { o.jet_snd -= dt; if (o.jet_snd <= 0) { o.jet_snd = 0.35f; gb_sfx(5, SND_JET, 70); } } }
        time_left -= dt; }
    for (Bolt &b : bolts) { b.p += b.v * dt; b.t += dt; }
    for (Rock &r : rocks) { r.p += r.v * dt; r.t += dt; r.trail -= dt; if (r.trail <= 0.f) { r.trail = 0.04f; spks.push_back({r.p - norm(r.v) * 300.f, V(rnd(-300, 300), rnd(-300, 300), rnd(-300, 300)), 0.5f, G_SPK}); } }
    for (size_t i = 0; i < fxs.size();) { fxs[i].t += dt; if (fxs[i].t >= fxs[i].life) { fxs[i] = fxs.back(); fxs.pop_back(); } else i++; }
    for (size_t i = 0; i < spks.size();) { Spk &k = spks[i]; k.v.z -= grav * 0.3f * dt; k.p += k.v * dt; k.life -= dt;
        if (k.life <= 0) { spks[i] = spks.back(); spks.pop_back(); } else i++; }
}
/* true = this tick was a linked slave's, inside the master's game */
static bool net_tick(double el, const VPIX_Input &in) {
    v1knet::pump();
    if (v1knet::master) { net_master(); net_log(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n >= offsetof(NetSt, b) && v1knet::my_seat >= 0) { static NetSt s; memset(&s, 0, sizeof(s)); memcpy(&s, d, (std::min)(n, sizeof(s))); if (s.n >= 1 && s.n <= 4 && s.nr <= 8 && s.nb <= 24) net_apply(s, v1knet::round_id); }
        else if (slave_game) net_leave("the master's round is over");
    }
    if (slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
    net_log();
    if (!slave_game) { v1knet::send_input(NULL, 0); return false; }
    NetIn o; memset(&o, 0, sizeof(o)); const Plyr &q = pl[me];
    o.x = q.p.x; o.y = q.p.y; o.z = q.p.z; o.yaw = q.yaw; o.pitch = q.aim_pitch; o.moved = q.moved; o.epoch = q.epoch; o.jet = q.jet ? 1 : 0; o.pseq = net_pulse_seq; o.rseq = net_rock_seq; net3(o.f, net_f); net3(o.d, net_d);
    v1knet::send_input(&o, sizeof(o));
    const float dt = 1.f / 120.f; int k = 0;
    while (acc >= dt && k < 24) { net_slave_step(in, dt); acc -= dt; sim_t += dt; st_t += dt; k++; }
    if (sound_on) { bool hub = st == S_PLAY; if (hub && !voices[7].on) gb_sfx(7, SND_CROWD, 46); else if (!hub && voices[7].on && voices[7].sound == SB + SND_CROWD) voices[7].on = false; }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); gb_build_frame(++frames, in); }
    return true;
}
static void gb_tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    static bool trig_was = false, start_was = false; static int coins_seen = 0;
    bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f, sel = trig && !trig_was; trig_was = trig;
    if (in.start && !start_was) sel = true; start_was = in.start;
    if (in.coins != coins_seen) { if (in.coins > coins_seen) sel = true; coins_seen = in.coins; }
    static const bool att_hold = getenv("GRIDBUST_ATTRACT") != nullptr;
    if (autoplay && (st == S_TITLE || st == S_OVER) && st_t > 2.f && !att_hold) sel = true; if (d1k::hs::run("gridbust", st == S_OVER && st_t > 2.f, pl.empty() ? 0 : pl[me].score, in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (!v1knet::can_start()) sel = false;   /* a linked slave: pod 1 starts the game */
    if (v1knet::on && net_tick(el, in)) { PIC_AddEvent(gb_tick, 2.0); return; }
    if (st == S_TITLE && sel) { gb_start_round(); st = S_INTRO; st_t = 0; gb_sfx(0, SND_GO, 100); }
    else if (st == S_OVER && sel && st_t > 3.f) { st = S_TITLE; st_t = 0; v1knet::end_round(); pl.clear(); bolts.clear(); rocks.clear(); fxs.clear(); spks.clear(); }
    int n = 0;
    while (acc >= dt && n < 24) {
        if (st == S_INTRO && st_t >= (autoplay ? 0.5f : intro_len)) { st = S_PLAY; st_t = 0; dl("t=%.1f play", sim_t); }
        if ((st == S_PLAY || st == S_OVER) && !pl.empty()) {
            for (size_t i = 0; i < pl.size(); i++) { Plyr &q = pl[i];
                if (q.fire_cool > 0) q.fire_cool -= dt; if (q.rocket_cool > 0) q.rocket_cool -= dt; if (q.inv > 0) q.inv -= dt;
                if (!q.alive && st == S_PLAY) { q.dead_t -= dt; if (q.dead_t <= 0) { gb_place(q, false, (int)i); if ((int)i == me) have_off = false; } } }
            if (st == S_PLAY) {
                gb_step_human(in, dt); gb_guide(in, dt);
                for (size_t i = 1; i < pl.size(); i++) if (pl[i].alive && pl[i].cpu) { gb_step_cpu((int)i, dt); gb_move_bot((int)i, dt); }
                for (size_t i = 0; i < pl.size(); i++) if (!(v1knet::on && i != (size_t)me && !pl[i].cpu)) gb_step_body(pl[i], dt);   /* a linked person's pod does its own climbing */
                for (size_t a = 0; a < pl.size(); a++) for (size_t b = a + 1; b < pl.size(); b++) {   /* bodies do not overlap */
                    Plyr &A = pl[a], &B = pl[b]; if (!A.alive || !B.alive || fabsf(A.p.z - B.p.z) > 1500.f) continue;
                    V d(B.p.x - A.p.x, B.p.y - A.p.y, 0); float l = len(d); if (l >= 900.f || l < 1.f) continue;
                    V push = d * ((900.f - l) / l); bool a_fixed = a == (size_t)me && !autoplay;
                    if (a_fixed) B.p += push; else { A.p -= push * 0.5f; B.p += push * 0.5f; } }
                for (Plyr &q : pl) if (q.alive && q.jet) { q.jet_snd -= dt; if (q.jet_snd <= 0) { q.jet_snd = 0.35f; if (&q == &pl[me]) gb_sfx(5, SND_JET, 70); } }
                time_left -= dt;
                if (time_left <= 0) { time_left = 0; st = S_OVER; st_t = 0; gb_sfx(0, SND_END, 110);
                    dl("t=%.1f game over: %d %d %d %d", sim_t, pl[0].score, pl.size() > 1 ? pl[1].score : 0, pl.size() > 2 ? pl[2].score : 0, pl.size() > 3 ? pl[3].score : 0); }
            }
            gb_step_shots(dt);
        }
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    if (sound_on) { bool hub = st == S_PLAY; if (hub && !voices[7].on) gb_sfx(7, SND_CROWD, 46); else if (!hub && voices[7].on && voices[7].sound == SB + SND_CROWD) voices[7].on = false; }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); gb_build_frame(++frames, in); }
    if (sim_t >= next_log) { next_log = sim_t + 2.0;
        if (!pl.empty()) { const Plyr &q = pl[me];
            dl("t=%.0f st %d me (%.0f %.0f %.0f) alive %d shield %.2f jet %d | scores %d %d %d %d | bolts %zu rocks %zu | left %.0f",
               sim_t, (int)st, q.p.x, q.p.y, q.p.z, q.alive, q.shield, q.jet, pl[0].score, pl.size() > 1 ? pl[1].score : 0, pl.size() > 2 ? pl[2].score : 0, pl.size() > 3 ? pl[3].score : 0,
               bolts.size(), rocks.size(), time_left); }
        else dl("t=%.0f title", sim_t); }
    PIC_AddEvent(gb_tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".gridbust").c_str(), "w");
    walk_speed = envf("GRIDBUST_WALK", walk_speed); jet_acc = envf("GRIDBUST_JET", jet_acc); grav = envf("GRIDBUST_GRAV", grav); climb_max = envf("GRIDBUST_CLIMB", climb_max);
    bolt_speed = envf("GRIDBUST_BOLT", bolt_speed); rocket_speed = envf("GRIDBUST_ROCKET", rocket_speed); hit_score = (int)envf("GRIDBUST_HIT", (float)hit_score);
    pulse_dmg = envf("GRIDBUST_DMG", pulse_dmg); rocket_dmg = envf("GRIDBUST_RDMG", rocket_dmg); splash_r = envf("GRIDBUST_SPLASH", splash_r);
    arena_r = envf("GRIDBUST_RADIUS", arena_r); ceil_z = envf("GRIDBUST_CEIL", ceil_z); respawn_delay = envf("GRIDBUST_RESPAWN", respawn_delay);
    pulse_gap = envf("GRIDBUST_GAP", pulse_gap); rocket_gap = envf("GRIDBUST_RGAP", rocket_gap); kill_bonus = (int)envf("GRIDBUST_KILL", (float)kill_bonus);
    game_fps = envf("GRIDBUST_FPS", game_fps); game_len = envf("GRIDBUST_TIME", game_len); n_cpu = (int)envf("GRIDBUST_CPU", (float)n_cpu); intro_len = envf("GRIDBUST_INTRO", intro_len);
    autoplay = envf("GRIDBUST_AUTO", 0) != 0;
    gb_build_arena(); gb_build_attract(); gb_build_actors(); gb_make_hud(); gb_make_sounds();
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    PIC_AddEvent(gb_tick, 2.0);
    v1knet::init("GRBS", false, dl);   /* start-together match (Bobby: join-in-progress is for Flying Aces and VTOL) */
    dl("Grid Busters rebuild started");
}
}   /* namespace gridbust */
