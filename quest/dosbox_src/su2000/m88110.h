/*
 *  Motorola MC88110 interpreter (subset needed by the Expality PIX 1000 firmware).
 *
 *  Standalone: no DOSBox-X dependencies, so it can also be built into the test harness
 *  su2000/tests/run_pix.cpp. Instruction decoding uses the MAME mc88110 opcode table
 *  (m88110_ops.inc, BSD-3-Clause). Semantics follow the MC88100/MC88110 user's manuals as far as
 *  they are known; graphics-unit instructions (padd, pmul, ppack, punpk, prot, pcmp) are partly
 *  inferred from how the PIX firmware uses them - see su2000/docs/findings/m88110_core.md.
 */
#ifndef DOSBOX_M88110_H
#define DOSBOX_M88110_H

#include <stdint.h>
#include <string>
#include <vector>

class M88110Bus {
public:
    virtual ~M88110Bus() {}
    /* Physical, big-endian accesses. Addresses are naturally aligned by the core. */
    virtual uint8_t  rd8(uint32_t pa) = 0;
    virtual uint16_t rd16(uint32_t pa) = 0;
    virtual uint32_t rd32(uint32_t pa) = 0;
    virtual void     wr8(uint32_t pa, uint8_t v) = 0;
    virtual void     wr16(uint32_t pa, uint16_t v) = 0;
    virtual void     wr32(uint32_t pa, uint32_t v) = 0;
    /* Instruction fetch; may be served from a fast pointer. */
    virtual uint32_t fetch(uint32_t pa) { return rd32(pa); }
    /* Optional direct mapping of plain memory, per 4 MB physical region (index pa >> 22).
     * NULL entries go through the virtual accessors. */
    uint8_t *fast[1024] = {};
};

class M88110 {
public:
    explicit M88110(M88110Bus *bus);

    void reset(uint32_t pc);
    /* Run up to n instructions; returns the number executed. Stops early when halted. */
    uint64_t run(uint64_t n);

    /* State */
    uint32_t r[32];
    double   x[32];             /* extended register file, kept as double */
    uint32_t cr[64];            /* general control registers (pid, psr, vbr, ...) */
    uint32_t fcr[64];
    uint32_t pc;
    bool     halted;
    uint64_t icount;
    uint32_t batc_addr[8], batc_entry[8];   /* data BATC (written via dir/dbp) */

    /* Diagnostics */
    uint32_t unknown_count;
    uint32_t last_unknown_inst, last_unknown_pc;
    void (*trap_hook)(M88110 *cpu, unsigned vec, void *user);  /* tb0/tb1/tcnd/tbnd traps */
    void *trap_user;
    bool trace;
    /* optional history of the last executed instructions (debugging) */
    bool keep_hist = false;
    uint32_t hist_pc[64], hist_inst[64];
    unsigned hist_i = 0;
    /* run() stops when pc leaves [guard_lo, guard_hi) (debugging; guard_hi = 0 disables) */
    uint32_t guard_lo = 0, guard_hi = 0;
    bool guard_hit = false;
    /* MallVR (card draw order): optional data-access watch, called for every load / store (NULL = off) */
    void (*mem_hook)(void *user, uint32_t pc, uint32_t pa, uint32_t v, unsigned size, bool wr) = NULL;
    void *mem_user = NULL;
    uint32_t bp[4];             /* run() stops before executing an instruction at one of these (0 = unused) */
    std::string disasm(uint32_t inst, uint32_t at) const;

    /* Memory helpers using the data BATC translation */
    uint32_t xlate(uint32_t va) const { const uint32_t t = batc_tab[va >> 19]; return t ? (t & 0xFFF80000u) | (va & 0x7FFFFu) : va; }
    void     rebuild_batc();
    uint32_t batc_tab[8192];    /* per 512 KB logical block: physical base | 1, or 0 = untranslated */
    uint32_t rd32v(uint32_t va) { return bus->rd32(xlate(va)); }

private:
    M88110Bus *bus;
    struct Decoded { uint16_t op; uint8_t mode; uint8_t fk, td, t1, t2; };
    std::vector<uint32_t> dcache_tag;
    std::vector<Decoded> dcache;
    Decoded decode(uint32_t inst);

    void execute(uint32_t inst, Decoded d);
    void branch_to(uint32_t target, bool delayed);
    void trap(unsigned vec);
    uint32_t cmp_bits(uint32_t a, uint32_t b) const;
    uint32_t fcmp_bits(double a, double b) const;

    /* FP operand access by size code: 0 single, 1 double, 2 extended */
    double   fget(unsigned reg, unsigned size, bool xrf) const;
    void     fset(unsigned reg, unsigned size, bool xrf, double v);

    /* delayed-branch state */
    bool     pending_branch;
    uint32_t pending_target;
};

#endif
