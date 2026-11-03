/* Sphere (Virtuality 2000 series, 1994) - native rebuild from footage. No copy of the game turned up (H:\Games\Virtuality, both SU2000 dumps, Error Dan's su3000), so every model
   is built here and no game file is read. Included after vpix_dactyl1k.h; switched on by VPIX_NATIVE=sphere2k. Dev only, not in a release.
   Evidence: one promo, 3 copies of the same 16 s capture (Y4Qdx7oF-Ig; 20250728_rm_sphere_isdn, same picture, 8 kHz sound; 20250728_WMP_sphere 176x144 but the widest sound, Cybermind "Sphere for the SU & SD2000").
   Notes: Projects/Virtuality 1000 Revival/pass_1to1_2026-11-02/sphere2k.md. The whole clip runs under a blinking INSERT CREDIT: it is the machine's attract demo, not a paid game.
   FROM FOOTAGE: navy sky; grey and olive faceted mountains over dark green hills and bands of light green and olive; tripods of three pale legs with one big sphere on top (yellow; a far one cyan);
   three shots burst on a tripod (two low on the legs, one high), its legs cross and tumble flat, the sphere drops and flies to the shooter: +25 on the red box; a tank takes at least 8 hits and lives;
   score +2 per hit while the yellow box is up, +1 without it (25 27 29 ... 41, then 42 43 44); rivals score +1 a hit; three scores only, in the tank colours red, yellow, blue; shots red balls, far off orange dashes in a stream of about 6 a second;
   burst: yellow flash, orange ball, lilac ball, solid red ball, red flecks, about 0.6 s; tanks of cream-rimmed black tracks, grey deck, a colour band on the back, colour oval on the deck, silver turret, long pale barrel;
   the demo opens on a fixed camera your tank drives away from at about 2600 mm/s (rivals stand still and trade fire), then goes first person at 4:51; first person over the barrel, deck, tracks and red oval;
   sight [ ] with four black ticks fixed at picture centre, a yellow box round it with a falling number beside it while locked, then a small ring (red about 1.2 s, then blue);
   strip of ticks 4.4 picture units a degree (so 70 degrees across) whose 0 rides under the red arrow at the hull's bearing; clock from 5:00 twice in the top row; INSERT CREDIT 0.5 s on, 0.5 s off;
   a still picture (pink mottled sky, pale ridge, two tanks, three missiles, navy bars top and bottom) in the attract. Sounds taken from the spectrum of the WMP copy: a 1 s falling and rising whoop after the sphere lands, a rasp of about 6 pulses a second
   while firing, a low hum. No voice (speech recognition found none).
   INVENTED (no source): what the number by the box really is (we show distance / 1000 mm; it fell 25 to 7 at 4.4 a second), lock range and angle, the ring's meaning, hits to kill (16), respawn, sphere return time, CPU play, how many tripods (4 in the clip, 6 here),
   that 3 shots always bring a tripod down (the clip only shows the last 3 at most), whether a paid game opens first person (we do), winner, end of game, what cyan means, attract durations, boom sound, driving and control feel.
   Footage shows no 4th score though listings say 1-4 players: a 4th tank's score is not drawn.
   Units: mm, seconds. World X right, Y forward, Z up. Env: SPHERE2K_AUTO, _CAM, _LOOK (yawdeg,pitchdeg), _FPS, _TIME, _SPEED, _TURN, _HP, _RESPAWN, _SKILL, _CPUS. */
#include <functional>
#include "vpix_1k_net.h"         /* link play, 2-4 pods: see the "link play" block above the loop */
namespace sphere2k {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at;
using d1k::Mesh; using d1k::dput; using d1k::blit; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound; using d1k::add_bitmap;

/* ---- tuning ---- */
static float game_len = 300.f;                             /* the clock reads 4:57 three seconds in: 5:00 */
static float game_fps = 20.f;
static float drive_speed = 2600.f, turn_rate = 0.9f, respawn_delay = 3.f, cpu_skill = 1.f;   /* speed solved from the clip: tank and tripod both agree on about 2600 */
static int tank_hp = 16, n_cpu = 2;                        /* HP invented; the clip's locked tank takes 8 hits and lives */
static bool autoplay = false;
static const float SHOT_SPEED = 24000.f, FIRE_GAP = 0.165f, LOCK_DEG = 7.f, LOCK_MIN = 6000.f, LOCK_MAX = 30000.f, HIT_R = 2200.f, CHASE_SECS = 8.6f, DEMO_SECS = 40.f, STILL_SECS = 8.f, CAM_Y = -100000.f;

enum { M_LAND = 0x210001, M_RIM, M_TRIPOD, M_LEG, M_BALL, M_CYAN, M_TANK, M_TANK2, M_TANK3, M_TANK4, M_TURRET, M_GUNVIEW, M_SHOT, M_DASH, M_BURST_A, M_BURST_B, M_BURST_C, M_BURST_D };
enum { BM_W = 400, BM_B = 412, BM_R = 424, BM_Y = 436, BM_LET = 450, BM_BOX = 480, BM_TGT, BM_ARROW, BM_TICK, BM_TICK2, BM_SIGHT, BM_RING_R, BM_RING_B, BM_STILL };
enum { SND_FIRE = 0x7E000001, SND_BOOM, SND_TAKE, SND_HURT, SND_GO, SND_ENG };

/* ---- helpers ---- */
static V tw(const M &R, V a) { return R.r[0] * a.x + R.r[1] * a.y + R.r[2] * a.z; }
static M yawed(float y) { return d1k::yawed(y); }
static V fwd_of(float y) { return V(-sinf(y), cosf(y), 0); }
static float wrap(float a) { while (a > 3.14159265f) a -= 6.2831853f; while (a < -3.14159265f) a += 6.2831853f; return a; }
static void quad(Mesh &m, V a, V b, V c, V d, uint32_t rgb) { m.poly({a, b, c, d}, rgb); }
static uint32_t lit(uint32_t c, float f) { return d1k::kit_shade(c, f); }

/* ---- the land: a heightfield. The stretch the demo camera looks down is solved from the clip's frames; mountains round the edge ---- */
static const int GN = 64; static const float CELL = 10000.f, HALF = GN * CELL * 0.5f;
static float hgt[GN + 1][GN + 1]; static float pat[GN + 1][GN + 1];
static float noise2(float x, float y, int seed) {   /* smooth value noise */
    auto h = [&](int i, int j) { uint32_t n = (uint32_t)(i * 73856093) ^ (uint32_t)(j * 19349663) ^ (uint32_t)(seed * 83492791); n = (n ^ (n >> 13)) * 1274126177u; return ((n ^ (n >> 16)) & 0xFFFF) / 65535.f; };
    int i = (int)floorf(x), j = (int)floorf(y); float u = x - i, v = y - j; u = u * u * (3 - 2 * u); v = v * v * (3 - 2 * v);
    return (h(i, j) * (1 - u) + h(i + 1, j) * u) * (1 - v) + (h(i, j + 1) * (1 - u) + h(i + 1, j + 1) * u) * v;
}
static float profile(float d) {   /* ground height against distance from the demo camera */
    static const float D[] = {-40000, 0, 6800, 21000, 34000, 44000, 52000, 70000, 100000, 140000}, H[] = {4000, 4600, 4000, 0, 0, -1000, 5000, 5000, 10000, 5000};
    if (d <= D[0]) return H[0]; for (int i = 0; i < 9; i++) if (d < D[i + 1]) { float u = (d - D[i]) / (D[i + 1] - D[i]); u = u * u * (3 - 2 * u); return H[i] + (H[i + 1] - H[i]) * u; } return H[9];
}
static void make_heights() {
    for (int a = 0; a <= GN; a++) for (int b = 0; b <= GN; b++) {
        float x = -HALF + a * CELL, y = -HALF + b * CELL, r = sqrtf(x * x + y * y);
        float low = (noise2(x / 70000.f, y / 70000.f, 1) - 0.5f) * 7000.f;
        float w = expf(-(x * x) / (70000.f * 70000.f)), d = y - CAM_Y; float dw = clampf((d + 60000.f) / 30000.f, 0.f, 1.f) * clampf((170000.f - d) / 30000.f, 0.f, 1.f); w *= dw;
        float e = clampf((r - 190000.f) / 110000.f, 0.f, 1.f); e = e * e * (3 - 2 * e);
        float mt = e * (45000.f + 35000.f * noise2(x / 70000.f, y / 70000.f, 3) + 16000.f * noise2(x / 26000.f, y / 26000.f, 4));
        hgt[a][b] = low * (1 - w) + profile(d) * w + mt; pat[a][b] = noise2(x / 60000.f, y / 60000.f, 5);
    }
}
static float ground_at(float x, float y) {
    float fx = clampf((x + HALF) / CELL, 0.f, GN - 0.001f), fy = clampf((y + HALF) / CELL, 0.f, GN - 0.001f); int a = (int)fx, b = (int)fy; float u = fx - a, v = fy - b;
    if (u + v < 1.f) return hgt[a][b] + (hgt[a + 1][b] - hgt[a][b]) * u + (hgt[a][b + 1] - hgt[a][b]) * v;
    return hgt[a + 1][b + 1] + (hgt[a][b + 1] - hgt[a + 1][b + 1]) * (1 - u) + (hgt[a + 1][b] - hgt[a + 1][b + 1]) * (1 - v);
}
static uint32_t hash3(int a, int b, int c) { uint32_t n = (uint32_t)(a * 73856093) ^ (uint32_t)(b * 19349663) ^ (uint32_t)(c * 83492791); n = (n ^ (n >> 13)) * 1274126177u; return n ^ (n >> 16); }
/* colours sampled off the clip: big facets on the rock, bands and big patches below, no speckle. fa, fb, fc name the large facet a triangle belongs to */
static uint32_t land_col(float x, float y, float h, float p, int fa, int fb, int fc) {
    static const uint32_t G[6] = {0x757B70, 0x8A8C80, 0x9C9E92, 0x6C6E64, 0x828274, 0xA4A49A}; uint32_t n = hash3(fa, fb, fc); float d = y - CAM_Y;
    if (h > 24000.f) return (n % 9) == 0 ? 0x7E7C65 : G[n % 6];
    if (h > 12000.f) return (n % 5) < 2 ? 0x8C8A50 : G[n % 6];                     /* the zigzag of grey and olive at the foot */
    float rr = sqrtf(x * x + y * y);
    if (rr > 150000.f || (fabsf(x) < 90000.f && d > 56000.f && d < 110000.f)) { int k = n % 10; return k < 3 ? 0x057709 : k < 6 ? 0x3C9048 : 0x5FA868; }   /* dark green hills */
    float dd = d + 0.25f * x;
    if (fabsf(x) < 80000.f && dd < 17000.f && d > -60000.f) return 0x52BA58;
    if (p > 0.64f) return 0x889934; if (p > 0.5f) return 0x5FAE5E; if (p > 0.34f) return 0x7DC482; return 0x46A850;
}
static void build_land() {
    Mesh m;
    /* rock is laid in big triangles, as the clip's; everything lower keeps the land's small cells */
    const int F = 3;
    for (int A = 0; A + F <= GN; A += F) for (int B = 0; B + F <= GN; B += F) {
        float hm = (hgt[A][B] + hgt[A + F][B] + hgt[A][B + F] + hgt[A + F][B + F]) * 0.25f, xc = -HALF + (A + F * 0.5f) * CELL, yc = -HALF + (B + F * 0.5f) * CELL;
        if (hm > 12000.f) { auto P = [&](int a, int b) { return V(-HALF + a * CELL, -HALF + b * CELL, hgt[a][b]); }; bool flip = hash3(A, B, 7) & 1;
            if (flip) { m.poly({P(A, B), P(A + F, B), P(A, B + F)}, land_col(xc, yc, hm, 0, A, B, 0)); m.poly({P(A + F, B), P(A + F, B + F), P(A, B + F)}, land_col(xc, yc, hm, 0, A, B, 1)); }
            else { m.poly({P(A, B), P(A + F, B), P(A + F, B + F)}, land_col(xc, yc, hm, 0, A, B, 0)); m.poly({P(A, B), P(A + F, B + F), P(A, B + F)}, land_col(xc, yc, hm, 0, A, B, 1)); }
            continue; }
        for (int a = A; a < A + F; a++) for (int b = B; b < B + F; b++) {
            float x0 = -HALF + a * CELL, y0 = -HALF + b * CELL, x1 = x0 + CELL, y1 = y0 + CELL;
            V p00(x0, y0, hgt[a][b]), p10(x1, y0, hgt[a + 1][b]), p01(x0, y1, hgt[a][b + 1]), p11(x1, y1, hgt[a + 1][b + 1]);
            float h = (p00.z + p10.z + p01.z + p11.z) * 0.25f; uint32_t c = land_col(x0 + CELL * 0.5f, y0 + CELL * 0.5f, h, pat[a / 2 * 2][b / 2 * 2], a / F, b / F, (a % F) + (b % F) >= F ? 1 : 0);
            m.poly({p00, p10, p01}, c); m.poly({p10, p11, p01}, c); } }
    /* laid just over the ground: the light bands across the view on a slant, and the olive middle narrowing away. The clip's edges are clean lines, finer than the land's cells */
    { const float LIFT = 90.f; auto G = [&](float x, float y) { return V(x, y, ground_at(x, y) + LIFT); };
      { const float OL[5][2] = {{16000, 26000}, {22000, 25000}, {30000, 17000}, {38000, 12000}, {46000, 9000}}; const uint32_t OC[4] = {0x889934, 0x97A23C, 0x889934, 0x8E9C36};
        for (int i = 0; i < 4; i++) for (int k = 0; k < 8; k++) { float u0 = -1 + k * 0.25f, u1 = u0 + 0.25f; float y0 = CAM_Y + OL[i][0], y1 = CAM_Y + OL[i + 1][0], w0 = OL[i][1], w1 = OL[i + 1][1];
            quad(m, G(u0 * w0, y0 - 0.25f * u0 * w0), G(u1 * w0, y0 - 0.25f * u1 * w0), G(u1 * w1, y1 - 0.25f * u1 * w1), G(u0 * w1, y1 - 0.25f * u0 * w1), OC[i]); } }
      static const uint32_t BC[5] = {0x62C870, 0x52BA58, 0x76CC80, 0x62C870, 0x7DC482}; const float BW = 5000.f;
      for (int b = 0; b < 5; b++) for (int r2 = 0; r2 < 2; r2++) for (int k = -8; k < 8; k++) { float x0 = k * 6000.f, x1 = x0 + 6000.f, dd0 = -8000.f + (b + r2 * 0.5f) * BW, dd1 = dd0 + BW * 0.5f;
          quad(m, G(x0, CAM_Y + dd0 - 0.25f * x0), G(x1, CAM_Y + dd0 - 0.25f * x1), G(x1, CAM_Y + dd1 - 0.25f * x1), G(x0, CAM_Y + dd1 - 0.25f * x0), BC[b]); } }
    m.add(M_LAND);
    /* outside the heightfield: a dark skirt so the ring never shows its edge */
    { Mesh s; const float R = 900000.f; for (int i = 0; i < 16; i++) { float a0 = 6.2831853f * i / 16, a1 = 6.2831853f * (i + 1) / 16;
          quad(s, V(cosf(a0) * R, sinf(a0) * R, -2000), V(cosf(a1) * R, sinf(a1) * R, -2000), V(cosf(a1) * R * 0.5f, sinf(a1) * R * 0.5f, -2000), V(cosf(a0) * R * 0.5f, sinf(a0) * R * 0.5f, -2000), 0x5A5A50); }
      s.add(M_RIM); }
}

/* ---- the attract's still picture, painted from scratch to the clip's frame (13.4 s on): it never moves ---- */
static void make_still() {
    const int W = 352, H = 288; std::vector<uint32_t> px((size_t)W * H);
    auto rgb = [](float r, float g, float b) { auto c = [](float v) { return (uint32_t)clampf(v, 0.f, 255.f); }; return 0xFF000000u | (c(b) << 16) | (c(g) << 8) | c(r); };
    auto edge = [](float x, const float (*P)[2], int n) { for (int i = 0; i < n - 1; i++) if (x <= P[i + 1][0]) { float u = (x - P[i][0]) / (P[i + 1][0] - P[i][0]); return P[i][1] + (P[i + 1][1] - P[i][1]) * u; } return P[n - 1][1]; };
    static const float SKY[][2] = {{0, 40}, {62, 42}, {85, 70}, {110, 82}, {170, 82}, {280, 68}, {352, 55}};
    static const float LAND[][2] = {{0, 108}, {60, 115}, {130, 124}, {220, 122}, {352, 120}};
    static const float RIDGE[][2] = {{0, 118}, {60, 128}, {165, 160}, {280, 190}, {352, 195}};
    const int TOP = 22, BOT = 265;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { float fx = (float)x, fy = (float)y, r, g, b;
        if (y < TOP || y >= BOT) { px[(size_t)y * W + x] = rgb(1, 2, 65); continue; }
        float n1 = noise2(fx / 22.f, fy / 12.f, 11), n2 = noise2(fx / 7.f, fy / 5.f, 12);
        if (fy < edge(fx, SKY, 7)) { float v = n1 * 0.7f + n2 * 0.3f; r = 104 + 80 * v; g = 56 + 70 * v; b = 64 + 62 * v; float vein = clampf((0.42f - n1) * 5.f, 0.f, 1.f); r += 24 * vein; g -= 24 * vein; b -= 18 * vein; }
        else if (fy < edge(fx, LAND, 5)) { float u = (fy - edge(fx, SKY, 7)) / 50.f, pale = clampf((fx - 80.f) / 60.f, 0.f, 1.f) * clampf(1.1f - u, 0.f, 1.f) * n1 * 1.5f; r = 118 + 110 * pale + 20 * n2; g = 120 + 106 * pale + 20 * n2; b = 112 + 76 * pale + 10 * n2; float ol = clampf((u - 0.45f) * 1.8f, 0.f, 1.f); r += (136 - r) * ol; g += (130 - g) * ol; b += (64 - b) * ol; }
        else if (fy < edge(fx, RIDGE, 5)) { float v = n1; r = 80 + 48 * v; g = 160 + 22 * v; b = 92 - 8 * v; float ol = clampf((fx - 170.f) / 120.f, 0.f, 1.f) * clampf((fy - 150.f) / 30.f + 0.6f, 0.f, 1.f); r += (150 - r) * ol; g += (156 - g) * ol; b += (60 - b) * ol;
            auto blob = [&](float cx, float cy, float rx, float ry) { float q = ((fx - cx) * (fx - cx)) / (rx * rx) + ((fy - cy) * (fy - cy)) / (ry * ry); float t = clampf(1.15f - q, 0.f, 1.f); r += (5 - r) * t; g += (140 - g) * t; b += (22 - b) * t; };
            blob(272, 136, 34, 21); blob(172, 132, 24, 8); blob(268, 132, 14, 8); }
        else { float v = n1 * 0.4f + n2 * 0.1f; r = 62 + 40 * v; g = 172 + 22 * v; b = 92 + 28 * v; float ol = clampf((165.f - fx) / 90.f, 0.f, 1.f) * clampf((fy - 225.f) / 25.f, 0.f, 1.f); r += (154 - r) * ol; g += (156 - g) * ol; b += (62 - b) * ol; }
        px[(size_t)y * W + x] = rgb(r, g, b); }
    auto fill = [&](std::initializer_list<std::pair<float, float>> pts, uint32_t col) { std::vector<std::pair<float, float>> P(pts); float y0 = 1e9f, y1 = -1e9f; for (auto &q : P) { y0 = (std::min)(y0, q.second); y1 = (std::max)(y1, q.second); }
        for (int y = (int)y0; y <= (int)y1 && y < H; y++) { float xa = 1e9f, xb = -1e9f; for (size_t i = 0; i < P.size(); i++) { auto a = P[i], b = P[(i + 1) % P.size()]; float yy = y + 0.5f; if ((a.second <= yy) != (b.second <= yy)) { float x = a.first + (b.first - a.first) * (yy - a.second) / (b.second - a.second); xa = (std::min)(xa, x); xb = (std::max)(xb, x); } }
            for (int x = (int)xa; x <= (int)xb; x++) if (x >= 0 && x < W && y >= 0) px[(size_t)y * W + x] = col; } };
    auto oval = [&](float cx, float cy, float rx, float ry, uint32_t col) { for (int y = (int)(cy - ry); y <= (int)(cy + ry); y++) for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++) { float a = (x - cx) / rx, b = (y - cy) / ry; if (a * a + b * b <= 1 && x >= 0 && x < W && y >= 0 && y < H) px[(size_t)y * W + x] = col; } };
    const uint32_t CR = rgb(200, 190, 178), BK = rgb(38, 38, 38), GY = rgb(150, 152, 150), SI = rgb(214, 218, 216), YL = rgb(245, 235, 30), RD = rgb(235, 50, 30), OR = rgb(245, 130, 30), WH = rgb(240, 235, 200), GN2 = rgb(20, 130, 50), MG = rgb(210, 40, 120);
    /* far tank */
    fill({{91, 160}, {100, 157}, {143, 166}, {142, 176}, {103, 178}, {92, 172}}, BK); fill({{91, 160}, {103, 158}, {112, 161}, {100, 166}, {93, 170}}, GY); oval(99, 167, 3, 2, RD); fill({{112, 162}, {125, 159}, {129, 164}, {113, 166}}, SI);
    /* near tank */
    fill({{123, 197}, {135, 191}, {199, 205}, {197, 219}, {168, 230}, {132, 215}}, CR); fill({{128, 200}, {135, 196}, {164, 208}, {156, 217}, {134, 210}}, BK); fill({{158, 216}, {197, 204}, {195, 214}, {168, 227}}, BK); fill({{139, 193}, {185, 201}, {160, 212}, {132, 199}}, GY);
    oval(154, 203, 8, 3, YL); fill({{133, 205}, {137, 202}, {150, 212}, {147, 215}}, YL); fill({{157, 197}, {172, 193}, {176, 203}, {160, 203}}, SI); fill({{172, 193}, {174, 184}, {178, 185}, {175, 196}}, SI);
    /* missiles */
    auto missile = [&](float x, float y) { fill({{x - 22, y - 1}, {x - 2, y - 2}, {x - 2, y + 1}}, WH); oval(x + 2, y + 1, 8, 4, OR); oval(x + 6, y + 2, 4, 3, RD); oval(x + 3, y + 4, 4, 1.5f, MG); };
    fill({{60, 112}, {70, 123}, {78, 123}, {62, 128}}, GN2); fill({{72, 113}, {80, 122}, {88, 122}, {76, 124}}, GN2); missile(82, 123); missile(162, 140); missile(201, 156);
    add_bitmap(BM_STILL, W, H, px);
}

/* ---- the sphere: smooth, lit from the upper left, as the clip's sphere reads ---- */
static void ball(Mesh &m, V c, float R, uint32_t col) {
    const int NA = 16, NE = 10; const V light = norm(V(-0.35f, -0.75f, 0.55f));
    auto P = [&](int a, int e) { float th = 6.2831853f * a / NA, ph = -1.5708f + 3.14159f * e / NE; return c + V(cosf(th) * cosf(ph) * R, sinf(th) * cosf(ph) * R, sinf(ph) * R); };
    for (int a = 0; a < NA; a++) for (int e = 0; e < NE; e++) { V p0 = P(a, e), p1 = P(a + 1, e), p2 = P(a + 1, e + 1), p3 = P(a, e + 1); V n = norm((p0 + p1 + p2 + p3) * 0.25f - c);
        float s = 0.68f + 0.32f * clampf(dot(n, light) + 0.25f, 0.f, 1.f); if (e == 0) m.poly({p0, p2, p3}, lit(col, s)); else if (e == NE - 1) m.poly({p0, p1, p3}, lit(col, s)); else m.poly({p0, p1, p2, p3}, lit(col, s)); }
}
static const float TRI_H = 8200.f, TRI_FOOT = 2700.f, BALL_R = 1500.f, LEG_T = 190.f;
static const float LEG_LEN = 4500.f;
static void build_tripod() {
    Mesh t; const uint32_t L = 0xD4D6D0; V top(0, 0, TRI_H);
    for (int k = 0; k < 3; k++) { float a = 6.2831853f * k / 3 + 1.5708f; d1k::kit_bar(t, V(cosf(a) * TRI_FOOT, sinf(a) * TRI_FOOT, -500), top, LEG_T, LEG_T, L); }
    t.add(M_TRIPOD);
    { Mesh l; d1k::kit_bar(l, V(0, 0, -LEG_LEN), V(0, 0, LEG_LEN), LEG_T, LEG_T, L); l.add(M_LEG); }
    { Mesh b; ball(b, V(0, 0, 0), BALL_R, 0xF4F01C); b.add(M_BALL); }
    { Mesh b; ball(b, V(0, 0, 0), BALL_R, 0x2CC8C4); b.add(M_CYAN); }
}

/* ---- tanks: cream-rimmed black tracks, grey deck, colour band on the back, colour oval on the deck ---- */
static const float TK_XI = 950.f, TK_XO = 1700.f, TK_L = 2450.f, TK_H = 1000.f, DECK_Z = 825.f, EYE_Z = 1465.f;
static void track(Mesh &m, float xi, float xo) {
    const uint32_t CR = 0xC9BFB5, CR2 = 0xA89E94, BK = 0x2C2A28, LG = 0x55524E; const float L = TK_L, H = TK_H, ch = 300.f, out = xo > xi ? 1.f : -1.f;
    auto side = [&](float x, uint32_t c) { m.poly({V(x, -L + ch, 0), V(x, L - ch, 0), V(x, L, H * 0.5f), V(x, L - ch * 0.6f, H), V(x, -L + ch * 0.6f, H), V(x, -L, H * 0.5f)}, c); };
    side(xo, CR); side(xi, BK);
    { float x = xo + out * 12.f, i = 150.f; m.poly({V(x, -L + ch, i), V(x, L - ch, i), V(x, L - i, H * 0.5f), V(x, L - ch, H - i), V(x, -L + ch, H - i), V(x, -L + i, H * 0.5f)}, BK); }   /* the black face inside its rim */
    quad(m, V(xi, -L + ch * 0.6f, H), V(xo, -L + ch * 0.6f, H), V(xo, L - ch * 0.6f, H), V(xi, L - ch * 0.6f, H), CR);   /* top plate */
    quad(m, V(xo, L - ch * 0.6f, H), V(xi, L - ch * 0.6f, H), V(xi, L, H * 0.5f), V(xo, L, H * 0.5f), CR2); quad(m, V(xi, L, H * 0.5f), V(xi, L - ch, 0), V(xo, L - ch, 0), V(xo, L, H * 0.5f), BK);   /* front */
    quad(m, V(xi, -L + ch * 0.6f, H), V(xo, -L + ch * 0.6f, H), V(xo, -L, H * 0.5f), V(xi, -L, H * 0.5f), CR); quad(m, V(xi, -L, H * 0.5f), V(xo, -L, H * 0.5f), V(xo, -L + ch, 0), V(xi, -L + ch, 0), CR2);   /* the tall cream plate at the back */
    { float a = xi - out * 14.f; for (int e = 0; e < 2; e++) { float yc = e ? L - 520.f : -L + 520.f, zc = H * 0.5f; for (int i = 0; i < 6; i++) { float t0 = 6.2831853f * i / 6, t1 = t0 + 0.55f; const float r0 = 240.f, r1 = 400.f; quad(m, V(a, yc + cosf(t0) * r0, zc + sinf(t0) * r0), V(a, yc + cosf(t1) * r0, zc + sinf(t1) * r0), V(a, yc + cosf(t1) * r1, zc + sinf(t1) * r1), V(a, yc + cosf(t0) * r1, zc + sinf(t0) * r1), i & 1 ? 0xB4B4AC : 0x707068); } } }   /* the wheels' pale patches */
    for (int i = 0; i < 7; i++) { float y = -L + ch + (2 * L - 2 * ch) * (i + 0.2f) / 7; float a = xi - out * 10.f; quad(m, V(a, y, 40), V(a, y + 260, 40), V(a, y + 260, H - 120), V(a, y, H - 120), LG); float b = xo + out * 18.f; quad(m, V(b, y, 200), V(b, y + 200, 200), V(b, y + 200, H - 200), V(b, y, H - 200), LG); }
}
static void build_tank(int id, uint32_t band) {
    Mesh m; const uint32_t DK = 0xC9C9C7, GY = 0x9A9F9C, LT = 0xB8BAB6;
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; track(m, q * TK_XI, q * TK_XO); }
    const float X = TK_XI + 4.f, Y0 = -2100.f, Y1 = 2000.f;
    quad(m, V(-X, Y0, DECK_Z - 200), V(X, Y0, DECK_Z - 200), V(X, Y1, DECK_Z), V(-X, Y1, DECK_Z), DK);          /* deck */
    quad(m, V(-X, Y1, DECK_Z), V(X, Y1, DECK_Z), V(X, Y1 + 200, 200), V(-X, Y1 + 200, 200), GY);                 /* nose */
    quad(m, V(-X, Y0, 200), V(X, Y0, 200), V(X, Y0, DECK_Z - 200), V(-X, Y0, DECK_Z - 200), GY);                 /* back */
    quad(m, V(-X, Y0 - 12, 300), V(X, Y0 - 12, 300), V(X, Y0 - 12, 660), V(-X, Y0 - 12, 660), band);             /* the colour band */
    quad(m, V(-X, Y0 - 8, 200), V(X, Y0 - 8, 200), V(X, Y0 - 8, 300), V(-X, Y0 - 8, 300), LT);
    { V a[8]; for (int i = 0; i < 8; i++) { float t = 6.2831853f * i / 8; float y = 1550.f + cosf(t) * 440.f; a[i] = V(sinf(t) * 520.f, y, DECK_Z - 200 + 200.f * (y - Y0) / (Y1 - Y0) + 14.f); }
      m.poly({a[0], a[7], a[6], a[5], a[4], a[3], a[2], a[1]}, band); }                                        /* the oval on the deck */
    m.add(id);
}
static void build_turret() {
    Mesh m; const uint32_t SI = 0xE8EAE6, SD = 0xCCD0CC; const int N = 8; const float R0 = 660.f, R1 = 420.f, Z0 = DECK_Z - 40.f, Z1 = Z0 + 380.f;
    for (int i = 0; i < N; i++) { float a0 = 6.2831853f * i / N, a1 = 6.2831853f * (i + 1) / N; uint32_t c = lit(SD, 0.8f + 0.2f * sinf(a0 + 0.8f)); quad(m, V(cosf(a0) * R0, sinf(a0) * R0, Z0), V(cosf(a1) * R0, sinf(a1) * R0, Z0), V(cosf(a1) * R1, sinf(a1) * R1, Z1), V(cosf(a0) * R1, sinf(a0) * R1, Z1), c); }
    { V a[8]; for (int i = 0; i < 8; i++) { float t = 6.2831853f * i / 8; a[i] = V(cosf(t) * R1, sinf(t) * R1, Z1); } m.poly({a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]}, SI); }
    m.box(V(-220, -220, Z1), V(0, 40, Z1 + 90), 0xB8C840, 0x90A030, 0x90A030);    /* the hatches */
    m.box(V(70, -240, Z1), V(300, -40, Z1 + 60), 0x202024);
    { const int S = 8; const V b0(0, 300, Z0 + 200), b1(0, 2700, Z0 + 640); for (int i = 0; i < S; i++) { float a0 = 6.2831853f * i / S, a1 = 6.2831853f * (i + 1) / S; float r0 = 140.f, r1 = 70.f; uint32_t c = lit(0xE4E4DC, 0.72f + 0.28f * sinf(a0 + 0.6f));
          quad(m, b0 + V(cosf(a0) * r0, 0, sinf(a0) * r0), b0 + V(cosf(a1) * r0, 0, sinf(a1) * r0), b1 + V(cosf(a1) * r1, 0, sinf(a1) * r1), b1 + V(cosf(a0) * r1, 0, sinf(a0) * r1), c); } }
    m.add(M_TURRET);
}
/* the view from inside, eye at the origin: solved from the clip. Barrel tip 11% of the way down (tan .113), edges 60 units each side at the bottom of the picture (tan .24): a round-nosed cone, lit on the left. White dome across the bottom */
static void build_views() {
    { Mesh m; const int N = 16; struct S { float y, r, z; } st[4] = {{600.f, 115.f, -350.f}, {1800.f, 130.f, -335.f}, {2950.f, 85.f, -339.f}, {3100.f, 8.f, -335.f}};
      for (int s = 0; s < 3; s++) for (int i = 0; i < N; i++) { float a0 = 6.2831853f * i / N, a1 = 6.2831853f * (i + 1) / N, am = (a0 + a1) * 0.5f, sh = 0.5f - 0.5f * cosf(am); uint32_t c = lit(0xF2F2EC, 0.66f + 0.34f * (1 - sh * sh));
          quad(m, V(cosf(a0) * st[s].r, st[s].y, st[s].z + sinf(a0) * st[s].r), V(cosf(a1) * st[s].r, st[s].y, st[s].z + sinf(a1) * st[s].r), V(cosf(a1) * st[s + 1].r, st[s + 1].y, st[s + 1].z + sinf(a1) * st[s + 1].r), V(cosf(a0) * st[s + 1].r, st[s + 1].y, st[s + 1].z + sinf(a0) * st[s + 1].r), c); }
      { const int K = 24; for (int i = 0; i < K; i++) { float t0 = 6.2831853f * i / K, t1 = 6.2831853f * (i + 1) / K; m.poly({V(0, 300.f, -600.f), V(cosf(t0) * 1200.f, 300.f + sinf(t0) * 1200.f, -600.f), V(cosf(t1) * 1200.f, 300.f + sinf(t1) * 1200.f, -600.f)}, 0xE6EAE6); } }
      m.add(M_GUNVIEW); }
    { Mesh m; ball(m, V(0, 0, 0), 400.f, 0xF01808); m.add(M_SHOT); }   /* shots: small red balls... */
    { Mesh m; m.box(V(-110, -680, -110), V(110, 680, 110), 0xF06020, 0xE83818, 0xE83818); m.add(M_DASH); }   /* ...far off, orange dashes */
    auto bur = [](int id, std::initializer_list<uint32_t> cols, int seed) { Mesh m; std::mt19937 q(seed); std::vector<uint32_t> C(cols); const int NA = 12, NE = 7; const float R = 1000.f;
        auto P = [&](int a, int e) { float th = 6.2831853f * a / NA, ph = -1.5708f + 3.14159f * e / NE; return V(cosf(th) * cosf(ph) * R, sinf(th) * cosf(ph) * R, sinf(ph) * R); };
        for (int a = 0; a < NA; a++) for (int e = 0; e < NE; e++) { uint32_t c = C[q() % C.size()]; V p0 = P(a, e), p1 = P(a + 1, e), p2 = P(a + 1, e + 1), p3 = P(a, e + 1); if (e == 0) m.poly({p0, p2, p3}, c); else if (e == NE - 1) m.poly({p0, p1, p3}, c); else m.poly({p0, p1, p2, p3}, c); }
        m.add(id); };
    bur(M_BURST_A, {0xF0C020, 0xF08020, 0xE05818, 0xF4E040}, 3);
    bur(M_BURST_B, {0xA870C0, 0xD0A040, 0xE07040, 0xB888C8, 0xE8C050}, 4);
    bur(M_BURST_C, {0xE81408, 0xD01008, 0xF02810, 0x901008, 0xE81408}, 5);
    { Mesh m; std::mt19937 q(6); const float R = 1000.f;   /* the last of it: scattered red flecks */
      static const uint32_t C[4] = {0xE81408, 0xD02010, 0xB01808, 0xC8A040};
      for (int i = 0; i < 90; i++) { float a = (q() % 628) / 100.f, z = ((q() % 200) - 100) / 100.f, s = sqrtf(1 - z * z); V n(cosf(a) * s, sinf(a) * s, z), u = norm(cross(n, V(0.3f, 0.5f, 0.8f))), w = cross(n, u), c = n * R; float t = 0.06f + (q() % 8) / 100.f;
          m.poly({c + u * (t * R * 1.6f), c - u * (t * R * 0.8f) + w * (t * R * 0.4f), c - u * (t * R * 0.8f) - w * (t * R * 0.4f)}, C[q() % 4]); }
      m.add(M_BURST_D); }
}

/* ---- HUD pictures: a small square font (the clip's digits, the gold letters) ---- */
static void make_hud_sphere() {
    static const char *DG[10] = {"111101101101111", "010110010010111", "111001111100111", "111001111001111", "101101111001001", "111100111001111", "111100111101111", "111001010010010", "111101111101111", "111101111001111"};
    const uint32_t cols[4] = {0xFFF0F0F0u, 0xFFF04830u, 0xFF3028F8u, 0xFF70E0E8u}; const int base[4] = {BM_W, BM_B, BM_R, BM_Y};   /* colours are A B G R */
    for (int c = 0; c < 4; c++) { for (int d = 0; d < 10; d++) { std::vector<uint32_t> px(6 * 10, 0);
            for (int y = 0; y < 5; y++) for (int x = 0; x < 3; x++) if (DG[d][y * 3 + x] == '1') for (int b = 0; b < 2; b++) for (int a = 0; a < 2; a++) px[(size_t)(y * 2 + b) * 6 + x * 2 + a] = cols[c];
            add_bitmap(base[c] + d, 6, 10, px); }
        { std::vector<uint32_t> px(4 * 10, 0); for (int y : {2, 3, 6, 7}) for (int a = 1; a < 3; a++) px[(size_t)y * 4 + a] = cols[c]; add_bitmap(base[c] + 10, 4, 10, px); } }
    for (int ch = 0; ch < 26; ch++) { std::vector<uint32_t> px(d1k::FW * d1k::FH, 0);
        for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (d1k::FONT[ch][y * 5 + x] == '1') for (int b = 0; b < 2; b++) for (int a = 0; a < 2; a++) px[(y * 2 + b) * d1k::FW + x * 2 + a] = 0xFF40C0F0u;
        add_bitmap(BM_LET + ch, d1k::FW, d1k::FH, px); }
    { const int W = 46, H = 18; std::vector<uint32_t> px((size_t)W * H, 0); for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (x < 2 || y < 2 || x >= W - 2 || y >= H - 2) px[(size_t)y * W + x] = 0xFF2020F0u; add_bitmap(BM_BOX, W, H, px); }
    { const int W = 30, H = 22; std::vector<uint32_t> px((size_t)W * H, 0); for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (x < 2 || y < 2 || x >= W - 2 || y >= H - 2) px[(size_t)y * W + x] = 0xFF20F0F0u; add_bitmap(BM_TGT, W, H, px); }
    { const int W = 11, H = 26; std::vector<uint32_t> px((size_t)W * H, 0); for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { int dx = abs(x - 5); if ((y < 8 && dx <= y * 5 / 7) || (y >= 8 && dx <= 1)) px[(size_t)y * W + x] = 0xFF2020E8u; } add_bitmap(BM_ARROW, W, H, px); }
    { std::vector<uint32_t> px(2 * 8, 0xFFF0F0F0u); add_bitmap(BM_TICK, 2, 8, px); }
    { std::vector<uint32_t> px(2 * 20, 0xFFF0F0F0u); add_bitmap(BM_TICK2, 2, 20, px); }
    { const int W = 44, H = 44; std::vector<uint32_t> px((size_t)W * H, 0); auto set = [&](int x, int y, uint32_t c) { px[(size_t)y * W + x] = c; }; const uint32_t WH = 0xFFF0F0F0u, BK = 0xFF101010u;
      for (int y = 16; y < 28; y++) { set(15, y, WH); set(28, y, WH); } for (int k = 0; k < 3; k++) { set(15 + k, 16, WH); set(15 + k, 27, WH); set(28 - k, 16, WH); set(28 - k, 27, WH); }     /* [ ] */
      for (int k = 0; k < 6; k++) { set(21, k, BK); set(21, 43 - k, BK); set(k, 21, BK); set(43 - k, 21, BK); } for (int k = 19; k < 24; k++) { set(k, 2, BK); set(k, 41, BK); }   /* the black ticks round it */
      add_bitmap(BM_SIGHT, W, H, px); }
    for (int v = 0; v < 2; v++) { const int S = 11; std::vector<uint32_t> px((size_t)S * S, 0); for (int y = 0; y < S; y++) for (int x = 0; x < S; x++) { float d = sqrtf((x - 5.f) * (x - 5.f) + (y - 5.f) * (y - 5.f)); if (d < 5.5f && d > 4.1f) px[(size_t)y * S + x] = v ? 0xFFE04030u : 0xFF2020E8u; } add_bitmap(v ? BM_RING_B : BM_RING_R, S, S, px); }
    make_still();
}
static void num(VPIX_Frame &f, int base, int n, float cx, float y, float sc) { char b[16]; snprintf(b, sizeof b, "%d", n); float w = strlen(b) * 7.f * sc, x = cx - w * 0.5f; for (const char *c = b; *c; c++) { blit(f, base + (*c - '0'), 6, 10, x, y, sc); x += 7.f * sc; } }
static void clock_text(VPIX_Frame &f, float secs, float x, float y) { int s = (int)ceilf(secs); if (s < 0) s = 0; blit(f, BM_W + (s / 60) % 10, 6, 10, x, y, 1.f); blit(f, BM_W + 10, 4, 10, x + 8, y, 1.f); blit(f, BM_W + (s % 60) / 10, 6, 10, x + 14, y, 1.f); blit(f, BM_W + (s % 60) % 10, 6, 10, x + 23, y, 1.f); }
static void gold(VPIX_Frame &f, const char *s, float cx, float y, float sc) { float w = strlen(s) * 11.4f * sc, x = cx - w * 0.5f; for (const char *c = s; *c; c++) { if (*c >= 'A' && *c <= 'Z') blit(f, BM_LET + (*c - 'A'), d1k::FW, d1k::FH, x, y, sc); x += 11.4f * sc; } }

/* ---- the game ---- */
struct Tank { V p; float yaw = 0, turret = 0, speed = 0; int hp = 5, spheres = 0, points = 0, id = 0; bool alive = true; float dead_t = 0, cool = 0, invuln = 0, flash = 0; float wander = 0, burst = 0; int goal = 0; };
struct Tripod { V p; int stage = 0; float t = 0; int by = -1, hits = 0; bool cyan = false; };   /* stage 0 stands, 1 shot down: legs tumble while the sphere waits then flies, 2 empty, waits */
struct Shot { V p, v; int owner; int aim; float life; bool locked; };
struct Burst { V p; float t, size; };
enum State { S_TITLE, S_DEMO, S_PLAY, S_OVER };
static State st = S_DEMO; static float st_t = 0, time_left = 0, lock_lost = 99.f; static double sim_t = 0, intro_t = 0;
static std::vector<Tank> tk; static std::vector<Tripod> tri; static std::vector<Shot> ms; static std::vector<Burst> bu; static int lock_idx = -1; static float lock_range = 0;
static int my = 0;   /* linked slave: the tank this pod drives (the master is always 0) */
static void sfx(int ch, uint32_t key, int vol, int pan = 0) { buggy::play(ch, (int)key, vol, pan); }
static const float T_STAND = 0.7f, T_BALL = 1.15f, T_FLY = 0.85f, T_LEGS = 1.5f, T_BACK = 40.f;
static void place(Tank &t, bool first) {
    static const float X[4] = {2400, -20000, 17900, 30000}, D[4] = {-2000, 44000, 44000, 90000};
    t.p = V(X[t.id & 3], CAM_Y + D[t.id & 3], 0); if (!first) { t.p.x += rnd(-20000, 20000); t.p.y += rnd(-20000, 20000); }
    t.p.z = ground_at(t.p.x, t.p.y); static const float Y[4] = {-0.06f, -1.5708f, 1.5708f, 3.1f}; t.yaw = Y[t.id & 3]; t.turret = 0; t.speed = 0; t.hp = tank_hp; t.alive = true; t.invuln = 2.f;
}
static void new_round(bool demo) {
    tk.clear(); tri.clear(); ms.clear(); bu.clear();
    const bool lnk = v1knet::on && v1knet::master && !demo; const int nc = lnk ? 3 : n_cpu;   /* linked: 4 tanks, the CPU ones fill the seats nobody takes */
    my = 0; if (demo) v1knet::end_round();
    for (int i = 0; i < nc + 1; i++) { Tank t; t.id = i; place(t, true); t.goal = i; tk.push_back(t); }
    static const float TP[6][2] = {{0, 21000}, {-700, 52000}, {24000, 40000}, {-95000, 70000}, {140000, 60000}, {-110000, 190000}};
    for (int i = 0; i < 6; i++) { Tripod q; q.p = V(TP[i][0], CAM_Y + TP[i][1], 0); q.p.z = ground_at(q.p.x, q.p.y); q.cyan = i == 1; tri.push_back(q); }
    if (lnk) v1knet::begin_round((int)tk.size());
    time_left = game_len; st = demo ? S_DEMO : S_PLAY; st_t = 0; intro_t = 0; lock_idx = -1; lock_lost = 99.f; if (!demo) sfx(7, SND_GO, 90);
    dl("t=%.1f new %s, %d tanks", sim_t, demo ? "demo" : "round", (int)tk.size());
}
static M hull_frame(const Tank &t) {   /* the tank lies on the slope */
    const float d = 1800.f; V f = fwd_of(t.yaw), r(f.y, -f.x, 0);
    float zf = ground_at(t.p.x + f.x * d, t.p.y + f.y * d), zb = ground_at(t.p.x - f.x * d, t.p.y - f.y * d), zr = ground_at(t.p.x + r.x * d, t.p.y + r.y * d), zl = ground_at(t.p.x - r.x * d, t.p.y - r.y * d);
    V F = norm(V(f.x * 2 * d, f.y * 2 * d, zf - zb)), R0 = norm(V(r.x * 2 * d, r.y * 2 * d, zr - zl)), U = norm(cross(R0, F)); M m; m.r[0] = norm(cross(F, U)); m.r[1] = F; m.r[2] = U; return m;
}
static void add_burst(V p, float size) { Burst b; b.p = p; b.t = 0; b.size = size; bu.push_back(b); }
static void hit_tank(Tank &t, const Shot &m) {
    if (t.invuln > 0) return; t.hp--; t.flash = 0.2f; if (t.id == 0 && st == S_PLAY) { sfx(2, SND_HURT, 110); VPIX_Haptic(2, 0.2f, 0.8f); }
    if (m.owner >= 0 && m.owner < (int)tk.size()) tk[m.owner].points += m.locked ? 2 : 1;
    if (t.hp <= 0) { t.alive = false; t.dead_t = 0; add_burst(t.p + V(0, 0, 1500), 3200.f); sfx(3, SND_BOOM, 110); }
}
static void fire(Tank &t, V dir, int aim, bool locked) { if (t.cool > 0 || !t.alive) return; t.cool = FIRE_GAP; Shot m; m.p = t.p + V(0, 0, 1650) + dir * 3200.f; m.v = dir * SHOT_SPEED; m.owner = t.id; m.aim = aim; m.life = 5.f; m.locked = locked; ms.push_back(m); if (t.id == 0) sfx(1, SND_FIRE, 90); }
static void drive(Tank &t, float thr, float steer, float dt) {
    t.yaw = wrap(t.yaw + steer * turn_rate * dt);
    float tgt = thr * drive_speed; t.speed += clampf(tgt - t.speed, -3000.f * dt, 3000.f * dt);
    V np = t.p + fwd_of(t.yaw) * (t.speed * dt); float r = sqrtf(np.x * np.x + np.y * np.y); if (r > 175000.f) { np.x *= 175000.f / r; np.y *= 175000.f / r; t.speed *= 0.5f; }
    np.z = ground_at(np.x, np.y); t.p = np;
}
static void step_tripods(float dt) {
    for (Tripod &q : tri) { q.t += dt;
        if (q.stage == 1 && q.t > T_BALL + T_FLY) { q.stage = 2; q.t = 0; if (q.by >= 0 && q.by < (int)tk.size()) { tk[q.by].points += 25; tk[q.by].spheres++; if (q.by == 0) sfx(5, SND_TAKE, 110); dl("t=%.1f tank %d takes a sphere", sim_t, q.by); } }
        else if (q.stage == 2 && q.t > T_BACK) { q.stage = 0; q.t = 0; q.by = -1; q.hits = 0; } }
}
static int nearest_tripod(const V &p, float maxd) { int best = -1; float bd = maxd; for (size_t i = 0; i < tri.size(); i++) if (tri[i].stage == 0) { float d = len(tri[i].p - p); if (d < bd) { bd = d; best = (int)i; } } return best; }
static V tri_aim(const Tripod &q) { return q.p + V(0, 0, 3500.f); }
static void step_cpu(Tank &t, float dt) {
    if (t.cool > 0) t.cool -= dt; t.burst -= dt; if (t.burst < -3.f) t.burst = 2.f;   /* they fire in streams */
    Tank *foe = nullptr; float fd = 42000.f; for (Tank &o : tk) { if (o.id == t.id || o.id == 0 || !o.alive) continue; float l = len(o.p - t.p); if (l < fd) { fd = l; foe = &o; } }
    if (st == S_DEMO && !foe) { for (Tank &o : tk) if (o.id != t.id && o.alive && len(o.p - t.p) < 42000.f) foe = &o; }
    if (st == S_DEMO && foe && intro_t < CHASE_SECS) { V to = foe->p - t.p; float err = wrap(atan2f(-to.x, to.y) - t.yaw); drive(t, 0, clampf(err, -1, 1), dt); if (t.burst > 0 && fabsf(err) < 0.3f && intro_t > 6.f) { V dir = norm(to + V(0, 0, 400) + V(rnd(-1, 1), rnd(-1, 1), 0) * (4200.f * cpu_skill)); fire(t, dir, -1, false); } return; }   /* the clip's two rivals stand and trade fire */
    int best = st == S_DEMO ? -1 : nearest_tripod(t.p, 300000.f);
    V goal = best >= 0 ? tri[best].p : V(sinf(t.wander) * 120000.f, cosf(t.wander) * 120000.f, 0); t.wander += dt * 0.1f * (t.id + 1);
    V d = goal - t.p; float err = wrap(atan2f(-d.x, d.y) - t.yaw); drive(t, 0.8f, clampf(err * 2.f, -1.f, 1.f), dt);
    if (best >= 0 && len(d) < 30000.f && fabsf(err) < 0.25f) { fire(t, norm(tri_aim(tri[best]) - t.p - V(0, 0, 1650)), -1, false); return; }
    for (Tank &o : tk) { if (o.id == t.id || !o.alive || st == S_DEMO) continue; V to = o.p - t.p; float l = len(to); if (l > 42000.f) continue; float a = wrap(atan2f(-to.x, to.y) - t.yaw); if (fabsf(a) < 0.5f && t.burst > 0) { fire(t, norm(to + V(0, 0, 400) + V(rnd(-1, 1), rnd(-1, 1), 0) * (3000.f * cpu_skill)), -1, false); break; } }
}
/* ---- link play (vpix_1k_net.h), part 1: what a pod sends, who is a person, the lock, a person's seat. Pod 1 runs everything; another pod drives its own tank and draws the master's state ---- */
#pragma pack(push, 1)
struct NetIn { float thr, steer, yaw, aim[3]; uint8_t fire, pad; };
struct NetT { float p[3], yaw, turret, speed, invuln; int8_t hp; int16_t points; uint8_t spheres, flags; };
struct NetTr { float p[3], t; int8_t by; uint8_t stage, hits, cyan; };
struct NetB { float p[3], t, size; };
struct NetS { float p[3], v[3]; int8_t owner; uint8_t locked; };
struct NetSt { uint8_t st, n, nshots, nb; float st_t, time_left; NetT t[4]; NetTr tr[6]; NetB b[10]; NetS sh[30]; };
#pragma pack(pop)
static bool human(int id) { return id == 0 || (v1knet::on && v1knet::master && id > 0 && id < v1knet::SEATS && v1knet::seat_pod[id]); }
/* the tank in the sight: nearest the aim line inside LOCK_DEG, never the one looking (the clip's yellow box) */
static void lock_find(int self, V eye, V aim, int &idx, float &range) {
    idx = -1; float best = cosf(LOCK_DEG * 3.14159f / 180.f);
    for (size_t i = 0; i < tk.size(); i++) { if ((int)i == self || !tk[i].alive) continue; V to = tk[i].p + V(0, 0, 1000) - eye; float l = len(to); if (l > LOCK_MAX || l < LOCK_MIN) continue; float c = dot(to * (1.f / l), aim); if (c > best) { best = c; idx = (int)i; range = l; } }
}
/* master: a person's tank, as its pod drives it (throttle, steer, gun direction, trigger) */
static void seat_step(Tank &t, const NetIn &p, float dt) {
    if (t.cool > 0) t.cool -= dt;
    if (!t.alive) { t.dead_t += dt; if (t.dead_t > respawn_delay) place(t, false); return; }
    drive(t, clampf(p.thr, -0.5f, 1.f), clampf(p.steer, -1.f, 1.f), dt); t.turret = wrap(p.yaw - t.yaw);
    V aim(p.aim[0], p.aim[1], p.aim[2]); if (!(len(aim) > 0.5f) || len(aim) > 1.5f) aim = fwd_of(p.yaw); else aim = norm(aim);
    int li; float lr = 0; lock_find(t.id, t.p + V(0, 0, EYE_Z), aim, li, lr);
    if (p.fire) { V dir = aim; if (li >= 0) dir = norm(tk[li].p + V(0, 0, 900) - t.p - V(0, 0, 1650)); fire(t, dir, li, li >= 0); }
}
static void step_world(const VPIX_Input &in, bool trig, float dt) {
    if (st != S_PLAY && st != S_DEMO) return;
    const bool ai = autoplay || st == S_DEMO;
    time_left -= dt; intro_t += dt;
    Tank &me = tk[0];
    if (me.cool > 0) me.cool -= dt;
    float thr = in.joy[1], steer = -in.joy[0];   /* W and stick forward are +1 */
    if (in.vr) { thr = fabsf(in.joy_l[1]) > 0.12f ? in.joy_l[1] : in.brake; steer = -in.joy_r[0]; }   /* Bobby 11-03, VR: dual thumbsticks: right steers, left stick or left trigger throttle; the gun follows the head */
    /* the demo's pilot: first the tripod ahead, then the nearest rival */
    int aim_t = -1, aim_k = -1; { float bd = 34000.f; for (size_t i = 1; i < tk.size(); i++) { if (!tk[i].alive) continue; float l = len(tk[i].p - me.p); if (l < bd) { bd = l; aim_k = (int)i; } } int q = nearest_tripod(me.p, 30000.f); if (st == S_DEMO && intro_t < 20.f && q != 0) q = -1;   /* the clip takes one sphere in its 16 s */
        if (q >= 0 && (intro_t < CHASE_SECS || aim_k < 0)) { aim_t = q; aim_k = -1; } }
    if (ai) { thr = 1.f; steer = intro_t > CHASE_SECS ? clampf(me.turret * 1.2f, -1.f, 1.f) : 0.f;   /* the hull comes round to the gun, as the clip's arrow does */
        if (intro_t > CHASE_SECS + 12.f && aim_k < 0) { int q = nearest_tripod(me.p, 400000.f); if (q >= 0) { V d = tri[q].p - me.p; steer = clampf(wrap(atan2f(-d.x, d.y) - me.yaw) * 2.f, -1.f, 1.f); } } }
    if (me.alive) drive(me, clampf(thr, -0.5f, 1.f), clampf(steer, -1.f, 1.f), dt);
    else { me.dead_t += dt; if (me.dead_t > respawn_delay) place(me, false); }
    /* the turret follows the head; with no head it swings to what is nearest */
    V aim; float ty;
    if (in.head.valid && !ai) { M cr = yawed(me.yaw); V hd = tw(cr, V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2])); ty = atan2f(-hd.x, hd.y); aim = norm(hd); me.turret = wrap(ty - me.yaw); }
    else { float want = 0; if (aim_k >= 0) { V to = tk[aim_k].p - me.p; want = wrap(atan2f(-to.x, to.y) - me.yaw); } else if (aim_t >= 0) { V to = tri[aim_t].p - me.p; want = wrap(atan2f(-to.x, to.y) - me.yaw); }
        me.turret += clampf(wrap(want - me.turret), -0.6f * dt, 0.6f * dt); ty = me.yaw + me.turret; aim = fwd_of(ty); }
    /* the yellow box: a tank close to the sight, inside the distances the clip's box lived at */
    int was = lock_idx; lock_find(0, me.p + V(0, 0, EYE_Z), aim, lock_idx, lock_range);
    if (was >= 0 && lock_idx < 0) lock_lost = 0; lock_lost += dt;
    bool want_fire = trig && !ai;
    if (ai) { if (aim_t >= 0) { V to = tri[aim_t].p - me.p; want_fire = intro_t > 2.4f && tri[aim_t].hits < 3 && fabsf(wrap(me.turret - wrap(atan2f(-to.x, to.y) - me.yaw))) < 0.06f; } else want_fire = lock_idx >= 0 || (aim_k >= 0 && fmodf((float)intro_t, 2.f) < 1.2f); }
    if (want_fire && me.alive) { V dir = aim; if (lock_idx >= 0) dir = norm(tk[lock_idx].p + V(0, 0, 900) - me.p - V(0, 0, 1650)); else if (ai && aim_t >= 0) dir = norm(tri_aim(tri[aim_t]) - me.p - V(0, 0, 1650)); fire(me, dir, lock_idx, lock_idx >= 0); }
    for (size_t i = 1; i < tk.size(); i++) { Tank &t = tk[i]; const NetIn *pi = v1knet::on && v1knet::master ? (const NetIn *)v1knet::seat_input((int)i, sizeof(NetIn)) : NULL;
        if (pi) seat_step(t, *pi, dt); else if (t.alive) step_cpu(t, dt); else { t.dead_t += dt; if (t.dead_t > respawn_delay) place(t, false); } }   /* linked: a person's seat is driven by its pod */
    for (Tank &t : tk) { if (t.invuln > 0) t.invuln -= dt; if (t.flash > 0) t.flash -= dt; }
    step_tripods(dt);
    for (size_t i = 0; i < ms.size();) { Shot &m = ms[i]; m.life -= dt;
        if (m.aim >= 0 && m.aim < (int)tk.size() && tk[m.aim].alive) { V want = norm(tk[m.aim].p + V(0, 0, 900) - m.p); m.v = norm(m.v * (1.f / SHOT_SPEED) + want * (2.2f * dt)) * SHOT_SPEED; }
        m.p += m.v * dt; bool dead = m.life <= 0;
        { float gz = ground_at(m.p.x, m.p.y) + 500.f; if (m.p.z < gz) { if (gz > 16000.f) { dead = true; add_burst(m.p, 1100.f); } else if (human(m.owner) && !m.locked && m.life < 4.6f) { dead = true; add_burst(m.p, 1900.f); } else m.p.z = gz; } }   /* they hug the rolling land and die on the mountains */
        for (Tank &t : tk) if (!dead && t.alive && t.id != m.owner && len(t.p + V(0, 0, 800) - m.p) < HIT_R) { dead = true; add_burst(m.p, 1900.f); hit_tank(t, m); if (t.id == 0 || m.owner == 0) sfx(3, SND_BOOM, 70); }
        for (Tripod &q : tri) if (!dead && q.stage == 0) { V o = m.p - q.p; if (sqrtf(o.x * o.x + o.y * o.y) < 2300.f && o.z < TRI_H + BALL_R) { dead = true; q.hits++;   /* two bursts low on the legs, the third high: the clip's picture */
                const V at[3] = {V(-1400, -200, 2800), V(1400, -200, 2800), V(0, -300, 5400)}; add_burst(q.p + at[(q.hits - 1) % 3], 1400.f); sfx(3, SND_BOOM, m.owner == 0 ? 80 : 40);
                if (q.hits >= 3) { q.stage = 1; q.t = 0; q.by = m.owner; dl("t=%.1f tripod down by %d", sim_t, m.owner); } } }
        if (dead) { ms[i] = ms.back(); ms.pop_back(); } else i++; }
    for (size_t i = 0; i < bu.size();) { bu[i].t += dt; if (bu[i].t > 0.65f) { bu[i] = bu.back(); bu.pop_back(); } else i++; }
    if (st == S_DEMO && intro_t > DEMO_SECS) { st = S_TITLE; st_t = 0; return; }
    if (st == S_PLAY && time_left <= 0) { st = S_OVER; st_t = 0; dl("t=%.1f round over: %d points, %d spheres", sim_t, me.points, me.spheres); }
}

/* ---- drawing ---- */
static M compose(const M &parent, const M &local) { M o; for (int k = 0; k < 3; k++) o.r[k] = tw(parent, local.r[k]); return o; }
static void put_world(VPIX_Frame &f, bool third_me) {
    M I; dput(f, M_RIM, I, V(0, 0, 0)).bright = 1; dput(f, M_LAND, I, V(0, 0, 0)).bright = 1;
    for (const Tripod &q : tri) { V top = q.p + V(0, 0, TRI_H + BALL_R * 0.85f); const int bm = q.cyan ? M_CYAN : M_BALL;
        if (q.stage == 0) { dput(f, M_TRIPOD, I, q.p).bright = 1; dput(f, bm, I, top).bright = 1; }
        if (q.stage == 1) { float u = q.t - T_STAND;
            if (u < 0) dput(f, M_TRIPOD, I, q.p).bright = 1;
            else if (u < T_LEGS) { float k = clampf(u / 0.9f, 0.f, 1.f); V c = q.p + V(0, 0, 4100.f - 3000.f * k);    /* the legs cross at their middles, spin and lie down */
                for (int l = 0; l < 3; l++) { float az = 6.2831853f * l / 3 + 1.5708f + u * 2.2f, tilt = (0.3f + 1.2f * k) * (l == 2 ? 0.15f : 1.f); V d(cosf(az) * sinf(tilt), sinf(az) * sinf(tilt), cosf(tilt)), a = norm(cross(d, V(0.31f, 0.2f, 0.93f))), b = cross(d, a); M R; R.r[0] = a; R.r[1] = b; R.r[2] = d; float s = 1.f - clampf((u - 1.1f) / 0.4f, 0.f, 0.7f); VPIX_Instance &i = dput(f, M_LEG, R, c); i.bright = 1; i.S[2] = s; } }
            if (q.t < T_BALL) dput(f, bm, I, top).bright = 1;
            else if (q.by >= 0 && q.by < (int)tk.size()) { float w = clampf((q.t - T_BALL) / T_FLY, 0.f, 1.f); V to = tk[q.by].p + V(0, 0, 1400); dput(f, bm, I, top + (to - top) * w).bright = 1; } } }
    for (const Tank &t : tk) { if (!t.alive) continue; if (t.id == my && !third_me) continue; if (t.invuln > 0 && fmodf((float)sim_t, 0.3f) < 0.1f && t.id != 0) continue; M h = hull_frame(t); dput(f, M_TANK + (t.id & 3), h, t.p).bright = 1; dput(f, M_TURRET, compose(h, yawed(t.turret)), t.p).bright = 1; }
    V eye(f.cam_T[0], f.cam_T[1], f.cam_T[2]);
    for (const Shot &m : ms) { float d = len(m.p - eye); if (d > 28000.f) { M R = look_at(m.p, m.p + m.v); dput(f, M_DASH, R, m.p).bright = 1; } else dput(f, M_SHOT, M(), m.p).bright = 1; }
    for (const Burst &b : bu) { float u = b.t; int id = u < 0.22f ? M_BURST_A : u < 0.32f ? M_BURST_B : u < 0.48f ? M_BURST_C : M_BURST_D; float sc = u < 0.08f ? 0.3f : u < 0.22f ? 0.55f : u < 0.32f ? 0.8f : u < 0.48f ? 1.f : 1.1f;
        VPIX_Instance &i = dput(f, id, yawed(b.t * 3.f), b.p); i.bright = 1; float s = sc * b.size / 1000.f; for (int k = 0; k < 3; k++) i.S[k] = s; }
}
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = 0x01; f.clear[1] = 0x03; f.clear[2] = 0x48; f.top_clear = true;   /* the navy sky */
    f.n_lights = 1; f.lights[0] = {0.72f, 0.28f, {-0.3f, -0.4f, 0.87f}};
    const float CX = 176.f, SX = 173.f, SY = 143.f; M cam; V eye; static const char *fixed = getenv("SPHERE2K_CAM");
    bool cockpit = false; const bool demo = st == S_DEMO;
    auto set_cam = [&]() { for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; } f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z; };
    if (st == S_TITLE) { eye = V(0, 0, 0); set_cam(); }
    else if (fixed && *fixed) { float a[6] = {0, -80000, 15000, 0, 0, 3000}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5])); set_cam(); put_world(f, true); }
    else if (demo && intro_t < CHASE_SECS) {   /* the clip opens on a camera that stays put while your tank drives away from it */
        eye = V(0, CAM_Y, ground_at(0, CAM_Y + 21000.f) + 7650.f); cam.r[1] = norm(V(0, 1.f, -0.2f)); cam.r[0] = V(1, 0, 0); cam.r[2] = cross(cam.r[0], cam.r[1]); set_cam(); put_world(f, true);
    } else if (!tk.empty()) {
        const Tank &me = tk[my]; M hull = hull_frame(me); cockpit = true; eye = me.p + tw(hull, V(0, 0, EYE_Z));
        const bool head = in.head.valid && !autoplay && !demo;
        cam = head ? hull : compose(hull, yawed(me.turret));
        if (demo) { float pi = clampf((float)intro_t - CHASE_SECS, 0.f, 8.f) * 0.45f * 3.14159f / 180.f; V fw = cam.r[1], up = cam.r[2]; cam.r[1] = fw * cosf(pi) + up * sinf(pi); cam.r[2] = up * cosf(pi) - fw * sinf(pi); }   /* the demo's view creeps upward, as the clip's does */
        static const char *lk = getenv("SPHERE2K_LOOK"); if (lk && *lk) { float ya = 0, pi = 0; sscanf(lk, "%f,%f", &ya, &pi); ya *= 3.14159f / 180.f; pi *= 3.14159f / 180.f;
            V fw = cam.r[1], ri = cam.r[0], up = cam.r[2]; V f2 = fw * cosf(ya) + ri * sinf(ya), r2 = ri * cosf(ya) - fw * sinf(ya); V f3 = f2 * cosf(pi) + up * sinf(pi), u3 = up * cosf(pi) - f2 * sinf(pi); cam.r[0] = r2; cam.r[1] = f3; cam.r[2] = u3; }
        set_cam(); put_world(f, false);
        if (me.alive) { { VPIX_Instance &hv = dput(f, M_TANK + (my & 3), hull, me.p); hv.on_top = 2; hv.bright = 1; } VPIX_Instance &g = dput(f, M_GUNVIEW, compose(hull, yawed(me.turret)), eye); g.on_top = 2; g.bright = 1; }
    }
    f.in_head = false;
    if (st != S_TITLE && !tk.empty()) { const Tank &me = tk[my];
        clock_text(f, time_left, 55, 40); blit(f, BM_BOX, 46, 18, 97, 36, 1.f); num(f, BM_R, tk[0].points, 120, 40, 1.f);
        num(f, BM_Y, tk.size() > 2 ? tk[2].points : 0, 174, 40, 1.f); num(f, BM_B, tk.size() > 1 ? tk[1].points : 0, 229, 40, 1.f); clock_text(f, time_left, 270, 40);
        if (cockpit) {   /* the strip: 4.4 picture units a degree, its 0 rides under the red arrow at the hull's bearing */
            const float PX = 4.4f; float rel = -me.turret * 180.f / 3.14159f, ax = SX + rel * PX;
            for (int d = -180; d <= 180; d += 2) { float x = ax + d * PX; if (x < 0 || x > 351) continue; if (d % 10 == 0) { blit(f, BM_TICK2, 2, 20, x, 242, 1.f); if (d != 0) num(f, BM_W, abs(d), x, 232, 0.7f); } else blit(f, BM_TICK, 2, 8, x, 250, 1.f); }
            blit(f, BM_ARROW, 11, 26, clampf(ax, 6.f, 346.f) - 7.7f, 227, 1.4f);
            if (me.alive) { blit(f, BM_SIGHT, 44, 44, SX - 22, SY - 22, 1.f);
                if (lock_idx >= 0) { blit(f, BM_TGT, 30, 22, SX - 15, SY - 11, 1.f); num(f, BM_W, (int)(lock_range / 1000.f), SX + 24, SY + 4, 0.9f); }
                else blit(f, lock_lost < 1.2f ? BM_RING_R : BM_RING_B, 11, 11, SX - 5.5f, SY - 8, 1.f); }
        }
        if (st == S_OVER) d1k::text(f, "GAME OVER", CX, 96, 1.2f, 1);
    }
    if (v1knet::on && st == S_TITLE) {
        if (!v1knet::master) d1k::text(f, !v1knet::master_heard() ? "LOOKING FOR PLAYER 1" : v1knet::nseats > 0 ? "YOU JOIN NEXT ROUND" : "PLAYER 1 STARTS THE GAME", CX, 200, 0.8f);
        else if (v1knet::live_pods() > 0) { char b[32]; snprintf(b, sizeof(b), "LINKED %d PLAYERS", v1knet::live_pods() + 1); d1k::text(f, b, CX, 200, 0.8f); }
    }
    if (st == S_TITLE) blit(f, BM_STILL, 352, 288, 0, 0, 1.f);
    if ((st == S_TITLE || st == S_DEMO) && fmodf((float)sim_t, 1.f) < 0.5f) gold(f, "INSERT CREDIT", SX, 76, 1.f);
    d1k::hs::draw(f, "sphere2k", st == S_TITLE, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* Shapes read off the WMP copy's spectrum (2.85-3.85 s; 5.46-9.9 and 10.5-11.5 s; the low band throughout). Synthesised, not sampled. */
static void make_sounds() {
    std::mt19937 r(5); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(SND_FIRE, 0.16f, [&](float t) { float e = exp(-t * 16.f); return (sinf(6.2831853f * 1500.f * t) + sinf(6.2831853f * 1750.f * t) * 0.8f + sinf(6.2831853f * 2100.f * t) * 0.5f) * (0.5f + 0.5f * noise()) * e * 0.45f + noise() * e * 0.12f; });
    { float ph = 0, ph2 = 0; make_sound(SND_TAKE, 1.0f, [&](float t) { float v = fabsf(t - 0.5f) * 2.f, fq = 1900.f + 1100.f * v; ph += fq / 22050.f; ph2 += fq * 0.42f / 22050.f; return (sinf(6.2831853f * ph) * 0.55f + sinf(6.2831853f * ph2) * 0.35f) * sinf(3.14159f * t); }); }
    make_sound(SND_BOOM, 0.7f, [&](float t) { return noise() * exp(-t * 4.5f) * 0.8f + sinf(6.2831853f * 60.f * t) * exp(-t * 3.f) * 0.5f; });   /* INVENTED */
    make_sound(SND_HURT, 0.4f, [&](float t) { return (fmodf(t, 0.08f) < 0.04f ? 0.6f : -0.6f) * exp(-t * 5.f); });   /* INVENTED */
    make_sound(SND_GO, 0.6f, [&](float t) { return sinf(6.2831853f * (t < 0.3f ? 660.f : 880.f) * t) * 0.5f; });   /* INVENTED */
    make_sound(SND_ENG, 1.f, [&](float t) { return (sinf(6.2831853f * 110.f * t) * 0.5f + sinf(6.2831853f * 170.f * t) * 0.35f + sinf(6.2831853f * 240.f * t) * 0.45f) * 0.5f; }); sounds[SND_ENG].loop = true;   /* the low hum: whole cycles, loops clean */
}

/* ---- link play, part 2: snapshots (pod 1 -> the others, 30 a second), a slave's own pilot, the log. A start-together match: 4 tanks, seats nobody takes stay CPU tanks ---- */
static bool slave_game = false; static double acc = 0, frame_acc = 0; static unsigned long frames = 0;
static void net_log() {
    static double next = 0; if (sim_t < next) return; next = sim_t + 5.0;
    if (tk.size() < 4 || (st != S_PLAY && st != S_OVER)) { dl("t=%.1f link: %s, title, pods heard %d", sim_t, v1knet::master ? "master" : "slave", v1knet::live_pods()); return; }
    dl("t=%.1f link: %s seat %d st %d left %.0f | t0 %s(%.0f %.0f) %d pts | t1 %s(%.0f %.0f) %d | t2 %s(%.0f %.0f) %d | t3 %s(%.0f %.0f) %d", sim_t, v1knet::master ? "master" : "slave", my, (int)st, time_left,
       v1knet::seat_pod[0] ? "POD" : "cpu", tk[0].p.x, tk[0].p.y, tk[0].points, v1knet::seat_pod[1] ? "POD" : "cpu", tk[1].p.x, tk[1].p.y, tk[1].points,
       v1knet::seat_pod[2] ? "POD" : "cpu", tk[2].p.x, tk[2].p.y, tk[2].points, v1knet::seat_pod[3] ? "POD" : "cpu", tk[3].p.x, tk[3].p.y, tk[3].points);
}
static void net_master() {
    if (!v1knet::due(30)) return;
    if ((st != S_PLAY && st != S_OVER) || tk.size() < 2) { v1knet::send_snapshot(NULL, 0); return; }
    static NetSt s; memset(&s, 0, sizeof(s)); s.st = (uint8_t)st; s.n = (uint8_t)(std::min)(tk.size(), (size_t)4); s.st_t = st_t; s.time_left = time_left;
    for (int i = 0; i < s.n; i++) { const Tank &q = tk[i]; NetT &n = s.t[i]; n.p[0] = q.p.x; n.p[1] = q.p.y; n.p[2] = q.p.z; n.yaw = q.yaw; n.turret = q.turret; n.speed = q.speed; n.invuln = q.invuln;
        n.hp = (int8_t)q.hp; n.points = (int16_t)q.points; n.spheres = (uint8_t)q.spheres; n.flags = q.alive ? 1 : 0; }
    for (size_t k = 0; k < tri.size() && k < 6; k++) { const Tripod &q = tri[k]; NetTr &n = s.tr[k]; n.p[0] = q.p.x; n.p[1] = q.p.y; n.p[2] = q.p.z; n.t = q.t; n.by = (int8_t)q.by; n.stage = (uint8_t)q.stage; n.hits = (uint8_t)q.hits; n.cyan = q.cyan ? 1 : 0; }
    for (size_t k = 0; k < bu.size() && s.nb < 10; k++) { NetB &n = s.b[s.nb++]; n.p[0] = bu[k].p.x; n.p[1] = bu[k].p.y; n.p[2] = bu[k].p.z; n.t = bu[k].t; n.size = bu[k].size; }
    for (size_t k = 0; k < ms.size() && s.nshots < 30; k++) { NetS &n = s.sh[s.nshots++]; n.p[0] = ms[k].p.x; n.p[1] = ms[k].p.y; n.p[2] = ms[k].p.z; n.v[0] = ms[k].v.x; n.v[1] = ms[k].v.y; n.v[2] = ms[k].v.z; n.owner = (int8_t)ms[k].owner; n.locked = ms[k].locked ? 1 : 0; }
    v1knet::send_snapshot(&s, offsetof(NetSt, sh) + s.nshots * sizeof(NetS));
}
static void net_leave(const char *why) { slave_game = false; st = S_TITLE; st_t = 0; my = 0; tk.clear(); ms.clear(); bu.clear(); tri.clear(); lock_idx = -1; dl("t=%.1f link: %s, back to the title", sim_t, why); }
static void net_apply(const NetSt &s, uint8_t round) {
    static uint8_t my_round = 0; const bool fresh = !slave_game || round != my_round || (int)tk.size() != s.n; my_round = round;
    my = v1knet::my_seat < s.n ? v1knet::my_seat : 0;
    if (fresh) { tk.assign(s.n, Tank()); for (int i = 0; i < s.n; i++) tk[i].id = i; ms.clear(); bu.clear(); tri.assign(6, Tripod()); slave_game = true; lock_idx = -1; lock_lost = 99.f; sfx(7, SND_GO, 90); dl("t=%.1f link: in the master's round, seat %d of %d", sim_t, my, (int)s.n); }
    for (int i = 0; i < s.n; i++) { Tank &q = tk[i]; const NetT &n = s.t[i]; const bool alive = (n.flags & 1) != 0;
        if (!fresh) {
            if (i == my && n.hp < q.hp && alive) { sfx(2, SND_HURT, 110); VPIX_Haptic(2, 0.2f, 0.8f); q.flash = 0.2f; }
            if (q.alive && !alive) { add_burst(q.p + V(0, 0, 1500), 3200.f); sfx(3, SND_BOOM, i == my ? 110 : 70); }
            if (i == my && n.points >= q.points + 25) sfx(5, SND_TAKE, 110); }
        q.p = V(n.p[0], n.p[1], n.p[2]); q.yaw = n.yaw; if (i != my || fresh) q.turret = n.turret; q.speed = n.speed; q.invuln = n.invuln; q.hp = n.hp; q.points = n.points; q.spheres = n.spheres; q.alive = alive; }
    for (int k = 0; k < 6; k++) { Tripod &q = tri[k]; const NetTr &n = s.tr[k]; q.p = V(n.p[0], n.p[1], n.p[2]); q.t = n.t; q.by = n.by; q.stage = n.stage; q.hits = n.hits; q.cyan = n.cyan != 0; }
    bu.resize(s.nb); for (int k = 0; k < s.nb; k++) { bu[k].p = V(s.b[k].p[0], s.b[k].p[1], s.b[k].p[2]); bu[k].t = s.b[k].t; bu[k].size = s.b[k].size; }
    ms.resize(s.nshots); for (int k = 0; k < s.nshots; k++) { ms[k].p = V(s.sh[k].p[0], s.sh[k].p[1], s.sh[k].p[2]); ms[k].v = V(s.sh[k].v[0], s.sh[k].v[1], s.sh[k].v[2]); ms[k].owner = s.sh[k].owner; ms[k].locked = s.sh[k].locked != 0; ms[k].aim = -1; ms[k].life = 5.f; }
    st = (State)s.st; st_t = s.st_t; time_left = s.time_left;
}
/* a slave's pilot: its controls go to pod 1 as NetIn; its gun and view are its own, the world between snapshots is carried on */
static NetIn slave_pilot(const VPIX_Input &in, bool trig, float dt) {
    NetIn o; memset(&o, 0, sizeof(o)); Tank &me = tk[my];
    float thr = in.joy[1], steer = -in.joy[0]; if (in.vr) { thr = fabsf(in.joy_l[1]) > 0.12f ? in.joy_l[1] : in.brake; steer = -in.joy_r[0]; }
    float want = me.turret; bool have = false;
    { float bd = 34000.f; for (size_t i = 0; i < tk.size(); i++) { if ((int)i == my || !tk[i].alive) continue; float l = len(tk[i].p - me.p); if (l < bd) { bd = l; V to = tk[i].p - me.p; want = wrap(atan2f(-to.x, to.y) - me.yaw); have = true; } }
      if (!have) { int q = nearest_tripod(me.p, 30000.f); if (q >= 0) { V to = tri[q].p - me.p; want = wrap(atan2f(-to.x, to.y) - me.yaw); have = true; } } }
    if (autoplay) { thr = 1.f; steer = clampf(me.turret * 1.2f, -1.f, 1.f); }
    V aim; float ty;
    if (in.head.valid && !autoplay) { M cr = yawed(me.yaw); V hd = tw(cr, V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2])); ty = atan2f(-hd.x, hd.y); aim = norm(hd); me.turret = wrap(ty - me.yaw); }
    else { me.turret += clampf(wrap(want - me.turret), -0.6f * dt, 0.6f * dt); ty = me.yaw + me.turret; aim = fwd_of(ty); }
    int was = lock_idx; lock_find(my, me.p + V(0, 0, EYE_Z), aim, lock_idx, lock_range); if (was >= 0 && lock_idx < 0) lock_lost = 0; lock_lost += dt;
    bool fire_on = autoplay ? (lock_idx >= 0 || (have && fabsf(wrap(me.turret - want)) < 0.06f)) : trig;
    if (me.cool > 0) me.cool -= dt; if (fire_on && me.alive && me.cool <= 0) { me.cool = FIRE_GAP; sfx(1, SND_FIRE, 90); }
    o.thr = me.alive ? thr : 0.f; o.steer = steer; o.yaw = ty; o.aim[0] = aim.x; o.aim[1] = aim.y; o.aim[2] = aim.z; o.fire = fire_on ? 1 : 0; return o;
}
static void net_slave_step(float dt) {
    for (Tank &q : tk) { if (q.invuln > 0) q.invuln -= dt; if (q.flash > 0) q.flash -= dt; if (q.alive) { q.p += fwd_of(q.yaw) * (q.speed * dt); q.p.z = ground_at(q.p.x, q.p.y); } }
    for (Shot &m : ms) m.p += m.v * dt;
    for (Burst &b : bu) b.t += dt;
    for (Tripod &q : tri) q.t += dt;
    time_left -= dt;
}
/* true = this tick was a linked slave's, inside the master's game */
static bool net_tick(double el, const VPIX_Input &in, bool trig) {
    v1knet::pump();
    if (v1knet::master) { net_master(); net_log(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n >= offsetof(NetSt, t) + sizeof(NetT) * 2 && v1knet::my_seat >= 0) { static NetSt s; memset(&s, 0, sizeof(s)); memcpy(&s, d, (std::min)(n, sizeof(s))); if (s.n >= 2 && s.n <= 4 && s.nb <= 10 && s.nshots <= 30 && (s.st == S_PLAY || s.st == S_OVER)) net_apply(s, v1knet::round_id); }
        else if (slave_game) net_leave("the master's round is over");
    }
    if (slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
    net_log();
    if (!slave_game) { v1knet::send_input(NULL, 0); return false; }
    { const bool hum = sound_on && (st == S_PLAY); if (hum && !voices[8].on) buggy::play(8, (int)SND_ENG, 40); else if (!hum && voices[8].on) voices[8].on = false; }
    const float dt = 1.f / 120.f; int k = 0;
    NetIn o; memset(&o, 0, sizeof(o));
    while (acc >= dt && k < 24) { if (st == S_PLAY) o = slave_pilot(in, trig, dt); net_slave_step(dt); acc -= dt; sim_t += dt; st_t += dt; k++; }
    if (st == S_PLAY) { o.thr = clampf(o.thr, -1.f, 1.f); v1knet::send_input(&o, sizeof(o)); } else v1knet::send_input(NULL, 0);
    d1k::hs::run("sphere2k", st == S_OVER && st_t > 2.f, tk[my].points, in, trig, (float)el, autoplay);
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    return true;
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at game_fps ---- */
static auto t_last = std::chrono::steady_clock::now();
static double next_log = 2;
static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    static bool trig_was = false;
    bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.start, sel = trig && !trig_was; trig_was = trig;
    if (autoplay && st != S_PLAY && st_t > 2.f) sel = true;
    if (st == S_OVER && st_t > 15.f) sel = true; if (d1k::hs::run("sphere2k", st == S_OVER && st_t > 2.f, tk.empty() ? 0 : tk[0].points, in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (!v1knet::can_start()) sel = false;   /* a linked slave: pod 1 starts the game */
    if (v1knet::on && net_tick(el, in, trig)) { PIC_AddEvent(tick, 2.0); return; }
    if ((st == S_TITLE || st == S_DEMO) && sel) new_round(false);
    else if (st == S_OVER && sel && st_t > 3.f) { st = S_TITLE; st_t = 0; v1knet::end_round(); tk.clear(); ms.clear(); bu.clear(); tri.clear(); }
    if (st == S_TITLE && st_t > STILL_SECS) new_round(true);
    { const bool hum = sound_on && (st == S_PLAY); if (hum && !voices[8].on) buggy::play(8, (int)SND_ENG, 40); else if (!hum && voices[8].on) voices[8].on = false; }
    int n = 0;
    while (acc >= dt && n < 24) { step_world(in, trig, dt); acc -= dt; sim_t += dt; st_t += dt; n++; }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    if (sim_t >= next_log) { next_log = sim_t + 2.0; if (!tk.empty()) dl("t=%.0f st %d me (%.0f %.0f %.0f) yaw %.2f tur %.2f hp %d alive %d spheres %d points %d y %d b %d lock %d left %.0f", sim_t, (int)st, tk[0].p.x, tk[0].p.y, tk[0].p.z, tk[0].yaw, tk[0].turret, tk[0].hp, tk[0].alive ? 1 : 0, tk[0].spheres, tk[0].points, tk.size() > 2 ? tk[2].points : 0, tk.size() > 1 ? tk[1].points : 0, lock_idx, time_left); }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".sphere2k").c_str(), "w");
    game_len = envf("SPHERE2K_TIME", game_len); game_fps = envf("SPHERE2K_FPS", game_fps); drive_speed = envf("SPHERE2K_SPEED", drive_speed); turn_rate = envf("SPHERE2K_TURN", turn_rate);
    tank_hp = (int)envf("SPHERE2K_HP", (float)tank_hp); respawn_delay = envf("SPHERE2K_RESPAWN", respawn_delay); cpu_skill = envf("SPHERE2K_SKILL", cpu_skill); n_cpu = (int)envf("SPHERE2K_CPUS", (float)n_cpu); autoplay = envf("SPHERE2K_AUTO", 0) != 0;
    if (game_fps < 1.f) game_fps = 1.f; n_cpu = n_cpu < 0 ? 0 : n_cpu > 3 ? 3 : n_cpu;
    v1knet::init("SP2K", false, dl);   /* start-together match (Bobby: join-in-progress is for Flying Aces and VTOL) */
    d1k::make_hud(); make_hud_sphere(); make_heights(); build_land(); build_tripod();
    build_tank(M_TANK, 0xFF0903); build_tank(M_TANK2, 0x2030E0); build_tank(M_TANK3, 0xE8D820); build_tank(M_TANK4, 0x30C040); build_turret(); build_views(); make_sounds();
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    new_round(true);   /* the attract: the demo, then the still */
    PIC_AddEvent(tick, 2.0);
    dl("Sphere rebuild started%s", autoplay ? ", auto" : "");
}
}   /* namespace sphere2k */
