/* High scores and name entry for the native rebuilds. Included inside namespace d1k, after blit / make_sound.
   Name entry is a letter drum: the stick spins it, and the trigger fires the front letter up into its slot with a burst of sparks. Three letters and you are on the board, your row flashing.
   The board also takes a turn in each game's attract. Saved in %APPDATA%\MallVR\rebuild_scores.txt, one "game score NAME" per line, top 10 per game. A self-playing run picks letters itself and saves nothing.
   A game calls: run() each tick after reading its input (true = the board has the screen, so clear the game's own select press), and draw() last in its frame, before VPIX_Render_Submit. All of this is new, none of it is from footage.
   Env: VPIX_HS_TEST=<score> opens name entry two seconds in, VPIX_HS_FILE=<path> uses another score file. */
namespace hs {
struct Row { int score = 0; char name[4] = {0, 0, 0, 0}; };
enum Mode { HS_OFF, HS_ENTRY, HS_BOARD };
enum { BM_W = 3000, BM_A = 3040, BM_D = 3080, BM_BLACK = 3090, BM_SPARK = 3091 };
enum { SND_TICK = 0x7E0000F0, SND_PICK, SND_DONE };
static const int N_ROWS = 10; static const char SYM[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ-*"; static const int N_SYM = 28;
static Mode mode = HS_OFF; static std::vector<Row> rows; static std::string cur_game; static int my_score = 0, my_row = -1, n_picked = 0; static char picked[4] = {0, 0, 0, 0};
static float wheel = 0, wheel_v = 0, mode_t = 0, fly_t = 1, auto_t = 0, last_tick = 0; static int fly_sym = 0; static bool made = false, armed = false, was_trig = false, used = false, is_auto = false;
struct Spark { float x, y, vx, vy, t; }; static std::vector<Spark> sparks;
static uint32_t glow(uint32_t abgr, float k) { uint32_t r = abgr & 0xFF, g = (abgr >> 8) & 0xFF, b = (abgr >> 16) & 0xFF; return 0xFF000000u | ((uint32_t)(b * k) << 16) | ((uint32_t)(g * k) << 8) | (uint32_t)(r * k); }
static const char *DIG5x7[] = { "01110100011001110101110011000101110", "00100011000010000100001000010001110", "01110100010000100110010001000011111", "11110000010000101110000010000111110", "00010001100101010010111110001000010", "11111100001111000001000011000101110", "00110010001000011110100011000101110", "11111000010001000100010000100001000", "01110100011000101110100011000101110", "01110100011000101111000010001001100" };
static void make() {
    if (made) return; made = true;
    auto glyph = [](const char *bits, uint32_t c, std::vector<uint32_t> &px) { px.assign(FW * FH, 0); for (int y = 0; y < 7; y++) for (int x = 0; x < 5; x++) if (bits[y * 5 + x] == '1') for (int b = 0; b < 2; b++) for (int a = 0; a < 2; a++) px[(y * 2 + b) * FW + x * 2 + a] = c; };
    std::vector<uint32_t> px;
    for (int ch = 0; ch < 26; ch++) { glyph(FONT[ch], 0xFFFFFFFFu, px); add_bitmap(BM_W + ch, FW, FH, px); glyph(FONT[ch], 0xFF20D0F8u, px); add_bitmap(BM_A + ch, FW, FH, px); }
    static const char *DASH = "00000000000000011111000000000000000", *STAR = "00000101010111011111011101010100000";
    glyph(DASH, 0xFFFFFFFFu, px); add_bitmap(BM_W + 26, FW, FH, px); glyph(DASH, 0xFF20D0F8u, px); add_bitmap(BM_A + 26, FW, FH, px);
    glyph(STAR, 0xFFFFFFFFu, px); add_bitmap(BM_W + 27, FW, FH, px); glyph(STAR, 0xFF20D0F8u, px); add_bitmap(BM_A + 27, FW, FH, px);
    for (int d = 0; d < 10; d++) { glyph(DIG5x7[d], 0xFFFFFFFFu, px); add_bitmap(BM_D + d, FW, FH, px); }
    add_bitmap(BM_BLACK, 64, 64, std::vector<uint32_t>(64 * 64, 0xFF080408u)); add_bitmap(BM_SPARK, 2, 2, std::vector<uint32_t>(4, 0xFF30C8FFu));
    make_sound(SND_TICK, 0.03f, [](float t) { return sinf(6.2831853f * 1800.f * t) * 0.3f; });
    make_sound(SND_PICK, 0.22f, [](float t) { return sinf(6.2831853f * (500.f + 2200.f * t) * t) * exp(-t * 9.f) * 0.7f; });
    make_sound(SND_DONE, 0.8f, [](float t) { static const float N[4] = {523.f, 659.f, 784.f, 1047.f}; int i = (std::min)((int)(t / 0.14f), 3); return sinf(6.2831853f * N[i] * t) * exp(-fmodf(t, 0.14f) * 6.f) * 0.5f; });
}
static std::string file_path() { const char *e = getenv("VPIX_HS_FILE"); if (e && *e) return e; const char *a = getenv("APPDATA"); return std::string(a ? a : ".") + "\\MallVR\\rebuild_scores.txt"; }
static void load(const char *game) {   /* this game's rows, best first */
    rows.clear(); FILE *fp = fopen(file_path().c_str(), "r"); if (!fp) return; char g[32], n[8]; int sc;
    while (fscanf(fp, "%31s %d %7s", g, &sc, n) == 3) if (!strcmp(g, game)) { Row r; r.score = sc; strncpy(r.name, n, 3); rows.push_back(r); }
    fclose(fp); std::stable_sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) { return a.score > b.score; }); if ((int)rows.size() > N_ROWS) rows.resize(N_ROWS);
}
static void save(const char *game) {   /* every other game's lines kept, this game's rewritten */
    std::string path = file_path(), out; FILE *fp = fopen(path.c_str(), "r");
    if (fp) { char line[128], g[32]; while (fgets(line, sizeof line, fp)) if (sscanf(line, "%31s", g) == 1 && strcmp(g, game)) out += line; fclose(fp); }
    #ifdef _WIN32
    { size_t cut = path.find_last_of("\\/"); if (cut != std::string::npos) _mkdir(path.substr(0, cut).c_str()); }
#endif
    fp = fopen(path.c_str(), "w"); if (!fp) return; fputs(out.c_str(), fp); for (const Row &r : rows) fprintf(fp, "%s %d %s\n", game, r.score, r.name); fclose(fp);
}
static void finish_entry() {
    Row r; r.score = my_score; for (int i = 0; i < 3; i++) r.name[i] = picked[i] ? picked[i] : '-';
    my_row = 0; while (my_row < (int)rows.size() && rows[my_row].score >= my_score) my_row++;
    rows.insert(rows.begin() + my_row, r); if ((int)rows.size() > N_ROWS) rows.resize(N_ROWS);
    if (!is_auto) save(cur_game.c_str());
    mode = HS_BOARD; mode_t = 0; buggy::play(6, SND_DONE, 110, 0); dl("hs: %s %d on the board, row %d", r.name, my_score, my_row + 1);
}
/* game = its short name; over = the round has ended; autoplay = a self-playing run. True while the board has the screen. */
static bool run(const char *game, bool over, int score, const VPIX_Input &in, bool trig, float el, bool autoplay) {
    make();
    static const char *test = getenv("VPIX_HS_TEST"); static float test_t = 0;
    if (test && *test && !used) { test_t += el; if (test_t > 2.f) { over = true; score = atoi(test); } else over = false; }
    if (!over) { if (mode == HS_OFF) used = false; }
    else if (!used && mode == HS_OFF) { used = true; cur_game = game; load(game); my_score = score; my_row = -1; is_auto = autoplay;
        bool makes = score > 0 && ((int)rows.size() < N_ROWS || score > rows.back().score);
        if (!makes) return false;
        mode = HS_ENTRY; mode_t = 0; n_picked = 0; picked[0] = picked[1] = picked[2] = 0; wheel = 0; wheel_v = 0; armed = false; was_trig = trig; auto_t = 0; dl("hs: name entry for %d", score); }
    if (mode == HS_OFF) return false;
    mode_t += el;
    if (mode == HS_ENTRY) {
        float x = in.joy[0]; if (fabsf(x) < 0.15f && fabsf(in.loco[0]) > 1.f) x = in.loco[0] > 0 ? 0.7f : -0.7f;
        if (fabsf(x) < 0.15f) { float near_s = roundf(wheel); wheel_v += (near_s - wheel) * 60.f * el; wheel_v *= powf(0.002f, el); }   /* it settles on a letter like a real drum */
        else wheel_v = x * 14.f;
        float was = wheel; wheel = fmodf(wheel + wheel_v * el + N_SYM * 100.f, (float)N_SYM);
        if ((int)floorf(was + 0.5f) != (int)floorf(wheel + 0.5f) && mode_t - last_tick > 0.03f) { buggy::play(5, SND_TICK, 50, 0); last_tick = mode_t; }
        if (!trig) armed = true;
        bool fire = armed && trig && !was_trig;
        if (is_auto) { auto_t += el; wheel = (float)(n_picked * 9 + 1); wheel_v = 0; fire = auto_t > 0.9f; if (fire) auto_t = 0; }
        if (fire && fly_t >= 1.f) { fly_sym = ((int)roundf(wheel)) % N_SYM; fly_t = 0; buggy::play(4, SND_PICK, 110, 0); VPIX_Haptic(2, 0.08f, 0.6f); }
        if (fly_t < 1.f) { fly_t += el / 0.22f; if (fly_t >= 1.f) { fly_t = 1.f; float sx = 176.f + (n_picked - 1) * 52.f; for (int i = 0; i < 26; i++) { float a = i * 2.399f, s = 30.f + (i % 7) * 26.f; Spark p; p.x = sx; p.y = 96.f; p.vx = cosf(a) * s; p.vy = sinf(a) * s; p.t = 0; sparks.push_back(p); } picked[n_picked++] = SYM[fly_sym]; if (n_picked >= 3) finish_entry(); } }
        if (mode == HS_ENTRY && mode_t > 30.f) finish_entry();
    } else if (mode == HS_BOARD) { bool press = trig && !was_trig; if (mode_t > 8.f || (press && mode_t > 1.5f)) mode = HS_OFF; }
    for (size_t i = 0; i < sparks.size();) { Spark &p = sparks[i]; p.t += el; p.x += p.vx * el; p.y += p.vy * el; p.vy += 120.f * el; if (p.t > 0.7f) { sparks[i] = sparks.back(); sparks.pop_back(); } else i++; }
    was_trig = trig;
    return true;
}
static float wrapf_sym(float d) { while (d > N_SYM * 0.5f) d -= N_SYM; while (d < -N_SYM * 0.5f) d += N_SYM; return d; }
static void glyph_at(VPIX_Frame &f, char c, float cx, float cy, float sc, bool amber) {
    float x = cx - FW * sc * 0.5f, y = cy - FH * sc * 0.5f;
    if (c >= '0' && c <= '9') blit(f, BM_D + (c - '0'), FW, FH, x, y, sc);
    else if (c >= 'A' && c <= 'Z') blit(f, (amber ? BM_A : BM_W) + (c - 'A'), FW, FH, x, y, sc);
    else if (c == '-') blit(f, (amber ? BM_A : BM_W) + 26, FW, FH, x, y, sc);
    else if (c == '*') blit(f, (amber ? BM_A : BM_W) + 27, FW, FH, x, y, sc);
}
static void line(VPIX_Frame &f, const char *s, float cx, float cy, float sc, bool amber) { float step = 12.f * sc, x = cx - (strlen(s) - 1) * step * 0.5f; for (const char *c = s; *c; c++, x += step) glyph_at(f, *c, x, cy, sc, amber); }
static void black(VPIX_Frame &f) { blit(f, BM_BLACK, 64, 64, -16.f, -16.f, 6.f); }
static void draw_board(VPIX_Frame &f, const char *title, float t) {
    black(f); line(f, title, 176, 28, 1.3f, true);
    for (int i = 0; i < N_ROWS; i++) { float y = 58.f + i * 21.f; char b[32]; bool mine = i == my_row && mode == HS_BOARD; if (mine && fmodf(t, 0.4f) < 0.14f) continue;
        if (i < (int)rows.size()) snprintf(b, sizeof b, "%2d %-3s %7d", i + 1, rows[i].name, rows[i].score); else snprintf(b, sizeof b, "%2d --- %7s", i + 1, "-");
        line(f, b, 176, y, 1.05f, mine); }
}
/* Last thing in a game's frame. title = the game is on its attract, t = any running clock: the board takes a turn there. */
static void draw(VPIX_Frame &f, const char *game, bool title, float t) {
    make();
    if (mode == HS_ENTRY) {
        black(f); line(f, "NEW HIGH SCORE", 176, 28, 1.3f, true); char b[16]; snprintf(b, sizeof b, "%d", my_score); line(f, b, 176, 56, 1.1f, false);
        for (int i = 0; i < 3; i++) { float sx = 176.f + (i - 1) * 52.f; bool next = i == n_picked; line(f, "-", sx, 118.f, 2.f, next && fmodf(mode_t, 0.5f) < 0.3f); if (picked[i]) glyph_at(f, picked[i], sx, 96.f, 3.2f, true); }
        const float R = 150.f; for (int i = 0; i < N_SYM; i++) { float a = wrapf_sym(i - wheel) * 6.2831853f / N_SYM * 1.6f; if (fabsf(a) > 1.5f) continue; float c = cosf(a), sc = 0.8f + 3.4f * c * c * c * c; glyph_at(f, SYM[i], 176.f + sinf(a) * R, 222.f - 34.f * (1.f - c), sc, fabsf(a) < 0.14f); }
        line(f, "STICK SPINS  TRIGGER FIRES", 176, 268, 0.62f, false);
        if (fly_t < 1.f) { float k = fly_t * fly_t, tx = 176.f + (n_picked - 1) * 52.f; glyph_at(f, SYM[fly_sym], 176.f + (tx - 176.f) * k, 222.f + (96.f - 222.f) * k, 4.2f - 1.f * k, true); }
    } else if (mode == HS_BOARD) { draw_board(f, "HIGH SCORES", mode_t); }
    else if (title && fmodf(t, 30.f) > 23.f) { static std::string shown; static float reload = -1; if (shown != game || fabsf(t - reload) > 29.f) { shown = game; reload = t; cur_game = game; load(game); my_row = -1; } draw_board(f, "HIGH SCORES", t); }
    for (const Spark &p : sparks) { float s = 1.f - p.t / 0.7f; blit(f, BM_SPARK, 2, 2, p.x, p.y, 0.5f + 1.2f * s); }
}
}   /* namespace hs */
