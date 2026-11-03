/* Total Destruction (Virtuality 1000SD, 1991) - native rebuild from footage and period press. A stock-car RACE with contact: 8 cars, the result is your finishing position.
   No 1000-series software survives in public, so every model is built here and no game file is read. Included after vpix_dactyl1k.h, so d1k:: and buggy::
   are reachable. Dev only, not in a release. Full evidence: Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\totald.md.
   FROM FOOTAGE (Octavius King, 2YrPW5cU06A t1153-t1211, the only footage known; a 4:3 picture stretched to 16:9): title picture and sky strip cut from it (vpix_1k_totald_pics.h);
   the select screen: left map fixed with the white start mark, right map turning, car pairs tan+red, pink+white, dark+green each about 1.8 s, dots filling bottom row then top at about 11 a second with a yellow dot leading (down-pointing on top, up-pointing below);
   the in-car view solved from frames (red bonnet plate, grey dash, black frame, pale panels, red block, octagonal 3-spoke wheel, A pillar, red sill; horizon at 34% of the height);
   the grid: you last, tan right ahead, pink ahead on the right, gantry with 4 lights turning green from the left (t1161.7: one green); NO readout on the grid, gold n/8 after it, stuck at 8/8 for this slow player;
   the car: long flat deck, black rear panel, paint-coloured windscreen frame with no roof, a red dome inside, pale door panel, big tyres with pale hubs (t1159.9, t1195, t1198, t1205.5, t1210.5);
   flat black tyres lying in rows (t1171, t1210) that fly up high when hit (t1172.2-t1172.7); orange boxes across the road and a stack (t1165, t1176); red-white-red drums staggered, not a wall (t1186);
   a long low dark red block by the road (t1165.4, t1168.5, t1205.9), taken as the jump's side; pale blue rail seen from the grid (t1161); black-and-white kerbs; open grass; cars white, yellow, tan, pink, red, grey, green, blue.
   FROM PERIOD PRESS (New Computer Express 147 31 Aug 1991, ACE 49 and The One 37 Oct 1991, Your Commodore 83 Sep 1991; local copies Library\Virtuality Preservation\Total_Destruction\pages):
   8 cars (you, up to 3 humans, 4 computer); about 5 minutes; grid and lights; position at the end is the result, crossing the line is not; both maps as described: "figure of eight" with a jump in the centre = right map, "flattened triple figure-eight" = left map (3 crossings);
   flat crossings, drive anywhere including the grass and the wrong way; bash a car enough and it blows up in a fountain of panels, panels litter the track; your view goes out to a bird's eye, a pause, the car reassembles, the view sweeps in through the rear window;
   the running commentary tells positions and shouts the game's name at a wreck (voice: Dr Al Humrich); wheel, 2 pedals, forward/reverse lever on the left; computer cars aggressive and hard to beat.
   SOURCES DISAGREE: NCE's fences round the circuit and giant overhead arrows are not in 45 s of driving, and ACE crosses the grass: left out, footage wins on looks. "Derby" (Maplin, Compute) is the label of people who were not there; eyewitnesses say race.
   INFERRED, not stated: the readout is position/cars (8 cars, you start last, the footage's player never gets anywhere); the left map is the one the footage drives (it alone has the mark).
   INVENTED: the jump's exact shape and size (wedge with the crossing road running under it, sheer lip) and which crossing has it on the left map; speeds, handling, damage numbers, wreck phase timings, light timing after the first green, AI, where on the lap the tyres/boxes/drums sit,
   the end screen, how a choice of map is made (footage shows no choosing), all sounds; commentary wording bar "Total Destruction", and its voice is Windows SAPI, a stand-in. CD music unknown, none played. The 3 human slots are driven by the computer until network play exists.
   The high score is position first: (9 - place) * 1000, plus laps run as an invented tie-break.
   Colours are from Octavius alone; its capture dulls green, so grass is lifted slightly. Dev: TOTALD_LOOK=yawdeg,pitchdeg.
   Units: mm, seconds. World X right, Y forward, Z up. The camera is the player's head (in_head), so VR late correction applies.
   Env: TOTALD_FPS, TOTALD_TIME, TOTALD_RESPAWN (whole wreck sequence, s), TOTALD_VMAX, TOTALD_ACCEL, TOTALD_DAMAGE, TOTALD_BUMP, TOTALD_COURSE (0 right map, 1 left), TOTALD_CPU (other cars, 1-7), TOTALD_SKILL, TOTALD_ROAD,
        TOTALD_AUTO, TOTALD_CAM, TOTALD_WRECKME=<s> (dev: blow you up s into the race), TOTALD_START=<idx> (dev: put your car there). */
#include "vpix_1k_totald_pics.h"
#include "vpix_1k_net.h"         /* link play, 2-4 pods: see the "link play" block below */
namespace totald {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at; using buggy::to_world;
using d1k::Mesh; using d1k::dput; using d1k::blit; using d1k::text; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound;
using d1k::yawed; using d1k::pitched; using d1k::add_bitmap; using d1k::make_digit;

/* ---- tuning ---- */
static float game_fps = 20.f;                              /* measured on the footage elsewhere: 20 at best */
static float vmax = 24000.f;                               /* INVENTED: mm/s top speed (86 km/h); the footage shows slow bumps */
static float accel_max = 7500.f;                           /* INVENTED: mm/s^2 */
static float game_len = 300.f;                             /* about five minutes for 2 pounds (NCE 147); RePlay 1993 gives 3.5 min for the later US sites */
static float respawn_delay = 6.f;                          /* INVENTED: the whole wreck sequence, s; only "a moment's pause" is stated, and it must cost places */
static float dmg_scale = 1.f;                              /* INVENTED: damage points per m/s of impact over 2 = 1.6 * this, wreck at 100 ("bash an opponent enough times") */
static float bump_e = 0.35f;                               /* INVENTED: bounce, low because the hits looked like polite little bumps */
static int   n_cpu = 7;                                    /* 8 cars: 4 computer + 3 human slots (driven by the computer here) + you */
static float cpu_skill = 1.f;                              /* INVENTED */
static float road_half = 0.f;                              /* INVENTED: half road width, mm; 0 = per course default */
static int   course_sel = 1;                               /* 0 right map (figure-eight), 1 left map (triple figure-eight, marked and driven in the footage) */
static bool  autoplay = false;
static const float CAR_R = 950.f, CAR_M = 1200.f;          /* INVENTED: two collision circles per car */
static const V SEAT(380.f, 150.f, 1180.f);                 /* INVENTED: driver's eye in the car, right-hand drive */

/* ---- mesh handles (d1k::MH + id; d1k uses ids below 60) ---- */
enum { T_SKY = 1000, T_GROUND, T_TRACK, /* +1 for course 1 */ T_DASH = 1010, T_WHEEL, T_BODY, T_CAR = 1020, /* +paint */ T_CARX = 1030, /* +paint, scorched */
       T_TYRE = 1040, T_BOX, T_STACK, T_BARR, T_SPARK_O, T_SPARK_Y, T_SMOKE, T_GANTRY, T_MAPQ, T_MAPR, T_MAPM, T_PANEL = 1060 /* +paint */, T_RAMP = 1070 /* +1 for course 1 */, T_LAMP_R = 1072, T_LAMP_G };
static const uint32_t TEX_SKY = 0x7FFE2000u, TEX_MAP = 0x7FFE2001u;

/* ---- the courses ---- */
static const int NP = 180, GRID = 5;
struct Track { V c[NP], t[NP], n[NP]; float half = 5500.f; };
static Track trk[2];
static int course = 0;
/* the two courses, traced from the select screen (Octavius t1158), in that picture's lines; the one under the car on the screen is joined up by eye */
static const float TRK0[][2] = {{400,75},{440,80},{460,100},{476,125},{470,150},{460,185},{452,202},{440,230},{436,250},{445,272},{460,274},{480,262},{510,248},{530,235},{528,220},{510,210},{480,203},{452,199},{420,192},{380,180},{340,175},{310,168},{300,155},{303,140},{305,120},{315,95},{325,70},{345,62},{370,68}};
static const float TRK1[][2] = {{165,151},{140,151},{90,151},{40,151},{20,165},{25,180},{45,190},{90,190},{115,190},{135,200},{140,212},{125,224},{105,222},{90,212},{88,195},{87,151},{87,130},{95,115},{120,111},{160,111},{178,120},{180,140},{178,180},{175,225},{185,242},{205,245},{225,235},{228,215},{228,175},{225,160},{205,150},{185,150}};
static void t_trace(int k, std::vector<V> &o) {   /* Catmull-Rom through the traced points, closed, fine steps */
    const float (*pt)[2] = k == 0 ? TRK0 : TRK1; const int n = k == 0 ? (int)(sizeof TRK0 / sizeof TRK0[0]) : (int)(sizeof TRK1 / sizeof TRK1[0]); o.clear();
    for (int i = 0; i < n; i++) { const float *a = pt[(i + n - 1) % n], *b = pt[i], *c = pt[(i + 1) % n], *d = pt[(i + 2) % n];
        for (int q = 0; q < 12; q++) { float u = q / 12.f, u2 = u * u, u3 = u2 * u; o.push_back(V(0.5f * (2 * b[0] + (c[0] - a[0]) * u + (2 * a[0] - 5 * b[0] + 4 * c[0] - d[0]) * u2 + (3 * b[0] - a[0] - 3 * c[0] + d[0]) * u3), 0.5f * (2 * b[1] + (c[1] - a[1]) * u + (2 * a[1] - 5 * b[1] + 4 * c[1] - d[1]) * u2 + (3 * b[1] - a[1] - 3 * c[1] + d[1]) * u3), 0)); } }
}
static void t_make_track(int k) {
    Track &T = trk[k]; std::vector<V> f; t_trace(k, f);
    const float S = 1100.f, cx = k == 0 ? 415.f : 125.f, cy = k == 0 ? 168.f : 178.f;   /* mm per line of the picture; x is taken back to 4:3 */
    std::vector<float> L(f.size() + 1, 0.f); for (size_t i = 0; i < f.size(); i++) { V a = f[i], b = f[(i + 1) % f.size()]; L[i + 1] = L[i] + hypotf((b.x - a.x) * .75f, b.y - a.y); }
    for (int i = 0; i < NP; i++) { float want = L.back() * i / NP; size_t q = 0; while (q + 1 < f.size() && L[q + 1] < want) q++; float u = (want - L[q]) / (std::max)(0.001f, L[q + 1] - L[q]); V a = f[q], b = f[(q + 1) % f.size()];
        T.c[i] = V((a.x + (b.x - a.x) * u - cx) * .75f * S, -(a.y + (b.y - a.y) * u - cy) * S, 0); }
    for (int i = 0; i < NP; i++) {
        V d = T.c[(i + 1) % NP] - T.c[(i + NP - 1) % NP]; d.z = 0; T.t[i] = norm(d); T.n[i] = V(-T.t[i].y, T.t[i].x, 0);
    }
    T.half = road_half > 0 ? road_half : 6000.f;
}
/* the jump: a wedge up the road, a sheer lip past the crossing, the crossing road running under it. Its size and form are INVENTED; "precipitous jump bridge in the centre" (NCE), "flying ramp leap" (ACE), a long low dark red block by the road (t1165.4, t1205.9) */
struct Ramp { V c, t, n; float U0 = 30000.f, U1 = 8500.f, Lip = 10000.f, H = 4500.f, hw = 6800.f; };
static Ramp ramps[2];
static float r_g(const Ramp &R, float u) { return R.H * (u + R.U0) / (R.U0 + R.Lip); }
static void t_make_ramp(int k) {
    const Track &T = trk[k]; Ramp &R = ramps[k]; const float S = 1100.f, cx = k == 0 ? 415.f : 125.f, cy = k == 0 ? 168.f : 178.f;
    const float px = k == 0 ? 452.f : 88.f, py = k == 0 ? 200.f : 190.f; V at((px - cx) * .75f * S, -(py - cy) * S, 0), want = k == 0 ? norm(V(-.25f, -1.f, 0)) : V(1.f, 0, 0);   /* the crossing on the picture, the way the strand runs */
    int best = 0; float bd = 1e30f; for (int i = 0; i < NP; i++) { if (dot(T.t[i], want) < 0.8f) continue; V d = T.c[i] - at; float l = d.x * d.x + d.y * d.y; if (l < bd) { bd = l; best = i; } }
    R.c = T.c[best]; R.t = T.t[best]; R.n = T.n[best]; R.hw = T.half + 800.f;
    auto P = [&](float u, float v, float z) { V p = R.c + R.t * u + R.n * v; p.z = z; return p; };
    Mesh m; const uint32_t RD = 0x7A2A20, RD2 = 0x5E2018, TOP = 0x8C847A; float a = -R.U0, b = -R.U1, c = R.U1, d = R.Lip, hw = R.hw, ga = 30.f, gb = r_g(R, b), gc = r_g(R, c), gd = R.H, lip = 500.f;
    m.poly({P(a, -hw, ga), P(d, -hw, gd), P(d, hw, gd), P(a, hw, ga)}, TOP);
    for (int s = 0; s < 2; s++) { float v = s ? hw : -hw;
        m.poly({P(a, v, 0), P(b, v, 0), P(b, v, gb), P(a, v, ga)}, RD);
        m.poly({P(b, v, gb - lip), P(c, v, gc - lip), P(c, v, gc), P(b, v, gb)}, RD);
        m.poly({P(c, v, 0), P(d, v, 0), P(d, v, gd), P(c, v, gc)}, RD); }
    m.poly({P(b, -hw, 0), P(b, hw, 0), P(b, hw, gb), P(b, -hw, gb)}, RD2); m.poly({P(c, -hw, 0), P(c, hw, 0), P(c, hw, gc), P(c, -hw, gc)}, RD2); m.poly({P(d, -hw, 0), P(d, hw, 0), P(d, hw, gd), P(d, -hw, gd)}, RD2);
    m.poly({P(b, -hw, gb - lip), P(c, -hw, gc - lip), P(c, hw, gc - lip), P(b, hw, gb - lip)}, RD2);
    for (int s = 0; s < 2; s++) { float v = s ? hw : -hw; for (int i = 0; i <= 8; i++) { float u = a + (d - a) * i / 8; d1k::kit_bar(m, P(u, v, r_g(R, u)), P(u, v, r_g(R, u) + 1300.f), 50, 50, 0x8898B0); } d1k::kit_bar(m, P(a, v, ga + 1300.f), P(d, v, gd + 1300.f), 50, 50, 0x8898B0); }   /* pale blue rails: the rail seen ahead-left from the grid (t1161), taken as the jump's; that is a guess */
    dl("jump on course %d: idx %d at (%.0f %.0f) along (%.2f %.2f)", k, best, R.c.x, R.c.y, R.t.x, R.t.y);
    m.add(T_RAMP + k);
}
/* the ground under a point for something at height z; wall = it ran into the side of the jump */
static float t_ground(V p, float z, bool &wall) {
    const Ramp &R = ramps[course]; wall = false; V d = p - R.c; float u = d.x * R.t.x + d.y * R.t.y, v = d.x * R.n.x + d.y * R.n.y;
    if (fabsf(v) > R.hw || u < -R.U0 || u > R.Lip) return 0.f;
    float g = r_g(R, u); if (u > -R.U1 && u < R.U1 && z < g - 500.f) return 0.f;   /* the road underneath */
    if (g - z > 500.f) { wall = true; return 0.f; }
    return g;
}
static uint32_t t_dark(uint32_t c, float k) { return ((uint32_t)(((c >> 16) & 255) * k) << 16) | ((uint32_t)(((c >> 8) & 255) * k) << 8) | (uint32_t)((c & 255) * k); }

static void t_build_track_mesh(int k) {
    const Track &T = trk[k]; Mesh m; const float h = T.half, KW = 800.f;
    for (int i = 0; i < NP; i++) {
        int j = (i + 1) % NP; float z = (float)(i * 8 / NP) * 3.f;   /* each eighth sits a little higher so the crossings do not flicker */
        V c0 = T.c[i], c1 = T.c[j], n0 = T.n[i], n1 = T.n[j];
        V L0 = c0 + n0 * h, R0 = c0 - n0 * h, L1 = c1 + n1 * h, R1 = c1 - n1 * h;
        V Lo0 = c0 + n0 * (h + KW), Ro0 = c0 - n0 * (h + KW), Lo1 = c1 + n1 * (h + KW), Ro1 = c1 - n1 * (h + KW);
        L0.z = R0.z = L1.z = R1.z = z;
        m.poly({R0, R1, L1, L0}, (i & 1) ? 0x847C72 : 0x80786E);
        uint32_t kc = (i & 1) ? 0x161616 : 0xE6E6DC; z += 12.f;   /* black and white, not red and white (t1185) */
        L0.z = L1.z = R0.z = R1.z = Lo0.z = Lo1.z = Ro0.z = Ro1.z = z;
        m.poly({L0, L1, Lo1, Lo0}, kc); m.poly({Ro0, Ro1, R1, R0}, kc);
    }
    m.add(T_TRACK + k);
}

/* ---- meshes ---- */
static void t_build_meshes() {
    /* the backdrop: footage's photo strip of sky and hills on a cylinder moved with the eye; 60 degrees a repeat, each second one mirrored so it joins. Bottom of the strip = the horizon, top about 18 degrees up */
    { std::vector<uint32_t> px(TD_SW * TD_SH); for (int i = 0; i < TD_SW * TD_SH; i++) px[i] = TD_SKY_PAL[TD_SKY[i]]; VPIX_Render_AddTextureRGBA(TEX_SKY, TD_SW, TD_SH, px.data());
      Mesh m; const float R = 280000.f, zt = 0.329f * R, zb = -1500.f; for (int i = 0; i < 12; i++) { float a0 = 6.2831853f * i / 12, a1 = 6.2831853f * (i + 1) / 12; float u0 = (i % 4 == 0) ? .012f : (i % 4 == 2) ? .988f : .5f, u1 = ((i + 1) % 4 == 0) ? .012f : ((i + 1) % 4 == 2) ? .988f : .5f;
          VPIX_MeshPoly q; q.flags = 0x20FFFFFF; q.tex = TEX_SKY; q.n = 4; V cn[4] = {V(cosf(a0) * R, sinf(a0) * R, zb), V(cosf(a1) * R, sinf(a1) * R, zb), V(cosf(a1) * R, sinf(a1) * R, zt), V(cosf(a0) * R, sinf(a0) * R, zt)}; const float uu[4] = {u0, u1, u1, u0}, vv[4] = {1, 1, 0, 0};
          for (int c = 0; c < 4; c++) { q.idx[c] = m.vert(cn[c]); q.uv[c][0] = uu[c]; q.uv[c][1] = vv[c]; } q.nx = -cosf(a0); q.ny = -sinf(a0); q.nz = 0; m.p.push_back(q); }
      m.add(T_SKY); }
    { Mesh m; const float G = 290000.f; m.poly({V(-G, -G, -40), V(G, -G, -40), V(G, G, -40), V(-G, G, -40)}, 0x2B4126); m.add(T_GROUND); }
    t_make_track(0); t_make_track(1); t_build_track_mesh(0); t_build_track_mesh(1); t_make_ramp(0); t_make_ramp(1);
    /* the dashboard, solved from t1163-t1185 and t1150+10 (looking down), level directions (x tan, v tan) from the eye (380, 150, 1180): the red plate's far edge -.204, near -.378, its right corner at (.449, -.234);
       black frame from -.543, pale panels from -.60 and -.668 */
    { Mesh m; const uint32_t RD = 0xA84430, RD2 = 0x7A2A1A, GR = 0x7E7A72, GRD = 0x6E6A64, BK = 0x121214, PALE = 0xC8C4BC, FL = 0x2E2A26;
      d1k::kit_slab(m, {V(-1060, 950, 878), V(1060, 950, 878), V(1035, 1049, 878), V(959, 1440, 878), V(300, 1560, 878), V(-300, 1560, 878), V(-959, 1440, 878), V(-1035, 1049, 878)}, V(0, 0, 20), RD, RD2);
      m.poly({V(-1060, 800, 862), V(1060, 800, 862), V(1060, 950, 878), V(-1060, 950, 878)}, GR);
      m.box(V(-1060, 790, 480), V(1060, 800, 862), GRD); m.box(V(-1060, 790, 0), V(1060, 800, 480), FL);
      m.box(V(120, 784, 480), V(870, 790, 827), BK);
      m.poly({V(195, 780, 540), V(705, 780, 540), V(705, 780, 760), V(195, 780, 790)}, PALE);
      d1k::kit_eye = SEAT; d1k::kit_quad(m, V(-200, 788, 470), V(180, 0, 0), V(0, 0, 370), 0x8A2C20);
      m.box(V(-980, 150, 1650), V(980, 640, 1750), 0x505050);
      m.add(T_DASH); }
    /* the rest of the car round the driver (Bobby 2026-11-02). INVENTED: footage only looks forward */
    { Mesh m; const uint32_t RD = 0xC02820, IN_ = 0x6A6A6A, DK = 0x505050, ST = 0x2E2E34, YC = 0xD8C020;
      m.box(V(-130, 100, 150), V(130, 760, 400), ST); m.box(V(-18, 420, 400), V(18, 460, 680), 0x18181C); m.box(V(-40, 400, 680), V(40, 480, 750), 0xE0E0E0); m.box(V(-75, 120, 400), V(-45, 340, 500), 0x18181C);   /* console, gear lever, handbrake */
      m.box(V(-90, 540, 1600), V(180, 570, 1660), 0x18181C); m.box(V(-85, 538, 1605), V(175, 540, 1655), 0x9CC0D8);   /* mirror */
      m.box(V(250, 900, 150), V(330, 1090, 200), 0x8A8A8A); m.box(V(440, 900, 150), V(520, 1090, 200), 0x8A8A8A);   /* pedals */
      m.box(V(-950, -2300, 100), V(950, 800, 150), 0x303034);                                          /* floor */
      m.box(V(-980, -1300, 1700), V(980, 150, 1750), DK);                                              /* roof */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; auto bx = [&](float x0, float y0, float z0, float x1, float y1, float z1, uint32_t t, uint32_t s) { m.box(V((std::min)(q * x0, q * x1), y0, z0), V((std::max)(q * x0, q * x1), y1, z1), t, s, s); };
          bx(930, -1300, 150, 980, 800, 950, RD, IN_);                                                 /* door */
          bx(880, -500, 950, 980, -380, 1700, IN_, IN_); bx(880, -1300, 950, 980, -1100, 1700, IN_, IN_);  /* pillars */
          float sx = q * 380.f; m.box(V(sx - 250, -270, 300), V(sx + 250, -190, 1000), ST); m.box(V(sx - 130, -270, 1000), V(sx + 130, -200, 1140), ST); m.box(V(sx - 250, -190, 150), V(sx + 250, 400, 320), ST);   /* front seats */
          d1k::kit_quad(m, V(sx - 180, -188, 330), V(70, 0, 0), V(0, 0, 660), 0xC02820); d1k::kit_quad(m, V(sx + 110, -188, 330), V(70, 0, 0), V(0, 0, 660), 0xC02820);   /* race belts */
          /* door card: armrest, handle, winder, speaker */
          d1k::kit_quad(m, V(q * 926, -1250, 200), V(0, 2000, 0), V(0, 0, 700), 0x585860); bx(860, -300, 600, 926, 500, 660, ST, ST);
          bx(875, 350, 720, 930, 480, 750, 0xC0C0C8, 0xC0C0C8); bx(880, -80, 690, 930, -40, 800, 0xC0C0C8, 0xC0C0C8);
          d1k::kit_dial(m, V(q * 900, 150, 360), V(0, 1, 0), V(0, 0, 1), V(-q, 0, 0), 120, 0x303034, 0x18181C, 0x303034);
          /* roll cage */
          bx(840, -1000, 150, 880, -960, 1680, YC, YC); bx(840, -1000, 1660, 880, -300, 1690, YC, YC);   /* behind the seats: looking right the footage shows no cage ahead */
          d1k::kit_bar(m, V(q * 860, -980, 1635), V(-q * 860, -980, 1635), 20, 15, YC);
          d1k::kit_bar(m, V(q * 860, -980, 1630), V(-q * 860, -980, 220), 20, 20, YC);
          m.poly({V(q * 1060, 950, 878), V(q * 1035, 1049, 878), V(q * 1035, 1049, 450), V(q * 1060, 950, 450)}, 0x7A2A1A); m.poly({V(q * 1035, 1049, 878), V(q * 959, 1440, 878), V(q * 959, 1440, 450), V(q * 1035, 1049, 450)}, 0x7A2A1A);
          /* the A pillar: narrow at the dash, wide at the roof, two greys; the broad red sill beside it (t1195, t1199) */
          { V b0(q * 950, 640, 870), b1(q * 860, 640, 870), t0(q * 960, 280, 1690), t1(q * 790, 280, 1690), f1(0, 60, 0); m.poly({b0, b1, t1, t0}, 0x76726C); m.poly({b1, b1 + f1, t1 + f1, t1}, 0x5A5650); }
          d1k::kit_slab(m, {V(q * 860, 640, 880), V(q * 985, 640, 880), V(q * 985, -100, 880), V(q * 730, -100, 880)}, V(0, 0, 20), RD, 0x7A2A1A); }
      m.box(V(-900, -1250, 150), V(900, -950, 450), ST); m.box(V(-900, -1300, 450), V(900, -1180, 850), ST);   /* back seat */
      m.box(V(-950, -1350, 150), V(950, -1300, 950), DK);                                              /* under the back window */
      d1k::kit_slab(m, {V(-950, -1350, 925), V(950, -1350, 925), V(950, -2500, 875), V(-950, -2500, 875)}, V(0, 0, 25), RD, 0x601410);  /* boot, with its sides and tail */
      m.box(V(-950, -2520, 450), V(950, -2470, 880), 0x8A1C14); for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; d1k::kit_slab(m, {V(q * 930, -1350, 930), V(q * 930, -2500, 880), V(q * 930, -2500, 450), V(q * 930, -1350, 450)}, V(q * 20, 0, 0), 0x8A1C14, 0x601410); }
      m.add(T_BODY); }
    { Mesh m; const int N = 8; const float R0 = 215.f, R1 = 165.f, D = 20.f;   /* the wheel: a thick octagonal ring, three spokes, a big hub (t1160) */
      for (int k = 0; k < N; k++) { float a0 = 6.2831853f * (k + .5f) / N, a1 = 6.2831853f * (k + 1.5f) / N; uint32_t c = (k & 1) ? 0x5E5C58 : 0x54524E;
          V a(cosf(a0) * R0, -D, sinf(a0) * R0), b(cosf(a1) * R0, -D, sinf(a1) * R0), e(cosf(a1) * R1, -D, sinf(a1) * R1), d(cosf(a0) * R1, -D, sinf(a0) * R1);
          m.poly({a, b, e, d}, c); m.poly({V(a.x, D, a.z), V(b.x, D, b.z), b, a}, 0x3E3C38); m.poly({V(d.x, D, d.z), V(e.x, D, e.z), e, d}, 0x3E3C38); }
      for (int k = 0; k < 3; k++) { float a = 1.5707963f + 2.0943951f * k, cx = cosf(a), cz = sinf(a), px = -cz * 34.f, pz = cx * 34.f; V p(cx * 160, 0, cz * 160), q(px, 0, pz);
          m.poly({V(q.x, 0, q.z), V(p.x + q.x, 0, p.z + q.z), V(p.x - q.x, 0, p.z - q.z), V(-q.x, 0, -q.z)}, 0x6A6862); }
      { std::vector<V> h; for (int i = 0; i < 8; i++) { float a = 6.2831853f * i / 8; h.push_back(V(cosf(a) * 55, -6, sinf(a) * 55)); } m.poly({h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]}, 0x62605A); }
      m.add(T_WHEEL); }
    /* the cars (t1159.9, t1194.6-1195, t1198, t1205.5, t1210.5, select screen t1156): long flat deck, sloping bonnet, black rear panel and sill, a paint-coloured windscreen frame with no roof, a red dome inside,
       a pale panel on the door, tyres with pale hubs. Paints white, yellow, tan, pink, red, dark grey, green, blue */
    static const uint32_t PC[8] = {0xE6E6E0, 0xE0D060, 0xE0A878, 0xD050C0, 0xC03020, 0x62625E, 0x40B040, 0x4848B4};
    for (int p = 0; p < 8; p++) for (int sc = 0; sc < 2; sc++) {
        uint32_t col = sc ? t_dark(PC[p], 0.3f) : PC[p], col2 = t_dark(col, 0.78f), pale = sc ? 0x282828 : 0xC8C0A8, dark = 0x121212, redd = sc ? 0x301410 : 0x8C2A20, screen = sc ? 0x202020 : 0x9A9488; Mesh m;
        const float W = 900.f, YR = -2350.f, YC = 700.f, YN = 2250.f, Z0 = 200.f, ZS = 430.f, ZD = 820.f, ZN = 560.f; using d1k::kit_bar;
        m.poly({V(-W, YR, ZD), V(W, YR, ZD), V(W, YC, ZD), V(-W, YC, ZD)}, col);                      /* the deck */
        m.poly({V(-W, YC, ZD), V(W, YC, ZD), V(W, YN, ZN), V(-W, YN, ZN)}, col);                      /* the bonnet */
        m.poly({V(-W, YN, Z0), V(W, YN, Z0), V(W, YN, ZN), V(-W, YN, ZN)}, col2);                     /* the nose */
        m.poly({V(-W, YR, Z0), V(W, YR, Z0), V(W, YR, ZD), V(-W, YR, ZD)}, dark);                     /* the rear panel, black from deck to near the road (t1162, t1198) */
        m.poly({V(-W, YR, Z0), V(W, YR, Z0), V(W, YN, Z0), V(-W, YN, Z0)}, dark);
        m.poly({V(-620, -90, ZD + 4), V(620, -90, ZD + 4), V(620, 700, ZD + 4), V(-620, 700, ZD + 4)}, 0x161616);   /* the open cockpit from above */
        { V a(-790, 1000, ZD + 8), b(790, 1000, ZD + 8), c(650, 330, 1070), d(-650, 330, 1070); m.poly({a, b, c, d}, screen); m.poly({d, c, b, a}, screen); }   /* the screen, a grey band from above (t1156) */
        for (int k = 0; k < 2; k++) { float q = k ? W : -W, sg = k ? 1.f : -1.f;
            m.poly({V(q, YR, ZS), V(q, YN, ZS), V(q, YN, ZN), V(q, YC, ZD), V(q, YR, ZD)}, k ? col2 : col);
            m.poly({V(q, YR, Z0), V(q, YN, Z0), V(q, YN, ZS), V(q, YR, ZS)}, dark);                  /* the black sill */
            m.box(V((std::min)(q, q + sg * 16.f), 470, 500), V((std::max)(q, q + sg * 16.f), 1050, 780), pale);   /* the door panel */
            kit_bar(m, V(sg * 800, 1050, ZD), V(sg * 650, 300, 1090), 38, 38, col);                  /* the frame: forward post, back post */
            kit_bar(m, V(sg * 650, 300, 1090), V(sg * 780, -100, ZD), 38, 38, col);
            /* the tyres: solid drums standing proud of the body, tread faces all round, so they read from behind and from the side (Bobby 11-02); hubs pale as t1159.9 */
            for (int w = 0; w < 2; w++) { const int N = 10; float yc = w ? 1500.f : -750.f, rr = 340.f, zc = 340.f, x0 = q - sg * 340.f, x1 = q + sg * 120.f; std::vector<V> a, b, u;
                for (int i = 0; i < N; i++) { float t = 6.2831853f * i / N; a.push_back(V(x0, yc + cosf(t) * rr, zc + sinf(t) * rr)); b.push_back(V(x1, yc + cosf(t) * rr, zc + sinf(t) * rr)); }
                for (int i = 0; i < 8; i++) { float t = 6.2831853f * i / 8; u.push_back(V(x1 + sg * 6.f, yc + cosf(t) * rr * .55f, zc + sinf(t) * rr * .55f)); }
                for (int i = 0; i < N; i++) { int j = (i + 1) % N; m.poly({a[i], a[j], b[j], b[i]}, (i & 1) ? 0x262626 : 0x363636); }
                for (int i = 1; i + 1 < N; i++) { m.poly({a[0], a[i], a[i + 1]}, 0x101010); m.poly({b[0], b[i], b[i + 1]}, 0x101010); }
                m.poly({u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7]}, sc ? 0x404040 : 0xB8B8B0);
                /* the arch: a flared lip in the paint over the proud part of the tyre, so the tyre top stays under the bodywork (Bobby 11-02) */
                { float xa = q - sg * 2.f, xb = q + sg * 150.f, y0 = yc - 440.f, y1 = yc + 440.f; m.box(V((std::min)(xa, xb), y0, 690), V((std::max)(xa, xb), y1, 730), col, col2, col2); } } }
        kit_bar(m, V(-650, 300, 1090), V(650, 300, 1090), 38, 38, col);   /* below the eye: from behind it only just shows over the deck (t1161) */
        m.octa(V(0, 150, ZD), 190, 400, redd); m.octa(V(0, 150, ZD + 400), 120, 90, redd);       /* the red dome: from behind only its top shows over the bulkhead, above the horizon (t1161, t1198); from the side through the frame (t1194.6); a driver's head? */
        m.box(V(-650, -150, ZD), V(650, -90, 1060), col, col2, col2);                                 /* the bulkhead behind the seat, the raised band seen from behind (t1161) */
        m.add((sc ? T_CARX : T_CAR) + p);
        /* a body panel: solid, 900 x 620 x 70 */
        if (!sc) { Mesh q; q.box(V(-450, -310, -35), V(450, 310, 35), PC[p], t_dark(PC[p], 0.7f), t_dark(PC[p], 0.5f)); q.add(T_PANEL + p); }
    }
    { Mesh m; m.octa(V(0, 0, 0), 340, 150, 0x1A1614); m.octa(V(0, 0, 150), 145, 6, 0x060606); m.add(T_TYRE); }   /* a tyre lying flat, low and black (t1171, t1210); it flies as a black octagon (t1172.5) */
    { const uint32_t A = 0xC89058, B = 0xB07C48, C = 0x8A6038; Mesh m; m.box(V(-450, -450, 0), V(450, 450, 900), A, B, C); m.add(T_BOX);   /* orange (t1176.6) */
      Mesh s; s.box(V(-900, -450, 0), V(0, 450, 900), A, B, C); s.box(V(0, -450, 0), V(900, 450, 900), A, B, C); s.box(V(-450, -450, 900), V(450, 450, 1800), A, B, C); s.add(T_STACK); }
    { Mesh m; m.octa(V(0, 0, 0), 340, 280, 0xD8D8D0); m.octa(V(0, 0, 280), 340, 320, 0x8C2A24); m.octa(V(0, 0, 600), 340, 280, 0xD8D8D0); m.add(T_BARR); }   /* a drum: white, dark red, white (t1186) */

    /* the start gantry: a tall orange braced tower at the road's edge, a beam, a dark sign with red lights, sized and placed from t1160 */
    { Mesh m; const uint32_t O = 0xD8A860, D = 0x281C14; using d1k::kit_bar; const float H = 9800.f, B = 1000.f, T = 620.f, TH = 80.f; auto wd = [&](float z) { return B + (T - B) * z / H; };
      for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) kit_bar(m, V(a ? B : -B, b ? B : -B, 0), V(a ? T : -T, b ? T : -T, H), TH, TH, O);
      for (int k = 0; k < 5; k++) { float z0 = k * 1960.f, z1 = z0 + 1960.f, w0 = wd(z0), w1 = wd(z1); kit_bar(m, V(-w0, -w0, z0), V(w1, -w1, z1), 55, 55, O); kit_bar(m, V(w0, -w0, z0), V(-w1, -w1, z1), 55, 55, O); kit_bar(m, V(-w1, -w1, z1), V(w1, -w1, z1), 55, 55, O); }
      m.box(V(-3200, -110, 9620), V(700, 110, 9900), O); m.box(V(-4700, -60, 8800), V(-2300, 60, 10300), D, D, D);
      m.add(T_GANTRY); }
    { Mesh a, b; a.box(V(-150, -80, -175), V(150, -60, 175), 0xE03020); a.add(T_LAMP_R); b.box(V(-150, -80, -175), V(150, -60, 175), 0x30E040); b.add(T_LAMP_G); }   /* the 4 lights on the sign, set per frame */
    const uint32_t spc[2] = {0xF09020, 0xF0E040}; const int sid[2] = {T_SPARK_O, T_SPARK_Y};
    for (int k = 0; k < 2; k++) { Mesh s; s.poly({V(-60, 0, -40), V(60, 0, -40), V(0, 0, 150)}, spc[k]); s.poly({V(0, -60, -40), V(0, 60, -40), V(0, 0, 150)}, spc[k]); s.add(sid[k]); }
    { Mesh s; V a(-250, 0, 0), b(0, 0, -250), c(250, 0, 0), d(0, 0, 250), e(0, -250, 0), f(0, 250, 0);
      s.poly({a, b, c, d}, 0x606060); s.poly({d, c, b, a}, 0x606060); s.poly({e, b, f, d}, 0x585858); s.poly({d, f, b, e}, 0x585858); s.add(T_SMOKE); }
}

/* ---- sounds: INVENTED placeholders, nothing of the original's audio is known beyond "harsh" collisions ---- */
enum { TSND_ENG = 0x7E000001, TSND_BUMP, TSND_THUD, TSND_BOOM, TSND_BEEP, TSND_GO, TSND_SELECT, TSND_LEVER, TSND_V0 = 0x7E000020 };   /* TSND_V0 + TD_VOICE index: the stand-in commentary */
static void sfx(int ch, uint32_t key, int vol, int pan = 0) { if (!sound_on) return; buggy::play(ch, (int)key, vol, pan); }
static void t_make_sounds() {
    std::mt19937 r(5); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(TSND_ENG, 0.2f, [&](float t) { const float w = 6.2831853f * 100.f * t; return (sinf(w) + 0.5f * sinf(2.f * w) + 0.3f * sinf(3.f * w) + 0.15f * sinf(5.f * w)) * 0.35f; });
    sounds[TSND_ENG].loop = true;   /* 20 whole cycles: loops without a click */
    make_sound(TSND_BUMP, 0.35f, [&](float t) { return (noise() * 0.8f + sinf(6.2831853f * 90.f * t) * 0.5f) * expf(-t * 12.f); });
    make_sound(TSND_THUD, 0.25f, [&](float t) { return sinf(6.2831853f * 60.f * t) * expf(-t * 12.f) * 0.9f; });
    make_sound(TSND_BOOM, 0.8f, [&](float t) { return (noise() * 0.7f + sinf(6.2831853f * 50.f * t) * 0.6f) * expf(-t * 4.5f); });
    make_sound(TSND_BEEP, 0.15f, [&](float t) { return sinf(6.2831853f * 880.f * t) * 0.5f; });
    make_sound(TSND_GO, 0.4f, [&](float t) { return sinf(6.2831853f * 1320.f * t) * 0.5f * expf(-t * 3.f); });
    make_sound(TSND_SELECT, 0.1f, [&](float t) { return sinf(6.2831853f * 660.f * t) * 0.4f; });
    make_sound(TSND_LEVER, 0.06f, [&](float t) { return noise() * expf(-t * 60.f) * 0.5f; });
    for (int i = 0; i < 12; i++) { VSound s; s.rate = 11025; s.loop = false; s.pcm.resize((size_t)TD_VOICE[i].n); for (int q = 0; q < TD_VOICE[i].n; q++) s.pcm[q] = (int16_t)(TD_VOICE[i].d[q] * 200); sounds[TSND_V0 + i] = std::move(s); sound_names[TSND_V0 + i] = "totald_commentary"; }
}

/* ---- HUD bitmaps ---- */
enum { BM_GDIG8 = 258, BM_GSLASH = 260, BM_MAP = 320 };
static void t_make_font_set(int set, uint32_t col) {   /* the same 5 x 7 letters as d1k::make_hud, one more colour (set 3 = green INSERT CREDIT) */
    for (int ch = 0; ch < 26; ch++) { std::vector<uint32_t> px(d1k::FW * d1k::FH, 0);
        for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (d1k::FONT[ch][y * 5 + x] == '1')
            for (int b = 0; b < 2; b++) for (int a = 0; a < 2; a++) px[(y * 2 + b) * d1k::FW + x * 2 + a] = col;
        add_bitmap(d1k::BM_TEXT + set * 40 + ch, d1k::FW, d1k::FH, px); }
}
static void t_make_map() {   /* the track-select screen, as a picture under the 3D cars: both courses, the mark on the selected one's start */
    const int W = 352, H = 288; std::vector<uint32_t> px(W * H);
    { std::mt19937 r(4); for (uint32_t &c : px) { int v = (int)(r() % 7); c = 0xFF000000u | ((uint32_t)(30 + v) << 16) | ((uint32_t)(48 + v) << 8) | (uint32_t)(32 + v); } }
    auto disk = [&](float cx, float cy, float rad, uint32_t col) {
        for (int y = (int)(cy - rad) - 1; y <= (int)(cy + rad) + 1; y++) for (int x = (int)(cx - rad) - 1; x <= (int)(cx + rad) + 1; x++)
            if (x >= 0 && y >= 0 && x < W && y < H && (x - cx) * (x - cx) + (y - cy) * (y - cy) <= rad * rad) px[y * W + x] = col; };
    { std::vector<V> f; t_trace(1, f); for (const V &p : f) disk(p.x * 352.f / 640.f, p.y * 288.f / 360.f, 3.4f, 0xFF66707Cu); }
    VPIX_Render_AddTextureRGBA(TEX_MAP, W, H, px.data());
    { Mesh m; VPIX_MeshPoly q; q.flags = 0x20FFFFFF; q.tex = TEX_MAP; q.n = 4; const float a = 7000.f, b = 5250.f; V cn[4] = {V(-a, -b, 0), V(a, -b, 0), V(a, b, 0), V(-a, b, 0)}; const float uu[4] = {0, 1, 1, 0}, vv[4] = {1, 1, 0, 0}; for (int c = 0; c < 4; c++) { q.idx[c] = m.vert(cn[c]); q.uv[c][0] = uu[c]; q.uv[c][1] = vv[c]; } q.nx = 0; q.ny = 0; q.nz = 1; m.p.push_back(q); m.add(T_MAPQ); }
    /* the figure-8 turns slowly on the screen about its middle, with the cars (t1155-t1158: about 45 degrees a second); the other course stays still */
    { Mesh m, k; std::vector<V> f; t_trace(0, f); const float MX = 14000.f / 640.f, MY = 10500.f / 360.f, HW = 105.f; auto W2 = [&](const V &p) { return V((p.x - 425.f) * MX, -(p.y - 168.f) * MY, 20); };
      const int N = (int)f.size(); for (int i = 0; i < N; i += 2) { V a = W2(f[i]), b = W2(f[(i + 2) % N]), d = norm(b - a), n(-d.y * HW, d.x * HW, 0); m.poly({a - n, a + n, b + n, b - n}, (i * 8 / N) & 1 ? 0x786E64 : 0x7C7268); }
      V a = W2(f[0]), b = W2(f[2]), d = norm(b - a), n(-d.y, d.x, 0); V z(0, 0, 8); m.add(T_MAPR);   /* no mark on this one in the footage */
      k.poly({a - n * 170.f + z * 2.f, a + n * 170.f + z * 2.f, a + n * 170.f + d * 200.f + z * 2.f, a - n * 170.f + d * 200.f + z * 2.f}, 0xF0F0F0); k.add(T_MAPM); }
    /* the marks: a house-shaped red dot, a yellow one that is the last of the row; the white start mark */
    /* top row points down, bottom row up (t1156.5); yellow leads */
    for (int k = 0; k < 2; k++) for (int fl = 0; fl < 2; fl++) { std::vector<uint32_t> d(12 * 12, 0); uint32_t c = k ? 0xFF40C0D0u : 0xFF2838B0u; for (int y = 0; y < 12; y++) for (int x = 0; x < 12; x++) { int w = y < 5 ? y + 2 : 6; if (x >= 6 - w && x < 6 + w) d[(fl ? 11 - y : y) * 12 + x] = c; } add_bitmap(BM_MAP + (fl ? 4 : 0) + k, 12, 12, d); }
    { std::vector<uint32_t> d(4 * 4, 0xFFF0F0F0u); add_bitmap(BM_MAP + 2, 4, 4, d); }
    { std::vector<uint32_t> d(TD_TW * TD_TH); for (int i = 0; i < TD_TW * TD_TH; i++) d[i] = TD_TITLE_PAL[TD_TITLE[i]]; add_bitmap(BM_MAP + 3, TD_TW, TD_TH, d); }
}
static void t_make_hud() {
    d1k::make_hud();
    t_make_font_set(3, 0xFF50D050u);
    for (int d = 1; d <= 8; d++) make_digit(50 + d, d, 0xFF58B4C8u, 0xFF103C5Cu);   /* gold digits (t1173), slightly 3D by their edge */
    std::vector<uint32_t> px(d1k::DW * d1k::DH, 0);
    for (int pass = 0; pass < 2; pass++) { int g = pass ? 0 : 1; uint32_t c = pass ? 0xFF58B4C8u : 0xFF103C5Cu;
        for (int y = 2; y < 24; y++) { int x0 = 12 - (y - 2) * 9 / 22;
            for (int x = x0 - g; x < x0 + 3 + g; x++) if (x >= 0 && x < d1k::DW) px[y * d1k::DW + x] = c; } }
    add_bitmap(260, d1k::DW, d1k::DH, px);
    t_make_map();
}

/* ---- cars, objects, particles ---- */
struct Pan { V p, v, rest, from; float a = 0, s = 0; };   /* one of a wrecked car's panels */
struct Car {
    bool cpu = false, wrecked = false, re = false, gear_rev = false, air = false, aggr = false; int paint = 0, idx = 0, last_hit = -1, cp = 0, rank = 8;
    V p, v; float vz = 0, pitch = 0, yaw = 0, yawrate = 0, wheel = 0, damage = 0, wreck_t = 0, dist = 0, hit_cool = 0, smoke_t = 0, stuck_t = 0, rev_t = 0, ai_off = 0, skill = 1.f, race = 0; Pan pan[12];
};
static std::vector<Car> cars; static int me = 0;   /* linked: your seat, a car number */
enum { O_TYRE, O_BOX, O_STACK, O_BARR };
struct Obj { V p, v; float yaw = 0, rad = 500, mass = 50, vz = 0, tumble = 0, spin = 0; int type = 0; bool fixed = false; };
static std::vector<Obj> objs;
struct Part { V p, v; float life = 1, age = 0, yaw = 0, tilt = 0, spin = 0; int kind = 0, paint = 0; };   /* 0 orange spark, 1 yellow spark, 2 smoke, 3 body panel, which stays where it lands */
static std::vector<Part> parts;
static const char *PAINT_NAME[8] = {"WHITE", "YELLOW", "TAN", "PINK", "RED", "GREY", "GREEN", "BLUE"};
static const int GRID_PAINT[8] = {4, 2, 3, 1, 0, 7, 5, 6};   /* you, then tan right ahead and pink ahead on the right as t1160; your bonnet is red */

enum TState { TS_TITLE, TS_SELECT, TS_COUNT, TS_PLAY, TS_OVER };
static const float SELECT_T = 6.f, LIGHT0 = 3.f, LIGHT_D = 0.6f; static float select_left = SELECT_T;   /* footage: at least 3.4 s of select, all red for at least 2.5 s, one green at t1161.7; the rest of the timing is a guess */
static TState ts = TS_TITLE; static float st_t = 0, time_left = 0; static double sim_t = 0;
static bool have_ref = false; static V head_ref; static int beeped = -1; static int demo_n = 0; static float wreck_me_at = 0; static bool wreck_me_done = false; static int start_at = -1;
static int leader = -1, last_rank = 0; static float say_cool = 0;

static void say(int i) { if (!sound_on || (voices[6].on && i != 0)) return; buggy::play(6, (int)(TSND_V0 + i), 120); }
static void t_burst(V at, int n, int kind, float speed, float life) {
    for (int i = 0; i < n && parts.size() < 160; i++) { Part q; q.p = at; q.v = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-0.2f, 1.3f))) * (speed * rnd(0.3f, 1.f)); q.life = life * rnd(0.6f, 1.f); q.kind = kind; parts.push_back(q); }
}
static void t_shed(V at, V v, int paint, int n) {   /* "bits of body panel litter the track" (ACE 49); the oldest go when there are too many */
    for (int i = 0; i < n; i++) { int cnt = 0, old = -1; for (size_t k = 0; k < parts.size(); k++) if (parts[k].kind == 3) { cnt++; if (old < 0) old = (int)k; } if (cnt >= 40 && old >= 0) parts.erase(parts.begin() + old);
        Part q; q.p = at; q.v = v * 0.5f + V(rnd(-3500, 3500), rnd(-3500, 3500), rnd(3000, 7000)); q.life = 1e9f; q.kind = 3; q.paint = paint; q.yaw = rnd(0, 6.28f); q.tilt = rnd(0, 6.28f); q.spin = rnd(-9, 9); parts.push_back(q); }
}
static int t_probe(const Track &T, V p, int hint, float &dist) {
    int best = hint; float bd = 1e30f;
    for (int d = -10; d <= 10; d++) { int k = ((hint + d) % NP + NP) % NP; V q = T.c[k] - p; float l = q.x * q.x + q.y * q.y; if (l < bd) { bd = l; best = k; } }
    if (bd > 12000.f * 12000.f) for (int k = 0; k < NP; k++) { V q = T.c[k] - p; float l = q.x * q.x + q.y * q.y; if (l < bd) { bd = l; best = k; } }
    dist = sqrtf(bd); return best;
}
static float yaw_of(V t) { return atan2f(-t.x, t.y); }
static M car_R(const Car &c) { return pitched(yawed(c.yaw), c.pitch); }
static void t_grid(int i) {   /* two staggered files; you are last in the left one, tan close ahead of you, pink ahead on the right (t1160) */
    Car &c = cars[i]; const Track &T = trk[course]; static const float OFS[8][2] = {{-8000, 3000}, {-3000, 3000}, {-2800, -100}, {-7800, -100}, {3200, 3000}, {3400, -100}, {8800, 3000}, {9000, -100}};   /* tan and pink solved from t1160: both about 5.5 m ahead, 3.1 m apart */
    int g = i & 7; c.p = T.c[GRID] + T.n[GRID] * OFS[g][1] + T.t[GRID] * OFS[g][0]; c.p.z = 0; c.v = V(); c.vz = 0; c.yaw = yaw_of(T.t[GRID]); c.yawrate = 0; c.pitch = 0; c.idx = GRID; c.damage = 0; c.wrecked = c.re = c.air = false; c.wreck_t = 0; c.stuck_t = c.rev_t = 0; c.hit_cool = 0; c.gear_rev = false; c.cp = 0;
}
static void t_place_objs(int k) {
    objs.clear(); const Track &T = trk[k]; const Ramp &R = ramps[k]; const float h = T.half;
    auto clear = [&](V p) { V d = p - R.c; float u = d.x * R.t.x + d.y * R.t.y, v = d.x * R.n.x + d.y * R.n.y; if (fabsf(v) < R.hw + 1500.f && u > -R.U0 - 8000.f && u < R.Lip + 30000.f) return false; return hypotf(d.x, d.y) > 22000.f; };
    auto add = [&](int type, V p, float yaw, float rad, float mass, bool fixed) { if (!clear(p)) return; Obj o; o.type = type; o.p = p; o.p.z = 0; o.yaw = yaw; o.rad = rad; o.mass = mass; o.fixed = fixed; objs.push_back(o); };
    /* loose tyres lying in rows, as t1171 and t1210, the last on the run in to the gantry */
    const int ty[8] = {GRID + NP - 14, GRID + NP - 22, 38, 56, 88, 118, 140, 165};
    for (int q = 0; q < 8; q++) { int i = ty[q] % NP; for (int s = 0; s < 4; s++) { float lat = (-0.66f + 0.44f * s + rnd(-0.08f, 0.08f)) * h; add(O_TYRE, T.c[i] + T.n[i] * lat + T.t[i] * rnd(-1500, 1500), rnd(0, 6.28f), 380.f, 60.f, false); } }
    /* orange boxes across the road, as t1176; stacks by the edge (spoken, t1165) */
    const int bx[2] = {30, 110}, st[2] = {62, 150}; for (int q = 0; q < 2; q++) { int i = bx[q]; for (int s = 0; s < 3; s++) add(O_BOX, T.c[i] + T.n[i] * ((s - 1) * h * 0.6f), yaw_of(T.t[i]), 650.f, 25.f, false); i = st[q]; add(O_STACK, T.c[i] + T.n[i] * ((q ? -1.f : 1.f) * h * 0.55f), yaw_of(T.t[i]), 1100.f, 120.f, false); }
    /* drums, staggered down the road (t1186) */
    const int br[2] = {72, 130}; for (int q = 0; q < 2; q++) { int i = br[q]; for (int s = 0; s < 7; s++) add(O_BARR, T.c[i] + T.n[i] * ((s & 1 ? 1.f : -1.f) * h * (0.25f + 0.1f * (s % 3))) + T.t[i] * (s * 4500.f), 0, 420.f, 0.f, true); }
}
static void t_rank() {
    const Track &T = trk[course];
    for (size_t i = 0; i < cars.size(); i++) { Car &c = cars[i]; int base = (GRID + c.cp * 15) % NP, tgt = (base + 15) % NP, off = ((c.idx - tgt) % NP + NP) % NP;
        if (off < 15 && c.dist < T.half * 2.5f) { c.cp++; base = tgt; }   /* checkpoints in order, so a short cut over the grass buys no laps */
        int rel = ((c.idx - base) % NP + NP) % NP; if (rel >= NP / 2) rel -= NP; rel = (int)clampf((float)rel, -8.f, 15.f);
        V nx = T.c[(c.idx + 1) % NP] - T.c[c.idx]; c.race = c.cp * 15.f + rel + dot(c.p - T.c[c.idx], norm(nx)) / (std::max)(1.f, len(nx)) - (float)i * 0.001f; }
    for (size_t i = 0; i < cars.size(); i++) { int r = 1; for (size_t k = 0; k < cars.size(); k++) if (k != i && cars[k].race > cars[i].race) r++; cars[i].rank = r; }
}
static void t_start_cars(bool demo) {
    cars.assign(1 + (size_t)n_cpu, Car());
    for (size_t i = 0; i < cars.size(); i++) { Car &c = cars[i]; c.paint = GRID_PAINT[i & 7]; c.cpu = demo || i != (size_t)me;
        bool real_cpu = i >= 4;   /* cars 1-3 stand in for the human slots; the 4 computer cars are the fast, aggressive ones (NCE, The One) */
        c.skill = (i == (size_t)me ? 0.9f : real_cpu ? rnd(0.93f, 1.f) : rnd(0.78f, 0.9f)) * cpu_skill; c.aggr = real_cpu;
        c.ai_off = rnd(-0.3f, 0.3f); t_grid((int)i); c.last_hit = -1; }
    t_place_objs(course); parts.clear(); t_rank(); leader = -1; last_rank = cars[me].rank; say_cool = 0;
}
static void t_start_demo() {
    v1knet::end_round(); course = demo_n++ & 1; t_start_cars(true); ts = TS_TITLE; st_t = 0; voices[8].on = false;
    dl("t=%.1f attract demo on course %d", sim_t, course);
}
static void t_start_round() {
    course = course_sel; t_start_cars(false); time_left = game_len; have_ref = false; beeped = -1; ts = TS_COUNT; st_t = 0; wreck_me_done = false;
    if (start_at >= 0) { Car &c = cars[me]; const Track &T = trk[course]; int k = start_at % NP; c.p = T.c[k]; c.yaw = yaw_of(T.t[k]); c.idx = k; }
    if (v1knet::on) v1knet::begin_round((int)(std::min)(cars.size(), (size_t)4));   /* linked: cars 0-3 are the four seats, pods fill them, the other cars stay computer drivers */
    dl("t=%.1f round start, course %d, %zu cars", sim_t, course, cars.size());
}

/* ---- collisions ---- */
static float t_impulse(V &va, float ma, V &vb, float mb, V n, float e) {   /* n points from b to a; mass 0 = fixed; returns the closing speed, 0 if separating */
    float vr = dot(va - vb, n); if (vr >= 0) return 0.f;
    float ia = ma > 0 ? 1.f / ma : 0.f, ib = mb > 0 ? 1.f / mb : 0.f;
    if (ia + ib <= 0) return 0.f;
    float j = -(1.f + e) * vr / (ia + ib); va += n * (j * ia); vb -= n * (j * ib); return -vr;
}
static void t_wreck(int i, int by) {
    Car &c = cars[i]; if (c.wrecked) return;
    c.wrecked = true; c.re = false; c.wreck_t = 0; c.damage = 100.f;
    const M CR = yawed(c.yaw); static const float PX[12] = {-500, 500, -500, 500, -500, 500, -500, 500, 0, -900, 900, 0}, PY[12] = {-1900, -1900, -1000, -1000, 200, 200, 1200, 1200, 1900, -300, -300, -600}, PZ[12] = {800, 800, 800, 800, 800, 800, 720, 720, 620, 600, 600, 1000};
    for (int k = 0; k < 12; k++) { Pan &q = c.pan[k]; q.rest = V(PX[k], PY[k], PZ[k]); q.p = c.p + to_world(CR, q.rest); q.v = c.v * 0.5f + V(rnd(-6000, 6000), rnd(-6000, 6000), rnd(6000, 14000)); q.a = 0; q.s = rnd(-9, 9); }   /* a fountain of panels (The One 37) */
    t_shed(c.p + V(0, 0, 800), c.v, c.paint, 3);
    t_burst(c.p + V(0, 0, 900), 14, 0, 3200.f, 0.9f); t_burst(c.p + V(0, 0, 900), 8, 1, 2600.f, 0.7f); t_burst(c.p + V(0, 0, 1000), 6, 2, 1500.f, 2.5f);   /* a half-hearted explosion (spoken, Octavius) */
    sfx(3, TSND_BOOM, i == me ? 120 : 90); say(0);   /* the shout (The One 37) */
    dl("t=%.1f car %d wrecked by %d", sim_t, i, by);
}
static void t_wreck_step(int i, float dt) {   /* out to the bird's eye, a pause, it comes back together; the camera side is in build_frame */
    Car &c = cars[i]; c.wreck_t += dt; float fr = c.wreck_t / respawn_delay; const Track &T = trk[course];
    if (fr < 0.5f) { for (Pan &q : c.pan) { q.p += q.v * dt; q.v.z -= 9800.f * dt; q.a += q.s * dt; if (q.p.z <= 40.f) { q.p.z = 40.f; q.v = V(); q.s = 0; } } }
    else { if (!c.re) { c.re = true; c.v = V(); c.yaw = yaw_of(T.t[c.idx]); c.pitch = 0; for (Pan &q : c.pan) q.from = q.p; }
        float u = clampf((fr - 0.5f) / 0.26f, 0.f, 1.f); u = u * u * (3.f - 2.f * u); M CR = yawed(c.yaw); for (Pan &q : c.pan) { V to = c.p + to_world(CR, q.rest); q.p = q.from + (to - q.from) * u; q.a = q.a * (1.f - u * 0.15f); } }
    if (fr >= 1.f) { c.wrecked = c.re = false; c.damage = 0; c.wreck_t = 0; c.v = V(); c.vz = 0; c.stuck_t = c.rev_t = 0; c.hit_cool = 0.5f; dl("t=%.1f car %d back together", sim_t, i); }
}
static void t_circles(const Car &c, V out[2]) { V f(-sinf(c.yaw), cosf(c.yaw), 0); out[0] = c.p + f * 1150.f; out[1] = c.p - f * 1150.f; }
static void t_car_hit(int ia, int ib, V n, float impact, float a_toward, float b_toward, V at) {   /* n points from b to a */
    Car &a = cars[ia], &b = cars[ib];
    if (impact < 500.f) return;
    float d = (std::max)(0.f, impact - 2000.f) / 1000.f * 1.6f * dmg_scale;   /* INVENTED damage; the polite little bumps (Octavius) do none */
    a.damage += d; b.damage += d;
    int aggr = a_toward > b_toward ? ia : ib, vic = aggr == ia ? ib : ia;
    if (a.hit_cool <= 0 && b.hit_cool <= 0) {
        cars[vic].last_hit = aggr; a.hit_cool = b.hit_cool = 0.25f;
        sfx(((ia + ib) & 1) ? 1 : 2, TSND_BUMP, (int)clampf(50.f + impact / 80.f, 50.f, 127.f));
        t_burst(at + V(0, 0, 600), 4, 1, 1800.f, 0.4f);
        if (impact > 5000.f) t_shed(at + V(0, 0, 700), cars[vic].v, cars[vic].paint, 1);
        dl("t=%.1f car %d hit car %d at %.1f m/s: damage %.0f/%.0f", sim_t, aggr, vic, impact / 1000.f, a.damage, b.damage);
    }
    if (a.damage >= 100.f && !a.wrecked) t_wreck(ia, a.last_hit);
    if (b.damage >= 100.f && !b.wrecked) t_wreck(ib, b.last_hit);
}
static void t_collide() {
    for (size_t a = 0; a < cars.size(); a++) for (size_t b = a + 1; b < cars.size(); b++) {
        if (cars[a].wrecked || cars[b].wrecked || fabsf(cars[a].p.z - cars[b].p.z) > 1100.f) continue;
        for (int ca = 0; ca < 2; ca++) for (int cb = 0; cb < 2; cb++) {
            V A[2], B[2]; t_circles(cars[a], A); t_circles(cars[b], B);
            V d = A[ca] - B[cb]; d.z = 0; float dist = len(d);
            if (dist >= 2.f * CAR_R || dist < 1.f) continue;
            V n = d * (1.f / dist); float pen = 2.f * CAR_R - dist;
            cars[a].p += n * (pen * 0.5f); cars[b].p -= n * (pen * 0.5f);
            float at = -dot(cars[a].v, n), bt = dot(cars[b].v, n);
            float imp = t_impulse(cars[a].v, CAR_M, cars[b].v, CAR_M, n, bump_e);
            if (imp > 0) {
                V arm = A[ca] - cars[a].p; cars[a].yawrate = clampf(cars[a].yawrate + (arm.x * n.y - arm.y * n.x) * imp * 0.0000002f, -3.f, 3.f);
                arm = B[cb] - cars[b].p; cars[b].yawrate = clampf(cars[b].yawrate - (arm.x * n.y - arm.y * n.x) * imp * 0.0000002f, -3.f, 3.f);
                t_car_hit((int)a, (int)b, n, imp, at, bt, (A[ca] + B[cb]) * 0.5f);
            }
        }
    }
    for (size_t i = 0; i < cars.size(); i++) { if (cars[i].wrecked) continue; for (size_t k = 0; k < objs.size(); k++) {
        Obj &o = objs[k]; Car &c = cars[i]; if (o.p.z > c.p.z + 1000.f || o.p.z < c.p.z - 800.f) continue;
        for (int cc = 0; cc < 2; cc++) {
            V C[2]; t_circles(c, C); V d = C[cc] - o.p; d.z = 0; float dist = len(d), R = CAR_R + o.rad;
            if (dist >= R || dist < 1.f) continue;
            V n = d * (1.f / dist); float pen = R - dist;
            if (o.fixed) c.p += n * pen; else { c.p += n * (pen * 0.4f); o.p -= n * (pen * 0.6f); }
            V cv = c.v; float imp = t_impulse(c.v, CAR_M, o.v, o.fixed ? 0.f : o.mass, n, o.fixed ? 0.15f : 0.2f);
            if (imp > 400.f) {
                V arm = C[cc] - c.p; c.yawrate = clampf(c.yawrate + (arm.x * n.y - arm.y * n.x) * imp * 0.0000002f, -3.f, 3.f);
                if (imp > 2000.f) sfx(4 + (int)(k & 1), o.type == O_BARR ? TSND_BUMP : TSND_THUD, (int)clampf(40.f + imp / 100.f, 40.f, 120.f));
                c.damage += imp / 1000.f * (o.fixed ? 0.5f : 0.05f) * dmg_scale;   /* INVENTED: drums hurt, tyres and boxes barely */
                if (c.damage >= 100.f && !c.wrecked) t_wreck((int)i, c.last_hit);
                if (o.type == O_TYRE && imp > 1500.f) { o.v = cv * 1.25f + V(rnd(-1500, 1500), rnd(-1500, 1500), 0); o.v.z = 0; o.vz = 8000.f + imp * 0.25f; o.spin = rnd(4, 9); }   /* it leaps ahead of you, high up (t1172.2-t1172.7) */
                if (o.type == O_STACK && imp > 5000.f) {   /* the stack comes apart into three boxes */
                    Obj proto = o; proto.type = O_BOX; proto.rad = 600.f; proto.mass = 20.f;
                    o = proto; o.v = proto.v + V(0, -2500.f, 0);
                    for (int q = 0; q < 2; q++) { Obj b2 = proto; b2.p = proto.p + V(q ? 700.f : -700.f, q ? -400.f : 400.f, 0); b2.v = proto.v + V(q ? 2500.f : -2500.f, 1500.f, 0); objs.push_back(b2); }
                    break;   /* objs grew: o is dangling now, leave this object */
                }
            }
        }
    } }
}

/* ---- driving ---- */
static void t_step_car(Car &c, float dt, float steer_in, float thr_in, bool brake_only) {
    const Track &T = trk[course];
    c.idx = t_probe(T, c.p, c.idx, c.dist);
    bool on = c.dist <= T.half + 700.f;
    if (c.wrecked) { steer_in = 0; thr_in = 0; }
    V f(-sinf(c.yaw), cosf(c.yaw), 0), r(cosf(c.yaw), sinf(c.yaw), 0);
    float vf = dot(c.v, f), vl = dot(c.v, r), top = on ? vmax : vmax * 0.55f;
    if (!c.air) {
        if (thr_in > 0) vf += thr_in * accel_max * clampf(1.f - vf / top, 0.f, 1.5f) * dt;
        else if (thr_in < 0) { if (vf > 300.f) vf = (std::max)(0.f, vf + thr_in * 14000.f * dt); else if (brake_only) { if (vf < 0) vf = (std::min)(0.f, vf - thr_in * 14000.f * dt); } else vf += thr_in * accel_max * 0.5f * clampf(1.f + vf / (vmax * 0.3f), 0.f, 1.f) * dt; }
        vf -= vf * (c.wrecked ? 1.5f : 0.25f) * dt; if (!on) vf -= vf * 1.2f * dt;
        c.v += f * (vf - dot(c.v, f)); c.v -= r * (vl * (1.f - expf(-(on ? 8.f : 4.f) * dt)));
        float ang = steer_in * 0.5f / (1.f + fabsf(vf) / 12000.f);
        float want = -vf / 2800.f * tanf(ang);   /* positive steer = right = yaw decreases */
        c.yawrate += (want - c.yawrate) * clampf(10.f * dt, 0.f, 1.f); c.yaw += c.yawrate * dt;
    }
    if (c.dist > 120000.f) { c.v = c.v * (1.f - 2.f * dt); V back = T.c[c.idx] - c.p; back.z = 0; c.v += norm(back) * (3000.f * dt); }
    V old = c.p; c.p.x += c.v.x * dt; c.p.y += c.v.y * dt; c.v.z = 0;
    bool wall = false; float G = t_ground(c.p, c.p.z, wall);
    if (wall) { float sp = len(c.v); c.p = old; c.v = c.v * -0.3f; c.yawrate *= 0.5f; if (sp > 3000.f && c.hit_cool <= 0) { c.hit_cool = 0.25f; c.damage += sp / 1000.f * 1.2f * dmg_scale; sfx(5, TSND_BUMP, 110); } G = c.p.z; }
    if (!c.air) { if (G < c.p.z - 120.f) c.air = true; else { c.vz = clampf((G - c.p.z) / dt, 0.f, 6000.f); c.p.z = G; } }
    if (c.air) { c.vz -= 9800.f * dt; c.p.z += c.vz * dt;
        if (c.p.z <= G) { float hit = -c.vz; c.p.z = G; c.vz = 0; c.air = false; if (hit > 5000.f && !c.wrecked) { c.damage += (hit - 5000.f) / 1000.f * 8.f * dmg_scale; sfx(5, TSND_THUD, 120); if (hit > 7000.f) t_shed(c.p + V(0, 0, 700), c.v, c.paint, 1); } } }   /* "smashed to bits just doing this" (NCE 147); the numbers are INVENTED */
    { const Ramp &R = ramps[course]; float want = 0; if (c.air) want = clampf(atan2f(c.vz, (std::max)(3000.f, len(c.v))), -0.45f, 0.3f); else if (c.p.z > 1.f) want = atanf(R.H / (R.U0 + R.Lip)) * dot(f, R.t); c.pitch += (want - c.pitch) * clampf(8.f * dt, 0.f, 1.f); }
    c.wheel += (steer_in * 2.0f - c.wheel) * clampf(12.f * dt, 0.f, 1.f);
    if (c.hit_cool > 0) c.hit_cool -= dt;
    if ((c.damage > 60.f && !c.wrecked) || (c.wrecked && !c.re)) { c.smoke_t -= dt; if (c.smoke_t <= 0) { c.smoke_t = c.wrecked ? 0.1f : 0.25f; t_burst(c.p + f * 1500.f + V(0, 0, 900), 1, 2, 900.f, 1.8f); } }
}
static void t_ai(int i, float dt, float &steer, float &thr) {
    Car &c = cars[i]; const Track &T = trk[course];
    V f(-sinf(c.yaw), cosf(c.yaw), 0), r(cosf(c.yaw), sinf(c.yaw), 0);
    float vf = dot(c.v, f);
    int ti = (c.idx + 3 + (int)(fabsf(vf) / 4500.f)) % NP;
    V tgt = T.c[ti] + T.n[ti] * (c.ai_off * T.half);
    bool hunting = c.aggr && fmodf((float)sim_t + i * 3.f, 18.f) < 4.f, chase = false; float bd = hunting ? 14000.f : 9000.f; int vic = -1;
    for (size_t k = 0; k < cars.size(); k++) { if ((int)k == i || cars[k].wrecked) continue;   /* computer cars "can get very aggressive indeed" (The One 37): ram whoever is close ahead */
        V d = cars[k].p - c.p; d.z = 0; float dd = len(d); if (dd < bd && dot(norm(d), f) > 0.5f) { bd = dd; vic = (int)k; } }
    if (vic >= 0 && hunting && c.dist < T.half * 1.5f) { tgt = cars[vic].p + cars[vic].v * 0.35f; chase = true; }
    else if (vic >= 0) { V dv = cars[vic].p - T.c[c.idx]; float side = dot(dv, T.n[c.idx]) > 0 ? -1.f : 1.f; tgt += T.n[ti] * (side * 3200.f); }   /* otherwise go round it */
    V d = tgt - c.p; d.z = 0;
    float ang = atan2f(dot(d, r), dot(d, f));
    steer = clampf(ang * 1.8f, -1.f, 1.f);
    float want = vmax * c.skill * (chase ? 1.0f : 0.92f) * (1.f - 0.5f * fabsf(steer));
    thr = clampf((want - vf) / 3500.f, -0.6f, 1.f);
    if (c.rev_t > 0) { c.rev_t -= dt; thr = -1.f; steer = -steer; }
    else { if (fabsf(vf) < 700.f && thr > 0.3f) c.stuck_t += dt; else c.stuck_t = 0; if (c.stuck_t > 1.5f) { c.rev_t = 1.2f; c.stuck_t = 0; } }
}
/* link play: what the master read from the pod in seat i (a car number 1-3) this tick */
static bool net_has[4] = {false, false, false, false}; static float net_steer[4], net_ped[4];
static void t_parts_step(float dt) {
    for (size_t i = 0; i < parts.size();) { Part &q = parts[i]; q.age += dt; q.p += q.v * dt;
        if (q.kind == 3) { q.v.z -= 9800.f * dt; q.tilt += q.spin * dt; if (q.p.z <= 40.f) { q.p.z = 40.f; q.v = V(); q.spin = 0; q.tilt = 0; } }
        else if (q.kind == 2) q.v = V(q.v.x * (1.f - dt), q.v.y * (1.f - dt), q.v.z + 400.f * dt * (q.v.z < 1500.f ? 1.f : 0.f)); else q.v.z -= 19600.f * 0.25f * dt;
        if (q.age >= q.life) { parts[i] = parts.back(); parts.pop_back(); } else i++; }
}
static void t_world(const VPIX_Input &in, float dt) {
    if (cars.empty()) return;
    bool live = ts == TS_PLAY || ts == TS_TITLE || ts == TS_SELECT;
    for (size_t i = 0; i < cars.size(); i++) {
        Car &c = cars[i]; float steer = 0, thr = 0; bool brake_only = false;
        if (live && !c.wrecked) {
            if (i == (size_t)me && ts == TS_PLAY && !autoplay) {   /* wheel, accelerator and brake pedals, a forward/reverse lever (YC 83, The One 37): the brake never reverses */
                steer = clampf(in.joy[0], -1.f, 1.f); float ped = clampf(in.joy[1] + in.throttle - in.brake, -1.f, 1.f);
                if (ped > 0) thr = c.gear_rev ? -ped : ped; else if (ped < 0) { thr = ped; brake_only = true; } }
            else if (i < 4 && net_has[i] && ts == TS_PLAY) {   /* linked: a person on another pod in this seat */
                steer = clampf(net_steer[i], -1.f, 1.f); float ped = clampf(net_ped[i], -1.f, 1.f);
                if (ped > 0) thr = c.gear_rev ? -ped : ped; else if (ped < 0) { thr = ped; brake_only = true; } }
            else t_ai((int)i, dt, steer, thr);
        }
        t_step_car(c, dt, steer, thr, brake_only);
        if (c.wrecked) t_wreck_step((int)i, dt);
        else if (c.damage >= 100.f) t_wreck((int)i, c.last_hit);
    }
    t_collide();
    for (Obj &o : objs) { if (o.fixed) continue; o.p.x += o.v.x * dt; o.p.y += o.v.y * dt; o.v.z = 0;
        if (o.p.z > 0 || o.vz > 0) { o.vz -= 9800.f * dt; o.p.z += o.vz * dt; o.tumble += o.spin * dt; o.yaw += o.spin * 0.3f * dt; if (o.p.z <= 0) { o.p.z = 0; o.vz = -o.vz * 0.35f; o.v = o.v * 0.7f; if (o.vz < 900.f) { o.vz = 0; o.spin = 0; o.tumble = 0; } } }
        else { o.v = o.v * expf(-1.2f * dt); if (o.type != O_TYRE) o.yaw += len(o.v) * 0.00003f * dt * 60.f; } }
    t_parts_step(dt);
    t_rank();
    if (ts == TS_PLAY) {
        if (wreck_me_at > 0 && !wreck_me_done && st_t >= wreck_me_at) { wreck_me_done = true; t_wreck(me, -1); }
        say_cool -= dt;   /* the running commentary tells you who is where (ACE 49); the words are INVENTED */
        int lead = 0; for (size_t i = 0; i < cars.size(); i++) if (cars[i].rank == 1) lead = (int)i;
        if (lead != leader && say_cool <= 0) { leader = lead; say_cool = 6.f; if (lead == me) say(10); else say(2 + cars[lead].paint); }
        else if (cars[me].rank == (int)cars.size() && last_rank != cars[me].rank && say_cool <= 0) { say_cool = 6.f; say(11); }
        last_rank = cars[me].rank;
        time_left -= dt; if (time_left <= 0) { ts = TS_OVER; st_t = 0; voices[8].on = false;   /* the race ends and where you are is what counts: "not whether you crossed the finish line" (NCE 147) */
        dl("t=%.1f race over: you are %d of %zu, %d checkpoints; leader %d checkpoints", sim_t, cars[me].rank, cars.size(), cars[me].cp, cars[leader < 0 ? 0 : leader].cp); } }
}


/* ---- the picture ---- */
static void t_blit(VPIX_Frame &f, int id, int w, int h, float x, float y, float dw, float dh) {
    VPIX_Blit b = {}; b.handle = d1k::BMPH + id; b.sw = w; b.sh = h; b.x = (int)x; b.y = (int)y; b.dw = dw; b.dh = dh; b.key = 0; b.alpha = 1.f; b.prio = 1000000; b.tint[0] = b.tint[1] = b.tint[2] = 1.f; f.blits.push_back(b);
}
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = 0x6E; f.clear[1] = 0x88; f.clear[2] = 0x9C;
    f.n_lights = 1; f.lights[0] = {0.65f, 0.35f, {-0.218f, -0.437f, 0.874f}};   /* as Dactyl: flat shaded and bright, a guess */
    bool wreck_cam = !cars.empty() && ts >= TS_COUNT && cars[me].wrecked; bool in_car = !cars.empty() && ts >= TS_COUNT && !wreck_cam, dash_on = false;
    M cam; V eye; bool head_cam = false;
    static const char *fixed = getenv("TOTALD_CAM");
    if (fixed && *fixed) { float a[6] = {0, -30000, 20000, 0, 0, 0}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5])); in_car = false; }
    else if (wreck_cam) {   /* "the camera view zooms out with the explosion, a bird's-eye view of your own demise", a pause, then a smooth path in through the back windscreen (NCE 147); phase times INVENTED */
        const Car &c = cars[me]; float fr = c.wreck_t / respawn_delay; V fw(-sinf(c.yaw), cosf(c.yaw), 0); V seat = c.p + to_world(yawed(c.yaw), SEAT), bird = c.p + V(0, -7000, 26000), back = c.p - fw * 9000.f + V(0, 0, 2600), look = c.p + V(0, 0, 500);
        auto sm = [](float x) { x = clampf(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); };
        if (fr < 0.37f) { float q = sm(fr / 0.37f); eye = seat + (bird - seat) * q; }
        else if (fr < 0.72f) eye = bird;
        else { float q = (fr - 0.72f) / 0.28f; q = sm(q); V a = bird + (back - bird) * q, b = back + (seat - back) * q; eye = a + (b - a) * q; look = look + ((c.p + fw * 20000.f + V(0, 0, 900)) - look) * q; dash_on = q > 0.93f; }
        cam = look_at(eye, look);
    }
    else if (in_car) {
        const Car &c = cars[me]; M CR = car_R(c);
        if (in.head.valid && !autoplay) {
            V hp(in.head.p[0], in.head.p[1], in.head.p[2]);
            if (!have_ref) { head_ref = hp; have_ref = true; }
            for (int k = 0; k < 3; k++) cam.r[k] = to_world(CR, V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2]));
            V dh = hp - head_ref; if (len(dh) > 600.f) dh = norm(dh) * 600.f;
            eye = c.p + to_world(CR, SEAT + dh); head_cam = true;
        } else { cam = CR; eye = c.p + to_world(CR, SEAT);
            float ya = 0, pi = -9.5f; static const char *lk = getenv("TOTALD_LOOK"); if (lk && *lk) sscanf(lk, "%f,%f", &ya, &pi); ya *= 0.0174533f; pi *= 0.0174533f;   /* the footage's horizon sits at 34% of the height */
            V fw = cam.r[1], ri = cam.r[0], up = cam.r[2]; V f2 = fw * cosf(ya) + ri * sinf(ya), r2 = ri * cosf(ya) - fw * sinf(ya); V f3 = f2 * cosf(pi) + up * sinf(pi), u3 = up * cosf(pi) - f2 * sinf(pi); cam.r[0] = r2; cam.r[1] = f3; cam.r[2] = u3; }
    } else if (!cars.empty()) {   /* title and select: a chase view of one of the computer cars */
        const Car &c = cars[((int)(st_t / 12.f) + 1) % cars.size()]; V fw(-sinf(c.yaw), cosf(c.yaw), 0);
        eye = c.p - fw * 9000.f + V(0, 0, 3500.f); cam = look_at(eye, c.p + fw * 5000.f + V(0, 0, 700.f));
    } else { eye = V(0, -60000, 30000); cam = look_at(eye, V(0, 0, 0)); }

    if (ts == TS_SELECT) {   /* a flat picture and two big 3D cars over it, turning over in pairs (t1156-t1159) */
        M I; cam.r[0] = V(1, 0, 0); cam.r[1] = V(0, 0, -1); cam.r[2] = V(0, 1, 0); eye = V(0, 0, 10000);
        VPIX_Instance &q0 = dput(f, T_MAPQ, I, V(0, 0, 0)); q0.bright = 1; q0.on_top = 2;
        static const int pair[3][2] = {{2, 4}, {3, 0}, {5, 6}}; const int *pr = pair[(int)(st_t / 1.8f) % 3];
        const float turn = st_t * 0.785f; { M RT = yawed(turn); V cp((425.f - 320.f) * 14000.f / 640.f, 0, 0); { VPIX_Instance &rq = dput(f, T_MAPR, RT, cp); rq.on_top = 2; rq.bright = 1; } if (course_sel == 0) { VPIX_Instance &mk = dput(f, T_MAPM, RT, cp); mk.on_top = 2; mk.bright = 1; } }
        for (int k = 0; k < 2; k++) { float yw = (k ? 3.14159f : 0.f) + turn; VPIX_Instance &q = dput(f, T_CAR + pr[k], yawed(yw), V(k ? -40.f : 0.f, k ? -2800.f : 2800.f, 1800.f)); q.S[0] = q.S[1] = q.S[2] = 0.5f; q.on_top = 2; }
    }
    else { M I; dput(f, T_SKY, I, eye).bright = 1; dput(f, T_GROUND, I, V(0, 0, 0)); dput(f, T_TRACK + course, I, V(0, 0, 0));
      dput(f, T_RAMP + course, I, V(0, 0, 0));
      if (!cars.empty()) { const Track &T = trk[course]; V p = T.c[GRID] + T.t[GRID] * 7000.f - T.n[GRID] * (T.half + 300.f); M GR = yawed(yaw_of(T.t[GRID])); dput(f, T_GANTRY, GR, p);
          int greens = ts == TS_COUNT ? (st_t < LIGHT0 ? 0 : (std::min)(4, 1 + (int)((st_t - LIGHT0) / LIGHT_D))) : 4;   /* the 4 lights: red, then green from the left (t1161.7) */
          for (int i = 0; i < 4; i++) { float x = -4350.f + i * 560.f; dput(f, i < greens ? T_LAMP_G : T_LAMP_R, GR, p + to_world(GR, V(x, 0, 9525))).bright = 1; } } }
    if (ts != TS_SELECT) for (size_t i = 0; i < cars.size(); i++) { const Car &c = cars[i];
        if (in_car && (int)i == me) continue;   /* your own car is the dashboard */
        if (c.wrecked) { float fr = c.wreck_t / respawn_delay; if (fr < 0.76f) for (const Pan &q : c.pan) { VPIX_Instance &pi = dput(f, T_PANEL + c.paint, pitched(yawed(q.a * 0.6f + c.yaw), q.a), q.p); (void)pi; }
            if (fr < 0.72f || (dash_on && (int)i == me)) continue; }
        dput(f, T_CAR + c.paint, car_R(c), c.p); }
    if (ts != TS_SELECT) for (const Obj &o : objs) { static const int mid[4] = {T_TYRE, T_BOX, T_STACK, T_BARR}; dput(f, mid[o.type], pitched(yawed(o.yaw), o.tumble), o.p); }
    if (ts != TS_SELECT) for (const Part &q : parts) { M I; if (q.kind == 3) dput(f, T_PANEL + q.paint, pitched(yawed(q.yaw), q.tilt), q.p);
        else if (q.kind == 2) { VPIX_Instance &s = dput(f, T_SMOKE, I, q.p); float sz = 0.22f + q.age * 0.45f; s.S[0] = s.S[1] = s.S[2] = sz; s.bright = 1; }
        else dput(f, q.kind ? T_SPARK_Y : T_SPARK_O, I, q.p).bright = 1; }
    if (in_car || dash_on) {   /* dashboard, wheel and A-pillar are fixed to the car, so they move across the view when you turn your head */
        const Car &c = cars[me]; M CR = car_R(c);
        dput(f, T_DASH, CR, c.p).on_top = 2; dput(f, T_BODY, CR, c.p).on_top = 2;
        float ca = cosf(c.wheel), sa = sinf(c.wheel), ct = cosf(0.5f), st2 = sinf(0.5f);
        V rows[3] = {V(ca, 0, -sa), V(0, 1, 0), V(sa, 0, ca)}; M W;
        for (int k = 0; k < 3; k++) { V q = rows[k]; V t2(q.x, q.y * ct - q.z * st2, q.y * st2 + q.z * ct); W.r[k] = to_world(CR, t2); }   /* rolled about its axis, then leaned back */
        dput(f, T_WHEEL, W, c.p + to_world(CR, V(380.f, 640.f, 800.f))).on_top = 2;
    }
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    f.in_head = head_cam; if (head_cam) f.head_fed = in.head;

    const float CX = 176.f;
    if (ts == TS_TITLE) {
        t_blit(f, BM_MAP + 3, TD_TW, TD_TH, 0, 0, 352.f, 288.f);
    } else if (ts == TS_SELECT) {
        if (course_sel == 1) { const float *m = TRK1[0], z = 3.f; t_blit(f, BM_MAP + 2, 4, 4, m[0] * .55f - z, m[1] * .8f - z, 2.f * z, 2.f * z); }   /* the white mark, left map only in the footage; moving it to the other map is INVENTED, the footage shows no choosing */
        int n = (int)(st_t * 11.4f) % 16 + 1;   /* bottom row fills, then the top, a yellow dot leading; both clear; about 1.4 s a run (t1155.6-t1158.6) */
        for (int i = 0, bot = (std::min)(n, 8); i < bot; i++) t_blit(f, BM_MAP + (i == bot - 1 && n <= 8 ? 1 : 0), 12, 12, 215.f + i * 15.9f, 250.f, 11.f, 11.f);
        for (int i = 0, top = (std::max)(0, n - 8); i < top; i++) t_blit(f, BM_MAP + 4 + (i == top - 1 ? 1 : 0), 12, 12, 215.f + i * 15.9f, 22.f, 11.f, 11.f);
    } else {
        /* the gold readout, top right: your place of 8, the 8 smaller. Inferred: it reads 8/8 for the whole of the footage's slow race from the back of the grid, and press says 8 cars. Not there on the grid (t1159-t1162). */
        if ((ts == TS_PLAY || ts == TS_OVER) && !cars.empty()) { int place = (std::max)(1, (std::min)(8, cars[me].rank)); t_blit(f, 250 + place, d1k::DW, d1k::DH, 246.f, 20.f, 18.f, 34.f); t_blit(f, BM_GSLASH, d1k::DW, d1k::DH, 263.f, 20.f, 15.f, 34.f); t_blit(f, BM_GDIG8, d1k::DW, d1k::DH, 279.f, 34.f, 13.f, 20.f); }
        if (ts == TS_OVER && !cars.empty()) {   /* INVENTED end screen: the finishing order */
            for (int r = 1; r <= (int)cars.size(); r++) for (size_t i = 0; i < cars.size(); i++) if (cars[i].rank == r) { char b[40]; snprintf(b, sizeof b, "%d %s", r, PAINT_NAME[cars[i].paint]); text(f, b, CX, 62.f + 22.f * (r - 1), 1.f, (int)i == me ? 1 : 0); }
            text(f, "RACE OVER", 150.f, 34, 1.1f, 1);
        }
    }
    d1k::look_test(f);
    d1k::hs::draw(f, "totald", ts == TS_TITLE, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at game_fps ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 1; static unsigned long frames = 0;
static void t_engine_sound() {
    if (sound_on && (ts == TS_COUNT || ts == TS_PLAY) && !cars.empty() && !cars[me].wrecked) {   /* the engine note follows speed; a live voice's bend and volume are set directly, as Buggy does */
        float sp = clampf(len(cars[me].v) / vmax, 0.f, 1.5f);
        if (!voices[8].on) buggy::play(8, (int)TSND_ENG, 60);
        voices[8].bend = (int)((sp * 12.f - 3.f) * 32.f); voices[8].vol = 40 + (int)(sp * 50.f);
    } else voices[8].on = false;
}

/* ---- link play (vpix_1k_net.h): pod 1 runs the race; cars 0-3 are the four seats, the other pods drive their own car with the wheel and pedals and see the master's cars and loose objects.
   Start-together. INVENTED: the real game's three human slots were other cars in the same room; pods fill them in the order they join. Panels, sparks and smoke on another pod are made locally from the wreck, not copied. ---- */
#pragma pack(push, 1)
struct NetIn { float steer, ped; uint8_t lever_seq, pad[3]; };
struct NetC { float x, y; int16_t z, vx, vy; uint16_t yaw; int8_t pitch, wheel; uint8_t dmg, flags, wfr, rank, cp, paint; };   /* v in 2 mm/s, flags 1 wrecked 2 air 4 reverse */
struct NetO { uint8_t type; int16_t x, y, z; uint8_t yaw, tum; };   /* positions in 10 mm */
struct NetSt { uint8_t ts, ncars, nobj, course; float st_t, time_left; NetC c[8]; NetO o[48]; };
#pragma pack(pop)
static bool slave_game = false; static std::vector<Obj> net_fixed; static uint8_t net_lev[4]; static bool net_lev_ok[4]; static bool net_was_person[4];
static int16_t n16(float v, float k) { return (int16_t)clampf(v * k, -32000.f, 32000.f); }
static void net_log() {
    static double next = 0; if (sim_t < next) return; next = sim_t + 5.0;
    if (cars.size() < 4 || ts < TS_COUNT) { dl("t=%.1f link: %s, not in a race, pods heard %d", sim_t, v1knet::master ? "master" : "slave", v1knet::live_pods()); return; }
    dl("t=%.1f link: %s seat %d ts %d left %.0f | c0 (%.0f %.0f) place %d | c1 %s(%.0f %.0f) place %d | c2 (%.0f %.0f) | c3 (%.0f %.0f)", sim_t, v1knet::master ? "master" : "slave", me, (int)ts, time_left,
       cars[0].p.x, cars[0].p.y, cars[0].rank, net_has[1] ? "person " : "cpu ", cars[1].p.x, cars[1].p.y, cars[1].rank, cars[2].p.x, cars[2].p.y, cars[3].p.x, cars[3].p.y);
}
static void net_master() {
    for (int i = 1; i < 4 && i < (int)cars.size(); i++) { const NetIn *p = (const NetIn *)v1knet::seat_input(i, sizeof(NetIn));
        if (!p || ts < TS_COUNT) { if (net_was_person[i]) { net_was_person[i] = false; dl("t=%.1f link: seat %d back to the computer", sim_t, i); } net_has[i] = false; net_lev_ok[i] = false; continue; }
        if (!net_was_person[i]) { net_was_person[i] = true; dl("t=%.1f link: seat %d is a person now", sim_t, i); }
        net_has[i] = true; net_steer[i] = p->steer; net_ped[i] = p->ped;
        if (net_lev_ok[i] && p->lever_seq != net_lev[i] && ts == TS_PLAY && !cars[i].wrecked) { cars[i].gear_rev = !cars[i].gear_rev; sfx(0, TSND_LEVER, 60); }
        net_lev[i] = p->lever_seq; net_lev_ok[i] = true; }
    if (!v1knet::due(30)) return;
    if (ts < TS_COUNT || cars.empty()) { v1knet::send_snapshot(NULL, 0); return; }
    static NetSt s; memset(&s, 0, sizeof(s)); s.ts = (uint8_t)ts; s.ncars = (uint8_t)(std::min)(cars.size(), (size_t)8); s.course = (uint8_t)course; s.st_t = st_t; s.time_left = time_left;
    for (int i = 0; i < s.ncars; i++) { const Car &c = cars[i]; NetC &n = s.c[i]; n.x = c.p.x; n.y = c.p.y; n.z = n16(c.p.z, 1.f); n.vx = n16(c.v.x, 0.5f); n.vy = n16(c.v.y, 0.5f);
        float y = fmodf(c.yaw, 6.2831853f); if (y < 0) y += 6.2831853f; n.yaw = (uint16_t)(y / 6.2831853f * 65535.f);
        n.pitch = (int8_t)clampf(c.pitch * 100.f, -127.f, 127.f); n.wheel = (int8_t)clampf(c.wheel * 50.f, -127.f, 127.f); n.dmg = (uint8_t)clampf(c.damage * 2.f, 0.f, 255.f);
        n.wfr = (uint8_t)clampf(c.wreck_t / respawn_delay * 255.f, 0.f, 255.f); n.flags = (c.wrecked ? 1 : 0) | (c.air ? 2 : 0) | (c.gear_rev ? 4 : 0);
        n.rank = (uint8_t)c.rank; n.cp = (uint8_t)(std::min)(c.cp, 255); n.paint = (uint8_t)c.paint; }
    for (const Obj &o : objs) { if (o.fixed || s.nobj >= 48) continue; NetO &n = s.o[s.nobj++]; n.type = (uint8_t)o.type; n.x = n16(o.p.x, 0.1f); n.y = n16(o.p.y, 0.1f); n.z = n16(o.p.z, 0.1f);
        float y = fmodf(o.yaw, 6.2831853f); if (y < 0) y += 6.2831853f; n.yaw = (uint8_t)(y / 6.2831853f * 255.f);
        float t = fmodf(o.tumble, 6.2831853f); if (t < 0) t += 6.2831853f; n.tum = (uint8_t)(t / 6.2831853f * 255.f); }
    v1knet::send_snapshot(&s, offsetof(NetSt, o) + s.nobj * sizeof(NetO));
}
static void net_leave(const char *why) {
    slave_game = false; me = 0; voices[8].on = false; t_start_demo(); dl("t=%.1f link: %s, back to the title", sim_t, why);
}
static void net_apply(const NetSt &s, uint8_t round) {
    static uint8_t my_round = 0; bool fresh = !slave_game || round != my_round || cars.size() != s.ncars; my_round = round;
    me = v1knet::my_seat; const TState nts = (TState)s.ts;
    if (fresh) { course = s.course & 1; cars.assign(s.ncars, Car()); parts.clear(); have_ref = false; beeped = -1; wreck_me_done = false; slave_game = true; leader = -1;
        for (size_t i = 0; i < cars.size(); i++) { Car &c = cars[i]; c.paint = GRID_PAINT[i & 7]; c.cpu = (int)i != me; c.skill = 0.95f; c.ai_off = 0.f; }
        t_place_objs(course); net_fixed.clear(); for (const Obj &o : objs) if (o.fixed) net_fixed.push_back(o);
        dl("t=%.1f link: in the master's race, seat %d of %d cars, course %d", sim_t, me, (int)s.ncars, course); }
    const Track &T = trk[course];
    for (int i = 0; i < s.ncars; i++) { Car &c = cars[i]; const NetC &n = s.c[i];
        c.p = V(n.x, n.y, (float)n.z); c.v = V(n.vx * 2.f, n.vy * 2.f, 0.f); c.yaw = n.yaw / 65535.f * 6.2831853f; c.pitch = n.pitch / 100.f; c.wheel = n.wheel / 50.f; c.damage = n.dmg / 2.f;
        c.air = (n.flags & 2) != 0; c.gear_rev = (n.flags & 4) != 0; c.rank = n.rank; c.cp = n.cp; c.paint = n.paint & 7;
        float dist; c.idx = t_probe(T, c.p, c.idx, dist); c.dist = dist;
        const bool w = (n.flags & 1) != 0 && n.wfr < 255;
        if (w && !c.wrecked) t_wreck(i, -1);   /* the bang, panels and the shout are made here */
        c.wrecked = w; if (!w) c.re = false; else c.wreck_t = n.wfr / 255.f * respawn_delay; }
    objs = net_fixed;
    for (int k = 0; k < s.nobj && k < 48; k++) { const NetO &n = s.o[k]; Obj o; o.type = n.type & 3; o.p = V(n.x * 10.f, n.y * 10.f, n.z * 10.f); o.yaw = n.yaw / 255.f * 6.2831853f; o.tumble = n.tum / 255.f * 6.2831853f; objs.push_back(o); }
    if (!fresh && ts == TS_COUNT && nts == TS_PLAY) { sfx(0, TSND_GO, 110); say(1); }
    if (!fresh && ts != TS_OVER && nts == TS_OVER) voices[8].on = false;
    ts = nts; st_t = s.st_t; time_left = s.time_left;
}
static bool net_lev_was = false; static uint8_t net_lev_seq = 0;
/* true = this tick was a linked slave's, inside the master's race */
static bool net_tick(double el, const VPIX_Input &in, bool trig) {
    v1knet::pump();
    if (v1knet::master) { net_master(); net_log(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n >= offsetof(NetSt, o) && v1knet::my_seat >= 0) { static NetSt s; memset(&s, 0, sizeof(s)); memcpy(&s, d, (std::min)(n, sizeof(s))); if (s.ncars >= 2 && s.ncars <= 8 && v1knet::my_seat < s.ncars) net_apply(s, v1knet::round_id); }
        else if (slave_game) net_leave("the master's race is over");
    }
    if (slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
    net_log();
    if (!slave_game || cars.empty() || me < 0 || me >= (int)cars.size()) { v1knet::send_input(NULL, 0); return false; }
    NetIn o; memset(&o, 0, sizeof(o));
    o.steer = clampf(in.joy[0], -1.f, 1.f); o.ped = clampf(in.joy[1] + in.throttle - in.brake, -1.f, 1.f);
    if (autoplay) { float st = 0, th = 0; if (ts == TS_PLAY) t_ai(me, (float)el, st, th); o.steer = st; o.ped = th; }
    bool lev = in.top || in.b2 || (in.buttons & VPIX_BTN_3) != 0; if (lev && !net_lev_was && ts == TS_PLAY) { net_lev_seq++; sfx(0, TSND_LEVER, 90); } net_lev_was = lev;
    o.lever_seq = net_lev_seq; v1knet::send_input(&o, sizeof(o));
    if (ts == TS_COUNT) { int lit = st_t < LIGHT0 ? -1 : (int)((st_t - LIGHT0) / LIGHT_D); if (lit != beeped && lit >= 0 && lit < 3) { beeped = lit; sfx(0, TSND_BEEP, 100); } }
    d1k::hs::run("totald", ts == TS_OVER && st_t > 2.f, (9 - cars[me].rank) * 1000 + (std::min)(999, cars[me].cp / 12 * 10), in, trig, (float)el, autoplay);
    const float dt = 1.f / 120.f; int k = 0;
    while (acc >= dt && k < 24) {
        for (size_t i = 0; i < cars.size(); i++) { Car &c = cars[i]; if (c.wrecked) t_wreck_step((int)i, dt); else { c.p.x += c.v.x * dt; c.p.y += c.v.y * dt; } }
        t_parts_step(dt); acc -= dt; sim_t += dt; st_t += dt; k++; }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); t_engine_sound(); build_frame(++frames, in); }
    return true;
}

static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    static bool trig_was = false, start_was = false, l_was = false, r_was = false, init = false; static int coins_was = 0;
    if (!init) { coins_was = in.coins; init = true; }
    bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f, go = trig && !trig_was; trig_was = trig;
    if (in.coins > coins_was) go = true; coins_was = in.coins;
    if (in.start && !start_was) go = true; start_was = in.start;
    bool lf = in.joy[0] < -0.5f, rt = in.joy[0] > 0.5f, left = lf && !l_was, right = rt && !r_was; l_was = lf; r_was = rt;
    int final_score = cars.empty() ? 0 : (9 - cars[me].rank) * 1000 + (std::min)(999, cars[me].cp / 12 * 10);   /* place decides, as the real game; laps only break a tie (INVENTED) */
    { static bool lev_was = false; bool lev = in.top || in.b2 || (in.buttons & VPIX_BTN_3) != 0 || (GetAsyncKeyState('R') & 0x8000) != 0; if (lev && !lev_was && ts == TS_PLAY && !cars.empty()) { cars[me].gear_rev = !cars[me].gear_rev; sfx(0, TSND_LEVER, 90); } lev_was = lev; }   /* the two-position lever: top button, R in a window */
    if (autoplay && ts != TS_PLAY && ts != TS_COUNT && st_t > 2.f) go = true; if (d1k::hs::run("totald", ts == TS_OVER && st_t > 2.f, final_score, in, trig, (float)el, autoplay)) { go = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (!v1knet::can_start()) go = false;   /* a linked slave: pod 1 starts the race */
    if (v1knet::on && net_tick(el, in, trig)) { PIC_AddEvent(tick, 2.0); return; }
    if (ts == TS_TITLE) {
        if (go) { ts = TS_SELECT; st_t = 0; select_left = SELECT_T; sfx(0, TSND_SELECT, 100); }
        else if (st_t > 50.f) t_start_demo();
    } else if (ts == TS_SELECT) {
        select_left -= (float)el;
        if (left || right) { course_sel ^= 1; select_left = SELECT_T; sfx(0, TSND_SELECT, 90); }
        if ((go && st_t > 0.3f) || select_left <= 0.f) { sfx(0, TSND_GO, 100); t_start_round(); }
    } else if (ts == TS_COUNT) {
        int lit = st_t < LIGHT0 ? -1 : (int)((st_t - LIGHT0) / LIGHT_D); if (lit != beeped && lit >= 0 && lit < 3) { beeped = lit; sfx(0, TSND_BEEP, 100); }
        if (st_t >= LIGHT0 + 3.f * LIGHT_D) { ts = TS_PLAY; st_t = 0; sfx(0, TSND_GO, 110); say(1); }   /* all 4 green, "they're off" */
    } else if (ts == TS_OVER) {
        if ((go && st_t > 2.5f) || st_t > 10.f) t_start_demo();
    }
    int n = 0;
    while (acc >= dt && n < 24) { t_world(in, dt); acc -= dt; sim_t += dt; st_t += dt; n++; }
    if (frame_acc >= 1.0 / game_fps) {
        frame_acc = fmod(frame_acc, 1.0 / game_fps);
        t_engine_sound();
        build_frame(++frames, in);
    }
    if (sim_t >= next_log && !cars.empty()) { next_log = sim_t + 2.0; const Car &q = cars[me];
        dl("t=%.0f st %d course %d me (%.0f %.0f z %.0f) v %.1f dmg %.0f idx %d cp %d place %d air %d | places %d/%d/%d/%d/%d/%d/%d/%d | left %.0f",
           sim_t, (int)ts, course, q.p.x, q.p.y, q.p.z, len(q.v) / 1000.f, q.damage, q.idx, q.cp, q.rank, q.air ? 1 : 0,
           cars[0].rank, cars.size() > 1 ? cars[1].rank : 0, cars.size() > 2 ? cars[2].rank : 0, cars.size() > 3 ? cars[3].rank : 0, cars.size() > 4 ? cars[4].rank : 0, cars.size() > 5 ? cars[5].rank : 0, cars.size() > 6 ? cars[6].rank : 0, cars.size() > 7 ? cars[7].rank : 0, time_left); }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".totald").c_str(), "w");
    game_fps = envf("TOTALD_FPS", game_fps); game_len = envf("TOTALD_TIME", game_len); respawn_delay = envf("TOTALD_RESPAWN", respawn_delay);
    vmax = envf("TOTALD_VMAX", vmax); accel_max = envf("TOTALD_ACCEL", accel_max); dmg_scale = envf("TOTALD_DAMAGE", dmg_scale); bump_e = envf("TOTALD_BUMP", bump_e);
    n_cpu = (int)clampf(envf("TOTALD_CPU", (float)n_cpu), 1.f, 7.f); cpu_skill = envf("TOTALD_SKILL", cpu_skill); road_half = envf("TOTALD_ROAD", 0.f);
    course_sel = (int)envf("TOTALD_COURSE", (float)course_sel) & 1; autoplay = envf("TOTALD_AUTO", 0) != 0; wreck_me_at = envf("TOTALD_WRECKME", 0); start_at = (int)envf("TOTALD_START", -1);
    t_build_meshes(); t_make_hud(); t_make_sounds();
    v1knet::init("TOTD", false, dl);   /* start-together race (join in progress is for Flying Aces and VTOL only) */
    t_start_demo();
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    PIC_AddEvent(tick, 2.0);
    dl("Total Destruction rebuild started");
}
}   /* namespace totald */
