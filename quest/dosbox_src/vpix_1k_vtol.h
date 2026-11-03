/* VTOL (Virtuality 1000SD, 1991) - native rebuild from footage and period press. No 1000-series software survives in public, so every model is built here and
   no game file is read. Included after vpix_dactyl1k.h (needs d1k:: and buggy::). Dev only, not in a release. Entry point: vtol::start(logbase).
   Evidence: Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\vtol.md (every item below with its source), REMAKE_GUIDE_2026-11-02.md, research_2026-11-02\09, 12, 13.
   FROM FOOTAGE (Octavius 2YrPW5cU06A t1107-t1146; BBC Tomorrow's World 1 Nov 1990, 8A3lXFsHe_A t250-t370, the clean original of the old dji9YiPZ4AM report): menu PREVIOUS / NEXT / PLAY, FOUR rolling pages CADET, PILOT, ACE, PILOT VS PILOT with the next three as thumbnails,
   red marks counting down about 2 a second that a click does not reset; HUD SCORE nnn and n/5 in dark blue, +100 and +1 per kill; green gun box, cross, ring and line in 3D (Octavius build; the BBC and publicity clips show an older small square sight with a ring); two bars low left and right; map screen north up with a pale jet outline that turns with the heading;
   a runway with a wide grey apron, a brown control tower on a plinth, long low building, slate hangars, a parked blue jet and a red jet (the other pod's, it takes off in BBC t290-t301), a red shed, a pair of pale posts with black tops, a crossing strip; low green hills, sea, frigate, flat khaki carrier with white lines;
   a dark helicopter in the distance; grey long-nosed enemies with red fins, orange fireball with dark edge and debris; a crash puts you back at the airfield; full roll and dive authority (upside down at publicity t1145); the canopy hoop, struts, hood and panels measured from frames as (tx * d, eye y + d, eye z + tv * d).
   FROM PERIOD PRESS: vertical take-off, five bandits, machine gun plus a missile on a click, a refuelling sequence then more jets over the blue ocean, the end swims out of view (Your Commodore 79, May 1991); refuelling must be done to progress, 3-7 minutes by skill for 1 pound (Amiga User Int., Jun 1991);
   low-level bombing of an enemy HQ (The One 33); more fuel by clearing waves (W.descrip1); C-130 tanker, automatic on the first two levels and by hand on Ace, trigger cannon, top button missile when the sight finds a target, left stick speed and sideways correction, afterburner on Ace, Pilot VS Pilot (Computer+Videogiochi 9, Oct 1991); carrier landing (MC 127, Mar 1993); look all round the sky (Amiga Format 23); Sea Harrier, coastline, ocean (MC 107).
   INVENTED (marked in the code): every flight number, the order and number of stages, how a stage buys time, enemy tactics and damage, how close the drogue must be, the tanker's look, the HQ's look and place, bombs on the top button with no target, the helicopter's path, the seat, consoles, stick and back of the jet (Bobby asked for them; footage never looks there), every sound, MISSION COMPLETE and the title card, scales.
   UNKNOWN: what the left bar means (here height: it stays low and does not fall in 26 s, so not fuel); the right bar is read as gun charge because it falls while kills come and climbs after; why the map screen goes dark in the fight (Octavius has three irregular spells; here it stays lit, VTOL_MAPDARK=1 replays them); radio and tower voices (MC 127), microphone between pods, speech; music; Pilot VS Pilot without a link (here: one green bot at a time). A second human needs the queued online play.
   Colours: Octavius has a lilac sky, olive ground, grey hood; BBC and the publicity clip agree on bright green and blue-slate hood; sky pales in BBC. The sky here leans to Octavius and publicity; ground and hood to BBC.
   BOBBY'S MEMORY (11-03, first-hand): a thin needle on the nose point; the tanker trailed "an empty cup on a tether" and you put the needle's point in it. Built that way. No footage or press shows it; the footage's nose is too soft to show a needle or rule one out; the cockpit shows the needle only while refuelling because the forward views in footage have none.
   BOBBY'S MEMORY (11-03, unsure): waves of enemies out over the ocean, the game directed you to them and taught missile lock-on; "then land" to finish (MC 127 and BBC t351 say the carrier). Built: a mark on the map from stage 2, Cadet-only text, a landing last. No VTOL clip shows any instruction or lock word: Octavius t1120-t1130 has the ring on jets and hits and no text, level unknown; so the text is on Cadet only.
   BOBBY'S MEMORY (11-03): dual sticks, right flies, left throttle, sticks only, no hand motion. The grips in the cockpit show it (BBC material, Nz_BeaBS_cg t309, t319). VR uses in.joy_r and in.joy_l.
   BOBBY'S MEMORY (11-03, confident): refuelling happened once in a mission. Built: after the first five (YC79). Fuel burn halved to suit, INVENTED. BOBBY'S PICKS (footage silent): enemy boats that fire back, jets that chase you if you run, waves strung along the route, first two jets straight at you, slow and not firing, on Cadet and Pilot.
   BOBBY'S MEMORY (11-03): the line extended from the tanker and retracted again. Built; speed INVENTED.
   BOBBY'S CALL (11-03, no source): hitting the tanker or the hose blows up, in chunks with sparks like the Dactyl kill.
   BOBBY'S MEMORY (11-03): about 3 minutes, a few waves out to sea, one refuel at sea, back to land on the carrier or at base, either counts. Built: three waves east over the ocean, the third an HQ on an islet with enemy boats, then back.
   BOBBY'S MEMORY (11-03): "an easy training mode and then the normal level", never played multiplayer. Kept the four menu pages the footage shows; Cadet is the easy one, Pilot the normal.
   REAL-WORLD REFERENCE, not evidence about the game: basket shaped like a shuttlecock, hose off a unit on the Hercules cargo ramp, amber and green lamps, retractable probes. Used only where the footage is silent.
   Dev: VTOL_LOOK=yawdeg,pitchdeg or sweep; VTOL_GAPTEST=1 cockpit only on magenta; VTOL_SIT=s the auto player waits on the ground; VTOL_RFTEST=1 scripted clean contact, 2 cup into the wing, 3 into the tanker (use with VTOL_SKILL=2); VTOL_CHASE=1 outside view of the refuelling.
   Scale: units are mm like Dactyl, but this game is written in METRES and drawn through mp(); the world is about 1/3 real size so it fits inside the renderer's 400 m far plane (fog hides the edge).
   Camera: like Buggy Ball (a vehicle), not like Dactyl: the game camera is the aircraft, f.in_head stays false, and the live head pose is composed on top by the renderer's free look (seat reference taken from the first head pose), so VR late correction is automatic.
   Controls: see vt_controls. Env: VTOL_SKILL(0-3) VTOL_FPS VTOL_TIME VTOL_WAVES VTOL_PERWAVE VTOL_ENEMIES VTOL_SPEED VTOL_CLIMB VTOL_TURN VTOL_ROLL VTOL_PITCH VTOL_BURN VTOL_REFUEL VTOL_LANDASSIST VTOL_AIM(deg, 0 = by skill) VTOL_BANNERS VTOL_AUTO VTOL_CAM(x,y,z,tx,ty,tz in metres) */
#include <functional>
#include <algorithm>
#include "vpix_1k_vtol_pics.h"
#include "vpix_1k_net.h"         /* link play, 2-4 pods: see the "link play" block before the effects */
namespace vtol {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at; using buggy::to_world;
using d1k::Mesh; using d1k::dput; using d1k::blit; using d1k::text; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound;

static const float PI_F = 3.14159265f;
static const float SEA_Z = -3.f;
static const float TIP_Y = 9.52f, TIP_Z = 1.6f, CUP_R = 1.3f;   /* the needle's tip from the jet's origin, the cup's mouth radius, metres. INVENTED */
static const float LAND_X0 = -340.f, LAND_X1 = 170.f, LAND_Y0 = -230.f, LAND_Y1 = 320.f;   /* the island, metres */
static const float CAR_X = 800.f, CAR_Y = -240.f, CAR_DECK = 10.f;                        /* the carrier: out to sea, on the way back from the last wave. BBC speech t297-t328: "head out to sea and see if I can find the aircraft carrier", over the mountains, over the sea, then it comes up. Place INVENTED */
static const float ISLE_X = 1100.f, ISLE_Y = 120.f, ISLE_R = 115.f;                         /* the enemy's islet, for the HQ. INVENTED; "bombing an enemy HQ" (The One 33) has to be on some land */
static const float CENTRE_X = 0.f, CENTRE_Y = 20.f;

/* ---- tuning (all INVENTED unless said) ---- */
static float v_max = 85.f;            /* m/s at full stick */
static float climb_rate = 28.f;       /* m/s at full vertical thrust */
static float turn_rate = 1.1f;        /* rad/s of heading at 90 degrees of bank, and in the hover */
static float roll_rate = 2.2f, pitch_rate = 1.1f;   /* rad/s at full stick. Publicity t1145 flies upside down, TV report t321 dives: the stick has full authority */
static float grav = 9.8f;
static float ceiling_z = 175.f, limit_r = 340.f;
static float safe_vz = 7.f, safe_speed = 30.f;   /* a landing is safe below both */
static float game_fps = 20.f;         /* as Dactyl: measured 20 at best on the 1000-series footage */
static float game_len = 180.f, game_max = 225.f, stage_time = 15.f;   /* "three to seven minutes" for 1 pound "depending upon the skill shown" (Amiga User Int. Jun 1991); how the skill buys time is ours: a stage passed adds a minute */
static int waves_total = 3, per_wave = 5;   /* five bandits per pass: footage n/5, Your Commodore 79. Number of stages ours */
static float fuel_burn = .7f, refuel_rate = 10.f;   /* fuel tank = 100 */
static bool land_assist = true;       /* soft sink rate near the runway, deck and grass */
static float stick_lx = 0.f, stick_x = 0.f, stick_y = 0.f;   /* what the right stick is doing, for the cockpit stick */
static float hint_ring = 0.f, hint_lock = 0.f, hint_dir = 0.f; static bool in_vr = false;   /* the Cadet page's instructions. Bobby 11-03, unsure: it "instructed you on how to lock on with missles". Wording INVENTED except LOCKED ON, which is Flying Aces' word (Octavius t995.7) and Bobby's memory; no VTOL clip shows a lock word */
static bool map_dark_env = false, nose_always = false;   /* VTOL_NOSE=1: nose and needle always in view; off, they show only while refuelling, because the footage's forward views show bare hood edge and ground */
static bool gap_test = false;   /* dev: VTOL_GAPTEST=1 draws only the cockpit on magenta: any thin magenta line is a hole */
static float sit_s = 0.f;   /* dev: VTOL_SIT, the auto player waits this long on the ground, for pictures */
static float aim_cone_deg = 0.f;      /* 0 = by skill */
static bool banners = false;   /* no text banner in any footage */
static float bullet_speed = 380.f, fire_gap = 0.11f, gun_drain = 0.125f, gun_fill = 0.11f, missile_gap = 2.f;   /* drain and fill measured off the right bar, Octavius t1121-t1127, t1127-t1134 */
static int enemies_override = 0;
struct Skill { const char *name; int max_alive; float edmg, efire, espeed, eturn, burn, cone; };
/* four menu pages, Octavius t1108-t1111: the list rolls CADET, PILOT, ACE, then a two-jet page captioned PILOT over PILOT = "Pilot VS Pilot" (Computer+Videogiochi 9). This is also why MC 127 counts four levels and C+V 9 three */
static const Skill SKILLS[4] = {{"CADET", 2, 6.f, 1.5f, 36.f, 0.9f, 0.7f, 0.24f}, {"PILOT", 3, 9.f, 1.0f, 44.f, 1.2f, 1.0f, 0.17f}, {"ACE", 4, 12.f, 0.7f, 52.f, 1.6f, 1.3f, 0.10f}, {"PILOT VS PILOT", 1, 9.f, 1.0f, 44.f, 1.2f, 1.0f, 0.17f}};
static int skill = 0;   /* the footage opens on CADET */
static const Skill &SK() { return SKILLS[skill]; }

static V mp(float x, float y, float z) { return V(x * 1000.f, y * 1000.f, z * 1000.f); }   /* metres -> mm */
static float wrap_pi(float a) { while (a > PI_F) a -= 2.f * PI_F; while (a < -PI_F) a += 2.f * PI_F; return a; }

/* ---- meshes ---- */
enum { M_SEA = 501, M_LAND, M_CLOUDS, M_CARRIER, M_FRIGATE, M_HARRIER, M_ENEMY, M_JETB, M_JETR, M_COCKPIT, M_COCKL, M_BUL, M_MIS, M_BOMB, M_TANKER, M_DROGUE, M_JETG, M_ISLE, M_STICKL, M_STICKR, M_HOSE, M_NEEDLE, M_LAMPA, M_LAMPG, M_SPK_R, M_SPK_O, M_JC0, M_JC1, M_JC2, M_JC3, M_JC4, M_TC0, M_TC1, M_TC2, M_TC3, M_TC4, M_POST, M_HELI, M_ROTOR, M_HQ0, M_HQ1, M_EBUL, M_FIRE0, M_FIRE1, M_DEBRIS, M_BOX3D, M_CROSS, M_RINGG, M_RINGR, M_LINE, M_MAP, M_BARY, M_BARG, M_BARR };
static const uint32_t TEX_MAP = 0x7FFE1000u, TEX_FIRE0 = 0x7FFE1001u;
static M compose(const M &parent, const M &local) { M o; for (int k = 0; k < 3; k++) o.r[k] = to_world(parent, local.r[k]); return o; }

static void vt_poly(Mesh &m, const std::vector<V> &c, uint32_t rgb) {   /* the same record as Mesh::poly, from a vector */
    VPIX_MeshPoly q; q.flags = 0x20000000u | ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | (rgb >> 16); int n = 0;
    for (const V &x : c) { if (n >= 16) break; q.idx[n++] = m.vert(x); }
    q.n = (uint8_t)n; V nn = norm(cross(c[1] - c[0], c[2] - c[0])); q.nx = nn.x; q.ny = nn.y; q.nz = nn.z;
    m.p.push_back(q);
}
static void vt_out(Mesh &m, std::vector<V> c, uint32_t rgb, V centre) {   /* wound so the normal points away from 'centre' */
    V mid; for (const V &x : c) mid += x;
    mid = mid * (1.f / (float)c.size());
    V n = cross(c[1] - c[0], c[2] - c[0]);
    if (dot(n, mid - centre) < 0) std::reverse(c.begin(), c.end());
    vt_poly(m, c, rgb);
}
static void vt_flat(Mesh &m, float x0, float y0, float x1, float y1, float z, uint32_t rgb) {   /* a horizontal quad, up-facing, metres */
    vt_poly(m, {mp(x0, y0, z), mp(x1, y0, z), mp(x1, y1, z), mp(x0, y1, z)}, rgb);
}

/* a small jet, nose along +y, metres. harrier: nozzles, a little anhedral. sweep: how far the wing tip trails the root. */
/* a closed tube from a to b, millimetres: never paper thin */
static void vt_tube(Mesh &m, V a, V b, float ra, float rb, uint32_t col) {
    V ax = norm(b - a), u = fabsf(ax.z) > .9f ? V(1, 0, 0) : V(0, 0, 1), s1 = norm(cross(ax, u)), s2 = cross(ax, s1), c = (a + b) * .5f; const int N = 6; std::vector<V> ca, cb;
    for (int i = 0; i < N; i++) { float t = 2.f * PI_F * i / N, t2 = 2.f * PI_F * (i + 1) / N; V d1 = s1 * cosf(t) + s2 * sinf(t), d2 = s1 * cosf(t2) + s2 * sinf(t2);
        vt_out(m, {a + d1 * ra, a + d2 * ra, b + d2 * rb, b + d1 * rb}, col, c); ca.push_back(a + d1 * ra); cb.push_back(b + d1 * rb); }
    for (int i = 1; i + 1 < N; i++) { vt_out(m, {ca[0], ca[i], ca[i + 1]}, col, c); vt_out(m, {cb[0], cb[i], cb[i + 1]}, col, c); }
}
static void vt_jet(Mesh &m, float L, float S, uint32_t body, uint32_t belly, uint32_t wing, uint32_t trim, float sweep, bool harrier, uint32_t tail = 0, uint32_t canopy = 0x5A88C0, bool twin = false) {
    const V c = mp(0, 0, 0);
    const float yN = 0.5f * L, yA = 0.15f * L, yB = -0.30f * L, yT = -0.5f * L;
    const float w1 = 0.075f * L, t1 = 0.065f * L, b1 = 0.05f * L, w2 = 0.065f * L, t2 = 0.055f * L, b2 = 0.04f * L, w3 = 0.035f * L, t3 = 0.03f * L, b3 = 0.025f * L;
    V N = mp(0, yN, 0);
    V A[4] = {mp(0, yA, t1), mp(w1, yA, 0), mp(0, yA, -b1), mp(-w1, yA, 0)};
    V B[4] = {mp(0, yB, t2), mp(w2, yB, 0), mp(0, yB, -b2), mp(-w2, yB, 0)};
    V T[4] = {mp(0, yT, t3), mp(w3, yT, 0), mp(0, yT, -b3), mp(-w3, yT, 0)};
    for (int i = 0; i < 4; i++) { int j = (i + 1) & 3; uint32_t col = (i == 2) ? belly : body;
        if (harrier) { const float k = .5f, yM = yA + (yN - yA) * .5f; V Mi = mp(0, yM, 0) + (A[i] - mp(0, yA, 0)) * k, Mj = mp(0, yM, 0) + (A[j] - mp(0, yA, 0)) * k; vt_out(m, {A[i], A[j], Mj, Mi}, col, c); vt_out(m, {Mi, Mj, N}, 0x141418, c); }   /* the last sixth of the jet is a long dark point: Nz_BeaBS_cg t299.5, t311.5 */
        else vt_out(m, {N, A[i], A[j]}, col, c);
        vt_out(m, {A[i], A[j], B[j], B[i]}, col, c); vt_out(m, {B[i], B[j], T[j], T[i]}, col, c); }
    vt_out(m, {T[0], T[1], T[2], T[3]}, trim, c);
    { V P = mp(0, 0.04f * L, 0.105f * L), f = mp(0, 0.13f * L, 0.058f * L), r = mp(0.04f * L, 0.f, 0.062f * L), b = mp(0, -0.10f * L, 0.052f * L), l = mp(-0.04f * L, 0.f, 0.062f * L);
      V cc = mp(0, 0.f, 0.03f * L); uint32_t gl = canopy;
      vt_out(m, {P, f, r}, gl, cc); vt_out(m, {P, r, b}, gl, cc); vt_out(m, {P, b, l}, gl, cc); vt_out(m, {P, l, f}, gl, cc); }
    const V below = mp(0, 0, -10.f);
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f;
        float yf0 = 0.12f * L, yb0 = -0.30f * L, yf1 = yf0 - sweep * L, yb1 = yb0 - sweep * L * 0.8f, dz = harrier ? -0.02f * L : 0.f;
        d1k::kit_slab(m, {mp(q * w1, yf0, 0), mp(q * S, yf1, dz), mp(q * S, yb1, dz), mp(q * w2, yb0, 0)}, mp(0, 0, 0.02f * L) - mp(0, 0, 0), wing, d1k::kit_shade(wing, 0.7f));
        d1k::kit_slab(m, {mp(q * w3, -0.38f * L, 0), mp(q * S * 0.45f, -0.50f * L, 0), mp(q * S * 0.45f, -0.56f * L, 0), mp(q * w3, -0.50f * L, 0)}, mp(0, 0, 0.016f * L) - mp(0, 0, 0), tail ? tail : wing, d1k::kit_shade(tail ? tail : wing, 0.7f)); }
    if (twin) for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f, x0 = q * 0.045f * L, x1 = q * 0.095f * L;
        d1k::kit_slab(m, {mp(x0, -0.22f * L, t2 * 0.7f), mp(x0, -0.49f * L, t3 * 0.7f), mp(x1, -0.56f * L, 0.21f * L), mp(x1, -0.42f * L, 0.21f * L)}, mp(0.012f * L, 0, 0) - mp(0, 0, 0), trim, d1k::kit_shade(trim, 0.7f)); }
    else d1k::kit_slab(m, {mp(0, -0.26f * L, t2), mp(0, -0.49f * L, t3), mp(0, -0.55f * L, 0.20f * L), mp(0, -0.40f * L, 0.19f * L)}, mp(0.012f * L, 0, 0) - mp(0, 0, 0), trim, d1k::kit_shade(trim, 0.7f));
    if (harrier) for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; m.box(mp((std::min)(q * .065f * L, q * .105f * L), .02f * L, -.03f * L), mp((std::max)(q * .065f * L, q * .105f * L), .1f * L, .04f * L), 0x101014); }   /* the black intake mouths at the shoulders: Nz_BeaBS_cg t310.5 */
    if (harrier) vt_tube(m, mp(0, 0.46f * L, 0), mp(0, 0.68f * L, 0), 30.f, 10.f, 0x2A2A2E);   /* the refuelling needle out of the nose point: Bobby 11-03. Footage too soft to show or deny it */
    if (harrier) for (int k = 0; k < 4; k++) { float x = (k & 1) ? 0.085f * L : -0.085f * L, y = (k & 2) ? -0.15f * L : 0.08f * L;
        m.box(mp(x - 0.02f * L, y - 0.03f * L, -0.07f * L), mp(x + 0.02f * L, y + 0.03f * L, -0.025f * L), 0x2A2A2E); }
}

struct Mtn { float x, y, r, h; };   /* low green hills along the island's edge: the footage's horizon is a low band, the "mountains" of the speech are these */
static const Mtn MTN[14] = {{-300, 240, 95, 44}, {-190, 290, 110, 38}, {-70, 300, 105, 46}, {60, 295, 100, 40}, {145, 230, 70, 30}, {160, 110, 55, 20}, {160, 0, 50, 16}, {155, -120, 55, 20},
                            {-320, 110, 90, 40}, {-320, -30, 95, 44}, {-310, -160, 90, 36}, {-200, -205, 80, 28}, {-70, -210, 75, 24}, {60, -205, 80, 26}};
struct Box { float x0, y0, x1, y1, top; };
static std::vector<Box> obst;   /* fixed obstacles on the island */

/* the hills are low and rounded on the horizon, never peaks (BBC t263, t266, t297): a dome of four rings */
static const float HILL_R[5] = {1.f, .72f, .46f, .22f, 0.f}, HILL_Z[5] = {0.f, .5f, .82f, .97f, 1.f};
static void vt_mountain(Mesh &m, const Mtn &q) {
    const int N = 10;
    V cen = mp(q.x, q.y, q.h * 0.3f);
    for (int b = 0; b < 4; b++) for (int i = 0; i < N; i++) {
        float a0 = 2.f * PI_F * i / N, a1 = 2.f * PI_F * (i + 1) / N, r0 = q.r * HILL_R[b], r1 = q.r * HILL_R[b + 1], z0 = q.h * HILL_Z[b], z1 = q.h * HILL_Z[b + 1];
        V p0 = mp(q.x + cosf(a0) * r0, q.y + sinf(a0) * r0, z0), p1 = mp(q.x + cosf(a1) * r0, q.y + sinf(a1) * r0, z0), p2 = mp(q.x + cosf(a1) * r1, q.y + sinf(a1) * r1, z1), p3 = mp(q.x + cosf(a0) * r1, q.y + sinf(a0) * r1, z1);
        uint32_t c = (b == 0) ? ((i & 1) ? 0x3F7A38 : 0x437E3B) : (b == 1) ? ((i & 1) ? 0x3A7236 : 0x3E7639) : ((i & 1) ? 0x376A33 : 0x3A6E35);
        if (b == 3) vt_out(m, {p0, p1, p2}, c, cen); else vt_out(m, {p0, p1, p2, p3}, c, cen);
    }
}

static V bar_base[2], bar_top[2];   /* the side bars, aircraft-local mm */
static const float GUN_Y = 5000.f, BOX_ZT = 2433.f, BOX_ZB = 1500.f, CROSS_Z = 2060.f;
static void vt_build_assets() {
    obst.clear();
    /* the sea: 12 x 12 tiles of 120 m, two blues, so it reads as moving under you */
    { Mesh m; const float T = 120.f; for (int i = 0; i < 12; i++) for (int j = 0; j < 12; j++) { float x = -720.f + i * T, y = -720.f + j * T;
          vt_flat(m, x, y, x + T, y + T, SEA_Z, ((i + j) & 1) ? 0x1E86DC : 0x1C82D8); }
      m.add(M_SEA); }
    /* the island: grass, field patches, runway, apron, tower, hangar, mountains */
    { Mesh m;
      m.box(mp(LAND_X0, LAND_Y0, -12), mp(LAND_X1, LAND_Y1, 0), 0x4A8E3E, 0x85724E, 0x40382A);
      std::mt19937 r2(11); auto rr = [&](float a, float b) { return a + (b - a) * (r2() / (float)(r2.max)()); };
      const uint32_t fc[5] = {0x4E9240, 0x468A3A, 0x529642, 0x427F38, 0x4C8E3E};
      for (int i = 0; i < 46; i++) {
          float w = rr(18, 44), d = rr(18, 44), x = rr(LAND_X0 + 10, LAND_X1 - w - 10), y = rr(LAND_Y0 + 10, LAND_Y1 - d - 10); bool bad = false;
          if (x < 90 && x + w > -90 && y < 175 && y + d > -150) bad = true;
          if (x + w > -150 && x < -65 && y + d > 20 && y < 90) bad = true;
          for (const Mtn &q : MTN) if (x + w > q.x - q.r - 5 && x < q.x + q.r + 5 && y + d > q.y - q.r - 5 && y < q.y + q.r + 5) bad = true;
          if (!bad) vt_flat(m, x, y, x + w, y + d, 0.3f, fc[i % 5]);
      }
      vt_flat(m, -20, -122, 20, 162, 0.4f, 0x96968E);                                    /* the runway */
      vt_flat(m, -84, -140, -18, -40, 0.4f, 0x96968E); vt_flat(m, 18, -140, 84, -40, 0.4f, 0x96968E);   /* aprons either side of the start */
      vt_flat(m, -19.6f, -120, -18.0f, 160, 0.8f, 0xE8E8E8); vt_flat(m, 18.0f, -120, 19.6f, 160, 0.8f, 0xE8E8E8);   /* the broad white edge line */
      for (float y = -100.f; y < 150.f; y += 24.f) vt_flat(m, -0.9f, y, 0.9f, y + 13.f, 0.8f, 0xF0F0F0);   /* white dashes (footage) */
      for (int k = 0; k < 8; k++) { float x = -14.f + k * 4.f; vt_flat(m, x, -117, x + 1.6f, -107, 0.8f, 0xF0F0F0); vt_flat(m, x, 151, x + 1.6f, 161, 0.8f, 0xF0F0F0); }
      /* a second strip crossing the runway at a slant (publicity t1145) */
      vt_poly(m, {mp(-70, 40, 0.5f), mp(-58, 28, 0.5f), mp(70, 96, 0.5f), mp(58, 108, 0.5f)}, 0x96968E);
      for (int k = 0; k < 6; k++) { float t = (k + .5f) / 6.f, xa = -64.f + 128.f * t, ya = 34.f + 68.f * t; vt_poly(m, {mp(xa - 4, ya - 2.8f, 0.8f), mp(xa + 4, ya + 1.4f, 0.8f), mp(xa + 3.5f, ya + 2.3f, 0.8f), mp(xa - 4.5f, ya - 1.9f, 0.8f)}, 0xF0F0F0); }
      /* the control tower: a thin dark shaft with a pale cap, behind a long low dark building with an olive roof (TV report t518) */
      m.box(mp(-64.5f, -59.5f, 0), mp(-61.5f, -56.5f, 22), 0x5C2C24, 0x4A241E, 0x4A241E);   /* brown, on a low brown plinth: the BBC original t263 and publicity t1144.5; the darker re-encode made it look black */
      m.box(mp(-65.2f, -60.2f, 22), mp(-60.8f, -55.8f, 25), 0xB4B4AC, 0x8E8E86, 0x5A2618);
      m.box(mp(-67.f, -62.f, 0), mp(-59.f, -54.f, 1.2f), 0x5C2C24, 0x4A241E, 0x4A241E);
      m.box(mp(-101, -96, 0), mp(-90, -54, 7.5f), 0x8A8A4E, 0x6A2A20, 0x6A2A20);
      obst.push_back({-65.5f, -60.5f, -60.5f, -55.5f, 25.f}); obst.push_back({-101.f, -96.f, -90.f, -54.f, 7.5f});
      /* slate-blue hangars with a dark gable roof, a red shed with a grey roof (TV report t539, t547) */
      auto hangar = [&](float x0, float y0, float x1, float y1, float hh) { m.box(mp(x0, y0, 0), mp(x1, y1, hh), 0x6C6A9A, 0x6C6A9A, 0x303040); float xm = (x0 + x1) * 0.5f, rg = hh * 1.45f;
          vt_out(m, {mp(x0, y0, hh), mp(xm, y0, rg), mp(xm, y1, rg), mp(x0, y1, hh)}, 0x20222E, mp(xm, (y0 + y1) * 0.5f, hh)); vt_out(m, {mp(x1, y0, hh), mp(xm, y0, rg), mp(xm, y1, rg), mp(x1, y1, hh)}, 0x20222E, mp(xm, (y0 + y1) * 0.5f, hh));
          vt_out(m, {mp(x0, y0, hh), mp(x1, y0, hh), mp(xm, y0, rg)}, 0x6C6A9A, mp(xm, y1, hh)); vt_out(m, {mp(x0, y1, hh), mp(x1, y1, hh), mp(xm, y1, rg)}, 0x6C6A9A, mp(xm, y0, hh)); obst.push_back({x0, y0, x1, y1, rg}); };
      hangar(56, -112, 74, -96, 7.5f); hangar(-70, -122, -50, -100, 7.5f);
      m.box(mp(47, -75, 0), mp(58, -69, 3), 0x8C8C86, 0xC82018, 0x501810); obst.push_back({47.f, -75.f, 58.f, -69.f, 3.f});
      /* low dark sheds with red and white stripes by the far runway (publicity clip t1144) */
      for (int k = 0; k < 3; k++) { float y = 30.f + k * 26.f; m.box(mp(-40, y, 0), mp(-30, y + 14, 3), 0x28282C, 0x28282C, 0x202024); vt_flat(m, -39, y + 2, -31, y + 5, 3.2f, 0xC82018); vt_flat(m, -39, y + 7, -31, y + 10, 3.2f, 0xE8E8E8); obst.push_back({-40.f, y, -30.f, y + 14, 3.f}); }
      obst.push_back({-42.f, -98.f, -30.f, -86.f, 4.f});   /* the parked blue jet */
      obst.push_back({32.f, -86.f, 48.f, -72.f, 4.f});     /* the parked red jet */
      for (const Mtn &q : MTN) vt_mountain(m, q);
      m.add(M_LAND); }
    /* the carrier: local z is absolute (deck at 10 m), pale grey as on the TV report t599 */
    { Mesh m;
      m.box(mp(-12, -60, -8), mp(12, 60, 8.5f), 0xA0A094, 0x94948A, 0x34383C);
      m.box(mp(-15, -58, 8.5f), mp(15, 58, 10), 0xA8A89C, 0x94948A, 0x34383C);
      vt_poly(m, {mp(-12, 60, 10), mp(12, 60, 10), mp(0, 78, 10)}, 0xA8A89C);
      vt_out(m, {mp(-12, 60, -8), mp(-12, 60, 8.5f), mp(0, 78, 8.5f), mp(0, 78, -8)}, 0x8C8C86, mp(0, 40, 0));
      vt_out(m, {mp(12, 60, -8), mp(12, 60, 8.5f), mp(0, 78, 8.5f), mp(0, 78, -8)}, 0x8C8C86, mp(0, 40, 0));
      vt_flat(m, -5.5f, -56, -4.5f, 74, 10.35f, 0xF0F0F0); vt_flat(m, 1.5f, -56, 2.5f, 74, 10.35f, 0xF0F0F0);   /* two long white lines and a few across at the bow, no yellow (BBC t351) */
      for (int k = 0; k < 3; k++) vt_flat(m, -8.f, 46.f + k * 7.f, 5.f, 47.f + k * 7.f, 10.35f, 0xF0F0F0);
      m.box(mp(9, 10, 10), mp(14.5f, 26, 18), 0xB0B6BC, 0x9AA0A6, 0x9AA0A6);
      m.box(mp(8.5f, 13, 18), mp(14.8f, 23, 22), 0xC0C6CC, 0x8A9096, 0x8A9096);
      m.box(mp(11.3f, 16, 22), mp(11.7f, 16.4f, 31), 0x303236);
      m.add(M_CARRIER); }
    /* the frigate: local z 0 = waterline, origin amidships, bow +y */
    { Mesh m;
      m.box(mp(-4.5f, -28, -5), mp(4.5f, 22, 3), 0x5A6068, 0x7C838A, 0x34383C);
      vt_poly(m, {mp(-4.5f, 22, 3), mp(4.5f, 22, 3), mp(0, 32, 3)}, 0x5A6068);
      vt_out(m, {mp(-4.5f, 22, -5), mp(-4.5f, 22, 3), mp(0, 32, 3), mp(0, 32, -5)}, 0x7C838A, mp(0, 0, 0));
      vt_out(m, {mp(4.5f, 22, -5), mp(4.5f, 22, 3), mp(0, 32, 3), mp(0, 32, -5)}, 0x7C838A, mp(0, 0, 0));
      m.box(mp(-3, -8, 3), mp(3, 10, 9), 0xD0D4D8, 0xB4BAC0, 0x8A9096);
      m.box(mp(-2.5f, 2, 9), mp(2.5f, 9, 13), 0xC8CCD0, 0xA8AEB4, 0x8A9096);
      m.box(mp(-1.2f, -6, 9), mp(1.2f, -3.5f, 13), 0x303236);
      m.box(mp(-0.2f, 5, 13), mp(0.2f, 5.4f, 20), 0x303236);
      m.box(mp(-1.5f, 14, 3), mp(1.5f, 18, 5.5f), 0x7C838A);
      m.box(mp(-0.2f, 18, 4.2f), mp(0.2f, 23, 4.8f), 0x303236);
      vt_flat(m, -3.5f, -26, 3.5f, -12, 3.3f, 0x4C5258);
      m.add(M_FRIGATE); }
    /* aircraft */
    { Mesh m; vt_jet(m, 14.f, 4.6f, 0x8A9096, 0x60646A, 0x7A8086, 0x3C3C40, 0.09f, true); m.add(M_HARRIER); }
    /* the enemy: grey long-nosed jet, swept grey wings, red twin fins and tailplanes, a dark engine band with an orange ring, yellow-and-black missiles on the wings (Octavius t1121-t1125) */
    { Mesh m; const float L = 11.f; vt_jet(m, L, 4.6f, 0x8E8E84, 0x6A6A62, 0x868680, 0xE03A18, 0.20f, false, 0xE03A18, 0x3A3A40, true);
      m.box(mp(-0.075f * L, -0.42f * L, -0.035f * L), mp(0.075f * L, -0.34f * L, 0.06f * L), 0x18181C); m.box(mp(-0.05f * L, -0.53f * L, -0.03f * L), mp(0.05f * L, -0.50f * L, 0.03f * L), 0xF08020);
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; for (int t = 0; t < 4; t++) { float y0 = -0.26f * L + t * 0.08f * L; m.box(mp(q * 0.34f * L - 0.02f * L, y0, -0.05f * L), mp(q * 0.34f * L + 0.02f * L, y0 + 0.08f * L, -0.015f * L), (t & 1) ? 0x18181C : 0xE8C020); } }
      m.add(M_ENEMY); }
    { Mesh m; vt_jet(m, 14.f, 4.6f, 0x1830D8, 0x1020A0, 0x1830D8, 0x182450, 0.09f, true, 0, 0xE0E8F0); m.add(M_JETB); }   /* the parked blue jet (TV report t518) */
    { Mesh m; vt_jet(m, 14.f, 5.f, 0xE82438, 0xB01C2C, 0xF03048, 0xD01C30, 0.14f, true, 0, 0xE0E8F0); m.add(M_JETR); }  /* the red pod's Harrier, bright pink-red, tall fin (BBC t290-t301: it lifts off and flies) */
    { Mesh m; vt_jet(m, 14.f, 5.f, 0x30E040, 0x20A030, 0x38E848, 0x20C030, 0.14f, true, 0, 0xE0E8F0); m.add(M_JETG); }  /* "the green plane" (BBC t285, t351) */
    /* the pair of pale posts with black tops at the runway's far end (BBC t291-t297; what they are is unknown) */
    { Mesh m; for (int k = 0; k < 2; k++) { float x = k * 2.6f; m.box(mp(x - .5f, -.5f, 0), mp(x + .5f, .5f, 3.2f), 0xC8C8C0, 0xA8A8A0, 0x606058); m.box(mp(x - .45f, -.45f, 3.2f), mp(x + .45f, .45f, 4.4f), 0x18181C); } m.add(M_POST); }
    { Mesh m; const uint32_t K = 0x24282E, D = 0x18191E; m.box(mp(-1.2f, -2.5f, -1.3f), mp(1.2f, 3.2f, 1.2f), K, D, D); m.box(mp(-.4f, -9, -.1f), mp(.4f, -2.5f, .7f), K, D, D); m.box(mp(-.15f, -9.6f, -.2f), mp(.15f, -8.6f, 2.4f), K, D, D); m.box(mp(-2, -7.4f, .1f), mp(2, -6.6f, .4f), K, D, D);
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; m.box(mp(q * 1.6f - .12f, -2.6f, -2.2f), mp(q * 1.6f + .12f, 3, -1.9f), D); } m.box(mp(-.25f, -.25f, 1.2f), mp(.25f, .25f, 2.2f), D); m.add(M_HELI); }
    { Mesh m; m.box(mp(-7, -.3f, 2.2f), mp(7, .3f, 2.4f), 0x18191E); m.box(mp(-.3f, -7, 2.2f), mp(.3f, 7, 2.4f), 0x18191E); m.add(M_ROTOR); }
    /* the tanker: C-130 (Computer+Videogiochi 9). Look INVENTED, none in footage: dark grey, high straight wing, four engines, tall fin, a hose and drogue behind */
    { Mesh m; const uint32_t G = 0x60666C, D = 0x484C52, K = 0x2A2C30; m.box(mp(-2, -11, -2), mp(2, 13, 2), G, D, D); vt_tube(m, mp(0, 13, -.1f), mp(0, 15.5f, -.3f), 1950.f, 1500.f, G); vt_tube(m, mp(0, 15.5f, -.3f), mp(0, 18.5f, -.7f), 1500.f, 350.f, D); m.box(mp(-1.2f, -18, -.2f), mp(1.2f, -11, 1.9f), D, D, K);
      m.box(mp(-20, 0, 2), mp(20, 5, 2.7f), G, D, D); m.box(mp(-7, -18, .6f), mp(7, -15, 1.1f), G, D, D); m.box(mp(-.35f, -18.5f, 1.9f), mp(.35f, -13, 8), D, K, K);
      const float ex[4] = {-13.f, -7.f, 7.f, 13.f}; for (float x : ex) m.box(mp(x - .9f, 1, .9f), mp(x + .9f, 8, 2.3f), D, K, K);
      m.box(mp(-.6f, -12.5f, -2.3f), mp(.6f, -10, -1.2f), D, K, K); m.add(M_TANKER); }   /* the hose unit on the cargo ramp, as on the RAF's 1982 Hercules tankers (real-world reference, not game evidence) */
    { Mesh m; vt_tube(m, V(0, 0, 0), V(0, 1000, 0), 90.f, 90.f, 0x18181C); m.add(M_HOSE); }   /* one metre of line, stretched per frame as it reels out */
    /* the drogue: "an empty cup on a tether" (Bobby 11-03). Open toward you, walled, deep, closed only at the throat where the hose joins. Origin = the centre of the mouth, +y toward the tanker. Shape from real baskets ("shuttlecock"); mouth radius 1.3 m is INVENTED, real ones are far smaller */
    { Mesh m; const int N = 12; const float RO = CUP_R, RI = CUP_R - .1f, D1 = .9f, D2 = 1.7f;
      auto P = [](float r, float t, float y) { return mp(r * cosf(t), y, r * sinf(t)); };
      for (int i = 0; i < N; i++) { float a0 = 2.f * PI_F * i / N, a1 = 2.f * PI_F * (i + 1) / N, am = (a0 + a1) * .5f; uint32_t rim = (i & 1) ? 0xF0F0E8 : 0xC82018;
          vt_out(m, {P(RO, a0, 0), P(RO, a1, 0), P(.2f, a1, 1.85f), P(.2f, a0, 1.85f)}, (i & 1) ? 0xC8C8C0 : 0xA8A8A0, mp(0, .8f, 0));      /* outside wall */
          vt_out(m, {P(RO, a0, 0), P(RO, a1, 0), P(RI, a1, -.02f), P(RI, a0, -.02f)}, rim, mp(0, 2, 0));                                     /* the rim, facing you */
          V out = P(RO * 3.f, am, 0);                                                                                                        /* the inside walls face the axis: dark, darker further in */
          vt_out(m, {P(RI, a0, -.02f), P(RI, a1, -.02f), P(.62f, a1, D1), P(.62f, a0, D1)}, (i & 1) ? 0x62625E : 0x565652, out);
          vt_out(m, {P(.62f, a0, D1), P(.62f, a1, D1), P(.14f, a1, D2), P(.14f, a0, D2)}, 0x262628, P(RO * 3.f, am, D1));
          vt_out(m, {mp(0, D2, 0), P(.14f, a0, D2), P(.14f, a1, D2)}, 0x0C0C0E, mp(0, 5, 0)); }
      m.add(M_DROGUE); }
    /* the needle as you see it down the nose, only while refuelling: every forward view in the footage shows no needle, and real probes are often retractable (real-world reference). Tip at TIP_Y, TIP_Z */
    { Mesh m; vt_tube(m, V(0, 4300, 1845), V(0, 6200, 1705), 125.f, 75.f, 0x8A9096); vt_tube(m, V(0, 6200, 1705), V(0, 7000, 1610), 75.f, 22.f, 0x141418);   /* your own nose section ahead of the hood, the same line as M_HARRIER's nose: Bobby 11-03, "make a nose section". Solid, closed; the shape is ours */
      vt_tube(m, V(0, 6950, 1612), V(0, TIP_Y * 1000.f, TIP_Z * 1000.f), 10.f, 8.f, 0x3A3A40); m.add(M_NEEDLE); }
    /* the two pod joysticks, one at each hand on the arm rests: a black ball grip with a red top button (BBC material in Nz_BeaBS_cg t309, both hands at the sides t319). They move with the stick input, never with the hand's motion: Bobby 11-03 */
    for (int k = 0; k < 2; k++) { Mesh m; m.box(V(-14, -14, 0), V(14, 14, 150), 0x2A2E34); m.box(V(-40, -40, 150), V(40, 40, 255), 0x101012); m.box(V(-30, -30, 255), V(30, 30, 275), 0x181818); m.box(V(-16, -16, 275), V(16, 16, 288), 0xD02018); m.add(k ? M_STICKR : M_STICKL); }
    { Mesh m; m.box(mp(-.25f, -12.7f, -2.0f), mp(-.02f, -12.5f, -1.75f), 0xF0A020); m.add(M_LAMPA); }   /* real units show amber ready, green when fuel flows (real-world reference); here because nothing else tells you */
    { Mesh m; m.box(mp(.02f, -12.7f, -2.0f), mp(.25f, -12.5f, -1.75f), 0x30F040); m.add(M_LAMPG); }
    { Mesh m; m.box(mp(-.22f, -.22f, -.22f), mp(.22f, .22f, .22f), 0xE03018); m.add(M_SPK_R); }
    { Mesh m; m.box(mp(-.22f, -.22f, -.22f), mp(.22f, .22f, .22f), 0xF09020); m.add(M_SPK_O); }
    /* the pieces, the same idea as the Dactyl break_up: closed chunks that fly apart with sparks. Each around its own centre */
    { const uint32_t B = 0x8A9096, BL = 0x60646A, W = 0x7A8086, TR = 0x3C3C40;
      { Mesh m; vt_tube(m, mp(0, -2, 0), mp(0, 3, 0), 950.f, 60.f, B); m.add(M_JC0); }                                              /* nose */
      { Mesh m; m.box(mp(-1, -3, -.7f), mp(1, 3, .9f), B, BL, BL); m.add(M_JC1); }                                                   /* middle */
      { Mesh m; m.box(mp(-.5f, -2, -.4f), mp(.5f, 2, .4f), B, BL, BL); m.box(mp(-.1f, -2, .4f), mp(.1f, -.4f, 2.8f), TR); m.add(M_JC2); }   /* tail and fin */
      { Mesh m; m.box(mp(-1.9f, -1.6f, -.14f), mp(1.9f, 1.6f, .14f), W, TR, TR); m.add(M_JC3); }                                     /* a wing */
      { Mesh m; m.box(mp(-1.6f, -1.3f, -.14f), mp(1.6f, 1.3f, .14f), W, TR, TR); m.add(M_JC4); } }
    { const uint32_t G = 0x60666C, D = 0x484C52, K = 0x2A2C30;
      { Mesh m; m.box(mp(-2, -7, -2), mp(2, 4, 2), G, D, D); vt_tube(m, mp(0, 4, -.1f), mp(0, 6.5f, -.3f), 1950.f, 1500.f, G); vt_tube(m, mp(0, 6.5f, -.3f), mp(0, 9.5f, -.7f), 1500.f, 350.f, D); m.add(M_TC0); }   /* front fuselage */
      { Mesh m; m.box(mp(-2, -7, -2), mp(2, 7, 2), G, D, D); m.box(mp(-.6f, -6.5f, -2.3f), mp(.6f, -4, -1.2f), D, K, K); m.add(M_TC1); }   /* back fuselage */
      { Mesh m; m.box(mp(-1.2f, -4, -1), mp(1.2f, 3, 1), D, D, K); m.box(mp(-7, -4, -.3f), mp(7, -1, .3f), G, D, D); m.box(mp(-.35f, -4.5f, .9f), mp(.35f, 1, 7), D, K, K); m.add(M_TC2); }   /* tail */
      for (int k = 0; k < 2; k++) { Mesh m; m.box(mp(-9, -2.5f, -.35f), mp(9, 2.5f, .35f), G, D, D); for (float x : {-3.f, 3.f}) m.box(mp(x - .9f, -1.5f, -1.4f), mp(x + .9f, 5.5f, 0), D, K, K); m.add(k ? M_TC4 : M_TC3); } }
    /* the enemy's islet: a flat green disc with a shore, closed down to the sea. INVENTED */
    { Mesh m; const int N = 20; const V ctr = mp(ISLE_X, ISLE_Y, -8.f); for (int i = 0; i < N; i++) { float a0 = 2.f * PI_F * i / N, a1 = 2.f * PI_F * (i + 1) / N; auto Q = [&](float r, float a, float z) { return mp(ISLE_X + r * cosf(a), ISLE_Y + r * sinf(a), z); };
          vt_out(m, {mp(ISLE_X, ISLE_Y, 0), Q(ISLE_R, a0, 0), Q(ISLE_R, a1, 0)}, (i & 1) ? 0x4A8551 : 0x47814D, ctr); vt_out(m, {Q(ISLE_R, a0, 0), Q(ISLE_R, a1, 0), Q(ISLE_R + 7.f, a1, SEA_Z), Q(ISLE_R + 7.f, a0, SEA_Z)}, 0xB8B088, ctr); }
      m.add(M_ISLE); }
    /* the enemy HQ: "low-level bombing of an enemy HQ" (The One 33). Look INVENTED; five dark blocks so the n/5 counter still counts */
    { Mesh m; m.box(mp(-7, -5, 0), mp(7, 5, 6), 0x4C5238, 0x383C2A, 0x20241A); m.add(M_HQ0); }
    { Mesh m; m.box(mp(-4, -4, 0), mp(4, 4, 11), 0x5A3028, 0x46241E, 0x20241A); m.box(mp(-.2f, -.2f, 11), mp(.2f, .2f, 18), 0x18181C); m.add(M_HQ1); }
    /* the canopy frame, aircraft-local mm; the eye is at (0, 3000, 2200) */
    { Mesh m; const uint32_t G1 = 0x8E8E86, G2 = 0x6A6A64;
      auto PT = [](float tx, float tv, float d) { return V(tx * d, 3000.f + d, 2200.f + tv * d); };   /* a direction measured on screen (ndc x * .70, ndc y * .525) at depth d in front of the eye */
      const uint32_t HP = 0xC4C6BE, HPI = 0x8E9288, SL = 0x74769E, SLD = 0x54567A, BK = 0x121216;
      /* the canopy hoop: an arch across the top of the picture, coming nearer as it comes down (Octavius t1118, publicity t1145) */
      { struct K { float nx, ny, d; }; static const K H[7] = {{0.f, .62f, 1500}, {.25f, .60f, 1450}, {.5f, .44f, 1220}, {.75f, .18f, 960}, {1.f, -.13f, 800}, {1.3f, -.56f, 650}, {1.6f, -1.1f, 520}};
        std::vector<V> O, IN_; for (int s = -6; s <= 6; s++) { const K &k = H[abs(s)]; V q = PT((s < 0 ? -1.f : 1.f) * k.nx * .7f, k.ny * .525f, k.d); V n = V(0, q.y, 1950.f) - q; n.y = 0; O.push_back(q); IN_.push_back(q + norm(n) * 112.f); }
        for (size_t i = 0; i + 1 < O.size(); i++) { V f1(0, 80, 0); m.poly({O[i], O[i + 1], IN_[i + 1], IN_[i]}, HP); m.poly({IN_[i], IN_[i + 1], IN_[i + 1] + f1, IN_[i] + f1}, HPI); } }
      /* two struts from the hoop down to the far corners of the hood */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; V a0 = PT(q * .256f, .44f * .525f, 1480), b0 = PT(q * .20f, -.19f, 1300);   /* outer edge .256 to .20, inner .175 to .15 of the tangent */
          V a1 = PT(q * .175f, .44f * .525f, 1480), b1 = PT(q * .15f, -.19f, 1300), f1(0, 55, 0);
          m.poly({a0, a1, b1, b0}, HP); m.poly({a1, a1 + f1, b1 + f1, b1}, HPI); }
      m.box(V(-264, 4295, 1890), V(264, 4322, 1974), SL);   /* the lip at the hood's far edge the strut feet sink into: the gap scan found a crack there */
      /* the hood over the map screen: top plate, dark brow, side wings, sill (Octavius t1124, t1134) */
      { V NCr = PT(.29f, -.32f, 900), NCl = PT(-.29f, -.32f, 900), FCr = PT(.196f, -.175f, 1300), FCl = PT(-.196f, -.175f, 1300);
        V SRt(170, 3990, 1842), SLt(-170, 3990, 1842), SRb(170, 3990, 1480), SLb(-170, 3990, 1480), BCr(200, 4000, 1440), BCl(-200, 4000, 1440);
        m.poly({NCl, NCr, FCr, FCl}, SL); m.poly({NCl, NCr, SRt, SLt}, BK); m.poly({NCr, SRt, SRb, BCr}, SL); m.poly({NCl, SLt, SLb, BCl}, SL); m.poly({SLb, SRb, BCr, BCl}, BK);
        m.box(V(-258, 3996, 1300), V(258, 4060, 1900), BK);   /* a solid front cover behind the screen: Bobby 11-03 saw the outside through a line round it */
        m.poly({NCr, FCr, V(FCr.x + 20, FCr.y - 40, 1700)}, SLD); m.poly({NCl, FCl, V(FCl.x - 20, FCl.y - 40, 1700)}, SLD);
        m.box(V(-210, 3950, 1330), V(210, 4000, 1440), BK); m.box(V(-120, 3944, 1380), V(-40, 3950, 1420), 0xE8E8E8); m.box(V(20, 3944, 1380), V(100, 3950, 1420), 0x4858C0);
        /* the side panels: from the hood out to a point near the screen edge; black below them. The bar boxes lie on these (Octavius t1124, t1120.8) */
        for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; V E1 = k ? NCr : NCl, E2 = PT(q * .87f, -.83f, 460), E3 = PT(q * .30f, -1.05f, 620), BC = k ? BCr : BCl;
            m.poly({E1, E2, E3}, SL); m.poly({E1, E3, BC}, SL); m.poly({E2, E3, V(E3.x, E3.y, 950), V(q * 800, 3350, 950)}, BK); m.poly({E3, BC, V(BC.x, BC.y, 950), V(E3.x, E3.y, 950)}, BK);
            /* the side body (Bobby 11-02): on screen the panel's top edge runs straight on from the hood corner through E2 to the hoop's foot (Octavius t1122, t1130, publicity t1145), and the foot stands on slate (TV report t547).
               Past that the footage never looks; the sill plate back to the wall, the shoulder over it and the dark wall under the edge are what closes it up */
            { V F = PT(q * 1.12f, -.5775f, 520), W0(q * 760, 3300, 1720), W1(q * 800, 3300, 1720), W2(q * 800, 2150, 1720), W3(q * 760, 2150, 1720), C0(q * 1020, 3300, 1500), C1(q * 1050, 2150, 1480), C2(q * 700, 3700, 1250);   /* 1:1 pass: the sill is 31 degrees under the eye: the BBC original t263, t266 and t291 show open ground to the picture's foot at 35-70 degrees to the side */
              const uint32_t WL = 0x24242E; m.poly({E1, F, E2}, SL); m.poly({F, W0, W1}, SLD); m.poly({F, W1, C0}, SL); m.poly({W1, C0, C1, W2}, SL); m.poly({F, C0, C2}, SLD); m.poly({W0, W1, W2, W3}, SLD);
              m.poly({E2, F, W0, V(q * 760, 3300, 1500)}, WL); m.poly({E2, V(q * 760, 3300, 1500), V(q * 800, 3350, 950)}, WL); }
            V N = cross(E2 - E1, E3 - E1), EY(0, 3000, 2200); const float h = dot(N, E1 - EY);
            auto ON = [&](float tx, float tv, float lift) { V d(q * tx, 1, tv); float t = h / dot(N, d) * (1.f - lift); return EY + d * t; };
            m.poly({ON(.33f, -.50f, .004f), ON(.40f, -.50f, .004f), ON(.42f, -.86f, .004f), ON(.34f, -.86f, .004f)}, BK);
            m.poly({ON(.345f, -.52f, .008f), ON(.385f, -.52f, .008f), ON(.40f, -.84f, .008f), ON(.355f, -.84f, .008f)}, k ? 0x8088C4 : 0x7C7050);
            bar_base[k] = (ON(.355f, -.84f, .012f) + ON(.40f, -.84f, .012f)) * 0.5f; bar_top[k] = (ON(.345f, -.52f, .012f) + ON(.385f, -.52f, .012f)) * 0.5f; } }
      m.box(V(-800, 3350, 1290), V(800, 4000, 1340), BK); m.box(V(-800, 3985, 1340), V(800, 4000, 1440), BK);   /* the well under the hood is closed: no ground to be seen past the panel top */
      m.box(V(-760, 2200, 2750), V(-700, 3000, 2800), G1, G2, G2); m.box(V(700, 2200, 2750), V(760, 3000, 2800), G1, G2, G2);   /* the rails behind the eye */
      m.box(V(-760, 2150, 1700), V(-700, 2200, 3000), G1, G2, G2); m.box(V(700, 2150, 1700), V(760, 2200, 3000), G1, G2, G2);
      m.box(V(-760, 2150, 2950), V(760, 2200, 3000), G1, G2, G2);
      /* the rest of the Harrier: nose, tub, seat, back end, wings, intakes and nozzles, the shape of M_HARRIER with the eye inside it (Bobby 2026-11-02). INVENTED: footage only looks forward */
      { const uint32_t B = 0x8A9096, BL = 0x60646A, W = 0x7A8086, TR = 0x3C3C40, DK = 0x202024;
        struct Sec { V t, r, b, l; }; auto sec = [](float y, float tp, float w, float bt, float zc) { Sec q = {V(0, y, tp), V(w, y, zc), V(0, y, bt), V(-w, y, zc)}; return q; };
        auto join = [&](const Sec &a, const Sec &c) { m.poly({a.t, a.r, c.r, c.t}, B); m.poly({a.t, a.l, c.l, c.t}, B); m.poly({a.b, a.r, c.r, c.b}, BL); m.poly({a.b, a.l, c.l, c.b}, BL); };
        Sec s0 = sec(4350, 1650, 240, 950, 1400), s2 = sec(2150, 2350, 1050, 900, 1600), s3 = sec(-4200, 2300, 910, 1040, 1600), s4 = sec(-7000, 2020, 490, 1250, 1600);
        V tip(0, 6500, 1300); m.poly({tip, s0.t, s0.r}, B); m.poly({tip, s0.t, s0.l}, B); m.poly({tip, s0.b, s0.r}, BL); m.poly({tip, s0.b, s0.l}, BL);
        join(s2, s3); join(s3, s4); m.poly({s4.t, s4.r, s4.b, s4.l}, TR); m.poly({s2.t, s2.r, s2.b, s2.l}, DK);
        m.box(V(-800, 2150, 950), V(-760, 3350, 1700), TR); m.box(V(760, 2150, 950), V(800, 3350, 1700), TR); m.box(V(-760, 2150, 1300), V(760, 3350, 1340), DK);   /* the tub */
        /* the cockpit proper: ejector seat, side consoles, stick and throttle, panel with screens, avionics behind the seat (Bobby 2026-11-02). INVENTED */
        { using d1k::kit_quad; using d1k::kit_stripes; using d1k::kit_lamps; using d1k::kit_dial; d1k::kit_eye = V(0, 3000, 2200); Mesh l; const uint32_t S = 0x3A3F46, S2 = 0x2A2E34, YL = 0xE8C020, BK = 0x18181C, PN = 0x30343A, OL = 0x4A5238;
          m.box(V(-250, 2380, 1340), V(250, 2470, 2000), S, S2, S2); m.box(V(-140, 2370, 2000), V(140, 2460, 2190), S, S2, S2); m.box(V(-260, 2470, 1340), V(260, 2860, 1580), OL, S2, S2);   /* seat */
          kit_stripes(m, V(-250, 2472, 2005), V(500, 0, 0), V(0, 0, 40), 8, YL, BK); m.box(V(-90, 2430, 2190), V(-65, 2470, 2290), YL); m.box(V(65, 2430, 2190), V(90, 2470, 2290), YL); m.box(V(-90, 2430, 2270), V(90, 2470, 2295), YL);   /* the ejection handle */
          kit_quad(m, V(-180, 2472, 1590), V(60, 0, 0), V(0, 0, 410), 0xC87820); kit_quad(m, V(120, 2472, 1590), V(60, 0, 0), V(0, 0, 410), 0xC87820);   /* straps */
          for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; auto bx = [&](float x0, float y0, float z0, float x1, float y1, float z1, uint32_t c, uint32_t c2) { m.box(V((std::min)(q * x0, q * x1), y0, z0), V((std::max)(q * x0, q * x1), y1, z1), c, c2, c2); };
              bx(450, 2200, 1340, 760, 3340, 1660, PN, S2); bx(760, 2150, 1660, 800, 3300, 1720, 0x3C3C42, S2);        /* console, sill */
              kit_quad(l, V(q * 490, 2300, 1662), V(q * 230, 0, 0), V(0, 340, 0), 0x18A060); kit_lamps(l, V(q * 490, 2700, 1662), V(0, 80, 0), V(q * 60, 0, 0), V(0, 45, 0), 7); kit_lamps(l, V(q * 620, 2700, 1662), V(0, 80, 0), V(q * 60, 0, 0), V(0, 45, 0), 7);
              bx(300, 2160, 1700, 720, 2195, 2200, PN, S2); kit_lamps(l, V(q * 340, 2200, 2110), V(q * 75, 0, 0), V(q * 45, 0, 0), V(0, 0, 45), 5); kit_lamps(l, V(q * 340, 2200, 2020), V(q * 75, 0, 0), V(q * 45, 0, 0), V(0, 0, 45), 5); }   /* avionics behind the seat */
          for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; m.box(V(q * 540 - 70, 2880, 1660), V(q * 540 + 70, 3020, 1680), BK); }   /* the bases of the two sticks; the sticks move: M_STICKL, M_STICKR */
          m.box(V(-750, 3335, 1340), V(750, 3350, 1660), S2);                                                         /* panel */
          kit_quad(l, V(-690, 3332, 1400), V(360, 0, 0), V(0, 0, 220), 0x18A060); kit_quad(l, V(330, 3332, 1400), V(360, 0, 0), V(0, 0, 220), 0x2070D0);
          kit_dial(m, V(0, 3332, 1500), V(1, 0, 0), V(0, 0, 1), V(0, -1, 0), 130, 0x505860, 0x2070C0, 0xF0F0F0);
          kit_lamps(l, V(-300, 3330, 1345), V(90, 0, 0), V(55, 0, 0), V(0, 0, 18), 8);
          l.add(M_COCKL); }
        for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; auto bx = [&](float x0, float y0, float z0, float x1, float y1, float z1, uint32_t c) { m.box(V((std::min)(q * x0, q * x1), y0, z0), V((std::max)(q * x0, q * x1), y1, z1), c); };
            d1k::kit_slab(m, {V(q * 1000, 1680, 1600), V(q * 4600, 420, 1320), V(q * 4600, -5200, 1320), V(q * 880, -4200, 1600)}, V(0, 0, 60), W, d1k::kit_shade(W, 0.7f));   /* wing */
            d1k::kit_slab(m, {V(q * 460, -5320, 1600), V(q * 2070, -7000, 1600), V(q * 2070, -7840, 1600), V(q * 460, -7000, 1600)}, V(0, 0, 45), W, d1k::kit_shade(W, 0.7f));   /* tailplane */
            bx(1000, -200, 1150, 1500, 1700, 2050, B); bx(1050, 1690, 1250, 1450, 1740, 1950, DK);                     /* intake */
            bx(910, 700, 620, 1470, 1540, 1250, TR); bx(910, -2520, 620, 1470, -1680, 1250, TR); }                     /* nozzles */
        d1k::kit_slab(m, {V(0, -3640, 2300), V(0, -6860, 1950), V(0, -7700, 4400), V(0, -5600, 4260)}, V(50, 0, 0), TR, d1k::kit_shade(TR, 0.7f)); }                    /* fin */
      m.add(M_COCKPIT); }
    { Mesh m; m.box(V(-70, -7000, -70), V(70, 7000, 70), 0xE02818); m.add(M_BUL); }   /* thin red lines (Octavius t1123) */
    { Mesh m; m.box(V(-150, -2000, -150), V(150, 2000, 150), 0xFF6020); m.add(M_EBUL); }
    { Mesh m; m.box(V(-180, -1100, -180), V(180, 1100, 180), 0xE8E8E0, 0xC8C8C0, 0xC8C8C0); m.box(V(-90, -2600, -90), V(90, -1100, 90), 0xF08020); m.add(M_MIS); }
    { Mesh m; m.box(V(-250, -700, -250), V(250, 700, 250), 0x303238); m.add(M_BOMB); }
    /* the fireball: a picture with a dark ragged edge (Octavius t1127, t1128), two turns of one quad, half size 1 m, scaled per frame */
    { const int W = 64; std::vector<uint32_t> px(W * W); std::mt19937 r6(77); float lobes[9]; for (float &l : lobes) l = 0.72f + 0.28f * (r6() / (float)(r6.max)());
      std::vector<float> nz(W * W); for (float &n : nz) n = r6() / (float)(r6.max)();
      for (int y = 0; y < W; y++) for (int x = 0; x < W; x++) { float dx = (x - 31.5f) / 31.5f, dy = (y - 31.5f) / 31.5f, rr = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx); float u = (a + PI_F) / (2.f * PI_F) * 9.f; int i0 = (int)u % 9, i1 = (i0 + 1) % 9; float fu = u - floorf(u), edge = lobes[i0] * (1.f - fu) + lobes[i1] * fu;
          edge += (nz[(y / 4 * 4) * W + x / 4 * 4] - 0.5f) * 0.14f; float k = rr / edge, g = nz[y * W + x] * 0.12f; uint32_t c = 0;
          if (k < 1.f) { float t = k + g - 0.06f; int R, G, B; if (t < .3f) { R = 255; G = 235; B = 150; } else if (t < .55f) { R = 250; G = 170; B = 50; } else if (t < .75f) { R = 215; G = 95; B = 25; } else if (t < .9f) { R = 110; G = 50; B = 20; } else { R = 30; G = 22; B = 18; }
              c = 0xFF000000u | ((uint32_t)B << 16) | ((uint32_t)G << 8) | (uint32_t)R; }
          px[y * W + x] = c; }
      VPIX_Render_AddTextureRGBA(TEX_FIRE0, W, W, px.data());
      for (int k = 0; k < 2; k++) { Mesh m; VPIX_MeshPoly q; q.flags = 0x20FFFFFF; q.tex = TEX_FIRE0; q.n = 4; float c = 1000.f; V cn[4] = {V(-c, 0, -c), V(c, 0, -c), V(c, 0, c), V(-c, 0, c)}; const float uu[4] = {0, 1, 1, 0}, vv[4] = {1, 1, 0, 0}; for (int i = 0; i < 4; i++) { q.idx[i] = m.vert(cn[i]); int j = k ? (i + 1) & 3 : i; q.uv[i][0] = uu[j]; q.uv[i][1] = vv[j]; }
          q.nx = 0; q.ny = -1; q.nz = 0; m.p.push_back(q); m.add(M_FIRE0 + k); } }
    { Mesh m; V a = mp(0, 0.9f, 0), b = mp(0.8f, -0.5f, 0), c = mp(-0.8f, -0.5f, 0), d = mp(0, 0, 0.9f);
      vt_out(m, {a, b, d}, 0xF08A18, mp(0, 0, 0.2f)); vt_out(m, {b, c, d}, 0xD84010, mp(0, 0, 0.2f)); vt_out(m, {c, a, d}, 0xFFC030, mp(0, 0, 0.2f)); vt_out(m, {a, c, b}, 0xA04010, mp(0, 0, 0.2f));
      m.add(M_DEBRIS); }
    /* gun box, cross, ring and line: green and 3D, fixed to the cockpit 2 m in front of the eye. Box top ndc +.222, half width ndc .242; the cross 64 picture lines under the top (Octavius t1118-t1125) */
    { Mesh m; const uint32_t G = 0x40C850; m.box(V(-322, GUN_Y, BOX_ZB), V(-312, GUN_Y + 2, BOX_ZT), G); m.box(V(312, GUN_Y, BOX_ZB), V(322, GUN_Y + 2, BOX_ZT), G); m.box(V(-322, GUN_Y, BOX_ZT - 10), V(322, GUN_Y + 2, BOX_ZT), G); m.add(M_BOX3D); }
    { Mesh m; const uint32_t G = 0x40C850; m.box(V(-55, 0, -4), V(55, 2, 4), G); m.box(V(-4, 0, -36), V(4, 2, 36), G); m.box(V(-22, 0, -30), V(22, 2, -24), G); m.add(M_CROSS); }
    for (int k = 0; k < 2; k++) { Mesh m; const uint32_t C = k ? 0xE02818 : 0x40C850; const float R0 = 62.f, R1 = 53.f; for (int i = 0; i < 16; i++) { float a0 = 2.f * PI_F * i / 16, a1 = 2.f * PI_F * (i + 1) / 16;
            m.poly({V(cosf(a0) * R0, 0, sinf(a0) * R0), V(cosf(a1) * R0, 0, sinf(a1) * R0), V(cosf(a1) * R1, 0, sinf(a1) * R1), V(cosf(a0) * R1, 0, sinf(a0) * R1)}, C); }
        m.box(V(-R1, 0, -3), V(R1, 1, 3), C); m.box(V(-3, 0, -R1), V(3, 1, R1), C); m.add(k ? M_RINGR : M_RINGG); }
    { Mesh m; m.box(V(0, 0, -3), V(1000, 1, 3), 0x40C850); m.add(M_LINE); }
    /* the bars' fills: 1000 long along local z, scaled to the level, on the side panels */
    { const uint32_t C[3] = {0xE8D028, 0x38C048, 0xE04020}; for (int k = 0; k < 3; k++) { Mesh m; m.poly({V(-16, 0, 0), V(16, 0, 0), V(16, 0, 1000), V(-16, 0, 1000)}, C[k]); m.add(M_BARY + k); } }
    /* the map screen: a live texture on a quad in the hood */
    { Mesh m; VPIX_MeshPoly q; q.flags = 0x20FFFFFF; q.tex = TEX_MAP; q.n = 4; V cn[4] = {V(-178, 3992, 1472), V(178, 3992, 1472), V(178, 3992, 1850), V(-178, 3992, 1850)}; const float uu[4] = {0, 1, 1, 0}, vv[4] = {1, 1, 0, 0};
      for (int i = 0; i < 4; i++) { q.idx[i] = m.vert(cn[i]); q.uv[i][0] = uu[i]; q.uv[i][1] = vv[i]; } q.nx = 0; q.ny = -1; q.nz = 0; m.p.push_back(q); m.add(M_MAP); }
    dl("vtol assets built");
}

/* ---- the world's surfaces ---- */
struct Ship { float x = 250.f, y = 100.f, yaw = 0.f, sp = 5.f; bool turning = false; float yaw_t = 0.f; };
static Ship ship;
static float vt_mtn_h(float x, float y) {
    float best = -1e9f;
    for (const Mtn &q : MTN) { float u = sqrtf((x - q.x) * (x - q.x) + (y - q.y) * (y - q.y)) / q.r; if (u >= 1.f) continue; float hh = q.h * (u > .72f ? (1.f - u) / .28f * .5f : u > .46f ? .5f + (.72f - u) / .26f * .32f : u > .22f ? .82f + (.46f - u) / .24f * .15f : .97f + (.22f - u) / .22f * .03f); if (hh > best) best = hh; }
    return best;
}
/* highest surface under (x,y). kind: 0 sea, 1 grass, 2 runway or apron, 3 carrier deck (landing part), 4 solid thing (building, ship, island), 5 mountain */
static float vt_surface(float x, float y, int &kind) {
    float h = SEA_Z; kind = 0;
    if (x >= LAND_X0 && x <= LAND_X1 && y >= LAND_Y0 && y <= LAND_Y1) { h = 0.f; kind = 1; }
    if (fabsf(x) <= 20.f && y >= -122.f && y <= 162.f) { h = 0.f; kind = 2; }
    else if (y >= -140.f && y <= -40.f && fabsf(x) >= 18.f && fabsf(x) <= 84.f) { h = 0.f; kind = 2; }
    { float ix = x - ISLE_X, iy = y - ISLE_Y; if (ix * ix + iy * iy < ISLE_R * ISLE_R) { h = 0.f; kind = 1; } }
    float cx = x - CAR_X, cy = y - CAR_Y;
    if (fabsf(cy) <= 58.f && cx >= -14.5f && cx <= 14.5f) { h = CAR_DECK; kind = (cx <= 7.f) ? 3 : 4; if (cx >= 9.f && cy >= 10.f && cy <= 26.f) h = 22.f; }
    else if (fabsf(cx) <= 12.f && fabsf(cy) <= 78.f) { h = CAR_DECK; kind = 4; }
    float mh = vt_mtn_h(x, y); if (mh > h) { h = mh; kind = 5; }
    for (const Box &b : obst) if (x >= b.x0 && x <= b.x1 && y >= b.y0 && y <= b.y1 && b.top > h) { h = b.top; kind = 4; }
    if (fabsf(x - ship.x) <= 5.5f && fabsf(y - ship.y) <= 30.f && SEA_Z + 13.f > h) { h = SEA_Z + 13.f; kind = 4; }
    return h;
}

/* ---- state ---- */
enum State { S_TITLE, S_MENU, S_PLAY, S_OVER };
static State st = S_TITLE; static float st_t = 0; static double sim_t = 0;
static bool autoplay = false;
struct Plane { V p; M R; float yaw = 0, speed = 0, vz = 0, power = 0, fuel = 100.f, dmg = 0, gun = 1.f; bool ground = true, burner = false; };   /* R: rows right, forward, up */
static Plane ply;
static float crash_t = -1.f, crash_hold = 3.f; static V crash_eye, crash_at;
struct Enemy { bool slow = false, chaser = false; V p, vel; float yaw = 0, pitch = 0, roll = 0, st_t = 0, cool = 2.f, brk = 0, brk_yaw = 0; int state = 0, id = 0; bool alive = false; };
static std::vector<Enemy> en;
struct Bul { V p, v; float t = 0; bool mine = false; int kind = 0, tgt = 0, owner = 0; };   /* owner: the seat that fired it (link play) */   /* kind 0 tracer, 1 missile, 2 bomb */
static std::vector<Bul> buls;
struct Fx { V p, v, w; M R; float t = 0, life = 1.f; int kind = 0, maxi = 3, mesh = 0; };   /* kind 0 fireball, 1 debris, 2 a chunk, 3 a spark */
static std::vector<Fx> fxs;
static int score = 0, wave = 1, kills = 0, spawned = 0, total_kills = 0, crashes = 0;
static float spawn_timer = 0, interlude = 0, banner_t = 0, time_left = 0, fire_cool = 0, warn_t = 0, flash_t = 0; static int flash_btn = -1;
static bool final_wave = false;
/* the stages. Footage: n/5. Your Commodore 79: five bandits, "a refuelling sequence" then "more jets over the blue ocean". Amiga User Int.: refuelling must be done to progress. The One 33: bombing the HQ. MC 127: landing on a carrier. The order is ours */
static const int HQ_WAVE = 3, REFUEL_WAVE = 1;   /* three waves, one refuelling, back to land, about 3 minutes: Bobby 11-03. Last wave: the HQ (The One 33) with enemy boats, the island's frigate copied (Bobby's pick) */
static bool vt_tgt_wave() { return wave == HQ_WAVE; }
struct Tgt { float x, y; int mesh; float top; bool alive; };
static Tgt hq[5];
/* THE SWITCH: the lowest level whose refuelling you fly by hand. 2 = Ace only, as Computer+Videogiochi 9 says (automatic on the first two). Bobby may remember flying it by hand on easier levels: set 0 (or env VTOL_HAND_FROM=0) for that. Pilot VS Pilot counts as Pilot */
static int hand_from = 2;
static bool vt_rf_auto() { return (skill == 3 ? 1 : skill) < hand_from; }
static float rf_reel = 0.f, tank_off = 0.f;   /* how far the line is out, 0 to 1; how far the tanker still is from its place on the circle. Line goes out and back: Bobby 11-03 "it detracts and retracted". Speed INVENTED */
static float tank_gone = 0, hose_gone = 0; static int rf_test = 0; static bool rf_chase = false, rf_tested = false; static float rf_test_t = 0;   /* a wreck takes them away for a while. Dev: VTOL_RFTEST, VTOL_CHASE */
enum { RF_NONE, RF_COME, RF_LINK, RF_LEAVE }; static int rf = RF_NONE; static float rf_t = 0, tank_a = 0; static V tank_p, tank_f; static float stage_extra = 0;
static int lock_id = 0, next_id = 1; static float missile_cool = 0;
static const float TANK_RAD = 210.f, TANK_W = 0.2f, TANK_Z = 95.f; static float TANK_X = 275.f, TANK_Y = 30.f;   /* the tanker's circle, over the sea between one stage and the next */
/* the mission goes away from the island: Bobby's memory 11-03, unsure ("I think ... the mission went further away from this island"); fits Your Commodore 79 "more jets over the blue ocean". Footage only shows the first stage or two, round the island, so nothing against it. Spacing and direction INVENTED */
static V vt_stage_at(int w) { static const float C[4][2] = {{0, 20}, {550, 40}, {ISLE_X, ISLE_Y}, {CAR_X, CAR_Y}}; int i = w < 1 ? 0 : w > 4 ? 3 : w - 1; return V(C[i][0], C[i][1], 0); }
static V vt_stage_c();
static char banner[32] = ""; static const char *over_msg = "";
static bool have_ref = false; static V head_ref; static float yaw0 = 0;
static float tick_hz_log = 0;
static const float MENU_T = 7.f; static float menu_left = MENU_T, zoom_t = -1.f;   /* the red marks count down about 2 a second; reaching none starting the round is a guess */

/* ---- sounds: INVENTED placeholders ---- */
enum { SND_GUN = 0x7E000001, SND_BOOM, SND_CRASH, SND_HIT, SND_WARN, SND_ENG, SND_GO, SND_MENU };
static void vt_snd(int ch, uint32_t key, int vol, int pan = 0) { if (sound_on) buggy::play(ch, (int)key, vol, pan); }
static void vt_make_sounds() {
    std::mt19937 r(9); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(SND_GUN, 0.12f, [&](float t) { return noise() * exp(-t * 30.f) * 0.7f + sinf(6.2831853f * 180.f * t) * exp(-t * 40.f) * 0.4f; });
    make_sound(SND_BOOM, 0.9f, [&](float t) { return noise() * exp(-t * 4.5f) * 0.8f + sinf(6.2831853f * 60.f * t) * exp(-t * 3.f) * 0.6f; });
    make_sound(SND_CRASH, 1.6f, [&](float t) { return noise() * exp(-t * 2.5f) * 0.9f + sinf(6.2831853f * 45.f * t) * exp(-t * 2.f) * 0.7f; });
    make_sound(SND_HIT, 0.25f, [&](float t) { return noise() * exp(-t * 12.f) * 0.8f + sinf(6.2831853f * 300.f * t) * exp(-t * 14.f) * 0.3f; });
    make_sound(SND_WARN, 0.2f, [&](float t) { return sinf(6.2831853f * 1200.f * t) * exp(-t * 8.f) * 0.5f; });
    make_sound(SND_GO, 0.5f, [&](float t) { return sinf(6.2831853f * (440.f + 400.f * t) * t) * exp(-t * 3.f) * 0.6f; });
    make_sound(SND_MENU, 0.08f, [&](float t) { return sinf(6.2831853f * 900.f * t) * exp(-t * 30.f) * 0.5f; });
    { VSound s; s.rate = 22050; s.loop = true; size_t n = (size_t)(0.5f * s.rate); s.pcm.resize(n);   /* a buzzy turbine hum: harmonics of 40 Hz, loops cleanly */
      float ph[10]; for (int k = 0; k < 10; k++) ph[k] = rnd(0.f, 6.2831853f);
      for (size_t i = 0; i < n; i++) { float t = (float)i / s.rate, v = 0; for (int k = 1; k <= 10; k++) v += sinf(6.2831853f * 40.f * k * t + ph[k - 1]) / (float)k;
          s.pcm[i] = (int16_t)(clampf(v * 0.12f, -1.f, 1.f) * 24000.f); }
      sounds[SND_ENG] = std::move(s); sound_names[SND_ENG] = "vtol_synth"; }
}
static void vt_engine_sound(bool on, float vol01, float semis) {
    if (!sound_on) return;
    if (on) { if (!voices[0].on || voices[0].sound != (uint32_t)SND_ENG) buggy::play(0, (int)SND_ENG, 50);
        voices[0].vol = (int)clampf(30.f + 70.f * vol01, 0.f, 127.f); voices[0].bend = (int)(semis * 32.f); }
    else if (voices[0].on && voices[0].sound == (uint32_t)SND_ENG) voices[0].on = false;
}

/* ---- HUD bitmaps ---- */
enum { B_BTN_GREY = 400, B_BTN_YEL, B_BTN_ORG, B_MARK, B_BG, B_PIC, B_THUMB = 410, B_SLASH = 420, B_BLACK = 421 };
static void vt_solid(int id, int w, int h, uint32_t fill, uint32_t border, int bw) {
    std::vector<uint32_t> px(w * h, fill);
    if (bw > 0) for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if (x < bw || y < bw || x >= w - bw || y >= h - bw) px[y * w + x] = border;
    d1k::add_bitmap(id, w, h, px);
}
static void vt_make_hud() {
    d1k::make_hud();
    const uint32_t colset[2] = {0xFF823C3Cu /* dark blue, as the footage's HUD */, 0xFF101010u};
    for (int s = 0; s < 2; s++) for (int ch = 0; ch < 26; ch++) {
        std::vector<uint32_t> px(d1k::FW * d1k::FH, 0);
        for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (d1k::FONT[ch][y * 5 + x] == '1')
            for (int b = 0; b < 2; b++) for (int a = 0; a < 2; a++) px[(y * 2 + b) * d1k::FW + x * 2 + a] = colset[s];
        d1k::add_bitmap(d1k::BM_TEXT + (3 + s) * 40 + ch, d1k::FW, d1k::FH, px);
    }
    for (int d = 0; d < 10; d++) d1k::make_digit(50 + d, d, 0xFF823C3Cu, 0u);
    { std::vector<uint32_t> px(8 * 14, 0); for (int y = 0; y < 14; y++) { int x = 6 - (y * 6) / 13; px[y * 8 + x] = 0xFF823C3Cu; px[y * 8 + x + 1] = 0xFF823C3Cu; } d1k::add_bitmap(B_SLASH, 8, 14, px); }
    /* the menu: footage art, a dark streaky background made here */
    { auto pic = [](int id, const uint8_t *d, const uint32_t *pal, int w, int h) { std::vector<uint32_t> px(w * h); for (int i = 0; i < w * h; i++) px[i] = pal[d[i]]; d1k::add_bitmap(id, w, h, px); };
      pic(B_PIC, VT_PIC_CADET, VT_PIC_CADET_PAL, VT_PW, VT_PH); pic(B_PIC + 1, VT_PIC_PILOT, VT_PIC_PILOT_PAL, VT_PW, VT_PH); pic(B_PIC + 2, VT_PIC_ACE, VT_PIC_ACE_PAL, VT_PW, VT_PH); pic(B_PIC + 3, VT_PIC_PVP, VT_PIC_PVP_PAL, VT_PW, VT_PH);
      pic(B_THUMB, VT_THUMB0, VT_THUMB0_PAL, VT_TW, VT_TH); pic(B_THUMB + 1, VT_THUMB1, VT_THUMB1_PAL, VT_TW, VT_TH); pic(B_THUMB + 2, VT_THUMB2, VT_THUMB2_PAL, VT_TW, VT_TH); pic(B_THUMB + 3, VT_THUMB3, VT_THUMB3_PAL, VT_TW, VT_TH);
      pic(B_BTN_GREY, VT_BTN_PREV, VT_BTN_PREV_PAL, VT_BW, VT_BH); pic(B_BTN_YEL, VT_BTN_NEXT, VT_BTN_NEXT_PAL, VT_BW, VT_BH); pic(B_BTN_ORG, VT_BTN_PLAY, VT_BTN_PLAY_PAL, VT_BW, VT_BH);
      std::mt19937 r9(3); std::vector<uint32_t> bg(176 * 144); std::vector<float> run(144, 0.f); for (int x = 0; x < 176; x++) for (int y = 0; y < 144; y++) { run[y] = run[y] * 0.86f + (r9() / (float)(r9.max)() - 0.5f) * 0.4f; int v = (int)clampf(44.f + 34.f * run[y], 24.f, 72.f); bg[y * 176 + x] = 0xFF000000u | ((uint32_t)(v + 4) << 16) | ((uint32_t)(v + 1) << 8) | (uint32_t)v; }
      d1k::add_bitmap(B_BG, 176, 144, bg); }
    vt_solid(B_MARK, 4, 8, 0xFF2020C8u, 0, 0); vt_solid(B_BLACK, 4, 4, 0xFF000000u, 0, 0);
}
/* the map screen's picture: mottled greens from the island, orange round the airfield, blue sea, turned and moved under a pale wire Harrier that stays in the middle (Octavius t1122, t1128) */
static const int MTEX = 96, MW = 256; static std::vector<uint32_t> map_px, map_world; static const float MSPAN = 1500.f;
static void vt_map_init() {
    map_world.resize(MW * MW); map_px.assign(MTEX * MTEX, 0xFF080808u); std::mt19937 r(21); std::vector<float> a(MW * MW, 0.f), b(MW * MW, 0.f);
    auto lay = [&](std::vector<float> &o, int cell) { int n = MW / cell + 2; std::vector<float> g(n * n); for (float &x : g) x = r() / (float)(r.max)();
        for (int y = 0; y < MW; y++) for (int x = 0; x < MW; x++) { float fx = (float)x / cell, fy = (float)y / cell; int ix = (int)fx, iy = (int)fy; fx -= ix; fy -= iy;
            o[y * MW + x] += ((g[iy * n + ix] * (1 - fx) + g[iy * n + ix + 1] * fx) * (1 - fy) + (g[(iy + 1) * n + ix] * (1 - fx) + g[(iy + 1) * n + ix + 1] * fx) * fy) * (float)cell / 60.f; } };
    lay(a, 32); lay(a, 16); lay(a, 8); lay(a, 4); lay(b, 16); lay(b, 8);
    for (int y = 0; y < MW; y++) for (int x = 0; x < MW; x++) { float wx = ((float)x / MW - .5f) * MSPAN, wy = ((float)y / MW - .5f) * MSPAN, v = a[y * MW + x], o = b[y * MW + x], w = (v - .75f) * 240.f; int R, G, B;
        if (!(wx > LAND_X0 + w && wx < LAND_X1 - w && wy > LAND_Y0 + w && wy < LAND_Y1 - w)) { R = 30; G = 70; B = 150; }
        else { float k = clampf((v - .25f) * 1.8f, 0.f, 1.f); k = k * k; R = 12 + (int)(70.f * k); G = 28 + (int)(115.f * k); B = 12 + (int)(36.f * k);
            float d = sqrtf(wx * wx + (wy + 50.f) * (wy + 50.f)); if (d < 90.f * o * o + 10.f) { R = 150 + (int)(90.f * o); G = 90 + (int)(60.f * o); B = 44; } }
        map_world[y * MW + x] = 0xFF000000u | ((uint32_t)B << 16) | ((uint32_t)G << 8) | (uint32_t)R; }
    VPIX_Render_AddTextureRGBA(TEX_MAP, MTEX, MTEX, map_px.data());
}
static void vt_map_line(float x0, float y0, float x1, float y1) { int n = (int)(std::max)(fabsf(x1 - x0), fabsf(y1 - y0)) * 2 + 1; for (int i = 0; i <= n; i++) { float t = (float)i / n; int x = (int)(x0 + (x1 - x0) * t), y = (int)(y0 + (y1 - y0) * t); if (x >= 0 && y >= 0 && x < MTEX && y < MTEX) map_px[y * MTEX + x] = 0xFFC8E8E8u; } }
static bool mark_on = false; static float mark_x = 0, mark_y = 0;   /* where the next wave is. Bobby 11-03, unsure: "it directed you to hit waves of enemys". How is unknown, nothing in footage, press has radio and tower voices (MC 127). The mark is INVENTED */
static void vt_map_update(bool dark, float px, float py, float yaw) {
    if (dark) { std::fill(map_px.begin(), map_px.end(), 0xFF080808u); VPIX_Render_UpdateTextureRGBA(TEX_MAP, MTEX, MTEX, map_px.data()); return; }
    const float zoom = 420.f / MTEX, cy = cosf(yaw), sy = sinf(yaw);   /* the ground stays north up and only scrolls; the jet turns on it (Octavius t1113-t1135, t1144, BBC t265, t319) */
    for (int y = 0; y < MTEX; y++) for (int x = 0; x < MTEX; x++) { float u = (x - MTEX * .5f) * zoom, v = (MTEX * .5f - y) * zoom, wx = px + u, wy = py + v; int ix = (int)((wx / MSPAN + .5f) * MW), iy = (int)((wy / MSPAN + .5f) * MW);
        map_px[y * MTEX + x] = (ix >= 0 && iy >= 0 && ix < MW && iy < MW) ? map_world[iy * MW + ix] : 0xFF96461Eu; }
    /* the nose is closed and blunt with a refuelling probe out in front: Bobby 11-03, "a nose with a point or tube for refueling" (his memory, unsure; the footage is too soft to show it or rule it out) */
    static const float J[][4] = {{45.5f,17,44,34},{50.5f,17,52,34},{45.5f,17,47,13},{50.5f,17,49,13},{47,13,49,13},{48,13,48,3},{44,34,18,58},{52,34,78,58},{18,58,20,63},{78,58,76,63},{20,63,42,56},{76,63,54,56},{42,56,41,70},{54,56,55,70},{41,70,32,80},{55,70,64,80},{32,80,34,84},{64,80,62,84},{34,84,48,78},{62,84,48,78},{44,34,42,56},{52,34,54,56}};
    const auto RX = [&](float x, float y) { return 48.f + (x - 48.f) * cy + (y - 48.f) * sy; }; const auto RY = [&](float x, float y) { return 48.f - (x - 48.f) * sy + (y - 48.f) * cy; };
    for (const auto &l : J) vt_map_line(RX(l[0], l[1]), RY(l[0], l[1]), RX(l[2], l[3]), RY(l[2], l[3]));
    if (mark_on) { int mx = (int)clampf(MTEX * .5f + (mark_x - px) / zoom, 5.f, MTEX - 6.f), my = (int)clampf(MTEX * .5f - (mark_y - py) / zoom, 5.f, MTEX - 6.f); for (int b = -3; b <= 3; b++) for (int a = -3; a <= 3; a++) map_px[(my + b) * MTEX + mx + a] = (abs(a) < 2 && abs(b) < 2) ? 0xFF1820E0u : 0xFF20E0F0u; }
    VPIX_Render_UpdateTextureRGBA(TEX_MAP, MTEX, MTEX, map_px.data());
}
static void vt_blit(VPIX_Frame &f, int id, int w, int h, float x, float y, float dw, float dh) {
    VPIX_Blit b = {}; b.handle = d1k::BMPH + id; b.sw = w; b.sh = h; b.x = (int)x; b.y = (int)y; b.dw = dw; b.dh = dh; b.key = 0; b.alpha = 1.f; b.prio = 1000000;
    b.tint[0] = b.tint[1] = b.tint[2] = 1.f;
    f.blits.push_back(b);
}
static void vt_text(VPIX_Frame &f, const char *s, float x, float y, float sc, int set) {   /* left-aligned; sets 0 white, 1 yellow, 2 red, 3 dark blue, 4 black */
    for (const char *c = s; *c; c++) {
        if (*c >= 'A' && *c <= 'Z') vt_blit(f, d1k::BM_TEXT + set * 40 + (*c - 'A'), d1k::FW, d1k::FH, x, y, d1k::FW * sc, d1k::FH * sc);
        else if (*c >= '0' && *c <= '9') { int d = *c - '0';
            if (set >= 3) vt_blit(f, d1k::BM_DIG + 50 + d, d1k::DW, d1k::DH, x + sc, y - 2.f * sc, d1k::DW * 0.55f * sc, d1k::DH * 0.55f * sc);
            else vt_blit(f, d1k::BM_DIG + 20 + d, d1k::DW, d1k::DH, x, y - 2.f * sc, d1k::DW * 0.6f * sc, d1k::DH * 0.6f * sc); }
        else if (*c == '/') vt_blit(f, B_SLASH, 8, 14, x + 2.f * sc, y, 8.f * sc, 14.f * sc);
        x += 12.f * sc;
    }
}
static void vt_textc(VPIX_Frame &f, const char *s, float cx, float y, float sc, int set) { vt_text(f, s, cx - (float)strlen(s) * 6.f * sc, y, sc, set); }

/* ---- the aircraft's frame: rows right, forward, up ---- */
static M vt_basis(float yaw, float pitch, float roll) {
    V fw(-sinf(yaw) * cosf(pitch), cosf(yaw) * cosf(pitch), sinf(pitch)), r0(cosf(yaw), sinf(yaw), 0), u0 = cross(r0, fw);
    M m; m.r[0] = r0 * cosf(roll) - u0 * sinf(roll); m.r[1] = fw; m.r[2] = u0 * cosf(roll) + r0 * sinf(roll); return m;
}

/* ---- link play (vpix_1k_net.h) data. Pod 1 runs the whole world: waves, enemy jets, chasers, boats, tanker, bullets, the clock. Another pod flies its OWN jet here (flight, fuel, crash and respawn are the pod's), sends the jet's pose and its fire / missile / crash counts,
   and draws the master's state. The master turns those counts into shots and tells the pod when it was hit. INVENTED: co-op (the five jets of a wave are shared, each pilot keeps their own score, one more enemy may be up per extra pilot); a pod joins at the airfield (as after a crash);
   the mission ends when the master or any pilot lands after the last wave; only the master's own jet flies the tanker sequence, another pod is topped up while its needle point is in the cup. Late join: a pod plays its own solo mission until the master's game gives it a seat. ---- */
#pragma pack(push, 1)
struct NetIn { float p[3], f[3], u[3], speed; uint16_t fire_seq, msl_seq, crash_seq; uint8_t flags; };   /* flags: 1 crashed, 2 landed */
struct NetPl { float p[3], f[3], u[3], speed; uint8_t flags, hits; int16_t score; };                      /* flags: 1 flying, 2 crashed, 4 on the line */
struct NetEn { float p[3], yaw, pitch, roll; uint16_t id; uint8_t flags; };                               /* flags: 1 chaser, 2 slow */
struct NetBl { float p[3]; int16_t v[3]; uint8_t flags; };                                                /* v in tenths of a m/s; flags: kind, 4 = mine */
struct NetEv { float p[3]; uint8_t maxi, life10, src, id; };                                              /* an explosion */
struct NetSt { uint8_t st, skill, wave, kills, flags, rf, hq, nen, nbl, ev_top, pad[2]; float time_left, reel, tank_p[3], tank_f[2], ship[3]; NetPl pl[4]; NetEv ev[8]; NetEn en[8]; NetBl bl[36]; };   /* flags: 1 last wave, 2 tanker gone, 4 line gone, 8 won */
#pragma pack(pop)
static_assert(sizeof(NetSt) <= v1knet::SNAP_MAX && sizeof(NetIn) <= v1knet::IN_MAX, "link snapshot too big");
struct Remote { bool active = false, crashed = false, landed = false, fuelling = false; Plane a; uint16_t fire_seq = 0, msl_seq = 0, crash_seq = 0; uint8_t hits = 0; int score = 0; };
static Remote rem[4];                       /* master: the other pods' jets, seats 1-3 */
struct Ghost { bool show = false; V p; M R; };
static Ghost ghost[4];                      /* the other pilots' jets as drawn here */
static NetEv ev_ring[8]; static uint8_t ev_id = 0, ev_seen = 0; static int ev_src = 0;
static bool slave_game = false;             /* a linked pod flying inside the master's round */
static uint16_t net_fire_seq = 0, net_msl_seq = 0, net_crash_seq = 0; static uint8_t net_hits_seen = 0; static bool net_fuel = false;

/* ---- effects, kills, crashes ---- */
static void vt_explode(V p, int maxi, float life) {
    if (v1knet::on && v1knet::master) { NetEv &e = ev_ring[(uint8_t)(ev_id + 1) & 7]; e.p[0] = p.x; e.p[1] = p.y; e.p[2] = p.z; e.maxi = (uint8_t)maxi; e.life10 = (uint8_t)(std::min)(255.f, life * 10.f); e.src = (uint8_t)ev_src; e.id = ++ev_id; }
    Fx f; f.p = p; f.life = life; f.kind = 0; f.maxi = maxi; fxs.push_back(f);
    for (int k = 0; k < 8; k++) { Fx d; d.kind = 1; d.p = p; d.v = norm(V(rnd(-1.f, 1.f), rnd(-1.f, 1.f), rnd(0.1f, 1.2f))) * rnd(8.f, 30.f);
        d.w = V(rnd(-6.f, 6.f), rnd(-6.f, 6.f), rnd(-6.f, 6.f)); d.life = rnd(1.6f, 2.8f); fxs.push_back(d); }
}
static void extern_chase_reset();
static void vt_sparks(V at, V base, int n, float speed, float life) {   /* the Dactyl burst, at this game's size */
    for (int k = 0; k < n; k++) { Fx s; s.kind = 3; s.mesh = (k & 1) ? M_SPK_O : M_SPK_R; s.p = at; s.v = base + norm(V(rnd(-1.f, 1.f), rnd(-1.f, 1.f), rnd(-1.f, 1.f))) * (speed * rnd(.3f, 1.f)); s.life = life * rnd(.6f, 1.f); fxs.push_back(s); }
}
static void vt_chunk(int mesh, V at, V base, const M &R0, float fly) {
    Fx c; c.kind = 2; c.mesh = mesh; c.p = at; c.R = R0; c.v = base + norm(V(rnd(-1.f, 1.f), rnd(-1.f, 1.f), rnd(.2f, 1.2f))) * (fly * rnd(.5f, 1.f)); c.w = V(rnd(-3.f, 3.f), rnd(-3.f, 3.f), rnd(-3.f, 3.f)); c.life = rnd(4.5f, 6.5f); fxs.push_back(c);
}
static void vt_break_jet() {   /* your own craft comes apart, pieces and sparks: Bobby 11-03, "explode apart like the new dac explosion". No footage of a crash up close */
    const Plane &a = ply; V o = a.p + V(0, 0, 1.6f), r = a.R.r[0], f = a.R.r[1], base = f * (a.speed * .5f);
    vt_chunk(M_JC0, o + f * 5.f, base, a.R, 16.f); vt_chunk(M_JC1, o, base, a.R, 10.f); vt_chunk(M_JC2, o - f * 6.f, base, a.R, 14.f); vt_chunk(M_JC3, o - r * 3.f, base, a.R, 16.f); vt_chunk(M_JC4, o + r * 3.f, base, a.R, 16.f);
    vt_sparks(o, base, 70, 26.f, 2.4f);
}
static void vt_crash(const char *why) {
    if (crash_t >= 0.f) return;
    crash_t = 0.f; crashes++; rf_test = 0;
    V fw(-sinf(ply.yaw), cosf(ply.yaw), 0);
    if (rf == RF_LINK) { rf = RF_COME; rf_t = 0.f; }
    crash_at = ply.p + V(0, 0, 2);
    crash_eye = ply.p - fw * 32.f + V(0, 0, 14);
    { int k; float h = vt_surface(crash_eye.x, crash_eye.y, k); if (crash_eye.z < h + 5.f) crash_eye.z = h + 5.f; }
    vt_explode(crash_at, 3, 2.4f); vt_break_jet();
    vt_snd(3, SND_CRASH, 127);
    dl("t=%.1f crash (%s) at %.0f %.0f %.0f speed %.0f vz %.1f fuel %.0f", sim_t, why, ply.p.x, ply.p.y, ply.p.z, ply.speed, ply.vz, ply.fuel);
}
static void vt_respawn() {
    ply = Plane(); ply.p = V(0, -100.f, 0); ply.yaw = 0.f; ply.ground = true; ply.R = vt_basis(0.f, 0.f, 0.f);
    buls.clear(); crash_t = -1.f; extern_chase_reset();
    dl("t=%.1f at the airfield", sim_t);
}
static void vt_finish(bool won, const char *msg) {
    st = S_OVER; st_t = 0; over_msg = msg;
    dl("t=%.1f round end (%s): score %d kills %d wave %d crashes %d", sim_t, won ? "won" : "lost", score, total_kills, wave, crashes);
}
static void vt_new_wave();
static V vt_stage_c() { return vt_stage_at(final_wave ? 6 : wave); }
static void vt_wave_done() {   /* n/5 done: the tanker comes. Automatic on Cadet and Pilot, by hand on Ace (Computer+Videogiochi 9) */
    if (rf != RF_NONE || final_wave) return;
    final_wave = wave >= waves_total;
    if (wave != REFUEL_WAVE || final_wave) { dl("t=%.1f stage %d cleared, score %d", sim_t, wave, score); vt_new_wave(); return; }   /* one refuelling in a mission, after the first five: Bobby 11-03 "only happend once", Your Commodore 79 for where */
    { V a0 = vt_stage_at(wave), a1 = vt_stage_at(final_wave ? 6 : wave + 1); TANK_X = (a0.x + a1.x) * .5f; TANK_Y = (a0.y + a1.y) * .5f; }
    rf = RF_COME; rf_t = 0.f; rf_reel = 0.f; tank_off = 420.f; { V to = ply.p - V(TANK_X, TANK_Y, 0); tank_a = atan2f(to.y, to.x); }
    dl("t=%.1f stage %d cleared, score %d: tanker", sim_t, wave, score);
}
static void vt_new_wave() {   /* the refuel is done: the next stage and a minute more */
    hint_dir = 5.f;
    float add = (std::min)(stage_time, game_max - stage_extra - game_len); if (add > 0.f) { time_left += add; stage_extra += add; }
    if (final_wave) { dl("t=%.1f all stages done: land on the carrier", sim_t); return; }
    wave++; kills = 0; spawned = 0; spawn_timer = 2.5f; extern_chase_reset();
    snprintf(banner, sizeof banner, "WAVE %d", wave); banner_t = 2.5f;
    dl("t=%.1f stage %d%s, time left %.0f", sim_t, wave, wave == HQ_WAVE ? " (the enemy HQ)" : "", time_left);
}
static void vt_add_score(int owner, int n) { if (owner > 0 && owner < 4) rem[owner].score += n; else score += n; }
static const Plane *vt_victim(V from) {   /* what an enemy goes after: the nearest jet that is flying, yours or a linked pod's */
    const Plane *best = crash_t < 0.f ? &ply : NULL; float bd = best ? len(from - ply.p) : 1e30f;
    if (v1knet::on) for (int i = 1; i < 4; i++) if (rem[i].active && !rem[i].crashed) { float d = len(from - rem[i].a.p); if (d < bd) { bd = d; best = &rem[i].a; } }
    return best ? best : &ply;
}
static void vt_kill_enemy(Enemy &e, bool by_player, int owner = 0) {
    if (!e.alive) return;
    e.alive = false;
    vt_explode(e.p, 2, 1.1f);
    vt_snd(4, SND_BOOM, 110);
    if (by_player) vt_add_score(owner, 100);
    if (!e.chaser) kills++;
    total_kills++;
    dl("t=%.1f enemy down%s: %d/%d score %d fuel %.0f", sim_t, by_player ? "" : " (collision)", kills, per_wave, score, ply.fuel);
    if (kills >= per_wave) vt_wave_done();
}
static void vt_spawn() {
    Enemy e; float ang = rnd(0.f, 2.f * PI_F), r = rnd(190.f, 250.f);   /* "planes coming at you from all directions" (Games-X 18), round this stage's place: "more jets over the blue ocean" */
    V sc = vt_stage_c() - ply.p; sc.z = 0.f; float sd = len(sc); V base = sd < 400.f ? vt_stage_c() : ply.p + sc * (200.f / sd);
    if (!final_wave) { V s0 = vt_stage_at(wave), s1 = vt_stage_at(wave + 1); base = s0 + (s1 - s0) * ((float)spawned / (float)(std::max)(per_wave, 1)); base.z = 0.f; }   /* each wave strung out along the way, so fighting it carries you on: Bobby's pick 11-03, footage silent */
    if (wave == 1 && spawned < 2 && skill <= 1) { V fw(-sinf(ply.yaw), cosf(ply.yaw), 0); e.p = ply.p + fw * (300.f + 60.f * spawned) + V(0, 0, 30.f); e.p.z = (std::max)(e.p.z, 50.f); e.slow = true; e.yaw = ply.yaw + PI_F; e.alive = true; e.cool = 9.f; e.id = next_id++; en.push_back(e); return; }   /* the first two come straight at you, slow, not firing, to learn the lock: Bobby's pick 11-03 */
    e.p = V(base.x + cosf(ang) * r, base.y + sinf(ang) * r, clampf(ply.p.z + rnd(-20.f, 50.f), 35.f, 130.f));
    e.yaw = atan2f(-(ply.p.x - e.p.x), ply.p.y - e.p.y); e.alive = true; e.cool = rnd(1.5f, 3.f); e.id = next_id++;
    en.push_back(e);
}

/* ---- enemies (INVENTED behaviour: close in, fire when lined up, break away, come round again) ---- */
static void vt_enemy_step(Enemy &e, float dt) {
    const Plane &P = *vt_victim(e.p);   /* link play: the nearest flying jet; alone, always yours */
    if (e.slow) { V to2 = P.p + V(0, 0, 4) - e.p; float d2 = len(to2); if (d2 < 70.f) e.slow = false; else { V way = to2 * (1.f / d2); e.vel = way * 28.f; e.p += e.vel * dt; e.yaw = atan2f(-way.x, way.y); e.pitch = asinf(clampf(way.z, -1.f, 1.f)); e.roll = 0.f; return; } }
    const Skill &S = SK();
    V tgt = P.p + V(0, 0, 2); V to = tgt - e.p; float dist = len(to), hd = sqrtf(to.x * to.x + to.y * to.y), wy, wp;
    const float wf = 1.f + 0.06f * (float)(wave - 1);
    if (e.state == 0) {
        wy = atan2f(-to.x, to.y); wp = clampf(atan2f(to.z, hd), -0.7f, 0.7f);
        if (dist < 45.f) { e.state = 1; e.st_t = 0; e.brk = (rnd(0.f, 1.f) < 0.5f ? -1.f : 1.f) * rnd(1.0f, 1.8f); e.brk_yaw = e.yaw + e.brk; }
    } else { wy = e.brk_yaw; wp = 0.2f; e.st_t += dt; if (e.st_t > 2.6f) e.state = 0; }
    float dy = wrap_pi(wy - e.yaw), turn = S.eturn * dt * (e.state == 1 ? 1.4f : 1.f);
    e.yaw += clampf(dy, -turn, turn);
    e.pitch += clampf(wp - e.pitch, -0.8f * dt, 0.8f * dt);
    e.roll += (clampf(-dy * 1.2f, -0.9f, 0.9f) - e.roll) * clampf(dt * 3.f, 0.f, 1.f);
    float cp = cosf(e.pitch); V fw(-sinf(e.yaw) * cp, cosf(e.yaw) * cp, sinf(e.pitch));
    e.vel = fw * (S.espeed * wf); e.p += e.vel * dt;
    { int k; float h = vt_surface(e.p.x, e.p.y, k), minz = h + 14.f; if (e.p.z < minz) { e.p.z = minz; if (e.pitch < 0.1f) e.pitch = 0.1f; } }
    if (e.p.z > ceiling_z + 20.f) e.p.z = ceiling_z + 20.f;
    e.cool -= dt;
    if (e.state == 0 && e.cool <= 0.f && dist < 150.f && dist > 20.f && (crash_t < 0.f || &P != &ply) && dot(norm(to), fw) > 0.985f) {
        Bul b; V d = norm(to + V(rnd(-1.f, 1.f), rnd(-1.f, 1.f), rnd(-1.f, 1.f)) * (dist * 0.04f));
        b.p = e.p + fw * 5.f; b.v = d * 190.f; b.mine = false; buls.push_back(b);
        e.cool = S.efire * rnd(0.8f, 1.4f) / wf;
        vt_snd(5, SND_GUN, 45);
    }
}

/* ---- controls ---- */
struct Ctl { float want = -1.f, sx = 0, sy = 0, lift = 0, lat = 0; bool fire = false, missile = false, sel = false, burn = false; };
/* Computer+Videogiochi 9: right stick = flight stick, trigger cannon, top button missile when the sight finds a target; left stick = faster or slower and a sideways correction, its trigger = afterburner on Ace. Your Commodore 79 agrees the missile is a click.
   VR: right thumbstick flies, right trigger cannon, right grip missile (A when the input has it), left trigger forward, left stick click lift, B sink, left stick sideways. Desktop: W/S nose down/up, A/D roll, Shift/Ctrl power, Q/E sideways, Space or left mouse cannon, F missile, Tab afterburner */
static Ctl vt_controls(const VPIX_Input &in) {
    in_vr = in.vr; Ctl c; float jx = in.joy[0], jy = in.joy[1], lift = in.brake - (in.b2 ? 1.f : 0.f); bool space = false, fkey = false, tab = false;
#ifdef _WIN32
    if (!in.vr) {
        auto key = [](int k) { return (GetAsyncKeyState(k) & 0x8000) != 0; };
        float kx = (key('D') ? 1.f : 0.f) - (key('A') ? 1.f : 0.f), ky = (key('W') ? 1.f : 0.f) - (key('S') ? 1.f : 0.f);
        if (fabsf(kx) > fabsf(jx)) jx = kx;
        if (fabsf(ky) > fabsf(jy)) jy = ky;
        lift += (key(VK_SHIFT) ? 1.f : 0.f) - (key(VK_CONTROL) ? 1.f : 0.f);
        c.lat = (key('E') ? 1.f : 0.f) - (key('Q') ? 1.f : 0.f);
        space = key(VK_SPACE); fkey = key('F'); tab = key(VK_TAB);
    }
#endif
    if (in.vr) {   /* Bobby's scheme 11-03. Sticks only, no hand motion. UNTESTED, no headset here */
        jx = in.joy_r[0]; jy = in.joy_r[1]; c.lat = in.joy_l[0]; stick_lx = in.joy_l[0];
        static bool lift_mode = false, click_was = false; const bool click = in.voice_mute; if (click && !click_was) lift_mode = !lift_mode; click_was = click;   /* left stick click; shares the voice mute's input until it has its own */
        c.want = in.b2 ? .05f : in.brake > .05f ? .4f + .6f * in.brake : lift_mode ? .4f : .2f;   /* left trigger forward, let off and it falls back to the hover; click climbs; B sinks */
    }
    c.sx = clampf(jx, -1.f, 1.f); c.sy = clampf(jy, -1.f, 1.f); c.lift = clampf(lift, -1.f, 1.f);
    c.sel = in.throttle > 0.6f || (in.buttons & VPIX_BTN_TRIGGER) != 0 || space;
    c.fire = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f || space;
    c.missile = in.top || fkey || (in.vr && in.fire2); c.burn = tab || in.brake > 0.9f;
    return c;
}
static const Enemy *vt_nearest() { const Enemy *t = NULL; float bd = 1e9f; for (const Enemy &e : en) if (e.alive) { float d = len(e.p - ply.p); if (d < bd) { bd = d; t = &e; } } return t; }
static const Tgt *vt_near_hq() { const Tgt *t = NULL; float bd = 1e9f; for (const Tgt &g : hq) if (g.alive) { float d = len(V(g.x, g.y, 0) - ply.p); if (d < bd) { bd = d; t = &g; } } return t; }
static V vt_hdu() { return tank_p - tank_f * 11.5f + V(0, 0, -2.1f); }
static V vt_drogue() { V A = vt_hdu(), F = tank_p - tank_f * 30.f + V(0, 0, -5.f); return A + (F - A) * rf_reel; }   /* the middle of the cup's mouth */
static V vt_hold() { return vt_drogue() + tank_f * .5f - tank_f * TIP_Y - V(0, 0, TIP_Z); }   /* where the jet sits with the tip in the cup */
static Ctl vt_auto() {   /* VTOL_AUTO: the player plays itself */
    Ctl c; const Plane &a = ply;
    if (crash_t >= 0.f || time_left > game_len - sit_s) return c;
    V tgt; bool shoot = false, land = false; const Enemy *t = vt_nearest(); const Tgt *g = vt_tgt_wave() ? vt_near_hq() : NULL;
    if (rf != RF_NONE) tgt = vt_hold();
    else if (final_wave) { tgt = V(CAR_X - 4.f, CAR_Y, CAR_DECK); land = true; }
    else if (t) { tgt = t->p; shoot = true; }
    else if (g) { tgt = V(g->x, g->y, g->top + 3.f); shoot = true; }
    else { V sc = vt_stage_c(); tgt = V(sc.x, sc.y, 60.f); }
    V to = tgt - a.p; float hd = sqrtf(to.x * to.x + to.y * to.y), err = wrap_pi(atan2f(-to.x, to.y) - a.yaw);
    float want_z = tgt.z; if (hd > 60.f && !g) want_z = (std::max)(want_z, 45.f);
    if (g && hd > 90.f) want_z = 45.f;
    { int k; V ah = a.p + a.R.r[1] * 90.f; float h = vt_surface(ah.x, ah.y, k); if (h + 30.f > want_z && !land) want_z = h + 30.f; float h0 = vt_surface(a.p.x, a.p.y, k); if (h0 + 22.f > want_z && !(land && hd < 60.f)) want_z = h0 + 22.f; }
    float bank = asinf(clampf(-a.R.r[0].z, -1.f, 1.f)), des_bank = clampf(-err * 1.6f, -1.1f, 1.1f);
    c.sx = clampf((des_bank - bank) * 2.5f, -1.f, 1.f);
    float des_pitch = clampf(atan2f(want_z - a.p.z, (std::max)(hd, 40.f)), -.5f, .6f);
    c.sy = clampf((a.R.r[1].z - sinf(des_pitch)) * 4.f, -1.f, 1.f);
    float want_p = fabsf(err) < .7f ? .85f : .65f;
    if (a.ground || (a.speed < 25.f && a.p.z < 25.f)) want_p = .4f;
    if (land) { if (hd < 130.f) want_p = .45f + hd / 130.f * .2f; if (hd < 16.f || (a.speed < 2.f && hd < 30.f)) { float wv = a.p.z > CAR_DECK + 25.f ? -9.f : -3.f; want_p = .2f + wv / climb_rate * .2f; c.sx = -clampf(err * 2.f, -1.f, 1.f); } }
    if (a.ground && land && hd < 22.f) { want_p = 0.f; }
    if (rf != RF_NONE && hd < 140.f) want_p = .4f + .6f * 42.f / v_max + clampf((hd - 30.f) * .002f, -.02f, .15f);
    c.lift = clampf((want_p - a.power) * 8.f, -1.f, 1.f);
    if (a.ground) { c.sx = 0.f; c.sy = 0.f; }
    float ang = len(to) > 1.f ? dot(norm(to), a.R.r[1]) : 0.f;
    c.fire = shoot && ang > .985f && len(to) < 280.f;
    c.missile = shoot && lock_id != 0 && !g;
    if (g && hd < 40.f && a.p.z < g->top + 40.f) c.missile = true;
    return c;
}

/* ---- the player's flight. The shape is sourced, every number INVENTED: "vertical take-off" (Your Commodore 79), "get into vertical thrust" (BBC 1990), so the power lever's bottom part is thrust straight down: under .2 you sink, .2 holds, .4 climbs;
   above .4 the nozzles swing back and the jet flies on its wings, where the stick rolls and pitches with full authority (upside down at publicity t1145). In the hover it levels itself and the stick yaws. ---- */
static int vt_lock_of(const Plane &pl_) {   /* what the ring points at */
    int found = 0; const M &A = pl_.R; float best = aim_cone_deg > 0.f ? aim_cone_deg * 0.0174533f : SK().cone;
    for (const Enemy &e : en) { if (!e.alive) continue; V to = e.p - pl_.p; float d = len(to); if (d > 420.f || d < 5.f) continue; float ang = acosf(clampf(dot(norm(to), A.r[1]), -1.f, 1.f)); if (ang < best) { best = ang; found = e.id; } }
    return found;
}
static void vt_lock() { lock_id = vt_lock_of(ply); }
static void vt_player_fire(const Plane &a, int owner = 0) {
    V fw = a.R.r[1], up = a.R.r[2], mz = a.p + up * 1.8f + fw * 9.f, dir = norm(fw - up * 0.07f);   /* the cross sits a little under the view's centre */
    float best = aim_cone_deg > 0.f ? aim_cone_deg * 0.0174533f : SK().cone;   /* INVENTED aim assist: inside this cone the shot is aimed at the enemy */
    for (const Enemy &e : en) { if (!e.alive) continue;
        V to = e.p - mz; float d = len(to); if (d > 420.f || d < 5.f) continue;
        float ang = acosf(clampf(dot(norm(to), fw), -1.f, 1.f));
        if (ang < best) { best = ang; dir = norm(e.p + e.vel * (d / bullet_speed) - mz); } }
    Bul b; b.p = mz; b.v = dir * bullet_speed; b.mine = true; b.owner = owner; buls.push_back(b);
    if (owner == 0) vt_snd(1, SND_GUN, 80);
}
static void vt_weapon2(Plane &a, int lock, int owner) {   /* the top button: a missile "when the targeting system finds a target" (C+V 9); with none, low over the ground, it lets go a bomb (The One 33; how bombs are dropped is ours) */
    Bul b; b.mine = true; b.owner = owner; b.p = a.p - a.R.r[2] * 1.f;
    if (lock) { b.kind = 1; b.tgt = lock; b.v = a.R.r[1] * (a.speed + 60.f); if (owner == 0) vt_snd(1, SND_GO, 90); dl("t=%.1f missile seat %d", sim_t, owner); }
    else { b.kind = 2; b.v = a.R.r[1] * a.speed + V(0, 0, a.vz); if (owner == 0) vt_snd(1, SND_MENU, 90); }
    buls.push_back(b); if (owner == 0) missile_cool = missile_gap;
}
static void vt_fly(const Ctl &c, float dt) {
    Plane &a = ply; const bool engine = a.fuel > 0.f;
    a.power = engine ? clampf(a.power + (c.want >= 0.f ? clampf((c.want - a.power) * 4.f, -1.f, 1.f) : c.lift) * 0.6f * dt, 0.f, 1.f) : 0.f;
    a.burner = engine && skill == 2 && c.burn && a.power > .95f;   /* "afterburner" on Ace only */
    const float h = clampf(1.f - a.speed / 30.f, 0.f, 1.f);   /* 1 = all on the nozzles */
    float vt = engine ? v_max * (std::max)(0.f, (a.power - .4f) / .6f) * (a.burner ? 1.35f : 1.f) : 0.f;
    if (a.ground) vt = (std::min)(vt, 22.f);
    a.speed += (vt - a.speed) * clampf(dt * (a.ground ? 0.6f : 0.7f), 0.f, 1.f);
    if (!a.ground) a.speed = (std::max)(0.f, a.speed - a.R.r[1].z * 5.f * dt);
    if (!engine) { a.vz -= grav * dt; if (a.vz < -40.f) a.vz = -40.f; }
    else {
        float vzt = h * ((std::min)(a.power, .4f) - .2f) / .2f * climb_rate;
        if (a.ground && vzt < 0.f) { vzt = 0.f; a.vz = 0.f; }
        a.vz += (vzt - a.vz) * clampf(dt * 2.5f, 0.f, 1.f);
        if (land_assist && a.speed < safe_speed) { int k; float hh = vt_surface(a.p.x, a.p.y, k);
            if (k >= 1 && k <= 3) { float cap = 1.5f + (a.p.z - hh) * 0.5f; if (a.vz < -cap) a.vz = -cap; } }
    }
    M &R = a.R; const V right = R.r[0], fw = R.r[1];
    if (!a.ground) {
        float rr = c.sx * roll_rate * (1.f - h) + right.z * 2.f * h, pr = -c.sy * pitch_rate * (1.f - h * .8f) - fw.z * 2.f * h;
        if (fabsf(c.sx) < .1f && h < 1.f) rr += right.z * .5f;   /* INVENTED: lets go of the stick and it comes back level, slowly */
        float k = right.z * turn_rate * (1.f - h) * clampf(a.speed / 30.f, 0.f, 1.f) - c.sx * turn_rate * h;
        buggy::rotate(R, fw * rr + right * pr + V(0, 0, k), dt);
    } else if (a.speed > 3.f || a.power > .05f) buggy::rotate(R, V(0, 0, -c.sx * turn_rate * .5f), dt);
    { V f = R.r[1]; if (fabsf(f.x) + fabsf(f.y) > .05f) a.yaw = atan2f(-f.x, f.y); }
    { V sc = vt_stage_c(); float dx = sc.x - a.p.x, dy = sc.y - a.p.y, d = sqrtf(dx * dx + dy * dy);   /* INVENTED: away from the stage's place the jet is turned toward it, which is also what leads you on to the next one; not on the way to the tanker */
      if (d > limit_r && !a.ground && rf == RF_NONE) buggy::rotate(R, V(0, 0, clampf(wrap_pi(atan2f(-dx, dy) - a.yaw), -1.5f, 1.5f)), dt); }
    a.p += R.r[1] * (a.speed * dt) + R.r[0] * (c.lat * 10.f * dt); a.p.z += a.vz * dt;   /* the left stick's sideways correction (C+V 9) */
    if (a.p.z > ceiling_z) { a.p.z = ceiling_z; if (a.vz > 0.f) a.vz = 0.f; if (R.r[1].z > 0.f) buggy::rotate(R, R.r[0] * -1.f, dt); }
    int kind; float hg = vt_surface(a.p.x, a.p.y, kind);
    if (a.p.z <= hg + (kind == 0 ? 0.8f : 0.f)) {
        bool safe = kind >= 1 && kind <= 3 && a.vz > -safe_vz && a.speed < safe_speed && a.p.z >= hg - 1.5f && R.r[2].z > .7f;
        if (safe) { a.p.z = hg; if (a.vz < 0.f) a.vz = 0.f; if (!a.ground) R = vt_basis(a.yaw, 0.f, 0.f); a.ground = true; }
        else { vt_crash(kind == 0 ? "sea" : kind == 5 ? "mountain" : kind == 4 ? "structure" : "hard landing"); return; }
    } else a.ground = false;
    float burn = fuel_burn * SK().burn * ((a.ground && a.speed < 1.f && a.power < .05f) ? 0.05f : (0.45f + 0.5f * a.speed / v_max)) * (a.burner ? 2.5f : 1.f);
    a.fuel = (std::max)(0.f, a.fuel - burn * dt);
    if (a.ground && a.speed < 2.f && (kind == 2 || kind == 3)) { a.fuel = (std::min)(100.f, a.fuel + refuel_rate * dt); a.dmg = (std::max)(0.f, a.dmg - 6.f * dt); }
    if (a.fuel < 20.f && a.fuel > 0.f) { warn_t -= dt; if (warn_t <= 0.f) { warn_t = 1.5f; vt_snd(6, SND_WARN, 70); } }
    if (a.ground && final_wave && rf == RF_NONE && (kind == 3 || kind == 2) && a.speed < 3.f && st == S_PLAY && !slave_game) vt_finish(true, "MISSION COMPLETE");   /* landing on the carrier (MC 127, BBC t351) */
}
/* the guns and top button work in the air, whatever the state; the right bar is the guns' charge (it falls while kills are coming and climbs after: Octavius t1121-t1134. Inferred, not proven) */
static void vt_weapons(const Ctl &c, float dt) {
    Plane &a = ply; vt_lock();
    if (fire_cool > 0.f) fire_cool -= dt;
    if (missile_cool > 0.f) missile_cool -= dt;
    if (c.fire && a.gun > 0.f && rf != RF_LINK) { a.gun = (std::max)(0.f, a.gun - gun_drain * dt); if (fire_cool <= 0.f) { fire_cool = fire_gap; vt_player_fire(a); } }
    else a.gun = (std::min)(1.f, a.gun + gun_fill * dt);
    static bool m_was = false; if (c.missile && !m_was && missile_cool <= 0.f && !a.ground && rf != RF_LINK) vt_weapon2(a, lock_id, 0); m_was = c.missile;
}
/* the refuelling sequence. Sources: a C-130 (C+V 9); automatic on Cadet and Pilot, by hand on Ace with the left stick's sideways correction (C+V 9); "precision flying and refuelling must be achieved to progress" (Amiga User Int.).
   Bobby's memory 11-03: the tanker trailed a cone on a hose, "an empty cup on a tether and you had to put the point of the nose needle in it". So contact counts only when the tip goes in through the mouth from behind.
   Bobby's call, no source: a wreck blows up, the tanker in chunks with sparks. Really a clumsy contact only whips the hose and shears the probe (real-world reference). How close, how fast, how long: INVENTED */
static void vt_wreck_tanker() {
    M B = vt_basis(atan2f(-tank_f.x, tank_f.y), 0.f, 0.f); V r = B.r[0], f = B.r[1], base = f * 30.f;
    vt_chunk(M_TC0, tank_p + f * 8.f, base, B, 12.f); vt_chunk(M_TC1, tank_p - f * 4.f, base, B, 9.f); vt_chunk(M_TC2, tank_p - f * 15.f, base, B, 13.f); vt_chunk(M_TC3, tank_p - r * 11.f + f * 2.f + V(0, 0, 2.3f), base, B, 16.f); vt_chunk(M_TC4, tank_p + r * 11.f + f * 2.f + V(0, 0, 2.3f), base, B, 16.f);
    vt_explode(tank_p, 3, 1.5f); vt_explode(tank_p + r * 10.f, 2, 1.2f); vt_explode(tank_p - r * 10.f, 2, 1.3f); vt_explode(tank_p - f * 12.f, 2, 1.1f);
    vt_sparks(tank_p, base, 110, 42.f, 2.8f);
    tank_gone = 12.f; dl("t=%.1f the tanker is wrecked", sim_t);
}
static bool vt_in_jet(V q, const Plane &a) {   /* is this world point inside the jet's fuselage or wings */
    V d = q - a.p; float fx = dot(d, a.R.r[0]), fy = dot(d, a.R.r[1]), fz = dot(d, a.R.r[2]);
    return (fabsf(fx) < 1.3f && fy > -7.f && fy < 7.f && fz > .6f && fz < 3.2f) || (fabsf(fx) < 4.8f && fy > -5.f && fy < 2.f && fz > 1.f && fz < 2.2f);
}
static bool vt_hits_tanker(const Plane &a, const M &B) {
    V d = a.p + V(0, 0, 1.6f) - tank_p; float x = fabsf(dot(d, B.r[0])), y = dot(d, B.r[1]), z = dot(d, B.r[2]); const float q = 3.f;
    return (x < 2 + q && y > -18 - q && y < 17 + q && fabsf(z) < 2 + q) || (x < 20 + q && y > -q && y < 5 + q && z > 2 - q && z < 2.7f + q) || (x < 7 + q && y > -18 - q && y < -15 + q && z > .6f - q && z < 1.1f + q) || (x < .35f + q && y > -18.5f - q && y < -13 + q && z > 1.9f - q && z < 8 + q);
}
static void vt_refuel(float dt) {
    tank_a += TANK_W * dt; tank_f = V(-sinf(tank_a), cosf(tank_a), 0); tank_p = V(TANK_X + cosf(tank_a) * TANK_RAD, TANK_Y + sinf(tank_a) * TANK_RAD, TANK_Z + tank_off * .08f) + tank_f * tank_off;
    static float prev_ty = -99.f; static V prev_tip;
    if (tank_gone > 0.f) { tank_gone -= dt; prev_ty = -99.f; if (tank_gone <= 0.f) { hose_gone = 0.f; V to = ply.p - V(TANK_X, TANK_Y, 0); tank_a = atan2f(to.y, to.x); rf_t = 0.f; rf_reel = 0.f; tank_off = 420.f; dl("t=%.1f a new tanker", sim_t); } return; }
    if (hose_gone > 0.f) { hose_gone -= dt; prev_ty = -99.f; if (rf != RF_LEAVE) return; }
    rf_t += dt;
    if (rf == RF_LEAVE) { rf_reel = (std::max)(0.f, rf_reel - dt / 4.f); if (rf_reel <= 0.f) { tank_off += 70.f * dt; if (tank_off > 420.f) { rf = RF_NONE; tank_off = 0.f; return; } } }   /* the line reels in, then the tanker climbs away into the haze instead of vanishing */
    else { tank_off = (std::max)(0.f, tank_off - 60.f * dt); if (tank_off < 150.f) rf_reel = (std::min)(1.f, rf_reel + dt / 4.f); }
    const bool autom = vt_rf_auto(); Plane &a = ply; V d = vt_drogue(); M B = vt_basis(atan2f(-tank_f.x, tank_f.y), 0.f, 0.f);
    if (crash_t >= 0.f) { prev_ty = -99.f; return; }
    if (rf_test && (rf == RF_COME || rf == RF_LINK)) {   /* dev: a scripted approach, for the contact and the wrecks. Tests the rules, not how hard it is to fly */
        if (rf_reel >= 1.f) rf_test_t += dt; float back = rf_test == 3 ? 0.f : rf_test == 2 ? 42.f - 8.f * rf_test_t : (std::max)(0.f, 42.f - 8.f * rf_test_t); a.R = B; a.yaw = atan2f(-tank_f.x, tank_f.y); a.speed = 42.f; a.vz = 0.f; a.ground = false; a.power = .75f;
        if (rf_test == 3) a.p = tank_p - tank_f * (70.f - 8.f * rf_test_t) - V(0, 0, 1.6f);
        else if (rf == RF_LINK) a.p = vt_hold();
        else a.p = d - tank_f * (TIP_Y + back - 3.f) - V(0, 0, TIP_Z) + B.r[0] * (rf_test == 2 ? 3.f : 0.f);
    }
    const bool hand = !autom || rf_test;
    if (rf == RF_COME && !hand && !a.ground) {
        V hold = vt_hold(); float L = len(hold - a.p); V to = hold - tank_f * clampf(L * .6f, 0.f, 60.f) - a.p, way = norm(to + V(0, 0, .001f)); float tl = len(to);   /* up from behind, slowing as it comes */
        M want = vt_basis(atan2f(-to.x, to.y), 0.f, 0.f); if (L < 30.f) want = B;
        for (int k = 0; k < 3; k++) a.R.r[k] = norm(a.R.r[k] + (want.r[k] - a.R.r[k]) * clampf(dt * 1.5f, 0.f, 1.f)); buggy::ortho(a.R);
        a.p += way * (std::min)(tl + 42.f * dt, clampf(45.f + L * .5f, 45.f, 95.f) * dt); a.speed = 42.f; a.vz = 0.f; a.power = .75f; a.yaw = atan2f(-a.R.r[1].x, a.R.r[1].y);
        if (L < 1.f && rf_t > 4.f && rf_reel >= 1.f) { rf = RF_LINK; rf_t = 0.f; dl("t=%.1f on the hose (auto)", sim_t); }
    } else if (rf == RF_LINK && !hand) {
        a.p = vt_hold(); a.R = B; a.yaw = atan2f(-tank_f.x, tank_f.y); a.speed = 42.f; a.vz = 0.f;
    } else if (rf == RF_COME || rf == RF_LINK) {   /* by hand */
        V tip = a.p + a.R.r[1] * TIP_Y + a.R.r[2] * TIP_Z, rel = tip - d; float ty = dot(rel, B.r[1]), tx = dot(rel, B.r[0]), tz = dot(rel, B.r[2]), radial = sqrtf(tx * tx + tz * tz);
        if (rf == RF_COME) {
            if (rf_reel >= 1.f && prev_ty > -90.f && prev_ty < 0.f && ty >= 0.f) {   /* the tip crossed the plane of the mouth, from behind */
                V pr = prev_tip - d; float k = -prev_ty / (ty - prev_ty), cx = dot(pr, B.r[0]) * (1.f - k) + tx * k, cz = dot(pr, B.r[2]) * (1.f - k) + tz * k, closing = (ty - prev_ty) / (std::max)(dt, .001f);
                if (sqrtf(cx * cx + cz * cz) < CUP_R) {
                    if (closing > 22.f) { dl("t=%.1f into the cup at %.0f m/s", sim_t, closing); hose_gone = 8.f; rf_reel = 0.f; vt_sparks(d, tank_f * 42.f, 40, 18.f, 1.6f); vt_crash("rammed the cup"); prev_ty = -99.f; return; }
                    rf = RF_LINK; rf_t = 0.f; dl("t=%.1f on the hose (by hand) closing %.0f m/s", sim_t, closing);
                }
            }
        } else if (radial > CUP_R + .8f || ty < -1.f || ty > 2.5f) { rf = RF_COME; rf_t = 0.f; dl("t=%.1f off the hose", sim_t); }
        prev_ty = ty; prev_tip = tip;
        /* missing badly: the cup or hose goes into the airframe */
        bool hit = rf_reel > .02f && vt_in_jet(d, a); if (rf_reel > .02f && !hit) { V A = vt_hdu(); for (int k = 1; k < 5; k++) if (vt_in_jet(A + (d - A) * (k / 5.f), a)) hit = true; }
        if (hit) { dl("t=%.1f the cup hit the airframe", sim_t); hose_gone = 8.f; rf_reel = 0.f; vt_sparks(d, tank_f * 42.f, 40, 18.f, 1.6f); vt_crash("hit the hose"); prev_ty = -99.f; return; }
    }
    if (rf == RF_LINK) { a.fuel = (std::min)(100.f, a.fuel + 18.f * dt); if (a.fuel >= 100.f && rf_t > 6.f) { rf = RF_LEAVE; rf_t = 0.f; rf_test = 0; dl("t=%.1f refuelled", sim_t); vt_new_wave(); } }
    /* the tanker itself: on Ace, or once you have the stick back */
    if ((hand || rf == RF_LEAVE) && !a.ground && vt_hits_tanker(a, B)) { rf_test = 0; vt_wreck_tanker(); vt_crash("hit the tanker"); crash_at = tank_p; crash_eye = a.p - tank_f * 70.f + V(0, 0, 30.f); crash_hold = 5.f; rf = RF_COME; rf_t = 0.f; }
    if (fabsf(a.p.x) > 1e5f) a.p.x = 0;
}

/* ---- one step of the world ---- */
static float chase_min = 1e9f, chase_cool = 0.f, boat_cool = 3.f;
static void extern_chase_reset() { chase_min = 1e9f; }
static void vt_waves(float dt) {
    /* fly away from where the mission is and jets come after you: Bobby's pick 11-03. Numbers INVENTED */
    { V sc = vt_stage_c() - ply.p; sc.z = 0.f; float d = len(sc); if (d < chase_min) chase_min = d; chase_cool -= dt; int nc = 0; for (const Enemy &e : en) if (e.alive && e.chaser) nc++;
      if (rf == RF_NONE && crash_t < 0.f && !ply.ground && d > 500.f && d > chase_min + 300.f && chase_cool <= 0.f && nc < 3) { Enemy e; float ang = rnd(0.f, 2.f * PI_F); e.p = V(ply.p.x + cosf(ang) * 230.f, ply.p.y + sinf(ang) * 230.f, clampf(ply.p.z + rnd(-10.f, 30.f), 35.f, 130.f)); e.yaw = atan2f(-(ply.p.x - e.p.x), ply.p.y - e.p.y); e.alive = true; e.chaser = true; e.cool = 2.f; e.id = next_id++; en.push_back(e); chase_cool = 5.f; dl("t=%.1f a chaser", sim_t); } }
    /* the enemy boats fire back: Bobby's pick 11-03 */
    if (vt_tgt_wave() && crash_t < 0.f) { boat_cool -= dt; if (boat_cool <= 0.f) { boat_cool = rnd(1.4f, 2.6f); for (const Tgt &g : hq) if (g.alive && g.mesh == M_FRIGATE) { V o(g.x, g.y, 9.f), to = ply.p - o; if (len(to) < 320.f && rnd(0.f, 1.f) < .5f) { Bul b; b.p = o; b.v = norm(to + V(rnd(-12.f, 12.f), rnd(-12.f, 12.f), rnd(-6.f, 6.f))) * 190.f; b.mine = false; buls.push_back(b); break; } } } }
    if (rf == RF_COME || rf == RF_LINK || final_wave || vt_tgt_wave()) return;
    spawn_timer -= dt;
    int alive = 0; for (const Enemy &e : en) if (e.alive) alive++;
    int max_alive = enemies_override > 0 ? enemies_override : SK().max_alive; if (v1knet::on) for (int i = 1; i < 4; i++) if (rem[i].active) max_alive++;   /* link play: one more up per extra pilot. INVENTED */
    if (spawned < per_wave && alive < max_alive && spawn_timer <= 0.f) { vt_spawn(); spawned++; spawn_timer = rnd(1.5f, 3.2f); }
}
static void vt_step(const Ctl &c, float dt) {
    time_left -= dt; stick_x = c.sx; stick_y = c.sy;
    if (lock_id != 0) hint_lock += dt; else { bool any = false; for (const Enemy &e : en) if (e.alive) any = true; if (any) hint_ring += dt; }
    if (hint_dir > 0.f) hint_dir -= dt;
    if (banner_t > 0.f) banner_t -= dt;
    /* the frigate patrols along y and turns round on the spot */
    if (!ship.turning) { ship.x += -sinf(ship.yaw) * ship.sp * dt; ship.y += cosf(ship.yaw) * ship.sp * dt;
        if (ship.y > 200.f && cosf(ship.yaw) > 0.f) { ship.turning = true; ship.yaw_t = ship.yaw + PI_F; }
        if (ship.y < 40.f && cosf(ship.yaw) < 0.f) { ship.turning = true; ship.yaw_t = ship.yaw + PI_F; } }
    else { ship.yaw += 0.2f * dt; if (ship.yaw >= ship.yaw_t) { ship.yaw = ship.yaw_t; ship.turning = false; } }
    if (crash_t >= 0.f) { crash_t += dt; if (crash_t > crash_hold) { crash_hold = 3.f; vt_respawn(); } }
    else {
        if (!(rf == RF_LINK && vt_rf_auto()) && !(rf == RF_COME && vt_rf_auto() && !ply.ground) && !(rf_test && (rf == RF_COME || rf == RF_LINK))) vt_fly(c, dt);
        if (crash_t < 0.f) vt_weapons(c, dt);
        for (Enemy &e : en) if (e.alive && crash_t < 0.f && len(e.p - ply.p) < 9.f) { vt_kill_enemy(e, false); vt_crash("collision"); }
        if (v1knet::on) for (Enemy &e : en) if (e.alive) for (int s = 1; s < 4; s++) if (rem[s].active && !rem[s].crashed && len(e.p - rem[s].a.p) < 9.f) { vt_kill_enemy(e, false); rem[s].hits += 17; break; }   /* a pod flew into a jet: its pod makes the crash */
    }
    if (rf_test && !rf_tested && time_left < game_len - 4.f) { rf_tested = true; vt_wave_done(); }
    if (rf != RF_NONE) vt_refuel(dt);
    vt_waves(dt);
    for (Enemy &e : en) if (e.alive) vt_enemy_step(e, dt);
    for (size_t i = 0; i < buls.size();) { Bul &b = buls[i]; bool gone = false;
        if (b.kind == 1) { for (const Enemy &e : en) if (e.alive && e.id == b.tgt) b.v = norm(b.v + norm(e.p - b.p) * (380.f * dt * 4.f)) * (std::min)(240.f, len(b.v) + 200.f * dt); }
        if (b.kind == 2) b.v.z -= grav * dt;
        b.p += b.v * dt; b.t += dt;
        if (b.mine) { if (b.kind != 2) for (Enemy &e : en) if (e.alive && len(b.p - e.p) < 7.5f) { vt_kill_enemy(e, true, b.owner); gone = true; break; }
            if (!gone && vt_tgt_wave() && b.p.z < 22.f) for (Tgt &g : hq) if (g.alive && fabsf(b.p.x - g.x) < (b.kind == 2 ? 22.f : 14.f) && fabsf(b.p.y - g.y) < (b.kind == 2 ? 22.f : 14.f) && b.p.z < g.top + 2.f) { g.alive = false; vt_explode(V(g.x, g.y, g.top), 3, 1.6f); vt_snd(4, SND_BOOM, 120); vt_add_score(b.owner, 100); kills++; total_kills++; gone = true; dl("t=%.1f HQ block down: %d/%d score %d", sim_t, kills, per_wave, score); if (kills >= per_wave) vt_wave_done(); break; }
            if (b.kind == 2 && !gone && b.p.z < 0.f) { vt_explode(b.p, 2, .8f); gone = true; } }
        else if (crash_t < 0.f && len(b.p - (ply.p + V(0, 0, 1.8f))) < 6.f) {
            ply.dmg += SK().edmg; gone = true; vt_snd(7, SND_HIT, 100);
            if (ply.dmg >= 100.f) vt_crash("shot down"); }
        else if (v1knet::on) for (int s = 1; s < 4; s++) if (rem[s].active && !rem[s].crashed && len(b.p - (rem[s].a.p + V(0, 0, 1.8f))) < 6.f) { rem[s].hits++; gone = true; break; }   /* a shot hit a pod's jet: the pod takes the damage */
        if (b.t > (b.kind == 1 ? 4.f : b.kind == 2 ? 8.f : b.mine ? 1.1f : 1.7f) || b.p.z < SEA_Z) gone = true;
        if (gone) { buls[i] = buls.back(); buls.pop_back(); } else i++; }
    en.erase(std::remove_if(en.begin(), en.end(), [](const Enemy &e) { return !e.alive; }), en.end());
    for (size_t i = 0; i < fxs.size();) { Fx &x = fxs[i]; x.t += dt;
        if (x.kind == 1 || x.kind == 2) { x.v.z -= grav * dt; x.p += x.v * dt; buggy::rotate(x.R, x.w, dt); }
        else if (x.kind == 3) { x.v.z -= grav * .25f * dt; x.p += x.v * dt; }
        if (x.t >= x.life || x.p.z < SEA_Z - 2.f) { fxs[i] = fxs.back(); fxs.pop_back(); } else i++; }
    if (v1knet::on && final_wave && rf == RF_NONE && st == S_PLAY) for (int s = 1; s < 4; s++) if (rem[s].active && rem[s].landed) { dl("t=%.1f seat %d landed", sim_t, s); vt_finish(true, "MISSION COMPLETE"); break; }   /* any pilot landing after the last wave ends the mission */
    if (time_left <= 0.f && st == S_PLAY) vt_finish(false, "TIME UP");
}

static void vt_start_round(const VPIX_Input &in) {
    (void)in;
    score = 0; wave = 1; kills = 0; spawned = 0; total_kills = 0; crashes = 0; spawn_timer = 12.f; interlude = 0.f; final_wave = false; rf = RF_NONE; hint_ring = hint_lock = hint_dir = 0.f; tank_gone = hose_gone = 0.f; rf_reel = 0.f; tank_off = 0.f; rf_tested = false; rf_test_t = 0.f; stage_extra = 0.f; lock_id = 0; missile_cool = 0.f;
    banner_t = 0.f;
    { const float P[5][3] = {{ISLE_X + 10, ISLE_Y - 10, 0}, {ISLE_X + 30, ISLE_Y + 5, 1}, {ISLE_X + 165, ISLE_Y - 60, 2}, {ISLE_X - 170, ISLE_Y + 40, 2}, {ISLE_X + 40, ISLE_Y + 170, 2}}; for (int i = 0; i < 5; i++) { hq[i].x = P[i][0]; hq[i].y = P[i][1]; hq[i].mesh = P[i][2] > 1.5f ? M_FRIGATE : P[i][2] > .5f ? M_HQ1 : M_HQ0; hq[i].top = P[i][2] > 1.5f ? 8.f : P[i][2] > .5f ? 11.f : 6.f; hq[i].alive = true; } }
    en.clear(); buls.clear(); fxs.clear(); time_left = game_len; fire_cool = 0; warn_t = 0;
    ship = Ship(); vt_respawn();
    for (Remote &r : rem) r = Remote(); for (Ghost &g : ghost) g.show = false; ev_seen = ev_id; v1knet::begin_round(v1knet::SEATS);   /* link play: four seats, a pod fills one when it is heard (also later in the round) */
    have_ref = false;   /* the seat is set again from the head pose in the first frame */
    dl("t=%.1f round start: skill %s, %d waves of %d, %.0f s", sim_t, SK().name, waves_total, per_wave, game_len);
}

/* ---- drawing ---- */
static void vt_hudtext(VPIX_Frame &f, const char *s, float x, float y, float pitch) {   /* the footage's tall HUD text: letter 13 x 22 on the 352 x 288 screen */
    for (const char *c = s; *c; c++, x += pitch) {
        if (*c >= 'A' && *c <= 'Z') vt_blit(f, d1k::BM_TEXT + 3 * 40 + (*c - 'A'), d1k::FW, d1k::FH, x, y, 13.f, 22.f);
        else if (*c >= '0' && *c <= '9') vt_blit(f, d1k::BM_DIG + 50 + (*c - '0'), d1k::DW, d1k::DH, x, y, 13.f, 22.f);
        else if (*c == '/') vt_blit(f, B_SLASH, 8, 14, x, y, 12.f, 22.f); }
}
static void vt_hud(VPIX_Frame &f, const VPIX_Input &in) {
    (void)in;
    char buf[40];
    vt_hudtext(f, "SCORE", 48.f, 10.f, 14.5f);
    snprintf(buf, sizeof buf, "%d", score); vt_hudtext(f, buf, 144.f, 10.f, 14.3f);
    snprintf(buf, sizeof buf, "%d/%d", kills, per_wave); vt_hudtext(f, buf, 272.f, 10.f, 13.2f);
    if (skill == 0 && st == S_PLAY && crash_t < 0.f) {
        const auto say = [&](const char *t, float y, float r, float g, float b) { size_t n0 = f.blits.size(); vt_textc(f, t, 176.f, y, 1.f, 0); for (size_t i = n0; i < f.blits.size(); i++) { f.blits[i].tint[0] = r; f.blits[i].tint[1] = g; f.blits[i].tint[2] = b; } };
        if (lock_id != 0) { say("LOCKED ON", 46.f, .3f, 1.f, .3f); if (hint_lock < 6.f) say(in_vr ? "GRIP FIRES A MISSILE" : "F FIRES A MISSILE", 60.f, 1.f, 1.f, 1.f); }
        else if (hint_ring > 0.f && hint_ring < 5.f && vt_nearest()) say("PUT THE RING ON A JET", 46.f, 1.f, 1.f, 1.f);
        if (hint_dir > 0.f) say(final_wave ? "LAND ON THE CARRIER OR AT BASE" : "NEXT WAVE FOLLOW THE MAP", 74.f, 1.f, 1.f, 1.f);
    }
    if (banners && banner_t > 0.f) vt_textc(f, banner, 176.f, 60.f, 1.4f, 0);   /* INVENTED */
    if (banners && ply.fuel < 20.f && fmodf((float)sim_t, 0.8f) < 0.4f) vt_textc(f, ply.fuel > 0.f ? "LOW FUEL" : "NO FUEL", 176.f, 200.f, 1.f, 2);   /* INVENTED */
}
/* the 3D parts of the cockpit that move: ring, line, bars, map screen */
static void vt_cockpit_live(VPIX_Frame &f, const M &A, V OP) {
    auto top = [&](int id, const M &R, V p) -> VPIX_Instance & { VPIX_Instance &q = dput(f, id, R, OP + to_world(A, p)); q.on_top = 2; q.bright = 1; return q; };
    M I; top(M_BOX3D, A, V()); const V cl(0, GUN_Y - 1.f, CROSS_Z); top(M_CROSS, A, cl);
    /* the ring goes to the enemy nearest the nose, held on the box when the enemy is outside it, and turns red on target; in a refuel it shows the drogue, over the HQ the nearest block */
    const Enemy *t = NULL; float best = -2.f; bool near_e = false; V tp; bool have = false;
    for (const Enemy &e : en) if (e.alive) { V rel = e.p - ply.p; float dl2 = len(rel), c = dot(norm(rel), A.r[1]); if (c > best) { best = c; t = &e; } if (dl2 < 200.f) near_e = true; }
    if (t) { tp = t->p; have = true; }
    else if ((rf == RF_COME || rf == RF_LINK) && tank_gone <= 0.f && hose_gone <= 0.f && rf_reel >= 1.f) { tp = vt_drogue(); have = true; best = dot(norm(tp - ply.p), A.r[1]); }
    else if (vt_tgt_wave()) { const Tgt *g = vt_near_hq(); if (g) { tp = V(g->x, g->y, g->top); have = true; best = dot(norm(tp - ply.p), A.r[1]); } }
    float rx = -30.f, rz = CROSS_Z + 26.f; bool red = false;
    if (have) { V rel = tp - ply.p; float xr = dot(rel, A.r[0]), yr = dot(rel, A.r[1]), zr = dot(rel, A.r[2]), Dp = GUN_Y - 3000.f;
        if (yr < 1.f) { float k = sqrtf(xr * xr + zr * zr) + 0.01f; xr = xr / k * 1e4f; zr = zr / k * 1e4f; yr = 1.f; }
        float wx = xr / yr * Dp, wz = 2200.f + zr / yr * Dp; rx = clampf(wx, -305.f, 305.f); rz = clampf(wz, 1780.f, BOX_ZT - 20.f);
        red = rx == wx && rz == wz && acosf(clampf(best, -1.f, 1.f)) < (aim_cone_deg > 0.f ? aim_cone_deg * 0.0174533f : SK().cone); }
    M R; V rp(rx, GUN_Y - 2.f, rz); top(red ? M_RINGR : M_RINGG, A, rp);
    { float dx = rx, dz = rz - CROSS_Z, L = sqrtf(dx * dx + dz * dz); if (L > 1.f) { float c = dx / L, s = dz / L; M Lm; Lm.r[0] = V(c, 0, s); Lm.r[1] = V(0, 1, 0); Lm.r[2] = V(-s, 0, c); top(M_LINE, compose(A, Lm), cl).S[0] = L / 1000.f; } }
    /* the bars: left yellow, right green or red above half (both read off Octavius t1113-t1138). Left stays low and does not fall in 26 s, so not fuel: height is our guess. Right falls while the kills come and climbs after: the guns' charge is our reading */
    for (int k = 0; k < 2; k++) { float lv = k ? clampf(ply.gun, 0.0f, 1.f) : clampf(ply.p.z / ceiling_z, 0.06f, 1.f); V d = bar_top[k] - bar_base[k]; float Lb = len(d); d = d * (1.f / Lb);
        M B; B.r[0] = norm(cross(d, V(0, -1, 0))); B.r[2] = d; B.r[1] = cross(d, B.r[0]); if (B.r[1].y > 0.f) B.r[1] = B.r[1] * -1.f;
        top(k ? (lv > 0.5f ? M_BARR : M_BARG) : M_BARY, compose(A, B), bar_base[k]).S[2] = lv * Lb / 1000.f; }
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; M T = k ? vt_basis(0.f, -stick_y * .3f, stick_x * .3f) : vt_basis(0.f, -(ply.power - .5f) * .6f, stick_lx * .3f); top(k ? M_STICKR : M_STICKL, compose(A, T), V(q * 540, 2950, 1680)); }
    /* Octavius t1113-t1138 has the screen dark about t1120.5-1121.5, t1122.5-1125, t1129-1129.5, only in the fight, irregular, cause unknown. Our old rule (a 3.5 s timer whenever an enemy was near) was invented and read as a flicker: Bobby 11-03. Steady now. VTOL_MAPDARK=1 replays the footage's spells once from the first enemy near */
    static float dark_t = -1.f; bool dark = false;
    if (map_dark_env) { if (dark_t < 0.f && near_e) dark_t = (float)sim_t; if (dark_t >= 0.f) { float q = (float)sim_t - dark_t; dark = (q < 1.f) || (q > 2.f && q < 4.5f) || (q > 8.5f && q < 9.f); } }
    { V sc = vt_stage_c(); mark_on = rf == RF_NONE && (wave >= 2 || final_wave); mark_x = sc.x; mark_y = sc.y; }
    vt_map_update(dark, ply.p.x, ply.p.y, ply.yaw); top(M_MAP, A, V());
}
static void vt_build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f;
    f.n_lights = 1; f.lights[0] = {0.65f, 0.35f, {-0.218f, -0.437f, 0.874f}};
    const bool menu = st == S_MENU;
    if (menu) { f.clear[0] = 0; f.clear[1] = 0; f.clear[2] = 0; f.fog_end = 0; }
    else { f.clear[0] = f.fog_col[0] = 166; f.clear[1] = f.fog_col[1] = 176; f.clear[2] = f.fog_col[2] = 226; f.fog_start = 260000.f; f.fog_end = 460000.f; }
    M I, cam; V eye; bool cockpit = false;
    static const char *fixed = getenv("VTOL_CAM");
    const bool fixed_cam = fixed && *fixed && !menu;
    if (menu) {
        eye = mp(0, -30, 3); cam = look_at(eye, mp(0, 0, 0));
    } else {
        if (gap_test) { f.clear[0] = 255; f.clear[1] = 0; f.clear[2] = 255; f.fog_end = 0; }
        if (!gap_test) { float sx0 = ply.p.x, sy0 = ply.p.y; if (fixed) sscanf(fixed, "%f,%f", &sx0, &sy0); float sx = roundf(sx0 / 240.f) * 240.f, sy = roundf(sy0 / 240.f) * 240.f; dput(f, M_SEA, I, mp(sx, sy, 0)); dput(f, M_LAND, I, V()); dput(f, M_ISLE, I, V()); }   /* the sea's tiles come along with you */
        if (!gap_test) dput(f, M_CARRIER, I, mp(CAR_X, CAR_Y, 0));
        if (!gap_test) dput(f, M_FRIGATE, d1k::yawed(ship.yaw), mp(ship.x, ship.y, SEA_Z));
        if (!gap_test) dput(f, M_FRIGATE, d1k::yawed(.6f), mp(CAR_X + 170.f, CAR_Y + 130.f, SEA_Z));   /* a frigate in sight of the carrier: BBC speech t337-343, "I can see the frigate over there, and ... the aircraft carrier" */
        dput(f, M_JETB, d1k::yawed(0.f), mp(-36, -92, 1.2f)); dput(f, M_JETR, d1k::yawed(-1.2f), mp(40, -79, 1.2f));
        dput(f, M_POST, I, mp(82, -43, 0));
        /* the dark helicopter: BBC t266 at the horizon, t290 near, t351-t361 out over the sea with the carrier. Only ever seen far off; its path and that it cannot be shot are ours */
        { float ha = (float)sim_t * 0.11f, hx = 120.f + cosf(ha) * 240.f, hy = -40.f + sinf(ha) * 240.f; VPIX_Instance &hi = dput(f, M_HELI, d1k::yawed(ha), mp(hx, hy, 48.f + 8.f * sinf(ha * 3.f))); (void)hi; dput(f, M_ROTOR, d1k::yawed((float)sim_t * 9.f), mp(hx, hy, 48.f + 8.f * sinf(ha * 3.f))); }
        for (const Tgt &g : hq) if (g.alive && g.mesh) dput(f, g.mesh, I, mp(g.x, g.y, g.mesh == M_FRIGATE ? SEA_Z : 0.f));
        if (rf != RF_NONE && tank_gone <= 0.f) { M TB = vt_basis(atan2f(-tank_f.x, tank_f.y), 0.f, 0.f); V tm = mp(tank_p.x, tank_p.y, tank_p.z); dput(f, M_TANKER, TB, tm); V dr = vt_drogue(); dput(f, rf == RF_LINK ? M_LAMPG : M_LAMPA, TB, tm).bright = 1; if (hose_gone <= 0.f && rf_reel > .01f) { V ha = vt_hdu(), hb = dr + tank_f * 1.85f, hm = (ha + hb) * .5f + V(0, 0, -.5f * rf_reel); const V seg[3] = {ha, hm, hb};   /* the line, in two lengths with a little sag, then the cup */
                for (int k = 0; k < 2; k++) { V p0 = seg[k] * 1000.f, p1 = seg[k + 1] * 1000.f; VPIX_Instance &hq_ = dput(f, M_HOSE, look_at(p0, p1), p0); hq_.S[1] = len(seg[k + 1] - seg[k]); }
                dput(f, M_DROGUE, TB, mp(dr.x, dr.y, dr.z)); } }
        const bool play = st == S_PLAY || st == S_OVER;
        if (play) {
            for (const Enemy &e : en) if (e.alive) dput(f, skill == 3 ? M_JETG : M_ENEMY, vt_basis(e.yaw, e.pitch, e.roll), mp(e.p.x, e.p.y, e.p.z));
            for (int gi = 0; gi < 4; gi++) if (v1knet::on && ghost[gi].show && !(slave_game && gi == v1knet::my_seat)) dput(f, M_HARRIER, ghost[gi].R, mp(ghost[gi].p.x, ghost[gi].p.y, ghost[gi].p.z + 1.6f));   /* the other pilots' jets */
            for (const Bul &b : buls) dput(f, b.kind == 1 ? M_MIS : b.kind == 2 ? M_BOMB : b.mine ? M_BUL : M_EBUL, look_at(b.p * 1000.f, (b.p + b.v) * 1000.f), b.p * 1000.f).bright = 1;
        }
        M A = ply.R;
        if (fixed_cam) {
            float a[6] = {0, -60, 25, 0, 0, 5}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5);
            eye = mp(a[0], a[1], a[2]); cam = look_at(eye, mp(a[3], a[4], a[5]));
            if (play && crash_t < 0.f) dput(f, M_HARRIER, A, mp(ply.p.x, ply.p.y, ply.p.z + 1.6f));
        } else if (rf_chase && st == S_PLAY && rf != RF_NONE && crash_t < 0.f && tank_gone <= 0.f) {   /* dev: a chase view of the refuelling, for pictures */
            V at = vt_drogue(); V rr(tank_f.y, -tank_f.x, 0); V e = at - tank_f * 16.f + rr * 13.f + V(0, 0, 4.5f); eye = mp(e.x, e.y, e.z); cam = look_at(eye, mp(at.x, at.y, at.z));
            dput(f, M_HARRIER, A, mp(ply.p.x, ply.p.y, ply.p.z + 1.6f));
        } else if (st == S_TITLE) {   /* attract: a slow flyover of the airfield */
            float a = (float)sim_t * 0.12f; eye = mp(sinf(a) * 170.f, -cosf(a) * 170.f, 55.f); cam = look_at(eye, mp(0, 20, 10));
        } else if (crash_t >= 0.f) {   /* the wreck, from behind and above */
            eye = crash_eye * 1000.f; cam = look_at(eye, crash_at * 1000.f);
        } else {
            cam = A; eye = mp(ply.p.x, ply.p.y, ply.p.z) + to_world(A, V(0, 3000.f, 2200.f)); cockpit = true;
            static const char *lk = getenv("VTOL_LOOK"); if (lk && *lk) { float ya = 0, pi = 0; if (!strcmp(lk, "sweep")) { static const float SW[][2] = {{0,-10},{25,-10},{-25,-10},{45,-15},{-45,-15},{60,-20},{-60,-20},{75,0},{-75,0},{90,-25},{-90,-25},{90,-45},{-90,-45},{110,-25},{-110,-25},{135,-30},{-135,-30},{160,-20},{-160,-20},{180,-30},{30,-45},{-30,-45},{60,-50},{-60,-50},{0,-60},{120,-60},{-120,-60},{0,40},{70,30},{-70,30},{150,20},{-150,20}}; const int nsw = sizeof SW / sizeof SW[0]; int q = (int)(sim_t / 3.f) % nsw; ya = SW[q][0]; pi = SW[q][1]; } else sscanf(lk, "%f,%f", &ya, &pi); ya *= PI_F / 180.f; pi *= PI_F / 180.f;   /* dev: a fixed look round the cockpit for pictures */
                V fw = cam.r[1], ri = cam.r[0], up = cam.r[2]; V f2 = fw * cosf(ya) + ri * sinf(ya), r2 = ri * cosf(ya) - fw * sinf(ya); V f3 = f2 * cosf(pi) + up * sinf(pi), u3 = up * cosf(pi) - f2 * sinf(pi); cam.r[0] = r2; cam.r[1] = f3; cam.r[2] = u3; }
            dput(f, M_COCKPIT, A, mp(ply.p.x, ply.p.y, ply.p.z)).on_top = 2;
            { VPIX_Instance &li = dput(f, M_COCKL, A, mp(ply.p.x, ply.p.y, ply.p.z)); li.on_top = 2; li.bright = 1; }
            vt_cockpit_live(f, A, mp(ply.p.x, ply.p.y, ply.p.z));
            if (rf == RF_COME || rf == RF_LINK || nose_always) dput(f, M_NEEDLE, A, mp(ply.p.x, ply.p.y, ply.p.z));
        }
        if (play) for (const Fx &x : fxs) {
            if (x.kind == 0) { float u = x.t / x.life, sz = (x.maxi >= 3 ? 16.f : 10.f) * (0.45f + 0.75f * (std::min)(1.f, u * 2.5f)) * (u > 0.8f ? (1.f - u) * 5.f : 1.f); VPIX_Instance &q = dput(f, M_FIRE0 + ((int)(x.t * 7.f) & 1), look_at(x.p * 1000.f, eye), x.p * 1000.f); q.S[0] = q.S[2] = sz; q.bright = 1; }
            else if (x.kind == 1) dput(f, M_DEBRIS, x.R, x.p * 1000.f).bright = 1;
            else if (x.kind == 2) dput(f, x.mesh, x.R, x.p * 1000.f);
            else dput(f, x.mesh, I, x.p * 1000.f).bright = 1;
        }
    }
    /* the seat reference: where you face when the round starts is straight ahead (as Buggy Ball) */
    if (in.head.valid && !have_ref) { head_ref = V(in.head.p[0], in.head.p[1], in.head.p[2]); yaw0 = atan2f(-in.head.R[1][0], in.head.R[1][1]); have_ref = true; }
    float cy = cosf(yaw0), sy = sinf(yaw0);
    M cr; cr.r[0] = cam.r[0] * cy - cam.r[1] * sy; cr.r[1] = cam.r[0] * sy + cam.r[1] * cy; cr.r[2] = cam.r[2];
    if (in.head.valid && in.vr && have_ref && cockpit) {
        V dh = V(in.head.p[0], in.head.p[1], in.head.p[2]) - head_ref, dw = to_world(cr, dh) * world_scale;
        if (len(dw) > 900.f) dw = norm(dw) * 900.f;
        eye += dw;
    }
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cr.r[k].x; f.cam_R[k][1] = cr.r[k].y; f.cam_R[k][2] = cr.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    f.in_head = false;

    const float CX = 176.f;
    if (st == S_TITLE) {
        vt_textc(f, "VTOL", CX, 70.f, 3.f, 1);
        if (fmodf((float)sim_t, 1.2f) < 0.8f) vt_textc(f, "PRESS TRIGGER", CX, 200.f, 1.f, 0);
    } else if (menu) {
        /* layout measured on Octavius t1109-t1111, in the 352 x 288 screen */
        if (zoom_t >= 0.f) { float u = clampf(zoom_t / 0.25f, 0.f, 1.f), x0 = 129.f + (30.f - 129.f) * u, y0 = 74.f + (13.f - 74.f) * u, x1 = 298.f + (315.f - 298.f) * u, y1 = 254.f + (274.f - 254.f) * u; vt_blit(f, B_PIC + skill, VT_PW, VT_PH, x0, y0, x1 - x0, y1 - y0); }
        else {
            vt_blit(f, B_BG, 176, 144, 0, 0, 352.f, 288.f);
            const int bid[3] = {B_BTN_GREY, B_BTN_YEL, B_BTN_ORG}; const float bx[3] = {48.f, 142.f, 235.f};
            for (int i = 0; i < 3; i++) { bool fl = flash_btn == i && flash_t > 0.f; vt_blit(f, bid[i], VT_BW, VT_BH, bx[i] - (fl ? 2.f : 0.f), 5.f - (fl ? 2.f : 0.f), 74.f + (fl ? 4.f : 0.f), 45.f + (fl ? 4.f : 0.f)); }
            const float ty[3] = {71.f, 136.f, 200.f}; for (int i = 0; i < 3; i++) vt_blit(f, B_THUMB + (skill + 1 + i) % 4, VT_TW, VT_TH, 58.f, ty[i], 54.f, 58.f);   /* the list shows the next three pages (t1108 vs t1110) */
            vt_blit(f, B_PIC + skill, VT_PW, VT_PH, 129.f, 74.f, 169.f, 180.f);
            int marks = (int)ceilf(14.f * clampf(menu_left / MENU_T, 0.f, 1.f)); for (int i = 0; i < marks; i++) vt_blit(f, B_MARK, 4, 8, 98.f + i * 4.f, 266.f, 3.f, 9.f); }
    } else {
        vt_hud(f, in);
        if (st == S_OVER) {
            { VPIX_Blit b = {}; b.handle = d1k::BMPH + B_BLACK; b.sw = 4; b.sh = 4; b.x = 0; b.y = 0; b.dw = 352.f; b.dh = 288.f; b.key = -1; b.alpha = clampf(st_t / 2.f, 0.f, 1.f); b.prio = 1000000; b.tint[0] = b.tint[1] = b.tint[2] = 1.f; f.blits.push_back(b); }   /* "everything swims out of view" (Your Commodore 79) */
            char b2[32]; snprintf(b2, sizeof b2, "SCORE %d", score);
            vt_textc(f, over_msg, CX, 96.f, 1.6f, 1); vt_textc(f, b2, CX, 134.f, 1.f, 0);
        }
    }
    d1k::look_test(f);
    d1k::hs::draw(f, "vtol", st == S_TITLE, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at the machine's rate ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 1; static unsigned long frames = 0;
/* ---- link play (vpix_1k_net.h): see the data block before the effects. Master = pod 1; any other pod flies its own jet and draws the master's world ---- */
static V net_v3(const float *o) { return V(o[0], o[1], o[2]); }
static void net_f3(float *o, V a) { o[0] = a.x; o[1] = a.y; o[2] = a.z; }
static void net_log() {
    static double next = 0; if (sim_t < next) return; next = sim_t + 5.0;
    const bool inround = slave_game || (v1knet::master && (st == S_PLAY || st == S_OVER));
    if (!inround) { dl("t=%.1f link: %s, not in a round, pods heard %d", sim_t, v1knet::master ? "master" : "pod", v1knet::live_pods()); return; }
    const int me = slave_game ? v1knet::my_seat : 0; char sb[4][48];
    for (int k = 0; k < 4; k++) { if (k == me) snprintf(sb[k], sizeof sb[k], "you(%.0f %.0f %.0f)%s", ply.p.x, ply.p.y, ply.p.z, crash_t >= 0.f ? " crashed" : "");
        else if (ghost[k].show) snprintf(sb[k], sizeof sb[k], "(%.0f %.0f %.0f)", ghost[k].p.x, ghost[k].p.y, ghost[k].p.z); else snprintf(sb[k], sizeof sb[k], "-"); }
    dl("t=%.1f link: %s seat %d st %d wave %d %d/%d score %d | s0 %s | s1 %s | s2 %s | s3 %s | enemies %zu bullets %zu", sim_t, v1knet::master ? "master" : "pod", me, (int)st, wave, kills, per_wave, score, sb[0], sb[1], sb[2], sb[3], en.size(), buls.size());
}
static void net_master_in() {
    for (int s = 1; s < 4; s++) { Remote &r = rem[s]; const NetIn *p = (const NetIn *)v1knet::seat_input(s, sizeof(NetIn));
        if (!p || (st != S_PLAY && st != S_OVER)) { if (r.active) { r = Remote(); ghost[s].show = false; dl("t=%.1f link: seat %d is empty", sim_t, s); } continue; }
        if (!r.active) { r = Remote(); r.active = true; r.fire_seq = p->fire_seq; r.msl_seq = p->msl_seq; r.crash_seq = p->crash_seq; dl("t=%.1f link: seat %d is a person now", sim_t, s); }
        r.a.p = net_v3(p->p); r.a.R.r[1] = net_v3(p->f); r.a.R.r[2] = net_v3(p->u); r.a.R.r[0] = cross(r.a.R.r[1], r.a.R.r[2]); r.a.speed = p->speed;
        r.crashed = (p->flags & 1) != 0; r.landed = (p->flags & 2) != 0;
        const int nf = (uint16_t)(p->fire_seq - r.fire_seq), nm = (uint16_t)(p->msl_seq - r.msl_seq); r.fire_seq = p->fire_seq; r.msl_seq = p->msl_seq;
        if (!r.crashed && st == S_PLAY) { for (int k = 0; k < nf && k < 3; k++) vt_player_fire(r.a, s); if (nm > 0) vt_weapon2(r.a, vt_lock_of(r.a), s); }
        if (p->crash_seq != r.crash_seq) { r.crash_seq = p->crash_seq; ev_src = s; vt_explode(r.a.p + V(0, 0, 2), 3, 2.4f); ev_src = 0; dl("t=%.1f link: seat %d crashed", sim_t, s); }
        r.fuelling = rf != RF_NONE && tank_gone <= 0.f && hose_gone <= 0.f && rf_reel >= .9f && !r.crashed && len(r.a.p + r.a.R.r[1] * TIP_Y + r.a.R.r[2] * TIP_Z - vt_drogue()) < 4.5f;
        ghost[s].show = !r.crashed; ghost[s].p = r.a.p; ghost[s].R = r.a.R; }
}
static void net_master_out() {
    if (!v1knet::due(30)) return;
    if (st != S_PLAY && st != S_OVER) { v1knet::send_snapshot(NULL, 0); return; }
    static NetSt s; memset(&s, 0, sizeof(s));
    s.st = (uint8_t)st; s.skill = (uint8_t)skill; s.wave = (uint8_t)wave; s.kills = (uint8_t)(std::min)(kills, 255); s.flags = (final_wave ? 1 : 0) | (tank_gone > 0.f ? 2 : 0) | (hose_gone > 0.f ? 4 : 0) | (strcmp(over_msg, "MISSION COMPLETE") == 0 ? 8 : 0);
    s.rf = (uint8_t)rf; for (int i = 0; i < 5; i++) if (hq[i].alive) s.hq |= (uint8_t)(1 << i);
    s.time_left = time_left; s.reel = rf_reel; net_f3(s.tank_p, tank_p); s.tank_f[0] = tank_f.x; s.tank_f[1] = tank_f.y; s.ship[0] = ship.x; s.ship[1] = ship.y; s.ship[2] = ship.yaw;
    { NetPl &n = s.pl[0]; net_f3(n.p, ply.p); net_f3(n.f, ply.R.r[1]); net_f3(n.u, ply.R.r[2]); n.speed = ply.speed; n.flags = 1 | (crash_t >= 0.f ? 2 : 0) | (rf == RF_LINK ? 4 : 0); n.score = (int16_t)(std::min)(score, 32767); }
    for (int i = 1; i < 4; i++) { const Remote &r = rem[i]; NetPl &n = s.pl[i]; if (!r.active) continue;
        net_f3(n.p, r.a.p); net_f3(n.f, r.a.R.r[1]); net_f3(n.u, r.a.R.r[2]); n.speed = r.a.speed; n.flags = 1 | (r.crashed ? 2 : 0) | (r.fuelling ? 4 : 0); n.hits = r.hits; n.score = (int16_t)(std::min)(r.score, 32767); }
    memcpy(s.ev, ev_ring, sizeof(s.ev)); s.ev_top = ev_id;
    for (const Enemy &e : en) { if (!e.alive || s.nen >= 8) continue; NetEn &n = s.en[s.nen++]; net_f3(n.p, e.p); n.yaw = e.yaw; n.pitch = e.pitch; n.roll = e.roll; n.id = (uint16_t)e.id; n.flags = (e.chaser ? 1 : 0) | (e.slow ? 2 : 0); }
    for (int pass = 0; pass < 2; pass++) for (const Bul &b : buls) { if (s.nbl >= 36 || ((b.kind != 0 || !b.mine) != (pass == 0))) continue;   /* missiles, bombs and enemy fire first, then the tracers */
        NetBl &n = s.bl[s.nbl++]; net_f3(n.p, b.p); const float bv[3] = {b.v.x, b.v.y, b.v.z}; for (int k = 0; k < 3; k++) n.v[k] = (int16_t)clampf(bv[k] * 10.f, -32000.f, 32000.f); n.flags = (uint8_t)(b.kind | (b.mine ? 4 : 0)); }
    v1knet::send_snapshot(&s, offsetof(NetSt, bl) + s.nbl * sizeof(NetBl));
}
static void net_leave(const char *why) {
    slave_game = false; st = S_TITLE; st_t = 0; en.clear(); buls.clear(); fxs.clear(); rf = RF_NONE; final_wave = false; tank_gone = hose_gone = 0.f; crash_t = -1.f; for (Ghost &g : ghost) g.show = false; vt_respawn();
    dl("t=%.1f link: %s, back to my own game", sim_t, why);
}
static void net_apply(const NetSt &s, uint8_t round) {
    static uint8_t my_round = 0; const bool fresh = !slave_game || round != my_round; my_round = round; const int me = v1knet::my_seat;
    if (fresh) { VPIX_Input none; vt_start_round(none); slave_game = true; ev_seen = s.ev_top; net_hits_seen = s.pl[me].hits; net_fire_seq = net_msl_seq = net_crash_seq = 0; vt_snd(2, SND_GO, 100); dl("t=%.1f link: in the master's round, seat %d", sim_t, me); }
    skill = (int)clampf((float)s.skill, 0.f, 3.f); wave = s.wave; kills = s.kills; final_wave = (s.flags & 1) != 0; st = (State)s.st; over_msg = (s.flags & 8) ? "MISSION COMPLETE" : "TIME UP"; time_left = s.time_left;
    rf = s.rf; rf_reel = s.reel; tank_p = net_v3(s.tank_p); tank_f = V(s.tank_f[0], s.tank_f[1], 0); tank_gone = (s.flags & 2) ? 1.f : 0.f; hose_gone = (s.flags & 4) ? 1.f : 0.f;
    ship.x = s.ship[0]; ship.y = s.ship[1]; ship.yaw = s.ship[2]; for (int i = 0; i < 5; i++) hq[i].alive = ((s.hq >> i) & 1) != 0;
    score = s.pl[me].score; net_fuel = (s.pl[me].flags & 4) != 0;
    for (int k = 0; k < 4; k++) { const NetPl &n = s.pl[k]; Ghost &g = ghost[k]; g.show = (n.flags & 1) && !(n.flags & 2); if (!g.show) continue; g.p = net_v3(n.p); g.R.r[1] = net_v3(n.f); g.R.r[2] = net_v3(n.u); g.R.r[0] = cross(g.R.r[1], g.R.r[2]); }
    en.clear(); for (int i = 0; i < s.nen && i < 8; i++) { const NetEn &n = s.en[i]; Enemy e; e.alive = true; e.p = net_v3(n.p); e.yaw = n.yaw; e.pitch = n.pitch; e.roll = n.roll; e.id = n.id; e.chaser = (n.flags & 1) != 0; e.slow = (n.flags & 2) != 0; en.push_back(e); }
    buls.clear(); for (int i = 0; i < s.nbl && i < 36; i++) { const NetBl &n = s.bl[i]; Bul b; b.p = net_v3(n.p); b.v = V(n.v[0] * .1f, n.v[1] * .1f, n.v[2] * .1f); b.kind = n.flags & 3; b.mine = (n.flags & 4) != 0; buls.push_back(b); }
    if ((uint8_t)(s.ev_top - ev_seen) > 8) ev_seen = s.ev_top;
    while (ev_seen != s.ev_top) { const uint8_t want = (uint8_t)(ev_seen + 1); const NetEv &e = s.ev[want & 7]; ev_seen = want; if (e.id == want && e.src != me) { vt_explode(net_v3(e.p), e.maxi, e.life10 / 10.f); vt_snd(4, SND_BOOM, 100); } }
    const int nh = (uint8_t)(s.pl[me].hits - net_hits_seen); net_hits_seen = s.pl[me].hits;   /* the master says how many shots found you */
    if (nh > 0 && crash_t < 0.f && st == S_PLAY) { ply.dmg += SK().edmg * (float)nh; vt_snd(7, SND_HIT, 100); if (ply.dmg >= 100.f) vt_crash("shot down"); }
}
static void net_slave_step(const Ctl &c, float dt) {
    stick_x = c.sx; stick_y = c.sy;
    if (crash_t >= 0.f) { crash_t += dt; if (crash_t > crash_hold) { crash_hold = 3.f; vt_respawn(); } }
    else if (st == S_PLAY) {
        vt_fly(c, dt);
        if (crash_t < 0.f) {   /* the guns: the pod counts shots and the master spawns them */
            Plane &a = ply; vt_lock(); if (fire_cool > 0.f) fire_cool -= dt; if (missile_cool > 0.f) missile_cool -= dt;
            if (c.fire && a.gun > 0.f && rf != RF_LINK) { a.gun = (std::max)(0.f, a.gun - gun_drain * dt); if (fire_cool <= 0.f) { fire_cool = fire_gap; net_fire_seq++; vt_snd(1, SND_GUN, 80); } }
            else a.gun = (std::min)(1.f, a.gun + gun_fill * dt);
            static bool m_was = false; if (c.missile && !m_was && missile_cool <= 0.f && !a.ground && rf != RF_LINK) { net_msl_seq++; missile_cool = missile_gap; vt_snd(1, lock_id ? SND_GO : SND_MENU, 90); } m_was = c.missile;
            if (net_fuel) a.fuel = (std::min)(100.f, a.fuel + 18.f * dt); } }
    for (size_t i = 0; i < fxs.size();) { Fx &x = fxs[i]; x.t += dt;
        if (x.kind == 1 || x.kind == 2) { x.v.z -= grav * dt; x.p += x.v * dt; buggy::rotate(x.R, x.w, dt); }
        else if (x.kind == 3) { x.v.z -= grav * .25f * dt; x.p += x.v * dt; }
        if (x.t >= x.life || x.p.z < SEA_Z - 2.f) { fxs[i] = fxs.back(); fxs.pop_back(); } else i++; }
}
/* true = this tick was a linked pod's, inside the master's game */
static bool net_tick(double el, const VPIX_Input &in, const Ctl &c) {
    v1knet::pump();
    if (v1knet::master) { net_master_in(); net_master_out(); net_log(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n >= offsetof(NetSt, bl) && v1knet::my_seat >= 0) { static NetSt s; memset(&s, 0, sizeof(s)); memcpy(&s, d, (std::min)(n, sizeof(s)));
            const size_t room = (n - offsetof(NetSt, bl)) / sizeof(NetBl); if (s.nbl > room) s.nbl = (uint8_t)room;
            if (s.st == S_PLAY || s.st == S_OVER) net_apply(s, v1knet::round_id); else if (slave_game) net_leave("the master's round is over"); }
        else if (slave_game) net_leave("the master's round is over");
    }
    if (slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
    net_log();
    if (!slave_game) { v1knet::send_input(NULL, 0); return false; }   /* not in the master's round yet: play my own, the master hears me and gives me a seat */
    int kind; vt_surface(ply.p.x, ply.p.y, kind); static int last_crashes = 0; if (crashes != last_crashes) { last_crashes = crashes; net_crash_seq++; }
    NetIn o; memset(&o, 0, sizeof(o)); net_f3(o.p, ply.p); net_f3(o.f, ply.R.r[1]); net_f3(o.u, ply.R.r[2]); o.speed = ply.speed; o.fire_seq = net_fire_seq; o.msl_seq = net_msl_seq; o.crash_seq = net_crash_seq;
    o.flags = (crash_t >= 0.f ? 1 : 0) | ((ply.ground && final_wave && rf == RF_NONE && (kind == 2 || kind == 3) && ply.speed < 3.f && crash_t < 0.f) ? 2 : 0);
    v1knet::send_input(&o, sizeof(o));
    d1k::hs::run("vtol", st == S_OVER && st_t > 2.f, score, in, c.sel, (float)el, autoplay);
    const float dt = 1.f / 120.f; int k = 0;
    while (acc >= dt && k < 24) { net_slave_step(autoplay ? vt_auto() : c, dt); acc -= dt; sim_t += dt; st_t += dt; k++; }
    vt_engine_sound(st == S_PLAY && crash_t < 0.f && ply.fuel > 0.f, clampf(0.25f + 0.75f * ply.power, 0.f, 1.f), 6.f * ply.power);
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); vt_build_frame(++frames, in); }
    return true;
}
static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    Ctl c = vt_controls(in);
    if (v1knet::on && net_tick(el, in, c)) { PIC_AddEvent(tick, 2.0); return; }   /* a linked pod inside the master's round */
    static bool sel_was = false, l_was = false, r_was = false;
    bool sel = c.sel && !sel_was, left = c.sx < -0.5f && !l_was, right = c.sx > 0.5f && !r_was;
    sel_was = c.sel; l_was = c.sx < -0.5f; r_was = c.sx > 0.5f;
    if (autoplay && ((st == S_TITLE && st_t > 1.5f) || (st == S_MENU && st_t > 1.5f) || (st == S_OVER && st_t > 3.5f))) sel = true; if (d1k::hs::run("vtol", st == S_OVER && st_t > 2.f, score, in, c.sel, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (flash_t > 0.f) flash_t -= (float)el;
    switch (st) {
    case S_TITLE: if (sel && st_t > 0.3f) { st = S_MENU; st_t = 0; menu_left = MENU_T; zoom_t = -1.f; vt_snd(2, SND_MENU, 90); } break;
    case S_MENU:
        if (zoom_t >= 0.f) { zoom_t += (float)el; if (zoom_t > 1.4f) { zoom_t = -1.f; vt_start_round(in); st = S_PLAY; st_t = 0; } break; }
        menu_left -= (float)el;
        if (left) { skill = (skill + 3) % 4; flash_btn = 0; flash_t = 0.15f; vt_snd(2, SND_MENU, 90); }   /* the marks do not reset on a click: 12 on CADET t1109, 10 on the next page t1110 */
        if (right) { skill = (skill + 1) % 4; flash_btn = 1; flash_t = 0.15f; vt_snd(2, SND_MENU, 90); }
        if ((sel && st_t > 0.3f) || menu_left <= 0.f) { flash_btn = 2; flash_t = 0.2f; zoom_t = 0.f; vt_snd(2, SND_GO, 100); }
        break;
    case S_OVER: if (sel && st_t > 3.f) { st = S_TITLE; st_t = 0; en.clear(); buls.clear(); fxs.clear(); v1knet::end_round(); } break;
    default: break;
    }
    int n = 0;
    while (acc >= dt && n < 24) {
        if (st == S_PLAY) vt_step(autoplay ? vt_auto() : c, dt);
        else if (st == S_OVER) { for (size_t i = 0; i < fxs.size();) { Fx &x = fxs[i]; x.t += dt; if (x.kind == 1 || x.kind == 2) { x.v.z -= grav * dt; x.p += x.v * dt; buggy::rotate(x.R, x.w, dt); } else if (x.kind == 3) { x.v.z -= grav * .25f * dt; x.p += x.v * dt; }
                if (x.t >= x.life) { fxs[i] = fxs.back(); fxs.pop_back(); } else i++; } }
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    vt_engine_sound(st == S_PLAY && crash_t < 0.f && ply.fuel > 0.f, clampf(0.25f + 0.75f * ply.power, 0.f, 1.f), 6.f * ply.power);
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); vt_build_frame(++frames, in); }
    if (sim_t >= next_log) { next_log = sim_t + 2.0;
        dl("t=%.0f st %d | pos (%.0f %.0f %.0f) spd %.0f vz %.1f yaw %.0f pwr %.2f ground %d fuel %.0f dmg %.0f gun %.2f rf %d | wave %d %d/%d score %d enemies %zu crashes %d left %.0f",
           sim_t, (int)st, ply.p.x, ply.p.y, ply.p.z, ply.speed, ply.vz, ply.yaw * 57.3f, ply.power, ply.ground, ply.fuel, ply.dmg, ply.gun, rf, wave, kills, per_wave, score, en.size(), crashes, time_left); }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".vtol").c_str(), "w");
    game_fps = envf("VTOL_FPS", game_fps); game_len = envf("VTOL_TIME", game_len); waves_total = (int)envf("VTOL_WAVES", (float)waves_total); per_wave = (std::max)(1, (int)envf("VTOL_PERWAVE", (float)per_wave));
    enemies_override = (int)envf("VTOL_ENEMIES", 0.f); v_max = envf("VTOL_SPEED", v_max); climb_rate = envf("VTOL_CLIMB", climb_rate); turn_rate = envf("VTOL_TURN", turn_rate);
    roll_rate = envf("VTOL_ROLL", roll_rate); pitch_rate = envf("VTOL_PITCH", pitch_rate); fuel_burn = envf("VTOL_BURN", fuel_burn);
    refuel_rate = envf("VTOL_REFUEL", refuel_rate);
    land_assist = envf("VTOL_LANDASSIST", 1.f) != 0.f; aim_cone_deg = envf("VTOL_AIM", 0.f); banners = envf("VTOL_BANNERS", 0.f) != 0.f;
    hand_from = (int)envf("VTOL_HAND_FROM", (float)hand_from); rf_test = (int)envf("VTOL_RFTEST", 0.f); rf_chase = envf("VTOL_CHASE", 0.f) != 0.f; nose_always = envf("VTOL_NOSE", 0.f) != 0.f; map_dark_env = envf("VTOL_MAPDARK", 0.f) != 0.f; gap_test = envf("VTOL_GAPTEST", 0.f) != 0.f; sit_s = envf("VTOL_SIT", 0.f); skill = (int)clampf(envf("VTOL_SKILL", 0.f), 0.f, 3.f); autoplay = envf("VTOL_AUTO", 0.f) != 0.f;
    v1knet::init("VTOL", true, dl);   /* join in progress: a pod plays the waves alone until the master's game gives it a seat (Bobby) */
    vt_build_assets(); vt_make_hud(); vt_make_sounds(); vt_map_init();
    ply = Plane(); ply.p = V(0, -100.f, 0); ply.R = vt_basis(0.f, 0.f, 0.f);
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    PIC_AddEvent(tick, 2.0);
    dl("VTOL rebuild started");
}
}   /* namespace vtol */
