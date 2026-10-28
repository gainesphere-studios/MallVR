/* Voice chat between linked players (VPIX_VOICE=1, play.ps1 turns it on for linked / online play). Included at the end of
   vpix_hle.cpp after vpix_seat.h.
   Mic: vpix_net.cpp captures the default recording device (the headset's mic when Windows / SteamVR uses it), 16 kHz mono,
   20 ms packets of 320 mu-law bytes (128 kbit/s), only while you talk (loudness gate VPIX_VOICE_GATE, default 500).
   Out: the remote-seat link ("SEAT" 'V' packets, Boxing 2-player) or the pod network (Ethernet type 0x88B6, linked pods);
   online both ride the launcher's relay like the game's own packets.
   In: one small jitter buffer per talker (starts after 60 ms, never more than 250 ms behind), mixed into its own mixer channel
   at VPIX_VOICE_GAIN (default 1.0).
   Per-friend volume (Button Masher Menu > VOICE CHAT, Bobby 10-26: "so people can turn their friends down if too loud or up if not
   loud enough"): VPIX_VOICE_VOL_FILE = a small text file the launcher rewrites when a volume changes, lines "<pod> <percent>" and
   "all <percent>" (0-200; a missing line = 100). Read again once a second. Only changes what this player hears.
   "ptt 1" in the same file = push to talk (hold the mic button to talk), "ptt 0" = mic live, the button mutes (the menu's PUSH TO
   TALK line). Mute itself is shared with the in-game menu (VPIX_Voice_Muted: MIC LIVE / MUTED on the VOICE CHAT page). */

#include <deque>
static MixerChannel *voice_chan = NULL;
static std::mutex voice_mtx;
static std::map<int, std::deque<int16_t>> voice_in;     /* talker -> samples waiting */
static std::map<int, bool> voice_playing;
static unsigned long voice_tx = 0, voice_rx_n = 0;
static std::map<int, float> voice_vol;                   /* talker -> volume (1 = 100%) */
static float voice_vol_all = 1.f;
static int voice_ptt_file = -1;                          /* -1 = the file says nothing: VPIX_VOICE_MODE decides */

static void voice_vol_poll() {
    static const char *path = getenv("VPIX_VOICE_VOL_FILE");
    if (!path || !*path) return;
    static double next = 0; static std::string was;
    double now = seat_now(); if (now < next) return; next = now + 1.0;
    FILE *f = fopen(path, "r"); if (!f) return;
    char txt[512]; size_t n = fread(txt, 1, sizeof(txt) - 1, f); fclose(f); txt[n] = 0;
    if (was == txt) return;
    was = txt;
    std::map<int, float> m; float all = 1.f; int ptt = -1;
    for (char *l = strtok(txt, "\r\n"); l; l = strtok(NULL, "\r\n")) {
        char k[16]; int pct;
        if (sscanf(l, "%15s %d", k, &pct) != 2) continue;
        float v = (float)(std::max)(0, (std::min)(200, pct)) / 100.f;
        if (!strcmp(k, "ptt")) ptt = pct ? 1 : 0;
        else if (!strcmp(k, "all")) all = v; else if (atoi(k) > 0) m[atoi(k)] = v;
    }
    {   std::lock_guard<std::mutex> lk(voice_mtx); voice_vol.swap(m); voice_vol_all = all; }
    voice_ptt_file = ptt;
    lg("VOICE volume: all %d%%, %u talker(s) set", (int)(all * 100.f + 0.5f), (unsigned)voice_vol.size());
}

static int16_t ulaw_dec(uint8_t u) {
    u = ~u;
    int sign = u & 0x80, exp = (u >> 4) & 7, mant = u & 0x0F;
    int s = ((mant << 3) + 0x84) << exp; s -= 0x84;
    return (int16_t)(sign ? -s : s);
}

static void voice_mix(Bitu len) {
    static std::vector<int16_t> buf; buf.assign(len, 0);
    static const float gain = getenv("VPIX_VOICE_GAIN") ? (float)atof(getenv("VPIX_VOICE_GAIN")) : 1.f;
    {   std::lock_guard<std::mutex> lk(voice_mtx);
        for (auto &kv : voice_in) {
            std::deque<int16_t> &q = kv.second; bool &on = voice_playing[kv.first];
            if (!on && q.size() >= 960) on = true;           /* 60 ms banked before a talker starts */
            if (!on) continue;
            while (q.size() > 4000) q.pop_front();            /* > 250 ms behind: catch up */
            auto vv = voice_vol.find(kv.first);
            const float g = gain * voice_vol_all * (vv != voice_vol.end() ? vv->second : 1.f);
            for (Bitu i = 0; i < len && !q.empty(); i++) {
                int v = buf[i] + (int)(q.front() * g); q.pop_front();
                buf[i] = (int16_t)(std::max)(-32768, (std::min)(32767, v));
            }
            if (q.empty()) on = false;                        /* ran dry: bank again */
        }
    }
    voice_chan->AddSamples_m16(len, buf.data());
}

static void voice_rx(int src, const uint8_t *ulaw, size_t n) {
    if (n > 2000) return;
    std::lock_guard<std::mutex> lk(voice_mtx);
    std::deque<int16_t> &q = voice_in[src];
    for (size_t i = 0; i < n; i++) q.push_back(ulaw_dec(ulaw[i]));
    voice_rx_n++;
}
static void voice_rx_net(int src, const uint8_t *ulaw, size_t n) { voice_rx(src, ulaw, n); }   /* pod network (rx thread) */

/* once per game frame: send what the mic recorded */
static void voice_tick() {
    static const bool on = getenv("VPIX_VOICE") && atoi(getenv("VPIX_VOICE"));
    if (!on) return;
    static bool init = false;
    if (!init) {
        init = true;
        VPIX_Voice_Start(); VPIX_Voice_SetSink(voice_rx_net);
        voice_chan = MIXER_AddChannel(voice_mix, 16000, "VPIXVOICE"); voice_chan->Enable(true);
        lg("VOICE on: mic -> %s, gate %s", seat_sock >= 0 ? "remote-seat link" : VPIX_NET_Enabled() ? "pod network" : "nowhere (not linked)",
           getenv("VPIX_VOICE_GATE") ? getenv("VPIX_VOICE_GATE") : "500");
    }
    voice_vol_poll();
    /* mute: Index left stick click / window M, every game; short buzz = muted, long buzz = live again */
    /* VPIX_VOICE_MODE (launcher VOICE setting): open = mic always live, the button toggles mute; ptt = push to talk, hold the button */
    static const bool ptt_env = getenv("VPIX_VOICE_MODE") && !strcmp(getenv("VPIX_VOICE_MODE"), "ptt");
    const bool ptt = voice_ptt_file >= 0 ? voice_ptt_file == 1 : ptt_env;
    static bool was = false, was_ptt = false;
    if (was_ptt && !ptt) VPIX_Voice_SetMuted(0);             /* push to talk switched off: the mic is live again */
    was_ptt = ptt;
    if (ptt) VPIX_Voice_SetMuted(!cur_in.voice_mute);
    else if (cur_in.voice_mute && !was) { VPIX_Voice_SetMuted(!VPIX_Voice_Muted()); VPIX_Haptic(1, VPIX_Voice_Muted() ? 0.05f : 0.3f, 1.f); lg("VOICE %s", VPIX_Voice_Muted() ? "MUTED" : "live"); }
    const bool muted = VPIX_Voice_Muted() != 0;
    if (ptt && cur_in.voice_mute && !was) VPIX_Haptic(1, 0.03f, 0.6f);   /* a tick when you start talking */
    was = cur_in.voice_mute;
    uint8_t u[320];
    static const bool tone = getenv("VPIX_VOICE_TONE") && atoi(getenv("VPIX_VOICE_TONE"));   /* dev: a 440 Hz beep instead of the mic */
    auto pop = [&]() -> bool {
        if (!tone) return VPIX_Voice_PopMic(u) != 0;
        static double t0 = seat_now(), sent = 0; static uint32_t ph = 0;
        if ((seat_now() - t0) * 50.0 <= sent) return false;
        sent += 1; for (int k = 0; k < 320; k++, ph++) u[k] = (uint8_t)~(0x80 * (ph % 36 < 18) | 0x50);   /* mu-law square wave ~444 Hz */
        return true;
    };
    while (pop()) {
        if (muted) continue;
        if (seat_sock >= 0 && !seat_peer_host.empty() && (seat_is_client || seat2_live())) {
            uint8_t pk[5 + 320]; memcpy(pk, "SEATV", 5); memcpy(pk + 5, u, 320);
            VPIX_Udp_SendTo(seat_sock, seat_peer_host.c_str(), seat_peer_port, pk, sizeof(pk));
        } else if (VPIX_NET_Enabled()) VPIX_Voice_SendNet(u, 320);
        voice_tx++;
    }
    static unsigned long nl = 0; if (nl++ % 1800 == 0) lg("VOICE f=%lu: %lu packets sent, %lu heard", frame, voice_tx, voice_rx_n);
}
