/*
 *  Polhemus InsideTrak ISA card (Virtuality SU2000 head/hand tracking) - minimal responder.
 *
 *  Interface as used by TRKI_IT_SendData / TRKI_IT_GetData in SFL.EXE (see su2000/docs/PROTOCOL.md §8):
 *    base+1  read : status. bit0 = a response word is waiting, bit1 = ready to accept a command byte
 *    base+0  write: one ASCII command byte (Polhemus command set)
 *    base+0  read : 16-bit response word, two ASCII/binary bytes, first byte in the low half
 *
 *  Milestone 1 only needs the open sequence to succeed: 'W' (drain), then 'S' must return a 28-word
 *  status record with a version number at byte 0x0F and "InsideTRAK" at byte 0x19 (byte 0x17 for
 *  firmware version 151; TRKI_IT_Open @873c0). Every other command is logged and gets no reply until
 *  Milestone 3 decodes the record format. All replies here are FAKED.
 */

#include "dosbox.h"
#include "inout.h"
#include "logging.h"
#include "su2000.h"
#include "mem.h"
#include "pic.h"
#include "paging.h"

#include <deque>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <math.h>

namespace {

/* Linear reads that never fault: the DAC hooks below look at memory of whatever game is running, and a guest page fault
   (mem_readb on an unmapped page) kills it ("Abnormal program termination: Page fault" in Boxing). Unmapped = 0. */
bool lin_ok(uint32_t a, PhysPt &phys) {
    PageNum pg = a >> 12;
    if (!PAGING_MakePhysPage(pg)) return false;
    phys = (PhysPt)((pg << 12) | (a & 0xfff));
    return true;
}
uint8_t gread8(uint32_t a) { PhysPt p; return lin_ok(a, p) ? phys_readb(p) : 0; }
uint16_t gread16(uint32_t a) { return (uint16_t)(gread8(a) | gread8(a + 1) << 8); }
uint32_t gread32(uint32_t a) { return (uint32_t)gread16(a) | (uint32_t)gread16(a + 2) << 16; }

struct InsideTrak {
    uint32_t io = 0;
    std::deque<uint8_t> out;
    std::string cmd;
    /* alignment frame per station ('A' / 'R'): rows = the frame's x, y, z axes in tracker coordinates */
    double align[4][3][3] = { { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }, { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
                              { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }, { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } } };
    IO_ReadHandleObject rd;
    IO_WriteHandleObject wr;
};

InsideTrak trk[2];
unsigned int ntrk = 0;

InsideTrak *find(Bitu port) {
    for (unsigned int i = 0; i < ntrk; i++)
        if (port >= trk[i].io && port < trk[i].io + 8) return &trk[i];
    return NULL;
}

void reply(InsideTrak &t, const char *s, size_t n) {
    for (size_t i = 0; i < n; i++) t.out.push_back((uint8_t)s[i]);
    if (n & 1) t.out.push_back(0);
}

/* Raw InsideTrak record words per sensor: x, y, z, azimuth, elevation, roll (TRKI_IT_GetRawTrackerRecord:
   position = word * 0.0916 cm, angle = word * pi/32767 rad; azimuth +pi/2 and roll -word+pi are added by the
   library). Set from [su2000] tracker pose, adjusted at run time by TRACKER_MouseDelta. */
int16_t pose[4][6];
bool calibrate = false;
double pose_f[4][6];

void set_alignment(InsideTrak &t, const std::string &line) {
    /* A<station>,Ox,Oy,Oz,Xx,Xy,Xz,Yx,Yy,Yz: origin, a point on +x, a point in the +y half plane (Polhemus alignment).
       The origin is ignored: the poses are placed in game units already (see send_record). R<station> = reset. */
    const unsigned st = (unsigned)atoi(line.c_str() + 1);
    if (st < 1 || st > 4) return;
    double (&m)[3][3] = t.align[st - 1];
    if (line[0] == 'R') {
        for (int i = 0; i < 3; i++) for (int k = 0; k < 3; k++) m[i][k] = i == k ? 1.0 : 0.0;
        return;
    }
    double v[9];
    if (sscanf(line.c_str() + 1, "%*u,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8]) != 9) return;
    double x[3], y[3];
    for (int i = 0; i < 3; i++) { x[i] = v[3 + i] - v[i]; y[i] = v[6 + i] - v[i]; }
    const double nx = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    if (nx < 1e-9) return;
    for (int i = 0; i < 3; i++) x[i] /= nx;
    const double d = x[0] * y[0] + x[1] * y[1] + x[2] * y[2];
    for (int i = 0; i < 3; i++) y[i] -= d * x[i];
    const double ny = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    if (ny < 1e-9) return;
    for (int i = 0; i < 3; i++) { y[i] /= ny; m[0][i] = x[i]; m[1][i] = y[i]; }
    m[2][0] = x[1] * y[2] - x[2] * y[1]; m[2][1] = x[2] * y[0] - x[0] * y[2]; m[2][2] = x[0] * y[1] - x[1] * y[0];
}
bool filter_seen = false;      /* DAC's data object (with this filter string) is in memory */
void command_line(InsideTrak &t, const std::string &line) {
    if (!line.empty() && line[0] == 'v') filter_seen = true;
    if (!line.empty() && (line[0] == 'A' || line[0] == 'R')) set_alignment(t, line);
    if (!line.empty() && line[0] == 'l') {
        /* active station query: bytes 3 and 4 = '1' for sensors 1 and 2 present (TRKI_IT_Open 0x97386) */
        static const char rec[8] = { '2', '1', 'l', '1', '1', ' ', '\r', '\n' };
        reply(t, rec, sizeof(rec));
    }
    static unsigned logged = 0;
    if (logged++ < 32) LOG_MSG("SU2000: InsideTrak command '%s'", line.c_str());
}

void command(InsideTrak &t, uint8_t c) {
    if (t.cmd.empty() && (c == 'S' || c == 'W' || c == 'D' || c == 'P')) {
        if (c == 'S') {
            /* 28 words = 56 bytes. Layout only as far as TRKI_IT_Open checks it. [FAKED] */
            char rec[56];
            memset(rec, ' ', sizeof(rec));
            memcpy(rec, "21S", 3);
            memcpy(rec + 0x0F, "3.00", 4);           /* version (anything but 151) */
            memcpy(rec + 0x19, "InsideTRAK", 10);
            rec[54] = '\r'; rec[55] = '\n';
            reply(t, rec, sizeof(rec));
        } else if (c == 'W') {
            t.out.clear();
        }
        return;
    }
    if (c == '\r' || c == '\n') {
        if (!t.cmd.empty()) command_line(t, t.cmd);
        t.cmd.clear();
        return;
    }
    if (t.cmd.size() < 128) t.cmd.push_back((char)c);
}

bool autoplace = true;
double target[3] = { 150.0, 450.0, 1750.0 };   /* x right, y forward, z up; head is at (0, 0, 2100) */
int32_t data_delta = 0x7fffffff;

/* VR headset input (xrserver.cpp). Angles go straight into the raw sensor words; positions become placement targets,
   expressed in the head's own yaw frame (game axes: x right, y forward, z up, mm) so they turn with the game azimuth. */
struct XrState {
    bool on = false;
    bool init = false;
    double yaw0 = 0, p0[3] = { 0, 0, 0 };
    double head_off[3] = { 0, 0, 0 };     /* head displacement from the recentre point */
    double hand_off[3] = { 0, 0, 0 };     /* hand relative to the head */
    bool hand = false;
};
XrState xr[2];                             /* per player: player 2 drives stations 3 / 4 (tracker card 2) */
double base_pose[2][6];                    /* sensor 1 / 2 words from [su2000] tracker pose */

/* Closed-loop placement of the hand sensor for DAC: the raw -> game mapping of the hand position goes through the
   library alignment and the game's own scaling, so solve it numerically. Broyden update of a 3x3 Jacobian
   (game units per raw unit), started from a measured column set. Target = DAC's FIXED_TRACKER hand. */
struct Placer {
    double J[3][3];
    double lastg[3], lastr[3];
    bool have;
};
Placer hand_pl = { { { 0.0, -0.37, 0.27 }, { 0.37, 0.27, 0.0 }, { 0.09, 0.0, 0.09 } }, { 0 }, { 0 }, false };
Placer head_pl = { { { 0.0915, 0.0, 0.0 }, { 0.0, 0.0915, 0.0 }, { 0.0, 0.0, 0.0915 } }, { 0 }, { 0 }, false };

void place(Placer &P, const double *g, double *r, const double *target) {
    double (&J)[3][3] = P.J;
    double *lastg = P.lastg, *lastr = P.lastr;
    bool &have = P.have;
    static const bool no_broyden = getenv("SU2K_NOBROYDEN") != NULL;     /* debug: keep the starting Jacobian */
    if (have && !no_broyden) {
        double dr[3], dg[3], n2 = 0;
        for (int i = 0; i < 3; i++) { dr[i] = r[i] - lastr[i]; dg[i] = g[i] - lastg[i]; n2 += dr[i] * dr[i]; }
        if (n2 > 1.0) {
            for (int i = 0; i < 3; i++) {
                double jd = 0;
                for (int k = 0; k < 3; k++) jd += J[i][k] * dr[k];
                for (int k = 0; k < 3; k++) J[i][k] += (dg[i] - jd) * dr[k] / n2;
            }
        }
    }
    double e[3], err = 0;
    for (int i = 0; i < 3; i++) { e[i] = target[i] - g[i]; err += e[i] * e[i]; }
    for (int i = 0; i < 3; i++) { lastg[i] = g[i]; lastr[i] = r[i]; }
    have = true;
    if (err < 25.0) return;
    /* solve J * dr = e (Cramer) */
    const double a = J[0][0], b = J[0][1], c = J[0][2], d = J[1][0], ee = J[1][1], f = J[1][2], gg = J[2][0], h = J[2][1], i9 = J[2][2];
    const double det = a * (ee * i9 - f * h) - b * (d * i9 - f * gg) + c * (d * h - ee * gg);
    if (fabs(det) < 1e-9) return;
    double dr[3];
    dr[0] = (e[0] * (ee * i9 - f * h) - b * (e[1] * i9 - f * e[2]) + c * (e[1] * h - ee * e[2])) / det;
    dr[1] = (a * (e[1] * i9 - f * e[2]) - e[0] * (d * i9 - f * gg) + c * (d * e[2] - e[1] * gg)) / det;
    dr[2] = (a * (ee * e[2] - e[1] * h) - b * (d * e[2] - e[1] * gg) + e[0] * (d * h - ee * gg)) / det;
    for (int k = 0; k < 3; k++) {
        double s = dr[k] * 0.5;
        if (s > 3000) s = 3000;
        if (s < -3000) s = -3000;
        r[k] += s;
    }
    TRACKER_MouseDelta(0, 0, false);
}

void find_delta(void) {
    /* find the data object once: the IT filter string "v0.2,0.2,0.8,0.8" is at link address 0xde870 */
    int32_t &delta = data_delta;
    if (delta != 0x7fffffff) return;
    static const char key[] = "v0.2,0.2,0.8,0.8\rx0.2";
    /* Other games built on the same library (Boxing) have the string too: DAC's sits at a page-aligned delta (0x16e000) */
    for (uint32_t a = 0x100870; a < 0x1000000 && delta == 0x7fffffff; a += 0x1000) {
        PhysPt ph;
        if (!lin_ok(a, ph)) continue;
        unsigned k = 0;
        while (key[k] && gread8(a + k) == (uint8_t)key[k]) k++;
        if (!key[k]) delta = (int32_t)(a - 0xde870);
    }
    LOG_MSG("SU2000: DAC data delta %08x", (unsigned)delta);
    if (delta == 0x7fffffff) delta = 0;
}

void place_direct(unsigned pl) {
    const XrState &X = xr[pl];
    double (&hp)[6] = pose_f[2 * pl], (&dp)[6] = pose_f[2 * pl + 1];
    /* Any other game: VR positions straight into the raw words. Raw axes = game axes (x right, y forward, z up) at
       0.0916 mm per word (the default head pose, raw z 22636, is DAC's 2100 mm standing height; the head Jacobian is
       0.0915 per word) [inferred from DAC]. Offsets are in the head's yaw frame; game azimuth = raw azimuth + pi/2. */
    if (!X.on) return;
    static const double S = 1.0 / 0.0916, K = 3.14159265358979 / 32767.0;
    const double az = hp[3] * K + 3.14159265358979 / 2.0, c = cos(az), s = sin(az);
    const double *o = X.head_off;
    for (int i = 0; i < 3; i++) hp[i] = base_pose[0][i];
    hp[0] += (o[0] * c - o[1] * s) * S; hp[1] += (o[0] * s + o[1] * c) * S; hp[2] += o[2] * S;
    if (X.hand) {
        const double *d = X.hand_off;
        dp[0] = hp[0] + (d[0] * c - d[1] * s) * S;
        dp[1] = hp[1] + (d[0] * s + d[1] * c) * S;
        dp[2] = hp[2] + d[2] * S;
    } else {
        for (int i = 0; i < 3; i++) dp[i] = base_pose[1][i] + hp[i] - base_pose[0][i];
    }
    TRACKER_MouseDelta(0, 0, false);
}

void debug_game_pose(void) {
    /* DAC.EXE (Solo): head[] at link 0x17d9b0, hand[] at 0x17da10, runtime = link + 0x18c000 [inferred] */
    static double last = -1e9, last_log = -1e9;
    const double now = PIC_FullIndex();
    if (now - last < (xr[0].on ? 40 : 100)) return;
    static const bool fast_log = getenv("SU2K_POSELOG") != NULL;     /* debug: log every sample */
    const bool do_log = (now - last_log) >= (fast_log ? 0 : 2000);
    if (do_log) last_log = now;
    last = now;
    /* find the data object once: the IT filter string "v0.2,0.2,0.8,0.8" is at link address 0xde870 */
    find_delta();
    const int32_t delta = data_delta;
    if (!delta) { place_direct(0); place_direct(1); return; } /* not DAC */
    float v[12];
    for (unsigned i = 0; i < 6; i++) {
        uint32_t a = gread32(0x17d9b0 + delta + 4 * i), b = gread32(0x17da10 + delta + 4 * i);
        memcpy(&v[i], &a, 4); memcpy(&v[6 + i], &b, 4);
    }
    /* calibration sweep (su2000 tracker calibrate): perturb hand raw x, y, z by +1000 in turn */
    static unsigned phase = 0;
    if (calibrate && v[0] != 0.0f) {
        static double base[3]; static bool have = false;
        if (!have) { for (int i = 0; i < 3; i++) base[i] = pose_f[1][i]; have = true; }
        const unsigned ph = (phase++ / 2) % 4;
        for (int i = 0; i < 3; i++) pose_f[1][i] = base[i] + ((ph == (unsigned)i + 1) ? 1000.0 : 0.0);
        TRACKER_MouseDelta(0, 0, false);
        LOG_MSG("SU2000: calibrate phase %u", ph);
    }
    if (autoplace && delta && v[2] > 100.0f) {
        /* head stays at standing height; the hand target turns with the head (game frame: x right, y forward, z up) */
        const double gh[3] = { v[0], v[1], v[2] }, gd[3] = { v[6], v[7], v[8] };
        const double az = v[3], c = cos(az), s = sin(az);
        double head_target[3] = { 0.0, 0.0, 2100.0 };
        double ht[3] = { target[0] * c - target[1] * s, target[0] * s + target[1] * c, target[2] };
        if (xr[0].on) {
            const double *o = xr[0].head_off;
            head_target[0] += o[0] * c - o[1] * s; head_target[1] += o[0] * s + o[1] * c; head_target[2] += o[2];
            if (xr[0].hand) {
                const double *d = xr[0].hand_off;
                ht[0] = head_target[0] + d[0] * c - d[1] * s; ht[1] = head_target[1] + d[0] * s + d[1] * c; ht[2] = head_target[2] + d[2];
            } else {
                ht[0] += head_target[0]; ht[1] += head_target[1]; ht[2] += o[2];
            }
        }
        place(head_pl, gh, pose_f[0], head_target);
        place(hand_pl, gd, pose_f[1], ht);
        if (do_log && fast_log) LOG_MSG("SU2000: pose t=%.2f hand game %.0f %.0f %.0f target %.0f %.0f %.0f raw %.0f %.0f %.0f", now / 1000.0,
                                        gd[0], gd[1], gd[2], ht[0], ht[1], ht[2], pose_f[1][0], pose_f[1][1], pose_f[1][2]);
    }
    if (do_log) LOG_MSG("SU2000: game gotIm %u sound cards %02x", (unsigned)gread8(0x17d554 + delta), (unsigned)gread8(0x18e036 + delta));
    if (do_log) LOG_MSG("SU2000: game numPlayers %u game_mode %u %u nextMode %u remote %u", (unsigned)gread32(0xd6cd4 + delta),
                        (unsigned)gread32(0x17eedc + delta), (unsigned)gread32(0x17eee0 + delta),
                        (unsigned)gread32(0x18acd4 + delta), (unsigned)gread32(0x18acd8 + delta));
    if (do_log) for (unsigned p = 2; p < 4; p++) {
        float v[3]; for (unsigned i = 0; i < 3; i++) { uint32_t u = gread32(0x17d9b0 + p * 0x18 + i * 4 + delta); memcpy(&v[i], &u, 4); }
        LOG_MSG("SU2000: game player %u head %.0f %.0f %.0f", p + 1, v[0], v[1], v[2]);
    }
    if (do_log) LOG_MSG("SU2000: game head %.1f %.1f %.1f  %.3f %.3f %.3f   hand %.1f %.1f %.1f  %.3f %.3f %.3f",
            v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11]);
}

} // namespace

/* Debug: every change of DAC's game state, sampled from the display tick (also in attract mode, where DAC does not read
   the tracker). Starts once DAC has opened the tracker (the data object is found on its first record request). */
void TRACKER_DebugTick(void) {
    if (!ntrk || !filter_seen) return;
    find_delta();
    if (data_delta == 0) return;
    const int32_t d = data_delta;
    /* numPlayers, game_mode x2, nextMode, agreed mode (0x18acd8), the other pod's game_mode x2, FourPLink, credits x4, buttons */
    static uint32_t last[12];
    const uint32_t cur[12] = { gread32(0xd6cd4 + d), gread32(0x17eedc + d), gread32(0x17eee0 + d), gread32(0x18acd4 + d),
                               gread32(0x18acd8 + d), gread32(0x17eee4 + d), gread32(0x17eee8 + d), gread16(0x17dc78 + d),
                               gread16(0x17dc70 + d), gread16(0x17dc72 + d), gread16(0x17dc74 + d) | (uint32_t)gread16(0x17dc76 + d) << 16,
                               FCARD_GetButtons() };
    if (memcmp(cur, last, sizeof(cur)) == 0) return;
    memcpy(last, cur, sizeof(cur));
    LOG_MSG("SU2000: state t=%.1f numPlayers %u game_mode %u %u nextMode %u agreed %u remote game_mode %u %u 4plink %u credit %u %u remote %u %u buttons %03x",
            PIC_FullIndex() / 1000.0, cur[0], cur[1], cur[2], cur[3], cur[4], cur[5], cur[6], cur[7], cur[8], cur[9], cur[10] & 0xFFFFu,
            cur[10] >> 16, cur[11]);
}
namespace {

void steer(void) {
    /* Ctrl+arrows (the format card stick keys) turn the hand sensor: left/right = azimuth, up/down = elevation */
    static double last = -1;
    const double now = PIC_FullIndex();
    const double dt = last < 0 ? 0 : now - last;
    last = now;
    int x, y;
    FCARD_GetStick(x, y);
    if ((x || y) && dt > 0 && dt < 200) { TRACKER_MouseDelta(-x * dt * 12.0, 0, true); TRACKER_MouseDelta(-x * dt * 12.0, -y * dt * 8.0, false); }
}

void send_record(InsideTrak &t, unsigned sensor) {
    debug_game_pose();
    steer();
    if (sensor < 1 || sensor > 4) return;
    /* The poses are worked out for DAC's alignment (Solo CONFIG.VPC WORLD_OFFSET 0 0 0 180 0 180 -> 'A' with y and z
       reversed). Another alignment (e.g. no WORLD_OFFSET: Ghost Train on the two-player disk) gets the same physical pose
       seen through its own frame: M = A_station * A_DAC^T applied to position and orientation. */
    int16_t p[6];
    /* station: card 1 has stations 1 / 2 (player 1), card 2 stations 3 / 4 (player 2; CONFIG.VPC SENSOR_MAP ..12) */
    const unsigned station = (unsigned)(&t - trk) * 2u + sensor;
    if (station > 4) return;
    memcpy(p, pose[station - 1], sizeof(p));
    {
        const double (&a)[3][3] = t.align[sensor - 1];
        static const double base_diag[3] = { 1.0, -1.0, -1.0 };
        double M[3][3];
        bool ident = true;
        for (int i = 0; i < 3; i++)
            for (int k = 0; k < 3; k++) {
                M[i][k] = 0;
                M[i][k] = a[i][k] * base_diag[k];
            }
        for (int i = 0; i < 3; i++)
            for (int k = 0; k < 3; k++)
                if (fabs(M[i][k] - (i == k ? 1.0 : 0.0)) > 1e-6) ident = false;
        if (!ident) {
            static const double K = 3.14159265358979 / 32767.0;
            const double az = p[3] * K, el = p[4] * K, rl = p[5] * K;
            const double ca = cos(az), sa = sin(az), ce = cos(el), se = sin(el), cr = cos(rl), sr = sin(rl);
            /* R = Rz(az) Ry(el) Rx(roll) */
            const double R[3][3] = { { ca * ce, ca * se * sr - sa * cr, ca * se * cr + sa * sr },
                                      { sa * ce, sa * se * sr + ca * cr, sa * se * cr - ca * sr },
                                      { -se, ce * sr, ce * cr } };
            double Q[3][3], pos[3];
            for (int i = 0; i < 3; i++) {
                pos[i] = M[i][0] * p[0] + M[i][1] * p[1] + M[i][2] * p[2];
                for (int k = 0; k < 3; k++) Q[i][k] = M[i][0] * R[0][k] + M[i][1] * R[1][k] + M[i][2] * R[2][k];
            }
            const double q20 = Q[2][0] > 1.0 ? 1.0 : (Q[2][0] < -1.0 ? -1.0 : Q[2][0]);
            const double ang[3] = { atan2(Q[1][0], Q[0][0]), asin(-q20), atan2(Q[2][1], Q[2][2]) };
            for (int i = 0; i < 3; i++) {
                const double v = pos[i] > 32767.0 ? 32767.0 : (pos[i] < -32767.0 ? -32767.0 : pos[i]);
                double w = ang[i] / K;
                if (w > 32767.0) w = 32767.0;
                if (w < -32767.0) w = -32767.0;
                p[i] = (int16_t)v; p[3 + i] = (int16_t)w;
            }
        }
    }
    uint8_t rec[16];
    rec[0] = '0'; rec[1] = (uint8_t)('0' + sensor);       /* header word [inferred, not checked by DAC] */
    for (unsigned i = 0; i < 6; i++) { rec[2 + 2 * i] = (uint8_t)p[i]; rec[3 + 2 * i] = (uint8_t)((uint16_t)p[i] >> 8); }
    rec[14] = rec[15] = 0;
    reply(t, (const char *)rec, sizeof(rec));
}

Bitu trk_read(Bitu port, Bitu iolen) {
    InsideTrak *t = find(port);
    Bitu v = 0xFF;
    if (t) {
        if (port - t->io == 1) {
            v = 0x02u | (t->out.empty() ? 0u : 0x01u);
        } else if (port - t->io == 0) {
            uint8_t lo = 0, hi = 0;
            if (!t->out.empty()) { lo = t->out.front(); t->out.pop_front(); }
            if (iolen >= 2 && !t->out.empty()) { hi = t->out.front(); t->out.pop_front(); }
            v = (Bitu)lo | ((Bitu)hi << 8u);
        } else {
            v = 0;
        }
    }
    SU2K_Log(SU2K_IO_READ, (uint8_t)iolen, (uint32_t)port, (uint32_t)v);
    return v;
}

void trk_write(Bitu port, Bitu val, Bitu iolen) {
    SU2K_Log(SU2K_IO_WRITE, (uint8_t)iolen, (uint32_t)port, (uint32_t)val);
    InsideTrak *t = find(port);
    if (!t) return;
    if (port == t->io) command(*t, (uint8_t)val);
    else send_record(*t, (unsigned)(port - t->io));   /* base+n: request a record for hardware sensor n */
}
} // namespace

void TRACKER_SetCalibrate(bool on) { calibrate = on; }

static bool use_mouse = false;
void TRACKER_SetMouse(bool on) { use_mouse = on; }

/* Called from the DOS mouse emulation. While a tracker is emulated and [su2000] tracker mouse is on, the mouse aims the
   hand sensor and its buttons are the Solo player's trigger (left, format card button 8 = Ctrl+5) and walk button
   (right, button 9 = Ctrl+6); DOS programs then see no mouse. */
bool SU2000_MouseMove(float xrel, float yrel) {
    if (!use_mouse || !ntrk) return false;
    TRACKER_MouseDelta(-xrel * 40.0, 0, true);      /* turn: head and hand together */
    TRACKER_MouseDelta(-xrel * 40.0, -yrel * 40.0, false);
    return true;
}

bool SU2000_MouseButton(uint8_t button, bool pressed) {
    if (!use_mouse || !ntrk) return false;
    if (button == 0) FCARD_SetButton(8, pressed);
    else if (button == 1) FCARD_SetButton(9, pressed);
    return true;
}

void TRACKER_SetHandTarget(const char *s) {
    double v[3];
    if (s && sscanf(s, "%lf %lf %lf", &v[0], &v[1], &v[2]) == 3)
        for (int i = 0; i < 3; i++) target[i] = v[i];
}

void TRACKER_SetPose(const char *s) {
    double v[24] = { 0 };
    unsigned n = 0;
    while (s && *s && n < 24) {
        char *e;
        const double d = strtod(s, &e);
        if (e == s) { s++; continue; }
        v[n++] = d; s = e;
    }
    if (n <= 12) for (unsigned i = 12; i < 24; i++) v[i] = v[i - 12];   /* player 2 (stations 3 / 4) starts like player 1 */
    for (unsigned i = 0; i < 24; i++) pose_f[i / 6][i % 6] = v[i];
    for (unsigned i = 0; i < 12; i++) base_pose[i / 6][i % 6] = v[i];
    TRACKER_MouseDelta(0, 0, false);
}

/* WebXR pose (reference space: x right, y up, -z forward, metres; quaternion x y z w) -> yaw (counter-clockwise seen
   from above) and pitch (up positive) of the forward axis */
static void xr_angles(const double *p, double &yaw, double &pitch) {
    const double x = p[3], y = p[4], z = p[5], w = p[6];
    /* forward = q * (0, 0, -1) */
    const double fx = -2.0 * (x * z + w * y), fy = -2.0 * (y * z - w * x), fz = -(1.0 - 2.0 * (x * x + y * y));
    yaw = atan2(-fx, -fz);
    pitch = asin(fy < -1 ? -1 : fy > 1 ? 1 : fy);
}

/* world vector (metres) -> head yaw frame in game axes (mm): x right, y forward, z up */
static void xr_local(const double *d, double yaw, double *out) {
    const double c = cos(yaw), s = sin(yaw);
    const double lx = d[0] * c - d[2] * s, lz = d[0] * s + d[2] * c;
    out[0] = lx * 1000.0; out[1] = -lz * 1000.0; out[2] = d[1] * 1000.0;
}

void TRACKER_XRPose(int player, const double *head, const double *hand, bool recenter) {
    if (player < 1 || player > 2) return;
    XrState &X = xr[player - 1];
    const unsigned h = 2u * (unsigned)(player - 1);
    if (!head) { X = XrState(); return; }
    static const double K = 32767.0 / 3.14159265358979;
    double yaw, pitch;
    xr_angles(head, yaw, pitch);
    if (!X.init || recenter) {
        X.yaw0 = yaw;
        for (int i = 0; i < 3; i++) X.p0[i] = head[i];
        X.init = true;
        LOG_MSG("SU2000: VR recentre (player %d)", player);
    }
    X.on = true;
    double dyaw = yaw - X.yaw0;
    pose_f[h][3] = base_pose[0][3] + dyaw * K;
    pose_f[h][4] = base_pose[0][4] + pitch * K;
    const double d[3] = { head[0] - X.p0[0], head[1] - X.p0[1], head[2] - X.p0[2] };
    xr_local(d, yaw, X.head_off);
    const double r = sqrt(X.head_off[0] * X.head_off[0] + X.head_off[1] * X.head_off[1] + X.head_off[2] * X.head_off[2]);
    if (r > 1500.0) for (int i = 0; i < 3; i++) X.head_off[i] *= 1500.0 / r;
    X.hand = hand != NULL;
    if (hand) {
        double hy, hp;
        xr_angles(hand, hy, hp);
        pose_f[h + 1][3] = base_pose[1][3] + (hy - X.yaw0) * K;
        pose_f[h + 1][4] = base_pose[1][4] + hp * K;
        /* wrist roll: the controller's right axis dipping below level (sign unverified; the library negates the roll word) */
        const double x = hand[3], y = hand[4], z = hand[5], w = hand[6];
        const double ry = 2.0 * (x * y + w * z), uy = 1.0 - 2.0 * (x * x + z * z);
        pose_f[h + 1][5] = base_pose[1][5] - atan2(-ry, uy) * K;
        while (pose_f[h + 1][5] > 32767) pose_f[h + 1][5] -= 65534;
        while (pose_f[h + 1][5] < -32767) pose_f[h + 1][5] += 65534;
        const double dh[3] = { hand[0] - head[0], hand[1] - head[1], hand[2] - head[2] };
        xr_local(dh, yaw, X.hand_off);
    } else {
        pose_f[h + 1][3] = base_pose[1][3] + dyaw * K;
        pose_f[h + 1][4] = base_pose[1][4];
    }
    for (unsigned s = h; s < h + 2; s++) {
        while (pose_f[s][3] > 32767) pose_f[s][3] -= 65534;
        while (pose_f[s][3] < -32767) pose_f[s][3] += 65534;
    }
    TRACKER_MouseDelta(0, 0, false);
}

void TRACKER_MouseDelta(double dx, double dy, bool head) {
    /* mouse moves the hand (or, with head = true, the head) in azimuth / elevation, in raw angle units */
    double *p = pose_f[head ? 0 : 1];
    p[3] += dx; p[4] += dy;
    if (p[4] > 14000) p[4] = 14000;
    if (p[4] < -14000) p[4] = -14000;
    while (p[3] > 32767) p[3] -= 65534;
    while (p[3] < -32767) p[3] += 65534;
    for (unsigned s = 0; s < 4; s++)
        for (unsigned i = 0; i < 6; i++) {
            double d = pose_f[s][i];
            if (d > 32767) d = 32767;
            if (d < -32767) d = -32767;
            pose[s][i] = (int16_t)d;
        }
}

void TRACKER_Setup(const uint32_t *ports, unsigned int n) {
    TRACKER_Shutdown();
    ntrk = n > 2 ? 2 : n;
    for (unsigned int i = 0; i < ntrk; i++) {
        trk[i].io = ports[i];
        trk[i].out.clear();
        trk[i].rd.Install(ports[i], trk_read, IO_MA, 8);
        trk[i].wr.Install(ports[i], trk_write, IO_MA, 8);
    }
    if (ntrk) LOG_MSG("SU2000: InsideTrak responder at %03xh%s", ports[0], ntrk > 1 ? " (+1)" : "");
}

void TRACKER_Shutdown(void) {
    for (unsigned int i = 0; i < ntrk; i++) {
        trk[i].rd.Uninstall();
        trk[i].wr.Uninstall();
    }
    ntrk = 0;
}
