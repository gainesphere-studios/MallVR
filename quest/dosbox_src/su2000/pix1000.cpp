/*
 *  Expality PIX 1000 (Virtuality SU2000 graphics) - Milestone 1 logging stub.
 *
 *  Model derived from the SFL.EXE host library (symbols from SFL.SYM) - see
 *  su2000/docs/PROTOCOL.md for the evidence behind every register below.
 *
 *  Processor card (one per video channel, 2x 88110 "A" and "B" + DRAM), I/O base P:
 *    P+0  write: control. bit0 = run CPU A, bit1 = run CPU B, bit4 = map board memory into
 *                the host window, bit5 = (cleared by PIXI_DisableProcCards; meaning unknown).
 *         read : status. bit7 = FIFO not full, bit6 = FIFO not half full, bit5 = FIFO not empty
 *                (all active low as used by PIXI_FIFOStatus). Low bits: unknown.
 *    P+1  write: window page, board address bits 31..24
 *    P+2  write: window page, board address bits 23..16
 *    P+3  write: 1 = reset (CPU A?)  [inferred, PIXI_ResetProcCards]
 *    P+4  write: 1 = reset (CPU B?)  [inferred]
 *  Host memory window: procMem (0xD0000), 64KB, maps board address page<<16 | offset.
 *    The board is big-endian; the host accesses it as 16-bit halfwords, high half first.
 *  Broadcast FIFO: 16-bit OUT to the FIFO port reaches every selected card's CPU A.
 *
 *  Two modes:
 *   - emulation (default): each processor card is a PixBoard (pixboard.cpp) running the uploaded
 *     MAINA/MAINB firmware - CPU A in the MC88110 interpreter, CPU B's rasteriser in HLE with
 *     interpreter fallback - on its own host thread. FIFO flags and board memory are real.
 *   - stub (pix emulation = false): board RAM only, faked boot handshake, always-empty FIFO.
 *  Every access is optionally logged to the bus trace either way.
 */

#include "dosbox.h"
#include "inout.h"
#include "mem.h"
#include "paging.h"
#include "logging.h"
#include "regs.h"
#include "cpu.h"
#include "pic.h"
#include "su2000.h"
#include <string.h>

#include "pixboard.h"

#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

namespace {

const uint32_t BOARD_RAM = 16u * 1024u * 1024u;   /* covers 8MB DRAM + 4MB VRAM decode */

struct ProcCard {
    uint32_t io = 0;
    PixBoard *board = NULL;                 /* emulation mode */
    std::thread *thread = NULL;
    std::mutex lock;                        /* held by the worker while it runs the CPUs */
    std::atomic<bool> quit{false};
    std::mutex frame_lock;
    PixFrame frame;                         /* last displayed frame (worker -> main thread) */
    std::atomic<uint32_t> frame_seq{0};
    PixFrame hist[4];                       /* the last frames by sequence number (hist[seq & 3]), for split pictures */
    uint32_t hist_seq[4] = { 0, 0, 0, 0 };
    uint8_t  ctrl = 0;
    uint8_t  page_hi = 0, page_lo = 0;
    uint8_t  reg[8] = {};
    std::vector<uint8_t> ram;
    std::unordered_map<uint32_t, uint8_t> high;    /* board addresses >= 16MB (registers etc.) */

    uint32_t page() const { return ((uint32_t)page_hi << 24u) | ((uint32_t)page_lo << 16u); }
    uint8_t &at(uint32_t a) {
        if (board) {
            if (a < PixBoard::DRAM_SIZE) return board->dram[a];
            if ((a & 0xFFC00000u) == 0x40000000u) return board->vram[a & (PixBoard::VRAM_SIZE - 1)];
            return high[a];
        }
        if (a < BOARD_RAM) return ram[a];
        return high[a];
    }
    void put16(uint32_t a, uint16_t v) { at(a) = (uint8_t)(v >> 8u); at(a + 1) = (uint8_t)v; }
    uint16_t get16(uint32_t a) { return (uint16_t)((at(a) << 8u) | at(a + 1)); }
};

ProcCard cards[4];
ProcCard twins[4];                          /* stereo: right-eye copy of each card, fed the same host writes */
bool stereo = false;
float stereo_sep = 65.0f;
bool emulate = true;
bool hle = true;
unsigned int ncards = 0;
uint32_t fifo_port = 0, video_port = 0, procmem = 0;
bool fake_boot = true;
unsigned int fake_rev = 9;
bool installed = false;
IO_ReadHandleObject  rd_proc[4], rd_fifo, rd_video;
IO_WriteHandleObject wr_proc[4], wr_fifo, wr_video;
uint8_t video_reg[16];

/* the card whose memory the window currently shows (first with bit4 set) */
ProcCard *window_card() {
    for (unsigned int i = 0; i < ncards; i++)
        if (cards[i].ctrl & 0x10) return &cards[i];
    return NULL;
}

/* ---- memory window ------------------------------------------------------------------------ */

class PixWindow : public PageHandler {
public:
    PixWindow() : PageHandler(PFLAG_NOCODE) {}

    uint32_t board(PhysPt addr, ProcCard *c) const { return c->page() | ((uint32_t)addr - procmem); }

    uint16_t rd16(PhysPt addr) {
        ProcCard *c = window_card();
        uint16_t v = 0xFFFF;
        if (c) v = c->get16(board(addr, c) & ~1u);
        SU2K_Log(SU2K_MEM_READ, 2, (uint32_t)addr, v, c ? board(addr, c) : 0xFFFFFFFFu);
        return v;
    }
    void wr16(PhysPt addr, uint16_t v) {
        bool any = false;
        for (unsigned int i = 0; i < ncards; i++) {
            if (!(cards[i].ctrl & 0x10)) continue;
            cards[i].put16(board(addr, &cards[i]) & ~1u, v);
            if (twins[i].board) twins[i].put16(board(addr, &cards[i]) & ~1u, v);
            if (!any) SU2K_Log(SU2K_MEM_WRITE, 2, (uint32_t)addr, v, board(addr, &cards[i]));
            any = true;
        }
        if (!any) SU2K_Log(SU2K_MEM_WRITE, 2, (uint32_t)addr, v, 0xFFFFFFFFu);
    }

    /* Byte lanes: assume the bus bridge swaps bytes within a halfword [inferred]. */
    uint8_t readb(PhysPt addr) override {
        ProcCard *c = window_card();
        uint8_t v = 0xFF;
        if (c) v = c->at(board(addr, c) ^ 1u);
        SU2K_Log(SU2K_MEM_READ, 1, (uint32_t)addr, v, c ? board(addr, c) : 0xFFFFFFFFu);
        return v;
    }
    void writeb(PhysPt addr, uint8_t val) override {
        for (unsigned int i = 0; i < ncards; i++)
            if (cards[i].ctrl & 0x10) {
                cards[i].at(board(addr, &cards[i]) ^ 1u) = val;
                if (twins[i].board) twins[i].at(board(addr, &cards[i]) ^ 1u) = val;
            }
        ProcCard *c = window_card();
        SU2K_Log(SU2K_MEM_WRITE, 1, (uint32_t)addr, val, c ? board(addr, c) : 0xFFFFFFFFu);
    }
    uint16_t readw(PhysPt addr) override {
        if (addr & 1) return (uint16_t)(readb(addr) | (readb(addr + 1) << 8u));
        return rd16(addr);
    }
    void writew(PhysPt addr, uint16_t val) override {
        if (addr & 1) { writeb(addr, (uint8_t)val); writeb(addr + 1, (uint8_t)(val >> 8u)); return; }
        wr16(addr, val);
    }
    /* A 32-bit host access is two halfword cycles: low word at addr, high word at addr+2 */
    uint32_t readd(PhysPt addr) override {
        return (uint32_t)readw(addr) | ((uint32_t)readw(addr + 2) << 16u);
    }
    void writed(PhysPt addr, uint32_t val) override {
        writew(addr, (uint16_t)val);
        writew(addr + 2, (uint16_t)(val >> 16u));
    }
};

PixWindow window;

/* ---- processor card I/O ------------------------------------------------------------------ */

ProcCard *card_for_port(Bitu port) {
    for (unsigned int i = 0; i < ncards; i++)
        if (port >= cards[i].io && port < cards[i].io + 8) return &cards[i];
    return NULL;
}

void boot_handshake(ProcCard &c, unsigned int idx, bool cpu_a) {
    SU2K_Log(SU2K_EVENT, 4, cpu_a ? SU2K_EV_RUN_A : SU2K_EV_RUN_B, c.page(), idx);
    if (c.board) {
        std::lock_guard<std::mutex> g(c.lock);
        c.board->run_cpu(cpu_a);
        if (idx < 4 && twins[idx].board) { std::lock_guard<std::mutex> g2(twins[idx].lock); twins[idx].board->run_cpu(cpu_a); }
        LOG_MSG("SU2000: PIX card %u CPU %c started", idx, cpu_a ? 'A' : 'B');
        return;
    }
    if (!fake_boot) return;
    /* MAINB writes its PID revision to board 0x2102; MAINA waits for that, then writes its own
     * to 0x2100 (MAINA.OUT .text 0x4a764-0x4a780). PIXI_TestProcessorMasks needs both >= 9. */
    const uint32_t a = cpu_a ? 0x2100u : 0x2102u;
    c.put16(a, (uint16_t)fake_rev);
    SU2K_Log(SU2K_EVENT, 2, SU2K_EV_FAKE_WRITE, fake_rev, (idx << 24u) | a);
}

Bitu proc_read(Bitu port, Bitu iolen) {
    ProcCard *c = card_for_port(port);
    Bitu v = 0xFF;
    if (c) {
        const unsigned int r = (unsigned int)(port - c->io);
        if (c->board) c->board->time_us.store(PIC_FullIndex() * 1000.0, std::memory_order_relaxed);
        if (r == 0 && c->board) {
            /* FIFO flags, active low: bit7 full, bit6 half full, bit5 empty */
            const size_t lvl = c->board->fifo_level();
            v = (c->ctrl & 0x1Fu) | (lvl < PixBoard::FIFO_SIZE ? 0x80u : 0u) | (lvl < PixBoard::FIFO_SIZE / 2 ? 0x40u : 0u) | (lvl ? 0x20u : 0u);
        }
        else if (r == 0) v = 0xC0u | (c->ctrl & 0x1Fu);   /* stub: not full, not half full, empty */
        else v = c->reg[r];
    }
    SU2K_Log(SU2K_IO_READ, (uint8_t)iolen, (uint32_t)port, (uint32_t)v);
    return v;
}

void proc_write(Bitu port, Bitu val, Bitu iolen) {
    SU2K_Log(SU2K_IO_WRITE, (uint8_t)iolen, (uint32_t)port, (uint32_t)val);
    ProcCard *c = card_for_port(port);
    if (!c) return;
    const unsigned int idx = (unsigned int)(c - cards);
    const unsigned int r = (unsigned int)(port - c->io);
    const uint8_t v = (uint8_t)val;
    c->reg[r] = v;
    switch (r) {
        case 0: {
            const uint8_t old = c->ctrl;
            c->ctrl = v;
            if (c->board && !(v & 0x03) && (old & 0x03)) {
                { std::lock_guard<std::mutex> g(c->lock); c->board->stop(); }
                if (twins[idx].board) { std::lock_guard<std::mutex> g(twins[idx].lock); twins[idx].board->stop(); }
            }
            if ((v & 0x02) && !(old & 0x02)) boot_handshake(*c, idx, false);
            if ((v & 0x01) && !(old & 0x01)) boot_handshake(*c, idx, true);
            break;
        }
        case 1: c->page_hi = v; break;
        case 2: c->page_lo = v; break;
        case 3: case 4:
            SU2K_Log(SU2K_EVENT, 1, SU2K_EV_RESET, v, (idx << 8u) | r);
            if (c->board) { std::lock_guard<std::mutex> g(c->lock); c->board->stop(); }
            if (twins[idx].board) { std::lock_guard<std::mutex> g(twins[idx].lock); twins[idx].board->stop(); }
            break;
        default: break;
    }
}

/* ---- broadcast FIFO ---------------------------------------------------------------------- */

Bitu fifo_read(Bitu port, Bitu iolen) {
    SU2K_Log(SU2K_IO_READ, (uint8_t)iolen, (uint32_t)port, 0xFFFF);
    return (iolen == 1) ? 0xFF : 0xFFFF;
}

/* For FIFO records, addr/aux carry the stack dwords at [esp+12]/[esp+8] instead of the port:
 * inside PIXI_OutWordBCFIFO (2 pushes) and PIXI_OutLong/FloatBCFIFO (3 pushes) one of them is the
 * return address into the PIX_Send* function, which lets tools/su2klog.py split the stream into
 * commands by host function. */
void fifo_log(Bitu val, Bitu iolen) {
    const PhysPt sp = SegPhys(ss) + (cpu.stack.big ? (PhysPt)reg_esp : (PhysPt)reg_sp);
    SU2K_Log(SU2K_FIFO, (uint8_t)iolen, mem_readd(sp + 12), (uint32_t)val, mem_readd(sp + 8));
}

/* A full FIFO holds the ISA write (IOCHRDY) on the real board - nothing is ever dropped. Wait for CPU A to make room,
 * but not forever (a stopped card would otherwise hang the emulator). */
void push_word(PixBoard *b, uint16_t w, unsigned i, double t) {
    b->time_us.store(t, std::memory_order_relaxed);
    if (b->fifo_push(w)) return;
    const auto t0 = std::chrono::steady_clock::now();
    while (!b->fifo_push(w)) {
        if (std::chrono::steady_clock::now() - t0 > std::chrono::milliseconds(500)) {
            static unsigned dropped = 0;
            if (dropped++ < 8) LOG_MSG("SU2000: card %u FIFO full for 500 ms, word dropped", i);
            break;
        }
        std::this_thread::yield();
    }
}

void fifo_deliver(uint16_t w) {
    if (!emulate) return;
    const double t = PIC_FullIndex() * 1000.0;
    for (unsigned int i = 0; i < ncards; i++)
        if (cards[i].board && (cards[i].ctrl & 0x21u)) {     /* bit5 = listen to the broadcast FIFO [inferred] */
            push_word(cards[i].board, w, i, t);
            if (twins[i].board) push_word(twins[i].board, w, i, t);
        }
}

void fifo_write(Bitu port, Bitu val, Bitu iolen) {
    (void)port;
    if (iolen == 4) {   /* split like the 16-bit ISA bus would: low word first */
        if (SU2K_LogEnabled()) { fifo_log(val & 0xFFFFu, 2); fifo_log(val >> 16u, 2); }
        fifo_deliver((uint16_t)val);
        fifo_deliver((uint16_t)(val >> 16u));
        return;
    }
    if (SU2K_LogEnabled()) fifo_log(val, iolen);
    fifo_deliver((uint16_t)val);
}

/* ---- worker threads --------------------------------------------------------------------- */

void card_thread(ProcCard *c, unsigned idx) {
    while (!c->quit.load()) {
        bool idle;
        {
            std::lock_guard<std::mutex> g(c->lock);
            c->board->run(200000);
            idle = c->board->idle || (!c->board->a_on && !c->board->b_on);
        }
        if (idle) std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    (void)idx;
}

void frame_cb(PixBoard *b, void *user) {
    ProcCard *c = (ProcCard *)user;
    PixFrame f;
    if (!b->render_frame(f)) return;
    std::lock_guard<std::mutex> g(c->frame_lock);
    c->frame.width = f.width;
    c->frame.height = f.height;
    c->frame.band_lo = f.band_lo;
    c->frame.band_hi = f.band_hi;
    c->frame.pixels.swap(f.pixels);
    const uint32_t seq = c->frame_seq.load() + 1;
    c->hist[seq & 3] = c->frame;
    c->hist_seq[seq & 3] = seq;
    c->frame_seq.store(seq);
}

/* ---- video card (logging only) ----------------------------------------------------------- */

Bitu video_read(Bitu port, Bitu iolen) {
    const Bitu v = video_reg[(port - video_port) & 15u];
    SU2K_Log(SU2K_IO_READ, (uint8_t)iolen, (uint32_t)port, (uint32_t)v);
    return v;
}

void video_write(Bitu port, Bitu val, Bitu iolen) {
    SU2K_Log(SU2K_IO_WRITE, (uint8_t)iolen, (uint32_t)port, (uint32_t)val);
    video_reg[(port - video_port) & 15u] = (uint8_t)val;
}

} // namespace

/* Main-thread access to the latest frame of card i (for the display window / frame dumps). */
bool PIX1000_GetFrame(unsigned i, PixFrame &out, uint32_t &seq) {
    /* i >= ncards: right-eye twin of card i - ncards (stereo) */
    ProcCard *c = i < ncards ? &cards[i] : (i < 2 * ncards ? &twins[i - ncards] : NULL);
    if (!c || !c->board) return false;
    std::lock_guard<std::mutex> g(c->frame_lock);
    if (!c->frame.width) return false;
    out = c->frame;
    seq = c->frame_seq.load();
    return true;
}

/* Frame number seq of card i if it is still kept (the cards of one picture run on their own threads and can be a
   frame apart; the display joins bands with the same number). */
bool PIX1000_GetFrameSeq(unsigned i, uint32_t seq, PixFrame &out) {
    ProcCard *c = i < ncards ? &cards[i] : (i < 2 * ncards ? &twins[i - ncards] : NULL);
    if (!c || !c->board) return false;
    std::lock_guard<std::mutex> g(c->frame_lock);
    if (c->hist_seq[seq & 3] != seq || !c->hist[seq & 3].width) return false;
    out = c->hist[seq & 3];
    return true;
}

unsigned PIX1000_NumCards(void) { return ncards; }
bool PIX1000_Stereo(void) { return stereo && emulate; }
void PIX1000_SetStereo(bool on, float separation) { stereo = on; stereo_sep = separation; }

void PIX1000_Tick(void) {
    const double t = PIC_FullIndex() * 1000.0;
    static double last_report = 0;
    if (t - last_report > 5e6) {
        last_report = t;
        for (unsigned int i = 0; i < ncards; i++) {
            PixBoard *b = cards[i].board;
            if (!b) continue;
            LOG_MSG("SU2000: card %u A %s pc=%08x %lluM  B %s pc=%08x %lluM  fifo %u (full hits %llu) pops %llu draws %llu ctrl %02x band %u..%u busy[214c]=%04x%04x [2158]=%04x%04x", i,
                    b->a_on ? "on" : "off", b->cpu_a->pc, (unsigned long long)(b->cpu_a->icount / 1000000),
                    b->b_on ? "on" : "off", b->cpu_b->pc, (unsigned long long)(b->cpu_b->icount / 1000000),
                    (unsigned)b->fifo_level(), (unsigned long long)b->fifo_overflow, (unsigned long long)b->fifo_pops, (unsigned long long)b->draws, cards[i].ctrl, (unsigned)((b->dram[0x210E] << 8) | b->dram[0x210F]), (unsigned)((b->dram[0x2112] << 8) | b->dram[0x2113]), (unsigned)((b->dram[0x214C] << 8) | b->dram[0x214D]), (unsigned)((b->dram[0x214E] << 8) | b->dram[0x214F]), (unsigned)((b->dram[0x2158] << 8) | b->dram[0x2159]), (unsigned)((b->dram[0x215A] << 8) | b->dram[0x215B]));
            { extern void pix_pchist_report(unsigned card); pix_pchist_report(i); }
            LOG_MSG("SU2000: card %u view hooks hit %llu (twin %llu)", i, (unsigned long long)b->view_hits, twins[i].board ? (unsigned long long)twins[i].board->view_hits : 0ull);
            if (PixBoard *tb = twins[i].board)
                LOG_MSG("SU2000: twin %u A %s pc=%08x %lluM  B %s pc=%08x  fifo %u (full hits %llu) pops %llu frames %u/%u", i,
                        tb->a_on ? "on" : "off", tb->cpu_a->pc, (unsigned long long)(tb->cpu_a->icount / 1000000), tb->b_on ? "on" : "off",
                        tb->cpu_b->pc, (unsigned)tb->fifo_level(), (unsigned long long)tb->fifo_overflow, (unsigned long long)tb->fifo_pops,
                        (unsigned)twins[i].frame_seq.load(), (unsigned)cards[i].frame_seq.load());
            LOG_MSG("SU2000: card %u B r1=%08x r8=%08x r10=%08x r18=%08x r19=%08x r29=%08x r30=%08x r31=%08x", i,
                    b->cpu_b->r[1], b->cpu_b->r[8], b->cpu_b->r[10], b->cpu_b->r[18], b->cpu_b->r[19], b->cpu_b->r[29], b->cpu_b->r[30], b->cpu_b->r[31]);
        }
    }
    for (unsigned int i = 0; i < ncards; i++) {
        if (cards[i].board) cards[i].board->time_us.store(t, std::memory_order_relaxed);
        if (twins[i].board) twins[i].board->time_us.store(t, std::memory_order_relaxed);
    }
}

void PIX1000_SetMode(bool emulation, bool hle_b) { emulate = emulation; hle = hle_b; }

void PIX1000_Setup(uint32_t fifo, const uint32_t *proc_ports, unsigned int nproc,
                   uint32_t video, uint32_t mem, bool fake, unsigned int rev) {
    PIX1000_Shutdown();
    /* board diagnostics into the DOSBox-X log (low volume; may be called from the card threads) */
    pix_log_sink = [](const char *msg) { LOG_MSG("%s", msg); };
    fifo_port = fifo; video_port = video; procmem = mem & ~0xFFFFu;
    fake_boot = fake; fake_rev = rev;
    ncards = nproc > 4 ? 4 : nproc;
    for (unsigned int i = 0; i < ncards; i++) {
        cards[i].ctrl = 0; cards[i].page_hi = cards[i].page_lo = 0;
        memset(cards[i].reg, 0, sizeof(cards[i].reg));
        cards[i].high.clear();
        cards[i].io = proc_ports[i];
        if (emulate) {
            cards[i].board = new PixBoard();
            cards[i].board->card_id = i;
            cards[i].board->hle_b = hle;
            cards[i].board->frame_cb = frame_cb;
            cards[i].board->frame_user = &cards[i];
            cards[i].quit = false;
            cards[i].thread = new std::thread(card_thread, &cards[i], i);
            if (stereo) {
                /* left eye = the real card, right eye = a twin; the camera moves by half the separation each way */
                cards[i].board->eye_shift = -0.5f * stereo_sep;
                twins[i].io = proc_ports[i];
                twins[i].board = new PixBoard();
                twins[i].board->card_id = i;
                twins[i].board->hle_b = hle;
                twins[i].board->eye_shift = 0.5f * stereo_sep;
                twins[i].board->frame_cb = frame_cb;
                twins[i].board->frame_user = &twins[i];
                twins[i].quit = false;
                twins[i].thread = new std::thread(card_thread, &twins[i], i);
            }
        } else cards[i].ram.assign(BOARD_RAM, 0);
        rd_proc[i].Install(cards[i].io, proc_read, IO_MA, 8);
        wr_proc[i].Install(cards[i].io, proc_write, IO_MA, 8);
    }
    if (fifo_port) {
        rd_fifo.Install(fifo_port, fifo_read, IO_MA, 2);
        wr_fifo.Install(fifo_port, fifo_write, IO_MA, 2);
    }
    if (video_port) {
        for (auto &r : video_reg) r = 0;
        rd_video.Install(video_port, video_read, IO_MA, 16);
        wr_video.Install(video_port, video_write, IO_MA, 16);
    }
    if (procmem) {
        MEM_SetPageHandler(procmem >> 12u, 16, &window);
        PAGING_ClearTLB();
    }
    installed = true;
    LOG_MSG("SU2000: PIX 1000 %s, %u processor card(s), FIFO %03xh, window %05xh", emulate ? (hle ? "emulation (CPU A LLE, CPU B HLE)" : "emulation (LLE)") : "stub",
            ncards, fifo_port, procmem);
}

void PIX1000_Shutdown(void) {
    if (!installed) return;
    for (unsigned int i = 0; i < ncards; i++) {
        rd_proc[i].Uninstall();
        wr_proc[i].Uninstall();
        if (cards[i].thread) {
            cards[i].quit = true;
            cards[i].thread->join();
            delete cards[i].thread;
            cards[i].thread = NULL;
        }
        delete cards[i].board;
        cards[i].board = NULL;
        if (twins[i].thread) {
            twins[i].quit = true;
            twins[i].thread->join();
            delete twins[i].thread;
            twins[i].thread = NULL;
        }
        delete twins[i].board;
        twins[i].board = NULL;
        cards[i].ram.clear();
        cards[i].ram.shrink_to_fit();
        cards[i].high.clear();
    }
    rd_fifo.Uninstall(); wr_fifo.Uninstall();
    rd_video.Uninstall(); wr_video.Uninstall();
    if (procmem) {
        MEM_ResetPageHandler_Unmapped(procmem >> 12u, 16);
        PAGING_ClearTLB();
    }
    installed = false;
}
