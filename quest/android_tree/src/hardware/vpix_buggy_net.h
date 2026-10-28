/* Buggy Ball link play: up to 4 pods on the MallVR pod network (VPIX_NET=<pod>: UDP between copies on one PC / a LAN, or the
   launcher's online relay, same as the linked DOS games). Included by vpix_buggy.h inside namespace buggy.
   Buggy Ball's own design (BALL.MAP: TX_controller / RX_gamestuff / GAME_netgameloop, operator menu NETWORK MASTER / SLAVE):
   the MASTER pod (1) runs the whole game - menus, physics, computer cars, goals, timer - and each SLAVE pod sends its controls
   and draws the master's state. Ours does the same with its own packets: raw Ethernet II, type 0x88B5, broadcast, 60 a second
   each way. Between state packets a slave runs the same physics on the last known controls (own car on its live controls), so
   motion stays smooth; each state packet then puts every car and the ball back where the master has them.
   Slaves play their own attract while the master is on its title; the master's trigger starts the game for everyone; the master
   chooses the level, each player chooses their own car (the master's pick starts the round). A slave silent for 3 s becomes a
   computer car; a slave that hears nothing from the master for 3 s goes back to its title.
   Env: VPIX_BUGGY_NET=0 = play alone even with VPIX_NET set. */
namespace net {
static bool on = false, master = true; static int pod = 1;
static double t0_s = 0;
static double now_s() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count() - t0_s; }
#pragma pack(push, 1)
struct Hdr { uint8_t dst[6], src[6], et[2]; char magic[4]; uint8_t ver, kind, pod, pad; uint32_t seq; };
struct In { Hdr h; float steer, throttle; uint8_t boost, car_pick, pad[2]; };
struct NCar { int8_t type, pod; uint8_t score, cpu; float p[3], v[3], w[3], R[9]; float steer, throttle, boost_left, boost_charge; float ws[4], spin[4]; };
struct St {
    Hdr h; uint8_t st, level, ncars, goal_by, scored, sel_level, pad[2];
    float st_t, time_left, level_left, goal_banner, level_banner;
    float bp[3], bv[3], bw[3], bR[9]; NCar car[4];
};
#pragma pack(pop)
struct Remote { float steer = 0, throttle = 0; bool boost = false; int pick = 0; double seen = -1e9; };
static Remote rin[9];
static double master_seen = -1e9, last_send = -1; static bool follow = false; static uint32_t seq = 0;
static bool have_car = false;
static unsigned long n_rx = 0, n_tx = 0;

static void init() {
    const char *e = getenv("VPIX_BUGGY_NET");
    on = VPIX_NET_Enabled() && !(e && *e == '0');
    if (!on) return;
    pod = VPIX_NET_Pod(); master = pod <= 1; t0_s = now_s();
    bl("link play: pod %d = %s", pod, master ? "MASTER (runs the game)" : "SLAVE (sends controls, shows the master's game)");
}
static void hdr(Hdr &h, char kind) {
    memset(h.dst, 0xFF, 6);
    const uint8_t mac[6] = {0x00, 0x00, 0xC0, 0x56, 0x50, (uint8_t)pod}; memcpy(h.src, mac, 6);
    h.et[0] = 0x88; h.et[1] = 0xB5; memcpy(h.magic, "BUGY", 4); h.ver = 1; h.kind = (uint8_t)kind; h.pod = (uint8_t)pod; h.pad = 0; h.seq = ++seq;
}
static void v3(float *o, V a) { o[0] = a.x; o[1] = a.y; o[2] = a.z; }
static V v3(const float *o) { return V(o[0], o[1], o[2]); }
static void m9(float *o, const M &m) { for (int k = 0; k < 3; k++) v3(o + 3 * k, m.r[k]); }
static void m9(M &m, const float *o) { for (int k = 0; k < 3; k++) m.r[k] = v3(o + 3 * k); ortho(m); }

/* pods that have been heard from in the last 3 s (slaves; the master counts itself apart) */
static std::vector<int> live_pods() {
    std::vector<int> v; double t = now_s();
    for (int p = 2; p <= 8; p++) if (t - rin[p].seen < 3.0) v.push_back(p);
    return v;
}
/* master, start of a round: one car for each linked player after your own (slot 0); returns how many */
static int add_humans() {
    if (!on || !master) return 0;
    if (!cars.empty()) cars[0].pod = pod;
    int h = 0;
    for (int p : live_pods()) {
        if (h >= 3) break;
        int car = lv.cars.empty() ? 3 : lv.cars[rin[p].pick % lv.cars.size()];
        make_car(car, (int)cars.size(), false);
        cars.back().pod = p; h++;
        bl("link: pod %d drives car %zu (type %d)", p, cars.size() - 1, car);
    }
    return h;
}

static void stop_all() { movie_stop(); for (auto &v : voices) v.on = false; }
/* slave: what the master's state change does here (its sounds and movies) */
static void enter(State ns, int my_score) {
    State was = st;
    st = ns;
    bl("t=%.1f link: master state %d -> %d", sim_t, (int)was, (int)ns);
    switch (ns) {
    case S_LEVEL: play(14, sound(29), 100); break;
    case S_CAR: sel_car = 0; play(14, sound(29), 100); break;
    case S_INTRO: movie("RUNFILES/FLIC/FINS.FLC", 0); break;
    case S_COUNT: if (was == S_INTRO) movie_stop(); play(14, sound(was == S_PLAY ? 29 : 30), 110); break;
    case S_PLAY: play(14, sound(30), 120); if (!voices[31].on) music_play(); break;
    case S_OVER: music_stop(); movie("RUNFILES\\FLIC\\END.FLC", 1); play(13, sound(my_score >= 1 ? 50 : 60), 120); break;
    default: break;
    }
}
static void back_to_title(const char *why) {
    follow = false; stop_all(); cars.clear(); me = 0; have_car = false; demo = false; attract = 0; set_state(S_TITLE);
    bl("t=%.1f link: %s - back to the title", sim_t, why);
}
static void apply(const St &s) {
    master_seen = now_s();
    State ns = (State)s.st;
    if (ns == S_TITLE) { if (follow) back_to_title("master is on its title"); return; }
    if (!follow) { stop_all(); demo = false; cars.clear(); follow = true; st = S_TITLE; bl("t=%.1f link: joining the master's game", sim_t); }
    if ((int)s.level != cur_level) {
        setup_level(s.level);
        for (auto &c : cars) c.d = &cardefs[c.type];   /* setup_level reloads the car table */
        show_car = Car();
    }
    sel_level = s.sel_level;
    /* cars: same list as the master (type + who drives it), else build it again */
    int nc = (std::min)((int)s.ncars, 4);
    bool same = (int)cars.size() == nc;
    for (int i = 0; same && i < nc; i++) same = cars[i].type == s.car[i].type && cars[i].pod == s.car[i].pod;
    if (!same) {
        cars.clear();
        for (int i = 0; i < nc; i++) { make_car(s.car[i].type, i, s.car[i].cpu != 0); cars.back().pod = s.car[i].pod; }
        bl("t=%.1f link: %d cars from the master", sim_t, nc);
    }
    me = 0; have_car = false;
    for (int i = 0; i < nc; i++) if (cars[i].pod == pod) { me = i; have_car = true; }
    int goals_before = 0, goals_after = 0;
    for (int i = 0; i < nc; i++) {
        Car &c = cars[i]; const NCar &n = s.car[i];
        goals_before += c.score; goals_after += n.score;
        c.p = v3(n.p); c.v = v3(n.v); c.w = v3(n.w); m9(c.R, n.R);
        c.score = n.score; c.cpu = n.cpu != 0;
        if (!(have_car && i == me)) { c.steer = n.steer; c.throttle = n.throttle; }   /* own car keeps your live controls */
        c.boost_left = n.boost_left; c.boost_charge = n.boost_charge;
        for (int k = 0; k < 4; k++) { c.wh[k].s = c.wh[k].s_prev = n.ws[k]; c.wh[k].spin = n.spin[k]; }
    }
    if (ns != st) enter(ns, have_car ? cars[me].score : 0);
    st_t = s.st_t; time_left = s.time_left; level_left = s.level_left; level_banner = s.level_banner;
    if (goals_after > goals_before && st == S_PLAY) {   /* a goal on the master: banner, cheer, rumble if it was yours */
        play(13, sound_file_key("SOUNDS\\BONUS.WAV", 1), 120);
        if (have_car && s.goal_by == me) VPIX_Haptic(2, 0.4f, 1.f);
    }
    goal_banner = s.goal_banner; goal_by = s.goal_by == 255 ? -1 : s.goal_by;
    ball.p = v3(s.bp); ball.v = v3(s.bv); ball.w = v3(s.bw); m9(ball.R, s.bR); ball.scored = s.scored != 0;
}
static void receive() {
    uint8_t buf[1600];
    size_t n;
    while ((n = VPIX_Net_PopFrame(buf, sizeof(buf))) > 0) {
        if (n < sizeof(Hdr)) continue;
        const Hdr *h = (const Hdr *)buf;
        if (h->et[0] != 0x88 || h->et[1] != 0xB5 || memcmp(h->magic, "BUGY", 4) || h->ver != 1 || h->pod == pod) continue;
        n_rx++;
        if (master && h->kind == 'I' && n >= sizeof(In) && h->pod >= 2 && h->pod <= 8) {
            const In *p = (const In *)buf; Remote &r = rin[h->pod];
            if (r.seen < now_s() - 3.0) bl("t=%.1f link: pod %d is here", sim_t, h->pod);
            r.steer = clampf(p->steer, -1, 1); r.throttle = clampf(p->throttle, -1, 1); r.boost = p->boost != 0; r.pick = p->car_pick; r.seen = now_s();
        } else if (!master && h->kind == 'S' && h->pod == 1 && n >= sizeof(St)) apply(*(const St *)buf);
    }
}

/* every tick, before the state machine; true = this slave is in the master's game (skip its own state machine) */
static bool pre_tick(float el, bool &sel, bool left, bool right) {
    if (!on) return false;
    receive();
    double t = now_s();
    if (master) {   /* linked players' cars take their pod's controls; a pod silent for 3 s hands its car to the computer */
        for (size_t i = 0; i < cars.size(); i++) {
            Car &c = cars[i];
            if (c.pod < 2) continue;
            Remote &r = rin[c.pod];
            if (t - r.seen > 3.0) { if (!c.cpu) { c.cpu = true; bl("t=%.1f link: pod %d gone, car %zu is a computer car now", sim_t, c.pod, i); } continue; }
            if (c.cpu) bl("t=%.1f link: pod %d back in car %zu", sim_t, c.pod, i);
            c.cpu = false; c.steer = r.steer;
            c.throttle = st == S_PLAY ? r.throttle : 0.f; c.boost_req = st == S_PLAY && r.boost;
        }
        return false;
    }
    if (follow && t - master_seen > 3.0) back_to_title("no word from the master for 3 s");
    if (!follow) { sel = false; return false; }   /* own attract plays; the master's trigger starts the game */
    if (st == S_CAR) {   /* your own car pick (sent to the master) */
        int n = (std::max)(1, (int)lv.cars.size());
        if (left) { sel_car = (sel_car + n - 1) % n; play(14, sound(4), 70); }
        if (right) { sel_car = (sel_car + 1) % n; play(14, sound(4), 70); }
    }
    if ((st == S_INTRO || st == S_OVER) && mov_on) movie_run(el);
    return true;
}
static bool following() { return on && !master && follow; }

/* after the simulation: the master sends the state, a slave its controls (60 a second) */
static void post_tick() {
    if (!on) return;
    double t = now_s();
    if (t - last_send < 1.0 / 60.0) return;
    last_send = t;
    if (master) {
        St s; memset(&s, 0, sizeof(s)); hdr(s.h, 'S');
        s.st = (uint8_t)st; s.level = (uint8_t)(cur_level < 0 ? 0 : cur_level); s.sel_level = (uint8_t)sel_level;
        s.goal_by = goal_by < 0 ? 255 : (uint8_t)goal_by; s.scored = ball.scored;
        s.st_t = st_t; s.time_left = time_left; s.level_left = level_left; s.goal_banner = goal_banner; s.level_banner = level_banner;
        v3(s.bp, ball.p); v3(s.bv, ball.v); v3(s.bw, ball.w); m9(s.bR, ball.R);
        bool game = st != S_TITLE;
        s.ncars = game ? (uint8_t)(std::min)(cars.size(), (size_t)4) : 0;
        for (int i = 0; i < s.ncars; i++) {
            const Car &c = cars[i]; NCar &n = s.car[i];
            n.type = (int8_t)c.type; n.pod = (int8_t)c.pod; n.score = (uint8_t)c.score; n.cpu = c.cpu;
            v3(n.p, c.p); v3(n.v, c.v); v3(n.w, c.w); m9(n.R, c.R);
            n.steer = c.steer; n.throttle = c.throttle; n.boost_left = c.boost_left; n.boost_charge = c.boost_charge;
            for (int k = 0; k < 4; k++) { n.ws[k] = c.wh[k].s; n.spin[k] = c.wh[k].spin; }
        }
        VPIX_Net_SendFrame((const uint8_t *)&s, sizeof(s)); n_tx++;
    } else {
        In p; memset(&p, 0, sizeof(p)); hdr(p.h, 'I');
        if (follow && have_car && st == S_PLAY) {
            Car &c = cars[me];
            if (autopilot) cpu_drive(c, 1.f / 60.f);   /* test runs: the slave's car drives itself, through the link */
            p.steer = c.steer; p.throttle = c.throttle; p.boost = c.boost_req;
        }
        p.car_pick = (uint8_t)sel_car;
        VPIX_Net_SendFrame((const uint8_t *)&p, sizeof(p)); n_tx++;
    }
    static double next_log = 0;
    if (t >= next_log) { next_log = t + 5.0;
        bl("t=%.1f link: %s, sent %lu, got %lu, players %zu%s", sim_t, master ? "master" : follow ? "slave in game" : "slave waiting",
           n_tx, n_rx, live_pods().size() + 1, have_car ? "" : (master ? "" : " (no car yet)")); }
}

/* the link's lines on the 2D screen (352 x 288) */
static void draw(VPIX_Frame &f) {
    if (!on) return;
    const float CX = 176.f;
    if (master) {
        if (st == S_TITLE || st == S_LEVEL || st == S_CAR) {
            size_t h = live_pods().size();
            text(f, h ? "LINKED  " + std::to_string(h + 1) + " PLAYERS" : std::string("WAITING FOR LINKED PLAYERS"), CX, 10, 0.6f, 0x80FFFF, true);
        }
        return;
    }
    if (!follow) text(f, now_s() - master_seen < 3.0 ? "LINKED  PLAYER 1 STARTS THE GAME" : "LOOKING FOR PLAYER 1", CX, 10, 0.6f, 0x80FFFF, true);
    else if (st == S_LEVEL) text(f, "PLAYER 1 CHOOSES THE LEVEL", CX, 240, 0.6f, 0x80FFFF, true);
    else if (st == S_CAR) text(f, "PLAYER 1 STARTS THE ROUND", CX, 255, 0.6f, 0x80FFFF, true);
}
}   /* namespace net */
