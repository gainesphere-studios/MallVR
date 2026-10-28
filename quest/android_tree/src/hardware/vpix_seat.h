/* Remote seat (FOUR_PLAYER_PLAN route 4): a Duo game's second seat played from another MallVR copy. Included at the end of
   vpix_hle.cpp (uses its statics).
   Host PC:  VPIX_SEAT_HOST=<udp port>    takes the seat PC's input, feeds it to seat 2 (sensors 3/4, joystick 2, front-panel
             start), sends seat 2's frame (window 1 = PIX channel 2) and the game's sound commands back every game frame. Replies
             go to wherever the input came from (the seat PC, or the launcher's online relay port).
   Seat PC:  VPIX_SEAT_CLIENT=<host:port> [VPIX_SEAT_BIND=<port>] runs the same game only for its models / bitmaps / textures /
             sounds (same files loaded in the same order = the same handles), sends its input every frame, draws the host's frames
             and plays the host's sounds; its own game gets no buttons (stays in attract) and no sound.
   play.ps1 -Pod 1 / -Pod 2 sets these (games.ps1 Seat2); online the launcher relays them like pod frames (MallOnline, <= 1600 bytes
   per packet), so frames go out in pieces.
   Packets: "SEAT" + 'I' + VPIX_Input (both sides run the same build); "SEAT" + 'P' + piece header + part of a frame
   (FrameHdr + instances + blits); "SEAT" + 'S' + sound commands. */

struct SeatFrameHdr {
    uint32_t frame;
    float cam_R[3][3], cam_T[3], fov_deg;
    uint8_t clear[3], fog_col[3];
    float fog_start, fog_end;
    uint8_t aim_beam, in_head, hold, pad;
    float aim_o[3], aim_d[3];
    VPIX_Pose head_fed;
    float player_height, crouch_mm;
    int32_t game_mode;
    uint32_t ninst, nblit;
};
#pragma pack(push, 1)
struct SeatPiece { uint32_t id; uint16_t idx, count; uint32_t total; };   /* frame id, piece number, pieces, whole frame bytes */
struct SeatSnd { uint8_t op, ch; int8_t semis, pad; uint32_t edx, ebx; };
#pragma pack(pop)
static const size_t SEAT_PIECE = 1300;      /* payload per packet: the online relay drops anything over 1600 */

static int seat_sock = -2;                  /* -2 = not set up yet, -1 = off / failed */
static bool seat_is_host = false, seat_is_client = false;
static std::string seat_peer_host; static int seat_peer_port = 0;   /* host: where the input came from; client: the host */
static double seat_last_in = -1e9;          /* host: when the seat PC's input last arrived (s) */
static std::vector<uint8_t> seat_rx_frame;  /* client: the latest whole frame (SeatFrameHdr + arrays) */
static double seat_rx_at = -1e9;
static unsigned long seat_tx = 0, seat_rx = 0, seat_rx_bad = 0;
static std::vector<SeatSnd> seat_snd_out;   /* host: this frame's sound commands */

static double seat_now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

static void seat_setup() {
    if (seat_sock != -2) return;
    seat_sock = -1;
    const char *h = getenv("VPIX_SEAT_HOST"), *c = getenv("VPIX_SEAT_CLIENT"), *b = getenv("VPIX_SEAT_BIND");
    if (h && *h) {
        seat_sock = VPIX_Udp_Open(atoi(h)); seat_is_host = seat_sock >= 0;
        lg("SEAT host on UDP %d: %s", atoi(h), seat_is_host ? "listening for the seat PC" : "** bind failed");
    } else if (c && *c) {
        std::string s(c); size_t k = s.rfind(':');
        seat_peer_host = k == std::string::npos ? s : s.substr(0, k); seat_peer_port = k == std::string::npos ? 27101 : atoi(s.c_str() + k + 1);
        seat_sock = VPIX_Udp_Open(b && *b ? atoi(b) : 0); seat_is_client = seat_sock >= 0;
        lg("SEAT client (bound %d) -> %s:%d %s", b && *b ? atoi(b) : 0, seat_peer_host.c_str(), seat_peer_port, seat_is_client ? "" : "** socket failed");
    }
}

static bool seat2_live() { return seat_is_host && seat_now() - seat_last_in < 2.0; }
static bool seat_client_on() { seat_setup(); return seat_is_client; }
static bool seat_sound_local_off() { return seat_client_on(); }
static void seat_sound_event(int op, uint32_t ch, uint32_t edx, uint32_t ebx, int semis) {
    if (!seat2_live() || seat_snd_out.size() >= 120) return;
    SeatSnd e; e.op = (uint8_t)op; e.ch = (uint8_t)ch; e.semis = (int8_t)semis; e.pad = 0; e.edx = edx; e.ebx = ebx;
    seat_snd_out.push_back(e);
}

/* drain the socket: host keeps the newest input; client puts frame pieces together and plays sound commands */
static void seat_poll() {
    static std::vector<uint8_t> buf(70000);
    static uint32_t asm_id = 0; static std::vector<uint8_t> asm_buf; static std::vector<bool> asm_have; static uint16_t asm_left = 0;
    char from[64]; int fport = 0, n;
    while ((n = VPIX_Udp_Recv(seat_sock, buf.data(), buf.size(), from, sizeof(from), &fport)) >= 5) {
        if (memcmp(buf.data(), "SEAT", 4)) continue;
        const char k = (char)buf[4];
        if (seat_is_host && k == 'I' && n == 5 + (int)sizeof(VPIX_Input)) {
            bool was = seat2_live();
            memcpy(&seat2_in, buf.data() + 5, sizeof(VPIX_Input));
            seat_last_in = seat_now(); seat_rx++;
            if (!was || seat_peer_host != from || seat_peer_port != fport) lg("SEAT seat PC %s:%d connected (seat 2 now played from there)", from, fport);
            seat_peer_host = from; seat_peer_port = fport;
        } else if (seat_is_client && k == 'P' && n >= 5 + (int)sizeof(SeatPiece)) {
            SeatPiece pc; memcpy(&pc, buf.data() + 5, sizeof(pc));
            size_t len = n - 5 - sizeof(pc), at = (size_t)pc.idx * SEAT_PIECE;
            if (pc.count == 0 || pc.idx >= pc.count || pc.total > 200000 || at + len > pc.total) { seat_rx_bad++; continue; }
            if ((int32_t)(pc.id - asm_id) < 0) continue;              /* an older frame's piece */
            if (pc.id != asm_id || asm_buf.size() != pc.total) {      /* a newer frame starts (an unfinished one is dropped) */
                asm_id = pc.id; asm_buf.assign(pc.total, 0); asm_have.assign(pc.count, false); asm_left = pc.count;
            }
            if (asm_have[pc.idx]) continue;
            memcpy(asm_buf.data() + at, buf.data() + 5 + sizeof(pc), len); asm_have[pc.idx] = true;
            if (--asm_left == 0) { seat_rx_frame.swap(asm_buf); asm_buf.clear(); seat_rx_at = seat_now(); seat_rx++; asm_id++; }
        } else if (k == 'V' && n > 5) {
            voice_rx(seat_is_host ? 2 : 1, buf.data() + 5, n - 5);   /* voice chat (vpix_voice.h) */
        } else if (seat_is_client && k == 'S') {
            for (int o = 5; o + (int)sizeof(SeatSnd) <= n; o += sizeof(SeatSnd)) {
                SeatSnd e; memcpy(&e, buf.data() + o, sizeof(e));
                snd_apply(e.op, e.ch, e.edx, e.ebx, e.semis);
            }
        }
    }
}

/* once per game frame, after the normal frame went out */
static void seat_tick() {
    seat_setup();
    if (seat_sock < 0) return;
    seat_poll();
    if (seat_is_client) {
        uint8_t pk[5 + sizeof(VPIX_Input)]; memcpy(pk, "SEATI", 5);
        VPIX_Input in; VPIX_Input_Get(in); memcpy(pk + 5, &in, sizeof(in));
        VPIX_Udp_SendTo(seat_sock, seat_peer_host.c_str(), seat_peer_port, pk, sizeof(pk));
        static unsigned long nl = 0; if (nl++ % 1800 == 0) lg("SEAT client f=%lu: %lu frames in (%lu bad pieces), last %.1f s ago", frame, seat_rx, seat_rx_bad, seat_now() - seat_rx_at);
        return;
    }
    if (!seat2_live()) { seat_snd_out.clear(); return; }
    if (!seat_snd_out.empty()) {
        std::vector<uint8_t> sp(5 + seat_snd_out.size() * sizeof(SeatSnd)); memcpy(sp.data(), "SEATS", 5);
        memcpy(sp.data() + 5, seat_snd_out.data(), seat_snd_out.size() * sizeof(SeatSnd));
        VPIX_Udp_SendTo(seat_sock, seat_peer_host.c_str(), seat_peer_port, sp.data(), sp.size());
        seat_snd_out.clear();
    }
    VPIX_Frame f2; frame_capture = &f2; frame_force_win = 1;
    submit_frame();
    frame_capture = NULL; frame_force_win = -1;
    SeatFrameHdr hd = {};
    hd.frame = (uint32_t)f2.frame; memcpy(hd.cam_R, f2.cam_R, sizeof(hd.cam_R)); memcpy(hd.cam_T, f2.cam_T, sizeof(hd.cam_T)); hd.fov_deg = f2.fov_deg;
    memcpy(hd.clear, f2.clear, 3); memcpy(hd.fog_col, f2.fog_col, 3); hd.fog_start = f2.fog_start; hd.fog_end = f2.fog_end;
    hd.aim_beam = f2.aim_beam; hd.in_head = f2.in_head; hd.hold = f2.hold;
    memcpy(hd.aim_o, f2.aim_o, sizeof(hd.aim_o)); memcpy(hd.aim_d, f2.aim_d, sizeof(hd.aim_d));
    hd.head_fed = f2.head_fed; hd.player_height = f2.player_height; hd.crouch_mm = f2.crouch_mm; hd.game_mode = f2.game_mode;
    size_t ni = (std::min)(f2.inst.size(), (size_t)400), nb = (std::min)(f2.blits.size(), (size_t)400);
    hd.ninst = (uint32_t)ni; hd.nblit = (uint32_t)nb;
    std::vector<uint8_t> whole(sizeof(hd) + ni * sizeof(VPIX_Instance) + nb * sizeof(VPIX_Blit));
    memcpy(whole.data(), &hd, sizeof(hd));
    if (ni) memcpy(whole.data() + sizeof(hd), f2.inst.data(), ni * sizeof(VPIX_Instance));
    if (nb) memcpy(whole.data() + sizeof(hd) + ni * sizeof(VPIX_Instance), f2.blits.data(), nb * sizeof(VPIX_Blit));
    static uint32_t fid = 1; fid++;
    SeatPiece pc; pc.id = fid; pc.total = (uint32_t)whole.size(); pc.count = (uint16_t)((whole.size() + SEAT_PIECE - 1) / SEAT_PIECE);
    uint8_t pk[5 + sizeof(SeatPiece) + SEAT_PIECE]; memcpy(pk, "SEATP", 5);
    for (uint16_t i = 0; i < pc.count; i++) {
        size_t at = (size_t)i * SEAT_PIECE, len = (std::min)(SEAT_PIECE, whole.size() - at);
        pc.idx = i; memcpy(pk + 5, &pc, sizeof(pc)); memcpy(pk + 5 + sizeof(pc), whole.data() + at, len);
        VPIX_Udp_SendTo(seat_sock, seat_peer_host.c_str(), seat_peer_port, pk, 5 + sizeof(pc) + len);
    }
    seat_tx++;
    static unsigned long nl = 0; if (nl++ % 1800 == 0) lg("SEAT host f=%lu: seat 2 frame %u models %u bitmaps, %u bytes in %u pieces (%lu sent, %lu inputs in)", frame, hd.ninst, hd.nblit, (unsigned)whole.size(), pc.count, seat_tx, seat_rx);
}

/* seat PC: draw the host's latest frame in place of our own game's */
static void seat_client_replace(VPIX_Frame &f) {
    /* once the host has been heard, its last frame stays up through gaps (our own idle game's attract camera flashed in when a
       frame ran late, Bobby 10-20 'camera went to looking behind the boxer'); after 10 s of silence our own screen comes back */
    if (!seat_is_client || seat_now() - seat_rx_at > 10.0 || seat_rx_frame.size() < sizeof(SeatFrameHdr)) return;
    const uint8_t *p = seat_rx_frame.data(); SeatFrameHdr hd; memcpy(&hd, p, sizeof(hd)); p += sizeof(hd);
    if (seat_rx_frame.size() < sizeof(hd) + hd.ninst * sizeof(VPIX_Instance) + hd.nblit * sizeof(VPIX_Blit)) return;
    VPIX_Frame g; g.frame = f.frame;
    memcpy(g.cam_R, hd.cam_R, sizeof(g.cam_R)); memcpy(g.cam_T, hd.cam_T, sizeof(g.cam_T)); g.fov_deg = hd.fov_deg;
    memcpy(g.clear, hd.clear, 3); memcpy(g.fog_col, hd.fog_col, 3); g.fog_start = hd.fog_start; g.fog_end = hd.fog_end;
    g.aim_beam = hd.aim_beam != 0; g.in_head = hd.in_head != 0; g.hold = hd.hold != 0;
    memcpy(g.aim_o, hd.aim_o, sizeof(g.aim_o)); memcpy(g.aim_d, hd.aim_d, sizeof(g.aim_d));
    g.head_fed = hd.head_fed; g.player_height = hd.player_height; g.crouch_mm = hd.crouch_mm; g.game_mode = hd.game_mode;
    g.inst.resize(hd.ninst); if (hd.ninst) memcpy(g.inst.data(), p, hd.ninst * sizeof(VPIX_Instance)); p += hd.ninst * sizeof(VPIX_Instance);
    g.blits.resize(hd.nblit); if (hd.nblit) memcpy(g.blits.data(), p, hd.nblit * sizeof(VPIX_Blit));
    f = std::move(g);
}
