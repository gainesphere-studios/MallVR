/*
 *  Virtuality SU2000 hardware probe (PIX1000 / VID1000 / InsideTrak / Format card).
 *
 *  Stage 1: a logging stub. Claims the SU2000's I/O ports and the PIX shared-memory
 *  window, answers "ready" where known (port 0x300/0x360 -> 0xCF, per SailorSat's ETS
 *  capture), echoes back register writes, and records everything the game sends so the
 *  protocol can be decoded.
 *
 *  Enabled only when the environment variable VPIX_LOG is set to a writable file path.
 *  Broadcast-FIFO words (port 0x320) are also streamed raw to VPIX_LOG + ".fifo".
 */

#include "dosbox.h"
#include "inout.h"
#include "mem.h"
#include "paging.h"
#include "setup.h"
#include "control.h"
#include "logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>

static FILE *vlog = NULL;
static FILE *vfifo = NULL;
static uint8_t regs[0x400];            /* last value written to each port 0x000-0x3FF */
static uint8_t pixmem[0x10000];        /* 0xD0000-0xDFFFF PIX shared window */
static uint8_t fmtmem[0x4000];         /* 0xE0000-0xE3FFF format card window */
static unsigned long long nlines = 0;
static const unsigned long long MAX_LINES = 3000000;
static FILE *vmemw = NULL;   /* raw memory-write stream: [u32 addr][u16 pagereg][u16 len][bytes] */
static PhysPt run_start = 0, run_next = 0; static const char *run_name = NULL;
static uint8_t run_buf[0x10000]; static size_t run_len = 0;
static void flush_run();

/* collapse runs of identical accesses so polling loops don't flood the log */
static char last_line[128];
static unsigned long repeat = 0;

static void vemit_raw(const char *line);
static void vemit(const char *line) { flush_run(); vemit_raw(line); }
static void vemit_raw(const char *line) {
    if (!vlog) return;
    if (strcmp(line, last_line) == 0) { repeat++; return; }
    if (repeat) { fprintf(vlog, "   (x%lu more)\n", repeat); repeat = 0; }
    if (nlines++ < MAX_LINES) fprintf(vlog, "%s\n", line);
    strncpy(last_line, line, sizeof(last_line) - 1);
}

static const char *portname(Bitu p) {
    if (p >= 0x300 && p < 0x310) return "PIX1";
    if (p >= 0x360 && p < 0x370) return "PIX2";
    if (p >= 0x340 && p < 0x350) return "VID";
    if (p >= 0x320 && p < 0x330) return "BCST";
    if (p >= 0x270 && p < 0x280) return "TRAK";
    if (p >= 0x210 && p < 0x220) return "FMT";
    return "?";
}

void VPIX_HLE_Arm(void); void VPIX_HLE_Init(const char *); void VPIX_HLE_Shutdown(void);
#include "vpix_net.h"

static Bitu vpix_read(Bitu port, Bitu iolen) {
    if (port >= 0x300 && port < 0x370) VPIX_HLE_Arm();
    Bitu v;
    if ((port == 0x300 || port == 0x360) && iolen == 1) v = 0xCF;  /* ready */
    else {
        v = regs[port & 0x3FF];
        if (iolen >= 2) v |= (Bitu)regs[(port + 1) & 0x3FF] << 8;
        if (iolen >= 4) v |= ((Bitu)regs[(port + 2) & 0x3FF] << 16) | ((Bitu)regs[(port + 3) & 0x3FF] << 24);
    }
    char b[128];
    snprintf(b, sizeof(b), "R%u %03X %-4s -> %0*X", (unsigned)iolen, (unsigned)port, portname(port), (int)iolen * 2, (unsigned)v);
    vemit(b);
    return v;
}

static void vpix_write(Bitu port, Bitu val, Bitu iolen) {
    if (port >= 0x300 && port < 0x370) VPIX_HLE_Arm();
    for (Bitu i = 0; i < iolen; i++) regs[(port + i) & 0x3FF] = (uint8_t)(val >> (8 * i));
    if (port >= 0x320 && port < 0x330 && vfifo) {
        uint16_t w = (uint16_t)val;
        fwrite(&w, 2, 1, vfifo);
        if (iolen == 4) { w = (uint16_t)(val >> 16); fwrite(&w, 2, 1, vfifo); }
    }
    char b[128];
    snprintf(b, sizeof(b), "W%u %03X %-4s <- %0*X", (unsigned)iolen, (unsigned)port, portname(port), (int)iolen * 2, (unsigned)val);
    vemit(b);
}

class VPIX_MemHandler : public PageHandler {
public:
    uint8_t *mem; PhysPt base; size_t size; const char *name;
    VPIX_MemHandler(uint8_t *m, PhysPt b, size_t s, const char *n)
        : PageHandler(PFLAG_NOCODE), mem(m), base(b), size(s), name(n) {}
    uint8_t readb(PhysPt addr) override {
        PhysPt o = (addr - base) % size;
        char bl[128]; snprintf(bl, sizeof(bl), "M%s R %05X -> %02X", name, (unsigned)addr, mem[o]); vemit(bl);
        return mem[o];
    }
    void writeb(PhysPt addr, uint8_t val) override {
        PhysPt o = (addr - base) % size;
        mem[o] = val;
        if (run_len && (addr != run_next || run_name != name || run_len >= sizeof(run_buf))) flush_run();
        if (!run_len) { run_start = addr; run_name = name; }
        run_buf[run_len++] = val; run_next = addr + 1;
    }
    uint16_t readw(PhysPt a) override { return (uint16_t)(readb(a) | (readb(a + 1) << 8)); }
    uint32_t readd(PhysPt a) override { return (uint32_t)readw(a) | ((uint32_t)readw(a + 2) << 16); }
    void writew(PhysPt a, uint16_t v) override { writeb(a, (uint8_t)v); writeb(a + 1, (uint8_t)(v >> 8)); }
    void writed(PhysPt a, uint32_t v) override { writew(a, (uint16_t)v); writew(a + 2, (uint16_t)(v >> 16)); }
};

static void flush_run() {
    if (!run_len) return;
    size_t n = run_len; run_len = 0;
    char bl[160];
    unsigned pg = regs[0x301] | (regs[0x302] << 8); unsigned live = (regs[0x300] & 0x10 ? 1 : 0) | (regs[0x360] & 0x10 ? 2 : 0);
    if (n <= 8) {
        int k = snprintf(bl, sizeof(bl), "M%s W %05X <-", run_name, (unsigned)run_start);
        for (size_t i = 0; i < n; i++) k += snprintf(bl + k, sizeof(bl) - k, " %02X", run_buf[i]);
    } else {
        snprintf(bl, sizeof(bl), "M%s W %05X-%05X (%u bytes) page=%04X live=%u first: %02X %02X %02X %02X", run_name,
                 (unsigned)run_start, (unsigned)(run_start + n - 1), (unsigned)n, pg, live, run_buf[0], run_buf[1], run_buf[2], run_buf[3]);
    }
    vemit_raw(bl);
    if (vmemw) { uint32_t a = (uint32_t)run_start; uint16_t p16 = (uint16_t)pg, l16 = (uint16_t)n;
        fwrite(&a, 4, 1, vmemw); fwrite(&p16, 2, 1, vmemw); fwrite(&l16, 2, 1, vmemw); fwrite(run_buf, 1, n, vmemw); }
}

/* The PIX window at 0xD0000 maps 64 KB of a card's own memory. Card address =
   (page << 8) + window offset, page = port base+1 | base+2 << 8. A card's window is
   live while bit 4 of its control port (base+0) is set; both cards can be live at
   once (broadcast upload). */
#include <vector>
struct PixCard { Bitu base; std::vector<uint8_t> mem; };
static PixCard cards[2];
static inline bool card_live(int c) { return (regs[cards[c].base & 0x3FF] & 0x10) != 0; }
static inline uint32_t card_addr(int c, PhysPt addr) {
    Bitu b = cards[c].base;
    uint32_t page = regs[(b + 1) & 0x3FF] | (regs[(b + 2) & 0x3FF] << 8);
    return ((page << 8) + (uint32_t)(addr - 0xD0000)) & 0xFFFFFF;
}
class VPIX_PixWindow : public VPIX_MemHandler {
public:
    VPIX_PixWindow() : VPIX_MemHandler(pixmem, 0xD0000, sizeof(pixmem), "PIX") {}
    uint8_t readb(PhysPt addr) override {
        uint8_t v = 0xFF; int which = -1;
        for (int c = 0; c < 2; c++) if (card_live(c)) { v = cards[c].mem[card_addr(c, addr)]; which = c; break; }
        char bl[128]; snprintf(bl, sizeof(bl), "MPIX R %05X card%d@%06X -> %02X", (unsigned)addr, which + 1,
                               which >= 0 ? (unsigned)card_addr(which, addr) : 0u, v); vemit(bl);
        return v;
    }
    void writeb(PhysPt addr, uint8_t val) override {
        for (int c = 0; c < 2; c++) if (card_live(c)) cards[c].mem[card_addr(c, addr)] = val;
        VPIX_MemHandler::writeb(addr, val);   /* logging / run capture */
    }
};

static VPIX_MemHandler *pix_handler = NULL;
static VPIX_MemHandler *fmt_handler = NULL;

static void VPIX_Shutdown(Section *) {
    VPIX_HLE_Shutdown();
    VPIX_NET_Shutdown();
    flush_run();
    if (vmemw) { fclose(vmemw); vmemw = NULL; }
    if (vlog) {
        if (repeat) fprintf(vlog, "   (x%lu more)\n", repeat);
        fclose(vlog); vlog = NULL;
    }
    if (vfifo) { fclose(vfifo); vfifo = NULL; }
    const char *p = getenv("VPIX_LOG");
    if (p && !getenv("VPIX_PLAY")) {             /* card memory dumps are for debugging only */
        for (int c = 0; c < 2; c++) {
            std::string d = std::string(p) + ".card" + std::to_string(c + 1);
            FILE *f = fopen(d.c_str(), "wb"); if (f) { fwrite(cards[c].mem.data(), 1, cards[c].mem.size(), f); fclose(f); }
        }
    }
}

void VPIX_RemapMemory(void) {
    if (!pix_handler) return;
    MEM_RegisterHandler(0xD0, pix_handler, 0x10);   /* 0xD0000-0xDFFFF */
    MEM_RegisterHandler(0xE0, fmt_handler, 0x4);    /* 0xE0000-0xE3FFF */
    VPIX_NET_RemapMemory();                         /* pod network card RAM 0xC8000 (VPIX_NET) */
}

void VPIX_Init() {
    const char *p = getenv("VPIX_LOG");
    if (!p || !*p) return;
    vlog = fopen(p, "w");
    if (!getenv("VPIX_PLAY")) {                 /* VPIX_PLAY=1: packaged play, no raw card-traffic dumps */
        std::string fp = std::string(p) + ".fifo";
        vfifo = fopen(fp.c_str(), "wb");
        std::string mp = std::string(p) + ".memw";
        vmemw = fopen(mp.c_str(), "wb");
    }
    if (vlog) fprintf(vlog, "VPIX probe started\n");
    VPIX_NET_Init(p);                           /* before the hook table loads: it drops the NET_ stubs */
    VPIX_HLE_Init(p);

    static const Bitu ranges[][2] = {
        {0x300, 0x10}, {0x360, 0x10}, {0x340, 0x10}, {0x320, 0x10}, {0x270, 0x10}, {0x210, 0x10}
    };
    for (auto &r : ranges) {
        IO_RegisterReadHandler(r[0], vpix_read, IO_MA, r[1]);
        IO_RegisterWriteHandler(r[0], vpix_write, IO_MA, r[1]);
    }
    memset(pixmem, 0, sizeof(pixmem));
    memset(fmtmem, 0xFF, sizeof(fmtmem));   /* no Format C card: empty bus (PAC's CTRL lib otherwise waits on it forever) */
    cards[0].base = 0x300; cards[1].base = 0x360;
    for (auto &c : cards) c.mem.assign(0x1000000, 0);
    pix_handler = new VPIX_PixWindow();
    fmt_handler = new VPIX_MemHandler(fmtmem, 0xE0000, sizeof(fmtmem), "FMT");
    VPIX_RemapMemory();
    AddExitFunction(AddExitFunctionFuncPair(VPIX_Shutdown), true);
}
