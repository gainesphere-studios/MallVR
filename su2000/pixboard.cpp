/*
 *  PIX 1000 processor card model - see pixboard.h.
 */
#include "pixboard.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ---- logging --------------------------------------------------------------------------------- */

void (*pix_log_sink)(const char *msg) = NULL;

void pix_logf(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (pix_log_sink) {
        size_t n = strlen(buf);
        while (n && buf[n - 1] == '\n') buf[--n] = 0;
        pix_log_sink(buf);
    } else fputs(buf, stderr);
}

/* Every trap the firmware takes (tb0/tb1/tcnd/tbnd, divide by zero ...). The PIX firmware's vectors lead to a register-dump
 * handler, so a trap usually means the emulation went wrong somewhere before it. */
static void pix_trap_hook(M88110 *c, unsigned vec, void *user) {
    PixBoard *b = (PixBoard *)user;
    static unsigned logged = 0;
    if (logged++ < 32)
        pix_logf("PIX: card %u CPU %c trap vector %#x at %08x (r1=%08x r2=%08x r3=%08x r29=%08x r30=%08x r31=%08x)\n", b->card_id,
                 c == b->cpu_a ? 'A' : 'B', vec, c->pc - 4, c->r[1], c->r[2], c->r[3], c->r[29], c->r[30], c->r[31]);
}

/* ---- bus glue -------------------------------------------------------------------------------- */

static inline uint32_t be32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static inline void wbe32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }

/* returns a host pointer for plain memory, or NULL for devices */
static inline uint8_t *mem_ptr(PixBoard *b, uint32_t pa) {
    const uint32_t top = pa >> 28;
    if (top == 0x0 || top == 0x1) return &b->dram[pa & (PixBoard::DRAM_SIZE - 1)];
    if ((pa & 0xFFC00000u) == 0x40000000u) return &b->vram[pa & (PixBoard::VRAM_SIZE - 1)];
    return NULL;
}

uint8_t PixBus::rd8(uint32_t pa) { uint8_t *p = mem_ptr(board, pa); return p ? *p : (uint8_t)board->dev_read(which, pa, 1); }
uint16_t PixBus::rd16(uint32_t pa) { uint8_t *p = mem_ptr(board, pa); return p ? (uint16_t)((p[0] << 8) | p[1]) : (uint16_t)board->dev_read(which, pa, 2); }
uint32_t PixBus::rd32(uint32_t pa) { uint8_t *p = mem_ptr(board, pa); return p ? be32(p) : board->dev_read(which, pa, 4); }
void PixBus::wr8(uint32_t pa, uint8_t v) { uint8_t *p = mem_ptr(board, pa); if (p) *p = v; else board->dev_write(which, pa, v, 1); }
void PixBus::wr16(uint32_t pa, uint16_t v) { uint8_t *p = mem_ptr(board, pa); if (p) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; } else board->dev_write(which, pa, v, 2); }
void PixBus::wr32(uint32_t pa, uint32_t v) {
    uint8_t *p = mem_ptr(board, pa);
    if (p) {
        wbe32(p, v);

    } else board->dev_write(which, pa, v, 4);
}
uint32_t PixBus::fetch(uint32_t pa) { return rd32(pa); }

uint32_t pix_pchist[4][0x20000];
void pix_pchist_report(unsigned card) {
    if (!getenv("VPIX_CARD_PCHIST") || card >= 4) return;
    uint32_t top[8] = {0}, idx[8] = {0};
    for (uint32_t i = 0; i < 0x20000; i++) { const uint32_t v = pix_pchist[card][i];
        for (int k = 0; k < 8; k++) if (v > top[k]) { for (int j = 7; j > k; j--) { top[j] = top[j - 1]; idx[j] = idx[j - 1]; } top[k] = v; idx[k] = i; break; } }
    pix_logf("PCHIST card %u: %05x:%u %05x:%u %05x:%u %05x:%u %05x:%u %05x:%u %05x:%u %05x:%u\n", card, idx[0] << 2, top[0], idx[1] << 2, top[1],
             idx[2] << 2, top[2], idx[3] << 2, top[3], idx[4] << 2, top[4], idx[5] << 2, top[5], idx[6] << 2, top[6], idx[7] << 2, top[7]);
    memset(pix_pchist[card], 0, sizeof(pix_pchist[card]));
}

/* ---- board ----------------------------------------------------------------------------------- */

PixBoard::PixBoard() : bus_a(this, 0), bus_b(this, 1) {
    memset(sam, 0, sizeof(sam));
    dram.assign(DRAM_SIZE, 0);
    vram.assign(VRAM_SIZE, 0);
    for (PixBus *bus : {&bus_a, &bus_b}) {
        for (unsigned i = 0; i < DRAM_SIZE >> 22; i++) {
            bus->fast[i] = &dram[(size_t)i << 22];
            bus->fast[(0x10000000u >> 22) + i] = &dram[(size_t)i << 22];   /* uncached alias */
        }
        bus->fast[0x40000000u >> 22] = &vram[0];
    }
    cpu_a = new M88110(&bus_a);
    cpu_b = new M88110(&bus_b);
}

PixBoard::~PixBoard() {
    delete cpu_a;
    delete cpu_b;
}

void PixBoard::host_write8(uint32_t a, uint8_t v) { uint8_t *p = mem_ptr(this, a); if (p) *p = v; }
void PixBoard::host_write16(uint32_t a, uint16_t v) { uint8_t *p = mem_ptr(this, a & ~1u); if (p) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; } }
uint8_t PixBoard::host_read8(uint32_t a) { uint8_t *p = mem_ptr(this, a); return p ? *p : 0xFF; }
uint16_t PixBoard::host_read16(uint32_t a) { uint8_t *p = mem_ptr(this, a & ~1u); return p ? (uint16_t)((p[0] << 8) | p[1]) : 0xFFFF; }

void PixBoard::run_cpu(bool a) {
    /* The CPU fetches its reset branch from address 0 as it is at this moment. */
    M88110 *c = a ? cpu_a : cpu_b;
    const uint32_t w0 = be32(&dram[0]);
    c->reset((w0 >> 26) == 0x30 ? (uint32_t)((int32_t)((w0 & 0x03FFFFFFu) << 6) >> 4) : 0);
    c->trap_hook = pix_trap_hook;
    c->trap_user = this;
    if (a) { a_on = true; find_a_layout(c->pc); find_view_hooks(c->pc); }
    else { b_on = true; find_b_entry_points(); }
}

void PixBoard::start(bool a, bool b) {
    if (b && !b_on) run_cpu(false);
    if (a && !a_on) run_cpu(true);
}

void PixBoard::stop() { a_on = b_on = false; }

/* Locate draw32B/draw16B and the idle poll loop in MAINB without symbols:
 *   mainB loads r27 = draw32B (or.u r27,r0,HI / or r27,r27,LO), conditionally r27 = draw16B, then jsr r27;
 *   the idle loop is  ld r28,r30,$SLOT / cmp r29,r28,r0 / bb1 2,r29,-1  (SFL MAINB 0xaa78; SLOT = 0x100 in 1995
 *   firmware, 0xAC in BOX and 0xB8 in ZONE, 1994); the draw16B choice is  ld.usr r26,[0x2104] / bb0 BIT,r26
 *   (BIT = 9 in 1995 firmware, 8 in 1994). */
void PixBoard::find_b_entry_points() {
    const uint32_t w0 = be32(&dram[0]);
    uint32_t mainb = 0;
    if ((w0 >> 26) == 0x30) mainb = (uint32_t)((int32_t)((w0 & 0x03FFFFFFu) << 6) >> 4);
    mainb_pc = mainb;
    b_lost_logged = false;
    cpu_b->keep_hist = true;   /* cheap enough; used for the "left its code" report */
    if (mainb) { cpu_b->guard_lo = mainb - 0x1000; cpu_b->guard_hi = mainb + 0x20000; cpu_b->guard_hit = false; }
    draw32_addr = draw16_addr = mainb_poll_pc = 0;
    uint32_t hi[32] = {0};
    for (uint32_t pc = mainb; pc < mainb + 0x1000 && pc + 8 < DRAM_SIZE; pc += 4) {
        const uint32_t w = be32(&dram[pc]);
        const unsigned op = w >> 26, d = (w >> 21) & 31, s1 = (w >> 16) & 31;
        if (op == 0x17 && s1 == 0) hi[d] = (w & 0xFFFFu) << 16;
        else if (op == 0x16 && d == 27 && s1 == 27) {
            const uint32_t t = hi[27] | (w & 0xFFFFu);
            if (!draw32_addr) draw32_addr = t; else if (!draw16_addr) draw16_addr = t;
        }
        if ((w & 0xFFFF0000u) == 0x179E0000u && be32(&dram[pc + 4]) == 0xF7BC7C00u && be32(&dram[pc + 8]) == 0xD85DFFFEu && !mainb_poll_pc) {
            mainb_poll_pc = pc;
            slot_off = w & 0xFFFFu;
        }
        /* or r26,r0,$2104 ; ld.usr r26,r26,r0 ; bb0 BIT,r26,... */
        if (w == 0x5B402104u && be32(&dram[pc + 4]) == 0xF75A1500u && (be32(&dram[pc + 8]) & 0xFC1F0000u) == 0xD01A0000u)
            px16_bit = (be32(&dram[pc + 8]) >> 21) & 31;
    }
    disp32 = draw32_addr ? draw32_addr + 0x2C : 0;   /* ld.d r4,r28,$0: record dispatch head */
    disp16 = draw16_addr ? draw16_addr + 0x2C : 0;
    if (disp32 && be32(&dram[disp32]) != 0x109C0000u) disp32 = 0;
    if (disp16 && be32(&dram[disp16]) != 0x109C0000u) disp16 = 0;
    cpu_b->bp[0] = disp32 ? disp32 : 0xFFFFFFFFu;
    cpu_b->bp[1] = disp16 ? disp16 : 0xFFFFFFFFu;
    cpu_b->bp[2] = mainb_poll_pc ? mainb_poll_pc : 0xFFFFFFFFu;
    pix_logf("PIX: dispatch heads %#x %#x\n", disp32, disp16);
    pix_logf("PIX: mainB %#x draw32B %#x draw16B %#x poll %#x slot +%#x 16-bit flag bit %u\n", mainb, draw32_addr,
            draw16_addr, mainb_poll_pc, slot_off, px16_bit);
}

/* MAINA's buffer clear: or.u r29,r29,$4f00 followed by ld r27,r30,$ROWS (8 KB rows per buffer; +0x50 in 1995 firmware,
 * +0x4C in 1994) - DN2 MAINA 0x4d2d8. */
void PixBoard::find_a_layout(uint32_t maina) {
    const uint32_t lo = maina > 0x8000 ? maina - 0x8000 : 0;
    for (uint32_t pc = lo; pc < maina + 0x20000 && pc + 12 < DRAM_SIZE; pc += 4) {
        if (be32(&dram[pc]) != 0x5FBD4F00u) continue;
        for (unsigned k = 1; k <= 2; k++) {
            const uint32_t w = be32(&dram[pc + 4 * k]);
            if ((w & 0xFFFF0000u) == 0x177E0000u) {
                rows_off = w & 0xFFFFu;
                pix_logf("PIX: buffer rows at global +%#x\n", rows_off);
                return;
            }
        }
    }
}

/* MAINA view commands (DAC 0x4bb40 ProcViewMAT, DN2 equivalent):
 *   bsr waitWordFIFO ; mak r29,r29,<7> ; add r2,r2,r29        r2 = view record (128 bytes per view)
 *   then N x (bsr waitLongFIFO ; st r29,r2,$off)                 MAT: 12 floats at 0..0x2c, ROT: 9 at 0..0x20, POS: 3 at 0x24..0x2c
 * The hook address is the instruction after the last store; floats 9..11 (+0x24..+0x2c) are the translation. */
void PixBoard::find_view_hooks(uint32_t maina) {
    view_hook[0] = view_hook[1] = view_hook[2] = 0;
    unsigned nh = 0;
    const uint32_t lo = maina > 0x8000 ? maina - 0x8000 : 0;
    for (uint32_t pc = lo; pc < maina + 0x20000 && pc + 0x80 < DRAM_SIZE; pc += 4) {
        if (be32(&dram[pc]) != 0xF3BDA007u || be32(&dram[pc + 4]) != 0xF442701Du) continue;
        uint32_t p = pc + 8, first = 0xFFFFFFFFu, last = 0;
        unsigned n = 0;
        while ((be32(&dram[p]) >> 26) == 0x32 && (be32(&dram[p + 4]) & 0xFFFF0000u) == 0x27A20000u) {
            const uint32_t off = be32(&dram[p + 4]) & 0xFFFFu;
            if (first == 0xFFFFFFFFu) first = off;
            last = off; n++; p += 8;
        }
        if (((n == 12 && first == 0) || (n == 3 && first == 0x24)) && last == 0x2C && nh < 3) view_hook[nh++] = p;
    }
    for (unsigned k = 0; k < 3; k++) cpu_a->bp[k] = view_hook[k] ? view_hook[k] : 0xFFFFFFFFu;
    pix_logf("PIX: view hooks %#x %#x %#x\n", view_hook[0], view_hook[1], view_hook[2]);
}

/* VRAM row transfers (MAINA swapBuffers/clear code, e.g. DN2 0x4d2d0-0x4d380):
 *   read  0x48xxxxxx / 0x49xxxxxx : read transfer - load the 8 KB VRAM row containing xxxxxx into the SAM
 *   write 0x4Fxxxxxx              : write transfer - store the SAM into that row, bits enabled by the value
 * [inferred from usage: clears load one blank row and write it to every row of a buffer] */
bool PixBoard::vram_transfer(uint32_t pa, bool write, uint32_t v) {
    const uint32_t hi = pa >> 24;
    const uint32_t row = pa & (VRAM_SIZE - 1) & ~0x1FFFu;
    if (!write && (hi == 0x48 || hi == 0x49)) { memcpy(sam, &vram[row], sizeof(sam)); return true; }
    if (write && hi == 0x4F) {
        if (v == 0xFFFFFFFFu) memcpy(&vram[row], sam, sizeof(sam));
        else {
            const uint8_t m[4] = {(uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v};
            for (unsigned i = 0; i < sizeof(sam); i++) vram[row + i] = (uint8_t)((vram[row + i] & ~m[i & 3]) | (sam[i] & m[i & 3]));
        }
        return true;
    }
    return false;
}

uint32_t PixBoard::dev_read(int cpu, uint32_t pa, unsigned size) {
    if (vram_transfer(pa, false, 0)) return 0;
    if ((pa & ~1u) == 0x20000006u || pa == 0x20000004u) {
        uint16_t w;
        if (fifo_pop(w)) { last_fifo = w; fifo_pops++; if (cpu == 0) a_waiting_fifo = false; }
        return size == 4 ? ((uint32_t)last_fifo << 16) : last_fifo;   /* only 16-bit reads seen */
    }
    if (pa == 0x30000000u) {
        /* The video timing generator runs independently of the host, so it is clocked from CPU A's own
         * instruction count; waitVBI needs to see every line value (it tests for equality). */
        const uint32_t line = (uint32_t)((clock / insns_per_line) % lines_per_frame);
        uint32_t v = ((uint32_t)(card_id & 15u) << 25) | ((line & 0x3FFu) << 1);
        if (fifo_level()) { v |= 1u << 29; if (cpu == 0) a_waiting_fifo = false; }
        else if (cpu == 0) {
            /* blocked only if this is the FIFO poll loop: next instruction is bb0 29,rX (waitWordFIFO) */
            const uint32_t next = be32(&dram[cpu_a->pc & (DRAM_SIZE - 1)]);
            a_waiting_fifo = (next & 0xFFE00000u) == 0xD3A00000u;
        }
        return v;
    }
    if ((pa & ~1u) == 0x30000006u) return ctrl;
    unknown_io++;
    if (unknown_log++ < 40) pix_logf("PIX: cpu %c read%u %08x\n", cpu ? 'B' : 'A', size * 8, pa);
    return 0;
}

void PixBoard::dev_write(int cpu, uint32_t pa, uint32_t v, unsigned size) {
    if ((pa & ~1u) == 0x30000006u) { ctrl = (uint16_t)v; return; }
    if (vram_transfer(pa, true, v)) return;
    unknown_io++;
    if (unknown_log++ < 40) pix_logf("PIX: cpu %c write%u %08x = %x\n", cpu ? 'B' : 'A', size * 8, pa, v);
}

/* MallVR dev trace (VPIX_CARD_TRACE=<n>): after n display swaps, record 3 frames of CPU A loads/stores + CPU B draw records to
   <dir>cardtrace<id>.bin (16-byte records: kind 0 A load / 1 A store / 2 B record / 3 swap | size << 8, pc, pa, v) and the DRAM at the
   start to cardtrace<id>_dram.bin. State kept here, not in PixBoard: a DOSBox-X header changes struct packing in some files, so new
   class members gave the files different layouts (crash at card start). */
namespace {
struct CardTrace { const PixBoard *b = NULL; long after = -1; int left = 0; FILE *f = NULL; uint64_t swaps = 0; };
CardTrace card_trace[8];
CardTrace &trace_of(const PixBoard *b) {
    for (CardTrace &t : card_trace) if (t.b == b) return t;
    for (CardTrace &t : card_trace) if (!t.b) { t.b = b; return t; }
    return card_trace[7];
}
void trace_rec(CardTrace &t, uint32_t kind, uint32_t pc, uint32_t pa, uint32_t v) {
    if (!t.f) return;
    const uint32_t r[4] = {kind, pc, pa, v}; fwrite(r, 16, 1, t.f);
}
void trace_mem_a(void *user, uint32_t pc, uint32_t pa, uint32_t v, unsigned size, bool wr) {
    trace_rec(*(CardTrace *)user, (wr ? 1u : 0u) | (size << 8), pc, pa, v);
}
void trace_swap(PixBoard *pb) {
    CardTrace &t = trace_of(pb);
    t.swaps++;
    if (t.after < 0) { const char *e = getenv("VPIX_CARD_TRACE"); t.after = e ? atol(e) : 0x7FFFFFFFL; }
    {   /* VPIX_CARD_SWAPLOG=1: swap count + wall clock every 50 swaps, to pick a VPIX_CARD_TRACE swap from a reference pair's time */
        static const bool sl = getenv("VPIX_CARD_SWAPLOG") != NULL;
        if (sl && t.swaps % 50 == 0) pix_logf("PIX: card %u swap %llu at %lld\n", pb->card_id, (unsigned long long)t.swaps, (long long)time(NULL)); }
    if (t.f) {
        trace_rec(t, 3, 0, pb->display_base, (uint32_t)t.swaps);
        if (--t.left <= 0) { fclose(t.f); t.f = NULL; pb->cpu_a->mem_hook = NULL; pix_logf("PIX: card %u trace done\n", pb->card_id); }
        return;
    }
    if ((long)t.swaps != t.after) return;
    const char *d = getenv("VPIX_CARD_TRACE_DIR");
    std::string base = std::string(d ? d : "H:/Games/Virtuality/_work/") + "cardtrace" + std::to_string(pb->card_id);
    if (FILE *f = fopen((base + "_dram.bin").c_str(), "wb")) { fwrite(pb->dram.data(), 1, pb->dram.size(), f); fclose(f); }
    t.f = fopen((base + ".bin").c_str(), "wb");
    if (!t.f) return;
    t.left = 3;
    trace_rec(t, 3, 0, pb->display_base, (uint32_t)t.swaps);
    pb->cpu_a->mem_user = &t; pb->cpu_a->mem_hook = trace_mem_a;
    pix_logf("PIX: card %u trace start at swap %llu\n", pb->card_id, (unsigned long long)t.swaps);
}
} // namespace

uint64_t PixBoard::run(uint64_t n) {
    /* Interleave CPU A and CPU B in slices. A does not run while it is blocked on an empty FIFO;
     * B does not run while it polls an empty draw-list slot. Returns instructions executed by both;
     * sets idle when neither CPU has work. */
    uint64_t done = 0;
    const uint64_t slice = 2000;
    idle = false;
    while (done < n) {
        bool a_ran = false, b_ran = false;
        if (a_on && !(a_waiting_fifo && !fifo_level())) {
            a_waiting_fifo = false;
            const uint64_t k = cpu_a->run(slice);
            {   /* MallVR dev (VPIX_CARD_PCHIST=1): where CPU A spends its time (card 0 only), reported by pix_pchist_report() */
                extern uint32_t pix_pchist[4][0x20000]; static const bool on = getenv("VPIX_CARD_PCHIST") != NULL;
                if (on && card_id < 4) pix_pchist[card_id][(cpu_a->pc >> 2) & 0x1FFFF]++; }
            done += k;
            clock += k;
            a_ran = true;
            if (eye_shift != 0.0f && cpu_a->pc && (cpu_a->pc == view_hook[0] || cpu_a->pc == view_hook[1] || cpu_a->pc == view_hook[2])) {
                /* a view was just stored at r2: move the camera sideways (view-space x translation) */
                const uint32_t a = (cpu_a->r[2] + 0x24u) & (DRAM_SIZE - 1);
                uint32_t bits = be32(&dram[a]);
                float x;
                memcpy(&x, &bits, 4);
                x += eye_shift;   /* measured: the stored x grows when the camera moves right */
                memcpy(&bits, &x, 4);
                dram[a] = (uint8_t)(bits >> 24); dram[a + 1] = (uint8_t)(bits >> 16); dram[a + 2] = (uint8_t)(bits >> 8); dram[a + 3] = (uint8_t)bits;
                view_hits++;
            }
            const uint32_t disp = be32(&dram[0x200C]);
            if (disp != display_base) {
                trace_swap(this);
                /* The 1994 firmware (Boxing) switches the display when it hands the list to B, before B has drawn it; the
                   video card shows the new buffer from the next refresh, by then B is done. Grab when B goes idle. */
                if (grab_pending) grab_frame();
                display_base = disp;
                grab_pending = true;
                /* 1995 firmware (slot +0x100: DN2, SFL, GHOST) swaps after B is done; there, grab at once so the cards of a
                   split picture (DN2 Solo: two bands) stay on the same frame */
                if (!b_on || !mainb_poll_pc || slot_off == 0x100) { grab_pending = false; grab_frame(); }
            }
            if (cpu_a->unknown_count) {
                pix_logf("PIX: A unknown opcode %08x at %08x\n", cpu_a->last_unknown_inst, cpu_a->last_unknown_pc);
                cpu_a->unknown_count = 0;
            }
        }
        if (b_on) {
            const uint64_t b0 = cpu_b->icount;
            uint64_t left = slice;
            while (left) {
                const uint32_t bpc = cpu_b->pc;
                if (bpc == mainb_poll_pc && mainb_poll_pc && be32(&dram[0x7000 + slot_off]) == 0) {   /* idle */
                    if (grab_pending) { grab_pending = false; grab_frame(); }
                    break;
                }
                if (bpc && (bpc == disp32 || bpc == disp16)) {
                    /* close the previous record's statistics */
                    if (cur_type >= 0) {
                        TypeStat &t = type_stats[cur_type & 255];
                        t.insns += cpu_b->icount - cur_insn0;
                        t.bytes += cpu_b->r[28] - cur_ptr;
                    }
                    const uint32_t ptr = cpu_b->r[28];
                    const int type = (int8_t)dram[ptr & (DRAM_SIZE - 1)];
                    { CardTrace &t = trace_of(this); if (t.f) trace_rec(t, 2, bpc, ptr, be32(&dram[ptr & (DRAM_SIZE - 1)])); }
                    cur_type = type; cur_insn0 = cpu_b->icount; cur_ptr = ptr;
                    type_stats[type & 255].count++;
                    if (type == 0) cur_type = -1;
                    else if (hle_b) {
                        extern bool PixRaster_Record(PixBoard *b, M88110 *cpu, int type, bool bpp32);
                        if (compare_every && (type_stats[type & 255].count % compare_every) == 1) {
                            /* run the HLE on a copy, then the original handler, and diff VRAM */
                            std::vector<uint8_t> before(vram);
                            uint32_t regs[32];
                            memcpy(regs, cpu_b->r, sizeof(regs));
                            if (PixRaster_Record(this, cpu_b, type, bpc == disp32)) {
                                std::vector<uint8_t> hle(vram);
                                const uint32_t hle_r28 = cpu_b->r[28];
                                vram.swap(before);
                                memcpy(cpu_b->r, regs, sizeof(regs));
                                cpu_b->pc = bpc;
                                const uint64_t i0 = cpu_b->icount;
                                do { cpu_b->run(1u << 30); } while (cpu_b->pc != disp32 && cpu_b->pc != disp16 && cpu_b->pc != mainb_poll_pc);
                                size_t diff = 0, first = 0;
                                for (size_t i = 0; i < vram.size(); i++) if (vram[i] != hle[i]) { if (!diff) first = i; diff++; }
                                compare_runs++;
                                if (diff || hle_r28 != cpu_b->r[28]) {
                                    compare_fail++;
                                    if (compare_fail < 20)
                                        pix_logf("PIX compare: type %#x rec %08x: %zu bytes differ (first %#zx hle %02x lle %02x), r28 hle %08x lle %08x, lle %llu insns\n",
                                                type & 255, regs[28], diff, first, hle[first], vram[first], hle_r28, cpu_b->r[28],
                                                (unsigned long long)(cpu_b->icount - i0));
                                }
                                cur_type = -1;
                                continue;
                            }
                        }
                        if (PixRaster_Record(this, cpu_b, type, bpc == disp32)) {
                            type_stats[type & 255].hle++;
                            cur_type = -1;
                            /* Resume where the original handlers return: two instructions before the dispatch head
                             * (or.u r6 / or r6 reload the jump-table base, which handlers - and the HLE - clobber). */
                            if (be32(&dram[(bpc - 8) & (DRAM_SIZE - 1)]) == 0x5CC00000u) cpu_b->pc = bpc - 8;
                            continue;
                        }
                    }
                }
                const uint64_t k = cpu_b->run(left);
                left -= k < left ? k : left;
                if (!k || cpu_b->guard_hit) break;
            }
            if (cpu_b->unknown_count) {
                pix_logf("PIX: B unknown opcode %08x at %08x\n", cpu_b->last_unknown_inst, cpu_b->last_unknown_pc);
                cpu_b->unknown_count = 0;
            }
            done += cpu_b->icount - b0;
            /* CPU B outside its own code (mainB .. mainB + 0x20000): report the draw-list record it was last given */
            if (cpu_b->guard_hit && !b_lost_logged) {
                b_lost_logged = true;
                b_on = false;              /* freeze CPU B so its state can be inspected */
                const uint32_t p = cur_ptr & (DRAM_SIZE - 1);
                pix_logf("PIX: card %u CPU B left its code: pc %08x r1 %08x; last record type %d at %08x: %08x %08x %08x %08x; list slot %08x\n",
                         card_id, cpu_b->pc, cpu_b->r[1], cur_type, cur_ptr, be32(&dram[p]), be32(&dram[p + 4]), be32(&dram[p + 8]),
                         be32(&dram[p + 12]), be32(&dram[0x7000 + slot_off]));
                pix_logf("PIX: card %u B regs r4 %08x r5 %08x r6 %08x r28 %08x r29 %08x\n", card_id, cpu_b->r[4], cpu_b->r[5],
                         cpu_b->r[6], cpu_b->r[28], cpu_b->r[29]);
                for (unsigned k = 0; k < 64; k++) {
                    const unsigned j = (cpu_b->hist_i + k) & 63;
                    pix_logf("PIX:   c%u %02u %08x %08x\n", card_id, k, cpu_b->hist_pc[j], cpu_b->hist_inst[j]);
                }
            }
            b_ran = cpu_b->icount != b0;
        }
        if (!a_ran && !b_ran) { idle = true; break; }
    }
    return done;
}

void PixBoard::hle_draw(bool bpp32) { (void)bpp32; }


bool PixBoard::render_frame(PixFrame &out) {
    const uint32_t stride = be32(&dram[0x7040]);
    const uint32_t rows = be32(&dram[0x7000 + rows_off]);
    if ((display_base & 0xFFC00000u) != 0x40000000u || stride == 0 || stride > 8192 || rows == 0 || rows > 512) return false;
    const bool px16 = (be32(&dram[0x2104]) >> px16_bit) & 1;
    const unsigned bpp = px16 ? 2 : 4;
    /* the screen is narrower than the buffer line (768 pixels): 2 x (centre x + 1), the centre x being the firmware
       global just before the buffer-rows one (0x7048 = 175 in DN2 / DAC / Ghost Train, 0x7044 in Boxing: 352 pixels).
       Drawing beyond it (Ghost Train's cave) is not shown on the real Visette. */
    out.width = stride / bpp;
    const uint32_t cx = be32(&dram[0x7000 + rows_off - 8]);
    if (cx >= 16 && 2 * (cx + 1) < out.width) out.width = 2 * (cx + 1);
    out.height = rows * 0x2000u / stride;
    if (out.height > 1024) out.height = 1024;
    out.band_lo = be32(&dram[0x210C]);
    out.band_hi = be32(&dram[0x2110]);
    if (out.band_hi <= out.band_lo || out.band_hi >= out.height) { out.band_lo = 0; out.band_hi = out.height - 1; }
    out.pixels.resize((size_t)out.width * out.height);
    for (unsigned y = 0; y < out.height; y++) {
        const uint32_t row = (display_base + y * stride) & (VRAM_SIZE - 1);
        uint32_t *dst = &out.pixels[(size_t)y * out.width];
        for (unsigned x = 0; x < out.width; x++) {
            const uint8_t *p = &vram[(row + x * bpp) & (VRAM_SIZE - 1)];
            if (px16) {
                const unsigned v = ((unsigned)p[0] << 8) | p[1];
                /* bits 11..8 = blue, 7..4 = green, 3..0 = red (red and blue were swapped on screen) */
                dst[x] = (((v >> 8) & 15u) * 17u) | (((v >> 4) & 15u) * 17u << 8) | ((v & 15u) * 17u << 16);
            } else dst[x] = ((uint32_t)p[3] << 16) | ((uint32_t)p[2] << 8) | p[1];   /* xBGR in VRAM */
        }
    }
    return true;
}

void PixBoard::grab_frame() {
    if (frame_cb) frame_cb(this, frame_user);
}

void PixBoard::report(FILE *f) {
    fprintf(f, "PIX board: A pc=%08x (%llu insns)  B pc=%08x (%llu insns)  fifo pops %llu  draws %llu  unknown io %llu  fifo left %zu\n",
            cpu_a->pc, (unsigned long long)cpu_a->icount, cpu_b->pc, (unsigned long long)cpu_b->icount,
            (unsigned long long)fifo_pops, (unsigned long long)draws, (unsigned long long)unknown_io, fifo_level());
    fprintf(f, "HLE compare: %llu checked, %llu mismatched\n", (unsigned long long)compare_runs, (unsigned long long)compare_fail);
    fprintf(f, "draw-list records: type count avg_bytes avg_B_insns hle\n");
    for (int t = -128; t < 128; t++) {
        const TypeStat &s = type_stats[t & 255];
        if (!s.count) continue;
        const uint64_t lle = s.count - s.hle;
        fprintf(f, "  %4d (%#04x) %10llu %8.1f %10.1f %llu\n", t, t & 255, (unsigned long long)s.count,
                lle ? (double)s.bytes / lle : 0.0, lle ? (double)s.insns / lle : 0.0, (unsigned long long)s.hle);
    }
}
