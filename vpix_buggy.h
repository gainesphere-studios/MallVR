#include <random>
/* Buggy Ball (Virtuality SD2000, 1995) - native rebuild. BALL.EXE survives nowhere, so this is a new game loop that uses the
   original assets (MODELS, TEXTURES, SOUNDS) and the game's own plain-text rules (RUNFILES\*.DBS), drawing through the MallVR
   renderer like the HLE games. Included at the end of vpix_hle.cpp (shares its mixer); switched on by VPIX_NATIVE=buggy.
   Rules from the SD2000 Duo flyer: push the ball OUT of the arena; each time you score the ball resets in the middle.
   Units: game units (~1.85 per mm, like the other SU2000 games), seconds. World X right, Y forward, Z up.
   Matrices: rows = local axes in world (right, forward, up); world = sum v[k] * R[k] + T. The bowl is a half ellipsoid
   (easybowl.bin vertices: (r/a)^2 + (z/c)^2 = 1, rim at z = 0, floor at z = -c).
   First slice (2026-10-18): level 0 bowl, your car (croc's buggy, CARS 3), the ball, computer cars driving at the ball,
   goals + ball reset, engine / clonk / goal sounds, rumble. No menus, HUD or timer yet. */
namespace buggy {

struct V { float x = 0, y = 0, z = 0; V() {} V(float a, float b, float c) : x(a), y(b), z(c) {} };
static inline V operator+(V a, V b) { return V(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline V operator-(V a, V b) { return V(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline V operator*(V a, float s) { return V(a.x * s, a.y * s, a.z * s); }
static inline V operator-(V a) { return V(-a.x, -a.y, -a.z); }
static inline V &operator+=(V &a, V b) { a = a + b; return a; }
static inline V &operator-=(V &a, V b) { a = a - b; return a; }
static inline float dot(V a, V b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V cross(V a, V b) { return V(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
static inline float len(V a) { return sqrtf(dot(a, a)); }
static inline V norm(V a) { float l = len(a); return l > 1e-9f ? a * (1.f / l) : V(0, 0, 1); }
static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
struct M { V r[3]; M() { r[0] = V(1, 0, 0); r[1] = V(0, 1, 0); r[2] = V(0, 0, 1); } };
static inline V to_world(const M &m, V l) { return m.r[0] * l.x + m.r[1] * l.y + m.r[2] * l.z; }
static inline V to_local(const M &m, V w) { return V(dot(m.r[0], w), dot(m.r[1], w), dot(m.r[2], w)); }
static void ortho(M &m) {   /* keep forward, rebuild right and up */
    m.r[1] = norm(m.r[1]); m.r[0] = norm(cross(m.r[1], m.r[2])); m.r[2] = cross(m.r[0], m.r[1]);
}
static void rotate(M &m, V w, float dt) { for (int k = 0; k < 3; k++) m.r[k] += cross(w, m.r[k]) * dt; ortho(m); }

static FILE *blog = NULL;
static void bl(const char *fmt, ...) {
    if (!blog) return;
    va_list ap; va_start(ap, fmt); vfprintf(blog, fmt, ap); va_end(ap); fputc('\n', blog); fflush(blog);
}
static std::string dir;   /* the game folder (BALL) on the host */

static bool read_host(const std::string &rel, std::vector<uint8_t> &out) {
    std::string p = dir + "\\" + rel;
    FILE *f = fopen(p.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    out.resize(n > 0 ? (size_t)n : 0);
    size_t got = n > 0 ? fread(out.data(), 1, (size_t)n, f) : 0;
    fclose(f);
    return got == out.size();
}
/* DBS / TSC text: lines of whitespace-separated tokens, ';' starts a comment */
static std::vector<std::vector<std::string>> read_dbs(const std::string &rel) {
    std::vector<std::vector<std::string>> L;
    std::vector<uint8_t> d;
    if (!read_host(rel, d)) { bl("missing %s", rel.c_str()); return L; }
    std::string s(d.begin(), d.end()), line;
    size_t p = 0;
    while (p <= s.size()) {
        size_t q = s.find('\n', p); if (q == std::string::npos) q = s.size();
        line = s.substr(p, q - p); p = q + 1;
        size_t c = line.find(';'); if (c != std::string::npos) line.resize(c);
        std::vector<std::string> t; std::string w;
        for (char ch : line) { if (ch == ' ' || ch == '\t' || ch == '\r') { if (!w.empty()) t.push_back(w), w.clear(); } else w += ch; }
        if (!w.empty()) t.push_back(w);
        if (!t.empty()) L.push_back(t);
    }
    return L;
}
static float fa(const std::vector<std::string> &t, size_t i, float def = 0) { return i < t.size() ? (float)atof(t[i].c_str()) : def; }
static std::string up(std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::toupper); return s; }

/* ---- rules ---- */
struct Part { int model[3] = {0, 0, 0}; uint32_t pen[16] = {0}; bool penset[16] = {false}; V off; float radius = 0, mu = 0; };
struct CarDef {
    std::string name; std::vector<Part> parts;
    float steermax = 45, accel = 1, top = 1, boost_a = 1.5f, boost_t = 1.4f, recharge = 15, mass = 1000;
    V coll_lo = V(-1500, -2000, -500), coll_hi = V(1500, 2000, 1200), moi = V(1.5e10f, 7.5e9f, 5e9f), eye;
    std::vector<V> cage; bool fwd4 = false; int engine_snd = 12;
};
struct Level {
    std::vector<int> cars; float speed_f = 1, accel_f = 1, cripple = 1;
    float a = 199000, c = 39750; int bowl_model = 243; uint32_t rim1 = 0xC8C800, rim2 = 0, middle = 0xB4B4C8; int bowl_tex = 1;
    float ball_clip = 5000;
    float ball_r = 4200, ball_bounce = 1.2f, ball_mu = 0.5f, ball_mass = 400, ball_air = 0.006f; int ball_model = 245;
    uint32_t ballpen[8] = {0xC80000, 0xFFFF00}; uint8_t sky[3] = {32, 32, 150};
};
static std::map<int, std::string> model_file;
static std::map<int, CarDef> cardefs;
static Level lv;
static std::map<int, std::string> sound_file; static std::map<int, bool> sound_loop;
static uint32_t rgb(const std::vector<std::string> &t, size_t i) {
    return ((uint32_t)fa(t, i) & 255) << 16 | ((uint32_t)fa(t, i + 1) & 255) << 8 | ((uint32_t)fa(t, i + 2) & 255);
}

static void load_rules(int level) {
    for (auto &t : read_dbs("RUNFILES\\MODELS.DBS")) if (up(t[0]) == "MODEL" && t.size() >= 3) model_file[atoi(t[1].c_str())] = t[2];
    for (auto &t : read_dbs("RUNFILES\\SOUND.DBS")) if (up(t[0]) == "SOUND" && t.size() >= 3) {
        sound_file[atoi(t[1].c_str())] = t[2]; sound_loop[atoi(t[1].c_str())] = t.size() > 3 && up(t[3]) == "Y"; }
    CarDef *c = NULL; Part *pt = NULL; int cage_rows = 0, coll_row = -1, moi_row = -1; bool in_ccp = false;
    for (auto &t : read_dbs("RUNFILES\\CARS.DBS")) {
        std::string k = up(t[0]);
        if (k == "CAR") { c = &cardefs[atoi(t[1].c_str())]; c->name = t.size() > 2 ? t[2] : ""; pt = NULL; cage_rows = 0; coll_row = moi_row = -1; in_ccp = false; continue; }
        if (!c) continue;
        if (k == "CCP") { in_ccp = true; continue; }
        if (k == "ENDCCP") { in_ccp = false; continue; }
        if (in_ccp) continue;
        bool num = (isdigit((unsigned char)t[0][0]) || t[0][0] == '-' || t[0][0] == '.');
        if (num && cage_rows > 0) { c->cage.push_back(V(fa(t, 0), fa(t, 1), fa(t, 2))); cage_rows--; continue; }
        if (num && coll_row >= 0 && coll_row < 3) { float lo = fa(t, 0), hi = fa(t, 1);
            (&c->coll_lo.x)[coll_row] = lo; (&c->coll_hi.x)[coll_row] = hi; coll_row++; continue; }
        if (num && moi_row >= 0 && moi_row < 3) { (&c->moi.x)[moi_row] = fa(t, 0); moi_row++; continue; }
        if (num) continue;
        coll_row = moi_row = -1;
        if (k == "PART") { c->parts.emplace_back(); pt = &c->parts.back(); continue; }
        if (k == "MODEL" && pt) { for (int i = 0; i < 3; i++) pt->model[i] = (int)fa(t, 1 + i, fa(t, 1)); continue; }
        if (k == "LOCALPEN" && pt) { int e = (int)fa(t, 1) & 15; pt->pen[e] = rgb(t, 2); pt->penset[e] = true; continue; }
        if (k == "OFFSET" && pt) { pt->off = V(fa(t, 1), fa(t, 2), fa(t, 3)); continue; }
        if (k == "RADIUS" && pt) { pt->radius = fa(t, 1); continue; }
        if (k == "MU" && pt) { pt->mu = fa(t, 1); continue; }
        if (k == "ROLLCAGE") { c->cage.push_back(V(fa(t, 1), fa(t, 2), fa(t, 3))); cage_rows = 3; continue; }
        if (k == "COLL") { c->coll_lo.x = fa(t, 1); c->coll_hi.x = fa(t, 2); coll_row = 1; continue; }
        if (k == "MOI") { c->moi.x = fa(t, 1); moi_row = 1; continue; }
        if (k == "STEERMAX") c->steermax = fa(t, 1);
        else if (k == "ACCELERATION") c->accel = fa(t, 1);
        else if (k == "TOP_SPEED") c->top = fa(t, 1);
        else if (k == "BOOST") { c->boost_a = fa(t, 1); c->boost_t = fa(t, 2); }
        else if (k == "BOOSTRECHARGE") c->recharge = fa(t, 1);
        else if (k == "MASS") c->mass = fa(t, 1);
        else if (k == "FOURWHEELDRIVE") c->fwd4 = true;
        else if (k == "EYEOFFSET") c->eye = V(fa(t, 1), fa(t, 2), fa(t, 3));
        else if (k == "ENGINESOUND") c->engine_snd = (int)fa(t, 1);
    }
    bool in = false; int pens = 0;
    for (auto &t : read_dbs("RUNFILES\\LEVELS.DBS")) {
        std::string k = up(t[0]);
        if (k == "LEVEL") { in = atoi(t[1].c_str()) == level; continue; }
        if (!in) continue;
        if (k == "ENDLEVEL") break;
        if (k == "CARS") { lv.cars.clear(); for (size_t i = 1; i < t.size(); i++) lv.cars.push_back(atoi(t[i].c_str())); }
        else if (k == "SPEED_FACTOR") lv.speed_f = fa(t, 1);
        else if (k == "ACCEL_FACTOR") lv.accel_f = fa(t, 1);
        else if (k == "COMP_CRIPPLE") lv.cripple = fa(t, 1);
        else if (k == "BOWLRADIUS") lv.a = fa(t, 1);
        else if (k == "BOWLDEPTH") lv.c = fa(t, 1);
        else if (k == "BOWLMODEL") lv.bowl_model = (int)fa(t, 1);
        else if (k == "BOWLRIM1") lv.rim1 = rgb(t, 1);
        else if (k == "BOWLRIM2") lv.rim2 = rgb(t, 1);
        else if (k == "BOWLMIDDLE") lv.middle = rgb(t, 1);
        else if (k == "BOWLTEXTURE") lv.bowl_tex = (int)fa(t, 1);
        else if (k == "BALLRADIUS") lv.ball_r = fa(t, 1);
        else if (k == "BALLPEN") { int e = (int)fa(t, 1) & 7; lv.ballpen[e] = rgb(t, 2); pens++; }
        else if (k == "BALLMODELS") lv.ball_model = (int)fa(t, 1);
        else if (k == "BALLBOUNCE") lv.ball_bounce = fa(t, 1);
        else if (k == "BALLMU") lv.ball_mu = fa(t, 1);
        else if (k == "BALLMASS") lv.ball_mass = fa(t, 1);
        else if (k == "BALLAIRRESIST") lv.ball_air = fa(t, 1);
        else if (k == "BALLCLIPHEIGHT") lv.ball_clip = fa(t, 1);
        else if (k == "SKYCOLOURS") { lv.sky[0] = (uint8_t)fa(t, 1); lv.sky[1] = (uint8_t)fa(t, 2); lv.sky[2] = (uint8_t)fa(t, 3); }
    }
    /* the model files are a little bigger than BOWLRADIUS / BOWLDEPTH: collide with the drawn surface */
    bl("level %d: bowl model %d a=%.0f c=%.0f, ball r=%.0f, cars %zu, %zu car types", level, lv.bowl_model, lv.a, lv.c, lv.ball_r, lv.cars.size(), cardefs.size());
}

/* ---- assets -> renderer ---- */
static const uint32_t MH = 0x100, TH = 0x4000;   /* our model / texture handle bases */
static std::set<int> models_ok;
static bool use_model(int id) {
    if (models_ok.count(id)) return true;
    auto it = model_file.find(id);
    std::vector<uint8_t> d;
    if (it == model_file.end() || !read_host(it->second, d)) { bl("model %d not found", id); return false; }
    if (!VPIX_Render_AddModel(MH + id, d.data(), d.size())) { bl("model %d (%s) did not parse", id, it->second.c_str()); return false; }
    VPIX_Render_ModelName(MH + id, it->second.c_str());
    models_ok.insert(id);
    return true;
}
static float bowl_a = 200000, bowl_c = 40000;    /* drawn bowl surface (from the model's vertices) */
static void bowl_shape(int id) {
    std::vector<uint8_t> d;
    auto it = model_file.find(id);
    if (it == model_file.end() || !read_host(it->second, d) || d.size() < 64) return;
    /* first sub-model's vertex list: "BIN 0200", u32 12, [len][type] block, sub header offsets (big-endian) */
    auto be = [&](size_t o) { return o + 4 <= d.size() ? (uint32_t)d[o] << 24 | d[o + 1] << 16 | d[o + 2] << 8 | d[o + 3] : 0u; };
    size_t blob = 20; uint32_t ty = be(16);
    size_t sub = blob + ((ty >> 16) == 1 ? be(blob) : 0);
    uint32_t ov = be(sub), nv = be(sub + ov - 4);
    float rmax = 0, zmin = 0;
    for (uint32_t i = 0; i < nv && i < 100000; i++) {
        uint32_t w[3]; float f[3];
        for (int k = 0; k < 3; k++) { w[k] = be(sub + ov + 12 * i + 4 * k); memcpy(&f[k], &w[k], 4); }
        rmax = (std::max)(rmax, sqrtf(f[0] * f[0] + f[1] * f[1])); zmin = (std::min)(zmin, f[2]);
    }
    if (rmax > 1000 && zmin < -1000) { bowl_a = rmax; bowl_c = -zmin; }
    bl("bowl surface from %s: a=%.0f c=%.0f (%u vertices)", it->second.c_str(), bowl_a, bowl_c, nv);
}
static void load_textures() {
    std::map<std::string, uint32_t> loaded; uint32_t next = TH;
    for (auto &t : read_dbs("TEXTURES\\BUG.TSC")) {
        if (up(t[0]) != "TEXTURE" || t.size() < 10) continue;
        uint32_t n = (uint32_t)atoi(t[1].c_str()); std::string file = up(t[2]);
        if (!loaded.count(file)) {
            std::vector<uint8_t> d;
            if (!read_host(file, d) || !VPIX_Render_AddTexture(next, d.data(), d.size())) { bl("texture %s failed", file.c_str()); continue; }
            loaded[file] = next++;
        }
        float w = fa(t, 3), h = fa(t, 4), A = fa(t, 5), B = fa(t, 6), o[3] = {fa(t, 7), fa(t, 8), fa(t, 9)};
        std::string pl = t.size() > 10 ? up(t[10]) : "XY";
        uint32_t fl = pl.rfind("XZ", 0) == 0 ? 0x40000 : pl.rfind("YZ", 0) == 0 ? 0x80000 : 0x20000;
        /* TSC: texture size in texels, then the world span of one repeat (units, negative = mirrored), origin, plane */
        VPIX_Render_SetTexture(n, fl, loaded[file], o, A != 0 ? w / A : 1.f, B != 0 ? h / B : 1.f, 0, 0);
    }
    bl("%zu texture files", loaded.size());
}
static std::map<int, uint32_t> snd_key;
static int sound(int id) {   /* -> mixer key, loaded on first use */
    auto k = snd_key.find(id);
    if (k != snd_key.end()) return (int)k->second;
    auto it = sound_file.find(id);
    std::vector<uint8_t> d; VSound s;
    if (it == sound_file.end() || !read_host(it->second, d) || !parse_wav(d, s)) { bl("sound %d missing", id); snd_key[id] = 0; return 0; }
    s.loop = sound_loop[id];
    uint32_t key = 0x60000u | (uint32_t)id;
    sounds[key] = std::move(s); sound_names[key] = it->second; snd_key[id] = key;
    return (int)key;
}
static int sound_file_key(const char *rel, uint32_t id) {   /* a WAV not in SOUND.DBS */
    uint32_t key = 0x61000u | id;
    if (sounds.count(key)) return (int)key;
    std::vector<uint8_t> d; VSound s;
    if (!read_host(rel, d) || !parse_wav(d, s)) { bl("sound %s missing", rel); return 0; }
    sounds[key] = std::move(s); sound_names[key] = rel;
    return (int)key;
}
static void play(int ch, int key, int vol, int pan = 0, float semis = 0) {
    if (!key || ch < 0 || ch >= 32) return;
    auto it = sounds.find((uint32_t)key);
    if (it == sounds.end()) return;
    VVoice &v = voices[ch];
    v.s = &it->second; v.sound = (uint32_t)key; v.pos = 0; v.on = true; v.loop = it->second.loop;
    v.vol = (std::max)(0, (std::min)(127, vol)); v.pan = (std::max)(-64, (std::min)(63, pan)); v.semis = 0; v.bend = (int)(semis * 32.f);
}

/* ---- the bowl ---- */
/* signed distance from the bowl surface: > 0 = inside the ground (outside the ellipsoid), < 0 = inside the bowl;
   n = surface normal pointing into the bowl (up out of the ground) */
/* the drawn bowl is 32 flat panels round (a vertex every 11.25 deg from 1.5 deg): mid-panel it lies cos(5.625 deg) inside the
   smooth ellipse (~960 units at the rim) and cars sank into it near the lip (Bobby 10-18). Collide with the panels: scale the
   point's horizontal distance by the panel's factor at its azimuth. */
static float panel_scale(float x, float y) {
    const float step = 0.19634954f, ph0 = 0.02617994f;
    float a = atan2f(y, x) - ph0;
    float d = fmodf(a, step); if (d < 0) d += step;
    return cosf(step * 0.5f) / cosf(d - step * 0.5f);
}
static float bowl_dist(V p, V &n) {
    { float k = 1.f / panel_scale(p.x, p.y); p.x *= k; p.y *= k; }
    float a2 = bowl_a * bowl_a, c2 = bowl_c * bowl_c;
    float f = (p.x * p.x + p.y * p.y) / a2 + p.z * p.z / c2;
    V g(2 * p.x / a2, 2 * p.y / a2, 2 * p.z / c2);
    float gl = len(g);
    if (gl < 1e-12f) { n = V(0, 0, 1); return -bowl_c; }
    n = -(g * (1.f / gl));
    return (f - 1.f) / gl;
}
/* ray (origin o, unit direction d) to the bowl floor: distance t, or -1 when it misses (above the rim) */
static float bowl_ray(V o, V d) {
    float ia = 1.f / (bowl_a * panel_scale(o.x, o.y)), ic = 1.f / bowl_c;
    V qo(o.x * ia, o.y * ia, o.z * ic), qd(d.x * ia, d.y * ia, d.z * ic);
    float A = dot(qd, qd), B = 2 * dot(qo, qd), C = dot(qo, qo) - 1;
    float disc = B * B - 4 * A * C;
    if (disc < 0) return -1;
    float s = sqrtf(disc), t;
    if (C < 0) t = (-B + s) / (2 * A);          /* inside: the far wall along the ray */
    else { if (o.z < 0) return 0; t = (-B - s) / (2 * A); if (t < 0) return -1; }   /* below ground: buried */
    V h = o + d * t;
    return h.z > 0 ? -1 : t;
}

/* ---- simulation ---- */
static float G = 18150.f;
/* engine: top speed / acceleration per unit of the CARS.DBS TOP_SPEED / ACCELERATION; grip; speed downforce so cars ride the
   bowl walls like the promo video (Bobby 10-18: "way underpowered up the bowl"; the 4x4 topped out ~10000 units/s) */
/* top speed = (v_base + v_per_top * TOP_SPEED) * SPEED_FACTOR: STATES.DBS (snapshots of real games) has the cars at 37500-45600
   units/s, the croc buggy (TOP 1.4) fastest, the 4x4 (0.6) 37600 */
static float v_per_top = 20000.f;
static float v_base = 34000.f, a_base = 52000.f, tyre_mu = 2.2f, downforce = 2.0e-5f;   /* Bobby 10-18: faster, turn better, more power up the embankments */
static float spin_k = 2.5f;   /* how hard hits spin the cars round (the footage: more than plain physics) */
static float ball_g = 0.5f, ball_floor_e = 0.6f, ball_lift = 0.7f;   /* VPIX_BUGGY_BALL=g,floor bounce,lift */   /* 9.81 m/s^2 at 1.85 units / mm */
struct Wheel { V off; float r = 600, s = 0, s_prev = 0, spin = 0, spin_v = 0; bool contact = false, front = false, driven = true; };
struct Car {
    int type = 3; const CarDef *d = NULL; V p, v, w; M R;
    float steer = 0, throttle = 0, boost_left = 0, boost_charge = 1; bool boost_req = false;
    Wheel wh[4]; bool cpu = true; int score = 0; float flip_t = 0, out_t = 0;
    int ch_engine = -1; float hit_cool = 0, stuck_t = 0, back_t = 0; int turn_dir = 0;
    int pod = 0;   /* link play: the pod driving it (0 = this PC's own / computer car), vpix_buggy_net.h */
};
struct Ball { V p, v, w; M R; int last = -1; float out_t = 0, wait = 0; bool scored = false; };
static std::vector<Car> cars;
/* effects from the arcade footage (Bobby 10-18, youtube MYva58-PUS0): explosions + smoke on hits, dust off the tyres */
struct Puff { V p, v; float t = 0, life = 1, size = 2000, grow = 1.5f; int bmp = 204; bool anim = false; };
static std::vector<Puff> puffs;
static void puff(V p, V v, float life, float size, int bmp, bool anim = false, float grow = 1.5f) {
    if (puffs.size() > 160) puffs.erase(puffs.begin());
    Puff q; q.p = p; q.v = v; q.life = life; q.size = size; q.bmp = bmp; q.anim = anim; q.grow = grow; puffs.push_back(q);
}
static void explosion(V p, float strength) {
    puff(p, V(0, 0, 1500), 0.45f, 3500.f + strength * 0.08f, 100, true, 1.2f);
    for (int k = 0; k < 4; k++) {
        float a = k * 1.5708f + (float)(rand() % 100) * 0.01f;
        puff(p + V(cosf(a) * 800, sinf(a) * 800, 400), V(cosf(a) * 2500, sinf(a) * 2500, 3000), 1.4f, 2600.f, 204, false, 2.2f);
    }
}
static float goal_banner = 0; static int goal_by = -1;
static Ball ball;
static const float TRAVEL = 400.f;   /* suspension travel each way from the rest position (units) */
static int me = 0;
static double sim_t = 0;
static bool autopilot = false;

static void place_car(Car &c, int slot) {
    float ang = 3.14159265f * 0.25f + slot * 3.14159265f * 0.5f;          /* round the arena, facing the middle */
    float r = bowl_a * 0.55f;
    V p(cosf(ang) * r, sinf(ang) * r, 0);
    float t = bowl_ray(V(p.x, p.y, -10), V(0, 0, -1));
    V floor(p.x, p.y, -10 - (t > 0 ? t : bowl_c * 0.5f)), n; bowl_dist(floor + V(0, 0, 50), n);
    p = floor + n * 1500.f;
    c.p = p; c.v = V(0, 0, 0); c.w = V(0, 0, 0);
    V fwd = norm(V(-p.x, -p.y, 0));
    c.R.r[2] = n; c.R.r[1] = norm(fwd - n * dot(fwd, n)); ortho(c.R);
    c.flip_t = c.out_t = 0; c.boost_left = 0;
}
static void make_car(int type, int slot, bool cpu) {
    Car c; c.type = type; c.d = &cardefs[type]; c.cpu = cpu;
    for (int i = 0; i < 4; i++) {
        const Part *pt = (size_t)(i + 1) < c.d->parts.size() ? &c.d->parts[i + 1] : NULL;
        c.wh[i].off = pt ? pt->off : V(i & 1 ? 1500.f : -1500.f, i < 2 ? 1500.f : -1900.f, 0);
        c.wh[i].r = pt && pt->radius > 0 ? pt->radius : 625;
        c.wh[i].front = i < 2; c.wh[i].driven = c.d->fwd4 || i >= 2;
    }
    for (const Part &p : c.d->parts) for (int k = 0; k < 3; k++) if (p.model[k]) { use_model(p.model[0]); break; }
    place_car(c, slot);
    cars.push_back(c);
}
static void reset_ball() {
    ball.p = V(0, 0, -bowl_c + lv.ball_r + 15000); ball.v = V(0, 0, 0); ball.w = V(0, 0, 0); ball.R = M();
    ball.scored = false; ball.out_t = 0; ball.last = -1;
}

static void cpu_drive(Car &c, float dt) {
    /* drive to the point behind the ball on the side of the arena middle, then through the ball (pushes it outward) */
    V b = ball.p, out = norm(V(b.x, b.y, 0));
    if (len(V(b.x, b.y, 0)) < 5000) out = norm(V(b.x - c.p.x, b.y - c.p.y, 0));
    V tgt = b;
    V to = V(b.x - c.p.x, b.y - c.p.y, 0);
    if (dot(norm(to), out) < 0.4f) tgt = b - out * (lv.ball_r + 9000);
    V l = to_local(c.R, tgt - c.p);
    float ang = atan2f(l.x, l.y);
    float sp = dot(c.v, c.R.r[1]);
    /* ball behind: the angle flips between +180 and -180 every frame, so the steering flipped and the car sat reversing on the
       spot (Bobby 10-18). Pick a side once, hold full lock until it faces the target, keep driving forward round the circle. */
    if (c.turn_dir == 0 && fabsf(ang) > 2.0f) c.turn_dir = ang > 0 ? 1 : -1;
    if (c.turn_dir != 0 && fabsf(ang) < 1.0f) c.turn_dir = 0;
    if (c.turn_dir != 0) { c.steer = (float)c.turn_dir; c.throttle = 0.8f; }
    else { c.steer = clampf(ang * 2.f, -1, 1); c.throttle = 1.f; }
    (void)sp;
    c.boost_req = fabsf(ang) < 0.15f && len(l) < 60000;
    /* pinned against the ball (pushing it into the wall: no goal ever comes of it, test 10-18): back off, then ram it */
    if (len(ball.p - c.p) < lv.ball_r + 7000 && len(ball.v) < 3000 && fabsf(sp) < 5000) c.stuck_t += dt; else c.stuck_t = 0;
    if (c.stuck_t > 0.8f) { c.back_t = 1.3f; c.stuck_t = 0; }
    if (c.back_t > 0) { c.back_t -= dt; c.throttle = -1; c.steer = 0; c.boost_req = false; }
}

static void step_car(Car &c, int idx, float dt) {
    const CarDef &d = *c.d;
    V F(0, 0, -G * d.mass), T(0, 0, 0);
    auto add_at = [&](V f, V at) { F += f; T += cross(at - c.p, f); };
    V up_w = c.R.r[2], fw_c = c.R.r[1], rt_c = c.R.r[0];
    float vfwd = dot(c.v, fw_c);
    /* engine */
    float vmax = (v_base + v_per_top * d.top) * lv.speed_f, amax = a_base * d.accel * lv.accel_f;
    if (c.cpu) { vmax *= lv.cripple; amax *= lv.cripple; }
    if (c.boost_left > 0) { vmax *= d.boost_t; amax *= d.boost_a; c.boost_left -= dt; if (c.boost_left <= 0) c.boost_charge = 0; }
    else { c.boost_charge = (std::min)(1.f, c.boost_charge + dt / (d.recharge > 0 ? d.recharge : 15.f));
        if (c.boost_req && c.boost_charge >= 1.f) { c.boost_left = 3.f; if (idx == me) play(9, sound(2), 110); } }
    float steer_ang = c.steer * d.steermax * 0.0174533f * (1.f - 0.35f * clampf(fabsf(vfwd) / vmax, 0, 1));
    int ndriven = 0; for (auto &w : c.wh) ndriven += w.driven;
    float k_s = d.mass * G / 4.f / 150.f, c_s = 2.f * 0.45f * sqrtf(k_s * d.mass / 4.f);
    bool any = false;
    for (Wheel &w : c.wh) {
        V anchor = c.p + to_world(c.R, w.off + V(0, 0, TRAVEL));
        float t = bowl_ray(anchor, -up_w);
        w.s_prev = w.s; w.contact = false;
        float hub_t = t - w.r;
        if (t < 0 || hub_t > 2 * TRAVEL) { w.s = -TRAVEL; w.spin_v *= 0.995f; w.spin += w.spin_v * dt; continue; }
        w.s = TRAVEL - hub_t;
        float extra = 0;
        if (w.s > TRAVEL) { extra = w.s - TRAVEL; w.s = TRAVEL; }
        w.contact = any = true;
        V hit = anchor - up_w * t, n; bowl_dist(hit + up_w * 50.f, n);
        float ds = (w.s - w.s_prev) / dt;
        float Fs = k_s * (w.s + 150.f) + c_s * ds + k_s * 20.f * extra;
        if (Fs < 0) Fs = 0;
        add_at(up_w * Fs, hit);
        /* tyre: wheel heading in the ground plane */
        float sa = w.front ? steer_ang : 0.f;
        V wf = fw_c * cosf(sa) + rt_c * sinf(sa);
        wf = norm(wf - n * dot(wf, n));
        V ws = cross(wf, n);
        V vc = c.v + cross(c.w, hit - c.p);
        float vl = dot(vc, wf), vs = dot(vc, ws);
        float mu = tyre_mu, Fmax = mu * Fs;
        float Flat = -clampf(vs / 1200.f, -1, 1) * Fmax;
        float Flong = 0;
        if (c.throttle > 0.02f) {
            if (vl < -1500.f) Flong = Fmax * 0.8f;                                   /* braking out of reverse */
            else if (w.driven) Flong = c.throttle * d.mass * amax / ndriven * clampf(1.f - vl / vmax, 0, 1);
        } else if (c.throttle < -0.02f) {
            if (vl > 1500.f) Flong = -Fmax * 0.8f * -c.throttle;                    /* brakes */
            else if (w.driven) Flong = c.throttle * d.mass * amax * 0.5f / ndriven * clampf(1.f + vl / (vmax * 0.35f), 0, 1);
        } else Flong = -clampf(vl / 2000.f, -1, 1) * Fs * 0.04f;                    /* rolling */
        float tot = sqrtf(Flat * Flat + Flong * Flong);
        if (tot > Fmax && tot > 0) { Flat *= Fmax / tot; Flong *= Fmax / tot; }
        add_at(wf * Flong + ws * Flat, hit);
        {   static float acc_d = 0; float slide = fabsf(vs) + (std::max)(0.f, fabsf(Flong) / (Fmax + 1.f) - 0.6f) * 20000.f;
            if (slide > 4000.f && len(c.v) > 3000.f) { acc_d += slide * dt * 0.0012f; if (acc_d > 1.f) { acc_d = 0;
                puff(hit + n * 300.f, c.v * 0.15f + n * 1500.f, 0.9f, 1300.f, 204, false, 2.4f); } }
        }
        w.spin_v = vl / w.r; w.spin += w.spin_v * dt;
    }
    /* body + roll cage against the bowl */
    std::vector<V> pts = d.cage;
    for (int i = 0; i < 8; i++) pts.push_back(V(i & 1 ? d.coll_hi.x : d.coll_lo.x, i & 2 ? d.coll_hi.y : d.coll_lo.y, i & 4 ? d.coll_hi.z : d.coll_lo.z));
    float k_b = d.mass * G / 25.f;
    for (const V &lp : pts) {
        V P = c.p + to_world(c.R, lp), n;
        if (P.z > 0) continue;
        float g = bowl_dist(P, n);
        if (g <= 0) continue;
        V vp = c.v + cross(c.w, P - c.p);
        float vn = dot(vp, n);
        float Fn = k_b * g - 2.f * sqrtf(k_b * d.mass / 6.f) * (std::min)(vn, 0.f);
        if (Fn < 0) Fn = 0;
        V vt = vp - n * vn; float vtl = len(vt);
        V Ff = vtl > 1 ? vt * (-0.5f * Fn / vtl) * clampf(vtl / 800.f, 0, 1) : V(0, 0, 0);
        add_at(n * Fn + Ff, P);
    }
    if (any) { float vv = dot(c.v, c.v); add_at(-up_w * (d.mass * downforce * vv), c.p); }   /* sticks to the walls at speed */
    /* air drag + a little angular damping */
    F -= c.v * (d.mass * 0.05f);
    V wl = to_local(c.R, c.w);
    V Tl = to_local(c.R, T);
    V al(Tl.x / d.moi.x, Tl.y / d.moi.y, Tl.z / d.moi.z);
    wl = wl + al * dt;
    float damp = any ? 0.985f : 0.995f;
    wl = wl * powf(damp, dt * 60.f);
    if (!any) {   /* in the air: the pod game let you level out a little (MAXANGVEL); hold the nose up */
        wl.x = clampf(wl.x, -4.f, 4.f); wl.y = clampf(wl.y, -6.f, 6.f);
    }
    c.w = to_world(c.R, wl);
    c.v += F * (dt / d.mass);
    c.p += c.v * dt;
    rotate(c.R, c.w, dt);
    /* upside down for 2 s, or out of the arena: put it back */
    V n; bowl_dist(c.p, n);
    if (dot(c.R.r[2], n) < 0.1f && len(c.v) < 6000) c.flip_t += dt; else c.flip_t = 0;
    float rr = sqrtf(c.p.x * c.p.x + c.p.y * c.p.y);
    if (rr > bowl_a + 3000 || c.p.z > 60000 || c.p.z < -bowl_c - 20000) c.out_t += dt; else c.out_t = 0;
    if (c.flip_t > 2.f || c.out_t > 1.5f) { bl("t=%.1f car %d reset (%s)", sim_t, idx, c.flip_t > 2 ? "upside down" : "out"); place_car(c, idx); }
    if (c.hit_cool > 0) c.hit_cool -= dt;
}

static void step_ball(float dt) {
    Ball &b = ball;
    float rb = lv.ball_r;
    b.v.z -= G * ball_g * dt;   /* the promo video's ball hangs in the air: lighter gravity than the cars */
    b.v = b.v * powf(1.f - lv.ball_air, dt * 30.f);
    V n;
    if (b.p.z < rb && !b.scored) {
        float g = bowl_dist(b.p, n) + rb;
        if (g > 0 && b.p.z < 0) {
            b.p += n * g;
            float vn = dot(b.v, n);
            if (vn < 0) {
                float e = vn < -2000 ? ball_floor_e : 0.f;
                b.v -= n * ((1.f + e) * vn);
                if (vn < -6000) play(10, sound(4), (int)clampf(-vn / 300.f, 20, 100));
            }
            /* rolling: the ball spins to match the ground, rolling resistance */
            V vt = b.v - n * dot(b.v, n);
            b.v -= vt * clampf(0.15f * dt, 0, 1);
            b.w = cross(n, vt) * (1.f / rb);
        }
    }
    b.p += b.v * dt;
    rotate(b.R, b.w, dt);
    b.w = b.w * powf(0.99f, dt * 30.f);
}

/* car-ball and car-car collisions (impulses) */
static void collide(float dt) {
    (void)dt;
    float rb = lv.ball_r, mb = lv.ball_mass;
    for (size_t i = 0; i < cars.size(); i++) {
        Car &c = cars[i]; const CarDef &d = *c.d;
        V lp = to_local(c.R, ball.p - c.p);
        V cp(clampf(lp.x, d.coll_lo.x, d.coll_hi.x), clampf(lp.y, d.coll_lo.y, d.coll_hi.y), clampf(lp.z, d.coll_lo.z, d.coll_hi.z));
        V dl = lp - cp; float dist = len(dl);
        if (dist >= rb) continue;
        V nl;
        if (dist > 1e-3f) nl = dl * (1.f / dist);
        else {   /* centre inside the box: out the nearest face */
            float best = 1e30f; for (int k = 0; k < 3; k++) {
                float lo = (&lp.x)[k] - (&d.coll_lo.x)[k], hi = (&d.coll_hi.x)[k] - (&lp.x)[k];
                if (lo < best) { best = lo; nl = V(0, 0, 0); (&nl.x)[k] = -1; }
                if (hi < best) { best = hi; nl = V(0, 0, 0); (&nl.x)[k] = 1; }
            }
            dist = 0;
        }
        V n = to_world(c.R, nl), P = c.p + to_world(c.R, cp);
        {   /* up the bowl wall the ball gets more lift: pushed into the wall it was pinned there (test 10-18: no goals in a round) */
            V bn; bowl_dist(ball.p, bn); float wall = 1.f - clampf(bn.z, 0, 1);
            V wall_up = norm(V(0, 0, 1) - bn * bn.z);   /* up the wall, toward the rim lip */
            if (fabsf(nl.z) < 0.6f) n = norm(n + c.R.r[2] * ball_lift + wall_up * (2.5f * wall));
        }   /* side / nose hits scoop the ball up (BALLBOUNCE 1.2: hits launch it) */
        float pen = rb - dist;
        ball.p += n * (pen * 0.85f); c.p -= n * (pen * 0.15f);
        V r = P - c.p;
        V vrel = ball.v - (c.v + cross(c.w, r));
        float vn = dot(vrel, n);
        if (vn >= 0) continue;
        V rn = to_local(c.R, cross(r, n));
        V ang = to_world(c.R, V(rn.x / d.moi.x, rn.y / d.moi.y, rn.z / d.moi.z));
        float kk = 1.f / mb + 1.f / d.mass + dot(cross(ang, r), n);
        float e = clampf(0.75f * lv.ball_bounce, 0.3f, 1.15f);
        float j = -(1.f + e) * vn / kk;
        ball.v += n * (j / mb);
        c.v -= n * (j / d.mass);
        c.w -= ang * j * spin_k;
        if (-vn > 9000) puff(P, n * 3000.f + V(0, 0, 2000), 1.0f, 2200.f, 204, false, 2.f);
        ball.last = (int)i;
        if (-vn > 3000) bl("t=%.1f hit car %zu vn %.0f j/m %.0f -> ball v (%.0f %.0f %.0f) at r %.0f z %.0f", sim_t, i, vn, j / mb, ball.v.x, ball.v.y, ball.v.z, sqrtf(ball.p.x * ball.p.x + ball.p.y * ball.p.y), ball.p.z);
        if (-vn > 2500) {
            play(11, sound(3), (int)clampf(-vn / 250.f, 30, 120), (int)clampf(dot(n, cars[me].R.r[0]) * -40.f, -64, 63));
            if ((int)i == me) VPIX_Haptic(2, 0.06f + clampf(-vn / 400000.f, 0, 0.1f), clampf(-vn / 30000.f, 0.3f, 1.f));
        }
    }
    for (size_t i = 0; i < cars.size(); i++) for (size_t k = i + 1; k < cars.size(); k++) {
        Car &a = cars[i], &b = cars[k];
        float ra = 0.5f * (a.d->coll_hi.x - a.d->coll_lo.x + a.d->coll_hi.y - a.d->coll_lo.y) * 0.5f;
        float rbb = 0.5f * (b.d->coll_hi.x - b.d->coll_lo.x + b.d->coll_hi.y - b.d->coll_lo.y) * 0.5f;
        V dv = b.p - a.p; float dd = len(dv);
        if (dd >= ra + rbb || dd < 1) continue;
        V n = dv * (1.f / dd); float pen = ra + rbb - dd;
        float ma = a.d->mass, mbb = b.d->mass;
        a.p -= n * (pen * mbb / (ma + mbb)); b.p += n * (pen * ma / (ma + mbb));
        float vn = dot(b.v - a.v, n);
        if (vn >= 0) continue;
        float j = -(1.3f) * vn / (1.f / ma + 1.f / mbb);
        a.v -= n * (j / ma); b.v += n * (j / mbb);
        {   /* spin: the side of the car that was hit is shoved, the car yaws round (off-centre hits spin most) */
            V cp = (a.p + b.p) * 0.5f;
            V ta = cross(cp - a.p, -n * j), tb = cross(cp - b.p, n * j);
            a.w += a.R.r[2] * (dot(ta, a.R.r[2]) / a.d->moi.z * spin_k) + a.R.r[2] * (((i + k) & 1 ? 1.f : -1.f) * clampf(-vn / 20000.f, 0, 1.5f));
            b.w += b.R.r[2] * (dot(tb, b.R.r[2]) / b.d->moi.z * spin_k) - b.R.r[2] * (((i + k) & 1 ? 1.f : -1.f) * clampf(-vn / 20000.f, 0, 1.5f));
            if (-vn > 6000) explosion(cp + V(0, 0, 1200), -vn);
        }
        if (-vn > 4000 && a.hit_cool <= 0) {
            a.hit_cool = b.hit_cool = 0.5f;
            if ((int)i == me || (int)k == me) { play(12, sound(34 + (int)(sim_t * 10) % 3), 120); VPIX_Haptic(2, 0.18f, 1.f); }
        }
    }
}

static void check_goal(float dt) {
    float rr = sqrtf(ball.p.x * ball.p.x + ball.p.y * ball.p.y);
    /* over the rim: the bowl's top edge is vertical, so a ball rammed up the wall rose straight up and fell back in (no goal in any
       test round). LEVELS.DBS BALLCLIPHEIGHT (5000 / 7500) = the ball counts as out once its centre is that close under the rim
       top out on the wall; it then flies out over the rim */
    if (!ball.scored && (rr > bowl_a || (rr > bowl_a * 0.8f && ball.p.z > -lv.ball_clip))) {
        ball.scored = true; ball.wait = 2.5f;
        V out = norm(V(ball.p.x, ball.p.y, 0)); float vo = dot(ball.v, out);
        ball.v += out * (std::max)(0.f, 14000.f - vo); if (ball.v.z < 12000.f) ball.v.z = 12000.f;
        if (ball.last >= 0) cars[ball.last].score++;
        goal_banner = 2.5f; goal_by = ball.last;
        bl("t=%.1f GOAL by car %d (scores: %s)", sim_t, ball.last, [] { std::string s; for (auto &c : cars) s += std::to_string(c.score) + " "; return s; }().c_str());
        play(13, sound_file_key("SOUNDS\\BONUS.WAV", 1), 120);
        if (ball.last == me) VPIX_Haptic(2, 0.4f, 1.f);
    }
    if (!ball.scored && ball.p.z < -bowl_c - 30000) { ball.scored = true; ball.wait = 0.5f; }
    if (ball.scored) { ball.wait -= dt; if (ball.wait <= 0) { reset_ball(); play(14, sound(8), 90); } }
}

/* ---- 2D bitmaps (RUNFILES\BITMAP.DBS: .ONE = 1 bpp glyphs drawn in a tint, .VST = 16-bit pictures), screen 352 x 288 ---- */
static const uint32_t BH = 0x20000;
struct Bmp { int w = 0, h = 0; bool ok = false; };
static std::map<int, Bmp> bmps;
static std::map<int, std::string> bmp_file;
static const Bmp &bmp(int id) {
    Bmp &b = bmps[id];
    if (b.ok || b.w < 0) return b;
    std::vector<uint8_t> d;
    auto it = bmp_file.find(id);
    if (it == bmp_file.end() || !read_host(it->second, d) || d.size() < 18 || !VPIX_Render_AddBitmap(BH + id, d.data(), d.size())) {
        bl("bitmap %d missing", id); b.w = -1; return b; }
    b.w = d[12] | d[13] << 8; b.h = d[14] | d[15] << 8; b.ok = true;
    return b;
}
static void blit(VPIX_Frame &f, int id, float x, float y, float sc = 1.f, uint32_t tint = 0xFFFFFF, float alpha = 1.f, float frac = 1.f) {
    const Bmp &b = bmp(id);
    if (!b.ok) return;
    VPIX_Blit bt = {};
    bt.handle = BH + id; bt.sx = 0; bt.sy = 0; bt.sw = (int)(b.w * frac + 0.5f); bt.sh = b.h;
    bt.x = (int)x; bt.y = (int)y; bt.dw = bt.sw * sc; bt.dh = b.h * sc; bt.key = 0; bt.alpha = alpha;
    bt.tint[0] = (tint >> 16 & 255) / 255.f; bt.tint[1] = (tint >> 8 & 255) / 255.f; bt.tint[2] = (tint & 255) / 255.f;
    bt.prio = 1000000; bt.world = false;
    if (bt.sw > 0) f.blits.push_back(bt);
}
static int glyph(char ch) {
    if (ch >= 'A' && ch <= 'Z') return 65 + (ch - 'A');
    if (ch >= 'a' && ch <= 'z') return 65 + (ch - 'a');
    if (ch >= '0' && ch <= '9') return 48 + (ch - '0');
    if (ch == ':') return 45;
    if (ch == '%') return 37;
    if (ch == '*') return 42;
    if (ch == '.') return 46;
    return 0;
}
static float text_w(const std::string &s, float sc) {
    float w = 0; for (char ch : s) { int g = glyph(ch); w += (g ? (float)(std::max)(bmp(g).w, 0) : 9.f) * sc + 2 * sc; } return w;
}
static void text(VPIX_Frame &f, const std::string &s, float x, float y, float sc = 1.f, uint32_t tint = 0xFFFFFF, bool centre = false, float alpha = 1.f) {
    if (centre) x -= text_w(s, sc) * 0.5f;
    for (char ch : s) { int g = glyph(ch); if (g) { blit(f, g, x, y, sc, tint, alpha); x += bmp(g).w * sc + 2 * sc; } else x += 11 * sc; }
}

static void music_play();
/* ---- FLIC movies: HLE decoder (VFlic / flic_frame), host file, paced at the file's speed; FLIC_SND.DBS sound scripts ---- */
struct FlicCue { int frame; std::string op; int a = 0, b = 0, c = 0, d = 0; };
static std::map<int, std::vector<FlicCue>> flic_cues;
static VFlic mov; static bool mov_on = false; static double mov_t = 0; static int mov_script = -1; static float mov_speed = 1;
static void flic_scripts() {
    int cur = -1, fr = 0;
    for (auto &t : read_dbs("RUNFILES\\FLIC_SND.DBS")) {
        std::string k = up(t[0]);
        if (k == "FLIC_SOUNDSCRIPT") { cur = (int)fa(t, 1); continue; }
        if (k == "END_SOUNDSCRIPT") { cur = -1; continue; }
        if (cur < 0) continue;
        if (k == "FRAME") { fr = (int)fa(t, 1); continue; }
        FlicCue q; q.frame = fr; q.op = k; q.a = (int)fa(t, 1); q.b = (int)fa(t, 2); q.c = (int)fa(t, 3); q.d = (int)fa(t, 4);
        if (k == "FLICSPEED") q.a = (int)(fa(t, 1) * 1000);
        flic_cues[cur].push_back(q);
    }
}
static bool movie(const char *rel, int script) {
    mov = VFlic(); mov_on = false;
    if (!read_host(rel, mov.d) || mov.d.size() < 128) { bl("movie %s missing", rel); return false; }
    mov.frames = mov.d[6] | mov.d[7] << 8; mov.w = mov.d[8] | mov.d[9] << 8; mov.h = mov.d[10] | mov.d[11] << 8;
    uint32_t sp; memcpy(&sp, mov.d.data() + 16, 4); mov.ms = sp > 0 && sp < 1000 ? sp : 70;
    mov.first = mov.pos = 128; mov.px.assign((size_t)mov.w * mov.h, 0); memset(mov.pal, 0, sizeof(mov.pal));
    mov_on = true; mov_t = 0; mov_script = script; mov_speed = 1;
    bl("t=%.1f movie %s %dx%d %d frames", sim_t, rel, mov.w, mov.h, mov.frames);
    return true;
}
static void movie_stop() { if (mov_on) VPIX_Render_SetFlic(NULL, 0, 0, 0, 0, false); mov_on = false; for (int i = 16; i < 24; i++) voices[i].on = false; }
/* advance; false when it has played through */
static bool movie_run(float el) {
    if (!mov_on) return false;
    mov_t += el * 1000.0 * mov_speed;
    bool shown = false;
    while (mov.cur == 0 || mov_t >= mov.ms * (mov.cur + 1)) {
        if (mov.cur >= mov.frames) { movie_stop(); return false; }
        flic_frame(mov); shown = true;
        if (mov_script >= 0) for (const FlicCue &q : flic_cues[mov_script]) if (q.frame == mov.cur) {
            if (q.op == "PLAY2D") play(16 + (q.a & 7), sound(q.b), q.d ? q.d : 127);
            else if (q.op == "STOP") voices[16 + (q.a & 7)].on = false;
            else if (q.op == "VOLUMEFADE") voices[16 + (q.a & 7)].vol = q.b;
            else if (q.op == "FLICSPEED") mov_speed = q.a / 1000.f;
            else if (q.op == "PLAYCD") music_play();
        }
    }
    if (shown) {
        std::vector<uint32_t> rgba(mov.px.size());
        for (size_t i = 0; i < mov.px.size(); i++) { uint32_t c = mov.pal[mov.px[i]]; rgba[i] = c | 0xFF000000u; }
        VPIX_Render_SetFlic(rgba.data(), mov.w, mov.h, (352 - mov.w) / 2, (288 - mov.h) / 2, true);
    }
    return true;
}

/* ---- the Gainesphere badge on the ball (Bobby 10-18: "put our logo on the ball"): two round decals on opposite sides,
   a polar patch of the sphere just outside the ball, textured with buggy_rebuild\assets\ball_logo_256.rgba (256x256 RGBA,
   rows bottom-up; made from Gainesphere Store\Logo Concepts 2026-09-25\final\gsphere_badge.png) ---- */
static const uint32_t LOGO_TEX = 0x7100, LOGO_MESH = MH + 0x700;
static bool logo_ok = false; static float logo_r = 0;
static void make_logo(float r) {
    if (logo_ok && logo_r == r) return;
    static bool tex_done = false;
    if (!tex_done) {
        tex_done = true;
        const char *e = getenv("VPIX_BUGGY_LOGO");
        std::string fn = e && *e ? e : "C:\\Users\\bobby\\Desktop\\Claude's Folder\\Projects\\Virtuality VR\\buggy_rebuild\\assets\\ball_logo_256.rgba";
        for (int i = 0; i < 2; i++) {   /* side 0 the Gainesphere badge, side 1 MallVR (yellow on navy) */
            std::string f1 = i ? fn.substr(0, fn.size() - 13) + "_logo2_256.rgba" : fn;   /* ..._logo_256.rgba -> ..._logo2_256.rgba */
            if (i) f1 = fn.substr(0, fn.rfind("ball_logo_256.rgba")) + "ball_logo2_256.rgba";
            FILE *f = fopen(f1.c_str(), "rb");
            std::vector<uint32_t> px(256 * 256);
            if (!f || fread(px.data(), 4, px.size(), f) != px.size()) { if (f) fclose(f); bl("ball logo %s missing", f1.c_str()); return; }
            fclose(f);
            VPIX_Render_AddTextureRGBA(LOGO_TEX + i, 256, 256, px.data());
        }
    }
    std::vector<float> vs; std::vector<VPIX_MeshPoly> ps;
    const int RINGS = 6, SEG = 24; const float TMAX = 0.62f, R = r * 1.006f;   /* ~35 deg round each pole of the decal */
    for (int side = 0; side < 2; side++) {
        V A = side ? V(-1, 0, 0) : V(1, 0, 0), B = side ? V(0, -1, 0) : V(0, 1, 0), C(0, 0, 1);   /* B = right, C = up as seen from outside */
        uint32_t base = (uint32_t)(vs.size() / 3);
        auto vert = [&](float th, float ph) { V d = A * cosf(th) + (B * cosf(ph) + C * sinf(ph)) * sinf(th); vs.push_back(d.x * R); vs.push_back(d.y * R); vs.push_back(d.z * R); };
        auto uvof = [&](int k, int m, float out[2]) { float q = 0.5f * k / RINGS, ph = 6.2831853f * m / SEG; out[0] = 0.5f + q * cosf(ph); out[1] = 0.5f + q * sinf(ph); };
        vert(0, 0);
        for (int k = 1; k <= RINGS; k++) for (int m = 0; m < SEG; m++) vert(TMAX * k / RINGS, 6.2831853f * m / SEG);
        auto id = [&](int k, int m) { return k == 0 ? base : base + 1 + (uint32_t)((k - 1) * SEG + (m % SEG)); };
        for (int k = 0; k < RINGS; k++) for (int m = 0; m < SEG; m++) {
            VPIX_MeshPoly q; q.flags = 0x20FFFFFF; q.tex = LOGO_TEX + side;
            int ks[4] = {k, k + 1, k + 1, k}, ms[4] = {m, m, m + 1, m + 1};
            int nv = k == 0 ? 3 : 4;
            int order[4] = {0, 1, 2, 3}; if (k == 0) { order[0] = 0; order[1] = 1; order[2] = 2; }
            for (int i = 0; i < nv; i++) { int j = order[i]; q.idx[i] = id(ks[j], ms[j]); uvof(ks[j], ms[j], q.uv[i]); }
            q.n = (uint8_t)nv;
            float th = TMAX * (k + 0.5f) / RINGS, ph = 6.2831853f * (m + 0.5f) / SEG;
            V d = A * cosf(th) + (B * cosf(ph) + C * sinf(ph)) * sinf(th); q.nx = d.x; q.ny = d.y; q.nz = d.z;
            ps.push_back(q);
        }
    }
    logo_ok = VPIX_Render_AddMesh(LOGO_MESH, vs, ps); logo_r = r;
    bl("ball logo mesh r=%.0f: %s (%zu polys)", r, logo_ok ? "ok" : "FAILED", ps.size());
}
static void put_logo(VPIX_Frame &f) {
    if (!logo_ok) return;
    VPIX_Instance in = {}; in.light_mask = 1;
    in.handle = LOGO_MESH;
    for (int k = 0; k < 3; k++) { in.R[k][0] = ball.R.r[k].x; in.R[k][1] = ball.R.r[k].y; in.R[k][2] = ball.R.r[k].z; in.S[k] = 1; }
    in.T[0] = ball.p.x; in.T[1] = ball.p.y; in.T[2] = ball.p.z;
    for (int e = 0; e < 16; e++) in.pal[e] = in.palw[e] = 0xFFFFFF;
    in.slot = (uint16_t)f.inst.size();
    f.inst.push_back(in);
}

/* ---- music: Buggy Ball played CD audio (FLIC_SND.DBS 'PLAYCD 1 ; start the CD up!!'); no disc rip exists, so any WAVs in
   VPIX_BUGGY_MUSIC (default _work\buggy_music) play in its place (Bobby 10-18: a friend is making music for games missing it) ---- */
static std::vector<std::string> music_files; static size_t music_next = 0; static bool music_scanned = false;
static void music_play() {
    if (!music_scanned) {
        music_scanned = true;
        const char *e = getenv("VPIX_BUGGY_MUSIC");
        std::string d = e && *e ? e : "H:\\Games\\Virtuality\\_work\\buggy_music";
#ifdef _WIN32
        WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA((d + "\\*.wav").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) { do music_files.push_back(d + "\\" + fd.cFileName); while (FindNextFileA(h, &fd)); FindClose(h); }
#endif
        std::sort(music_files.begin(), music_files.end());
        bl("music: %zu WAV files in %s", music_files.size(), d.c_str());
    }
    if (music_files.empty()) return;
    const std::string &fn = music_files[music_next++ % music_files.size()];
    uint32_t key = 0x62000u | (uint32_t)(music_next % 256);
    FILE *f = fopen(fn.c_str(), "rb"); if (!f) return;
    std::vector<uint8_t> d; fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n > 0) { d.resize((size_t)n); if (fread(d.data(), 1, d.size(), f) != d.size()) d.clear(); }
    fclose(f);
    VSound snd; if (!parse_wav(d, snd)) { bl("music %s: not a WAV we read", fn.c_str()); return; }
    snd.loop = true; voices[31].on = false; sounds[key] = std::move(snd); sound_names[key] = fn;
    play(31, (int)key, 85);
    bl("t=%.1f music %s", sim_t, fn.c_str());
}
static void music_stop() { voices[31].on = false; }

/* ---- game flow ---- */
enum State { S_TITLE, S_LEVEL, S_CAR, S_COUNT, S_PLAY, S_OVER, S_INTRO };
static State st = S_TITLE; static float st_t = 0;
static int sel_level = 0, sel_car = 0, cur_level = -1;
static float round_len = 78.22f, time_left = 78.22f;   /* round_len = one level (TIME.DBS); time_left = the whole game */
static Car show_car;
static float game_len = 240.f, level_left = 78.22f; static float level_banner = 0;
static bool outside_view = true;      /* the promo video is played almost all in the low chase view; A toggles the cockpit */
static int ncpu_cfg = -1;
static int attract = 0; static bool demo = false;
static std::string car_name(int type) {
    std::string n = cardefs[type].name; size_t u = n.find('_'); if (u != std::string::npos) n.resize(u);
    if (n.size() > 2 && n.back() == 's' && n[n.size() - 2] != 's') n.pop_back();
    return up(n);
}
static uint32_t car_colour(const Car &c) { return c.d->parts.empty() ? 0xFFFFFF : c.d->parts[0].pen[0] ? c.d->parts[0].pen[0] : 0xFFFFFF; }
static void set_state(State s) { st = s; st_t = 0; bl("t=%.1f state %d", sim_t, (int)s); }
static void setup_level(int level) {
    if (level != cur_level) {
        model_file.clear(); cardefs.clear(); lv = Level();
        load_rules(level); bowl_shape(lv.bowl_model); cur_level = level; make_logo(lv.ball_r);
        for (auto &t : read_dbs("RUNFILES\\TIME.DBS")) if (!t.empty() && atof(t[0].c_str()) > 5) round_len = (float)atof(t[0].c_str());
    }
}
#include "vpix_buggy_net.h"
static void start_round() {
    cars.clear();
    int car = lv.cars.empty() ? 3 : lv.cars[sel_car % lv.cars.size()];
    make_car(car, 0, false);
    int humans = demo ? 0 : net::add_humans();   /* link play: linked players' cars next */
    int n = ncpu_cfg >= 0 ? ncpu_cfg : 3;   /* the flyer: up to 4 players; the pod filled the rest with computer drivers */
    n = (std::min)(n, 3 - humans);
    /* link play with 2-3 people (Bobby 10-19): matches alternate - humans only (a personal 2- or 3-player match), then
       the computer fills the empty seats (4 cars), and so on. Alone you always get computer drivers. VPIX_BUGGY_ALTERNATE=0
       = always fill; VPIX_BUGGY_CPU sets a fixed count. */
    static int match_no = 0;
    if (!demo && humans > 0 && humans < 3 && ncpu_cfg < 0) {
        static const bool alt = !(getenv("VPIX_BUGGY_ALTERNATE") && *getenv("VPIX_BUGGY_ALTERNATE") == '0');
        if (alt && ++match_no % 2 == 1) n = 0;
        bl("round: match %d with %d linked players: %s", match_no, humans + 1, n ? "computer fills the empty seats" : "humans only");
    }
    /* opponents: one of each character, none the same as yours (Bobby 10-18: all three were moose) - this level's cars first,
       then the other drivable ones (6 elephant 4x4 0, moose 1, rabbit 2, croc 3, quad 4, mini 5; 6-8 are cheat cars) */
    /* ...then random, pairs allowed (Bobby: "or have it random, some times pairs"), never all three the same */
    static std::mt19937 rng((unsigned)std::chrono::steady_clock::now().time_since_epoch().count());
    std::vector<int> pool; for (int t = 0; t <= 5; t++) if (cardefs.count(t)) pool.push_back(t);
    std::vector<int> pick;
    for (int tries = 0; tries < 20; tries++) {
        pick.clear(); for (int i = 0; i < n && i < 3; i++) pick.push_back(pool[rng() % pool.size()]);
        if (pick.size() < 3 || !(pick[0] == pick[1] && pick[1] == pick[2])) break;
    }
    for (size_t i = 0; i < pick.size(); i++) make_car(pick[i], (int)cars.size(), true);
    puffs.clear();
    reset_ball(); time_left = game_len; level_left = round_len; goal_banner = 0; level_banner = 3.f;
    if (demo) { for (auto &c : cars) c.cpu = true; return; }
    play(15, sound(cardefs[car].engine_snd == 12 ? 23 : 22), 110);
    bl("round: level %d, car %d (%s) + %d computer cars, %.1f s", cur_level, car, cardefs[car].name.c_str(), n, round_len);
}

/* ---- frame ---- */
static void put(VPIX_Frame &f, int model, const M &R, V T, const uint32_t *pen = NULL, const bool *penset = NULL, uint32_t texpen3 = 0) {
    if (!model || !use_model(model)) return;
    VPIX_Instance in = {}; in.light_mask = 1;
    in.handle = MH + model;
    for (int k = 0; k < 3; k++) { in.R[k][0] = R.r[k].x; in.R[k][1] = R.r[k].y; in.R[k][2] = R.r[k].z; in.S[k] = 1; }
    in.T[0] = T.x; in.T[1] = T.y; in.T[2] = T.z;
    for (int e = 0; e < 16; e++) { in.pal[e] = pen && (!penset || penset[e]) ? pen[e] : 0x808080; in.palw[e] = in.pal[e]; }
    if (texpen3) in.palw[3] = texpen3;
    in.slot = (uint16_t)f.inst.size();
    f.inst.push_back(in);
}
static void put_car(VPIX_Frame &f, const Car &c, bool hide_driver) {
    const CarDef &d = *c.d;
    for (size_t pi = 0; pi < d.parts.size(); pi++) {
        const Part &p = d.parts[pi];
        if (pi >= 1 && pi <= 4) {      /* wheels: suspension, steering, spin */
            const Wheel &w = c.wh[pi - 1];
            float sa = w.front ? c.steer * d.steermax * 0.0174533f * 0.8f : 0.f;
            M carrier; carrier.r[1] = c.R.r[1] * cosf(sa) + c.R.r[0] * sinf(sa); carrier.r[2] = c.R.r[2]; ortho(carrier);
            M W; float cs = cosf(w.spin), sn = sinf(w.spin);
            W.r[0] = carrier.r[0];
            W.r[1] = carrier.r[1] * cs - carrier.r[2] * sn;
            W.r[2] = carrier.r[1] * sn + carrier.r[2] * cs;
            if (w.off.x > 0) { W.r[0] = -W.r[0]; W.r[1] = -W.r[1]; }   /* right side: hub cap outward */
            put(f, p.model[0], W, c.p + to_world(c.R, w.off + V(0, 0, w.s)), p.pen, p.penset);
            continue;
        }
        if (pi == 5 && hide_driver) continue;   /* your own driver: you are sitting in it */
        put(f, p.model[0], c.R, c.p + to_world(c.R, p.off), p.pen, p.penset);
    }
}
static V head_ref; static float yaw0 = 0; static bool have_ref = false;
static V eye_local; static bool eye_forced = false;
/* model bounding box from the file (first sub-model's vertices) */
static bool model_box(int id, V &lo, V &hi) {
    std::vector<uint8_t> d; auto it = model_file.find(id);
    if (it == model_file.end() || !read_host(it->second, d) || d.size() < 64) return false;
    auto be = [&](size_t o) { return o + 4 <= d.size() ? (uint32_t)d[o] << 24 | d[o + 1] << 16 | d[o + 2] << 8 | d[o + 3] : 0u; };
    uint32_t ty = be(16); size_t sub = 20 + ((ty >> 16) == 1 ? be(20) : 0);
    uint32_t ov = be(sub), nv = be(sub + ov - 4);
    if (!nv || nv > 100000) return false;
    lo = V(1e9f, 1e9f, 1e9f); hi = V(-1e9f, -1e9f, -1e9f);
    for (uint32_t i = 0; i < nv; i++) { float f[3]; for (int k = 0; k < 3; k++) { uint32_t w = be(sub + ov + 12 * i + 4 * k); memcpy(&f[k], &w, 4); }
        lo = V((std::min)(lo.x, f[0]), (std::min)(lo.y, f[1]), (std::min)(lo.z, f[2])); hi = V((std::max)(hi.x, f[0]), (std::max)(hi.y, f[1]), (std::max)(hi.z, f[2])); }
    return true;
}
/* cockpit eye: at the car's own driver (CARS.DBS part 5, the animal) - its head height, middle of its body, plus EYEOFFSET's
   height share; one fixed seat put the view inside the other cars' bodies (Bobby 10-18) */
static V car_eye(const CarDef &d) {
    if (eye_forced || d.parts.size() < 6) return eye_local;
    const Part &dr = d.parts[5]; V lo, hi;
    if (!model_box(dr.model[0], lo, hi)) return dr.off + V(0, 0, 600);
    return dr.off + V(0, (lo.y + hi.y) * 0.5f, hi.z * 0.85f + d.eye.z * 0.3f);
}
static V chase_p; static bool chase_init = false;
static M look_at(V from, V to) {
    M m; m.r[1] = norm(to - from); m.r[0] = norm(cross(m.r[1], V(0, 0, 1))); m.r[2] = cross(m.r[0], m.r[1]); return m;
}

static void step_puffs(float el) {
    for (Puff &q : puffs) { q.t += el; q.p += q.v * el; q.v = q.v * powf(0.3f, el); q.v.z += 800.f * el; }
    puffs.erase(std::remove_if(puffs.begin(), puffs.end(), [](const Puff &q) { return q.t >= q.life; }), puffs.end());
}
static void draw_puffs(VPIX_Frame &f) {
    for (const Puff &q : puffs) {
        float u = q.t / q.life;
        int id = q.anim ? 100 + (std::min)(4, (int)(u * 5.f)) : q.bmp;
        const Bmp &b = bmp(id); if (!b.ok) continue;
        VPIX_Blit bt = {};
        bt.handle = BH + id; bt.sw = b.w; bt.sh = b.h; bt.key = 0; bt.world = true;
        float sz = q.size * (1.f + q.grow * u);
        bt.dw = bt.dh = sz; bt.pivot[0] = bt.pivot[1] = sz * 0.5f;
        bt.pos[0] = q.p.x; bt.pos[1] = q.p.y; bt.pos[2] = q.p.z;
        bt.alpha = q.anim ? 1.f - u * 0.5f : 0.75f * (1.f - u);
        bt.prio = 0;
        f.blits.push_back(bt);
    }
}
static void build_frame(unsigned long fr, const VPIX_Input &in) {
    VPIX_Frame f; f.frame = fr; f.fov_deg = 70.f;
    {   /* card rules (VPIX_CARD_ORDER): BALL.EXE (which set the real lights) is lost; LEVELS.DBS only gives BOWLLIGHT 255 255 255.
           Stand-in = Dactyl Nightmare 2's light 0 (same era): ambient 0.7, diffuse 0.3, from above. A GUESS (10-25). */
        static const bool card = getenv("VPIX_CARD_ORDER") && atoi(getenv("VPIX_CARD_ORDER"));
        if (card) { f.n_lights = 1; f.lights[0] = {0.7f, 0.3f, {-0.218f, -0.437f, 0.874f}}; }
    }
    f.clear[0] = lv.sky[0]; f.clear[1] = lv.sky[1]; f.clear[2] = lv.sky[2];
    {   M I; uint32_t pen[16] = {lv.rim1, lv.rim2, lv.middle, 0x808080};
        put(f, lv.bowl_model, I, V(0, 0, 0), pen, NULL, (uint32_t)lv.bowl_tex); f.inst.back().prio = -1; }   /* stadium first (card order) */
    for (int k = 0; k < 6; k++) {   /* light towers: A-frames standing outside the rim, tops leaning in over the bowl (promo video) */
        float a = (k + 0.5f) * 3.14159265f / 3.f;
        M R; R.r[1] = V(cosf(a), sinf(a), 0); R.r[2] = V(0, 0, 1); R.r[0] = cross(R.r[1], R.r[2]);
        uint32_t pen[16] = {0xFFFFFF, 0xDADA9A};
        put(f, 237, R, R.r[1] * (bowl_a + 12000.f), pen);
    }
    {   uint32_t pen[16]; for (int e = 0; e < 16; e++) pen[e] = lv.ballpen[e & 1];
        put(f, lv.ball_model, ball.R, ball.p, pen);
        put_logo(f);
        float t = bowl_ray(ball.p, V(0, 0, -1));
        if (t > 0 && ball.p.z < 0) {
            V hp = ball.p - V(0, 0, t), n; bowl_dist(hp, n);
            M S; S.r[2] = n; S.r[1] = norm(V(0, 1, 0) - n * n.y); ortho(S);
            uint32_t sp[16] = {0x101010};
            put(f, 250, S, hp + n * 40.f, sp);
        }
    }
    bool playing = st == S_COUNT || st == S_PLAY || st == S_OVER || (st == S_TITLE && demo);
    if (playing) draw_puffs(f);
    M cam; V eye;
    if (playing && !cars.empty()) {
        const Car &c = cars[me];
        for (size_t i = 0; i < cars.size(); i++) put_car(f, cars[i], (int)i == me && !outside_view);
        if (outside_view) {   /* low chase view: behind and above, lagging a little */
            V fl = norm(V(c.R.r[1].x, c.R.r[1].y, 0));
            {   /* ball cam: look from behind the car toward the ball (Bobby 10-18: "dynamic follow cam that stays trained on the ball") */
                static V cdir = fl; V tb = V(ball.p.x - c.p.x, ball.p.y - c.p.y, 0);
                V want_dir = len(tb) > 3000.f ? norm(tb) : fl;
                cdir = norm(cdir + (want_dir - cdir) * 0.06f); fl = cdir;
            }
            V want = c.p - fl * 13000.f + V(0, 0, 4200);
            {   /* keep the camera inside the arena and 3000 above the floor under it (the old push failed near the rim: views in the floor) */
                float r = sqrtf(want.x * want.x + want.y * want.y), rmax = bowl_a * 0.93f;
                if (r > rmax) { want.x *= rmax / r; want.y *= rmax / r; r = rmax; }
                float k = r / (bowl_a * panel_scale(want.x, want.y)), zf = -bowl_c * sqrtf((std::max)(0.f, 1.f - k * k));
                if (want.z < zf + 3000.f) want.z = zf + 3000.f;
            }
            if (!chase_init || len(want - chase_p) > 60000) chase_p = want;
            chase_p = chase_p + (want - chase_p) * 0.12f; chase_init = true;
            {   V tb = ball.p - c.p; float dl = len(tb);
                V tgt = c.p + fl * 4000.f + V(0, 0, 800);
                if (dl > 1000.f && dl < 150000.f) {   /* frame car + ball: aim a little past the car toward the ball, kept low so the car sits in the lower third */
                    float w = clampf(2500.f / dl + 0.15f, 0.15f, 0.4f);
                    tgt = c.p + V(tb.x, tb.y, 0) * w + V(0, 0, 900.f + clampf(tb.z * 0.2f, -1500.f, 2500.f));
                }
                cam = look_at(chase_p, tgt); eye = chase_p; }
        } else { static int eye_type = -1; static V eye_l; if (eye_type != c.type) { eye_l = car_eye(*c.d); eye_type = c.type; bl("cockpit eye car %d: %.0f %.0f %.0f", c.type, eye_l.x, eye_l.y, eye_l.z); }
            cam = c.R; eye = c.p + to_world(c.R, eye_l); chase_init = false; }
    } else {   /* menus: slow orbit over the rim */
        float a = (float)sim_t * 0.04f;
        eye = V(sinf(a) * 150000.f, -cosf(a) * 150000.f, 22000.f);
        cam = look_at(eye, V(0, 0, -28000));
        if (st == S_CAR && !lv.cars.empty()) {   /* the car on offer, turning in front of you */
            int type = lv.cars[sel_car % lv.cars.size()];
            if (show_car.type != type || !show_car.d) { show_car = Car(); show_car.type = type; show_car.d = &cardefs[type];
                for (int i = 0; i < 4; i++) { const Part *pt = (size_t)(i + 1) < show_car.d->parts.size() ? &show_car.d->parts[i + 1] : NULL;
                    show_car.wh[i].off = pt ? pt->off : V(); show_car.wh[i].front = i < 2; } }
            float ya = (float)sim_t * 0.8f;
            show_car.R.r[0] = V(cosf(ya), sinf(ya), 0); show_car.R.r[1] = V(-sinf(ya), cosf(ya), 0); show_car.R.r[2] = V(0, 0, 1);
            V hf = norm(V(cam.r[1].x, cam.r[1].y, 0));
            show_car.p = eye + hf * 11000.f - V(0, 0, 2500);
            put_car(f, show_car, false);
        }
    }
    /* where you face when the game starts (or you hold A) is forward; head movement leans you (cockpit view) */
    float cy = cosf(yaw0), sy = sinf(yaw0);
    M cr; cr.r[0] = cam.r[0] * cy - cam.r[1] * sy; cr.r[1] = cam.r[0] * sy + cam.r[1] * cy; cr.r[2] = cam.r[2];
    if (in.head.valid && in.vr && have_ref && playing && !outside_view) {
        V dh = V(in.head.p[0], in.head.p[1], in.head.p[2]) - head_ref;
        V dw = to_world(cr, dh) * world_scale;
        if (len(dw) > 900) dw = norm(dw) * 900.f;
        eye += dw;
    }
    for (int k = 0; k < 3; k++) { f.cam_R[k][0] = cr.r[k].x; f.cam_R[k][1] = cr.r[k].y; f.cam_R[k][2] = cr.r[k].z; }
    f.cam_T[0] = eye.x; f.cam_T[1] = eye.y; f.cam_T[2] = eye.z;
    f.in_head = false;

    /* ---- 2D ---- */
    const float CX = 176.f;
    if (st == S_TITLE) {   /* attract: movies, then a computer demo game with the rules over it (the flyer's wording) */
        if (demo) {
            static const char *rules[] = {"BUGGY BALL", "PUSH THE BALL", "OUT OF THE ARENA", "OVER THE RIM", "", "A GAINESPHERE REBUILD",
                                          "EVERY GOAL", "SCORES ONE POINT", "THE BALL COMES BACK", "TO THE MIDDLE", "FOUR LEVELS IN 4:00", "MOST GOALS WINS",
                                          "THANKS TO", "JEREMY SHARTZER", "FOR BUGGY BALL", "", "ERRORDAN", "FOR THE 3000 SERIES"};
            static const char *ctrl[] = {"CONTROLS", "RIGHT TRIGGER  GAS", "LEFT TRIGGER  BRAKE", "STICK  STEER", "GRIP OR B  TURBO", "",
                                         "A  CHANGE VIEW", "HOLD A  RECENTRE", "", "RAM THE BALL", "UP THE WALL", "TO SCORE",
                                         "STILL MISSING", "SPHERE", "XTREME STRIKE", "", "GOT THEM", "TELL US ON DISCORD"};
            const char **pg = attract == 1 ? rules : ctrl;
            int page = (int)(st_t / 6.f) % 3;   /* 6 lines a page */
            for (int i = 0; i < 6; i++) {
                const char *ln = pg[page * 6 + i];
                bool head = i == 0 && page != 1;
                if (*ln) text(f, ln, CX, 40.f + i * 26.f, head ? 1.1f : 0.75f, head ? 0xFFFF00 : 0xFFFFFF, true);
            }
        }
        if (fmodf(st_t, 1.2f) < 0.8f) blit(f, 12, CX - 100, 205, 1.f, 0xFFFFFF);   /* PRESS TRIGGER WHEN READY */
        blit(f, 214, CX - 68, 258, 1.f, 0x80FF80);                                 /* FREE PLAY */
        if (demo && goal_banner > 0) text(f, "GOAL", CX, 120, 2.5f, 0xFFFF00, true, 0.9f);
    } else if (st == S_LEVEL) {
        text(f, "CHOOSE LEVEL", CX, 50, 1.f, 0xFFFF00, true);
        for (int i = 0; i < 4; i++) {
            bool on = i == sel_level; float sc = on ? 0.52f : 0.4f;
            float w = 175 * sc, h = 120 * sc, x = CX + (i - 1.5f) * 78.f - w * 0.5f, y = 140 - h * 0.5f;
            blit(f, 30 + i, x, y, sc, 0xFFFFFF, on ? 1.f : 0.45f);
        }
        static const char *nm[4] = {"EASY", "MEDIUM", "HARD", "CRAZY"};
        text(f, nm[sel_level & 3], CX, 200, 1.f, 0xFFFFFF, true);
    } else if (st == S_CAR) {
        blit(f, 14, CX - 144, 40, 1.f, 0xFFFF00);                                 /* CHOOSE YOUR VEHICLE */
        if (!lv.cars.empty()) text(f, car_name(lv.cars[sel_car % lv.cars.size()]), CX, 225, 1.f, 0xFFFFFF, true);
    } else if (st == S_COUNT) {
        int n = 3 - (int)st_t;
        if (n >= 1) text(f, std::to_string(n), CX, 110, 3.f, 0xFFFF00, true);
    }
    static const char *lvname[4] = {"EASY", "MEDIUM", "HARD", "CRAZY"};
    if ((st == S_COUNT || st == S_PLAY) && level_banner > 0 && cur_level >= 0)
        text(f, "LEVEL " + std::to_string(cur_level + 1) + " " + lvname[cur_level & 3], CX, 180, 1.f, 0x80FFFF, true);
    if (st == S_COUNT || st == S_PLAY) {
        if (st == S_PLAY && st_t < 1.f) text(f, "GO", CX, 110, 3.f, 0x40FF40, true);
        int s = (int)ceilf((std::max)(time_left, 0.f));
        char tb[16]; snprintf(tb, sizeof(tb), "%d:%02d", s / 60, s % 60);
        text(f, tb, CX, 34, 1.f, time_left < 10.f ? 0xFF4040 : 0xFFFFFF, true);
        if (cur_level >= 0) text(f, lvname[cur_level & 3], CX, 58, 0.6f, 0x80FFFF, true);
        if (!cars.empty()) {   /* scores: yours left, the others right in their car colours */
            text(f, "YOU " + std::to_string(cars[me].score), 24, 34, 1.f, car_colour(cars[me]));
            float y = 34;
            for (size_t i = 0; i < cars.size(); i++) if ((int)i != me) {
                std::string sc = std::to_string(cars[i].score);
                text(f, sc, 326 - text_w(sc, 1.f), y, 1.f, car_colour(cars[i])); y += 24;
            }
            /* turbo bar: empty frame, full part = charge (or what is left of a boost) */
            const Car &c = cars[me];
            float fr2 = c.boost_left > 0 ? c.boost_left / 3.f : c.boost_charge;
            blit(f, 2, CX - 37, 240, 1.f, 0xFFFFFF);
            blit(f, 1, CX - 37, 240, 1.f, c.boost_charge >= 1.f || c.boost_left > 0 ? 0xFFFF00 : 0xFF8000, 1.f, clampf(fr2, 0, 1));
        }
        if (goal_banner > 0) {
            text(f, "GOAL", CX, 100, 2.5f, 0xFFFF00, true);
            if (goal_by >= 0 && goal_by < (int)cars.size())
                text(f, goal_by == me ? std::string("YOU SCORED") : car_name(cars[goal_by].type) + " SCORED", CX, 160, 1.f, car_colour(cars[goal_by]), true);
        }
    }
    if (st == S_OVER && !cars.empty() && !mov_on) {
        int rank = 0; for (size_t i = 0; i < cars.size(); i++) if ((int)i != me && cars[i].score > cars[me].score) rank++;
        blit(f, 20 + (std::min)(rank, 3), CX - 57, 40, 0.6f);                    /* cup: 1st .. 4th */
        std::vector<int> ord; for (size_t i = 0; i < cars.size(); i++) ord.push_back((int)i);
        std::sort(ord.begin(), ord.end(), [](int a, int b) { return cars[a].score > cars[b].score; });
        float y = 170;
        for (int i : ord) { text(f, (i == me ? std::string("YOU") : car_name(cars[i].type)) + " " + std::to_string(cars[i].score), CX, y, 1.f, car_colour(cars[i]), true); y += 24; }
    }
    net::draw(f);
    VPIX_Render_Submit(f);
}

/* ---- loop: a PIC event every 2 ms of DOS time, simulated on the host clock at 240 Hz ---- */
static auto t_last = std::chrono::steady_clock::now();
static double acc = 0; static unsigned long frames = 0; static double next_log = 1;
static bool running = false;
struct Edge { bool was = false; bool operator()(bool now) { bool e = now && !was; was = now; return e; } };
static std::string scores_str() { std::string s; for (auto &c : cars) s += std::to_string(c.score) + " "; return s; }
static void tick(Bitu) {
    if (!running) return;
    auto now = std::chrono::steady_clock::now();
    double el = std::chrono::duration<double>(now - t_last).count(); t_last = now;
    acc += (std::min)(el, 0.1);
    const float dt = 1.f / 240.f;
    VPIX_Input in; VPIX_Input_Get(in);
    /* seat reference from the first REAL headset pose (the desktop fallback pose at start put Bobby's view in the hood, 10-18) */
    if (in.vr && in.head.valid && !have_ref) { head_ref = V(in.head.p[0], in.head.p[1], in.head.p[2]); yaw0 = atan2f(-in.head.R[1][0], in.head.R[1][1]); have_ref = true;
        bl("t=%.1f seat set (head %.0f %.0f %.0f yaw %.1f)", sim_t, head_ref.x, head_ref.y, head_ref.z, yaw0 * 57.3f); }
    /* A (menu action): tap = chase / cockpit view, hold 0.8 s = recentre the seat */
    static float a_held = 0; static bool a_was = false, a_used = false;
    if (in.start) { a_held += (float)el; if (a_held > 0.8f && !a_used && in.head.valid) { a_used = true;
            head_ref = V(in.head.p[0], in.head.p[1], in.head.p[2]); yaw0 = atan2f(-in.head.R[1][0], in.head.R[1][1]); have_ref = true;
            VPIX_Haptic(2, 0.1f, 0.6f); bl("t=%.1f seat recentred (yaw %.1f)", sim_t, yaw0 * 57.3f); } }
    else { if (a_was && !a_used && (st == S_PLAY || st == S_COUNT)) { outside_view = !outside_view; bl("t=%.1f view %s", sim_t, outside_view ? "chase" : "cockpit"); }
        a_held = 0; a_used = false; }
    a_was = in.start;
#ifdef _WIN32
    {   static Edge v_e;   /* desktop: V = chase / cockpit view */
        if (v_e((GetAsyncKeyState('V') & 0x8000) != 0) && !in.vr && (st == S_PLAY || st == S_COUNT)) outside_view = !outside_view; }
#endif
    /* menu input: trigger (or Space) = select, stick left/right = choose */
    static Edge sel_e, l_e, r_e;
    bool sel = sel_e(in.throttle > 0.6f || (in.buttons & VPIX_BTN_TRIGGER) != 0);
    bool left = l_e(in.joy[0] < -0.5f), right = r_e(in.joy[0] > 0.5f);
    if (autopilot && st != S_PLAY && st != S_COUNT && st_t > 1.f) sel = true;
    if (!net::pre_tick((float)el, sel, left, right))   /* link play: a slave in the master's game follows its state instead */
    switch (st) {
    case S_TITLE:
        if (sel && st_t > 0.5f) { movie_stop(); demo = false; cars.clear(); for (auto &v : voices) v.on = false; play(14, sound(29), 100); set_state(S_LEVEL); break; }
        if (attract == 0 || attract == 2) {
            if (!mov_on && st_t < 0.05f) movie(attract == 0 ? "RUNFILES\\FLIC\\TITLE.FLC" : "RUNFILES\\FLIC\\FINS.FLC", attract == 0 ? -1 : 0);
            if (!movie_run((float)el)) { attract++; demo = true; setup_level(0); start_round(); st_t = 0; } }
        else if (st_t > 18.f) { attract = (attract + 1) % 4; demo = false; cars.clear(); for (auto &v : voices) v.on = false; st_t = 0; }
        break;
    case S_LEVEL:
        if (left) { sel_level = (sel_level + 3) % 4; play(14, sound(4), 70); }
        if (right) { sel_level = (sel_level + 1) % 4; play(14, sound(4), 70); }
        if (sel && st_t > 0.3f) { setup_level(sel_level); sel_car = 0; play(14, sound(29), 100); set_state(S_CAR); }
        break;
    case S_CAR: {
        int n = (std::max)(1, (int)lv.cars.size());
        if (left) { sel_car = (sel_car + n - 1) % n; play(14, sound(4), 70); }
        if (right) { sel_car = (sel_car + 1) % n; play(14, sound(4), 70); }
        if (sel && st_t > 0.3f) { start_round(); set_state(S_INTRO); movie("RUNFILES/FLIC/FINS.FLC", 0); }   /* the garage: lights on, door up, cars drive out */
        break; }
    case S_INTRO: if (!movie_run((float)el) || (sel && st_t > 1.f)) { movie_stop(); play(14, sound(30), 110); set_state(S_COUNT); } break;
    case S_COUNT: if (st_t >= 3.f) { set_state(S_PLAY); play(14, sound(30), 120); if (!voices[31].on) music_play(); } break;
    case S_PLAY:
        /* next level: the level's TIME.DBS time is up and at least 30 s of the game are left for it (else this one runs to the end) */
        if (level_left <= 0 && cur_level < 3 && time_left > 30.f) {
            int keep[4] = {0, 0, 0, 0}; for (size_t i = 0; i < cars.size() && i < 4; i++) keep[i] = cars[i].score;
            setup_level(cur_level + 1);
            for (size_t i = 0; i < cars.size(); i++) { cars[i].d = &cardefs[cars[i].type]; place_car(cars[i], (int)i); cars[i].score = keep[i & 3];
                for (Wheel &w : cars[i].wh) w.s = w.s_prev = 0; }
            show_car = Car(); reset_ball(); level_left = round_len; level_banner = 3.f;
            bl("t=%.1f level up -> %d, %.0f s of the game left", sim_t, cur_level, time_left);
            play(14, sound(29), 110); set_state(S_COUNT); break;
        }
        if (time_left <= 0) { music_stop(); set_state(S_OVER); movie("RUNFILES\\FLIC\\END.FLC", 1); play(13, sound(cars[me].score >= 1 ? 50 : 60), 120);
            bl("t=%.1f round over: %s", sim_t, scores_str().c_str()); } break;
    case S_OVER:
        if (mov_on) { movie_run((float)el); if (sel && st_t > 1.f) movie_stop(); break; }
        if ((sel && st_t > 3.f) || st_t > 25.f) { for (auto &v : voices) v.on = false; attract = 0; set_state(S_TITLE); } break;
    }
    int n = 0;
    bool sim = st == S_COUNT || st == S_PLAY || st == S_OVER || (st == S_TITLE && demo);
    bool live = st == S_PLAY || (st == S_TITLE && demo);
    if (sim && !cars.empty() && !autopilot && !demo) {
        Car &pc = cars[me];
        bool drive = st == S_PLAY;
        pc.steer = clampf(in.joy[0], -1, 1);
        pc.throttle = drive ? clampf(in.joy[1] + in.throttle - in.brake, -1, 1) : 0.f;
        bool rmb = false;
#ifdef _WIN32
        rmb = !in.vr && (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;   /* desktop: right mouse = turbo (Bobby 10-18) */
#endif
        pc.boost_req = drive && (in.top || in.b2 || rmb || (!in.vr && (in.buttons & VPIX_BTN_TRIGGER)));
    }
    while (acc >= dt && n < 24) {
        if (sim) {
            for (size_t i = 0; i < cars.size(); i++) {
                if (!net::following()) {   /* a linked slave drives every car on the master's controls */
                    if ((cars[i].cpu || (autopilot && cars[i].pod < 2)) && live) cpu_drive(cars[i], dt);
                    if (!live) { cars[i].throttle = 0; cars[i].boost_req = false; if (cars[i].cpu) cars[i].steer = 0; } }
                step_car(cars[i], (int)i, dt);
            }
            step_ball(dt); collide(dt);
            if (live && !net::following()) { check_goal(dt); if (st == S_PLAY) { time_left -= dt; level_left -= dt; } }
        }
        if (goal_banner > 0) goal_banner -= dt;
        if (level_banner > 0) level_banner -= dt;
        acc -= dt; sim_t += dt; st_t += dt; n++;
    }
    net::post_tick();
    if (n) {
        step_puffs(n * dt);
        build_frame(++frames, in);
        VVoice &v = voices[8];
        if (sim && !cars.empty() && !demo) {   /* engine note: pitch with speed */
            Car &pc = cars[me];
            float sp = fabsf(dot(pc.v, pc.R.r[1])), vmax = (v_base + v_per_top * pc.d->top) * lv.speed_f;
            if (!v.on) play(8, sound(pc.d->engine_snd), 70);
            v.bend = (int)((clampf(sp / vmax, 0, 1.5f) * 12.f + fabsf(pc.throttle) * 2.f - 3.f) * 32.f);
            v.vol = 55 + (int)(fabsf(pc.throttle) * 40.f);
        } else v.on = false;
    }
    if (sim_t >= next_log && sim && !cars.empty()) {
        next_log = sim_t + 1.0;
        Car &pc = cars[me];
        bl("t=%.0f st %d me p=(%.0f %.0f %.0f) v=%.0f up.z=%.2f wheels %d%d%d%d | ball p=(%.0f %.0f %.0f) v=%.0f | in joy %.2f %.2f trig %.2f %.2f",
           sim_t, (int)st, pc.p.x, pc.p.y, pc.p.z, len(pc.v), pc.R.r[2].z, pc.wh[0].contact, pc.wh[1].contact, pc.wh[2].contact, pc.wh[3].contact,
           ball.p.x, ball.p.y, ball.p.z, len(ball.v), in.joy[0], in.joy[1], in.throttle, in.brake);
    }
    PIC_AddEvent(tick, 2.0);
}

static void start(const char *logbase) {
    const char *e = getenv("VPIX_BUGGY_DIR");
    dir = e && *e ? e : "H:\\Games\\Virtuality\\_work\\buggy_extract\\BALL";
    if (logbase && *logbase) blog = fopen((std::string(logbase) + ".buggy").c_str(), "w");
    sel_level = getenv("VPIX_BUGGY_LEVEL") ? atoi(getenv("VPIX_BUGGY_LEVEL")) & 3 : 0;
    {   /* GAME LENGTH (operator menu 3:00 3:30 4:00 4:30 5:00); VPIX_BUGGY_GAME_LEN seconds overrides */
        static const float lens[5] = {180, 210, 240, 270, 300};
        for (auto &t : read_dbs("RUNFILES\\OPTIONS.DBS")) if (up(t[0]) == "OPTIONS" && t.size() > 3) { int k = atoi(t[3].c_str()); if (k >= 0 && k < 5) game_len = lens[k]; }
        if ((e = getenv("VPIX_BUGGY_GAME_LEN")) != NULL && atof(e) > 10) game_len = (float)atof(e);
    }
    if (getenv("VPIX_BUGGY_CPU")) ncpu_cfg = atoi(getenv("VPIX_BUGGY_CPU"));
    if (getenv("VPIX_BUGGY_G")) G = (float)atof(getenv("VPIX_BUGGY_G"));
    if ((e = getenv("VPIX_BUGGY_CAR_TUNE")) != NULL) sscanf(e, "%f,%f,%f,%f", &v_base, &a_base, &tyre_mu, &downforce);
    if ((e = getenv("VPIX_BUGGY_SPIN")) != NULL) spin_k = (float)atof(e);
    if ((e = getenv("VPIX_BUGGY_BALL")) != NULL) sscanf(e, "%f,%f,%f", &ball_g, &ball_floor_e, &ball_lift);
    autopilot = getenv("VPIX_BUGGY_AUTOPILOT") && atoi(getenv("VPIX_BUGGY_AUTOPILOT"));
    if (getenv("VPIX_BUGGY_COCKPIT") && atoi(getenv("VPIX_BUGGY_COCKPIT"))) outside_view = false;
    if ((e = getenv("VPIX_WORLD_SCALE")) != NULL && atof(e) > 0) world_scale = (float)atof(e);
    bl("Buggy Ball native rebuild, assets %s", dir.c_str());
    for (auto &t : read_dbs("RUNFILES\\BITMAP.DBS")) if (up(t[0]) == "BITMAP" && t.size() >= 3) bmp_file[atoi(t[1].c_str())] = t[2];
    setup_level(sel_level);
    eye_local = V(0, -650, 1050);
    if ((e = getenv("VPIX_BUGGY_EYE")) != NULL && (eye_forced = true)) sscanf(e, "%f,%f,%f", &eye_local.x, &eye_local.y, &eye_local.z);
    load_textures(); flic_scripts();
    use_model(lv.bowl_model); use_model(lv.ball_model); use_model(250); use_model(237);
    reset_ball();
    if (!snd_chan && sound_on) { snd_chan = MIXER_AddChannel(snd_mix, SND_RATE, "VPIXSND"); snd_chan->Enable(true); }
    net::init();
    running = true; t_last = std::chrono::steady_clock::now();
    PIC_AddEvent(tick, 2.0);
    bl("started: %zu bitmaps listed, title screen", bmp_file.size());
}

}   /* namespace buggy */
