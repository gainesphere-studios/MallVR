/* Dactyl Nightmare (Virtuality 1000CS, 1991) - native rebuild from footage. No 1000-series software survives in public. Included after vpix_buggy.h; switched on by VPIX_NATIVE=dactyl1k. Dev only, not in a release.
   Evidence: Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\dactyl.md (what changed, with times), REMAKE_GUIDE_2026-11-02.md, research_2026-11-02\06 + 07.
   Units: mm, seconds. World X right, Y forward, Z up. The camera is the player's head (in_head), so VR late correction applies.
   GAME FILES READ AT RUN TIME (the one rebuild that does; look at this before any release; Bobby 11-03: "wire all the sounds from 2000 to this one", the same game): from the later DAC 2000 disc, folder DAC\SOUNDS, never embedded, no copy in the repo or a package, each category falling back to the made sound (or silence) when its files are absent. See load_bank and snd_amb: DACTWEP5/7/8/9 (shot), HIT1-3A (bird shot down), EXPLO1-4 and BODHIT01-03 (a player breaks up), OUCH03-08 and FEMOUCH1-7 (hurt), BODFALL1-4 (landing), FSCREAM3-5 (let go by the bird), FALL01-04 (off the edge), ORIGFT1A/B and FOOT1/2 (steps), FLAP3 and DACTLOOP (bird), HUNGRY1-4, COMING1-3, TIMTODIE and TIMTODI2 (warning, swoop, start), HRTBEAT3 (bird has you marked), LIFTACC, LIFTLOOP and LIFTEND, GAMEEND, VS01-23 and their A takes (the rival's taunts and verdict). Unused on purpose: PORTAL*, PORT*, TELEPORT, NEWEAP*, NUDACT*, DACTLOOP-less bird variants, SELECT*, LSTARTUP, DN_INSTR, VR28, S06, LIFT02, LIFTSTOP, FOOTLEFT/RITE, DNFUT*, VS16/17 (time warp).
   FROM FOOTAGE (ABC 1991 hBkP2to1P_c / NNK536j-V2Q, GamePro REl9qOEebcA, GamesMaster Y7mtN7Sxk-E): the five platforms and their tile plans, four flights of 8 steep steps, centre frame with its six legs, gold cones, plates that shuttle between neighbouring raised platforms, the 0:00 shot over the arena, HUD (hollow digits, box and bird icon blinking together, 0.7 s), figure, gun with a ball tip, your own pieces seen from above when shot, black screen after the fall, landing on the centre board, about 1 m/s walk (rough). Shot speed and drop: measured on two shots, ABC.
   FROM DAC 2000, LATER VERSION (footage silent, or agreeing; shapes in vpix_dactyl1k_dac.h, no model file is read): ring = BLOCK; arch = ARCPIL + ARCHTOP; shot and gun tip = BULLET; plate underside = LIFT; bird = NEWDACTY parts at DACTYPE.DBS offsets; lift routes and the 5 s wait (DAC.EXE, Pearl); 8 steps per flight; the three voices above; bird after 10 shots (DEFAULTS.DN2, as the manual).
   PRESS: small area and short-range lobbed shot, footsteps, smash on landing, ducking (Video Games & Computer Entertainment 42, Jul 1992); shot down at the last second (Electronic Entertainment 1, Jan 1994); a ping for the elevator, "Birdy's hungry!" (W.review 1991).
   MANUAL: bird after 10 shots (GamesMaster's commentary says 8: D1K_BIRD_SHOTS=8), 180 s, F7/F8 modes, edge detection.
   INVENTED: absolute sizes are scaled from a 1.8 m figure; the black time after being shot, which bird comes, shot, hit, flap, step and thud sounds (synth, no footage match), plate colours beyond what ABC shows, all Capture the Flag rules, the computer players' route. Standing on a plate: nobody is filmed on one, it rests on Bobby's memory and the later games' lift code.
   Env: D1K_WALK, D1K_SHOT, D1K_SHOTG, D1K_RESPAWN, D1K_BIRD_SHOTS, D1K_FPS, D1K_CPU, D1K_TIME, D1K_EDGE, D1K_LIFT, D1K_MODE, D1K_CAPTURE, D1K_AUTO, D1K_CAM. */
#include <functional>
#include <algorithm>
#include "vpix_dactyl1k_dac.h"   /* shapes copied from DAC 2000 as arrays; no model file is read */
#include "vpix_1k_net.h"         /* link play, 2-4 pods: see the "link play" block below */
#ifdef _WIN32
#include <direct.h>
#endif
namespace d1k {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at;

static FILE *dlog = NULL;
static void dl(const char *fmt, ...) {
    if (!dlog) return;
    va_list ap; va_start(ap, fmt); vfprintf(dlog, fmt, ap); va_end(ap); fputc('\n', dlog); fflush(dlog);
}
static float envf(const char *n, float def) { const char *e = getenv(n); return e && *e ? (float)atof(e) : def; }
static std::mt19937 rng(1991);
static float rnd(float a, float b) { return a + (b - a) * (rng() / (float)(rng.max)()); }

/* ---- tuning ---- */
/* Size: footage, not the later disc. DAC's PLAT50/PLAT25 are 50000/25000 units, but its player is about 3400 units tall, so its unit is about 0.53 mm and those slabs are about 26 m and 13 m;
   and that is the 2000-series game. Our tile is 1 m from a 1.8 m figure standing on it (ABC 0:56), which gives an 8 x 8 tile centre and 8 x 6 raised platforms. Press: "like playing in someone's living room". */
static const float TILE = 1000.f, RAISE = 3800.f, EYE_DESK = 1700.f;
static const float WC = 4000.f, RW = 4000.f, RD = 3000.f, GAP = 3000.f, SW = 650.f, DP = WC + GAP + RD;   /* centre half width; raised half width and half depth; stair run; stair half width; centre to raised centre */
static const float GRAV = 19600.f;                         /* Waldern, ABC 1991: gravity doubled. Falling bodies only: the shot has its own, measured below */
static float walk_speed = 1000.f;                          /* rough: opponent 0.9 m/s ABC 2:53-2:51, 0.6-0.8 on the stair GamesMaster 0:22-0:18 */
static float shot_speed = 7000.f;                          /* measured, two shots tracked frame by frame in ABC 1991 (clock 1:55-1:54, 1:48-1:47): 6-8 m/s, a faceted ball shrinking to the target in 0.6-0.9 s at about 5.6 m. Bobby: "shot a bit more straight" */
static float shot_grav = 3000.f;                           /* measured on the same two shots: about 1.4 and 4.9 m/s2, far below the doubled gravity. Still a lob at range, which is what VGCE 42 calls it */
static float fire_gap = 2.f;                               /* Kotaku 2016, one source; Bobby: 2 s is fine */
static float respawn_delay = 3.5f;                         /* pieces about 2 s (ABC 2:22, GamePro 0:46-0:44), then black. Length of the black is a reading */
static int bird_shots = 10;                                /* the operator manual's default and the 1991 talk; one TV commentary says 8 */
static float game_len = 180.f;                             /* 3:00 default */
static float game_fps = 20.f;                              /* measured: 20 at best on clean footage */

/* ---- meshes ---- */
static const uint32_t MH = 0x7D000000u, BMPH = 0x7D100000u;
enum { M_ARENA = 1, M_STARS, M_GUN, M_DART, M_SPARK_R, M_SPARK_O, M_SPARK_G, M_BIRD, M_WING_L, M_WING_R, M_GUNF, M_OWING_L, M_OWING_R, M_DISC /* + 0-2 */,
       M_PLATE = 60 /* + 0-2 */, M_FLAG = 64 /* + team */, M_AXE = 66, M_XBOW = 67, M_BODY = 20 /* + 8 * colour set: 0 torso, 1 head, 2 thigh L, 3 thigh R, 4 shin L, 5 shin R, 6 arm L, 7 upper arm R */, M_FOREARM = 52 /* + colour set */ };
struct Mesh {
    std::vector<float> v; std::vector<VPIX_MeshPoly> p;
    uint32_t vert(V a) { v.push_back(a.x); v.push_back(a.y); v.push_back(a.z); return (uint32_t)(v.size() / 3 - 1); }
    void poly(std::initializer_list<V> c, uint32_t rgb) {
        VPIX_MeshPoly q; q.flags = 0x20000000u | ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | (rgb >> 16); int n = 0; V a[8];   /* flat colours are stored BGR */
        for (V x : c) { a[n] = x; q.idx[n] = vert(x); n++; }
        q.n = (uint8_t)n; V nn = norm(cross(a[1] - a[0], a[2] - a[0])); q.nx = nn.x; q.ny = nn.y; q.nz = nn.z;
        p.push_back(q);
    }
    void box(V lo, V hi, uint32_t top, uint32_t side, uint32_t bottom) {
        V a(lo.x, lo.y, lo.z), b(hi.x, lo.y, lo.z), c(hi.x, hi.y, lo.z), d(lo.x, hi.y, lo.z);
        V e(lo.x, lo.y, hi.z), f(hi.x, lo.y, hi.z), g(hi.x, hi.y, hi.z), h(lo.x, hi.y, hi.z);
        poly({e, f, g, h}, top); poly({d, c, b, a}, bottom);
        poly({a, b, f, e}, side); poly({b, c, g, f}, side); poly({c, d, h, g}, side); poly({d, a, e, h}, side);
    }
    void box(V lo, V hi, uint32_t rgb) { box(lo, hi, rgb, rgb, rgb); }
    void octa(V base, float r, float h, uint32_t rgb) {   /* an 8-sided pillar */
        static const float a = 0.7071f; const float cx[8] = {1, a, 0, -a, -1, -a, 0, a}, cy[8] = {0, a, 1, a, 0, -a, -1, -a};
        for (int i = 0; i < 8; i++) { int j = (i + 1) & 7;
            poly({base + V(cx[i] * r, cy[i] * r, 0), base + V(cx[j] * r, cy[j] * r, 0), base + V(cx[j] * r, cy[j] * r, h), base + V(cx[i] * r, cy[i] * r, h)}, rgb); }
        V t = base + V(0, 0, h);
        poly({t + V(cx[0] * r, cy[0] * r, 0), t + V(cx[1] * r, cy[1] * r, 0), t + V(cx[2] * r, cy[2] * r, 0), t + V(cx[3] * r, cy[3] * r, 0),
              t + V(cx[4] * r, cy[4] * r, 0), t + V(cx[5] * r, cy[5] * r, 0), t + V(cx[6] * r, cy[6] * r, 0), t + V(cx[7] * r, cy[7] * r, 0)}, rgb);
    }
    bool add(int id) { return VPIX_Render_AddMesh(MH + id, v, p); }
};

/* colours read from hBkP2to1P_c (the cleanest ABC copy), checked against NNK536j-V2Q (washed, reds go pink) and GamesMaster */
static const uint32_t C_GREY = 0x8C8A88, C_DARK = 0x4A4A4A, C_WHITE = 0xE6E6E0, C_RED = 0xF0401C, C_BLUE = 0x2C2870, C_ORANGE = 0xF08A16, C_ORANGE2 = 0xC06C12,
                      C_YELLOW = 0xE6D23C, C_PALE = 0xE8E0A0, C_GREEN = 0x2FA84A, C_PINK = 0xD880A8, C_BIRD = 0x3F8A38, C_BIRD2 = 0x2E6E2C, C_GOLD = 0xC8A860, C_RING2 = 0xB42C12;

struct Plat { float cx, cy, hx, hy, z; bool red; int stair_dir; };   /* stair_dir: the way the stair leaves a raised platform toward the centre: 0 -y, 1 +y, 2 -x, 3 +x */
struct Stair { float x0, y0, x1, y1; bool along_y; float z0, z1; };   /* z0 at the low coordinate end */
struct Pillar { float x, y, z, r, h; };
static std::vector<Plat> plats;
static std::vector<Stair> stairs;
static std::vector<Pillar> pillars;
/* The plates at the yellow triangles are lifts between NEIGHBOURING raised platforms. ABC 0:00 (hBkP2to1P_c t40-44) shows a plate leave the north platform's east edge and head for the east platform's north edge, and one cross from west to north.
   The later DAC.EXE (2000 series) has the same four routes in its table: (15000,30500)-(30500,15000) and its rotations, 5 s at each end, then off by itself; Pearl's turtles run the same way (LIFT_init_lifts). Bobby remembers them so, first hand.
   Two to a route, one from each end: ABC shows more plates in the air than DAC's one a route. Lime at rest, pink or green moving (ABC). 3.4 m/s measured (ABC); DAC's 7500 units/s is about 4 m/s. */
struct Lift { V p, home; int col; bool rider, armed; int show; };
struct LiftPair { int a, b; bool moving; float t, dur, arc; V bulge; float rest; };
static std::vector<Lift> lifts;
static std::vector<LiftPair> lift_pairs;
static const float LIFT_H = 650.f;
static float lift_speed = 3400.f;
static bool shootout_look = true;   /* false in Capture the Flag: GamesMaster's match has a spinning disc, coloured cones and pads */
static void kit_bar(Mesh &m, V a, V b, float hu, float hw, uint32_t c);
static void kit_slab(Mesh &m, std::initializer_list<V> c, V t, uint32_t face, uint32_t side);
static uint32_t kit_shade(uint32_t c, float f);

/* a quad whose winding is turned to face 'want' */
static void face4(Mesh &m, V a, V b, V c, V d, V want, uint32_t col) { if (dot(cross(b - a, c - a), want) < 0) m.poly({d, c, b, a}, col); else m.poly({a, b, c, d}, col); }
/* eight sides from radius r0 at c to r1 at c + h */
static void frustum(Mesh &m, V c, float r0, float r1, float h, uint32_t rgb) {
    static const float a = 0.7071f; const float cx[8] = {1, a, 0, -a, -1, -a, 0, a}, cy[8] = {0, a, 1, a, 0, -a, -1, -a}; V t = c + V(0, 0, h);
    for (int i = 0; i < 8; i++) { int j = (i + 1) & 7;
        m.poly({c + V(cx[i] * r0, cy[i] * r0, 0), c + V(cx[j] * r0, cy[j] * r0, 0), t + V(cx[j] * r1, cy[j] * r1, 0), t + V(cx[i] * r1, cy[i] * r1, 0)}, rgb); }
}
/* a chamfered column on a flared foot (ABC 2:52, 1:27) */
/* a part of DAC 2000 (see vpix_dactyl1k_dac.h) put through 'at'; 'paint' gets the world normal and the model's colour. thick > 0 gives a one-sided sheet a real back and edges (Bobby: no paper-thin parts) */
static void dac_add(Mesh &m, const dac::Part &P, const std::function<V(V)> &at, const std::function<uint32_t(V, uint32_t)> &paint, bool flip = false, const std::function<bool(V)> &keep = nullptr, float thick = 0.f) {
    const unsigned short *ix = P.ix;
    auto emit = [&](const std::vector<V> &q, uint32_t rgb) { VPIX_MeshPoly o; o.flags = 0x20000000u | ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | ((rgb >> 16) & 0xFF); int n = 0;
        for (const V &x : q) { if (n < 16) o.idx[n++] = m.vert(x); } o.n = (uint8_t)n; V nn = norm(cross(q[1] - q[0], q[2] - q[0])); o.nx = nn.x; o.ny = nn.y; o.nz = nn.z; m.p.push_back(o); };
    for (int i = 0; i < P.np; i++) { int n = *ix++; std::vector<V> q;
        for (int k = 0; k < n; k++) { const float *w = P.v + 3 * ix[k]; q.push_back(at(V(w[0], w[1], w[2]))); }
        ix += n; if (flip) std::reverse(q.begin(), q.end());
        V nn = norm(cross(q[1] - q[0], q[2] - q[0])); if (keep && !keep(nn)) continue;
        uint32_t c = paint(nn, P.col[i]); emit(q, c);
        if (thick > 0.f) { V dn = (nn.z > 0 ? V(0, 0, -thick) : V(0, 0, thick)); std::vector<V> b; for (int k = n - 1; k >= 0; k--) b.push_back(q[k] + dn); emit(b, kit_shade(c, 0.7f));
            for (int k = 0; k < n; k++) { int j = (k + 1) % n; std::vector<V> w = {q[j], q[k], q[k] + dn, q[j] + dn}; emit(w, kit_shade(c, 0.6f)); } } }
}
static void pillar(Mesh &m, V at, float h = 2500.f, float r = 170.f) {
    frustum(m, at, r * 2.1f, r * 2.1f, 70.f, C_ORANGE2); frustum(m, at + V(0, 0, 70), r * 2.1f, r, 190.f, C_ORANGE);
    frustum(m, at + V(0, 0, 260), r, r, h - 400.f, C_ORANGE); frustum(m, at + V(0, 0, h - 140.f), r, r * 1.5f, 140.f, C_ORANGE2);
    pillars.push_back({at.x, at.y, at.z, r, h});
}
/* a thick beam along e, half length 'half', half thickness 'th' along n, flat top, the underside cut into 'scallops' round arches (one on a platform's arch, two on the centre frame) */
static void arched_beam(Mesh &m, V c, V e, V n, float half, float th, float spring, float rise, float top, int scallops) {
    const int N = 6 * scallops; V up(0, 0, 1); V pa; float za = 0;
    for (int i = 0; i <= N; i++) {
        float a = -half + 2.f * half * i / N, u = fmodf((float)i * scallops / N, 1.f), z = spring + rise * sinf(u * 3.14159265f);
        V p = c + e * a;
        if (i > 0) {
            V pl = pa + V(0, 0, za), pr = p + V(0, 0, z), tl = pa + V(0, 0, top), tr = p + V(0, 0, top);
            face4(m, pl + n * th, pr + n * th, tr + n * th, tl + n * th, n, C_ORANGE);
            face4(m, pl - n * th, pr - n * th, tr - n * th, tl - n * th, n * -1.f, C_ORANGE);
            face4(m, pl + n * th, pr + n * th, pr - n * th, pl - n * th, up * -1.f, C_ORANGE2);
            face4(m, tl + n * th, tr + n * th, tr - n * th, tl - n * th, up, C_ORANGE);
        }
        pa = p; za = z;
    }
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; V p = c + e * (q * half);
        face4(m, p + n * th + V(0, 0, spring), p - n * th + V(0, 0, spring), p - n * th + V(0, 0, top), p + n * th + V(0, 0, top), e * q, C_ORANGE2); }
}
/* the ring, Bobby's "donut": DAC 2000's BLOCK (later version), a thick chamfered square with a square window, set on the floor at the footage's height. Footage (ABC 1:12-1:08, 0:27, 0:00 and t92) shows it nearly square, thick, and standing still: 07 took it for something held. It stays red on ABC and GamePro; Piku's machine cycled its colour, GamesMaster's spun a disc */
static void ring(Mesh &m, V c, V a, V n, uint32_t col, uint32_t col2) {
    const float H = 2300.f, K = H / 3142.f; V up(0, 0, 1);
    dac_add(m, dac::BLOCK, [&](V w) { return c + a * (w.x * K) + n * (w.y * K) + up * (w.z * K + H * 0.5f); }, [&](V nn, uint32_t) { return fabsf(dot(nn, n)) > 0.7f ? col : col2; }, dot(cross(a, n), up) < 0);
    for (int k = 0; k < 2; k++) { float s = k ? 1.f : -1.f; V at = c + a * (s * 800.f); pillars.push_back({at.x, at.y, at.z, 380.f, H}); }
}
static void cone(Mesh &m, V at, uint32_t rgb) {   /* about a tile wide and taller than that (ABC 1:31, 1:27) */
    const float r = 450.f, h = 1100.f; const int N = 8; V top = at + V(0, 0, h);
    for (int i = 0; i < N; i++) { float a = 6.2831853f * i / N, b = 6.2831853f * (i + 1) / N;
        m.poly({at + V(cosf(a) * r, sinf(a) * r, 0), at + V(cosf(b) * r, sinf(b) * r, 0), top}, rgb); }
}
static void platform(Mesh &m, const Plat &P) {
    const float T = P.z, TH = 300.f, a0 = 12.f;
    m.box(V(P.cx - P.hx, P.cy - P.hy, T - TH), V(P.cx + P.hx, P.cy + P.hy, T), C_GREY, C_DARK, C_DARK);
    /* tile plan from ABC 0:00 and 0:13: blocks 3 wide either side of a one-tile grey aisle. Raised: 3 x 2 blocks, long edge to the stair. Centre: 3 x 3 blocks, the inner corner tile left grey */
    const bool along_y = P.stair_dir < 2; const int rows = P.red ? 2 : 3;
    for (int a = -3; a <= 3; a++) for (int b = -rows; b <= rows; b++) {
        if (a == 0 || b == 0) continue;
        int i = abs(a) - 1, j = abs(b) - 1;
        if (!P.red && i == 0 && j == 0) continue;
        uint32_t c = P.red ? (((i + j) & 1) ? C_RED : C_WHITE) : (((i + j) & 1) ? C_WHITE : C_BLUE);
        float u = a * TILE, w = b * TILE;
        float x = P.cx + (along_y ? u : w) - TILE * 0.5f, y = P.cy + (along_y ? w : u) - TILE * 0.5f;
        m.poly({V(x, y, T + a0), V(x + TILE, y, T + a0), V(x + TILE, y + TILE, T + a0), V(x, y + TILE, T + a0)}, c);
    }
    if (!P.red) return;   /* the centre board has stairs on every side */
    /* yellow triangles at the middle of the two side edges, pointing out, each with a plate at it; the far edge has none (ABC 0:00, 0:13; VGCE 42 p.80) */
    const float tz = T + 14.f, ts = 340.f;
    for (int k = 0; k < 4; k++) {
        if (k == P.stair_dir || k == (P.stair_dir ^ 1)) continue;
        V d = k == 0 ? V(0, -1, 0) : k == 1 ? V(0, 1, 0) : k == 2 ? V(-1, 0, 0) : V(1, 0, 0), s(-d.y, d.x, 0);
        V e = V(P.cx, P.cy, tz) + V(d.x * (P.hx - 280.f), d.y * (P.hy - 280.f), 0);
        m.poly({e + d * ts, e + s * ts, e - s * ts}, C_YELLOW);
        V pc = V(P.cx, P.cy, T - 20.f) + V(d.x * (P.hx + LIFT_H), d.y * (P.hy + LIFT_H), 0);
        int col = (k == 0 || k == 2) ? 0 : 1;
        lifts.push_back({pc, pc, col, false, true, 2});
    }
}
static void stair_mesh(Mesh &m, const Stair &s) {
    const int N = 8;   /* 8: counted on ABC 2:48 and 0:00 (7-8; the older "about 12" was a misread), and DAC 2000's STAIR has 8. Treads light, risers and sides darker; solid saw-tooth from the side (ABC 2:49, 1:13) */
    float L = s.along_y ? s.y1 - s.y0 : s.x1 - s.x0, step = L / N, dz = (s.z1 - s.z0) / N, deep = fabsf(dz) + 260.f;
    for (int i = 0; i < N; i++) {
        float a = (s.along_y ? s.y0 : s.x0) + step * i, b = a + step, top = s.z0 + dz * (i + 0.5f);
        if (s.along_y) m.box(V(s.x0, a, top - deep), V(s.x1, b, top), C_ORANGE, C_ORANGE2, C_ORANGE2);
        else m.box(V(a, s.y0, top - deep), V(b, s.y1, top), C_ORANGE, C_ORANGE2, C_ORANGE2);
    }
}
/* the player's gun, the only one in the Shootout: a grey shaft narrowing from the hand, a pale band part way up, then flaring to a big square head with a dark muzzle (ABC 0:48 large, hBkP2to1P_c t60; 1:54-1:45). GamePro calls it a spring pistol that shoots suction-tip darts. Hand at the origin, y forward */
static void gun_part(Mesh &g) {
    kit_bar(g, V(0, 20, 0), V(0, 170, 0), 50.f, 50.f, 0x8C8C8C);
    kit_bar(g, V(0, 170, 0), V(0, 300, 0), 38.f, 38.f, 0x989898);
    g.box(V(-42, 300, -42), V(42, 322, 42), 0xE0E0E0, 0xC0C0C0, 0xA8A8A8);
    kit_bar(g, V(0, 322, 0), V(0, 430, 0), 28.f, 28.f, 0x9C9C9C);
    /* the tip: a black and white faceted ball on the end of the shaft. Opponent's gun, ABC 1:06-1:04 (t36-37); your own from behind, ABC t92, reads as white and dark facets. The shape is DAC 2000's BULLET (later version): Bobby's "rocket tip" */
    dac_add(g, dac::BULLET, [&](V w) { return V(w.x * 0.75f, 240.f + w.y * 0.75f, w.z * 0.75f); }, [&](V, uint32_t c) { return c ? 0xE0E0E0u : 0x585858u; });   /* mid grey, not black: ABC 0:48 close up */
}
static void arm_part(Mesh &g) {   /* first person: pale hand and thick forearm from the lower right */
    g.box(V(-60, -60, -62), V(60, 22, 50), 0xE0B090, 0xC89878, 0xB08868);
    g.box(V(-58, -430, -110), V(58, -60, 10), 0xE0B090, 0xC89878, 0xB08868);
    g.box(V(-62, -490, -116), V(62, -420, 16), 0xE8E8E8);
}
/* the bird: DAC 2000's NEWDACTY parts and DACTYPE.DBS offsets (later version; span 9 m, which is what the footage gave), with the sheet wings given a back (Bobby: no paper-thin parts) and footage's green. Bobby's call, from memory: no extra flaps, so the 3 triangles per wing part that made a saw-toothed back edge (read from GamesMaster 0:41, ABC 0:22) are gone. BOD has no fins, only a short forked tail end (130 mm), kept */
static void bird_meshes() {
    auto green = [](V nn, uint32_t) { return nn.z > 0.3f ? C_BIRD : C_BIRD2; }; auto off = [](V o) { return [o](V w) { return w + o; }; };
    Mesh b; const V neck(0, 1485, 84), head = neck + V(0, 204, 21), jaw = head + V(0, 126, -20);
    dac_add(b, dac::BOD, off(V(0, 0, 0)), green); dac_add(b, dac::NECK, off(neck), green); dac_add(b, dac::HEAD, off(head), green); dac_add(b, dac::JAW, off(jaw), green);
    dac_add(b, dac::FUT, off(V(-290, -189, 0)), green); dac_add(b, dac::FUT, off(V(290, -189, 0)), green);
    b.add(M_BIRD);
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; Mesh in, ot;
        dac_add(in, k ? dac::INR : dac::INL, off(V(0, 0, 0)), green, false, nullptr, 50.f);
        dac_add(ot, k ? dac::OWR : dac::OWL, off(V(0, 0, 0)), green, false, nullptr, 50.f);
        in.add(k ? M_WING_R : M_WING_L); ot.add(k ? M_OWING_R : M_OWING_L); }
}
static void tapered(Mesh &m, float hx0, float hy0, float z0, float hx1, float hy1, float z1, uint32_t c, uint32_t c2) {
    V a(-hx0, -hy0, z0), b(hx0, -hy0, z0), g(hx0, hy0, z0), d(-hx0, hy0, z0), e(-hx1, -hy1, z1), f(hx1, -hy1, z1), h(hx1, hy1, z1), i(-hx1, hy1, z1);
    m.poly({e, f, h, i}, c); m.poly({d, g, b, a}, c2); m.poly({a, b, f, e}, c); m.poly({b, g, h, f}, c2); m.poly({g, d, i, h}, c); m.poly({d, a, e, i}, c2);
}
static void build_arena() {
    plats.clear(); stairs.clear(); pillars.clear(); lifts.clear(); lift_pairs.clear();
    plats.push_back({0, 0, WC, WC, 0, false, -1});
    plats.push_back({0, DP, RW, RD, RAISE, true, 0}); plats.push_back({0, -DP, RW, RD, RAISE, true, 1});
    plats.push_back({DP, 0, RD, RW, RAISE, true, 2}); plats.push_back({-DP, 0, RD, RW, RAISE, true, 3});
    stairs.push_back({-SW, WC, SW, WC + GAP, true, 0, RAISE});
    stairs.push_back({-SW, -WC - GAP, SW, -WC, true, RAISE, 0});
    stairs.push_back({WC, -SW, WC + GAP, SW, false, 0, RAISE});
    stairs.push_back({-WC - GAP, -SW, -WC, SW, false, RAISE, 0});
    Mesh m;
    for (const Plat &p : plats) platform(m, p);
    for (const Stair &s : stairs) stair_mesh(m, s);
    /* the centre's frame: beams with two round cuts on all four sides; legs at the corners, and a third leg halfway along the east and west sides only: three in a row at ABC 2:52 and GamesMaster 0:56, none in the middle at GamesMaster 0:29 */
    const float F = 2250.f, H = 2900.f, TH = 170.f;
    for (int i = 0; i < 4; i++) pillar(m, V(i < 2 ? -F : F, (i & 1) ? -F : F, 0), H - 300.f);
    pillar(m, V(F, 0, 0), H - 300.f); pillar(m, V(-F, 0, 0), H - 300.f);
    arched_beam(m, V(0, F, 0), V(1, 0, 0), V(0, 1, 0), F + TH, TH, H - 560.f, 260.f, H, 2); arched_beam(m, V(0, -F, 0), V(1, 0, 0), V(0, 1, 0), F + TH, TH, H - 560.f, 260.f, H, 2);
    arched_beam(m, V(F, 0, 0), V(0, 1, 0), V(1, 0, 0), F + TH, TH, H - 560.f, 260.f, H, 2); arched_beam(m, V(-F, 0, 0), V(0, 1, 0), V(1, 0, 0), F + TH, TH, H - 560.f, 260.f, H, 2);
    /* on each raised platform, from the stair: an arch about a tile in, then the ring on the middle, facing the stair (ABC 1:05, 0:13). A ring on the centre too, edge on from the north-south aisle (ABC 0:00) */
    const float AS = 1300.f, AH = 2900.f;
    const V ds[4] = {V(0, 1, 0), V(0, -1, 0), V(1, 0, 0), V(-1, 0, 0)}, ss[4] = {V(1, 0, 0), V(1, 0, 0), V(0, 1, 0), V(0, 1, 0)};
    for (int k = 0; k < 4; k++) { V d = ds[k], s = ss[k], c = d * (DP - RD + 1100.f) + V(0, 0, RAISE);
        { const float K = 0.547f;   /* DAC 2000's ARCPIL and ARCHTOP (later version) at the span the footage gives; it comes out the footage's height, about 2.8 m */
          auto orange = [](V nn, uint32_t) { return nn.z > -0.2f ? C_ORANGE : C_ORANGE2; }; V up(0, 0, 1); bool fl = dot(cross(s, d), up) < 0;
          for (int e = -1; e <= 1; e += 2) { V base = c + s * (AS * e); dac_add(m, dac::ARCPIL, [&](V w) { return base + s * (w.x * K) + d * (w.y * K) + up * ((w.z + 2210.f) * K); }, orange, fl); pillars.push_back({base.x, base.y, base.z, 130.f, 4420.f * K}); }
          dac_add(m, dac::ARCHTOP, [&](V w) { return c + s * (w.x * K) + d * (w.y * K) + up * ((w.z + 4420.f + 380.f) * K); }, orange, fl); }
        ring(m, d * DP + V(0, 0, RAISE), s, d, shootout_look ? C_RED : 0xD8D8E8, shootout_look ? C_RING2 : 0x3040A0); }
    ring(m, V(0, 0, 0), V(0, 1, 0), V(1, 0, 0), shootout_look ? C_RED : C_ORANGE, shootout_look ? C_RING2 : C_ORANGE2);   /* orange in GamesMaster's match, 0:28-0:27 */
    /* cones on the centre's corner blocks: gold on ABC and GamePro; GamesMaster's match has red and green */
    const float cc = 3000.f; const uint32_t c0 = shootout_look ? C_GOLD : C_RED, c1 = shootout_look ? C_GOLD : 0x30D040;
    cone(m, V(cc, cc, 0), c0); cone(m, V(-cc, cc, 0), c0); cone(m, V(cc, -cc, 0), c1); cone(m, V(-cc, -cc, 0), c1);
    if (!shootout_look) {   /* the pale pads with a dark mark on the aisle: only in GamesMaster's match */
        const float pd = 500.f, pz = 14.f; auto pad = [&](float x, float y) { m.poly({V(x - pd, y - pd, pz), V(x + pd, y - pd, pz), V(x + pd, y + pd, pz), V(x - pd, y + pd, pz)}, 0xD0D4D0); m.poly({V(x - 120, y - 120, pz + 4), V(x + 120, y - 120, pz + 4), V(x + 120, y + 120, pz + 4), V(x - 120, y + 120, pz + 4)}, 0x404040); };
        pad(0, 3000.f); pad(0, -3000.f); pad(3000.f, 0); pad(-3000.f, 0);
    }
    m.add(M_ARENA);
    { const uint32_t col[3] = {C_PINK, 0x30B050, 0xC8D840};   /* moving: pink, green; at rest: lime. A grey plate with a bright middle, ABC 0:13 */
      for (int k = 0; k < 3; k++) { Mesh pm; const float R = LIFT_H * 1.05f, K = R * 2.f / 5000.f, A = 0.7071f; const float cx[8] = {1, A, 0, -A, -1, -A, 0, A}, cy[8] = {0, A, 1, A, 0, -A, -1, -A};
          V o[8]; for (int i = 0; i < 8; i++) o[i] = V(cx[i] * R, cy[i] * R, 0);
          pm.poly({o[0], o[1], o[2], o[3], o[4], o[5], o[6], o[7]}, 0x8C8C90);
          /* the underside: DAC 2000's LIFT bowl (later version). Level with your eye it is the long flat grey leaf that crosses ABC 1:50-1:49, the "dart" of the older notes */
          dac_add(pm, dac::LIFT, [&](V w) { return V(w.x * K, w.y * K, w.z * K * 0.6f); }, [](V nn, uint32_t) { return nn.z < -0.8f ? 0x7A7A7Eu : 0x9A9A9Eu; }, false, [](V nn) { return nn.z < 0.5f; });
          const float q = R * 0.5f, z = 8.f; pm.poly({V(q, 0, z), V(0, q, z), V(-q, 0, z), V(0, -q, z)}, col[k]); pm.add(M_PLATE + k); }
      auto at = [&](float x, float y) { int b = 0; float bd = 1e30f; for (size_t i = 0; i < lifts.size(); i++) { float l = fabsf(lifts[i].p.x - x) + fabsf(lifts[i].p.y - y); if (l < bd) { bd = l; b = (int)i; } } return b; };
      const float Q = RW + LIFT_H; float rest = 0.5f;
      auto route = [&](float ax, float ay, float bx, float by) { LiftPair w; w.a = at(ax, ay); w.b = at(bx, by); w.moving = false; w.t = 0; w.dur = 1; w.arc = 200.f; V dir = norm(V(bx - ax, by - ay, 0)); w.bulge = V(-dir.y, dir.x, 0) * (LIFT_H * 1.3f); w.rest = rest; rest += 1.2f; lift_pairs.push_back(w); };
      route(Q, DP, DP, Q); route(DP, -Q, Q, -DP); route(-Q, -DP, -DP, -Q); route(-DP, Q, -Q, DP); }
    dl("arena mesh: %zu polys", m.p.size());

    Mesh st; std::mt19937 r2(7);
    for (int i = 0; i < 260; i++) {
        float z = (r2() / (float)(r2.max)()) * 2.f - 1.f, a = (r2() / (float)(r2.max)()) * 6.2831853f, q = sqrtf(1.f - z * z), R = 400000.f, s = 500.f;
        V c(cosf(a) * q * R, sinf(a) * q * R, z * R), e0 = norm(cross(c, V(0.3f, 0.5f, 0.8f))), e1 = norm(cross(c, e0));
        uint32_t lum = 0x909090 + (r2() % 3) * 0x303030;
        st.poly({c - e0 * s, c - e1 * s, c + e0 * s, c + e1 * s}, lum);
    }
    st.add(M_STARS);

    { Mesh g; gun_part(g); arm_part(g); g.add(M_GUN); }
    { Mesh g; gun_part(g); g.add(M_GUNF); }
    /* Capture the Flag's weapons: a crossbow and a "chopper" axe. GamesMaster's match is the only footage: curved brown bow, long stock, green bolt, thin string */
    { Mesh a; a.box(V(-22, -10, -70), V(22, 620, -20), 0x6A4A2A);
      kit_slab(a, {V(0, 520, -20), V(0, 680, 220), V(0, 400, 220)}, V(14, 0, 0), 0xC8C8D0, 0xA0A0A8);
      arm_part(a); a.add(M_AXE); }
    { Mesh c; const uint32_t BR = 0x5A3A22; kit_bar(c, V(0, -260, -10), V(0, 560, -10), 34.f, 26.f, BR);
      V prev(-620, 300, 10); for (int i = 1; i <= 8; i++) { float u = i / 8.f, x = -620.f + 1240.f * u, y = 300.f + 300.f * sinf(u * 3.14159265f); V p(x, y, 10); kit_bar(c, prev, p, 20.f, 18.f, BR); prev = p; }
      kit_bar(c, V(-620, 300, 16), V(0, 40, 16), 5.f, 5.f, 0xB0B0B0); kit_bar(c, V(620, 300, 16), V(0, 40, 16), 5.f, 5.f, 0xB0B0B0);
      kit_bar(c, V(0, 40, 30), V(0, 480, 30), 16.f, 16.f, 0x30D050); c.box(V(-40, 40, 10), V(40, 150, 56), 0x30D050);
      c.add(M_XBOW); }
    for (int k = 0; k < 2; k++) { Mesh w; uint32_t col = k ? C_RED : 0x3050D8;
      w.box(V(-30, -30, 0), V(30, 30, 2400), 0xE6E6E0);
      kit_slab(w, {V(0, 0, 2400), V(0, 0, 1700), V(900, 0, 2050)}, V(0, 14, 0), col, col);
      w.add(M_FLAG + k); }
    /* the shield of GamesMaster's match: a round pale disc that swings with the hand, its rim going through colours (Andy's view 0:37-0:28); Black Ice 1 and Mondo 2000 6 list shields */
    { const uint32_t rim[3] = {0x30D050, 0xD02818, 0x3050D8};
      for (int k = 0; k < 3; k++) { Mesh c; const int N = 16; const float R = 430.f, Rr = 340.f, TK = 30.f; V z(-260, 240, 70);
          for (int i = 0; i < N; i++) { float a = 6.2831853f * i / N, b = 6.2831853f * (i + 1) / N; V p0(cosf(a), 0, sinf(a)), p1(cosf(b), 0, sinf(b));
              V a0 = z + p0 * R, a1 = z + p1 * R, a2 = z + p1 * Rr, a3 = z + p0 * Rr, f(0, TK, 0);
              face4(c, a0 + f, a1 + f, a2 + f, a3 + f, V(0, 1, 0), rim[k]); face4(c, a0 - f, a1 - f, a2 - f, a3 - f, V(0, -1, 0), rim[k]);
              face4(c, a0 + f, a1 + f, a1 - f, a0 - f, p0 + p1, rim[k]);
              c.poly({z + f * 0.6f, a3 + f * 0.6f, a2 + f * 0.6f}, 0xD8D8D8); c.poly({z - f * 0.6f, a2 - f * 0.6f, a3 - f * 0.6f}, 0xD8D8D8); }
          c.add(M_DISC + k); } }

    Mesh d;   /* the shot: a black and white ball on a short stem, ball first. It looks like the gun's tip (ABC 1:54, 1:48-1:47: faceted pale and dark, shrinking away). Shape: DAC 2000's BULLET (later version) */
    dac_add(d, dac::BULLET, [&](V w) { return V(w.x * 0.6f, (w.y - 400.f) * 0.6f, w.z * 0.6f); }, [&](V, uint32_t c) { return c ? 0xE8E8E8u : 0x202020u; });
    d.add(M_DART);

    const uint32_t sc[3] = {0xE03018, 0xF09020, C_BIRD}; const int sid[3] = {M_SPARK_R, M_SPARK_O, M_SPARK_G};
    for (int k = 0; k < 3; k++) { Mesh s; s.poly({V(-24, 0, -14), V(24, 0, -14), V(0, 0, 48)}, sc[k]); s.poly({V(0, -24, -14), V(0, 24, -14), V(0, 0, 48)}, sc[k]); s.add(sid[k]); }   /* fine specks (ABC 1:46) */

    bird_meshes();

    /* figures: long thin box limbs (ABC 1:05): rust cap with a brim, yellow visor band, shirt narrowing to the waist with sleeves to the elbow, pale forearms, blue jeans, dark boots. Each part has its own origin so it can swing or fly off */
    const uint32_t shirt[4] = {0x9A4020, 0xE6E0D0, 0x3A7A3A, 0xC8A020}, shirt2[4] = {0x7A3018, 0xB8B0A0, 0x2A5A2A, 0x9A7A18}, jeans[4] = {0x46468E, 0x3A4A9A, 0x2A5A2A, 0x6A5A20}, hat[4] = {0x7A3A20, 0x3A2A20, 0x2A5A2A, 0xE0D040};
    for (int s = 0; s < 4; s++) {
        int base = M_BODY + 8 * s; const uint32_t SK = 0xE0B090, SK2 = 0xC89878;
        { Mesh t; tapered(t, 145, 95, -30, 235, 110, 620, shirt[s], shirt2[s]); t.add(base); }             /* torso, origin at the waist */
        { Mesh h; h.box(V(-55, -55, -50), V(55, 55, 30), SK, SK2, SK2);                                    /* neck into the head */
          h.box(V(-90, -95, 0), V(90, 105, 230), SK, SK2, SK2);
          h.box(V(-98, -102, 175), V(98, 108, 265), hat[s]); h.box(V(-85, 100, 175), V(85, 220, 205), hat[s]);   /* cap and brim */
          h.box(V(-102, 30, 90), V(102, 118, 170), 0xE8B020, 0xB88020, 0xB88020); h.add(base + 1); }      /* visor band */
        for (int k = 0; k < 2; k++) { Mesh t2; tapered(t2, 70, 75, -430, 90, 90, 10, jeans[s], kit_shade(jeans[s], 0.8f)); t2.add(base + 2 + k); }   /* thigh */
        for (int k = 0; k < 2; k++) { Mesh sh; tapered(sh, 55, 60, -430, 72, 76, 10, jeans[s], kit_shade(jeans[s], 0.8f));
            sh.box(V(-62, -80, -440), V(62, 170, -370), 0x2A2A2A); sh.add(base + 4 + k); }                  /* shin + boot */
        { Mesh a; a.box(V(-52, -52, -340), V(52, 52, 10), shirt[s], shirt2[s], shirt2[s]); a.box(V(-42, -42, -640), V(42, 42, -330), SK, SK2, SK2); a.add(base + 6); }  /* left arm, hanging */
        { Mesh a; a.box(V(-52, -52, -340), V(52, 52, 10), shirt[s], shirt2[s], shirt2[s]); a.add(base + 7); }   /* right upper arm */
        { Mesh a; a.box(V(-42, -42, -310), V(42, 42, 10), SK, SK2, SK2); a.add(M_FOREARM + s); }           /* right forearm */
    }
}

/* ---- the floors ---- */
static bool floor_at(float x, float y, float z, float &out) {   /* highest surface at x,y no more than a step above z */
    const float STEP = 450.f; bool got = false; float best = -1e9f;
    for (const Plat &p : plats) if (fabsf(x - p.cx) <= p.hx && fabsf(y - p.cy) <= p.hy && p.z <= z + STEP && p.z > best) { best = p.z; got = true; }
    for (const Lift &l : lifts) if (fabsf(x - l.p.x) <= LIFT_H && fabsf(y - l.p.y) <= LIFT_H && l.p.z <= z + STEP && l.p.z > best) { best = l.p.z; got = true; }
    for (const Stair &s : stairs) if (x >= s.x0 && x <= s.x1 && y >= s.y0 && y <= s.y1) {
        float q = s.along_y ? (y - s.y0) / (s.y1 - s.y0) : (x - s.x0) / (s.x1 - s.x0), h = s.z0 + (s.z1 - s.z0) * q;
        if (h <= z + STEP && h > best) { best = h; got = true; } }
    out = best; return got;
}
static void push_out(V &p) {   /* out of the pillars */
    const float R = 300.f;
    for (const Pillar &c : pillars) { if (p.z + 200.f < c.z || p.z > c.z + c.h) continue;
        float dx = p.x - c.x, dy = p.y - c.y, d = sqrtf(dx * dx + dy * dy), m = c.r + R;
        if (d < m && d > 1.f) { p.x = c.x + dx / d * m; p.y = c.y + dy / d * m; } }
}

static bool edge_detect = true;                            /* the manual's operator option; on keeps you on the walkways, D1K_EDGE=0 lets you walk off */
static bool can_stand(float x, float y, float z) { float h; return floor_at(x, y, z, h) && z - h < 700.f; }
static void keep_on(V &p, V was) {   /* slide along an edge instead of walking off it */
    if (!edge_detect || can_stand(p.x, p.y, p.z)) return;
    if (can_stand(p.x, was.y, p.z)) p.y = was.y;
    else if (can_stand(was.x, p.y, p.z)) p.x = was.x;
    else { p.x = was.x; p.y = was.y; }
}

/* ---- players, shots, bursts, the birds ---- */
struct Gib { int part = 0; V p, v, w; M R; };
struct Player {
    bool cpu = false, alive = true, falling = false, xbow = false, dropped = false, here = false; int look = 0, score = 0, shots = 0; uint8_t epoch = 0;   /* epoch: counts teleports, so a link input sent before one is ignored */
    V p;                        /* feet */
    float yaw = 0, aim_pitch = 0, vz = 0, phase = 0, moved = 0, fire_cool = 0, dead_t = 0, dead_total = 1, hit_h = 1800.f;
    int node = -1, from = -1; float think = 0; V spawn; float spawn_yaw = 0; int ride = -1; bool rode = false;
    std::vector<Gib> gibs; float gib_t = 0;
};
static std::vector<Player> pl;
static int me = 0;          /* your seat: 0 except on a linked slave */
struct Shot { V p, v; int owner; float t; M R; };
static std::vector<Shot> shots;
struct Spark { V p, v; float life; int id; };
static std::vector<Spark> sparks;
enum BirdState { B_PATROL, B_SWOOP, B_CARRY, B_LEAVE, B_GONE };
/* two birds: ABC "two prehistoric birds", GamePro "giant pair", VGCE 42 "a few" circling. One takes the shooter when his count comes up; which one is INVENTED */
struct Bird { BirdState s = B_PATROL; V p, v; float a = 0, t = 0, warn = -1; int target = -1; V drop; float gone = 0; };
static Bird birds[2];
static V walk_off;         /* the player's own walking, added to the tracked head */
static bool have_off = false;

static void burst(V at, int n, int id, float speed, float life) {
    for (int i = 0; i < n; i++) { V d = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-0.3f, 1.2f)));
        sparks.push_back({at, d * (speed * rnd(0.3f, 1.f)), life * rnd(0.6f, 1.f), rnd(0, 1) < 0.5f && id == M_SPARK_R ? M_SPARK_O : id}); }
}
static void sfx(int ch, uint32_t key, int vol, int pan = 0) {
    static const bool snd_log = getenv("D1K_SNDLOG") != NULL;   /* test runs are muted: D1K_SNDLOG=1 writes each sound played to the .d1k log */
    if (snd_log && key) { auto it = sound_names.find(key); dl("snd ch%d %s vol %d", ch, it == sound_names.end() ? "?" : it->second.c_str(), vol); }
    buggy::play(ch, (int)key, vol, pan);
}
enum { SND_SHOT = 0x62001, SND_HIT, SND_MISS, SND_WARN, SND_FLAP, SND_THUD, SND_GO, SND_FOOT, SND_COMING, SND_START };
static void make_sound(uint32_t key, float secs, const std::function<float(float)> &wave) {
    VSound s; s.rate = 22050; s.loop = false; size_t n = (size_t)(secs * s.rate); s.pcm.resize(n);
    for (size_t i = 0; i < n; i++) s.pcm[i] = (int16_t)(clampf(wave((float)i / s.rate), -1, 1) * 24000.f);
    sounds[key] = std::move(s); sound_names[key] = "d1k_synth";
}
/* DAC 2000 sound bank (Bobby 11-03: "wire all the sounds from 2000 to this one", same game). Which clip is which: SOUND.DN2 slot table on the disc, DAC.EXE's own LIFT/HEART/FALL/HUNGRY volume names,
   and a local whisper read of the speech (Library\Virtuality Preservation\Dactyl_Nightmare\dac2000_sounds.md). Run-time load only, never embedded; a category with no file left keeps the made sound or is silent. */
enum SCat { C_SHOT, C_HIT, C_EXPLO, C_BODFALL, C_BODHIT, C_HURT, C_SCREAM, C_FALL, C_FOOT, C_FLAP, C_HUNGRY, C_COMING, C_TIME, C_HEART, C_LIFTACC, C_LIFTEND, C_END, C_VS_SCORE, C_VS_DIED, C_VS_GRAB, C_VS_OVER, C_VS_WON, C_VS_2ND, C_VS_3RD, C_VS_LAST, C_N };
static std::vector<uint32_t> bank[C_N]; static uint32_t bank_next = 0x62100, k_dactloop = 0, k_liftloop = 0; static int bank_ok = 0, bank_bad = 0;
static std::vector<std::string> dac_roots() {
    std::vector<std::string> roots; if (const char *e = getenv("D1K_DAC_DIR")) roots.push_back(e);
    roots.push_back("DAC"); roots.push_back("H:\\Games\\Virtuality\\Simon Marston UK Museum\\VR 2000 Series Disc Image\\DAC");
    roots.push_back("H:\\Games\\Virtuality\\release\\MallVR_v0.10.1_WithGames\\games\\VR 2000 Series Disc Image (Simon's)\\DAC");
    return roots;
}
static bool dac_read(uint32_t key, const char *name, bool loop = false) {
    for (const std::string &r : dac_roots()) { std::string p = r + "\\SOUNDS\\" + name; FILE *h = fopen(p.c_str(), "rb"); if (!h) continue;
        fseek(h, 0, SEEK_END); long n = ftell(h); fseek(h, 0, SEEK_SET); std::vector<uint8_t> d(n > 0 ? (size_t)n : 0); size_t got = n > 0 ? fread(d.data(), 1, (size_t)n, h) : 0; fclose(h);
        VSound w; if (got == d.size() && parse_wav(d, w)) { w.loop = loop; sounds[key] = std::move(w); sound_names[key] = name; dl("sound %s read from %s", name, r.c_str()); return true; } }
    return false;
}
/* The three voices: read from the later disc when it's there, never embedded. "Birdy's hungry!" is W.review's wording for the 1991 talk, "it warns you it's coming" Harry Yack's, "Time to Die" W.review's start sample; SOUND.DN2 slots 1-3. No clip has them clear enough to match the waveform (best 0.14), so this is wording and timing, not a sample match */
static void dac_wav(uint32_t key, const char *name) { if (!dac_read(key, name)) dl("sound %s not found, made sound kept", name); }
static void bank_add(int cat, const char *name) { uint32_t k = bank_next++; if (dac_read(k, name)) { bank[cat].push_back(k); bank_ok++; } else { bank_bad++; dl("sound %s missing", name); } }
static void bank_adds(int cat, std::initializer_list<const char *> names) { for (const char *n : names) bank_add(cat, n); }
static uint32_t snd_rng = 12345u;   /* its own dice: the game's own random stream (bird drops, CPU thinking) is not touched */
static uint32_t pk(int cat, uint32_t fallback = 0) {   /* a clip of the category, not the one played last; the made sound when none loaded */
    std::vector<uint32_t> &v = bank[cat]; if (v.empty()) return fallback; if (v.size() == 1) return v[0];
    static uint32_t last[C_N]; uint32_t k; int tries = 0; do { snd_rng = snd_rng * 1664525u + 1013904223u; k = v[(snd_rng >> 16) % v.size()]; } while (k == last[cat] && ++tries < 8);
    last[cat] = k; return k;
}
static float clip_secs(uint32_t k) { auto it = sounds.find(k); return it == sounds.end() || !it->second.rate ? 0.f : (float)it->second.pcm.size() / (float)it->second.rate; }
static void load_bank() {
    bank_adds(C_SHOT, {"DACTWEP5.WAV", "DACTWEP7.WAV", "DACTWEP8.WAV", "DACTWEP9.WAV"});
    bank_adds(C_HIT, {"HIT1.WAV", "HIT2.WAV", "HIT3.WAV", "HIT3A.WAV"});
    bank_adds(C_EXPLO, {"EXPLO1.WAV", "EXPLO2.WAV", "EXPLO3.WAV", "EXPLO4.WAV"});
    bank_adds(C_BODFALL, {"BODFALL1.WAV", "BODFALL2.WAV", "BODFALL3.WAV", "BODFALL4.WAV"});
    bank_adds(C_BODHIT, {"BODHIT01.WAV", "BODHIT02.WAV", "BODHIT03.WAV"});
    bank_adds(C_HURT, {"OUCH03.WAV", "OUCH04.WAV", "OUCH05.WAV", "OUCH06.WAV", "OUCH07.WAV", "OUCH08.WAV", "FEMOUCH1.WAV", "FEMOUCH2.WAV", "FEMOUCH3.WAV", "FEMOUCH4.WAV", "FEMOUCH5.WAV", "FEMOUCH6.WAV", "FEMOUCH7.WAV"});
    bank_adds(C_SCREAM, {"FSCREAM3.WAV", "FSCREAM4.WAV", "FSCREAM5.WAV"});
    bank_adds(C_FALL, {"FALL01.WAV", "FALL02.WAV", "FALL03.WAV", "FALL04.WAV"});
    bank_adds(C_FOOT, {"ORIGFT1A.WAV", "ORIGFT1B.WAV", "FOOT1.WAV", "FOOT2.WAV"});
    bank_adds(C_FLAP, {"FLAP3.WAV"});
    bank_adds(C_HUNGRY, {"HUNGRY1.WAV", "HUNGRY2.WAV", "HUNGRY3.WAV", "HUNGRY4.WAV"});
    bank_adds(C_COMING, {"COMING1.WAV", "COMING2.WAV", "COMING3.WAV"});
    bank_adds(C_TIME, {"TIMTODIE.WAV", "TIMTODI2.WAV"});
    bank_adds(C_HEART, {"HRTBEAT3.WAV"});
    bank_adds(C_LIFTACC, {"LIFTACC.WAV"});
    bank_adds(C_LIFTEND, {"LIFTEND.WAV"});
    bank_adds(C_END, {"GAMEEND.WAV"});
    /* the rival's voice set (VS*.WAV, whisper read; the A takes are shorter, same words): taunts when you score, when you are shot, when the bird takes you, and the result at the end */
    bank_adds(C_VS_SCORE, {"VS01.WAV", "VS01A.WAV", "VS02.WAV", "VS02A.WAV", "VS03.WAV", "VS03A.WAV", "VS04.WAV", "VS04A.WAV", "VS05.WAV", "VS05A.WAV", "VS06.WAV", "VS06A.WAV", "VS07.WAV", "VS07A.WAV"});
    bank_adds(C_VS_DIED, {"VS08.WAV", "VS08A.WAV", "VS09.WAV", "VS09A.WAV", "VS10.WAV", "VS10A.WAV", "VS11.WAV", "VS11A.WAV", "VS14.WAV", "VS14A.WAV", "VS15.WAV", "VS15A.WAV"});
    bank_adds(C_VS_GRAB, {"VS12.WAV", "VS12A.WAV", "VS13.WAV", "VS13A.WAV"});
    bank_adds(C_VS_OVER, {"VS18.WAV", "VS18A.WAV", "VS19.WAV", "VS19A.WAV"});
    bank_adds(C_VS_WON, {"VS20.WAV", "VS20A.WAV"}); bank_adds(C_VS_2ND, {"VS21.WAV", "VS21A.WAV"}); bank_adds(C_VS_3RD, {"VS22.WAV", "VS22A.WAV"}); bank_adds(C_VS_LAST, {"VS23.WAV", "VS23A.WAV"});
    if (dac_read(bank_next, "DACTLOOP.WAV", true)) k_dactloop = bank_next++; else { bank_bad++; dl("sound DACTLOOP.WAV missing"); }
    if (dac_read(bank_next, "LIFTLOOP.WAV", true)) k_liftloop = bank_next++; else { bank_bad++; dl("sound LIFTLOOP.WAV missing"); }
    dl("DAC 2000 sound bank: %d clips loaded, %d missing, loops: bird %d lift %d", bank_ok + (k_dactloop ? 1 : 0) + (k_liftloop ? 1 : 0), bank_bad, k_dactloop != 0, k_liftloop != 0);
}
static void shot_hit_snd(V at) {   /* a shot lands on the floor: the made tick (DAC's HIT clips are 1-4 s, they go to the bird going down), quieter the further from you */
    float d = pl.empty() ? 1e9f : len(at - pl[me].p); if (d > 40000.f) return;
    sfx(6, SND_MISS, (int)clampf(80.f * 8000.f / (std::max)(d, 800.f), 14.f, 80.f));
}
static void fall_start(int i, bool dropped) {   /* off the edge or let go by the bird: the long scream, quieter for the others */
    if (i == me) sfx(9, pk(dropped ? C_SCREAM : C_FALL), 110); else sfx(10, pk(dropped ? C_SCREAM : C_FALL), 45);
}
static void make_sounds() {   /* stand-ins, synthesised. Press gives what they were: echoing footsteps (VGCE 42), wing beats. dac_wav swaps in the voices */
    std::mt19937 r(5); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(SND_SHOT, 0.28f, [&](float t) { return (sinf(6.2831853f * (620.f - 900.f * t) * t) * 0.8f + noise() * 0.25f) * exp(-t * 14.f); });
    make_sound(SND_HIT, 0.9f, [&](float t) { return noise() * exp(-t * 4.f) * 0.9f + sinf(6.2831853f * 70.f * t) * exp(-t * 3.f) * 0.6f; });
    make_sound(SND_MISS, 0.3f, [&](float t) { return noise() * exp(-t * 16.f) * 0.5f; });
    make_sound(SND_WARN, 0.5f, [&](float t) { return (fmodf(t, 0.25f) < 0.16f ? sinf(6.2831853f * 880.f * t) : 0.f) * 0.5f; });
    make_sound(SND_COMING, 0.9f, [&](float t) { return (fmodf(t, 0.15f) < 0.1f ? sinf(6.2831853f * (880.f + 500.f * t) * t) : 0.f) * 0.55f; });
    make_sound(SND_FLAP, 0.55f, [&](float t) { return noise() * sinf(3.14159f * t / 0.55f) * 0.45f; });
    make_sound(SND_THUD, 0.35f, [&](float t) { return sinf(6.2831853f * 55.f * t) * exp(-t * 9.f); });
    make_sound(SND_START, 0.6f, [&](float t) { return sinf(6.2831853f * (440.f + 300.f * t) * t) * exp(-t * 3.f) * 0.6f; });
    make_sound(SND_GO, 0.6f, [&](float t) { return sinf(6.2831853f * (440.f + 300.f * t) * t) * exp(-t * 3.f) * 0.6f; });
    auto step = [&](float t) { return t < 0.f ? 0.f : (sinf(6.2831853f * 95.f * t) * 0.7f + noise() * 0.3f) * exp(-t * 28.f); };
    make_sound(SND_FOOT, 0.6f, [&](float t) { return step(t) * 0.7f + step(t - 0.17f) * 0.3f + step(t - 0.34f) * 0.14f; });   /* "the echoing sound of footsteps" */
}

/* ---- states ---- */
enum State { S_TITLE, S_PLAY, S_OVER };
static State st = S_TITLE; static float st_t = 0, time_left = 0; static double sim_t = 0;
static bool autoplay = false;
static float kill_flash = 0;

/* vehicle interior parts: a panel, hazard stripes, a round dial and a row of lamps, on any plane */
static V kit_eye(0, 0, 0); static float kit_lift = 20.f;   /* every part stands kit_lift off whatever is behind it, towards the pilot (Bobby: too close to the same plane flickers) */
static uint32_t kit_shade(uint32_t c, float f) { uint32_t r = (uint32_t)(((c >> 16) & 255) * f), g = (uint32_t)(((c >> 8) & 255) * f), b = (uint32_t)((c & 255) * f); return (r << 16) | (g << 8) | b; }
/* a solid block standing kit_lift off its surface and sunk 4 mm into it (Bobby: no paper-thin parts) */
static void kit_quad(Mesh &m, V o, V u, V v, uint32_t c) { V n = norm(cross(u, v)); if (dot(n, kit_eye - o) < 0) n = n * -1.f; V t = n * kit_lift, b = n * -4.f; const uint32_t s = kit_shade(c, 0.6f);
    V t0 = o + t, t1 = o + u + t, t2 = o + u + v + t, t3 = o + v + t, b0 = o + b, b1 = o + u + b, b2 = o + u + v + b, b3 = o + v + b;
    m.poly({t0, t1, t2, t3}, c); m.poly({b0, b1, t1, t0}, s); m.poly({b1, b2, t2, t1}, s); m.poly({b2, b3, t3, t2}, s); m.poly({b3, b0, t0, t3}, s); m.poly({b3, b2, b1, b0}, s); }
/* any flat convex shape given real thickness: 't' is half the thickness, both ways */
static void kit_slab(Mesh &m, std::initializer_list<V> c, V t, uint32_t face, uint32_t side) { V a[8]; int n = 0; for (V x : c) a[n++] = x;
    for (int i = 1; i + 1 < n; i++) { m.poly({a[0] + t, a[i] + t, a[i + 1] + t}, face); m.poly({a[0] - t, a[i + 1] - t, a[i] - t}, face); }
    for (int i = 0; i < n; i++) { int j = (i + 1) % n; m.poly({a[i] - t, a[j] - t, a[j] + t, a[i] + t}, side); } }
/* a four-sided bar from a to b, for struts, wires and diagonals */
static void kit_bar(Mesh &m, V a, V b, float hu, float hw, uint32_t c) { V d = norm(b - a), u = norm(cross(d, fabsf(d.z) < 0.9f ? V(0, 0, 1) : V(1, 0, 0))), w = cross(d, u); u = u * hu; w = w * hw; const uint32_t s = kit_shade(c, 0.75f);
    m.poly({a + u + w, b + u + w, b + u - w, a + u - w}, c); m.poly({a - u + w, a - u - w, b - u - w, b - u + w}, c); m.poly({a + u + w, a - u + w, b - u + w, b + u + w}, s); m.poly({a + u - w, b + u - w, b - u - w, a - u - w}, s);
    m.poly({a + u + w, a + u - w, a - u - w, a - u + w}, s); m.poly({b + u + w, b - u + w, b - u - w, b + u - w}, s); }
/* a short eight-sided drum round c, 'ax' half its length: wheels, dials */
static void kit_drum(Mesh &m, V c, V ax, V u, V w, float r, uint32_t face, uint32_t side) { const float K = 0.7071f; const float cx[8] = {1, K, 0, -K, -1, -K, 0, K}, cy[8] = {0, K, 1, K, 0, -K, -1, -K}; V a[8], b[8];
    for (int i = 0; i < 8; i++) { V o = u * (cx[i] * r) + w * (cy[i] * r); a[i] = c + ax + o; b[i] = c - ax + o; }
    m.poly({a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]}, face); m.poly({b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]}, face);
    for (int i = 0; i < 8; i++) { int j = (i + 1) & 7; m.poly({a[i], a[j], b[j], b[i]}, side); } }
static void kit_stripes(Mesh &m, V o, V u, V v, int n, uint32_t a, uint32_t b) { for (int i = 0; i < n; i++) kit_quad(m, o + u * ((float)i / n), u * (1.f / n), v, (i & 1) ? a : b); }
static void kit_dial(Mesh &m, V c, V u, V w, V n, float r, uint32_t rim, uint32_t face, uint32_t needle) {
    V t = c + n * 26.f;
    kit_drum(m, c + n * 9.f, n * 11.f, u, w, r, rim, kit_shade(rim, 0.7f)); kit_drum(m, c + n * 23.f, n * 3.f, u, w, r * 0.8f, face, kit_shade(rim, 0.7f));
    const float was = kit_lift; kit_lift = 7.f; kit_quad(m, t - u * (r * 0.06f), u * (r * 0.12f) + w * (r * 0.05f), w * (r * 0.65f) + u * (r * 0.3f), needle); kit_lift = was;
}
static void kit_lamps(Mesh &m, V o, V step, V u, V v, int n) { const uint32_t c[4] = {0xFF2820, 0x30E050, 0xFFB020, 0xF0F0F0}; const float was = kit_lift; kit_lift = 36.f; for (int i = 0; i < n; i++) kit_quad(m, o + step * (float)i, u, v, c[(i * 7 + 1) % 4]); kit_lift = was; }

/* dev: V1K_LOOK=yaw,pitch,spin (degrees, degrees, degrees a second; yaw left, pitch up) turns the picture without a headset, to look round a vehicle in test runs and video */
static void look_test(VPIX_Frame &f) {
    f.top_clear = true;   /* every vehicle game comes through here */
    static const char *e = getenv("V1K_LOOK"); if (!e || !*e) return;
    static float a[3] = {0, 0, 0}; static bool rd = false; static clock_t t0 = 0; if (!rd) { sscanf(e, "%f,%f,%f", a, a + 1, a + 2); rd = true; t0 = clock(); }
    const float R = 3.14159265f / 180.f, y = (a[0] + a[2] * (float)(clock() - t0) / (float)CLOCKS_PER_SEC) * R, p = a[1] * R, cy = cosf(y), sy = sinf(y), cp = cosf(p), sp = sinf(p);
    for (int k = 0; k < 3; k++) { float r0 = f.cam_R[0][k], r1 = f.cam_R[1][k], r2 = f.cam_R[2][k], x = r0 * cy + r1 * sy, w = r1 * cy - r0 * sy;
        f.cam_R[0][k] = x; f.cam_R[1][k] = w * cp + r2 * sp; f.cam_R[2][k] = r2 * cp - w * sp; }
}

/* Capture the Flag, the manual's F7 mode (D1K_MODE=ctf). Confirmed: a crossbow per player from the centre of the arena, an axe at close range.
   How the flags work was never documented, so these rules are OUR OWN DESIGN and the title screen says so: take the other side's flag home, your own
   must be at its base, a capture scores capture_points, a kill still scores 1. (The Computer Paper, Oct 1994: the flag flaps on a floating marble platform. Not built.) */
static bool ctf = false;
static int capture_points = 3;
static const float AXE_REACH = 1600.f, XBOW_AT = 1900.f;
static struct Flag { V home, p; int carrier = -1; } flags[2];

static bool carried(int i) { for (const Bird &b : birds) if (b.s == B_CARRY && b.target == i) return true; return false; }
static bool targeted(int i) { for (const Bird &b : birds) if (b.target == i && b.s != B_GONE) return true; return false; }
/* where you start: a raised platform each (VGCE 42), inside its arch looking down the stair */
static void place(Player &q) {
    q.epoch++; q.p = q.spawn; q.yaw = q.spawn_yaw; q.alive = true; q.falling = false; q.dropped = false; q.here = false; q.vz = 0; q.gibs.clear(); q.gib_t = 0; q.fire_cool = 1.f; q.node = -1; q.from = -1; q.think = 0; q.xbow = false; q.ride = -1; q.rode = false;
}
static void start_round() {
    const float D = DP - 1000.f; size_t n = 1 + (size_t)clampf(envf("D1K_CPU", 1), 1, 3);
    V at[4] = {V(-D, 0, RAISE), V(D, 0, RAISE), V(0, D, RAISE), V(0, -D, RAISE)};
    if (v1knet::on) { n = v1knet::SEATS; v1knet::begin_round((int)n); }   /* linked: 4 seats, CPU players fill the empty ones */
    pl.assign(n, Player());
    for (size_t i = 0; i < n; i++) { Player &q = pl[i]; q.cpu = i != (size_t)me; q.look = (int)i & 3; q.spawn = at[i]; q.spawn_yaw = atan2f(at[i].x, -at[i].y) + 3.14159265f; place(q); }
    if (v1knet::on) { if (ctf) for (size_t i = 0; i < n; i++) pl[i].look = 2 + ((int)i & 1); }
    else { pl[me].look = ctf ? 2 : 0; if (n > 1) pl[1].look = ctf ? 3 : 0; }   /* both ABC figures wear the same clothes; GamesMaster's are green and yellow */
    shots.clear(); sparks.clear(); for (Bird &b : birds) { if (b.s != B_GONE) b.s = B_PATROL; b.warn = -1; b.target = -1; }
    time_left = game_len; have_off = false;
    for (int k = 0; k < 2; k++) { flags[k].home = flags[k].p = at[k]; flags[k].carrier = -1; }
    dl("t=%.1f round start, %zu players", sim_t, n);
}
static void break_up(int victim, float total) {   /* pieces and sparks */
    Player &v = pl[victim];
    v.alive = false; v.dead_t = total; v.dead_total = total; v.gib_t = 0; v.gibs.clear();
    V c = v.p + V(0, 0, 1000);
    for (int k = 0; k < 8; k++) { Gib g; g.part = k; g.p = c + V(rnd(-200, 200), rnd(-200, 200), rnd(-300, 500));
        g.v = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(0.4f, 1.2f))) * rnd(1800, 3600); g.w = V(rnd(-5, 5), rnd(-5, 5), rnd(-5, 5)); v.gibs.push_back(g); }
    burst(c, 70, M_SPARK_R, 3200.f, 2.4f);
    sfx(3, pk(C_EXPLO, SND_HIT), victim == me ? 127 : 100); sfx(6, pk(C_BODHIT), victim == me ? 110 : 70);
    if (victim == me) sfx(9, pk(C_HURT), 110); else sfx(10, pk(C_HURT), 55);   /* the hurt voice, male or female ones at random */
    for (Flag &fl : flags) if (fl.carrier == victim) { fl.carrier = -1; fl.p = v.p; float h; if (!floor_at(fl.p.x, fl.p.y, fl.p.z + 100.f, h)) fl.p = fl.home; else fl.p.z = h; }
    for (Bird &b : birds) if (b.target == victim && b.s != B_CARRY) { b.s = B_PATROL; b.target = -1; b.warn = -1; }
}
static void kill(int victim, int killer) {
    Player &v = pl[victim];
    if (!v.alive) return;
    if (killer >= 0 && killer != victim) { pl[killer].score++; if (killer == me) kill_flash = 0.6f; }
    break_up(victim, respawn_delay);
    dl("t=%.1f player %d killed by %d: scores %d-%d", sim_t, victim, killer, pl[0].score, pl.size() > 1 ? pl[1].score : 0);
}
static bool slave_game = false; static V net_fire_from, net_fire_dir; static uint32_t net_fire_seq = 0;   /* a linked slave asks the master to fire */
static void fire(int who, V from, V dir) {
    Player &q = pl[who];
    if (q.fire_cool > 0 || !q.alive) return;
    if (slave_game) { if (who == me) { net_fire_from = from; net_fire_dir = dir; net_fire_seq++; bool axe = ctf && !q.xbow; q.fire_cool = axe ? 0.7f : fire_gap; sfx(1, axe ? SND_MISS : pk(C_SHOT, SND_SHOT), 110); } return; }
    if (ctf && !q.xbow) {   /* no crossbow yet: the axe, close up */
        q.fire_cool = 0.7f; sfx(who == me ? 1 : 2, SND_MISS, 100);
        for (size_t k = 0; k < pl.size(); k++) { const Player &e = pl[k]; V d = e.p - q.p;
            if ((int)k != who && ((int)k & 1) != (who & 1) && e.alive && len(d) < AXE_REACH) { kill((int)k, who); break; } }
        return;
    }
    q.fire_cool = fire_gap; q.shots++;
    Shot s; s.p = from; s.v = dir * shot_speed; s.owner = who; s.t = 0; shots.push_back(s);
    sfx(who == me ? 1 : 2, pk(C_SHOT, SND_SHOT), who == me ? 110 : 70);
    /* the bird counts shots fired, not hits (AppsLab 1994; Electronic Entertainment Jan 1994: "whoever shoots the most") */
    if (q.shots >= bird_shots && !targeted(who)) for (Bird &b : birds) if (b.target < 0 && b.s == B_PATROL) {
        b.target = who; b.warn = rnd(3.f, 9.f);   /* the icon blinks 5 s (ABC 0:26-0:21) to 12 s (GamePro 0:52-0:40) before the grab; the swoop itself takes 2-3 */
        if (who == me) sfx(4, pk(C_HUNGRY, SND_WARN), 100);
        dl("t=%.1f bird warning for player %d after %d shots", sim_t, who, q.shots);
        break;
    }
}

/* the computer gun-slingers just walk the platforms and fire (interview on the Yack clip: "walking forwards"); the route is ours */
static std::vector<V> nodes; static std::vector<std::vector<int>> links;
static void build_nodes() {
    nodes.clear(); links.clear();
    const float H = 1400.f; nodes.push_back(V(H, H, 0)); nodes.push_back(V(-H, H, 0)); nodes.push_back(V(-H, -H, 0)); nodes.push_back(V(H, -H, 0));   /* round the middle */
    links.resize(4);
    auto link = [&](int a, int b) { links[a].push_back(b); links[b].push_back(a); };
    for (int k = 0; k < 4; k++) link(k, (k + 1) & 3);
    const V dir[4] = {V(0, 1, 0), V(0, -1, 0), V(1, 0, 0), V(-1, 0, 0)}; const int hub[4][2] = {{0, 1}, {2, 3}, {0, 3}, {1, 2}};
    for (int k = 0; k < 4; k++) { V d = dir[k], s(-d.y, d.x, 0), up(0, 0, RAISE); int b = (int)nodes.size();
        nodes.push_back(d * (WC - 400.f)); nodes.push_back(d * (WC + GAP + 400.f) + up); nodes.push_back(d * (DP - 1000.f) + up);
        nodes.push_back(d * DP + s * (RW - 300.f) + up); nodes.push_back(d * DP - s * (RW - 300.f) + up);   /* the two lift ends */
        links.resize(nodes.size());
        link(b, hub[k][0]); link(b, hub[k][1]); link(b, b + 1); link(b + 1, b + 2); link(b + 2, b + 3); link(b + 2, b + 4); }
}
static int nearest_node(V p) { int b = 0; float bd = 1e30f; for (size_t i = 0; i < nodes.size(); i++) { V d = nodes[i] - p; d.z *= 3.f; float l = dot(d, d); if (l < bd) { bd = l; b = (int)i; } } return b; }
static bool solve_aim(V from, V to, V &dir) {   /* lob at a fixed speed: the low solution */
    V d = to - from; float h = sqrtf(d.x * d.x + d.y * d.y), v2 = shot_speed * shot_speed, g = shot_grav;
    float q = v2 * v2 - g * (g * h * h + 2.f * d.z * v2);
    if (q < 0 || h < 1.f) return false;
    float th = atan2f(v2 - sqrtf(q), g * h);
    dir = norm(V(d.x / h * cosf(th), d.y / h * cosf(th), sinf(th)));
    return true;
}
static void step_cpu(int i, float dt) {
    Player &q = pl[i];
    if (q.falling) return;
    if (q.ride >= 0) {   /* on or stepping onto a lift: stand still while it goes, get off where it stops */
        Lift &l = lifts[q.ride]; bool moving = false; for (const LiftPair &w : lift_pairs) if ((w.a == q.ride || w.b == q.ride) && w.moving) moving = true;
        if (moving) { q.rode = true; q.moved = 0; }
        else if (q.rode) { q.ride = -1; q.rode = false; q.node = nearest_node(q.p); q.from = q.node; dl("t=%.1f player %d off the lift at (%.0f %.0f)", sim_t, i, q.p.x, q.p.y); }
        else { V d2 = l.p - q.p; d2.z = 0; if (len(d2) > 150.f) { V was1 = q.p; q.p += norm(d2) * (walk_speed * dt); q.moved = walk_speed; keep_on(q.p, was1); } else q.moved = 0; }
    }
    if (q.node < 0) { q.node = nearest_node(q.p); q.from = q.node; }
    V to = nodes[q.node] - q.p; to.z = 0;
    if (q.ride < 0 && len(to) < 300.f && !ctf && rnd(0, 1) < 0.6f) for (size_t k = 0; k < lifts.size(); k++) {   /* at a lift end with a lift waiting: take it */
        const Lift &l = lifts[k]; V d2 = l.p - q.p; d2.z = 0; bool ok = len(d2) < 1800.f && fabsf(l.p.z - q.p.z) < 200.f;
        for (const LiftPair &w : lift_pairs) if ((w.a == (int)k || w.b == (int)k) && (w.moving || w.rest < 1.8f)) ok = false;
        if (ok) { q.ride = (int)k; q.rode = false; dl("t=%.1f player %d boards lift %zu at (%.0f %.0f)", sim_t, i, k, q.p.x, q.p.y); break; }
    }
    int ei = (i == me) ? 1 : me;
    if (v1knet::on) { float bd = 1e30f; for (size_t k = 0; k < pl.size(); k++) if ((int)k != i && pl[k].alive) { float l = len(pl[k].p - q.p); if (l < bd) { bd = l; ei = (int)k; } } }   /* linked: the nearest of the other three */
    const Player &e = pl[ei];
    if (q.ride < 0 && len(to) < 300.f) {
        const std::vector<int> &L = links[q.node];
        int nx = L[rng() % L.size()]; if (L.size() > 1) while (nx == q.from) nx = L[rng() % L.size()];
        if (ctf) {   /* the crossbow first, then the other side's flag, then home with it */
            const Flag &mine = flags[i & 1], &theirs = flags[(i & 1) ^ 1];
            V goal = !q.xbow ? V(0, 0, 0) : theirs.carrier == i ? mine.home : theirs.p; float bd = 1e30f;
            for (int c : L) { float l = len(nodes[c] - goal); if (l < bd) { bd = l; nx = c; } }
            if (nodes[q.node].x == nodes[nx].x && nodes[q.node].y == nodes[nx].y) nx = q.node;
        } else if (rnd(0, 1) < 0.7f) {   /* mostly toward the other man: the small arena and short shot make the filmed matches close quarters */
            float bd = 1e30f; for (int c : L) { float l = len(nodes[c] - e.p); if (l < bd) { bd = l; nx = c; } }
        }
        q.from = q.node; q.node = nx; to = nodes[q.node] - q.p; to.z = 0;
    }
    if (q.ride >= 0) { to = lifts[q.ride].p - q.p; to.z = 0; if (len(to) < 1.f) to = V(0, 1, 0); }
    V d = norm(to);
    float want = atan2f(-d.x, d.y), dy = want - q.yaw; while (dy > 3.14159f) dy -= 6.2831853f; while (dy < -3.14159f) dy += 6.2831853f;
    q.yaw += clampf(dy, -3.f * dt, 3.f * dt);
    if (q.ride < 0) { float sp = walk_speed * 0.9f; V was0 = q.p; q.p += d * (sp * dt); q.moved = sp; keep_on(q.p, was0); }
    if (e.alive && !carried(ei)) {
        V tgt = e.p + V(0, 0, 1000); V from = q.p + V(0, 0, 1300), dir;
        float range = len(tgt - from);
        if (ctf && !q.xbow) { if (q.fire_cool <= 0 && len(e.p - q.p) < AXE_REACH) fire(i, from, V(0, 1, 0)); }
        else if (q.fire_cool <= 0 && range < 9000.f && solve_aim(from, tgt, dir)) {
            float err = 0.03f + range / 150000.f;
            dir = norm(dir + V(rnd(-err, err), rnd(-err, err), rnd(-err, err)));
            fire(i, from + dir * 300.f, dir);
            if (q.fire_cool > 0) q.fire_cool = rnd(fire_gap, fire_gap * 2.5f);
        }
    }
}

/* ---- one step of the world ---- */
static bool third = false;        /* the player's view is outside the body: from the pickup until the screen goes black after the landing */
static void land(int i) {
    Player &q = pl[i];
    q.falling = false; q.vz = 0; q.node = -1;
    if (q.dropped) {   /* VGCE 42: you "smash apart", as bad as being shot; no point to anybody (ABC, GamePro). Then a black screen for 3 s (GamePro 0:31-0:29) and you are back where you landed, on the centre board (GamePro 0:28, GamesMaster 0:39) */
        q.dropped = false; q.here = true; sfx(5, pk(C_BODFALL, SND_THUD), i == me ? 110 : 60); dl("t=%.1f player %d lands and breaks up", sim_t, i); break_up(i, 3.4f); return; }
    if (i == me) { third = false; have_off = false; sfx(5, pk(C_BODFALL, SND_THUD), 110); }
}
static void move_lift(Lift &l, V to) {   /* whoever stands on it goes along */
    V d = to - l.p;
    for (size_t k = 0; k < pl.size(); k++) { Player &q = pl[k];
        if (!q.alive || carried((int)k) || q.falling || fabsf(q.p.x - l.p.x) > LIFT_H || fabsf(q.p.y - l.p.y) > LIFT_H || fabsf(q.p.z - l.p.z) > 250.f) continue;
        q.p += d; if ((int)k == me) walk_off += V(d.x, d.y, 0); }
    l.p = to;
}
static void step_lifts(float dt) {   /* 5 s at each end, then off by itself, rider or not: DAC.EXE LIFT_* and Pearl's turtles. Without a rider too: ABC 0:00, nobody aboard */
    for (LiftPair &w : lift_pairs) { Lift &A = lifts[w.a], &B = lifts[w.b];
        if (!w.moving) {
            w.rest -= dt; A.show = B.show = 2;
            if (w.rest <= 0) { w.moving = true; w.t = 0; w.dur = (std::max)(1.f, len(B.home - A.home) / lift_speed); }
            else { move_lift(A, A.home + V(0, 0, sinf((float)sim_t * 1.3f + w.a) * 25.f)); move_lift(B, B.home + V(0, 0, sinf((float)sim_t * 1.3f + w.b) * 25.f)); continue; }
        }
        A.show = 0; B.show = 1;
        w.t = (std::min)(w.t + dt / w.dur, 1.f);
        float e = w.t * w.t * (3.f - 2.f * w.t), up = sinf(w.t * 3.14159f);
        move_lift(A, A.home + (B.home - A.home) * e + w.bulge * up + V(0, 0, up * w.arc));
        move_lift(B, B.home + (A.home - B.home) * e - w.bulge * up + V(0, 0, up * w.arc));
        if (w.t >= 1.f) { w.moving = false; w.rest = 5.f; V h = A.home; A.home = B.home; B.home = h; }
    }
}
static void step_flags() {
    for (size_t i = 0; i < pl.size(); i++) { Player &q = pl[i];
        if (q.alive && !q.xbow && len(q.p - V(0, 0, 0)) < XBOW_AT) { q.xbow = true; sfx(1, SND_GO, (int)i == me ? 100 : 40); dl("t=%.1f player %zu has a crossbow", sim_t, i); } }
    for (int t = 0; t < 2; t++) { Flag &fl = flags[t], &other = flags[t ^ 1];
        if (fl.carrier >= 0) { Player &c = pl[fl.carrier]; fl.p = c.p;
            if (len(c.p - other.home) < 1500.f && other.carrier < 0 && len(other.p - other.home) < 1.f) {
                c.score += capture_points; dl("t=%.1f player %d captures: %d-%d", sim_t, fl.carrier, pl[0].score, pl.size() > 1 ? pl[1].score : 0);
                sfx(0, SND_GO, 120); fl.carrier = -1; fl.p = fl.home; }
            continue; }
        for (size_t i = 0; i < pl.size(); i++) { const Player &q = pl[i];
            if (!q.alive || carried((int)i) || len(q.p - fl.p) > 1200.f) continue;
            if (((int)i & 1) == t) { if (len(fl.p - fl.home) > 1.f) { fl.p = fl.home; dl("t=%.1f player %zu returned a flag", sim_t, i); } }
            else { fl.carrier = (int)i; if ((int)i == me) sfx(5, SND_GO, 100); dl("t=%.1f player %zu has the flag", sim_t, i); break; } }
    }
}
static void step_body(int i, float dt) {   /* gravity for anybody on foot or dropped */
    Player &q = pl[i];
    if (!q.alive || carried(i)) return;
    float h = 0; bool got = floor_at(q.p.x, q.p.y, q.p.z, h);
    if (got && q.p.z + q.vz * dt <= h) { if (q.falling) land(i); q.p.z = h; q.vz = 0; }
    else if (got && q.p.z - h < 60.f && !q.falling) q.p.z = h;
    else { if (!q.falling) fall_start(i, false); q.falling = true; q.vz -= GRAV * dt; q.p.z += q.vz * dt; }
    if (q.p.z < -30000.f) {   /* off the edge and gone: back to the start, no point to anybody (INVENTED) */
        dl("t=%.1f player %d fell off", sim_t, i); place(q); if (i == me) { have_off = false; third = false; } }
}
/* Where your gun sits. In a window the tracked hand is below the picture, so the gun is held at lower right, on the line of the default shot: the arrow keys turn it there and the shot leaves its muzzle. */
static V gun_place(const VPIX_Input &in, V hand_world, float stand_z) {
    if (in.vr || !in.head.valid) return hand_world;
    V eye(walk_off.x + in.head.p[0], walk_off.y + in.head.p[1], stand_z + in.head.p[2]);
    return eye + V(in.head.R[0][0], in.head.R[0][1], in.head.R[0][2]) * 200.f + V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2]) * 1100.f - V(in.head.R[2][0], in.head.R[2][1], in.head.R[2][2]) * 450.f;
}
/* Walking for the games that walk where you look. The thumb button walks forward. In a window (no headset) W/S and A/D come in as the joystick when VPIX_JOYAXES is on,
   as the launcher sets for these games, so they are read here: W forward, S back, A/D sidestep. True while moving. */
static bool desk_walk(const VPIX_Input &in, V f, float speed, float dt, V &walk_off) {
    float fw = (in.buttons & VPIX_BTN_WALK) ? 1.f : 0.f, sd = 0.f;
    if (!in.vr) { if (fabsf(in.joy[1]) > 0.1f) fw = in.joy[1]; sd = in.joy[0]; }
    if (fw == 0.f && fabsf(sd) < 0.1f) return false;
    walk_off += (f * fw + V(f.y, -f.x, 0) * sd) * (speed * dt);
    return true;
}
static void step_human(const VPIX_Input &in, float dt) {
    Player &q = pl[me];
    if (autoplay) { if (q.alive && !carried(me)) step_cpu(me, dt); return; }
    if (!q.alive || carried(me)) return;
    V hxy(in.head.p[0], in.head.p[1], 0);
    if (!have_off) { walk_off = V(q.p.x, q.p.y, 0) - hxy; have_off = true; }
    if (q.falling && third) return;
    if (in.vr && in.head.valid) q.hit_h = clampf(in.head.p[2] + 150.f, 700.f, 1900.f);   /* ducking: VGCE 42 "dodging incoming bullets", CBS 1992 */
    /* the thumb button walks you the way you look: VGCE 42 says the direction you look is the direction you go */
    V f = norm(V(in.head.R[1][0], in.head.R[1][1], 0));
    q.moved = desk_walk(in, f, walk_speed, dt, walk_off) ? walk_speed : 0;
    static float foot = 0; if (q.moved > 0 && !q.falling) { foot -= dt; if (foot <= 0) { foot = 0.62f; sfx(8, pk(C_FOOT, SND_FOOT), 80); } } else foot = 0;
    V p(walk_off.x + hxy.x, walk_off.y + hxy.y, q.p.z), was = p;
    push_out(p);
    { V stood(q.p.x, q.p.y, q.p.z); keep_on(p, stood); }
    walk_off += p - was;
    q.p.x = p.x; q.p.y = p.y; q.yaw = atan2f(-f.x, f.y);
    if ((in.buttons & VPIX_BTN_TRIGGER) && in.hand.valid) {
        V fw(in.hand.R[1][0], in.hand.R[1][1], in.hand.R[1][2]);
        V h = gun_place(in, V(walk_off.x + in.hand.p[0], walk_off.y + in.hand.p[1], q.p.z + in.hand.p[2]), q.p.z);
        if (q.fire_cool <= 0) { V dv = norm(fw); dl("t=%.1f you fire: gun (%.2f %.2f %.2f), view (%.2f %.2f %.2f)", sim_t, dv.x, dv.y, dv.z, in.head.R[1][0], in.head.R[1][1], in.head.R[1][2]); fire(me, h + fw * 620.f, dv); VPIX_Haptic(0, 0.08f, 0.6f); }
    }
}
static void step_bits(float dt);
static void step_shots(float dt) {
    for (size_t i = 0; i < shots.size();) {
        Shot &s = shots[i]; bool gone = false;
        s.v.z -= shot_grav * dt; s.p += s.v * dt; s.t += dt;
        float h;
        if (floor_at(s.p.x, s.p.y, s.p.z + 200.f, h) && s.p.z <= h + 10.f) { s.p.z = h + 10.f; burst(s.p, 12, M_SPARK_R, 2200.f, 0.9f); shot_hit_snd(s.p); gone = true; }
        if (!gone) for (const Pillar &c : pillars) { float dx = s.p.x - c.x, dy = s.p.y - c.y;
            if (dx * dx + dy * dy < c.r * c.r && s.p.z > c.z && s.p.z < c.z + c.h) { burst(s.p, 10, M_SPARK_O, 2000.f, 0.8f); gone = true; break; } }
        if (!gone) for (size_t k = 0; k < pl.size(); k++) { Player &q = pl[k];
            if ((int)k == s.owner || !q.alive || carried((int)k)) continue;
            float dz = clampf(s.p.z, q.p.z, q.p.z + q.hit_h); V cl(q.p.x, q.p.y, dz);
            if (len(s.p - cl) < 380.f) { kill((int)k, s.owner); gone = true; break; } }
        /* VGCE 42: you can shoot the pterodactyls if they come close enough; Electronic Entertainment 1: at the last second as he swoops */
        if (!gone) for (Bird &b : birds) if ((b.s == B_SWOOP || b.s == B_PATROL) && len(s.p - b.p) < 1900.f) {
            burst(b.p, 40, M_SPARK_G, 4000.f, 2.2f); sfx(3, pk(C_HIT, SND_HIT), 110);
            if (b.target >= 0) pl[b.target].shots = 0;
            dl("t=%.1f bird shot down by %d", sim_t, s.owner);
            b.s = B_GONE; b.gone = 15.f; b.target = -1; b.warn = -1; gone = true; break;
        }
        if (!gone && (s.t > 7.f || s.p.z < -40000.f)) gone = true;
        if (gone) { shots[i] = shots.back(); shots.pop_back(); } else i++;
    }
    step_bits(dt);
}
static void step_bits(float dt) {
    for (size_t i = 0; i < sparks.size();) { Spark &k = sparks[i]; k.v.z -= GRAV * 0.25f * dt; k.p += k.v * dt; k.life -= dt;
        if (k.life <= 0) { sparks[i] = sparks.back(); sparks.pop_back(); } else i++; }
    for (Player &q : pl) if (!q.gibs.empty()) { q.gib_t += dt;
        for (Gib &g : q.gibs) { g.v.z -= GRAV * 0.5f * dt; g.p += g.v * dt; float h; if (g.v.z < 0 && floor_at(g.p.x, g.p.y, g.p.z + 100.f, h) && g.p.z < h + 60.f) { g.p.z = h + 60.f; g.v = V(g.v.x * 0.5f, g.v.y * 0.5f, -g.v.z * 0.3f); g.w = g.w * 0.5f; } buggy::rotate(g.R, g.w, dt); }
    }
}
static void step_bird(Bird &bird, int idx, float dt) {
    const float R = 19000.f, Z = RAISE + 6500.f;
    static float flap_next = 0;
    switch (bird.s) {
    case B_PATROL: {
        if (st == S_TITLE && idx == 0) {   /* the attract shot: one bird right under the camera, wheeling over the centre (ABC 0:00, t40-44) */
            bird.a += 0.5f * dt; V c(1500, -2000, 5500); bird.p = c + V(cosf(bird.a) * 6000.f, sinf(bird.a) * 3500.f, sinf(bird.a * 2.f) * 500.f); bird.v = V(-sinf(bird.a) * 3000.f, cosf(bird.a) * 1750.f, 0); break; }
        bird.a += 0.2f * dt;
        V want(cosf(bird.a) * R, sinf(bird.a) * R, Z + sinf(bird.a * 3.f) * 2000.f);
        bird.p += (want - bird.p) * clampf(dt * 0.8f, 0, 1);
        bird.v = V(-sinf(bird.a), cosf(bird.a), 0) * 4000.f;
        if (bird.warn > 0) { bird.warn -= dt;
            if (bird.warn <= 0 && bird.target >= 0 && pl[bird.target].alive) { bird.s = B_SWOOP; bird.warn = -1; if (bird.target == me) sfx(4, pk(C_COMING, SND_COMING), 110); dl("t=%.1f bird swoops on %d", sim_t, bird.target); } }
        break; }
    case B_SWOOP: {
        Player &q = pl[bird.target];
        V tg = q.p + V(0, 0, 2300); V to = tg - bird.p; float d = len(to);
        bird.v = norm(to) * 11000.f; bird.p += bird.v * dt;
        if (d < 1500.f && !q.falling) {
            bird.s = B_CARRY; bird.t = 0;
            /* every drop on film lands on the centre board: GamePro 0:32, ABC 0:10, GamesMaster 1:00 and 0:40. 19 m is a 1.4 s fall at doubled gravity (06); Electronic Entertainment's "about 100 feet" is by eye */
            bird.drop = V(rnd(-3000, 3000), rnd(-3000, 3000), 19000.f + 2400.f);
            if (bird.target == me) third = true;
            q.shots = 0;
            dl("t=%.1f bird has player %d", sim_t, bird.target);
        }
        break; }
    case B_CARRY: {
        Player &q = pl[bird.target];
        const float T = 6.f;   /* 5-9 s seen */
        bird.t += dt;
        V to = bird.drop - bird.p; float left = (std::max)(T - bird.t, 0.2f);
        bird.v = to * (1.f / left); bird.p += bird.v * dt;
        q.p = bird.p - V(0, 0, 2400); q.vz = 0;
        if (bird.t >= T) {   /* let go */
            q.falling = true; q.dropped = true; fall_start(bird.target, true); dl("t=%.1f bird drops player %d", sim_t, bird.target);
            bird.s = B_LEAVE; bird.target = -1; bird.t = 0;
        }
        break; }
    case B_LEAVE:
        bird.t += dt; bird.v = norm(V(bird.p.x, bird.p.y, 0) * (1.f / 30000.f) + V(0, 0, 0.5f)) * 9000.f; bird.p += bird.v * dt;
        if (bird.t > 4.f) { bird.s = B_PATROL; bird.a = atan2f(bird.p.y, bird.p.x); }
        break;
    case B_GONE:
        bird.gone -= dt;
        if (bird.gone <= 0) { bird.s = B_PATROL; bird.p = V(40000, 0, 20000); }
        break;
    }
    if (!k_dactloop && bird.s != B_GONE && bird.s != B_PATROL) { flap_next -= dt; if (flap_next <= 0) { flap_next = (std::max)(0.6f, clip_secs(pk(C_FLAP, SND_FLAP))); sfx(7, pk(C_FLAP, SND_FLAP), 90); } }   /* with DACTLOOP the wing bed is snd_amb's, FLAP3 plays once at the dive */
}

/* DAC 2000 sounds that follow the game's state rather than one event, so a linked slave gets them too: it reads the same birds, lifts, scores and state from the master's snapshot.
   Channels: 9 your voice (hurt, scream), 10 other players' voices, 11 announcer, 12 heartbeat, 13 lift start and stop, 14 lift loop, 15 bird loop. Solo logic is untouched. */
static bool in_cat(int cat, uint32_t key) { for (uint32_t k : bank[cat]) if (k == key) return true; return false; }
static void announce(int cat, int vol = 110) {   /* one line at a time: skipped while the last is still talking */
    if (bank[cat].empty() || voices[11].on) return;
    sfx(11, pk(cat), vol);
}
static const float LIFT_EAR = 13000.f;   /* the arena is only ~20 m across and four lifts run in turn: heard from the two nearest, not all four, or they ran into each other */
static void snd_amb(float dt) {
    static State pst = S_TITLE; static int pscore = 0, pbs[2] = {B_PATROL, B_PATROL}; static float hb = 0, end_say = -1; static bool pal = true; static int pend_cat = -1; static bool pm[4] = {false, false, false, false}, bird_on = false, lift_on = false;
    if (pl.empty() || me >= (int)pl.size()) { pst = st; pscore = 0; hb = 0; for (bool &m : pm) m = false; if (bird_on) voices[15].on = false; if (lift_on) voices[14].on = false; bird_on = lift_on = false; return; }
    const Player &q = pl[me];
    if (pst != st) {
        if (st == S_PLAY) { pscore = q.score; pal = true; end_say = -1; }
        if (pst == S_PLAY && st == S_OVER) {   /* GAMEEND, then the rival's verdict once it has finished: first, second, third, last; level on top is just "game over" */
            int ahead = 0, level = 0; for (size_t i = 0; i < pl.size(); i++) if ((int)i != me) { if (pl[i].score > q.score) ahead++; else if (pl[i].score == q.score) level++; }
            pend_cat = level && !ahead ? C_VS_OVER : ahead == 0 ? C_VS_WON : ahead == 1 ? C_VS_2ND : ahead == 2 ? C_VS_3RD : C_VS_LAST;
            sfx(0, pk(C_END), 110); end_say = bank[C_END].empty() ? 0.2f : clip_secs(bank[C_END][0]) + 0.25f; }
        pst = st; }
    if (end_say >= 0) { end_say -= dt; if (end_say < 0) announce(pend_cat); }
    if (st == S_PLAY && q.score > pscore) announce(C_VS_SCORE);   /* you hit the rival: the taunt turns to praise */
    pscore = q.score;
    if (st == S_PLAY && pal && !q.alive) { if (!q.here) announce(C_VS_DIED); }   /* shot (not the smash after a drop, which sets "here") */
    pal = q.alive;
    /* the heartbeat while a bird has you marked: from the warning until it takes you */
    bool marked = false; for (const Bird &b : birds) if (b.target == me && q.alive && ((b.s == B_PATROL && b.warn > 0) || b.s == B_SWOOP)) marked = true;
    hb -= dt; if (marked && hb <= 0 && !bank[C_HEART].empty()) { uint32_t k = bank[C_HEART][0]; sfx(12, k, 90); hb = (std::max)(0.5f, clip_secs(k)); }
    if (!marked) hb = 0;
    /* a bird near: DACTLOOP as the wing bed, louder the closer; FLAP3 once as it dives (without the loop file the old repeating flap plays) */
    float bd = 1e9f; bool flying = false;
    for (int k = 0; k < 2; k++) { const Bird &b = birds[k];
        if (b.s == B_SWOOP || b.s == B_CARRY || b.s == B_LEAVE) { flying = true; bd = (std::min)(bd, len(b.p - q.p)); }
        if (k_dactloop && b.s == B_SWOOP && pbs[k] != B_SWOOP) sfx(7, pk(C_FLAP, SND_FLAP), 90);
        if (b.s == B_CARRY && pbs[k] != B_CARRY && b.target == me) announce(C_VS_GRAB);   /* the bird has you: "better you than me" */
        pbs[k] = b.s; }
    if (k_dactloop && flying) { int v = (int)clampf(95.f - bd * (60.f / 30000.f), 35.f, 95.f);
        if (!bird_on) { sfx(15, k_dactloop, v); bird_on = true; } else voices[15].vol = v; }
    else if (bird_on) { voices[15].on = false; bird_on = false; }
    /* lifts: LIFTACC as one sets off, LIFTEND as it stops, LIFTLOOP while the nearest one is moving; all fade with distance from you */
    float best = 1e9f;
    for (size_t k = 0; k < lift_pairs.size() && k < 4; k++) { const LiftPair &w = lift_pairs[k];
        float d = len((lifts[w.a].p + lifts[w.b].p) * 0.5f - q.p), nr = clampf(1.f - d / LIFT_EAR, 0.f, 1.f);
        if (nr > 0.1f) { if (w.moving && !pm[k]) sfx(13, pk(C_LIFTACC), (int)(100 * nr)); if (!w.moving && pm[k]) sfx(13, pk(C_LIFTEND), (int)(100 * nr)); }
        pm[k] = w.moving; if (w.moving) best = (std::min)(best, d); }
    if (k_liftloop && best < LIFT_EAR) { int v = (int)clampf(90.f * (1.f - best / LIFT_EAR), 10.f, 90.f);
        if (!lift_on) { sfx(14, k_liftloop, v); lift_on = true; } else voices[14].vol = v; }
    else if (lift_on) { voices[14].on = false; lift_on = false; }
    /* off the edge: stop the scream once you stand again (the smash cuts it by itself) */
    if (q.alive && !q.falling) { const VVoice &v = voices[9]; if (v.on && (in_cat(C_FALL, v.sound) || in_cat(C_SCREAM, v.sound))) voices[9].on = false; }
}

/* ---- HUD bitmaps, built here ---- */
static const char *FONT[] = {   /* 5 x 7, rows top to bottom, for A-Z */
 "01110100011000111111100011000110001","11110100011000111110100011000111110","01110100011000010000100001000101110","11110100011000110001100011000111110",
 "11111100001000011110100001000011111","11111100001000011110100001000010000","01110100011000010111100011000101111","10001100011000111111100011000110001",
 "01110001000010000100001000010001110","00111000100001000010000101001001100","10001100101010011000101001001010001","10000100001000010000100001000011111",
 "10001110111010110101100011000110001","10001110011010110011100011000110001","01110100011000110001100011000101110","11110100011000111110100001000010000",
 "01110100011000110001101011001001101","11110100011000111110101001001010001","01111100001000001110000010000111110","11111001000010000100001000010000100",
 "10001100011000110001100011000101110","10001100011000110001100010101000100","10001100011000110101101011101110001","10001100010101000100010101000110001",
 "10001100010101000100001000010000100","11111000010001000100010001000011111"};
static const uint32_t RGBA_WHITE = 0xFFFFFFFFu, RGBA_YEL = 0xFF30E0FFu, RGBA_RED = 0xFF3030F0u, RGBA_BLU = 0xFFC03030u;
enum { BM_TEXT = 0, BM_DIG = 200, BM_BOX = 300, BM_BIRD = 310 };
static const int FW = 10, FH = 14, DW = 16, DH = 26;
static void add_bitmap(int id, int w, int h, const std::vector<uint32_t> &topdown) {
    std::vector<uint32_t> up(w * h);
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) up[(h - 1 - y) * w + x] = topdown[y * w + x];
    VPIX_Render_AddBitmapRGBA(BMPH + id, w, h, up.data());
}
static void make_digit(int id, int d, uint32_t fill, uint32_t edge) {
    static const uint8_t SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};   /* a top, b upper right, c lower right, d bottom, e lower left, f upper left, g middle */
    std::vector<uint32_t> px(DW * DH, 0);
    auto rect = [&](int x0, int y0, int x1, int y1, uint32_t c) { for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) if (x >= 0 && y >= 0 && x < DW && y < DH) px[y * DW + x] = c; };
    uint8_t s = SEG[d];
    for (int pass = 0; pass < 2; pass++) { int g = pass ? 0 : 1; uint32_t c = pass ? fill : edge;
        if (s & 1) rect(3 - g, 2 - g, 13 + g, 5 + g, c);
        if (s & 2) rect(11 - g, 2 - g, 14 + g, 13 + g, c);
        if (s & 4) rect(11 - g, 13 - g, 14 + g, 24 + g, c);
        if (s & 8) rect(3 - g, 21 - g, 13 + g, 24 + g, c);
        if (s & 16) rect(2 - g, 13 - g, 5 + g, 24 + g, c);
        if (s & 32) rect(2 - g, 2 - g, 5 + g, 13 + g, c);
        if (s & 64) rect(3 - g, 12 - g, 13 + g, 15 + g, c); }
    add_bitmap(BM_DIG + id, DW, DH, px);
}
static void make_hud() {
    const uint32_t tc[3] = {RGBA_WHITE, RGBA_YEL, RGBA_RED};
    for (int set = 0; set < 3; set++) for (int ch = 0; ch < 26; ch++) {
        std::vector<uint32_t> px(FW * FH, 0);
        for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (FONT[ch][y * 5 + x] == '1')
            for (int b = 0; b < 2; b++) for (int a = 0; a < 2; a++) px[(y * 2 + b) * FW + x * 2 + a] = tc[set];
        add_bitmap(BM_TEXT + set * 40 + ch, FW, FH, px);
    }
    for (int d = 0; d < 10; d++) { make_digit(d, d, RGBA_WHITE, RGBA_BLU); make_digit(10 + d, d, RGBA_WHITE, RGBA_RED); make_digit(20 + d, d, RGBA_WHITE, 0xFF808080u); }
    { std::vector<uint32_t> px(6 * 26, 0);   /* the clock's colon */
      for (int y = 6; y < 10; y++) for (int x = 1; x < 5; x++) px[y * 6 + x] = RGBA_WHITE;
      for (int y = 16; y < 20; y++) for (int x = 1; x < 5; x++) px[y * 6 + x] = RGBA_WHITE;
      add_bitmap(BM_DIG + 40, 6, 26, px); }
    for (int k = 0; k < 2; k++) { const int W = 40, H = 32; std::vector<uint32_t> px(W * H, 0); uint32_t c = k ? 0xFFD0D0D0u : RGBA_RED;   /* the box round your own score */
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (x < 2 || y < 2 || x >= W - 2 || y >= H - 2) px[y * W + x] = c;
      add_bitmap(BM_BOX + k, W, H, px); }
    { static const char *b[] = {   /* the green warning bird that blinks in place of the clock */
        "............##..................","..........####..................","########..#####.......##########",".###########.######.###########.",
        "...############################.","......#####################.....",".........###############........","............######...##.........",
        "............####.....###........","...........###.......##.........","..........###...................","................................"};
      const int W = 32, H = 12; std::vector<uint32_t> px(W * H, 0);
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (b[y][x] == '#') px[y * W + x] = 0xFF50D050u;
      add_bitmap(BM_BIRD, W, H, px); }
}
/* The machine's own HUD, read at full size from hBkP2to1P_c (0:26-0:25): hollow pale seven-segment digits with cut corners and a coloured edge (red your count, blue the clock and the other's count; GamesMaster's own count is the team colour).
   The clock's serif font in GamesMaster is the TV show's overlay. A pale box round your count, on and off every 0.35 s; the green bird shows in place of the clock during the on half. No box in the 0:00 shot. */
enum { BM_SEG = 400 /* + set * 10 + digit */, BM_SEGBOX = 440, BM_SEGBIRD = 441, BM_SEGCOL = 442 };
static const int SEGW = 24, SEGH = 34;
static bool in_poly(float x, float y, const float *p, int n) { bool in = false; for (int i = 0, j = n - 1; i < n; j = i++) { float xi = p[i * 2], yi = p[i * 2 + 1], xj = p[j * 2], yj = p[j * 2 + 1]; if ((yi > y) != (yj > y) && x < (xj - xi) * (y - yi) / (yj - yi) + xi) in = !in; } return in; }
static void make_hud_d1k() {
    static const float S[7][12] = {
        {4,1, 20,1, 16,5, 8,5},                 /* a top */
        {21,2, 21,15, 17,13, 17,6},             /* b */
        {21,19, 21,32, 17,28, 17,21},           /* c */
        {4,33, 20,33, 16,29, 8,29},             /* d */
        {3,19, 7,21, 7,28, 3,32},               /* e */
        {3,2, 7,6, 7,13, 3,15},                 /* f */
        {5,17, 8,15, 16,15, 19,17, 16,19, 8,19} /* g */ };
    static const int SN[7] = {4, 4, 4, 4, 4, 4, 6}; static const uint8_t SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    const uint32_t FILL = 0xFFF4E4E8u, EDGE[3] = {0xFF2030D8u, 0xFFD04038u, 0xFF30B830u};
    for (int set = 0; set < 3; set++) for (int d = 0; d < 10; d++) { std::vector<uint32_t> px(SEGW * SEGH, 0);
        for (int y = 0; y < SEGH; y++) for (int x = 0; x < SEGW; x++) { bool on = false, near_seg = false;
            for (int s = 0; s < 7; s++) if (SEG[d] >> s & 1) { if (in_poly(x + 0.5f, y + 0.5f, S[s], SN[s])) on = true;
                for (int a = -1; a <= 1; a++) for (int b = -1; b <= 1; b++) if (in_poly(x + 0.5f + a * 1.2f, y + 0.5f + b * 1.2f, S[s], SN[s])) near_seg = true; }
            if (on) px[y * SEGW + x] = FILL; else if (near_seg) px[y * SEGW + x] = EDGE[set]; }
        add_bitmap(BM_SEG + set * 10 + d, SEGW, SEGH, px); }
    { const int W = 42, H = 31; std::vector<uint32_t> px(W * H, 0);
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { bool o2 = x < 2 || y < 2 || x >= W - 2 || y >= H - 2, o3 = x < 3 || y < 3 || x >= W - 3 || y >= H - 3; if (o2) px[y * W + x] = 0xFFE8E8F4u; else if (o3) px[y * W + x] = 0xFF3030C0u; }
      add_bitmap(BM_SEGBOX, W, H, px); }
    { const int W = 68, H = 30; std::vector<uint32_t> px(W * H, 0);
      static const float P[] = {0,5, 11,12, 25,14, 31,11, 34,0, 37,11, 43,14, 57,12, 68,5, 57,18, 43,24, 34,30, 25,24, 11,18};
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (in_poly(x + 0.5f, y + 0.5f, P, 14)) px[y * W + x] = x < 34 ? 0xFF38B028u : 0xFF308C24u;
      add_bitmap(BM_SEGBIRD, W, H, px); }
    { std::vector<uint32_t> px(6 * SEGH, 0); for (int y = 0; y < SEGH; y++) for (int x = 1; x < 5; x++) if ((y >= 10 && y < 14) || (y >= 21 && y < 25)) px[y * 6 + x] = FILL;
      add_bitmap(BM_SEGCOL, 6, SEGH, px); }
}
static void blit(VPIX_Frame &f, int id, int w, int h, float x, float y, float sc = 1.f) {
    VPIX_Blit b = {}; b.handle = BMPH + id; b.sw = w; b.sh = h; b.x = (int)x; b.y = (int)y; b.dw = w * sc; b.dh = h * sc; b.key = 0; b.alpha = 1.f; b.prio = 1000000;
    b.tint[0] = b.tint[1] = b.tint[2] = 1.f;
    f.blits.push_back(b);
}
static void text(VPIX_Frame &f, const char *s, float cx, float y, float sc, int set = 0) {
    float w = 0; for (const char *c = s; *c; c++) w += 12.f * sc;
    float x = cx - w * 0.5f;
    for (const char *c = s; *c; c++) { if (*c >= 'A' && *c <= 'Z') blit(f, BM_TEXT + set * 40 + (*c - 'A'), FW, FH, x, y, sc);
        else if (*c >= '0' && *c <= '9') blit(f, BM_DIG + 20 + (*c - '0'), DW, DH, x, y - 2.f * sc, sc * 0.6f);
        x += 12.f * sc; }
}
static void two(VPIX_Frame &f, int set, int n, float x, float y) {
    n = (std::max)(0, (std::min)(n, 99));
    blit(f, BM_SEG + set * 10 + n / 10, SEGW, SEGH, x, y, 0.72f); blit(f, BM_SEG + set * 10 + n % 10, SEGW, SEGH, x + 18, y, 0.72f);
}
static void hud(VPIX_Frame &f, int own, int opp, float secs, bool bird_icon, bool playing) {
    const float Y = 7.f; const bool on = fmodf((float)sim_t, 0.7f) < 0.35f;
    if (playing && on) blit(f, BM_SEGBOX, 42, 31, 38, Y - 3);
    two(f, ctf ? 2 : 0, own, 41, Y);                                       /* left: your own score, boxed every other 0.35 s */
    two(f, 1, opp, 284, Y);                                                /* right: the opponent's */
    int s = (int)ceilf((std::max)(secs, 0.f)), m = s / 60, r = s % 60; const float cx = 151.f, sc = 0.85f;
    if (bird_icon && on) { blit(f, BM_SEGBIRD, 68, 30, cx, Y + 1); return; }
    blit(f, BM_SEG + 10 + m % 10, SEGW, SEGH, cx, Y, sc); blit(f, BM_SEGCOL, 6, SEGH, cx + 21, Y, sc);
    blit(f, BM_SEG + 10 + r / 10, SEGW, SEGH, cx + 28, Y, sc); blit(f, BM_SEG + 10 + r % 10, SEGW, SEGH, cx + 48, Y, sc);
}

/* ---- drawing ---- */
static VPIX_Instance &dput(VPIX_Frame &f, int id, const M &R, V T) {
    VPIX_Instance in = {}; in.light_mask = 1; in.handle = MH + id;
    for (int k = 0; k < 3; k++) { in.R[k][0] = R.r[k].x; in.R[k][1] = R.r[k].y; in.R[k][2] = R.r[k].z; in.S[k] = 1; }
    in.T[0] = T.x; in.T[1] = T.y; in.T[2] = T.z;
    for (int e = 0; e < 16; e++) in.pal[e] = in.palw[e] = 0xFFFFFF;
    in.slot = (uint16_t)f.inst.size(); f.inst.push_back(in);
    return f.inst.back();
}
#include "vpix_1k_scores.h"
static M yawed(float yaw) { M m; m.r[0] = V(cosf(yaw), sinf(yaw), 0); m.r[1] = V(-sinf(yaw), cosf(yaw), 0); m.r[2] = V(0, 0, 1); return m; }
static M pitched(const M &b, float a) {   /* turned about its own x: a > 0 lifts the forward axis */
    M m; m.r[0] = b.r[0]; m.r[1] = b.r[1] * cosf(a) + b.r[2] * sinf(a); m.r[2] = b.r[2] * cosf(a) - b.r[1] * sinf(a); return m;
}
static M d1k_turning() { return yawed((float)sim_t * 1.5f); }
static void put_figure(VPIX_Frame &f, const Player &q) {
    const int B = M_BODY + 8 * q.look, idx = (int)(&q - pl.data()); M R = yawed(q.yaw); const float t = q.phase;
    V hip = q.p + V(0, 0, 860);
    dput(f, B, R, hip); dput(f, B + 1, R, hip + V(0, 0, 640));
    float sw = q.moved > 1.f ? sinf(t) * 0.55f : 0.f;
    if (carried(idx) || q.falling) sw = sinf((float)sim_t * 7.f) * 0.5f;   /* flailing (Perth) */
    for (int k = 0; k < 2; k++) {
        float a = k ? -sw : sw, side = k ? 100.f : -100.f; M th = pitched(R, a);
        V h = hip + R.r[0] * side; dput(f, B + 2 + k, th, h);
        float kn = (std::max)(0.f, k ? sw : -sw) * 0.9f; M sh = pitched(R, a - kn);
        dput(f, B + 4 + k, sh, h - th.r[2] * 420.f);
    }
    V sh_l = hip + V(0, 0, 590) - R.r[0] * 285.f, sh_r = hip + V(0, 0, 590) + R.r[0] * 285.f;
    dput(f, B + 6, pitched(R, -sw * 0.8f), sh_l);
    /* the right arm holds the gun up near the chest, the way players hold the stick (ABC 0:56, 0:27) */
    M up = pitched(R, 0.35f); dput(f, B + 7, up, sh_r);
    V elbow = sh_r - up.r[2] * 330.f; M fa = pitched(R, 1.9f + q.aim_pitch); dput(f, M_FOREARM + q.look, fa, elbow);
    V hand = elbow - fa.r[2] * 300.f; M gun = pitched(R, 0.8f + q.aim_pitch); dput(f, ctf ? (q.xbow ? M_XBOW : M_AXE) : M_GUNF, gun, hand); if (ctf && !q.xbow) dput(f, M_DISC + ((int)(sim_t / 1.7) % 3), gun, hand);
}
static void put_gibs(VPIX_Frame &f, const Player &q) {
    if (q.gib_t > 3.f) return;
    const int B = M_BODY + 8 * q.look;
    for (const Gib &g : q.gibs) dput(f, B + g.part, g.R, g.p);
}
static void put_bird(VPIX_Frame &f, const Bird &bird) {
    if (bird.s == B_GONE) return;
    M R = look_at(bird.p, bird.p + (len(bird.v) > 1.f ? bird.v : V(0, 1, 0)));
    dput(f, M_BIRD, R, bird.p);
    const float ph = (float)sim_t * (bird.s == B_PATROL ? 1.8f : 4.5f) + (&bird - birds) * 1.7f;
    float fa = sinf(ph) * 0.4f, fb = sinf(ph - 0.7f) * 0.45f;
    auto turned = [&](float a, float q) { M w = R; float c = cosf(a), s = sinf(a) * q; w.r[0] = R.r[0] * c + R.r[2] * s; w.r[2] = R.r[2] * c - R.r[0] * s; return w; };
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; M in = turned(fa, q), out = turned(fa + fb, q);
        V root = bird.p + R.r[0] * (q * 176.f) + R.r[1] * 489.f, tip = root + in.r[0] * (k ? 1100.f : -1154.f) + in.r[1] * 782.f + in.r[2] * (k ? 264.f : 284.f);   /* DACTYPE.DBS */
        dput(f, k ? M_WING_R : M_WING_L, in, root); dput(f, k ? M_OWING_R : M_OWING_L, out, tip); }
}
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = f.clear[1] = f.clear[2] = 0;
    f.n_lights = 1; f.lights[0] = {0.78f, 0.26f, {-0.218f, -0.437f, 0.874f}};   /* flat and bright: an unlit side of a pillar reads about 0.78 of the lit side (hBkP2to1P_c 0:14) */
    bool play = st != S_TITLE && !pl.empty();
    /* a black screen with the HUD on it after the pieces: GamePro 0:31-0:29, ABC 1:19-1:17 */
    const bool blackout = play && !pl[me].alive && pl[me].dead_t < (pl[me].here ? 3.0f : (std::max)(respawn_delay - 2.f, 0.f));
    { M I; dput(f, M_STARS, I, V(0, 0, 0)).bright = 1; if (!blackout) dput(f, M_ARENA, I, V(0, 0, 0)); }
    if (!blackout) {
    for (size_t i = 0; i < pl.size() && play; i++) {
        const Player &q = pl[i];
        if (q.alive) { if ((int)i != me || third) put_figure(f, q); }
        put_gibs(f, q);
    }
    { M I; for (const Lift &l : lifts) dput(f, M_PLATE + l.show, I, l.p); }
        if (ctf && play) { M I; for (int k = 0; k < 2; k++) dput(f, M_FLAG + k, I, flags[k].p);
        dput(f, M_XBOW, d1k_turning(), V(0, 0, 2700)); }
    for (const Bird &b : birds) put_bird(f, b);
    for (const Shot &s : shots) dput(f, M_DART, look_at(s.p, s.p + s.v), s.p);
    { M I; for (const Spark &k : sparks) dput(f, k.id, I, k.p).bright = 1; }
    }

    M cam; V eye; bool head_cam = false;
    static const char *fixed = getenv("D1K_CAM");
    if (fixed && *fixed) { float a[6] = {0, -30000, 8000, 0, 0, 1000}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5])); }
    else if (!play) {
        /* attract: over the south platform looking north at 00 0:00 00, the camera sinking slowly forward (ABC t38-44), the bird under it */
        float a = fmodf((float)sim_t, 18.f); eye = V(0, -4800.f + 450.f * a, 14800.f - 250.f * a); cam = look_at(eye, V(0, eye.y + 6600.f, 0.f));   /* pitch about 66 degrees, fitted to the plate widths at ABC 0:00 */
    } else {
        const Player &q = pl[me];
        if (third) {   /* above and behind your own figure while the bird has you, and down with it to the floor (ABC 0:11-0:10) */
            static V ch; static bool ci = false; V fw(0, 1, 0); for (const Bird &b : birds) if (b.s == B_CARRY && b.target == me) fw = norm(V(b.v.x, b.v.y, 0.001f));
            if (q.alive) { V want = carried(me) ? q.p - fw * 6500.f + V(0, 0, 4200.f) : q.p + V(600, -1800, 3200.f); if (!ci) ch = want; ch += (want - ch) * (carried(me) ? 0.25f : 0.6f); ci = true; }
            eye = ch; cam = look_at(eye, q.p + V(0, 0, 900));
        } else if (!q.alive) {   /* shot: out of the body, rising, looking down on your own pieces (ABC 2:22, GamePro 0:46) */
            float age = q.dead_total - q.dead_t; eye = q.p + V(100, -500, 1700.f + 1800.f * age); cam = look_at(eye, q.p + V(0, 0, 200));
        } else if (autoplay || !in.head.valid) {
            const Player &e = pl.size() > 1 ? pl[me == 1 ? 0 : 1] : q;
            eye = q.p + V(0, 0, EYE_DESK); cam = look_at(eye, e.p + V(0, 0, 1200));
            if (q.alive && envf("D1K_SHOWGUN", 0) != 0) { M g = pitched(cam, 0.35f); VPIX_Instance &gi = dput(f, ctf ? (q.xbow ? M_XBOW : M_AXE) : M_GUN, g, eye + cam.r[0] * 200.f + cam.r[1] * 330.f - cam.r[2] * 300.f); gi.on_top = 2; if (ctf && !q.xbow) dput(f, M_DISC + ((int)(sim_t / 1.7) % 3), g, eye + cam.r[0] * 200.f + cam.r[1] * 330.f - cam.r[2] * 300.f).on_top = 2; }   /* test hook: your gun in the bot's view */
        } else {
            for (int k = 0; k < 3; k++) cam.r[k] = V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2]);
            eye = V(walk_off.x + in.head.p[0], walk_off.y + in.head.p[1], q.p.z + in.head.p[2]);
            head_cam = true;
            if (q.alive && in.hand.valid) {   /* your own gun, tracked */
                M g; for (int k = 0; k < 3; k++) g.r[k] = V(in.hand.R[k][0], in.hand.R[k][1], in.hand.R[k][2]);
                V h = gun_place(in, V(walk_off.x + in.hand.p[0], walk_off.y + in.hand.p[1], q.p.z + in.hand.p[2]), q.p.z);
                VPIX_Instance &gi = dput(f, ctf ? (q.xbow ? M_XBOW : M_AXE) : M_GUN, g, h); gi.on_top = 2;
                if (ctf && !q.xbow) dput(f, M_DISC + ((int)(sim_t / 1.7) % 3), g, h).on_top = 2;
            }
        }
    }
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    f.in_head = head_cam; if (head_cam) f.head_fed = in.head;

    const float CX = 176.f;
    if (!play) {
        hud(f, 0, 0, 0, false, false);
        text(f, "DACTYL NIGHTMARE", CX, 70, 1.6f, 1);
        if (ctf) text(f, "CAPTURE THE FLAG", CX, 104, 0.8f);

        if (v1knet::on && !v1knet::master && !v1knet::late) text(f, !v1knet::master_heard() ? "LOOKING FOR PLAYER 1" : v1knet::nseats > 0 ? "YOU JOIN NEXT ROUND" : "PLAYER 1 STARTS THE GAME", CX, 200, 0.8f);
        else if (fmodf((float)sim_t, 1.2f) < 0.8f) text(f, "PRESS TRIGGER", CX, 200, 1.f);
        if (v1knet::on && v1knet::master && v1knet::live_pods() > 0) { char b[32]; snprintf(b, sizeof(b), "LINKED %d PLAYERS", v1knet::live_pods() + 1); text(f, b, CX, 170, 0.8f); }
    } else {
        int opp = 0; for (size_t i = 0; i < pl.size(); i++) if ((int)i != me) opp = (std::max)(opp, pl[i].score);
        bool icon = false; for (const Bird &b : birds) if (st == S_PLAY && b.target == me && (b.warn > 0 || b.s == B_SWOOP)) icon = true;
        hud(f, pl[me].score, opp, st == S_OVER ? 0.f : time_left, icon, st == S_PLAY);
        if (st == S_OVER) {
            text(f, "GAME OVER", CX, 96, 1.6f, 1);
            text(f, pl[me].score > opp ? "YOU WIN" : pl[me].score < opp ? "YOU LOSE" : "A DRAW", CX, 130, 1.f);
        }
    }
    hs::draw(f, "dactyl1k", st == S_TITLE, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at the machine's rate ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 1; static unsigned long frames = 0;
/* ---- link play (vpix_1k_net.h): pod 1 runs everything; another pod walks and aims its own figure, asks to fire, and draws the master's state. A start-together match; V1K_NET_LATE=1 lets a pod take a CPU seat mid-round ---- */
#pragma pack(push, 1)
struct NetIn { float x, y, yaw, pitch, moved, hit_h; uint8_t epoch; uint32_t fire_seq; float from[3], dir[3]; };
struct NetP { float p[3], yaw, pitch, vz, moved, dead_t, dead_total, hit_h; uint8_t flags, look, score, shots, epoch; };
struct NetB { int8_t s, target; float p[3], v[3], a, t, warn, gone; };
struct NetL { float p[3]; uint8_t show; };
struct NetLP { uint8_t moving; float rest; };
struct NetF { float p[3]; int8_t carrier; };
struct NetS { float p[3], v[3], t; int8_t owner; };
struct NetSt { uint8_t st, n, nshots, pad; float st_t, time_left; NetP pl[4]; NetB b[2]; NetL l[8]; NetLP lp[4]; NetF fl[2]; NetS sh[24]; };
#pragma pack(pop)
static void net3(float *o, V a) { o[0] = a.x; o[1] = a.y; o[2] = a.z; }
static V net3(const float *o) { return V(o[0], o[1], o[2]); }
static uint32_t net_last_fire[4];
static void net_log() {
    static double next = 0; if (sim_t < next) return; next = sim_t + 5.0;
    if (pl.size() < 4) { dl("t=%.1f link: %s, title, pods heard %d", sim_t, v1knet::master ? "master" : "slave", v1knet::live_pods()); return; }
    dl("t=%.1f link: %s seat %d st %d left %.0f | p0 %s(%.0f %.0f %.0f) %d | p1 %s(%.0f %.0f %.0f) %d | p2 (%.0f %.0f) %d | p3 (%.0f %.0f) %d", sim_t, v1knet::master ? "master" : "slave", me, (int)st, time_left,
       pl[0].cpu ? "cpu " : "", pl[0].p.x, pl[0].p.y, pl[0].p.z, pl[0].score, pl[1].cpu ? "cpu " : "", pl[1].p.x, pl[1].p.y, pl[1].p.z, pl[1].score, pl[2].p.x, pl[2].p.y, pl[2].score, pl[3].p.x, pl[3].p.y, pl[3].score);
}
static void net_master() {
    for (size_t i = 1; i < pl.size(); i++) { Player &q = pl[i]; const NetIn *p = (const NetIn *)v1knet::seat_input((int)i, sizeof(NetIn));
        if (!p) { if (!q.cpu) { q.cpu = true; q.node = -1; q.ride = -1; dl("t=%.1f link: seat %zu back to the computer", sim_t, i); } continue; }
        if (q.cpu) { q.cpu = false; q.ride = -1; net_last_fire[i] = p->fire_seq; dl("t=%.1f link: seat %zu is a person now", sim_t, i); }
        if (p->epoch == q.epoch && q.alive && !carried((int)i) && !q.dropped) { q.p.x = p->x; q.p.y = p->y; q.yaw = p->yaw; q.aim_pitch = p->pitch; q.moved = p->moved; q.hit_h = p->hit_h; }
        if (p->fire_seq != net_last_fire[i]) { net_last_fire[i] = p->fire_seq; fire((int)i, net3(p->from), net3(p->dir)); } }
    if (!v1knet::due(30)) return;
    if (st == S_TITLE || pl.empty()) { v1knet::send_snapshot(NULL, 0); return; }
    static NetSt s; memset(&s, 0, sizeof(s)); s.st = (uint8_t)st; s.n = (uint8_t)(std::min)(pl.size(), (size_t)4); s.st_t = st_t; s.time_left = time_left;
    for (int i = 0; i < s.n; i++) { const Player &q = pl[i]; NetP &n = s.pl[i]; net3(n.p, q.p); n.yaw = q.yaw; n.pitch = q.aim_pitch; n.vz = q.vz; n.moved = q.moved; n.dead_t = q.dead_t; n.dead_total = q.dead_total; n.hit_h = q.hit_h;
        n.flags = (q.alive ? 1 : 0) | (q.falling ? 2 : 0) | (q.dropped ? 4 : 0) | (q.here ? 8 : 0) | (q.xbow ? 16 : 0) | (q.cpu ? 32 : 0); n.look = (uint8_t)q.look; n.score = (uint8_t)q.score; n.shots = (uint8_t)q.shots; n.epoch = q.epoch; }
    for (int k = 0; k < 2; k++) { const Bird &b = birds[k]; NetB &n = s.b[k]; n.s = (int8_t)b.s; n.target = (int8_t)b.target; net3(n.p, b.p); net3(n.v, b.v); n.a = b.a; n.t = b.t; n.warn = b.warn; n.gone = b.gone;
        net3(s.fl[k].p, flags[k].p); s.fl[k].carrier = (int8_t)flags[k].carrier; }
    for (size_t k = 0; k < lifts.size() && k < 8; k++) { net3(s.l[k].p, lifts[k].p); s.l[k].show = (uint8_t)lifts[k].show; }
    for (size_t k = 0; k < lift_pairs.size() && k < 4; k++) { s.lp[k].moving = lift_pairs[k].moving; s.lp[k].rest = lift_pairs[k].rest; }
    for (size_t k = 0; k < shots.size() && s.nshots < 24; k++) { NetS &n = s.sh[s.nshots++]; net3(n.p, shots[k].p); net3(n.v, shots[k].v); n.t = shots[k].t; n.owner = (int8_t)shots[k].owner; }
    v1knet::send_snapshot(&s, offsetof(NetSt, sh) + s.nshots * sizeof(NetS));
}
static void net_leave(const char *why) { slave_game = false; st = S_TITLE; st_t = 0; pl.clear(); shots.clear(); sparks.clear(); me = 0; third = false; have_off = false; dl("t=%.1f link: %s, back to the title", sim_t, why); }
static void net_apply(const NetSt &s, uint8_t round) {
    static uint8_t my_round = 0; bool fresh = !slave_game || round != my_round || pl.size() != s.n; my_round = round;
    me = v1knet::my_seat;
    if (fresh) { pl.assign(s.n, Player()); shots.clear(); sparks.clear(); third = false; have_off = false; slave_game = true; sfx(0, pk(C_TIME, SND_START), 110); dl("t=%.1f link: in the master's round, seat %d of %d", sim_t, me, (int)s.n); }
    const bool was_carried = !fresh && carried(me);
    for (int k = 0; k < 2; k++) { Bird &b = birds[k]; const NetB &n = s.b[k];
        if (n.target == me && n.warn > 0 && b.warn <= 0) sfx(4, pk(C_HUNGRY, SND_WARN), 100);
        if (n.target == me && n.s == B_SWOOP && b.s != B_SWOOP) sfx(4, pk(C_COMING, SND_COMING), 110);
        if (n.s == B_GONE && b.s != B_GONE) { burst(b.p, 40, M_SPARK_G, 4000.f, 2.2f); sfx(3, pk(C_HIT, SND_HIT), 110); }
        b.s = (BirdState)n.s; b.target = n.target; b.p = net3(n.p); b.v = net3(n.v); b.a = n.a; b.t = n.t; b.warn = n.warn; b.gone = n.gone;
        flags[k].p = net3(s.fl[k].p); flags[k].carrier = s.fl[k].carrier; }
    for (size_t k = 0; k < lifts.size() && k < 8; k++) { move_lift(lifts[k], net3(s.l[k].p)); lifts[k].show = s.l[k].show; }   /* move_lift carries you if you stand on it */
    for (size_t k = 0; k < lift_pairs.size() && k < 4; k++) { lift_pairs[k].moving = s.lp[k].moving != 0; lift_pairs[k].rest = s.lp[k].rest; }
    for (int i = 0; i < s.n; i++) { Player &q = pl[i]; const NetP &n = s.pl[i]; const bool alive = (n.flags & 1) != 0, falling = (n.flags & 2) != 0, dropped = (n.flags & 4) != 0, own = i == me;
        if (!fresh) {
            if (q.alive && !alive) break_up(i, n.dead_t);                                       /* pieces, sparks and the bang are made here */
            if (!q.alive && alive) { q.gibs.clear(); if (own) { have_off = false; third = false; } }
            if (n.shots > q.shots && !own) sfx(2, pk(C_SHOT, SND_SHOT), 70);
            if (!q.falling && falling && alive) fall_start(i, dropped);
            if (own && n.score > q.score) { kill_flash = 0.6f; dl("t=%.1f link: you scored, %d", sim_t, (int)n.score); }
            if (own && q.falling && !falling && alive && !dropped) { have_off = false; third = false; sfx(5, pk(C_BODFALL, SND_THUD), 110); }
        }
        const bool keep = !fresh && own && q.alive && alive && !carried(i) && !was_carried && !dropped && q.epoch == n.epoch && (have_off || autoplay);   /* you lead your own walking */
        if (!keep) { q.p = net3(n.p); q.yaw = n.yaw; q.moved = n.moved; if (own) have_off = false; } else if (falling) q.p.z = n.p[2];
        q.aim_pitch = n.pitch; q.vz = n.vz; if (!own) { q.moved = n.moved; q.hit_h = n.hit_h; } q.dead_t = n.dead_t; q.dead_total = n.dead_total;
        q.alive = alive; q.falling = falling; q.dropped = dropped; q.here = (n.flags & 8) != 0; q.xbow = (n.flags & 16) != 0; q.cpu = (n.flags & 32) != 0; q.look = n.look; q.score = n.score; q.shots = n.shots; q.epoch = n.epoch; }
    if (carried(me)) third = true;
    shots.resize(s.nshots); for (int k = 0; k < s.nshots; k++) { shots[k].p = net3(s.sh[k].p); shots[k].v = net3(s.sh[k].v); shots[k].t = s.sh[k].t; shots[k].owner = s.sh[k].owner; }
    st = (State)s.st; st_t = s.st_t; time_left = s.time_left;
}
static void net_slave_step(const VPIX_Input &in, float dt) {
    static float flap = 0; bool flapping = false; for (const Bird &b : birds) if (b.s == B_SWOOP || b.s == B_CARRY || b.s == B_LEAVE) flapping = true;
    if (flapping && !k_dactloop) { flap -= dt; if (flap <= 0) { flap = (std::max)(0.6f, clip_secs(pk(C_FLAP, SND_FLAP))); sfx(7, pk(C_FLAP, SND_FLAP), 90); } }
    for (Player &q : pl) { q.phase += q.moved * dt * 0.006f; if (!q.alive) q.dead_t -= dt; }
    Player &o = pl[me]; if (o.fire_cool > 0) o.fire_cool -= dt;
    if (st == S_PLAY) {
        step_human(in, dt);
        if (autoplay) push_out(o.p);
        float h; if (o.alive && !carried(me) && !o.falling && floor_at(o.p.x, o.p.y, o.p.z, h) && o.p.z - h < 600.f) o.p.z = h;
        time_left -= dt; }
    for (Shot &s : shots) { s.v.z -= shot_grav * dt; s.p += s.v * dt; s.t += dt; }
    step_bits(dt);
}
/* true = this tick was a linked slave's, inside the master's game */
static bool net_tick(double el, const VPIX_Input &in, bool trig) {
    v1knet::pump();
    if (v1knet::master) { net_master(); net_log(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n >= offsetof(NetSt, sh) && v1knet::my_seat >= 0) { static NetSt s; memset(&s, 0, sizeof(s)); memcpy(&s, d, (std::min)(n, sizeof(s))); if (s.n >= 1 && s.n <= 4) net_apply(s, v1knet::round_id); }
        else if (slave_game) net_leave("the master's round is over");
    }
    if (slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
    net_log();
    if (!slave_game) { v1knet::send_input(NULL, 0); return false; }
    NetIn o; memset(&o, 0, sizeof(o)); const Player &q = pl[me];
    o.x = q.p.x; o.y = q.p.y; o.yaw = q.yaw; o.pitch = q.aim_pitch; o.moved = q.moved; o.hit_h = q.hit_h; o.epoch = q.epoch; o.fire_seq = net_fire_seq; net3(o.from, net_fire_from); net3(o.dir, net_fire_dir);
    v1knet::send_input(&o, sizeof(o));
    hs::run("dactyl1k", st == S_OVER && st_t > 2.f, q.score * 100, in, trig, (float)el, autoplay);
    const float dt = 1.f / 120.f; int k = 0;
    while (acc >= dt && k < 24) { net_slave_step(in, dt); acc -= dt; sim_t += dt; st_t += dt; k++; }
    snd_amb((float)el);
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    return true;
}
static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    static bool trig_was = false; bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f, sel = trig && !trig_was; trig_was = trig;
    if (autoplay && st != S_PLAY && st_t > 2.f) sel = true; if (hs::run("dactyl1k", st == S_OVER && st_t > 2.f, pl.empty() ? 0 : pl[me].score * 100, in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (!v1knet::can_start()) sel = false;   /* a linked slave: pod 1 starts the game */
    if (v1knet::on && net_tick(el, in, trig)) { PIC_AddEvent(tick, 2.0); return; }
    if (st == S_TITLE && sel) { start_round(); st = S_PLAY; st_t = 0; third = false; sfx(0, pk(C_TIME, SND_START), 110); }
    else if (st == S_OVER && sel && st_t > 3.f) { st = S_TITLE; st_t = 0; v1knet::end_round(); pl.clear(); shots.clear(); sparks.clear(); }
    int n = 0;
    while (acc >= dt && n < 24) {
        for (int b = 0; b < 2; b++) step_bird(birds[b], b, dt);
        step_lifts(dt);
        if (st != S_TITLE && !pl.empty()) {
            for (Player &q : pl) { if (q.fire_cool > 0) q.fire_cool -= dt; q.phase += q.moved * dt * 0.006f;
                if (!q.alive) { q.dead_t -= dt; if (q.dead_t <= 0) { int idx = (int)(&q - pl.data()); if (q.here) { q.alive = true; q.here = false; q.gibs.clear(); q.fire_cool = 1.f; } else place(q); if (idx == me) { have_off = false; third = false; } } } }
            if (st == S_PLAY) {
                if (ctf) step_flags();
                step_human(in, dt);
                for (size_t i = 1; i < pl.size(); i++) if (pl[i].cpu && pl[i].alive && !carried((int)i)) step_cpu((int)i, dt);
                for (Player &q : pl) if (q.cpu || autoplay) push_out(q.p);
                for (size_t i = 0; i < pl.size(); i++) step_body((int)i, dt);
                time_left -= dt;
                if (time_left <= 0) { st = S_OVER; st_t = 0; dl("t=%.1f game over: %d-%d", sim_t, pl[0].score, pl.size() > 1 ? pl[1].score : 0); }
            }
            step_shots(dt);
        }
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    snd_amb((float)el);
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    if (sim_t >= next_log && !pl.empty()) { next_log = sim_t + 2.0; const Player &q = pl[me];
        dl("t=%.0f st %d me (%.0f %.0f %.0f) alive %d falling %d shots %d | cpu (%.0f %.0f %.0f) shots %d | bird %d (%.0f %.0f %.0f) warn %.1f | %d-%d left %.0f",
           sim_t, (int)st, q.p.x, q.p.y, q.p.z, q.alive, q.falling, q.shots, pl.size() > 1 ? pl[1].p.x : 0, pl.size() > 1 ? pl[1].p.y : 0, pl.size() > 1 ? pl[1].p.z : 0, pl.size() > 1 ? pl[1].shots : 0,
           (int)birds[0].s, birds[0].p.x, birds[0].p.y, birds[0].p.z, birds[0].warn, q.score, pl.size() > 1 ? pl[1].score : 0, time_left); }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) dlog = fopen((std::string(logbase) + ".d1k").c_str(), "w");
    walk_speed = envf("D1K_WALK", walk_speed); shot_speed = envf("D1K_SHOT", shot_speed); shot_grav = envf("D1K_SHOTG", shot_grav); respawn_delay = envf("D1K_RESPAWN", respawn_delay);
    bird_shots = (int)envf("D1K_BIRD_SHOTS", (float)bird_shots); game_fps = envf("D1K_FPS", game_fps); game_len = envf("D1K_TIME", game_len);
    autoplay = envf("D1K_AUTO", 0) != 0; edge_detect = envf("D1K_EDGE", 1) != 0; lift_speed = envf("D1K_LIFT", lift_speed);
    { const char *m = getenv("D1K_MODE"); ctf = m && !strcmp(m, "ctf"); shootout_look = !ctf; } capture_points = (int)envf("D1K_CAPTURE", (float)capture_points);
    build_arena(); build_nodes(); make_hud(); make_hud_d1k(); make_sounds();
    load_bank();
    for (int b = 0; b < 2; b++) { birds[b].a = b * 3.14159f; birds[b].p = V(cosf(birds[b].a) * 19000.f, sinf(birds[b].a) * 19000.f, RAISE + 6500.f); birds[b].v = V(0, 4000, 0); }
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    v1knet::init("D1KN", false, dl);   /* start-together match (Bobby: join-in-progress is for Flying Aces and VTOL) */
    PIC_AddEvent(tick, 2.0);
    dl("Dactyl Nightmare rebuild started");
}
}   /* namespace d1k */
