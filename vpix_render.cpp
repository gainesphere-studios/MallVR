/*
 *  Virtuality SU2000 HLE — live renderer + SteamVR.
 *
 *  Own Win32 window + OpenGL context on its own thread, so it never touches DOSBox-X's
 *  video path. vpix_hle.cpp hands us meshes (parsed from MODELS\*.BIN as the game loads
 *  them) and, once per PIX_RenderSwap, a frame: camera + list of model instances.
 *
 *  With SteamVR running (VPIX_VR not 0) this thread also owns OpenVR: it waits on the
 *  compositor for poses (90 Hz), publishes headset + controller state for the HLE thread
 *  (which hands them to the game as InsideTrak records and joystick bits), renders each eye
 *  into its own FBO, submits both, and mirrors the left eye into the window.
 *
 *  PIX conventions (verified against the attract demo): world X right, Y forward, Z up.
 *  Matrices are stored as rows = local axes in world space:
 *      world = Rm^T (S * v) + Tm          camera = Rv (world - Tv)
 *
 *  Late head correction: the game builds its camera from the head pose we fed it (head_fed).
 *  Each eye is rendered as  R_eye = E * Hfed^T * Rcam,  T_eye = Tcam + Rcam^T Hfed (e - hfed)
 *  where E/e are the eye's current orientation/position, i.e. the game camera with the
 *  difference between "head now" and "head the game saw" applied in the body frame.
 *
 *  MallVR - Copyright (c) 2026 Gainesphere Studios (gainesphere.com).
 *  Part of a modified DOSBox-X, free software under the GNU GPL v2.
 */

#include "vpix_render.h"
#include "vpix_splash.h"

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include "openvr.h"
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "H:/Games/Virtuality/_work/openvr/lib/win64/openvr_api.lib")
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <math.h>
#include <mutex>
#include <thread>
#include <atomic>
#include <map>
#include <memory>
#include <algorithm>
#include <set>
#include <vector>
#include "../libs/xBRZ/xbrz.h"   /* VPIX_BLIT_UPSCALE */

namespace {

struct Poly {
    uint8_t n;
    uint32_t idx[16];
    uint32_t flags;          /* raw PIX polygon flags (type in top byte) */
    uint32_t vcol[16];       /* Gouraud: per-vertex 0x20RRGGBB */
    float nx, ny, nz;        /* face normal (model space) */
    uint32_t sel;            /* 3rd polygon word: select bits (Zone SCOREPOW.BIN: 0x1 'K', 0x2 '1', 0x4 '5', 0x8 '10', 0x10 'x', 0x20/40/80 '2/3/5') */
    int layer;               /* 0 = base, 1.. = coplanar decal drawn on top (VPIX_DECAL_STACK: decal on a decal = 2, ...) */
};
struct Mesh { std::vector<float> v; std::vector<Poly> p; int layers = 2; };

/* Per-game decal settings (games.ps1 DecalStack / DecalGap / DecalBias; defaults = the old behaviour):
   VPIX_DECAL_GAP   plane distance (units) under which a polygon lies ON another (default 0.5)
   VPIX_DECAL_STACK 1 = layers in the card's drawing order: it drew each presorted polygon list in order, so of
                    two overlapping polygons on one plane the later one is on top; a decal on a decal gets the
                    next layer (Boxing: canvas > white disc > logo letters; DN2 PLAT50: chequer tiles > corner
                    circles). The old rule (0) puts every smaller polygon of a plane on one decal layer, so
                    overlapping decals z-fought with each other.
   VPIX_DECAL_BIAS  > 0: each decal layer is drawn this fraction of its distance nearer the eye (projection
                    depth scaled by 1 + bias * layer; 2e-5 = 0.8 units at 40000, 0.02 at 1000) instead of the
                    old fixed polygon offset, which in a float depth buffer depends on each polygon's own
                    nearest depth and so did not order decal layers reliably. */
static float decal_gap() { static const float g = getenv("VPIX_DECAL_GAP") && atof(getenv("VPIX_DECAL_GAP")) > 0 ? (float)atof(getenv("VPIX_DECAL_GAP")) : 0.5f; return g; }
static bool decal_stack() { static const bool s = getenv("VPIX_DECAL_STACK") && atoi(getenv("VPIX_DECAL_STACK")); return s; }
static float decal_bias() { static const float b = getenv("VPIX_DECAL_BIAS") && atof(getenv("VPIX_DECAL_BIAS")) > 0 ? (float)atof(getenv("VPIX_DECAL_BIAS")) : 0.f; return b; }

static inline uint32_t be32(const uint8_t *b) { return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3]; }
static inline float bef(const uint8_t *b) { uint32_t u = be32(b); float f; memcpy(&f, &u, 4); return f; }

/* MODELS\*.BIN: "BIN 0200", u32 12, blocks [u32 len][u32 type][data]. Type 9 = one sub-model,
   0x10009 = 8 presorted sub-models (we take the first). Sub: u32 off_vert, off_norm, off_poly,
   off_bbox, size; each list's u32 count sits just before its data. */
/* Block type 0x2000D (GUN, SFL, MISS, GHOST, DAC's starfield): geometry header like type 9
   [off_vert, off_norm, -, off_bbox, size]; then a table tagged 00 02 00 0D (textured models keep
   an extra table before it) with 8 octant offsets; each octant list = tag + 4 offsets
   (vertex indices, normal indices, polygons, end). Polygons, 16-bit fields: u16 count, then
   u16 n, u32 flags, u32 0, u16 idx[n] (Gouraud: u16 idx, u32 colour, u16 normal per vertex).
   Face normals are computed from the vertices. */
static inline uint16_t be16(const uint8_t *b) { return (uint16_t)((b[0] << 8) | b[1]); }
static Mesh *parse_d(const uint8_t *b, size_t bsz) {
    if (bsz < 24) return NULL;
    uint32_t ov = be32(b), size = be32(b + 16);
    if (ov < 4 || ov > bsz || size > bsz) return NULL;
    uint32_t nv = be32(b + ov - 4);
    if (ov + 12ull * nv > bsz) return NULL;
    size_t tab = size;
    while (tab + 40 <= bsz && !(b[tab] == 0 && b[tab + 1] == 2 && b[tab + 2] == 0 && b[tab + 3] == 0x0D)) tab += 4;
    if (tab + 40 > bsz) return NULL;
    uint32_t o = 0;
    for (int k = 0; k < 8 && !o; k++) o = be32(b + tab + 4 + 4 * k);
    size_t L = tab + o;
    if (!o || L + 20 > bsz) return NULL;
    size_t q = L + be32(b + L + 12), end = L + be32(b + L + 16);
    if (end > bsz || q + 2 > end) return NULL;
    Mesh *m = new Mesh();
    for (uint32_t i = 0; i < nv * 3; i++) m->v.push_back(bef(b + ov + 4 * i));
    uint32_t np = be16(b + q); q += 2;
    for (uint32_t i = 0; i < np && q + 10 <= end; i++) {
        Poly pl; memset(&pl, 0, sizeof(pl));
        uint32_t n = be16(b + q); pl.flags = be32(b + q + 2); q += 10;
        if (n < 3 || n > 16) { delete m; return NULL; }
        pl.n = (uint8_t)n;
        uint32_t top = pl.flags >> 24;
        bool gouraud = (top == 0x40 || top == 0x48);
        for (uint32_t k = 0; k < n; k++) {
            if (gouraud) { pl.idx[k] = be16(b + q); pl.vcol[k] = be32(b + q + 2); q += 8; }
            else { pl.idx[k] = be16(b + q); q += 2; }
            if (pl.idx[k] >= nv) pl.idx[k] = 0;
        }
        const float *v0 = &m->v[3 * pl.idx[0]], *v1 = &m->v[3 * pl.idx[1]], *v2 = &m->v[3 * pl.idx[2]];
        float ux = v1[0] - v0[0], uy = v1[1] - v0[1], uz = v1[2] - v0[2], wx = v2[0] - v0[0], wy = v2[1] - v0[1], wz = v2[2] - v0[2];
        float nx = uy * wz - uz * wy, ny = uz * wx - ux * wz, nz = ux * wy - uy * wx, l = sqrtf(nx * nx + ny * ny + nz * nz);
        if (l > 0) { pl.nx = nx / l; pl.ny = ny / l; pl.nz = nz / l; }
        m->p.push_back(pl);
    }
    return m;
}

/* VPIX_DECAL_STACK: polygon i lies on an earlier polygon j when both face the same way (geometric normals),
   every corner of each is within gap of the other's plane, and their outlines really overlap (convex clip
   in the plane, not just touching edges); i then gets layer(j) + 1. */
static void stack_decals(Mesh *m, float gap) {
    size_t N = m->p.size();
    std::vector<float> nrm(3 * N), d(N), area(N), bb(6 * N);
    for (size_t i = 0; i < N; i++) {
        const Poly &a = m->p[i];
        float n[3] = {0, 0, 0};                                  /* Newell normal: also fine for slightly bent quads */
        for (int k = 0; k < a.n; k++) {
            const float *p = &m->v[3 * a.idx[k]], *q = &m->v[3 * a.idx[(k + 1) % a.n]];
            n[0] += (p[1] - q[1]) * (p[2] + q[2]); n[1] += (p[2] - q[2]) * (p[0] + q[0]); n[2] += (p[0] - q[0]) * (p[1] + q[1]);
        }
        float l = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        area[i] = 0.5f * l;
        for (int c = 0; c < 3; c++) nrm[3 * i + c] = l > 0 ? n[c] / l : 0;
        const float *v0 = &m->v[3 * a.idx[0]];
        d[i] = nrm[3 * i] * v0[0] + nrm[3 * i + 1] * v0[1] + nrm[3 * i + 2] * v0[2];
        for (int c = 0; c < 3; c++) { bb[6 * i + c] = 1e30f; bb[6 * i + 3 + c] = -1e30f; }
        for (int k = 0; k < a.n; k++)
            for (int c = 0; c < 3; c++) { float x = m->v[3 * a.idx[k] + c]; bb[6 * i + c] = (std::min)(bb[6 * i + c], x); bb[6 * i + 3 + c] = (std::max)(bb[6 * i + 3 + c], x); }
    }
    auto off_plane = [&](size_t i, size_t j) {               /* farthest corner of j from i's plane */
        float g = 0; const float *n = &nrm[3 * i];
        for (int k = 0; k < m->p[j].n; k++) { const float *v = &m->v[3 * m->p[j].idx[k]]; g = (std::max)(g, fabsf(n[0] * v[0] + n[1] * v[1] + n[2] * v[2] - d[i])); }
        return g;
    };
    int maxl = 0;
    for (size_t i = 0; i < N; i++) {
        Poly &a = m->p[i];
        if (area[i] <= 0) continue;
        const float *ni = &nrm[3 * i];
        int ax = fabsf(ni[0]) > fabsf(ni[1]) ? (fabsf(ni[0]) > fabsf(ni[2]) ? 0 : 2) : (fabsf(ni[1]) > fabsf(ni[2]) ? 1 : 2);
        int u = ax == 0 ? 1 : 0, w = ax == 2 ? 1 : 2;           /* in-plane axes */
        for (size_t j = 0; j < i; j++) {
            if (area[j] <= 0 || m->p[j].layer + 1 <= a.layer) continue;
            const float *nj = &nrm[3 * j];
            if (ni[0] * nj[0] + ni[1] * nj[1] + ni[2] * nj[2] < 0.999f) continue;
            bool sep = false;
            for (int c = 0; c < 3; c++) if (bb[6 * i + c] > bb[6 * j + 3 + c] + gap || bb[6 * j + c] > bb[6 * i + 3 + c] + gap) sep = true;
            if (sep || off_plane(i, j) >= gap || off_plane(j, i) >= gap) continue;
            /* clip i's outline by j's (both convex), in the plane */
            std::vector<float> P, Q;
            for (int k = 0; k < a.n; k++) { const float *v = &m->v[3 * a.idx[k]]; P.push_back(v[u]); P.push_back(v[w]); }
            const Poly &b = m->p[j];
            std::vector<float> B;
            for (int k = 0; k < b.n; k++) { const float *v = &m->v[3 * b.idx[k]]; B.push_back(v[u]); B.push_back(v[w]); }
            float sb = 0; int nb = b.n;
            for (int k = 0; k < nb; k++) sb += B[2 * k] * B[2 * ((k + 1) % nb) + 1] - B[2 * ((k + 1) % nb)] * B[2 * k + 1];
            for (int k = 0; k < nb && P.size() >= 6; k++) {
                int k0 = sb >= 0 ? k : nb - 1 - k, k1 = sb >= 0 ? (k + 1) % nb : (2 * nb - 2 - k) % nb;   /* walk j counter-clockwise */
                float ex = B[2 * k1] - B[2 * k0], ey = B[2 * k1 + 1] - B[2 * k0 + 1];
                Q.clear();
                size_t np = P.size() / 2;
                for (size_t e = 0; e < np; e++) {
                    float px = P[2 * e], py = P[2 * e + 1], qx = P[2 * ((e + 1) % np)], qy = P[2 * ((e + 1) % np) + 1];
                    float sp = ex * (py - B[2 * k0 + 1]) - ey * (px - B[2 * k0]), sq = ex * (qy - B[2 * k0 + 1]) - ey * (qx - B[2 * k0]);
                    if (sp >= 0) { Q.push_back(px); Q.push_back(py); }
                    if ((sp >= 0) != (sq >= 0)) { float t = sp / (sp - sq); Q.push_back(px + t * (qx - px)); Q.push_back(py + t * (qy - py)); }
                }
                P.swap(Q);
            }
            float ia = 0; size_t np = P.size() / 2;
            for (size_t e = 0; e < np; e++) ia += P[2 * e] * P[2 * ((e + 1) % np) + 1] - P[2 * ((e + 1) % np)] * P[2 * e + 1];
            ia = fabsf(ia) * 0.5f / (std::max)(fabsf(ni[ax]), 0.05f);   /* projected -> true area */
            if (np >= 3 && ia > 0.5f && ia > 1e-3f * (std::min)(area[i], area[j])) a.layer = (std::min)(m->p[j].layer + 1, 63);   /* 63: Zone's 10K bonus box stacks 27 strokes + a black cut (15 squashed them) */
        }
        maxl = (std::max)(maxl, a.layer);
    }
    m->layers = maxl + 1;
}

static Mesh *parse_bin(const uint8_t *d, size_t len) {
    if (len < 20 || (memcmp(d, "BIN 0200", 8) != 0 && memcmp(d, "BIN 0300", 8) != 0)) return NULL;   /* 0300 (PAC): extra type-0xB header block */
    size_t p = 12;
    while (p + 8 <= len) {
        uint32_t blen = be32(d + p), type = be32(d + p + 4);
        const uint8_t *blob = d + p + 8; size_t bsz = blen >= 4 ? blen - 4 : 0;
        p += 4 + blen;
        if (p > len) continue;
        Mesh *m = NULL;
        if (type == 0x2000D) m = parse_d(blob, bsz);
        if (type == 0x2000D) { if (!m) return NULL; }
        else {
        uint32_t bt = type & 0xFFFF;                     /* 9 and 8: 5-word sub-header, 6 and 7: 4 words (no bbox); */
        if (bt < 6 || bt > 9) continue;                  /* we only use the first three offsets either way           */
        /* type 0x1000x: 8 sub-models, one per view octant, each holding only the polygons that face
           that way (the card's back-face removal), each with its own vertex list. We draw with the
           depth buffer instead, so merge all 8, dropping polygons already seen (same corner points). */
        int nsub = (type >> 16) == 1 ? 8 : 1;
        m = new Mesh();
        std::set<std::vector<int32_t>> seen;
        for (int sub = 0; sub < nsub; sub++) {
        uint32_t so = (type >> 16) == 1 ? be32(blob + 4 * sub) : 0;
        if (so >= bsz) continue;
        const uint8_t *s = blob + so;
        size_t room = bsz - so;
        uint32_t ov = be32(s), on = be32(s + 4), op = be32(s + 8);
        if (ov < 4 || on < 4 || op < 4 || op > room) { if (sub) continue; delete m; return NULL; }
        uint32_t nv = be32(s + ov - 4), base = (uint32_t)(m->v.size() / 3);
        if (ov + 12ull * nv > room) continue;
        for (uint32_t i = 0; i < nv * 3; i++) m->v.push_back(bef(s + ov + 4 * i));
        uint32_t nn = be32(s + on - 4);
        uint32_t np = be32(s + op - 4);
        size_t q = op;
        for (uint32_t i = 0; i < np && q + 12 <= room; i++) {
            Poly pl; memset(&pl, 0, sizeof(pl));
            uint32_t n = be32(s + q); pl.flags = be32(s + q + 4); pl.sel = be32(s + q + 8);
            if (n < 3 || n > 4) { if (sub) break; delete m; return NULL; }
            pl.n = (uint8_t)n;
            uint32_t top = pl.flags >> 24;
            bool gouraud = (top == 0x40 || top == 0x48);
            /* 0x62 (PEARL models\temple\pillar.bin): textured + smooth-shaded, per vertex (index, normal index) */
            bool tex_smooth = (top == 0x62 || top == 0x6A);
            for (uint32_t k = 0; k < n; k++) {
                if (gouraud) { pl.idx[k] = be32(s + q + 12 + 12 * k); pl.vcol[k] = be32(s + q + 16 + 12 * k); }
                else if (tex_smooth) pl.idx[k] = be32(s + q + 12 + 8 * k);
                else pl.idx[k] = be32(s + q + 12 + 4 * k);
                if (pl.idx[k] >= nv) pl.idx[k] = 0;
                pl.idx[k] += base;
            }
            q += 12 + (gouraud ? 12 : tex_smooth ? 8 : 4) * n;
            if (i < nn) { pl.nx = bef(s + on + 12 * i); pl.ny = bef(s + on + 12 * i + 4); pl.nz = bef(s + on + 12 * i + 8); }
            /* key: sorted corner coordinates (rounded) + flags */
            std::vector<int32_t> key;
            std::vector<std::vector<int32_t>> cs;
            for (uint32_t k = 0; k < n; k++) {
                const float *v = &m->v[3 * pl.idx[k]];
                cs.push_back({(int32_t)lroundf(v[0] * 8), (int32_t)lroundf(v[1] * 8), (int32_t)lroundf(v[2] * 8)});
            }
            std::sort(cs.begin(), cs.end());
            for (auto &c : cs) key.insert(key.end(), c.begin(), c.end());
            key.push_back((int32_t)pl.flags);
            if (!seen.insert(key).second) continue;
            m->p.push_back(pl);
        }
        }
        }
        /* coplanar decals (chequer squares on a platform): biggest polygon on a plane is the
           base, the rest are drawn with a depth offset so they never z-fight with it */
        std::vector<float> area(m->p.size()), dist(m->p.size());
        for (size_t i = 0; i < m->p.size(); i++) {
            Poly &a = m->p[i]; const float *v0 = &m->v[3 * a.idx[0]];
            float ar = 0;
            for (int k = 1; k + 1 < a.n; k++) {
                const float *v1 = &m->v[3 * a.idx[k]], *v2 = &m->v[3 * a.idx[k + 1]];
                float ux = v1[0] - v0[0], uy = v1[1] - v0[1], uz = v1[2] - v0[2], wx = v2[0] - v0[0], wy = v2[1] - v0[1], wz = v2[2] - v0[2];
                float cx = uy * wz - uz * wy, cy = uz * wx - ux * wz, cz = ux * wy - uy * wx;
                ar += 0.5f * sqrtf(cx * cx + cy * cy + cz * cz);
            }
            area[i] = ar; dist[i] = a.nx * v0[0] + a.ny * v0[1] + a.nz * v0[2];
        }
        if (decal_stack()) { stack_decals(m, decal_gap()); return m; }
        for (size_t i = 0; i < m->p.size(); i++)
            for (size_t j = 0; j < m->p.size(); j++) {
                if (i == j || area[j] <= area[i]) continue;
                Poly &a = m->p[i], &b = m->p[j];
                if (a.nx * b.nx + a.ny * b.ny + a.nz * b.nz > 0.999f && fabsf(dist[i] - dist[j]) < decal_gap()) { a.layer = 1; break; }
            }
        return m;
    }
    return NULL;
}

/* .TEX: "TEX 0103", u32 12, block [u32 len][u32 type][u32 -1][w][h][0x14][..][..][..], then texels.
   w/h are either log2 (<16) or the size itself. Texels: 16-bit big-endian 0x0RGB (4:4:4), or
   32-bit 0x00RRGGBB when the block holds 4 bytes per texel. Row 0 = v 0. */
struct Tex { int w = 0, h = 0; std::vector<uint32_t> rgba; unsigned gl = 0; bool b16 = false, mono = false, clear0 = false;
             std::vector<uint16_t> raw; std::map<int, unsigned> keyed; };   /* bitmaps: raw 0BGR + GL texture per colour key;
                                                                               textures: GL texture per mask colour */
struct TexSlot { uint32_t handle = 0, flags = 0, tflags = 0, mask = 0; float o[3] = {0, 0, 0}, su = 0, sv = 0; bool set = false; };
static Tex *parse_tex(const uint8_t *d, size_t len) {
    if (len < 48 || memcmp(d, "TEX 01", 6) != 0) return NULL;   /* 0101 (BOX) and 0103 share the layout */
    uint32_t a = be32(d + 24), b = be32(d + 28);
    int w = a < 16 ? 1 << a : (int)a, h = b < 16 ? 1 << b : (int)b;
    if (w <= 0 || h <= 0 || w > 1024 || h > 1024) return NULL;
    size_t n = (size_t)w * h, avail = len - 48;
    Tex *t = new Tex(); t->w = w; t->h = h; t->rgba.resize(n);
    const uint8_t *px = d + 48;
    if (avail >= n * 4) {                     /* 00 BB GG RR -> GL RGBA bytes */
        for (size_t i = 0; i < n; i++) t->rgba[i] = 0xFF000000u | ((uint32_t)px[4 * i + 1] << 16) | ((uint32_t)px[4 * i + 2] << 8) | px[4 * i + 3];
    } else if (avail >= n * 2) {
        t->b16 = true;
        for (size_t i = 0; i < n; i++) {
            uint32_t v = ((uint32_t)px[2 * i] << 8) | px[2 * i + 1];
            uint32_t bl = (v >> 8) & 15, g = (v >> 4) & 15, r = v & 15;
            t->rgba[i] = 0xFF000000u | (bl * 17u << 16) | (g * 17u << 8) | (r * 17u);   /* nibbles are 0BGR; GL wants R in the low byte */
        }
    } else { delete t; return NULL; }
    return t;
}

/* .VST = TGA type 2, 16 bpp, pixels big-endian 0x0BGR (like .TEX), rows bottom-up unless
   descriptor bit 5. Black is the transparent key for overlays. */
static Tex *parse_vst(const uint8_t *d, size_t len) {
    if (len < 18 || d[2] != 2 || (d[16] != 16 && d[16] != 1 && d[16] != 32 && d[16] != 4)) return NULL;
    int w = d[12] | (d[13] << 8), h = d[14] | (d[15] << 8);
    size_t off = 18 + d[0] + (d[1] ? (size_t)(d[5] | (d[6] << 8)) * ((d[7] + 7) / 8) : 0);
    /* Shoot For Loot's redarrow.16b (the compass, 256 x 128 x 16 bit) says 11 bytes of image ID follow the header, but the file
       is exactly 18 + 65536 bytes: the pixels start at 18. When the ID block doesn't fit, it isn't there (it failed to load). */
    if (d[16] == 16 && !d[1] && off + (size_t)w * h * 2 > len && 18 + (size_t)w * h * 2 <= len) off = 18;
    bool top = (d[17] & 0x20) != 0;
    if (d[16] == 1) {                                    /* 1 bpp glyphs (PEARL digits): rows padded to bytes, MSB first */
        size_t stride = ((size_t)w + 7) / 8;
        if (w <= 0 || h <= 0 || off + stride * h > len) return NULL;
        Tex *t = new Tex(); t->w = w; t->h = h; t->raw.resize((size_t)w * h); t->mono = true;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                int row = top ? y : h - 1 - y;           /* 1 bpp files are top-down whatever the descriptor says */
                t->raw[(size_t)row * w + x] = (d[off + stride * y + x / 8] >> (7 - x % 8)) & 1 ? 0xFFF : 0;
            }
        return t;
    }
    if (d[16] == 4) {                                    /* 2 bits per pixel, 4 shades (PEARL's pearl-counter strips newblits\egg1-16.vst: 28 px wide, up to 16 pearls
                                                            stacked): rows padded to bytes, MSB first, top-down like the 1 bpp files. 0 = clear, 1-3 = grey to white */
        size_t stride = ((size_t)w + 3) / 4;
        if (w <= 0 || h <= 0 || off + stride * h > len) return NULL;
        Tex *t = new Tex(); t->w = w; t->h = h; t->raw.resize((size_t)w * h); t->mono = true; t->clear0 = true;   /* shade 0 is see-through */
        static const uint16_t shade[4] = {0x000, 0x888, 0xCCC, 0xFFF};
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                t->raw[(size_t)(h - 1 - y) * w + x] = shade[(d[off + stride * y + x / 4] >> (6 - 2 * (x % 4))) & 3];   /* raw row 0 = bottom */
        return t;
    }
    if (d[16] == 32) {                                   /* 32 bpp (ZONE): standard TGA B,G,R,A -> 12-bit 0BGR */
        if (w <= 0 || h <= 0 || off + (size_t)w * h * 4 > len) return NULL;
        Tex *t = new Tex(); t->w = w; t->h = h; t->raw.resize((size_t)w * h); t->mono = true;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                const uint8_t *q = d + off + 4 * ((size_t)y * w + x);
                int row = top ? h - 1 - y : y;
                t->raw[(size_t)row * w + x] = (uint16_t)(((q[0] >> 4) << 8) | ((q[1] >> 4) << 4) | (q[2] >> 4));
            }
        return t;
    }
    if (w <= 0 || h <= 0 || off + (size_t)w * h * 2 > len) return NULL;
    Tex *t = new Tex(); t->w = w; t->h = h; t->raw.resize((size_t)w * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const uint8_t *q = d + off + 2 * ((size_t)y * w + x);
            int row = top ? h - 1 - y : y;                     /* store bottom-up: row 0 = bottom */
            t->raw[(size_t)row * w + x] = (uint16_t)((((uint32_t)q[0] << 8) | q[1]) & 0xFFF);
        }
    return t;
}

/* GL texture of a bitmap with one colour keyed out (key -1 = fully opaque) */
static unsigned bitmap_gl(Tex *t, int key) {
    auto it = t->keyed.find(key);
    if (it != t->keyed.end()) return it->second;
    std::vector<uint32_t> px(t->raw.size());
    for (size_t i = 0; i < px.size(); i++) {
        uint32_t v = t->raw[i], bl = (v >> 8) & 15, g = (v >> 4) & 15, r = v & 15;
        px[i] = ((int)v == key || (t->clear0 && v == 0) ? 0u : 0xFF000000u) | (bl * 17u << 16) | (g * 17u << 8) | (r * 17u);
    }
    /* VPIX_BLIT_UPSCALE=n (games.ps1 BlitUpscale, 2-6): the screen bitmaps (FOCUS VISETTE picture, HUD text) are native
       352x288 art shown across the whole view; xBRZ (DOSBox-X's own pixel-art scaler, src/libs/xBRZ) scales them n times
       with smooth edges, keyed pixels as alpha, then drawn with linear filtering (Bobby 10-17 'can the text be sharper') */
    static const int up = getenv("VPIX_BLIT_UPSCALE") ? (std::max)(0, (std::min)(6, atoi(getenv("VPIX_BLIT_UPSCALE")))) : 0;
    int n = up >= 2 && t->w * up <= 4096 && t->h * up <= 4096 ? up : 1;
    if (n > 1) {
        auto swap_rb = [](uint32_t c) { return (c & 0xFF00FF00u) | ((c & 0xFF) << 16) | ((c >> 16) & 0xFF); };   /* GL RGBA bytes <-> xBRZ ARGB */
        std::vector<uint32_t> src(px.size()), big((size_t)t->w * n * t->h * n);
        for (size_t i = 0; i < px.size(); i++) src[i] = swap_rb(px[i]);
        xbrz::scale((size_t)n, src.data(), big.data(), t->w, t->h, xbrz::ColorFormat::ARGB);
        for (auto &c : big) c = swap_rb(c);
        px.swap(big);
    }
    GLuint id; glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
    const GLint filt = n > 1 ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filt); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filt);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w * n, t->h * n, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    t->keyed[key] = id;
    return id;
}

std::mutex mtx;
std::map<uint32_t, std::shared_ptr<Mesh>> meshes;
std::map<uint32_t, std::shared_ptr<Tex>> bitmaps;
float screen_w = 352.f, screen_h = 288.f;          /* PIX 2D screen space */
static float clip_h = 0;                            /* VPIX_SCREEN_CLIP: visible bottom of the 2D layer, if above screen_h */
float screen_aspect = 4.f / 3.f;                   /* displayed width / height */
std::vector<uint32_t> flic_px; int flic_w = 0, flic_h = 0, flic_x = 0, flic_y = 0; bool flic_on = false, flic_dirty = false;
unsigned flic_gl = 0;
std::map<uint32_t, std::shared_ptr<Tex>> textures;
TexSlot texslot[0x400];
/* PIXMATERIAL texture numbers (VPIX_MATERIALS games only, else all -1): type 0x68 polygons' low bits
   pick a material, whose +0xC holds the texture number (Pac-Man's frightened ghost BLUE.BIN = material 1
   -> texture 0xA, BLUEA; read as texture 1 it drew the unassigned DEFCHECK checker as a box) */
static int mattex[32] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
/* material opacity (PIXMATERIAL type 1, u16 @+4 / 255); < 1 = type 0x68 polygons drawn blended in a second pass */
static float matalpha[32] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
/* PEARL: material colours for type 0x28 polygons (VPIX_Render_SetMaterialColour; mat28 on once any is set) */
static uint32_t matcol[32]; static bool mat28 = false;
VPIX_Frame pending; bool have_pending = false;
std::atomic<bool> running(false);
std::thread th;
int win_w = 960, win_h = 720;

std::mutex in_mtx;
VPIX_Input input;                         /* latest controller/headset state */
FILE *rlog = NULL;

static void rl(const char *fmt, ...) {
    if (!rlog) return;
    va_list ap; va_start(ap, fmt); vfprintf(rlog, fmt, ap); va_end(ap);
    fputc('\n', rlog); fflush(rlog);
}

static void mat_mul3(const float a[3][3], const float b[3][3], float o[3][3]) {
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) o[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j];
}
static void mat_tr3(const float a[3][3], float o[3][3]) { for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) o[i][j] = a[j][i]; }

/* The game camera for one eye, after late head correction.
   In play (game camera = the player's head): the game's camera direction is the fed head
   direction turned by a body yaw (joystick/teleport turns; it also drops head roll). We
   recover that yaw, then render the eye's *current* full orientation turned by it, at the
   camera position plus the eye's offset from the fed head (world units = mm * scale).
   Anywhere else (attract, cut-scenes): the headset is free look on top of the game camera. */
static float world_scale = 1.85f;
static float yaw_of(const float fw[3]) { return atan2f(-fw[0], fw[1]); }
static void eye_camera(const VPIX_Frame &f, const VPIX_Pose &eye, const VPIX_Pose &head_now, float R[3][3], float T[3]) {
    memcpy(R, f.cam_R, sizeof(f.cam_R)); memcpy(T, f.cam_T, sizeof(f.cam_T));
    if (!eye.valid) return;
    const VPIX_Pose &hf = f.head_fed;
    bool follows = f.hold || (f.in_head && hf.valid && fabsf(f.cam_R[1][2] - hf.R[1][2]) < 0.03f);
    /* VPIX_FOLLOW_LATCH (games.ps1 FollowLatch, Zone Hunter): the game keeps InHead = 1 through its attract loop, whose cameras
       are level, so with the head level the pitch test passed by chance and the head's left/right turns were cancelled (Bobby
       10-17: 'in the attract mode you can't look right or left'). Decide only while the head clearly looks up or down (pitch
       sin > 0.12), where a level camera can't match by chance; while level, keep the last decision. */
    static const bool latch = getenv("VPIX_FOLLOW_LATCH") && atoi(getenv("VPIX_FOLLOW_LATCH"));
    if (latch && !f.hold && f.in_head && hf.valid) {
        static bool ok = true; static unsigned long fr = ~0ul;
        if (f.frame != fr) {
            fr = f.frame;
            if (!follows) { if (ok) rl("follow latch OFF f=%lu (game camera ignores the head)", f.frame); ok = false; }
            else if (fabsf(hf.R[1][2]) > 0.12f) { if (!ok) rl("follow latch ON f=%lu (game camera follows the head)", f.frame); ok = true; }
        }
        follows = follows && ok;
    }
    /* VPIX_FOLLOW_PITCH (games.ps1 FollowPitch, Missile Command): the game camera is the fed head pitched a constant amount up
       (AIM log 10-17: camera = head + 42.0 deg every frame: our saved aim trim 12 + the game's own ~30), so the pitch test failed,
       the view fell to free look and the head pitch was added a second time on top of the already pitched camera: level matched,
       but looking up 20 put the view 40 up while the shots went 20 (Bobby: 'the gun is not shooting the center of the screen').
       Follow the head with that offset once it has held (within 0.5 deg) over 10 game frames in which the head pitch moved; then the view centre is the
       game's aim, where the sight and the shots are. */
    float follow_po = 0;
    static const bool fpitch = getenv("VPIX_FOLLOW_PITCH") && atoi(getenv("VPIX_FOLLOW_PITCH"));
    if (fpitch && !follows && !f.hold && f.in_head && hf.valid) {
        static float last_po = 1e9f, last_hp = 1e9f; static int steady = 0; static unsigned long pfr = ~0ul; static bool on = false;
        float hp = asinf(hf.R[1][2]), po = asinf(f.cam_R[1][2]) - hp;
        if (f.frame != pfr) {
            pfr = f.frame;
            /* steady = the camera pitch moved WITH the head: only frames where the head pitch changed count (a still head over a
               still menu camera also gives a constant difference: test 10-17 switched on in attract and glued the view to the head) */
            if (fabsf(po - last_po) >= 0.0087f) steady = 0;
            else if (fabsf(hp - last_hp) > 0.004f && steady < 1000) steady++;
            last_po = po; last_hp = hp;
            bool now = steady >= 10;
            if (now != on) { rl("follow pitch %s f=%lu (camera = head %+.1f deg)", now ? "ON" : "OFF", f.frame, po * 57.29578f); on = now; }
        }
        if (on) { follows = true; follow_po = po; }
    }
    {   /* dev VPIX_AIMLOG (Missile Command sight vs shots, Bobby 10-17): game camera pitch vs the fed and live head pitch */
        static const bool al = getenv("VPIX_AIMLOG") && atoi(getenv("VPIX_AIMLOG"));
        static unsigned long fr = ~0ul;
        if (al && f.frame != fr && f.frame % 30 == 0) { fr = f.frame;
            const float d2r = 57.29578f;
            rl("AIM f=%lu in_head %d follows %d cam pitch %.1f yaw %.1f | fed head pitch %.1f yaw %.1f | live eye pitch %.1f", f.frame, f.in_head ? 1 : 0, follows ? 1 : 0,
               asinf(f.cam_R[1][2]) * d2r, yaw_of(f.cam_R[1]) * d2r, hf.valid ? asinf(hf.R[1][2]) * d2r : 0.f, hf.valid ? yaw_of(hf.R[1]) * d2r : 0.f, asinf(eye.R[1][2]) * d2r); }
    }
    if (follows) {
        float b = yaw_of(f.cam_R[1]) - yaw_of(hf.R[1]), c = cosf(b), s = sinf(b);
        for (int k = 0; k < 3; k++) { R[k][0] = c * eye.R[k][0] - s * eye.R[k][1]; R[k][1] = s * eye.R[k][0] + c * eye.R[k][1]; R[k][2] = eye.R[k][2]; }
        if (follow_po != 0) {                          /* VPIX_FOLLOW_PITCH: camera = the fed head pitched up po about its own right axis */
            /* exact (Bobby 10-17: 'great up and down, far left and right it gets off a bit'): the yaw-then-pitch split above drifts
               once the head rolls a little. W = the world rotation taking the pitched fed head onto the game camera; the view is
               W applied to the pitched live eye. */
            float cp = cosf(follow_po), sp = sinf(follow_po), ph[3][3], pe[3][3], W[3][3];
            for (int k = 0; k < 3; k++) {
                ph[0][k] = hf.R[0][k]; ph[1][k] = hf.R[1][k] * cp + hf.R[2][k] * sp; ph[2][k] = hf.R[2][k] * cp - hf.R[1][k] * sp;
                pe[0][k] = eye.R[0][k]; pe[1][k] = eye.R[1][k] * cp + eye.R[2][k] * sp; pe[2][k] = eye.R[2][k] * cp - eye.R[1][k] * sp;
            }
            for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) W[i][j] = f.cam_R[0][i] * ph[0][j] + f.cam_R[1][i] * ph[1][j] + f.cam_R[2][i] * ph[2][j];
            for (int k = 0; k < 3; k++) for (int i = 0; i < 3; i++) R[k][i] = W[i][0] * pe[k][0] + W[i][1] * pe[k][1] + W[i][2] * pe[k][2];
        }
        /* crouch button (VPIX_CROUCH): head_fed was fed f.crouch_mm lower; lower the eye the same, so the
           view is exactly what a physical squat that deep gives (and a real squat on top just adds) */
        const float ez = eye.p[2] - f.crouch_mm;
        float d[3] = {eye.p[0] - hf.p[0], eye.p[1] - hf.p[1], ez - hf.p[2]};
        /* the game camera ignores head height (except a crouch state below player_height-200):
           add the head's height above the calibrated standing height while standing */
        static float auto_ref = 0;                     /* games without player_height: first standing height seen */
        if (f.player_height <= 500 && auto_ref == 0 && hf.p[2] + f.crouch_mm > 500) auto_ref = hf.p[2] + f.crouch_mm;
        float ref_h = f.player_height > 500 ? f.player_height : auto_ref;
        /* not with VPIX_FOLLOW_PITCH: Missile Command's eye sits at the game camera (as in free look before); the first-frame
           height reference put Bobby under the gun barrel (10-17: 'we are still too low and under the gun barrel view') */
        if (ref_h > 500 && hf.p[2] > ref_h - 200 && follow_po == 0) d[2] = ez - ref_h;
        if (follow_po != 0) {
            static unsigned long nz = 0;
            if (nz++ % 600 == 0) rl("follow pitch eye: head z %.0f, ref %.0f, eye offset z %.0f mm, cam z %.0f", hf.p[2], ref_h, d[2], f.cam_T[2]);
        }
        T[0] = f.cam_T[0] + world_scale * (c * d[0] - s * d[1]);
        T[1] = f.cam_T[1] + world_scale * (s * d[0] + c * d[1]);
        T[2] = f.cam_T[2] + world_scale * d[2];
        return;
    }
    /* free look: eye orientation relative to the player's reference yaw, on top of the camera */
    float a = -((f.in_head && hf.valid) ? yaw_of(hf.R[1]) : 0.f), c = cosf(a), s = sinf(a);
    float N[3][3];
    for (int k = 0; k < 3; k++) { N[k][0] = c * eye.R[k][0] - s * eye.R[k][1]; N[k][1] = s * eye.R[k][0] + c * eye.R[k][1]; N[k][2] = eye.R[k][2]; }
    for (int k = 0; k < 3; k++) for (int i = 0; i < 3; i++) R[k][i] = f.cam_R[0][i] * N[k][0] + f.cam_R[1][i] * N[k][1] + f.cam_R[2][i] * N[k][2];
    float d[3] = {eye.p[0] - head_now.p[0], eye.p[1] - head_now.p[1], eye.p[2] - head_now.p[2]};
    float dn[3] = {c * d[0] - s * d[1], s * d[0] + c * d[1], d[2]};
    for (int i = 0; i < 3; i++) T[i] += world_scale * (f.cam_R[0][i] * dn[0] + f.cam_R[1][i] * dn[1] + f.cam_R[2][i] * dn[2]);
}

/* GL texture of a .TEX with its mask colour see-through (PIXTEXTURE type bit 0x100000, maskcolor
   PIXCOL = 0x00BBGGRR like our texels). Masked texels take their neighbours' colour so linear
   filtering doesn't fringe the edges with the key colour; drawn with an alpha test. */
static unsigned masked_gl(Tex *t, uint32_t mask) {
    mask &= 0xFFFFFF;
    auto it = t->keyed.find((int)mask);
    if (it != t->keyed.end()) return it->second;
    const uint32_t cmp = t->b16 ? 0xF0F0F0u : 0xFFFFFFu;     /* 16-bit texels only hold the top nibble */
    std::vector<uint32_t> px(t->rgba);
    for (uint32_t &v : px) if ((v & cmp) == (mask & cmp)) v = 0;
    for (int pass = 0; pass < 4; pass++) {                   /* bleed opaque colours into the key */
        std::vector<uint32_t> nx(px);
        for (int y = 0; y < t->h; y++)
            for (int x = 0; x < t->w; x++) {
                if (px[(size_t)y * t->w + x] >> 24) continue;
                unsigned r = 0, g = 0, b = 0, k = 0;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int xx = (x + dx + t->w) % t->w, yy = (y + dy + t->h) % t->h;
                        uint32_t q = px[(size_t)yy * t->w + xx];
                        if (!(q >> 24) && !(q & 0xFFFFFF)) continue;
                        r += q & 255; g += (q >> 8) & 255; b += (q >> 16) & 255; k++;
                    }
                if (k) nx[(size_t)y * t->w + x] = ((b / k) << 16) | ((g / k) << 8) | (r / k);
            }
        px.swap(nx);
    }
    GLuint id; glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    t->keyed[(int)mask] = id;
    return id;
}

static void draw_blits(const VPIX_Frame &f, const float proj[16], bool backdrop = false, bool mid = false);
static void draw_sprites(const VPIX_Frame &f, const float camR[3][3], const float camT[3], int which = 0);
/* PEARL mid bitmaps (see draw_scene): the highest priority among them, false if none */
static bool mid_split(const VPIX_Frame &f, int32_t &sp) {
    bool any = false; sp = INT32_MIN;
    for (const VPIX_Blit &b : f.blits) if (b.mid) { any = true; if (b.prio > sp) sp = b.prio; }
    return any;
}
static bool rev_z = false;                 /* reversed-Z float depth active (VPIX_DEPTH_FIX + glClipControl, see frustum) */
static float z_near = 20.f;                /* near plane, world units */
static bool zf_rev = false;                /* VPIX_ZFIGHT probe (dev): models + polygons in reverse order */
/* Draw the scene. proj = column-major GL projection; cam R/T in PIX world. */
static void draw_scene(const VPIX_Frame &f, const float proj[16], const float camR[3][3], const float camT[3], int vw, int vh) {
    glViewport(0, 0, vw, vh);
    glClearColor(f.clear[0] / 255.f, f.clear[1] / 255.f, f.clear[2] / 255.f, 1);
    glClearDepth(rev_z ? 0.0 : 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (const VPIX_Blit &b : f.blits) if (b.backdrop) { draw_blits(f, proj, true); break; }   /* VPIX_BLIT_PRIORITY backdrops */
    glEnable(GL_DEPTH_TEST); glDepthFunc(rev_z ? GL_GEQUAL : GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glMatrixMode(GL_PROJECTION); glLoadMatrixf(proj);
    glMatrixMode(GL_MODELVIEW);

    /* PIX camera space (X right, Y fwd, Z up) -> GL eye (x right, y up, -z fwd) */
    static const float C[3][3] = {{1, 0, 0}, {0, 0, 1}, {0, -1, 0}};
    float CRv[3][3]; mat_mul3(C, camR, CRv);
    const float L[3] = {-0.35f, 0.75f, 0.55f};          /* eye-space light, from above-left */

    /* VPIX_PRIO_LAYERS (games.ps1 PrioLayers, Olin Shotgun): the card drew by the slot +0x54 priority (higher = front);
       the HLE sorts the models by it and each new priority is a new layer (depth cleared), so back panels / the big-top
       dome (low prio) can't hide targets and the sight + rifle (300+) stay on top. */
    static const bool prio_layers = getenv("VPIX_PRIO_LAYERS") && atoi(getenv("VPIX_PRIO_LAYERS"));
    bool first_inst = true; int32_t last_prio = 0;
    bool any_trans = false;
    for (int i = 0; i < 32; i++) if (matalpha[i] < 1.f) any_trans = true;
    /* mid bitmaps (HLE: a 3D model has a higher priority, PEARL): models up to the highest mid-bitmap priority first,
       then those bitmaps, depth cleared, then the models in front of them (PEARL's attract 'sell' messages 360 / 3D SOUND /
       VIRTUAL / FUTURE, prio 3000, fly over the black fade screen, prio 2100) */
    int32_t split_prio; const bool split = mid_split(f, split_prio);
    for (int phase = 0; phase < (split ? 2 : 1); phase++) {
    if (phase == 1) {
        draw_sprites(f, camR, camT, 1);                  /* world sprites (fish, pearls) at or below the split go under the fade too */
        static const bool middbg = getenv("VPIX_MIDDBG") != NULL;   /* dev: leave the mid bitmaps out */
        if (!middbg) draw_blits(f, proj, false, true);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); glDepthFunc(rev_z ? GL_GEQUAL : GL_LEQUAL);
        glMatrixMode(GL_PROJECTION); glLoadMatrixf(proj);
        glMatrixMode(GL_MODELVIEW);
    }
    /* VPIX_ON_TOP models (games.ps1 OnTop, Missile Command skip.bin): drawn after everything else with the depth test off. The card
       had no depth buffer and painted the 'skip intro' sign over the gun housing round the eye (guns1.bin, 58 units away); with a
       depth test the barrels hid it whenever you looked at it (Bobby 10-17). */
    bool any_top = false; for (const auto &in : f.inst) if (in.on_top) { any_top = true; break; }
    GLint depth_fn = GL_LEQUAL; if (any_top) glGetIntegerv(GL_DEPTH_FUNC, &depth_fn);
    for (int pass = 0; pass < (any_trans ? 2 : 1); pass++)      /* pass 1: translucent material polygons, blended, no depth write */
    for (size_t jj = 0; jj < f.inst.size() * (any_top ? 2 : 1); jj++) {
        const size_t ii = jj % f.inst.size(); const bool top_pass = jj >= f.inst.size();
        if (any_top && ii == 0) glDepthFunc(top_pass ? GL_ALWAYS : depth_fn);
        const auto &inst = f.inst[zf_rev && !prio_layers ? f.inst.size() - 1 - ii : ii];
        if ((inst.on_top != 0) != top_pass) continue;
        if (split && (inst.prio > split_prio) != (phase == 1)) continue;
        auto it = meshes.find(inst.handle);
        if (it == meshes.end()) continue;
        const Mesh &m = *it->second;
        if (prio_layers) { if (!first_inst && inst.prio != last_prio) glClear(GL_DEPTH_BUFFER_BIT); first_inst = false; last_prio = inst.prio; }
        float RmT[3][3]; mat_tr3(inst.R, RmT);
        /* head-locked (PEARL sell messages): placed by the game relative to its camera (straight ahead, 3500 units) for a player who
           faced one way; drawn relative to the game camera instead of the eye, so they stay in front wherever the head looks */
        float CRh[3][3]; if (inst.headlock) mat_mul3(C, f.cam_R, CRh);
        const float (*CRu)[3] = inst.headlock ? CRh : CRv; const float *Tu = inst.headlock ? f.cam_T : camT;
        float A[3][3]; mat_mul3(CRu, RmT, A);           /* eye = A (S v) + b */
        float t[3] = {inst.T[0] - Tu[0], inst.T[1] - Tu[1], inst.T[2] - Tu[2]};
        float b[3]; for (int i = 0; i < 3; i++) b[i] = CRu[i][0] * t[0] + CRu[i][1] * t[1] + CRu[i][2] * t[2];
        { static const bool middbg2 = getenv("VPIX_MIDDBG") != NULL; static unsigned long nd = 0;   /* dev: where the high-priority models land in eye space */
          if (middbg2 && inst.prio >= 3000 && pass == 0 && nd++ % 60 == 0) rl("MIDDBG h=%u prio=%d phase=%d split=%d eye=(%.0f %.0f %.0f) polys=%u layers=%d hold=%d in_head=%d hf=%d camfwd=(%.2f %.2f %.2f) gamefwd=(%.2f %.2f %.2f)", inst.handle, inst.prio, phase, (int)split, b[0], b[1], b[2], (unsigned)m.p.size(), m.layers, (int)f.hold, (int)f.in_head, (int)f.head_fed.valid, camR[1][0], camR[1][1], camR[1][2], f.cam_R[1][0], f.cam_R[1][1], f.cam_R[1][2]); }
        float M[16] = {A[0][0] * inst.S[0], A[1][0] * inst.S[0], A[2][0] * inst.S[0], 0,
                       A[0][1] * inst.S[1], A[1][1] * inst.S[1], A[2][1] * inst.S[1], 0,
                       A[0][2] * inst.S[2], A[1][2] * inst.S[2], A[2][2] * inst.S[2], 0,
                       b[0], b[1], b[2], 1};
        glLoadMatrixf(M);
        for (int layer = 0; layer < m.layers; layer++) {
            /* VPIX_NEAR_MODELS: the whole model counts 4 decal layers up (a separate model on the same plane as another's polygons:
               Zone's doorl/doorr against the room wall, Bobby 10-17 'this door flickers') */
            const int lay = layer + (inst.near_bias ? 4 : 0);
            if (lay && (decal_bias() > 0 || inst.near_bias)) {   /* VPIX_DECAL_BIAS: layer n drawn n * bias of its distance nearer */
                float Pl[16]; memcpy(Pl, proj, sizeof(Pl)); Pl[14] *= 1.f + (decal_bias() > 0 ? decal_bias() : 2e-5f) * lay;
                glMatrixMode(GL_PROJECTION); glLoadMatrixf(Pl); glMatrixMode(GL_MODELVIEW);
                glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(rev_z ? (float)lay : -(float)lay, 0.f);   /* + slope term */
            }
            else if (layer) { glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(rev_z ? 1.f : -1.f, rev_z ? 4.f : -4.f); }   /* toward the eye */
            for (size_t pi = 0; pi < m.p.size(); pi++) {
                const Poly &p = m.p[zf_rev ? m.p.size() - 1 - pi : pi];
                if (p.layer != layer) continue;
                if (inst.poly_select && p.sel && !(p.sel & inst.select)) continue;   /* VPIX_POLY_SELECT models: only the polygons the instance picks */
                bool trans = ((p.flags >> 24) == 0x68 || (mat28 && (p.flags >> 24) == 0x28)) && matalpha[p.flags & 31] < 1.f;
                if (trans != (pass == 1)) continue;
                float ex = A[0][0] * p.nx + A[0][1] * p.ny + A[0][2] * p.nz;
                float ey = A[1][0] * p.nx + A[1][1] * p.ny + A[1][2] * p.nz;
                float ez = A[2][0] * p.nx + A[2][1] * p.ny + A[2][2] * p.nz;
                float lam = inst.headlock ? 1.f : 0.45f + 0.55f * fabsf(ex * L[0] + ey * L[1] + ez * L[2]);   /* sell messages: full colour */
                uint32_t top = p.flags >> 24, rgb;
                /* VPIX_ONE_SIDED models only (HLE: model file name match): type 0x22 drawn ONE-SIDED (the card's back-face removal): drawn only when its face
                   normal points away from the eye, the card's front-facing winding. Zone Hunter: the building windows (44 per
                   BUILDING.BIN, wound like their walls) and the VIRTUALITY logo's white visor quads, which sit on the blue disc faces
                   with the V + headset cut out: from the front the near quad is culled and the far one shows through the V, the
                   same from the back (Bobby 10-17: the back showed the whole white quad over the disc). */
                bool inside = false;                         /* mode 3: back side seen -> flat suit grey */
                if ((top == 0x22 && inst.one_sided) || inst.one_sided >= 2) {   /* 2: every polygon (VPIX_ONE_SIDED 'name*'), 3: 'name#' */
                    /* winding of the corners as placed in eye space (a mirrored instance flips it, as on the card's screen test) */
                    float ve[3][3];
                    for (int c = 0; c < 3; c++) { const float *v = &m.v[3 * p.idx[c]];
                        for (int i = 0; i < 3; i++) ve[c][i] = A[i][0] * v[0] * inst.S[0] + A[i][1] * v[1] * inst.S[1] + A[i][2] * v[2] * inst.S[2] + b[i]; }
                    float e1[3] = {ve[1][0] - ve[0][0], ve[1][1] - ve[0][1], ve[1][2] - ve[0][2]}, e2[3] = {ve[2][0] - ve[0][0], ve[2][1] - ve[0][1], ve[2][2] - ve[0][2]};
                    float cn[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
                    if (cn[0] * ve[0][0] + cn[1] * ve[0][1] + cn[2] * ve[0][2] <= 0.f) { if (inst.one_sided != 3) continue; inside = true; }
                }
                const TexSlot *ts = NULL; Tex *tx = NULL;
                if (top >= 0x60) {
                    /* 0x78 = palette-textured: the low bits pick a model palette entry holding the texture
                       number, so one model can wear each instance's own face (Boxing) */
                    uint32_t tn = top == 0x78 ? inst.palw[p.flags & 15] & 0x3FF : p.flags & 0x3FF;
                    if (top == 0x68 && mattex[p.flags & 31] >= 0) tn = (uint32_t)mattex[p.flags & 31];
                    if (inst.scared && top == 0x60) { if (tn == 0xB || tn == 0xC) tn = 0xD; else if (tn == 0xE) tn = 0xF; }   /* Pac-Man scared eyes / mouth */
                    if (texslot[tn].set) {
                        ts = &texslot[tn];
                        auto ti = textures.find(ts->handle);
                        if (ti != textures.end()) tx = ti->second.get();
                    }
                }
                if (mat28 && top == 0x28) rgb = matcol[p.flags & 31];       /* PEARL: material colour (RGB like the palette) */
                else if (top == 0x28 || top == 0x38 || top == 0x30) rgb = inst.pal[p.flags & 15];
                else if (top >= 0x60) rgb = tx ? 0xFFFFFF : 0xB08C5A;     /* untextured fallback */
                else rgb = p.flags & 0xFFFFFF;
                if (tx) {
                    if (!tx->gl) {
                        GLuint id; glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tx->w, tx->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tx->rgba.data());
                        tx->gl = id;
                    }
                    bool masked = (ts->flags & 0x100000) != 0;
                    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, masked ? masked_gl(tx, ts->mask) : tx->gl);
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    if (masked) { glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.5f); }
                }
                int ua = 0, va = 2;                          /* projection plane: XZ unless flagged */
                if (ts) { if (ts->flags & 0x20000) { ua = 0; va = 1; } else if (ts->flags & 0x80000) { ua = 1; va = 2; } }
                if (trans) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE); }
                /* VPIX_SEAL models (Zone rooms): base polygons get a 2nd pass as their outline, which fills the hairline cracks
                   where one polygon's edge meets another's corner (T-junction). Room5's door bevel showed the dark tunnel through
                   such cracks (Bobby 10-17, circled); the card's rasterizer did not leave them. */
                for (int sp = 0; sp < ((inst.seal && !layer && !trans) ? 2 : 1); sp++) {
                glBegin(sp ? GL_LINE_LOOP : GL_POLYGON);
                for (int k = 0; k < p.n; k++) {
                    /* flat/Gouraud/texel colours are 0xBBGGRR on this hardware; the palette is RGB */
                    uint32_t c = (top == 0x40 || top == 0x48) ? p.vcol[k] : rgb;
                    bool palc = top == 0x28 || top == 0x38 || top == 0x30;
                    /* Gouraud vertex words 0x20BBGGRR = colour; 0x18 (palette bit 0x08) = model palette
                       index in the low bits (Boxing's gloves take each boxer's colour) */
                    if ((top == 0x40 || top == 0x48) && (c >> 24 & 0x08)) { c = inst.pal[c & 15]; palc = true; }
                    if (!palc) c = ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF);
                    if (inside) c = 0xB4B4B4;                /* kickW suit grey, unlit-looking */
                    glColor4f(((c >> 16) & 255) / 255.f * lam, ((c >> 8) & 255) / 255.f * lam, (c & 255) / 255.f * lam, trans ? matalpha[p.flags & 31] : 1.f);
                    const float *v = &m.v[3 * p.idx[k]];
                    if (tx) glTexCoord2f((v[ua] - ts->o[ua]) * ts->su / tx->w, (v[va] - ts->o[va]) * ts->sv / tx->h);
                    if (inst.ymove && fabsf(v[1] - inst.ymove_from) < 0.5f) { float vc[3] = {v[0], inst.ymove_to, v[2]}; glVertex3fv(vc); }   /* VPIX_MOVE_Y */
                    else if (v[1] < inst.ymin || v[1] > inst.ymax) { float vc[3] = {v[0], v[1] < inst.ymin ? inst.ymin : inst.ymax, v[2]}; glVertex3fv(vc); }   /* VPIX_CLAMP_Y trim */
                    else glVertex3fv(v);
                }
                glEnd();
                }
                if (trans) { glDisable(GL_BLEND); glDepthMask(GL_TRUE); }
                if (tx) { glDisable(GL_TEXTURE_2D); glDisable(GL_ALPHA_TEST); }
            }
            if (layer || inst.near_bias) glDisable(GL_POLYGON_OFFSET_FILL);
            if ((layer && decal_bias() > 0) || inst.near_bias) { glMatrixMode(GL_PROJECTION); glLoadMatrixf(proj); glMatrixMode(GL_MODELVIEW); }
        }
    }
    if (any_top) glDepthFunc(depth_fn);                         /* VPIX_ON_TOP pass done */
    }   /* phase */
}

static void upload(Tex *tx, bool nearest) {
    if (tx->gl) return;
    GLuint id; glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
    GLint f = nearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, nearest ? GL_CLAMP : GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, nearest ? GL_CLAMP : GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tx->w, tx->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tx->rgba.data());
    tx->gl = id;
}

/* The 2D layer, drawn in eye space as a panel filling the game's original field of view
   (the Visette overlaid it on the 3D image). proj = this eye's projection. */
static void draw_blits(const VPIX_Frame &f, const float proj[16], bool backdrop, bool mid) {
    if (f.blits.empty()) return;
    glMatrixMode(GL_PROJECTION); glLoadMatrixf(proj);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D); glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    const float z = -1000.f;                       /* beyond the near plane (20) */
    float tx_ = -z * tanf(f.fov_deg * 3.14159265f / 360.f), ty_ = tx_ / screen_aspect;   /* PAL pixels aren't square: shown 4:3 */
    /* VPIX_BLIT_NUDGE="dx,dy" (games.ps1 BlitNudge, screen pixels, + = right / down): Missile Command draws its 16x16 sight at
       (167,115) = centre (175,123), a pixel up-left of the 3D centre (176,124), and its laser beams end ~55 units under the
       centre line: the shots sat 'a hair' under the sight (Bobby 10-17). */
    static float nud[2] = {0, 0}; static bool nrd = false;
    if (!nrd) { nrd = true; const char *e = getenv("VPIX_BLIT_NUDGE"); if (e && *e) sscanf(e, "%f,%f", &nud[0], &nud[1]); }
    glTranslatef(nud[0] / screen_w * 2 * tx_, -nud[1] / screen_h * 2 * ty_, 0);
    for (const VPIX_Blit &b : f.blits) {
        if (b.world || b.backdrop != backdrop || b.mid != mid) continue;   /* VPIX_BLIT_PRIORITY: backdrops go under the 3D scene; mid: between models (draw_scene) */
        auto it = bitmaps.find(b.handle);
        if (it == bitmaps.end()) continue;
        Tex *t = it->second.get();
        glBindTexture(GL_TEXTURE_2D, bitmap_gl(t, b.key));
        if (t->mono) glColor4f(b.tint[0], b.tint[1], b.tint[2], b.alpha); else glColor4f(1, 1, 1, b.alpha);
        /* screen y runs top-down; source rect is measured from the bitmap's top row; texture row 0 = bottom */
        float u0 = (float)b.sx / t->w, u1 = (float)(b.sx + b.sw) / t->w;
        float v1 = 1.f - (float)b.sy / t->h, v0 = 1.f - (float)(b.sy + b.sh) / t->h;
        /* clip to the visible screen: the card drew nothing past its bottom edge, and games park
           sprites there (Ghost Train's start-prompt letters wait at y 268 before flying up) */
        float dh = b.dh, bot = (std::min)(screen_h, clip_h > 0 ? clip_h : screen_h);
        if (b.y >= bot) continue;
        if (b.y + dh > bot) { v0 += (v1 - v0) * (b.y + dh - bot) / dh; dh = bot - b.y; }
        float x0 = (b.x / screen_w * 2 - 1) * tx_, x1 = ((b.x + b.dw) / screen_w * 2 - 1) * tx_;
        float y1 = (1 - b.y / screen_h * 2) * ty_, y0 = (1 - (b.y + dh) / screen_h * 2) * ty_;
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex3f(x0, y0, z); glTexCoord2f(u1, v0); glVertex3f(x1, y0, z);
        glTexCoord2f(u1, v1); glVertex3f(x1, y1, z); glTexCoord2f(u0, v1); glVertex3f(x0, y1, z);
        glEnd();
    }
    glDisable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

/* the FLIC video layer, same 2D panel space as the bitmaps */
static void draw_flic(const VPIX_Frame &f, const float proj[16]) {
    if (!flic_on || flic_w <= 0) return;
    if (!flic_gl) { glGenTextures(1, &flic_gl); }
    glBindTexture(GL_TEXTURE_2D, flic_gl);
    if (flic_dirty) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, flic_w, flic_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, flic_px.data());
        flic_dirty = false;
    }
    glMatrixMode(GL_PROJECTION); glLoadMatrixf(proj);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D); glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); glColor4f(1, 1, 1, 1);
    const float z = -1000.f;
    float tx_ = -z * tanf(f.fov_deg * 3.14159265f / 360.f), ty_ = tx_ / screen_aspect;
    /* VPIX_FLIC_CENTRE (Pac-Man): the board movie fills the whole screen behind the texts, no blue border */
    static const bool fill = getenv("VPIX_FLIC_CENTRE") && atoi(getenv("VPIX_FLIC_CENTRE"));
    float fx = fill ? 0.f : (float)flic_x, fy = fill ? 0.f : (float)flic_y, fw = fill ? screen_w : (float)flic_w, fh = fill ? screen_h : (float)flic_h;
    float x0 = (fx / screen_w * 2 - 1) * tx_, x1 = ((fx + fw) / screen_w * 2 - 1) * tx_;
    float y0 = (1 - fy / screen_h * 2) * ty_, y1 = (1 - (fy + fh) / screen_h * 2) * ty_;   /* row 0 = top */
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(x0, y0, z); glTexCoord2f(1, 0); glVertex3f(x1, y0, z);
    glTexCoord2f(1, 1); glVertex3f(x1, y1, z); glTexCoord2f(0, 1); glVertex3f(x0, y1, z);
    glEnd();
    glDisable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

/* Studio splash: a head-locked card over whatever is showing for the first SPLASH_SECS
   seconds of the session, fading out over the last one. */
static const double SPLASH_SECS = 0.0;   /* off until the splash art is made; 5.0 to show it */
static GLuint splash_gl = 0;
static void draw_splash(const float proj[16], float alpha) {
    if (alpha <= 0.f) return;
    if (!splash_gl) {
        glGenTextures(1, &splash_gl); glBindTexture(GL_TEXTURE_2D, splash_gl);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VPIX_SPLASH_W, VPIX_SPLASH_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, VPIX_SPLASH_RGBA);
    }
    glMatrixMode(GL_PROJECTION); glLoadMatrixf(proj);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, splash_gl);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); glColor4f(1, 1, 1, alpha);
    const float z = -1000.f, hw = 380.f, hh = hw * VPIX_SPLASH_H / VPIX_SPLASH_W;   /* ~42 degrees wide */
    glBegin(GL_QUADS);                             /* texture rows are top-down */
    glTexCoord2f(0, 1); glVertex3f(-hw, -hh, z); glTexCoord2f(1, 1); glVertex3f(hw, -hh, z);
    glTexCoord2f(1, 0); glVertex3f(hw, hh, z); glTexCoord2f(0, 0); glVertex3f(-hw, hh, z);
    glEnd();
    glDisable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

/* World-space sprites (flag 0x20): camera-facing quads at the slot position, depth tested.
   Call with the same camera as draw_scene. */
static void draw_sprites(const VPIX_Frame &f, const float camR[3][3], const float camT[3], int which) {
    /* which: 0 = all (with mid bitmaps: only those above the split), 1 = only those at or below the split */
    int32_t sp; const bool split = mid_split(f, sp);
    if (which == 1 && !split) return;
    auto pick = [&](const VPIX_Blit &b) { return b.world && (!split || ((b.prio <= sp) == (which == 1))); };
    bool any = false;
    for (const VPIX_Blit &b : f.blits) any |= pick(b);
    if (!any) return;
    static const float C[3][3] = {{1, 0, 0}, {0, 0, 1}, {0, -1, 0}};
    float CRv[3][3]; mat_mul3(C, camR, CRv);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.01f);
    glEnable(GL_TEXTURE_2D); glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    for (const VPIX_Blit &b : f.blits) {
        if (!pick(b)) continue;
        auto it = bitmaps.find(b.handle);
        if (it == bitmaps.end()) continue;
        Tex *t = it->second.get();
        glBindTexture(GL_TEXTURE_2D, bitmap_gl(t, b.key));
        glColor4f(1, 1, 1, b.alpha);
        float d[3] = {b.pos[0] - camT[0], b.pos[1] - camT[1], b.pos[2] - camT[2]}, e[3];
        for (int i = 0; i < 3; i++) e[i] = CRv[i][0] * d[0] + CRv[i][1] * d[1] + CRv[i][2] * d[2];   /* eye space */
        /* the pivot is measured from the bitmap's top-left, y DOWN (screen style): Shoot For Loot's 'shoot me' robot (SH00-19, 2298 x 3000,
           pivot 1149, 3000 = bottom centre) stands on its spot. Measured up, it hung 3000 units under the floor (Bobby 10-17: invisible,
           the gun still locked on). Every other game's sprites pivot at the centre, unchanged. */
        float x0 = e[0] - b.pivot[0], x1 = x0 + b.dw, y1 = e[1] + b.pivot[1], y0 = y1 - b.dh;
        float u0 = (float)b.sx / t->w, u1 = (float)(b.sx + b.sw) / t->w;
        float v1 = 1.f - (float)b.sy / t->h, v0 = 1.f - (float)(b.sy + b.sh) / t->h;
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex3f(x0, y0, e[2]); glTexCoord2f(u1, v0); glVertex3f(x1, y0, e[2]);
        glTexCoord2f(u1, v1); glVertex3f(x1, y1, e[2]); glTexCoord2f(u0, v1); glVertex3f(x0, y1, e[2]);
        glEnd();
    }
    glDisable(GL_TEXTURE_2D); glDisable(GL_BLEND); glDisable(GL_ALPHA_TEST);
}

/* VPIX_DEPTH_FIX (games.ps1 DepthFix): depth precision. The default is near 20 / far 400000 into a 24-bit
   fixed-point depth buffer: the depth step at distance z is ~z^2 / (20 * 2^24) units (1.2 units at 20000,
   7.5 at 50000), so surfaces a few units apart (raised decals on the floors, Dactyl's feet/head details,
   the game's 0.5-5 unit "stickers") z-fight in the distance. With the flag: reversed Z (glClipControl
   0..1, depth = near / distance, GL_GEQUAL, cleared to 0), an infinite far plane and a 32-bit float depth
   buffer for every 3D pass (eyes, spectator, and the desktop window through its own FBO): ~2^-23 relative
   precision at every distance (0.005 units at 40000). Without glClipControl it falls back to a near plane
   of 60 units (3x the precision; the own gun stays well beyond 32 mm). */
static bool depth_fix() { static const bool d = getenv("VPIX_DEPTH_FIX") && atoi(getenv("VPIX_DEPTH_FIX")); return d; }
/* standard GL projection -> reversed-Z infinite (clip z = near, w = -z_eye): depth 1 at the near plane, 0 at infinity */
static void rev_proj(float P[16]) { if (rev_z) { P[10] = 0; P[14] = z_near; } }
static void frustum(float fov_deg, float aspect, float out[16]) {
    float zn = z_near, zf = 400000.f;
    float hx = zn * tanf(fov_deg * 3.14159265f / 360.f), hy = hx / aspect;
    memset(out, 0, 64);
    out[0] = zn / hx; out[5] = zn / hy; out[10] = -(zf + zn) / (zf - zn); out[11] = -1; out[14] = -2 * zf * zn / (zf - zn);
    rev_proj(out);
}

#ifdef _WIN32
LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_SIZE) { win_w = LOWORD(l); win_h = HIWORD(l); return 0; }
    if (m == WM_CLOSE) { ShowWindow(h, SW_MINIMIZE); return 0; }   /* closing is DOSBox-X's job */
    return DefWindowProc(h, m, w, l);
}

static void save_bmp(const char *path) {
    int w = win_w & ~3, h = win_h;
    if (w <= 0 || h <= 0) return;
    std::vector<uint8_t> px((size_t)w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, px.data());
    FILE *f = fopen(path, "wb"); if (!f) return;
    uint32_t sz = 54 + (uint32_t)px.size();
    uint8_t hd[54] = {'B', 'M'}; memcpy(hd + 2, &sz, 4); hd[10] = 54; hd[14] = 40;
    memcpy(hd + 18, &w, 4); memcpy(hd + 22, &h, 4); hd[26] = 1; hd[28] = 24;
    fwrite(hd, 1, 54, f); fwrite(px.data(), 1, px.size(), f); fclose(f);
}

/* VPIX_ZFIGHT=<base path> (dev, desktop window with DepthFix): every 5 s draw the 3D scene twice, the second
   time with every model and polygon in reverse order and the camera nudged 0.05 units. Surfaces that are
   properly in front of each other look the same; z-fighting pixels (equal or rounding-close depths) flip.
   Logs "zfight <n> px changed" in <log>.render and saves the first image with the changed pixels in magenta. */
static void zfight_probe(const VPIX_Frame &vf, const float P[16], const float R[3][3], const float T[3], int w, int h, bool fbo, const char *path) {
    w &= ~3; if (w <= 0 || h <= 0) return;
    std::vector<uint8_t> a((size_t)w * h * 3), b(a.size());
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(fbo ? 0x8CE0 /* GL_COLOR_ATTACHMENT0 */ : GL_BACK);
    draw_scene(vf, P, R, T, w, h);
    glReadPixels(0, 0, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, a.data());
    float T2[3] = {T[0] + 0.05f, T[1] + 0.05f, T[2] + 0.05f};
    zf_rev = true; draw_scene(vf, P, R, T2, w, h); zf_rev = false;
    glReadPixels(0, 0, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, b.data());
    long n = 0, nlow = 0;
    for (size_t i = 0; i < a.size(); i += 3) {
        int dd = abs(a[i] - b[i]) + abs(a[i + 1] - b[i + 1]) + abs(a[i + 2] - b[i + 2]);
        if (dd > 24) { n++; if (i / 3 < (size_t)w * h / 2) nlow++; a[i] = 255; a[i + 1] = 0; a[i + 2] = 255; }
    }
    rl("zfight %s: %ld px changed of %d (%ld in the lower half)", path, n, w * h, nlow);
    FILE *f = fopen(path, "wb"); if (!f) return;
    uint32_t sz = 54 + (uint32_t)a.size();
    uint8_t hd[54] = {'B', 'M'}; memcpy(hd + 2, &sz, 4); hd[10] = 54; hd[14] = 40;
    memcpy(hd + 18, &w, 4); memcpy(hd + 22, &h, 4); hd[26] = 1; hd[28] = 24;
    fwrite(hd, 1, 54, f); fwrite(a.data(), 1, a.size(), f); fclose(f);
}

/* ---- GL framebuffer objects (GL 3.0 / ARB_fbo entry points) --------------------- */
#define GL_FRAMEBUFFER 0x8D40
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_LOWER_LEFT 0x8CA1
#define GL_ZERO_TO_ONE 0x935F
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_RGBA8 0x8058
typedef void (APIENTRY *PGenBuf)(GLsizei, GLuint *);
typedef void (APIENTRY *PBindBuf)(GLenum, GLuint);
typedef void (APIENTRY *PFbTex)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (APIENTRY *PRbStore)(GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY *PFbRb)(GLenum, GLenum, GLenum, GLuint);
typedef GLenum (APIENTRY *PFbStatus)(GLenum);
typedef void (APIENTRY *PBlit)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
PGenBuf glGenFramebuffers_, glGenRenderbuffers_; PBindBuf glBindFramebuffer_, glBindRenderbuffer_;
PFbTex glFramebufferTexture2D_; PRbStore glRenderbufferStorage_; PFbRb glFramebufferRenderbuffer_;
PFbStatus glCheckFramebufferStatus_; PBlit glBlitFramebuffer_;

static bool load_fbo() {
#define GP(v, t, n) v = (t)wglGetProcAddress(n)
    GP(glGenFramebuffers_, PGenBuf, "glGenFramebuffers"); GP(glGenRenderbuffers_, PGenBuf, "glGenRenderbuffers");
    GP(glBindFramebuffer_, PBindBuf, "glBindFramebuffer"); GP(glBindRenderbuffer_, PBindBuf, "glBindRenderbuffer");
    GP(glFramebufferTexture2D_, PFbTex, "glFramebufferTexture2D"); GP(glRenderbufferStorage_, PRbStore, "glRenderbufferStorage");
    GP(glFramebufferRenderbuffer_, PFbRb, "glFramebufferRenderbuffer"); GP(glCheckFramebufferStatus_, PFbStatus, "glCheckFramebufferStatus");
    GP(glBlitFramebuffer_, PBlit, "glBlitFramebuffer");
#undef GP
    return glGenFramebuffers_ && glBindFramebuffer_ && glFramebufferTexture2D_ && glGenRenderbuffers_ && glBindRenderbuffer_ &&
           glRenderbufferStorage_ && glFramebufferRenderbuffer_ && glCheckFramebufferStatus_ && glBlitFramebuffer_;
}

struct EyeFB { GLuint fbo = 0, tex = 0, depth = 0; };
static bool make_eye_fb(EyeFB &e, int w, int h) {
    glGenTextures(1, &e.tex); glBindTexture(GL_TEXTURE_2D, e.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glGenRenderbuffers_(1, &e.depth); glBindRenderbuffer_(GL_RENDERBUFFER, e.depth);
    glRenderbufferStorage_(GL_RENDERBUFFER, rev_z ? GL_DEPTH_COMPONENT32F : GL_DEPTH_COMPONENT24, w, h);
    glGenFramebuffers_(1, &e.fbo); glBindFramebuffer_(GL_FRAMEBUFFER, e.fbo);
    glFramebufferTexture2D_(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, e.tex, 0);
    glFramebufferRenderbuffer_(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, e.depth);
    bool ok = glCheckFramebufferStatus_(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer_(GL_FRAMEBUFFER, 0);
    return ok;
}
static void free_fb(EyeFB &e) {
    if (e.fbo) { typedef void (APIENTRY *PDel)(GLsizei, const GLuint *);
                 PDel df = (PDel)wglGetProcAddress("glDeleteFramebuffers"), dr = (PDel)wglGetProcAddress("glDeleteRenderbuffers");
                 if (df) df(1, &e.fbo); if (dr) dr(1, &e.depth); }
    if (e.tex) glDeleteTextures(1, &e.tex);
    e = EyeFB();
}
/* VPIX_DEPTH_FIX setup (needs the FBO entry points: every 3D pass then renders into a float-depth FBO) */
static void depth_setup() {
    if (!depth_fix()) return;
    typedef void (APIENTRY *PClip)(GLenum, GLenum);
    PClip cc = (PClip)wglGetProcAddress("glClipControl");
    if (!cc) cc = (PClip)wglGetProcAddress("glClipControlARB");
    if (cc && load_fbo()) { cc(GL_LOWER_LEFT, GL_ZERO_TO_ONE); rev_z = true; rl("depth fix: reversed Z, 32-bit float depth, infinite far plane (near %.0f)", z_near); }
    else { z_near = 60.f; rl("depth fix: no glClipControl/FBO -> 24-bit depth with near plane %.0f", z_near); }
}

/* ---- poses ------------------------------------------------------------------
   OpenVR standing space: x right, y up, -z forward, metres. Tracker space: X right,
   Y forward, Z up, mm.  A(v) = (v.x, -v.z, v.y). */
static float yaw_off = 0;              /* player's snap-turn offset, radians */
static float loco[2] = {0, 0};         /* right-stick back/strafe movement, mm (tracker space, after yaw_off) */
static float walk_yaw = 0;             /* VPIX_WALK_TURN: head turn fed to the game while back/strafe is held */
static float stick_back = 0, stick_side = 0;
static float test_hand[3] = {0, 0, 0};       /* test player: extra hand offset, mm (desktop) */
/* VPIX_LEAN=<mm> (games.ps1 Lean, Boxing): the game moves you by how far your head leans off centre
   ("lean or move forwards or backwards in the CS unit ... lean to the sides" - its help text), so the
   sticks hold a lean of up to this many mm while pushed instead of sliding you permanently */
static float lean_mm() { static const float l = getenv("VPIX_LEAN") ? (float)atof(getenv("VPIX_LEAN")) : 0.f; return l; }
static float lean_in[2] = {0, 0}, lean_off[2] = {0, 0};
/* VPIX_JOYAXES (games.ps1 JoyAxes: Ghost Train push/pull, Missile Command circling): the pod joystick
   had x / y axes as well as buttons; the thumbsticks feed them (the stronger stick wins) */
static bool joy_axes() { static const bool j = getenv("VPIX_JOYAXES") && atoi(getenv("VPIX_JOYAXES")); return j; }
static float joy_in[2] = {0, 0};   /* stick (side, forward) -1..1; offset mm (tracker space) */
static void lean_update(const VPIX_Pose &head) {
    float fx = head.R[1][0], fy = head.R[1][1], fl = sqrtf(fx * fx + fy * fy), L = lean_mm();
    lean_off[0] = lean_off[1] = 0;
    if (L <= 0 || fl < 0.1f) return;
    fx /= fl; fy /= fl;
    lean_off[0] = (fx * lean_in[1] + fy * lean_in[0]) * L;
    lean_off[1] = (fy * lean_in[1] - fx * lean_in[0]) * L;
}   /* desktop keys S/A/D (or the test player): -1..0 back, -1..1 side */
/* back/strafe for games that walk where the head faces (Pac-Man: locked to the maze, head position
   ignored): instead of sliding the position, hold the game's walk and turn the fed head that way */
static bool walk_turn() { static const bool w = getenv("VPIX_WALK_TURN") && atoi(getenv("VPIX_WALK_TURN")); return w; }
static bool walk_turn_set(float back, float side, uint32_t &btn) {
    if (!walk_turn()) return false;
    walk_yaw = (back != 0 || side != 0) ? atan2f(-side, back) : 0.f;   /* back = pi, right = -pi/2 */
    if (walk_yaw != 0 || back != 0) btn |= VPIX_BTN_WALK;
    return true;
}
static const float SNAP = 0.5235988f;  /* 30 degree snap turn */
static vr::IVRSystem *vrsys = NULL;    /* set when SteamVR starts */
static void trim_press(float px, float py);

/* SteamVR Input (action manifest next to the exe: mallvr_actions.json + default bindings for Index
   and Vive wands). The old controller-state API mixes the Index stick and touchpad onto one axis;
   actions keep them apart and let players remap in SteamVR. Without the manifest: legacy input. */
struct VPIX_Acts { bool fire = false, menu = false, walk = false, top = false, trim = false, turnl = false, turnr = false, aim = false, reach = false, crouch = false;
                   float rmove[2] = {0, 0}, lmove[2] = {0, 0}, pad[2] = {0, 0}, aimpad[2] = {0, 0}, reachpad[2] = {0, 0}; };
static struct { bool ok = false; vr::VRActionSetHandle_t set = 0;
                vr::VRActionHandle_t fire = 0, menu = 0, walk = 0, top = 0, rmove = 0, lmove = 0, pad = 0, trim = 0, turnl = 0, turnr = 0,
                                     aimpad = 0, aim = 0, reachpad = 0, reach = 0, crouch = 0, haptic = 0;
                vr::VRInputValueHandle_t hand_src[2] = {0, 0}; } acts;
static void acts_init() {
    char exe[MAX_PATH] = {0}; GetModuleFileNameA(NULL, exe, MAX_PATH);
    std::string man(exe); man = man.substr(0, man.find_last_of("\\/") + 1) + "mallvr_actions.json";
    if (GetFileAttributesA(man.c_str()) == INVALID_FILE_ATTRIBUTES) { rl("no %s: legacy controller input", man.c_str()); return; }
    vr::IVRInput *in = vr::VRInput();
    if (!in) return;
    vr::EVRInputError e = in->SetActionManifestPath(man.c_str());
    if (e != vr::VRInputError_None) { rl("SetActionManifestPath error %d: legacy controller input", (int)e); return; }
    in->GetActionSetHandle("/actions/main", &acts.set);
    in->GetActionHandle("/actions/main/in/fire", &acts.fire);
    in->GetActionHandle("/actions/main/in/menu", &acts.menu);
    in->GetActionHandle("/actions/main/in/walk", &acts.walk);
    in->GetActionHandle("/actions/main/in/top", &acts.top);
    in->GetActionHandle("/actions/main/in/move_right", &acts.rmove);
    in->GetActionHandle("/actions/main/in/move_left", &acts.lmove);
    in->GetActionHandle("/actions/main/in/trim_pad", &acts.pad);
    in->GetActionHandle("/actions/main/in/trim_press", &acts.trim);
    in->GetActionHandle("/actions/main/in/turn_left", &acts.turnl);
    in->GetActionHandle("/actions/main/in/turn_right", &acts.turnr);
    in->GetActionHandle("/actions/main/in/aim_pad", &acts.aimpad);
    in->GetActionHandle("/actions/main/in/aim_press", &acts.aim);
    in->GetActionHandle("/actions/main/in/reach_pad", &acts.reachpad);
    in->GetActionHandle("/actions/main/in/reach_press", &acts.reach);
    in->GetActionHandle("/actions/main/in/crouch", &acts.crouch);
    in->GetActionHandle("/actions/main/out/haptic", &acts.haptic);
    in->GetInputSourceHandle("/user/hand/right", &acts.hand_src[0]);
    in->GetInputSourceHandle("/user/hand/left", &acts.hand_src[1]);
    acts.ok = true;
    rl("SteamVR input actions: %s", man.c_str());
}
/* VPIX_Haptic: the HLE queues rumbles (game sound events, VPIX_HAPTIC); the render thread plays them after its input update */
struct HapticReq { int hand; float secs, amp; };
static std::mutex hap_mtx;
static std::vector<HapticReq> hap_q;
static void haptic_queue(int hand, float seconds, float amplitude) {
    std::lock_guard<std::mutex> lk(hap_mtx);
    if (hap_q.size() < 16) hap_q.push_back({hand, seconds, amplitude});
}
static void haptics_play() {
    std::vector<HapticReq> q;
    { std::lock_guard<std::mutex> lk(hap_mtx); q.swap(hap_q); }
    if (q.empty() || !acts.ok || !acts.haptic) return;
    vr::IVRInput *in = vr::VRInput();
    if (!in) return;
    for (const HapticReq &r : q)
        for (int h = 0; h < 2; h++)
            if (r.hand == 2 || r.hand == h)
                in->TriggerHapticVibrationAction(acts.haptic, 0.f, r.secs, 160.f, r.amp > 1.f ? 1.f : r.amp, acts.hand_src[h]);
}
static void acts_read(VPIX_Acts &a) {
    vr::IVRInput *in = vr::VRInput();
    vr::VRActiveActionSet_t as = {}; as.ulActionSet = acts.set;
    in->UpdateActionState(&as, sizeof(as), 1);
    auto b = [&](vr::VRActionHandle_t h) {
        vr::InputDigitalActionData_t d = {};
        return in->GetDigitalActionData(h, &d, sizeof(d), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && d.bActive && d.bState;
    };
    auto v = [&](vr::VRActionHandle_t h, float o[2]) {
        vr::InputAnalogActionData_t d = {};
        if (in->GetAnalogActionData(h, &d, sizeof(d), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && d.bActive) { o[0] = d.x; o[1] = d.y; }
    };
    a.fire = b(acts.fire); a.menu = b(acts.menu); a.walk = b(acts.walk); a.top = b(acts.top); a.trim = b(acts.trim);
    a.turnl = b(acts.turnl); a.turnr = b(acts.turnr);
    v(acts.rmove, a.rmove); v(acts.lmove, a.lmove); v(acts.pad, a.pad);
    a.aim = b(acts.aim); a.reach = b(acts.reach); v(acts.aimpad, a.aimpad); v(acts.reachpad, a.reachpad);
    a.crouch = b(acts.crouch);
}
/* VPIX_CROUCH (games.ps1 Crouch: Dactyl Nightmare 1 + 2, whose move_players crouches the player when the
   head is 200+ mm below the height calibrated at "focus visette"): hold the crouch button (a plain digital
   button: Vive = left menu button, Index = right B) to feed the head and hand VPIX_CROUCH_MM lower (default 400: past the
   game's deepest crouch frame at 375 mm). Inert in every other game. */
static float crouch_amount() {
    static const float c = !(getenv("VPIX_CROUCH") && atoi(getenv("VPIX_CROUCH"))) ? 0.f
                         : getenv("VPIX_CROUCH_MM") && atof(getenv("VPIX_CROUCH_MM")) > 0 ? (float)atof(getenv("VPIX_CROUCH_MM")) : 400.f;
    return c;
}
static float crouch_now = 0;           /* mm the head/hand are fed lower right now */
static void crouch_set(bool held) {
    float c = held ? crouch_amount() : 0.f;
    if (c != crouch_now) rl("crouch %s (%.0f mm)", c > 0 ? "ON" : "off", c);
    crouch_now = c;
}

/* VPIX_HOLD_VIEW (games.ps1 HoldView: Dactyl Nightmare 1 + 2). When a round ends (DAC game_mode 3, "GAME OVER"
   until the initials screen) the game keeps reading the tracker and keeps InHead set, but stops updating its
   camera and every model. eye_camera's late head correction only applies while the camera pitch matches the
   fed head; with a frozen camera that test fails and the free-look branch cancels the head's yaw (and all head
   translation), so the view locked rigid to the face (Bobby 10-10: sickening). Here: once the camera stays
   identical while the fed head keeps moving, hold the last camera as the position anchor together with the
   head pose that built it (head_fed of the last frame the camera changed), so eye_camera keeps applying the
   live head rotation and offset. The own body, gun, forearm and shots (tagged by the HLE) are hidden
   meanwhile (DAC: only at GAME OVER, game_mode 3). Ends the moment the game moves its camera again or leaves the in-head view. */
static bool hold_view_on() { static const bool h = getenv("VPIX_HOLD_VIEW") && atoi(getenv("VPIX_HOLD_VIEW")); return h; }
static struct HoldView {
    bool on = false, anchor = false, prev = false; int still = 0; unsigned long frame = ~0ul;
    float camR[3][3], camT[3]; VPIX_Pose prev_hf, hf, hand0;
} hold;
static bool differs(const float *a, const float *b, int n, float eps) {
    for (int i = 0; i < n; i++) if (fabsf(a[i] - b[i]) > eps) return true;
    return false;
}
/* once per new game frame the renderer picks up */
static void hold_update(const VPIX_Frame &f, const VPIX_Pose &hand) {
    if (!hold_view_on() || f.frame == hold.frame) return;
    hold.frame = f.frame;
    if (!f.in_head || !f.head_fed.valid) {
        if (hold.on) rl("hold view OFF f=%lu (left the in-head view)", f.frame);
        hold.on = hold.anchor = hold.prev = false; hold.still = 0; return;
    }
    /* games with the game_mode symbol (DAC): hold only at GAME OVER (mode 3, after the clock runs out), never
       for the brief freezes in play (death / respawn); others (DN2: no symbols) keep the frozen-camera test alone */
    const bool mode_ok = f.game_mode < 0 || f.game_mode == 3;
    if (!mode_ok && hold.on) { rl("hold view OFF f=%lu (game_mode %d)", f.frame, f.game_mode); hold.on = false; hold.still = 0; }
    bool cam = !hold.prev || differs(&f.cam_R[0][0], &hold.camR[0][0], 9, 1e-4f) || differs(f.cam_T, hold.camT, 3, 0.5f);
    bool head = hold.prev && (differs(f.head_fed.p, hold.prev_hf.p, 3, 0.5f) || differs(&f.head_fed.R[0][0], &hold.prev_hf.R[0][0], 9, 1e-3f));
    if (cam) {
        if (hold.on) rl("hold view OFF f=%lu (game camera moves again)", f.frame);
        hold.on = false; hold.still = 0;
        /* anchor only a frame whose camera really is the fed head (pitch matches, as eye_camera tests) */
        hold.anchor = fabsf(f.cam_R[1][2] - f.head_fed.R[1][2]) < 0.03f
                   && fabsf(f.cam_T[0]) + fabsf(f.cam_T[1]) + fabsf(f.cam_T[2]) < 1e7f;   /* not the parked camera (1e8) before play */
        if (hold.anchor) { hold.hf = f.head_fed; hold.hand0 = hand; }
    } else if (head && ++hold.still >= 4 && !hold.on && hold.anchor && mode_ok) {
        hold.on = true;
        rl("hold view ON f=%lu: game camera frozen at T=(%.0f %.0f %.0f) fwd=(%.3f %.3f %.3f), anchor head (%.0f %.0f %.0f)", f.frame,
           f.cam_T[0], f.cam_T[1], f.cam_T[2], f.cam_R[1][0], f.cam_R[1][1], f.cam_R[1][2], hold.hf.p[0], hold.hf.p[1], hold.hf.p[2]);
    }
    memcpy(hold.camR, f.cam_R, sizeof(hold.camR)); memcpy(hold.camT, f.cam_T, sizeof(hold.camT));
    hold.prev_hf = f.head_fed; hold.prev = true;
}
/* the frame to draw: f itself, or (holding) a copy with the anchor head and without the own rig.
   The player's own body (HLE tag 3: model slots 0-15), gun (1), gun forearm (2) and own shots (4) are NOT drawn
   while the view is held (DAC: GAME OVER only). Moving them instead failed: the body turned with the head and the
   gun with the controller, so the forearm + gun floated off the upper arm (Bobby 10-10, in the headset); his
   call: hide the own rig at GAME OVER, keep full head tracking. */
static const VPIX_Frame &hold_apply(const VPIX_Frame &f) {
    if (!hold.on) return f;
    static VPIX_Frame v;
    v = f; v.hold = true; v.head_fed = hold.hf; v.crouch_mm = crouch_now;   /* anchor z already has its crouch taken off */
    v.inst.erase(std::remove_if(v.inst.begin(), v.inst.end(), [](const VPIX_Instance &in) { return in.own_hand != 0; }), v.inst.end());
    static unsigned long nlog = 0;
    if (nlog++ % 450 == 0) rl("hold view f=%lu: own rig hidden (%u of %u models drawn)", f.frame, (unsigned)v.inst.size(), (unsigned)f.inst.size());
    return v;
}
/* VPIX_BOX_GLOVE_TRACK (games.ps1 GloveTrack, Boxing): draw your own gloves where the controllers ARE, the way Dactyl's gun
   sits on the controller. The game's arm IK only ever knows ONE punching arm (one pod joystick: the further-forward controller
   is fed, with reach / height / spread fudges and a 60 mm hand switch), so its gloves lagged, jumped and crossed (Bobby 10-16:
   'arm tracking is still horrible'). The hand feed to the game is unchanged (punch detection, hits, scoring); only the picture
   moves. The HLE tags your forced gloves (own_hand 5: man5\chgl.bin = forearm + glove, local +Y elbow -> fist, fist centre at
   +Y ~280) and their biceps (6, hidden: they would point at the game's glove spot). Each glove's pose = the controller's pose
   relative to the head, put on the late-corrected head camera exactly like the eyes (so it stays glued to the controller).
   Right glove = right controller, left = left; the game's glove to your right becomes the right one.
   Tuning: VPIX_BOX_GLOVE_PITCH (degrees about the controller's right axis, + = knuckles up), VPIX_BOX_GLOVE_OFF="x,y,z" mm
   in the controller's frame (grip centre vs the controller origin), VPIX_BOX_GLOVE_FIST (model units, fist centre along +Y). */
/* pose tilted by deg about its own right axis */
static VPIX_Pose pose_pitched(const VPIX_Pose &h, float deg) {
    VPIX_Pose g = h;
    float a = deg * 3.14159265f / 180.f, c = cosf(a), sn = sinf(a);
    for (int i = 0; i < 3; i++) { g.R[1][i] = h.R[1][i] * c + h.R[2][i] * sn; g.R[2][i] = h.R[2][i] * c - h.R[1][i] * sn; }
    return g;
}
static bool glove_track_on() { static const bool g = getenv("VPIX_BOX_GLOVE_TRACK") && atoi(getenv("VPIX_BOX_GLOVE_TRACK")); return g; }
static const VPIX_Frame &glove_apply(const VPIX_Frame &f, const VPIX_Pose &head_now, const VPIX_Pose &rh, const VPIX_Pose &lh) {
    if (!glove_track_on() || !head_now.valid || !f.in_head || !f.head_fed.valid) return f;
    if (fabsf(f.cam_R[1][2] - f.head_fed.R[1][2]) >= 0.03f) return f;        /* not the head camera (pan, cut-scene): leave the game's */
    int ng = 0; for (const VPIX_Instance &in : f.inst) if (in.own_hand == 5) ng++;
    if (!ng || (!rh.valid && !lh.valid)) return f;
    static float pitch = 0, off[3] = {0, -30, -30}, fist = 280; static bool rd = false;
    if (!rd) { rd = true; const char *e;
        if ((e = getenv("VPIX_BOX_GLOVE_PITCH"))) pitch = (float)atof(e);
        if ((e = getenv("VPIX_BOX_GLOVE_OFF"))) sscanf(e, "%f,%f,%f", &off[0], &off[1], &off[2]);
        if ((e = getenv("VPIX_BOX_GLOVE_FIST"))) fist = (float)atof(e);
        rl("glove track: pitch %.0f deg, grip offset (%.0f %.0f %.0f) mm, fist at +Y %.0f", pitch, off[0], off[1], off[2], fist); }
    float HR[3][3], HT[3];
    eye_camera(f, head_now, head_now, HR, HT);                              /* the head's own camera (eyes are offsets from it) */
    float b = yaw_of(f.cam_R[1]) - yaw_of(f.head_fed.R[1]), c = cosf(b), sn = sinf(b);
    auto rz = [&](const float v[3], float o[3]) { o[0] = c * v[0] - sn * v[1]; o[1] = sn * v[0] + c * v[1]; o[2] = v[2]; };
    static VPIX_Frame v;
    v = f; v.inst.clear();
    /* the game's gloves, sorted left -> right by their head-camera x (the game's right glove becomes the right controller's) */
    std::vector<VPIX_Instance> gl, bi;
    for (const VPIX_Instance &in : f.inst) {
        if (in.own_hand == 6) bi.push_back(in);                            /* bicep: re-aimed at its glove below */
        else if (in.own_hand == 5) gl.push_back(in); else v.inst.push_back(in);
    }
    auto camx = [&](const VPIX_Instance &in) { return (in.T[0] - HT[0]) * HR[0][0] + (in.T[1] - HT[1]) * HR[0][1] + (in.T[2] - HT[2]) * HR[0][2]; };
    std::sort(gl.begin(), gl.end(), [&](const VPIX_Instance &a, const VPIX_Instance &bb) { return camx(a) < camx(bb); });
    std::sort(bi.begin(), bi.end(), [&](const VPIX_Instance &a, const VPIX_Instance &bb) { return camx(a) < camx(bb); });
    const std::vector<VPIX_Instance> gl0 = gl;                             /* the game's own glove spots (its elbows) */
    if (gl.size() == 1) gl.push_back(gl[0]);                               /* one glove in the scene: both hands get one */
    const VPIX_Pose *hands[2] = {&lh, &rh};
    /* smallest rotation taking unit vector a onto unit vector t, applied to a model's local axes (rows of R) */
    auto turn = [](float R[3][3], const float a[3], const float t[3]) {
        float ax[3] = {a[1] * t[2] - a[2] * t[1], a[2] * t[0] - a[0] * t[2], a[0] * t[1] - a[1] * t[0]};
        float sn = sqrtf(ax[0] * ax[0] + ax[1] * ax[1] + ax[2] * ax[2]), cs = a[0] * t[0] + a[1] * t[1] + a[2] * t[2];
        if (sn < 1e-6f) return;
        for (int k = 0; k < 3; k++) ax[k] /= sn;
        for (int r = 0; r < 3; r++) {
            const float *p = R[r]; float kx[3] = {ax[1] * p[2] - ax[2] * p[1], ax[2] * p[0] - ax[0] * p[2], ax[0] * p[1] - ax[1] * p[0]};
            float kd = ax[0] * p[0] + ax[1] * p[1] + ax[2] * p[2], q[3];
            for (int k = 0; k < 3; k++) q[k] = p[k] * cs + kx[k] * sn + ax[k] * kd * (1 - cs);
            memcpy(R[r], q, sizeof(q));
        }
    };
    auto unit = [](float v3[3]) { float l = sqrtf(v3[0] * v3[0] + v3[1] * v3[1] + v3[2] * v3[2]); if (l > 1e-6f) for (int k = 0; k < 3; k++) v3[k] /= l; return l; };
    /* two-bone IK (Bobby 10-17: biceps back, but 'the elbows were not staying together'): the fist sits on the controller, the
       shoulder where the game puts it (the bicep's origin), the elbow is solved from the two bone lengths (bicep = the game's own
       shoulder -> elbow distance, forearm = the glove's elbow -> fist) and bends down and outward. The bicep aims at that elbow,
       the glove runs elbow -> fist (keeping the controller's roll), so the two always meet. Past full reach both stretch alike
       (up to x1.4). VPIX_BOX_BICEP=0: biceps hidden, gloves on the controllers (the 10-16 behaviour). */
    static const bool bicep_on = !(getenv("VPIX_BOX_BICEP") && !atoi(getenv("VPIX_BOX_BICEP")));
    for (int i = 0; i < 2; i++) {
        VPIX_Instance g = gl[i == 0 ? 0 : gl.size() - 1];
        const VPIX_Pose &h = *hands[i];
        /* this side's bicep: left-most <-> left glove, right-most <-> right; a lone one by its side of the head */
        int bj = -1;
        if (bi.size() >= 2) bj = i == 0 ? 0 : (int)bi.size() - 1;
        else if (bi.size() == 1 && (camx(bi[0]) < 0) == (i == 0)) bj = 0;
        if (!h.valid) { v.inst.push_back(g); if (bj >= 0) v.inst.push_back(bi[bj]); continue; }   /* that controller lost: the game's arm stays */
        VPIX_Pose hp = pose_pitched(h, pitch);
        float grip[3], d[3], w[3], F[3];
        for (int k = 0; k < 3; k++) grip[k] = h.p[k] + off[0] * h.R[0][k] + off[1] * h.R[1][k] + off[2] * h.R[2][k];
        for (int k = 0; k < 3; k++) d[k] = (grip[k] - head_now.p[k]) * world_scale;
        rz(d, w);
        for (int r = 0; r < 3; r++) rz(hp.R[r], g.R[r]);
        for (int k = 0; k < 3; k++) F[k] = HT[k] + w[k];                    /* fist centre = the controller */
        float gy0 = 0;                                                      /* the glove mesh's elbow end (lowest local y) */
        {   std::shared_ptr<Mesh> gm;
            { std::lock_guard<std::mutex> lk(mtx); auto it = meshes.find(g.handle); if (it != meshes.end()) gm = it->second; }
            if (gm && gm->v.size() >= 3) { gy0 = 1e30f; for (size_t q = 1; q < gm->v.size(); q += 3) gy0 = (std::min)(gy0, gm->v[q]); if (gy0 > fist - 50.f) gy0 = 0; }
        }
        float L2 = (fist - gy0) * g.S[1];
        bool ik = false;
        if (bicep_on && bj >= 0 && !gl0.empty()) {
            VPIX_Instance b = bi[bj];
            const VPIX_Instance *o = &gl0[0]; float best = 1e30f;           /* the game's glove on this bicep: its elbow sets the bicep length */
            for (const VPIX_Instance &g0 : gl0) { float d2 = 0; for (int k = 0; k < 3; k++) { float dd = g0.T[k] - b.T[k]; d2 += dd * dd; } if (d2 < best) { best = d2; o = &g0; } }
            float a[3], u[3];
            for (int k = 0; k < 3; k++) { a[k] = o->T[k] - b.T[k]; u[k] = F[k] - b.T[k]; }
            float L1 = unit(a), dist = unit(u);
            float tipw[3] = {0, 0, 0}; bool have_tip = false;               /* the bicep mesh's elbow tip, world offset from the shoulder */
            {   /* the bicep's real length: its mesh's furthest point along the shoulder -> elbow line. The game's own elbow gap
                   varies (486..775 in one round, log boxplay4), and using it opened a gap between bicep and forearm (Bobby 10-17) */
                std::shared_ptr<Mesh> bm;
                { std::lock_guard<std::mutex> lk(mtx); auto it = meshes.find(b.handle); if (it != meshes.end()) bm = it->second; }
                if (bm && bm->v.size() >= 3) {
                    float c[3]; for (int r = 0; r < 3; r++) c[r] = b.S[r] * (b.R[r][0] * a[0] + b.R[r][1] * a[1] + b.R[r][2] * a[2]);
                    /* the tip vertex is picked once per bicep model, then kept: re-picking it every frame made the joint wobble */
                    static std::map<uint32_t, size_t> tip_q;
                    auto tq = tip_q.find(b.handle);
                    if (tq == tip_q.end()) {
                        float mx = 0; size_t qt = 0;
                        for (size_t q = 0; q + 2 < bm->v.size(); q += 3) { float pr = bm->v[q] * c[0] + bm->v[q + 1] * c[1] + bm->v[q + 2] * c[2]; if (pr > mx) { mx = pr; qt = q; } }
                        if (mx > 100.f) tq = tip_q.emplace(b.handle, qt).first;
                    }
                    if (tq != tip_q.end() && tq->second + 2 < bm->v.size()) {
                        size_t qt = tq->second;
                        for (int k = 0; k < 3; k++) tipw[k] = b.S[0] * bm->v[qt] * b.R[0][k] + b.S[1] * bm->v[qt + 1] * b.R[1][k] + b.S[2] * bm->v[qt + 2] * b.R[2][k];
                        float lw = sqrtf(tipw[0] * tipw[0] + tipw[1] * tipw[1] + tipw[2] * tipw[2]);
                        if (lw > 100.f) { L1 = lw; have_tip = true; }
                    }
                }
            }
            /* VPIX_BOX_ARM=hinge (option; IK is the default, Bobby's pick 10-17 after trying both. Bobby 10-17: 'the glove should be the parent to the arm, and the elbow a hinge ... the
               upper arm just follows'): the glove stays locked to the controller exactly (the 10-16 tracking he called perfect), the
               elbow is its back end, and the bicep swings from the shoulder to that point, stretched or shortened along its own
               shoulder -> elbow line so its tip lands on it (x0.6..1.8). Default: the two-bone IK below (glove angle follows the arm). */
            static const bool hinge = getenv("VPIX_BOX_ARM") && !_stricmp(getenv("VPIX_BOX_ARM"), "hinge");
            if (hinge && have_tip) {
                float E[3], t[3], tw[3] = {tipw[0], tipw[1], tipw[2]};
                for (int k = 0; k < 3; k++) { E[k] = F[k] - (fist - gy0) * g.S[1] * g.R[1][k]; t[k] = E[k] - b.T[k]; }
                float lt = unit(t), lw = unit(tw);
                if (lt > 1.f && lw > 1.f) {
                    turn(b.R, tw, t);                                       /* tip direction onto the elbow */
                    float k2 = lt / lw; k2 = k2 < 0.6f ? 0.6f : k2 > 1.8f ? 1.8f : k2;
                    for (int r = 0; r < 3; r++) {                           /* stretch along t only: rows need not stay orthonormal (world = sum S v R) */
                        float pr = b.R[r][0] * t[0] + b.R[r][1] * t[1] + b.R[r][2] * t[2];
                        for (int k = 0; k < 3; k++) b.R[r][k] += (k2 - 1.f) * pr * t[k];
                    }
                    for (int k = 0; k < 3; k++) g.T[k] = F[k] - fist * g.S[1] * g.R[1][k];
                    v.inst.push_back(b); v.inst.push_back(g);
                    static unsigned long nh = 0;
                    if (nh++ % 1800 == 0) rl("glove track: %s arm hinge, bicep %.0f -> %.0f (x%.2f)", i ? "right" : "left", lw, lt, lt / lw);
                    continue;
                }
            }
            /* the arm has limits (Bobby 10-17: 'make it so the elbow and arm always stay together, the arm just has limits'):
               the reach is clamped to the arm's range, so past full reach the arm stays straight and the glove stops at its end
               (the game still gets the real controller: punches score as before). Never falls back to a detached arm. */
            if (have_tip && L1 < 1200.f && dist > 1.f) {
                float dmin = fabsf(L1 - L2) + 1.f, dmax = L1 + L2 - 0.5f;
                float dc = dist < dmin ? dmin : dist > dmax ? dmax : dist;
                float Fc[3]; for (int k = 0; k < 3; k++) Fc[k] = b.T[k] + dc * u[k];
                float ea = (L1 * L1 - L2 * L2 + dc * dc) / (2 * dc), eh2 = L1 * L1 - ea * ea, eh = eh2 > 0 ? sqrtf(eh2) : 0;
                /* bend direction (Bobby 10-17: the right elbow 'kept flipping' with a fixed down + outward pole): toward where the
                   controller's own angle puts the elbow, square to the shoulder -> fist line; down + outward when that is degenerate. Smoothed per arm. */
                float side = i == 0 ? -1.f : 1.f, pole[3];
                for (int k = 0; k < 3; k++) pole[k] = Fc[k] - L2 * g.R[1][k] - b.T[k];
                float pu = pole[0] * u[0] + pole[1] * u[1] + pole[2] * u[2];
                for (int k = 0; k < 3; k++) pole[k] -= pu * u[k];
                if (unit(pole) < 0.15f * L1) {
                    for (int k = 0; k < 3; k++) pole[k] = (k == 2 ? -1.f : 0.f) + 0.6f * side * HR[0][k];
                    pu = pole[0] * u[0] + pole[1] * u[1] + pole[2] * u[2];
                    for (int k = 0; k < 3; k++) pole[k] -= pu * u[k];
                    if (unit(pole) < 1e-3f) { pole[0] = HR[2][0]; pole[1] = HR[2][1]; pole[2] = HR[2][2]; }
                }
                static float prev_pole[2][3]; static bool have_prev[2] = {false, false};
                if (have_prev[i]) {
                    float pp[3]; for (int k = 0; k < 3; k++) pp[k] = 0.5f * pole[k] + 0.5f * prev_pole[i][k];
                    pu = pp[0] * u[0] + pp[1] * u[1] + pp[2] * u[2];
                    for (int k = 0; k < 3; k++) pp[k] -= pu * u[k];
                    if (unit(pp) > 1e-3f) memcpy(pole, pp, sizeof(pp));
                }
                memcpy(prev_pole[i], pole, sizeof(pole)); have_prev[i] = true;
                float E[3], be[3], ef[3], tw[3] = {tipw[0], tipw[1], tipw[2]};
                for (int k = 0; k < 3; k++) { E[k] = b.T[k] + ea * u[k] + eh * pole[k]; be[k] = E[k] - b.T[k]; ef[k] = Fc[k] - E[k]; }
                unit(be); unit(ef); unit(tw);
                turn(b.R, tw, be);                                          /* bicep: its tip vertex exactly onto the elbow */
                float gy[3] = {g.R[1][0], g.R[1][1], g.R[1][2]};
                turn(g.R, gy, ef);                                          /* glove: elbow -> fist, controller roll kept */
                for (int k = 0; k < 3; k++) g.T[k] = E[k] - gy0 * g.S[1] * g.R[1][k];   /* the glove's elbow end on the elbow */
                v.inst.push_back(b);
                ik = true;
                static unsigned long nb = 0;
                if (nb++ % 1800 == 0) rl("glove track: %s arm IK bicep %.0f (game gap %.0f) forearm %.0f (elbow end y %.0f) reach %.0f -> %.0f", i ? "right" : "left", L1, sqrtf(best), L2, gy0, dist, dc);
            }
            if (!ik) v.inst.push_back(b);                                   /* no sane IK this frame: the game's bicep as it drew it */
        }
        if (!ik) for (int k = 0; k < 3; k++) g.T[k] = F[k] - fist * g.S[1] * g.R[1][k];
        v.inst.push_back(g);
    }
    static unsigned long nlog = 0;
    if (nlog++ % 900 == 0) rl("glove track f=%lu: %d game glove(s), right %s, left %s", f.frame, ng, rh.valid ? "on" : "lost", lh.valid ? "on" : "lost");
    return v;
}
/* the controller axis that is a thumbstick (Index: not axis 0, which is its touchpad); 0 if none */
static int stick_axis(vr::TrackedDeviceIndex_t d) {
    static std::map<vr::TrackedDeviceIndex_t, int> cache;
    auto it = cache.find(d);
    if (it != cache.end()) return it->second;
    int a = 0;
    if (vrsys)
        for (int i = 0; i < 5; i++) {
            int32_t t = vrsys->GetInt32TrackedDeviceProperty(d, (vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + i));
            if (t == vr::k_eControllerAxis_Joystick) { a = i; break; }
        }
    rl("controller %u: stick on axis %d", (unsigned)d, a);
    return cache[d] = a;
}
static float head_xy_now[2] = {0, 0};  /* last head position (after yaw_off), mm: the turn pivot */
/* turn the player about their own head: yaw_off rotates the tracked space about its origin, so
   shift the stick offset to keep the head where it was (no orbiting around the room centre) */
static void snap_turn(float a) {
    static const bool no_turn = getenv("VPIX_NOTURN") && atoi(getenv("VPIX_NOTURN"));   /* games you only look around in (Ghost Train) */
    if (no_turn) return;
    yaw_off += a;
    float c = cosf(a), s = sinf(a), x = head_xy_now[0], y = head_xy_now[1];
    loco[0] += x - (c * x - s * y);
    loco[1] += y - (s * x + c * y);
    head_xy_now[0] = c * x - s * y; head_xy_now[1] = s * x + c * y;
}

static void rotz(float a, float v[3]) {
    float c = cosf(a), s = sinf(a), x = v[0], y = v[1];
    v[0] = x * c - y * s; v[1] = x * s + y * c;
}
static VPIX_Pose vr_to_pose(const vr::HmdMatrix34_t &m) {
    VPIX_Pose p; p.valid = true;
    float col[4][3];
    for (int c = 0; c < 4; c++) { col[c][0] = m.m[0][c]; col[c][1] = -m.m[2][c]; col[c][2] = m.m[1][c]; }
    for (int i = 0; i < 3; i++) { p.R[0][i] = col[0][i]; p.R[1][i] = -col[2][i]; p.R[2][i] = col[1][i]; p.p[i] = col[3][i] * 1000.f; }
    for (int r = 0; r < 3; r++) rotz(yaw_off, p.R[r]);
    rotz(yaw_off, p.p);
    return p;
}
static vr::HmdMatrix34_t mul34(const vr::HmdMatrix34_t &a, const vr::HmdMatrix34_t &b) {
    vr::HmdMatrix34_t o;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 4; j++)
            o.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + (j == 3 ? a.m[i][3] : 0.f);
    return o;
}

/* per-player settings that persist between sessions, per game: aim pitch trim (gun_pitch_<GAME>; games
   without one start from the old all-games gun_pitch, head-aimed games from 0) and hand reach
   (hand_reach_<GAME>). Every key in the file is kept, so games don't wipe each other's. */
static float gun_pitch = -20.f, hand_reach = 1.f, hand_zr = 0.f;
static std::map<std::string, float> settings;
static const char *settings_path() {         /* VPIX_SETTINGS (the package sets it), else the dev copy */
    const char *e = getenv("VPIX_SETTINGS");
    return e && *e ? e : "H:/Games/Virtuality/_work/vpix_settings.txt";
}
static std::string game_code() {             /* from the hook table name: .../GUN.hooks -> GUN */
    const char *e = getenv("VPIX_HOOKS");
    std::string g = e ? e : "";
    size_t s = g.find_last_of("/\\");
    if (s != std::string::npos) g = g.substr(s + 1);
    return g.substr(0, g.find('.'));
}
static std::string reach_key() { return "hand_reach_" + game_code(); }
static std::string pitch_key() { return "gun_pitch_" + game_code(); }
static std::string zr_key() { return "hand_zr_" + game_code(); }
static bool is_box() { return game_code() == "BOX"; }
static bool aim_head() { const char *e = getenv("VPIX_AIM_HEAD"); return e && atoi(e) != 0; }
static void load_settings() {
    FILE *f = fopen(settings_path(), "r");
    if (f) {
        char k[64]; float v;
        while (fscanf(f, "%63s %f", k, &v) == 2) settings[k] = v;
        fclose(f);
    }
    if (aim_head()) gun_pitch = 0.f;
    else if (settings.count("gun_pitch")) gun_pitch = settings["gun_pitch"];
    if (settings.count(pitch_key())) gun_pitch = settings[pitch_key()];
    if (settings.count(reach_key())) hand_reach = settings[reach_key()];
    if (is_box()) { const char *e = getenv("VPIX_HAND_Z_RIGHT"); if (e) hand_zr = (float)atof(e); if (settings.count(zr_key())) hand_zr = settings[zr_key()]; }
}
static void save_settings() {
    settings[pitch_key()] = gun_pitch; settings[reach_key()] = hand_reach;
    if (is_box()) settings[zr_key()] = hand_zr;
    FILE *f = fopen(settings_path(), "w"); if (!f) return;
    for (auto &kv : settings) fprintf(f, "%s %.3f\n", kv.first.c_str(), kv.second);
    fclose(f);
}
/* touchpad press: up/down = aim pitch trim (2 degree steps), left/right = hand reach x/÷1.25; both saved
   per game. On the right touchpad for Index controllers (their right stick moves), else the left pad. */
static void trim_press(float px, float py) {
    if (is_box() && fabsf(py) > 0.4f && fabsf(py) >= fabsf(px)) {   /* Boxing: up/down = the right arm's height (the pitch trim does nothing there), 20 mm steps */
        hand_zr = (std::min)(300.f, (std::max)(-300.f, hand_zr + (py > 0 ? -20.f : 20.f))); save_settings(); rl("%s %.0f", zr_key().c_str(), hand_zr);
    }
    else if (fabsf(py) > 0.4f && fabsf(py) >= fabsf(px)) { gun_pitch += py > 0 ? 2.f : -2.f; save_settings(); rl("%s %.0f", pitch_key().c_str(), gun_pitch); }
    else if (fabsf(px) > 0.4f) {
        hand_reach = (std::min)(20.f, (std::max)(0.1f, px > 0 ? hand_reach * 1.25f : hand_reach / 1.25f));
        save_settings(); rl("%s %.2f", reach_key().c_str(), hand_reach);
    }
}

/* desktop fallback: a standing head at 1.7 m, keyboard look/fire (only while our process has focus) */
static VPIX_Pose desk_head, desk_hand, desk_hand2;   /* desk_hand2: only the dev test player sets it (VPIX_TESTHAND=2) */
static float desk_az = 0, desk_el = 0, mouse_az = 0, mouse_el = 0;   /* mouse_*: gun aim offset from the view, arrow keys (desktop) */
static int desk_coins = 0;
static bool focused() {
    DWORD pid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}
/* keyboard arrows (game window focused): up/down = aim trim, left/right = arm reach, for any
   controller (Vive wands have no spare pad for them) */
static void arrow_trims() {
    static bool was[4] = {false, false, false, false};
    static const int keys[4] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT};
    static const float dx[4] = {0, 0, -1, 1}, dy[4] = {1, -1, 0, 0};
    bool f = focused();
    for (int k = 0; k < 4; k++) {
        bool on = f && (GetAsyncKeyState(keys[k]) & 0x8000) != 0;
        if (on && !was[k]) trim_press(dx[k], dy[k]);
        was[k] = on;
    }
}
static void azel_pose(VPIX_Pose &p, float az, float el, float x, float y, float z) {
    float fw[3] = {-sinf(az) * cosf(el), cosf(az) * cosf(el), sinf(el)};
    float rt[3] = {cosf(az), sinf(az), 0};
    float up[3] = {rt[1] * fw[2] - rt[2] * fw[1], rt[2] * fw[0] - rt[0] * fw[2], rt[0] * fw[1] - rt[1] * fw[0]};
    memcpy(p.R[0], rt, 12); memcpy(p.R[1], fw, 12); memcpy(p.R[2], up, 12);
    p.p[0] = x; p.p[1] = y; p.p[2] = z; p.valid = true;
}
/* VPIX_STICK_TWIST=<deg> (games.ps1 StickTwist, Missile Command): twisting the gun hand moves the gun along its track
   (the pod joystick's twist); stick left / right (window mode: A / D) adds up to this many degrees of twist to the hand the
   game gets (Bobby 10-17: 'i want stick left and right to do that also'; 10-18: A/D in window mode). Right = twist right. */
static void stick_twist(VPIX_Pose &hand) {
    static const float tw = getenv("VPIX_STICK_TWIST") ? (float)atof(getenv("VPIX_STICK_TWIST")) : 0.f;
    float v = joy_in[0];
    if (tw == 0 || !hand.valid || fabsf(v) == 0.f) return;
    float a = v * tw * 3.14159265f / 180.f, c = cosf(a), sn = sinf(a), r0[3], r2[3];
    for (int k = 0; k < 3; k++) { r0[k] = hand.R[0][k] * c - hand.R[2][k] * sn; r2[k] = hand.R[2][k] * c + hand.R[0][k] * sn; }
    for (int k = 0; k < 3; k++) { hand.R[0][k] = r0[k]; hand.R[2][k] = r2[k]; }
}
static void desktop_input(double dt, const VPIX_Frame *f) {
    uint32_t btn = 0;
    bool start = false, bot = false;   /* bot: the dev test player aims with its own hand-drop allowance (no convergence) */
    if (focused()) {
        float rs = (float)dt * 1.5f;
        if (GetAsyncKeyState('J') & 0x8000) desk_az += rs;
        if (GetAsyncKeyState('L') & 0x8000) desk_az -= rs;
        if (GetAsyncKeyState('I') & 0x8000) desk_el = (std::min)(desk_el + rs, 1.2f);
        if (GetAsyncKeyState('K') & 0x8000) desk_el = (std::max)(desk_el - rs, -1.2f);
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) btn |= VPIX_BTN_TRIGGER;   /* Space or left mouse button = fire */
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) btn |= VPIX_BTN_TRIGGER;
        /* mouse = head / view (relative: the cursor is re-centred each frame; Tab frees it), arrow keys = gun aim, R = gun back to centre */
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) ExitProcess(0);   /* Esc = quit the game (desktop window) */
        { static bool cap = true, tabw = false; bool tb = ((GetAsyncKeyState(VK_TAB) | GetAsyncKeyState(VK_MBUTTON)) & 0x8000) != 0;   /* Tab or middle click toggles mouse look */
          if (tb && !tabw) cap = !cap; tabw = tb;
          HWND fw = GetForegroundWindow(); RECT cr; POINT c = {0, 0};
          if (cap && !(GetAsyncKeyState(VK_MENU) & 0x8000) && GetClientRect(fw, &cr) && cr.right > 0) {   /* hold Alt = mouse free */
              c.x = cr.right / 2; c.y = cr.bottom / 2; ClientToScreen(fw, &c);
              POINT pt; GetCursorPos(&pt);
              desk_az -= (pt.x - c.x) * 0.0025f; desk_el = (std::max)(-1.2f, (std::min)(1.2f, desk_el - (pt.y - c.y) * 0.0025f));
              SetCursorPos(c.x, c.y); }
          if (GetAsyncKeyState(VK_LEFT) & 0x8000) mouse_az += rs;
          if (GetAsyncKeyState(VK_RIGHT) & 0x8000) mouse_az -= rs;
          if (GetAsyncKeyState(VK_UP) & 0x8000) mouse_el += rs;
          if (GetAsyncKeyState(VK_DOWN) & 0x8000) mouse_el -= rs;
          if (GetAsyncKeyState('R') & 0x8000) mouse_az = mouse_el = 0;
          mouse_az = (std::max)(-0.8f, (std::min)(0.8f, mouse_az)); mouse_el = (std::max)(-0.7f, (std::min)(0.7f, mouse_el)); }
        if ((GetAsyncKeyState('W') | GetAsyncKeyState(VK_RBUTTON)) & 0x8000) btn |= VPIX_BTN_WALK;   /* W or right mouse = the pod hand's thumb button (walk) */
        /* S = step back, A/D = strafe (the right stick in VR) */
        stick_back = (GetAsyncKeyState('S') & 0x8000) ? -1.f : 0.f;
        stick_side = ((GetAsyncKeyState('D') & 0x8000) ? 1.f : 0.f) - ((GetAsyncKeyState('A') & 0x8000) ? 1.f : 0.f);
        if (GetAsyncKeyState('1') & 0x8000) start = true;
        crouch_set((GetAsyncKeyState('C') & 0x8000) != 0);     /* C = hold to crouch (VPIX_CROUCH games) */
        static bool c5 = false; bool k5 = (GetAsyncKeyState('5') & 0x8000) != 0;
        if (k5 && !c5) desk_coins++;
        c5 = k5;
    } else { stick_back = stick_side = 0; crouch_set(false); }
    if (joy_axes()) {                             /* desktop joystick axes: W/S y, A/D x */
        /* VPIX_TOP_BUTTONS (Ghost Train): W stays the pod's top button (moves the car); the keyboard had none (Bobby 10-18) */
        static const bool top_btns = getenv("VPIX_TOP_BUTTONS") && atoi(getenv("VPIX_TOP_BUTTONS"));
        joy_in[0] = stick_side;
        if (top_btns) joy_in[1] = stick_back;
        else { joy_in[1] = (btn & VPIX_BTN_WALK) ? 1.f : stick_back; btn &= ~VPIX_BTN_WALK; }
    } else
    if (lean_mm() > 0) {                          /* desktop lean: W/S forward/back, A/D sides */
        lean_in[0] = stick_side; lean_in[1] = (btn & VPIX_BTN_WALK) ? 1.f : stick_back; btn &= ~VPIX_BTN_WALK;
        lean_update(desk_head.valid ? desk_head : desk_hand);
    } else
    if (!walk_turn_set(stick_back, stick_side, btn)) {
        float st = 1400.f * (float)(std::min)(dt, 0.1), fx = -sinf(desk_az), fy = cosf(desk_az);
        loco[0] += (fx * stick_back + fy * stick_side) * st; loco[1] += (fy * stick_back - fx * stick_side) * st;
    }
    azel_pose(desk_head, desk_az, desk_el, 0, 0, 1700);
    float hx = -sinf(desk_az) * 400 + cosf(desk_az) * 250, hy = cosf(desk_az) * 400 + sinf(desk_az) * 250;
    /* the hand sits 500 mm under / 250 mm right of the eye; pointed parallel to the view, every shot landed that far
       low-right of the screen centre (Bobby 10-17: 'aim way above things'). VPIX_DESK_CONVERGE=<mm> (default 8000, 0 = old):
       turn the gun onto the view ray at that distance. The HLE feeds the hand's x/y offset times hand_reach, so aim from there. */
    static const float conv = getenv("VPIX_DESK_CONVERGE") ? (float)atof(getenv("VPIX_DESK_CONVERGE")) : 8000.f;
    float gaz = desk_az, gel = desk_el;
    if (conv > 0 && !bot) {
        float fw[3] = {-sinf(desk_az) * cosf(desk_el), cosf(desk_az) * cosf(desk_el), sinf(desk_el)};
        float v[3] = {fw[0] * conv - hx * hand_reach, fw[1] * conv - hy * hand_reach, 1700 + fw[2] * conv - 1200};
        gaz = atan2f(-v[0], v[1]); gel = atan2f(v[2], sqrtf(v[0] * v[0] + v[1] * v[1]));
    }
    azel_pose(desk_hand, gaz + mouse_az, gel + mouse_el, hx + test_hand[0], hy + test_hand[1], 1200 + test_hand[2]);
    stick_twist(desk_hand);
    std::lock_guard<std::mutex> lk(in_mtx);
    input.head = desk_head; input.hand = desk_hand; input.hand2 = desk_hand2; input.buttons = btn; input.coins = desk_coins; input.start = start; input.vr = false; input.crouch_mm = crouch_now;
    input.gun_pitch = 0.f;   /* the -20 default is the Vive wand's barrel tilt; a mouse-aimed gun has none (it put shots 20 deg low) */
    input.loco[0] = loco[0] + lean_off[0]; input.loco[1] = loco[1] + lean_off[1]; input.hand_reach = hand_reach; input.hand_zr = hand_zr; input.walk_yaw = walk_yaw; input.joy[0] = joy_in[0]; input.joy[1] = joy_in[1];
}

/* ---- spectator camera (VPIX_SPECTATOR=1, for recording video) -------------------
   The window shows a steady widescreen view instead of the raw left-eye mirror: the eye pose
   low-passed (no head shake), rendered at VPIX_SPEC_SS x the window size, scaled down (anti-aliased). */
static void smooth_pose(VPIX_Pose &s, const VPIX_Pose &e, double dt, float tau) {
    if (!e.valid) return;
    if (!s.valid) { s = e; return; }
    float a = 1.f - expf(-(float)dt / tau);
    for (int i = 0; i < 3; i++) { s.p[i] += a * (e.p[i] - s.p[i]); for (int j = 0; j < 3; j++) s.R[i][j] += a * (e.R[i][j] - s.R[i][j]); }
    /* re-orthonormalise rows (Gram-Schmidt): 0 then 1, 2 = 0 x 1 */
    float *r0 = s.R[0], *r1 = s.R[1], *r2 = s.R[2];
    float n = sqrtf(r0[0]*r0[0] + r0[1]*r0[1] + r0[2]*r0[2]); for (int k = 0; k < 3; k++) r0[k] /= n;
    float d = r0[0]*r1[0] + r0[1]*r1[1] + r0[2]*r1[2]; for (int k = 0; k < 3; k++) r1[k] -= d * r0[k];
    n = sqrtf(r1[0]*r1[0] + r1[1]*r1[1] + r1[2]*r1[2]); for (int k = 0; k < 3; k++) r1[k] /= n;
    float c[3] = {r0[1]*r1[2] - r0[2]*r1[1], r0[2]*r1[0] - r0[0]*r1[2], r0[0]*r1[1] - r0[1]*r1[0]};
    if (c[0]*r2[0] + c[1]*r2[1] + c[2]*r2[2] < 0) for (int k = 0; k < 3; k++) c[k] = -c[k];
    for (int k = 0; k < 3; k++) r2[k] = c[k];
}

static void thread_main() {
    HINSTANCE hi = GetModuleHandle(NULL);
    WNDCLASSA wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = hi; wc.lpszClassName = "VPIXRender";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassA(&wc);
    /* VPIX_WIN_W / _H: window size (video capture: 1920x1080) */
    if (getenv("VPIX_WIN_W") && atoi(getenv("VPIX_WIN_W")) > 0) win_w = atoi(getenv("VPIX_WIN_W"));
    if (getenv("VPIX_WIN_H") && atoi(getenv("VPIX_WIN_H")) > 0) win_h = atoi(getenv("VPIX_WIN_H"));
    RECT r = {0, 0, win_w, win_h}; AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    /* VPIX_WIN_X: borderless window at (x, 0) - a full 1920x1080 capture area on one monitor */
    const char *wx = getenv("VPIX_WIN_X");
    HWND hw = wx ? CreateWindowExA(WS_EX_TOPMOST, "VPIXRender", "MallVR - Gainesphere Studios", WS_POPUP | WS_VISIBLE,
                                 atoi(wx), 0, win_w, win_h + 1, NULL, NULL, hi, NULL)   /* +1 row: an exact-fullscreen window goes to
                                    independent flip and screen capture reads black */
                 : CreateWindowA("VPIXRender", "MallVR - Gainesphere Studios", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 40, 40, r.right - r.left, r.bottom - r.top, NULL, NULL, hi, NULL);
    HDC dc = GetDC(hw);
    PIXELFORMATDESCRIPTOR pfd = {sizeof(pfd), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32};
    pfd.cDepthBits = 24;
    SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd);
    HGLRC rc = wglCreateContext(dc); wglMakeCurrent(dc, rc);
    typedef BOOL (WINAPI *SwapIntervalFn)(int);
    SwapIntervalFn swi = (SwapIntervalFn)wglGetProcAddress("wglSwapIntervalEXT");
    depth_setup();                         /* VPIX_DEPTH_FIX: before any FBO is made */
    EyeFB desk_fb; int desk_w = 0, desk_h = 0;   /* depth fix: the desktop view renders into its own float-depth FBO */

    /* ---- SteamVR ---- */
    vr::IVRSystem *sys = NULL;
    EyeFB eyes[2]; uint32_t ew = 0, eh = 0;
    const char *sw_ = getenv("VPIX_SCREEN_W"), *sh_ = getenv("VPIX_SCREEN_H");
    if (sw_ && atof(sw_) > 0) screen_w = (float)atof(sw_);
    if (sh_ && atof(sh_) > 0) screen_h = (float)atof(sh_);
    const char *sc_ = getenv("VPIX_SCREEN_CLIP");
    if (sc_ && atof(sc_) > 0) clip_h = (float)atof(sc_);
    const char *sa_ = getenv("VPIX_SCREEN_ASPECT");
    if (sa_ && atof(sa_) > 0) screen_aspect = (float)atof(sa_);
    const char *ws = getenv("VPIX_WORLD_SCALE");
    if (ws && atof(ws) > 0) world_scale = (float)atof(ws);
    const char *ve = getenv("VPIX_VR");
    if (!(ve && atoi(ve) == 0) && vr::VR_IsHmdPresent() && load_fbo()) {
        vr::EVRInitError err = vr::VRInitError_None;
        sys = vr::VR_Init(&err, vr::VRApplication_Scene); vrsys = sys; if (sys) acts_init();
        if (!sys || !vr::VRCompositor()) { rl("VR_Init failed: %s", vr::VR_GetVRInitErrorAsEnglishDescription(err)); sys = NULL; }
        else {
            sys->GetRecommendedRenderTargetSize(&ew, &eh);
            bool ok = make_eye_fb(eyes[0], ew, eh) && make_eye_fb(eyes[1], ew, eh);
            rl("SteamVR up: eye target %ux%u, FBOs %s", ew, eh, ok ? "ok" : "FAILED");
            if (!ok) { vr::VR_Shutdown(); sys = NULL; }
            else SetWindowTextA(hw, "MallVR - Gainesphere Studios - SteamVR (left eye mirror)");
        }
    } else rl("VR off (VPIX_VR=0, no headset, or no FBO support)");
    if (swi) swi(sys ? 0 : 1);             /* the compositor paces VR; never block on the monitor too */

    bool spect = getenv("VPIX_SPECTATOR") && atoi(getenv("VPIX_SPECTATOR")) > 0;
    int spec_ss = getenv("VPIX_SPEC_SS") ? atoi(getenv("VPIX_SPEC_SS")) : 2; if (spec_ss < 1) spec_ss = 1;
    float spec_tau = getenv("VPIX_SPEC_SMOOTH") ? (float)atof(getenv("VPIX_SPEC_SMOOTH")) : 0.15f;
    float spec_fov = getenv("VPIX_SPEC_FOV") ? (float)atof(getenv("VPIX_SPEC_FOV")) : 0.f;   /* 0 = the game's */
    EyeFB spec_fb; int spec_w = 0, spec_h = 0; VPIX_Pose spec_pose;
    VPIX_Frame cur; bool have = false;
    const char *shot = getenv("VPIX_SHOT");             /* base path: dump a BMP every 5 s of frames */
    unsigned long long next_shot = 0; int shot_no = 0;
    LARGE_INTEGER qf, q0, q1; QueryPerformanceFrequency(&qf); QueryPerformanceCounter(&q0);
    vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
    bool pad_was = false, menu_was = false, vr_start = false, lpad_was = false, lgrip_was = false, lturn_was = false, rpad_was = false, right_stick = false, aim_was = false, reach_was = false; int vr_coins = 0;
    load_settings();
    unsigned long long vr_frames = 0;
    double since_start = 0;
    while (running) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        QueryPerformanceCounter(&q1); double dt = (double)(q1.QuadPart - q0.QuadPart) / qf.QuadPart; q0 = q1;
        since_start += dt;
        float splash_a = (float)(since_start < SPLASH_SECS - 1 ? 1.0 : SPLASH_SECS - since_start);

        VPIX_Pose eye_pose[2], head_now;
        if (sys) {
            vr::VREvent_t ev;
            while (sys->PollNextEvent(&ev, sizeof(ev))) if (ev.eventType == vr::VREvent_Quit) { rl("SteamVR quit"); sys->AcknowledgeQuit_Exiting(); }
            vr::VRCompositor()->WaitGetPoses(poses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
            /* the gun hand: right-hand controller, else any controller */
            vr::TrackedDeviceIndex_t ci = sys->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
            if (ci == vr::k_unTrackedDeviceIndexInvalid || !poses[ci].bPoseIsValid)
                for (vr::TrackedDeviceIndex_t i = 1; i < vr::k_unMaxTrackedDeviceCount; i++)
                    if (sys->GetTrackedDeviceClass(i) == vr::TrackedDeviceClass_Controller && poses[i].bPoseIsValid) { ci = i; break; }
            uint32_t btn = 0;
            float sx = 0, sy = 0;                      /* strafe / back, applied below */
            if (acts.ok) {
                /* right stick: up walk, down back, left/right smooth turn. Left stick: up walk,
                   down back, left/right strafe. Index: left touchpad press up/down aim trim,
                   right touchpad press arm reach (wands: left pad both). Trigger fires, A/B coin + start;
                   grips turn only on wands (the bindings decide). */
                VPIX_Acts a; acts_read(a);
                haptics_play();
                if (a.fire) btn |= VPIX_BTN_TRIGGER;
                crouch_set(a.crouch);                      /* hold to crouch (VPIX_CROUCH games only) */
                /* VPIX_TOP_BUTTONS (games.ps1 TopButtons, Ghost Train: the pod's top button moves the car):
                   right grip / right stick click / left trigger */
                static const bool top_btns = getenv("VPIX_TOP_BUTTONS") && atoi(getenv("VPIX_TOP_BUTTONS"));
                if (top_btns && a.top) btn |= VPIX_BTN_WALK;
                float rx = a.rmove[0], ry = a.rmove[1], lsx = a.lmove[0], lsy = a.lmove[1];
                if (joy_axes()) {                          /* the sticks are the pod joystick's axes; click = top button */
                    if (a.walk) btn |= VPIX_BTN_WALK;
                    joy_in[0] = fabsf(lsx) >= fabsf(rx) ? lsx : rx; joy_in[1] = fabsf(lsy) >= fabsf(ry) ? lsy : ry;
                    for (float &v : joy_in) v = fabsf(v) < 0.15f ? 0.f : v;
                    rx = ry = 0;                             /* no stick turning */
                } else
                if (lean_mm() > 0) {                       /* lean games: both sticks lean; stick click = the top button */
                    if (a.walk) btn |= VPIX_BTN_WALK;
                    lean_in[0] = fabsf(lsx) >= fabsf(rx) ? lsx : rx; lean_in[1] = fabsf(lsy) >= fabsf(ry) ? lsy : ry;
                    for (float &v : lean_in) v = fabsf(v) < 0.2f ? 0.f : (v - (v > 0 ? 0.2f : -0.2f)) / 0.8f;
                } else
                if (a.walk || (ry > 0.5f && ry >= fabsf(rx)) || (lsy > 0.5f && lsy >= fabsf(lsx))) btn |= VPIX_BTN_WALK;
                else { sx = lsx; sy = (std::min)((std::min)(ry, lsy), 0.f); }
                /* smooth turn: up to 120 degrees/s at full deflection (the stick offset turns too) */
                if (fabsf(rx) > 0.25f && fabsf(rx) >= fabsf(ry) * 0.5f) {
                    float r = (rx - (rx > 0 ? 0.25f : -0.25f)) / 0.75f;
                    snap_turn(-r * 2.0944f * (float)(std::min)(dt, 0.1));
                }
                if (a.trim && !lpad_was) trim_press(a.pad[0], a.pad[1]);      /* wands: one pad for both */
                lpad_was = a.trim;
                /* Index: left touchpad press up/down = aim trim; right touchpad press = arm reach
                   (right or up further, left or down closer) */
                /* Index touchpads have no real click: a touch near an edge counts once; lift the thumb
                   or bring it back to the middle for the next step */
                if (!a.aim || fabsf(a.aimpad[1]) < 0.3f) aim_was = false;
                else if (!aim_was && fabsf(a.aimpad[1]) > 0.6f) { trim_press(0.f, a.aimpad[1]); aim_was = true; }
                {
                    float d = fabsf(a.reachpad[0]) >= fabsf(a.reachpad[1]) ? a.reachpad[0] : a.reachpad[1];
                    if (!a.reach || fabsf(d) < 0.3f) reach_was = false;
                    else if (!reach_was && fabsf(d) > 0.6f) { trim_press(d, 0.f); reach_was = true; }
                }
                if (a.turnl && !lgrip_was) snap_turn(SNAP);
                lgrip_was = a.turnl;
                if (a.turnr && !pad_was) snap_turn(-SNAP);
                pad_was = a.turnr;
                if (a.menu && !menu_was) vr_coins++;
                menu_was = a.menu; vr_start = a.menu;
                static float llx = 9, lly = 9, lrx = 9, lry = 9; static int nlog = 0;   /* controller diagnosis */
                if (nlog < 400 && (fabsf(rx - lrx) > 0.3f || fabsf(ry - lry) > 0.3f || fabsf(lsx - llx) > 0.3f || fabsf(lsy - lly) > 0.3f)) {
                    rl("sticks right=(%.2f %.2f) left=(%.2f %.2f) pad=(%.2f %.2f) trim=%d", rx, ry, lsx, lsy, a.pad[0], a.pad[1], a.trim ? 1 : 0);
                }
                {   /* touchpads (aim / reach) for diagnosis */
                    static bool la = false, lr = false; static int npl = 0;
                    if (npl < 200 && (a.aim != la || a.reach != lr)) {
                        rl("pads aim touch=%d (%.2f %.2f) reach touch=%d (%.2f %.2f)", a.aim, a.aimpad[0], a.aimpad[1], a.reach, a.reachpad[0], a.reachpad[1]);
                        la = a.aim; lr = a.reach; npl++;
                    }
                    lrx = rx; lry = ry; llx = lsx; lly = lsy; nlog++;
                }
            } else {
            if (ci != vr::k_unTrackedDeviceIndexInvalid) {
                vr::VRControllerState_t st;
                if (sys->GetControllerState(ci, &st, sizeof(st))) {
                    uint64_t pr = st.ulButtonPressed;
                    /* right stick (Index): up = the game's own walk (toward the gun), down = step back,
                       left/right = strafe; clicking the stick also walks. Grips = 30 degree snap turn
                       (right grip right, left grip left). */
                    /* Index controllers have a touchpad (axis 0) AND a thumbstick: use the axis the
                       driver reports as a joystick, else axis 0 (wands, other controllers) */
                    int ax = stick_axis(ci);
                    sx = st.rAxis[ax].x; sy = st.rAxis[ax].y;
                    bool pad = (pr & vr::ButtonMaskFromId(ax ? (vr::EVRButtonId)(vr::k_EButton_Axis0 + ax) : vr::k_EButton_SteamVR_Touchpad)) != 0;
                    right_stick = ax != 0;
                    if (right_stick) {                 /* Index: the right touchpad (axis 0) does the trims */
                        bool tp = (pr & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad)) != 0;
                        if (tp && !rpad_was) trim_press(st.rAxis[0].x, st.rAxis[0].y);
                        rpad_was = tp;
                    }
                    if ((pr & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) || st.rAxis[1].x > 0.6f) btn |= VPIX_BTN_TRIGGER;
                    /* the stick's direction wins: Index sticks can report a "click" when pushed far
                       over, so a click only walks while the stick is centred */
                    bool centred = fabsf(sx) < 0.3f && fabsf(sy) < 0.3f;
                    if ((sy > 0.5f && sy >= fabsf(sx)) || (pad && centred)) { btn |= VPIX_BTN_WALK; sx = sy = 0; }
                    {   /* log the right stick when it changes noticeably (controller diagnosis) */
                        static float lx = 9, ly = 9; static bool lp = false; static int nlog = 0;
                        if (nlog < 400 && (fabsf(sx - lx) > 0.3f || fabsf(sy - ly) > 0.3f || pad != lp)) {
                            rl("right stick (axis %d) x=%.2f y=%.2f click=%d | axes 0=(%.2f %.2f) 2=(%.2f %.2f) 3=(%.2f %.2f) 4=(%.2f %.2f) btns=%llX",
                               ax, sx, sy, pad ? 1 : 0, st.rAxis[0].x, st.rAxis[0].y, st.rAxis[2].x, st.rAxis[2].y,
                               st.rAxis[3].x, st.rAxis[3].y, st.rAxis[4].x, st.rAxis[4].y, (unsigned long long)pr);
                            lx = sx; ly = sy; lp = pad; nlog++;
                        }
                    }
                    bool grip = (pr & vr::ButtonMaskFromId(vr::k_EButton_Grip)) != 0;
                    if (grip && !pad_was) snap_turn(-SNAP);
                    pad_was = grip;
                    bool menu = (pr & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu)) != 0;
                    if (menu && !menu_was) vr_coins++;
                    menu_was = menu;
                    vr_start = menu;
                }
            }
            /* left controller: grip / thumbstick flick = snap turn; touchpad trims only without a right stick (wands) */
            vr::TrackedDeviceIndex_t li = sys->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
            if (li != vr::k_unTrackedDeviceIndexInvalid && li != ci) {
                vr::VRControllerState_t st;
                if (sys->GetControllerState(li, &st, sizeof(st))) {
                    bool pad = (st.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad)) != 0;
                    bool grip = (st.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip)) != 0;
                    if (grip && !lgrip_was) snap_turn(SNAP);
                    lgrip_was = grip;
                    /* left thumbstick (Index; the touchpad on axis 0 keeps the aim trims): flick left/right
                       = 30 degree snap turn, one per flick */
                    int lax = stick_axis(li);
                    if (lax) {
                        float tx = st.rAxis[lax].x;
                        if (!lturn_was && fabsf(tx) > 0.7f) { snap_turn(tx > 0 ? -SNAP : SNAP); lturn_was = true; }
                        else if (fabsf(tx) < 0.3f) lturn_was = false;
                    }
                    float py = st.rAxis[0].y, lx = st.rAxis[0].x;
                    if (pad && !lpad_was && !right_stick) trim_press(lx, py);   /* wands: trims stay on the left pad */
                    lpad_was = pad;
                }
            }
            }   /* legacy input */
            const vr::TrackedDevicePose_t &hp = poses[vr::k_unTrackedDeviceIndex_Hmd];
            if (hp.bPoseIsValid) {
                head_now = vr_to_pose(hp.mDeviceToAbsoluteTracking);
                head_xy_now[0] = head_now.p[0]; head_xy_now[1] = head_now.p[1];
                for (int e = 0; e < 2; e++)
                    eye_pose[e] = vr_to_pose(mul34(hp.mDeviceToAbsoluteTracking, sys->GetEyeToHeadTransform((vr::EVREye)e)));
            }
            /* back / strafe: slide the tracked position along the head's heading, as if stepping;
               the games move the player with the head tracker (physical walking) */
            static const bool no_walk = getenv("VPIX_NOWALK") && atoi(getenv("VPIX_NOWALK"));   /* rail / ring games */
            if (head_now.valid && !no_walk) {
                const float DZ = 0.25f, SPEED = 1400.f;      /* mm/s at full deflection */
                auto dz = [&](float v) { return fabsf(v) < DZ ? 0.f : (v - (v > 0 ? DZ : -DZ)) / (1 - DZ); };
                float back = sy < 0 ? dz(sy) : 0.f, side = dz(sx);
                if (walk_turn_set(back, side, btn)) back = side = 0;
                float fx = head_now.R[1][0], fy = head_now.R[1][1], fl = sqrtf(fx * fx + fy * fy);
                if (fl > 0.1f && (back != 0 || side != 0)) {
                    fx /= fl; fy /= fl;                      /* right = forward turned -90 degrees */
                    float step = SPEED * (float)(std::min)(dt, 0.1);
                    loco[0] += (fx * back + fy * side) * step;
                    loco[1] += (fy * back - fx * side) * step;
                }
            }
            std::lock_guard<std::mutex> lk(in_mtx);
            if (head_now.valid) input.head = head_now;
            if (ci != vr::k_unTrackedDeviceIndexInvalid && poses[ci].bPoseIsValid) input.hand = vr_to_pose(poses[ci].mDeviceToAbsoluteTracking);
            stick_twist(input.hand);
            {   /* the left controller as a second hand (VPIX_TWO_HANDS, Boxing: either hand punches) */
                vr::TrackedDeviceIndex_t li = sys->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
                input.hand2 = (li != vr::k_unTrackedDeviceIndexInvalid && li != ci && poses[li].bPoseIsValid)
                            ? vr_to_pose(poses[li].mDeviceToAbsoluteTracking) : VPIX_Pose();
            }
            input.buttons = btn; input.coins = vr_coins; input.start = vr_start; input.vr = true; input.gun_pitch = gun_pitch; input.hand_reach = hand_reach; input.hand_zr = hand_zr;
            input.crouch_mm = crouch_now;
            if (head_now.valid) lean_update(head_now);
            input.loco[0] = loco[0] + lean_off[0]; input.loco[1] = loco[1] + lean_off[1]; input.walk_yaw = walk_yaw; input.joy[0] = joy_in[0]; input.joy[1] = joy_in[1];
            arrow_trims();
        } else {
            desktop_input(dt, have ? &cur : NULL);
            head_now = desk_head; eye_pose[0] = desk_head;
        }

        {
            std::lock_guard<std::mutex> lk(mtx);
            if (have_pending) { cur = pending; have_pending = false; have = true;
                static unsigned long nfl = 0; if (getenv("VPIX_FRAMELOG") && nfl++ % 300 == 0)
                    rl("frame %lu inst %u blits %u in_head %d camT=(%.0f %.0f %.0f)", cur.frame, (unsigned)cur.inst.size(), (unsigned)cur.blits.size(), cur.in_head ? 1 : 0, cur.cam_T[0], cur.cam_T[1], cur.cam_T[2]); }
        }
        if (have) hold_update(cur, input.hand);           /* VPIX_HOLD_VIEW: frozen game camera (round over) */
        const VPIX_Frame &vf0 = have ? hold_apply(cur) : cur;   /* only this thread writes input */
        const VPIX_Frame &vf = (have && sys) ? glove_apply(vf0, head_now, input.hand, input.hand2) : vf0;   /* VPIX_BOX_GLOVE_TRACK */
        if (sys) {
            for (int e = 0; e < 2; e++) {
                glBindFramebuffer_(GL_FRAMEBUFFER, eyes[e].fbo);
                float P[16];
                vr::HmdMatrix44_t pm = sys->GetProjectionMatrix((vr::EVREye)e, z_near, 400000.f);
                for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) P[j * 4 + i] = pm.m[i][j];
                rev_proj(P);                                   /* VPIX_DEPTH_FIX: z row -> reversed infinite */
                if (have) {
                    float R[3][3], T[3];
                    eye_camera(vf, eye_pose[e], head_now, R, T);
                    std::lock_guard<std::mutex> lk(mtx);       /* meshes/textures may be added meanwhile */
                    draw_scene(vf, P, R, T, ew, eh);
                    draw_sprites(vf, R, T);
                    draw_flic(vf, P);
                    draw_blits(vf, P);
                } else { glViewport(0, 0, ew, eh); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
                draw_splash(P, splash_a);
                glBindFramebuffer_(GL_FRAMEBUFFER, 0);
                vr::Texture_t tx = {(void *)(uintptr_t)eyes[e].tex, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
                vr::EVRCompositorError ce = vr::VRCompositor()->Submit((vr::EVREye)e, &tx);
                if (ce != vr::VRCompositorError_None && vr_frames < 5) rl("Submit eye %d error %d", e, (int)ce);
            }
            glFlush();
            /* mirror the left eye, letterboxed into the window (the spectator view replaces it) */
            if (!spect) {
            glBindFramebuffer_(GL_READ_FRAMEBUFFER, eyes[0].fbo); glBindFramebuffer_(GL_DRAW_FRAMEBUFFER, 0);
            glViewport(0, 0, win_w, win_h); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
            int dw = win_w, dh = (int)((double)win_w * eh / ew);
            if (dh > win_h) { dh = win_h; dw = (int)((double)win_h * ew / eh); }
            glBlitFramebuffer_(0, 0, ew, eh, (win_w - dw) / 2, (win_h - dh) / 2, (win_w + dw) / 2, (win_h + dh) / 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer_(GL_FRAMEBUFFER, 0);
            }
            vr_frames++;
        } else if (have) {
            float R[3][3], T[3], P[16];
            eye_camera(vf, eye_pose[0], head_now, R, T);
            frustum(vf.fov_deg, win_h ? (float)win_w / win_h : 1.f, P);
            bool fb = rev_z && win_w > 0 && win_h > 0;      /* the window's own depth buffer is 24-bit fixed */
            if (fb && (desk_w != win_w || desk_h != win_h)) {
                free_fb(desk_fb); desk_w = win_w; desk_h = win_h;
                if (!make_eye_fb(desk_fb, desk_w, desk_h)) { rl("desktop depth FBO failed"); free_fb(desk_fb); }
            }
            fb = fb && desk_fb.fbo;
            if (fb) glBindFramebuffer_(GL_FRAMEBUFFER, desk_fb.fbo);
            std::lock_guard<std::mutex> lk(mtx);
            static const char *zf = getenv("VPIX_ZFIGHT"); static unsigned long long next_zf = 0; static int zf_no = 0;
            if (zf && *zf && GetTickCount64() >= next_zf) {
                char zp[512]; snprintf(zp, sizeof(zp), "%s%03d_f%lu.bmp", zf, zf_no++, cur.frame);
                zfight_probe(vf, P, R, T, win_w, win_h, fb, zp); next_zf = GetTickCount64() + 5000;
            }
            draw_scene(vf, P, R, T, win_w, win_h);
            draw_sprites(vf, R, T);
            draw_flic(vf, P);
            draw_blits(vf, P);
            draw_splash(P, splash_a);
            if (fb) {
                glBindFramebuffer_(GL_READ_FRAMEBUFFER, desk_fb.fbo); glBindFramebuffer_(GL_DRAW_FRAMEBUFFER, 0);
                glBlitFramebuffer_(0, 0, desk_w, desk_h, 0, 0, win_w, win_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                glBindFramebuffer_(GL_FRAMEBUFFER, 0);
            }
        } else {
            glViewport(0, 0, win_w, win_h); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
            float P[16]; frustum(60.f, win_h ? (float)win_w / win_h : 1.f, P);
            draw_splash(P, splash_a);
        }
        if (spect && have) {
            if (!spec_w) { spec_w = win_w * spec_ss; spec_h = win_h * spec_ss;
                           if (!make_eye_fb(spec_fb, spec_w, spec_h)) { rl("spectator FBO failed"); spect = false; }
                           else rl("spectator view %dx%d (x%d), smoothing %.2f s", win_w, win_h, spec_ss, spec_tau); }
        }
        if (spect && have) {
            smooth_pose(spec_pose, eye_pose[0], dt, spec_tau);
            float R[3][3], T[3], P[16];
            eye_camera(vf, spec_pose, head_now, R, T);
            frustum(spec_fov > 0 ? spec_fov : vf.fov_deg, (float)spec_w / spec_h, P);
            glBindFramebuffer_(GL_FRAMEBUFFER, spec_fb.fbo);
            {
                std::lock_guard<std::mutex> lk(mtx);
                draw_scene(vf, P, R, T, spec_w, spec_h);
                draw_sprites(vf, R, T);
                draw_flic(vf, P);
                draw_blits(vf, P);
            }
            draw_splash(P, splash_a);
            glBindFramebuffer_(GL_READ_FRAMEBUFFER, spec_fb.fbo); glBindFramebuffer_(GL_DRAW_FRAMEBUFFER, 0);
            glViewport(0, 0, win_w, win_h);
            glBlitFramebuffer_(0, 0, spec_w, spec_h, 0, 0, win_w, win_h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer_(GL_FRAMEBUFFER, 0);
        }
        if (shot && have && GetTickCount64() >= next_shot) {
            char p[512]; snprintf(p, sizeof(p), "%s%03d_f%lu.bmp", shot, shot_no++, cur.frame);
            static const unsigned long long shot_ms = getenv("VPIX_SHOT_MS") ? strtoull(getenv("VPIX_SHOT_MS"), NULL, 10) : 5000;   /* dev: shot interval */
            save_bmp(p); next_shot = GetTickCount64() + shot_ms;
        }
        SwapBuffers(dc);
        if (!swi && !sys) Sleep(10);
    }
    if (sys) vr::VR_Shutdown();
    wglMakeCurrent(NULL, NULL); wglDeleteContext(rc); ReleaseDC(hw, dc); DestroyWindow(hw);
}
#endif

} // namespace

float VPIX_Render_ModelMinZ(uint32_t handle, const float R[3][3], const float S[3], const float T[3]) {
    auto it = meshes.find(handle);          /* read on the HLE thread, the only writer of meshes: no lock (the draw thread holds it for a whole frame) */
    if (it == meshes.end() || it->second->v.size() < 3) return 1e9f;
    const std::vector<float> &v = it->second->v;
    float lo = 1e9f;                                   /* world = transpose(R) * (S v) + T: z = sum_j R[j][2] * S_j * v_j + T_z */
    for (size_t i = 0; i + 2 < v.size(); i += 3) {
        float z = R[0][2] * S[0] * v[i] + R[1][2] * S[1] * v[i + 1] + R[2][2] * S[2] * v[i + 2] + T[2];
        if (z < lo) lo = z;
    }
    return lo;
}

bool VPIX_Render_AddModel(uint32_t handle, const uint8_t *data, size_t len) {
    Mesh *m = parse_bin(data, len);
    if (!m) return false;
    std::lock_guard<std::mutex> lk(mtx);
    meshes[handle].reset(m);
    return true;
}

void VPIX_Render_ModelName(uint32_t handle, const char *file) {
    (void)handle; (void)file;
}

bool VPIX_Render_AddTexture(uint32_t handle, const uint8_t *data, size_t len) {
    Tex *t = parse_tex(data, len);
    if (!t) return false;
    { size_t blue = 0; for (uint32_t v : t->rgba) if (v == 0xFFFF0000u) blue++;
      rl("texture %08X %dx%d blue=%u", handle, t->w, t->h, (unsigned)blue); }
    std::lock_guard<std::mutex> lk(mtx);
    textures[handle].reset(t);
    return true;
}

bool VPIX_Render_AddBitmap(uint32_t handle, const uint8_t *data, size_t len) {
    Tex *t = parse_vst(data, len);
    if (!t) return false;
    std::lock_guard<std::mutex> lk(mtx);
    bitmaps[handle].reset(t);
    return true;
}

void VPIX_Render_SetFlic(const uint32_t *rgba, int w, int h, int x, int y, bool on) {
    std::lock_guard<std::mutex> lk(mtx);
    flic_on = on;
    if (!on || !rgba) return;
    flic_px.assign(rgba, rgba + (size_t)w * h); flic_w = w; flic_h = h; flic_x = x; flic_y = y; flic_dirty = true;
}

void VPIX_Render_SetTexture(uint32_t n, uint32_t flags, uint32_t handle, const float o[3], float su, float sv, uint32_t tflags, uint32_t mask) {
    if (n >= 0x400) return;
    std::lock_guard<std::mutex> lk(mtx);
    TexSlot &t = texslot[n];
    t.flags = flags; t.handle = handle; memcpy(t.o, o, sizeof(t.o)); t.su = su; t.sv = sv; t.tflags = tflags; t.mask = mask; t.set = true;
    rl("texslot %X type=%08X map=%08X flags=%08X mask=%08X", n, flags, handle, tflags, mask);
}

void VPIX_Render_SetMaterial(uint32_t n, int tex, float alpha) {
    if (n >= 32) return;
    std::lock_guard<std::mutex> lk(mtx);
    if (mattex[n] != tex || matalpha[n] != alpha) rl("material %X tex=%X alpha=%.2f", n, tex, alpha);
    mattex[n] = tex; matalpha[n] = alpha;
}

void VPIX_Render_SetMaterialColour(uint32_t n, uint32_t rgb) {
    if (n >= 32) return;
    std::lock_guard<std::mutex> lk(mtx);
    if (matcol[n] != rgb || !mat28) rl("material %X colour %06X (type 0x28 polygons)", n, rgb);
    matcol[n] = rgb; mat28 = true;
}

void VPIX_Render_Submit(const VPIX_Frame &f) {
    std::lock_guard<std::mutex> lk(mtx);
    pending = f; have_pending = true;
}

void VPIX_Input_Get(VPIX_Input &out) {
    std::lock_guard<std::mutex> lk(in_mtx);
    out = input;
    if (!out.head.valid) {                 /* before the render thread's first pass: standing, facing +Y */
        out.head.valid = true; out.head.p[2] = 1700;
        out.hand.valid = true; out.hand.p[1] = 400; out.hand.p[2] = 1200;
    }
}

void VPIX_Haptic(int hand, float seconds, float amplitude) {
#ifdef _WIN32
    haptic_queue(hand, seconds, amplitude);
#endif
}

void VPIX_Render_Start(void) {
#ifdef _WIN32
    if (running) return;
    const char *lb = getenv("VPIX_LOG");
    if (lb && *lb && !rlog) rlog = fopen((std::string(lb) + ".render").c_str(), "w");
    running = true;
    th = std::thread(thread_main);
#endif
}

void VPIX_Render_Stop(void) {
    if (!running) return;
    running = false;
    if (th.joinable()) th.join();
    if (rlog) { fclose(rlog); rlog = NULL; }
}
