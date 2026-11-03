/* Hero (Virtuality, 1991-92) - native rebuild, BOTH titles in one file. Included after vpix_dactyl1k.h; entry hero::start(logbase).
   HERO_MODE=puzzle (default) is the 1000CS stand-up room game "The Hero Challenge". HERO_MODE=glider is the sit-down hang-glider aftershave advert.
   Evidence and choices: Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\hero.md.

   PUZZLE, FROM A FIRST-PERSON PLAY ACCOUNT: Rod Lamirand, "How I Jumped in A Dry River and Other Virtual Adventures", The Computer Paper (Ontario), Oct 1994, pp.66-67, read from the page scans
   (Library\Virtuality Preservation\Hero_Puzzle\scans; the OCR scrambled it and missed the start). He played it at Virtuality Canada on a 1000CS.
   - The hub: a platform, four short walkways north, south, east and west, an air-lock style door at the end of each with the word Enter over it; a finished door says Done instead. Any order.
   - River: a green field under a blue sky, the grass ends in a drop-off, a river far below with a raft you can just see. Step off: whistling, the crack of a chute, a square chute overhead. Forward drift overshoots; pull the arm to the body to slow, stick it straight ahead to stop.
     Miss: water, blue, a few seconds, back on the cliff. On the raft: cliff walls, a roaring sound, the raft nears the bank, jump ashore, a tree to the right with the letter H in its branches, grasp it: blue, back on the platform.
   - Tree: three tapered cylinders for a trunk, a green semi-transparent plane round the upper branches.
   - Kitchen: a small room, table and counter. A rushing sound, you shrink to about two inches. You move slowly though the small feet sound quick; the table's edge only by looking straight up. Footsteps from the left: brown pants, black shoes. Run for the table. A mouse your size runs in from the other side and stops facing your right.
     Cheese on the floor about two feet away. Start forward: a crash, a big black shoe lands three inches ahead. You beat the other mouse, get the cheese, bring the stick up to your mouth, it gets closer, blue, platform.
   - Dog: a small room, a dog chained to the wall, a key on a hook to your right, the lock where the chain meets the wall. Unlocked, the dog runs down one of three tunnels, into darkness. His account stops there.
   - He says 16 colours, cylinders, planes and rectangles. It was the only game of the five there with no fighting.
   THE PROMPTS ARE THE OPERATOR'S, NOT THE GAME'S: "Jump", "Look up", "Get off the raft", "Eat the cheese", "Look for a key" are in brackets in the article and one is plainly Craig Williams shouting. Nothing shows the game spoke them. A rebuild has no operator, so they are written on screen (HERO_OPERATOR=0 turns them off).
   FROM FOOTAGE (_X_E_Kg9roE t1621-1629, ten seconds): the corridor: walls grey with a mauve cast, a maroon floor strip about 1000 wide (solved from the figure taken as a person of 1700), low blocks along both sides, a tall white pilaster with a white lattice on the floor at its foot, a narrow red arched door in a jagged stone frame, a thin grey person with a yellow oval, long orange tapered arms ending in a pointed hand. Nobody has written which room it is: the article never reached a fourth room, so it stands here as the fourth.
   FROM SPEECH: four scenes, a letter from each spelling HERO, four minutes, up to four players (Octavius 23:28; Simon Marston fwAQhUTsoyg 21:22).
   INVENTED: which door is which; E, R and O coming from these rooms (only H is sourced; he names no letter from the cheese); the corridor's rule (take the yellow thing, open the red door); the hub's surroundings; every room colour outside the corridor; the air-lock's look; heights, speeds and timings; being stepped on or beaten to the cheese sends you back to the start of the room, as the water does; staying on the raft does too;
   which tunnel is right (following the dog works here, he suspected a trick and we do not know); the letter at its end; the win and time-up screens; the clock in the last 30 s and letters on the hub; all sounds are placeholders. Solo only: the four-player form is not built. The puzzle's high score is new too.
   GLIDER: the order seagulls -> pylons -> city of blocks, flat shapes, red landing pad and another orange delta glider are from footage.
   GLIDER REFINED 2026-11-02, MEASURED from Octavius t1208-t1302: your wing's nose at the top of the level view, darker on one side; lollipop trees, the low ridge, the great striped drum at the coast; orange glider with a zebra fan under it, hanging pilot, and a white one;
   lattice pylons with spread feet, several wires, and the grey fog of that stretch; very big plain rust, ochre, pale and slate blocks with the eye below their roofs; avenue, plaza, kerbs, rows of little trees; the pad with its painted HERO and the HERO EXPERIENCE title (vpix_1k_hero_pics.h).
   NO SCORE: MC microcomputer 127 (Mar 1993, p.224, Italian operator's list) calls it a relaxing hang-glider flight "senza punteggio". Period, one source, and nothing disagrees: no HUD in any glider frame. The old deaths score is gone: hs::run stays wired with over = false, so name entry never opens, and the attract shows no board. HERO_GLIDE_SCORE=1 brings it back.
   The van chased by a police car in the city is Simon Marston's hearsay from a programmer, in no frame: placed in the avenue, looks INVENTED. Routes, speeds, droppings' effect and respawn are INVENTED.
   No second source shows this game: Octavius alone shows a lilac sky and dark olive ground; on VTOL the same capture read so where two other sources show periwinkle and bright green, so sky goes to periwinkle and green up a little only. Dev: HERO_LOOK=yawdeg,pitchdeg,rolldeg.
   Units: mm, seconds. World X right, Y forward, Z up. Camera is the head pose (in_head), so VR late correction applies.
   Env: HERO_MODE, HERO_AUTO, HERO_CAM, HERO_FPS, HERO_TIME, HERO_WALK, HERO_MOUSE_WALK, HERO_FIG, HERO_OPERATOR, HERO_ROOM (dev: 0 river, 1 kitchen, 2 dog, 3 corridor), HERO_BOT_MISS (puzzle),
        HERO_SPEED, HERO_STEER, HERO_CLIMB, HERO_GLIDE_SCORE (glider). */
#include "vpix_1k_hero_pics.h"
#include "vpix_1k_net.h"         /* link play, 2-4 pods: see the "link play" block before the loop */
namespace hero {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::look_at;
using d1k::Mesh; using d1k::dput; using d1k::blit; using d1k::text; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound;
using d1k::yawed; using d1k::add_bitmap;

/* ---- tuning ---- */
static bool glider_mode = false, autoplay = false;
static float game_fps = 20.f;                     /* measured on the other 1000-series footage: about 20 */
static float walk_speed = 1400.f;                 /* INVENTED (HERO_WALK) */
static float mouse_walk = 1200.f;                 /* INVENTED: two inches tall and "moving slowly" (article); about 17 s to the cheese (HERO_MOUSE_WALK) */
static bool glide_score = false;                  /* HERO_GLIDE_SCORE=1 brings back the old invented glider score: MC 127 says this game has none */
static float game_len = 240.f;                    /* four minutes (Octavius, arcade-history; the Canadian operator sold five minutes for eight dollars, which may be the whole session) (HERO_TIME) */
static float fig_speed = 600.f;                   /* INVENTED: how fast the small figure paces (HERO_FIG) */
static float glide_speed = 6000.f;                /* INVENTED (HERO_SPEED) */
static float steer_speed = 2200.f;                /* INVENTED: "limited steering" (HERO_STEER) */
static float climb_speed = 2000.f;                /* INVENTED (HERO_CLIMB) */
static const float REACH_USE = 1200.f;            /* INVENTED: how close counts as touching */
static const float FLIGHT_LEN = 540000.f;         /* INVENTED: about 90 s of flight */

#include "vpix_1k_hero_hand.h"
/* ---- ids ---- */
enum { HM_SHELL = 301 /* +0..4 */, HM_DOOR = 310, HM_KEY, HM_CHEESE, HM_MOUSE, HM_DOG, HM_DOGZ, HM_FIG, HM_AIRLOCK, HM_SENTER, HM_SDONE, HM_RAFT, HM_CHUTE, HM_TREE, HM_LEAF,
       HM_LET = 324 /* +0..3 */, HM_LEG = 328, HM_ARM, HM_LINK, HM_SPARK, HM_ARM2, HM_ARM_F, HM_ARM2_F,
       GM_GULL = 340, GM_WL, GM_WR, GM_PYLON, GM_WIRE, GM_DELTA, GM_LAND, GM_DROP, GM_SELF, GM_VAN, GM_CITY = 350 /* +0..3 */, GM_DELTA2 = 354, GM_COP, GM_COPLIT };
static const uint32_t TEX_HPLATE = 0x7FFE3000u;
enum { BM_SPLAT = 400, BM_HEND = 401, BM_BLUE = 402 };
enum { SK_USE = 0x7E000001, SK_DOOR, SK_GET, SK_GROWL, SK_ZAP, SK_GULL, SK_SPLAT, SK_BUMP, SK_GO, SK_WIN, SK_WHISTLE, SK_CRACK, SK_SPLASH, SK_ROAR, SK_RUSH, SK_STEP, SK_CRASH, SK_SCURRY };

enum State { S_TITLE, S_PLAY, S_OVER };
static State st = S_TITLE; static float st_t = 0, time_left = 0; static double sim_t = 0;
static bool won = false;

static void sfx(int ch, uint32_t key, int vol) { if (!sound_on) return; buggy::play(ch, (int)key, vol, 0); }
static float dxy(V a, V b) { return hypotf(a.x - b.x, a.y - b.y); }
static float face_yaw(V from, V to) { return atan2f(-(to.x - from.x), to.y - from.y); }
static VPIX_Instance &put_at(VPIX_Frame &f, int id, V p, float yw = 0.f) { return dput(f, id, yawed(yw), p); }

/* a thin flat shape seen from both sides: two polygons, the second pushed 15 mm under the first so they never fight */
static void two_sided(Mesh &m, V a, V b, V c, V d, uint32_t top, uint32_t bot) {
    bool up = cross(b - a, c - a).z >= 0; const V dz(0, 0, -15);
    if (up) { m.poly({a, b, c, d}, top); m.poly({a + dz, d + dz, c + dz, b + dz}, bot); }
    else { m.poly({a, d, c, b}, top); m.poly({a + dz, b + dz, c + dz, d + dz}, bot); }
}
static void two_sided3(Mesh &m, V a, V b, V c, uint32_t top, uint32_t bot) {
    bool up = cross(b - a, c - a).z >= 0; const V dz(0, 0, -15);
    if (up) { m.poly({a, b, c}, top); m.poly({a + dz, c + dz, b + dz}, bot); }
    else { m.poly({a, c, b}, top); m.poly({a + dz, b + dz, c + dz}, bot); }
}

/* ---- sounds: INVENTED placeholders ---- */
static void make_sounds() {
    std::mt19937 r(11); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    const float TP = 6.2831853f;
    make_sound(SK_USE, 0.12f, [&](float t) { return sinf(TP * 520.f * t) * expf(-t * 22.f) * 0.7f; });
    make_sound(SK_DOOR, 1.5f, [&](float t) { return (noise() * 0.35f + sinf(TP * 60.f * t) * 0.5f) * (t < 1.3f ? 1.f : 0.f) * 0.7f; });
    make_sound(SK_GET, 0.7f, [&](float t) { return (sinf(TP * 660.f * t) + sinf(TP * 990.f * t) * 0.6f) * expf(-t * 5.f) * 0.5f; });
    make_sound(SK_GROWL, 0.9f, [&](float t) { return noise() * (0.5f + 0.5f * sinf(TP * 28.f * t)) * sinf(3.14159f * t / 0.9f) * 0.8f; });
    make_sound(SK_ZAP, 1.0f, [&](float t) { return noise() * (fmodf(t, 0.08f) < 0.05f ? 1.f : 0.3f) * expf(-t * 2.5f) * 0.9f; });
    make_sound(SK_GULL, 0.5f, [&](float t) { return sinf(TP * (1400.f + 500.f * sinf(TP * 6.f * t)) * t) * sinf(3.14159f * t / 0.5f) * 0.4f; });
    make_sound(SK_SPLAT, 0.3f, [&](float t) { return noise() * expf(-t * 14.f) * 0.7f; });
    make_sound(SK_BUMP, 0.3f, [&](float t) { return (noise() * 0.5f + sinf(TP * 90.f * t) * 0.5f) * expf(-t * 12.f); });
    make_sound(SK_GO, 0.6f, [&](float t) { return sinf(TP * (440.f + 300.f * t) * t) * expf(-t * 3.f) * 0.6f; });
    make_sound(SK_WHISTLE, 1.4f, [&](float t) { return sinf(TP * (1500.f - 500.f * t) * t) * 0.35f + noise() * 0.12f; });
    make_sound(SK_CRACK, 0.35f, [&](float t) { return noise() * expf(-t * 16.f) * 0.95f; });
    make_sound(SK_SPLASH, 1.1f, [&](float t) { return noise() * expf(-t * 3.f) * 0.8f; });
    make_sound(SK_ROAR, 1.9f, [&](float t) { return (noise() * 0.7f + sinf(TP * 55.f * t) * 0.2f) * 0.6f; });
    make_sound(SK_RUSH, 2.0f, [&](float t) { return noise() * sinf(3.14159f * t / 2.f) * 0.6f * (0.5f + 0.5f * sinf(TP * (3.f + 9.f * t) * t)); });
    make_sound(SK_STEP, 0.25f, [&](float t) { return (sinf(TP * 70.f * t) * 0.8f + noise() * 0.2f) * expf(-t * 18.f); });
    make_sound(SK_CRASH, 0.7f, [&](float t) { return (sinf(TP * 48.f * t) * 0.7f + noise() * 0.5f) * expf(-t * 6.f); });
    make_sound(SK_SCURRY, 0.3f, [&](float t) { return noise() * (fmodf(t, 0.05f) < 0.015f ? 0.5f : 0.f); });
    make_sound(SK_WIN, 1.2f, [&](float t) { float f0 = t < 0.3f ? 523.f : t < 0.6f ? 659.f : 784.f; return sinf(TP * f0 * t) * expf(-fmodf(t, 0.3f) * 4.f) * 0.55f; });
}

/* ======================================================================== puzzle ======================================================================== */
enum { R_RIVER = 0, R_MOUSE = 1, R_DOG = 2, R_HALL = 3, R_HUB = 4 };
struct Rect { float x0, y0, x1, y1; };
struct Obst { Rect r; bool on; };
static std::vector<Rect> area; static std::vector<Obst> obst;
static const float BODY_R = 250.f;
static bool free_at(float x, float y) {   /* inside the walkable rectangles (a body's radius in from the edge) and clear of switched-on obstacles */
    bool in = false; const float R = BODY_R;
    for (const Rect &a : area) if (x >= a.x0 + R && x <= a.x1 - R && y >= a.y0 + R && y <= a.y1 - R) { in = true; break; }
    if (!in) return false;
    for (const Obst &o : obst) if (o.on && x > o.r.x0 - R && x < o.r.x1 + R && y > o.r.y0 - R && y < o.r.y1 + R) return false;
    return true;
}
static void slide_to(V &p, V was) {   /* slide along a wall instead of stopping dead */
    if (free_at(p.x, p.y)) return;
    if (free_at(p.x, was.y)) p.y = was.y;
    else if (free_at(was.x, p.y)) p.x = was.x;
    else { p.x = was.x; p.y = was.y; }
}

static int room = R_HUB, start_room = R_HUB; static bool got[4]; static bool letter_vis = false; static V letter_p, start_p;
static float poke_t = 0, jump_z = 0, jump_v = 0; static bool fist_r = false, fist_l = false;
static V pp; static float pz = 0, yaw = 0, pitch = 0, cur_speed = 1400.f; static V walk_off; static bool need_place = true;
static int bs = 0, bot_miss = 0; static float bot_t = 0;   /* the auto player's step; HERO_BOT_MISS: how many jumps it lets go by the raft */
static float blue_t = -1.f, blue_len = 1.2f; static int blue_to = R_HUB;   /* the screen goes blue, then a cut (the article, every room) */
static const char *say_s = ""; static float say_t = 0; static bool operator_on = true;
static void say(const char *s) { say_s = s; say_t = 3.5f; dl("t=%.1f operator: %s", sim_t, s); }
static void go_blue(int to, float len) { if (blue_t >= 0.f) return; blue_t = 0; blue_to = to; blue_len = len; }

/* the hub: north river, east kitchen, south dog, west corridor. Four walkways and doors are from the article, which door is which is INVENTED */
static const float DOOR_D = 7000.f;
static V door_pos(int k) { static const float X[4] = {0, 1, 0, -1}, Y[4] = {1, 0, -1, 0}; return V(X[k] * DOOR_D, Y[k] * DOOR_D, 0); }

/* the river: a green field 80 m above it (height INVENTED), the raft, the bank, the tree */
static const float CZ = 80000.f, RAFT_Y0 = 60000.f, RAFT_TOP = 400.f, BANK_Y = 120000.f, TREE_X = 5000.f, TREE_Y = 127000.f, RAFT_HX = 1700.f, RAFT_HY = 2200.f;
static const float FALL_T = 1.5f, FALL_VZ = 15000.f, CHUTE_VZ = 4000.f, CHUTE_F = 5500.f, RAFT_V = 2400.f;   /* INVENTED numbers; with no braking you overshoot, as he did */
static int rv_st = 0; static float rv_t = 0, raft_y = RAFT_Y0, rv_touch = 0, rv_vy = 0, rv_roar = 0; static bool rv_said = false, rv_warned = false;   /* 0 field, 1 falling, 2 chute, 3 raft, 4 ashore, 5 in the water */

/* the kitchen: authored in true mm, scaled KS; you shrink to about two inches, so the eye ends at 1700 in a world 34 times too big */
static const float KS = 34.f;
static const float T_X0 = -900.f, T_X1 = 100.f, T_Y0 = -350.f, T_Y1 = 350.f, CH_X = 650.f, CH_Y = -200.f;
static int km = 0, km_cheese = 0, km_mouse = 0, km_stomp = 0, wk_i = -99; static float km_t = 0, wk_x = -1900.f, stomp_t = 0, stomp_x = 0, stomp_y = 0, stomp_a = 0, eat_t = 0, scurry_t = 0; static V kmouse_p;
static float shrink_s() { if (room != R_MOUSE) return 1.f; if (km == 0) return KS; if (km == 1) { float u = clampf(km_t / 2.f, 0.f, 1.f); return KS + (1.f - KS) * (1.f - (1.f - u) * (1.f - u)); } return 1.f; }
static bool under_table(V p) { float x = p.x / KS, y = p.y / KS; return x > T_X0 && x < T_X1 && y > T_Y0 && y < T_Y1; }
static V K(float x, float y, float z = 0.f) { return V(x * KS, y * KS, z * KS); }

/* the dog */
static const V HOOK_P(2850, 3000, 1500), ANCHOR_P(0, 5950, 700);
static const float TUN_Y[3] = {1200.f, 3000.f, 4800.f}; static const float TUN_END = -18000.f;
static int d_st = 0, d_tun = 0; static V dog_p; static float dog_yaw = 0, dog_bark = 0;   /* 0 chained, 1 you hold the key, 2 free and running, 3 at the end of its tunnel */

/* the corridor */
static float fig_y = 16000, fig_dir = 1; static int key_st = 0; static float door_t = 0; static bool door_go = false;
static float fig_x() { return 200.f * sinf(fig_y * 0.001f); }
/* the low blocks of the corridor: x0, y0, height; each 900 x 1000 (read from t1627, spacing is a best fit) */
static const int NBLK = 11;
static const float BLK[NBLK * 3] = {-1500, 15000, 1000, 600, 15800, 1250, -1500, 17000, 1250, 600, 17800, 1000, -1500, 19000, 1000, 600, 19800, 1250, -1500, 21000, 1250, 600, 21800, 1000, 600, 13200, 1000, -1500, 11000, 1000, 600, 11000, 1250};

static uint8_t enter_n = 0;   /* counts room entries so a linked pod knows when to rebuild its copy of the room */
static void enter_room(int k) {
    enter_n++; room = k; area.clear(); obst.clear(); letter_vis = false; bs = 0; bot_t = 0; pz = 0; pitch = 0; cur_speed = walk_speed;
    if (k == R_HUB) {
        area = {{-3000, -3000, 3000, 3000}, {-1000, 2500, 1000, DOOR_D}, {-1000, -DOOR_D, 1000, -2500}, {2500, -1000, DOOR_D, 1000}, {-DOOR_D, -1000, -2500, 1000}};
        start_p = V(0, 0, 0);
    } else if (k == R_RIVER) {
        area = {{-19000, -15000, 19000, 900}}; start_p = V(0, -9000, 0); pz = CZ; rv_st = 0; rv_t = 0; raft_y = RAFT_Y0; rv_touch = 0; rv_vy = 0; rv_roar = 0; rv_said = false; rv_warned = false;
        letter_p = V(TREE_X, TREE_Y - 300.f, 2500); letter_vis = true;
    } else if (k == R_MOUSE) {
        area = {{-1500.f * KS, -1200.f * KS, 1500.f * KS, 1100.f * KS}}; start_p = K(0, -560); cur_speed = mouse_walk; km = 0; km_t = 0; km_cheese = 0; km_mouse = 0; km_stomp = 0; wk_x = -1900.f; wk_i = -99; eat_t = 0; scurry_t = 0;
        kmouse_p = K(-500, 330);
        for (int i = 0; i < 4; i++) { float x = (i & 1) ? T_X1 - 40.f : T_X0 + 40.f, y = (i & 2) ? T_Y1 - 40.f : T_Y0 + 40.f; obst.push_back({{(x - 25.f) * KS, (y - 25.f) * KS, (x + 25.f) * KS, (y + 25.f) * KS}, true}); }
        obst.push_back({{0, 0, 0, 0}, false});   /* obst[4] is the shoe */
    } else if (k == R_DOG) {
        area = {{-3000, 0, 3000, 6000}}; for (int i = 0; i < 3; i++) area.push_back({TUN_END, TUN_Y[i] - 700.f, -2500, TUN_Y[i] + 700.f});
        start_p = V(0, 800, 0); d_st = 0; d_tun = (int)rnd(0.f, 2.999f); dog_p = V(0, 4600, 0); dog_yaw = 3.14159f; dog_bark = 2.f;
        letter_p = V(-16300.f, TUN_Y[d_tun], 900); say("LOOK FOR A KEY");
    } else {
        area = {{-1500, 0, 1500, 23400}, {-1000, 22600, 1000, 24400}, {-1500, 23800, 1500, 27200}};
        obst.push_back({{-1000, 23000, 1000, 24000}, true}); obst.push_back({{-1500, 13000, -1100, 14400}, true});
        for (const float *b = BLK; b < BLK + NBLK * 3; b += 3) obst.push_back({{b[0], b[1], b[0] + 900.f, b[1] + 1000.f}, true});
        start_p = V(0, 1200, 0); letter_p = V(0, 25300, 900);
        fig_y = 16000; fig_dir = 1; key_st = 0; door_t = 0; door_go = false;
    }
    pp = start_p; need_place = true; yaw = 0;
    dl("t=%.1f enter room %d", sim_t, k);
}
static void reset_game() {
    for (int k = 0; k < 4; k++) got[k] = false;
    won = false; time_left = game_len; blue_t = -1.f; say_t = 0; enter_room(start_room);
}

static bool goto_xy(V t, float dt) {   /* auto player: walk toward t, true when there */
    V d = t - pp; d.z = 0; float l = len(d);
    if (l < 350.f) return true;
    V dir = d * (1.f / l); V was = pp; pp += dir * (cur_speed * dt); slide_to(pp, was); yaw = atan2f(-dir.x, dir.y);
    return false;
}
static void puzzle_bot(float dt, bool &use) {
    use = false; bot_t += dt;
    switch (room) {
    case R_HUB: { int k = 0; while (k < 3 && got[k]) k++; goto_xy(door_pos(k), dt); break; }
    case R_RIVER:
        if (rv_st == 0) goto_xy(V(0, 700, 0), dt);
        else if (rv_st == 1) pitch = -50.f;
        else if (rv_st == 2) pitch = rv_t < 2.f ? 75.f : -45.f;
        else if (rv_st == 3) { pitch = 0; if (rv_touch > 0.f) goto_xy(V(0, BANK_Y + 1500.f, 0), dt); }
        else if (rv_st == 4) { if (goto_xy(V(TREE_X - 1000.f, TREE_Y - 1200.f, 0), dt)) use = true; }
        break;
    case R_MOUSE:
        if (km >= 2 && km < 4 && !under_table(pp)) goto_xy(K(0, -250), dt);
        else if (km == 3) { yaw = face_yaw(pp, kmouse_p); bot_t = 0; }   /* it watches the other mouse come in */
        else if (km == 4 && bot_t < 2.5f) { yaw = face_yaw(pp, kmouse_p); }
        else if (km == 4) { if (goto_xy(K(CH_X, CH_Y), dt) || dxy(pp, K(CH_X, CH_Y)) < 2400.f) use = true; }
        else if (km == 5 && bot_t > 1.f) { bot_t = 0; use = true; }
        break;
    case R_DOG:
        if (d_st == 0) { if (goto_xy(V(1900, 3000, 0), dt)) use = true; }
        else if (d_st == 1) { if (goto_xy(V(300, 3900, 0), dt)) use = true; }
        else { if (bs == 0 && goto_xy(V(-2800, TUN_Y[d_tun], 0), dt)) bs = 1; if (bs == 1 && goto_xy(V(-15400.f, TUN_Y[d_tun], 0), dt)) use = true; }
        break;
    default:
        if (bs == 0) { V f(fig_x(), fig_y, 0); if (dxy(pp, f) < 900.f) { use = true; bs = 1; } else goto_xy(f, dt); }
        else if (bs == 1) { if (goto_xy(V(0, 21500, 0), dt)) { use = true; bs = 2; } }
        else if (letter_vis && goto_xy(V(0, 24700, 0), dt)) use = true;
        break;
    }
}

static void p_collect() {   /* grasp the letter: the screen goes blue, back on the platform (article) */
    got[room] = true; letter_vis = false; sfx(3, SK_GET, 110);
    dl("t=%.1f letter %c collected in room %d, %.0f s left", sim_t, "HERO"[room], room, time_left);
    go_blue(R_HUB, 1.2f);
}
static void step_river(const VPIX_Input &in, bool use, V fw, float dt) {
    if (rv_st == 0) {
        if (!rv_said && pp.y > -4000.f) { say("JUMP"); rv_said = true; }
        if (pp.y > 0.f) { rv_st = 1; rv_t = 0; rv_vy = 4000.f; pp.y = (std::max)(pp.y, 900.f); area.clear();   /* clear of the cliff face: the drop is steeper than the drift, and looking down from the lip showed the inside of the cliff (Bobby 11-03) */
            sfx(5, SK_WHISTLE, 110); dl("t=%.1f stepped off the cliff", sim_t); }
    } else if (rv_st == 1) {
        rv_t += dt; pp.y += rv_vy * dt; pz -= FALL_VZ * dt;
        if (rv_t >= FALL_T) { rv_st = 2; rv_t = 0; sfx(5, SK_CRACK, 120); say("LOOK UP"); }
    } else if (rv_st == 2) {
        rv_t += dt;
        /* pull the arm to the body: slower; stick it straight ahead: stop (article). The two arm readings are our reading of one sentence each. Desk and stick: forward or nothing keeps the drift, back brakes, full back stops (Bobby 11-03: forward stopping you in the air felt wrong) */
        float v = CHUTE_F, sx = 0.f;
        if (autoplay) { float tl = (std::max)((pz - RAFT_TOP) / CHUTE_VZ, 0.05f); v = bot_miss > 0 ? CHUTE_F : clampf((raft_y - pp.y) / tl, 0.f, CHUTE_F); }
        else if (in.vr && in.hand.valid && in.head.valid) { V hd(in.head.p[0], in.head.p[1], in.head.p[2]), hn(in.hand.p[0], in.hand.p[1], in.hand.p[2]); float e = dot(hn - hd, fw), side = dot(hn - hd, V(fw.y, -fw.x, 0));
            if (e > 450.f && hn.z > hd.z - 500.f) v = 0.f; else if (e < 200.f) v = CHUTE_F * 0.45f; sx = clampf(side / 400.f, -1.f, 1.f) * 1500.f; }
        else { if (in.joy[1] < -0.1f) v = CHUTE_F * clampf(1.f + in.joy[1], 0.f, 1.f); sx = clampf(in.joy[0], -1.f, 1.f) * 1500.f; }
        pp.y += v * dt; pp.x = clampf(pp.x + sx * dt, -18500.f, 18500.f); pz -= CHUTE_VZ * dt;
        if (pz <= RAFT_TOP) {
            if (fabsf(pp.x) < RAFT_HX + 400.f && fabsf(pp.y - raft_y) < RAFT_HY + 400.f) {
                rv_st = 3; pz = RAFT_TOP; pp.x = clampf(pp.x, -RAFT_HX + 300.f, RAFT_HX - 300.f); pp.y = clampf(pp.y, raft_y - RAFT_HY + 300.f, raft_y + RAFT_HY - 300.f); need_place = true; sfx(9, SK_BUMP, 100); dl("t=%.1f landed on the raft", sim_t);
                area = {{-RAFT_HX, raft_y - RAFT_HY, RAFT_HX, raft_y + RAFT_HY}};
            } else { rv_st = 5; rv_t = 0; sfx(9, SK_SPLASH, 120); dl("t=%.1f in the water", sim_t); if (bot_miss > 0) bot_miss--; go_blue(R_RIVER, 3.f); }
        }
    } else if (rv_st == 3) {
        float stop = BANK_Y - RAFT_HY + 700.f, was = raft_y; raft_y = (std::min)(stop, raft_y + RAFT_V * dt); float dy = raft_y - was; pp.y += dy; walk_off.y += dy;
        rv_roar -= dt; if (rv_roar <= 0.f) { rv_roar = 1.8f; sfx(7, SK_ROAR, (int)(40.f + 80.f * clampf((raft_y - RAFT_Y0) / (stop - RAFT_Y0), 0.f, 1.f))); }
        area.clear(); area.push_back({-RAFT_HX, raft_y - RAFT_HY, RAFT_HX, raft_y + RAFT_HY});
        if (!rv_warned && raft_y > stop - 12000.f) { say("GET OFF THE RAFT"); rv_warned = true; }
        if (raft_y >= stop) { rv_touch += dt; area.push_back({-19000, BANK_Y, 19000, 175000}); if (pp.y > BANK_Y + 300.f) { rv_st = 4; area.erase(area.begin()); dl("t=%.1f ashore", sim_t); } else if (rv_touch > 10.f) { sfx(9, SK_SPLASH, 120); dl("t=%.1f stayed on the raft", sim_t); go_blue(R_RIVER, 3.f); } }
    } else if (rv_st == 4) {
        if (use && !got[R_RIVER] && dxy(pp, letter_p) < 1900.f) p_collect();
    } else if (rv_st == 5) { pz = (std::max)(pz - CHUTE_VZ * dt, -300.f); }
}
static void step_mouse(const VPIX_Input &in, bool use, bool moved, float dt) {
    km_t += dt;
    if (km == 0 && km_t > 1.5f) { km = 1; km_t = 0; sfx(5, SK_RUSH, 110); dl("t=%.1f shrinking", sim_t); return; }
    if (km == 1) { if (km_t > 2.f) { km = 2; km_t = 0; dl("t=%.1f two inches tall", sim_t); } return; }
    if (km < 2) return;
    if (moved) { scurry_t -= dt; if (scurry_t <= 0.f) { scurry_t = 0.35f; sfx(8, SK_SCURRY, 70); } }   /* the small feet sound quick though you move slowly (article) */
    float px = pp.x / KS, py = pp.y / KS;
    if (wk_x < 2300.f) { wk_x += 200.f * dt; int i = (int)floorf(wk_x / 350.f);   /* brown pants and black shoes come from your left */
        if (i != wk_i) { wk_i = i; float fx = i * 350.f, fy = -650.f + ((i & 1) ? 80.f : -80.f); sfx(6, SK_STEP, 100);
            if (fabsf(px - fx) < 170.f && fabsf(py - fy) < 90.f) { sfx(6, SK_CRASH, 120); dl("t=%.1f stepped on", sim_t); go_blue(R_MOUSE, 2.5f); return; } } }
    if (km == 2 && under_table(pp)) { km = 3; km_t = 0; km_mouse = 1; dl("t=%.1f under the table", sim_t); }
    if (km_mouse == 1) { V t = K(-120, -170), d = t - kmouse_p; d.z = 0; float l = len(d); if (l < 300.f) { km_mouse = 2; if (km == 3) km = 4; } else kmouse_p += d * (1.f / l) * (5000.f * dt); }
    if (km == 4 && km_stomp == 0 && px > 150.f) { km_stomp = 1; stomp_t = 0; { float dx = CH_X - px, dy = CH_Y - py, l = (std::max)(hypotf(dx, dy), 1.f); stomp_a = atan2f(dy, dx); stomp_x = px + dx / l * 215.f; stomp_y = py + dy / l * 215.f; } dl("t=%.1f the shoe comes down", sim_t); }   /* three inches ahead of the face */
    if (km_stomp) { stomp_t += dt;
        if (km_stomp == 1 && stomp_t > 0.25f) { km_stomp = 2; stomp_t = 0; sfx(6, SK_CRASH, 120); VPIX_Haptic(0, 0.2f, 0.9f); obst[4].r = {(stomp_x - 110.f) * KS, (stomp_y - 110.f) * KS, (stomp_x + 110.f) * KS, (stomp_y + 110.f) * KS}; obst[4].on = true; if (km_mouse == 2) km_mouse = 3; }
        else if (km_stomp == 2 && stomp_t > 1.0f) { km_stomp = 3; stomp_t = 0; }
        else if (km_stomp == 3 && stomp_t > 0.4f) { km_stomp = 4; obst[4].on = false; } }
    V ch = K(CH_X, CH_Y);
    if (km_mouse == 3 && km_cheese == 0) { V d = ch - kmouse_p; d.z = 0; float l = len(d); if (l < 1500.f) { km_cheese = 9; dl("t=%.1f the other mouse got it", sim_t); go_blue(R_MOUSE, 2.5f); return; } kmouse_p += d * (1.f / l) * (900.f * dt); }
    if (km >= 4 && km_cheese == 0 && use && dxy(pp, ch) < 2600.f) { km_cheese = 1; km = 5; bot_t = 0; sfx(3, SK_USE, 90); say("EAT THE CHEESE"); dl("t=%.1f got the cheese", sim_t); return; }
    if (km_cheese == 1) { bool mouth = in.vr && in.hand.valid && in.head.valid && len(V(in.hand.p[0] - in.head.p[0], in.hand.p[1] - in.head.p[1], in.hand.p[2] - in.head.p[2])) < 300.f; if (mouth || use) { km_cheese = 2; eat_t = 0; } }   /* the stick up to the mouth (article); at a desk the trigger */
    if (km_cheese == 2) { eat_t += dt; if (eat_t > 1.f) { km_cheese = 3; got[R_MOUSE] = true; sfx(3, SK_GET, 110); dl("t=%.1f ate the cheese", sim_t); go_blue(R_HUB, 1.2f); } }
}
static void step_dog(bool use, float dt) {
    if (d_st < 2) { dog_bark -= dt; if (dog_bark <= 0.f) { dog_bark = 4.f; sfx(5, SK_GROWL, 70); } dog_yaw = face_yaw(dog_p, pp); }
    if (use) {
        if (d_st == 0 && dxy(pp, HOOK_P) < 1900.f) { d_st = 1; bot_t = 0; sfx(3, SK_USE, 90); dl("t=%.1f key off the hook", sim_t); }
        else if (d_st == 1 && dxy(pp, ANCHOR_P) < 2600.f) { d_st = 2; letter_vis = true; sfx(5, SK_DOOR, 100); dl("t=%.1f the chain is off, the dog runs for tunnel %d", sim_t, d_tun); }
    }
    if (d_st == 2) { V t = dog_p.x > -2700.f ? V(-2800.f, TUN_Y[d_tun], 0) : V(-15000.f, TUN_Y[d_tun], 0); V d = t - dog_p; d.z = 0; float l = len(d);   /* he goes down one of three tunnels */
        if (l < 200.f) { if (t.x < -3000.f) d_st = 3; else dog_p = t + V(-210.f, 0, 0); } else { dog_p += d * (1.f / l) * (3200.f * dt); dog_yaw = face_yaw(dog_p, t); } }
    if (use && letter_vis && !got[R_DOG] && dxy(pp, letter_p) < 1900.f) p_collect();
}
static void step_hall(bool use, float dt) {
    if (key_st == 0) { fig_y += fig_dir * fig_speed * dt; if (fig_y > 19500.f) fig_dir = -1; if (fig_y < 15000.f) fig_dir = 1; }
    if (use) {
        V fg(fig_x(), fig_y, 0);
        if (key_st == 0 && dxy(pp, fg) < REACH_USE) { key_st = 1; dl("t=%.1f the figure gave up the yellow thing", sim_t); }
        else if (key_st == 1 && dxy(pp, V(0, 23500, 0)) < 2800.f) { key_st = 2; door_go = true; sfx(5, SK_DOOR, 100); dl("t=%.1f red door unlocked", sim_t); }
    }
    if (door_go) { door_t = (std::min)(1.f, door_t + dt / 1.5f); obst[0].on = door_t < 0.9f; if (door_t >= 1.f && !got[R_HALL]) letter_vis = true; }
    if (use && letter_vis && !got[R_HALL] && dxy(pp, letter_p) < 1900.f) p_collect();
}
static void p_step(const VPIX_Input &in, bool use, float dt) {
    if (say_t > 0) say_t -= dt;
    time_left -= dt;
    if (time_left <= 0) { won = false; st = S_OVER; st_t = 0; dl("t=%.1f time up: %d letters", sim_t, (int)got[0] + got[1] + got[2] + got[3]); return; }
    if (blue_t >= 0.f) {
        blue_t += dt;
        if (blue_t >= blue_len) { int to = blue_to; blue_t = -1.f; enter_room(to);
            if (to == R_HUB && got[0] && got[1] && got[2] && got[3]) { won = true; st = S_OVER; st_t = 0; sfx(4, SK_WIN, 110); dl("t=%.1f HERO spelt, %.0f s left", sim_t, time_left); } }
        return;
    }
    /* ---- the player ---- */
    V hxy(in.head.p[0], in.head.p[1], 0);
    V fw = in.head.valid ? norm(V(in.head.R[1][0], in.head.R[1][1], 0)) : V(-sinf(yaw), cosf(yaw), 0);
    const bool can_walk = !(room == R_RIVER && (rv_st == 1 || rv_st == 2 || rv_st == 5)) && !(room == R_MOUSE && km < 2);
    const V before = pp;
    if (autoplay) { bool u; puzzle_bot(dt, u); use = use || u; }
    else if (!can_walk) { walk_off = V(pp.x, pp.y, 0) - hxy; need_place = false; yaw = atan2f(-fw.x, fw.y); }
    else {
        if (need_place) { walk_off = V(pp.x, pp.y, 0) - hxy; need_place = false; }
        d1k::desk_walk(in, fw, cur_speed, dt, walk_off);
        V raw(walk_off.x + hxy.x, walk_off.y + hxy.y, 0), p = raw;
        slide_to(p, pp);
        walk_off += p - raw; pp = p; yaw = atan2f(-fw.x, fw.y);
    }
    if (poke_t > 0.f) poke_t -= dt;
    if (use) { poke_t = 0.2f; sfx(1, SK_USE, 70); VPIX_Haptic(0, 0.05f, 0.5f); }   /* in a window the hand pushes forward so the press shows */
    /* ---- the room ---- */
    if (room == R_HUB) { for (int k = 0; k < 4; k++) if (!got[k] && dxy(pp, door_pos(k)) < 800.f) { sfx(2, SK_GO, 100); enter_room(k); break; } }
    else if (room == R_RIVER) step_river(in, use, fw, dt);
    else if (room == R_MOUSE) step_mouse(in, use, dxy(pp, before) > 0.5f, dt);
    else if (room == R_DOG) step_dog(use, dt);
    else step_hall(use, dt);
}

/* ---- puzzle meshes ---- */
static void letter_mesh(int id, int ch) {   /* a flat letter in 160 mm cells, front toward -y, origin at its bottom centre */
    Mesh m; const char *g = d1k::FONT[ch]; const float c = 160.f;
    for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (g[y * 5 + x] == '1') {
        float x0 = (x - 2.5f) * c, z0 = (6 - y) * c;
        m.box(V(x0, -20, z0), V(x0 + c, 20, z0 + c), 0xE6D23C);
    }
    m.add(id);
}
static void word_mesh(int id, const char *w, float c, uint32_t col) {   /* a word over a door, front toward -y, centred, origin at its bottom */
    Mesh m; const int n = (int)strlen(w); const float W = n * 6.f * c;
    for (int i = 0; i < n; i++) { const char *g = d1k::FONT[w[i] - 'A'];
        for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (g[y * 5 + x] == '1') { float x0 = -W * 0.5f + (i * 6 + x) * c, z0 = (6 - y) * c; m.box(V(x0, -30, z0), V(x0 + c, 30, z0 + c), col); } }
    m.add(id);
}
static void tcyl(Mesh &m, V b, float r0, V t, float r1, uint32_t c, uint32_t c2) {   /* a tapered eight-sided cylinder */
    for (int i = 0; i < 8; i++) { float a0 = 6.2831853f * i / 8.f, a1 = 6.2831853f * (i + 1) / 8.f; V d0(cosf(a0), sinf(a0), 0), d1(cosf(a1), sinf(a1), 0);
        m.poly({b + d0 * r0, b + d1 * r0, t + d1 * r1, t + d0 * r1}, (i & 1) ? c : c2); }
}
static void shell(Mesh &m, float x0, float y0, float x1, float y1, float h, uint32_t flo, uint32_t wal, uint32_t cei, bool south = true, bool north = true) {
    const float T = 400.f, ys = y0 - (south ? T : 0.f), yn = y1 + (north ? T : 0.f);
    m.box(V(x0 - T, ys, -200), V(x1 + T, yn, 0), flo);
    m.box(V(x0 - T, ys, h), V(x1 + T, yn, h + 200), cei);
    m.box(V(x0 - T, ys, 0), V(x0, yn, h), wal);
    m.box(V(x1, ys, 0), V(x1 + T, yn, h), wal);
    if (south) m.box(V(x0, y0 - T, 0), V(x1, y0, h), wal);
    if (north) m.box(V(x0, y1, 0), V(x1, y1 + T, h), wal);
}
static void build_puzzle() {
    { Mesh m;   /* the hub: a platform with four short walkways, an air-lock door at the end of each (article). Colours INVENTED, from the corridor's greys */
      const uint32_t PL = 0xA8A8B0, PS = 0x808088, WK = 0x8A7E84;
      m.box(V(-3000, -3000, -200), V(3000, 3000, 0), PL, PS, PS);
      m.box(V(-1000, 3000, -150), V(1000, DOOR_D, 0), WK, PS, PS); m.box(V(-1000, -DOOR_D, -150), V(1000, -3000, 0), WK, PS, PS);
      m.box(V(3000, -1000, -150), V(DOOR_D, 1000, 0), WK, PS, PS); m.box(V(-DOOR_D, -1000, -150), V(-3000, 1000, 0), WK, PS, PS);
      m.add(HM_SHELL + R_HUB); }
    { Mesh d;   /* an air-lock door: a heavy frame, an eight-sided hatch and its wheel, front toward -y. The look is INVENTED, "air-lock style" is the article's */
      const uint32_t F = 0x6A6A74, F2 = 0x54545E, H = 0x9A9AA4, Y = 0xD8B020; const float T = 400.f;
      d.box(V(-1300, 0, 0), V(-900, T, 2500), F, F2, F2); d.box(V(900, 0, 0), V(1300, T, 2500), F, F2, F2); d.box(V(-1300, 0, 2500), V(1300, T, 2800), F, F2, F2); d.box(V(-900, 150, 0), V(900, T, 2500), F2);
      V o[8]; for (int i = 0; i < 8; i++) { float a = 6.2831853f * (i + 0.5f) / 8.f; o[i] = V(cosf(a) * 860.f, 130, 1250.f + sinf(a) * 1130.f); }
      d.poly({o[7], o[6], o[5], o[4], o[3], o[2], o[1], o[0]}, H);
      for (int i = 0; i < 8; i++) { float a0 = 6.2831853f * i / 8.f, a1 = 6.2831853f * (i + 1) / 8.f; d1k::kit_bar(d, V(cosf(a0) * 260.f, 90, 1250.f + sinf(a0) * 260.f), V(cosf(a1) * 260.f, 90, 1250.f + sinf(a1) * 260.f), 30, 30, Y); }
      d1k::kit_bar(d, V(-260, 90, 1250), V(260, 90, 1250), 30, 30, Y); d1k::kit_bar(d, V(0, 90, 990), V(0, 90, 1510), 30, 30, Y);
      d.add(HM_AIRLOCK); }
    word_mesh(HM_SENTER, "ENTER", 62.f, 0x60E070); word_mesh(HM_SDONE, "DONE", 62.f, 0xC84030);
    { Mesh m;   /* the river: a green field on a cliff above a river under a blue sky; the grass ends in a drop-off, cliff walls, a far bank (article) */
      const uint32_t GR = 0x3E8A3A, GS = 0x6E5E48, RK = 0x7A6A54, RK2 = 0x5E5242, WA = 0x2D4A8C, SA = 0xB8A468;
      m.box(V(-60000, -40000, 0), V(60000, 0, CZ), GR, GS, GS);
      m.box(V(-60000, 0, 0), V(-20000, 190000, CZ), GR, RK, RK2); m.box(V(20000, 0, 0), V(60000, 190000, CZ), GR, RK2, RK);
      m.poly({V(-20000, 0, 0), V(20000, 0, 0), V(20000, BANK_Y + 2000, 0), V(-20000, BANK_Y + 2000, 0)}, WA);
      m.box(V(-20000, BANK_Y, 0), V(20000, 175000, RAFT_TOP), SA, 0x8A7C50, 0x8A7C50);
      m.box(V(-60000, 175000, 0), V(60000, 215000, CZ), GR, RK, RK2);
      m.add(HM_SHELL + R_RIVER); }
    { Mesh r; const uint32_t L = 0x8A6A3A, L2 = 0x6E5430; for (int i = 0; i < 6; i++) { float x0 = -RAFT_HX + i * (2.f * RAFT_HX / 6.f); r.box(V(x0 + 20.f, -RAFT_HY, -RAFT_TOP), V(x0 + 2.f * RAFT_HX / 6.f - 20.f, RAFT_HY, 0), (i & 1) ? L : L2, L2, L2); }
      r.box(V(-RAFT_HX, -RAFT_HY + 300, 0), V(RAFT_HX, -RAFT_HY + 500, 40), 0x5A4220); r.box(V(-RAFT_HX, RAFT_HY - 500, 0), V(RAFT_HX, RAFT_HY - 300, 40), 0x5A4220); r.add(HM_RAFT); }
    { Mesh c;   /* the square chute (article): overhead, seen by looking up, lines to your harness */
      const float Z = 7000.f, H = 3000.f; const uint32_t C1 = 0xE8E4D8, C2 = 0xC85030;
      for (int i = 0; i < 4; i++) { float x0 = -H + i * (2.f * H / 4.f), x1 = x0 + 2.f * H / 4.f; two_sided(c, V(x0, -H, Z), V(x1, -H, Z), V(x1, H, Z), V(x0, H, Z), (i & 1) ? C2 : C1, (i & 1) ? C2 : C1); }
      for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) d1k::kit_bar(c, V(a ? H : -H, b ? H : -H, Z - 20.f), V(a ? 300.f : -300.f, b ? 200.f : -200.f, 1200.f), 12, 12, 0x303038);
      c.add(HM_CHUTE); }
    { Mesh t;   /* the tree, to the writer's words: three tapered cylinders make the trunk, a few branches; the green plane is HM_LEAF (article) */
      const uint32_t B = 0x6A4820, B2 = 0x55381A;
      tcyl(t, V(0, 0, 0), 420, V(0, 0, 1300), 320, B, B2); tcyl(t, V(0, 0, 1300), 320, V(0, 0, 2500), 220, B, B2); tcyl(t, V(0, 0, 2500), 220, V(0, 0, 3600), 110, B, B2);
      tcyl(t, V(0, 0, 2300), 140, V(-1300, 200, 3400), 50, B, B2); tcyl(t, V(0, 0, 2600), 120, V(1200, -300, 3500), 50, B, B2);
      t.add(HM_TREE); }
    { Mesh l; V o[8]; const float Z = 3300.f, R = 2800.f; for (int i = 0; i < 8; i++) { float a = 6.2831853f * i / 8.f; o[i] = V(cosf(a) * R, sinf(a) * R, Z); }
      const V dn(0, 0, -15); l.poly({o[0], o[1], o[2], o[3], o[4], o[5], o[6], o[7]}, 0x30B040); l.poly({o[7] + dn, o[6] + dn, o[5] + dn, o[4] + dn, o[3] + dn, o[2] + dn, o[1] + dn, o[0] + dn}, 0x30B040); l.add(HM_LEAF); }
    { Mesh m;   /* the kitchen in true mm times KS: table and counter (article); colours and tiles INVENTED */
      const uint32_t W1 = 0xD8D0A8, W2 = 0xB8B088, CE = 0xE8E4D0, TB = 0x8A5A2A, TB2 = 0x6A4220, CT = 0xC8C8C0, CF = 0x7A6A9A, CF2 = 0x62548A;
      for (int i = 0; i < 12; i++) for (int j = 0; j < 12; j++) { float x0 = -1500.f + i * 250.f, y0 = -1200.f + j * 225.f; m.poly({K(x0, y0), K(x0 + 250.f, y0), K(x0 + 250.f, y0 + 225.f), K(x0, y0 + 225.f)}, ((i + j) & 1) ? 0xC8C0A8 : 0x8A8478); }
      m.poly({K(-1500, -1200, 2400), K(-1500, 1500, 2400), K(1500, 1500, 2400), K(1500, -1200, 2400)}, CE);
      m.poly({K(-1500, -1200), K(-1500, 1500), K(-1500, 1500, 2400), K(-1500, -1200, 2400)}, W1); m.poly({K(1500, 1500), K(1500, -1200), K(1500, -1200, 2400), K(1500, 1500, 2400)}, W2);
      m.poly({K(-1500, 1500), K(1500, 1500), K(1500, 1500, 2400), K(-1500, 1500, 2400)}, W1); m.poly({K(1500, -1200), K(-1500, -1200), K(-1500, -1200, 2400), K(1500, -1200, 2400)}, W2);
      m.box(K(T_X0, T_Y0, 720), K(T_X1, T_Y1, 760), TB, TB2, TB2);
      for (int i = 0; i < 4; i++) { float x = (i & 1) ? T_X1 - 40.f : T_X0 + 40.f, y = (i & 2) ? T_Y1 - 40.f : T_Y0 + 40.f; m.box(K(x - 25.f, y - 25.f, 0), K(x + 25.f, y + 25.f, 720), TB2); }
      m.box(K(-1500, 1100, 0), K(600, 1500, 880), CF, CF2, CF2); m.box(K(-1520, 1080, 880), K(620, 1500, 920), CT, 0xA8A8A0, 0xA8A8A0);
      m.add(HM_SHELL + R_MOUSE); }
    { Mesh l;   /* one leg: a black shoe, brown pants (article), authored at KS */
      const uint32_t BK = 0x141418, BK2 = 0x26262C, BR = 0x6A4422, BR2 = 0x52341A;
      l.box(K(-140, -50, 0), K(140, 50, 45), BK2, BK, BK); l.box(K(-140, -50, 45), K(10, 50, 95), BK2, BK, BK);
      l.box(K(-125, -60, 95), K(5, 60, 900), BR, BR2, BR2); l.add(HM_LEG); }
    { Mesh m;   /* the dog's small room, three tunnels in the west wall going into darkness (article). Room colours INVENTED */
      const uint32_t FL = 0x4A3A2C, WL = 0x7A6A55, WL2 = 0x665A48, CE = 0x3A3028, TN = 0x4A4034, TN2 = 0x3A3228; const float H = 3000.f, TH = 2200.f;
      m.box(V(-3400, -400, -200), V(3400, 6400, 0), FL); m.box(V(-3400, -400, H), V(3400, 6400, H + 200), CE);
      m.box(V(3000, -400, 0), V(3400, 6400, H), WL2); m.box(V(-3000, -400, 0), V(3000, 0, H), WL); m.box(V(-3000, 6000, 0), V(3000, 6400, H), WL);
      const float E[4][2] = {{0, 500}, {1900, 2300}, {3700, 4100}, {5500, 6000}}; for (int i = 0; i < 4; i++) m.box(V(-3400, E[i][0], 0), V(-3000, E[i][1], H), WL2);
      for (int i = 0; i < 3; i++) { float y = TUN_Y[i]; m.box(V(-3400, y - 700, TH), V(-3000, y + 700, H), WL2);
          m.poly({V(TUN_END, y - 700, 2), V(-3000, y - 700, 2), V(-3000, y + 700, 2), V(TUN_END, y + 700, 2)}, TN2); m.box(V(TUN_END, y - 700, TH), V(-3400, y + 700, TH + 200), TN2);
          m.box(V(TUN_END, y - 1000, 0), V(-3400, y - 700, TH), TN); m.box(V(TUN_END, y + 700, 0), V(-3400, y + 1000, TH), TN); m.box(V(TUN_END - 300, y - 700, 0), V(TUN_END, y + 700, TH), TN); }
      m.box(V(HOOK_P.x - 40, HOOK_P.y - 40, HOOK_P.z + 260), V(3000, HOOK_P.y + 40, HOOK_P.z + 340), 0x303030);   /* the hook */
      m.box(V(ANCHOR_P.x - 150, 5900, ANCHOR_P.z - 150), V(ANCHOR_P.x + 150, 6000, ANCHOR_P.z + 150), 0x505058);   /* the lock where the chain meets the wall */
      m.add(HM_SHELL + R_DOG); }
    { Mesh m;   /* room 0: the long dim corridor, red-brown floor, grey block pillars, a white window frame on the left (footage), the red arched door at the end */
      const uint32_t WL = 0x8A7E84, TOP = 0xA89E94, DKS = 0x6E5054, FL = 0x74303A;
      shell(m, -1500, 0, 1500, 27200, 3200, FL, WL, 0x6A6066);
      for (int k = -1; k <= 1; k += 2) { float a = k > 0 ? 520.f : -1500.f, b = k > 0 ? 1500.f : -520.f; m.poly({V(a, 0, 3), V(b, 0, 3), V(b, 23000, 3), V(a, 23000, 3)}, WL); }
      m.box(V(-1500, 23000, 0), V(-1040, 23800, 3200), WL); m.box(V(1040, 23000, 0), V(1500, 23800, 3200), WL); m.box(V(-1040, 23000, 2900), V(1040, 23800, 3200), WL);
      for (const float *b = BLK; b < BLK + NBLK * 3; b += 3) m.box(V(b[0], b[1], 0), V(b[0] + 900.f, b[1] + 1000.f, b[2]), TOP, DKS, DKS);
      for (int i = 0; i < 9; i++) { float z = i * 340.f, x0 = (i & 1) ? 800.f : 880.f; for (int k = -1; k <= 1; k += 2) m.box(V(k > 0 ? x0 : -1060.f, 23000, z), V(k > 0 ? 1060.f : -x0, 23340, z + 340.f), (i & 1) ? 0xB8A098 : 0x8A6E6C); }   /* the jagged frame */
      /* the white pilaster and the lattice at its foot */
      m.box(V(-1500, 13000, 0), V(-1100, 14400, 3200), 0xF0EEE6); m.box(V(-1098, 13680, 0), V(-1090, 13720, 3200), 0xA8A49C);
      for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) { float x = -1090.f + i * 200.f, y = 12300.f + j * 230.f; m.poly({V(x, y, 6), V(x + 170, y, 6), V(x + 170, y + 200, 6), V(x, y + 200, 6)}, 0xE8E6DE); }
      m.add(HM_SHELL + R_HALL); }
    { Mesh d; const uint32_t R = 0xA02028, R2 = 0x6E1418;   /* the red arched door, narrow and tall */
      d.box(V(-800, -150, 0), V(800, 150, 2000), R); for (int i = 0; i < 6; i++) { float a0 = 3.14159f * i / 6.f, a1 = 3.14159f * (i + 1) / 6.f; V p0(cosf(a0) * 800.f, -150, 2000.f + sinf(a0) * 900.f), p1(cosf(a1) * 800.f, -150, 2000.f + sinf(a1) * 900.f); d.poly({V(0, -150, 2000), p0, p1}, R); }
      d.box(V(-560, -170, 180), V(560, -150, 1950), R2);
      d.add(HM_DOOR); }
    { Mesh k; const uint32_t Y = 0xF0D820;   /* reads as a yellow oval in every frame; a key from the written account */
      k.box(V(-22, -320, -22), V(22, 150, 22), Y); k.box(V(22, -320, -22), V(100, -200, 22), Y); for (int s = 0; s < 2; s++) { float y0 = s ? -28.f : 28.f; for (int i = 0; i < 8; i++) { float a0 = 6.28318f * i / 8.f, a1 = 6.28318f * (i + 1) / 8.f; V c(0, 300, 0), p0 = c + V(cosf(a0) * 150.f, sinf(a0) * 210.f, y0), p1 = c + V(cosf(a1) * 150.f, sinf(a1) * 210.f, y0); if (s) k.poly({c + V(0, 0, y0), p1, p0}, 0xD8BC18); else k.poly({c + V(0, 0, y0), p0, p1}, Y); } } k.add(HM_KEY); }
    { Mesh c; V p0(-300, -300, 0), p1(300, -300, 0), p2(-300, 300, 0), q0(-300, -300, 300), q1(300, -300, 60), q2(-300, 300, 300);
      c.poly({p0, p2, p1}, 0xC8A020); c.poly({q0, q1, q2}, 0xF0C830); c.poly({p0, p1, q1, q0}, 0xE8B828); c.poly({p0, q0, q2, p2}, 0xE8B828); c.poly({p1, p2, q2, q1}, 0xF4D040);
      c.add(HM_CHEESE); }
    { Mesh m; m.box(V(-150, -300, 0), V(150, 260, 240), 0x9A9A9A); m.box(V(-90, 260, 20), V(90, 420, 180), 0xAAAAAA); m.box(V(-130, 140, 240), V(-60, 220, 300), 0xE0A0A0); m.box(V(60, 140, 240), V(130, 220, 300), 0xE0A0A0);
      m.box(V(-15, -700, 20), V(15, -300, 50), 0xD0A0A0); m.add(HM_MOUSE); }
    { Mesh d;   /* the dog up: a brown dog on four legs, facing +y */
      for (int i = 0; i < 4; i++) { float x = (i & 1) ? 180.f : -180.f, y = (i & 2) ? 450.f : -450.f; d.box(V(x - 60, y - 60, 0), V(x + 60, y + 60, 380), 0x5A3C20); }
      d.box(V(-250, -650, 380), V(250, 650, 900), 0x7A5430); d.box(V(-200, 650, 650), V(200, 1100, 1050), 0x6A4824); d.box(V(-100, 1100, 700), V(100, 1350, 900), 0xC8A070);
      d.box(V(-230, 780, 1050), V(-120, 900, 1250), 0x4A3018); d.box(V(120, 780, 1050), V(230, 900, 1250), 0x4A3018); d.box(V(-40, -780, 800), V(40, -650, 1200), 0x7A5430);
      d.add(HM_DOG); }
    { Mesh d; d.box(V(-260, -650, 0), V(260, 650, 420), 0x7A5430); d.box(V(-200, 650, 0), V(200, 1050, 330), 0x6A4824); d.box(V(-40, -900, 0), V(40, -650, 160), 0x7A5430); d.add(HM_DOGZ); }
    { Mesh f;   /* the small grey figure */
      const uint32_t G = 0xB0B0B4, G2 = 0x8A8A92, H = 0xC8C8CC;
      f.box(V(-125, -70, 0), V(-30, 70, 860), G2, G, G2); f.box(V(30, -70, 0), V(125, 70, 860), G2, G, G2); f.box(V(-150, -85, 860), V(150, 85, 1420), G, G, G2);
      f.box(V(-40, -40, 1420), V(40, 40, 1490), H); f.box(V(-85, -85, 1490), V(85, 90, 1700), H, H, G);
      f.box(V(-215, -50, 820), V(-150, 50, 1400), G, G, G2); f.box(V(150, -50, 840), V(215, 170, 1400), G, G, G2);
      f.add(HM_FIG); }
    letter_mesh(HM_LET + 0, 'H' - 'A'); letter_mesh(HM_LET + 1, 'E' - 'A'); letter_mesh(HM_LET + 2, 'R' - 'A'); letter_mesh(HM_LET + 3, 'O' - 'A');
    for (int mir = 0; mir < 4; mir++) { Mesh a;   /* an arm with ZENECA's hand on it (vpix_1k_hero_hand.h), hand at the origin, y forward, HM_ARM right, HM_ARM2 left, the _F ones shut. FROM FOOTAGE _X_E_Kg9roE t1621: peach, two arms. Using Zeneca's hand is Bobby's call */
      const uint32_t O = 0xF4BC94, O2 = 0xCC9270; const bool fist = mir >= 2; const float sx = (mir & 1) ? -1.f : 1.f;
      auto qd = [&](V p, V q, V r, V t, uint32_t c) { p.x *= sx; q.x *= sx; r.x *= sx; t.x *= sx; if (mir & 1) a.poly({t, r, q, p}, c); else a.poly({p, q, r, t}, c); };
      auto bar = [&](float x, float y0, float w0, float t0, float y1, float w1, float t1) {
          const V A[4] = {V(x - w0, y0, -t0), V(x + w0, y0, -t0), V(x + w0, y0, t0), V(x - w0, y0, t0)}, B[4] = {V(x - w1, y1, -t1), V(x + w1, y1, -t1), V(x + w1, y1, t1), V(x - w1, y1, t1)};
          qd(A[3], A[2], B[2], B[3], O); qd(A[1], A[0], B[0], B[1], O2); qd(A[0], A[3], B[3], B[0], O2); qd(A[2], A[1], B[1], B[2], O2); qd(A[0], A[1], A[2], A[3], O2); };
      bar(10, -1500, 78, 78, -30, 26, 46);     /* the arm */
      bar(10, -30, 26, 46, 12, 18, 46);        /* the wrist, into the heel of the hand */
      for (size_t i = 0, n = fist ? sizeof(HERO_FIST) / sizeof(HERO_FIST[0]) : sizeof(HERO_HAND) / sizeof(HERO_HAND[0]); i < n; i++) { const HeroHandTri &h = fist ? HERO_FIST[i] : HERO_HAND[i]; V p(sx * h.v[0], h.v[1], h.v[2]), q(sx * h.v[3], h.v[4], h.v[5]), r(sx * h.v[6], h.v[7], h.v[8]); if (mir & 1) a.poly({r, q, p}, h.rgb); else a.poly({p, q, r}, h.rgb); }
      a.add(mir == 0 ? HM_ARM : mir == 1 ? HM_ARM2 : mir == 2 ? HM_ARM_F : HM_ARM2_F); }
    { Mesh l; l.box(V(-60, -60, -60), V(60, 60, 60), 0x909090); l.add(HM_LINK); }
    { Mesh s; s.poly({V(-40, 0, -24), V(40, 0, -24), V(0, 0, 90)}, 0xFFE040); s.poly({V(0, -40, -24), V(0, 40, -24), V(0, 0, 90)}, 0xFFE040); s.add(HM_SPARK); }
}

/* ======================================================================== glider ======================================================================== */
static float gpx = 0, gpy = 0, gpz = 20000, alt_off = 0, flight_t = 0, zap_t = 0, splat_t = 0, bump_cd = 0; static int deaths = 0, splats = 0, zap_row = 0;
static V head_ref; static bool have_ref = false;
static const int N_ROWS = 6, N_GULLS = 12;
static const float ROW_Y0 = 200000.f, ROW_GAP = 26000.f, PY_SPAN = 12000.f, WIRE_SAG = 700.f;
static const float WIRE_Z[2] = {14500.f, 11000.f};
static int row_gap[N_ROWS];
struct Gull { float cx, cy, cz, r, w, a, cd; V p, v; };
static Gull gulls[N_GULLS];
struct Drop { V p, v; float age; };
static std::vector<Drop> drops;
static float row_y(int r) { return ROW_Y0 + r * ROW_GAP; }
static float zprofile(float y) {
    if (y < 180000.f) return 20000.f - 7000.f * (y / 180000.f);
    if (y < 360000.f) return 13000.f;
    if (y < 420000.f) return 13000.f - 3500.f * ((y - 360000.f) / 60000.f);
    if (y < 480000.f) return 9500.f;
    return 9500.f * (1.f - clampf((y - 480000.f) / 60000.f, 0.f, 1.f));
}
static void gull_move(Gull &g, float dt) {
    g.a += g.w * dt;
    g.p = V(g.cx + g.r * cosf(g.a), g.cy + g.r * sinf(g.a), g.cz + 600.f * sinf(2.f * g.a));
    g.v = V(-sinf(g.a), cosf(g.a), 0) * (g.r * g.w);
}
static void reset_flight() {
    gpx = 0; gpy = 0; gpz = zprofile(0); alt_off = 0; flight_t = 0; zap_t = 0; splat_t = 0; bump_cd = 0; deaths = 0; splats = 0; drops.clear(); have_ref = false;
    for (int i = 0; i < N_GULLS; i++) { Gull &g = gulls[i];
        g.cy = 8000.f + i * 15000.f; g.cx = rnd(-5000.f, 5000.f); g.r = rnd(3000.f, 7500.f); g.cz = zprofile(g.cy) + rnd(-3000.f, 4500.f);
        g.w = rnd(0.35f, 0.7f) * (rnd(0.f, 1.f) < 0.5f ? 1.f : -1.f); g.a = rnd(0.f, 6.2831853f); g.cd = rnd(1.f, 5.f); gull_move(g, 0); }
    int sides[N_ROWS] = {3, 4, 4, 3, 4, 3};
    for (int r = 0; r < N_ROWS; r++) row_gap[r] = sides[r];
}
static void zap(int r) {
    zap_t = 1.8f; zap_row = r; deaths++; sfx(7, SK_ZAP, 120); dl("t=%.1f ELECTROCUTED at pylon row %d (death %d)", sim_t, r, deaths);
}
static void g_step(const VPIX_Input &in, float dt) {
    for (int i = 0; i < N_GULLS; i++) gull_move(gulls[i], dt);
    for (size_t i = 0; i < drops.size();) { Drop &d = drops[i]; d.v.z -= 9800.f * dt; d.p += d.v * dt; d.age += dt;
        V e(gpx, gpy, gpz);
        if (len(d.p - e) < 700.f && splat_t <= 0.f) { splat_t = 3.f; splats++; sfx(8, SK_SPLAT, 110); dl("t=%.1f a seagull hit you (%d)", sim_t, splats); d.age = 99.f; }
        if (d.age > 8.f || d.p.z < 0) { drops[i] = drops.back(); drops.pop_back(); } else i++; }
    if (splat_t > 0) splat_t -= dt;
    if (bump_cd > 0) bump_cd -= dt;
    if (zap_t > 0) {
        zap_t -= dt;
        if (zap_t <= 0) { gpy = (std::max)(0.f, row_y(zap_row) - 22000.f); gpx = 0; alt_off = 0; gpz = zprofile(gpy); }   /* back before the row (INVENTED) */
        return;
    }
    flight_t += dt;
    float jx = in.joy[0], jy = in.joy[1];
    if (autoplay) {
        float tgt = 0.f;
        for (int r = 0; r < N_ROWS; r++) { float d = row_y(r) - gpy; if (d > -500.f && d < 45000.f) { tgt = ((row_gap[r] - 4) + 0.5f) * PY_SPAN; break; } }
        jx = clampf((tgt - gpx) / 3000.f, -1.f, 1.f); jy = 0.f;
    }
    jx = clampf(jx, -1.f, 1.f); jy = clampf(jy, -1.f, 1.f);
    gpy += glide_speed * dt;
    gpx += (jx * steer_speed + 700.f * sinf(flight_t * 0.35f)) * dt;
    if (gpy > FLIGHT_LEN - 100000.f) gpx += -gpx * 0.6f * dt;   /* the last stretch settles over the avenue */
    gpx = clampf(gpx, -14000.f, 14000.f);
    alt_off = clampf(alt_off - jy * climb_speed * dt, -3500.f, 3500.f);
    float fade = clampf((FLIGHT_LEN - gpy) / 60000.f, 0.f, 1.f);
    gpz = zprofile(gpy) + alt_off * fade;
    if (gpy > 360000.f && gpy < 480000.f) gpz = (std::max)(gpz, 9000.f);
    if (gpz < 300.f) gpz = 300.f;
    /* pylons: towers and wires kill */
    for (int r = 0; r < N_ROWS; r++) {
        if (fabsf(gpy - row_y(r)) > 450.f) continue;
        for (int j = -3; j <= 3; j++) if (fabsf(gpx - j * PY_SPAN) < 700.f && gpz < 20000.f) { zap(r); return; }
        int s = (int)floorf(gpx / PY_SPAN) + 4;   /* wire segment s runs from pylon s-4 to s-3 */
        if (s >= 1 && s <= 6 && s != row_gap[r]) {
            float u = (gpx - (s - 4) * PY_SPAN) / PY_SPAN;
            for (int h = 0; h < 2; h++) { float zw = WIRE_Z[h] - WIRE_SAG * 4.f * u * (1.f - u); if (fabsf(gpz - zw) < 450.f) { zap(r); return; } }
        }
    }
    /* seagulls: bump, and drop on you */
    for (int i = 0; i < N_GULLS; i++) { Gull &g = gulls[i];
        V e(gpx, gpy, gpz);
        if (bump_cd <= 0 && len(g.p - e) < 900.f) { gpx += (gpx >= g.p.x ? 1200.f : -1200.f); bump_cd = 1.f; sfx(9, SK_BUMP, 90); dl("t=%.1f bumped seagull %d", sim_t, i); }
        g.cd -= dt;
        if (g.cd <= 0) {
            float dz = g.p.z - gpz;
            if (dz > 2500.f && dz < 9000.f) { float tf = sqrtf(2.f * dz / 9800.f);
                V pg = g.p + g.v * tf, pe(gpx, gpy + glide_speed * tf, 0);
                if (hypotf(pg.x - pe.x, pg.y - pe.y) < 1400.f) { Drop d; d.p = g.p; d.v = g.v; d.age = 0; drops.push_back(d); g.cd = rnd(5.f, 9.f); } }
            else g.cd = 0.2f;
        }
    }
    if (gpy >= FLIGHT_LEN) { st = S_OVER; st_t = 0; sfx(2, SK_WIN, 110); dl("t=%.1f landed: %d deaths, %d droppings, flight %.0f s", sim_t, deaths, splats, flight_t); }
}

static void g_blit(VPIX_Frame &f, int id, int w, int h, float x, float y, float dw, float dh) {
    VPIX_Blit b = {}; b.handle = d1k::BMPH + id; b.sw = w; b.sh = h; b.x = (int)x; b.y = (int)y; b.dw = dw; b.dh = dh; b.key = 0; b.alpha = 1.f; b.prio = 1000000; b.tint[0] = b.tint[1] = b.tint[2] = 1.f; f.blits.push_back(b);
}
static void g_delta(Mesh &o, uint32_t top, int f0, int f1) {   /* top colour, the zebra fan under it from stripe f0 to f1 of 8; the pilot lies under it in orange in a dark frame (t1228, t1282, t1246) */
    const V A(0, 2400, 0), L(-4800, -2200, 0), Rr(4800, -2200, 0); two_sided3(o, A, L, Rr, top, top);
    for (int i = f0; i < f1; i++) { V p0 = L + (Rr - L) * (i / 8.f), p1 = L + (Rr - L) * ((i + 1) / 8.f); two_sided3(o, A + V(0, 0, -24), p0 + V(0, 0, -24), p1 + V(0, 0, -24), (i & 1) ? 0x141414 : 0xE8E8E8, (i & 1) ? 0x141414 : 0xE8E8E8); }
    o.box(V(-170, -800, -1560), V(170, 700, -1260), 0xE08020); o.box(V(-110, 700, -1540), V(110, 950, -1300), 0xD8A080);
    d1k::kit_bar(o, V(0, 200, -24), V(-1000, 500, -1800), 28, 28, 0x202024); d1k::kit_bar(o, V(0, 200, -24), V(1000, 500, -1800), 28, 28, 0x202024); d1k::kit_bar(o, V(-1000, 500, -1800), V(1000, 500, -1800), 28, 28, 0x202024);
}
static void build_glider() {
    { std::vector<uint32_t> px(HG_EW * HG_EH); for (int i = 0; i < HG_EW * HG_EH; i++) px[i] = HG_END_PAL[HG_END[i]]; add_bitmap(BM_HEND, HG_EW, HG_EH, px);
      std::vector<uint32_t> t2(HG_PW * HG_PH); for (int i = 0; i < HG_PW * HG_PH; i++) t2[i] = HG_PLATE_PAL[HG_PLATE[i]]; VPIX_Render_AddTextureRGBA(TEX_HPLATE, HG_PW, HG_PH, t2.data()); }
    /* your own glider overhead: a delta wing, keel, and the bar you hold (Bobby 2026-11-02). INVENTED: footage only looks forward. Eye at 0 */
    { Mesh s; const uint32_t O = 0xC05030, O2 = 0x8C4028, T = 0x505058;   /* the nose reaches the top of the level view, the edges spread about 2 to 1, one side darker (t1222, t1282, t1284) */
      s.poly({V(0, 1900, 1000), V(-5200, -1500, 1100), V(0, -800, 1500)}, O); s.poly({V(0, 1900, 1000), V(5200, -1500, 1100), V(0, -800, 1500)}, O2);
      d1k::kit_bar(s, V(0, 1880, 990), V(0, -800, 1480), 25, 25, T); s.box(V(-5100, -600, 1180), V(5100, -560, 1220), T);
      s.box(V(-700, 480, -340), V(700, 520, -300), T);
      d1k::kit_bar(s, V(-680, 500, -320), V(-30, 80, 1320), 22, 22, T); d1k::kit_bar(s, V(680, 500, -320), V(30, 80, 1320), 22, 22, T);
      s.add(GM_SELF); }
    { Mesh g;   /* the seagull: flat white shapes */
      g.poly({V(0, 520, 0), V(-130, 0, 50), V(0, -460, 0), V(130, 0, 50)}, 0xF0F0F0);
      g.poly({V(0, 520, -20), V(130, 0, 30), V(0, -460, -20), V(-130, 0, 30)}, 0xB8B8C8);
      g.poly({V(0, 620, 0), V(-60, 440, 30), V(60, 440, 30)}, 0xE0A030);
      g.add(GM_GULL); }
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; Mesh w;
        two_sided(w, V(0, 250, 0), V(q * 1500, -100, 60), V(q * 1400, -450, 50), V(0, -300, 0), 0xF2F2F2, 0xBBBBCC);
        two_sided3(w, V(q * 1500, -100, 60), V(q * 2100, -480, 70), V(q * 1400, -450, 50), 0xE8E8E8, 0xC0C0CC);   /* all white, bent wings (t1248) */
        w.add(k ? GM_WR : GM_WL); }
    { Mesh p; using d1k::kit_bar;   /* an electricity pylon: thin pale lattice, spread feet, cross-arms (t1264, t1292) */
      const uint32_t c1 = 0xA8A8A6; const float Z0 = 5000.f, Z1 = 20000.f, B = 2800.f, W0 = 800.f, W1 = 300.f, TH = 45.f;
      auto wd = [&](float z) { return W0 + (W1 - W0) * (z - Z0) / (Z1 - Z0); };
      for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) { float sx = a ? 1.f : -1.f, sy = b ? 1.f : -1.f; kit_bar(p, V(sx * B, sy * B, 0), V(sx * W0, sy * W0, Z0), TH, TH, c1); kit_bar(p, V(sx * W0, sy * W0, Z0), V(sx * W1, sy * W1, Z1), TH, TH, c1); }
      for (int k = 0; k < 5; k++) { float z0 = Z0 + k * 3000.f, z1 = z0 + 3000.f, w = wd(z0), w1 = wd(z1); kit_bar(p, V(-w, -w, z0), V(w, -w, z0), TH, TH, c1); kit_bar(p, V(-w, w, z0), V(w, w, z0), TH, TH, c1); kit_bar(p, V(-w, -w, z0), V(-w, w, z0), TH, TH, c1); kit_bar(p, V(w, -w, z0), V(w, w, z0), TH, TH, c1);
          kit_bar(p, V((k & 1) ? -w : w, -w, z0), V((k & 1) ? w1 : -w1, -w1, z1), TH, TH, c1); kit_bar(p, V((k & 1) ? w : -w, w, z0), V((k & 1) ? -w1 : w1, w1, z1), TH, TH, c1); }
      const float AZ[4] = {WIRE_Z[1], WIRE_Z[0], 17500.f, 19000.f}; for (int h = 0; h < 4; h++) kit_bar(p, V(-1800, 0, AZ[h]), V(1800, 0, AZ[h]), TH, TH, c1);
      p.add(GM_PYLON); }
    { Mesh w;   /* one span of wire, three straight pieces with the sag in them, a slanted ribbon seen from both sides */
      const int N = 3; const V d(0, 80, 80);
      for (int i = 0; i < N; i++) { float u0 = (float)i / N, u1 = (float)(i + 1) / N;
          V a(u0 * PY_SPAN, 0, -WIRE_SAG * 4.f * u0 * (1.f - u0)), b(u1 * PY_SPAN, 0, -WIRE_SAG * 4.f * u1 * (1.f - u1));
          d1k::kit_bar(w, a, b, 60, 60, 0x2C2A2C); }
      w.add(GM_WIRE); }
    { Mesh o; g_delta(o, 0xD05030, 3, 5); o.add(GM_DELTA); }
    { Mesh o; g_delta(o, 0xE0DED8, 2, 6); o.add(GM_DELTA2); }
    { Mesh m;   /* ground, ridge, sea, hills, the striped drum, avenue, plaza and pad */
      std::mt19937 r2(31); auto rf = [&r2](float a, float b) { return a + (b - a) * (r2() / (float)(r2.max)()); };
      const uint32_t GN = 0x485C32, G2 = 0x3E5630, G3 = 0x4C6238;
      m.poly({V(-300000, -50000, 0), V(300000, -50000, 0), V(300000, 620000, 0), V(-300000, 620000, 0)}, GN);
      m.poly({V(-300000, 70000, 0), V(300000, 70000, 0), V(300000, 88000, 8000), V(-300000, 88000, 8000)}, G3); m.poly({V(-300000, 88000, 8000), V(300000, 88000, 8000), V(300000, 106000, 0), V(-300000, 106000, 0)}, G2);
      m.poly({V(-300000, 104000, 8), V(-6000, 104000, 8), V(-12000, 170000, 8), V(-22000, 262000, 8), V(-300000, 262000, 8)}, 0x2D4A8C);
      for (int i = 0; i < 40; i++) { bool left = i >= 30; float hy = left ? rf(270000.f, 560000.f) : rf(120000.f, 560000.f), hx = left ? rf(-260000.f, -40000.f) : rf(30000.f, 240000.f); if (hy > 340000.f) hx += hx > 0 ? 100000.f : -100000.f;
          float rr = rf(12000.f, 30000.f), hh = rf(2000.f, 6000.f); uint32_t gc = (i & 1) ? G2 : G3, gc2 = (i & 1) ? 0x344A2A : 0x405430;
          V c0(hx + rr, hy - rr, 0), c1(hx + rr, hy + rr, 0), c2(hx - rr, hy + rr, 0), c3(hx - rr, hy - rr, 0), ap(hx, hy, hh);
          m.poly({c0, c1, ap}, gc); m.poly({c1, c2, ap}, gc2); m.poly({c2, c3, ap}, gc); m.poly({c3, c0, ap}, gc2); }
      /* lollipop trees: a faceted olive crown on a red-brown trunk (t1222, t1226) */
      auto tree = [&](float x, float y, float q) { m.box(V(x - 450 * q, y - 450 * q, 0), V(x + 450 * q, y + 450 * q, 8500 * q), 0x8C3828); const float rd = 4300.f * q, z0 = 7500.f * q, z1 = 11000.f * q, z2 = 15000.f * q; std::vector<V> a, b, c; for (int i = 0; i < 6; i++) { float t = 6.2831853f * i / 6 + x; a.push_back(V(x + cosf(t) * rd * .6f, y + sinf(t) * rd * .6f, z0)); b.push_back(V(x + cosf(t + .5f) * rd, y + sinf(t + .5f) * rd, z1)); c.push_back(V(x + cosf(t) * rd * .55f, y + sinf(t) * rd * .55f, z2)); }
          for (int i = 0; i < 6; i++) { int j = (i + 1) % 6; uint32_t k1 = (i & 1) ? 0x64683A : 0x565C34, k2 = (i & 1) ? 0x565A30 : 0x4A5030; m.poly({a[i], a[j], b[j], b[i]}, k2); m.poly({b[i], b[j], c[j], c[i]}, k1); } m.poly({c[0], c[1], c[2], c[3], c[4], c[5]}, 0x6C7040); m.poly({a[0], a[1], a[2], a[3], a[4], a[5]}, 0x3C4228); };
      for (int i = 0; i < 26; i++) { float sd = (i & 1) ? 1.f : -1.f; tree(sd * rf(15000.f, 70000.f), rf(14000.f, 68000.f), rf(.8f, 1.1f)); }
      /* the great striped drum near the coast: red-brown with broad orange and white stripes (t1234) */
      { const V C(33000, 112000, 0); const float Rd = 12000.f, Hg = 20000.f; const int N = 18; std::vector<V> top; for (int i = 0; i < N; i++) { float a0 = 6.2831853f * i / N, a1 = 6.2831853f * (i + 1) / N; int d = abs(((i - 13 + N + N / 2) % N) - N / 2); uint32_t c = d == 0 ? 0xE6E2DA : d == 1 ? 0xD08A40 : d == 2 ? 0xA05030 : 0x8C3C28;
            V p0(C.x + cosf(a0) * Rd, C.y + sinf(a0) * Rd, 0), p1(C.x + cosf(a1) * Rd, C.y + sinf(a1) * Rd, 0); m.poly({p0, p1, p1 + V(0, 0, Hg), p0 + V(0, 0, Hg)}, c); m.poly({V(C.x, C.y, Hg), p0 + V(0, 0, Hg), p1 + V(0, 0, Hg)}, 0x70301E); } }
      m.poly({V(-90000, 360000, 3), V(90000, 360000, 3), V(90000, 566000, 3), V(-90000, 566000, 3)}, 0x6C6860);   /* the streets, taupe */
      m.poly({V(-14500, 360000, 5), V(14500, 360000, 5), V(14500, 470000, 5), V(-14500, 470000, 5)}, 0x62665A);    /* the avenue */
      m.poly({V(-14500, 470000, 6), V(14500, 470000, 6), V(14500, 562000, 6), V(-14500, 562000, 6)}, G2);          /* the grass plaza, edged pale */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; m.poly({V(q * 14500, 360000, 8), V(q * 15300, 360000, 8), V(q * 15300, 562000, 8), V(q * 14500, 562000, 8)}, 0xA8A49A); }
      for (int i = 0; i < 10; i++) { float y = 474000.f + i * 9500.f; tree(-17000.f, y, .36f); tree(17000.f, y, .36f); }
      /* the pad: pale border, red-orange, an orange-yellow lip, HERO painted on it (t1298, t1300) */
      m.poly({V(-12000, 528000, 8), V(12000, 528000, 8), V(12000, 552000, 8), V(-12000, 552000, 8)}, 0xA0A098);
      m.poly({V(-10500, 530000, 10), V(10500, 530000, 10), V(10500, 550000, 10), V(-10500, 550000, 10)}, 0xC84828);
      m.poly({V(-10500, 530000, 11), V(10500, 530000, 11), V(10500, 531000, 11), V(-10500, 531000, 11)}, 0xE0A040);
      { VPIX_MeshPoly q; q.flags = 0x20FFFFFF; q.tex = TEX_HPLATE; q.n = 4; V cn[4] = {V(-9500, 536000, 13), V(9500, 536000, 13), V(9500, 544000, 13), V(-9500, 544000, 13)}; const float uu[4] = {0, 1, 1, 0}, vv[4] = {1, 1, 0, 0}; for (int c = 0; c < 4; c++) { q.idx[c] = m.vert(cn[c]); q.uv[c][0] = uu[c]; q.uv[c][1] = vv[c]; } q.nx = 0; q.ny = 0; q.nz = 1; m.p.push_back(q); }
      m.add(GM_LAND); }
    { Mesh dd; dd.poly({V(0, 0, 120), V(-90, 0, 0), V(90, 0, 0)}, 0xFFFFFF); dd.poly({V(0, 90, 0), V(0, -90, 0), V(0, 0, 120)}, 0xE8E8E8); dd.add(GM_DROP); }
    { Mesh cm[4]; std::mt19937 r3(77); auto rf = [&r3](float a, float b) { return a + (b - a) * (r3() / (float)(r3.max)()); };
      const uint32_t cols[6] = {0xBE5532, 0x967838, 0xB4AFAF, 0x46415A, 0xBE5532, 0x46415A};   /* very big plain blocks, the eye below their roofs (t1278, t1288, t1294, t1296) */
      static const float XC[3] = {25500.f, 52000.f, 80000.f}, XH[3] = {10000.f, 12000.f, 12000.f};
      for (int j = 0; j < 6; j++) for (int k = 0; k < 2; k++) for (int i = 0; i < 3; i++) {
          float x = (k ? 1.f : -1.f) * XC[i], y = 376000.f + j * 32000.f, hx = XH[i], hy = 13000.f, h = rf(14000.f, 30000.f); uint32_t c = cols[r3() % 6];
          cm[j * 4 / 6].box(V(x - hx, y - hy, 0), V(x + hx, y + hy, h), c + 0x101010, c, c); }
      for (int k = 0; k < 4; k++) cm[k].add(GM_CITY + k); }
    { Mesh s; s.poly({V(-40, 0, -24), V(40, 0, -24), V(0, 0, 90)}, 0xFFE040); s.add(HM_SPARK); }
    { Mesh v;   /* the 18-wheel van the programmers hid in the city (Simon Marston, hearsay, not in any frame): trailer, cab, six axles under the body */
      const uint32_t T = 0xD8D6CC, T2 = 0xA8A69C, C = 0x8C3828, C2 = 0x6E2C20, W = 0x18181C;
      v.box(V(-1250, -6500, 1100), V(1250, 4000, 4100), T, T2, T2); v.box(V(-1200, 4300, 900), V(1200, 7000, 3400), C, C2, C2); v.box(V(-500, 3900, 1000), V(500, 4400, 1500), 0x303034);
      v.poly({V(-1000, 7005, 2200), V(1000, 7005, 2200), V(1000, 7005, 3200), V(-1000, 7005, 3200)}, 0x2C3C50);
      const float AY[6] = {-5600, -4300, -3000, 2800, 4900, 6200}; for (int i = 0; i < 6; i++) for (int k = 0; k < 2; k++) { float x0 = k ? 800.f : -1300.f; v.box(V(x0, AY[i] - 500, 0), V(x0 + 500, AY[i] + 500, 1000), W); }
      v.add(GM_VAN); }
    for (int lit = 0; lit < 2; lit++) { Mesh c;   /* the police car: its roof bar flips between red and blue */
      const uint32_t B = 0x24304C, B2 = 0x1A2238, W = 0xE0E0DC, Wh = 0x18181C;
      c.box(V(-950, -2300, 450), V(950, 2300, 1150), B, B2, B2); c.box(V(-850, -1100, 1150), V(850, 800, 1750), W, 0xA8A8A4, 0xA8A8A4);
      c.box(V(-700, -250, 1750), V(0, 150, 2000), lit ? 0xF02020 : 0x501010); c.box(V(0, -250, 1750), V(700, 150, 2000), lit ? 0x102050 : 0x2040F0);
      const float AY[2] = {-1500, 1500}; for (int i = 0; i < 2; i++) for (int k = 0; k < 2; k++) { float x0 = k ? 550.f : -1000.f; c.box(V(x0, AY[i] - 450, 0), V(x0 + 450, AY[i] + 450, 800), Wh); }
      c.add(lit ? GM_COPLIT : GM_COP); }
}

/* ---- HUD: only the title screens, the timer and the letters of the puzzle (no HUD is visible in the glider footage) ---- */
static void clock_draw(VPIX_Frame &f, float secs, float cx, float y) {
    int s = (int)ceilf((std::max)(secs, 0.f)), m = s / 60, r = s % 60;
    blit(f, d1k::BM_DIG + m, d1k::DW, d1k::DH, cx, y); blit(f, d1k::BM_DIG + 40, 6, 26, cx + 18, y);
    blit(f, d1k::BM_DIG + r / 10, d1k::DW, d1k::DH, cx + 26, y); blit(f, d1k::BM_DIG + r % 10, d1k::DW, d1k::DH, cx + 44, y);
}

/* ---- frames ---- */
static void put_flat_hand(VPIX_Frame &f, const VPIX_Input &in, V &hp, M &hr, bool &ok) {
    ok = in.hand.valid;
    if (ok) { for (int k = 0; k < 3; k++) hr.r[k] = V(in.hand.R[k][0], in.hand.R[k][1], in.hand.R[k][2]); hp = V(walk_off.x + in.hand.p[0], walk_off.y + in.hand.p[1], in.hand.p[2]); }
    (void)f;
}
static void net_draw(VPIX_Frame &f);
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = f.clear[1] = f.clear[2] = 0;
    f.n_lights = 1;
    M cam; V eye; bool head_cam = false;
    static const char *fixed = getenv("HERO_CAM");
    bool play = st != S_TITLE;
    if (!glider_mode) {
        f.lights[0] = {0.50f, 0.60f, {0.6f, -0.3f, 0.74f}};   /* flat per face, no pools of light: t1625 has the left wall twice as bright as the right (0x51 against 0x27) */
        const int rm = room; const float s = shrink_s();
        dput(f, HM_SHELL + rm, M(), V(0, 0, 0));
        if (rm == R_HUB) {
            f.clear[0] = 22; f.clear[1] = 24; f.clear[2] = 44;   /* INVENTED: the article does not say what is round the platform */
            for (int k = 0; k < 4; k++) { V d = door_pos(k); float a = face_yaw(d, V(0, 0, 0)) + 3.14159f; put_at(f, HM_AIRLOCK, d, a); put_at(f, got[k] ? HM_SDONE : HM_SENTER, d + V(0, 0, 2920), a).bright = 1; }
        } else if (rm == R_RIVER) {
            f.lights[0] = {0.62f, 0.45f, {0.5f, -0.5f, 0.7f}}; f.clear[0] = 96; f.clear[1] = 140; f.clear[2] = 226;   /* a blue sky (article) */
            put_at(f, HM_RAFT, V(0, raft_y, RAFT_TOP));
            put_at(f, HM_TREE, V(TREE_X, TREE_Y, RAFT_TOP));
            put_at(f, HM_LEAF, V(TREE_X, TREE_Y, RAFT_TOP)).glass = 150;
            if (rv_st == 2 || rv_st == 5) put_at(f, HM_CHUTE, V(pp.x, pp.y, pz));
        } else if (rm == R_MOUSE) {
            f.lights[0] = {0.62f, 0.45f, {0.4f, -0.4f, 0.82f}};
            if (km >= 2 && wk_x < 2300.f) { const float st_l = 350.f, y0 = -650.f; int i = (int)floorf(wk_x / st_l); float u = wk_x / st_l - i;   /* one foot planted, the other swings past it */
                put_at(f, HM_LEG, K(i * st_l, y0 + ((i & 1) ? 80.f : -80.f)));
                put_at(f, HM_LEG, K((i - 1 + 2.f * u) * st_l, y0 + ((i & 1) ? -80.f : 80.f), sinf(3.14159f * u) * 120.f)); }
            if (km_stomp >= 1 && km_stomp < 4) { float z = km_stomp == 1 ? (1.f - stomp_t / 0.25f) * 700.f : km_stomp == 3 ? stomp_t / 0.4f * 900.f : 0.f; put_at(f, HM_LEG, K(stomp_x, stomp_y, z), stomp_a); }
            if (km_mouse >= 1 && km_cheese != 9) { VPIX_Instance &mi = put_at(f, HM_MOUSE, kmouse_p, km_mouse == 2 ? face_yaw(kmouse_p, kmouse_p + V(1, 0, 0)) : km_mouse == 1 ? face_yaw(kmouse_p, K(-120, -170)) : face_yaw(kmouse_p, K(CH_X, CH_Y))); for (int q = 0; q < 3; q++) mi.S[q] = 4.f; }
            if (km_cheese == 0) { VPIX_Instance &ci = put_at(f, HM_CHEESE, K(CH_X, CH_Y)); for (int q = 0; q < 3; q++) ci.S[q] = 7.f; }
        } else if (rm == R_DOG) {
            bool deep = pp.x < -3200.f; f.fog_start = deep ? 900.f : 7000.f; f.fog_end = deep ? 7500.f : 17000.f;   /* the tunnel's darkness (article); the room stays plainly lit */
            if (d_st < 3) put_at(f, HM_DOG, dog_p, dog_yaw); else put_at(f, HM_DOGZ, dog_p, dog_yaw);
            if (d_st < 2) { V a(ANCHOR_P.x, ANCHOR_P.y, ANCHOR_P.z), b(dog_p.x, dog_p.y + 900.f * cosf(dog_yaw), 850.f); b.x += -900.f * sinf(dog_yaw); for (int i = 0; i < 7; i++) put_at(f, HM_LINK, a + (b - a) * ((float)i / 6.f)); }
            if (d_st == 0) put_at(f, HM_KEY, HOOK_P, 1.57f).bright = 1;
        } else {
            if (door_t < 1.f) put_at(f, HM_DOOR, V(0, 23500, -door_t * 3000.f));
            V fg(fig_x(), fig_y, 0);
            put_at(f, HM_FIG, fg, fig_dir > 0 ? 0.f : 3.14159f);
            if (key_st == 0) put_at(f, HM_KEY, fg + V(290, 200, 950), sim_t * 0.8).bright = 1;
        }
        if (letter_vis && !got[rm]) {
            VPIX_Instance &li = put_at(f, HM_LET + rm, V(letter_p.x, letter_p.y, letter_p.z - 500.f + (rm == R_RIVER ? 0.f : sinf((float)sim_t * 2.f) * 100.f)), face_yaw(letter_p, pp) + 3.14159f);
            li.bright = 1;
        }
        /* camera */
        if (fixed && *fixed) { float a[6] = {0, 600, 1700, 0, 20000, 1500}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5])); }
        else if (!play) { eye = V(0, -2000.f, 1700.f); float an = (float)sim_t * 0.25f; cam = look_at(eye, eye + V(sinf(an) * 1000.f, cosf(an) * 1000.f, -80.f)); }   /* attract: looking round the platform */
        else if (autoplay || !in.head.valid) {
            eye = pp + V(0, 0, pz + (jump_z + 1700.f) * s); float pr = pitch * 0.0174533f; cam = look_at(eye, eye + V(-sinf(yaw) * cosf(pr), cosf(yaw) * cosf(pr), sinf(pr)) * 1000.f);
        } else {
            for (int k = 0; k < 3; k++) cam.r[k] = V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2]);
            eye = V(walk_off.x + in.head.p[0], walk_off.y + in.head.p[1], pz + (jump_z + in.head.p[2]) * s);
            eye.z = (std::max)(eye.z, pz + 300.f * s);   /* a low or badly zeroed headset never goes under the ground */
            if (rm == R_RIVER && (rv_st == 1 || rv_st == 2)) eye.y = (std::max)(eye.y, 500.f);   /* nor leans back into the cliff on the way down */
            head_cam = true;
        }
        /* the orange arm, the carried things */
        if (play && (autoplay || (fixed && *fixed) || !in.vr)) { for (int k = 0; k < 2; k++) {   /* both arms come in from the bottom corners and turn in, so the hands show side on */
            const float ta = k ? -0.35f : 0.35f, tc = cosf(ta), tsn = sinf(ta); M ar = cam; ar.r[1] = cam.r[1] * tc + cam.r[0] * (-tsn); ar.r[0] = cam.r[0] * tc + cam.r[1] * tsn;
            V hp = eye + cam.r[1] * (k ? 1300.f : 1500.f + (poke_t > 0.f ? 300.f : 0.f)) + cam.r[0] * (k ? -360.f : 340.f) - cam.r[2] * 440.f; VPIX_Instance &ai = dput(f, k ? (fist_l ? HM_ARM2_F : HM_ARM2) : (fist_r ? HM_ARM_F : HM_ARM), ar, hp); ai.on_top = 2; ai.bright = 1; } }
        const bool has_key = (rm == R_HALL && key_st == 1) || (rm == R_DOG && d_st == 1);
        if (play && !autoplay && in.vr && in.head.valid) {
            V hp; M hr; bool ok; put_flat_hand(f, in, hp, hr, ok);
            if (ok) {
                hp.z = pz + (jump_z + in.hand.p[2]) * s;
                VPIX_Instance &ai = dput(f, fist_r ? HM_ARM_F : HM_ARM, hr, hp); ai.on_top = 2; ai.bright = 1;
                if (in.hand2.valid) { M h2; for (int k = 0; k < 3; k++) h2.r[k] = V(in.hand2.R[k][0], in.hand2.R[k][1], in.hand2.R[k][2]); VPIX_Instance &a2 = dput(f, fist_l ? HM_ARM2_F : HM_ARM2, h2, V(walk_off.x + in.hand2.p[0], walk_off.y + in.hand2.p[1], pz + jump_z * s + in.hand2.p[2] * s)); a2.on_top = 2; a2.bright = 1; }
                if (has_key) { VPIX_Instance &ki = dput(f, HM_KEY, hr, hp + hr.r[1] * 250.f); ki.bright = 1; ki.on_top = 2; }
            }
        } else if (play && (autoplay || !in.vr) && has_key) put_at(f, HM_KEY, pp + V(-sinf(yaw) * 500.f, cosf(yaw) * 500.f, 1100.f + jump_z), yaw);
        if (play && rm == R_MOUSE && (km_cheese == 1 || km_cheese == 2)) {   /* the cheese in your hand; eating, it gets closer (article) */
            float u = km_cheese == 2 ? clampf(eat_t, 0.f, 1.f) : 0.f; V cp = eye + cam.r[1] * (900.f - 650.f * u) - cam.r[2] * (420.f * (1.f - u));
            VPIX_Instance &ci = dput(f, HM_CHEESE, cam, cp); ci.on_top = 2; for (int q = 0; q < 3; q++) ci.S[q] = 1.f;
        }
    } else {
        f.lights[0] = {0.62f, 0.38f, {-0.3f, -0.4f, 0.86f}};
        float ay = play ? gpy : fmodf((float)sim_t * glide_speed, 150000.f);
        { float fz = clampf((ay - 150000.f) / 40000.f, 0.f, 1.f) * clampf((365000.f - ay) / 30000.f, 0.f, 1.f);   /* the pylon stretch is in thick grey fog (t1264, t1292) */
          const uint8_t sk[3] = {(uint8_t)(128.f - 36.f * fz), (uint8_t)(132.f - 44.f * fz), (uint8_t)(190.f - 92.f * fz)};
          for (int k = 0; k < 3; k++) f.clear[k] = f.fog_col[k] = sk[k];
          f.fog_start = 90000.f - 84000.f * fz; f.fog_end = 210000.f - 165000.f * fz; }
        { M I; dput(f, GM_LAND, I, V(0, 0, 0)); }
        for (int k = 0; k < 4; k++) { float cy = 360000.f + 24000.f + k * 48000.f + 24000.f; if (cy - ay > -60000.f && cy - ay < 190000.f) { M I; dput(f, GM_CITY + k, I, V(0, 0, 0)); } }
        for (int r = 0; r < N_ROWS; r++) { float d = row_y(r) - ay; if (d < -6000.f || d > 110000.f) continue;
            for (int j = -3; j <= 3; j++) put_at(f, GM_PYLON, V(j * PY_SPAN, row_y(r), 0));
            for (int s = 1; s <= 6; s++) { if (s == row_gap[r]) continue;
                for (int h = 0; h < 2; h++) put_at(f, GM_WIRE, V((s - 4) * PY_SPAN, row_y(r), WIRE_Z[h])); }
            for (int s = 1; s <= 6; s++) { put_at(f, GM_WIRE, V((s - 4) * PY_SPAN, row_y(r), 17500.f)); put_at(f, GM_WIRE, V((s - 4) * PY_SPAN, row_y(r), 19000.f)); } }
        for (int i = 0; i < N_GULLS; i++) { const Gull &g = gulls[i]; if (g.p.y - ay < -8000.f || g.p.y - ay > 150000.f) continue;
            M R = yawed(atan2f(-g.v.x, g.v.y)); float fa = sinf((float)sim_t * 5.f + i) * 0.5f, c = cosf(fa), s = sinf(fa);
            M wl = R, wr = R; wr.r[0] = R.r[0] * c + R.r[2] * s; wr.r[2] = R.r[2] * c - R.r[0] * s; wl.r[0] = R.r[0] * c - R.r[2] * s; wl.r[2] = R.r[2] * c + R.r[0] * s;
            dput(f, GM_GULL, R, g.p); dput(f, GM_WL, wl, g.p); dput(f, GM_WR, wr, g.p); }
        { float t = (float)sim_t; V o(ay > 0 ? (play ? gpx : 0.f) + 4000.f + 9000.f * cosf(t * 0.15f) : 4000.f, ay + 24000.f + 8000.f * sinf(t * 0.1f), zprofile(ay) + 2500.f + 1500.f * sinf(t * 0.13f));
          if (o.y < 430000.f) put_at(f, GM_DELTA, o, 0.f);
          if (ay > 90000.f && ay < 200000.f) put_at(f, GM_DELTA2, V(-7000.f + 5000.f * sinf(t * 0.2f), ay + 15000.f, zprofile(ay) + 500.f), 0.f); }
        { const float cy = 366000.f + fmodf((float)sim_t * 9000.f, 96000.f); if (cy - ay < 190000.f && cy - ay > -40000.f) { put_at(f, GM_VAN, V(-6000.f, cy, 8)); if (cy > 384000.f) put_at(f, fmodf((float)sim_t, 0.5f) < 0.25f ? GM_COPLIT : GM_COP, V(-6000.f, cy - 18000.f, 8)); } }   /* the chase in the avenue */
        for (const Drop &d : drops) put_at(f, GM_DROP, d.p);
        if (play && zap_t > 0.f) { f.clear[0] = f.clear[1] = f.clear[2] = (fmodf((float)sim_t, 0.12f) < 0.06f && zap_t > 1.0f) ? 255 : f.clear[0]; }
        if (fixed && *fixed) { float a[6] = {0, -20000, 22000, 0, 0, 20000}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5])); }
        else if (!play) { eye = V(0, ay, zprofile(ay)); cam = look_at(eye, eye + V(sinf((float)sim_t * 0.2f) * 3000.f, 12000.f, -1500.f)); }
        else {
            eye = V(gpx, gpy, gpz);
            { M I; dput(f, GM_SELF, I, eye).on_top = 2; }
            if (in.head.valid) {
                for (int k = 0; k < 3; k++) cam.r[k] = V(in.head.R[k][0], in.head.R[k][1], in.head.R[k][2]);
                if (in.vr && have_ref) { V dh = V(in.head.p[0], in.head.p[1], in.head.p[2]) - head_ref; if (len(dh) > 900.f) dh = norm(dh) * 900.f; eye += dh; }   /* lean in the seat, as Buggy Ball */
                head_cam = !autoplay;
            } else cam = look_at(eye, eye + V(0, 10000, -400));
            if (autoplay) { cam = look_at(eye, eye + V(0, 10000, -400));
                static const char *lk = getenv("HERO_LOOK"); if (lk && *lk) { float ya = 0, pi = 0, ro = 0; sscanf(lk, "%f,%f,%f", &ya, &pi, &ro); ya *= 0.0174533f; pi *= 0.0174533f; ro *= 0.0174533f;
                    V fw = cam.r[1], ri = cam.r[0], up = cam.r[2]; V f2 = fw * cosf(ya) + ri * sinf(ya), r2 = ri * cosf(ya) - fw * sinf(ya); V f3 = f2 * cosf(pi) + up * sinf(pi), u3 = up * cosf(pi) - f2 * sinf(pi); cam.r[0] = r2 * cosf(ro) + u3 * sinf(ro); cam.r[1] = f3; cam.r[2] = u3 * cosf(ro) - r2 * sinf(ro); } }
            if (zap_t > 0.f) { M I; for (int i = 0; i < 6; i++) dput(f, HM_SPARK, I, eye + cam.r[1] * 700.f + cam.r[0] * rnd(-400, 400) + cam.r[2] * rnd(-300, 300)).bright = 1; }
        }
    }
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    net_draw(f);
    f.in_head = head_cam; if (head_cam) f.head_fed = in.head;

    /* ---- 2D ---- */
    const float CX = 176.f;
    if (st == S_TITLE && glider_mode) {
        g_blit(f, BM_HEND, HG_EW, HG_EH, 0, 0, 352.f, 288.f);
        if (fmodf((float)sim_t, 1.2f) < 0.8f) text(f, "PRESS TRIGGER", CX, 20, 1.f);
    } else if (st == S_TITLE) {
        text(f, "HERO", CX, 40, 1.8f, 1);
        text(f, "THE HERO CHALLENGE", CX, 90, 0.8f);
        
        if (fmodf((float)sim_t, 1.2f) < 0.8f) text(f, "PRESS TRIGGER", CX, 200, 1.f);
    } else if (!glider_mode) {
        if (time_left < 30.f || st == S_OVER) clock_draw(f, st == S_OVER ? 0.f : time_left, 136.f, 10.f);   /* INVENTED: nothing shows a clock; only the last half minute */
        if (room == R_HUB) { char lt[8] = {got[0] ? 'H' : ' ', ' ', got[1] ? 'E' : ' ', ' ', got[2] ? 'R' : ' ', ' ', got[3] ? 'O' : ' ', 0}; text(f, lt, 300.f, 14.f, 1.f, 1); }   /* INVENTED: only H is in the article, and no letter display */
        if (operator_on && say_t > 0.f && st == S_PLAY) text(f, say_s, CX, 40.f, 0.9f, 1);   /* the operator's coaching, written up because a rebuild has no operator */
        if (st == S_OVER) {
            text(f, won ? "HERO" : "TIME UP", CX, 96, 1.8f, won ? 1 : 2);
            text(f, won ? "YOU WIN" : "TRY AGAIN", CX, 140, 1.f);
        }
    } else {
        if (splat_t > 0.f) { blit(f, BM_SPLAT, 64, 48, 70, 70, 2.6f); blit(f, BM_SPLAT, 64, 48, 200, 120, 1.6f); }   /* INVENTED: the droppings land on the screen */
        if (st == S_OVER) g_blit(f, BM_HEND, HG_EW, HG_EH, 0, 0, 352.f, 288.f);   /* HERO EXPERIENCE, t1302 */
    }
    if (!glider_mode && blue_t >= 0.f && st == S_PLAY) { VPIX_Blit b = {}; b.handle = d1k::BMPH + BM_BLUE; b.sw = 8; b.sh = 8; b.x = -24; b.y = -24; b.dw = 400.f; b.dh = 336.f; b.key = -1; b.alpha = clampf(blue_t / 0.4f, 0.f, 1.f); b.prio = 1000000; b.tint[0] = b.tint[1] = b.tint[2] = 1.f; f.blits.push_back(b); }
    d1k::look_test(f);
    d1k::hs::draw(f, glider_mode ? "heroglide" : "hero", st == S_TITLE && (!glider_mode || glide_score), (float)sim_t);   /* no board in the glider attract: it has no score */
    VPIX_Render_Submit(f);
}

/* ---- loop ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 1; static unsigned long frames = 0;
static void start_round() {
    st = S_PLAY; st_t = 0; sfx(0, SK_GO, 100);
    if (v1knet::on) v1knet::begin_round(v1knet::SEATS);   /* linked: pods that are here take seats 1-3 (no CPU players in this game) */
    if (glider_mode) { reset_flight(); dl("t=%.1f glider flight start", sim_t); }
    else { reset_game(); dl("t=%.1f puzzle round start, %.0f s", sim_t, game_len); }
}

/* ---- link play (vpix_1k_net.h) --------------------------------------------------------------------------------------------------------------------------
   INVENTED (the original is one player): up to four pods share ONE world. Puzzle: one room, one clock, one set of letters for everybody; each pod walks its own figure (a person, a mouse in the kitchen), any pod can walk into a door, take the key,
   unlock the chain or the red door, grab the letter, eat the cheese; the raft/chute/kitchen-shrink sequences belong to pod 1 and the others ride along with it. Pod 1 alone can be stepped on, miss the raft or be caught by the other mouse.
   Glider: everybody flies the same sky (same seagulls, from pod 1), each on its own flight with its own deaths and droppings; the others show as the other orange glider. A start-together game; pod 1 starts it, a pod that is not seated waits.
   Pod 1 runs the puzzle and sends the room at 30 Hz; the others send their position and a use counter. Operator coaching text is pod 1's only. */
#pragma pack(push, 1)
struct NetIn { float x, y, z, yaw; uint8_t useq, zap; };
struct NetSeat { float x, y, z, yaw; uint8_t live; };
struct NetSt {
    uint8_t st, room, enter_n, rv_st, km, km_cheese, km_mouse, km_stomp, d_st, d_tun, key_st, got, letter_vis, won, glider, pad;
    float time_left, pz, raft_y, km_t, wk_x, stomp_t, stomp_x, stomp_y, stomp_a, eat_t, fig_y, fig_dir, door_t, blue_t, blue_len;
    float kmx, kmy, dogx, dogy, dogyaw; uint8_t blue_to, pad2[3];
    NetSeat seat[4];
    float gull[12][6];
};
#pragma pack(pop)
static NetSeat nseat[4]; static bool sg = false;   /* nseat: every pod's figure as pod 1 last told it; sg: this slave is in pod 1's game */
static uint8_t left_round = 0, last_enter = 0, my_useq = 0, seat_useq[4]; static bool seat_seen[4];
static V mpp; static float mpz = 0;                   /* pod 1's own spot, which a slave follows while pod 1 falls, floats or shrinks */

static void sync_river_area() {   /* a slave's copy of the river's walkable bits, as pod 1's river steps set them */
    area.clear();
    if (rv_st == 0) area.push_back({-19000, -15000, 19000, 900});
    else if (rv_st == 3) { area.push_back({-RAFT_HX, raft_y - RAFT_HY, RAFT_HX, raft_y + RAFT_HY}); if (raft_y >= BANK_Y - RAFT_HY + 700.f) area.push_back({-19000, BANK_Y, 19000, 175000}); }
    else if (rv_st == 4) area.push_back({-19000, BANK_Y, 19000, 175000});
}
static void net_leave(const char *why) { if (!sg && st == S_TITLE) return; sg = false; st = S_TITLE; st_t = 0; left_round = v1knet::round_id; dl("t=%.1f link: %s, back to the title", sim_t, why); }
static void net_fill_seat0() {
    NetSeat &q = nseat[0]; q.live = 1; q.yaw = yaw;
    if (glider_mode) { q.x = gpx; q.y = gpy; q.z = gpz; } else { q.x = pp.x; q.y = pp.y; q.z = pz; }
}
static void net_seat_use(int s) {   /* another pod pressed use at its own spot: run the room's use check from there */
    const V keep = pp; pp = V(nseat[s].x, nseat[s].y, 0);
    if (room == R_DOG) step_dog(true, 0.f);
    else if (room == R_HALL) step_hall(true, 0.f);
    else if (room == R_RIVER) { if (rv_st == 4 && !got[R_RIVER] && dxy(pp, letter_p) < 1900.f) p_collect(); }
    else if (room == R_MOUSE) {
        const V ch = K(CH_X, CH_Y);
        if (km >= 4 && km_cheese == 0 && dxy(pp, ch) < 2600.f) { km_cheese = 1; km = 5; bot_t = 0; sfx(3, SK_USE, 90); say("EAT THE CHEESE"); dl("t=%.1f got the cheese (a linked pod)", sim_t); }
        else if (km_cheese == 1) { km_cheese = 2; eat_t = 0; }
    }
    pp = keep;
}
static void net_master() {
    v1knet::pump();
    net_fill_seat0();
    for (int s = 1; s < v1knet::SEATS; s++) {
        const NetIn *p = (const NetIn *)v1knet::seat_input(s, sizeof(NetIn));
        if (!p || st == S_TITLE) { nseat[s].live = 0; seat_seen[s] = false; continue; }
        const NetIn q = *p;
        nseat[s].x = q.x; nseat[s].y = q.y; nseat[s].z = q.z; nseat[s].yaw = q.yaw; nseat[s].live = 1;
        if (!seat_seen[s]) { seat_seen[s] = true; seat_useq[s] = q.useq; continue; }
        if (glider_mode || st != S_PLAY || blue_t >= 0.f) { seat_useq[s] = q.useq; continue; }
        if (q.useq != seat_useq[s]) { seat_useq[s] = q.useq; net_seat_use(s); }
        if (room == R_HUB) for (int k = 0; k < 4; k++) if (!got[k] && dxy(V(q.x, q.y, 0), door_pos(k)) < 800.f && blue_t < 0.f) { sfx(2, SK_GO, 100); dl("t=%.1f a linked pod opened door %d", sim_t, k); enter_room(k); break; }
    }
    if (!v1knet::due(30)) return;
    if (st == S_TITLE) { v1knet::send_snapshot(NULL, 0); return; }
    static NetSt n; memset(&n, 0, sizeof(n));
    n.st = (uint8_t)st; n.room = (uint8_t)room; n.enter_n = enter_n; n.rv_st = (uint8_t)rv_st; n.km = (uint8_t)km; n.km_cheese = (uint8_t)km_cheese; n.km_mouse = (uint8_t)km_mouse; n.km_stomp = (uint8_t)km_stomp;
    n.d_st = (uint8_t)d_st; n.d_tun = (uint8_t)d_tun; n.key_st = (uint8_t)key_st; n.letter_vis = letter_vis; n.won = won; n.glider = glider_mode;
    for (int k = 0; k < 4; k++) if (got[k]) n.got |= (uint8_t)(1 << k);
    n.time_left = time_left; n.pz = pz; n.raft_y = raft_y; n.km_t = km_t; n.wk_x = wk_x; n.stomp_t = stomp_t; n.stomp_x = stomp_x; n.stomp_y = stomp_y; n.stomp_a = stomp_a; n.eat_t = eat_t;
    n.fig_y = fig_y; n.fig_dir = fig_dir; n.door_t = door_t; n.blue_t = blue_t; n.blue_len = blue_len; n.blue_to = (uint8_t)blue_to;
    n.kmx = kmouse_p.x; n.kmy = kmouse_p.y; n.dogx = dog_p.x; n.dogy = dog_p.y; n.dogyaw = dog_yaw;
    memcpy(n.seat, nseat, sizeof(nseat));
    for (int i = 0; i < N_GULLS; i++) { const Gull &g = gulls[i]; n.gull[i][0] = g.cx; n.gull[i][1] = g.cy; n.gull[i][2] = g.cz; n.gull[i][3] = g.r; n.gull[i][4] = g.w; n.gull[i][5] = g.a; }
    v1knet::send_snapshot(&n, sizeof(n));
}
static void net_apply(const NetSt &n) {
    memcpy(nseat, n.seat, sizeof(nseat));
    if (glider_mode) {
        for (int i = 0; i < N_GULLS; i++) { Gull &g = gulls[i]; g.cx = n.gull[i][0]; g.cy = n.gull[i][1]; g.cz = n.gull[i][2]; g.r = n.gull[i][3]; g.w = n.gull[i][4]; g.a = n.gull[i][5]; gull_move(g, 0.f); }
        return;
    }
    if (n.room > R_HUB) return;
    if (n.room != room || n.enter_n != last_enter) enter_room(n.room);
    last_enter = n.enter_n;
    st = (State)n.st; won = n.won != 0; time_left = n.time_left;
    for (int k = 0; k < 4; k++) got[k] = ((n.got >> k) & 1) != 0;
    rv_st = n.rv_st; km = n.km; km_cheese = n.km_cheese; km_mouse = n.km_mouse; km_stomp = n.km_stomp; d_st = n.d_st; d_tun = n.d_tun > 2 ? 0 : n.d_tun; key_st = n.key_st; letter_vis = n.letter_vis != 0;
    pz = n.pz; raft_y = n.raft_y; km_t = n.km_t; wk_x = n.wk_x; stomp_t = n.stomp_t; stomp_x = n.stomp_x; stomp_y = n.stomp_y; stomp_a = n.stomp_a; eat_t = n.eat_t;
    fig_y = n.fig_y; fig_dir = n.fig_dir; door_t = n.door_t; blue_t = n.blue_t; blue_len = n.blue_len; blue_to = n.blue_to;
    kmouse_p = V(n.kmx, n.kmy, 0); dog_p = V(n.dogx, n.dogy, 0); dog_yaw = n.dogyaw; mpp = V(n.seat[0].x, n.seat[0].y, 0); mpz = n.seat[0].z;
    if (room == R_DOG) letter_p.y = TUN_Y[d_tun];
    if (room == R_RIVER) sync_river_area();
    if (room == R_HALL && !obst.empty()) obst[0].on = door_t < 0.9f;
    if (room == R_MOUSE && obst.size() > 4) { obst[4].on = km_stomp == 2 || km_stomp == 3; obst[4].r = {(stomp_x - 110.f) * KS, (stomp_y - 110.f) * KS, (stomp_x + 110.f) * KS, (stomp_y + 110.f) * KS}; }
}
/* slave, once per tick, before the simulation steps */
static void net_slave_rx() {
    v1knet::pump();
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n < sizeof(NetSt) || v1knet::my_seat < 0) net_leave(n < sizeof(NetSt) ? "pod 1 is at the title" : "no seat in this game");
        else {
            static NetSt s; memcpy(&s, d, sizeof(s));
            if (!sg && v1knet::round_id != left_round && (s.glider != 0) == glider_mode) {
                sg = true; my_useq = 0; mpp = V(0, 0, 0); last_enter = (uint8_t)(s.enter_n - 1);
                st = S_PLAY; st_t = 0; if (glider_mode) reset_flight(); else reset_game();
                dl("t=%.1f link: joined pod 1's game as seat %d", sim_t, v1knet::my_seat);
            }
            if (sg) net_apply(s);
        }
    }
    if (sg && !v1knet::master_heard()) net_leave("no word from pod 1 for 3 s");
}
static void net_slave_step(const VPIX_Input &in, bool &use, float dt) {   /* the puzzle, a slave: walk my own figure; everything else is pod 1's */
    if (say_t > 0) say_t -= dt;
    if (blue_t >= 0.f) return;
    V hxy(in.head.p[0], in.head.p[1], 0);
    V fw = in.head.valid ? norm(V(in.head.R[1][0], in.head.R[1][1], 0)) : V(-sinf(yaw), cosf(yaw), 0);
    const bool follow = (room == R_RIVER && (rv_st == 1 || rv_st == 2 || rv_st == 5)) || (room == R_MOUSE && km < 2);
    cur_speed = room == R_MOUSE ? mouse_walk : walk_speed;
    if (follow) { pp = mpp + V(250.f * v1knet::my_seat, 0, 0); walk_off = V(pp.x, pp.y, 0) - hxy; need_place = false; if (!autoplay) yaw = atan2f(-fw.x, fw.y); return; }
    if (!free_at(pp.x, pp.y)) { pp = mpp; need_place = true; }   /* just left a followed stretch, or the room changed under me: start beside pod 1 */
    if (autoplay) { bool u; puzzle_bot(dt, u); use = use || u; }
    else {
        if (need_place) { walk_off = V(pp.x, pp.y, 0) - hxy; need_place = false; }
        d1k::desk_walk(in, fw, cur_speed, dt, walk_off);
        V raw(walk_off.x + hxy.x, walk_off.y + hxy.y, 0), p = raw;
        slide_to(p, pp); walk_off += p - raw; pp = p; yaw = atan2f(-fw.x, fw.y);
    }
}
static void net_send_input(bool in_game) {
    if (!in_game) { v1knet::send_input(NULL, 0); return; }
    NetIn o; o.x = glider_mode ? gpx : pp.x; o.y = glider_mode ? gpy : pp.y; o.z = glider_mode ? gpz : pz; o.yaw = yaw; o.useq = my_useq; o.zap = zap_t > 0.f;
    v1knet::send_input(&o, sizeof(o));
}
static void net_draw(VPIX_Frame &f) {   /* the other pods' figures */
    if (!v1knet::on || st == S_TITLE || (!v1knet::master && !sg)) return;
    for (int s = 0; s < v1knet::SEATS; s++) {
        if (s == v1knet::my_seat || !v1knet::seat_pod[s] || !nseat[s].live) continue;
        const NetSeat q = nseat[s];
        if (glider_mode) { put_at(f, GM_DELTA2, V(q.x, q.y, q.z), 0.f); continue; }
        if (room == R_MOUSE) { if (km >= 2) { VPIX_Instance &mi = put_at(f, HM_MOUSE, V(q.x, q.y, 0), q.yaw); for (int k = 0; k < 3; k++) mi.S[k] = 4.f; } continue; }
        if (room == R_RIVER && (rv_st == 2 || rv_st == 5)) put_at(f, HM_CHUTE, V(q.x, q.y, q.z));
        put_at(f, HM_FIG, V(q.x, q.y, room == R_RIVER ? q.z : 0.f), q.yaw);
    }
}

static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    /* INVENTED, nothing found for either in the footage or press, Bobby's call 11-03. In a puzzle round: window Space or E jumps, click uses and shuts the hands; VR B jumps, trigger or grip uses and shuts that hand. Space stays select in menus and in the glider */
    const bool sp_jump = !in.vr && st == S_PLAY && !glider_mode && (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0, jump_btn = sp_jump || (st == S_PLAY && !glider_mode && (in.vr ? in.b2 : in.jump));
    static bool trig_was = false; bool trig = (((in.buttons & VPIX_BTN_TRIGGER) != 0) && !sp_jump) || in.throttle > 0.6f || (in.top && st == S_PLAY && !glider_mode), sel = trig && !trig_was; trig_was = trig;
    fist_r = trig; fist_l = in.vr ? in.fire2 : trig;
    const bool ns = v1knet::on && !v1knet::master;   /* a linked slave */
    if (ns) net_slave_rx();
    if (autoplay && st != S_PLAY && st_t > 2.f) sel = true; const bool hs_over = !ns && st == S_OVER && st_t > 2.f && (!glider_mode || glide_score); if (d1k::hs::run(glider_mode ? "heroglide" : "hero", hs_over, glider_mode ? (std::max)(100, 10000 - deaths * 1000 + splats * 100) : ((int)got[0] + got[1] + got[2] + got[3]) * 1000 + (won ? (int)time_left * 10 : 0), in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (glider_mode && st == S_PLAY && !have_ref && in.head.valid && in.vr) { head_ref = V(in.head.p[0], in.head.p[1], in.head.p[2]); have_ref = true; }
    static bool use_pend = false; if (sel && st == S_PLAY) use_pend = true;   /* a press is kept until the next simulation step */
    if (!v1knet::can_start()) sel = false;   /* a linked slave: pod 1 starts the game */
    if (ns && sg && st == S_TITLE) { sg = false; left_round = v1knet::round_id; }
    if (st == S_TITLE && sel) start_round();
    else if (st == S_OVER && ((glider_mode && st_t > 6.f) || (sel && st_t > 3.f))) { st = S_TITLE; st_t = 0; if (!glider_mode) { start_room = R_HUB; enter_room(R_HUB); } v1knet::end_round(); }
    int n = 0;
    while (acc >= dt && n < 24) {
        { const bool ground = !(room == R_RIVER && (rv_st == 1 || rv_st == 2 || rv_st == 5)) && !(room == R_MOUSE && km < 2) && blue_t < 0.f && st == S_PLAY && !glider_mode;   /* the hop, off the floor the room already has */
            if (!ground) { jump_z = 0; jump_v = 0; }
            else if (jump_z <= 0.f && jump_btn && !autoplay) { jump_v = 3100.f; jump_z = 0.1f; dl("t=%.1f jump", sim_t); }
            else if (jump_z > 0.f) { jump_v -= 9800.f * dt; jump_z += jump_v * dt; if (jump_z <= 0.f) { jump_z = 0; sfx(9, SK_BUMP, 40); } } }
        if (st == S_PLAY) { if (glider_mode) g_step(in, dt); else if (ns) { if (sg) { bool u = use_pend; net_slave_step(in, u, dt); if (u) my_useq++; } use_pend = false; } else { p_step(in, use_pend, dt); use_pend = false; } }
        else if (st == S_OVER && glider_mode) { for (int i = 0; i < N_GULLS; i++) gull_move(gulls[i], dt); }
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    if (v1knet::on) { if (ns) net_send_input(sg); else net_master(); }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    if (sim_t >= next_log) { next_log = sim_t + 2.0;
        if (glider_mode) dl("t=%.0f st %d glider (%.0f %.0f %.0f) alt_off %.0f deaths %d droppings %d zap %.1f", sim_t, (int)st, gpx, gpy, gpz, alt_off, deaths, splats, zap_t);
        else dl("t=%.0f st %d room %d me (%.0f %.0f %.0f) letters %d%d%d%d left %.0f river %d kitchen %d dog %d", sim_t, (int)st, room, pp.x, pp.y, pz, (int)got[0], (int)got[1], (int)got[2], (int)got[3], time_left, rv_st, km, d_st);
    }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".hero").c_str(), "w");
    const char *m = getenv("HERO_MODE"); glider_mode = m && !strcmp(m, "glider");
    game_fps = envf("HERO_FPS", game_fps); game_len = envf("HERO_TIME", game_len); walk_speed = envf("HERO_WALK", walk_speed);
    fig_speed = envf("HERO_FIG", fig_speed); mouse_walk = envf("HERO_MOUSE_WALK", mouse_walk); operator_on = envf("HERO_OPERATOR", 1) != 0; glide_score = envf("HERO_GLIDE_SCORE", 0) != 0;
    start_room = (int)clampf(envf("HERO_ROOM", (float)R_HUB), 0.f, (float)R_HUB); bot_miss = (int)envf("HERO_BOT_MISS", 0);
    glide_speed = envf("HERO_SPEED", glide_speed); steer_speed = envf("HERO_STEER", steer_speed); climb_speed = envf("HERO_CLIMB", climb_speed);
    autoplay = envf("HERO_AUTO", 0) != 0;
    v1knet::init("HERO", false, dl);   /* start-together (Bobby: join-in-progress is for Flying Aces and VTOL) */
    d1k::make_hud(); make_sounds();
    { std::vector<uint32_t> px(64 * 48, 0);   /* the dropping splat: an uneven white blob */
      for (int y = 0; y < 48; y++) for (int x = 0; x < 64; x++) { float dx = (x - 32) / 30.f, dy = (y - 24) / 20.f, w = 1.f + 0.25f * sinf(x * 0.5f) * cosf(y * 0.7f);
          if (dx * dx + dy * dy < w) px[y * 64 + x] = 0xFFE8F0F0u; }
      add_bitmap(BM_SPLAT, 64, 48, px); add_bitmap(BM_BLUE, 8, 8, std::vector<uint32_t>(64, 0xFFC84020u)); }
    if (glider_mode) { build_glider(); reset_flight(); } else { build_puzzle(); reset_game(); }
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    t_last = std::chrono::steady_clock::now();
    PIC_AddEvent(tick, 2.0);
    dl("Hero rebuild started, mode %s", glider_mode ? "glider" : "puzzle");
}
}   /* namespace hero */
