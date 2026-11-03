/* Flying Aces (Virtuality 1000SD, 1992) - native rebuild from footage. No 1000-series software survives in public, so every model is
   built here and no game file is read. Included after vpix_dactyl1k.h (it reuses the d1k:: kit); switched on by VPIX_NATIVE=aces. Dev only.
   Full evidence, frame times and sources: Projects\Virtuality 1000 Revival\pass_1to1_2026-11-02\aces.md
   Frame times tNNN are seconds in Octavius 2YrPW5cU06A; "tape" is the operator tape YV1jk-t2XIk.
   FROM FOOTAGE: the title panel (t971, as captured); orange hangar with green back wall, two exits signed CADET and ACE in blue; green fields with pale
   and dark patches, jagged hills, a winding pale-blue river, white houses with red roofs, a red barn; tan biplanes with a black cross on white, pale prop
   disc and black wheels; the blue-white roundel on the own wing; ring sight fixed to the plane; a sector for the propeller; LOCKED ON and the points big,
   a little left of centre, flashing green and purple; the half-moon BONUS TIME dial with a needle that rises with points; pale green shards in fixed
   places that grow in groups with hits; the masked co-pilot in the REAR seat, seen only by looking round; a Zeppelin with black pods, barrage balloons
   and a dashed railway; the picture sinking to grey, then big black 3D GAME OVER letters with a pale edge over the airfield; 20 pictures a second at best.
   FROM THE TAPE: left stick speed (keep it forward or you stall), right stick climb/dive/bank, front trigger for unlimited bullets; three hits survived;
   always another chance; repair is over 2 s in contact with the ground; the Cadet exit is on the left.
   FROM PRESS: co-pilot chatter, tail warnings, moans and his verdict at the end (Sega Force 16, The One 53); no crash on scenery or landing (Octavius 16:58,
   Sega Force "Landing's easy"); slightly delayed plane (Octavius); extra time for points (Videogame 31); train and balloon targets (12_net_sitdown);
   a replay from a debriefing console (Sega Force 16); red aircraft worth more (tape, cut off).
   NOT SETTLED: guns follow the nose through the ring (default) or the head (ACES_HEAD_AIM=1): sources disagree, see aces.md.
   INVENTED (no source), each with a tuning variable: world size and layout, flight numbers, enemy behaviour and count, the 750-point line and the 30 s it gives,
   the point values of trains and balloons, the train's look, the replay camera, every sound, the co-pilot's words and voice (stand-in: Windows SAPI, American only).
   NOT BUILT, known from press only: the giant hand for friends, multi-pod play, the Zeppelin landing, the final duel with the Baron, the plane's shadow (tape t586). Hangar: nothing goes through it (ACES_CLIP_LOG=1 counts any plane part inside it; ACES_RAM=1 flies at it; no real frame shows a plane in a wall, see aces.md); houses and the barn have no collision, unknown on the real game.
   Units: mm, seconds (speeds in the flight code are m/s). World X right, Y forward, Z up. Plane axes: x right, y forward, z up, origin at the centre of gravity.
   Env: ACES_HITS ACES_FOE_HITS ACES_GT_HITS ACES_LIVES(0 = endless) ACES_REPAIR ACES_FOES ACES_ROUND ACES_BONUS_PTS ACES_BONUS_SECS ACES_PTS_FOE ACES_PTS_RED ACES_PTS_GT
        ACES_STALL ACES_LIFT ACES_VMAX ACES_BULLET ACES_RATE ACES_LOCK_DEG ACES_LOCK_RANGE ACES_ASSIST ACES_LAG ACES_HEAD_AIM ACES_CADET_SKILL ACES_ACE_SKILL ACES_TRAIN
        ACES_PITCH_RATE ACES_THRUST ACES_FPS ACES_INVERT ACES_INVULN ACES_AUTO ACES_AUTO_ACE ACES_CAM ACES_LOOK ACES_CHASE ACES_SKIN(1 = brown of the FA II stills; the 1000 tape shows green) ACES_TAILTEXT; for pictures ACES_METER ACES_HITS_NOW ACES_MODELS; for tests ACES_CLIP_LOG ACES_RAM ACES_ZEPTEST.
   BOBBY'S PICKS 11-03, not evidence: two zeppelins with the planes' fight weaving round them (ACES_PIC2 frames one in the fight) and flying into one or a balloon wrecks you. The "on your tail" text, see below. Hits and points on them are mine.
   VIEW: first person is the default and is the 1000 footage. C on the keyboard (ACES_CHASE=1 to start there) gives a chase view: Bobby's option 11-03, framed on the ACES_PIC2/3 stills of the later Flying Aces II; those stills
   are not the 1000. In chase the shots are a white tapering beam (ACES_PIC2, Bobby's pick of the Ghost Train beam shape); first person keeps the dark specks of the footage. Not bound in VR: a rigid banking camera wants a comfort decision first.
   Controls: VR - right thumbstick (in.joy_r) pitch and bank, left trigger (in.brake) throttle, let off for down, also right grip (in.top) up and B (in.b2) down, right trigger fires. The original's
   left stick is the throttle. Desktop - A/D bank, W nose down, S nose up (ACES_INVERT=1 swaps), Shift throttle up, Ctrl throttle down,
   left mouse or Space fire. Throttle stays where you leave it; at idle on the ground the brakes hold the plane. */
#include <functional>
#include "vpix_1k_aces_pics.h"
#include "vpix_1k_net.h"         /* link play, 2-4 pods: see the "link play" block below */
namespace aces {
using buggy::V; using buggy::M; using buggy::dot; using buggy::cross; using buggy::norm; using buggy::len; using buggy::clampf; using buggy::to_world; using buggy::look_at;
using d1k::Mesh; using d1k::dput; using d1k::blit; using d1k::text; using d1k::dl; using d1k::envf; using d1k::rnd; using d1k::make_sound; using d1k::yawed; using d1k::add_bitmap;

/* ---- tuning (INVENTED unless the note says otherwise) ---- */
static const float MM = 1000.f, GRAVITY = 9.81f, PIF = 3.14159265f;
static const float GEAR = 1700.f;          /* wheel bottoms below the centre of gravity */
static const float GROUND_Z = 100.f, ROOF_Z = 9800.f, WORLD_R = 4300000.f;
static int   hits_survive = 3;             /* operator tape YV1jk-t2XIk 9:35: "any more than three hits you're a..." (cut); the next one downs him */
static int   foe_hits = 2;                 /* hits a biplane takes (earlier note of the tape: two) */
static int   gt_hits = 3;                  /* hits a balloon or a truck of the train takes */
static int   lives_total = 0;              /* 0 = no limit, only the clock ends it: tape 9:48 "you always get another chance" */
static float repair_secs = 2.f;            /* tape 9:50: in contact with the ground for more than 2 s */
static int   foes_cadet = 2, foes_ace = 3, foes_override = 0;   /* how many are up at once; shot ones are replaced */
static float round_secs = 180.f;           /* Sega Force 16, Videogame 31: about 3 minutes */
static float bonus_pts = 750.f, bonus_secs = 30.f;   /* the dial fills with points: needle from the left, about 700 points just short of the line (t996, t1004); passing the line gives extra time (Videogame 31: extra time if you play well). The seconds are INVENTED */
static int   pts_foe = 200, pts_red = 300, pts_gt = 100;   /* 200 and 300 are seen popping up (t977, t989, t1001); which plane gives 300 is not seen; the ground figure is INVENTED */
static float v_stall = 22.f, v_lift = 27.f, v_max = 55.f;              /* m/s */
static float thrust_acc = 6.f;             /* m/s^2 at full throttle; drag is set from v_max */
static float bullet_speed = 380.f;         /* m/s relative to the plane */
static float fire_rate = 9.f;              /* shots per second, unlimited (tape) */
static float lock_deg = 22.f, lock_range = 900000.f, assist = 0.85f;   /* LOCKED ON shows with the enemy well outside the ring (t975.5, t976.5): about 22 degrees */
static float ctl_lag = 0.25f;              /* Octavius 16:32: the plane is a little delayed in taking directions; the figure is mine */
static float cadet_skill = 0.45f, ace_skill = 0.95f;
static float train_speed = 9.f;            /* m/s */
static float pitch_rate = 0.9f, taxi_turn = 0.5f, max_bank = 1.1f;
static float invuln_secs = 1.f;
static float game_fps = 20.f;              /* measured: 20 at best on clean footage */
static bool  skin_brown = false, chase_on = false, tail_text = true, invert = false, autoplay = false, auto_ace = false, head_aim = false;
static float DRAG_K = 0.002f;

/* ---- ids ---- */
enum { A_WORLD = 100, A_PL_TAN, A_PL_RED, A_PL_GREEN, A_PROP, A_COCKPIT, A_RING, A_FAN, A_FACE, A_NEEDLE, A_HOLE, A_SPIKE, A_TRC_P, A_SPK_O, A_SPK_R, A_SPK_Y, A_SPK_D, A_SMOKE,
       A_BALLOON, A_ZEP, A_LOCO, A_CAR, A_SCORCH, A_FADE, A_GO_B, A_GO_F, A_BEAM, A_RING_C, A_HEAD };
enum { BM_GFONT = 400, BM_PDIG = 460, BM_PFONT = 500, BM_GDIG = 530, BM_TITLE = 545 };
enum { SK_GUN = 0x7E000001, SK_HIT, SK_HURT, SK_BOOM, SK_ENG, SK_STALL, SK_FIX, SK_GO, SK_LOCK, SK_V0 = 0x7E000020 };   /* INVENTED placeholder sounds; SK_V0 + AC_VOICE index: the stand-in co-pilot */
enum { V_TAIL, V_BEHIND, V_RIGHT, V_LEFT, V_SPEED, V_STALL, V_CLIMB, V_BANK, V_GUNS, V_DOOR, V_GOOD, V_MOAN, V_MOAN2, V_HIT, V_LAND, V_FIXED, V_ENDGOOD, V_ENDBAD, V_COUNT };

static const uint32_t C_ORANGE = 0xE8923A, C_ORANGE2 = 0xC87A2A;

/* ---- small mesh helpers ---- */
static void quad(Mesh &m, V a, V b, V c, V d, uint32_t rgb) { m.poly({a, b, c, d}, rgb); }
static void tri(Mesh &m, V a, V b, V c, uint32_t rgb) { m.poly({a, b, c}, rgb); }
static void oct8(Mesh &m, V c, V u, V w, float r, uint32_t rgb) {   /* regular octagon facing u x w */
    V p[8]; for (int i = 0; i < 8; i++) { float a = 6.2831853f * (i + 0.5f) / 8.f; p[i] = c + u * (cosf(a) * r) + w * (sinf(a) * r); }
    m.poly({p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]}, rgb);
}
static void text3d(Mesh &m, const char *s, V o, V u, V w, float px, uint32_t rgb) {   /* letters of the 5 x 7 font as flat runs; faces u x w */
    float x = 0;
    for (const char *c = s; *c; c++) {
        if (*c >= 'A' && *c <= 'Z') {
            const char *g = d1k::FONT[*c - 'A'];
            for (int row = 0; row < 7; row++) {
                int col = 0;
                while (col < 5) {
                    if (g[row * 5 + col] != '1') { col++; continue; }
                    int e = col; while (e < 5 && g[row * 5 + e] == '1') e++;
                    float x0 = x + col * px, x1 = x + e * px, z1 = (7 - row) * px, z0 = z1 - px;
                    m.poly({o + u * x0 + w * z0, o + u * x1 + w * z0, o + u * x1 + w * z1, o + u * x0 + w * z1}, rgb);
                    col = e;
                }
            }
        }
        x += (*c == ' ' ? 4 : 6) * px;
    }
}
/* a box-like section tapering from y0 to y1 (each end a rectangle of half-width w, half-height h about height zc) */
static void taper(Mesh &m, float y0, float y1, float w0, float h0, float zc0, float w1, float h1, float zc1, uint32_t top, uint32_t side, uint32_t bot) {
    V a0(-w0, y0, zc0 - h0), b0(w0, y0, zc0 - h0), c0(w0, y0, zc0 + h0), d0(-w0, y0, zc0 + h0);
    V a1(-w1, y1, zc1 - h1), b1(w1, y1, zc1 - h1), c1(w1, y1, zc1 + h1), d1(-w1, y1, zc1 + h1);
    quad(m, d0, c0, c1, d1, top); quad(m, a0, a1, b1, b0, bot);
    quad(m, b0, b1, c1, c0, side); quad(m, a0, d0, d1, a1, side);
    quad(m, d1, c1, b1, a1, side); quad(m, a0, b0, c0, d0, side);
}
static void strut(Mesh &m, float x, float y, float z0, float z1, uint32_t rgb) {
    d1k::kit_bar(m, V(x, y, z0), V(x, y, z1), 38, 20, rgb);
}
static void cross_mark(Mesh &m, float x, float y, float z, uint32_t rgb) {
    quad(m, V(x - 550, y - 110, z), V(x + 550, y - 110, z), V(x + 550, y + 110, z), V(x - 550, y + 110, z), rgb);
    quad(m, V(x - 110, y - 550, z), V(x + 110, y - 550, z), V(x + 110, y + 550, z), V(x - 110, y + 550, z), rgb);
}
static void roundel(Mesh &m, V c, float r) {   /* octagonal blue / white / blue target seen on the own wing */
    V u(1, 0, 0), w(0, 1, 0);
    oct8(m, c, u, w, r, 0x2030C8); oct8(m, c + V(0, 0, 8), u, w, r * 0.68f, 0xF0F0F8); oct8(m, c + V(0, 0, 16), u, w, r * 0.33f, 0x2030C8);
}

struct PlaneCol { uint32_t body, body2, wing, wing2, mark; bool crosses; };
static void build_plane(int id, const PlaneCol &c) {
    /* t975.5, t977, t1030 and the promo inset t1014-1017: long upper wing, shorter lower one, struts, wheels on V legs; the marks are a black cross patee on a WHITE SQUARE (enemy) or the
       blue-white roundel (green planes, tape t586). Wings stay solid blocks so they never read as paper. */
    Mesh m; const uint32_t ST = 0x2A2A22, WH = 0xF0F0E8;
    taper(m, -4300, -300, 70, 80, 420, 470, 500, 120, c.body, c.body2, c.body2);        /* rear fuselage */
    taper(m, -300, 2200, 470, 500, 120, 430, 430, 120, c.body, c.body2, c.body2);       /* front fuselage */
    d1k::kit_drum(m, V(0, 2500, 120), V(0, 320, 0), V(1, 0, 0), V(0, 0, 1), 480, 0x161616, 0x0E0E0E);   /* the engine */
    m.box(V(-300, -700, 620), V(300, 300, 650), 0x18140E);                              /* the open cockpit */
    auto wing = [&](float hw, float y0, float y1, float z, float t) {   /* tips cut back, as drawn; closed all round */
        m.poly({V(-hw, y0 + 300, z + t), V(-hw + 300, y0, z + t), V(hw - 300, y0, z + t), V(hw, y0 + 300, z + t), V(hw, y1 - 200, z + t), V(hw - 300, y1, z + t), V(-hw + 300, y1, z + t), V(-hw, y1 - 200, z + t)}, c.wing);
        m.poly({V(-hw, y1 - 200, z), V(-hw + 300, y1, z), V(hw - 300, y1, z), V(hw, y1 - 200, z), V(hw, y0 + 300, z), V(hw - 300, y0, z), V(-hw + 300, y0, z), V(-hw, y0 + 300, z)}, c.wing2);
        const float X[8] = {-hw + 300, hw - 300, hw, hw, hw - 300, -hw + 300, -hw, -hw}, Y[8] = {y1, y1, y1 - 200, y0 + 300, y0, y0, y0 + 300, y1 - 200};
        for (int i = 0; i < 8; i++) { int j = (i + 1) & 7; quad(m, V(X[i], Y[i], z), V(X[i], Y[i], z + t), V(X[j], Y[j], z + t), V(X[j], Y[j], z), c.wing2); } };
    wing(4800, 100, 1800, 1700, 200); wing(3900, 0, 1500, -600, 170);
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; strut(m, q * 3000, 800, -430, 1700, ST); strut(m, q * 1500, 800, -430, 1700, ST); d1k::kit_bar(m, V(q * 300, 800, 620), V(q * 620, 1000, 1700), 26, 18, ST);
        d1k::kit_bar(m, V(q * 3000, 800, 1700), V(q * 1500, 800, -430), 10, 10, 0x404038);
        V hub(q * 900, 900, -1500); d1k::kit_bar(m, V(q * 300, 500, -380), hub, 28, 22, ST); d1k::kit_bar(m, V(q * 300, 1400, -380), hub, 28, 22, ST);
        d1k::kit_drum(m, hub, V(110, 0, 0), V(0, 1, 0), V(0, 0, 1), 380, 0x202020, 0x161616); }
    d1k::kit_bar(m, V(-900, 900, -1500), V(900, 900, -1500), 18, 18, ST);
    d1k::kit_slab(m, {V(0, -4300, 480), V(0, -3000, 480), V(0, -3400, 1500), V(0, -4200, 1500)}, V(40, 0, 0), c.body2, d1k::kit_shade(c.body2, 0.7f));   /* fin */
    d1k::kit_slab(m, {V(-1800, -4200, 460), V(1800, -4200, 460), V(1500, -3200, 460), V(-1500, -3200, 460)}, V(0, 0, 90), c.wing, c.wing2);               /* tailplane */
    m.box(V(-30, -4200, 150), V(30, -4100, 380), ST);
    if (c.crosses) {
        auto cr = [&](V o, V u, V w, float R) { V n = norm(cross(u, w)), a = o + n * 30.f, b = o + n * 60.f; const float I = R * 0.14f, E = R * 0.38f, A = R * 0.84f;
            quad(m, a - u * R - w * R, a + u * R - w * R, a + u * R + w * R, a - u * R + w * R, WH);
            quad(m, b - u * I - w * I, b + u * I - w * I, b + u * I + w * I, b - u * I + w * I, c.mark);
            for (int i = 0; i < 4; i++) { V du = i == 0 ? u : i == 1 ? w : i == 2 ? u * -1.f : w * -1.f, dv = cross(n, du);   /* the flared arms */
                quad(m, b + du * I - dv * I, b + du * A - dv * E, b + du * A + dv * E, b + du * I + dv * I, c.mark); } };
        for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f;
            cr(V(q * 3500, 950, 1900), V(1, 0, 0), V(0, 1, 0), 620); cr(V(q * 3500, 950, 1700), V(-1, 0, 0), V(0, 1, 0), 620); cr(V(q * 2900, 750, -430), V(1, 0, 0), V(0, 1, 0), 560); cr(V(q * 2900, 750, -600), V(-1, 0, 0), V(0, 1, 0), 560);
            cr(V(q * 300, -2300, 330), k ? V(0, 1, 0) : V(0, -1, 0), V(0, 0, 1), 300); }
    } else {
        for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; roundel(m, V(q * 3200, 950, 1910), 650); roundel(m, V(q * 2700, 750, -420), 600); }
        static const uint32_t RS[3] = {0x2030C8, 0xF0F0F8, 0x2030C8};                    /* the striped rudder (tape t586) */
        for (int i = 0; i < 3; i++) { float y0 = -4300 + i * 430.f, y1 = y0 + 430.f; quad(m, V(46, y0, 520), V(46, y1, 520), V(46, y1, 1480), V(46, y0, 1480), RS[i]); quad(m, V(-46, y1, 520), V(-46, y0, 520), V(-46, y0, 1480), V(-46, y1, 1480), RS[i]); }
        m.octa(V(0, -1000, 620), 170, 330, 0xD8E8C8); m.octa(V(0, 100, 620), 170, 330, 0x5A4632);   /* the co-pilot's pale mask in the rear seat (tape t528-530), the pilot's head */
    }
    m.add(id);
}

/* ---- the cockpit seen by the pilot: wings, struts, cowl, dial; plane axes, y forward ---- */
static V dial_c, dial_u(1, 0, 0), dial_w, dial_n;
struct HoleSlot { V p; float yaw; bool spike; };
static HoleSlot hole_slot[12];
static void build_cockpit() {
    /* Measured from Octavius 2YrPW5cU06A t972-t1030 + tape t526-t539: level view shows only a low trapezoid cowl (top edge 3/4 down the picture), the wing's
       underside as a band across the top with a notch at the middle, one long strut each side leaning outward as it rises and a ring sight on a post; look down and
       the cowl's faces open up: holed deck, dark bar, the panel nearly upright with the dial on it, black hatch below it, floor, rails to the corners. Ring and dial fit in one
       picture (t1003, t1011: about 38 degrees apart). The panel is blue on the tape (t588-593); Octavius drifts it from grey-green (t1003) to pink (t1016). */
    /* own plane green at about 0.62 of the grass, measured: tape t528 42A769 on 47FF95, t587 6AE689 on 62FF96; inset t1012 085301 on 33882A; Octavius t1000 476F6E on 62BA87. ACES_SKIN=1: the brown of the later Flying Aces II stills */
    Mesh m; const uint32_t G1 = skin_brown ? 0x633931 : 0x1E6E2A, G2 = skin_brown ? 0x4E2C26 : 0x185A24, G3 = skin_brown ? 0x523031 : 0x175426 /* the wing's underside */, ST = 0x6A6A3A, DECK = skin_brown ? 0x946B4A : 0x185F24, DECK2 = skin_brown ? 0x7A583C : 0x134E1E, PK = 0x4F72B8, PK2 = 0x41609E, PK3 = 0x344E84, FL = 0x7E6A62;
    const float DZ = 710.f;                                  /* the cowl deck's height; the eye is at z 1050 */
    /* upper wing: a strip of leading edge, then a big six-sided hole over the seat (t1007-1009 looking up: sky bounded by the strip below and by two slanting edges meeting at a point each side; t1000: the strip with sky above it) */
    { const float LE = 2150, TE = -1200, S = 4400; const float HX[6] = {-700, 700, 1500, 700, -700, -1500}, HY[6] = {1425, 1425, 500, -500, -500, 500};
      for (int k = 0; k < 2; k++) { float z = k ? 1800.f : 1700.f; uint32_t c = k ? G1 : G3;
          quad(m, V(-S, 1425, z), V(S, 1425, z), V(S, LE, z), V(-S, LE, z), c); quad(m, V(-S, TE, z), V(S, TE, z), V(S, -500, z), V(-S, -500, z), c);
          m.poly({V(-S, -500, z), V(-700, -500, z), V(-1500, 500, z), V(-700, 1425, z), V(-S, 1425, z)}, c); m.poly({V(700, -500, z), V(S, -500, z), V(S, 1425, z), V(700, 1425, z), V(1500, 500, z)}, c); }
      quad(m, V(-S, LE, 1700), V(S, LE, 1700), V(S, LE, 1800), V(-S, LE, 1800), G2);
      for (int i = 0; i < 6; i++) { int j = (i + 1) % 6; quad(m, V(HX[i], HY[i], 1700), V(HX[j], HY[j], 1700), V(HX[j], HY[j], 1800), V(HX[i], HY[i], 1800), G2); } }
    m.box(V(-4200, -100, -620), V(4200, 1500, -500), G1, G2, G2);    /* lower wing */
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f;
        d1k::kit_bar(m, V(q * 388, 1191, -500), V(q * 1100, 1890, 1700), 26, 14, ST);          /* the long strut each side */
        d1k::kit_bar(m, V(q * 3000, 900, -500), V(q * 3000, 1700, 1700), 30, 16, ST);          /* the outer ones, seen only by turning */
        roundel(m, V(q * 2600, 600, -480), 700);                     /* the roundel on the lower wing, as in the tape */
        roundel(m, V(q * 2800, 800, 1810), 650); }
    /* cowl: deck, dark bar across it, sides, nose */
    quad(m, V(-330, 800, DZ), V(330, 800, DZ), V(360, 1450, DZ - 10), V(-360, 1450, DZ - 10), DECK);
    quad(m, V(-330, 800, DZ + 6), V(330, 800, DZ + 6), V(320, 890, DZ + 6), V(-320, 890, DZ + 6), skin_brown ? 0x5A402C : 0x0E3A16);
    quad(m, V(330, 800, DZ), V(620, 800, 100), V(620, 1450, 100), V(360, 1450, DZ - 10), DECK2);
    quad(m, V(-330, 800, DZ), V(-360, 1450, DZ - 10), V(-620, 1450, 100), V(-620, 800, 100), DECK2);
    quad(m, V(-360, 1450, DZ - 10), V(360, 1450, DZ - 10), V(620, 1450, 100), V(-620, 1450, 100), DECK2);
    /* the pink wedge: a slant toward the pilot with the dial on it, cheeks, then the black hatch and the floor falling away to the corners */
    const V W0(-330, 800, DZ), W1(330, 800, DZ), W2(560, 690, 170), W3(-560, 690, 170);
    quad(m, W3, W2, W1, W0, PK);
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f;
        V a(q * 330, 800, DZ), b(q * 560, 690, 170), c(q * 760, 200, 60), d(q * 800, 800, 100);
        if (k) quad(m, b, c, d, a, PK2); else quad(m, a, d, c, b, PK2); }
    quad(m, V(-250, 690, 172), V(250, 690, 172), V(250, 300, 80), V(-250, 300, 80), 0x101014);   /* the black hatch */
    { const float zt = 375.f, yt = 800.f - (DZ - zt) * 0.2037f - 3.f; quad(m, V(-140, yt, zt), V(140, yt, zt), V(250, 687, 172), V(-250, 687, 172), 0x101014); }   /* it starts right under the dial (t1003) */
    quad(m, V(-560, 690, 170), V(-250, 690, 170), V(-250, 300, 80), V(-760, 200, 60), PK3); quad(m, V(250, 690, 170), V(560, 690, 170), V(760, 200, 60), V(250, 300, 80), PK3);
    quad(m, V(-760, 200, 60), V(-250, 300, 80), V(250, 300, 80), V(760, 200, 60), FL);
    /* the BONUS TIME dial: a white fan, not a half moon (t1004-1011): purple tick triangles left of a purple line, BONUS TIME right of it, slanted along the arc; a needle rises from the left edge */
    { V sl = norm(V(0, 110, 540)); dial_w = sl; dial_n = norm(cross(V(1, 0, 0), sl)); if (dial_n.z < 0) dial_n = dial_n * -1.f;
      const float R = 300.f, R0 = 95.f; dial_c = V(0, 800.f - (DZ - 396.f) * 0.2037f, 396.f) + dial_n * 10.f;
      auto at = [&](float deg, float r, float lift) { float a = deg * PIF / 180.f; return dial_c + dial_u * (cosf(a) * r) + dial_w * (sinf(a) * r) + dial_n * lift; };
      for (int k = 0; k < 8; k++) { float a0 = 50.f + 10.f * k, a1 = a0 + 10.f; quad(m, at(a0, R0, 0), at(a0, R, 0), at(a1, R, 0), at(a1, R0, 0), 0xF2EEDC); }
      for (int k = 0; k < 6; k++) { float a = 126.f - k * 6.2f; tri(m, at(a, R - 4.f, 4), at(a - 2.2f, R - 60.f, 4), at(a + 2.2f, R - 60.f, 4), 0x7A28A8); }
      quad(m, at(95.f, R0 + 10.f, 4), at(95.f, R - 2.f, 4), at(93.8f, R - 2.f, 4), at(92.8f, R0 + 10.f, 4), 0x7A28A8);
      const float rot = -0.40f; V tu = dial_u * cosf(rot) + dial_w * sinf(rot), tw = dial_w * cosf(rot) - dial_u * sinf(rot);
      text3d(m, "BONUS", dial_c + dial_u * 15.f + dial_w * 225.f + dial_n * 4.f, tu, tw, 4.9f, 0x7A28A8);
      text3d(m, "TIME", dial_c + dial_u * 24.f + dial_w * 178.f + dial_n * 4.f, tu, tw, 4.9f, 0x7A28A8); }
    /* the rest of the biplane behind and under the pilot (Bobby 2026-11-02). INVENTED: footage only looks forward and to the wings */
    { const uint32_t DK = 0x24241E, RD = 0xB03020, TEAL = DECK, TEAL2 = DECK2;
      quad(m, V(-560, -500, 720), V(560, -500, 720), V(90, -6500, 640), V(-90, -6500, 640), TEAL);                  /* rear deck */
      quad(m, V(560, -500, 720), V(560, -500, -500), V(90, -6500, 420), V(90, -6500, 640), TEAL2);                  /* sides, tapering to the tail */
      quad(m, V(-560, -500, 720), V(-560, -500, -500), V(-90, -6500, 420), V(-90, -6500, 640), TEAL2);
      quad(m, V(-560, -500, -500), V(560, -500, -500), V(90, -6500, 420), V(-90, -6500, 420), TEAL2);
      /* no fairing or headrest here: looking back the view is the co-pilot's head over his shoulders with only the tailplane behind him (t980-982) */
      quad(m, V(-90, -6500, 420), V(90, -6500, 420), V(90, -6500, 640), V(-90, -6500, 640), TEAL2);                 /* the end of the tail */
      d1k::kit_slab(m, {V(0, -5700, 620), V(0, -6600, 620), V(0, -6900, 1950), V(0, -6100, 1950)}, V(45, 0, 0), RD, d1k::kit_shade(RD, 0.7f));   /* fin and rudder */
      /* the tailplane: a bar each side of his head at mouth height, tapering to a point, pale above and dark below (t980.5: reaches about 0.39 of the picture each side) */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; V a(q * 80, -5300, 860), b(q * 3000, -6100, 860), c(q * 3000, -6180, 860), d(q * 80, -6500, 860);
          quad(m, a + V(0, 0, 70), b + V(0, 0, 8), c + V(0, 0, 8), d + V(0, 0, 70), G1); quad(m, a - V(0, 0, 70), b - V(0, 0, 8), c - V(0, 0, 8), d - V(0, 0, 70), G3);
          quad(m, a + V(0, 0, 70), a - V(0, 0, 70), b - V(0, 0, 8), b + V(0, 0, 8), 0x123A1C); }
      quad(m, V(620, 900, 100), V(620, 1450, 100), V(400, 1450, -300), V(560, 700, -500), TEAL2);                   /* under the cowl */
      quad(m, V(-620, 900, 100), V(-620, 1450, 100), V(-400, 1450, -300), V(-560, 700, -500), TEAL2);
      quad(m, V(-560, 700, -500), V(560, 700, -500), V(400, 1450, -300), V(-400, 1450, -300), TEAL2);
      m.box(V(-560, -500, -500), V(-540, 700, 100), TEAL2); m.box(V(540, -500, -500), V(560, 700, 100), TEAL2);     /* cockpit sides, down to the belly */
      m.box(V(-540, -500, -120), V(540, 700, -100), DK);                                                            /* floor */
      { using d1k::kit_quad; d1k::kit_eye = V(0, 150, 1050); const uint32_t BR = 0xC8A040, LE = 0x3A2414, WD = 0x8A6438, WD2 = 0x6A4A2A;
        m.box(V(-20, 230, -100), V(20, 270, 400), 0x2A2A22); m.box(V(-45, 220, 400), V(45, 280, 480), LE);          /* stick and rudder bar */
        m.box(V(-330, 340, -100), V(330, 390, -60), WD); m.box(V(-20, 320, -100), V(20, 410, -50), 0x2A2A22);
        for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; float x0 = q * 470, x1 = q * 670;
            m.box(V((std::min)(x0, x1), -560, 660), V((std::max)(x0, x1), 760, 720), DECK, DECK2, DECK2);            /* the rails down to the corners; these two sides, then the rear */
            kit_quad(m, V(q * 536, -500, -100), V(0, 1190, 0), V(0, 0, 780), WD);                                    /* the wooden wall, ribs and a stringer */
            for (int i = 0; i < 4; i++) m.box(V((std::min)(q * 536, q * 480), -460.f + 330.f * i, -100), V((std::max)(q * 536, q * 480), -420.f + 330.f * i, 660), WD2);
            m.box(V((std::min)(q * 536, q * 490), -500, 340), V((std::max)(q * 536, q * 490), 690, 370), WD2); }
        m.box(V(-470, -560, 660), V(470, -440, 720), LE, 0x2A1A0E, 0x2A1A0E);
        m.box(V(-536, 120, 330), V(-480, 340, 440), BR); m.box(V(-505, 220, 440), V(-485, 250, 610), 0x2A2A22); m.box(V(-515, 200, 610), V(-475, 270, 650), LE);   /* throttle on the left wall */
        m.box(V(470, 40, 150), V(530, 110, 420), 0xC02020); m.box(V(475, 45, 420), V(525, 105, 450), BR);            /* fire bottle */
        kit_quad(m, V(-300, -198, 150), V(90, 0, 0), V(0, 0, 610), 0xB89A68); kit_quad(m, V(210, -198, 150), V(90, 0, 0), V(0, 0, 610), 0xB89A68);   /* canvas straps on the seat */
        kit_quad(m, V(-380, -175, 400), V(760, 0, 0), V(0, 0, 70), 0xB89A68); }
      m.box(V(-380, -280, -100), V(380, -200, 760), 0x6A4A2A); m.box(V(-380, -200, -100), V(380, 200, 150), 0x5A3A22);   /* seat */
      m.box(V(-540, -540, -100), V(540, -500, 660), 0x3A2E20);                                                           /* bulkhead */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f;                                                       /* wheels on V struts, bracing wires */
          V hub(q * 1000, 1100, -1900); d1k::kit_bar(m, V(q * 430, 800, -490), hub, 32, 32, ST); d1k::kit_bar(m, V(q * 430, 1500, -490), hub, 32, 32, ST); d1k::kit_bar(m, V(0, 1100, -1900), hub, 28, 28, ST);
          d1k::kit_drum(m, hub, V(90, 0, 0), V(0, 1, 0), V(0, 0, 1), 470, 0x20201C, 0x14140F); d1k::kit_drum(m, hub, V(110, 0, 0), V(0, 1, 0), V(0, 0, 1), 130, 0x707068, 0x505048); }
      m.box(V(-40, -6400, 300), V(40, -6300, 440), ST); }                                                           /* tail skid */
    m.add(A_COCKPIT);
    /* the ring sight on its post, fixed to the plane: it moves in the picture when the head turns (t975, t1005, t1010) */
    { Mesh r; const uint32_t c = 0x30405C; const V o(0, 1350, 1022); const float R = 150.f;   /* it stays this dark blue when locked on (t975.5, t997) */
      for (int i = 0; i < 8; i++) { float a0 = 6.2831853f * (i + 0.5f) / 8.f, a1 = 6.2831853f * (i + 1.5f) / 8.f;
          d1k::kit_bar(r, o + V(cosf(a0) * R, 0, sinf(a0) * R), o + V(cosf(a1) * R, 0, sinf(a1) * R), 5, 5, c); }
      d1k::kit_bar(r, o + V(-R * 0.92f, 0, 0), o + V(R * 0.92f, 0, 0), 3, 3, c); d1k::kit_bar(r, V(0, 1350, DZ - 15), o + V(0, 0, R * 0.92f), 4, 4, c);
      r.add(A_RING);
      Mesh q; const V o2(0, 0, 0); for (int i = 0; i < 8; i++) { float a0 = 6.2831853f * (i + 0.5f) / 8.f, a1 = 6.2831853f * (i + 1.5f) / 8.f; d1k::kit_bar(q, o2 + V(cosf(a0) * R, 0, sinf(a0) * R), o2 + V(cosf(a1) * R, 0, sinf(a1) * R), 5, 5, c); }
      d1k::kit_bar(q, V(-R * 0.92f, 0, 0), V(R * 0.92f, 0, 0), 3, 3, c); d1k::kit_bar(q, V(0, 0, -R * 0.92f), V(0, 0, R * 0.92f), 3, 3, c); q.add(A_RING_C);
      Mesh hd; hd.octa(V(0, 0, 0), 270, 380, 0x5C4030); hd.add(A_HEAD);   /* the rear-seat man from behind: leather helmet (mask t979) */
      Mesh b; tri(b, V(-1, 0, 0), V(1, 0, 0), V(0, 1, 0), 0xFFFFFF); tri(b, V(1, 0, 0), V(-1, 0, 0), V(0, 1, 0), 0xFFFFFF); b.add(A_BEAM); }
    /* the propeller: one translucent grey sector that jumps round the hub (the "fan" of the tape) */
    { Mesh p; V a[7]; a[0] = V(0, 0, 0); for (int i = 0; i < 6; i++) { float t = (-47.f + i * 18.8f) * PIF / 180.f; a[i + 1] = V(sinf(t) * 820.f, 0, cosf(t) * 820.f); }
      p.poly({a[0], a[1], a[2], a[3], a[4], a[5], a[6]}, 0x60666E); p.poly({a[6], a[5], a[4], a[3], a[2], a[1], a[0]}, 0x60666E); p.add(A_FAN); }
    /* the co-pilot in the REAR seat (Octavius "the backseat", Videogame 31 looking back, Sega Force "look round"; t978.6-980 shows the head turning right round to him): a long pale green mask,
       lilac goggles with square black eyes, a nose wedge, a wide pale mouth ruled with three upright lines and one across, a black crease rising from each corner, leather helmet and flaps, dark shoulders */
    { Mesh h; const uint32_t SK = 0xD8E4A8, SK2 = 0xB4C088, SK3 = 0x98A474, LH = 0x6A5238, LH2 = 0x4E3C28, GO = 0xE4DCF8, BK = 0x101010, TE = 0xF4F4E4; const float y1 = -2, y2 = -4, y3 = -6, y4 = -8;
      h.poly({V(-150, y1, 160), V(150, y1, 160), V(250, y1, 60), V(260, y1, -120), V(-260, y1, -120), V(-250, y1, 60)}, SK);       /* the long mask, with darker jaw facets */
      h.poly({V(-260, y1, -120), V(260, y1, -120), V(170, y1, -430), V(80, y1, -600), V(-80, y1, -600), V(-170, y1, -430)}, SK);
      h.poly({V(-260, y2, -120), V(-190, y2, -120), V(-120, y2, -440), V(-80, y2, -600), V(-170, y2, -430)}, SK2); h.poly({V(190, y2, -120), V(260, y2, -120), V(170, y2, -430), V(80, y2, -600), V(120, y2, -440)}, SK2);
      h.poly({V(-80, y2, -600), V(80, y2, -600), V(60, y2, -520), V(-60, y2, -520)}, SK3);
      h.poly({V(-210, y1, 120), V(210, y1, 120), V(150, y1, 330), V(-150, y1, 330)}, LH); h.poly({V(-150, y1, 330), V(150, y1, 330), V(70, y1, 400), V(-70, y1, 400)}, LH2);    /* helmet */
      for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f;
          V a(q * 245, y1, 90), b(q * 345, y1, 40), c(q * 340, y1, -110), e(q * 255, y1, -130);
          if (k) h.poly({a, b, c, e}, LH); else h.poly({b, a, e, c}, LH);                                                       /* ear flaps */
          V g0(q * 25, y2, 5), g1(q * 225, y2, 15), g2(q * 200, y2, 145), g3(q * 35, y2, 130);
          if (k) h.poly({g0, g1, g2, g3}, GO); else h.poly({g1, g0, g3, g2}, GO);
          V e0(q * 75, y3, 50), e1(q * 135, y3, 50), e2(q * 135, y3, 100), e3(q * 75, y3, 100);
          if (k) h.poly({e0, e1, e2, e3}, BK); else h.poly({e1, e0, e3, e2}, BK);
          V c0(q * 180, y3, -190), c1(q * 300, y3, -120), c2(q * 300, y3, -100), c3(q * 165, y3, -160);                         /* the crease */
          if (k) h.poly({c0, c1, c2, c3}, BK); else h.poly({c1, c0, c3, c2}, BK); }
      h.poly({V(-28, y3, 120), V(28, y3, 120), V(70, y3, -150), V(-70, y3, -150)}, SK2);                                        /* nose wedge, running up between the goggles */
      h.poly({V(-195, y2, -210), V(195, y2, -210), V(150, y2, -330), V(60, y2, -385), V(-60, y2, -385), V(-150, y2, -330)}, TE); /* the mouth */
      for (int i = -1; i <= 1; i++) { float x = i * 75.f; h.poly({V(x - 9, y4, -215), V(x + 9, y4, -215), V(x + 9, y4, -375), V(x - 9, y4, -375)}, BK); }
      h.poly({V(-180, y4, -280), V(180, y4, -280), V(180, y4, -296), V(-180, y4, -296)}, BK);
      h.poly({V(-620, 40, -1300), V(620, 40, -1300), V(170, 40, -470), V(-170, 40, -470)}, LH2);                                /* shoulders */
      h.add(A_FACE); }
    { Mesh n; const uint32_t GD = 0xC8A820; tri(n, V(-6, 102, 0), V(6, 102, 0), V(0, 284, 0), GD); tri(n, V(-16, 193, 0), V(16, 193, 0), V(0, 284, 0), GD); n.add(A_NEEDLE); }   /* gold arrow (t1004-1011) */
    { Mesh p; oct8(p, V(0, 0, 0), V(1, 0, 0), V(0, 0, 1), 1050.f, 0xF0F0F0); oct8(p, V(0, 0, 0), V(0, 0, 1), V(1, 0, 0), 1050.f, 0xF0F0F0); p.add(A_PROP); }   /* the enemy's propeller: a pale disc (t975.5); it does not spin */
    /* the holes: fixed shapes on the deck, the same four each time (t1003-1011, t1022): pale hexagons with a dark sliver beside them, and pale spikes standing up (tape t588-593 agrees) */
    { Mesh h; const uint32_t PL = 0xD0F4DC; h.poly({V(-30, -70, 0), V(40, -70, 0), V(80, 0, 0), V(40, 70, 0), V(-30, 70, 0), V(-70, 0, 0)}, PL);
      h.poly({V(-115, -60, -2), V(-70, -30, -2), V(-70, 10, -2), V(-120, -20, -2)}, 0x101810); h.add(A_HOLE);
      Mesh k; tri(k, V(-22, 0, 0), V(22, 0, 0), V(6, 0, 160), PL); tri(k, V(22, 0, 0), V(-22, 0, 0), V(6, 0, 160), PL); tri(k, V(0, -22, 0), V(0, 22, 0), V(6, 0, 160), PL); tri(k, V(0, 22, 0), V(0, -22, 0), V(6, 0, 160), PL); k.add(A_SPIKE);
      static const float PX[4] = {-300, -110, 120, 250}, PY[4] = {900, 1020, 1320, 1360}; static const bool SP[4] = {true, false, false, true};
      std::mt19937 hr(5);
      for (int i = 0; i < 12; i++) { float x = i < 4 ? PX[i] : ((hr() % 520) - 260.f), y = i < 4 ? PY[i] : 900.f + (hr() % 480), yw = (hr() % 628) / 100.f;
          hole_slot[i].p = V(x, y, DZ + 8.f); hole_slot[i].yaw = i < 4 ? 0.f : yw; hole_slot[i].spike = i < 4 ? SP[i] : i % 2 == 0; } }
}

/* ---- ground targets, effects ---- */
static void loft(Mesh &m, std::initializer_list<V> st, uint32_t c) {   /* eight-sided body along y: each V is (y, radius, 0) */
    std::vector<V> s(st); const uint32_t c2 = d1k::kit_shade(c, 0.86f); static const float K = 0.7071f; const float cx[8] = {1, K, 0, -K, -1, -K, 0, K}, cz[8] = {0, K, 1, K, 0, -K, -1, -K};
    for (size_t i = 0; i + 1 < s.size(); i++) for (int a = 0; a < 8; a++) { int b = (a + 1) & 7; float y0 = s[i].x, r0 = s[i].y, y1 = s[i + 1].x, r1 = s[i + 1].y;
        quad(m, V(cx[a] * r0, y0, cz[a] * r0), V(cx[a] * r1, y1, cz[a] * r1), V(cx[b] * r1, y1, cz[b] * r1), V(cx[b] * r0, y0, cz[b] * r0), (a & 1) ? c : c2); }
}
static void build_small() {
    /* barrage balloons: two teardrops with fins high over the far hills (t1016-1017; Sega Force "giant", The One). Pale: the inset's teal shift makes them read green */
    { Mesh b; const uint32_t C = 0xC4CCB8, F = 0x8A9484; loft(b, {V(-11000, 300, 0), V(-7000, 3200, 0), V(0, 5000, 0), V(6000, 3600, 0), V(10000, 400, 0)}, C);
      d1k::kit_slab(b, {V(0, -10500, 1000), V(0, -6500, 3000), V(0, -7000, 7000), V(0, -11000, 6000)}, V(160, 0, 0), F, d1k::kit_shade(F, 0.7f));
      d1k::kit_slab(b, {V(800, -10500, 0), V(3000, -6500, 0), V(7000, -7000, 0), V(6000, -11000, 0)}, V(0, 0, 160), F, d1k::kit_shade(F, 0.7f));
      d1k::kit_slab(b, {V(-800, -10500, 0), V(-3000, -6500, 0), V(-7000, -7000, 0), V(-6000, -11000, 0)}, V(0, 0, 160), F, d1k::kit_shade(F, 0.7f)); b.add(A_BALLOON); }
    /* the Zeppelin: long pale cigar with black round pods (t1014-1015, being hit); on the title panel it carries the cross in a white square. Joystick 57 names Zeppelins as targets */
    { Mesh z; const uint32_t C = 0xD4DCCC; loft(z, {V(-36000, 600, 0), V(-27000, 5000, 0), V(-8000, 7400, 0), V(12000, 7400, 0), V(28000, 4600, 0), V(36000, 500, 0)}, C);
      static const float PX[3] = {-5500, 5500, 0}, PY[3] = {-13000, -13000, 12000}, PZ[3] = {-6200, -6200, -8200};
      for (int i = 0; i < 3; i++) { V c(PX[i], PY[i], PZ[i]); d1k::kit_drum(z, c, V(0, 1100, 0), V(1, 0, 0), V(0, 0, 1), 1700, 0x141414, 0x0C0C0C); }
      z.box(V(-1000, -4000, -8600), V(1000, 4000, -6800), 0x3A3A36);
      for (int k = 0; k < 2; k++) { V u = k ? V(0, 1, 0) : V(0, -1, 0), w(0, 0, 1); V n = norm(cross(u, w));
          V a = V(k ? 5900.f : -5900.f, -22000, 0) + n * 40.f, b = a + n * 40.f; const float R = 2600.f; quad(z, a - u * R - w * R, a + u * R - w * R, a + u * R + w * R, a - u * R + w * R, 0xF4F4EC);
          quad(z, b - u * 300.f - w * 2100.f, b + u * 300.f - w * 2100.f, b + u * 300.f + w * 2100.f, b - u * 300.f + w * 2100.f, 0x141414); quad(z, b - u * 2100.f - w * 300.f, b + u * 2100.f - w * 300.f, b + u * 2100.f + w * 300.f, b - u * 2100.f + w * 300.f, 0x141414); }
      d1k::kit_slab(z, {V(0, -35000, 1000), V(0, -24000, 4000), V(0, -26000, 11000), V(0, -36000, 9000)}, V(240, 0, 0), 0xA8B0A0, 0x808878); z.add(A_ZEP); }
    { Mesh l; l.box(V(-1500, -4500, 600), V(1500, 4500, 1500), 0x303038);                     /* chassis */
      l.box(V(-1250, -4000, 1500), V(1250, 1000, 3600), 0x2A2A30);                            /* cab */
      l.box(V(-1150, 1000, 1500), V(1150, 4300, 3300), 0x38383E);                             /* boiler */
      l.octa(V(0, 3600, 3300), 420, 1400, 0x202024); l.box(V(-1400, -1000, 3600), V(1400, 1000, 3750), 0xA82020);
      l.add(A_LOCO); }
    { Mesh c; c.box(V(-1600, -4000, 600), V(1600, 4000, 3200), 0x8A4A2A, 0x8A4A2A, 0x303030);
      c.box(V(-1700, -4100, 3200), V(1700, 4100, 3400), 0xC8C0A8); c.add(A_CAR); }
    { Mesh s; quad(s, V(-4500, -4500, 0), V(4500, -4500, 0), V(4500, 4500, 0), V(-4500, 4500, 0), 0x141410); s.add(A_SCORCH); }
    { Mesh t; t.octa(V(0, 0, -120), 120, 240, 0x18181C); t.add(A_TRC_P); }                    /* bullets: small dark specks, no tracer lines in any frame (t988-990, t1000) */
    const uint32_t sc[5] = {0xF09020, 0xE03018, 0xF0E060, 0x101014, 0x707070}; const int sid[5] = {A_SPK_O, A_SPK_R, A_SPK_Y, A_SPK_D, A_SMOKE}; const float ss[5] = {900.f, 800.f, 600.f, 130.f, 1500.f};
    for (int k = 0; k < 5; k++) { Mesh s; float r = ss[k];
        tri(s, V(-r, 0, -r * 0.6f), V(r, 0, -r * 0.6f), V(0, 0, r * 1.2f), sc[k]); tri(s, V(0, -r, -r * 0.6f), V(0, r, -r * 0.6f), V(0, 0, r * 1.2f), sc[k]); s.add(sid[k]); }
    { Mesh q; const float W = 9000.f; const uint32_t C = 0x787488; quad(q, V(-W, 0, -W), V(W, 0, -W), V(W, 0, W), V(-W, 0, W), C); quad(q, V(W, 0, -W), V(-W, 0, -W), V(-W, 0, W), V(W, 0, W), C); q.add(A_FADE); }   /* the fade to grey at the end (t1026-1031) */
    /* GAME OVER: huge black block letters with a pale edge, standing on the airfield (t1032-1034) */
    { struct Seg { float a, b, c, d; }; static const Seg G[] = {{3.6f,6,1,6},{1,6,0,5},{0,5,0,1},{0,1,1,0},{1,0,3,0},{3,0,4,1},{4,1,4,3},{4,3,2.2f,3}}, Aa[] = {{0,0,2,6},{2,6,4,0},{0.9f,2,3.1f,2}}, Mm[] = {{0,0,0,6},{0,6,2,2.5f},{2,2.5f,4,6},{4,6,4,0}},
        Ee[] = {{0,0,0,6},{0,6,4,6},{0,3,3,3},{0,0,4,0}}, Oo[] = {{1,0,3,0},{3,0,4,1},{4,1,4,5},{4,5,3,6},{3,6,1,6},{1,6,0,5},{0,5,0,1},{0,1,1,0}}, Vv[] = {{0,6,2,0},{2,0,4,6}}, Rr[] = {{0,0,0,6},{0,6,3,6},{3,6,4,5},{4,5,4,4},{4,4,3,3},{3,3,0,3},{1.8f,3,4,0}};
      struct Ch { const Seg *s; int n; }; const Ch word[9] = {{G, 8}, {Aa, 3}, {Mm, 4}, {Ee, 4}, {nullptr, 0}, {Oo, 8}, {Vv, 2}, {Ee, 4}, {Rr, 7}};
      const float U = 1500.f, ADV = 5.3f * U, T = 0.58f * U, X0 = -4.f * ADV - 2.f * U;
      for (int lay = 0; lay < 2; lay++) { Mesh w; uint32_t col = lay ? 0x101014 : 0xB4B2C0; float ox = lay ? 0.f : 0.28f * U, oz = lay ? 0.f : -0.28f * U;
          for (int i = 0; i < 9; i++) for (int j = 0; j < word[i].n; j++) { const Seg &g = word[i].s[j]; V a(X0 + i * ADV + g.a * U + ox, 0, 2000.f + g.b * U + oz), b(X0 + i * ADV + g.c * U + ox, 0, 2000.f + g.d * U + oz); V dir = norm(b - a), p(-dir.z * T, 0, dir.x * T), e = dir * (T * 0.45f);
              quad(w, a - e - p, a - e + p, b + e + p, b + e - p, col); quad(w, b + e - p, b + e + p, a - e + p, a - e - p, col); }
          w.add(lay ? A_GO_F : A_GO_B); } }
}

/* ---- the world: fields, hills, river, houses, hangar, runway, track ---- */
struct Rect { float x0, x1, y0, y1; };
static std::vector<Rect> walls;
static std::vector<V> trk; static std::vector<float> trk_s; static float trk_len = 1.f;
static const float HANGAR_X = 30000.f, HANGAR_Y0 = -30000.f, HANGAR_TOP = 19000.f, SIGN_Z = 3760.f, T_FRONT = 800.f;
static void facing(Mesh &m, V a, V b, V c, V toward, uint32_t rgb) {   /* a triangle wound so its normal faces 'toward' */
    V n = cross(b - a, c - a);
    if (dot(n, toward - a) < 0) m.poly({a, c, b}, rgb); else m.poly({a, b, c}, rgb);
}
static void house(Mesh &m, V at, float yaw, float w, float d, float h, uint32_t wall, uint32_t roof) {
    float cy = cosf(yaw), sy = sinf(yaw), hw = w * 0.5f, hd = d * 0.5f, rz = h * 1.45f;
    auto P = [&](float x, float y, float z) { return V(at.x + x * cy - y * sy, at.y + x * sy + y * cy, at.z + z); };
    quad(m, P(-hw, -hd, 0), P(hw, -hd, 0), P(hw, -hd, h), P(-hw, -hd, h), wall);
    quad(m, P(hw, hd, 0), P(-hw, hd, 0), P(-hw, hd, h), P(hw, hd, h), wall);
    quad(m, P(hw, -hd, 0), P(hw, hd, 0), P(hw, hd, h), P(hw, -hd, h), wall);
    quad(m, P(-hw, hd, 0), P(-hw, -hd, 0), P(-hw, -hd, h), P(-hw, hd, h), wall);
    quad(m, P(-hw - 300, -hd - 300, h), P(hw + 300, -hd - 300, h), P(hw + 300, 0, rz), P(-hw - 300, 0, rz), roof);
    quad(m, P(-hw - 300, 0, rz), P(hw + 300, 0, rz), P(hw + 300, hd + 300, h), P(-hw - 300, hd + 300, h), roof);
    tri(m, P(-hw, -hd, h), P(-hw, hd, h), P(-hw, 0, rz), wall); tri(m, P(hw, hd, h), P(hw, -hd, h), P(hw, 0, rz), wall);
}
static V trk_at(float s, float &yaw_out) {
    s = fmodf(s, trk_len); if (s < 0) s += trk_len;
    size_t i = 0; while (i + 1 < trk_s.size() && trk_s[i + 1] <= s) i++;
    size_t j = (i + 1) % trk.size(); float end = (i + 1 < trk_s.size()) ? trk_s[i + 1] : trk_len, seg = (std::max)(end - trk_s[i], 1.f), u = (s - trk_s[i]) / seg;
    V a = trk[i], b = trk[j], t = norm(b - a); yaw_out = atan2f(-t.x, t.y);
    return a + (b - a) * u;
}
static void build_world() {
    Mesh m; std::mt19937 wr(11);
    /* the fields: one sheet of irregular patches (no two surfaces fighting at a distance). Mid green with bright yellow-green, pale, dark and a few ochre ones (inset t1016-1017, tape t554, t567) */
    static const uint32_t gr[8] = {0x2F9C34, 0x2F9C34, 0x2F9C34, 0x88CC24, 0x58B830, 0xB4D838, 0x1E8432, 0xC8A030};
    const int N = 18; const float P = 650000.f, H = N * P * 0.5f, J = 210000.f; static V cn[N + 1][N + 1];
    for (int i = 0; i <= N; i++) for (int j = 0; j <= N; j++) { bool edge = i == 0 || j == 0 || i == N || j == N; float jx = (float)(wr() % 1000) / 500.f - 1.f, jy = (float)(wr() % 1000) / 500.f - 1.f; cn[i][j] = V(-H + i * P + (edge ? 0.f : jx * J), -H + j * P + (edge ? 0.f : jy * J), 0); }
    for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) {
        V c = (cn[i][j] + cn[i + 1][j + 1]) * 0.5f; uint32_t col = gr[wr() % 8]; if (c.x * c.x + c.y * c.y < 900000.f * 900000.f) col = 0x2F9C34;
        quad(m, cn[i][j], cn[i + 1][j], cn[i + 1][j + 1], cn[i][j + 1], col); }
    /* the horizon: one jagged dark green ridge all the way round (t972, t996, t1016). The "pale peaks" of the last pass were the propeller sector, misread: gone */
    { const int NH = 96; const float RH = 5.0e6f; float h0 = rnd(240000.f, 520000.f), h00 = h0;
      for (int i = 0; i < NH; i++) {
          float a0 = 6.2831853f * i / NH, a1 = 6.2831853f * (i + 1) / NH, h1 = (i == NH - 1) ? h00 : ((i & 1) ? rnd(380000.f, 560000.f) : rnd(220000.f, 320000.f));
          V b0(cosf(a0) * RH, sinf(a0) * RH, 0), b1(cosf(a1) * RH, sinf(a1) * RH, 0), t0 = b0 + V(0, 0, h0), t1 = b1 + V(0, 0, h1); V in(0, 0, 100000.f);
          facing(m, b0, b1, t1, in, (i & 1) ? 0x237A3A : 0x1E6C34); facing(m, b0, t1, t0, in, (i & 1) ? 0x237A3A : 0x1E6C34); h0 = h1; } }
    /* the river: wide, pale blue, a darker blue edge, sharp angular bends (tape t560-564, t579-584) */
    { static const float RX[] = {-5.0e6f, -3.4e6f, -2.5e6f, -1.5e6f, -0.9e6f, -0.35e6f, -0.05e6f, 0.45e6f, 0.75e6f, 1.3e6f, 1.7e6f, 2.6e6f, 3.6e6f, 5.0e6f}, RY[] = {2.4e6f, 2.1e6f, 2.5e6f, 2.0e6f, 2.35e6f, 1.95e6f, 1.62e6f, 1.82e6f, 1.5e6f, 1.72e6f, 2.2e6f, 1.9e6f, 2.3e6f, 2.0e6f};
      const int NR = 14; const float W = 34000.f, B = 7000.f;
      for (int i = 0; i + 1 < NR; i++) { V a(RX[i], RY[i], 0), b(RX[i + 1], RY[i + 1], 0), d = norm(b - a), n(-d.y, d.x, 0);
          for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f; quad(m, a + n * (q * W) + V(0, 0, 300), a + n * (q * (W + B)) + V(0, 0, 300), b + n * (q * (W + B)) + V(0, 0, 300), b + n * (q * W) + V(0, 0, 300), 0x3060E0); }
          quad(m, a - n * W + V(0, 0, 400), a + n * W + V(0, 0, 400), b + n * W + V(0, 0, 400), b - n * W + V(0, 0, 400), 0xA8D8F8);
          oct8(m, a + V(0, 0, 420), V(1, 0, 0), V(0, 1, 0), W, 0xA8D8F8); } }
    /* the pale road out of the hangar and a side road (inset t1016-1017, tape t586); the dark strip with white edges is the landing ground (tape t579-584). Its length is INVENTED */
    quad(m, V(-8000, 800, 500), V(8000, 800, 500), V(8000, 330000, 500), V(-8000, 330000, 500), 0xD8E8C8);
    { V a(0, 190000, 520), b(1.5e6f, 300000, 520); V d = norm(b - a), n(-d.y, d.x, 0); quad(m, a - n * 3500.f, a + n * 3500.f, b + n * 3500.f, b - n * 3500.f, 0xD8E8C8); }
    quad(m, V(-15000, 330000, 500), V(15000, 330000, 500), V(15000, 2.6e6f, 500), V(-15000, 2.6e6f, 500), 0x1E7A30);
    for (int k = 0; k < 2; k++) { float x = k ? 15000.f : -15000.f; quad(m, V(x - 700, 330000, 620), V(x + 700, 330000, 620), V(x + 700, 2.6e6f, 620), V(x - 700, 2.6e6f, 620), 0xE8F0D8); }
    /* the ploughed brown fields with a row of white posts (t1016-1017) */
    quad(m, V(-460000, 170000, 450), V(-150000, 150000, 450), V(-130000, 340000, 450), V(-440000, 370000, 450), 0xA05A20);
    for (int i = 0; i < 6; i++) { float x = -440000.f + i * 50000.f; m.box(V(x - 400, 148000, 0), V(x + 400, 149000, 2600), 0xF0F0E8); }
    quad(m, V(620000, 900000, 450), V(1050000, 860000, 450), V(1100000, 1120000, 450), V(660000, 1180000, 450), 0xA05A20);
    /* the hangar: orange inside, two exits, blue signs edged white hung on chains, CADET left and ACE right (tape t526, inset t1012: blue; Octavius reads them purple, its hue shift); a green floor and green back wall
       (tape, t980); dark roof with a gable; lamps with black caps at the door tops (t977); red-brown outside (tape t586) */
    const uint32_t OT = 0xD88428, RB = 0xA8452E, RBD = 0x6E2E22, BG = 0x2F9C34, DK = 0x34404C;
    const float T = 800.f, HH = 9000.f, RG = 13500.f;
    m.box(V(-30000, 0, 0), V(-26000, T, HH), OT, C_ORANGE, C_ORANGE);
    m.box(V(-6000, 0, 0), V(6000, T, HH), OT, C_ORANGE, C_ORANGE);
    m.box(V(26000, 0, 0), V(30000, T, HH), OT, C_ORANGE, C_ORANGE);
    m.box(V(-26000, 0, 6500), V(-6000, T, HH), OT, C_ORANGE, C_ORANGE);
    m.box(V(6000, 0, 6500), V(26000, T, HH), OT, C_ORANGE, C_ORANGE);
    m.box(V(-30800, -30800, 0), V(-30000, T, HH), OT, C_ORANGE2, C_ORANGE2);
    m.box(V(30000, -30800, 0), V(30800, T, HH), OT, C_ORANGE2, C_ORANGE2);
    m.box(V(-30800, -30800, 0), V(30800, -30000, HH), OT, BG, BG);
    /* outside skin: red-brown */
    { const float YO = T + 8.f; static const float PX0[5] = {-30808, -6000, 26000, -26000, 6000}, PX1[5] = {-26000, 6000, 30808, -6000, 26000}, PZ0[5] = {0, 0, 0, 6500, 6500};
      for (int i = 0; i < 5; i++) quad(m, V(PX1[i], YO, PZ0[i]), V(PX0[i], YO, PZ0[i]), V(PX0[i], YO, HH), V(PX1[i], YO, HH), RB);
      quad(m, V(30808, -30808, 0), V(30808, YO, 0), V(30808, YO, HH), V(30808, -30808, HH), RB); quad(m, V(-30808, YO, 0), V(-30808, -30808, 0), V(-30808, -30808, HH), V(-30808, YO, HH), RB);
      quad(m, V(-30808, -30808, 0), V(30808, -30808, 0), V(30808, -30808, HH), V(-30808, -30808, HH), RB);
      /* gables, roof and ceiling */
      m.poly({V(-30800, YO, HH), V(30800, YO, HH), V(0, YO, RG)}, RB); m.poly({V(30800, -8, HH), V(-30800, -8, HH), V(0, -8, RG)}, C_ORANGE); m.box(V(-30800, -8, HH - 2), V(30800, 0, HH), OT);
      m.poly({V(30800, -30808, HH), V(-30800, -30808, HH), V(0, -30808, RG)}, RB); m.poly({V(-30800, -30000, HH), V(30800, -30000, HH), V(0, -30000, RG)}, BG);
      quad(m, V(-30808, -30808, HH), V(0, -30808, RG), V(0, YO + 300, RG), V(-30808, YO + 300, HH), RBD); quad(m, V(0, -30808, RG), V(30808, -30808, HH), V(30808, YO + 300, HH), V(0, YO + 300, RG), RBD);
      quad(m, V(-30800, YO, HH - 4), V(0, YO, RG - 4), V(0, -30800, RG - 4), V(-30800, -30800, HH - 4), DK); quad(m, V(0, YO, RG - 4), V(30800, YO, HH - 4), V(30800, -30800, HH - 4), V(0, -30800, RG - 4), DK); }
    for (int k = 0; k < 2; k++) { float q = k ? 1.f : -1.f, cx = q * 16000.f, hw = k ? 4500.f : 5600.f;
        m.box(V(cx - hw - 140, -700, 3760), V(cx + hw + 140, -320, 5840), 0xF0F0F0); m.box(V(cx - hw, -730, 3900), V(cx + hw, -300, 5700), 0x2A34C8, 0x2030A0, 0x2030A0);
        for (int c = 0; c < 2; c++) { float x = cx + (c ? hw - 500.f : -hw + 500.f); m.box(V(x - 40, -540, 5840), V(x + 40, -460, 6500), 0x202020); }
        m.octa(V(q * 8000, -500, 5000), 520, 900, 0xE8DC90); m.octa(V(q * 8000, -500, 5900), 300, 600, 0x141414); }
    text3d(m, "CADET", V(-16000 - 3190, -740, 4800 - 770), V(1, 0, 0), V(0, 0, 1), 220.f, 0xF0F0F0);
    text3d(m, "ACE", V(16000 - 1870, -740, 4800 - 770), V(1, 0, 0), V(0, 0, 1), 220.f, 0xF0F0F0);
    walls.clear();
    walls.push_back({-30000, -26000, -200, T}); walls.push_back({-6000, 6000, -200, T}); walls.push_back({26000, 30000, -200, T});
    walls.push_back({-31600, -30000, -31600, T}); walls.push_back({30000, 31600, -31600, T}); walls.push_back({-31600, 31600, -31600, -30000});
    /* the villages: tight groups of white houses with red roofs beside the road (t1016-1017), and the red barn (tape t554) */
    { const float vx[3] = {-150000.f, 1100000.f, -1600000.f}, vy[3] = {410000.f, 1300000.f, 900000.f};
      for (int v = 0; v < 3; v++) for (int k = 0; k < 8; k++) { float a = k * 0.8f + rnd(-0.2f, 0.2f), r = 12000.f + k * 5500.f;
          house(m, V(vx[v] + cosf(a) * r * 1.4f, vy[v] + sinf(a) * r, 0), rnd(-0.5f, 0.5f), rnd(7000.f, 10000.f), rnd(6000.f, 8000.f), 3600.f, 0xEFEFE8, 0xC02030); }
      house(m, V(250000.f, 620000.f, 0), 0.3f, 16000.f, 10000.f, 5500.f, 0xB03A2A, 0x7A2A20); }
    /* the railway: a bold black and white dashed line right across the valley behind the village (t1016-1017). The train runs out and back */
    { const float ex[5] = {-4.6e6f, -1.8e6f, 0.f, 1.9e6f, 4.6e6f}, ey[5] = {1.05e6f, 1.2e6f, 1.08e6f, 1.25e6f, 1.1e6f};
      trk.clear(); trk_s.clear(); for (int i = 0; i < 5; i++) trk.push_back(V(ex[i], ey[i], 100.f)); for (int i = 3; i >= 1; i--) trk.push_back(V(ex[i], ey[i], 100.f));
      float acc = 0; for (size_t i = 0; i < trk.size(); i++) { trk_s.push_back(acc); acc += len(trk[(i + 1) % trk.size()] - trk[i]); } trk_len = acc;
      for (int i = 0; i < 4; i++) { V a = trk[(size_t)i], b = trk[(size_t)i + 1], d = norm(b - a), n(-d.y, d.x, 0); float L = len(b - a); int cnt = (int)(L / 42000.f);
          for (int q = 0; q < cnt; q++) { V p0 = a + d * (q * 42000.f), p1 = p0 + d * 42000.f; uint32_t col = (q & 1) ? 0xF0F0E8 : 0x181818; quad(m, p0 - n * 2200.f + V(0, 0, 560), p0 + n * 2200.f + V(0, 0, 560), p1 + n * 2200.f + V(0, 0, 560), p1 - n * 2200.f + V(0, 0, 560), col); } } }
    m.add(A_WORLD);
    dl("world mesh: %zu polys", m.p.size());
    build_plane(A_PL_TAN,   {0xB09C5C, 0x8A7A46, 0xBCA868, 0x907E4C, 0x141414, true});   /* tan, black cross on a white square (t975.5, t1030) */
    build_plane(A_PL_RED,   {0xB8442A, 0x8A301E, 0xC8583A, 0x9A402C, 0x141414, true});   /* tape 9:57 "the red aircraft are part of a..." (cut, Baron's); no frame shows one */
    if (skin_brown) build_plane(A_PL_GREEN, {0x946B4A, 0x7A583C, 0x633931, 0x4E2C26, 0x2030C8, false}); else build_plane(A_PL_GREEN, {0x1C6428, 0x145020, 0x1E6E2A, 0x185A24, 0x2030C8, false});  /* green with the blue-white roundel: the Allies (tape t586, inset t1015-1017) */
    build_cockpit(); build_small();
}

/* ---- sounds: INVENTED placeholders, and the stand-in co-pilot ---- */
static bool mute_seat = false;   /* link play: another pilot's plane is being stepped, so it makes no sound here */
static void sfx(int ch, uint32_t key, int vol, int pan = 0) { if (mute_seat) return; buggy::play(ch, (int)key, vol, pan); }
static void say(int i, bool force = false) { if (mute_seat || !sound_on || (::voices[7].on && !force)) return; buggy::play(7, (int)(SK_V0 + i), 120); }
static void make_sounds() {
    std::mt19937 r(9); auto noise = [&r]() { return (r() / (float)(r.max)()) * 2.f - 1.f; };
    make_sound(SK_GUN, 0.12f, [&](float t) { return (noise() * 0.7f + sinf(6.2831853f * 160.f * t) * 0.4f) * expf(-t * 30.f); });
    make_sound(SK_HIT, 0.25f, [&](float t) { return (noise() * 0.5f + sinf(6.2831853f * 900.f * t) * 0.4f) * expf(-t * 18.f); });
    make_sound(SK_HURT, 0.5f, [&](float t) { return noise() * expf(-t * 7.f) * 0.8f + sinf(6.2831853f * 90.f * t) * expf(-t * 5.f) * 0.6f; });
    make_sound(SK_BOOM, 1.2f, [&](float t) { return noise() * expf(-t * 3.f) * 0.8f + sinf(6.2831853f * 50.f * t) * expf(-t * 2.5f) * 0.7f; });
    make_sound(SK_ENG, 0.4f, [&](float t) { float ph = fmodf(t * 90.f, 1.f); return ((ph - 0.5f) * 0.7f + sinf(6.2831853f * 45.f * t) * 0.25f + noise() * 0.06f) * 0.5f; });
    ::sounds[SK_ENG].loop = true;
    make_sound(SK_STALL, 0.4f, [&](float t) { return (fmodf(t, 0.2f) < 0.1f ? sinf(6.2831853f * 700.f * t) : 0.f) * 0.5f; });
    make_sound(SK_FIX, 0.7f, [&](float t) { return sinf(6.2831853f * (500.f + 500.f * t) * t) * expf(-t * 3.f) * 0.6f; });
    make_sound(SK_GO, 0.6f, [&](float t) { return sinf(6.2831853f * (440.f + 300.f * t) * t) * expf(-t * 3.f) * 0.6f; });
    make_sound(SK_LOCK, 0.15f, [&](float t) { return sinf(6.2831853f * 1200.f * t) * expf(-t * 12.f) * 0.5f; });
    for (int i = 0; i < V_COUNT; i++) { VSound v; v.rate = 11025; v.loop = false; v.pcm.resize((size_t)AC_VOICE[i].n); for (int q = 0; q < AC_VOICE[i].n; q++) v.pcm[q] = (int16_t)(AC_VOICE[i].d[q] * 200); ::sounds[SK_V0 + i] = std::move(v); ::sound_names[SK_V0 + i] = "aces_copilot"; }
}

/* ---- state ---- */
enum State { ST_TITLE, ST_HANGAR, ST_PLAY, ST_DEAD, ST_FADE, ST_OVER, ST_REPLAY, ST_DONE };
static State st = ST_TITLE; static float st_t = 0, time_left = 0; static double sim_t = 0;
static int score = 0; static float meter = 0, chat_t = 0; static int chat_step = 0, spawn_no = 0;
static bool ace_mode = false, foes_spawned = false;

struct Craft { V p; float yaw = 0, pitch = 0, bank = 0, speed = 0; M R; };
static void orient(Craft &c) {
    float cy = cosf(c.yaw), sy = sinf(c.yaw), cp = cosf(c.pitch), sp = sinf(c.pitch), cb = cosf(c.bank), sb = sinf(c.bank);
    V r0(cy, sy, 0), f0(-sy, cy, 0), z(0, 0, 1);
    V f = f0 * cp + z * sp, u = z * cp - f0 * sp;
    c.R.r[0] = r0 * cb - u * sb; c.R.r[1] = f; c.R.r[2] = u * cb + r0 * sb;
}
static float wrap_pi(float a) { while (a > PIF) a -= 2.f * PIF; while (a < -PIF) a += 2.f * PIF; return a; }
static V vel_of(const Craft &c) { return c.R.r[1] * (c.speed * MM); }

struct Pilot { Craft c; float lever = 0, invuln = 0, repair_t = 0, fire_cool = 0, shake = 0, fall_t = 0, jx = 0, jy = 0;
               bool ground = true, stalled = false, fall = false, wreck = false; int hits = 0, lives = 2; };
static Pilot pl;
struct Foe { Craft c; int hits = 0, style = 0; bool alive = true, gone = false; float fire_cool = 1.f, evade = 0, dead_t = 0, phase = 0, orbit = 0, bank_dir = 1, smoke = 0, wait = 0; };
static std::vector<Foe> foes; static int foes_down = 0;
static const V ZC[2] = {V(0, 1800000.f, 330000.f), V(-1200000.f, 2700000.f, 400000.f)};   /* where the zeppelins circle: 2-3 km north of the hangar, about a minute away; heights and speeds invented */
struct GT { int type = 0; V p; float yaw = 0; int hits = 0; bool alive = true; int car = -1; float down = 0; };   /* type 2 locomotive, 3 truck, 4 barrage balloon, 5 Zeppelin */
static std::vector<GT> gts; static std::vector<V> scorch;
struct Bul { V p, v; int own; float t; };
static std::vector<Bul> buls;
struct Spark { V p, v; float life; int id; };
static std::vector<Spark> sparks;
struct Ctl { float jx = 0, jy = 0, lever_rate = 0, lever_set = -1.f; bool fire = false; };
static float train_s = 0;
static int lock_idx = -1; static V aim_dir(0, 1, 0), eye_now;
static V head_ref; static float yaw0 = 0; static bool have_ref = false;
static float hit_r = 5000.f;
/* the replay of the last 20 s from above: Sega Force 16 has it on a debriefing console outside the pods; the look is from the promo's chase shots t1016-1017 */
struct RecCraft { V p; float yaw, pitch, bank; int id; }; struct RecFrame { RecCraft c[9]; int n; };
static std::vector<RecFrame> rec; static float rec_t = 0;

/* ---- link play (vpix_1k_net.h) ----------------------------------------------------------------------------------------------------------
   INVENTED (the original is one player): a 4 pilot dogfight round the zeppelins and balloons. Four planes in one sky, each a person or a CPU pilot; guns hurt every other plane, a plane
   shot down scores 200 for the shooter, balloons, zeppelins and the train still pay and come back after 15 s. No hangar and no CPU enemy biplanes in a linked round: a crashed or shot
   down pilot is back in the air 3.5 s later. Pod 1 runs everything; another pod sends its sticks and draws the state. A pod joins a running round with no wait: until a seat comes it
   flies the same dogfight alone against CPU pilots. V1K_NET=0 (or no network) is the single player game, untouched. */
static Pilot spl[4];                                            /* seat i != me; the seat that is me lives in pl, so every old pl call is still the pilot at the controls */
static bool link_round = false, link_slave = false;             /* link_slave: inside the master's round (state comes by snapshot); else this PC runs the round itself */
static int me = 0, link_credit = 0, lscore[4]; static float link_rt[4], cpu_evade[4], cpu_orbit[4], my_lever = 0.6f;
static Pilot &seat_pl(int i) { return i == me ? pl : spl[i]; }
template <class F> static void as_seat(int j, F fn) {          /* run fn with seat j in pl (a pilot's own functions), quietly */
    if (j == me) { fn(); return; }
    const State s0 = st; const bool m0 = mute_seat; st = ST_PLAY; mute_seat = true; std::swap(pl, spl[j]); fn(); std::swap(pl, spl[j]); st = s0; mute_seat = m0;
}

static float pop_t = 0; static int pop_pts = 0;
static void add_points(int n) {
    if (link_round) { lscore[link_credit & 3] += n; if ((link_credit & 3) == me) { pop_t = 1.5f; pop_pts = n; } return; }
    score += n; meter += n; pop_t = 1.5f; pop_pts = n;   /* the number stays 1.5 s (t988.7-990.2) */
    while (meter >= bonus_pts) { meter -= bonus_pts; time_left += bonus_secs; sfx(5, SK_FIX, 100); dl("t=%.1f past the BONUS TIME line: +%.0f s", sim_t, bonus_secs); }
}
static void emit(V at, int n, int id, float speed, float life, float lift = 0.f) {
    for (int i = 0; i < n; i++) { if (sparks.size() > 400) break;
        V d = norm(V(rnd(-1, 1), rnd(-1, 1), rnd(-0.4f, 1.2f)));
        Spark s; s.p = at; s.v = d * (speed * rnd(0.3f, 1.f)) + V(0, 0, lift); s.life = life * rnd(0.6f, 1.f); s.id = id; sparks.push_back(s); }
}
static bool seg_hit(V a, V b, V c, float r) {
    V ab = b - a; float l2 = dot(ab, ab), t = l2 > 1e-6f ? clampf(dot(c - a, ab) / l2, 0.f, 1.f) : 0.f;
    return len(c - (a + ab * t)) < r;
}

/* ---- the player's plane ---- */
static void player_crash(const char *why) {
    if (pl.wreck) return;
    pl.wreck = true; pl.fall = false;
    emit(pl.c.p, 60, A_SPK_O, 16000.f, 2.5f); emit(pl.c.p, 30, A_SPK_R, 12000.f, 2.f); emit(pl.c.p, 20, A_SMOKE, 6000.f, 3.f, 3000.f);
    sfx(3, SK_BOOM, 127); VPIX_Haptic(2, 0.5f, 1.f);
    st = ST_DEAD; st_t = 0;
    dl("t=%.1f player destroyed (%s)", sim_t, why);
}
static void player_hit() {
    if (pl.invuln > 0 || pl.fall || pl.wreck || st != ST_PLAY) return;
    pl.hits++; pl.invuln = invuln_secs; pl.shake = 0.5f; sfx(2, SK_HURT, 120); VPIX_Haptic(2, 0.2f, 0.8f); say(V_HIT, true); chat_t = (std::max)(chat_t, 2.f);
    dl("t=%.1f player hit %d (down after %d)", sim_t, pl.hits, hits_survive + 1);
    if (pl.hits > hits_survive) { pl.fall = true; pl.fall_t = 0; pl.stalled = false; dl("t=%.1f player shot down", sim_t); }
}
static void wall_bump(Craft &c) {   /* on the ground it stops you; in the air it slews you round along the wall, so a plane never hangs on it. Mine: no footage shows a wall hit */
    if (pl.ground) c.speed *= 0.5f; else { c.yaw += 0.02f; c.speed = (std::max)(c.speed * 0.995f, v_stall + 2.f); }
}
static void hangar_collide(Craft &c) {   /* a bump, never a crash: Octavius could not get it to smash into the scenery (16:58) */
    const float RAD = 5300.f, WT = 1900.f;   /* the farthest point of the plane from its origin is the wing tip, 5204 mm, plus a margin; top of the upper wing above the origin */
    if (c.p.z - GEAR > ROOF_Z - 300.f) return;   /* higher up the roof rule in step_player lifts it over */
    const bool in_box = fabsf(c.p.x) < HANGAR_X && c.p.y > HANGAR_Y0 && c.p.y < -200.f;
    if (in_box && c.p.z + WT > 8900.f) { c.p.z = 8900.f - WT; c.pitch = (std::min)(c.pitch, 0.f); }   /* inside: under the ceiling */
    if (c.p.z + WT > SIGN_Z && fabsf(c.p.x) < HANGAR_X + RAD && c.p.y > -800.f - RAD && c.p.y < T_FRONT + RAD) {   /* too tall for the doors: the whole front is a wall */
        if (c.p.y < -300.f) c.p.y = -800.f - RAD; else c.p.y = T_FRONT + RAD;
        wall_bump(c); }
    for (const Rect &r : walls) {
        float cx = clampf(c.p.x, r.x0, r.x1), cy = clampf(c.p.y, r.y0, r.y1), dx = c.p.x - cx, dy = c.p.y - cy, d = sqrtf(dx * dx + dy * dy);
        if (d < RAD) {
            if (d > 1.f) { c.p.x = cx + dx / d * RAD; c.p.y = cy + dy / d * RAD; } else c.p.y = r.y0 - RAD;
            wall_bump(c);
        }
    }
}
/* how far outside a zeppelin (capsule along its length) or a balloon (sphere) a point is, mm; negative = inside. Sizes off the models */
static float air_gap(const GT &g, const V &p) {
    if (g.type == 4) return len(p - g.p) - 20000.f;
    V ax(-sinf(g.yaw), cosf(g.yaw), 0); float t = clampf(dot(p - g.p, ax), -28000.f, 28000.f); return len(p - (g.p + ax * t)) - 7400.f;
}
static bool over_hangar(const V &p, float pad) { return fabsf(p.x) < HANGAR_X + pad && p.y > HANGAR_Y0 - pad && p.y < T_FRONT + pad && p.z < HANGAR_TOP; }
static void step_player(const Ctl &k, float dt) {
    Craft &c = pl.c;
    pl.invuln = (std::max)(0.f, pl.invuln - dt); pl.shake = (std::max)(0.f, pl.shake - dt); pl.fire_cool -= dt;
    if (pl.wreck) return;
    if (pl.fall) {   /* shot down: nose over and spin to the ground */
        pl.fall_t += dt; c.pitch = (std::max)(c.pitch - 0.5f * dt, -0.9f); c.bank += 1.6f * dt; c.speed = (std::min)(c.speed + 6.f * dt, 55.f);
        orient(c); c.p += c.R.r[1] * (c.speed * MM * dt);
        if (c.p.z - GEAR <= GROUND_Z || over_hangar(c.p, 4800.f)) player_crash("shot down");
        return;
    }
    { float a = clampf(dt / ctl_lag, 0.f, 1.f); pl.jx += (k.jx - pl.jx) * a; pl.jy += (k.jy - pl.jy) * a; }
    if (k.lever_set >= 0) pl.lever = clampf(k.lever_set, 0.f, 1.f); else pl.lever = clampf(pl.lever + k.lever_rate * dt, 0.f, 1.f);
    float alt_m = (c.p.z - GEAR - GROUND_Z) / MM, thr = pl.lever * thrust_acc;
    if (alt_m > 700.f) thr *= (std::max)(0.f, 1.f - (alt_m - 700.f) / 200.f);   /* ceiling */
    float drag = DRAG_K * c.speed * c.speed;
    if (pl.ground) {
        float fr = c.speed > 0.05f ? 0.5f : 0.f; if (pl.lever < 0.05f) fr += 4.5f;   /* rolling friction; brakes at idle */
        c.speed = (std::max)(0.f, c.speed + (thr - fr - drag) * dt);
        c.bank = 0;
        if (c.speed >= v_lift) { c.pitch += -pl.jy * pitch_rate * dt; if (c.pitch > 0.08f) pl.ground = false; }
        else c.pitch += (0.05f - c.pitch) * 3.f * dt;
        c.pitch = clampf(c.pitch, 0.f, 0.6f);
        c.yaw += -pl.jx * taxi_turn * clampf(c.speed / 4.f, 0.35f, 1.f) * dt;
        orient(c); c.p += c.R.r[1] * (c.speed * MM * dt);
        if (pl.ground) c.p.z = GROUND_Z + GEAR;
        hangar_collide(c);
        if (pl.hits > 0 && st == ST_PLAY) {   /* tape 9:48-9:57: repairs are done if you stay in contact with the ground for more than 2 s; nothing says you must stop */
            pl.repair_t += dt;
            if (pl.repair_t > repair_secs) { pl.hits = 0; pl.repair_t = 0; sfx(5, SK_FIX, 110); say(V_FIXED, true); dl("t=%.1f plane repaired", sim_t); }
        } else pl.repair_t = 0;
        return;
    }
    pl.repair_t = 0;
    float auth = pl.stalled ? 0.25f : 1.f;
    c.bank += clampf((pl.jx * max_bank - c.bank) * 2.5f, -1.8f, 1.8f) * auth * dt;
    if (pl.stalled) c.bank += (c.bank >= 0 ? 0.4f : -0.4f) * dt;
    c.bank = clampf(c.bank, -1.4f, 1.4f);
    c.yaw += clampf(-GRAVITY * tanf(clampf(c.bank, -1.2f, 1.2f)) / (std::max)(c.speed, 18.f) * 1.2f, -1.1f, 1.1f) * dt;
    c.pitch += -pl.jy * pitch_rate * auth * dt;
    if (!pl.stalled && c.speed < v_stall) { pl.stalled = true; sfx(4, SK_STALL, 110); say(V_STALL, true); dl("t=%.1f stall at %.1f m/s", sim_t, c.speed); }
    else if (pl.stalled && c.speed > v_stall + 4.f) pl.stalled = false;
    if (pl.stalled) c.pitch -= 0.9f * dt;
    c.pitch = clampf(c.pitch, -1.25f, 1.25f);
    float ind = 3.f * (1.f / (std::max)(cosf(c.bank), 0.35f) - 1.f);
    c.speed = clampf(c.speed + (thr - drag - ind - GRAVITY * sinf(c.pitch) * 0.9f) * dt, 0.f, 75.f);
    orient(c); c.p += c.R.r[1] * (c.speed * MM * dt);
    { float r = sqrtf(c.p.x * c.p.x + c.p.y * c.p.y);   /* the edge of the map: turned back toward the airfield */
      if (r > WORLD_R) c.yaw += clampf(wrap_pi(atan2f(c.p.x, -c.p.y) - c.yaw), -0.6f, 0.6f) * dt; }
    for (const GT &g : gts) if (g.alive && g.type >= 4 && air_gap(g, c.p) < 4800.f) { player_crash(g.type == 5 ? "flew into a zeppelin" : "flew into a balloon"); return; }   /* Bobby's pick 11-03; nothing in footage or press */
    hangar_collide(c);
    if (fabsf(c.p.x) < HANGAR_X + 5300.f && c.p.y > HANGAR_Y0 - 5300.f && c.p.y < T_FRONT + 5300.f && c.p.z - GEAR < ROOF_Z + 4000.f && c.p.z - GEAR > ROOF_Z - 300.f) { c.p.z = ROOF_Z + 4000.f + GEAR; c.pitch = (std::max)(c.pitch, 0.1f); }   /* over the roof, never onto it */
    if (c.p.z - GEAR <= GROUND_Z) {   /* any touch of the ground is a landing: Sega Force "Landing's easy"; Octavius could not make it crash */
        float sink = -c.speed * sinf(c.pitch);
        pl.ground = true; c.p.z = GROUND_Z + GEAR; c.pitch = clampf(c.pitch, 0.f, 0.1f); c.bank = 0; pl.stalled = false; dl("t=%.1f touchdown, sink %.1f m/s, speed %.1f m/s", sim_t, sink, c.speed);
    }
}

/* ---- the enemy ---- */
static void steer(Craft &c, V target, float want_speed, float agility, float dt) {
    V d = target - c.p; float hd = sqrtf(d.x * d.x + d.y * d.y) + 1.f;
    float want_yaw = atan2f(-d.x, d.y), dy = wrap_pi(want_yaw - c.yaw);
    float want_bank = clampf(-dy * 2.2f, -1.f, 1.f) * 1.05f * agility;
    c.bank += clampf((want_bank - c.bank) * 2.5f, -1.8f, 1.8f) * dt;
    c.yaw += clampf(-GRAVITY * tanf(clampf(c.bank, -1.2f, 1.2f)) / (std::max)(c.speed, 18.f) * 1.2f, -1.1f, 1.1f) * dt;
    float want_p = clampf(atan2f(d.z, hd), -0.55f, 0.55f), alt = c.p.z - GEAR;
    if (alt < 90000.f) want_p = (std::max)(want_p, 0.15f);
    if (alt < 40000.f) want_p = (std::max)(want_p, 0.4f);
    c.pitch += clampf((want_p - c.pitch) * 1.6f, -0.7f, 0.7f) * dt;
    c.speed = clampf(c.speed + ((want_speed - c.speed) * 0.6f - GRAVITY * sinf(c.pitch) * 0.6f) * dt, 20.f, 60.f);
    orient(c); c.p += c.R.r[1] * (c.speed * MM * dt);
}
static void kill_foe(Foe &e) {
    e.alive = false; e.dead_t = 0; e.bank_dir = rnd(0, 1) < 0.5f ? -1.f : 1.f; foes_down++; add_points(e.style ? pts_red : pts_foe);
    emit(e.c.p, 26, A_SPK_D, 9000.f, 1.4f); emit(e.c.p, 12, A_SPK_R, 7000.f, 0.8f); emit(e.c.p, 8, A_SPK_Y, 6000.f, 0.6f);   /* a burst of dark specks with a little red and yellow (t989-990, t1015) */
    sfx(3, SK_BOOM, 110); say(V_GOOD);
    dl("t=%.1f enemy %d shot down (%d down, score %d)", sim_t, (int)(&e - foes.data()), foes_down, score);
}
static void spawn_one(Foe &e, int i, float a) {
    e = Foe(); e.c.p = ZC[(size_t)i % 2] + V(cosf(a) * 700000.f, sinf(a) * 700000.f, rnd(-80000.f, 80000.f)); e.c.speed = 36.f; e.c.yaw = a + PIF * 0.5f; e.style = (++spawn_no % 3 == 0) ? 1 : 0; e.phase = a; e.orbit = 0; e.fire_cool = 3.f + i; orient(e.c);
}
static void foe_step(Foe &e, float dt, bool attack) {
    Craft &c = e.c;
    if (!e.alive) {   /* going down in a spin and a trail of smoke */
        e.dead_t += dt; c.pitch = (std::max)(c.pitch - 0.6f * dt, -1.0f); c.bank += 2.2f * dt * e.bank_dir; c.speed = (std::min)(c.speed + 8.f * dt, 60.f);
        orient(c); c.p += c.R.r[1] * (c.speed * MM * dt);
        e.smoke -= dt; if (e.smoke <= 0) { e.smoke = 0.08f; emit(c.p, 2, A_SMOKE, 1500.f, 1.5f); emit(c.p, 1, A_SPK_O, 2500.f, 0.6f); }
        if (c.p.z < GEAR || e.dead_t > 6.f || over_hangar(c.p, 4800.f)) { emit(c.p, 40, A_SPK_O, 10000.f, 1.8f); emit(c.p, 14, A_SMOKE, 5000.f, 3.f, 3000.f); sfx(3, SK_BOOM, 80); e.gone = true; }
        return;
    }
    float sk = ace_mode ? ace_skill : cadet_skill;
    V tp = pl.c.p, d = tp - c.p; float dist = len(d);
    V target;
    /* the air battle is round the zeppelins (Bobby 11-03, ACES_PIC2 has one in the fight; numbers mine): they weave round the nearer one until you are within 900 m, and keep clear of it */
    const V &hub = ZC[(size_t)(&e - foes.data()) % 2];
    if (attack && dist < 900000.f) { float t = (std::min)(dist / (c.speed * MM + 1000.f), 3.f) * 0.5f; target = tp + vel_of(pl.c) * t; }
    else { e.orbit += dt * 0.09f; const float r = 500000.f + 260000.f * sinf(e.phase * 3.f + e.orbit * 2.f); target = hub + V(cosf(e.orbit + e.phase) * r, sinf(e.orbit + e.phase) * r, 110000.f * sinf(e.orbit * 3.f + e.phase)); }
    if (e.evade > 0) { e.evade -= dt; target = c.p + c.R.r[1] * 400000.f + V(0, 0, 60000.f); }
    else if (attack && dist < 30000.f) e.evade = 1.5f;
    if (sqrtf(c.p.x * c.p.x + c.p.y * c.p.y) > WORLD_R * 0.9f) target = V(0, 0, 250000.f);
    if (e.hits > 0) { e.smoke -= dt; if (e.smoke <= 0) { e.smoke = 0.2f; emit(c.p, 1, A_SMOKE, 1200.f, 1.2f); } }
    for (const GT &g : gts) if (g.alive && g.type >= 4) { const float ga = (std::min)(air_gap(g, c.p), air_gap(g, c.p + c.R.r[1] * 40000.f)); if (ga < 40000.f) { V aw = c.p - g.p; aw.z = fabsf(aw.z) + 20000.f; target = c.p + norm(aw) * 300000.f; } }
    if (over_hangar(c.p + c.R.r[1] * 60000.f, 12000.f) || over_hangar(c.p, 12000.f)) target = c.p + c.R.r[1] * 200000.f + V(0, 0, 200000.f);   /* climb over the hangar */
    steer(c, target, 34.f + 10.f * sk, 0.55f + 0.5f * sk, dt);
    if (over_hangar(c.p, 6000.f)) { c.p.z = HANGAR_TOP; c.pitch = (std::max)(c.pitch, 0.3f); orient(c); }
    e.fire_cool -= dt;
    if (attack && e.fire_cool <= 0 && dist < 700000.f && dist > 60000.f) {
        V to = norm(d); float ca = dot(c.R.r[1], to), cone = cosf(ace_mode ? 0.12f : 0.07f);
        if (ca > cone) {
            float sp = (1.f - sk) * 0.03f;
            for (int i = 0; i < 3; i++) { V dir = norm(c.R.r[1] + V(rnd(-sp, sp), rnd(-sp, sp), rnd(-sp, sp)) + to * (0.25f * sk));
                Bul b; b.p = c.p + c.R.r[1] * 2500.f + dir * (i * 4000.f); b.v = vel_of(c) * 0.3f + dir * (300.f * MM); b.own = 1; b.t = 0; buls.push_back(b); }
            e.fire_cool = ace_mode ? 0.9f : 2.0f; e.fire_cool *= rnd(0.8f, 1.3f);
            if (dist < 400000.f) sfx(1, SK_GUN, 60);
        }
    }
}
static void spawn_foes() {
    int n = foes_override > 0 ? foes_override : (ace_mode ? foes_ace : foes_cadet);
    foes.assign((size_t)n, Foe()); foes_down = 0; spawn_no = 0;
    for (int i = 0; i < n; i++) spawn_one(foes[i], i, 6.2831853f * i / n + 0.7f);
    dl("t=%.1f %d enemy biplanes (%s)", sim_t, n, ace_mode ? "ACE" : "CADET");
}

/* ---- ground targets, and what hangs in the air ---- */
static void step_train(float dt);
static void build_targets() {
    gts.clear(); scorch.clear();
    for (int i = 0; i < 5; i++) { GT g; g.type = i == 0 ? 2 : 3; g.car = i; gts.push_back(g); }
    { GT g; g.type = 4; g.p = V(-600000, 2600000, 420000); gts.push_back(g); g.p = V(520000, 2650000, 440000); gts.push_back(g); }
    for (int i = 0; i < 2; i++) { GT g; g.type = 5; g.car = -1 - i; gts.push_back(g); }   /* two: Bobby's memory of "the zeppelins"; the footage shows one at a time (t1015, ACES_PIC2) */
    step_train(0.f);
}
static void step_train(float dt) {
    train_s += train_speed * MM * dt;
    for (GT &g : gts) {
        if (g.car >= 0) { float yw; g.p = trk_at(train_s - g.car * 11000.f, yw); g.yaw = yw; g.p.z = GROUND_Z + 560.f; }
        else if (g.type == 5) { const int i = -1 - g.car; float a = (float)sim_t * 12000.f / 500000.f + i * 2.f; g.p = ZC[i] + V(cosf(a) * 500000.f, sinf(a) * 500000.f, 0); g.yaw = a; }
    }
}
static void kill_gt(GT &g) {
    g.alive = false; if (g.type < 4) scorch.push_back(g.p); add_points(g.type == 5 ? pts_gt * 3 : pts_gt);
    if (g.type >= 4) { emit(g.p, 70, A_SPK_O, 16000.f, 2.4f); emit(g.p, 40, A_SPK_D, 14000.f, 2.4f); emit(g.p, 30, A_SPK_R, 12000.f, 2.f); emit(g.p, 20, A_SMOKE, 6000.f, 4.f, 1500.f); }   /* burst: mine; t1015 shows only the hits */
    emit(g.p + V(0, 0, 2000), 40, A_SPK_O, 8000.f, 1.8f); emit(g.p + V(0, 0, 2000), 14, A_SPK_R, 6000.f, 1.2f); emit(g.p + V(0, 0, 2000), 16, A_SMOKE, 3000.f, 3.f, 2500.f);
    sfx(3, SK_BOOM, 100); say(V_GOOD);
    dl("t=%.1f target %d (type %d) destroyed", sim_t, (int)(&g - gts.data()), g.type);
}
static void hit_seat(int j, int s) {   /* a bullet of seat s struck seat j */
    const bool was = seat_pl(j).fall;
    as_seat(j, [] { player_hit(); });
    if (!was && seat_pl(j).fall) { lscore[s & 3] += pts_foe; dl("t=%.1f link: seat %d shot down seat %d", sim_t, s, j); if (s == me) { pop_t = 1.5f; pop_pts = pts_foe; sfx(3, SK_BOOM, 90); } }
}
static void step_bullets(float dt) {
    for (size_t i = 0; i < buls.size();) {
        Bul &b = buls[i]; V prev = b.p; b.p += b.v * dt; b.t += dt; bool gone = false;
        if (b.t > 2.2f) gone = true;
        else if (b.p.z < GROUND_Z) { if (b.own == 0) emit(b.p, 3, A_SPK_D, 2500.f, 0.5f, 2000.f); gone = true; }
        if (!gone && (b.own == 0 || b.own >= 10)) {
            if (b.own >= 10) link_credit = b.own - 10;
            for (Foe &e : foes) { if (!e.alive || e.gone) continue;
                if (seg_hit(prev, b.p, e.c.p, hit_r)) { e.hits++; emit(b.p, 8, A_SPK_D, 4000.f, 0.6f); sfx(1, SK_HIT, 90);
                    if (e.hits >= foe_hits) kill_foe(e); gone = true; break; } }
            if (!gone) for (GT &g : gts) { if (!g.alive) continue;
                float r = g.type == 2 ? 5500.f : g.type == 3 ? 5000.f : g.type == 4 ? 18000.f : 25000.f;
                if (seg_hit(prev, b.p, g.p + V(0, 0, g.type < 4 ? 2200.f : 0.f), r)) { g.hits++; emit(b.p, 6, A_SPK_R, 3000.f, 0.5f); sfx(1, SK_HIT, 80);
                    if (g.hits >= (g.type == 5 ? gt_hits * 3 : gt_hits)) kill_gt(g); gone = true; break; } }
            if (!gone && b.own >= 10) for (int j = 0; j < 4; j++) { if (j == b.own - 10) continue; const Pilot &q = seat_pl(j); if (q.wreck || q.fall) continue;
                if (seg_hit(prev, b.p, q.c.p, 4200.f)) { emit(b.p, 8, A_SPK_D, 4000.f, 0.6f); hit_seat(j, b.own - 10); gone = true; break; } }
        } else if (!gone && b.own == 1 && st == ST_PLAY && !pl.wreck && !pl.fall) {
            if (seg_hit(prev, b.p, pl.c.p, 4200.f)) { player_hit(); gone = true; }
        }
        if (gone) { buls[i] = buls.back(); buls.pop_back(); } else i++;
    }
    for (size_t i = 0; i < sparks.size();) { Spark &k = sparks[i]; k.v.z -= 3000.f * dt; k.p += k.v * dt; k.life -= dt;
        if (k.life <= 0) { sparks[i] = sparks.back(); sparks.pop_back(); } else i++; }
}
/* the co-pilot "chatters non-stop", warns of enemies on your tail, moans, helps (Sega Force 16; Octavius "the entire time"); Cadet gets the instructions (tape 8:44). Timing and words are mine */
static void chatter(float dt) {
    chat_t -= dt; if (chat_t > 0 || ::voices[7].on) return;
    chat_t = rnd(5.f, 9.f); const Craft &c = pl.c; int line = -1; float best = 500000.f; const Foe *nf = nullptr;
    for (const Foe &e : foes) if (e.alive && !e.gone) { float d = len(e.c.p - c.p); if (d < best) { best = d; nf = &e; } }
    if (nf) { V to = norm(nf->c.p - c.p); float b = dot(to, c.R.r[1]), r = dot(to, c.R.r[0]);
        if (b < -0.6f) line = rnd(0, 1) < 0.5f ? V_TAIL : V_BEHIND; else if (b < 0.5f) line = r > 0 ? V_RIGHT : V_LEFT; }
    if (line < 0 && pl.hits > 0) line = V_LAND;
    if (line < 0 && !pl.ground && c.speed < v_stall + 5.f) line = V_SPEED;
    static const int STEPS[4] = {V_SPEED, V_CLIMB, V_BANK, V_GUNS};
    if (line < 0 && !ace_mode && chat_step < 4) line = STEPS[chat_step++];
    if (line < 0) line = rnd(0, 1) < 0.5f ? V_MOAN : V_MOAN2;
    say(line);
}

/* ---- the sight: the ring is fixed to the plane and the guns fire through it; the head only looks ---- */
static void update_lock() {
    lock_idx = -1; float best = cosf(lock_deg * PIF / 180.f);
    for (size_t i = 0; i < foes.size(); i++) { const Foe &e = foes[i]; if (!e.alive || e.gone) continue;
        V d = e.c.p - eye_now; float r = len(d); if (r > lock_range || r < 1000.f) continue;
        float ca = dot(norm(d), aim_dir); if (ca > best) { best = ca; lock_idx = (int)i; } }
}
static double fired_t = -9.0;
static void gun_fire() {
    fired_t = sim_t;
    V dir = aim_dir;
    if (lock_idx >= 0) {   /* the lock leads the target a little for you */
        const Foe &e = foes[(size_t)lock_idx]; V vp = vel_of(pl.c), vf = vel_of(e.c), rel = e.c.p - pl.c.p; float bs = bullet_speed * MM, t = len(rel) / bs;
        for (int it = 0; it < 2; it++) { V want = rel + (vf - vp) * t; t = len(want) / bs; }
        V lead = norm(rel + (vf - vp) * t); dir = norm(dir * (1.f - assist) + lead * assist);
    }
    dir = norm(dir + V(rnd(-0.004f, 0.004f), rnd(-0.004f, 0.004f), rnd(-0.004f, 0.004f)));
    Bul b; b.p = pl.c.p + to_world(pl.c.R, V(0, 800, 300)); b.v = vel_of(pl.c) + dir * (bullet_speed * MM); b.own = link_round ? 10 + me : 0; b.t = 0; buls.push_back(b);
    sfx(0, SK_GUN, 100);
}

/* ---- link play: the round, the CPU pilots, the snapshot ---- */
#pragma pack(push, 1)
struct NetIn { float jx, jy, lever; uint8_t fire; };
struct NetCraft { float p[3], yaw, pitch, bank, speed; uint8_t flags, hits; int16_t score; };
struct NetBul { float p[3], v[3]; };
enum { NB_MAX = 40 };
struct NetSt { uint8_t n, pad; float time_left, train_s; uint16_t gt_alive; float zep[2][3]; NetCraft c[4]; uint8_t nb; NetBul b[NB_MAX]; };
#pragma pack(pop)
static void respawn_seat(int i) {   /* back in the air round the first zeppelin, spread out and flying round it */
    Pilot &p = seat_pl(i); p = Pilot(); link_rt[i] = 0; cpu_evade[i] = 0;
    const float a = (float)i * 1.5708f + rnd(-0.3f, 0.3f);
    p.c = Craft(); p.c.p = ZC[0] + V(cosf(a) * 650000.f, sinf(a) * 650000.f, rnd(-60000.f, 60000.f)); p.c.yaw = a + PIF * 0.5f; p.c.speed = 38.f; p.c.pitch = 0.f; orient(p.c);
    p.lever = 0.6f; p.ground = false; p.lives = lives_total; p.invuln = 2.f;
    if (i == me) { st = ST_PLAY; my_lever = 0.6f; }
}
static void seat_fire(int i) {
    if (i == me) { gun_fire(); return; }
    Pilot &p = seat_pl(i); V dir = norm(p.c.R.r[1] + V(rnd(-0.004f, 0.004f), rnd(-0.004f, 0.004f), rnd(-0.004f, 0.004f)));
    Bul b; b.p = p.c.p + to_world(p.c.R, V(0, 800, 300)); b.v = vel_of(p.c) + dir * (bullet_speed * MM); b.own = 10 + i; b.t = 0; buls.push_back(b);
    sfx(1, SK_GUN, 50);
}
static void cpu_seat(int i, float dt) {   /* a CPU pilot: the nearest other plane is the prey, else it weaves round a zeppelin */
    Pilot &p = seat_pl(i); Craft &c = p.c;
    p.invuln = (std::max)(0.f, p.invuln - dt); p.fire_cool -= dt;
    if (p.fall) { as_seat(i, [&] { step_player(Ctl(), dt); }); return; }
    int best = -1; float bd = 1e30f;
    for (int j = 0; j < 4; j++) { if (j == i) continue; const Pilot &q = seat_pl(j); if (q.wreck || q.fall) continue; const float d = len(q.c.p - c.p); if (d < bd) { bd = d; best = j; } }
    const bool attack = best >= 0 && bd < 900000.f; const float sk = 0.7f;
    V target; const V &hub = ZC[i % 2];
    if (attack) { const Craft &q = seat_pl(best).c; const float t = (std::min)(bd / (c.speed * MM + 1000.f), 3.f) * 0.5f; target = q.p + vel_of(q) * t; }
    else { cpu_orbit[i] += dt * 0.09f; const float r = 500000.f + 260000.f * sinf(i * 1.7f + cpu_orbit[i] * 2.f), a = cpu_orbit[i] + i * 1.5708f; target = hub + V(cosf(a) * r, sinf(a) * r, 110000.f * sinf(cpu_orbit[i] * 3.f + i)); }
    if (cpu_evade[i] > 0) { cpu_evade[i] -= dt; target = c.p + c.R.r[1] * 400000.f + V(0, 0, 60000.f); }
    else if (attack && bd < 30000.f) cpu_evade[i] = 1.5f;
    if (sqrtf(c.p.x * c.p.x + c.p.y * c.p.y) > WORLD_R * 0.9f) target = V(0, 0, 250000.f);
    for (const GT &g : gts) if (g.alive && g.type >= 4) { const float ga = (std::min)(air_gap(g, c.p), air_gap(g, c.p + c.R.r[1] * 40000.f)); if (ga < 40000.f) { V aw = c.p - g.p; aw.z = fabsf(aw.z) + 20000.f; target = c.p + norm(aw) * 300000.f; } }
    if (over_hangar(c.p + c.R.r[1] * 60000.f, 12000.f) || over_hangar(c.p, 12000.f)) target = c.p + c.R.r[1] * 200000.f + V(0, 0, 200000.f);
    steer(c, target, 34.f + 10.f * sk, 0.55f + 0.5f * sk, dt); p.lever = 0.7f;
    if (attack && p.fire_cool <= 0 && bd < 700000.f && bd > 60000.f) {
        const V to = norm(seat_pl(best).c.p - c.p);
        if (dot(c.R.r[1], to) > cosf(0.09f)) { const float sp2 = (1.f - sk) * 0.03f;
            for (int k = 0; k < 3; k++) { V dir = norm(c.R.r[1] + V(rnd(-sp2, sp2), rnd(-sp2, sp2), rnd(-sp2, sp2)) + to * (0.25f * sk));
                Bul b; b.p = c.p + c.R.r[1] * 2500.f + dir * (k * 4000.f); b.v = vel_of(c) * 0.3f + dir * (300.f * MM); b.own = 10 + i; b.t = 0; buls.push_back(b); }
            p.fire_cool = 0.9f * rnd(0.8f, 1.3f); if (bd < 400000.f) sfx(1, SK_GUN, 60); }
    }
}
static void link_start() {   /* pod 1, or a pod flying alone until its seat comes */
    link_round = true; link_slave = false; me = 0; link_credit = 0; mute_seat = false;
    if (v1knet::on && v1knet::master) v1knet::begin_round(v1knet::SEATS);
    foes.clear(); foes_down = 0; foes_spawned = true; buls.clear(); sparks.clear(); scorch.clear(); rec.clear(); build_targets();
    for (int i = 0; i < 4; i++) { lscore[i] = 0; spl[i] = Pilot(); }
    pl = Pilot(); for (int i = 0; i < 4; i++) respawn_seat(i);
    score = 0; meter = 0; pop_t = 0; time_left = round_secs; chat_t = 1.f; st = ST_PLAY; st_t = 0;
    sfx(5, SK_GO, 100); buggy::play(9, (int)SK_ENG, 60);
    dl("t=%.1f link: round starts, %s, %d seats", sim_t, v1knet::master ? "master" : "alone until a seat comes", 4);
}
static void link_sim(float dt, const Ctl &k, bool fire) {   /* one 1/120 s step of the whole round */
    for (int i = 0; i < 4; i++) {
        Pilot &p = seat_pl(i);
        if (p.wreck) { link_rt[i] += dt; if (link_rt[i] > 3.5f) { respawn_seat(i); dl("t=%.1f link: seat %d back in the air", sim_t, i); } continue; }
        Ctl kk; bool ff = false, human = false;
        if (i == me) { if (!autoplay) { kk = k; ff = fire; human = true; } }
        else if (const NetIn *r = (const NetIn *)v1knet::seat_input(i, sizeof(NetIn))) { kk.jx = clampf(r->jx, -1.f, 1.f); kk.jy = clampf(r->jy, -1.f, 1.f); kk.lever_set = clampf(r->lever, 0.f, 1.f); ff = r->fire != 0; human = true; }
        if (human) { as_seat(i, [&] { step_player(kk, dt); }); if (ff && p.fire_cool <= 0 && !p.fall && !p.wreck) { p.fire_cool = 1.f / fire_rate; link_credit = i; seat_fire(i); } }
        else cpu_seat(i, dt);
    }
    st = pl.wreck ? ST_DEAD : ST_PLAY;
    step_bullets(dt); step_train(dt);
    for (GT &g : gts) if (!g.alive) { g.down += dt; if (g.down > 15.f) { g.alive = true; g.hits = 0; g.down = 0; } }
    time_left -= dt;
    if (time_left <= 0) { dl("t=%.1f link: round over, scores %d %d %d %d", sim_t, lscore[0], lscore[1], lscore[2], lscore[3]); for (int i = 0; i < 4; i++) lscore[i] = 0; time_left = round_secs; }
}
static void net_send() {
    static NetSt s; memset(&s, 0, sizeof(s)); s.n = 4; s.time_left = time_left; s.train_s = train_s;
    int zi = 0; for (size_t k = 0; k < gts.size(); k++) { if (k < 16 && gts[k].alive) s.gt_alive |= (uint16_t)(1u << k); if (gts[k].type == 5 && zi < 2) { s.zep[zi][0] = gts[k].p.x; s.zep[zi][1] = gts[k].p.y; s.zep[zi][2] = gts[k].p.z; zi++; } }
    for (int i = 0; i < 4; i++) { const Pilot &p = seat_pl(i); NetCraft &n = s.c[i]; n.p[0] = p.c.p.x; n.p[1] = p.c.p.y; n.p[2] = p.c.p.z; n.yaw = p.c.yaw; n.pitch = p.c.pitch; n.bank = p.c.bank; n.speed = p.c.speed;
        n.flags = (uint8_t)((p.ground ? 1 : 0) | (p.stalled ? 2 : 0) | (p.fall ? 4 : 0) | (p.wreck ? 8 : 0)); n.hits = (uint8_t)p.hits; n.score = (int16_t)lscore[i]; }
    for (const Bul &b : buls) if (b.own >= 10 && s.nb < NB_MAX) { NetBul &n = s.b[s.nb++]; n.p[0] = b.p.x; n.p[1] = b.p.y; n.p[2] = b.p.z; n.v[0] = b.v.x; n.v[1] = b.v.y; n.v[2] = b.v.z; }
    v1knet::send_snapshot(&s, offsetof(NetSt, b) + s.nb * sizeof(NetBul));
}
static void net_apply(const NetSt &s, bool fresh) {
    me = v1knet::my_seat;
    for (int i = 0; i < 4; i++) { Pilot &p = seat_pl(i); const NetCraft &n = s.c[i]; const bool wreck = (n.flags & 8) != 0;
        if (!fresh) {
            if (wreck && !p.wreck) { emit(p.c.p, 60, A_SPK_O, 16000.f, 2.5f); emit(p.c.p, 30, A_SPK_R, 12000.f, 2.f); emit(p.c.p, 20, A_SMOKE, 6000.f, 3.f, 3000.f); sfx(3, SK_BOOM, i == me ? 127 : 70); if (i == me) VPIX_Haptic(2, 0.5f, 1.f); }
            if (i == me && n.hits > p.hits) { pl.shake = 0.5f; sfx(2, SK_HURT, 120); VPIX_Haptic(2, 0.2f, 0.8f); say(V_HIT, true); }
            if (i == me && n.score > lscore[i]) { pop_t = 1.5f; pop_pts = n.score - lscore[i]; sfx(3, SK_BOOM, 90); }
        }
        p.c.p = V(n.p[0], n.p[1], n.p[2]); p.c.yaw = n.yaw; p.c.pitch = n.pitch; p.c.bank = n.bank; p.c.speed = n.speed; orient(p.c);
        p.ground = (n.flags & 1) != 0; p.stalled = (n.flags & 2) != 0; p.fall = (n.flags & 4) != 0; p.wreck = wreck; p.hits = n.hits; lscore[i] = n.score; }
    pl.lever = my_lever;
    for (size_t k = 0; k < gts.size() && k < 16; k++) { const bool a = (s.gt_alive >> k) & 1; if (!fresh && gts[k].alive && !a && gts[k].type >= 4) { emit(gts[k].p, 70, A_SPK_O, 16000.f, 2.4f); emit(gts[k].p, 30, A_SPK_R, 12000.f, 2.f); sfx(3, SK_BOOM, 90); } gts[k].alive = a; }
    train_s = s.train_s; time_left = s.time_left;
    int zi = 0; for (GT &g : gts) if (g.type == 5 && zi < 2) { g.p = V(s.zep[zi][0], s.zep[zi][1], s.zep[zi][2]); g.yaw = atan2f(g.p.y - ZC[zi].y, g.p.x - ZC[zi].x); zi++; }
    const int nb = (std::min)((int)s.nb, (int)NB_MAX); buls.resize((size_t)nb);
    for (int k = 0; k < nb; k++) { Bul &b = buls[(size_t)k]; b.p = V(s.b[k].p[0], s.b[k].p[1], s.b[k].p[2]); b.v = V(s.b[k].v[0], s.b[k].v[1], s.b[k].v[2]); b.own = 10; b.t = 0; }
}

/* ---- dev: ACES_CLIP_LOG=1 counts any part of any plane inside the hangar's solid parts (Bobby 11-03: "planes in walls") ---- */
static unsigned long clip_n = 0, clip_checks = 0; static float min_gap_m = 1e9f;
static float zep_gap_m = 1e9f;
static void clip_check(const Craft &c, const char *who) {
    if (!strcmp(who, "enemy")) for (const GT &g : gts) if (g.alive && g.type >= 4) { float a = air_gap(g, c.p) / MM; if (a < zep_gap_m) zep_gap_m = a; }
    static const V PT[7] = {V(-4800, 900, 1800), V(4800, 900, 1800), V(-3900, 700, -600), V(3900, 700, -600), V(0, 3500, 100), V(0, -4200, 500), V(0, 0, 1900)};
    clip_checks++;
    for (const V &q : PT) { V p = c.p + to_world(c.R, q); bool hit = false;
        if (p.z < 9000.f) { for (const Rect &r : walls) if (p.x > r.x0 + 1 && p.x < r.x1 - 1 && p.y > r.y0 + 1 && p.y < r.y1 - 1) hit = true;
            if (p.z > 6500.f && fabsf(p.x) < HANGAR_X && p.y > -200.f && p.y < T_FRONT) hit = true; }
        else if (p.z < HANGAR_TOP - 5000.f && fabsf(p.x) < HANGAR_X && p.y > HANGAR_Y0 && p.y < T_FRONT) { float rz = 9000.f + (1.f - fabsf(p.x) / HANGAR_X) * 4500.f; if (p.z < rz) hit = true; }
        if (hit) { if (clip_n++ < 30) dl("t=%.1f CLIP %s at (%.0f %.0f %.0f) plane (%.0f %.0f %.0f)", sim_t, who, p.x, p.y, p.z, c.p.x, c.p.y, c.p.z); break; } }
    float gx = (std::max)(0.f, fabsf(c.p.x) - HANGAR_X), gy = c.p.y > T_FRONT ? c.p.y - T_FRONT : c.p.y < HANGAR_Y0 ? HANGAR_Y0 - c.p.y : 0.f, gz = (std::max)(0.f, c.p.z - 13500.f), g = sqrtf(gx * gx + gy * gy + gz * gz) / MM;
    if (g < min_gap_m && strcmp(who, "player")) min_gap_m = g;
}
static bool clip_on = false, ram_test = false; static int zep_test = 0;   /* ACES_ZEPTEST: 1 the bot flies to the zeppelins and shoots them, 2 it flies into them */

/* ---- the computer player (ACES_AUTO=1) ---- */
enum { AU_TAXI, AU_TAKEOFF, AU_FIGHT, AU_APPROACH, AU_LAND };
static int au_mode = AU_TAXI; static float au_t = 0;
static void ctl_for(float yaw_t, float pitch_t, Ctl &k) {
    const Craft &c = pl.c;
    k.jx = clampf(-wrap_pi(yaw_t - c.yaw) * 2.0f, -1.f, 1.f);
    k.jy = -clampf((pitch_t - c.pitch) * 2.5f, -1.f, 1.f);
}
static void auto_ctl(Ctl &k, bool &fire) {
    const Craft &c = pl.c; fire = false; au_t += 1.f / 120.f;
    if (st == ST_HANGAR) {
        au_mode = AU_TAXI;
        float door = auto_ace ? 16000.f : -16000.f;
        float yaw_t = c.p.y < -3000.f ? atan2f(-(door - c.p.x), 2000.f - c.p.y) : 0.f;
        ctl_for(yaw_t, 0.f, k); k.lever_set = c.speed < 3.f ? 0.15f : c.speed < 4.5f ? 0.09f : 0.04f;
        if (c.p.y > 3000.f) au_mode = AU_TAKEOFF;
        return;
    }
    float alt = (c.p.z - GEAR - GROUND_Z) / MM;
    if (au_mode == AU_TAXI) au_mode = AU_TAKEOFF;
    if (au_mode == AU_TAKEOFF) {
        float yaw_t = clampf(c.p.x * 0.00002f, -0.15f, 0.15f);
        if (pl.ground) {
            if (fabsf(wrap_pi(c.yaw - yaw_t)) > 0.25f) { ctl_for(yaw_t, 0.f, k); k.lever_set = c.speed < 5.f ? 0.2f : 0.04f; }
            else { ctl_for(yaw_t, 0.f, k); k.lever_set = 1.f; k.jy = c.speed > v_lift + 2.f ? -0.6f : 0.f; }
        } else { ctl_for(yaw_t, 0.2f, k); k.lever_set = 1.f; if (alt > 50.f) au_mode = AU_FIGHT; }
        return;
    }
    if (au_mode == AU_FIGHT) {
        if (pl.ground) { au_mode = AU_TAKEOFF; return; }
        if (ram_test) {   /* dev: fly at the hangar at door, sign, lintel and roof height, again and again */
            static const float HT[5] = {3000.f, 5000.f, 8000.f, 10500.f, 13000.f}, XT[3] = {-16000.f, 0.f, 16000.f}; static int pass = 0, was = 0; bool near_h = c.p.y < 60000.f && fabsf(c.p.x) < 60000.f; if (near_h && !was) pass++; was = near_h;
            V t(XT[pass % 3], near_h ? -60000.f : 0.f, HT[pass % 5]); V dr = t - c.p; float hd2 = sqrtf(dr.x * dr.x + dr.y * dr.y) + 1.f;
            if (c.p.y > 500000.f || near_h || (fabsf(wrap_pi(atan2f(-dr.x, dr.y) - c.yaw)) < 0.4f)) { ctl_for(atan2f(-dr.x, dr.y), clampf(atan2f(dr.z, hd2), -0.3f, 0.3f), k); k.lever_set = 0.7f; }
            else { ctl_for(0.f, 0.15f, k); k.lever_set = 1.f; }   /* out north first for a run in */
            return; }
        V target(0, 500000.f, 150000.f); bool at_foe = false; float best = 1e30f;
        for (const Foe &e : foes) { if (zep_test || !e.alive || e.gone) continue; float d = len(e.c.p - c.p); if (d < best) { best = d; target = e.c.p + vel_of(e.c) * (std::min)(d / 40000.f, 6.f) * 0.5f; at_foe = true; } }
        bool at_gt = false;
        if (!at_foe) for (const GT &g : gts) { if (!g.alive || (zep_test && g.type != 5)) continue; float d = len(g.p - c.p); if (d < best) { best = d; target = g.p + V(0, 0, 2000); at_gt = true; } }
        V d = target - c.p; float hd = sqrtf(d.x * d.x + d.y * d.y) + 1.f;
        float yaw_t = atan2f(-d.x, d.y), pitch_t = clampf(atan2f(d.z, hd), -0.5f, 0.5f);
        if (alt < 100.f) pitch_t = (std::max)(pitch_t, 0.2f);
        if (c.speed < v_stall + 6.f) pitch_t = (std::min)(pitch_t, 0.f);
        ctl_for(yaw_t, pitch_t, k); k.lever_set = 0.85f;
        if (at_foe) fire = lock_idx >= 0;
        else if (at_gt) { float ca = dot(c.R.r[1], norm(d)); fire = ca > (zep_test ? 0.985f : 0.995f) && len(d) < 600000.f; if (zep_test == 1 && len(d) < 90000.f) { k.jy = -1.f; k.jx = 1.f; } }
        if (st == ST_PLAY && pl.hits >= hits_survive && pl.hits > 0) { au_mode = AU_APPROACH; au_t = 0; dl("t=%.1f autopilot: one hit from death, going to land", sim_t); }
        return;
    }
    if (au_mode == AU_APPROACH || au_mode == AU_LAND) {
        V wp(0, 1800000.f, 90000.f); V d = wp - c.p; float hd = sqrtf(d.x * d.x + d.y * d.y);
        if (au_mode == AU_APPROACH) {
            ctl_for(atan2f(-d.x, d.y), clampf(atan2f(d.z, hd), -0.2f, 0.2f), k); k.lever_set = c.speed > 34.f ? 0.25f : 0.5f;
            if (hd < 250000.f || au_t > 120.f) au_mode = AU_LAND;
            return;
        }
        float ym = c.p.y / MM, alt_t = clampf((ym - 800.f) * 0.07f, 0.f, 90.f);
        float yaw_t = PIF + clampf(c.p.x * 0.00003f, -0.2f, 0.2f);
        float pitch_t = alt > 6.f ? clampf((alt_t - alt) * 0.03f, -0.12f, 0.1f) : 0.04f;
        if (pl.ground) { ctl_for(yaw_t, 0.f, k); k.lever_set = 0.f; }
        else { ctl_for(yaw_t, pitch_t, k); k.lever_set = alt > 6.f ? (c.speed > 30.f ? 0.1f : 0.35f) : 0.f; }
        if (pl.ground && c.speed < 0.3f && pl.hits == 0) { au_mode = AU_TAKEOFF; dl("t=%.1f autopilot: repaired, taking off again", sim_t); }
        return;
    }
}

/* ---- HUD bitmaps, built here ---- */
static void make_hud_aces() {
    d1k::make_hud();
    /* LOCKED ON and the points: tall narrow lettering about 0.47 of the picture wide, flashing green and purple every few frames (t975-977, t988.7-990.2 frame by frame) */
    for (int ch = 0; ch < 26; ch++) for (int set = 0; set < 2; set++) { const int CW = 3, CH = 5, W = 5 * CW, H = 7 * CH; std::vector<uint32_t> px((size_t)W * H, 0); uint32_t col = set ? 0xFFD76EBEu : 0xFFA0F58Cu;
        for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (d1k::FONT[ch][y * 5 + x] == '1') for (int b = 0; b < CH; b++) for (int a = 0; a <= CW; a++) if (x * CW + a < W) px[(size_t)(y * CH + b) * W + x * CW + a] = col;
        add_bitmap((set ? BM_PFONT : BM_GFONT) + ch, W, H, px); }
    static const char *PD[10] = {"01110100011000110001100011000101110", "00100011000010000100001000010001110", "01110100010000100110010001000011111", "11110000010000101110000010000111110", "00010001100101010010111110001000010",
                                 "11111100001111000001000011000101110", "00110010001000011110100011000101110", "11111000010001000100010000100001000", "01110100011000101110100011000101110", "01110100011000101111000010001001100"};   /* round zeros, no slash (t989) */
    for (int d = 0; d < 10; d++) for (int set = 0; set < 2; set++) { const int CW = 7, CH = 8, W = 5 * CW, H = 7 * CH; std::vector<uint32_t> px((size_t)W * H, 0); uint32_t col = set ? 0xFFD76EBEu : 0xFFA0F58Cu;
        for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (PD[d][y * 5 + x] == '1') for (int b = 0; b < CH; b++) for (int a = 0; a < CW; a++) px[(size_t)(y * CH + b) * W + x * CW + a] = col;
        add_bitmap((set ? BM_PDIG : BM_GDIG) + d, W, H, px); }
    { std::vector<uint32_t> d(AC_TW * AC_TH); for (int i = 0; i < AC_TW * AC_TH; i++) d[i] = AC_TITLE_PAL[AC_TITLE[i]]; add_bitmap(BM_TITLE, AC_TW, AC_TH, d); }
}
static bool foe_on_tail() {
    if (link_round) for (int j = 0; j < 4; j++) { if (j == me) continue; const Pilot &q = seat_pl(j); if (q.wreck) continue; V d = q.c.p - pl.c.p; float r = len(d); if (r < 150000.f && r > 1.f && dot(d * (1.f / r), pl.c.R.r[1]) < -0.85f) return true; }
    for (const Foe &e : foes) if (e.alive && !e.gone) { V d = e.c.p - pl.c.p; float r = len(d); if (r < 150000.f && r > 1.f && dot(d * (1.f / r), pl.c.R.r[1]) < -0.85f) return true; } return false; }
static void gtext(VPIX_Frame &f, const char *s, float cx, float y, int set) {
    const float ADV = 18.3f; float x = cx - strlen(s) * ADV * 0.5f;
    for (const char *c = s; *c; c++) { if (*c >= 'A' && *c <= 'Z') blit(f, (set ? BM_PFONT : BM_GFONT) + (*c - 'A'), 15, 35, x, y); x += ADV; }
}
static void ptext(VPIX_Frame &f, int n, float cx, float y, int set) {
    char b[16]; snprintf(b, sizeof b, "%d", n); const float ADV = 38.f; float x = cx - strlen(b) * ADV * 0.5f; for (const char *c = b; *c; c++) { blit(f, (set ? BM_PDIG : BM_GDIG) + (*c - '0'), 35, 56, x, y); x += ADV; }
}
static void full_blit(VPIX_Frame &f, int id, int w, int h) {
    VPIX_Blit b = {}; b.handle = d1k::BMPH + id; b.sw = w; b.sh = h; b.x = 0; b.y = 0; b.dw = 352.f; b.dh = 288.f; b.key = 0; b.alpha = 1.f; b.prio = 1000000; b.tint[0] = b.tint[1] = b.tint[2] = 1.f; f.blits.push_back(b);
}

/* ---- drawing ---- */
static M compose(const M &parent, const M &local) { M o; for (int k = 0; k < 3; k++) o.r[k] = to_world(parent, local.r[k]); return o; }
static void put_craft(VPIX_Frame &f, const Craft &c, int id) {
    dput(f, id, c.R, c.p);
    VPIX_Instance &p = dput(f, A_PROP, c.R, c.p + to_world(c.R, V(0, 2900, 120))); p.glass = 110; p.bright = 1;
}
static bool cockpit_view(const VPIX_Input &in, M &cr, V &eye, V &aim) {
    const M &R = pl.c.R;
    float cy = cosf(yaw0), sy = sinf(yaw0);
    cr.r[0] = R.r[0] * cy - R.r[1] * sy; cr.r[1] = R.r[0] * sy + R.r[1] * cy; cr.r[2] = R.r[2];   /* where you face at the start is forward */
    V seat(0, 150, 1050);
    if (in.vr && in.head.valid && have_ref) {   /* head movement leans you in the seat */
        V dh = V(in.head.p[0], in.head.p[1], in.head.p[2]) - head_ref;
        V dw = to_world(cr, dh) * 1.85f; if (len(dw) > 700.f) dw = norm(dw) * 700.f;
        eye = pl.c.p + to_world(R, seat) + dw;
    } else eye = pl.c.p + to_world(R, seat);
    if (pl.shake > 0) eye += V(rnd(-1, 1), rnd(-1, 1), rnd(-1, 1)) * (60.f * pl.shake);
    if (in.head.valid) aim = norm(to_world(cr, V(in.head.R[1][0], in.head.R[1][1], in.head.R[1][2]))); else aim = R.r[1];
    return in.head.valid;
}
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    static const char *fixed = getenv("ACES_CAM");
    const bool has_cam = fixed && *fixed, title = st == ST_TITLE && !has_cam;
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f; f.clear[0] = title ? 0 : 0x90; f.clear[1] = title ? 0 : 0xC0; f.clear[2] = title ? 0 : 0xF0;
    float fade = 0.f;   /* the picture sinks to grey at the end: a step down then slowly grey, about 5 s (t1026-1031); GAME OVER is grey too (t1032-1034) */
    if (st == ST_FADE) fade = 0.3f + 0.22f * clampf(st_t / 5.f, 0.f, 1.f); else if (st == ST_OVER || st == ST_DONE) fade = 0.52f;
    const float lk = 1.f - fade * 0.5f;
    f.n_lights = 1; f.lights[0] = {0.7f * lk, 0.3f * lk, {-0.218f, -0.437f, 0.874f}};   /* flat shaded and bright; values are a guess */
    const bool replay = st == ST_REPLAY && !rec.empty();
    if (!title) {
        { M I; dput(f, A_WORLD, I, V(0, 0, 0)); for (const V &s : scorch) dput(f, A_SCORCH, I, s + V(0, 0, 640)); }
        if (replay) { const RecFrame &r = rec[(std::min)(rec.size() - 1, (size_t)(st_t * 10.f))]; for (int i = 0; i < r.n; i++) { Craft c; c.p = r.c[i].p; c.yaw = r.c[i].yaw; c.pitch = r.c[i].pitch; c.bank = r.c[i].bank; orient(c); put_craft(f, c, r.c[i].id); } }
        else if (st != ST_OVER && st != ST_DONE) for (const Foe &e : foes) if (!e.gone) put_craft(f, e.c, e.style ? A_PL_RED : A_PL_TAN);
        if (link_round) { static const int LID[4] = {A_PL_GREEN, A_PL_RED, A_PL_TAN, A_PL_RED}; for (int j = 0; j < 4; j++) if (j != me && !seat_pl(j).wreck) put_craft(f, seat_pl(j).c, LID[j]); }
        for (const GT &g : gts) if (g.alive) { VPIX_Instance &q = dput(f, g.type == 2 ? A_LOCO : g.type == 3 ? A_CAR : g.type == 4 ? A_BALLOON : A_ZEP, yawed(g.yaw), g.p); if (g.type == 4) { q.S[0] = q.S[1] = q.S[2] = 3.f; } }
        for (const Bul &b : buls) dput(f, A_TRC_P, look_at(b.p, b.p + b.v), b.p).bright = 1;
        { M I; for (const Spark &k : sparks) dput(f, k.id, I, k.p).bright = 1; }
    }

    static const bool show_models = envf("ACES_MODELS", 0) != 0;   /* dev: a row of models for close-up pictures, with ACES_CAM */
    if (show_models) { const V B(0, 140000.f, 0); const int ids[3] = {A_PL_TAN, A_PL_RED, A_PL_GREEN}; for (int i = 0; i < 3; i++) { Craft c; c.p = B + V(i * 14000.f, 0, 4000.f); c.yaw = 0.5f; orient(c); put_craft(f, c, ids[i]); }   /* never at y 0: that is the hangar's front wall */
        M I; dput(f, A_LOCO, I, B + V(-14000, 0, 560)); dput(f, A_CAR, I, B + V(-14000, -11000, 560)); dput(f, A_BALLOON, I, B + V(-30000, 0, 12000)); dput(f, A_ZEP, I, B + V(0, 60000, 25000)); }
    M cam; V eye;
    if (has_cam) { float a[6] = {0, -80000, 15000, 0, 0, 3000}; sscanf(fixed, "%f,%f,%f,%f,%f,%f", a, a + 1, a + 2, a + 3, a + 4, a + 5); eye = V(a[0], a[1], a[2]); cam = look_at(eye, V(a[3], a[4], a[5])); }
    else if (title) { eye = V(0, -1000, 0); cam = look_at(eye, V(0, 0, 0)); }
    else if (st == ST_DEAD) { eye = V(pl.c.p.x + 40000.f, pl.c.p.y - 40000.f, pl.c.p.z + 15000.f); cam = look_at(eye, pl.c.p + V(0, 0, 3000)); }
    else if (st == ST_OVER || st == ST_DONE) { float y = 186000.f + (st == ST_OVER ? (std::min)(st_t, 5.f) : 5.f) * 3200.f; eye = V(0, y, 7000.f); cam = look_at(eye, V(0, y + 1000000.f, 7000.f)); }
    else if (replay) { const RecCraft &r = rec[(std::min)(rec.size() - 1, (size_t)(st_t * 10.f))].c[0]; V fw(-sinf(r.yaw), cosf(r.yaw), 0); eye = r.p - fw * 34000.f + V(0, 0, 20000.f); if (eye.z < 6000.f) eye.z = 6000.f; cam = look_at(eye, r.p); }
    else {
        V aim; cockpit_view(in, cam, eye, aim);
        if (chase_on) {   /* Bobby's option: rigid behind the plane, plane low in the picture and level while the world tilts, framed on ACES_PIC2/3 */
            cam = pl.c.R; eye = pl.c.p - pl.c.R.r[1] * 11400.f + pl.c.R.r[2] * 2600.f;
            if (fabsf(pl.c.p.x) < HANGAR_X && pl.c.p.y > HANGAR_Y0 && pl.c.p.y < 0.f) { eye.x = clampf(eye.x, -HANGAR_X + 400.f, HANGAR_X - 400.f); eye.y = (std::max)(eye.y, HANGAR_Y0 + 400.f); eye.z = (std::min)(eye.z, 8600.f); }
            if (eye.z < GROUND_Z + 400.f) eye.z = GROUND_Z + 400.f;
            dput(f, A_PL_GREEN, pl.c.R, pl.c.p);   /* no prop disc: from behind its see-through pass would paint over the body */
            dput(f, A_HEAD, pl.c.R, pl.c.p + to_world(pl.c.R, V(0, -1000, 640)));   /* the co-pilot's head in the rear seat (tape t529) */
            VPIX_Instance &rg = dput(f, A_RING_C, pl.c.R, pl.c.p + pl.c.R.r[1] * 131000.f); rg.S[0] = rg.S[1] = rg.S[2] = 98.f; rg.on_top = 2;
            /* the shots as a beam, from the guns along the real path to the farthest round. White stream: ACES_PIC2. Flat wedge, wide at the guns and running to a point: Bobby's pick, the look of the shot beams in Ghost Train (20250728_WMP_Ghost_Train t40.0-41.0) */
            float far_d = 0.f, near_d = 1e30f; V bf, bn; const V nose = pl.c.p + pl.c.R.r[1] * 3500.f;
            for (const Bul &b : buls) if (b.own == 0) { float d = len(b.p - nose); if (d > far_d) { far_d = d; bf = b.p; } if (d < near_d) { near_d = d; bn = b.p; } }
            if (far_d > 1000.f) { const V a = sim_t - fired_t < 0.15 ? nose : bn; const V dv = bf - a; const float L = len(dv);
                if (L > 500.f) { M bm; bm.r[1] = dv * (1.f / L); bm.r[0] = norm(cross(bm.r[1], norm(eye - a))); bm.r[2] = cross(bm.r[0], bm.r[1]);
                    VPIX_Instance &q = dput(f, A_BEAM, bm, a); q.S[0] = 230.f; q.S[1] = L; q.S[2] = 1.f; q.bright = 1; } }
        } else {
        static const char *lk2 = getenv("ACES_LOOK"); if (lk2 && *lk2) { float ya = 0, pi = 0; sscanf(lk2, "%f,%f", &ya, &pi); ya *= PIF / 180.f; pi *= PIF / 180.f;   /* dev: a fixed look round the cockpit for pictures */
            V fw = cam.r[1], ri = cam.r[0], up = cam.r[2]; V f2 = fw * cosf(ya) + ri * sinf(ya), r2 = ri * cosf(ya) - fw * sinf(ya); V f3 = f2 * cosf(pi) + up * sinf(pi), u3 = up * cosf(pi) - f2 * sinf(pi); cam.r[0] = r2; cam.r[1] = f3; cam.r[2] = u3; }
        dput(f, A_COCKPIT, pl.c.R, pl.c.p).on_top = 2;
        dput(f, A_RING, pl.c.R, pl.c.p).on_top = 2;
        if (pl.c.speed > 0.5f || pl.lever > 0.05f) { float fa = (float)fr * 2.399f; M fm; fm.r[0] = V(cosf(fa), 0, sinf(fa)); fm.r[1] = V(0, 1, 0); fm.r[2] = V(-sinf(fa), 0, cosf(fa));   /* a sector that jumps round the hub at the picture rate */
            VPIX_Instance &fan = dput(f, A_FAN, compose(pl.c.R, fm), pl.c.p + to_world(pl.c.R, V(0, 1500, 640))); fan.on_top = 2; fan.glass = 95; fan.bright = 1; }
        dput(f, A_FACE, compose(pl.c.R, yawed(PIF)), pl.c.p + to_world(pl.c.R, V(0, -1000, 1180))).on_top = 2;   /* always there in the rear seat; he only shows if you look round */
        { /* the BONUS TIME needle: points fill it from the left edge toward the purple line */
          float u = clampf(meter / bonus_pts, 0.f, 1.f), th = (130.f - 36.f * u) * PIF / 180.f;
          V D = dial_u * cosf(th) + dial_w * sinf(th); M nm; nm.r[1] = D; nm.r[2] = dial_n; nm.r[0] = cross(D, dial_n);
          dput(f, A_NEEDLE, compose(pl.c.R, nm), pl.c.p + to_world(pl.c.R, dial_c + dial_n * 9.f)).on_top = 2; }
        int shown = (std::min)(12, pl.hits * 4);   /* a group of holes for each hit taken */
        for (int i = 0; i < shown; i++) { M hm = yawed(hole_slot[i].yaw); dput(f, hole_slot[i].spike ? A_SPIKE : A_HOLE, compose(pl.c.R, hm), pl.c.p + to_world(pl.c.R, hole_slot[i].p)).on_top = 2; }
        }
    }
    if (fade > 0 && !has_cam) { VPIX_Instance &q = dput(f, A_FADE, cam, eye + cam.r[1] * 1500.f); q.on_top = 1; q.glass = (uint8_t)(fade * 255.f); q.bright = 1; }
    if ((st == ST_OVER || st == ST_DONE) && !has_cam) { M I; VPIX_Instance &a = dput(f, A_GO_B, I, V(0, 250000.f, -2900.f)); a.on_top = 1; a.glass = 255; a.bright = 1; a.S[2] = 1.45f; VPIX_Instance &b = dput(f, A_GO_F, I, V(0, 250000.f - 60.f, -2900.f)); b.on_top = 1; b.glass = 255; b.bright = 1; b.S[2] = 1.45f; }   /* glass so they draw in the same pass as the grey, after it */
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cam.r[k].x; f.cam_R[k][1] = cam.r[k].y; f.cam_R[k][2] = cam.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    /* The camera is the plane, with the live head turned on top of it by the renderer (as the Buggy Ball cockpit does). The Dactyl way,
       in_head with head_fed, would only follow while the camera pitch equals the head pitch, and a banking plane never does: the head
       would be applied twice. So in_head stays false here. */
    f.in_head = false;

    const float CX = 169.f;   /* the text sits a little left of centre, 0.48 across, and 0.46 down (t975.5, t989) */
    if (title) full_blit(f, BM_TITLE, AC_TW, AC_TH);
    else if (st == ST_PLAY || st == ST_HANGAR) {
        int set = (int)((fr / 3) & 1);
        if (pop_t > 0) ptext(f, pop_pts, CX, 104.f, set);
        else if (lock_idx >= 0) gtext(f, "LOCKED ON", CX, 115.f, set);
        else if (tail_text && foe_on_tail()) gtext(f, "ON YOUR TAIL", CX, 115.f, set);   /* NOT in any frame. Bobby's pick 11-03, in the LOCKED ON style; the press only has the co-pilot saying it (Sega Force 16). ACES_TAILTEXT=0 for the footage */
    }
    d1k::look_test(f);
    d1k::hs::draw(f, "aces", st == ST_TITLE, (float)sim_t);
    VPIX_Render_Submit(f);
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 120 Hz; pictures at the machine's rate ---- */
static void place_in_hangar() {
    pl.c = Craft(); pl.c.p = V(0, -22000.f, GROUND_Z + GEAR); pl.c.yaw = 0; pl.c.pitch = 0.05f; pl.c.speed = 0; orient(pl.c);
    pl.lever = 0; pl.ground = true; pl.stalled = false; pl.fall = false; pl.wreck = false; pl.hits = 0; pl.invuln = 0; pl.repair_t = 0; pl.fire_cool = 0; pl.shake = 0; pl.jx = pl.jy = 0;
    buls.clear(); st = ST_HANGAR; st_t = 0; au_mode = AU_TAXI;
}
static void new_game() {
    pl = Pilot(); pl.lives = lives_total; foes.clear(); foes_down = 0; foes_spawned = false; sparks.clear(); scorch.clear(); build_targets(); rec.clear(); rec_t = 0;
    score = 0; meter = envf("ACES_METER", 0.f); pop_t = 0; chat_t = 1.f; chat_step = 0; time_left = round_secs; place_in_hangar(); pl.hits = (int)envf("ACES_HITS_NOW", 0.f);
    if (autoplay) auto_ace = !auto_ace;   /* the test player alternates the two exits */
    buggy::play(9, (int)SK_ENG, 60); sfx(5, SK_GO, 100); say(V_DOOR, true);
    dl("t=%.1f new game", sim_t);
}
static void record_frame() {
    RecFrame r; r.n = 0; auto add = [&](const Craft &c, int id) { if (r.n < 9) { RecCraft &q = r.c[r.n++]; q.p = c.p; q.yaw = c.yaw; q.pitch = c.pitch; q.bank = c.bank; q.id = id; } };
    add(pl.c, A_PL_GREEN); for (const Foe &e : foes) if (!e.gone) add(e.c, e.style ? A_PL_RED : A_PL_TAN);
    rec.push_back(r); if (rec.size() > 200) rec.erase(rec.begin());
}
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0, frame_acc = 0, next_log = 1; static unsigned long frames = 0;
static void link_leave(const char *why) {
    link_round = false; link_slave = false; me = 0; st = ST_TITLE; st_t = 0; buls.clear(); sparks.clear(); foes.clear(); pl = Pilot(); ::voices[9].on = false;
    dl("t=%.1f link: %s, back to the title", sim_t, why);
}
/* true = this tick was a linked pod's, inside the master's round (its picture is built here). Pod 1, a pod alone and the title go on to the normal tick. */
static bool link_tick(double el, const VPIX_Input &in, const Ctl &k, bool fire) {
    v1knet::pump();
    static double next_ll = 0;
    if (!link_round) { if (v1knet::master) { if (v1knet::due(30)) v1knet::send_snapshot(NULL, 0); } else v1knet::send_input(NULL, 0); return false; }
    if (sim_t >= next_ll) { next_ll = sim_t + 2.0;
        dl("t=%.0f link: %s seat %d | %s | s0 (%.0f %.0f %.0f) %d w%d | s1 (%.0f %.0f %.0f) %d w%d | s2 (%.0f %.0f %.0f) %d w%d | s3 (%.0f %.0f %.0f) %d w%d | buls %d", sim_t, v1knet::master ? "master" : link_slave ? "slave" : "alone", me, link_slave ? "in the master's round" : "own round",
           seat_pl(0).c.p.x, seat_pl(0).c.p.y, seat_pl(0).c.p.z, lscore[0], (int)seat_pl(0).wreck, seat_pl(1).c.p.x, seat_pl(1).c.p.y, seat_pl(1).c.p.z, lscore[1], (int)seat_pl(1).wreck,
           seat_pl(2).c.p.x, seat_pl(2).c.p.y, seat_pl(2).c.p.z, lscore[2], (int)seat_pl(2).wreck, seat_pl(3).c.p.x, seat_pl(3).c.p.y, seat_pl(3).c.p.z, lscore[3], (int)seat_pl(3).wreck, (int)buls.size());
    }
    if (v1knet::master) { if (v1knet::due(30)) net_send(); return false; }
    const uint8_t *d; size_t n;
    if (v1knet::take_snapshot(d, n)) {
        if (n >= offsetof(NetSt, b) && v1knet::my_seat >= 0) {
            static NetSt s; memset(&s, 0, sizeof(s)); memcpy(&s, d, (std::min)(n, sizeof(s)));
            if (s.n == 4) { const bool fresh = !link_slave; if (fresh) { link_slave = true; my_lever = 0.6f; sparks.clear(); if (gts.empty()) build_targets(); pl.fire_cool = 0; dl("t=%.1f link: joined the master's round, seat %d", sim_t, v1knet::my_seat); }
                net_apply(s, fresh); } }
        else if (link_slave) { link_leave("the master's round is over"); return false; }
    }
    if (link_slave && !v1knet::master_heard()) { link_leave("no word from the master for 3 s"); return false; }
    if (!link_slave) { v1knet::send_input(NULL, 0); return false; }   /* alone on CPUs while it waits for a seat */
    my_lever = clampf(my_lever + k.lever_rate * (float)el, 0.f, 1.f);
    NetIn o; o.jx = k.jx; o.jy = k.jy; o.lever = my_lever; o.fire = fire ? 1 : 0; v1knet::send_input(&o, sizeof(o));
    const float dt = 1.f / 120.f; int cnt = 0;
    while (acc >= dt && cnt < 24) {
        for (int j = 0; j < 4; j++) { Pilot &p = seat_pl(j); if (!p.wreck && !p.ground) { orient(p.c); p.c.p += p.c.R.r[1] * (p.c.speed * MM * dt); } }   /* between snapshots every plane flies on */
        for (Bul &b : buls) b.p += b.v * dt;
        for (size_t i = 0; i < sparks.size();) { Spark &sk = sparks[i]; sk.v.z -= 3000.f * dt; sk.p += sk.v * dt; sk.life -= dt; if (sk.life <= 0) { sparks[i] = sparks.back(); sparks.pop_back(); } else i++; }
        { V zp[2]; float zy[2] = {0, 0}; int zi = 0; for (GT &g : gts) if (g.type == 5 && zi < 2) { zp[zi] = g.p; zy[zi] = g.yaw; zi++; }
          step_train(dt);   /* the zeppelins stay where the snapshot put them, circling on */
          zi = 0; for (GT &g : gts) if (g.type == 5 && zi < 2) { const float a = zy[zi] + 12000.f / 500000.f * dt; g.yaw = a; g.p = ZC[zi] + V(cosf(a) * 500000.f, sinf(a) * 500000.f, zp[zi].z - ZC[zi].z); zi++; } }
        pl.shake = (std::max)(0.f, pl.shake - dt); pl.fire_cool -= dt;
        if (fire && pl.fire_cool <= 0 && !pl.wreck && !pl.fall) { pl.fire_cool = 1.f / fire_rate; fired_t = sim_t; sfx(0, SK_GUN, 100); }
        pop_t = (std::max)(0.f, pop_t - dt); time_left -= dt;
        st = pl.wreck ? ST_DEAD : ST_PLAY;
        acc -= dt; sim_t += dt; st_t += dt; cnt++;
    }
    if (!::voices[9].on) buggy::play(9, (int)SK_ENG, 60);
    ::voices[9].vol = (int)(30 + pl.lever * 70.f); ::voices[9].bend = (int)((pl.lever * 5.f + pl.c.speed * 0.06f) * 32.f);
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    return true;
}
static void tick(Bitu) {
    auto now = std::chrono::steady_clock::now();
    double el = (std::min)(std::chrono::duration<double>(now - t_last).count(), 0.1); t_last = now;
    acc += el; frame_acc += el;
    const float dt = 1.f / 120.f;
    VPIX_Input in; VPIX_Input_Get(in);
    if (in.vr && in.head.valid && !have_ref) { head_ref = V(in.head.p[0], in.head.p[1], in.head.p[2]); yaw0 = atan2f(-in.head.R[1][0], in.head.R[1][1]); have_ref = true;
        dl("t=%.1f seat set (yaw %.1f)", sim_t, yaw0 * 57.3f); }
    { static float held = 0; static bool used = false;   /* hold A (menu action) 0.8 s: recentre the seat */
      if (in.start) { held += (float)el; if (held > 0.8f && !used && in.head.valid) { used = true; head_ref = V(in.head.p[0], in.head.p[1], in.head.p[2]); yaw0 = atan2f(-in.head.R[1][0], in.head.R[1][1]); have_ref = true; VPIX_Haptic(2, 0.1f, 0.6f); } }
      else { held = 0; used = false; } }
#ifdef _WIN32
    { static bool was = false; const bool k = !in.vr && (GetAsyncKeyState('C') & 0x8000) != 0; if (k && !was) { chase_on = !chase_on; dl("t=%.1f view: %s", sim_t, chase_on ? "chase" : "cockpit"); } was = k; }
#endif
    static bool trig_was = false; bool trig = (in.buttons & VPIX_BTN_TRIGGER) != 0 || in.throttle > 0.6f, sel = trig && !trig_was; trig_was = trig;
    if (autoplay && (st == ST_TITLE || st == ST_DONE) && st_t > 2.f) sel = true; if (d1k::hs::run("aces", st == ST_DONE && st_t > 1.f, score, in, trig, (float)el, autoplay)) { sel = false; st_t = 2.5f; }   /* the high score board has the screen */
    if (!v1knet::can_start()) sel = false;   /* a linked slave: pod 1 starts the game (Flying Aces is late: it always can) */
    if (link_round) {}   /* a linked round has its own states */
    else if (st == ST_TITLE && sel) { if (v1knet::on) link_start(); else new_game(); }
    else if (st == ST_REPLAY && sel && st_t > 1.f) { st = ST_DONE; st_t = 0; }
    else if (st == ST_DONE && sel && st_t > 3.f) { st = ST_TITLE; st_t = 0; foes.clear(); buls.clear(); sparks.clear(); pl.wreck = false; ::voices[9].on = false; }
    else if (st == ST_DEAD && st_t > 3.5f) {
        if (lives_total > 0 && --pl.lives <= 0) { st = ST_FADE; st_t = 0; buls.clear(); dl("t=%.1f no more chances", sim_t); }
        else { place_in_hangar(); dl("t=%.1f another chance", sim_t); }
    }
    else if (st == ST_FADE && st_t > 6.f) { st = ST_OVER; st_t = 0; ::voices[9].on = false; dl("t=%.1f game over, score %d", sim_t, score); }
    else if (st == ST_OVER && st_t > 4.5f) { st = ST_REPLAY; st_t = 0; say(score >= 600 ? V_ENDGOOD : V_ENDBAD, true); }   /* he gives his opinion of your flying at the end (The One 53) */
    else if (st == ST_REPLAY && st_t > rec.size() * 0.1f) { st = ST_DONE; st_t = 0; }
    /* the aim and lock: along the plane through the ring (ACES_HEAD_AIM=1: from the head) */
    { M cr; V eye, aim;
      if (st == ST_PLAY || st == ST_HANGAR) { cockpit_view(in, cr, eye, aim); if (autoplay || !head_aim) aim = pl.c.R.r[1]; eye_now = eye; aim_dir = aim; if (st == ST_PLAY) update_lock(); else lock_idx = -1; }
      else lock_idx = -1; }
    /* controls: the stick maps to pitch and bank, grip / left trigger to the throttle lever, trigger to the guns */
    Ctl k; bool fire = false;
    if (autoplay) { if (!link_round) auto_ctl(k, fire); }   /* a linked round: the CPU pilot code flies seat me */
    else {
        k.jx = in.joy_r[0]; k.jy = in.joy_r[1];   /* Bobby: right thumbstick flies, the left trigger is the throttle */
        float lr = 0.f; if (in.vr) lr = in.brake > 0.1f ? 0.5f : -0.5f; if (in.top) lr += 0.5f; if (in.b2) lr -= 0.5f;   /* Bobby 11-03, VR: left trigger is the throttle, let off and it comes down */
#ifdef _WIN32
        if (!in.vr) { if (GetAsyncKeyState(VK_SHIFT) & 0x8000) lr += 0.5f; if (GetAsyncKeyState(VK_CONTROL) & 0x8000) lr -= 0.5f; }
#endif
        k.lever_rate = lr; fire = trig;
    }
    if (invert && !autoplay) k.jy = -k.jy;
    if (v1knet::on && link_tick(el, in, k, fire)) { PIC_AddEvent(tick, 2.0); return; }
    int n = 0;
    while (acc >= dt && n < 24) {
        if (link_round) { link_sim(dt, k, fire); acc -= dt; sim_t += dt; st_t += dt; n++; continue; }
        if (st == ST_HANGAR || st == ST_PLAY || st == ST_FADE) step_player(k, dt);
        else if (st == ST_DEAD) step_player(Ctl(), dt);
        if (st == ST_PLAY && fire && pl.fire_cool <= 0 && !pl.fall && !pl.wreck) { pl.fire_cool = 1.f / fire_rate; gun_fire(); }
        if (st == ST_HANGAR && pl.c.p.y > 3000.f && !pl.wreck) {   /* out of the exit: the one you took is the level */
            ace_mode = pl.c.p.x > 0; if (!foes_spawned) { spawn_foes(); foes_spawned = true; }
            st = ST_PLAY; st_t = 0; sfx(5, SK_GO, 100); dl("t=%.1f left the hangar by the %s exit", sim_t, ace_mode ? "ACE" : "CADET");
        }
        if (st == ST_HANGAR || st == ST_PLAY || st == ST_DEAD || st == ST_FADE) {
            bool attack = st == ST_PLAY && !pl.fall && !pl.wreck;
            for (size_t i = 0; i < foes.size(); i++) { Foe &e = foes[i];
                if (e.gone) { e.wait += dt; if (e.wait > 6.f) spawn_one(e, (int)i, rnd(0.f, 6.28f)); }   /* the sky never empties; only the clock ends it (footage shows no other end) */
                else foe_step(e, dt, attack); }
            step_bullets(dt);
            if (foes_spawned && st != ST_FADE) {
                time_left -= dt;
                if (time_left <= 0) { time_left = 0; st = ST_FADE; st_t = 0; buls.clear(); dl("t=%.1f time up, score %d", sim_t, score); }
            }
            if (st == ST_PLAY) { chatter(dt); rec_t -= dt; if (rec_t <= 0) { rec_t = 0.1f; record_frame(); } }
        }
        if (clip_on && st != ST_REPLAY && st != ST_TITLE && st != ST_OVER && st != ST_DONE) { clip_check(pl.c, "player"); for (const Foe &e : foes) if (!e.gone) clip_check(e.c, e.alive ? "enemy" : "falling"); }
        step_train(dt);
        pop_t = (std::max)(0.f, pop_t - dt);
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    if (st == ST_HANGAR || st == ST_PLAY || st == ST_FADE) {   /* engine note follows the throttle and speed */
        if (!::voices[9].on) buggy::play(9, (int)SK_ENG, 60);
        ::voices[9].vol = (int)(30 + pl.lever * 70.f); ::voices[9].bend = (int)((pl.lever * 5.f + pl.c.speed * 0.06f) * 32.f);
        static int lock_was = -1; if (lock_idx >= 0 && lock_was < 0) sfx(4, SK_LOCK, 90); lock_was = lock_idx;
    }
    if (frame_acc >= 1.0 / game_fps) { frame_acc = fmod(frame_acc, 1.0 / game_fps); build_frame(++frames, in); }
    if (sim_t >= next_log) { next_log = sim_t + 2.0; const Craft &c = pl.c;
        int alive = 0; for (const Foe &e : foes) if (e.alive) alive++;
        dl("t=%.0f st %d plane (%.0f %.0f %.0f) spd %.1f pit %.2f bank %.2f lever %.2f ground %d stall %d hits %d | foes alive %d | clock %.0f meter %.0f score %d lock %d au %d clip %lu/%lu gap %.0f zepgap %.0f",
           sim_t, (int)st, c.p.x, c.p.y, c.p.z, c.speed, c.pitch, c.bank, pl.lever, pl.ground, pl.stalled, pl.hits, alive, time_left, meter, score, lock_idx, au_mode, clip_n, clip_checks, min_gap_m, zep_gap_m); }
    PIC_AddEvent(tick, 2.0);
}
static void start(const char *logbase) {
    if (logbase && *logbase) d1k::dlog = fopen((std::string(logbase) + ".aces").c_str(), "w");
    hits_survive = (int)envf("ACES_HITS", (float)hits_survive); foe_hits = (int)envf("ACES_FOE_HITS", (float)foe_hits); gt_hits = (int)envf("ACES_GT_HITS", (float)gt_hits);
    lives_total = (int)envf("ACES_LIVES", (float)lives_total); repair_secs = envf("ACES_REPAIR", repair_secs); foes_override = (int)envf("ACES_FOES", 0);
    round_secs = envf("ACES_ROUND", round_secs); bonus_pts = envf("ACES_BONUS_PTS", bonus_pts); bonus_secs = envf("ACES_BONUS_SECS", bonus_secs);
    pts_foe = (int)envf("ACES_PTS_FOE", (float)pts_foe); pts_red = (int)envf("ACES_PTS_RED", (float)pts_red); pts_gt = (int)envf("ACES_PTS_GT", (float)pts_gt);
    v_stall = envf("ACES_STALL", v_stall); v_lift = envf("ACES_LIFT", v_lift); v_max = envf("ACES_VMAX", v_max); thrust_acc = envf("ACES_THRUST", thrust_acc);
    bullet_speed = envf("ACES_BULLET", bullet_speed); fire_rate = envf("ACES_RATE", fire_rate); lock_deg = envf("ACES_LOCK_DEG", lock_deg); ctl_lag = (std::max)(envf("ACES_LAG", ctl_lag), 0.01f);
    lock_range = envf("ACES_LOCK_RANGE", lock_range / MM) * MM; assist = envf("ACES_ASSIST", assist);
    cadet_skill = envf("ACES_CADET_SKILL", cadet_skill); ace_skill = envf("ACES_ACE_SKILL", ace_skill); train_speed = envf("ACES_TRAIN", train_speed);
    pitch_rate = envf("ACES_PITCH_RATE", pitch_rate); game_fps = envf("ACES_FPS", game_fps); invuln_secs = envf("ACES_INVULN", invuln_secs);
    invert = envf("ACES_INVERT", 0) != 0; autoplay = envf("ACES_AUTO", 0) != 0; auto_ace = envf("ACES_AUTO_ACE", 1) != 0; head_aim = envf("ACES_HEAD_AIM", 0) != 0; skin_brown = envf("ACES_SKIN", 0) != 0; chase_on = envf("ACES_CHASE", 0) != 0; tail_text = envf("ACES_TAILTEXT", 1) != 0; clip_on = envf("ACES_CLIP_LOG", 0) != 0; ram_test = envf("ACES_RAM", 0) != 0; zep_test = (int)envf("ACES_ZEPTEST", 0);
    DRAG_K = thrust_acc / (v_max * v_max);   /* full throttle in level flight tops out at v_max */
    build_world(); make_hud_aces(); make_sounds();
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    pl = Pilot(); pl.lives = lives_total; pl.c.p = V(0, -22000.f, GROUND_Z + GEAR); orient(pl.c);
    build_targets(); time_left = round_secs;
    v1knet::init("ACES", true, dl);   /* join in progress: a pod takes a CPU seat at once and flies against CPUs while it waits (Bobby) */
    t_last = std::chrono::steady_clock::now();
    PIC_AddEvent(tick, 2.0);
    dl("Flying Aces rebuild started");
}
}   /* namespace aces */
