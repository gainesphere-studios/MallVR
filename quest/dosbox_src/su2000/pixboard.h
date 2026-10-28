/*
 *  One Expality PIX 1000 processor card: shared DRAM, VRAM, broadcast-FIFO input, CPU A (and B)
 *  in the M88110 interpreter, and the HLE replacement for CPU B's rasteriser.
 *
 *  Standalone (no DOSBox-X dependencies); used by pix1000.cpp and by su2000/tests/run_pix.
 *  Board physical map (from MAINA/MAINB, see su2000/docs/PROTOCOL.md §9):
 *    0x00000000  DRAM (16 MB decode; firmware uses < 8 MB). 0x1000_0000 is an uncached alias (BATC).
 *    0x20000006  FIFO data (16-bit read pops one word)
 *    0x30000000  status: bit29 FIFO not empty, bits 28..25 card id [inferred], bits 10..1 video line
 *    0x30000006  control (16-bit)
 *    0x40000000  VRAM, 4 MB (frame buffers, palettes at 0x403FC000)
 */
#ifndef DOSBOX_PIXBOARD_H
#define DOSBOX_PIXBOARD_H

#include "m88110.h"

#include <stdint.h>
#include <stdio.h>
#include <atomic>
#include <deque>
#include <string>
#include <vector>

class PixBoard;

/* Board diagnostics. Default: stderr. The emulator can redirect them (DOSBox-X: its log). */
extern void (*pix_log_sink)(const char *msg);
void pix_logf(const char *fmt, ...);

class PixBus : public M88110Bus {
public:
    PixBus(PixBoard *b, int cpu) : board(b), which(cpu) {}
    uint8_t  rd8(uint32_t pa) override;
    uint16_t rd16(uint32_t pa) override;
    uint32_t rd32(uint32_t pa) override;
    void     wr8(uint32_t pa, uint8_t v) override;
    void     wr16(uint32_t pa, uint16_t v) override;
    void     wr32(uint32_t pa, uint32_t v) override;
    uint32_t fetch(uint32_t pa) override;
private:
    PixBoard *board;
    int which;
};

struct PixFrame {
    /* A displayed frame: 32-bit pixels 0x00RRGGBB (after the firmware's pixel format conversion) */
    unsigned width = 0, height = 0;
    std::vector<uint32_t> pixels;
    /* Scan-line band this card is responsible for (board 0x210C / 0x2110, written by the host per card). Several cards
     * sharing one video channel (SU2000 Solo: "proc 0x300,1,0.0,0.5 / proc 0x360,2,0.5,1.0") each own a band; the
     * video card shows each band from its card. A card with the full screen has band 0..height-1. */
    unsigned band_lo = 0, band_hi = 0;
};

class PixBoard {
public:
    static const uint32_t DRAM_SIZE = 16u * 1024u * 1024u;
    static const uint32_t VRAM_SIZE = 4u * 1024u * 1024u;

    PixBoard();
    ~PixBoard();

    /* host side */
    void     host_write8(uint32_t a, uint8_t v);
    void     host_write16(uint32_t a, uint16_t v);   /* big-endian halfword */
    uint8_t  host_read8(uint32_t a);
    uint16_t host_read16(uint32_t a);
    /* Broadcast FIFO: single producer (host) / single consumer (CPU A), lock-free ring. */
    static const uint32_t FIFO_SIZE = 4096;    /* words; host sees "half full" at FIFO_SIZE/2 [inferred] */
    bool     fifo_push(uint16_t w) {
        const uint32_t h = fifo_head.load(std::memory_order_relaxed);
        if (h - fifo_tail.load(std::memory_order_acquire) >= FIFO_SIZE) { fifo_overflow++; return false; }
        fifo_ring[h & (FIFO_SIZE - 1)] = w;
        fifo_head.store(h + 1, std::memory_order_release);
        return true;
    }
    size_t   fifo_level() const { return fifo_head.load(std::memory_order_acquire) - fifo_tail.load(std::memory_order_acquire); }
    uint64_t fifo_overflow = 0;
    /* Emulated time in microseconds supplied by the host emulator (< 0: derive from CPU A's clock). */
    std::atomic<double> time_us{-1.0};
    void     run_cpu(bool cpu_a);      /* host set the run bit: CPU starts at address 0 */
    void     start(bool a_on, bool b_on);
    void     stop();

    /* execution: run up to n instructions (both CPUs); idle is set when neither CPU has work */
    uint64_t run(uint64_t n);
    bool     idle = false;
    bool     a_idle() const { return a_waiting_fifo; }

    /* configuration */
    bool hle_b = true;
    unsigned card_id = 0;
    uint64_t view_hits = 0;     /* views shifted by eye_shift */
    float eye_shift = 0.0f;     /* stereo: camera shift along its x axis, applied to every stored view (0 = off) */
    unsigned insns_per_line = 3200;   /* 64 us video line at ~50 MIPS [inferred clock] */
    unsigned lines_per_frame = 312;   /* PAL non-interlaced, 64 us per line */
    std::string frame_dir;            /* harness: write PPM frames here */
    int max_frames = 1000000;
    int frames_written = 0;
    void (*frame_cb)(PixBoard *b, void *user) = NULL;
    void *frame_user = NULL;

    /* displayed-frame state */
    uint32_t display_base = 0;        /* VRAM address shown (board 0x200c) */
    bool     grab_pending = false;    /* display switched; grab once CPU B has drained its draw list */
    PixFrame last_frame;
    void     grab_frame();
    /* Convert the displayed buffer to 0x00RRGGBB. Geometry comes from the firmware globals:
     * phys 0x7040 = line stride (bytes), 0x7050 = buffer size in 8 KB VRAM rows, 0x2104 bit 9 = 16-bit pixels
     * (4:4:4:4, R in bits 11..8) [inferred]. Returns false if the firmware has not set up a display yet. */
    bool     render_frame(PixFrame &out);

    /* stats */
    uint64_t fifo_pops = 0, draws = 0, unknown_io = 0;
    void report(FILE *f);
    /* draw-list statistics per record type (signed byte 0 of each 8-byte header) */
    struct TypeStat { uint64_t count = 0, insns = 0, bytes = 0, hle = 0; };
    TypeStat type_stats[256];
    bool collect_stats = false;
    unsigned compare_every = 0;       /* verify HLE against the interpreter on every Nth HLE record */
    uint64_t compare_runs = 0, compare_fail = 0;

    std::vector<uint8_t> dram, vram;
    M88110 *cpu_a, *cpu_b;
    bool a_on = false, b_on = false;

    /* device access from the CPUs */
    uint32_t dev_read(int cpu, uint32_t pa, unsigned size);
    void     dev_write(int cpu, uint32_t pa, uint32_t v, unsigned size);

private:
    PixBus bus_a, bus_b;
    uint16_t fifo_ring[FIFO_SIZE];
    std::atomic<uint32_t> fifo_head{0}, fifo_tail{0};
    bool fifo_pop(uint16_t &w) {
        const uint32_t t = fifo_tail.load(std::memory_order_relaxed);
        if (t == fifo_head.load(std::memory_order_acquire)) return false;
        w = fifo_ring[t & (FIFO_SIZE - 1)];
        fifo_tail.store(t + 1, std::memory_order_release);
        return true;
    }
    uint16_t last_fifo = 0;
    bool a_waiting_fifo = false;
    uint64_t clock = 0;
    uint16_t ctrl = 0;
    uint32_t draw32_addr = 0, draw16_addr = 0, mainb_poll_pc = 0, disp32 = 0, disp16 = 0;
    int cur_type = -1; uint64_t cur_insn0 = 0; uint32_t cur_ptr = 0;
    void find_b_entry_points();
    void find_a_layout(uint32_t maina);
    /* Stereo: shift of the camera along its own x axis applied whenever CPU A stores a view (0 = off).
       view_hook[] = ends of ProcView / ProcViewMAT (12 floats) and ProcViewPOS (3 floats). */
    uint32_t view_hook[3] = { 0, 0, 0 };
    void find_view_hooks(uint32_t maina);
    /* firmware-generation dependent layout, detected from the uploaded code (defaults: 1995 firmware) */
    uint32_t slot_off = 0x100, rows_off = 0x50;
    uint32_t mainb_pc = 0;
    bool b_lost_logged = false;
    unsigned px16_bit = 9;
    void hle_draw(bool bpp32);
    uint64_t unknown_log = 0;
    uint8_t sam[0x2000];              /* VRAM serial access memory (one 8 KB row) */
    bool vram_transfer(uint32_t pa, bool write, uint32_t v);
};

#endif
