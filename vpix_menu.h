/* MallVR in-game menu ("button masher menu", Bobby 10-25): hold A + B together for 1 second (Vive wands: both menu buttons) and the
   launcher's own panel comes up in front of you, over the game (launcher MallVrPanel.cs, a SteamVR overlay: nothing is drawn into the
   game). We only tell the launcher and rest the controls while it's open, so pointing at the panel doesn't shoot.
   VPIX_MENU_PORT=<port> (set by launcher.ps1): the launcher listens on 127.0.0.1:<port>. Without it the combo does nothing.
   Lines over localhost UDP: engine -> launcher "HELLO <game>" (at the first VR frame: the launcher learns where we are, so it can
   rest the controls for an online group's GO / STAY OUT question and close us for a group switch), "MENU <game>", "RESUME";
   launcher -> engine "OPEN" (its panel is up: rest the controls), "RESUME" (close the panel, play on) or "QUIT" (BACK TO MENU /
   group switch: the game closes, like Esc in a window). Included in vpix_render.cpp's anonymous namespace after VPIX_Acts.
   Play notes (launcher MallNotes.cs): every 10 s in the headset a "perf: frames N secs S worst W" line goes to the render log
   (worst = the longest single frame in ms, loading stalls over 0.5 s left out). */
#pragma comment(lib, "ws2_32.lib")

static struct {
    bool on = false, open = false, combo_was = false;
    double held = 0;                                   /* seconds A + B have been held */
    SOCKET s = INVALID_SOCKET; sockaddr_in to = {};
} pmenu;

static struct { int frames = 0; double secs = 0, worst = 0; } pperf;

static void pperf_flush() {
    if (pperf.frames > 0) rl("perf: frames %d secs %.2f worst %.1f", pperf.frames, pperf.secs, pperf.worst * 1000.0);
    pperf.frames = 0; pperf.secs = 0; pperf.worst = 0;
}

static void pmenu_init() {
    static bool tried = false;
    if (tried) return;
    tried = true;
    const char *p = getenv("VPIX_MENU_PORT");
    int port = p ? atoi(p) : 0;
    if (port <= 1024 || port >= 65536) return;
    WSADATA w; WSAStartup(MAKEWORD(2, 2), &w);
    pmenu.s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (pmenu.s == INVALID_SOCKET) return;
    sockaddr_in me = {}; me.sin_family = AF_INET; me.sin_addr.s_addr = htonl(INADDR_LOOPBACK); me.sin_port = 0;
    bind(pmenu.s, (sockaddr *)&me, sizeof(me));
    u_long nb = 1; ioctlsocket(pmenu.s, FIONBIO, &nb);
    pmenu.to.sin_family = AF_INET; pmenu.to.sin_addr.s_addr = htonl(INADDR_LOOPBACK); pmenu.to.sin_port = htons((u_short)port);
    pmenu.on = true;
    rl("menu: A + B held 1 s opens the launcher panel (port %d)", port);
}


static void pmenu_send(const char *line) {
    if (pmenu.on) sendto(pmenu.s, line, (int)strlen(line), 0, (sockaddr *)&pmenu.to, sizeof(pmenu.to));
}

/* every VR frame, right after the controllers are read. Returns true while the menu is open: the caller then gives the game
   "hands at rest, no buttons". While A + B are both down, neither reaches the game (no coin / crouch from the combo). */
static bool pmenu_frame(VPIX_Acts &a, double dt) {
    pperf.frames++; pperf.secs += dt;
    if (dt < 0.5 && dt > pperf.worst) pperf.worst = dt;
    if (pperf.secs >= 10.0) pperf_flush();
    static bool hello = false;
    pmenu_init();
    if (!pmenu.on) return false;
    if (!hello) { hello = true; std::string l = "HELLO " + game_code(); pmenu_send(l.c_str()); }
    char buf[64];
    int n;
    while ((n = recv(pmenu.s, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = 0;
        if (!strncmp(buf, "RESUME", 6)) { if (pmenu.open) rl("menu: resume"); pmenu.open = false; }
        else if (!strncmp(buf, "QUIT", 4)) { rl("menu: back to the launcher"); pperf_flush(); ExitProcess(0); }
        else if (!strncmp(buf, "OPEN", 4)) pmenu.open = true;     /* the launcher's panel is up (also after a lost MENU line) */
        else if (!strncmp(buf, "MIC ", 4)) { VPIX_Voice_SetMuted(atoi(buf + 4)); rl("menu: mic %s", atoi(buf + 4) ? "muted" : "live"); }
    }
    /* tell the launcher the mic state whenever it changes (its VOICE CHAT page shows MIC LIVE / MUTED) */
    static int mic_told = -1;
    if (VPIX_Voice_Muted() != mic_told) { mic_told = VPIX_Voice_Muted(); pmenu_send(mic_told ? "MIC 1" : "MIC 0"); }
    bool combo = a.menu && a.crouch;
    if (combo) {
        /* a small tick every quarter second while held, so a quick press-and-let-go is felt as "not yet" (Bobby 10-25: he pressed
           both and let go, and thought it didn't work) */
        if (!pmenu.combo_was && (int)((pmenu.held + dt) * 4) > (int)(pmenu.held * 4)) VPIX_Haptic(2, 0.02f, 0.4f);
        pmenu.held += dt;
        if (pmenu.held >= 1.0 && !pmenu.combo_was) {
            pmenu.combo_was = true;
            if (!pmenu.open) {
                pmenu.open = true;
                std::string l = "MENU " + game_code();
                pmenu_send(l.c_str());
                VPIX_Haptic(2, 0.12f, 1.f);
                rl("menu: open");
            } else {                                     /* the same hold again closes it (also if the launcher went away) */
                pmenu.open = false;
                pmenu_send("RESUME");
                VPIX_Haptic(2, 0.05f, 0.6f);
                rl("menu: closed by A + B");
            }
        }
        a.menu = a.crouch = false;
    } else { pmenu.held = 0; pmenu.combo_was = false; }
    return pmenu.open;
}
