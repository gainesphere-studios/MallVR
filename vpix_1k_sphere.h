/* Battlesphere (Virtuality 1000SD sit-down, 1991) - native rebuild from footage and press. No 1000-series software survives in public, so every model is
   built here and no game file is read. Included after vpix_dactyl1k.h (uses d1k:: and buggy::). Dev only, not in a release.
   Evidence and reasoning: Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\battlesphere.md. Footage: Octavius King 2YrPW5cU06A t941-t969, the only in-game footage (about 28 s, parts shown twice).
   Octavius zoomed this clip to fill 16:9: his picture is our 640 x 480 minus 44 lines at the top and 76 at the bottom, so frame fractions go through F() and HUD_Y; what he cut off is not known.
   FROM FOOTAGE (frame by frame, 25 fps pull): hollow green digits; clock starts 180 and drops 1 a second (129-126, 166-159 timed); it stood at 180 for at least 12.4 s of a pass over the base; score in tens, every kill seen = +10; one game in the clips (180/190, 166/200, 161/210, 130/230, 128/240).
   Comb, five bow ties at the right and one at the left never change. Reticle fixed in the view (three nested corners) with four outward triangles that blink in step with the yellow square on the right panel, with or without a target near the sights.
   Both pods and every panel; left tall panel: a fixed double dash with four small corner marks, five teeth growing under it once. Left monitor: static, or a black scope with a blip and a blue band sweeping top to bottom every 2.4-3.6 s (not linked to the base as first thought). Right: EJECT, icon rows, or static.
   Orange bar: dim slots, brightness spreading out from its middle. Meter: green core and orange sides, each side changing on its own. Yellow stick shots from under the nose into the sights about every 0.36 s. Red missiles in pairs passing over the seat, gone in 0.6 s, each ending in a fireball with no score (t946.9-t947.5; first read as "red capsule enemy ships"; that they are missiles is a reading).
   HYPERSPACE (t959.40 and again t962.68, one event shown twice; that it is hyperspace is a reading): background strobes red, yellow, red, yellow, red for 0.44 s, the view jumps at the first yellow, shots keep flying; both monitors go to static for 5.8 s; the frog waves on the left for about 1.2 s starting 1.3 s after; the teeth at 2.9 s; bar brightens over about 3 s.
   Blue ship: a faceted barrel, two pale pods on struts with red-orange ends, two gold prongs. Rocks: big chamfered facets, a pale side, a round green patch and small ones. Fireball: cream with an orange lumpy edge and bits of the wreck in it, about 1.4 s, then a cloud of specks. Lattice with three-way junctions, grey base with bay, yellow bands, white arrows, green block.
   FROM PERIOD PRESS: Computer+Videogiochi 9 Oct 1991: enormous geodesic sphere, asteroid showers, four levels Solo Trainer, Solo Ace, Dogfight, Battlesphere; controls like VTOL, upper right button selects weapons; sight tied to the helmet. MC microcomputer 127 Mar 1993: cannon and gaze-guided missiles, destroy a "Death Star" by dropping its force field and flying into the cooling ducts of the central reactor.
   Amiga Format 23 Jun 1991: fleets trapped in force fields, guns follow the helmet. The One 33: take out an enemy battleship. W.descrip1: shot types, hyperspace, chaos charge, swarms, tail-chasers, practice in asteroids before the motherships. Your Commodore 79: stereo guitar-rock soundtrack. Simon Marston: die and continue, unlimited credits.
   INVENTED, no source: what each level holds (who is where, how many), all speeds and numbers, field generators as the way to drop the force field, the shape of battleship and duct, friendly ships' colour, enemy shots and how hurt shows, rocks taking 3 hits and +10, why the clock stands (here: within 80 m of the base), what the meter, bar, teeth, blink and blips mean, chaos charge's look, how level is chosen, the keys beyond the press's two. All sounds are placeholders; the riff is ours, only "guitar rock" is sourced. The voice is not recoverable. Dogfight runs against computer pilots here; the real one was linked cabinets.
   NOT KNOWN: how score reached 190 with the clock on 180 (a continue that keeps score is one reading), whether the base is ours.
   Dev: SPHERE_PIN=x,y,z,yawdeg,pitchdeg, SPHERE_COMM=0|1|2, SPHERE_MODE=0-3, SPHERE_GEN_HP, SPHERE_DUCT=1 (field down from the start).
   Units: mm, seconds. World X right, Y forward, Z up. The camera is the ship plus the live head; head_fed is left invalid on purpose (see build_frame).
   Env: SPHERE_FPS, SPHERE_TIME, SPHERE_CRUISE, SPHERE_MAXSPEED, SPHERE_TURN, SPHERE_PITCH, SPHERE_BOLT, SPHERE_GAP, SPHERE_SKILL, SPHERE_FOE, SPHERE_CHASER,
   SPHERE_ARENA, SPHERE_HYPER_CD, SPHERE_ROCKS, SPHERE_SWARM, SPHERE_CHASERS, SPHERE_CHAOS, SPHERE_FREEPLAY (1), SPHERE_AUTO, SPHERE_CAM=x,y,z,tx,ty,tz.
   Desktop: mouse look = head, A/D turn, W/S stick forward/back (nose down/up), Q/E speed up/down, Space or left mouse fire, R weapon, H hyperspace, X chaos charge, Backspace held = eject, 5 = coin; A/D pick the level on the title.
   Headset: stick flies, trigger fire, right grip weapon (the "upper right button"), left trigger speed, A hyperspace, B chaos charge, grip + B held = eject; stick left/right picks the level on the title. */
namespace sphere {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at;
using buggy::to_world; using buggy::to_local; using buggy::rotate;
using d1k::Mesh; using d1k::dput; using d1k::blit; using d1k::text; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound;
using d1k::yawed; using d1k::pitched; using d1k::add_bitmap; using d1k::make_digit;

/* ---- tuning: everything here is INVENTED (no source gives a number); each can be changed by env SPHERE_<NAME> ---- */
static float game_fps = 20.f;                              /* measured 20 on clean footage */
static float game_time = 180.f;                            /* footage: 180 on the clock, dropping one a second */
static float cruise = 14000.f, max_speed = 42000.f;        /* INVENTED: 14 m/s cruise, 42 m/s flat out */
static float turn_rate = 1.5f, pitch_rate = 1.1f;          /* INVENTED: rad/s at full stick */
static float bolt_speed = 110000.f, fire_gap = 0.36f;      /* fire_gap measured: shots 8-11 video frames apart at t959-t960; speed INVENTED */
static float enemy_skill = 1.f, foe_speed = 21000.f, chaser_speed = 30000.f;   /* INVENTED */
static float arena_r = 185000.f;                           /* the frame lines are the sphere's skeleton, 200 m out (radius INVENTED); nothing gets out */
static float hyper_cd_len = 15.f;                          /* INVENTED */
static float gen_hp0 = 6.f;                                /* INVENTED: hits to knock out one field generator */
static int n_rocks = 6, n_swarm = 6, n_chasers = 3, chaos_max = 1, start_mode = 0;   /* INVENTED */
static bool freeplay = true;                               /* Simon's copy: unlimited credits, no front end */
static bool autoplay = false, duct_test = false;

static V BASE_P = V(0, 70000, -25000), BASE_H = V(36000, 20000, 11000);   /* the grey base with the docking bay (footage t949); size INVENTED */
static V START_P = V(0, -5000, -9000);                     /* so the first seconds are a pass over the deck, as at t949-t953; INVENTED */
static const float LATT_R = 200000.f;

/* ---- ids: meshes at MH + id, bitmaps at BMPH + id; sounds in our own block ---- */
enum { M_STARS = 600, M_LATT, M_BASE, M_ROCK0, M_ROCK1, M_ROCK2, M_FIGHT, M_ALLY, M_MISSILE, M_MOTHER, M_BOLT, M_PLASMA, M_SPARK_O, M_SPARK_W, M_SPARK_B,
       M_SHELL, M_DASH, M_QUAD, M_SLOT, M_YLINE, M_BLIP_E, M_BLIP_R, M_BLIP_B, M_RADAR, M_FROG0, M_FROG1, M_EJECT, M_ICONS, M_RADARB, M_STAT0 /* 4 variants */ = 640, M_HULL = 650, M_HULLL, M_FIRE0, M_FIRE1, M_FIRE2, M_GQUAD, M_YSQ, M_SLOTB, M_TOOTH, M_GEN, M_DUCTLIT };
static bool blink_on = false;
enum { BM_BR = 420 /* 4 corners */, BM_TIE = 424, BM_TICK_S, BM_TICK_T, BM_SOLID, BM_TRI_U, BM_TRI_L, BM_TRI_D, BM_TRI_R };
static const int BM_GDIG = 50;                             /* green digit set: make_digit id 50 + d, drawn at d1k::BM_DIG + 50 + d */
enum { SND_LASER = 0x7E000001, SND_EXPL, SND_ROCK, SND_HIT, SND_ENEMY, SND_HYPER, SND_CHAOS, SND_BABBLE, SND_START, SND_ALARM, SND_CLEAR, SND_MISSILE, SND_RIFF };
static const uint32_t RGBA_GRN = 0xFF68BC58u;   /* HUD green read from the frames: thin and dim */
static const uint8_t HUD_R = 0x4C, HUD_G = 0xA4, HUD_B = 0x5C;
static inline float HUD_Y(float fy) { return (fy * 0.75f + 0.0927f) * 288.f; }   /* a fraction of Octavius's frame height to our 288 lines */

/* ---- state ---- */
struct Rock { V p, v, w; M R; float r = 3000.f, hp = 3.f; int size = 2, var = 0; bool shower = false; };
struct Foe  { V p, v, ax; M R; int kind = 0; float hp = 1.f, fire_t = 2.f, t = 0.f, ax_t = 0.f, sp = 1.f; };   /* kind 0 fighter, 1 tail-chaser, 3 on your side */
struct Bolt { V p, v; float t = 0.f; bool mine = true, big = false, ally = false, missile = false, locked = false; };
struct Part { V p, v; float life = 1.f, life0 = 1.f, sz = 200.f, delay = 0.f; int id = M_SPARK_O; };
struct Fire { V p; float t = 0.f, sz = 3000.f; };
static std::vector<Fire> fires;
struct Ship { V p, v; float yaw = 0.f, pitch = 0.f, speed = 0.f, shield = 100.f, fire_cool = 0.f, miss_cool = 0.f, dead_t = 0.f, inv = 0.f; bool alive = true; int side = 0, weapon = 0; };
struct Mom { bool on = false; V p, v, way; M R; float gen[4] = {0, 0, 0, 0}, spawn_t = 6.f, fire_t = 3.f, way_t = 0.f; int side = 0; };
struct Ctl { float yaw = 0.f, pit = 0.f, thr = 0.f; bool fire = false, hyper = false, chaos = false, eject = false, weapon = false; V aim = V(0, 1, 0);  bool a_miss = false; };
static std::vector<Rock> rocks; static std::vector<Foe> foes; static std::vector<Bolt> bolts; static std::vector<Part> parts;
static Ship ship; static Mom mom;
enum State { S_TITLE, S_PLAY, S_OVER };
static State st = S_TITLE; static float st_t = 0.f, time_left = 0.f; static double sim_t = 0;
static int score = 0, game_mode = 0, over_why = 0, chaos_left = 0, credits = 0, coins_seen = 0;   /* over_why: 0 time up, 1 mission complete, 2 ejected */
static const char *MODE_NAME[4] = {"SOLO TRAINER", "SOLO ACE", "DOGFIGHT", "BATTLESPHERE"};
static float hyper_cd = 0.f, chaos_r = -1.f, eject_t = 0.f, eject_on = 0.f, hyper_age = 1e9f, spawn_cd = 0.f, shower_t = 20.f, duct_t = -1.f, bar_v = 0.f, riff_t = 0.f;
static float hyper_t = 0.f; static const float HYPER_LEN = 0.44f;
static V chaos_c; static bool hyper_done = false, chaos_hit_mom = false, field_up = false, wave_alt = false;
enum { C_STATIC, C_RADAR, C_FROG };
static int comm = C_STATIC; static float comm_t = 3.f;
static int kills = 0;
/* ---- link play (vpix_1k_net.h): see the block above tick. Pod 1 runs the whole sky; each other pod flies its own ship (crew[k]) in it, sending its sticks. All pilots are one team: shared score and clock. ---- */
struct PS { Ship s; float hyper_cd = 0.f, hyper_t = 0.f, hyper_age = 1e9f, comm_t = 3.f, eject_on = 0.f; bool hyper_done = false; int comm = C_STATIC, chaos_left = 0; };   /* one pilot: the ship plus the globals that belong to one cockpit */
static PS crew[4]; static bool crew_on[4] = {false, false, false, false};
static int me = 0; static bool slave_game = false;
struct Peer { V p; float yaw = 0.f, pitch = 0.f; bool on = false, alive = false; };
static Peer peer[4];
#pragma pack(push, 1)
struct NetEv { float p[3]; uint8_t n; };
#pragma pack(pop)
static NetEv evr[6]; static uint8_t ev_total = 0;
static void swap_ps(PS &o) { std::swap(ship, o.s); std::swap(hyper_cd, o.hyper_cd); std::swap(hyper_t, o.hyper_t); std::swap(hyper_age, o.hyper_age); std::swap(comm_t, o.comm_t); std::swap(eject_on, o.eject_on); std::swap(hyper_done, o.hyper_done); std::swap(comm, o.comm); std::swap(chaos_left, o.chaos_left); }
static PS cur_ps() { PS p; p.s = ship; p.hyper_cd = hyper_cd; p.hyper_t = hyper_t; p.hyper_age = hyper_age; p.comm_t = comm_t; p.eject_on = eject_on; p.hyper_done = hyper_done; p.comm = comm; p.chaos_left = chaos_left; return p; }
static void crew_reset(int k) { crew[k] = PS(); Ship &s = crew[k].s; s.p = START_P + V(k == 1 ? -6000.f : k == 2 ? 6000.f : 0.f, 0.f, k == 3 ? -6000.f : 0.f); s.speed = cruise; s.inv = 2.f; crew[k].eject_on = 5.7f; crew[k].comm_t = 5.7f; }
static void fill_peers() {   /* who to draw besides yourself (master side; a slave fills it from the snapshot) */
    peer[0].on = true; peer[0].p = ship.p; peer[0].yaw = ship.yaw; peer[0].pitch = ship.pitch; peer[0].alive = ship.alive;
    for (int k = 1; k < 4; k++) { peer[k].on = crew_on[k]; peer[k].p = crew[k].s.p; peer[k].yaw = crew[k].s.yaw; peer[k].pitch = crew[k].s.pitch; peer[k].alive = crew[k].s.alive; }
}
static const V GEN_L[4] = {V(38000, -6000, 0), V(-38000, -6000, 0), V(0, -6000, 38000), V(0, -6000, -38000)};   /* the field generators, on stalks; INVENTED */
static const float DUCT_Y0 = -30000.f, DUCT_Y1 = -1000.f;

static V rnd_dir() { return norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-1, 1))); }
static void sfx(int ch, uint32_t key, int vol, int pan = 0) { if (!sound_on) return; buggy::play(ch, (int)key, vol, pan); }
static M basis() { return pitched(yawed(ship.yaw), ship.pitch); }
static bool in_box(V p, V c, V h) { return fabsf(p.x - c.x) < h.x && fabsf(p.y - c.y) < h.y && fabsf(p.z - c.z) < h.z; }
static bool box_push(V &p, V c, V h, float pad) {
    V d = p - c; float ox = h.x + pad - fabsf(d.x), oy = h.y + pad - fabsf(d.y), oz = h.z + pad - fabsf(d.z);
    if (ox <= 0 || oy <= 0 || oz <= 0) return false;
    if (ox < oy && ox < oz) p.x += d.x >= 0 ? ox : -ox; else if (oy < oz) p.y += d.y >= 0 ? oy : -oy; else p.z += d.z >= 0 ? oz : -oz;
    return true;
}

/* ---- mesh helpers: flat polygons only ---- */
static void tri_to(Mesh &m, V a, V b, V c, V want, uint32_t rgb) { if (dot(cross(b - a, c - a), want) < 0) m.poly({a, c, b}, rgb); else m.poly({a, b, c}, rgb); }
static void quad_to(Mesh &m, V a, V b, V c, V d, V want, uint32_t rgb) { if (dot(cross(b - a, c - a), want) < 0) m.poly({d, c, b, a}, rgb); else m.poly({a, b, c, d}, rgb); }
static void qxz(Mesh &m, float x0, float z0, float x1, float z1, float y, uint32_t c) { m.poly({V(x0, y, z0), V(x1, y, z0), V(x1, y, z1), V(x0, y, z1)}, c); }
static void fan_front(Mesh &m, const std::vector<V> &p, uint32_t rgb) { for (size_t i = 1; i + 1 < p.size(); i++) tri_to(m, p[0], p[i], p[i + 1], V(0, -1, 0), rgb); }
static void ngon_xz(Mesh &m, float cx, float cz, float rx, float rz, float y, uint32_t c) {   /* an octagon on the dash plane */
    for (int i = 0; i < 8; i++) { float a = 0.3926991f + 0.7853982f * i, b = a + 0.7853982f;
        tri_to(m, V(cx, y, cz), V(cx + rx * cosf(a), y, cz + rz * sinf(a)), V(cx + rx * cosf(b), y, cz + rz * sinf(b)), V(0, -1, 0), c); }
}
static void glyph(Mesh &m, int ch, float x0, float ztop, float cell, float y, uint32_t c, float zc = 0.f) {   /* a letter from the shared 5 x 7 font, as flat cells */
    for (int yy = 0; yy < 7; yy++) for (int xx = 0; xx < 5; xx++) if (d1k::FONT[ch][yy * 5 + xx] == '1')
        qxz(m, x0 + xx * cell, ztop - (yy + 1) * (zc > 0 ? zc : cell), x0 + (xx + 1) * cell, ztop - yy * (zc > 0 ? zc : cell), y, c);
}

/* dash plane: everything faces the eye at DY mm; fractions are of the 352 x 288 picture, so the layout follows the frames */
static const float KX = 868.f, KZ = 651.f, DY = 620.f;
static V F(float fx, float fy, float y = DY) { return V((fx - 0.5f) * KX, y, -(fy * 0.75f + 0.0927f - 0.5f) * KZ); }
static void rect_f(Mesh &m, float fx0, float fy0, float fx1, float fy1, float y, uint32_t c) {
    V a = F(fx0, fy0, y), b = F(fx1, fy1, y);
    qxz(m, (std::min)(a.x, b.x), (std::min)(a.z, b.z), (std::max)(a.x, b.x), (std::max)(a.z, b.z), y, c);
}
static void pod(Mesh &m, const float (*O)[2], int n, float cxf) {   /* a pod: dark plate, a pale rolled rim over its top edge with a mid-grey band inside it (solved from the frame outline) */
    V c = F(cxf, 0.95f); std::vector<V> o, a, b;
    for (int i = 0; i < n; i++) { o.push_back(F(O[i][0], O[i][1])); V d = norm(c - o[i]); a.push_back(o[i] + d * 10.f + V(0, -2, 0)); b.push_back(a[i] + d * 6.f + V(0, -2, 0)); }
    for (int i = 0; i + 1 < n; i++) { quad_to(m, o[i], o[i + 1], a[i + 1], a[i], V(0, -1, 0), 0xC6C0C8); quad_to(m, a[i], a[i + 1], b[i + 1], b[i], V(0, -1, 0), 0x625E64);
        tri_to(m, b[i], b[i + 1], c + V(0, -4, 0), V(0, -1, 0), 0x46424A); quad_to(m, o[i], o[i + 1], o[i + 1] + V(0, 120, -20), o[i] + V(0, 120, -20), V(0, 0, 1), 0xB4AEB6); }
}

/* ---- the meshes ---- */
static void build_rock(int id, int kind, unsigned seed) {   /* big chamfered facets, a pale side, a round green patch and small ones (t967-t969) */
    std::mt19937 q(seed); auto u01 = [&q]() { return q() / (float)(q.max)(); }; const float ph1 = u01() * 6.f, ph2 = u01() * 6.f, K = 0.58f; V sc(0.85f + 0.2f * u01(), 0.85f + 0.2f * u01(), 0.8f + 0.2f * u01());
    auto J = [&](V d) { float r = 1.f + 0.11f * sinf(d.x * 2.9f + d.y * 2.1f + ph1) + 0.08f * sinf(d.y * 3.7f - d.z * 3.1f + ph2); return V(d.x * sc.x, d.y * sc.y, d.z * sc.z) * (800.f * r); };
    auto P = [&](int ax, float s, float u, float v) { float a[3]; a[ax] = s; a[(ax + 1) % 3] = u * K; a[(ax + 2) % 3] = v * K; return J(V(a[0], a[1], a[2])); };
    Mesh m; const V litd = norm(V(-0.7f, -0.55f, 0.45f));
    auto col = [&](V n) -> uint32_t { float l = dot(norm(n), litd); int g = 0x78 + (int)(q() % 12); if (l > 0.55f) return (uint32_t)0xB0A2A0 + (uint32_t)(q() % 6) * 0x010101u; return (((uint32_t)(g + 2)) << 16) | ((uint32_t)g << 8) | (uint32_t)(g + 3); };
    for (int ax = 0; ax < 3; ax++) for (int s0 = -1; s0 <= 1; s0 += 2) { float s = (float)s0; V a = P(ax, s, -1, -1), b = P(ax, s, 1, -1), c = P(ax, s, 1, 1), d = P(ax, s, -1, 1), n = a + b + c + d; quad_to(m, a, b, c, d, n, col(n)); }
    for (int ax = 0; ax < 3; ax++) { int nx = (ax + 1) % 3; for (int s0 = -1; s0 <= 1; s0 += 2) for (int s1 = -1; s1 <= 1; s1 += 2) {   /* the edge between face ax (sign s0) and face nx (sign s1), along the third axis */
        float s = (float)s0, t = (float)s1; V a = P(ax, s, t, -1), b = P(ax, s, t, 1), c = P(nx, t, 1, s), d = P(nx, t, -1, s), n = a + b + c + d; quad_to(m, a, b, c, d, n, col(n)); } }
    for (int sx = -1; sx <= 1; sx += 2) for (int sy = -1; sy <= 1; sy += 2) for (int sz = -1; sz <= 1; sz += 2) { V a = P(0, (float)sx, (float)sy, (float)sz), b = P(1, (float)sy, (float)sz, (float)sx), c = P(2, (float)sz, (float)sx, (float)sy), n = a + b + c; tri_to(m, a, b, c, n, col(n)); }
    auto blob = [&](V d, float rad) { V cn = J(d) * 1.05f, nn = norm(cn), e0 = norm(cross(nn, V(0.3f, 0.2f, 0.9f))), e1 = cross(nn, e0); for (int i = 0; i < 8; i++) { float a = 0.7854f * i, b = 0.7854f * (i + 1); tri_to(m, cn, cn + (e0 * cosf(a) + e1 * sinf(a)) * rad, cn + (e0 * cosf(b) + e1 * sinf(b)) * rad, nn, 0x30A040); } };
    blob(V(-0.25f, -1.f, -0.1f), 125.f); if (kind) { blob(V(-0.75f, -1.f, -0.45f), 50.f); blob(V(-0.85f, -0.75f, 0.1f), 38.f); }
    m.add(id);
}
static void build_fighter(int id, uint32_t body, uint32_t dark, uint32_t plate) {   /* the blue ship (t958.68 and t958.72, close pass): a faceted barrel with a pale plate along it, two pale pods on struts with red and orange ends, two gold prongs forward. Depth of it is a guess from the two views */
    Mesh m; const uint32_t PE = 0xD0CCEC, PE2 = 0xA8A4D4, GO = 0xD8B040; const float ys[4] = {1100, 500, -900, -1250}, rs[4] = {420, 900, 900, 520};
    for (int i = 0; i < 8; i++) { float a = 0.7854f * i + 0.3927f, b = a + 0.7854f, mid = a + 0.3927f; V nr(cosf(mid), 0, sinf(mid));
        for (int k = 0; k < 3; k++) { V a0(cosf(a) * rs[k], ys[k], sinf(a) * rs[k]), b0(cosf(b) * rs[k], ys[k], sinf(b) * rs[k]), a1(cosf(a) * rs[k + 1], ys[k + 1], sinf(a) * rs[k + 1]), b1(cosf(b) * rs[k + 1], ys[k + 1], sinf(b) * rs[k + 1]);
            quad_to(m, a0, b0, b1, a1, nr + V(0, k == 0 ? 0.5f : k == 2 ? -0.5f : 0.f, 0), sinf(mid) > 0.9f ? plate : (i & 1) ? body : dark); }
        tri_to(m, V(0, ys[0] + 140, 0), V(cosf(a) * rs[0], ys[0], sinf(a) * rs[0]), V(cosf(b) * rs[0], ys[0], sinf(b) * rs[0]), V(0, 1, 0), dark); tri_to(m, V(0, ys[3] - 140, 0), V(cosf(a) * rs[3], ys[3], sinf(a) * rs[3]), V(cosf(b) * rs[3], ys[3], sinf(b) * rs[3]), V(0, -1, 0), dark); }
    for (int k = 0; k < 2; k++) { float x = k ? 1700.f : -1700.f, sg = k ? 1.f : -1.f; const float R = 500.f, Y0 = -1500.f, Y1 = 150.f;
        for (int i = 0; i < 8; i++) { float a = 0.7854f * i + 0.3927f, b = a + 0.7854f, mid = a + 0.3927f; V c0(x + cosf(a) * R, Y0, sinf(a) * R), c1(x + cosf(b) * R, Y0, sinf(b) * R), c2(x + cosf(b) * R * 0.7f, Y1, sinf(b) * R * 0.7f), c3(x + cosf(a) * R * 0.7f, Y1, sinf(a) * R * 0.7f);
            quad_to(m, c0, c1, c2, c3, V(cosf(mid), 0, sinf(mid)), (i & 1) ? PE : PE2); tri_to(m, V(x, Y1 + 90, 0), c3, c2, V(0, 1, 0), PE2);
            const uint32_t E0 = k ? 0xF0A040 : 0xE05030; tri_to(m, V(x, Y0 - 30, 0), c0, c1, V(0, -1, 0), E0); }
        m.box(V((std::min)(sg * 700.f, sg * 1350.f), -700, -120), V((std::max)(sg * 700.f, sg * 1350.f), -100, 120), dark); }
    for (int k = 0; k < 2; k++) { float x = k ? 230.f : -230.f; m.box(V(x - 70, 1050, -70), V(x + 70, 2150, 70), GO); }
    m.add(id);
}
static void build_missile(int id) {   /* the red missile (t946.9-t947.5): a tapered orange-red body, a pale lilac rear ring, a white end */
    Mesh m; const uint32_t RD = 0xE05028, RD2 = 0xB83C20, LI = 0xC8C0E0, WH = 0xF0ECF4; const float ys[5] = {2400, 1200, -600, -1000, -2000}, rs[5] = {200, 800, 950, 950, 800}; const uint32_t cs[4] = {RD, RD, LI, LI};
    for (int i = 0; i < 8; i++) { float a = 0.7854f * i, b = 0.7854f * (i + 1); for (int k = 0; k < 4; k++) { V a0(cosf(a) * rs[k], ys[k], sinf(a) * rs[k]), b0(cosf(b) * rs[k], ys[k], sinf(b) * rs[k]), a1(cosf(a) * rs[k + 1], ys[k + 1], sinf(a) * rs[k + 1]), b1(cosf(b) * rs[k + 1], ys[k + 1], sinf(b) * rs[k + 1]);
            quad_to(m, a0, b0, b1, a1, V(cosf(a + 0.39f), 0, sinf(a + 0.39f)), k < 2 && (i & 1) ? RD2 : cs[k]); }
        tri_to(m, V(cosf(a) * 800, -2000, sinf(a) * 800), V(cosf(b) * 800, -2000, sinf(b) * 800), V(0, -2100, 0), V(0, -1, 0), WH); tri_to(m, V(cosf(a) * 200, 2400, sinf(a) * 200), V(cosf(b) * 200, 2400, sinf(b) * 200), V(0, 2500, 0), V(0, 1, 0), RD); }
    m.add(id);
}
static void build_mother() {   /* the enemy battleship: shape INVENTED (no frame shows one); press gives it a force field and cooling ducts running in to a central reactor */
    Mesh m; const uint32_t G1 = 0x8A8A94, G2 = 0x6C6C78, G3 = 0x484852;
    m.box(V(-8000, -26000, -5000), V(8000, 16000, 6000), G1, G2, G3);
    V tip(0, 32000, 500); V c0(-8000, 16000, -5000), c1(8000, 16000, -5000), c2(8000, 16000, 6000), c3(-8000, 16000, 6000);
    tri_to(m, tip, c0, c1, V(0, 0, -1), G3); tri_to(m, tip, c1, c2, V(1, 0, 0), G2); tri_to(m, tip, c2, c3, V(0, 0, 1), G1); tri_to(m, tip, c3, c0, V(-1, 0, 0), G2);
    m.box(V(8000, -14000, -1500), V(21000, 4000, 1500), G1, G2, G3); m.box(V(-21000, -14000, -1500), V(-8000, 4000, 1500), G1, G2, G3);
    m.box(V(-3000, -10000, 6000), V(3000, 2000, 11000), G1, G2, G3);
    m.box(V(-5000, -30000, -3000), V(5000, -26000, 4000), G2);
    qxz(m, -2600, -2200, 2600, 2200, -30010, 0x181820);     /* the duct's mouth at the stern */
    for (int k = 0; k < 4; k++) { V g = GEN_L[k]; float hx = fabsf(g.x) > 1 ? fabsf(g.x) : 500.f, hz = fabsf(g.z) > 1 ? fabsf(g.z) : 500.f; m.box(V(-hx * (g.x < 0 ? 1.f : 0.02f), -6500, -hz * (g.z < 0 ? 1.f : 0.02f)), V(hx * (g.x > 0 ? 1.f : 0.02f), -5500, hz * (g.z > 0 ? 1.f : 0.02f)), G3); }   /* stalks out to the generators */
    for (int i = 0; i < 10; i++) { float y0 = DUCT_Y0 + i * (DUCT_Y1 - DUCT_Y0) / 10.f, y1 = y0 + (DUCT_Y1 - DUCT_Y0) / 10.f; const uint32_t c = (i & 1) ? 0xC0A030 : 0x585860, c2 = (i & 1) ? 0x907820 : 0x404048;   /* the duct, seen from inside */
        quad_to(m, V(-2600, y0, -2200), V(2600, y0, -2200), V(2600, y1, -2200), V(-2600, y1, -2200), V(0, 0, 1), c2); quad_to(m, V(-2600, y0, 2200), V(2600, y0, 2200), V(2600, y1, 2200), V(-2600, y1, 2200), V(0, 0, -1), c2);
        quad_to(m, V(-2600, y0, -2200), V(-2600, y1, -2200), V(-2600, y1, 2200), V(-2600, y0, 2200), V(1, 0, 0), c); quad_to(m, V(2600, y0, -2200), V(2600, y1, -2200), V(2600, y1, 2200), V(2600, y0, 2200), V(-1, 0, 0), c); }
    quad_to(m, V(-2600, DUCT_Y1, -2200), V(2600, DUCT_Y1, -2200), V(2600, DUCT_Y1, 2200), V(-2600, DUCT_Y1, 2200), V(0, -1, 0), 0xE03018); quad_to(m, V(-1100, DUCT_Y1 - 20, -900), V(1100, DUCT_Y1 - 20, -900), V(1100, DUCT_Y1 - 20, 900), V(-1100, DUCT_Y1 - 20, 900), V(0, -1, 0), 0xFFF0C0);   /* the reactor */
    m.add(M_MOTHER);
    { Mesh g; g.box(V(-2600, -2600, -2600), V(2600, 2600, 2600), 0xE04028, 0xB02820, 0x802018); g.add(M_GEN); }
    { Mesh g; const uint32_t c = 0xFFB040; const float y = -30030; qxz(g, -3400, 2200, 3400, 2800, y, c); qxz(g, -3400, -2800, 3400, -2200, y, c); qxz(g, -3400, -2200, -2600, 2200, y, c); qxz(g, 2600, -2200, 3400, 2200, y, c); g.add(M_DUCTLIT); }
}
static void build_base() {   /* the base (t949-t953): a wide faceted grey hull; a bay in its front with ceiling and floor, a broad yellow band, thin yellow chevrons, white arrow-and-bar marks; a green block on top. Size INVENTED */
    Mesh m; const V h = BASE_H; const float X = h.x, Y = h.y, Z = h.z, wx = X * 0.46f, zo = Z * 0.42f, by = Y * 0.35f, zw = Z * 0.5f;
    const float ox[8] = {-1.f, -0.6f, 0.6f, 1.f, 1.f, 0.6f, -0.6f, -1.f}, oy[8] = {-0.35f, -1.f, -1.f, -0.35f, 0.6f, 1.f, 1.f, 0.6f};
    auto L = [&](int i, float sc, float z) { return V(ox[i & 7] * X * sc, oy[i & 7] * Y * sc, z); };
    const uint32_t TOP = 0xA8A49E, UP = 0x8E8A84, WL = 0x807C78, LO = 0x5E5A58, FL = 0x98948E, CE = 0xB0ACA6, YE = 0xFFE040, WH = 0xF0F0EC;
    m.poly({L(0, 0.82f, Z), L(1, 0.82f, Z), L(2, 0.82f, Z), L(3, 0.82f, Z), L(4, 0.82f, Z), L(5, 0.82f, Z), L(6, 0.82f, Z), L(7, 0.82f, Z)}, TOP);
    m.poly({L(7, 0.7f, -Z), L(6, 0.7f, -Z), L(5, 0.7f, -Z), L(4, 0.7f, -Z), L(3, 0.7f, -Z), L(2, 0.7f, -Z), L(1, 0.7f, -Z), L(0, 0.7f, -Z)}, LO);
    for (int i = 0; i < 8; i++) { V nrm = V(ox[i] + ox[(i + 1) & 7], oy[i] + oy[(i + 1) & 7], 0);
        quad_to(m, L(i, 1.f, zw), L(i + 1, 1.f, zw), L(i + 1, 0.82f, Z), L(i, 0.82f, Z), nrm + V(0, 0, 1), i == 1 ? 0x989490 : UP);
        quad_to(m, L(i, 0.7f, -Z), L(i + 1, 0.7f, -Z), L(i + 1, 1.f, -zw), L(i, 1.f, -zw), nrm + V(0, 0, -1), LO);
        if (i != 1) quad_to(m, L(i, 1.f, -zw), L(i + 1, 1.f, -zw), L(i + 1, 1.f, zw), L(i, 1.f, zw), nrm, WL); }
    const float fx = 0.6f * X; auto FR = [&](float x0, float z0, float x1, float z1, uint32_t c) { m.poly({V(x0, -Y, z0), V(x1, -Y, z0), V(x1, -Y, z1), V(x0, -Y, z1)}, c); };
    FR(-fx, -zw, -wx, zw, WL); FR(wx, -zw, fx, zw, WL); FR(-wx, zo, wx, zw, WL); FR(-wx, -zw, wx, -zo, WL);
    m.poly({V(-wx, by, -zo), V(wx, by, -zo), V(wx, -Y, -zo), V(-wx, -Y, -zo)}, FL); m.poly({V(-wx, -Y, zo), V(wx, -Y, zo), V(wx, by, zo), V(-wx, by, zo)}, CE);   /* floor, ceiling */
    m.poly({V(-wx, -Y, -zo), V(-wx, by, -zo), V(-wx, by, zo), V(-wx, -Y, zo)}, UP); m.poly({V(wx, by, -zo), V(wx, -Y, -zo), V(wx, -Y, zo), V(wx, by, zo)}, UP);
    m.poly({V(-wx, by, -zo), V(-wx, by, zo), V(wx, by, zo), V(wx, by, -zo)}, 0x202028);                                      /* the dark back */
    m.poly({V(-wx, by - 40, zo * 0.25f), V(-wx, by - 40, zo), V(wx, by - 40, zo), V(wx, by - 40, zo * 0.25f)}, YE);          /* the broad yellow band across its top */
    auto mark = [&](float y0, float z, float dir, float n, float k0 = 0.6f) {   /* a white arrow over a bar, lying on floor or ceiling */
        V a(0, y0 + dir * 3600.f * k0, z), b(-2600.f * k0, y0, z), c(2600.f * k0, y0, z); tri_to(m, a, b, c, V(0, 0, n), WH);
        V p0(-2600.f * k0, y0 - dir * 700.f * k0, z), p1(2600.f * k0, y0 - dir * 700.f * k0, z), p2(2600.f * k0, y0 - dir * 1400.f * k0, z), p3(-2600.f * k0, y0 - dir * 1400.f * k0, z); quad_to(m, p0, p1, p2, p3, V(0, 0, n), WH); };
    mark(-Y * 0.72f, -zo + 25, 1.f, 1.f); mark(-Y * 0.1f, -zo + 25, 1.f, 1.f); mark(-Y * 0.45f, zo - 25, -1.f, -1.f);
    auto chev = [&](float z, float n) { const float p[4][2] = {{-wx * 0.9f, -Y}, {-wx * 0.55f, -Y * 0.25f}, {wx * 0.55f, -Y * 0.25f}, {wx * 0.9f, -Y}}; for (int k = 0; k < 3; k++) { V a(p[k][0], p[k][1], z), b(p[k + 1][0], p[k + 1][1], z), d = norm(b - a), sd(-d.y * 450.f, d.x * 450.f, 0); quad_to(m, a - sd, b - sd, b + sd, a + sd, V(0, 0, n), YE); } };
    chev(-zo + 15, 1.f);
    /* the same marks on the deck, leading to the bay */
    mark(-Y * 0.6f, Z + 20, 1.f, 1.f, 0.38f); mark(-Y * 0.2f, Z + 20, 1.f, 1.f, 0.38f);
    { float z = Z + 18.f; const float p[4][2] = {{-X * 0.5f, -Y * 0.8f}, {-X * 0.25f, -Y * 0.4f}, {X * 0.25f, -Y * 0.4f}, {X * 0.5f, -Y * 0.8f}}; for (int k = 0; k < 3; k++) { V aa(p[k][0], p[k][1], z), bb(p[k + 1][0], p[k + 1][1], z), d = norm(bb - aa), sd(-d.y * 450.f, d.x * 450.f, 0); quad_to(m, aa - sd, bb - sd, bb + sd, aa + sd, V(0, 0, 1), YE); } }
    for (int q0 = -1; q0 <= 1; q0 += 2) { const float xa = q0 * X * 0.14f, xb = q0 * X * 0.82f, ya = Y * 0.2f, yb = Y * 0.82f, zt = Z + 3600.f; V a(xa, ya, Z + 10), b(xb, ya - Y * 0.5f, Z + 10), c(xb, yb, zt), d0(xa, yb, zt * 0.f + Z + 2400.f); if (q0 > 0) quad_to(m, a, b, c, d0, V(-0.2f, 0, 1), 0xA09C98); else quad_to(m, b, a, d0, c, V(0.2f, 0, 1), 0xA09C98); }
    { float z = Z + 18.f; const float p[4][2] = {{-X * 0.62f, Y * 0.28f}, {-X * 0.24f, -Y * 0.02f}, {X * 0.24f, -Y * 0.02f}, {X * 0.62f, Y * 0.28f}}; for (int k = 0; k < 3; k++) { V aa(p[k][0], p[k][1], z), bb(p[k + 1][0], p[k + 1][1], z), d = norm(bb - aa), sd(-d.y * 1100.f, d.x * 1100.f, 0); quad_to(m, aa - sd, bb - sd, bb + sd, aa + sd, V(0, 0, 1), YE); } }
    m.box(V(-2600, Y * 0.3f, Z), V(2600, Y * 0.42f, Z + 2500), 0x30A848, 0x289840, 0x206028);                               /* the green block */
    m.add(M_BASE);
}
static void build_world_meshes() {
    Mesh st_; std::mt19937 r2(31);   /* stars: tiny white squares far out */
    for (int i = 0; i < 900; i++) {
        float z = (r2() / (float)(r2.max)()) * 2.f - 1.f, a = (r2() / (float)(r2.max)()) * 6.2831853f, q = sqrtf(1.f - z * z), R = 380000.f, s = 800.f + 600.f * (r2() / (float)(r2.max)());
        V c(cosf(a) * q * R, sinf(a) * q * R, z * R), e0 = norm(cross(c, V(0.3f, 0.5f, 0.8f))), e1 = norm(cross(c, e0));
        uint32_t lum = 0x909090 + (r2() % 3) * 0x303030;
        st_.poly({c - e0 * s, c - e1 * s, c + e0 * s, c + e1 * s}, lum);
    }
    st_.add(M_STARS);
    Mesh lt; {   /* thin grey lines meeting three at a time at about 23 degrees apart: the edges of a truncated icosahedron, each as two crossed ribbons */
        const float PHI = 1.6180339f; V iv[12]; int n = 0; std::vector<V> tv;
        for (int a = -1; a <= 1; a += 2) for (int b = -1; b <= 1; b += 2) { iv[n++] = norm(V(0, (float)a, b * PHI)); iv[n++] = norm(V((float)a, b * PHI, 0)); iv[n++] = norm(V(a * PHI, 0, (float)b)); }
        for (int i = 0; i < 12; i++) for (int j = 0; j < 12; j++) if (i != j && len(iv[i] - iv[j]) < 1.2f) tv.push_back(norm(iv[i] * (2.f / 3.f) + iv[j] * (1.f / 3.f)) * LATT_R);
        for (size_t i = 0; i < tv.size(); i++) for (size_t j = i + 1; j < tv.size(); j++) {
            if (len(tv[i] - tv[j]) > LATT_R * 0.43f) continue;
            V a = tv[i], b = tv[j], d = norm(b - a), p1 = norm(cross(d, a)), p2 = norm(cross(d, p1)); const float W = 330.f;
            lt.poly({a - p1 * W, b - p1 * W, b + p1 * W, a + p1 * W}, 0xA09A9C); lt.poly({a - p2 * W, b - p2 * W, b + p2 * W, a + p2 * W}, 0x8C8688);
        }
    }
    lt.add(M_LATT);
    build_base();
    build_rock(M_ROCK0, 1, 11); build_rock(M_ROCK1, 0, 29); build_rock(M_ROCK2, 1, 7);
    build_fighter(M_FIGHT, 0x2C38C0, 0x1C2488, 0x9090E0);
    build_fighter(M_ALLY, 0x70747C, 0x484C54, 0xB0B4BC);   /* your side's ships: no frame shows any, colour INVENTED */
    build_missile(M_MISSILE);
    build_mother();
    { Mesh b; b.box(V(-45, -1300, -45), V(45, 1300, 45), 0xD8C040); b.add(M_BOLT); }   /* the yellow stick (t958.96, t959.28, t959.72) */
    { Mesh b; b.poly({V(-250, 0, 0), V(250, 0, 0), V(0, 0, 250)}, 0xFF5030); b.poly({V(0, -250, 0), V(0, 250, 0), V(0, 0, 250)}, 0xFFA040);
      b.poly({V(-250, 0, 0), V(250, 0, 0), V(0, 0, -250)}, 0xFF5030); b.poly({V(0, -250, 0), V(0, 250, 0), V(0, 0, -250)}, 0xFFA040); b.add(M_PLASMA); }
    const uint32_t sc[3] = {0xF09038, 0xFFF0C0, 0x2C38C0}; const int sid[3] = {M_SPARK_O, M_SPARK_W, M_SPARK_B};
    for (int k = 0; k < 3; k++) { Mesh s; V px(100, 0, 0), py(0, 100, 0), pz(0, 0, 100); tri_to(s, px, py, pz, V(1, 1, 1), sc[k]); tri_to(s, py, px * -1.f, pz, V(-1, 1, 1), sc[k]); tri_to(s, px * -1.f, py * -1.f, pz, V(-1, -1, 1), sc[k]); tri_to(s, py * -1.f, px, pz, V(1, -1, 1), sc[k]); tri_to(s, py, px, pz * -1.f, V(1, 1, -1), sc[k]); tri_to(s, px * -1.f, py, pz * -1.f, V(-1, 1, -1), sc[k]); tri_to(s, py * -1.f, px * -1.f, pz * -1.f, V(-1, -1, -1), sc[k]); tri_to(s, px, py * -1.f, pz * -1.f, V(1, -1, -1), sc[k]); s.add(sid[k]); }   /* specks, solid so they read from any side */
    { const uint32_t fc[3] = {0xF89848, 0xFCD090, 0xFFF4DC}; const float fr[3] = {1.f, 0.72f, 0.42f}; const int fid[3] = {M_FIRE0, M_FIRE1, M_FIRE2}; std::mt19937 q(4);   /* the soft fireball (t968): pale in the middle, orange at a lumpy edge; drawn facing the eye */
      for (int k = 0; k < 3; k++) { Mesh b; const int N = 22; V rim[N]; for (int i = 0; i < N; i++) { float a = 6.2831853f * i / N, r = fr[k] * ((i & 1) ? 0.74f + 0.2f * (q() / (float)(q.max)()) : 0.88f + 0.24f * (q() / (float)(q.max)())); rim[i] = V(cosf(a) * r * 1000.f, -40.f * k, sinf(a) * r * 1000.f); }
          for (int i = 0; i < N; i++) tri_to(b, V(0, -40.f * k, 0), rim[i], rim[(i + 1) % N], V(0, -1, 0), fc[k]); b.add(fid[k]); } }
    { Mesh s; const float PHI = 1.6180339f; V iv[12]; int n = 0;   /* the chaos charge's shell */
      for (int a = -1; a <= 1; a += 2) for (int b = -1; b <= 1; b += 2) { iv[n++] = V(0, (float)a, b * PHI); iv[n++] = V((float)a, b * PHI, 0); iv[n++] = V(a * PHI, 0, (float)b); }
      for (int i = 0; i < 12; i++) iv[i] = norm(iv[i]) * 1000.f;
      int fc = 0; for (int i = 0; i < 12; i++) for (int j = i + 1; j < 12; j++) { if (len(iv[i] - iv[j]) > 1200.f) continue;
          for (int k = j + 1; k < 12; k++) if (len(iv[i] - iv[k]) < 1200.f && len(iv[j] - iv[k]) < 1200.f) { s.poly({iv[i], iv[j], iv[k]}, (fc++ & 1) ? 0xFFE080 : 0xFFA030); } }
      s.add(M_SHELL); }
}

static void build_dash_meshes() {
    Mesh d;
    { static const float PL[11][2] = {{-0.03f, 0.69f}, {0.08f, 0.663f}, {0.17f, 0.632f}, {0.217f, 0.62f}, {0.27f, 0.628f}, {0.355f, 0.667f}, {0.395f, 0.738f}, {0.41f, 0.843f}, {0.408f, 1.0f}, {0.405f, 1.25f}, {0.4f, 1.3f}};
      static const float PR[11][2] = {{1.03f, 0.68f}, {0.967f, 0.632f}, {0.868f, 0.614f}, {0.737f, 0.604f}, {0.664f, 0.62f}, {0.618f, 0.68f}, {0.595f, 0.773f}, {0.585f, 0.867f}, {0.58f, 1.0f}, {0.58f, 1.25f}, {0.59f, 1.3f}};
      pod(d, PL, 11, 0.2f); pod(d, PR, 11, 0.8f);
      for (int k = 0; k < 2; k++) { /* close the bottoms down to the picture's edge */ float a = k ? 0.59f : -0.03f, b = k ? 1.03f : 0.4f; rect_f(d, a, 1.25f, b, 1.45f, DY, 0x46424A); } }
    rect_f(d, 0.39f, 0.97f, 0.61f, 1.45f, DY + 60, 0x8A848A);                                                                     /* the grey beam between the pods, its top just in the picture */
    const uint32_t BZ = 0x1A181C, OFF = 0x08080C, FR = 0x6A666C;
    rect_f(d, 0.178f, 0.76f, 0.372f, 1.01f, DY - 10, FR); rect_f(d, 0.187f, 0.778f, 0.364f, 0.995f, DY - 20, OFF);                 /* left monitor: grey frame, black inside */
    rect_f(d, 0.172f, 0.655f, 0.23f, 0.738f, DY - 10, FR); rect_f(d, 0.18f, 0.668f, 0.222f, 0.728f, DY - 20, 0x1E4630); rect_f(d, 0.187f, 0.696f, 0.215f, 0.7f, DY - 30, 0x3C7C50); rect_f(d, 0.19f, 0.678f, 0.212f, 0.682f, DY - 30, 0x3C7C50);   /* the little green scope */
    rect_f(d, 0.25f, 0.685f, 0.35f, 0.745f, DY - 10, BZ);                                                                       /* three small lights: blue, orange, green */
    rect_f(d, 0.258f, 0.7f, 0.277f, 0.732f, DY - 20, 0x7078D0); rect_f(d, 0.29f, 0.7f, 0.309f, 0.732f, DY - 20, 0xD89050); rect_f(d, 0.32f, 0.7f, 0.34f, 0.732f, DY - 20, 0x58B068);
    rect_f(d, 0.083f, 0.738f, 0.162f, 0.99f, DY - 10, FR); rect_f(d, 0.09f, 0.745f, 0.155f, 0.98f, DY - 20, 0x08080C); for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) rect_f(d, (a ? 0.145f : 0.096f), (b ? 0.948f : 0.768f), (a ? 0.149f : 0.1f), (b ? 0.954f : 0.774f), DY - 24, 0x48464C);   /* tall black panel with four small corner marks (t965.56-t967.96) */
    rect_f(d, 0.668f, 0.656f, 0.826f, 0.752f, DY - 10, FR); rect_f(d, 0.68f, 0.672f, 0.81f, 0.74f, DY - 20, BZ); rect_f(d, 0.688f, 0.686f, 0.803f, 0.733f, DY - 26, 0x301008);   /* orange slotted bar */
    rect_f(d, 0.862f, 0.685f, 0.914f, 0.755f, DY - 10, FR); rect_f(d, 0.868f, 0.694f, 0.908f, 0.748f, DY - 20, 0x0C0C10);          /* small dark square */
    rect_f(d, 0.624f, 0.765f, 0.818f, 1.01f, DY - 10, 0x28262A); rect_f(d, 0.632f, 0.773f, 0.81f, 0.995f, DY - 20, OFF);                 /* right panel */
    rect_f(d, 0.818f, 0.765f, 0.905f, 0.985f, DY - 10, FR); rect_f(d, 0.826f, 0.785f, 0.898f, 0.965f, DY - 20, 0x0C0C10);          /* gauge: black, a green cap and a green body with a dotted line */
    rect_f(d, 0.84f, 0.797f, 0.884f, 0.815f, DY - 26, 0x58B070); rect_f(d, 0.837f, 0.825f, 0.887f, 0.955f, DY - 26, 0xC85830);
    rect_f(d, 0.8605f, 0.828f, 0.8635f, 0.952f, DY - 40, 0x0C2410); for (int k = 0; k < 5; k++) rect_f(d, 0.857f, 0.838f + k * 0.024f, 0.867f, 0.846f + k * 0.024f, DY - 40, 0x0C2410);
    d.add(M_DASH);
    /* the rest of the ship round the seat (Bobby 2026-11-02). INVENTED: footage only looks forward. All of it stays outside the forward view, eye at 0 */
    { Mesh h; const uint32_t G1 = 0x6C6C76, G2 = 0x4A4A54, DK = 0x26262E;
      h.box(V(-1000, -1500, -620), V(1000, 120, -560), DK);                                          /* floor */
      h.box(V(-1050, -1560, -560), V(1050, -1500, 100), G1); h.box(V(-1050, -1560, 740), V(1050, -1500, 860), G1);   /* back wall with a window across it */
      h.box(V(-90, -1560, 100), V(90, -1500, 740), G2);
      h.box(V(-990, -1500, 800), V(990, -100, 860), G2);   /* roof */
      /* the cabin: seat, sticks, lit side consoles, back-wall hatch and screens, roof light (Bobby 2026-11-02). INVENTED */
      { using d1k::kit_quad; using d1k::kit_stripes; using d1k::kit_lamps; using d1k::kit_dial; d1k::kit_eye = V(0, 0, 0); Mesh l; const uint32_t S = 0x383840, S2 = 0x26262C, YL = 0xE8C020, BK = 0x141418, PN = 0x383842, RE = 0xC02020;
        h.box(V(-310, -620, -560), V(310, -500, -140), S, S2, S2); h.box(V(-140, -620, -140), V(140, -510, 60), S, S2, S2); h.box(V(-320, -500, -560), V(320, 80, -370), S, S2, S2);   /* seat */
        kit_quad(h, V(-190, -498, -360), V(60, 0, 0), V(0, 0, 160), RE); kit_quad(h, V(130, -498, -360), V(60, 0, 0), V(0, 0, 160), RE); kit_stripes(h, V(-310, -498, -190), V(620, 0, 0), V(0, 0, 40), 8, YL, BK);   /* straps, the eject bar */
        for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; auto bx = [&](float x0, float y0, float z0, float x1, float y1, float z1, uint32_t c, uint32_t c2) { h.box(V((std::min)(q * x0, q * x1), y0, z0), V((std::max)(q * x0, q * x1), y1, z1), c, c2, c2); };
            bx(320, -380, -370, 500, 200, -300, S, S2); bx(350, -300, -560, 430, -220, -370, S2, S2); bx(395, 110, -300, 435, 150, -130, BK, BK); bx(380, 90, -130, 450, 170, -50, RE, 0x901818);   /* armrest, stick */
            bx(680, -1300, -560, 1002, 100, -300, PN, S2);                                                            /* side console */
            h.poly({V(q * 680, -1300, -300), V(q * 680, 100, -300), V(q * 1002, 100, -123), V(q * 1002, -1300, -123)}, 0x50505C);
            h.poly({V(q * 680, -1300, -300), V(q * 1002, -1300, -300), V(q * 1002, -1300, -123)}, S2); h.poly({V(q * 680, 100, -300), V(q * 1002, 100, -300), V(q * 1002, 100, -123)}, S2);
            V su(0, 560, 0), sv(q * 230, 0, 126); kit_quad(l, V(q * 730, -1150, -272), su, sv, 0x1A3AD0); kit_quad(l, V(q * 730, -520, -272), su, sv, 0x18A060);
            bx(480, -1500, -420, 990, -1470, 60, PN, S2); kit_quad(l, V(q * 540, -1468, -380), V(q * 400, 0, 0), V(0, 0, 280), q > 0 ? 0xD06010 : 0x18A060); kit_lamps(l, V(q * 540, -1468, -60), V(q * 85, 0, 0), V(q * 50, 0, 0), V(0, 0, 40), 5);
            bx(760, -1500, 730, 830, -100, 806, 0x707880, 0x505860); }                                               /* roof pipes */
        kit_quad(h, V(-300, -1500, -560), V(600, 0, 0), V(0, 0, 618), 0x505058); kit_stripes(h, V(-330, -1500, 60), V(660, 0, 0), V(0, 0, 40), 8, YL, BK); h.box(V(-180, -1494, -300), V(180, -1440, -270), 0xB0B0B8);   /* hatch */
        for (int i = 0; i < 6; i++) for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; h.box(V((std::min)(q * 345, q * 660), -1400.f + 200.f * i, -565), V((std::max)(q * 345, q * 660), -1340.f + 200.f * i, -530), 0x3A3A44, 0x2C2C34, 0x2C2C34); }   /* floor ribs either side of the seat */
        kit_quad(l, V(-110, -1440, 798), V(220, 0, 0), V(0, 1250, 0), 0xE8F0FF); kit_lamps(l, V(-560, -1200, 798), V(0, 90, 0), V(50, 0, 0), V(0, 50, 0), 10); kit_lamps(l, V(510, -1200, 798), V(0, 90, 0), V(50, 0, 0), V(0, 50, 0), 10);
        l.add(M_HULLL); }
      h.box(V(-600, -4500, -300), V(600, -1560, 500), G1, G2, DK);                                    /* spine */
      d1k::kit_slab(h, {V(0, -2000, 470), V(0, -4500, 470), V(0, -4800, 2200), V(0, -3600, 2200)}, V(55, 0, 0), G2, DK);         /* fin */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; auto bx = [&](float x0, float y0, float z0, float x1, float y1, float z1, uint32_t c) { h.box(V((std::min)(q * x0, q * x1), y0, z0), V((std::max)(q * x0, q * x1), y1, z1), c); };
          bx(1000, -1500, -560, 1050, 120, -120, G2);                                                /* sill */
          bx(990, -150, -560, 1060, -50, 860, DK); bx(990, -1500, -560, 1060, -1400, 860, DK); bx(990, -1400, 800, 1060, -150, 860, DK);   /* pillars */
          d1k::kit_slab(h, {V(q * 1040, 100, -400), V(q * 6500, -1800, -700), V(q * 6500, -2600, -700), V(q * 1040, -1800, -400)}, V(0, 0, 55), G2, DK);   /* wing */
          bx(6400, -2600, -780, 6600, -500, -620, DK);                                                 /* wingtip gun */
          bx(1100, -3800, -700, 1900, -400, 100, G1); bx(1200, -420, -600, 1800, -350, 0, DK); }      /* engine, its intake */
      h.add(M_HULL); }
    { Mesh q; q.poly({V(-0.5f, 0, -1), V(0.5f, 0, -1), V(0.5f, 0, 0), V(-0.5f, 0, 0)}, 0xE03018); q.add(M_QUAD); }
    { Mesh q; q.poly({V(-0.5f, 0, -1), V(0.5f, 0, -1), V(0.5f, 0, 0), V(-0.5f, 0, 0)}, 0x389850); q.add(M_GQUAD); }
    { Mesh q; qxz(q, -6, -7, 6, 7, 0, 0xC8BC50); q.add(M_YSQ); }       /* unit quad hanging from its top edge: scaled by S */
    { Mesh q; qxz(q, -6, -14, 6, 14, 0, 0x8A3C20); q.add(M_SLOT); } { Mesh q; qxz(q, -6, -14, 6, 14, 0, 0xF8D090); q.add(M_SLOTB); }
    { Mesh q; qxz(q, -24, -1.2f, -3, 1.2f, 0, 0xE8E0B0); qxz(q, 3, -1.2f, 24, 1.2f, 0, 0xE8E0B0); q.add(M_YLINE); }
    { Mesh q; qxz(q, -9, -2, 9, 2, 0, 0xE0DCD0); q.add(M_TOOTH); }
    { Mesh q; qxz(q, -2.2f, -2.2f, 2.2f, 2.2f, 0, 0x6080F0); q.add(M_BLIP_E); }
    { Mesh q; qxz(q, -1.2f, -1.2f, 1.2f, 1.2f, 0, 0x404C88); q.add(M_BLIP_R); }
    { Mesh q; qxz(q, -2, -2, 2, 2, 0, 0xC8D0F0); q.add(M_BLIP_B); }
    for (int v = 0; v < 4; v++) {   /* TV static: random greys on a 10 x 8 grid of a unit square, scaled to the monitor */
        Mesh q; std::mt19937 r(100 + v);
        for (int i = 0; i < 56; i++) for (int j = 0; j < 42; j++) { uint32_t g = 0x58 + (r() % 8) * 0x07, c = g * 0x010101u + ((r() % 7) == 0 ? 0x101000u : 0u);
            qxz(q, -0.5f + i / 56.f, -0.5f + j / 42.f, -0.5f + (i + 1) / 56.f, -0.5f + (j + 1) / 42.f, 0, c); }
        q.add(M_STAT0 + v);
    }
    { Mesh q; const float y = 0;   /* radar: a black scope with thin corner marks and a dot or two (t945, t969) */
      qxz(q, -78, -62, 78, 62, y, 0x08080C);
      const uint32_t cb = 0x9A9AA8; for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) {
          qxz(q, (std::min)(sx * 70.f, sx * 58.f), (std::min)(sz * 54.f, sz * 52.f), (std::max)(sx * 70.f, sx * 58.f), (std::max)(sz * 54.f, sz * 52.f), y - 4, cb);
          qxz(q, (std::min)(sx * 70.f, sx * 68.f), (std::min)(sz * 54.f, sz * 44.f), (std::max)(sx * 70.f, sx * 68.f), (std::max)(sz * 54.f, sz * 44.f), y - 4, cb); }
      q.add(M_RADAR); }
    { Mesh q; const uint32_t bc[8] = {0x081040, 0x0C1858, 0x102068, 0x142C80, 0x183494, 0x2040A8, 0x3050B8, 0x5068C8};   /* the blue band that sweeps the scope (t947.7, t950.1, t953.7, t956.3, t969.0): dark at its top, bright at its leading edge; origin at the bottom, scaled to its height */
      for (int k = 0; k < 8; k++) qxz(q, -70, (7 - k) * 12.f, 70, (8 - k) * 12.f, 0, bc[7 - k]); q.add(M_RADARB); }
    for (int k = 0; k < 2; k++) {   /* the frog alien on the comm monitor, over the static (t961): bright green, a wide smile, big white eyes on top, an arm waving at the right with spread fingers */
        Mesh q; const float y = 0; const uint32_t SK = 0x40B840, SK2 = 0x2C9030, SK3 = 0x60D060;
        ngon_xz(q, -4, -44, 44, 18, y - 2, SK2); ngon_xz(q, -4, -30, 26, 22, y - 3, SK);                                               /* body */
        ngon_xz(q, -6, 6, 42, 26, y - 4, SK); ngon_xz(q, -6, 12, 34, 16, y - 5, SK3);                                                   /* the head */
        ngon_xz(q, -28, 30, 15, 16, y - 6, 0x123814); ngon_xz(q, 12, 30, 15, 16, y - 6, 0x123814);
        ngon_xz(q, -28, 30, 12, 13, y - 7, 0xF4F4E8); ngon_xz(q, 12, 30, 12, 13, y - 7, 0xF4F4E8); ngon_xz(q, -25 , 32, 5, 6, y - 8, 0x101010); ngon_xz(q, 15, 32, 5, 6, y - 8, 0x101010);   /* eyes */
        tri_to(q, V(-34, y - 6, 2), V(22, y - 6, 2), V(-6, y - 6, -12), V(0, -1, 0), 0x103A14); tri_to(q, V(-28, y - 7, 2), V(16, y - 7, 2), V(-6, y - 7, -6), V(0, -1, 0), SK);   /* the smile */
        float wx = k ? 54.f : 62.f, wz = k ? 22.f : 34.f;
        tri_to(q, V(22, y - 2, -40), V(36, y - 2, -48), V(wx + 6, y - 2, wz - 6), V(0, -1, 0), SK); tri_to(q, V(22, y - 2, -40), V(wx + 6, y - 2, wz - 6), V(wx - 6, y - 2, wz), V(0, -1, 0), SK);
        for (int f2 = 0; f2 < 3; f2++) { float a = 1.0f + f2 * 0.5f - (k ? 0.2f : 0.f), ex = wx + cosf(a) * 22.f, ez = wz + sinf(a) * 22.f; tri_to(q, V(wx - 4, y - 3, wz), V(wx + 4, y - 3, wz), V(ex, y - 3, ez), V(0, -1, 0), SK); }
        ngon_xz(q, -4, -40, 16, 12, y - 4, 0xB0E8A8); tri_to(q, V(-28, y - 2, -36), V(-44, y - 2, -40), V(-62, y - 2, 6), V(0, -1, 0), SK);                                         /* other arm */
        q.add(k ? M_FROG1 : M_FROG0); }
    { Mesh q; const float y = 0; const uint32_t RD = 0xB84028;   /* the red EJECT panel (t943): red frame, triangles above and below, the word EJECT */
      qxz(q, -68, -60, 68, 60, y, 0x0A0406);
      qxz(q, -68, 56, 68, 60, y - 3, RD); qxz(q, -68, -60, 68, -56, y - 3, RD); qxz(q, -68, -60, -64, 60, y - 3, RD); qxz(q, 64, -60, 68, 60, y - 3, RD);
      for (int s0 = -1; s0 <= 1; s0 += 2) { const float a = s0 * 36.f, b = s0 * 54.f, w = 21.f, sa = (float)s0;   /* outlined triangles pointing in at the word, top and bottom (t943) */
          tri_to(q, V(-w, y - 3, b), V(w, y - 3, b), V(0, y - 3, a), V(0, -1, 0), RD); tri_to(q, V(-w + 7, y - 4, b - sa * 2.5f), V(w - 7, y - 4, b - sa * 2.5f), V(0, y - 4, a + sa * 7.f), V(0, -1, 0), 0x0A0406); }
      const int L[5] = {4, 9, 4, 2, 19}; const float cell = 3.6f, x0 = -(5 * 6 - 1) * cell * 0.5f;
      for (int i = 0; i < 5; i++) { Mesh g0; glyph(q, L[i], x0 + i * 6 * cell, 22.f, cell, y - 4, 0xB84830, 6.2f); }
      q.add(M_EJECT); }
    { Mesh q; const float y = 0; const uint32_t G = 0x389850, K = 0x08080C, R = 0xC05030;   /* the system icons panel (t945, t953): four rows left of a column of green squares; the right part stays black */
      qxz(q, -68, -60, 68, 60, y, K);
      qxz(q, -62, 34, 12, 52, y - 3, G); qxz(q, -60, 36, 10, 50, y - 4, K); for (int k = 0; k < 2; k++) { float s0 = k ? 1.f : -1.f; qxz(q, -24 + s0 * 26 - 2, 38, -24 + s0 * 26 + 2, 48, y - 5, G); qxz(q, -24 + s0 * 18 - 1, 38, -24 + s0 * 18 + 1, 48, y - 5, G); } qxz(q, -32, 39, -16, 47, y - 5, G); qxz(q, -28, 41, -20, 45, y - 6, K);   /* a box in brackets */
      qxz(q, -62, 12, 12, 30, y - 3, G); qxz(q, -60, 14, 10, 28, y - 4, K); for (int k = 0; k < 9; k++) if (k != 4 && k != 5) qxz(q, -56 + k * 7.f, 16, -53 + k * 7.f, 26, y - 5, G); qxz(q, -28, 19, -20, 23, y - 5, G);   /* a row of bars */
      qxz(q, -62, -12, 12, 6, y - 3, G); qxz(q, -46, -4, -2, -2, y - 4, K); qxz(q, -25, -11, -23, 5, y - 4, K); tri_to(q, V(-40, y - 4, 4), V(-36, y - 4, 4), V(-10, y - 4, -10), V(0, -1, 0), K); tri_to(q, V(-40, y - 4, -10), V(-36, y - 4, -10), V(-10, y - 4, 4), V(0, -1, 0), K);   /* a star on green */
      qxz(q, -62, -38, 12, -20, y - 3, R); qxz(q, -32, -33, -28, -25, y - 4, K); qxz(q, -20, -33, -16, -25, y - 4, K);   /* two ticks on red */
      const float cz[4] = {34, 12, -12, -38}; for (int k = 0; k < 4; k++) qxz(q, 18, cz[k], 28, cz[k] + 18, y - 3, k == 3 ? R : G);
      q.add(M_ICONS); }
}

/* ---- sounds: INVENTED placeholders; the original's "talkative" voice and effects are not on the footage clearly enough to copy ---- */
static void make_sounds() {
    std::mt19937 r(5); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(SND_LASER, 0.22f, [&](float t) { return sinf(6.2831853f * (1500.f - 3800.f * t) * t) * expf(-t * 11.f) * 0.7f; });
    make_sound(SND_EXPL, 1.1f, [&](float t) { return (noise() * expf(-t * 3.5f) * 0.9f + sinf(6.2831853f * 60.f * t) * expf(-t * 4.f) * 0.6f); });
    make_sound(SND_ROCK, 0.45f, [&](float t) { return noise() * expf(-t * 8.f) * 0.7f + sinf(6.2831853f * 90.f * t) * expf(-t * 8.f) * 0.4f; });
    make_sound(SND_HIT, 0.5f, [&](float t) { return (noise() * 0.5f + sinf(6.2831853f * 110.f * t) * 0.5f) * expf(-t * 6.f); });
    make_sound(SND_ENEMY, 0.3f, [&](float t) { return sinf(6.2831853f * (420.f - 300.f * t) * t) * expf(-t * 8.f) * 0.5f; });
    make_sound(SND_HYPER, 1.0f, [&](float t) { return sinf(6.2831853f * (120.f + 1800.f * t * t) * t) * (t < 0.8f ? t / 0.8f : (1.f - t) / 0.2f) * 0.6f; });
    make_sound(SND_CHAOS, 1.6f, [&](float t) { return (sinf(6.2831853f * (50.f + 200.f * t) * t) * 0.6f + noise() * 0.3f) * (t < 1.2f ? 1.f : (1.6f - t) / 0.4f); });
    make_sound(SND_BABBLE, 1.8f, [&](float t) { int s = (int)(t / 0.11f); float f = 180.f + (float)((s * 7919) % 11) * 28.f, e = sinf(3.14159f * fmodf(t, 0.11f) / 0.11f);
        return (sinf(6.2831853f * f * t) + 0.5f * sinf(6.2831853f * f * 2.f * t)) * e * 0.4f * (s % 4 == 3 ? 0.f : 1.f); });
    make_sound(SND_START, 0.6f, [&](float t) { return sinf(6.2831853f * (330.f + 330.f * t) * t) * expf(-t * 3.f) * 0.6f; });
    make_sound(SND_ALARM, 0.5f, [&](float t) { return (fmodf(t, 0.25f) < 0.16f ? sinf(6.2831853f * 720.f * t) : 0.f) * 0.5f; });
    make_sound(SND_CLEAR, 0.7f, [&](float t) { return (sinf(6.2831853f * (t < 0.25f ? 520.f : t < 0.5f ? 660.f : 780.f) * t)) * expf(-fmodf(t, 0.25f) * 6.f) * 0.5f; });
    make_sound(SND_MISSILE, 0.6f, [&](float t) { return (noise() * 0.5f + sinf(6.2831853f * (220.f + 500.f * t) * t) * 0.4f) * (1.f - t / 0.6f); });
    /* OUR riff, standing in for the stereo guitar-rock soundtrack on CD (Your Commodore 79): distorted power chords, 8 s; nothing of the real music is known */
    make_sound(SND_RIFF, 8.f, [&](float t) { static const float roots[8] = {82.4f, 82.4f, 98.f, 82.4f, 82.4f, 110.f, 98.f, 73.4f}; const float beat = fmodf(t, 1.f); const float f = roots[(int)t % 8]; const float th = 6.2831853f * f * t;
        float g = (sinf(th) + sinf(th * 1.5f) + 0.6f * sinf(th * 2.f)) * 3.f; g = g / (1.f + fabsf(g)); float kick = beat < 0.12f ? sinf(6.2831853f * 60.f * t) * (1.f - beat / 0.12f) : 0.f; return (g * (0.55f + 0.45f * expf(-beat * 3.f)) + kick) * 0.55f; });
}

/* ---- HUD bitmaps ---- */
static void make_hud_bitmaps() {
    for (int d = 0; d < 10; d++) make_digit(BM_GDIG + d, d, 0u, RGBA_GRN);   /* hollow outlined digits */
    { const int W = 11, H = 7; std::vector<uint32_t> px(W * H, 0);   /* bow-tie icon */
      for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (x < 3 || x >= W - 3 || (y == 3 && x >= 3 && x < W - 3)) px[y * W + x] = RGBA_GRN;
      add_bitmap(BM_TIE, W, H, px); }
    { std::vector<uint32_t> px(2 * 4, RGBA_GRN); add_bitmap(BM_TICK_S, 2, 4, px); }
    { std::vector<uint32_t> px(2 * 8, RGBA_GRN); add_bitmap(BM_TICK_T, 2, 8, px); }
    { std::vector<uint32_t> px(4 * 4, 0xFF60D060u); add_bitmap(BM_SOLID, 4, 4, px); }
    for (int k = 0; k < 4; k++) {   /* the four small triangles round the sights, points outward: up, left, down, right (t958.56, t959.72) */
        const int W = 9, H = 9; std::vector<uint32_t> px(W * H, 0);
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { bool on = false;
            if (k == 0) on = y >= 1 && y < 8 && abs(x - 4) <= (y - 1) / 2 && (x == 4 - (y - 1) / 2 || x == 4 + (y - 1) / 2 || y == 7);
            if (k == 2) on = y >= 1 && y < 8 && abs(x - 4) <= (7 - y) / 2 && (x == 4 - (7 - y) / 2 || x == 4 + (7 - y) / 2 || y == 1);
            if (k == 1) on = x >= 1 && x < 8 && abs(y - 4) <= (x - 1) / 2 && (y == 4 - (x - 1) / 2 || y == 4 + (x - 1) / 2 || x == 7);
            if (k == 3) on = x >= 1 && x < 8 && abs(y - 4) <= (7 - x) / 2 && (y == 4 - (7 - x) / 2 || y == 4 + (7 - x) / 2 || x == 1);
            if (on) px[y * W + x] = RGBA_GRN; }
        add_bitmap(k == 3 ? (int)BM_TRI_R : BM_TRI_U + k, W, H, px); }
}
static void blit2(VPIX_Frame &f, int id, int w, int h, float x, float y, float dw, float dh) {
    VPIX_Blit b = {}; b.handle = d1k::BMPH + id; b.sw = w; b.sh = h; b.x = (int)x; b.y = (int)y; b.dw = dw; b.dh = dh; b.key = 0; b.alpha = 1.f; b.prio = 1000000;
    b.tint[0] = b.tint[1] = b.tint[2] = 1.f; f.blits.push_back(b);
}
static void green_num(VPIX_Frame &f, int v, int nd, float x, float y, float sp, float sc) {
    int p = 1; for (int i = 1; i < nd; i++) p *= 10;
    for (int i = 0; i < nd; i++) { int dgt = (v / p) % 10; p = (std::max)(p / 10, 1); blit(f, d1k::BM_DIG + BM_GDIG + dgt, d1k::DW, d1k::DH, x + i * sp, y, sc); blit(f, d1k::BM_DIG + BM_GDIG + dgt, d1k::DW, d1k::DH, x + i * sp + 0.6f, y, sc); }
}

/* ---- the world ---- */
static void burst(V at, int n, float speed, float life, float sz, bool white = false) {
    if (v1knet::on && v1knet::master) { NetEv &e = evr[ev_total % 6]; e.p[0] = at.x; e.p[1] = at.y; e.p[2] = at.z; e.n = (uint8_t)(std::min)(n, 255); ev_total = (uint8_t)((ev_total + 1) % 240); }   /* linked: pods draw the same bursts */
    if (n >= 14) { Fire fi; fi.p = at; fi.sz = clampf(1500.f + sz * 3.f, 1800.f, 7000.f); fires.push_back(fi);   /* a kill: the cream fireball with bits of the wreck in it, then a cloud of specks as it goes (t967.3-t969.3) */
        for (int i = 0; i < 4; i++) { Part q; q.p = at; q.v = rnd_dir() * rnd(400.f, 1400.f); q.life = q.life0 = rnd(0.7f, 1.1f); q.sz = fi.sz * 0.1f; q.id = i & 1 ? M_SPARK_B : M_SPARK_O; parts.push_back(q); }
        for (int i = 0; i < 36; i++) { Part q; q.p = at; q.v = rnd_dir() * rnd(1500.f, 4500.f); q.delay = rnd(0.9f, 1.3f); q.life = q.life0 = rnd(0.5f, 1.1f); q.sz = fi.sz * 0.012f; q.id = rnd(0, 1) < 0.6f ? M_SPARK_O : M_SPARK_W; parts.push_back(q); }
        n = 0; }
    for (int i = 0; i < n; i++) { Part q; q.p = at; q.v = rnd_dir() * (speed * rnd(0.25f, 1.f)); q.life = q.life0 = life * rnd(0.5f, 1.f); q.sz = sz * rnd(0.3f, 0.6f);
        q.id = (white && rnd(0, 1) < 0.5f) ? M_SPARK_W : (rnd(0, 1) < 0.7f ? M_SPARK_O : M_SPARK_W); parts.push_back(q); }
    if (parts.size() > 260) parts.erase(parts.begin(), parts.begin() + (parts.size() - 260));
}
static void add_score(int n) { score = (std::min)(score + n, 99999); }
static void spawn_rock(V p, V v, int size, bool shower = false) {
    Rock r; r.p = p; r.v = v; r.size = size; r.shower = shower; r.r = size == 2 ? 6000.f : size == 1 ? 3300.f : 1800.f; r.hp = size == 2 ? 3.f : 1.f; r.var = size == 2 ? 2 : (int)(d1k::rng() % 2); r.w = rnd_dir() * rnd(0.2f, 0.9f);
    rotate(r.R, V(rnd(-1, 1), rnd(-1, 1), rnd(-1, 1)), 3.f); rocks.push_back(r);
}
static void spawn_foe(int kind, V p, float hp = 1.f, float sp = 1.f) {
    Foe e; e.p = p; e.kind = kind; e.hp = hp; e.sp = sp; e.fire_t = rnd(1.f, 3.f); e.ax = rnd_dir(); e.v = rnd_dir() * 4000.f; e.ax_t = rnd(1.f, 3.f); foes.push_back(e);
}
static V rand_pos(float rmin, float rmax, float from_ship) {
    V p = START_P;
    for (int i = 0; i < 16; i++) { p = rnd_dir() * rnd(rmin, rmax); if (len(p - BASE_P) > 45000.f && len(p - ship.p) > from_ship) break; }
    return p;
}
static void reset_ship() {
    ship = Ship(); ship.p = START_P; ship.speed = cruise; ship.inv = 2.f; eject_on = 5.7f; comm = C_STATIC; comm_t = 5.7f;   /* EJECT lit and static first, for 5.7 s (t941-t946.7) */
}
static int enemy_count() { int n = 0; for (const Foe &e : foes) if (e.kind != 3) n++; return n; }
static void spawn_mode(int m) {   /* the four levels are named in Computer+Videogiochi 9; what each holds beyond the press lines is INVENTED */
    rocks.clear(); foes.clear(); bolts.clear(); mom.on = false; field_up = false; duct_t = -1.f; spawn_cd = 0.f; shower_t = 20.f; wave_alt = false;
    for (int i = 0; i < n_rocks; i++) { V p = rand_pos(35000.f, 115000.f, 30000.f); spawn_rock(p, rnd_dir() * rnd(1500.f, 4500.f), 2); }
    if (m == 0) { for (int i = 0; i < 3; i++) spawn_foe(0, rand_pos(50000.f, 110000.f, 40000.f)); }   /* Solo Trainer: "practise" in the asteroids, blue ships that do not fire back */
    else if (m == 1) { V c = rand_pos(60000.f, 110000.f, 50000.f); for (int i = 0; i < n_swarm; i++) spawn_foe(0, c + rnd_dir() * 8000.f); }   /* Solo Ace: swarms and tail-chasers */
    else if (m == 2) { for (int i = 0; i < 3; i++) spawn_foe(0, rand_pos(60000.f, 110000.f, 50000.f), 2.f, 1.35f); }   /* Dogfight: linked pilots on the real thing; computer pilots here */
    else { mom = Mom(); mom.on = true; for (int k = 0; k < 4; k++) mom.gen[k] = duct_test ? 0.f : gen_hp0; field_up = !duct_test; mom.p = V(-60000, -90000, 30000); mom.v = V(2000, 4000, 0); mom.way = V(0, 0, 0); mom.R = look_at(mom.p, mom.p + mom.v);   /* Battlesphere: the two fleets and the battleship */
        for (int i = 0; i < 5; i++) spawn_foe(0, mom.p + rnd_dir() * 40000.f); for (int i = 0; i < 4; i++) spawn_foe(3, BASE_P + rnd_dir() * 30000.f + V(0, 0, 25000), 2.f); }
    dl("t=%.1f mode %d start: rocks %zu foes %zu mother %d", sim_t, m, rocks.size(), foes.size(), (int)mom.on);
}
static void start_round() {
    score = 0; time_left = game_time; chaos_left = chaos_max; hyper_cd = 0; hyper_t = 0; hyper_age = 1e9f; chaos_r = -1.f; kills = 0; eject_t = 0; riff_t = 0;
    parts.clear(); fires.clear(); reset_ship(); spawn_mode(game_mode); bar_v = 0;
    sfx(0, SND_START, 100); sfx(6, SND_BABBLE, 110);
    if (v1knet::on) { v1knet::begin_round(4); for (int k = 1; k < 4; k++) { crew_on[k] = v1knet::seat_pod[k] != 0; if (crew_on[k]) crew_reset(k); } }   /* linked: a ship for every pod in the round; empty seats have no ship */
    dl("t=%.1f round start (clock %.0f)", sim_t, time_left);
}

static void ship_destroy() {
    ship.alive = false; ship.dead_t = 3.f; burst(ship.p, 80, 9000.f, 1.6f, 500.f, true); sfx(3, SND_EXPL, 127);
    dl("t=%.1f ship destroyed, clock %.0f", sim_t, time_left);   /* "die and continue" (Simon): no cost but the time */
}
static void ship_hit(float dmg) {
    if (!ship.alive || ship.inv > 0) return;
    ship.shield -= dmg; ship.inv = 0.6f; sfx(4, SND_HIT, 120); VPIX_Haptic(2, 0.15f, 0.8f);
    if (ship.shield <= 0) ship_destroy();
}
static void hit_rock(int j, float dmg = 1.f) {
    rocks[j].hp -= dmg; if (rocks[j].hp > 0) { burst(rocks[j].p + rnd_dir() * rocks[j].r * 0.8f, 5, 2500.f, 0.4f, 90.f); sfx(10, SND_ROCK, 60); return; }
    Rock r = rocks[j]; rocks.erase(rocks.begin() + j); add_score(10); kills++;   /* INVENTED: nothing shows a rock being shot up; no splitting */
    burst(r.p, 26, 5000.f, 1.f, r.r * 0.12f); sfx(10, SND_ROCK, 100);
}
static void kill_foe(int j, bool by_me = true) {
    Foe e = foes[j]; foes.erase(foes.begin() + j); kills++;
    if (by_me && e.kind != 3) add_score(10);   /* every kill on the footage is +10: 00190 to 00240 */
    burst(e.p, 40, 9000.f, 1.3f, 420.f, true); sfx(3, SND_EXPL, 110);
    if ((kills % 8) == 0) sfx(6, SND_BABBLE, 80);
}
static void hit_foe(int j, float dmg, bool by_me = true) {
    foes[j].hp -= dmg;
    if (foes[j].hp <= 0) kill_foe(j, by_me); else { burst(foes[j].p, 8, 4000.f, 0.6f, 200.f, true); sfx(10, SND_ROCK, 60); }
}
static void end_round(int why) {
    over_why = why; st = S_OVER; st_t = 0;
    if (why == 1) add_score((int)(time_left * 10.f));   /* INVENTED: unused clock turns into points */
    dl("t=%.1f round over (%s): score %d clock %.0f", sim_t, why == 1 ? "mission complete" : why == 2 ? "ejected" : "time up", score, time_left);
}
static void kill_mother() {
    burst(mom.p, 140, 16000.f, 2.4f, 900.f, true); sfx(3, SND_EXPL, 127); add_score(500); mom.on = false;
    for (int i = (int)foes.size() - 1; i >= 0; i--) if (foes[i].kind != 3) { burst(foes[i].p, 20, 6000.f, 1.f, 350.f, true); foes.erase(foes.begin() + i); }
    dl("t=%.1f battleship destroyed", sim_t);
}
static bool hit_gen(V at, float dmg) {   /* generators take shots from outside the field; when the last goes, so does the field (the means is INVENTED, the dropping is MC 127) */
    if (!mom.on) return false;
    for (int k = 0; k < 4; k++) if (mom.gen[k] > 0 && len(at - (mom.p + to_world(mom.R, GEN_L[k]))) < 4500.f) { mom.gen[k] -= dmg; burst(at, 6, 4000.f, 0.5f, 160.f, true); sfx(10, SND_ROCK, 80);
        if (mom.gen[k] <= 0) { burst(mom.p + to_world(mom.R, GEN_L[k]), 40, 9000.f, 1.3f, 700.f, true); sfx(3, SND_EXPL, 120); add_score(50); bool any = false; for (int g = 0; g < 4; g++) any = any || mom.gen[g] > 0; if (!any) { field_up = false; sfx(9, SND_CLEAR, 120); dl("t=%.1f force field down", sim_t); } }
        return true; }
    return false;
}
static void foe_fire(V from, V target, V tvel, float jitter, bool big, float spd, bool ally = false) {
    float d0 = len(target - from); V lead = target + tvel * (d0 / spd * 0.6f);
    V d = norm(lead - from + rnd_dir() * (d0 * jitter));
    Bolt b; b.p = from; b.v = d * spd; b.mine = false; b.big = big; b.ally = ally; bolts.push_back(b); if (!ally) sfx(5, SND_ENEMY, big ? 100 : 70);
}

static const Ship &near_ship(V p) {   /* the pilot a foe goes for: nearest alive ship of the team (alone: yours) */
    const Ship *b = &ship; float bd = ship.alive ? len(ship.p - p) : 1e30f;
    if (v1knet::on && v1knet::master) for (int k = 1; k < 4; k++) if (crew_on[k] && crew[k].s.alive) { float d = len(crew[k].s.p - p); if (d < bd) { bd = d; b = &crew[k].s; } }
    return *b;
}
static bool crew_hit(V at, bool big) {   /* an enemy shot against the other pods' ships */
    if (!(v1knet::on && v1knet::master)) return false;
    for (int k = 1; k < 4; k++) if (crew_on[k] && crew[k].s.alive && len(at - crew[k].s.p) < 2400.f) { swap_ps(crew[k]); ship_hit(big ? 14.f : 9.f); swap_ps(crew[k]); return true; }
    return false;
}
/* ---- one step of the world ---- */
static void step_rocks(float dt) {
    for (size_t i = 0; i < rocks.size();) { Rock &r = rocks[i]; r.p += r.v * dt; rotate(r.R, r.w, dt);
        float d = len(r.p);
        if (d > arena_r * 0.98f && dot(r.p, r.v) > 0) { if (r.shower) { rocks[i] = rocks.back(); rocks.pop_back(); continue; } V n = r.p * (1.f / d); r.v = r.v - n * (2.f * dot(r.v, n)); }
        i++; }
}
static void step_foes(float dt) {
    for (size_t i = 0; i < foes.size(); i++) {
        Foe &e = foes[i]; e.t += dt; e.fire_t -= dt; e.ax_t -= dt;
        if (e.ax_t <= 0) { e.ax = rnd_dir(); e.ax_t = rnd(2.f, 4.f); }
        const Ship &ts = near_ship(e.p); V tgt_p = ts.p, tgt_v = ts.v; bool have = ts.alive && st == S_PLAY;   /* a foe goes for the nearest of your side; your side goes for the nearest foe */
        float nd = have && e.kind != 3 ? len(ts.p - e.p) : 1e30f; if (e.kind == 3) have = false;
        for (const Foe &o : foes) if ((o.kind == 3) != (e.kind == 3)) { float dd = len(o.p - e.p); if (dd < nd) { nd = dd; tgt_p = o.p; tgt_v = o.v; have = true; } }
        V to = tgt_p - e.p; float d = len(to); V dn = to * (1.f / (std::max)(d, 1.f)), want; float spd = (e.kind == 1 ? chaser_speed : foe_speed) * e.sp;
        if (!have) want = e.ax;
        else if (e.kind == 1) { V tgt = ts.p - pitched(yawed(ts.yaw), ts.pitch).r[1] * 6000.f; want = norm(tgt - e.p); if (d < 9000.f) spd *= 0.75f; }
        else if (d > 32000.f) want = norm(dn + e.ax * 0.35f);
        else { V side = norm(cross(dn, e.ax)); want = norm(side * 0.9f + dn * clampf((d - 22000.f) / 22000.f, -1.f, 1.f)); }
        for (size_t k = 0; k < foes.size(); k++) if (k != i) { V a = e.p - foes[k].p; float l = len(a); if (l < 7000.f && l > 1.f) want = norm(want + a * (1.f / l) * 1.2f); }
        e.v += (want * spd - e.v) * clampf(dt * 1.2f, 0.f, 1.f); e.p += e.v * dt;
        box_push(e.p, BASE_P, BASE_H, 2500.f);
        float dc = len(e.p); if (dc > arena_r) e.p = e.p * (arena_r / dc);
        V fw = e.v; fw.z += 0.001f; e.R = look_at(e.p, e.p + fw);
        if (st == S_PLAY && game_mode > 0 && have && e.fire_t <= 0 && d < (e.kind == 1 ? 26000.f : 60000.f)) {   /* no shot from a blue ship is on the footage: all this is INVENTED */
            foe_fire(e.p + e.R.r[1] * 1800.f, tgt_p, tgt_v, 0.05f / enemy_skill, false, 42000.f, e.kind == 3);
            e.fire_t = rnd(1.6f, 3.4f) / enemy_skill * (e.kind == 1 ? 0.6f : 1.f);
        }
    }
}
static void step_mother(float dt) {
    if (!mom.on) return;
    if (duct_t < 0) { mom.way_t -= dt; if (mom.way_t <= 0) { mom.way = rnd_dir() * rnd(30000.f, 90000.f); mom.way_t = 12.f; }
        V want = norm(mom.way - mom.p) * 5000.f; mom.v += (want - mom.v) * clampf(dt * 0.4f, 0.f, 1.f); mom.p += mom.v * dt;
        box_push(mom.p, BASE_P, BASE_H, 45000.f);
        V fw = mom.v; fw.z += 0.001f; mom.R = look_at(mom.p, mom.p + fw); }
    mom.spawn_t -= dt; mom.fire_t -= dt;
    if (mom.spawn_t <= 0 && enemy_count() < 6) { V bay = mom.p + mom.R.r[1] * 20000.f; for (int i = 0; i < 2; i++) spawn_foe(0, bay + rnd_dir() * 2500.f); mom.spawn_t = 9.f; sfx(5, SND_ENEMY, 70); }
    const Ship &mt = near_ship(mom.p);
    if (mom.fire_t <= 0 && mt.alive && len(mt.p - mom.p) < 110000.f) {
        mom.side ^= 1; V tur = mom.p + mom.R.r[0] * (mom.side ? 21000.f : -21000.f);
        foe_fire(tur, mt.p, mt.v, 0.04f / enemy_skill, true, 38000.f); mom.fire_t = 2.4f / enemy_skill;
    }
}
static void step_bolts(float dt) {
    for (size_t i = 0; i < bolts.size();) {
        Bolt &b = bolts[i];
        if (b.missile) {   /* gaze-guided (MC 127): takes the foe nearest its line, else flies on and bursts at 0.6 s as the pair at t947.5 does */
            V fw = norm(b.v); float best = 0.91f; V tg; bool lk = false; for (const Foe &e : foes) if (e.kind != 3) { V r = norm(e.p - b.p); float c = dot(r, fw); if (c > best) { best = c; tg = e.p; lk = true; } }
            b.locked = b.locked || lk; if (lk) { float sp = len(b.v); b.v = norm(fw + (norm(tg - b.p) - fw) * clampf(dt * 4.f, 0.f, 1.f)) * sp; } }
        b.p += b.v * dt; b.t += dt; bool gone = b.t > (b.missile ? (b.locked ? 3.f : 0.62f) : b.mine ? 2.2f : 5.f) || len(b.p) > 195000.f;
        const bool miss = b.missile; const V at = b.p; const float dm = miss ? 3.f : 1.f; const bool mine = b.mine, ally = b.ally, big = b.big;
        if (gone && miss) { burst(at, 26, 5000.f, 1.f, 1000.f); sfx(3, SND_EXPL, 90); }
        if (!gone && mine) {
            for (size_t j = 0; j < rocks.size(); j++) if (len(at - rocks[j].p) < rocks[j].r * 0.9f + 150.f) { hit_rock((int)j, dm); gone = true; break; }
            if (!gone) for (size_t j = 0; j < foes.size(); j++) if (foes[j].kind != 3 && len(at - foes[j].p) < (miss ? 3200.f : 2300.f)) { hit_foe((int)j, dm); gone = true; break; }
            if (!gone && hit_gen(at, dm)) gone = true;
            if (!gone && mom.on && field_up && len(at - mom.p) < 34000.f) { burst(at, 5, 3000.f, 0.4f, 160.f, true); gone = true; }   /* the field stops it */
            if (!gone && mom.on && !field_up && len(at - mom.p) < 17000.f) { burst(at, 5, 3000.f, 0.4f, 160.f); gone = true; }     /* armour: only the reactor counts */
            if (!gone && in_box(at, BASE_P, BASE_H)) { burst(at, 5, 3000.f, 0.4f, 120.f); gone = true; }
        } else if (!gone && ally) {
            for (size_t j = 0; j < foes.size(); j++) if (foes[j].kind != 3 && len(at - foes[j].p) < 2400.f) { hit_foe((int)j, 1.f, false); gone = true; break; }
        } else if (!gone) {
            if (ship.alive && len(at - ship.p) < 2400.f) { ship_hit(big ? 14.f : 9.f); gone = true; }
            else if (crew_hit(at, big)) gone = true;
            else if (in_box(at, BASE_P, BASE_H)) gone = true;
            else for (size_t j = 0; j < foes.size(); j++) if (foes[j].kind == 3 && len(at - foes[j].p) < 2400.f) { hit_foe((int)j, 1.f, false); gone = true; break; }
        }
        if (gone) { bolts[i] = bolts.back(); bolts.pop_back(); } else i++;
    }
}
static void step_parts(float dt) {
    for (size_t i = 0; i < fires.size();) { fires[i].t += dt; if (fires[i].t > 1.4f) { fires[i] = fires.back(); fires.pop_back(); } else i++; }
    for (size_t i = 0; i < parts.size();) { Part &q = parts[i]; if (q.delay > 0) { q.delay -= dt; q.p += q.v * dt * 0.1f; i++; continue; } q.p += q.v * dt; q.v = q.v * (1.f - 0.6f * dt); q.life -= dt;
        if (q.life <= 0) { parts[i] = parts.back(); parts.pop_back(); } else i++; }
}
static void collide_ship() {
    if (!ship.alive) return;
    for (size_t j = 0; j < rocks.size(); j++) { Rock &r = rocks[j]; V d = ship.p - r.p; float dd = len(d), m = r.r * 0.85f + 1500.f;
        if (dd < m) { V n = d * (1.f / (std::max)(dd, 1.f)); ship.p = r.p + n * m; ship.speed *= 0.4f; r.v = r.v - n * 1500.f;
            if (ship.inv <= 0) burst(ship.p, 10, 3000.f, 0.5f, 200.f, true);
            ship_hit(6.f + 5.f * r.size); if (!ship.alive) return; } }
    for (size_t j = 0; j < foes.size();) { if (foes[j].kind != 3 && len(foes[j].p - ship.p) < 3000.f) { kill_foe((int)j); ship_hit(10.f); if (!ship.alive) return; continue; } j++; }
    if (box_push(ship.p, BASE_P, BASE_H, 1800.f)) { ship.speed *= 0.3f; ship_hit(15.f); if (!ship.alive) return; }
    if (mom.on && duct_t < 0) { V d = ship.p - mom.p; float dd = len(d), m = field_up ? 35000.f : 18500.f;
        if (!field_up) { V mouth = mom.p + mom.R.r[1] * (DUCT_Y0 - 1500.f); if (len(ship.p - mouth) < 7000.f) { duct_t = 0.f; sfx(7, SND_HYPER, 110); dl("t=%.1f into the duct", sim_t); return; } }   /* MC 127: fly into the cooling ducts of the reactor */
        if (dd < m) { ship.p = mom.p + d * (m / (std::max)(dd, 1.f)); ship.speed *= 0.3f; ship_hit(field_up ? 6.f : 15.f); } }
}
static void step_chaos(float dt) {
    if (chaos_r < 0) return;
    chaos_r += 80000.f * dt;
    for (size_t j = 0; j < foes.size();) { if (foes[j].kind != 3 && len(foes[j].p - chaos_c) < chaos_r) { kill_foe((int)j); continue; } j++; }
    for (size_t j = 0; j < rocks.size();) { if (len(rocks[j].p - chaos_c) < chaos_r) { Rock r = rocks[j]; rocks.erase(rocks.begin() + j); add_score(10); kills++; burst(r.p, 14, 5000.f, 1.f, r.r * 0.12f); continue; } j++; }
    for (size_t j = 0; j < bolts.size();) { if (!bolts[j].mine && !bolts[j].ally && len(bolts[j].p - chaos_c) < chaos_r) { bolts[j] = bolts.back(); bolts.pop_back(); continue; } j++; }
    if (mom.on && !chaos_hit_mom && len(mom.p - chaos_c) < chaos_r) { chaos_hit_mom = true; for (int k = 0; k < 4; k++) if (mom.gen[k] > 0) { hit_gen(mom.p + to_world(mom.R, GEN_L[k]), mom.gen[k]); break; } }
    if (chaos_r > 100000.f) chaos_r = -1.f;
}
static void step_ship(const Ctl &c, float dt) {
    if (!ship.alive) {
        ship.dead_t -= dt;
        if (ship.dead_t <= 0) { reset_ship(); sfx(0, SND_START, 90); }
        return;
    }
    ship.inv = (std::max)(ship.inv - dt, 0.f); ship.fire_cool -= dt; ship.miss_cool -= dt; hyper_cd -= dt;
    if (duct_t >= 0) {   /* in the duct: the ship is carried along it and the reactor goes (how long it takes is INVENTED) */
        duct_t += dt; float u = clampf(duct_t / 2.2f, 0.f, 1.f); V fw = mom.R.r[1]; ship.yaw = atan2f(-fw.x, fw.y); ship.pitch = asinf(clampf(fw.z, -1.f, 1.f));
        ship.p = mom.p + fw * (DUCT_Y0 - 1500.f + (DUCT_Y1 - 400.f - DUCT_Y0 + 1500.f) * u); ship.v = fw * 14000.f; ship.speed = 14000.f; return; }
    ship.yaw -= c.yaw * turn_rate * dt;                              /* stick right = turn right */
    ship.pitch = clampf(ship.pitch + c.pit * pitch_rate * dt, -1.45f, 1.45f);
    float tgt = c.thr >= 0 ? cruise + c.thr * (max_speed - cruise) : cruise * (1.f + c.thr * 0.85f);
    ship.speed += (tgt - ship.speed) * clampf(dt * 1.5f, 0.f, 1.f);
    M sh = basis(); ship.v = sh.r[1] * ship.speed; ship.p += ship.v * dt;
    float dc = len(ship.p); if (dc > arena_r) ship.p = ship.p * (arena_r / dc);   /* the field: nothing leaves (Amiga Format 23) */
    if (c.weapon) { ship.weapon ^= 1; sfx(9, SND_CLEAR, 50); }       /* "upper right button selects weapons" (Computer+Videogiochi 9) */
    if (c.fire && ship.weapon == 0 && ship.fire_cool <= 0) {         /* guns follow the gaze; one stick from under the nose (footage) */
        ship.fire_cool = fire_gap; ship.side ^= 1;
        V mz = ship.p + to_world(sh, V(ship.side ? 120.f : -120.f, 1300.f, -520.f)), at = ship.p + norm(c.aim) * 40000.f;
        Bolt b; b.p = mz; b.v = norm(at - mz) * bolt_speed + ship.v; b.mine = true; bolts.push_back(b); sfx(1, SND_LASER, 100, ship.side ? 20 : -20);
    }
    if (((c.fire && ship.weapon == 1) || c.a_miss) && ship.miss_cool <= 0) {         /* a pair of missiles, 80 m/s measured off t947.04-t947.4; the gap is INVENTED */
        ship.miss_cool = 1.2f; for (int k = 0; k < 2; k++) { Bolt b; b.p = ship.p + to_world(sh, V(k ? 3200.f : -3200.f, 2000.f, 1300.f)); V at = ship.p + norm(c.aim) * 60000.f; b.v = norm(at - b.p) * 80000.f + ship.v; b.mine = true; b.missile = true; bolts.push_back(b); }
        sfx(1, SND_MISSILE, 110);
    }
    if (c.hyper && hyper_cd <= 0 && hyper_t <= 0) { hyper_t = HYPER_LEN; hyper_age = 0.f; hyper_done = false; hyper_cd = hyper_cd_len; comm = C_STATIC; comm_t = 5.8f; sfx(7, SND_HYPER, 110); dl("t=%.1f hyperspace", sim_t); }
    if (hyper_t > 0) {
        hyper_t -= dt;
        if (!hyper_done && hyper_t < HYPER_LEN - 0.12f) {   /* the jump, at the first yellow: somewhere quiet, away from the base and the foes (where it lands is INVENTED) */
            hyper_done = true; V best = ship.p; float bs = -1.f;
            for (int i = 0; i < 8; i++) { V p = rnd_dir() * rnd(20000.f, 100000.f); if (len(p - BASE_P) < 45000.f) continue;
                float s = 1e9f; for (const Foe &e : foes) s = (std::min)(s, len(e.p - p)); if (mom.on) s = (std::min)(s, len(mom.p - p) - 40000.f);
                if (s > bs) { bs = s; best = p; } }
            ship.p = best; ship.yaw = rnd(-3.14159f, 3.14159f); ship.pitch = 0; ship.inv = 1.5f; }
    }
    if (c.chaos && chaos_left > 0 && chaos_r < 0 && hyper_t <= 0) { chaos_left--; chaos_r = 0.f; chaos_c = ship.p; chaos_hit_mom = false; sfx(8, SND_CHAOS, 127); dl("t=%.1f chaos charge", sim_t); }
}
static void step_mode(float dt) {
    if (duct_t > 2.2f && mom.on) { kill_mother(); duct_t = -1.f; end_round(1); return; }
    /* asteroid showers (Computer+Videogiochi 9): streams of small rocks across the sphere; how often and how many is INVENTED */
    shower_t -= dt; if (shower_t <= 0 && rocks.size() < 18) { shower_t = rnd(22.f, 34.f); V d = rnd_dir(), s = norm(cross(d, rnd_dir())), u = cross(d, s), o = d * (arena_r * -0.9f) + s * rnd(-40000.f, 40000.f) + u * rnd(-40000.f, 40000.f);
        for (int i = 0; i < 7; i++) spawn_rock(o + s * rnd(-25000.f, 25000.f) + u * rnd(-25000.f, 25000.f) - d * rnd(0.f, 10000.f), d * rnd(16000.f, 26000.f), i % 3 == 0 ? 1 : 0, true); dl("t=%.1f asteroid shower", sim_t); }
    spawn_cd -= dt; if (spawn_cd > 0) return; const int en = enemy_count();
    if (game_mode == 0 && en < 3) { spawn_foe(0, rand_pos(50000.f, 110000.f, 40000.f)); spawn_cd = 4.f; }
    else if (game_mode == 1 && en <= 1) { wave_alt = !wave_alt; V sh = basis().r[1]; if (wave_alt) for (int i = 0; i < n_chasers; i++) spawn_foe(1, ship.p - sh * 45000.f + rnd_dir() * 9000.f, 2.f); else { V c = rand_pos(60000.f, 110000.f, 50000.f); for (int i = 0; i < n_swarm; i++) spawn_foe(0, c + rnd_dir() * 8000.f); } spawn_cd = 3.f; sfx(6, SND_BABBLE, 90); }
    else if (game_mode == 2 && en < 3) { spawn_foe(0, rand_pos(60000.f, 110000.f, 50000.f), 2.f, 1.35f); spawn_cd = 5.f; }
    else spawn_cd = 1.f;
}
static void step_comm(float dt) {
    hyper_age += dt; eject_on = (std::max)(eject_on - dt, 0.f);
    if (comm_t > 0) { comm_t -= dt; if (comm_t <= 0) comm = C_RADAR; }
    float want = hyper_age < 5.8f ? clampf(hyper_age / 3.f, 0.f, 1.f) : clampf((ship.speed - cruise * 0.9f) / (max_speed - cruise), 0.f, 1.f);   /* the bar's bright part: after a jump it spreads over 3 s (t963-t966); speed otherwise is INVENTED */
    bar_v += (want - bar_v) * clampf(dt * 3.f, 0.f, 1.f);
}

/* ---- input ---- */
static bool key_down(int vk) {
#ifdef _WIN32
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
#else
    (void)vk; return false;
#endif
}
static M cam_basis(const VPIX_Input &in, const M &sh) {   /* the eye's axes in the world: the live head on top of the ship */
    M c = sh;
    if (in.head.valid) for (int k = 0; k < 3; k++) c.r[k] = norm(to_world(sh, V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2])));
    return c;
}
static Ctl read_ctl(const VPIX_Input &in, const M &sh, float dt) {
    Ctl c; auto dz = [](float v) { return fabsf(v) < 0.12f ? 0.f : v; }; float jx = dz(in.joy_r[0]), jy = dz(in.joy_r[1]); bool kh = false, kx = false, kb = false, kr = false; float thr = in.vr ? (in.brake > 0.1f ? in.brake : -0.5f) : 0.f;   /* Bobby 11-03, VR: right thumbstick flies, left trigger is speed, let off to slow */
    if (!in.vr) { float ax = (key_down('D') ? 1.f : 0.f) - (key_down('A') ? 1.f : 0.f), ay = (key_down('W') ? 1.f : 0.f) - (key_down('S') ? 1.f : 0.f); if (fabsf(ax) > fabsf(jx)) jx = ax; if (fabsf(ay) > fabsf(jy)) jy = ay;
        thr = (key_down('Q') ? 1.f : 0.f) - (key_down('E') ? 1.f : 0.f); kh = key_down('H'); kx = key_down('X'); kb = key_down(VK_BACK); kr = key_down('R'); }
    c.yaw = clampf(jx, -1, 1); c.pit = -clampf(jy, -1, 1); c.thr = clampf(thr, -1, 1);   /* "like VTOL": the stick flies, pushed forward = nose down */
    c.fire = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f; c.a_miss = in.vr && in.fire2;   /* Bobby 11-03, VR: A fires missiles whichever weapon is selected */
    c.hyper = kh || (in.vr && in.start); c.chaos = kx || (in.vr && in.b2 && !in.top);
    static bool w_was = false; bool w = kr || (in.vr && in.top && !in.b2); c.weapon = w && !w_was; w_was = w;
    bool ej = kb || (in.top && in.b2);
    eject_t = ej ? eject_t + dt : 0.f; c.eject = eject_t > 1.2f;
    c.aim = in.head.valid ? norm(to_world(sh, V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2]))) : sh.r[1];
    return c;
}
static Ctl auto_ctl(const M &sh) {   /* SPHERE_AUTO: the ship plays itself, for headless runs */
    Ctl c; V best; float bd = 1e30f; bool have = false; float tv = bolt_speed; static double next_h = 25.0, next_m = 6.0;
    auto consider = [&](V p, V v) { float d = len(p - ship.p); if (d < bd) { bd = d; best = p + (v - ship.v) * (d / tv); have = true; } };
    bool steering_duct = false;
    if (mom.on && !field_up) { V fw = mom.R.r[1], mouth = mom.p + fw * (DUCT_Y0 - 1500.f), l0 = to_local(mom.R, ship.p - mom.p); bool behind = l0.y < DUCT_Y0 - 8000.f && fabsf(l0.x) < 9000.f && fabsf(l0.z) < 9000.f; V aimp = behind ? mouth : mouth - fw * 30000.f; bd = len(aimp - ship.p); best = aimp; have = true; steering_duct = true; }
    else if (mom.on) for (int k = 0; k < 4; k++) { if (mom.gen[k] > 0) consider(mom.p + to_world(mom.R, GEN_L[k]), mom.v); }
    if (!steering_duct) { for (const Foe &e : foes) if (e.kind != 3 && !mom.on) consider(e.p, e.v); if (!have) for (const Rock &r : rocks) consider(r.p, r.v); }
    if (!have) { c.thr = 0.2f; c.aim = sh.r[1]; return c; }
    V d = norm(best - ship.p), l = to_local(sh, d);
    float ye = atan2f(l.x, l.y), pe = atan2f(l.z, sqrtf(l.x * l.x + l.y * l.y));
    c.yaw = clampf(ye * 2.5f, -1.f, 1.f); c.pit = clampf(pe * 2.5f, -1.f, 1.f); c.thr = bd < 25000.f ? -0.5f : 0.3f; c.aim = d;
    c.fire = !steering_duct && fabsf(ye) < 0.08f && fabsf(pe) < 0.08f && bd < 90000.f;
    if (steering_duct) c.thr = 0.6f;
    if (in_box(ship.p + sh.r[1] * 25000.f, BASE_P, V(BASE_H.x + 6000.f, BASE_H.y + 6000.f, BASE_H.z + 6000.f))) { c.thr = -1.f; c.pit = 1.f; }
    if (sim_t >= next_h && hyper_cd <= 0 && !steering_duct) { c.hyper = true; next_h = sim_t + 25.0; }   /* so a run shows the jump */
    if (sim_t >= next_m) { next_m = sim_t + 11.0; c.weapon = true; }
    if (chaos_left > 0 && (foes.size() >= 7)) c.chaos = true;
    return c;
}

/* ---- the frame ---- */
static VPIX_Instance &dash_put(VPIX_Frame &f, int id, const M &sh, V local, bool bright) {
    VPIX_Instance &i = dput(f, id, sh, ship.p + to_world(sh, local)); i.on_top = 2; i.bright = bright ? 1 : 0; return i;
}
static void put_dash(VPIX_Frame &f, const M &sh) {
    dash_put(f, M_HULL, sh, V(0, 0, 0), false); dash_put(f, M_HULLL, sh, V(0, 0, 0), true);
    dash_put(f, M_DASH, sh, V(0, 0, 0), true);
    const int fr = (int)f.frame;
    const V LM = F(0.2755f, 0.8865f, DY - 30), RM = F(0.721f, 0.884f, DY - 30); const float LSX = 0.177f * KX, LSZ = 0.217f * 0.75f * KZ, RSX = 0.178f * KX, RSZ = 0.222f * 0.75f * KZ;
    const bool frog = hyper_age > 1.32f && hyper_age < 2.5f;   /* he comes up over the static after a jump (t964.0-t965.2) */
    if (comm == C_STATIC || comm == C_FROG) { VPIX_Instance &i = dash_put(f, M_STAT0 + (fr & 3), sh, LM, true); i.S[0] = LSX; i.S[2] = LSZ;
        if (frog || comm == C_FROG) { VPIX_Instance &j = dash_put(f, M_FROG0 + ((int)(sim_t * 2.5) & 1), sh, LM + V(0, -8, 0), true); j.S[0] = LSX / 156.f; j.S[2] = LSZ / 124.f; } }
    else {
        { VPIX_Instance &i = dash_put(f, M_RADAR, sh, LM, true); i.S[0] = LSX / 156.f; i.S[2] = LSZ / 124.f; } dash_put(f, M_BLIP_B, sh, LM + V(0, -6, 0), true);
        { float sw = fmodf((float)sim_t, 2.9f) / 1.9f; if (sw < 1.f) {   /* the band: appears at the top, grows to half the scope, slides off the bottom in 1.9 s of every 2.9 (measured 1.7-2.9 of 2.4-3.6) */
              float b = sw * 1.5f, t0 = (std::max)(0.f, b - 0.5f), b0 = (std::min)(1.f, b), top = LSZ * 0.42f, sp = LSZ * 0.84f; if (b0 > t0 + 0.01f) { VPIX_Instance &i = dash_put(f, M_RADARB, sh, LM + V(0, -3, top - b0 * sp), true); i.S[0] = LSX / 146.f; i.S[2] = (b0 - t0) * sp / 96.f; } } }
        const float RANGE = 120000.f; int nb = 0;
        auto blip = [&](V p, int id) { if (nb++ > 40) return; V l = to_local(sh, p - ship.p); float x = clampf(l.x / RANGE, -1.f, 1.f) * 66.f, z = clampf(l.y / RANGE, -1.f, 1.f) * 50.f;
            VPIX_Instance &i = dash_put(f, id, sh, LM + V(x, -8, z), true); float s = l.z > 8000.f ? 1.5f : l.z < -8000.f ? 0.7f : 1.f; i.S[0] = s; i.S[2] = s; };
        for (const Foe &e : foes) if (e.kind != 3) blip(e.p, M_BLIP_E);
        if (mom.on) { blip(mom.p, M_BLIP_E); blip(mom.p + V(2000, 0, 0), M_BLIP_E); }
        for (const Rock &r : rocks) if (len(r.p - ship.p) < RANGE * 0.5f) blip(r.p, M_BLIP_R);
    }
    bool low = ship.shield < 30.f || st != S_PLAY || eject_on > 0.f;   /* EJECT shows from the start for a while (t941-t946.7) */
    if (st == S_PLAY && comm == C_STATIC && !low) { VPIX_Instance &i = dash_put(f, M_STAT0 + ((fr + 2) & 3), sh, RM, true); i.S[0] = RSX; i.S[2] = RSZ; }
    else { VPIX_Instance &i = dash_put(f, low ? M_EJECT : M_ICONS, sh, low ? RM + V(0, 0, 7) : RM, true); float pk = low ? 0.945f : 1.f; i.S[0] = pk * RSX / 136.f; i.S[2] = pk * RSZ / 120.f; }
    /* the orange bar: all slots dim, a bright part spreading from the middle (t947-t948, t952-t953, t963-t966) */
    for (int k = 0; k < 20; k++) { V p = F(0.692f + k * (0.107f / 19.f), 0.71f, DY - 32); bool br = fabsf(k - 9.5f) < bar_v * 10.5f; VPIX_Instance &i = dash_put(f, br ? M_SLOTB : M_SLOT, sh, p, true); i.S[0] = 0.28f; i.S[2] = 0.48f; }
    {   /* the meter: green over orange, each side of its centre line changing on its own (footage); that the shield drives it is INVENTED */
        float sf = clampf(ship.shield, 0.f, 100.f) / 100.f, gr = clampf(sf * 2.f - 1.f, 0.f, 1.f), gl = clampf(sf * 2.f, 0.f, 1.f); const float hw = 0.025f, cxm = 0.862f, ht = 0.13f * 0.75f * KZ;
        if (gl > 0.01f) { VPIX_Instance &i = dash_put(f, M_GQUAD, sh, F(cxm - hw * gl * 0.5f, 0.825f, DY - 34), true); i.S[0] = hw * gl * KX; i.S[2] = ht; }
        if (gr > 0.01f) { VPIX_Instance &i = dash_put(f, M_GQUAD, sh, F(cxm + hw * gr * 0.5f, 0.825f, DY - 34), true); i.S[0] = hw * gr * KX; i.S[2] = ht; } }
    if (blink_on && !low && comm != C_STATIC) dash_put(f, M_YSQ, sh, F(0.777f, 0.955f, DY - 34), true);
    /* the left tall panel: a double dash that never moves, and five teeth that grow under it once, 2.9 s after a jump (t965.56-t966.28; meaning unknown) */
    dash_put(f, M_YLINE, sh, F(0.1225f, 0.857f, DY - 30), true);
    if (hyper_age > 2.9f && hyper_age < 3.62f) { int n = (int)clampf((hyper_age - 2.9f) / 0.05f, 0.f, 5.f); for (int k = 0; k < n; k++) for (int s0 = -1; s0 <= 1; s0 += 2) dash_put(f, M_TOOTH, sh, F(0.1225f + s0 * 0.014f, 0.871f + k * 0.019f, DY - 31), true); }
}
static void put_world(VPIX_Frame &f) {
    M I; dput(f, M_STARS, I, ship.p).bright = 1; dput(f, M_LATT, I, V(0, 0, 0)).bright = 1;
    dput(f, M_BASE, I, BASE_P);
    for (const Rock &r : rocks) { if (len(r.p - ship.p) > 190000.f) continue; VPIX_Instance &i = dput(f, M_ROCK0 + r.var, r.R, r.p); i.S[0] = i.S[1] = i.S[2] = r.r / 1000.f; }
    for (const Foe &e : foes) dput(f, e.kind == 3 ? M_ALLY : M_FIGHT, e.R, e.p);
    if (v1knet::on) { if (v1knet::master) fill_peers(); for (int k = 0; k < 4; k++) if (peer[k].on && peer[k].alive && k != me) dput(f, M_ALLY, pitched(yawed(peer[k].yaw), peer[k].pitch), peer[k].p); }   /* the other pilots, in your side's grey */
    if (mom.on) { dput(f, M_MOTHER, mom.R, mom.p); for (int k = 0; k < 4; k++) if (mom.gen[k] > 0) dput(f, M_GEN, mom.R, mom.p + to_world(mom.R, GEN_L[k]));
        if (field_up) { VPIX_Instance &i = dput(f, M_SHELL, mom.R, mom.p); i.bright = 1; i.S[0] = i.S[1] = i.S[2] = 34.f; i.glass = 42; } else dput(f, M_DUCTLIT, mom.R, mom.p).bright = 1; }
    for (const Bolt &b : bolts) { V fw = b.v; fw.z += 0.001f; if (b.missile) dput(f, M_MISSILE, look_at(b.p, b.p + fw), b.p); else if (b.mine || b.ally) dput(f, M_BOLT, look_at(b.p, b.p + fw), b.p).bright = 1;
        else { VPIX_Instance &i = dput(f, M_PLASMA, I, b.p); i.bright = 1; i.S[0] = i.S[1] = i.S[2] = b.big ? 1.4f : 0.7f; } }
    { size_t n = 0; for (const Part &q : parts) { if (q.delay > 0) continue; if (n++ > 150) break; float u = q.life / q.life0; VPIX_Instance &i = dput(f, q.id, I, q.p); i.bright = 1; i.S[0] = i.S[1] = i.S[2] = q.sz / 100.f * (0.4f + 0.6f * u); } }
    { M bb = basis(); for (const Fire &fi : fires) { float u = fi.t / 1.4f, sc = fi.sz / 1000.f * (0.5f + 1.1f * sqrtf(u)); const int ids[3] = {M_FIRE0, M_FIRE1, M_FIRE2}; for (int k = 0; k < 3; k++) { if (k == 2 && u > 0.6f) continue; VPIX_Instance &i = dput(f, ids[k], bb, fi.p); i.bright = 1; i.S[0] = i.S[1] = i.S[2] = sc; i.glass = (uint8_t)clampf((1.f - u) * (k ? 460.f : 340.f), 12.f, k ? 240.f : 215.f); } } }
    if (chaos_r > 0) { VPIX_Instance &i = dput(f, M_SHELL, I, chaos_c); i.bright = 1; i.S[0] = i.S[1] = i.S[2] = chaos_r / 1000.f; }
}
static void put_hud(VPIX_Frame &f, const VPIX_Input &in, const M &cam) {
    int t = (int)clampf(ceilf((std::max)(time_left, 0.f)), 0.f, 999.f);
    const float dy = HUD_Y(0.06f);
    green_num(f, t, 3, 44.f, dy, 13.f, 0.86f);             /* top left, counts down (footage) */
    green_num(f, score, 5, 267.f, dy, 16.6f, 0.86f);       /* top right, five digits running to the edge (footage) */
    auto hl = [&](float x0, float y0, float x1, float y1) { for (int k = 0; k < 2; k++) { VPIX_Frame::Line l; l.x0 = x0 + k * 0.5f; l.y0 = y0 + k * 0.5f; l.x1 = x1 + k * 0.5f; l.y1 = y1 + k * 0.5f; l.rgb[0] = HUD_R; l.rgb[1] = HUD_G; l.rgb[2] = HUD_B; f.lines.push_back(l); } };
    /* the dotted bar: a plain comb (it does not visibly change on the footage) */
    { float y0 = HUD_Y(0.065f), y1 = HUD_Y(0.085f); for (int i = 0; i < 41; i++) { float x = 128.5f + i * 2.33f; hl(x, y0, x, y1); } }
    /* the reticle, solved from t941: three nested corners in each quadrant, centred on the gaze: corner offset and arm lengths */
    const float cx = 176.f, cy = 144.f; const float RC[3][4] = {{22.f, 20.f, 25.f, 20.f}, {14.5f, 13.f, 18.f, 14.f}, {11.f, 9.f, 13.f, 11.f}};
    for (int k = 0; k < 3; k++) for (int sx = -1; sx <= 1; sx += 2) for (int sy = -1; sy <= 1; sy += 2) { float x = cx + sx * RC[k][0], y = cy + sy * RC[k][1]; hl(x, y, x + sx * RC[k][2], y); hl(x, y, x, y + sy * RC[k][3]); }
    /* the triangles and the yellow square blink together, with no target near the sights too (t942-t958); shown here while a foe is within radar reach; the left triangle drops out for part of it (t958.6-t959.4) */
    { bool near_foe = false; for (const Foe &e : foes) if (e.kind != 3 && len(e.p - ship.p) < 120000.f) near_foe = true; if (mom.on) near_foe = true; const float ph = fmodf((float)sim_t, 0.6f); blink_on = near_foe && ph < 0.26f;
      if (st == S_PLAY && ship.alive && blink_on) { blit(f, BM_TRI_U, 9, 9, cx - 4.5f, cy - 16); blit(f, BM_TRI_D, 9, 9, cx - 4.5f, cy + 7); blit(f, BM_TRI_R, 9, 9, cx + 9, cy - 4.5f); if (fmodf((float)sim_t, 2.4f) < 1.6f) blit(f, BM_TRI_L, 9, 9, cx - 18, cy - 4.5f); } }
    /* five bow ties at the right, one at the left: the same in every frame of the tape */
    for (int i = 0; i < 5; i++) blit(f, BM_TIE, 11, 5, 333.f, HUD_Y(0.255f) + i * 7.5f);
    blit(f, BM_TIE, 11, 5, 22.f, HUD_Y(0.39f));
    (void)in; (void)cam;
}
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = f.clear[1] = f.clear[2] = 0;
    f.n_lights = 1; f.lights[0] = {0.65f, 0.35f, {-0.218f, -0.437f, 0.874f}};   /* flat shaded and bright; values are a guess, as in Dactyl */
    if (hyper_t > 0) {   /* measured, frame by frame, t959.36-t959.76: dim, red, yellow, red, yellow, red, dark red; lattice and stars stay on it */
        static const uint8_t SC[7][3] = {{34, 26, 27}, {165, 64, 56}, {177, 155, 72}, {145, 53, 43}, {174, 153, 74}, {145, 53, 42}, {119, 45, 38}}; static const float ST[7] = {0.04f, 0.12f, 0.20f, 0.28f, 0.36f, 0.40f, 0.44f};
        float a = HYPER_LEN - hyper_t; int k = 0; while (k < 6 && a > ST[k]) k++; f.clear[0] = SC[k][0]; f.clear[1] = SC[k][1]; f.clear[2] = SC[k][2]; }
    { static const char *pn = getenv("SPHERE_PIN"); if (pn && *pn) { float a[5] = {0, 0, 0, 0, 0}; sscanf(pn, "%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4); ship.p = V(a[0], a[1], a[2]); ship.yaw = a[3] / 57.29578f; ship.pitch = a[4] / 57.29578f; ship.alive = true; ship.v = V(); }
      static const char *cm = getenv("SPHERE_COMM"); if (cm && *cm) comm = atoi(cm);
      static const char *ha = getenv("SPHERE_HYPERAGE"); if (ha && *ha) { hyper_age = (float)atof(ha); eject_on = 0; comm = C_STATIC; comm_t = 9.f; }
      static const char *sb = getenv("SPHERE_STROBE"); if (sb && *sb) { hyper_t = HYPER_LEN - (float)atof(sb); hyper_done = true; hyper_cd = 9.f; } }   /* dev: a fixed place and comm picture for shots */
    put_world(f);
    { static const char *sw = getenv("SPHERE_SHOW"); if (sw && *sw) { float d = 0; int md = 0; sscanf(sw, "%f,%d", &d, &md); M bs = basis(); V fw = bs.r[1], rg = bs.r[0], up = bs.r[2]; auto at = [&](float nx, float ny, float dep) { return ship.p + fw * dep + rg * (nx * 0.7f * dep) + up * (ny * 0.525f * dep); };
        V fp; if (md == 0) { VPIX_Instance &i = dput(f, M_ROCK2, yawed(0.7f), at(0.2f, 0.02f, d)); i.S[0] = i.S[1] = i.S[2] = d / 3200.f; fp = at(-0.22f, -0.3f, d * 1.1f); }
        else { dput(f, M_FIGHT, yawed(0.6f), at(-0.4f, 0.42f, d)); dput(f, M_FIGHT, yawed(-0.4f), at(-0.13f, 0.36f, d * 0.8f)); dput(f, M_MISSILE, pitched(yawed(0.f), 0.8f), at(0.12f, 0.72f, d * 0.8f)); fp = at(-0.42f, 0.86f, d); }
        if (fires.empty() || fires[0].t > 0.5f) { fires.clear(); Fire fi; fi.p = fp; fi.t = 0.2f; fi.sz = d * (md ? 0.12f : 0.1f); fires.push_back(fi); } } }
    M sh = basis(), cam = sh; V eye = ship.p; bool head_cam = false, dash = false;
    static const char *fixed = getenv("SPHERE_CAM");
    if (fixed && *fixed) { float a[6] = {0, -30000, 8000, 0, 0, 1000}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5])); }
    else if (ship.alive) { dash = true; head_cam = true; }
    else { eye = ship.p + V(0, -6000, 3500); cam = look_at(eye, ship.p); }
    if (dash) put_dash(f, sh);
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    /* The camera is the ship's frame and the renderer puts the live head on top of it (its "free look" path). head_fed is given but marked invalid
       so the renderer's guess whether the camera "follows" the head (a test on pitch only) can never fire: with a pitched or banked ship it would
       drop the ship's tilt. in_head stays true, as in Dactyl. */
    f.in_head = head_cam; if (head_cam) { f.head_fed = in.head; f.head_fed.valid = false; }
    const float CX = 176.f;
    put_hud(f, in, head_cam ? cam_basis(in, sh) : cam);
    if (st == S_TITLE) {
        text(f, "BATTLESPHERE", CX, 62, 1.6f, 1);
        text(f, MODE_NAME[game_mode], CX, 96, 1.f);
        if (v1knet::on && !v1knet::master) text(f, !v1knet::master_heard() ? "LOOKING FOR PLAYER 1" : v1knet::nseats > 0 ? "YOU JOIN NEXT ROUND" : "PLAYER 1 STARTS THE GAME", CX, 200, 0.8f);
        else if (fmodf((float)sim_t, 1.2f) < 0.8f) text(f, (freeplay || credits > 0) ? "PRESS TRIGGER" : "INSERT CREDIT", CX, 200, 1.f);
        if (v1knet::on && v1knet::master && v1knet::live_pods() > 0) { char b[32]; snprintf(b, sizeof(b), "LINKED %d PLAYERS", v1knet::live_pods() + 1); text(f, b, CX, 170, 0.8f); }
    } else if (st == S_OVER) {
        text(f, over_why == 1 ? "MISSION COMPLETE" : over_why == 2 ? "EJECTED" : "TIME UP", CX, 70, 1.4f, 1);
        green_num(f, score, 5, CX - 38.f, 98.f, 15.f, 0.95f);
    }
    d1k::look_test(f);
    d1k::hs::draw(f, "sphere", st == S_TITLE, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at the machine's rate ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 1; static unsigned long frames = 0;
/* ---- link play (vpix_1k_net.h, tag SPHR, start together): pod 1 runs the sky (rocks, foes, battleship, shots); every other pod flies its own ship in it and sends its sticks; all pilots are one team (shared score and clock, foes go for the nearest, shots hurt the pilot they hit). INVENTED: the original was one cabin per player and its Dogfight was linked cabinets; here Dogfight is the team against computer pilots. Empty seats have no ship (the game has no wingmen). Not carried over the link: chaos charge and eject for pods 2-4. ---- */
#pragma pack(push, 1)
struct NetIn { float yaw, pit, thr, aim[3]; uint8_t fl, wseq; };
struct NetSeat { float p[3], yaw, pitch, speed, shield, dead_t, hyper_t, hyper_age, hyper_cd, comm_t, eject_on; uint8_t fl, comm, weapon, chaos; };
struct NetFoe { float p[3]; int16_t v[3]; uint8_t kind; };
struct NetRock { float p[3]; uint16_t r; uint8_t var; };
struct NetBolt { float p[3]; int16_t v[3]; uint8_t fl; };
enum { NF = 14, NR = 22, NB = 22, NE = 6 };
struct NetSt { uint8_t st, mode, fl, ev_total, over_why, nfoe, nrock, nbolt; int32_t score; float time_left, chaos_r, chaos_c[3], duct_t, momp[3], momv[3], gen[4]; NetSeat seat[4]; NetFoe foe[NF]; NetRock rock[NR]; NetBolt bolt[NB]; NetEv ev[NE]; };
#pragma pack(pop)
static_assert(sizeof(NetSt) <= v1knet::SNAP_MAX, "sphere snapshot too big");
static_assert(sizeof(NetIn) <= v1knet::IN_MAX, "sphere input too big");
static void net3(float *o, V a) { o[0] = a.x; o[1] = a.y; o[2] = a.z; }
static V net3(const float *o) { return V(o[0], o[1], o[2]); }
static void net3s(int16_t *o, V a, float k) { o[0] = (int16_t)clampf(a.x / k, -32000.f, 32000.f); o[1] = (int16_t)clampf(a.y / k, -32000.f, 32000.f); o[2] = (int16_t)clampf(a.z / k, -32000.f, 32000.f); }
static V net3s(const int16_t *o, float k) { return V(o[0] * k, o[1] * k, o[2] * k); }
static NetIn cin_[4]; static bool has_in[4]; static uint8_t last_wseq[4];
static void net_log() {
    static double next = 0; if (sim_t < next) return; next = sim_t + 5.0;
    if (st == S_TITLE) { dl("t=%.1f link: %s, title, pods heard %d", sim_t, v1knet::master ? "master" : "slave", v1knet::live_pods()); return; }
    dl("t=%.1f link: %s seat %d st %d left %.0f score %d | mine (%.0f %.0f %.0f) alive %d | crew1 %d (%.0f %.0f %.0f) in %d | foes %zu rocks %zu", sim_t, v1knet::master ? "master" : "slave", me, (int)st, time_left, score, ship.p.x, ship.p.y, ship.p.z, (int)ship.alive,
       (int)crew_on[1], crew[1].s.p.x, crew[1].s.p.y, crew[1].s.p.z, (int)has_in[1], foes.size(), rocks.size());
}
static void net_master() {
    for (int k = 1; k < 4; k++) { const NetIn *p = (const NetIn *)v1knet::seat_input(k, sizeof(NetIn));
        if (crew_on[k] && !v1knet::seat_pod[k]) { crew_on[k] = false; dl("t=%.1f link: seat %d left", sim_t, k); }
        if (p) { if (!has_in[k]) dl("t=%.1f link: seat %d is flying", sim_t, k); cin_[k] = *p; has_in[k] = true; } else has_in[k] = false; }
    if (!v1knet::due(30)) return;
    if (st == S_TITLE) { v1knet::send_snapshot(NULL, 0); return; }
    static NetSt n; memset(&n, 0, sizeof(n)); n.st = (uint8_t)st; n.mode = (uint8_t)game_mode; n.fl = (uint8_t)((mom.on ? 1 : 0) | (field_up ? 2 : 0)); n.ev_total = ev_total; n.over_why = (uint8_t)over_why; n.score = score; n.time_left = time_left;
    n.chaos_r = chaos_r; net3(n.chaos_c, chaos_c); n.duct_t = duct_t; net3(n.momp, mom.p); net3(n.momv, mom.v); for (int g = 0; g < 4; g++) n.gen[g] = mom.gen[g];
    auto put_seat = [&](int k, const PS &q, bool on) { NetSeat &o = n.seat[k]; net3(o.p, q.s.p); o.yaw = q.s.yaw; o.pitch = q.s.pitch; o.speed = q.s.speed; o.shield = q.s.shield; o.dead_t = q.s.dead_t; o.hyper_t = q.hyper_t; o.hyper_age = q.hyper_age; o.hyper_cd = q.hyper_cd; o.comm_t = q.comm_t; o.eject_on = q.eject_on;
        o.fl = (uint8_t)((q.s.alive ? 1 : 0) | (on ? 2 : 0) | (q.hyper_done ? 4 : 0) | (q.s.inv > 0 ? 8 : 0)); o.comm = (uint8_t)q.comm; o.weapon = (uint8_t)q.s.weapon; o.chaos = (uint8_t)q.chaos_left; };
    put_seat(0, cur_ps(), true); for (int k = 1; k < 4; k++) put_seat(k, crew[k], crew_on[k]);
    for (const Foe &e : foes) { if (n.nfoe >= NF) break; NetFoe &o = n.foe[n.nfoe++]; net3(o.p, e.p); net3s(o.v, e.v, 2.f); o.kind = (uint8_t)e.kind; }
    for (const Rock &r : rocks) { if (n.nrock >= NR) break; NetRock &o = n.rock[n.nrock++]; net3(o.p, r.p); o.r = (uint16_t)r.r; o.var = (uint8_t)r.var; }
    for (size_t i = bolts.size() > NB ? bolts.size() - NB : 0; i < bolts.size(); i++) { const Bolt &b = bolts[i]; NetBolt &o = n.bolt[n.nbolt++]; net3(o.p, b.p); net3s(o.v, b.v, 8.f); o.fl = (uint8_t)((b.mine ? 1 : 0) | (b.big ? 2 : 0) | (b.ally ? 4 : 0) | (b.missile ? 8 : 0) | (b.locked ? 16 : 0)); }
    for (int i = 0; i < NE; i++) n.ev[i] = evr[i];
    v1knet::send_snapshot(&n, sizeof(n));
}
static void crew_step(float dt) {   /* master: fly every other pod's ship one step, inside its own cockpit state */
    for (int k = 1; k < 4; k++) if (crew_on[k]) {
        Ctl c; M sh = pitched(yawed(crew[k].s.yaw), crew[k].s.pitch); c.aim = sh.r[1];
        if (has_in[k]) { const NetIn &n = cin_[k]; c.yaw = clampf(n.yaw, -1.f, 1.f); c.pit = clampf(n.pit, -1.f, 1.f); c.thr = clampf(n.thr, -1.f, 1.f); c.fire = (n.fl & 1) != 0; c.hyper = (n.fl & 2) != 0; c.a_miss = (n.fl & 4) != 0;
            V a = net3(n.aim); if (len(a) > 0.5f) c.aim = norm(a); c.weapon = n.wseq != last_wseq[k]; last_wseq[k] = n.wseq; }
        const float bv = bar_v, d0 = duct_t; swap_ps(crew[k]);
        step_ship(c, dt); collide_ship(); step_comm(dt);
        swap_ps(crew[k]); bar_v = bv; if (d0 >= 0.f) duct_t = d0; }
}
static void net_leave(const char *why) {
    slave_game = false; st = S_TITLE; st_t = 0; me = 0; rocks.clear(); foes.clear(); bolts.clear(); parts.clear(); fires.clear(); mom = Mom(); field_up = false; duct_t = -1.f; chaos_r = -1.f; for (Peer &p : peer) p = Peer();
    reset_ship(); spawn_mode(game_mode); dl("t=%.1f link: %s, back to the title", sim_t, why);
}
static void net_apply(const NetSt &s, uint8_t round) {
    static uint8_t my_round = 0, last_ev = 0; static float last_shield = 100.f; const bool fresh = !slave_game || round != my_round; my_round = round; me = v1knet::my_seat;
    if (fresh) { slave_game = true; rocks.clear(); foes.clear(); bolts.clear(); parts.clear(); fires.clear(); last_ev = s.ev_total; last_shield = 100.f; st_t = 0; sfx(0, SND_START, 100); dl("t=%.1f link: in the master round, seat %d", sim_t, me); }
    if ((State)s.st != st) st_t = 0;
    st = (State)s.st; game_mode = s.mode; over_why = s.over_why; score = s.score; time_left = s.time_left; chaos_r = s.chaos_r; chaos_c = net3(s.chaos_c); duct_t = s.duct_t;
    mom.on = (s.fl & 1) != 0; field_up = (s.fl & 2) != 0; mom.p = net3(s.momp); mom.v = net3(s.momv); for (int g = 0; g < 4; g++) mom.gen[g] = s.gen[g]; { V fw = mom.v; fw.z += 0.001f; mom.R = look_at(mom.p, mom.p + fw); }
    const NetSeat &o = s.seat[me < 0 ? 0 : me];
    ship.p = net3(o.p); ship.yaw = o.yaw; ship.pitch = o.pitch; ship.speed = o.speed; ship.shield = o.shield; ship.dead_t = o.dead_t; ship.alive = (o.fl & 1) != 0; ship.inv = (o.fl & 8) ? 1.f : 0.f; ship.weapon = o.weapon;
    hyper_t = o.hyper_t; hyper_age = o.hyper_age; hyper_cd = o.hyper_cd; hyper_done = (o.fl & 4) != 0; comm_t = o.comm_t; comm = o.comm; eject_on = o.eject_on; chaos_left = o.chaos;
    if (!fresh && ship.shield < last_shield - 0.5f && ship.alive) sfx(4, SND_HIT, 110);
    last_shield = ship.shield;
    for (int k = 0; k < 4; k++) { const NetSeat &q = s.seat[k]; peer[k].on = (q.fl & 2) != 0; peer[k].alive = (q.fl & 1) != 0; peer[k].p = net3(q.p); peer[k].yaw = q.yaw; peer[k].pitch = q.pitch; }
    foes.resize(s.nfoe > NF ? NF : s.nfoe); for (size_t i = 0; i < foes.size(); i++) { Foe &e = foes[i]; e.p = net3(s.foe[i].p); e.v = net3s(s.foe[i].v, 2.f); e.kind = s.foe[i].kind; V fw = e.v; fw.z += 0.001f; e.R = look_at(e.p, e.p + fw); }
    { const size_t nr = s.nrock > NR ? NR : s.nrock, old = rocks.size(); rocks.resize(nr); for (size_t i = 0; i < nr; i++) { Rock &r = rocks[i]; r.p = net3(s.rock[i].p); r.r = s.rock[i].r; r.var = s.rock[i].var; if (i >= old) r.w = V(sinf(i * 1.7f), cosf(i * 2.3f), sinf(i * 0.9f)) * 0.5f; } }
    bolts.resize(s.nbolt > NB ? NB : s.nbolt); for (size_t i = 0; i < bolts.size(); i++) { Bolt &b = bolts[i]; const NetBolt &n = s.bolt[i]; b.p = net3(n.p); b.v = net3s(n.v, 8.f); b.mine = (n.fl & 1) != 0; b.big = (n.fl & 2) != 0; b.ally = (n.fl & 4) != 0; b.missile = (n.fl & 8) != 0; b.locked = (n.fl & 16) != 0; }
    int d = (s.ev_total + 240 - last_ev) % 240; if (d > NE) d = NE; if (fresh) d = 0;
    for (int i = 0; i < d; i++) { const NetEv &e = s.ev[((s.ev_total + 240 - d + i) % 240) % 6]; V at = net3(e.p); burst(at, e.n, e.n >= 14 ? 9000.f : 4000.f, e.n >= 14 ? 1.3f : 0.6f, e.n >= 14 ? 420.f : 150.f, true); sfx(e.n >= 14 ? 3 : 10, e.n >= 14 ? SND_EXPL : SND_ROCK, e.n >= 14 ? 110 : 60); }
    last_ev = s.ev_total;
}
/* true = this tick was a linked slave's, inside the master game */
static bool net_tick(double el, const VPIX_Input &in) {
    v1knet::pump();
    if (v1knet::master) { net_master(); net_log(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n == sizeof(NetSt) && v1knet::my_seat >= 0) { static NetSt s; memcpy(&s, d, sizeof(s)); net_apply(s, v1knet::round_id); }
        else if (slave_game) net_leave("the master round is over");
    }
    if (slave_game && !v1knet::master_heard()) net_leave("no word from the master for 3 s");
    net_log();
    if (!slave_game) { v1knet::send_input(NULL, 0); return false; }
    const M sh = basis(); const Ctl c = autoplay ? auto_ctl(sh) : read_ctl(in, sh, (float)el);
    static uint8_t wseq = 0; if (c.weapon) wseq++;
    NetIn o; memset(&o, 0, sizeof(o)); o.yaw = c.yaw; o.pit = c.pit; o.thr = c.thr; net3(o.aim, c.aim); o.fl = (uint8_t)((c.fire ? 1 : 0) | (c.hyper ? 2 : 0) | (c.a_miss ? 4 : 0)); o.wseq = wseq;
    v1knet::send_input(&o, sizeof(o));
    const float dt = 1.f / 120.f; int k = 0;
    while (acc >= dt && k < 24) { step_parts(dt); step_comm(dt); for (Rock &r : rocks) rotate(r.R, r.w, dt); acc -= dt; sim_t += dt; st_t += dt; k++; }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    return true;
}
static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    credits += in.coins - coins_seen; coins_seen = in.coins;
    static bool trig_was = false; bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f, sel = trig && !trig_was; trig_was = trig;
    if (autoplay && st != S_PLAY && st_t > 2.f) sel = true; if (d1k::hs::run("sphere", st == S_OVER && st_t > 2.f, score, in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (!v1knet::can_start()) sel = false;   /* a linked slave: pod 1 starts the game */
    if (v1knet::on && net_tick(el, in)) { PIC_AddEvent(tick, 2.0); return; }
    if (st == S_TITLE && !autoplay && (!v1knet::on || v1knet::master)) { static int was = 0; float jx = in.joy[0]; if (!in.vr) jx += (key_down('D') ? 1.f : 0.f) - (key_down('A') ? 1.f : 0.f); int now = jx > 0.5f ? 1 : jx < -0.5f ? -1 : 0; if (now && !was) { game_mode = (game_mode + now + 4) % 4; spawn_mode(game_mode); } was = now; }   /* how the level was chosen is INVENTED */
    if (st == S_TITLE && sel && (freeplay || credits > 0 || autoplay)) { if (!freeplay && credits > 0 && !autoplay) credits--; start_round(); st = S_PLAY; st_t = 0; }
    else if (st == S_OVER && sel && st_t > 3.f) { st = S_TITLE; st_t = 0; v1knet::end_round(); for (int k = 1; k < 4; k++) crew_on[k] = false; reset_ship(); spawn_mode(game_mode); bolts.clear(); parts.clear(); voices[11].on = false; }
    if (st == S_PLAY && sound_on) { riff_t -= (float)el; if (riff_t <= 0) { riff_t = 8.f; buggy::play(11, (int)SND_RIFF, 60); } }
    int n = 0;
    while (acc >= dt && n < 24) {
        M sh = basis();
        if (st == S_PLAY) {
            Ctl c = autoplay ? auto_ctl(sh) : read_ctl(in, sh, dt);
            if (autoplay && ship.alive && c.eject) c.eject = false;
            if (c.eject) end_round(2);
            else {
                step_ship(c, dt); step_foes(dt); step_mother(dt); step_rocks(dt); step_bolts(dt); step_chaos(dt); step_parts(dt); collide_ship(); step_comm(dt);
                if (v1knet::on && v1knet::master) crew_step(dt);
                if (!(len(ship.p - BASE_P) < 80000.f && ship.alive)) time_left -= dt;   /* it stood at 180 through a pass over the base (t941-t953); that the base is the reason is INVENTED */
                if (st == S_PLAY) { step_mode(dt); if (time_left <= 0 && st == S_PLAY) end_round(0); }
            }
        } else {   /* title and game over: the field keeps drifting, the ship sits at the launch point and sways */
            if (st == S_TITLE) ship.yaw = 0.35f * sinf((float)sim_t * 0.15f);
            ship.inv = 1.f;   /* nothing hurts the ship outside play */
            step_rocks(dt); step_parts(dt); step_bolts(dt); step_foes(dt); step_mother(dt); step_chaos(dt);
        }
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    if (sim_t >= next_log) { next_log = sim_t + 2.0;
        dl("t=%.0f st %d mode %d | ship (%.0f %.0f %.0f) spd %.0f shield %.0f alive %d | rocks %zu foes %zu mother %d field %d | bolts %zu parts %zu | score %d clock %.0f chaos %d hyper_cd %.0f",
           sim_t, (int)st, game_mode, ship.p.x, ship.p.y, ship.p.z, ship.speed, ship.shield, (int)ship.alive, rocks.size(), foes.size(), (int)mom.on, (int)field_up, bolts.size(), parts.size(),
           score, time_left, chaos_left, (std::max)(hyper_cd, 0.f)); }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".sphere").c_str(), "w");
    game_fps = envf("SPHERE_FPS", game_fps); game_time = envf("SPHERE_TIME", game_time); cruise = envf("SPHERE_CRUISE", cruise); max_speed = envf("SPHERE_MAXSPEED", max_speed);
    turn_rate = envf("SPHERE_TURN", turn_rate); pitch_rate = envf("SPHERE_PITCH", pitch_rate); bolt_speed = envf("SPHERE_BOLT", bolt_speed); fire_gap = envf("SPHERE_GAP", fire_gap);
    enemy_skill = envf("SPHERE_SKILL", enemy_skill); foe_speed = envf("SPHERE_FOE", foe_speed); chaser_speed = envf("SPHERE_CHASER", chaser_speed);
    arena_r = envf("SPHERE_ARENA", arena_r); hyper_cd_len = envf("SPHERE_HYPER_CD", hyper_cd_len); gen_hp0 = envf("SPHERE_GEN_HP", gen_hp0); duct_test = envf("SPHERE_DUCT", 0) != 0;
    n_rocks = (int)envf("SPHERE_ROCKS", (float)n_rocks); n_swarm = (int)envf("SPHERE_SWARM", (float)n_swarm); n_chasers = (int)envf("SPHERE_CHASERS", (float)n_chasers);
    chaos_max = (int)envf("SPHERE_CHAOS", (float)chaos_max); start_mode = (int)envf("SPHERE_MODE", (float)start_mode); game_mode = (int)clampf((float)start_mode, 0, 3);
    freeplay = envf("SPHERE_FREEPLAY", 1) != 0; autoplay = envf("SPHERE_AUTO", 0) != 0;
    n_swarm = (std::max)(n_swarm, 1);
    d1k::make_hud(); make_hud_bitmaps(); build_world_meshes(); build_dash_meshes(); make_sounds();
    reset_ship(); spawn_mode(game_mode); comm = C_STATIC; comm_t = 3.f;
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    v1knet::init("SPHR", false, dl);   /* start-together (Bobby: join-in-progress is for Flying Aces and VTOL) */
    PIC_AddEvent(tick, 2.0);
    dl("Battlesphere rebuild started");
}
}   /* namespace sphere */
