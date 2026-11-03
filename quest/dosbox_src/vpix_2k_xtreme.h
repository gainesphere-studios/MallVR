/* X-Treme Strike (Virtuality 2000 series, 1994) - native rebuild from footage, stills and press. No copy of the game turned up, so every model is built here and no game file is read.
   Included after vpix_dactyl1k.h; switched on by VPIX_NATIVE=xtreme. Dev only, not in a release. Write-up with evidence: Projects/Virtuality 1000 Revival/pass_1to1_2026-11-02/xtreme.md
   SOURCES: promo clip m08AaGt-uGI (34 s, attract only, same footage as local WMP_xtreme and rm_xtreme_isdn); Cybermind's page xtremestrike.JPG and stills xs_pic1/2.bmp (Jebus7754/VR/Bilder, "actual game shot");
   Cybermind's vrgames.htm; press 1994: PC Power 7, Hyper 9, GamePro 58, Canadian Coin Box, Atari ST User 102, Super PC 17.
   FROM STILLS (play): first-person from the cockpit with no ship drawn ahead; orange ring sight with four ticks dead centre; 7-digit score top centre in a wide rounded font, crimson; a cream panel bottom centre;
   yellow-and-green diamonds near the view's corners, big yellow panels off to the side; a tapering yellow beam from up by a top corner to the ring; a ribbed octagonal tunnel (score 0000000) with stars at its end;
   a dark starry sky over a blue surface with pale patches, a dark red tower with a ribbed pale cone, a ribbed green pylon (score 0046429); a blue river between dark rock pyramids (1724140); the dam of slanted panels with
   lilac sky and green land (1878865); a hollow magenta triangle on a target. The scores give the order of the stages. Not-round scores, so points are not round.
   FROM PRESS (rules): attack ship, asteroid-ridden space, then skim planet surfaces, a deadly planet-fall, destroy the vital core reactor on Zenon (Atari ST User, PC Power); the central control base and the Commander
   (Cybermind); ever increasing defence waves (Cybermind page); unlimited lasers, auto-lock plasma missiles, head-mounted sight, power-ups (Hyper 9); a shield that cannot be repaired (Cybermind); fuel gauge (Cybermind, Coin Box);
   droid sidekick (GamePro, Hyper); no time limit, ends on death, gets harder (PC Power); the better you are, the longer you play (Cybermind); sit-down, up to 2 players (only 1 here so far).
   FROM FOOTAGE (attract, timed to the clip): chase view over green hills to the dam, jets, over the crest, the lake; INSERT CREDIT on and off; cards traced from the clip: 360 ACTION, 3D DIGITAL SOUND, VIRTUAL REALITY, LIVE IT!;
   magenta bars wipe; a white panelled box whose zigzag door opens, turning away; the XTREME STRIKE logo flying in. The chase view belongs to the attract only.
   READ FROM THE STILLS, NOT PROVEN: the big yellow panels are your forward-swept wings; the beam is your laser, guns at the top corners; the tunnel is the launch tube and the white box in the attract its carrier; the triangle is the missile lock;
   the panel's two wells are shield and fuel; the green capsule in xs_pic1 is a fuel power-up; the pale cone is a searchlight.
   INVENTED (no source): the flight model; every number (speeds, distances, damage, fuel burn, hits to kill, missile count); point values, scaled so a run passes the stills' scores; what power-ups give beyond fuel and missiles; asteroid and enemy behaviour; how the droid shows itself (chirps here); the core, the Commander and the
   base's look; what happens after the core (round again, harder); GAME OVER text; all sounds.
   Units: mm, seconds. World X right, Y forward, Z up. Env: XTREME_AUTO, _LOOK (yawdeg,pitchdeg), _STAGE (0 tube 1 space 2 night 3 valley 4 core), _Y (start there), _ATTRACT (stay on the attract), _ATTR_T (start time), _FPS, _SPEED, _TURN, _HP, _FUEL. */
#include <functional>
#include "vpix_1k_net.h"         /* link play, 2-4 pods: see the "link play" block below */
namespace xtreme {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at;
using d1k::Mesh; using d1k::dput; using d1k::blit; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound; using d1k::add_bitmap;

static float game_fps = 20.f, fly_speed = 62000.f, turn_rate = 1.1f, start_hp = 100.f, start_fuel = 100.f; static bool autoplay = false, attract_only = false; static float look_env[2] = {0, 0}; static bool have_look = false; static int start_stage = 0; static float start_y = -1e30f;
static const float DAM_Y = 620000.f, FOV = 60.f;   /* FOV: Visette 2, 60 x 46.9 degrees per eye (virtuality.com 1997) */
static const float TUBE_R = 14000.f, TUBE_L = 17000.f; static const int TUBE_N = 30;   /* measured off the still: rings at about 1.7, 2.6, 3.3, 4.8 radii ahead, black beyond */
static const float SPACE_LEN = 2800000.f, NIGHT_LEN = 2800000.f, VALLEY_Y0 = -1400000.f, CORE_Y = 1210000.f;
static const float FIRE_GAP = 0.14f, RANGE = 420000.f;
enum { M_LAND = 0x220001, M_SKY, M_DAM, M_JET, M_LAKE, M_ROCK, M_CRAFT, M_CRAFT2, M_CMDR, M_BOLT, M_BOOM, M_PLATE, M_SHARD, M_BOX, M_DOOR_T, M_DOOR_B, M_DOOR_IN, M_LOGO, M_TUBE, M_STARS, M_NSKY, M_SEA, M_PATCH, M_PYLON, M_TOWER, M_CONE,
       M_RIVER, M_PYRS, M_ROID0, M_ROID1, M_ROID2, M_COCK, M_COCK_DIM, M_BEAM, M_LOCK, M_FUEL, M_AMMO, M_MISS, M_CORE, M_ORB, M_SPARK };
enum { BM_DIGIT = 510, BM_RING = 530, BM_PANEL, BM_BAR, BM_WIPE, BM_CARD = 540, BM_LET = 550 };
enum { SND_FIRE = 0x7E000001, SND_BOOM, SND_HURT, SND_GO, SND_DROID, SND_MISS, SND_PICK, SND_LOCK };
enum Stage { ST_TUBE, ST_SPACE, ST_NIGHT, ST_VALLEY, ST_CORE };

static V tw(const M &R, V a) { return R.r[0] * a.x + R.r[1] * a.y + R.r[2] * a.z; }
static float wrap(float a) { while (a > 3.14159265f) a -= 6.2831853f; while (a < -3.14159265f) a += 6.2831853f; return a; }
static void quad(Mesh &m, V a, V b, V c, V d, uint32_t rgb) { m.poly({a, b, c, d}, rgb); }
static uint32_t lit(uint32_t c, float f) { return d1k::kit_shade(c, f); }
static uint32_t mix(uint32_t a, uint32_t b, float t) { t = clampf(t, 0.f, 1.f); uint32_t r = 0; for (int s = 0; s <= 16; s += 8) { float x = ((a >> s) & 255) * (1 - t) + ((b >> s) & 255) * t; r |= (uint32_t)x << s; } return r; }
static uint32_t col20(uint32_t rgb) { return 0x20000000u | ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | (rgb >> 16); }
static void grad(Mesh &m, std::initializer_list<V> pts, std::initializer_list<uint32_t> cols) {   /* a smooth-shaded polygon, as the clip's mottled land and sky read */
    m.poly(pts, 0xFFFFFF); VPIX_MeshPoly &q = m.p.back(); q.flags = 0x40000000u; int i = 0; for (uint32_t c : cols) q.vcol[i++] = col20(c);
}
static float noise2(float x, float y, int seed) {
    auto h = [&](int i, int j) { uint32_t n = (uint32_t)(i * 73856093) ^ (uint32_t)(j * 19349663) ^ (uint32_t)(seed * 83492791); n = (n ^ (n >> 13)) * 1274126177u; return ((n ^ (n >> 16)) & 0xFFFF) / 65535.f; };
    int i = (int)floorf(x), j = (int)floorf(y); float u = x - i, v = y - j; u = u * u * (3 - 2 * u); v = v * v * (3 - 2 * v);
    return (h(i, j) * (1 - u) + h(i + 1, j) * u) * (1 - v) + (h(i, j + 1) * (1 - u) + h(i + 1, j + 1) * u) * v;
}
/* ---- lettering: strokes on a 4 x 6 grid, "x y" digit pairs, | starts a new line ---- */
static const char *strokes(char c) { switch (c) {
    case '0': return "103041453616050110"; case '1': return "112026"; case '2': return "01103041423313040646"; case '3': return "003041423313|3344453606"; case '4': return "36300444"; case '5': return "4000033344453606";
    case '6': return "40100105163645443303"; case '7': return "004026"; case '8': return "10304142331304051636454433|13020110"; case '9': return "06364541301001021343";
    case 'I': return "2026|1030|1636"; case 'N': return "06004640"; case 'S': return "40100102133344453606"; case 'E': return "40000646|0333"; case 'R': return "06003041423303|3346"; case 'T': return "0040|2026"; case 'C': return "4130100105163645"; case 'D': return "06003041453606";
    case 'X': return "0046|4006"; case 'M': return "0600244046"; case 'K': return "0600|4003|1346"; case '-': return "0343"; } return ""; }
template <class F> static void for_segments(char c, F fn) { const char *s = strokes(c); float px = 0, py = 0; bool have = false; for (; *s; ) { if (*s == '|') { have = false; s++; continue; } float x = (float)(s[0] - '0'), y = (float)(s[1] - '0'); s += 2; if (have) fn(px, py, x, y); px = x; py = y; have = true; } }
static void make_glyph(int id, char c, int w, int h, float th, uint32_t top, uint32_t bot) {   /* colours are ABGR */
    std::vector<uint32_t> px(w * h, 0); const float mx = th + 1.f, my = th + 1.f, sx = (w - 2 * mx) / 4.f, sy = (h - 2 * my) / 6.f;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) { float best = 1e9f; for_segments(c, [&](float x0, float y0, float x1, float y1) { float ax = mx + x0 * sx, ay = my + y0 * sy, bx = mx + x1 * sx, by = my + y1 * sy, dx = bx - ax, dy = by - ay, t = clampf(((x + .5f - ax) * dx + (y + .5f - ay) * dy) / (dx * dx + dy * dy + 1e-6f), 0.f, 1.f), ex = x + .5f - (ax + t * dx), ey = y + .5f - (ay + t * dy); best = (std::min)(best, ex * ex + ey * ey); });
        if (best <= th * th) { float k = (float)y / (h - 1); uint32_t c2 = 0xFF000000u; for (int s = 0; s <= 16; s += 8) c2 |= (uint32_t)(((top >> s) & 255) * (1 - k) + ((bot >> s) & 255) * k) << s; px[y * w + x] = c2; } }
    add_bitmap(id, w, h, px);
}
/* the attract's four cards, traced from the clip: run lengths, 2 base-36 digits each, off first, / between rows */
struct Card { const char *rle; int w, h, x, y; uint32_t col; };
static const Card CARDS[4] = {
    {"290a280620/270e0z0c0s0c0x040w/260h0w0d0r0e0u080u/250i0w0e0q0f0r0c0s/250k0u0d0p0i0o0f0r/230m0u0d0o0k0n0g0q/230n0t0c0p0k0n0g0q/230n0t0c0o0m0m0g0q/220p0r0d0o0m0l0j0o/220p0r0c0p0n0j0k0o/210r0p0d0o0o0j0l0n/210r0p0c0o0q0h0m0n/210r0p0c0o0q0h0n0m/210s0n0d0n0r0g0o0m/210s0n0c0o0s0f0p0l/210s0m0d0o0s0f0p0l/210c020e0m0d0o0s0f0p0l/200c040d0m0c0o0u0e0b020c0l/200c050c0m0c0o0d030e0e0b030b0l/1z0c070b0l0d0o0d040d0e0b030b0l/200b070b0k0e0o0d040d0e0b030b0l/200a080b0k0d0p0d050d0d0a040b0l/200a080b0k0d0p0c060d0c0b040b0l/200a080b0j0e0p0c060d0c0b040b0l/200a080b0k0c0q0b080c0c0b040b0l/200a080b0k0c0q0b080c0c0b040b0l/200a080b0j0d0q0b080d0b0b040b0l/200a080b0j0c0r0b080d0b0b040b0l/2i0b0j0c0p0d080d0b0b040b0l/2i0b0i0d0p0d080d0b0b040b0l/2i0b0i0c0q0d080d0b0b040b0l/2i0b0i0c0q0d080d0b0b040b0l/2i0b0i0c0q0d080d0b0b040b0l/2h0c0h0d0q0c090d0b0b040b0l/2h0c0g0d02070i0c090d0b0b040b0l/2h0c0g0d01090h0c090d0b0b040b0l/2f0d0h0o0g0c090d0c0b030b0l/2e0e0h0p0f0c090d0c0b030b0l/2d0f0h0r0d0c090d0c0b030b0l/290i0i0r0d0c090d0c0b030b0l/290i0i0r0d0b0a0d0c0b030b0l/290i0i0s0c0b0a0d0c0p0l/290i0i0t0b0b0a0d0c0p0l/290i0i0t0b0b0a0d0c0p0l/290g0k0t0b0b0a0d0c0o0m/290g0k0u0a0b0a0d0d0n0m/290f0l0u0a0b0a0d0d0m0n/290e0m0u0a0b0a0d0e0l0n/290f0l0u0a0b0a0d0e0k0o/290g0j0w090b0a0d0f0j0o/290h0i0w090b0a0d0g0h0p/290i0g0g020f090b0a0d0g0h0p/290j0f0f030f090b0a0d0g0g0q/290j0f0f040e090b0a0d0g0f0r/290j0f0e060d090b0a0d0j0b0s/290k0e0d070d090c090d0l070u/2b0i0e0d070d080d090d1m/2d0h0d0d070d080d090d1m/2f0f0d0d070d080d090d1m/2f0f0d0d070d080d090d1m/2g0f0c0d070d080d090d1m/2h0e0c0c080d080d090d1m/2h0e0c0c080d080d090d1m/2h0e0c0c080d080d090d1m/2h0e0b0d080d090c090d1m/2h0e0b0d080d090c090d1m/2h0e0a0e080d090c090d1m/2h0e0a0e080d090c090d1m/2h0e0a0e080d090c090d1m/2h0e0a0e080d090c090d1m/2h0e0a0e080d090d080d1m/2h0e0a0e080d0a0c080d1m/1x0c080e0a0e080d0a0c070d1n/1x0c080e0a0e080d0a0c070d1n/1x0c080e0a0e080d0a0c070d1n/1x0c080e0a0e080d0a0c070d1n/1x0c080e0b0e070d0b0c060d1n/1x0c080e0c0d070d0b0c060c1o/1x0c080e0c0e060c0c0d040d1o/1x0c080e0c0e060c0c0d050c1o/1x0d070d0d0e050d0c0e030d1o/1x0e050e0d0e040e0c0t1p/1x0e040f0d0f020f0c0t1p/1x0e040f0d0v0e0s1p/1y0w0e0u0e0r1q/1y0x0d0u0e0r1q/1y0w0f0t0e0r1q/1z0v0f0t0e0q1r/1z0u0g0s0g0p1r/1z0u0h0q0i0o1r/200s0j0p0j0n1r/210r0j0o0k0m1s/210q0l0n0l0k1t/220o0n0m0l0k1t/230m0o0l0n0i1u/240k0q0k0n0h1v/250i0s0h0r0e1w/250g0v0e0t0d1x/270c0y0b0w0b1y/6x/6x/6x/6x/6x/6x/6x/6x/6x/0e0d66/0e0d1a044s/0e0e17080c0w0a0c0q0e0p0d0d0d/0e0e130e0a0w0a0c0q0f0o0d0d0d/0e0e110g0a0w0a0c0o0i0n0d0d0d/0e0e110g0a0w0a0c0n0k0m0d0d0d/0e0e0z0i0a0w0a0c0m0m0l0d0d0d/0d0f0z0i0a0w0a0c0k0p0k0e0c0d/0d0g0x0j0a0w0a0c0k0q0j0e0c0d/0d0g0v0l0a0w0a0c0k0q0j0e0c0d/0d0g0u0m0a0w0a0c0j0s0i0e0c0d/0d0g0u0m0a0w0a0c0i0u0h0f0b0d/0d0g0t0n0a0w0a0c0i0u0h0f0b0d/0d0g0s0o0a0w0a0c0h0w0g0f0b0d/0c0h0r0p0a0w0a0c0g0x0g0f0b0d/0c0i0q0p0a0w0a0c0g0y0f0g0a0d/0c0i0p0q0a0w0a0c0g0y0f0g0a0d/0c0i0o0r0a0w0a0c0g0y0f0g0a0d/0c0i0o0r0a0w0a0c0g0z0e0g0a0d/0c0i0o0r0a0w0a0c0f0g030h0e0h090d/0b0j0n0k0s0c0k0c0f0f070f0d0h090d/0b0k0m0i0u0c0k0c0e0f080f0d0h090d/0b0k0m0h0v0c0k0c0e0f080f0d0h090d/0b0k0m0h0v0c0k0c0e0f080f0d0h090d/0b0k0l0h0w0c0k0c" "0e0e0a0f0c0i080d/0b0k0l0g0x0c0k0c0e0d0b0f0c0i080d/0b0k0k0g0y0c0k0c0e0d0b0f0c0j070d/0a0l0k0f0z0c0k0c0e0d0c0e0c0j070d/0a0l0k0f0z0c0k0c0d0e0c0e0c0j070d/0a0l0k0f0z0c0k0c0d0e0d0d0c0j070d/0a0l0k0e100c0k0c0c0f0d0e0b0k060d/0a0l0j0e110c0k0c0c0f0d0e0b0k060d/090n0i0e110c0k0c0c0e0e0e0b0k060d/090b010b0i0d120c0k0c0c0e0e0e0b0k060d/090b010b0h0e120c0k0c0c0e0e0e0b0l050d/090b010c0g0e120c0k0c0c0e0e0e0b0l050d/090b010c0g0e120c0k0c0c0e0e0f0a0l050d/090b010c0g0e120c0k0c0c0e0e0f0a0l050d/090b010c0f0e130c0k0c0c0e0e0f0a0l050d/090b010c0f0e130c0k0c0c0e0f0e0a0m040d/080b020c0f0e130c0k0c0c0d0g0e0a0m040d/080b020d0e0e130c0k0c0c0d0g0e0a0m040d/080a040c0e0d140c0k0c0c0d0g0e0a0n030d/080b030c0e0e130c0k0c0c0d0g0e0a0n030d/080b030c0e0d140c0k0c0c0d0g0e0a0n030d/080b030c0e0d140c0k0c0c0d0g0e0a0c010a030d/070c030c0e0d140c0k0c0c0d0g0e0a0c010b020d/070c030d0d0d140c0k0c0c0d0g0e0a0c010b020d/070c030d0d0d140c0k0c0c0d0g0e0a0c010b020d/070c030d0d0d140c0k0c0b0e0g0e0a0c020a020d/070c040c0d0d140c0k0c0b0e0g0e0a0c020b010d/070c040c0d0d140c0k0c0b0e0g0e0a0c020b010d/070c040c0d0d140c0k0c0b0e0g0e0a0c020b010d/060d040c0d0d140c0k0c0b0e0g0e0a0c030a010d/060d040c0d0d140c0k0c0b0e0g0e0a0c030o/050e040d0c0d140c0k0c0b0e0g0e0a0c030o/060d040d0c0d140c0k0c0b0e0g0e0a0c030o/060c050c0d0d140c0k0c0b0e0g0e0a0c040n/050d050d0c0d140c0k0c0b0e0g0e0a0c040n/050d050d0c0d140c0k0c0b0e0g0e0a0c040n/050d060d0b0e130c0k0c0b0e0f0f0a0c040n/050d060d0b0e130c0k0c0b0f0e0f0a0c050m/050d060d0b0e130c0k0c0b0f0e0f0a0c050m/050d060d0b0f120c0k0c0b0f0e0f0a0c050m/050d060d0c0e120c0k0c0c0e0e0e0b0c050m/040e050e0c0e120c0k0c0c0e0e0e0b0c060l/040x0c0e120c0k0c0c0e0e0e0b0c060l/040y0b0e120c0k0c0c0e0e0e0b0c060l/040y0c0e110c0k0c0c0e0d0f0b0c060l/040y0c0e110c0k0c0c0e0d0f0b0c070k/040y0c0f100c0k0c0c0f0c0f0b0c070k/040y0d0e100c0k0c0d0e0c0e0c0c070k/03100c0f0z0c0k0c0e0d0c0e0c0c070k/03100c0f0z0c0k0c0e0e0b0e0c0c080j/03100c0g0y0c0k0c0e0e0a0f0c0c080j/03100c0g0y0c0k0c0e0e0a0f0c0c080j/03100d0g0x0c0k0c0e0f090e0d0c090i/03100d0h0w0c0k0c0e0f080f0d0c090i/02110e0h0v0c0k0c0e0g060g0d0c090i/02110e0i0u0c0k0c0e0g050h0d0c090i/02110e0i0u0c0k0c0f0h020h0e0c090i/020d0b0d0e0l03040k0c0k0c0f100e0c0a0h/020d0c0c0e0s0k0c0k0c0g0z0e0c0a0h/020d0c0c0e0s0k0c0k0c0g0y0f0c0a0h/010e0c0d0f0q0k0c0k0c0g0y0f0c0b0g/010e0c0d0f0q0k0c0k0c0g0y0f0c0b0g/010d0d0e0f0p0k0c0k0c0h0w0g0c0b0g/010d0d0e0g0o0k0c0k0c0h0w0g0c0b0g/010d0d0e0g0o0k0c0k0c0i0u0h0c0c0f/010d0d0d0i0n0k0c0k0c0i0t0i0c0c0f/010d0d0e0i0m0k0c0k0c0j0s0i0c0c0f/000e0d0f0h0m0k0c0k0c0k0q0j0c0c0f/000d0e0f0i0l0k0c0k0c0k0p0k0c0d0e/000d0e0f0j0k0k0c0k0c0k0o0l0c0d0e/000d0e0f0l0i0k0c0k0c0m0l0m0c0d0e/000d0f0e0l0i0k0c0k0c0n0j0n0c0d0e/000d0e0f0n0g0k0c0k0c0p0g0o0c0e0d/1t0g0k0c0k0c0q0d0q0c0e0d/1v0e0k0c0k0c0t070t0c0e0d/1z084q", 249, 208, 49, 40, 0xFFFFFFFFu},
    {"190c2v/150j0j0a1z/150l0h0i1r/130p0f0k1p/120r0e0o1l/110t0d0p1k/110u0c0q1j/100v0c0r1i/100w0b0s1h/0z0x0b0t1g/0z0e040f0b0u1f/0z0d050f0b0v1e/0z0d070d0b0c020h1e/0z0c080d0b0c040g1d/0z0c090c0b0b060f1d/0z0c090c0b0b070e1d/0z0c090c0b0b090d1c/1k0c0b0b090d1c/1j0d0b0b090e1b/1j0d0b0b090e1b/1j0d0b0b0a0d1b/1j0d0b0b0a0e1a/1h0e0c0b0b0d1a/1f0g0c0b0b0d1a/190m0c0b0b0d1a/190l0d0b0b0d1a/190k0e0b0b0d1a/190i0g0b0b0d1a/190i0g0b0b0d1a/190h0h0b0b0d1a/190i0g0b0b0d1a/190k0e0b0b0d1a/190m0c0b0b0d1a/190m0c0b0b0d1a/190n0b0b0b0d1a/1f0h0b0b0b0d1a/1g0h0a0b0b0d1a/1h0h090b0b0d1a/1j0f090b0b0d1a/1j0f090b0b0d1a/1j0f090b0a0d1b/1j0g080b0a0d1b/1j0f090b0a0d1b/1j0f090b090e1b/1j0f090b090e1b/0v0e0a0f090b090d1c/0w0d0a0f090b070f1c/0w0d0a0f090b060g1c/0w0e090f090b050g1d/0w0f080f090b030i1d/0w0f060h090v1e/0w0f060h090v1e/0x100a0v1e/0x100a0u1f/0y0x0c0t1g/0z0w0c0s1h/0z0u0e0r1i/110s0e0o1l/110p0h0n1m/130m0i0k1p/170g0k0j1q/4g/4g/4g/4g/4g/4g/4g/3z0908/00070n090g080e09030l0b0a0c0908/000d0h080g0b0c09030l0b0a0c0908/000f0f080f0e0a09030l0b0a0c0908/000h0d080d0h0909030l0b0a0c0908/000h0d080d0i0809030l0b0a0c0908/000j0b080d0h0909030l0b0a0c0908/000j0b080c0j0809030l0b0b0b0908/000k0a080b0l0709030l0b0b0b0908/000k0a080b0l0709030l0a0c0b0908/000l09080a0n0609030l0a0c0b0908/000l09080a0n0609040k0a0c0b0908/000m0808090b030a06090a090f0d0a0908/0009020b0808090a060806090a080g0d0a0908/0009030a08080909070806090b070g0d0a0908/0009030b07080909070707090b070f0e0a0908/0009040a07080908080608090b070f0e0a0908/0009040a0708090808040a090b070f0f090908/0009040a0708090808020c090b070f0f090908/00090509070809080m090b070f0f090908/00090608070809080m090b070f0f090908/00090608070809080m090b070f0f090908/00090608070809070n090b070f0g080908/00090608070808080n090b070f0g080908/00090608070807090n090b070f0g080908/00090608070807090n090b070f0g080908/00090608070807090n090b070e070208080908/00090608070807090n090b070e070208080908/0009060807080709050d05090b070d080209070908/0009060807080709050d05090b070d080209070908/0009070707080709050d05090b070d080209070908/0009070707080709050d05090b070d080209070908/0009070707080709050d05090b070d080209070908/0009070707080709050d05090b070d080209070908/0009060807080709050d05090b070d080209070908/0009060807080709050d05090b070d08020a060908/0009060807080709050d05090b070c08030a060908/0009060807080709050d05090b070c08030a060908/0009060807080809080905090b070c080409060908/000906080708070a090805090b070c080508060908/0009060807080809080905090b070c080409060908/0009060807080809080905090b070b090409060908/0009060807080908080905090b070b0n050908/0009060807080908080905090b070b0n050908/0009050907080908080806090b070b0n050908/0009040a07080909070806090b070b0n050908/0009030b07080909070806090b070b0n050908/0008030b08080909070806090b070b0n050908/0009020b0808090a060707090b070b0o040h/000l0908090a060707090b070b0o040h/000m0808090b050707090b070b0o040h/000l09080a0m07090b070b08060a040h/000l09080a0m07090b070b080709040h/000l09080b0k08090b070b080709040h/000k0a080b0k08090b070a090808040h/000j0b080b0k08090b0709090909030h/000j0b080c0i09090b0709090909030h/000i0c080d0g0a090b070909090" "9030h/000h0c090d0g0a090b0709090909030h/000f0f090e0e0a090b0709090909030h/000c0h0a0f0b0c090b07090909090k/1i0a2o/4g/4g/4g/4g/13090d0907080708090806080t/0d070j0a0c09070807090709060d0o/0d0a0e0e0a09070807090709060f0m/0b0d0b0g0a09070807090808060g0l/0b0d0b0i0809070807090808060h0k/0a0e0b0i0809070808090708060i0j/090f0a0k0709070808090708060j0i/090f090l0709070808090708060k0h/090f090m06090708080a0608060l0g/080g090m06090708080a0608060l0g/080g090m06090708080a0608060m0f/080g090a010b06090708080a0608060m0f/08090205080a040a05090708080a06080609020b0f/07090504060a050a05090708080b05080609030b0e/07090701070a060a04090708080b05080609040a0e/07080g09070a04090708080b0508060905090e/07080g09080904090708080c04080609050a0d/07080g09080904090708080c04080609050a0d/07080g080a0804090708080d0308060906090d/07090f080a0804090708080d0308060906090d/08080e0a090804090708080d0308060906090d/08090d090a0804090708080e0208060907080d/08090d090a0804090708080e0208060907080d/080a0b0a0a0804090708080e0208060907080d/080a0b0a0a0804090708080e0208060907090c/080b0a0a0a0804090708080o060907090c/080b0a090b0804090708080o060907090c/090a0a090b0804090708080o060907090c/090b0a080b0804090708080o060907090c/090c09080b08040907080807010g060907090c/0a0b09080b08040907080807010g060907090c/0a0c08080b08040907080807010g060907090c/0b0c07080b08040907080807010g060907090c/0b0d06080b08040907080807020f060907090c/0c0d05080b08040907080807030e060907090c/0d0d04080b08040907080807020f060907090c/0e0b05080b08040907080807030e060907090c/0f0b04090a08040907080807030e060907090c/0g0a04090a08040907080807030e060907090c/0h0904090a08040907080807030e060907090c/0h0905080a08040907080807040d060906090d/0h0905080a08040907080807040d060906090d/0i0805080909040907080807050c060906090d/0j0705080909040907080807050c060906090d/0j070509070a040907080807050c0609050a0d/0j070509070a040a06080807050c0609050a0d/0j07050a060a050906080807060b0609040b0d/0j07050a050b050905090807070a0609030b0e/0i08050b030b070904090807070a0609020c0e/08020709050c020b0709030a0807070a060n0e/07040609060o070m0807070a060m0f/080i060n080m0807070a060l0g/080i070m080l09070809060l0g/080i070m080l09070809060k0h/080i070l0a0k09070908060k0h/080i090j0a0k09070908060k0h/080h0a0i0c0j09070908060i0j/080g0c0g0d0i0a070908060h0k/080f0e0e0e0i0a070908060g0l/080f0f0c0g0g0b070908060e0n/090c0i080m0c27/0a0b3v", 160, 196, 93, 46, 0xFFFFFFFFu},
    {"000a0e0c020a080g0e0q040a0a0a0f0b0g0b0d/000a0e0c020a080h0d0q040a0a0a0f0c0f0b0d/000a0e0c020a080j0b0q040a0a0a0f0c0f0b0d/000a0e0c020b070m080q040a0a0a0f0c0f0b0d/000a0e0c020a080n070q040a0a0a0f0c0f0b0d/000a0e0b030a080o060q040a0a0a0f0c0f0b0d/000b0d0b030b070o060q040a0a0a0f0c0f0b0d/010b0b0c030b070o060q040a0a0a0f0d0e0b0d/010b0b0c030b070p050q040a0a0a0e0e0e0b0d/010b0a0d030b070p050q040a0a0a0e0e0e0b0d/010b0b0b040a080q040q040a0a0a0e0e0e0b0d/020a0b0b040a080q040q040a0a0a0e0e0e0b0d/020a0b0a050a080a020e040q040a0a0a0e0f0d0b0d/020a0b0a050a080b030d030q040a0a0a0e0f0d0b0d/020a0a0c040a080a060b0b0a0c0a0a0a0e0f0d0b0d/020a0a0b050a080a060b0b0a0c0a0a0a0e0f0d0b0d/020b090b050a080a070a0b0a0c0a0a0a0e0f0d0b0d/020b090b050b070a070a0b0a0c0a0a0a0e0f0d0b0d/020b090b050b070a070a0b0a0c0a0a0a0d0g0d0b0d/030a090b050b070a070a0b0a0c0a0a0a0d0h0c0b0d/030a090a060b070a060b0b0a0c0a0a0a0c0i0c0b0d/030a090a060b070a060b0b0a0c0a0a0a0c0i0c0b0d/0409080b060b070a060b0b0a0c0a0a0a0c0i0c0b0d/040a070a070a080a060b0b0a0c0a0a0a0c0i0c0b0d/040a070a070a080a060b0b0a0c0a0a0a0c0i0c0b0d/040a060b070a080a060b0b0a0c0a0a0a0c0i0c0b0d/0509070a070a080a060b0b0a0c0a0a0a0b0k0b0b0d/0509070a070a080a060b0b0a0c0a0a0a0b0k0b0b0d/0509060b070a080a060b0b0a0c0a0a0a0b0k0b0b0d/0509060b070a080a060b0b0a0c0a0a0a0b0k0b0b0d/040a060a080a080a040d0b0a0c0a0a0a0b0a01090b0b0d/0509060a080a080b020d0c0a0c0a0a0a0b0a02090a0b0d/0608060a080a080q0c0a0c0a0a0a0a0b02090a0b0d/0608050a090a080q0c0a0c0a0a0a0a0b02090a0b0d/0609040b080a080q0c0a0c0a0a0a0a0b02090a0b0d/060a030a090a080p0d0a0c0a0a0a0a0b02090a0b0d/060a030a090a080p0d0a0c0a0a0a0a0b02090a0b0d/060a030a090a080o0e0a0c0a0a0a0a0b02090a0b0d/060a030a090a080o0e0a0c0a0a0a0a0a03090a0b0d/060a030a090a080n0f0a0c0a0a0a0a0a030a090b0d/060a03090a0a080m0g0a0c0a0a0a0a0a030a090b0d/070903090a0a080l0h0a0c0a0a0a0a0a030a090b0d/070a010a0a0a080l0h0a0c0a0a0a0a0a030a090b0d/070a01090b0a080l0h0a0c0a0a0a0a0a030a090b0d/070a01090b0a080l0h0a0c0a0a0a090b040a080b0d/080k0a0a080l0h0a0c0a0a0a080b050a080b0d/080j0b0a080m0g0a0c0a0a0a080b050a080b0d/080j0b0a080m0g0a0c0a0a0a080b050a080b0d/080j0b0a080n0f0a0c0a0a0a080b050a080b0d/080j0b0a080a010c0f0a0c0a0a0a080b050a080b0d/080i0c0a080a020b0f0a0c0a0a0a080q080b0d/080i0c0a080a020b0f0a0c0a0a0a080q080b0d/080h0d0a080a020b0f0a0c0a0a0a070s070b0d/080h0d0a080a020b0f0a0c0a0a0a070s070b0d/090g0d0a080a020c0e0a0c0a0a0a070s070b0d/090g0d0a080a020c0e0a0c0b090a070s070b0d/090g0d0a080a030c0d0a0c0b090a070s070b0d/090g0d0a080a030c0d0a0c0b090a070s070b0d/0a0f0d0a080a030c0d0a0c0b080b070t060b0d/0a0f0d0a080a040b0d0a0c0b070c070t060b0d/0a0e0e0a080a040c0c0a0c0c060c060u060b0d/0a0f0d0a080a040c0c0a0c0c060c060u060b0d/0a0f0d0a080a040c0c0a0c0d040c070u060l03/0b0d0e0a080a050c0b0a0c0f010d070b090a060l03/0b0d0e0a080a050c0b0a0d0s070a0a0b050l03/0b0d0e0a080a060b0b0a0d0s070a0a0b050l03/0b0d0e0a080a060b0b0a0d0s070a0a0b050l03/0c0c0e0a080a060b0b0a0e0q08090b0b050l03/0c0c0e0a080a060b0b0a0e0q08090b0b050l03/0c0c0e0a080a060b0b0a0f0o09090b0b050l03/0c0b0f0a080a060c0a0a0f0o09090b0c040l03/0c0b0f0a080a070b0a0a0g0n09090b0c040l03/0c0b0f0a080a070b0a0a0g0m0a090b0c040l03/0d0a0" "f0a080a070b0a0a0g0m0a090c0b040l03/0d0a0f0a080b070a0a0a0i0i0c090c0b040l03/0d0a0f0a080a080a0a0a0i0h0d090d0a040l03/3g0e22/3i0b23/5w/5w/5w/5w/5w/5w/5w/5w/040c0p0l0e0d0f0c0e0c040q040c0b0d/040h0k0l0e0d0f0c0e0c030r050c0a0d/040j0i0l0e0d0f0c0e0c030r050c0a0d/040k0h0l0e0d0f0c0e0c030r050c0a0d/040l0g0l0e0d0f0c0e0c040q050c0a0c01/040m0f0l0e0d0f0c0e0c040q050c0a0c01/040n0e0l0e0d0f0c0e0c040q060c090c01/040p0c0l0e0d0f0c0e0c040q060c090c01/040p0c0l0e0e0e0c0e0c040q060c080d01/040q0b0l0e0e0e0c0e0c040q070b080c02/040q0b0l0e0f0d0c0e0c040q070b080c02/040r0a0l0e0f0d0c0e0c040q080b070c02/040s090l0e0e0e0c0e0c040q080b070c02/040s090l0e0e0e0c0e0c040q080b070b03/040s090l0e0e0e0c0e0c040q080b060c03/040s090l0e0e0e0c0e0c040q080b060b04/040b050c090l0d0g0d0c0e0c040q080c050c03/040b050d080l0d0g0d0c0e0c040q090b050c03/040b060b090b0m0i0c0c0e0c0b0b0h0b040c04/040b060b090b0m0i0c0c0e0c0c0a0h0b040c04/040b060c080a0o0g0d0c0e0c0b0b0h0b040b05/040b070b080a0o0g0d0c0e0c0b0b0h0c030b05/040b070b080a0o0g0d0c0e0c0b0b0i0b020c05/040b080a080a0o0g0d0c0e0c0b0b0i0b020c05/040b080b070a0n0i0c0c0e0c0b0b0i0b020c05/040b080b070a0n0i0c0c0e0c0b0b0j0a020b06/040b080b070a0n0i0c0c0e0c0b0b0j0b010b06/040b080b070a0n0i0c0c0e0c0b0b0j0n06/040b080b070a0n0i0c0c0e0c0b0b0k0m06/040b080b070a0n0i0c0c0e0c0b0b0k0l07/040b080b070a0n0i0c0c0e0c0b0b0k0l07/040b080b070a0n0i0c0c0e0c0b0b0k0l07/040b080b070a0m0k0b0c0e0c0b0b0k0l07/040b080b070a0m0k0b0c0e0c0b0b0l0k07/040b070c070a0l0a010a0b0c0e0c0b0b0l0j08/040b070c070a0l0a020a0a0c0e0c0b0b0l0j08/040b060c080b0k0a02090b0c0e0c0b0b0l0j08/040b050d080b0k0a020a0a0c0e0c0b0b0l0j08/040b050d080k0b0a020a0a0c0e0c0b0b0m0h09/040b030e090l0a0a020a0a0c0e0c0b0b0m0h09/040c010f090l0a09030a0a0c0e0c0b0b0m0h09/040s090l0a09030a0a0c0e0c0b0b0m0h09/040r0a0k0b09030a0a0c0e0c0b0b0n0f0a/040r0a0k0b09030a0a0c0e0c0b0b0n0f0a/040q0b0k0b09030b090c0e0c0b0b0n0f0a/040q0b0k0b09030b090c0e0c0b0b0o0e0a/040p0c0k0b09030b090c0e0c0b0b0o0d0b/040p0c0k0a0a030c080c0e0c0b0b0o0d0b/040o0d0k0a0a030b090c0e0c0b0b0o0d0b/040o0d0k0a0a030c080c0e0c0b0b0o0d0b/040o0d0k090b030c080c0e0c0b0b0o0c0c/040n0e0k090b030c080c0e0c0b0b0o0c0c/040n0e0k090b030c080c0e0c0b0b0o0c0c/040n0e0k090b030d070c0e0c0b0b0o0c0c/040n0e0k090b030c080c0e0c0b0b0o0c0c/040n0e0k090b030c080c0e0c0b0b0o0c0c/040n0e0b0i0b030d070c0e0c0b0b0o0c0c/040n0e0b0i0a050c070c0e0c0b0b0o0c0c/040b010b0e0b0i0b050b070c0e0c0b0b0o0c0c/040a020b0e0b0i0b050c060c0e0c0b0b0o0c0c/040a020c0d0a0j0b050c060c0e0c0b0b0o0c0c/040a020c0d0a0j0b050c060c0e0c0b0b0o0c0c/040a020c0d0a0j0b050c060c0e0c0b0b0o0c0c/040a030b0d0a0i0c050c060c0e0c0b0b0o0c0c/040a020c0d0a0i0t060c0e0c0b0b0o0c0c/040a030b0d0a0i0t060c0e0c0b0b0o0c0c/040a030c0c0a0h0u060c0e0c0b0b0o0c0c/040a030c0c0a0h0u060c0e0c0b0b0o0c0c/040a040b0c0a0h0u060c0e0c0b0b0o0c0c/040a040c0b0a0h0u060c0e0c0b0b0o0c0c/040a040c0b0a0h0u060c0e0c0b0b0o0c0c/040a040c0b0a0h0u060c0e0c0b0b0o0c0c/040a050b0b0a0h0v050c0e0c0b0b0o0c0c/040a050b0b0a0h0v050c0e0c0b0b0o0c0c/040a050b0b0a0h0v050c0e0c0b0b0o0c0c/040a050b0b0a0h0v050c0e0c0b0b0o0c0c/040a060b0a0b0g0v050c0e0c0b0b0o0c0c/040a060c090l060v050o020c0b0b0o0c0c/040b050c090l050w050n030c0b0b0o0c" "0c/040b050c090m040c090b050n030c0b0b0o0c0c/040b050c090m030c0a0c040n030c0b0b0o0c0c/040b050c090m030c0a0c040n030c0b0b0o0c0c/040b050c090m030c0a0c040n030c0b0b0o0c0c/040b050c090m030c0a0c040n030c0b0b0o0c0c/040b050c090m030c0a0c040n030c0b0b0o0c0c/040b060c080m030c0a0c040n030c0b0b0o0c0c/040b060c080m030b0b0c040n030c0b0b0o0c0c/040b070c070m030b0b0c040n030c0b0b0o0c0c/040b070b080m030b0c0c030n030c0b0b0o0c0c/040b070c070m030b0c0c030n030c0b0b0o0c0c/040b070c070m030b0c0c030n030c0b0b0o0c0c/040b070c070m030b0d0c020n030c0b0b0o0c0c/040b080c060m030b0d0b030n030c0c0a0o0c0c/040a080d060l040c0c0b030n030c0c0a0o0c0c/040a090c060l040b0d0b030n030c0c0a0o0c0c", 212, 181, 68, 54, 0xFFFFFFFFu},
    {"000j0n0j070j0g0l0611/000k0l0l060l0e0m0412/000k0l0m050l0e0m0511/000k0l0l060l0e0m0511/000k0l0l070k0e0l0611/000k0l0l070k0e0l0611/000k0l0l070k0e0k0711/000k0l0l070k0d0m0611/000k0l0l070l0c0l0711/000k0l0l070l0c0l070l0g/000k0l0l080k0c0l070k0h/000k0l0l080k0b0m070j0i/000k0l0l080k0b0l080j0i/000k0l0l080k0b0l080j0i/000k0l0l080k0b0k090j0i/000k0l0l090k0a0k090j0i/000k0l0l090k0a0k090j0i/000k0l0l090k0a0k090j0i/000k0l0l090k0a0k090j0i/000k0l0l0a0j0a0k090j0i/000k0l0l0a0j090k0a0j0i/000k0l0l0a0j090k0a0j0i/000k0l0l0a0j080k0b0j0i/000k0l0l090l070l0a0j0i/000k0l0l090l070k0b0j0i/000k0l0l0b0j080j0b0j0i/000k0l0l0b0j080j0b0j0i/000k0l0l0b0j070k0b0j0i/000k0l0l0b0j070j0c0j0i/000k0l0l0b0j070j0c0j0i/000k0l0l0b0j070i0d0j0i/000k0l0l0c0j060i0d0j0i/000k0l0l0c0j060i0d0j0i/000k0l0l0c0j050j0d0j0i/000k0l0l0c0j050j0d0j0i/000k0l0l0d0i050j0d0k0h/000k0l0l0d0i050i0e0k0h/000k0l0l0d0i050i0e0z02/000k0l0l0d0i050i0e0z02/000k0l0l0d0i040j0e1001/000k0l0l0d0j030i0f1001/000k0l0l0d0j030i0f1001/000k0l0l0d0j030i0f1001/000k0l0l0e0i030i0f1001/000k0l0l0e0i030h0g1001/000k0l0l0e0i030h0g0k0h/000k0l0l0f0h030g0h0j0i/000k0l0l0f0h030g0h0j0i/000k0l0l0f100h0j0i/000k0l0l0f100h0j0i/000k0l0l0f100h0j0i/000k0l0l0f100h0j0i/000k0l0l0f0z0i0j0i/000k0l0l0f0z0i0j0i/000k0l0l0f0y0j0j0i/000k0l0l0g0y0i0j0i/000k0l0l0g0x0j0j0i/000k0l0l0g0x0j0j0i/000k0l0l0g0x0j0j0i/000k0l0l0h0w0j0j0i/000k0l0l0h0w0j0j0i/000k0l0l0h0w0j0j0i/000k0l0l0h0w0j0j0i/000k0l0l0h0v0k0j0i/000k0l0l0i0u0k0j0i/000k0l0l0h0v0k0j0i/000k0l0l0i0t0l0j0i/000k0l0l0i0t0l0j0i/000k0l0l0j0s0l0j0i/000k0l0l0j0s0l0j0i/000k0l0l0j0s0l0j0i/000k0l0l0k0r0l0j0i/000k0l0l0k0r0l0j0i/000k0l0l0k0r0l0j0i/000z060l0k0r0l0j0i/000z060l0k0q0m0k0h/000z060l0l0o0n11/000z060l0l0p0m11/000z060l0l0p0m11/000z060l0l0p0m11/000z060l0l0o0n11/000z060l0l0o0n11/000z060l0l0o0n11/000z060l0l0o0m12/000z060l0l0o0m12/000z070k0m0n0n11/1o012y/4n/4n/4n/4n/4n/4n/4n/4n/3b0o0o/0v0k0f160b0p0n/0w0k0e170a0q0m/0w0k0e170a0q0m/0w0k0e170a0q0m/0w0k0e170a0q0m/0w0k0e170a0q0m/0w0k0e170a0q0m/0w0k0o0l0m0q0m/0w0k0p0k0m0q0m/0w0k0p0k0m0q0m/0w0k0q0j0m0q0m/0w0k0p0k0m0q0m/0w0k0p0k0m0q0m/0w0k0p0k0m0q0m/0w0k0p0k0m0q0m/0w0k0p0k0m0q0m/0w0k0p0k0m0q0m/0w0k0p0k0m0q0m/0w0k0p0k0m0q0m/0w0k0p0k0n0p0m/0w0k0p0k0n0o0n/0w0k0p0k0n0o0n/0w0k0p0k0n0o0n/0w0k0p0k0o0n0n/0w0k0p0k0o0n0n/0w0k0p0k0o0n0n/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0m0o/0w0k0p0k0o0l0p/0w0k0p0k0o0k0q/0w0k0p0k0o0l0p/0w0k0p0k0o0l0p/0w0k0p0k0o0l0p/0w0k0p0k0o0l0p/0w0k0p0k0p0j0q/0w0k0p0k0q0i0q/0w0k0p0k0q0i0q/0w0k0p0k0q0h0r/0w0k0p0k0q0i0q/0w0k0p0k0q0i0q/0w0k0p0k0q0i0q/0w0k0p0k0q0i0q/0w0k0p0k0q0i0q/0w0k0p0k0q0i0q/0w0k0p0k0q0i0q/0w0k0p0k0q0i0q/0w0k0p0k0q0h0r/0w0k0p0k0q0h0r/0w0k0p0k0q0g0s/0w0k0p0k0q0g0s/0w0k0p0k0q0g0s/0w0k0p0k0q0g0s/0w0k0p0k0q0g0s/0w0k0p0k0q0g0s/0w0k0p0k0q0g0s/0w0k0p0k0q0g0s/0w0k0p0k0q0g0s/0w0k0p0k0q0g0s/0w0k0p0k1y/0w0k0p0k1y/0w0k0p0k1y/0w0k0p0k1y/0w0k0p0k1y/0w0k0p0k1y/0w0k0p0k1y/0w0k0p0k1y/0w0k0p0k0n0n0o/0w0k0p0k0n0o0n/0w0k0p0k0n0n0o/0w0k0p0k0n0n0o/0w0k0p0k0n0n0o/0w0k0p" "0k0n0n0o/0w0k0p0k0n0n0o/0w0k0p0k0n0n0o/0w0k0p0k0m0o0o/0w0k0p0k0n0n0o/0w0k0p0j0o0n0o", 167, 182, 89, 53, 0xFFF848F8u},
};
static void make_card(int id, const Card &c) { std::vector<uint32_t> px(c.w * c.h, 0); int y = 0, x = 0; bool on = false; auto dig = [](char k) { return k >= 'a' ? k - 'a' + 10 : k - '0'; };
    for (const char *s = c.rle; *s; ) { if (*s == '/') { y++; x = 0; on = false; s++; continue; } int n = dig(s[0]) * 36 + dig(s[1]); s += 2; if (on) for (int i = 0; i < n && x + i < c.w && y < c.h; i++) px[y * c.w + x + i] = c.col; x += n; on = !on; }
    add_bitmap(id, c.w, c.h, px); }
static void make_hud_x() {
    for (int d = 0; d < 10; d++) make_glyph(BM_DIGIT + d, (char)('0' + d), 24, 26, 2.4f, 0xFF4418D0u, 0xFF5C14C0u);   /* crimson: the brochure reads red, the small stills magenta */
    static const char L[] = "INSERTCD"; for (int i = 0; L[i]; i++) make_glyph(BM_LET + i, L[i], 18, 20, 1.7f, 0xFF30C8F8u, 0xFF1878F0u);
    { std::vector<uint32_t> px(44 * 44, 0); const float c = 21.5f; const uint32_t O = 0xFF30B8F0u; for (int y = 0; y < 44; y++) for (int x = 0; x < 44; x++) { float dx = x - c, dy = y - c, r = sqrtf(dx * dx + dy * dy); bool on = fabsf(r - 10.5f) < 1.1f; float ax = fabsf(dx), ay = fabsf(dy); if (r > 15.f && r < 20.f && fabsf(ax - ay) < 1.1f) on = true; if (on) px[y * 44 + x] = O; } add_bitmap(BM_RING, 44, 44, px); }
    { const int W = 148, H = 19; std::vector<uint32_t> px(W * H, 0xFFB8E0E8u); for (int x = 0; x < W; x++) { px[x] = 0xFFFFFFFFu; px[W + x] = 0xFFE8F4F4u; } for (int y = 0; y < H; y++) for (int x = W / 2 - 1; x <= W / 2; x++) px[y * W + x] = 0xFF708890u;
      for (int s = 0; s < 2; s++) for (int y = 6; y < 15; y++) for (int x = 0; x < 58; x++) px[y * W + (s ? W / 2 + 8 : 8) + x] = 0xFF485868u; add_bitmap(BM_PANEL, W, H, px); }
    add_bitmap(BM_BAR, 4, 4, std::vector<uint32_t>(16, 0xFF2088E8u));
    add_bitmap(BM_WIPE, 4, 4, std::vector<uint32_t>(16, 0xFFF848F8u));
    for (int i = 0; i < 4; i++) make_card(BM_CARD + i, CARDS[i]);
}
static void insert_credit(VPIX_Frame &f) { static const char *s = "INSERT CREDIT"; static const char L[] = "INSERTCD"; float x = 28.f; for (const char *c = s; *c; c++) { const char *p = strchr(L, *c); if (*c != ' ' && p) blit(f, BM_LET + (int)(p - L), 18, 20, x, 20.f, 1.f); x += 23.f; } }
/* ---- the valley: rolling land up to the dam, a lake behind it, hills to both sides ---- */
static const float VW = 95000.f;   /* half width of the valley floor */
static float land_h(float x, float y) {
    float side = clampf((fabsf(x) - VW) / 240000.f, 0.f, 1.f); side = side * side * (3 - 2 * side);
    float roll = (noise2(x / 70000.f, y / 70000.f, 1) - 0.5f) * 24000.f + (noise2(x / 26000.f, y / 26000.f, 2) - 0.5f) * 7000.f;
    float h = roll + side * (70000.f + 90000.f * noise2(x / 90000.f, y / 90000.f, 3));
    if (y > DAM_Y) { float k = clampf((y - DAM_Y) / 30000.f, 0.f, 1.f); float base = -9000.f + side * 90000.f * noise2(x / 90000.f, y / 90000.f, 3) + (1 - k) * 8000.f; h = base; }   /* the lake bed */
    return h;
}
static uint32_t land_rgb(float x, float y, float h) {
    float a = noise2(x / 24000.f, y / 24000.f, 7), b = noise2(x / 9000.f, y / 9000.f, 8);
    uint32_t c = a > 0.60f ? 0xB8E8B8 : a > 0.42f ? 0x60A868 : 0x48844E; c = mix(c, b > 0.5f ? 0xA8DCA8 : 0x387840, 0.28f);
    if (h > 26000.f) c = mix(c, 0x9A9048, clampf((h - 26000.f) / 40000.f, 0.f, 0.72f));
    return c;
}
static void build_land() {
    Mesh m; const int NX = 56, NY = 96; const float DX = 18000.f, DY = 18000.f, X0 = -NX * DX * 0.5f, Y0 = -200000.f;
    auto P = [&](int i, int j, uint32_t &c) { float x = X0 + i * DX, y = Y0 + j * DY, h = land_h(x, y); c = land_rgb(x, y, h); return V(x, y, h); };
    for (int i = 0; i < NX; i++) for (int j = 0; j < NY; j++) { uint32_t c0, c1, c2, c3; V a = P(i, j, c0), b = P(i + 1, j, c1), d = P(i + 1, j + 1, c2), e = P(i, j + 1, c3); grad(m, {a, b, d}, {c0, c1, c2}); grad(m, {a, d, e}, {c0, c2, c3}); }
    m.add(M_LAND);
    { Mesh s; const float R = 3000000.f; static const float EL[6] = {-1.2f, -0.004f, 0.012f, 0.035f, 0.12f, 1.2f}; static const uint32_t C[6] = {0x50A860, 0x50A860, 0xF8D8A0, 0xF8B8E0, 0x9C9CFF, 0x8080F4};   /* green under the horizon, cream, pink, lilac */
      for (int i = 0; i < 32; i++) for (int e = 0; e < 5; e++) { float a0 = 6.2831853f * i / 32, a1 = 6.2831853f * (i + 1) / 32; auto Q = [&](float a, float l) { return V(cosf(a) * cosf(l) * R, sinf(a) * cosf(l) * R, sinf(l) * R); };
          grad(s, {Q(a0, EL[e]), Q(a0, EL[e + 1]), Q(a1, EL[e + 1]), Q(a1, EL[e])}, {C[e], C[e + 1], C[e + 1], C[e]}); }
      s.add(M_SKY); }
    { Mesh d; const float T = 34000.f, H0 = -6000.f; const int NP = 10; const float PW = 2.f * (VW + 90000.f) / NP;   /* the dam: slanted buttress panels */
      for (int i = 0; i < NP; i++) { float x0 = -(VW + 90000.f) + i * PW, x1 = x0 + PW; uint32_t c = (i & 1) ? 0x949490 : 0xA8A8A4;
          V a(x0 + 900, DAM_Y - 26000, H0), b(x1 - 900, DAM_Y - 26000, H0), e(x1 - 900, DAM_Y, T), g(x0 + 900, DAM_Y, T); grad(d, {a, b, e, g}, {lit(c, 0.72f), lit(c, 0.72f), c, c});
          quad(d, V(x0 - 100, DAM_Y - 27500, H0), V(x0 + 900, DAM_Y - 26000, H0), g, V(x0 - 100, DAM_Y - 400, T), 0x505050); quad(d, V(x1 - 900, DAM_Y - 26000, H0), V(x1 + 100, DAM_Y - 27500, H0), V(x1 + 100, DAM_Y - 400, T), e, 0x606060); }
      quad(d, V(-(VW + 90000.f), DAM_Y, T), V(VW + 90000.f, DAM_Y, T), V(VW + 90000.f, DAM_Y + 7000, T), V(-(VW + 90000.f), DAM_Y + 7000, T), 0xB8B8B4);   /* the top */
      d.add(M_DAM); }
    { Mesh j; const float W = 5600.f, TY = 15800.f, TZ = 23000.f;   /* the clip at 0:14 and xs_pic1: a dark outlet two-thirds up a panel, a glowing blue fall a third of a panel wide, white at the foot */
      grad(j, {V(-W, -300, 0), V(W, -300, 0), V(W * 0.45f, TY - 300, TZ), V(-W * 0.45f, TY - 300, TZ)}, {0xB8E8FF, 0xB8E8FF, 0x1070E0, 0x1070E0}); grad(j, {V(-W * 0.4f, -500, 1500), V(W * 0.4f, -500, 1500), V(W * 0.2f, TY - 500, TZ - 500), V(-W * 0.2f, TY - 500, TZ - 500)}, {0xF0FCFF, 0xF0FCFF, 0x50B8FF, 0x50B8FF});
      for (int k = 0; k < 8; k++) { float a0 = k * 0.7854f, a1 = (k + 1) * 0.7854f; auto Q = [&](float a, float r) { return V(cosf(a) * r, TY - 700 + sinf(a) * r * 0.6f, TZ + 1600 + sinf(a) * r * 0.9f); }; j.poly({Q(a0, 3400), Q(a1, 3400), V(0, TY - 700, TZ + 1600)}, 0x101014); j.poly({Q(a0, 1500), Q(a1, 1500), V(0, TY - 800, TZ + 2300)}, k < 4 ? 0xE8E8E8 : 0x303034); }
      for (int i = 0; i < 44; i++) { float x = rnd(-W * 1.7f, W * 1.7f), y = rnd(-6000.f, 800.f), z = rnd(0.f, 5000.f), sz = rnd(300.f, 950.f); j.poly({V(x - sz, y, z), V(x + sz, y, z), V(x, y, z + sz * 1.6f)}, 0xF8F8FF); }
      j.add(M_JET); }
    { Mesh l; const float Y1 = DAM_Y; for (int i = 0; i < 18; i++) for (int k = 0; k < 8; k++) { float x0 = -800000.f + i * 90000.f, x1 = x0 + 90000.f, y0 = Y1 + k * 165000.f, y1 = y0 + 165000.f;
              auto C = [&](float x, float y) { return mix(0x1C80D0, 0x38B8E0, noise2(x / 80000.f, y / 80000.f, 11)); }; grad(l, {V(x0, y0, 15000), V(x1, y0, 15000), V(x1, y1, 15000), V(x0, y1, 15000)}, {C(x0, y0), C(x1, y0), C(x1, y1), C(x0, y1)}); }
      l.add(M_LAKE); }
    { Mesh r; std::mt19937 q(9);   /* the dark rock of the far shore */
      for (int i = 0; i < 70; i++) { float x = rnd(-560000.f, 560000.f), y = rnd(1300000.f, 1480000.f), w = rnd(50000.f, 130000.f), h = rnd(30000.f, 85000.f) + 15000.f * (fabsf(x) < 260000.f ? 2.f : 1.f); static const uint32_t C[4] = {0x3E2A1C, 0x6A4A30, 0x241810, 0x7E6444};
          V t(x + rnd(-4000, 4000), y, h); for (int k = 0; k < 5; k++) { float a0 = 6.2831853f * k / 5, a1 = 6.2831853f * (k + 1) / 5; r.poly({V(x + cosf(a0) * w, y + sinf(a0) * w, 15000), V(x + cosf(a1) * w, y + sinf(a1) * w, 15000), t}, C[q() % 4]); } }
      r.add(M_ROCK); }
    /* the river between rock pyramids (xs_pic2): pale water far off, deep blue near, faces in black, brown and green */
    { Mesh w; for (int j = 0; j < 30; j++) for (int i = 0; i < 12; i++) { float x0 = -540000.f + i * 90000.f, x1 = x0 + 90000.f, y0 = VALLEY_Y0 - 200000.f + j * 47000.f, y1 = y0 + 47000.f; auto C = [&](float x, float y) { return mix(0x0084B5, 0x28A0D0, noise2(x / 50000.f, y / 50000.f, 21)); }; grad(w, {V(x0, y0, 8000), V(x1, y0, 8000), V(x1, y1, 8000), V(x0, y1, 8000)}, {C(x0, y0), C(x1, y0), C(x1, y1), C(x0, y1)}); }
      w.add(M_RIVER); }
    { Mesh p; std::mt19937 q(17); auto R = [&](float a, float b) { return a + (b - a) * ((q() % 10000) / 10000.f); }; static const uint32_t RK[5] = {0x302222, 0x4A3A2A, 0x140C0C, 0x5C4A38, 0x221A14}; static const float FACE[4] = {1.f, 0.62f, 0.18f, 0.8f};
      for (int n = 0; n < 74; n++) { float side = n & 1 ? 1.f : -1.f, h = R(70000.f, 210000.f), hw = h * R(0.62f, 0.85f), x = side * (R(62000.f, 330000.f) + hw), y = VALLEY_Y0 - 150000.f + (n / 2) * 36000.f + R(-9000.f, 9000.f); bool green = q() % 4 == 0;
          V c[4] = {V(x - hw, y - hw, 6000), V(x + hw, y - hw, 6000), V(x + hw, y + hw, 6000), V(x - hw, y + hw, 6000)}, t(x, y, h);
          for (int k = 0; k < 4; k++) { V a = c[k], b = c[(k + 1) & 3]; const int RW = 4; for (int r = 0; r < RW; r++) { float u0 = (float)r / RW, u1 = (float)(r + 1) / RW; V l0 = a + (t - a) * u0, r0 = b + (t - b) * u0, l1 = a + (t - a) * u1, r1 = b + (t - b) * u1; int cols = RW - r;
                  for (int s = 0; s < cols; s++) { float s0 = (float)s / cols, s1 = (float)(s + 1) / cols; uint32_t col = lit(RK[q() % 5], FACE[k]); if (green && r == RW - 1 && k != 2) col = lit(0x48A048, FACE[k]); V p00 = l0 + (r0 - l0) * s0, p01 = l0 + (r0 - l0) * s1, p10 = l1 + (r1 - l1) * s0, p11 = l1 + (r1 - l1) * s1; if (u1 >= 0.999f) p.poly({p00, p01, t}, col); else quad(p, p00, p01, p11, p10, col); } } } }
      p.add(M_PYRS); }
}
/* ---- the launch tube: octagon of streaky panels with black beams along the corners and a black rib at each joint ---- */
static void build_tube() {
    Mesh m; std::mt19937 q(3); static const uint32_t TONE[7] = {0xF0F4EC, 0xDCE4DC, 0xC4CCC4, 0x9CA4A4, 0x5C6064, 0x2C2E32, 0x6E4A44}; const float L = TUBE_L, R = TUBE_R;
    auto P = [&](float a, float rr, float y) { return V(cosf(a) * rr, y, sinf(a) * rr); };
    for (int s = 0; s < 8; s++) { float a0 = s * 0.7853982f, a1 = (s + 1) * 0.7853982f; const int A = 7, B = 8;
        for (int i = 0; i < A; i++) for (int j = 0; j < B; j++) { float t0 = (float)i / A, t1 = (float)(i + 1) / A, y0 = j * L / B, y1 = (j + 1) * L / B; V e0 = P(a0, R, 0), e1 = P(a1, R, 0); V p0 = e0 + (e1 - e0) * t0, p1 = e0 + (e1 - e0) * t1; static uint32_t band[7]; if (j == 0) band[i] = TONE[q() % 7]; uint32_t c = mix(band[i], TONE[q() % 7], 0.45f); if (q() % 4) c = mix(c, TONE[q() % 3], 0.45f);
                V a(p0.x, y0, p0.z), b(p1.x, y0, p1.z), d(p1.x, y1, p1.z), e(p0.x, y1, p0.z); quad(m, a, b, d, e, c); }
        const float KW = 0.075f, INK = 0.94f; V c0 = P(a0 - KW, R * 0.999f, 0), c1 = P(a0 + KW, R * 0.999f, 0), k0 = P(a0 - KW * 0.6f, R * INK, 0), k1 = P(a0 + KW * 0.6f, R * INK, 0);
        quad(m, V(k0.x, 0, k0.z), V(k1.x, 0, k1.z), V(k1.x, L, k1.z), V(k0.x, L, k0.z), 0x0C0C0E); (void)c0; (void)c1;
        V r0 = P(a0, R * 0.999f, 0), r1 = P(a1, R * 0.999f, 0), r2 = P(a1, R * 0.90f, 0), r3 = P(a0, R * 0.90f, 0); quad(m, V(r0.x, 0, r0.z), V(r1.x, 0, r1.z), V(r2.x, 0, r2.z), V(r3.x, 0, r3.z), 0x0A0A0C); quad(m, V(r0.x, 900, r0.z), V(r1.x, 900, r1.z), V(r2.x, 900, r2.z), V(r3.x, 900, r3.z), 0x5A2A24);
        quad(m, V(r2.x, 0, r2.z), V(r3.x, 0, r3.z), V(r3.x, 900, r3.z), V(r2.x, 900, r2.z), 0x101012); }
    for (int i = 0; i < 16; i++) { float a = (q() % 628) / 100.f, r = R * (0.3f + (q() % 60) / 100.f), y = (q() % 1000) / 1000.f * L, s = 34.f; V c(cosf(a) * r, y, sinf(a) * r); quad(m, c + V(-s, 0, -s), c + V(s, 0, -s), c + V(s, 0, s), c + V(-s, 0, s), 0xF8F8F0); }
    m.add(M_TUBE);
    { Mesh s; std::mt19937 r(12); const float RR = 2400000.f; for (int i = 0; i < 520; i++) { float z = ((r() % 2000) / 1000.f) - 1.f, a = (r() % 6283) / 1000.f, c = sqrtf(1 - z * z); V n(cosf(a) * c, sinf(a) * c, z), u = norm(cross(n, V(0.31f, 0.52f, 0.8f))), w = cross(n, u); float k = 4200.f + (r() % 3000); V p = n * RR; uint32_t col = r() % 3 ? 0xE8E8E8 : 0xA0A8B8; quad(s, p + u * k, p + w * k, p - u * k, p - w * k, col); }
      s.add(M_STARS); }
}
/* ---- the dark planet: black sky over a blue glow, blue surface with pale patches ---- */
static void build_night() {
    { Mesh s; const float R = 2800000.f; static const float EL[6] = {-1.2f, -0.02f, 0.02f, 0.10f, 0.32f, 1.2f}; static const uint32_t C[6] = {0x20407C, 0x3C5C9C, 0x6080C8, 0x1C2C80, 0x060A28, 0x020208};
      for (int i = 0; i < 32; i++) for (int e = 0; e < 5; e++) { float a0 = 6.2831853f * i / 32, a1 = 6.2831853f * (i + 1) / 32; auto Q = [&](float a, float l) { return V(cosf(a) * cosf(l) * R, sinf(a) * cosf(l) * R, sinf(l) * R); }; grad(s, {Q(a0, EL[e]), Q(a0, EL[e + 1]), Q(a1, EL[e + 1]), Q(a1, EL[e])}, {C[e], C[e + 1], C[e + 1], C[e]}); }
      s.add(M_NSKY); }
    { Mesh g; for (int i = 0; i < 18; i++) for (int j = 0; j < 40; j++) { float x0 = -1350000.f + i * 150000.f, x1 = x0 + 150000.f, y0 = -300000.f + j * 100000.f, y1 = y0 + 100000.f; auto C = [&](float x, float y) { return mix(0x28487C, 0x3C5C94, noise2(x / 120000.f, y / 60000.f, 31)); }; grad(g, {V(x0, y0, 0), V(x1, y0, 0), V(x1, y1, 0), V(x0, y1, 0)}, {C(x0, y0), C(x1, y0), C(x1, y1), C(x0, y1)}); }
      g.add(M_SEA); }
    { Mesh p; std::mt19937 q(41); auto R = [&](float a, float b) { return a + (b - a) * ((q() % 10000) / 10000.f); };
      for (int n = 0; n < 230; n++) { float cx = R(-1200000.f, 1200000.f), cy = R(-250000.f, 3600000.f), wx = R(30000.f, 170000.f), wy = R(14000.f, 60000.f); const int K = 7; V rim[K]; for (int k = 0; k < K; k++) { float a = 6.2831853f * k / K, r = R(0.65f, 1.15f); rim[k] = V(cx + cosf(a) * wx * r, cy + sinf(a) * wy * r, 300); } V c(cx, cy, 350); uint32_t in = q() % 3 ? 0xF0F0C8 : 0xB8D0A0, out = 0x789070; for (int k = 0; k < K; k++) grad(p, {c, rim[k], rim[(k + 1) % K]}, {in, out, out}); }
      p.add(M_PATCH); }
    { Mesh m; const float R = 2300.f, H = 62000.f; const int RINGS = 16; for (int i = 0; i < RINGS; i++) { float z0 = i * H / RINGS, z1 = (i + 1) * H / RINGS; uint32_t c = i & 1 ? 0x70A060 : 0xB8D8A8; for (int s = 0; s < 6; s++) { float a0 = s * 1.0472f, a1 = (s + 1) * 1.0472f; quad(m, V(cosf(a0) * R, sinf(a0) * R, z0), V(cosf(a1) * R, sinf(a1) * R, z0), V(cosf(a1) * R, sinf(a1) * R, z1), V(cosf(a0) * R, sinf(a0) * R, z1), s & 1 ? lit(c, 0.8f) : c); } }
      const uint32_t D = 0x20242C; m.box(V(-1800, -1800, H), V(1800, 1800, H + 2600), D); m.poly({V(-9000, 0, H + 4200), V(0, -3800, H + 800), V(9000, 0, H + 4200), V(0, 3800, H + 800)}, D); m.add(M_PYLON); }
    { Mesh m; const uint32_t D = 0x401410, E = 0x701C14; for (int s = 0; s < 6; s++) { float a0 = s * 1.0472f, a1 = (s + 1) * 1.0472f; quad(m, V(cosf(a0) * 1800, sinf(a0) * 1800, 0), V(cosf(a1) * 1800, sinf(a1) * 1800, 0), V(cosf(a1) * 1000, sinf(a1) * 1000, 26000), V(cosf(a0) * 1000, sinf(a0) * 1000, 26000), s & 1 ? D : E); }
      d1k::kit_drum(m, V(0, -700, 26000), V(0, 1400, 0), V(1, 0, 0), V(0, 0, 1), 3600, 0x902018, D); m.add(M_TOWER); }
    { Mesh m; const int RINGS = 26; const float LEN = 130000.f, R1 = 26000.f; for (int i = 0; i < RINGS; i++) { float y0 = i * LEN / RINGS, y1 = (i + 1) * LEN / RINGS, r0 = 2600.f + (R1 - 2600.f) * i / RINGS, r1 = 2600.f + (R1 - 2600.f) * (i + 1) / RINGS; for (int s = 0; s < 14; s++) { float a0 = s * 0.4488f, a1 = (s + 1) * 0.4488f; uint32_t c = (i + s) & 1 ? 0xA8C8A0 : 0x88AC88; quad(m, V(cosf(a0) * r0, y0, sinf(a0) * r0), V(cosf(a1) * r0, y0, sinf(a1) * r0), V(cosf(a1) * r1, y1, sinf(a1) * r1), V(cosf(a0) * r1, y1, sinf(a0) * r1), c); } }
      m.add(M_CONE); }
}
/* ---- the craft: white, long swept wings with a slight gull, dark pods; the attract's lead ship and the fighters; you never see it in play ---- */
static void build_craft(int id, uint32_t band) {
    Mesh m; const uint32_t W = 0xF8F8F4, W2 = 0xC8C8C4, W3 = 0x989894, DK = 0x18181C;
    V nose(0, 4600, 0); const float S = 1080.f; V a(-S, 0, S * 0.65f), b(S, 0, S * 0.65f), c(S, 0, -S * 0.45f), d(-S, 0, -S * 0.45f); V e(-S * 0.7f, -3200, S * 0.55f), f(S * 0.7f, -3200, S * 0.55f), g(S * 0.7f, -3200, -S * 0.4f), h(-S * 0.7f, -3200, -S * 0.4f);
    m.poly({a, b, nose}, W); m.poly({b, c, nose}, W2); m.poly({d, a, nose}, W2); m.poly({c, d, nose}, W3); quad(m, a, e, f, b, W); quad(m, b, f, g, c, W2); quad(m, a, d, h, e, W2); quad(m, d, c, g, h, W3); quad(m, e, h, g, f, DK);
    quad(m, V(-S * 0.5f, 1600, S * 0.4f), V(S * 0.5f, 1600, S * 0.4f), V(S * 0.25f, 3000, S * 0.25f), V(-S * 0.25f, 3000, S * 0.25f), 0x303040);   /* canopy */
    quad(m, V(-S * 0.95f, -1200, S * 0.62f), V(S * 0.95f, -1200, S * 0.62f), V(S * 0.95f, -500, S * 0.66f), V(-S * 0.95f, -500, S * 0.66f), band);
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; V r0(q * S, 1500, 200), r1(q * S, -2400, 200), mid(q * 3400, -900, 950), mid2(q * 3400, -3500, 950), tip(q * 5900, -3100, 150), tip2(q * 5900, -4900, 150);
        /* a solid wing: thick at the root, thinner at the tip, with an underside and all four edges */
        auto panel = [&](V a0, V a1, V b1, V b0, float t0, float t1, uint32_t top) { V d0(0, 0, -t0), d1(0, 0, -t1), a2 = a0 + d0, a3 = a1 + d0, b3 = b1 + d1, b2 = b0 + d1; bool fl = k != 0;
            auto face = [&](V p, V qq, V rr, V u, uint32_t cc) { if (fl) quad(m, p, qq, rr, u, cc); else quad(m, u, rr, qq, p, cc); };
            face(a0, b0, b1, a1, top); face(a3, b3, b2, a2, W3); face(a2, b2, b0, a0, W2); face(a1, b1, b3, a3, W3); face(b0, b2, b3, b1, W3); };
        panel(r0, r1, mid2, mid, 760.f, 620.f, W); panel(mid, mid2, tip2, tip, 620.f, 340.f, W2);
        d1k::kit_drum(m, V(q * 2200, -2300, 280), V(0, 1400, 0), V(1, 0, 0), V(0, 0, 1), 260, DK, 0x303034); }
    m.add(id);
}
static void build_roid(int id, int seed) {   /* a lumpy dark ball with pale flecks, as the mottled rock in the clip at 0:16 */
    Mesh m; const int NA = 10, NB = 7; auto P = [&](int a, int b) { float t = 3.14159f * b / NB, p = 6.2831853f * a / NA; V n(sinf(t) * cosf(p), sinf(t) * sinf(p), cosf(t)); float r = 780.f + 420.f * noise2(a * 0.9f + seed * 7.f, b * 0.9f, seed); if (a % NA == 0) r = 780.f + 420.f * noise2(seed * 7.f, b * 0.9f, seed); return n * r; };
    std::mt19937 q(seed + 90); static const uint32_t C[6] = {0x3E2A1C, 0x6A5A48, 0x241810, 0x7E6C5A, 0x302828, 0xB0A898};
    for (int a = 0; a < NA; a++) for (int b = 0; b < NB; b++) { V p0 = P(a, b), p1 = P(a + 1, b), p2 = P(a + 1, b + 1), p3 = P(a, b + 1); uint32_t c = C[q() % 6]; if (b == 0) m.poly({p0, p3, p2}, c); else if (b == NB - 1) m.poly({p0, p3, p1}, c); else quad(m, p0, p3, p2, p1, c); }
    m.add(id);
}
/* ---- what you see of your own ship: diamonds near the corners, the forward-swept yellow wings when you look to the side ---- */
static void build_cockpit(int id, uint32_t yel, uint32_t grn) {
    Mesh m; const float D = 2000.f, X = 0.577f * D, Z = 0.434f * D;
    auto diamond = [&](float nx, float nz, float w, float h) { V c(nx * X, D, nz * Z); float W = w * X, H = h * Z; V l = c + V(-W, 0, 0), r = c + V(W, 0, 0), t = c + V(0, 0, H), b = c + V(0, 0, -H); quad(m, l, t, r, b, yel); V e(0, 30, 0); float S = 0.28f; m.poly({l + e, b + e, b + (l - b) * S + V(0, 0, -H * S) + e}, grn); m.poly({r + e, b + e, b + (r - b) * S + V(0, 0, -H * S) + e}, grn); };
    diamond(0.74f, 1.02f, 0.20f, 0.14f); diamond(-0.74f, 1.02f, 0.20f, 0.14f); diamond(0.70f, -1.03f, 0.22f, 0.16f); diamond(-0.72f, -1.03f, 0.22f, 0.16f);
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; V tip(q * 10000, 7000, -700), r0(q * 1500, -1200, -1100), r1(q * 1500, -9000, -1400), back(q * 7000, 1800, -900); bool f = k != 0; if (f) quad(m, tip, r0, r1, back, yel); else quad(m, back, r1, r0, tip, yel); }
    m.add(id);
}
static void build_small() {
    { Mesh s; const uint32_t Y = 0xF0B030; quad(s, V(-46, 0, 0), V(46, 0, 0), V(0.6f, 1, 0), V(-0.6f, 1, 0), Y); quad(s, V(-16, 0, 1), V(16, 0, 1), V(0.3f, 1, 1), V(-0.3f, 1, 1), 0xF8E070); s.add(M_BEAM); }
    { Mesh s; const float W = 220.f; quad(s, V(-W, -2200, 0), V(W, -2200, 0), V(W, 2200, 0), V(-W, 2200, 0), 0xF0D040); quad(s, V(0, -2200, -W), V(0, -2200, W), V(0, 2200, W), V(0, 2200, -W), 0xF0D040); s.add(M_BOLT); }
    { Mesh b; std::mt19937 q(6); static const uint32_t C[6] = {0xF08818, 0xF0A020, 0xE85010, 0xD82818, 0xF0C840, 0xE86818};   /* solid lumps: yellow and orange inside, red at the edge, as the clip at 0:02 and 0:21 */
      for (int i = 0; i < 240; i++) { float a = (q() % 628) / 100.f, z = ((q() % 200) - 100) / 100.f, s = sqrtf(1 - z * z), rr = 0.45f + (q() % 55) / 100.f; V n(cosf(a) * s, sinf(a) * s, z), u = norm(cross(n, V(0.3f, 0.5f, 0.8f))), w = cross(n, u), c = n * (1000.f * rr); float t = 120.f + q() % 110; uint32_t col = rr > 0.82f ? C[q() % 2 + 2] : (q() % 2 ? C[q() % 2] : C[4]);
          b.poly({c + u * t + w * t * 0.3f, c + w * t, c - u * t + w * t * 0.2f, c - u * t * 0.6f - w * t, c + u * t * 0.6f - w * t}, col); }
      for (int i = 0; i < 44; i++) { float a = i * 2.399f, z = -1.f + 2.f * i / 43.f, s = sqrtf((std::max)(0.f, 1 - z * z)); V n(cosf(a) * s, sinf(a) * s, z), u = norm(cross(n, V(0.3f, 0.5f, 0.8f))), w = cross(n, u), c = n * 480.f; float t = 300.f; b.poly({c + u * t, c + u * t * 0.3f + w * t, c - u * t * 0.8f + w * t * 0.6f, c - u * t * 0.8f - w * t * 0.6f, c + u * t * 0.3f - w * t}, i & 1 ? 0xF0C030 : 0xF09020); }
      b.add(M_BOOM); }
    { Mesh p; quad(p, V(-1800, -1200, 0), V(1600, -1500, 0), V(1800, 1300, 200), V(-1500, 1100, -200), 0xC8C8C4); quad(p, V(-1500, 1100, -200), V(1800, 1300, 200), V(1600, -1500, 0), V(-1800, -1200, 0), 0x989894); p.add(M_PLATE); }
    { Mesh p; p.poly({V(-500, -700, 0), V(600, -300, 0), V(0, 800, 0)}, 0xF8F8F8); p.poly({V(0, 800, 0), V(600, -300, 0), V(-500, -700, 0)}, 0xD0D0D0); p.add(M_SHARD); }
    { Mesh p; const uint32_t M3 = 0xF048A0; const float R = 1000.f, T = 130.f; V a(0, 0, R), b(R * 0.9f, 0, -R * 0.6f), c(-R * 0.9f, 0, -R * 0.6f); auto edge = [&](V s, V e) { V d = norm(e - s), n(-d.z, 0, d.x); quad(p, s - n * T, e - n * T, e + n * T, s + n * T, M3); }; edge(a, b); edge(b, c); edge(c, a); p.add(M_LOCK); }
    { Mesh p; const uint32_t G = 0x52C652, W = 0xF4F4F0; const float L = 3400.f, R = 1300.f; for (int s = 0; s < 8; s++) { float a0 = s * 0.7854f, a1 = (s + 1) * 0.7854f; V u0(cosf(a0) * R, 0, sinf(a0) * R), u1(cosf(a1) * R, 0, sinf(a1) * R); uint32_t c = s & 1 ? lit(G, 0.82f) : G; quad(p, u0 + V(0, -L, 0), u1 + V(0, -L, 0), u1 + V(0, L * 0.4f, 0), u0 + V(0, L * 0.4f, 0), c); quad(p, u0 + V(0, L * 0.4f, 0), u1 + V(0, L * 0.4f, 0), u1 + V(0, L, 0), u0 + V(0, L, 0), s & 1 ? 0xC8C8C4 : W); p.poly({u0 + V(0, L, 0), u1 + V(0, L, 0), V(0, L + 700, 0)}, W); p.poly({u1 + V(0, -L, 0), u0 + V(0, -L, 0), V(0, -L - 700, 0)}, G); } p.add(M_FUEL); }
    { Mesh p; p.box(V(-1500, -1500, -1500), V(1500, 1500, 1500), 0xF8D040, 0xE0A020, 0x906810); p.add(M_AMMO); }
    { Mesh p; const uint32_t O = 0xF0F0E8; quad(p, V(-110, -900, 0), V(110, -900, 0), V(110, 700, 0), V(-110, 700, 0), O); quad(p, V(0, -900, -110), V(0, -900, 110), V(0, 700, 110), V(0, 700, -110), O); p.poly({V(-260, -1000, 0), V(260, -1000, 0), V(0, -350, 0)}, 0xD03018); for (int i = 0; i < 6; i++) { float s = 240.f + i * 40.f; p.poly({V(-s, -950.f - i * 240.f, 0), V(s, -950.f - i * 240.f, 0), V(0, -1200.f - i * 240.f, 0)}, i & 1 ? 0xF8C040 : 0xE86020); } p.add(M_MISS); }
    { Mesh p; std::mt19937 q(55); const float R = 42000.f, H = 15000.f; static const uint32_t TN[4] = {0xE8E8E0, 0xC0C4C0, 0x808488, 0x383A3E}; for (int s = 0; s < 8; s++) { float a0 = s * 0.7854f, a1 = (s + 1) * 0.7854f; for (int k = 0; k < 4; k++) { float z0 = k * H / 4, z1 = (k + 1) * H / 4; V v0(cosf(a0) * R, sinf(a0) * R, z0), v1(cosf(a1) * R, sinf(a1) * R, z0), v2(cosf(a1) * R, sinf(a1) * R, z1), v3(cosf(a0) * R, sinf(a0) * R, z1); quad(p, v0, v1, v2, v3, TN[q() % 4]); } } p.add(M_CORE); }
    { Mesh p; const int NA = 12, NB = 8; const float R = 16000.f; auto P = [&](int a, int b) { float t = 3.14159f * b / NB, ph = 6.2831853f * a / NA; return V(sinf(t) * cosf(ph) * R, sinf(t) * sinf(ph) * R, cosf(t) * R); }; for (int a = 0; a < NA; a++) for (int b = 0; b < NB; b++) quad(p, P(a, b), P(a, b + 1), P(a + 1, b + 1), P(a + 1, b), (a + b) & 1 ? 0xF8C030 : 0xF08018); p.add(M_ORB); }
    { Mesh p; const float K = 700.f; quad(p, V(-K, 0, -30), V(K, 0, -30), V(K, 0, 30), V(-K, 0, 30), 0xF8F8F8); quad(p, V(-30, 0, -K), V(30, 0, -K), V(30, 0, K), V(-30, 0, K), 0xF8F8F8); p.add(M_SPARK); }
}
/* ---- the attract's white box: panels with dark dashes and rivets, a door with zigzag edges that parts top and bottom. Local: front in the plane y = 0, you are at -y ---- */
static void build_box() {
    const float W = 34000.f, H = 17500.f, DW = 18750.f, DH = 10500.f, DZ = -2700.f, DEEP = 30000.f; std::mt19937 q(8);
    Mesh m; auto R = [&](float a, float b) { return a + (b - a) * ((q() % 10000) / 10000.f); };
    const int NX = 12, NZ = 9; const float FW = W - 5000.f; auto rect = [&](float x0, float x1, float z0, float z1, uint32_t c) { if (x1 > x0 && z1 > z0) quad(m, V(x0, 0, z0), V(x1, 0, z0), V(x1, 0, z1), V(x0, 0, z1), c); };
    for (int i = 0; i < NX; i++) for (int k = 0; k < NZ; k++) { float x0 = -W + i * (W + FW) / NX, x1 = x0 + (W + FW) / NX, z0 = -H + k * 2 * H / NZ, z1 = z0 + 2 * H / NZ; uint32_t c = (i * 3 + k) & 1 ? 0xE6E6E2 : 0xDEDEDA; float zt = DZ + DH, zb = DZ - DH;
        if (x1 <= -DW || x0 >= DW || z1 <= zb || z0 >= zt) { rect(x0, x1, z0, z1, c); continue; } rect(x0, (std::min)(x1, -DW), z0, z1, c); rect((std::max)(x0, DW), x1, z0, z1, c); float mx0 = (std::max)(x0, -DW), mx1 = (std::min)(x1, DW); rect(mx0, mx1, (std::max)(z0, zt), z1, c); rect(mx0, mx1, z0, (std::min)(z1, zb), c); }
    for (int k = 0; k < 9; k++) { float z0 = -H + k * 2 * H / 9, z1 = z0 + 2 * H / 9; for (int d = 0; d < 3; d++) { float x0 = FW + d * 5000.f / 3, x1 = x0 + 5000.f / 3, y0 = d * 7000.f / 3, y1 = y0 + 7000.f / 3; quad(m, V(x0, y0, z0), V(x1, y1, z0), V(x1, y1, z1), V(x0, y0, z1), mix(0xA8A8A8, 0x282828, d / 2.f)); for (int r = 0; r < 2; r++) { float zz = z0 + (r + 0.5f) * (z1 - z0) / 2, xx = x0 + (x1 - x0) * (0.25f + 0.5f * r), yy = y0 + (y1 - y0) * (0.25f + 0.5f * r) - 60.f; quad(m, V(xx - 150, yy, zz - 150), V(xx + 150, yy, zz - 150), V(xx + 150, yy, zz + 150), V(xx - 150, yy, zz + 150), 0xF4F4F4); } } }
    for (int n = 0; n < 170; n++) { float x = R(-W, W - 5500.f), z = R(-H, H); if (fabsf(x) < DW + 500 && z > DZ - DH - 500 && z < DZ + DH + 500) continue; float a = R(300.f, 1400.f); quad(m, V(x, -40, z), V(x + a, -40, z - a * 0.25f), V(x + a, -40, z - a * 0.25f - 220), V(x, -40, z - 220), n & 1 ? 0x303034 : 0x909498); }
    quad(m, V(-W + 3000, -60, DZ + DH + 700), V(W - 3000, -60, DZ + DH + 700), V(W - 3000, -60, DZ + DH + 1700), V(-W + 3000, -60, DZ + DH + 1700), 0x8C9098);   /* the grey rail above the door */
    quad(m, V(-DW - 350, -60, DZ - DH - 350), V(-DW, -60, DZ - DH - 350), V(-DW, -60, DZ + DH), V(-DW - 350, -60, DZ + DH), 0x202024); quad(m, V(-DW, -60, DZ - DH - 350), V(DW, -60, DZ - DH - 350), V(DW, -60, DZ - DH), V(-DW, -60, DZ - DH), 0x202024);
    quad(m, V(-DW - 3200, -70, DZ + DH + 900), V(-DW - 1200, -70, DZ + DH + 900), V(-DW - 1200, -70, DZ + DH + 1500), V(-DW - 3200, -70, DZ + DH + 1500), 0xF08818); quad(m, V(DW + 1500, -70, DZ + DH + 700), V(DW + 2600, -70, DZ + DH + 700), V(DW + 2600, -70, DZ + DH + 1800), V(DW + 1500, -70, DZ + DH + 1800), 0x30B8F0);
    /* the sides and back go off grey into the dark, with white rivets on the right */
    for (int k = 0; k < 8; k++) { float z0 = -H + k * 2 * H / 8, z1 = z0 + 2 * H / 8; for (int d = 0; d < 6; d++) { float y0 = d * DEEP / 6, y1 = (d + 1) * DEEP / 6; uint32_t c = mix(0xA8A8A8, 0x181818, d / 5.f); quad(m, V(W, y0, z0), V(W, y1, z0), V(W, y1, z1), V(W, y0, z1), c); quad(m, V(-W, y1, z0), V(-W, y0, z0), V(-W, y0, z1), V(-W, y1, z1), c); for (int r = 0; r < 3; r++) { float yy = y0 + r * DEEP / 18 + 300, zz = z0 + (r + 1) * (z1 - z0) / 4; quad(m, V(W + 30, yy, zz - 160), V(W + 30, yy + 320, zz - 160), V(W + 30, yy + 320, zz + 160), V(W + 30, yy, zz + 160), 0xF4F4F4); } } }
    for (int i = 0; i < 8; i++) { float x0 = -W + i * 2 * W / 8, x1 = x0 + 2 * W / 8; for (int d = 0; d < 4; d++) { float y0 = d * DEEP / 4, y1 = (d + 1) * DEEP / 4; uint32_t c = (i + d) & 1 ? 0xDCDCD8 : 0xB8B8B4; quad(m, V(x0, y0, H), V(x1, y0, H), V(x1, y1, H), V(x0, y1, H), c); quad(m, V(x0, y1, -H), V(x1, y1, -H), V(x1, y0, -H), V(x0, y0, -H), 0x505050); } }
    quad(m, V(-W, DEEP, -H), V(W, DEEP, -H), V(W, DEEP, H), V(-W, DEEP, H), 0x404040);
    m.add(M_BOX);
    { Mesh d; quad(d, V(-DW, 4000, DZ - DH), V(DW, 4000, DZ - DH), V(DW, 4000, DZ + DH), V(-DW, 4000, DZ + DH), 0xF8F8F4); d.add(M_DOOR_IN); }
    /* a door half: white plate with the zigzag along its meeting edge, drawn to meet at z = 0 of the mesh */
    for (int half = 0; half < 2; half++) { Mesh d; float up = half ? -1.f : 1.f; const int NZ2 = 5; float step = 2 * DW / NZ2, amp = 3000.f; for (int i = 0; i < NZ2; i++) { float x0 = -DW + i * step, xm = x0 + step * 0.5f, x1 = x0 + step;
            /* plate above the teeth */ if (half == 0) { d.poly({V(x0, 300, amp * 0), V(xm, 300, amp), V(x1, 300, 0), V(x1, 300, 4200), V(x0, 300, 4200)}, 0xF0F0EC); } else { d.poly({V(x0, 300, -4200), V(x1, 300, -4200), V(x1, 300, 0), V(xm, 300, amp), V(x0, 300, 0)}, 0xF0F0EC); }
            const float S = 1150.f; float zb = half ? 0.f : 0.f; (void)up; V a0(x0, 270, zb), a1(xm, 270, zb + amp), a2(x1, 270, zb); float o = half ? -S : S; for (int seg = 0; seg < 2; seg++) { V p0 = seg ? a1 : a0, p1 = seg ? a2 : a1; const int ST = 3; for (int t = 0; t < ST; t++) { V s0 = p0 + (p1 - p0) * (t / (float)ST), s1 = p0 + (p1 - p0) * ((t + 1) / (float)ST); quad(d, s0, s1, s1 + V(0, 0, o), s0 + V(0, 0, o), (t + seg) % 3 == 1 ? 0x101010 : 0xF0D838); } } }
        d.add(half ? M_DOOR_B : M_DOOR_T); }
}
/* ---- the logo: pale slab letters with a red inlay. One mesh, centred, in the plane y = 0, facing -y ---- */
static void build_logo() {
    Mesh m; std::mt19937 q(19); static const char *LINES[2] = {"X-TREME", "STRIKE"}; const float U = 1000.f, TH = 1020.f, DEPTH = 1100.f; const float PITCH = 5.3f * U;
    for (int l = 0; l < 2; l++) { int n = (int)strlen(LINES[l]); float x0 = -(n - 1) * PITCH * 0.5f - 2 * U, z0 = l ? -1000.f : 7800.f; for (int c = 0; c < n; c++) { float lx = x0 + c * PITCH;
            for_segments(LINES[l][c], [&](float ax, float ay, float bx, float by) { V a(lx + ax * U, 0, z0 - ay * U), b(lx + bx * U, 0, z0 - by * U), d = norm(b - a), s(d.z, 0, -d.x); a = a - d * (TH * 0.7f); b = b + d * (TH * 0.7f); static const uint32_t G[3] = {0xF0F0EC, 0xD8D8D4, 0xC4C8CC}; uint32_t col = G[q() % 3];
                V p0 = a - s * TH, p1 = b - s * TH, p2 = b + s * TH, p3 = a + s * TH, bk(0, DEPTH, 0); quad(m, p0, p1, p2, p3, col); quad(m, p3 + bk, p2 + bk, p1 + bk, p0 + bk, 0x484C54); quad(m, p0 + bk, p1 + bk, p1, p0, 0x70747C); quad(m, p2 + bk, p3 + bk, p3, p2, 0xA8ACB4); quad(m, p1 + bk, p2 + bk, p2, p1, 0x70747C); quad(m, p3 + bk, p0 + bk, p0, p3, 0x90949C);
                V i0 = a + d * (TH * 0.6f) - s * (TH * 0.22f), i1 = b - d * (TH * 0.6f) - s * (TH * 0.22f), i2 = b - d * (TH * 0.6f) + s * (TH * 0.22f), i3 = a + d * (TH * 0.6f) + s * (TH * 0.22f), e(0, -30, 0); quad(m, i0 + e, i1 + e, i2 + e, i3 + e, 0xE85848); }); } }
    m.add(M_LOGO);
}

/* ---- the game ---- */
enum Kind { K_ROID, K_FIGHTER, K_PYLON, K_TOWER, K_JET, K_ORB, K_CMDR };
struct Foe { int kind = K_ROID, id = 0; V p, v, axis; float hp = 1, r = 1000, t = 0, cool = 0, size = 1, spin = 0, ang = 0, bank = 0; M R; bool alive = true; };
struct Bolt { V p, v; float life; };
struct Boom { V p; float t, size; };
struct Bit { V p, v; float t, spin; int id; };
struct Beam { V a, b; float t; };
struct Pick { V p; int fuel; float t; };
struct Miss { V p, v; int target; float life; };
enum State { S_TITLE, S_PLAY, S_OVER };
static State st = S_TITLE; static float st_t = 0; static double sim_t = 0;
static int stage = ST_TUBE, wave = 0, score = 0, missiles = 6, lock_idx = -1, gun_side = 0; static float stage_t = 0, shield = 100, fuel = 100, cool = 0, miss_cool = 0, crash_cool = 0, shake = 0, spawn_a = 0, spawn_b = 0, spawn_c = 0, chirp_cool = 0, attract_t = 0, core_cool = 0;
static V me_p; static float me_yaw = 0, me_pitch = 0, me_bank = 0, look_y = 0, look_p = 0; static M me_R; static V me_aim(0, 1, 0);
static std::vector<Foe> foes; static std::vector<Bolt> bolts; static std::vector<Boom> booms; static std::vector<Bit> bits; static std::vector<Beam> beams; static std::vector<Pick> picks; static std::vector<Miss> miss;
/* ---- link play (vpix_1k_net.h): pod 1 flies the ship and runs the world; every other pod is a wingman in the same world (INVENTED: the original is one ship, so a linked game is a formation). A wingman steers a spot beside the lead (stick), aims with the head, fires lasers and missiles; score, shield, fuel and missiles are shared. A start-together game ---- */
struct Wing { bool on = false; float ox = 0, oz = 0, sx = 0, sy = 0, cool = 0, mcool = 0; V aim_l = V(0, 1, 0); uint8_t fseq = 0, mseq = 0, lf = 0, lm = 0; };
static Wing wing[4]; static V lead_p; static bool slave_game = false;
#pragma pack(push, 1)
struct NetIn { float sx, sy, ax, ay, az; uint8_t fseq, mseq; };
#pragma pack(pop)
static void sfx(int ch, uint32_t key, int vol, int pan = 0) { buggy::play(ch, (int)key, vol, pan); }
static void orient_m(M &R, float yaw, float pitch, float bank) { float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch), cb = cosf(bank), sb = sinf(bank); V f(-sy * cp, cy * cp, sp), r0(cy, sy, 0), u0 = cross(r0, f); R.r[1] = f; R.r[0] = r0 * cb + u0 * (-sb); R.r[2] = cross(R.r[0], f); }
static void boom(V p, float size) { Boom b; b.p = p; b.t = 0; b.size = size; booms.push_back(b); sfx(3, SND_BOOM, 100);
    for (int i = 0; i < 7; i++) { Bit k; k.p = p; k.v = V(rnd(-1, 1), rnd(-1, 1), rnd(0.2f, 1.f)) * (14000.f * size); k.t = 0; k.spin = rnd(3, 9); k.id = i & 1 ? M_PLATE : M_SHARD; bits.push_back(k); } }
static float harder() { return 1.f + wave * 0.45f + stage_t / 70.f; }
static int stage_mult() { return stage >= ST_VALLEY ? 22 : 1; }
static void add_score(int base, float dist) { score += base * stage_mult() + (int)(dist / 370.f); if (score > 9999999) score = 9999999; }   /* the stills' scores are not round: a share comes from range */
static void chirp() { if (chirp_cool <= 0) { chirp_cool = 5.f; sfx(6, SND_DROID, 90); dl("t=%.1f droid chirp", sim_t); } }
static void add_static(int kind, int id, V p, float hp, float r) { Foe c; c.kind = kind; c.id = id; c.p = p; c.hp = hp; c.r = r; c.ang = rnd(-1.f, 1.f); c.t = rnd(0.f, 4.f); foes.push_back(c); }
static void enter_stage(int s, float y = -1e30f) {
    stage = s; stage_t = 0; foes.clear(); bolts.clear(); picks.clear(); miss.clear(); beams.clear(); spawn_a = 3; spawn_b = 6; spawn_c = 14; me_yaw = 0; me_pitch = 0; me_bank = 0; lock_idx = -1;
    if (s == ST_TUBE) me_p = V(0, 0, 0);
    if (s == ST_SPACE) me_p = V(0, 0, 0);
    if (s == ST_NIGHT) { me_p = V(0, 0, 230000.f); std::mt19937 q(77 + wave); auto R = [&](float a, float b) { return a + (b - a) * ((q() % 10000) / 10000.f); }; for (int i = 0; i < 26; i++) { float yy = 350000.f + i * 92000.f + R(-20000.f, 20000.f), x = R(-210000.f, 210000.f); if (i % 4 == 2) add_static(K_TOWER, M_TOWER, V(x, yy, 0), 3, 7000.f); else add_static(K_PYLON, M_PYLON, V(x, yy, 0), 3, 7000.f); } }
    if (s == ST_VALLEY) { me_p = V(0, VALLEY_Y0, 40000.f); for (int i = 0; i < 6; i++) add_static(K_JET, M_JET, V(-100000.f + i * 40000.f, DAM_Y - 26000.f, -2000.f), 1, 9000.f); }
    if (s == ST_CORE) { me_p = V(0, 1000000.f, 60000.f); add_static(K_ORB, M_ORB, V(0, CORE_Y, 15000.f + 26000.f), 40.f + wave * 10.f, 19000.f); Foe c; c.kind = K_CMDR; c.id = M_CMDR; c.p = V(60000.f, CORE_Y - 90000.f, 75000.f); c.hp = 10.f + wave * 4.f; c.r = 16000.f; c.size = 3.f; foes.push_back(c); }
    if (y > -1e29f) me_p.y = y;
    sfx(7, SND_GO, 90); dl("t=%.1f stage %d wave %d score %d", sim_t, s, wave, score);
}
static void new_round() {
    wave = 0; score = 0; missiles = 6; shield = start_hp; fuel = start_fuel; cool = 0; crash_cool = 0; shake = 0; chirp_cool = 0; core_cool = 0; booms.clear(); bits.clear(); look_y = look_p = 0;
    for (int i = 0; i < 4; i++) wing[i] = Wing();
    if (v1knet::on) v1knet::begin_round(v1knet::SEATS);   /* linked: pods that are here take wingman seats */
    st = S_PLAY; st_t = 0; enter_stage(start_stage, start_y); dl("t=%.1f new round", sim_t);
}
static float ground_at(float x, float y) { if (stage == ST_NIGHT) return 0.f; if (stage < ST_NIGHT) return -1e9f; if (y < -200000.f) return 8000.f; if (y > DAM_Y + 7000) return 15000.f; float h = land_h(x, y); if (y > DAM_Y - 27000) h = (std::max)(h, 34000.f * clampf((y - (DAM_Y - 26000)) / 26000.f, 0.f, 1.f)); return h; }
static void spawn_fighter(float off) { Foe c; c.kind = K_FIGHTER; c.id = M_CRAFT2; c.hp = 2; c.r = 7000.f; float a = rnd(-1.f, 1.f) + off; float d = 420000.f; c.p = me_p + V(sinf(a) * d, cosf(a) * d, rnd(10000.f, 70000.f)); if (stage >= ST_NIGHT) c.p.z = (std::max)(c.p.z, ground_at(c.p.x, c.p.y) + 40000.f); c.v = norm(me_p - c.p) * fly_speed; c.ang = rnd(0.f, 6.28f); foes.push_back(c); }
static void spawn_roid(bool fast) { Foe c; c.kind = K_ROID; c.id = M_ROID0 + rand() % 3; float s = rnd(4.f, 13.f); c.size = s; c.r = 1000.f * s; c.hp = s > 9.f ? 3.f : 2.f; V aim_at = me_p + V(rnd(-30000.f, 30000.f), fly_speed * 2.f, rnd(-22000.f, 22000.f)); c.p = me_p + V(rnd(-170000.f, 170000.f), rnd(420000.f, 620000.f), rnd(-90000.f, 110000.f)); if (stage == ST_NIGHT) { c.p.z = me_p.z + rnd(90000.f, 200000.f); } c.v = norm(aim_at - c.p) * rnd(30000.f, fast ? 90000.f : 55000.f); c.axis = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-1, 1))); c.spin = rnd(0.4f, 1.4f); foes.push_back(c); }
static void spawn_pick(int f) { Pick p; p.fuel = f; p.p = me_p + V(rnd(-50000.f, 50000.f), 430000.f, rnd(-10000.f, 25000.f)); if (stage >= ST_NIGHT) p.p.z = ground_at(p.p.x, p.p.y) + rnd(22000.f, 48000.f); p.t = 0; if (stage == ST_VALLEY && p.p.y < -200000.f) p.p.x = clampf(p.p.x, -40000.f, 40000.f); picks.push_back(p); }
static void hurt(float d) { shield -= d; shake = 0.5f; sfx(2, SND_HURT, 110); VPIX_Haptic(2, 0.2f, 0.8f); if (shield < 25.f) chirp(); }
static void kill(Foe &c, float dist) { c.alive = false; switch (c.kind) { case K_ROID: boom(c.p, c.size * 0.4f); add_score(300, dist); break; case K_FIGHTER: boom(c.p, 2.6f); add_score(800, dist); break; case K_JET: boom(c.p + V(0, 12000, 18000), 4.f); add_score(1800, dist); break; case K_CMDR: boom(c.p, 7.f); add_score(15000, dist); break; case K_ORB: boom(c.p, 14.f); add_score(30000, dist); break; default: boom(c.p + V(0, 0, 20000), 3.5f); add_score(1100, dist); } }
static void damage(Foe &c, float n) { if (c.kind == K_ORB) { bool cmdr = false; for (const Foe &o : foes) if (o.alive && o.kind == K_CMDR) cmdr = true; if (cmdr) n *= 0.25f; } c.hp -= n; if (c.hp <= 0 && c.alive) kill(c, len(c.p - me_p)); else { Boom b; b.p = c.p + norm(me_p - c.p) * c.r; b.t = 0.65f; b.size = 0.6f; booms.push_back(b); } }
static V foe_centre(const Foe &c) { if (c.kind == K_PYLON) return c.p + V(0, 0, 40000.f); if (c.kind == K_TOWER) return c.p + V(0, 0, 22000.f); if (c.kind == K_JET) return c.p + V(0, 12000.f, 18000.f); return c.p; }
static bool visible_kind(const Foe &c) { return c.alive; }
static V wing_pos(int i) { return me_p + me_R.r[0] * wing[i].ox + me_R.r[2] * wing[i].oz; }
static bool near_ship(V p, float r) { if (len(p - me_p) < r) return true; if (v1knet::on) for (int i = 1; i < 4; i++) if (wing[i].on && len(p - wing_pos(i)) < r) return true; return false; }
static void wing_fire(int i) {
    Wing &w = wing[i]; V org = wing_pos(i), aim = norm(tw(me_R, w.aim_l)); V from = org + tw(me_R, V((i & 1) ? 1000.f : -1000.f, 1200.f, 560.f)); float best = RANGE; int hit = -1;
    for (size_t k = 0; k < foes.size(); k++) { const Foe &c = foes[k]; if (!visible_kind(c)) continue; V to = foe_centre(c) - org; float along = dot(to, aim); if (along < 0 || along > best) continue; V off = to - aim * along; if (len(off) < c.r * 1.35f + along * 0.012f) { best = along; hit = (int)k; } }
    Beam b; b.a = from; b.b = org + aim * best; b.t = 0; beams.push_back(b); sfx(1, SND_FIRE, 60);
    if (hit >= 0) damage(foes[hit], 1.f);
}
static void wing_missile(int i) {
    Wing &w = wing[i]; V org = wing_pos(i), aim = norm(tw(me_R, w.aim_l)); int t = -1; float bs = cosf(14.f * 3.14159f / 180.f);
    for (size_t k = 0; k < foes.size(); k++) { const Foe &c = foes[k]; if (!c.alive) continue; V to = foe_centre(c) - org; float l = len(to); if (l > 600000.f || l < 1.f) continue; float cs = dot(to * (1.f / l), aim); if (cs > bs) { bs = cs; t = (int)k; } }
    if (t < 0) return; Miss m; m.p = org + tw(me_R, V(0, 1500, -700)); m.v = me_R.r[1] * 100000.f; m.target = t; m.life = 6.f; miss.push_back(m); missiles--; w.mcool = 0.8f; sfx(4, SND_MISS, 70);
}
static void net_wings(float dt) {   /* master: move and arm the wingmen */
    for (int i = 1; i < 4; i++) { Wing &w = wing[i]; if (!w.on) continue; const bool tube = stage == ST_TUBE; const float lim = tube ? 5500.f : 80000.f;
        w.ox = clampf(w.ox + w.sx * (tube ? 5000.f : 30000.f) * dt, -lim, lim); w.oz = clampf(w.oz + w.sy * (tube ? 5000.f : 30000.f) * dt, -lim * 0.6f, lim * 0.6f);
        if (!tube) { float gz = ground_at(me_p.x + w.ox, me_p.y) + 3500.f; if (me_p.z + w.oz < gz) w.oz = gz - me_p.z; if (stage == ST_SPACE) w.oz = clampf(w.oz, -100000.f - me_p.z, 100000.f - me_p.z); }
        if (w.cool > 0) w.cool -= dt; if (w.mcool > 0) w.mcool -= dt;
        if (w.fseq != w.lf) { w.lf = w.fseq; if (!tube && w.cool <= 0) { w.cool = 0.1f; wing_fire(i); } }
        if (w.mseq != w.lm) { w.lm = w.mseq; if (!tube && missiles > 0 && w.mcool <= 0) wing_missile(i); } }
}
static void fire_laser() {
    if (cool > 0) return; cool = FIRE_GAP; gun_side ^= 1; V from = me_p + tw(me_R, V(gun_side ? 1000.f : -1000.f, 1200.f, 560.f)); float best = RANGE; int hit = -1;
    for (size_t i = 0; i < foes.size(); i++) { const Foe &c = foes[i]; if (!visible_kind(c)) continue; V to = foe_centre(c) - me_p; float along = dot(to, me_aim); if (along < 0 || along > best) continue; V off = to - me_aim * along; if (len(off) < c.r * 1.35f + along * 0.012f) { best = along; hit = (int)i; } }
    Beam b; b.a = from; b.b = me_p + me_aim * best; b.t = 0; beams.push_back(b); sfx(1, SND_FIRE, 80);
    if (hit >= 0) damage(foes[hit], 1.f);
}
static void step_world(const VPIX_Input &in, bool trig, bool top, float dt) {
    if (st != S_PLAY) return;
    stage_t += dt; if (cool > 0) cool -= dt; if (miss_cool > 0) miss_cool -= dt; if (crash_cool > 0) crash_cool -= dt; if (shake > 0) shake -= dt; if (chirp_cool > 0) chirp_cool -= dt;
    float sx = in.joy_r[0], sy = -in.joy_r[1];   /* Bobby: the right thumbstick flies, not hand motion. Speed is fixed, so there is no throttle. A fires missiles (Bobby 11-03) */
    top = top || (in.vr && in.fire2);
    /* the self-play: flies the middle of the course, turns its head on what is nearest and shoots */
    int want = -1; if (autoplay) { float bd = 1e12f; V nose = me_R.r[1]; for (size_t i = 0; i < foes.size(); i++) { const Foe &c = foes[i]; if (!c.alive) continue; V to = foe_centre(c) - me_p; float l = len(to); if (l > RANGE * 0.9f) continue; float cs = dot(to * (1.f / l), nose); if (cs < 0.55f) continue; float sc = l * (1.6f - cs); if (sc < bd) { bd = sc; want = (int)i; } }
        float gx = 0, gz = me_p.z; if (stage == ST_SPACE) gz = 20000.f * sinf(stage_t * 0.3f); else if (stage >= ST_NIGHT) { float ahead = me_p.y + fly_speed * 1.2f; gz = (std::max)(ground_at(me_p.x, me_p.y), ground_at(me_p.x, ahead)) + 36000.f; if (stage == ST_NIGHT && stage_t < 10.f) gz = 230000.f - stage_t * 18000.f; if (stage >= ST_VALLEY && me_p.y > DAM_Y - 170000.f && me_p.y < DAM_Y + 30000.f) gz = 66000.f; }
        for (const Pick &p : picks) if (p.p.y > me_p.y && p.p.y < me_p.y + 300000.f) { gx = p.p.x; if (stage < ST_VALLEY) gz = p.p.z; break; }
        V d = V(gx, me_p.y + 90000.f, gz) - me_p; float a = wrap(atan2f(-d.x, d.y) - me_yaw); sx = clampf(-a * 1.6f, -1.f, 1.f); sy = clampf((atan2f(d.z, sqrtf(d.x * d.x + d.y * d.y)) - me_pitch) * 2.f, -1.f, 1.f); }
    float sp = fly_speed; if (stage == ST_TUBE) sp = fly_speed; if (stage == ST_CORE) sp = fly_speed * clampf((CORE_Y - 110000.f - me_p.y) / 70000.f, 0.f, 1.f);
    if (stage == ST_TUBE) { me_bank += clampf(sx * 0.3f - me_bank, -dt, dt); me_p.x = clampf(me_p.x + sx * 9000.f * dt, -TUBE_R * 0.5f, TUBE_R * 0.5f); me_p.z = clampf(me_p.z + sy * 9000.f * dt, -TUBE_R * 0.5f, TUBE_R * 0.5f); orient_m(me_R, 0, 0, me_bank); me_p.y += sp * dt; }
    else { me_bank += clampf(sx * 1.0f - me_bank, -2.f * dt, 2.f * dt); me_yaw = clampf(wrap(me_yaw - me_bank * turn_rate * dt), -0.8f, 0.8f); if (fabsf(sx) < 0.1f) me_yaw -= me_yaw * 0.25f * dt; me_pitch = clampf(me_pitch + sy * 0.9f * dt, -0.7f, 0.7f); if (fabsf(sy) < 0.1f) me_pitch -= me_pitch * 0.5f * dt; orient_m(me_R, me_yaw, me_pitch, me_bank);
        me_p += me_R.r[1] * (sp * dt); if (stage == ST_CORE) { me_p.x += -sinf(me_bank) * fly_speed * 0.6f * dt; }
        float xlim = stage == ST_VALLEY && me_p.y < -200000.f ? 55000.f : stage == ST_SPACE ? 170000.f : 210000.f; me_p.x = clampf(me_p.x, -xlim, xlim);
        if (stage == ST_SPACE) me_p.z = clampf(me_p.z, -100000.f, 100000.f); else { me_p.z = (std::min)(me_p.z, stage == ST_NIGHT && stage_t < 8.f ? 240000.f : 140000.f); float gz = ground_at(me_p.x, me_p.y) + 3500.f; if (me_p.z < gz) { me_p.z = gz; me_pitch = (std::max)(me_pitch, 0.15f); if (crash_cool <= 0) { crash_cool = 1.f; hurt(10.f); } } } }
    /* the fuel gauge: empty is the end, so power-ups are what buy a longer game */
    fuel -= 100.f / 118.f * dt; if (fuel < 25.f) chirp();
    /* the sight is where your head points */
    if (in.head.valid && !autoplay && !have_look) me_aim = norm(tw(me_R, V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2])));
    else { float ty = look_y, tp = look_p; if (have_look) { ty = look_env[0] * 3.14159f / 180.f; tp = look_env[1] * 3.14159f / 180.f; look_y = ty; look_p = tp; } else if (autoplay) { float wy = 0, wp = 0; if (want >= 0) { V to = foe_centre(foes[want]) - me_p; V l(dot(to, me_R.r[0]), dot(to, me_R.r[1]), dot(to, me_R.r[2])); wy = atan2f(l.x, l.y); wp = atan2f(l.z, sqrtf(l.x * l.x + l.y * l.y)); } look_y += clampf(wy - look_y, -2.2f * dt, 2.2f * dt); look_p += clampf(wp - look_p, -2.2f * dt, 2.2f * dt); } V l(sinf(look_y) * cosf(look_p), cosf(look_y) * cosf(look_p), sinf(look_p)); me_aim = norm(tw(me_R, l)); }
    /* the plasma missile's auto-lock: the nearest thing close to the sight */
    { int prev = lock_idx; lock_idx = -1; float bs = cosf(14.f * 3.14159f / 180.f); for (size_t i = 0; i < foes.size(); i++) { const Foe &c = foes[i]; if (!c.alive) continue; V to = foe_centre(c) - me_p; float l = len(to); if (l > 600000.f) continue; float cs = dot(to * (1.f / l), me_aim); if (cs > bs) { bs = cs; lock_idx = (int)i; } } if (lock_idx >= 0 && lock_idx != prev) sfx(5, SND_LOCK, 50); }
    bool auto_fire = false; if (autoplay && want >= 0) { V to = foe_centre(foes[want]) - me_p; auto_fire = dot(norm(to), me_aim) > 0.992f; }
    if (stage != ST_TUBE && (trig || auto_fire)) fire_laser();
    static bool top_was = false; bool auto_m = autoplay && lock_idx >= 0 && (foes[lock_idx].kind >= K_PYLON || foes[lock_idx].hp > 2.f) && miss_cool <= 0; if (((top && !top_was) || auto_m) && missiles > 0 && lock_idx >= 0 && stage != ST_TUBE && miss_cool <= 0) { Miss m; m.p = me_p + tw(me_R, V(0, 1500, -700)); m.v = me_R.r[1] * 100000.f; m.target = lock_idx; m.life = 6.f; miss.push_back(m); missiles--; miss_cool = 0.8f; sfx(4, SND_MISS, 90); dl("t=%.1f missile, %d left", sim_t, missiles); } top_was = top;
    if (v1knet::on && v1knet::master) net_wings(dt);
    /* what comes at you, by stage: ever increasing defence waves */
    float H = harder(); spawn_a -= dt; spawn_b -= dt; spawn_c -= dt; int live_f = 0; for (const Foe &c : foes) if (c.alive && c.kind == K_FIGHTER) live_f++;
    if (stage == ST_SPACE) { if (spawn_a <= 0) { spawn_a = 1.5f / H; spawn_roid(false); } if (spawn_b <= 0) { spawn_b = 14.f; for (int i = 0; i < 9; i++) spawn_roid(true); chirp(); } if (spawn_c <= 0) { spawn_c = 9.f / H; if (live_f < 6) for (int i = 0; i < 3; i++) spawn_fighter(0.f); } }
    if (stage == ST_NIGHT) { if (stage_t < 11.f && spawn_a <= 0) { spawn_a = 0.5f / H; spawn_roid(true); } if (spawn_c <= 0) { spawn_c = 10.f / H; if (live_f < 5) for (int i = 0; i < 2; i++) spawn_fighter(i ? 1.2f : -1.2f); } }
    if (stage == ST_VALLEY) { if (spawn_c <= 0) { spawn_c = 8.f / H; if (live_f < 6) for (int i = 0; i < 2; i++) spawn_fighter(0.f); } }
    if (stage == ST_CORE) { if (spawn_c <= 0) { spawn_c = 8.f / H; if (live_f < 5) spawn_fighter(0.f); } }
    { static float pick_t = 0; pick_t += dt; if (stage != ST_TUBE && pick_t > 20.f) { pick_t = 0; static int n = 0; n++; spawn_pick(n % 3 != 2); } }
    /* the foes */
    for (size_t i = 0; i < foes.size(); i++) { Foe &c = foes[i]; if (!c.alive) continue; c.t += dt; if (c.cool > 0) c.cool -= dt; V to = me_p - foe_centre(c); float l = len(to);
        if (c.kind == K_ROID) { c.p += c.v * dt; c.ang += c.spin * dt; if (l < c.r + 3500.f) { boom(c.p, c.size * 0.3f); c.alive = false; hurt(14.f); } else if (c.p.y < me_p.y - 120000.f) c.alive = false; }
        else if (c.kind == K_FIGHTER || c.kind == K_CMDR) { float period = c.kind == K_CMDR ? 9.f : 6.f; float ph = c.ang + c.t * 0.5f; V offs(sinf(ph) * 90000.f, fmodf(c.t, period) < period * 0.6f ? 60000.f : -110000.f, sinf(ph * 1.7f) * 26000.f + 9000.f); V goal = me_p + offs; if (stage >= ST_NIGHT) goal.z = (std::max)(goal.z, ground_at(goal.x, goal.y) + 26000.f); V want_v = norm(goal - c.p) * (fly_speed * (c.kind == K_CMDR ? 1.0f : 1.35f)); if (stage == ST_CORE && c.kind == K_CMDR) want_v = norm(V(sinf(c.t * 0.35f) * 150000.f, CORE_Y - 90000.f + cosf(c.t * 0.5f) * 30000.f, 70000.f + sinf(c.t) * 15000.f) - c.p) * fly_speed; V old = c.v; c.v += (want_v - c.v) * clampf(1.2f * dt, 0.f, 1.f); c.p += c.v * dt; V f = norm(c.v); V r0 = cross(f, V(0, 0, 1)); float side = dot(c.v - old, r0) / (dt * fly_speed + 1.f); c.bank += clampf(clampf(side * 0.9f, -0.7f, 0.7f) - c.bank, -1.2f * dt, 1.2f * dt); float yw = atan2f(-f.x, f.y), pt = asinf(clampf(f.z, -1.f, 1.f)); orient_m(c.R, yw, pt, c.bank);
            if (l < 230000.f && c.cool <= 0 && dot(norm(to), f) > 0.8f) { c.cool = (c.kind == K_CMDR ? 1.1f : 2.2f) / H; Bolt b; b.p = c.p + f * 5000.f; V lead = me_p + me_R.r[1] * (fly_speed * (l / 130000.f)) + V(rnd(-1, 1), rnd(-1, 1), rnd(-1, 1)) * 5000.f; b.v = norm(lead - b.p) * 130000.f; b.life = 3.f; bolts.push_back(b); }
            if (c.kind == K_FIGHTER && (c.t > 26.f || l > 900000.f)) c.alive = false; }
        else if (c.kind == K_PYLON || c.kind == K_TOWER) { if (l < 330000.f && c.cool <= 0 && c.p.y > me_p.y - 40000.f) { c.cool = 3.2f / H; Bolt b; b.p = foe_centre(c) + V(0, 0, 18000.f); V lead = me_p + me_R.r[1] * (fly_speed * (l / 130000.f)); b.v = norm(lead - b.p) * 130000.f; b.life = 3.5f; bolts.push_back(b); } if (c.p.y < me_p.y - 200000.f) c.alive = false; }
        else if (c.kind == K_ORB) { if (c.cool <= 0 && l < 500000.f) { c.cool = 1.6f / H; Bolt b; b.p = c.p; b.v = norm(me_p - c.p) * 130000.f; b.life = 4.f; bolts.push_back(b); } } }
    for (size_t i = 0; i < bolts.size();) { Bolt &b = bolts[i]; b.p += b.v * dt; b.life -= dt; bool dead = b.life <= 0; if (!dead && near_ship(b.p, 5500.f)) { dead = true; hurt(6.f); } if (dead) { bolts[i] = bolts.back(); bolts.pop_back(); } else i++; }
    for (size_t i = 0; i < miss.size();) { Miss &m = miss[i]; m.life -= dt; bool dead = m.life <= 0; if (m.target >= 0 && m.target < (int)foes.size() && foes[m.target].alive) { V to = foe_centre(foes[m.target]) - m.p; float l = len(to); V w = norm(to) * 190000.f; m.v += (w - m.v) * clampf(5.f * dt, 0.f, 1.f); if (l < foes[m.target].r + 5000.f) { damage(foes[m.target], 5.f); dead = true; } } m.p += m.v * dt; if (dead) { miss[i] = miss.back(); miss.pop_back(); } else i++; }
    for (size_t i = 0; i < picks.size();) { Pick &p = picks[i]; p.t += dt; bool dead = p.p.y < me_p.y - 60000.f; if (near_ship(p.p, 15000.f)) { if (p.fuel) fuel = (std::min)(100.f, fuel + 30.f); else missiles = (std::min)(12, missiles + 3); add_score(250, 0); sfx(5, SND_PICK, 90); dead = true; dl("t=%.1f power-up %s fuel %.0f missiles %d", sim_t, p.fuel ? "fuel" : "missiles", fuel, missiles); } if (dead) { picks[i] = picks.back(); picks.pop_back(); } else i++; }
    for (size_t i = 0; i < beams.size();) { beams[i].t += dt; if (beams[i].t > 0.09f) { beams[i] = beams.back(); beams.pop_back(); } else i++; }
    for (size_t i = 0; i < booms.size();) { booms[i].t += dt; if (booms[i].t > 1.1f) { booms[i] = booms.back(); booms.pop_back(); } else i++; }
    for (size_t i = 0; i < bits.size();) { Bit &k = bits[i]; k.t += dt; k.v.z -= 26000.f * dt; k.p += k.v * dt; if (k.t > 2.4f) { bits[i] = bits.back(); bits.pop_back(); } else i++; }
    /* the course: the launch tube, asteroid space, the planet-fall over the dark side, the river and the dam, then the core. The stills' scores give this order */
    if (stage == ST_TUBE && me_p.y > TUBE_N * TUBE_L) enter_stage(ST_SPACE);
    else if (stage == ST_SPACE && me_p.y > SPACE_LEN) { score += 5000; enter_stage(ST_NIGHT); }
    else if (stage == ST_NIGHT && me_p.y > NIGHT_LEN) { score += 1500000; enter_stage(ST_VALLEY); }
    else if (stage == ST_VALLEY && me_p.y > 1000000.f) { Foe keep; (void)keep; enter_stage(ST_CORE, me_p.y); }
    else if (stage == ST_CORE) { bool orb = false; for (const Foe &c : foes) if (c.alive && c.kind == K_ORB) orb = true; if (!orb) { core_cool += dt; if (core_cool > 3.f) { core_cool = 0; wave++; score = (std::min)(9999999, score + 500000); dl("t=%.1f core destroyed, round %d", sim_t, wave + 1); enter_stage(ST_TUBE); } } }
    if (shield <= 0 || fuel <= 0) { boom(me_p + me_R.r[1] * 9000.f, 6.f); st = S_OVER; st_t = 0; dl("t=%.1f over (%s), stage %d score %d", sim_t, shield <= 0 ? "shield" : "fuel", stage, score); }
}
static void put_cockpit(VPIX_Frame &f, V eye) { const M &R = me_R; VPIX_Instance &c = dput(f, stage == ST_TUBE ? M_COCK_DIM : M_COCK, R, eye); c.on_top = 2; c.bright = 1; f.top_clear = true; }
static void put_foes(VPIX_Frame &f, V eye, const M &cam) {
    for (const Foe &c : foes) { if (!c.alive) continue; M R = c.R;
        if (c.kind == K_ROID) { V a = c.axis; float s = sinf(c.ang), k = cosf(c.ang); auto rot = [&](V v) { return v * k + cross(a, v) * s + a * (dot(a, v) * (1 - k)); }; R.r[0] = rot(V(1, 0, 0)); R.r[1] = rot(V(0, 1, 0)); R.r[2] = rot(V(0, 0, 1)); }
        else if (c.kind == K_PYLON || c.kind == K_TOWER || c.kind == K_JET || c.kind == K_ORB) { R = M(); }
        VPIX_Instance &i = dput(f, c.id, R, c.p); i.bright = 1; if (c.size != 1.f) for (int s = 0; s < 3; s++) i.S[s] = c.size;
        if (c.kind == K_TOWER) { float a = -1.05f + 0.35f * sinf(c.t * 0.5f + c.ang); VPIX_Instance &q = dput(f, M_CONE, d1k::yawed(a), c.p + V(0, 0, 26000.f)); q.bright = 1; q.glass = 170; } }
    if (lock_idx >= 0 && lock_idx < (int)foes.size() && foes[lock_idx].alive) { V p = foe_centre(foes[lock_idx]), to = p - eye; float d = len(to); M b; b.r[1] = to * (1.f / d); b.r[0] = norm(cross(b.r[1], cam.r[2])); b.r[2] = cross(b.r[0], b.r[1]); VPIX_Instance &k = dput(f, M_LOCK, b, p - b.r[1] * (foes[lock_idx].r + 200.f)); k.bright = 1; k.on_top = 1; float sc = d * 0.023f / 1000.f; for (int s = 0; s < 3; s++) k.S[s] = sc; }
    for (const Bolt &b : bolts) { VPIX_Instance &i = dput(f, M_BOLT, look_at(b.p, b.p + b.v), b.p); i.bright = 1; }
    for (const Miss &m : miss) { VPIX_Instance &i = dput(f, M_MISS, look_at(m.p, m.p + m.v), m.p); i.bright = 1; for (int s = 0; s < 3; s++) i.S[s] = 2.f; }
    for (const Pick &p : picks) { VPIX_Instance &i = dput(f, p.fuel ? M_FUEL : M_AMMO, d1k::yawed(p.t * (p.fuel ? 0.f : 1.f) + 0.7f), p.p); i.bright = 1; for (int s = 0; s < 3; s++) i.S[s] = 2.f; }
    for (const Boom &b : booms) { VPIX_Instance &i = dput(f, M_BOOM, d1k::yawed(b.t * 2.f), b.p); i.bright = 1; float k = (2.5f + b.t * 5.f) * b.size; for (int a = 0; a < 3; a++) i.S[a] = k; }
    for (const Bit &k : bits) { float a = k.t * k.spin; M r = d1k::yawed(a); M p; p.r[0] = r.r[0]; p.r[1] = r.r[1] * cosf(a * 0.7f) + r.r[2] * sinf(a * 0.7f); p.r[2] = r.r[2] * cosf(a * 0.7f) - r.r[1] * sinf(a * 0.7f); VPIX_Instance &i = dput(f, k.id, p, k.p); i.bright = 1; for (int s = 0; s < 3; s++) i.S[s] = 2.2f; }
    for (const Beam &b : beams) { V d = b.b - b.a; float l = len(d); if (l < 1.f) continue; M m; m.r[1] = d * (1.f / l); m.r[0] = norm(cross(m.r[1], eye - b.a)); m.r[2] = cross(m.r[0], m.r[1]); VPIX_Instance &i = dput(f, M_BEAM, m, b.a); i.bright = 1; i.on_top = 2; i.S[1] = l; }
}
static void put_wings(VPIX_Frame &f) {   /* linked: the other pilots' ships, flying beside the lead */
    if (!v1knet::on) return; const int vs = v1knet::master ? 0 : v1knet::my_seat; const V lp = v1knet::master ? me_p : lead_p;
    if (vs != 0) { VPIX_Instance &i = dput(f, M_CRAFT, me_R, lp); i.bright = 1; }
    for (int s = 1; s < 4; s++) if (wing[s].on && s != vs) { VPIX_Instance &i = dput(f, M_CRAFT, me_R, lp + me_R.r[0] * wing[s].ox + me_R.r[2] * wing[s].oz); i.bright = 1; }
}
static void put_valley(VPIX_Frame &f, V eye) { M I; (void)eye; { VPIX_Instance &s = dput(f, M_SKY, I, V(eye.x, eye.y, 0)); s.bright = 1; s.backdrop = 1; } dput(f, M_RIVER, I, V(0, 0, 0)).bright = 1; dput(f, M_PYRS, I, V(0, 0, 0)).bright = 1; dput(f, M_LAKE, I, V(0, 0, 0)).bright = 1; dput(f, M_LAND, I, V(0, 0, 0)).bright = 1; dput(f, M_ROCK, I, V(0, 0, 0)); dput(f, M_DAM, I, V(0, 0, 0)).bright = 1; }
static void set_cam(VPIX_Frame &f, const M &cam, V eye) { for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; } f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z; f.in_head = false; }
static float lerp_table(const float (*T)[2], int n, float t) { if (t <= T[0][0]) return T[0][1]; for (int i = 0; i + 1 < n; i++) if (t < T[i + 1][0]) { float u = (t - T[i][0]) / (T[i + 1][0] - T[i][0]); return T[i][1] + (T[i + 1][1] - T[i][1]) * u; } return T[n - 1][1]; }
/* ---- the attract, cut as the clip: 0-26 s one flight to the dam and over it, with the cards laid over it, then LIVE IT, the wipe, the box, the logo ---- */
static const float ATTRACT_LEN = 34.f, BOARD_LEN = 7.f;
static void build_attract(VPIX_Frame &f) {
    const float t = attract_t; int card = t >= 3.05f && t < 5.0f ? 1 : t >= 10.05f && t < 12.0f ? 2 : t >= 17.05f && t < 19.0f ? 3 : t >= 25.9f && t < 28.1f ? 4 : 0;
    M cam; V eye(0, 0, 0); cam = look_at(eye, V(0, 1000, 0));
    if (t >= 34.f) { f.clear[0] = f.clear[1] = f.clear[2] = 0; set_cam(f, cam, eye); return; }
    if (t < 25.9f && !card) {
        static const float Y[][2] = {{0, 80000.f}, {2.5f, 195000.f}, {14.5f, 340000.f}, {20.f, 530000.f}, {22.8f, 612000.f}, {25.9f, 735000.f}}; float yy = lerp_table(Y, 6, t);
        float x = sinf(t * 0.45f) * 14000.f, br = sinf(t * 0.45f + 1.2f) * 0.22f, ground = (std::max)(land_h(x, yy), land_h(x, yy + 9000.f)); if (yy > DAM_Y - 60000.f) ground = (std::max)(ground, 44000.f); if (yy > DAM_Y + 20000.f) ground = 22000.f; float zz = ground + 15000.f;
        M cr; orient_m(cr, 0, 0, br * 0.6f); V cp(x, yy, zz); eye = cp + V(-x * 0.25f, -27000.f, 7000.f); eye.z = (std::max)(eye.z, (std::max)(yy > DAM_Y ? 15000.f : land_h(eye.x, eye.y), 0.f) + 3500.f); cam = look_at(eye, cp + V(0, 30000.f, -4500.f)); { float roll = sinf(t * 0.31f + 0.4f) * 0.04f; V r0 = cam.r[0], r2 = cam.r[2]; cam.r[0] = r0 * cosf(roll) + r2 * sinf(roll); cam.r[2] = r2 * cosf(roll) - r0 * sinf(roll); }
        put_valley(f, eye); M I; for (int i = 0; i < 6; i++) dput(f, M_JET, I, V(-100000.f + i * 40000.f, DAM_Y - 26000.f, -2000.f)).bright = 1;
        for (int i = 0; i < 8; i++) { float dir = i & 1 ? -1.f : 1.f, r = 60000.f + i * 11000.f, a = i * 1.05f + dir * t * 48000.f / r; M o; orient_m(o, dir > 0 ? a : a + 3.14159265f, 0, dir * -0.5f); dput(f, M_CRAFT2, o, V(x + cosf(a) * r, yy + 100000.f + sinf(a) * r * 0.6f, zz + 42000.f + i * 7000.f)).bright = 1; }
        static const float BT[5][3] = {{1.0f, 3.5f, 1.f}, {7.6f, 8.9f, 0.45f}, {8.9f, 10.f, 0.9f}, {13.6f, 16.6f, 0.5f}, {20.4f, 22.7f, 1.1f}};
        for (int n = 0; n < 5; n++) if (t >= BT[n][0] && t < BT[n][1]) { float bt = t - BT[n][0], len_t = BT[n][1] - BT[n][0], k = clampf(bt * 3.f, 0.15f, 1.f) * clampf((len_t - bt) * 4.f, 0.25f, 1.f) * BT[n][2]; V bc = cp + V(-200, 2500, -1800); VPIX_Instance &b = dput(f, M_BOOM, d1k::yawed(t * 0.9f), bc); b.bright = 1; for (int a = 0; a < 3; a++) b.S[a] = 7.5f * k;
                for (int i = 0; i < 14; i++) { float ang = i * 2.399f + n; V d(cosf(ang) * 15000.f, sinf(ang) * 8000.f + 4000.f, 8000.f + (i % 4) * 5500.f); V pos = bc + d * bt + V(0, 0, -7000.f * bt * bt); M r; orient_m(r, ang + bt * 0.8f, 0.4f + i, 0.3f * i); VPIX_Instance &q = dput(f, i & 1 ? M_PLATE : M_SHARD, r, pos); q.bright = 1; for (int s = 0; s < 3; s++) q.S[s] = i % 5 == 0 ? 2.4f : 1.2f; } }
        dput(f, M_CRAFT, cr, cp).bright = 1;
        set_cam(f, cam, eye);
        { static const float ON[][2] = {{1.0f, 3.0f}, {5.0f, 7.4f}, {8.9f, 10.0f}, {21.0f, 22.2f}, {24.1f, 25.8f}}; bool on = false; for (auto &o : ON) if (t >= o[0] && t < o[1]) on = true; if (on) insert_credit(f); }
    } else if (t < 28.1f || card) { f.clear[0] = f.clear[1] = f.clear[2] = 0; set_cam(f, cam, eye); if (card) blit(f, BM_CARD + card - 1, CARDS[card - 1].w, CARDS[card - 1].h, (float)CARDS[card - 1].x, (float)CARDS[card - 1].y, 1.f); }
    else if (t < 29.f) { f.clear[0] = f.clear[1] = f.clear[2] = 0; set_cam(f, cam, eye); float u = clampf((t - 28.1f) / 0.9f, 0.f, 1.f); blit(f, BM_WIPE, 4, 4, 300.f * (1 - u), 18.f - 18.f * u, 88.f); f.blits.back().dh = 16.f; blit(f, BM_WIPE, 4, 4, 0, 258.f + 16.f * u, 88.f); f.blits.back().dh = 18.f; }
    else if (t < 31.73f) { f.clear[0] = f.clear[1] = f.clear[2] = 0; dput(f, M_STARS, M(), V(0, 0, 0)).backdrop = 1; const float fw = 58000.f; M R; float u = t - 29.f, op = clampf((u - 0.5f) / 1.5f, 0.f, 1.f), turn = clampf((u - 2.1f) / 0.6f, 0.f, 1.f); turn = turn * turn; R = d1k::yawed(-0.35f - turn * 0.7f); { V r0 = R.r[0], r2 = R.r[2]; float rl = turn * 0.32f; R.r[0] = r0 * cosf(rl) + r2 * sinf(rl); R.r[2] = r2 * cosf(rl) - r0 * sinf(rl); } if (u < 2.1f) { R = d1k::yawed(0.12f); }
        V c = V(0, fw * (1.f + turn * 0.45f), -500.f + turn * 2000.f); { VPIX_Instance &b = dput(f, M_BOX, R, c); b.bright = 1; } dput(f, M_DOOR_IN, R, c).bright = 1; const float DZ = -2700.f; V up = tw(R, V(0, 0, 1)); { VPIX_Instance &d = dput(f, M_DOOR_T, R, c + up * (DZ + op * 8500.f)); d.bright = 1; } { VPIX_Instance &d = dput(f, M_DOOR_B, R, c + up * (DZ - op * 8500.f)); d.bright = 1; } set_cam(f, cam, eye); }
    else { f.clear[0] = f.clear[1] = f.clear[2] = 0; static const float DT[][2] = {{31.73f, 6000.f}, {32.f, 10000.f}, {32.6f, 33000.f}, {33.2f, 39000.f}, {33.8f, 41200.f}}; float u = clampf((t - 31.73f) / 0.9f, 0.f, 1.f), d = lerp_table(DT, 5, t); dput(f, M_STARS, M(), V(0, 0, 0)).backdrop = 1; float tilt = (1 - u) * (1 - u) * 0.5f; M R = d1k::yawed(tilt); V c(sinf(tilt) * -9000.f * (1 - u), d, 0); dput(f, M_LOGO, R, c).bright = 1; { float p = fmodf(t, 1.3f); VPIX_Instance &s = dput(f, M_SPARK, M(), V(11000.f, d - 200.f, -9500.f - 700.f * p)); s.bright = 1; for (int a = 0; a < 3; a++) s.S[a] = 1.f + p * 1.5f; } set_cam(f, cam, eye); }
}
static void build_play(VPIX_Frame &f, const VPIX_Input &in) {
    (void)in; f.clear[0] = f.clear[1] = f.clear[2] = 0; V eye = me_p; if (shake > 0) eye += V(rnd(-1, 1), rnd(-1, 1), rnd(-1, 1)) * (220.f * shake); M cam = me_R;
    if (have_look || autoplay) { float ya = look_y, pi = look_p; V fw = cam.r[1], ri = cam.r[0], up = cam.r[2]; V f2 = fw * cosf(ya) + ri * sinf(ya), r2 = ri * cosf(ya) - fw * sinf(ya); V f3 = f2 * cosf(pi) + up * sinf(pi), u3 = up * cosf(pi) - f2 * sinf(pi); cam.r[0] = r2; cam.r[1] = f3; cam.r[2] = u3; }
    M I;
    if (stage == ST_TUBE) { dput(f, M_STARS, I, eye).backdrop = 1; int s0 = (int)floorf(me_p.y / TUBE_L); for (int i = -2; i <= 3; i++) { int s = s0 + i; if (s < 0 || s >= TUBE_N) continue; dput(f, M_TUBE, I, V(0, s * TUBE_L, 0)).bright = 1; } }   /* the far rings are not drawn, the still shows black with stars after about four */
    else if (stage == ST_SPACE) { dput(f, M_STARS, I, eye).backdrop = 1; if (me_p.y < 700000.f) { M bx = d1k::yawed(3.14159265f); float yy = -60000.f; { dput(f, M_BOX, bx, V(0, yy, 0)).bright = 1; dput(f, M_DOOR_IN, bx, V(0, yy, 0)).bright = 1; V up(0, 0, 1); dput(f, M_DOOR_T, bx, V(0, yy, 0) + up * 5800.f).bright = 1; dput(f, M_DOOR_B, bx, V(0, yy, 0) + up * -11200.f).bright = 1; } } }
    else if (stage == ST_NIGHT) { { VPIX_Instance &s = dput(f, M_NSKY, I, eye); s.backdrop = 1; s.bright = 1; } dput(f, M_STARS, I, eye).backdrop = 1; dput(f, M_SEA, I, V(0, 0, 0)).bright = 1; dput(f, M_PATCH, I, V(0, 0, 0)).bright = 1; }
    else { put_valley(f, eye); if (stage == ST_CORE) dput(f, M_CORE, I, V(0, CORE_Y, 15000.f)).bright = 1; }
    put_foes(f, eye, cam); put_wings(f); put_cockpit(f, me_p); set_cam(f, cam, eye);
}
static void draw_hud(VPIX_Frame &f) {
    char b[16]; snprintf(b, sizeof b, "%07d", score); for (int i = 0; i < 7; i++) { blit(f, BM_DIGIT + (b[i] - '0'), 24, 26, 79.f + i * 26.5f, 12.f, 1.f); f.blits.back().alpha = 0.88f; }
    blit(f, BM_RING, 44, 44, 154.f, 122.f, 1.f);
    blit(f, BM_PANEL, 148, 19, 102.f, 269.f, 1.f);
    { float u = clampf(shield / start_hp, 0.f, 1.f); if (u > 0) { blit(f, BM_BAR, 4, 4, 110.f, 275.f, 1.f); f.blits.back().dw = 58.f * u; f.blits.back().dh = 9.f; } float v = clampf(fuel / 100.f, 0.f, 1.f); if (v > 0) { blit(f, BM_BAR, 4, 4, 184.f, 275.f, 1.f); f.blits.back().dw = 58.f * v; f.blits.back().dh = 9.f; } }
}
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = FOV; f.n_lights = 1; f.lights[0] = {1.0f, 0.f, {-0.3f, -0.4f, 0.87f}};
    if (st == S_TITLE) { build_attract(f); }
    else if (st == S_PLAY) { build_play(f, in); draw_hud(f); }
    else { build_play(f, in); draw_hud(f); d1k::text(f, "GAME OVER", 176, 130, 1.4f, 1); }
    /* the board takes its turn between passes of the attract, not over them */
    d1k::hs::draw(f, "xtreme", st == S_TITLE && attract_t >= ATTRACT_LEN, st == S_TITLE && attract_t >= ATTRACT_LEN ? 23.f + (attract_t - ATTRACT_LEN) * 0.99f : (float)sim_t);
    VPIX_Render_Submit(f);
}
static void make_sounds() {   /* INVENTED placeholders. The clip's soundtrack is music under everything, no game effects can be picked out of it */
    std::mt19937 r(5); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(SND_FIRE, 0.16f, [&](float t) { return sinf(6.2831853f * (1400.f - 3600.f * t) * t) * exp(-t * 14.f) * 0.6f; });
    make_sound(SND_BOOM, 1.1f, [&](float t) { return noise() * exp(-t * 3.f) * 0.9f + sinf(6.2831853f * 55.f * t) * exp(-t * 2.5f) * 0.6f; });
    make_sound(SND_HURT, 0.4f, [&](float t) { return (fmodf(t, 0.08f) < 0.04f ? 0.6f : -0.6f) * exp(-t * 5.f); });
    make_sound(SND_GO, 0.6f, [&](float t) { return sinf(6.2831853f * (t < 0.3f ? 660.f : 880.f) * t) * 0.5f; });
    make_sound(SND_DROID, 0.5f, [&](float t) { static const float N[5] = {1320.f, 1760.f, 990.f, 1980.f, 1480.f}; int i = (std::min)((int)(t / 0.1f), 4); return (sinf(6.2831853f * N[i] * t) > 0 ? 0.35f : -0.35f) * exp(-fmodf(t, 0.1f) * 12.f); });
    make_sound(SND_MISS, 0.7f, [&](float t) { return noise() * exp(-t * 3.f) * 0.5f + sinf(6.2831853f * (200.f + 500.f * t) * t) * 0.3f; });
    make_sound(SND_PICK, 0.3f, [&](float t) { return sinf(6.2831853f * (700.f + 1400.f * t) * t) * 0.5f; });
    make_sound(SND_LOCK, 0.08f, [&](float t) { return sinf(6.2831853f * 2000.f * t) * 0.4f; });
}
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 2; static unsigned long frames = 0;
/* ---- link play: snapshot out of the master, wingman input into it ---- */
struct NW { uint8_t *b; size_t n; template <class T> void put(T v) { memcpy(b + n, &v, sizeof(T)); n += sizeof(T); } };
struct NR { const uint8_t *b; size_t n, i; bool ok; template <class T> T get() { T v{}; if (i + sizeof(T) > n) { ok = false; return v; } memcpy(&v, b + i, sizeof(T)); i += sizeof(T); return v; } };
static int16_t q16(float a) { return (int16_t)lroundf(clampf(wrap(a), -3.14159f, 3.14159f) * 10000.f); }
static float d16(int16_t v) { return v * 0.0001f; }
static int8_t q8(float v) { return (int8_t)lroundf(clampf(v, -1.f, 1.f) * 127.f); }
static uint16_t qa(float a, float period, float k) { a = fmodf(a, period); if (a < 0) a += period; return (uint16_t)(a * k); }
static void put_dir(NW &w, V d) { V n = len(d) > 1e-3f ? norm(d) : V(0, 1, 0); w.put<int8_t>(q8(n.x)); w.put<int8_t>(q8(n.y)); w.put<int8_t>(q8(n.z)); }
static V get_dir(NR &r) { float x = r.get<int8_t>() / 127.f, y = r.get<int8_t>() / 127.f, z = r.get<int8_t>() / 127.f; return V(x, y, z); }
static void put_pos(NW &w, V p) { w.put<float>(p.x); w.put<float>(p.y); w.put<float>(p.z); }
static V get_pos(NR &r) { float x = r.get<float>(), y = r.get<float>(), z = r.get<float>(); return V(x, y, z); }
enum { N_FOES = 24, N_BOLTS = 14, N_MISS = 6, N_BOOMS = 6, N_BEAMS = 5, N_PICKS = 3, NET_MIN = 40 };
static void net_send() {
    if (!v1knet::due(30)) return;
    if (st == S_TITLE) { v1knet::send_snapshot(NULL, 0); return; }
    static uint8_t buf[v1knet::SNAP_MAX]; NW w{buf, 0};
    w.put<uint8_t>((uint8_t)st); w.put<uint8_t>((uint8_t)stage); w.put<uint8_t>((uint8_t)wave); w.put<uint8_t>((uint8_t)missiles); w.put<int32_t>(score); w.put<float>(shield); w.put<float>(fuel);
    put_pos(w, me_p); w.put<int16_t>(q16(me_yaw)); w.put<int16_t>(q16(me_pitch)); w.put<int16_t>(q16(me_bank)); w.put<uint8_t>(shake > 0 ? 1 : 0);
    for (int s = 1; s < 4; s++) { w.put<uint8_t>(wing[s].on ? 1 : 0); w.put<float>(wing[s].ox); w.put<float>(wing[s].oz); }
    std::vector<std::pair<float, int>> idx; for (size_t i = 0; i < foes.size(); i++) if (foes[i].alive) { V d = foes[i].p - me_p; idx.push_back({dot(d, d), (int)i}); }
    std::sort(idx.begin(), idx.end()); if (idx.size() > N_FOES) idx.resize(N_FOES);
    w.put<uint8_t>((uint8_t)idx.size());
    for (auto &pr : idx) { const Foe &c = foes[pr.second]; V f = c.R.r[1]; w.put<uint8_t>((uint8_t)c.kind); w.put<uint8_t>((uint8_t)(c.id - M_LAND)); w.put<uint8_t>((uint8_t)clampf(c.size * 8.f, 0.f, 255.f)); put_pos(w, c.p);
        w.put<int16_t>(q16(atan2f(-f.x, f.y))); w.put<int16_t>(q16(asinf(clampf(f.z, -1.f, 1.f)))); w.put<int16_t>(q16(c.bank)); w.put<uint16_t>(qa(c.ang, 6.2831853f, 10000.f)); w.put<uint16_t>(qa(c.t, 12.566371f, 1000.f)); put_dir(w, c.axis); }
    int n = (int)(std::min)((size_t)N_BOLTS, bolts.size()); w.put<uint8_t>((uint8_t)n); for (int i = 0; i < n; i++) { put_pos(w, bolts[i].p); put_dir(w, bolts[i].v); }
    n = (int)(std::min)((size_t)N_MISS, miss.size()); w.put<uint8_t>((uint8_t)n); for (int i = 0; i < n; i++) { put_pos(w, miss[i].p); put_dir(w, miss[i].v); }
    n = (int)(std::min)((size_t)N_BOOMS, booms.size()); w.put<uint8_t>((uint8_t)n); for (int i = 0; i < n; i++) { put_pos(w, booms[i].p); w.put<uint8_t>((uint8_t)clampf(booms[i].t * 100.f, 0.f, 255.f)); w.put<uint8_t>((uint8_t)clampf(booms[i].size * 8.f, 0.f, 255.f)); }
    n = (int)(std::min)((size_t)N_BEAMS, beams.size()); w.put<uint8_t>((uint8_t)n); for (int i = 0; i < n; i++) { put_pos(w, beams[i].a); put_pos(w, beams[i].b); w.put<uint8_t>((uint8_t)clampf(beams[i].t * 1000.f, 0.f, 255.f)); }
    n = (int)(std::min)((size_t)N_PICKS, picks.size()); w.put<uint8_t>((uint8_t)n); for (int i = 0; i < n; i++) { put_pos(w, picks[i].p); w.put<uint8_t>((uint8_t)picks[i].fuel); w.put<uint16_t>((uint16_t)clampf(picks[i].t * 100.f, 0.f, 65000.f)); }
    v1knet::send_snapshot(buf, w.n);
}
static bool net_apply(const uint8_t *d, size_t len_) {
    NR r{d, len_, 0, true}; static int prev_booms = 0, prev_beams = 0; static float prev_shield = 100;
    st = (State)r.get<uint8_t>(); stage = r.get<uint8_t>(); wave = r.get<uint8_t>(); missiles = r.get<uint8_t>(); score = r.get<int32_t>(); shield = r.get<float>(); fuel = r.get<float>();
    lead_p = get_pos(r); me_yaw = d16(r.get<int16_t>()); me_pitch = d16(r.get<int16_t>()); me_bank = d16(r.get<int16_t>()); shake = r.get<uint8_t>() ? 0.2f : 0.f;
    for (int s = 1; s < 4; s++) { wing[s].on = r.get<uint8_t>() != 0; wing[s].ox = r.get<float>(); wing[s].oz = r.get<float>(); }
    if (!r.ok || stage > ST_CORE || st > S_OVER) return false;
    orient_m(me_R, me_yaw, me_pitch, me_bank); lock_idx = -1;
    foes.clear(); int n = r.get<uint8_t>();
    for (int i = 0; i < n && r.ok; i++) { Foe c; c.kind = r.get<uint8_t>(); c.id = M_LAND + r.get<uint8_t>(); c.size = r.get<uint8_t>() / 8.f; c.p = get_pos(r); float yw = d16(r.get<int16_t>()), pt = d16(r.get<int16_t>()); c.bank = d16(r.get<int16_t>()); c.ang = r.get<uint16_t>() * 0.0001f; c.t = r.get<uint16_t>() * 0.001f; c.axis = get_dir(r); orient_m(c.R, yw, pt, c.bank); c.alive = true; foes.push_back(c); }
    bolts.clear(); n = r.get<uint8_t>(); for (int i = 0; i < n && r.ok; i++) { Bolt b; b.p = get_pos(r); b.v = get_dir(r) * 130000.f; b.life = 1; bolts.push_back(b); }
    miss.clear(); n = r.get<uint8_t>(); for (int i = 0; i < n && r.ok; i++) { Miss m; m.p = get_pos(r); m.v = get_dir(r) * 190000.f; m.target = -1; m.life = 1; miss.push_back(m); }
    booms.clear(); n = r.get<uint8_t>(); for (int i = 0; i < n && r.ok; i++) { Boom b; b.p = get_pos(r); b.t = r.get<uint8_t>() * 0.01f; b.size = r.get<uint8_t>() / 8.f; booms.push_back(b); }
    beams.clear(); n = r.get<uint8_t>(); for (int i = 0; i < n && r.ok; i++) { Beam b; b.a = get_pos(r); b.b = get_pos(r); b.t = r.get<uint8_t>() * 0.001f; beams.push_back(b); }
    picks.clear(); n = r.get<uint8_t>(); for (int i = 0; i < n && r.ok; i++) { Pick p; p.p = get_pos(r); p.fuel = r.get<uint8_t>(); p.t = r.get<uint16_t>() * 0.01f; picks.push_back(p); }
    if (!r.ok) return false;
    bits.clear(); const int ms = v1knet::my_seat; me_p = lead_p + me_R.r[0] * wing[ms].ox + me_R.r[2] * wing[ms].oz;   /* the eye is my spot beside the lead */
    if ((int)booms.size() > prev_booms) sfx(3, SND_BOOM, 70); if ((int)beams.size() > prev_beams) sfx(1, SND_FIRE, 40); if (shield < prev_shield - 1.f) sfx(2, SND_HURT, 100);
    prev_booms = (int)booms.size(); prev_beams = (int)beams.size(); prev_shield = shield; return true;
}
static void net_leave(const char *why) { if (!slave_game) return; slave_game = false; st = S_TITLE; st_t = 0; attract_t = 0; foes.clear(); bolts.clear(); booms.clear(); bits.clear(); beams.clear(); picks.clear(); miss.clear(); for (int i = 0; i < 4; i++) wing[i] = Wing(); shake = 0; orient_m(me_R, 0, 0, 0); dl("t=%.1f link: %s, back to the title", sim_t, why); }
static void net_log() {
    static double nl = 0; if (sim_t < nl) return; nl = sim_t + 2.0;
    if (!v1knet::master && !slave_game) { dl("t=%.1f link: slave, title, master heard %d", sim_t, v1knet::master_heard() ? 1 : 0); return; }
    if (v1knet::master && st == S_TITLE) { dl("t=%.1f link: master, title, pods heard %d", sim_t, v1knet::live_pods()); return; }
    dl("t=%.1f link: %s seat %d st %d stage %d score %d shield %.0f fuel %.0f | me (%.0f %.0f %.0f) | wings %d%d%d off1 (%.0f %.0f) off2 (%.0f %.0f) off3 (%.0f %.0f) | foes %d bolts %d", sim_t, v1knet::master ? "master" : "slave", v1knet::master ? 0 : v1knet::my_seat, (int)st, stage, score, shield, fuel, me_p.x, me_p.y, me_p.z,
       wing[1].on, wing[2].on, wing[3].on, wing[1].ox, wing[1].oz, wing[2].ox, wing[2].oz, wing[3].ox, wing[3].oz, (int)foes.size(), (int)bolts.size());
}
static void net_master_seats() {
    for (int s = 1; s < 4; s++) { Wing &w = wing[s]; const NetIn *p = (const NetIn *)v1knet::seat_input(s, sizeof(NetIn));
        if (!p || st != S_PLAY) { w.on = false; continue; }
        if (!w.on) { const float sp = stage == ST_TUBE ? 4000.f : 40000.f; w.on = true; w.ox = s == 1 ? -sp : s == 2 ? sp : 0.f; w.oz = s == 3 ? sp * 0.5f : 0.f; w.lf = w.fseq = p->fseq; w.lm = w.mseq = p->mseq; w.cool = w.mcool = 0; dl("t=%.1f link: wingman seat %d joins", sim_t, s); }
        w.sx = clampf(p->sx, -1.f, 1.f); w.sy = clampf(p->sy, -1.f, 1.f); V a(p->ax, p->ay, p->az); w.aim_l = len(a) > 0.5f && len(a) < 1.5f ? a : V(0, 1, 0); w.fseq = p->fseq; w.mseq = p->mseq; }
}
static bool net_tick(double el, const VPIX_Input &in, bool trig, bool top) {   /* true = this tick is a linked wingman's, the normal one is skipped */
    v1knet::pump();
    if (v1knet::master) { net_master_seats(); net_send(); net_log(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) { if (n < NET_MIN || v1knet::my_seat < 0) net_leave("the master's game has no seat for me"); else if (net_apply(d, n)) slave_game = true; else net_leave("bad snapshot"); }
    if (slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
    net_log();
    if (!slave_game) { v1knet::send_input(NULL, 0); return false; }
    static uint8_t fs = 0, ms = 0; static float fcool = 0; static bool top_was = false;
    float sx = in.joy_r[0], sy = -in.joy_r[1]; bool fire = trig, mis = top;
    if (autoplay) { sx = 0.8f * sinf((float)sim_t * 0.6f); sy = 0.4f * cosf((float)sim_t * 0.4f); fire = fmodf((float)sim_t, 1.2f) < 0.7f; mis = fmodf((float)sim_t, 9.f) < 0.3f; }
    fcool -= (float)el; if (fire && fcool <= 0) { fs++; fcool = FIRE_GAP; } if (mis && !top_was) ms++; top_was = mis;
    NetIn o; o.sx = sx; o.sy = sy; V a = in.head.valid ? V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2]) : V(0, 1, 0); o.ax = a.x; o.ay = a.y; o.az = a.z; o.fseq = fs; o.mseq = ms;
    v1knet::send_input(&o, sizeof(o));
    sim_t += el; st_t += (float)el; acc = 0;
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    return true;
}
static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    static bool trig_was = false;
    bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.start, sel = trig && !trig_was; trig_was = trig;
    bool top = in.top || in.b2 || (in.buttons & VPIX_BTN_3) != 0 || (GetAsyncKeyState('M') & 0x8000) != 0;   /* the plasma missile: top button, M on the desktop */
    if (autoplay && st != S_PLAY && st_t > 2.f && !attract_only) sel = true;
    if (st == S_OVER && st_t > 15.f) sel = true; if (d1k::hs::run("xtreme", st == S_OVER && st_t > 2.f, score, in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (!v1knet::can_start()) sel = false;   /* a linked wingman: pod 1 starts the game */
    if (v1knet::on && net_tick(el, in, trig, top)) { PIC_AddEvent(tick, 2.0); return; }
    if (st == S_TITLE && sel) new_round();
    else if (st == S_OVER && sel && st_t > 3.f) { st = S_TITLE; st_t = 0; attract_t = 0; v1knet::end_round(); for (int i = 0; i < 4; i++) wing[i] = Wing(); foes.clear(); bolts.clear(); booms.clear(); bits.clear(); beams.clear(); }
    int n = 0;
    while (acc >= dt && n < 24) { step_world(in, trig, top, dt); acc -= dt; sim_t += dt; st_t += dt; if (st == S_TITLE) { attract_t += dt; if (attract_t >= ATTRACT_LEN + BOARD_LEN) attract_t = 0; } n++; }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    if (sim_t >= next_log) { next_log = sim_t + 2.0; dl("t=%.0f st %d stage %d me (%.0f %.0f %.0f) shield %.0f fuel %.0f missiles %d score %d lock %d foes %d", sim_t, (int)st, stage, me_p.x, me_p.y, me_p.z, shield, fuel, missiles, score, lock_idx, (int)foes.size()); }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".xtreme").c_str(), "w");
    game_fps = envf("XTREME_FPS", game_fps); fly_speed = envf("XTREME_SPEED", fly_speed); turn_rate = envf("XTREME_TURN", turn_rate); start_hp = envf("XTREME_HP", start_hp); start_fuel = envf("XTREME_FUEL", start_fuel); autoplay = envf("XTREME_AUTO", 0) != 0; attract_only = envf("XTREME_ATTRACT", 0) != 0; attract_t = envf("XTREME_ATTR_T", 0); start_stage = (int)envf("XTREME_STAGE", 0); start_y = envf("XTREME_Y", -1e30f);
    { const char *lk = getenv("XTREME_LOOK"); if (lk && *lk && sscanf(lk, "%f,%f", &look_env[0], &look_env[1]) == 2) have_look = true; }
    if (game_fps < 1.f) game_fps = 1.f;
    d1k::make_hud(); make_hud_x(); build_land(); build_tube(); build_night(); build_box(); build_logo(); build_craft(M_CRAFT, 0x202024); build_craft(M_CRAFT2, 0x303038); build_craft(M_CMDR, 0xD01818); build_roid(M_ROID0, 1); build_roid(M_ROID1, 2); build_roid(M_ROID2, 3); build_small();
    build_cockpit(M_COCK, 0xFCD040, 0x60B048); build_cockpit(M_COCK_DIM, 0x95AE36, 0x608020); make_sounds(); orient_m(me_R, 0, 0, 0);
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    PIC_AddEvent(tick, 2.0);
    v1knet::init("XTRK", false, dl);   /* start-together game */
    dl("X-Treme Strike rebuild started%s", autoplay ? ", auto" : "");
}
}   /* namespace xtreme */
