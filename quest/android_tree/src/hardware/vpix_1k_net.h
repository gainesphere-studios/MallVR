/* Link play for the 1000-series rebuilds: up to 4 pods on the MallVR pod network (VPIX_NET=<pod>: UDP on one PC / LAN, or the launcher's online relay; raw frames, type 0x88B5, as Buggy Ball).
   PATTERN: master-authoritative with a compact per-game snapshot, not lockstep. Pod 1 runs the game; other pods send input and draw the master's state. Why: Flying Aces and VTOL must let a player join mid-round with no wait (Bobby), and a joiner
   here just takes a CPU seat on the next snapshot; lockstep would need a full state transfer plus exact float determinism between PCs. All other games start together. Costs one snapshot writer / reader per game.

   RECIPE, one game joins (worked example: the "link play" block in vpix_dactyl1k.h and its calls in start_round, fire, tick):
   1. start(): v1knet::init("TAG4", late, dl);  late = true only for Flying Aces / VTOL (join in progress + play while waiting). Env V1K_NET=0 = alone, V1K_NET_LATE=1/0 overrides late.
   2. Packed structs: In (your pilot: position or sticks, fire sequence + shot), under IN_MAX bytes; St (state, every seat, shots, whatever is on screen), under SNAP_MAX, lists capped. `me` must be a variable, not a constant 0, and loops must not assume the human is seat 0.
   3. Round start: if (v1knet::on) { n = SEATS; begin_round(n); } with every seat starting as a CPU. Back to the title: end_round().
   4. In tick, after reading input: v1knet::pump(); then
      master: each seat i >= 1: p = seat_input(i, sizeof(In)); p ? drive seat i from p (first time: stop its AI) : give it back to the AI. if (due(30)) send_snapshot(&st, size), at the title send_snapshot(NULL, 0).
      slave:  send_input(in the game ? &in : NULL, size) each tick; take_snapshot(p, n) -> n too small or my_seat < 0 = leave the master's game, else apply it (me = my_seat); !master_heard() = leave too.
              In the master's game run only your own pilot + cosmetics, build the frame and return, skipping the normal state machine.
      A slave firing: count it in In.fire_seq, the master spawns the shot. Sounds and bursts on a slave: from changes between snapshots.
   5. Title press: if (!can_start()) sel = false. With late, a slave plays alone on CPUs until the first snapshot gives it a seat, then drops its solo round; the master starts at once and never waits.
   6. games.ps1: Link = $true; MaxPlayers = 4. Test: <game's self-play env> bash run_1k_link.sh <native name> 2 60, then read _work/link1k_p*.* for "link:" lines. */
#ifndef VPIX_1K_NET_H
#define VPIX_1K_NET_H
#include <chrono>
#include <cstddef>
namespace v1knet {
enum { SEATS = 4, PODS = 9, IN_MAX = 96, SNAP_MAX = 1440 };
#pragma pack(push, 1)
struct Hdr { uint8_t dst[6], src[6], et[2]; char magic[4]; uint8_t ver, kind, pod, round; uint32_t seq; uint8_t nseats, seat[SEATS], pad[3]; };
#pragma pack(pop)
static bool on = false, master = true, late = false;
static int pod = 1, my_seat = 0, nseats = 0;
static uint8_t seat_pod[SEATS] = {0, 0, 0, 0}, round_id = 0;   /* seat_pod: 0 = a CPU player, else the pod in that seat */
static char tag[4]; static uint32_t seq = 0; static void (*lg)(const char *, ...) = NULL;
static double heard[PODS], master_seen = -1e9, last_send = -1;
static uint8_t in_buf[PODS][IN_MAX]; static size_t in_n[PODS];
static std::vector<uint8_t> snap; static bool snap_new = false;
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static void init(const char *tag4, bool late_join, void (*log_fn)(const char *, ...)) {
    const char *e = getenv("V1K_NET"); on = VPIX_NET_Enabled() && !(e && *e == '0'); if (!on) return;
    memcpy(tag, tag4, 4); lg = log_fn; pod = VPIX_NET_Pod(); master = pod <= 1; my_seat = master ? 0 : -1;
    e = getenv("V1K_NET_LATE"); late = e && *e ? *e != '0' : late_join;
    for (int i = 0; i < PODS; i++) { heard[i] = -1e9; in_n[i] = 0; }
    lg("link: pod %d = %s%s", pod, master ? "MASTER" : "SLAVE", late ? ", join in progress" : "");
}
static bool can_start() { return !on || master || late; }
static bool master_heard() { return now() - master_seen < 3.0; }
static int live_pods() { int n = 0; for (int p = 2; p < PODS; p++) if (now() - heard[p] < 3.0) n++; return n; }
static void give_seat(int p) { for (int s = 1; s < nseats; s++) if (!seat_pod[s]) { seat_pod[s] = (uint8_t)p; in_n[p] = 0; lg("link: pod %d takes seat %d", p, s); return; } }
static void begin_round(int n) {
    if (!on || !master) return;
    nseats = n > SEATS ? SEATS : n; round_id++; memset(seat_pod, 0, sizeof(seat_pod)); seat_pod[0] = (uint8_t)pod;
    for (int p = 2; p < PODS; p++) if (now() - heard[p] < 3.0) give_seat(p);
}
static void end_round() { if (on && master) { nseats = 0; memset(seat_pod, 0, sizeof(seat_pod)); } }
static void pump() {
    if (!on) return;
    static uint8_t buf[1600]; size_t n; const double t = now();
    while ((n = VPIX_Net_PopFrame(buf, sizeof(buf))) > 0) {
        if (n < sizeof(Hdr)) continue;
        const Hdr *h = (const Hdr *)buf; const size_t body = n - sizeof(Hdr);
        if (h->et[0] != 0x88 || h->et[1] != 0xB5 || memcmp(h->magic, tag, 4) || h->ver != 1 || h->pod == pod) continue;
        if (master && h->kind == 'I' && h->pod >= 2 && h->pod < PODS) {
            if (t - heard[h->pod] > 3.0) lg("link: pod %d is here", h->pod);
            heard[h->pod] = t;
            in_n[h->pod] = h->round == round_id && body <= IN_MAX ? body : 0;
            if (in_n[h->pod]) memcpy(in_buf[h->pod], buf + sizeof(Hdr), body);
        } else if (!master && h->kind == 'S' && h->pod == 1) {
            master_seen = t; nseats = h->nseats; round_id = h->round; memcpy(seat_pod, h->seat, SEATS);
            my_seat = -1; for (int s = 0; s < nseats && s < SEATS; s++) if (seat_pod[s] == pod) my_seat = s;
            snap.assign(buf + sizeof(Hdr), buf + n); snap_new = true;
        }
    }
    if (!master) return;
    for (int s = 1; s < SEATS; s++) if (seat_pod[s] && t - heard[seat_pod[s]] > 3.0) { lg("link: pod %d gone, seat %d is a CPU player", seat_pod[s], s); seat_pod[s] = 0; }
    if (late && nseats > 0) for (int p = 2; p < PODS; p++) if (t - heard[p] < 3.0) {
        bool has = false; for (int s = 1; s < SEATS; s++) if (seat_pod[s] == p) has = true;
        if (!has) give_seat(p); }
}
/* master: the newest input of the pod in this seat, NULL = the seat is a CPU player's (or its pod isn't in the round yet) */
static const void *seat_input(int s, size_t size) { if (!on || s < 0 || s >= SEATS || !seat_pod[s] || in_n[seat_pod[s]] != size) return NULL; return in_buf[seat_pod[s]]; }
static bool take_snapshot(const uint8_t *&p, size_t &n) { if (!snap_new) return false; snap_new = false; p = snap.data(); n = snap.size(); return true; }
static bool due(double hz) { double t = now(); if (t - last_send < 1.0 / hz) return false; last_send = t; return true; }
static void tx(char kind, const void *d, size_t n) {
    static uint8_t f[1514]; Hdr *h = (Hdr *)f; if (sizeof(Hdr) + n > sizeof(f)) return;
    memset(h, 0, sizeof(*h)); memset(h->dst, 0xFF, 6); const uint8_t mac[6] = {0x00, 0x00, 0xC0, 0x56, 0x50, (uint8_t)pod}; memcpy(h->src, mac, 6);
    h->et[0] = 0x88; h->et[1] = 0xB5; memcpy(h->magic, tag, 4); h->ver = 1; h->kind = (uint8_t)kind; h->pod = (uint8_t)pod; h->round = round_id; h->seq = ++seq; h->nseats = (uint8_t)nseats; memcpy(h->seat, seat_pod, SEATS);
    if (n) memcpy(f + sizeof(Hdr), d, n);
    VPIX_Net_SendFrame(f, sizeof(Hdr) + n);
}
static void send_snapshot(const void *d, size_t n) { tx('S', d, n); }
static void send_input(const void *d, size_t n) { if (due(60)) tx('I', d, d ? n : 0); }
}   /* namespace v1knet */
#endif
