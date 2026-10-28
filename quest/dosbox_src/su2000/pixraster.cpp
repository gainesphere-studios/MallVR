/*
 *  HLE replacement for CPU B's rasteriser (draw32B / draw16B in MAINB.OUT).
 *
 *  CPU B walks a draw list built by CPU A. Every record starts with an 8-byte header whose signed top
 *  byte is the record type; draw32B jumps through a table indexed by type. PixBoard stops CPU B at the
 *  dispatch head and calls PixRaster_Record(). A record implemented here is executed in C++ on CPU B's
 *  register file and memory (so later records see the same state as with the original code); anything
 *  else returns false and the interpreter runs the original handler. Record formats:
 *  su2000/docs/findings/drawlist.md.  Handlers ported from DN2 MAINB.OUT (1995-02-21) draw32B.
 */
#include "pixboard.h"

#include <string.h>
#include <stdio.h>

namespace {

inline uint32_t rd32(PixBoard *b, uint32_t pa) {
    const uint8_t *p;
    if ((pa >> 28) <= 1) p = &b->dram[pa & (PixBoard::DRAM_SIZE - 1)];
    else if ((pa & 0xFFC00000u) == 0x40000000u) p = &b->vram[pa & (PixBoard::VRAM_SIZE - 1)];
    else return 0;
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

inline void wr32(PixBoard *b, uint32_t pa, uint32_t v) {
    uint8_t *p;
    if ((pa >> 28) <= 1) p = &b->dram[pa & (PixBoard::DRAM_SIZE - 1)];
    else if ((pa & 0xFFC00000u) == 0x40000000u) p = &b->vram[pa & (PixBoard::VRAM_SIZE - 1)];
    else return;
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

/* Pixel blend as written in draw32B with the graphics unit:
 *   punpk.b x20,src ; pmul x20,r20,a ; punpk.b x22,dst ; pmul x22,r22,(256-a) ; padd.h ; ppack.32.h
 * i.e. per byte: ((s*a & 0xFFFF) + (d*(256-a) & 0xFFFF)) & 0xFFFF, then the high byte. Same arithmetic
 * as M88110's PUNPK/PMUL/PADD/PPACK implementation. */
inline uint32_t blend(uint32_t s, uint32_t d, uint32_t a) {
    const uint32_t na = 256u - a;
    uint32_t out = 0;
    for (unsigned i = 0; i < 4; i++) {
        const uint32_t sb = (s >> (8 * i)) & 0xFF, db = (d >> (8 * i)) & 0xFF;
        const uint32_t f = (((sb * a) & 0xFFFF) + ((db * na) & 0xFFFF)) & 0xFFFF;
        out |= (f >> 8) << (8 * i);
    }
    return out;
}

/* ---- type 0x0A: scaled bitmap (DN2 draw32B 0xd0c8-0xd590, main path) ------------------------ */
bool type0a_bitmap32(PixBoard *b, M88110 *c) {
    uint32_t *r = c->r;
    const uint32_t p = r[28];
    const uint32_t flags = rd32(b, p + 0x28), mode = rd32(b, p + 0x2C);
    if (!((mode & 6u) || !(mode & 1u))) return false;        /* alternate path at 0xd594 */
    if (flags & 0x0F000000u) return false;                     /* 16-bit / four-colour / other sources */
    const uint32_t src0 = rd32(b, p + 4) + rd32(b, p + 0x30);
    const uint32_t sstride = rd32(b, p + 8);
    const uint32_t ustep = rd32(b, p + 0x10), vstep = rd32(b, p + 0x14);
    const uint32_t pos = rd32(b, p + 0x18), size = rd32(b, p + 0x1C);
    const uint32_t key = rd32(b, p + 0x20), alpha = rd32(b, p + 0x24);
    const uint32_t stride = r[2], base = r[3];
    const int32_t width = (int32_t)size >> 16;
    const uint32_t lines = size & 0xFFFFu;
    uint32_t dst = (uint32_t)(((int32_t)pos >> 16) << 2) + (pos & 0xFFFFu) * stride + base;
    uint32_t v = (vstep & 0x80000000u) ? 0xFFFFu : 0u;
    const bool m_key = flags & (1u << 16), m_blend = flags & (1u << 17), m_const = flags & (1u << 23);
    const uint32_t cpart_na = 256u - alpha;

    auto pixel = [&](uint32_t s, uint32_t dptr) {
        if (m_key && m_blend) { if (s != key) wr32(b, dptr, blend(s, rd32(b, dptr), alpha)); return; }
        if (m_key) { if (s != key) wr32(b, dptr, s); return; }
        if (m_blend) { wr32(b, dptr, blend(s, rd32(b, dptr), alpha)); return; }
        if (m_const) { wr32(b, dptr, blend(s, key, alpha)); return; }
        wr32(b, dptr, s);
    };
    (void)cpart_na;
    for (uint32_t line = 0; line < lines; line++) {
        const uint32_t srow = (uint32_t)((int32_t)v >> 16) * sstride + src0;
        uint32_t u = 0;
        for (int32_t x = 0; x < width; x++) {
            const uint32_t s = rd32(b, srow + (uint32_t)(((int32_t)u >> 16) << 2));
            pixel(s, dst + (uint32_t)x * 4u);
            u += ustep;
        }
        v += vstep;
        dst += stride;
    }
    /* registers as the original leaves them (the ones later records can observe) */
    r[4] = flags; r[5] = src0; r[6] = sstride; r[8] = ustep; r[9] = vstep;
    r[12] = key; r[13] = alpha; r[14] = (uint32_t)width; r[15] = lines; r[16] = lines;
    r[17] = v; r[7] = dst; r[20] = rd32(b, p + 0x30); r[1] = mode;
    r[28] = p + 0x38;
    return true;
}

} // namespace

bool PixRaster_Record(PixBoard *b, M88110 *cpu, int type, bool bpp32) {
    if (!bpp32) return false;
    switch (type) {
        case 0x0A: return type0a_bitmap32(b, cpu);
        default: return false;
    }
}
