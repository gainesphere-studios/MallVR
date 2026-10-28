/*
 *  Motorola MC88110 interpreter - see m88110.h.
 */
#include "m88110.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

namespace {

enum Mode { AM_TRIADIC, AM_FP, AM_IMM6, AM_BITFIELD, AM_SIMM16, AM_IMM16, AM_CR, AM_SI16_GRF, AM_SI16_XRF,
            AM_SCALED, AM_JUMP, AM_VEC9, AM_D16, AM_D26, AM_NONE };

enum Op : uint16_t {
#define M88110_ENUM
#include "m88110_ops.inc"
#undef M88110_ENUM
    OP_COUNT,
    /* encodings missing from the MAME 88110 table */
    OP_TB0, OP_TB1, OP_TCND, OP_FLT_X /* flt with XRF destination (bit 9) */, OP_UNKNOWN
};

const char *const op_names[] = {
#define M88110_NAMES
#include "m88110_ops.inc"
#undef M88110_NAMES
};

struct TableEntry { uint32_t value, mask; uint16_t op; uint8_t mode; };
const TableEntry table[] = {
#define M88110_TABLE
#include "m88110_ops.inc"
#undef M88110_TABLE
};

/* FP operand size letters in mnemonics like "fadd.dsx": index 0 = destination, 1 = S1, 2 = S2 */
uint8_t fsz(char c) { return c == 's' ? 0 : c == 'd' ? 1 : 2; }

enum FpKind : uint8_t { FK_NONE, FK_ADD, FK_SUB, FK_MUL, FK_DIV, FK_SQRT, FK_CMP, FK_FLT, FK_INT, FK_NINT, FK_TRNC, FK_CVT, FK_MOV };
FpKind fp_kind(const char *n) {
    if (!strncmp(n, "fadd", 4)) return FK_ADD;
    if (!strncmp(n, "fsub", 4)) return FK_SUB;
    if (!strncmp(n, "fmul", 4)) return FK_MUL;
    if (!strncmp(n, "fdiv", 4)) return FK_DIV;
    if (!strncmp(n, "fsqrt", 5)) return FK_SQRT;
    if (!strncmp(n, "fcmp", 4)) return FK_CMP;
    if (!strncmp(n, "flt.", 4)) return FK_FLT;
    if (!strncmp(n, "int.", 4)) return FK_INT;
    if (!strncmp(n, "nint.", 5)) return FK_NINT;
    if (!strncmp(n, "trnc.", 5)) return FK_TRNC;
    if (!strncmp(n, "fcvt.", 5)) return FK_CVT;
    if (!strncmp(n, "mov", 3)) return FK_MOV;
    return FK_NONE;
}
struct FpSizes { uint8_t td, t1, t2; };
FpSizes fp_sizes[OP_COUNT];
bool fp_sizes_init = false;

void init_fp_sizes() {
    if (fp_sizes_init) return;
    for (unsigned i = 0; i < OP_COUNT; i++) {
        const char *n = op_names[i];
        const char *dot = strchr(n, '.');
        FpSizes s = {0, 0, 0};
        if (dot && n[0] == 'f') {
            const char *t = dot + 1;
            size_t l = strlen(t);
            if (l == 3) { s.td = fsz(t[0]); s.t1 = fsz(t[1]); s.t2 = fsz(t[2]); }
            else if (l == 2) { s.td = fsz(t[0]); s.t1 = s.t2 = fsz(t[1]); }
        } else if (dot && (!strncmp(n, "int.", 4) || !strncmp(n, "nint.", 5) || !strncmp(n, "trnc.", 5))) {
            s.td = 0; s.t1 = s.t2 = fsz(dot[2]);
        } else if (!strncmp(n, "flt.", 4)) {
            s.td = fsz(dot[1]); s.t1 = s.t2 = 0;
        }
        fp_sizes[i] = s;
    }
    fp_sizes_init = true;
}

inline uint32_t bf_mask(unsigned w) { return w == 0 || w >= 32 ? 0xFFFFFFFFu : ((1u << w) - 1u); }

inline float u2f(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }
inline uint32_t f2u(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
inline double u2d(uint32_t hi, uint32_t lo) { uint64_t u = ((uint64_t)hi << 32) | lo; double d; memcpy(&d, &u, 8); return d; }
inline uint64_t d2u(double d) { uint64_t u; memcpy(&u, &d, 8); return u; }

int32_t round_nearest_even(double v) { return (int32_t)nearbyint(v); }

} // namespace

M88110::M88110(M88110Bus *b) : bus(b) {
    init_fp_sizes();
    trap_hook = NULL;
    trap_user = NULL;
    trace = false;
    bp[0] = bp[1] = bp[2] = bp[3] = 0xFFFFFFFFu;
    dcache_tag.assign(1u << 16, 0xFFFFFFFFu);
    dcache.assign(1u << 16, Decoded{OP_UNKNOWN, AM_NONE, 0, 0, 0, 0});
    reset(0);
}

void M88110::reset(uint32_t start) {
    memset(r, 0, sizeof(r));
    for (auto &v : x) v = 0.0;
    memset(cr, 0, sizeof(cr));
    memset(fcr, 0, sizeof(fcr));
    memset(batc_addr, 0, sizeof(batc_addr));
    memset(batc_entry, 0, sizeof(batc_entry));
    memset(batc_tab, 0, sizeof(batc_tab));
    /* PID: architecture 1 (88110), revision 9 in bits 7..1 (PIX_TestProcessorMasks needs >= 9), master */
    cr[0] = 0x00000100u | (9u << 1) | 1u;
    cr[1] = 0x80000000u;   /* PSR: supervisor */
    pc = start;
    halted = false;
    icount = 0;
    unknown_count = 0;
    last_unknown_inst = last_unknown_pc = 0;
    pending_branch = false;
    pending_target = 0;
}

std::string M88110::disasm(uint32_t inst, uint32_t at) const {
    char buf[64];
    M88110 *self = const_cast<M88110 *>(this);
    Decoded d = self->decode(inst);
    snprintf(buf, sizeof(buf), "%08x: %08x %s", at, inst, d.op < OP_COUNT ? op_names[d.op] :
             d.op == OP_TB0 ? "tb0" : d.op == OP_TB1 ? "tb1" : d.op == OP_TCND ? "tcnd" : d.op == OP_FLT_X ? "flt(x)" : "???");
    return buf;
}

M88110::Decoded M88110::decode(uint32_t inst) {
    const uint32_t h = (inst ^ (inst >> 16) ^ (inst >> 7)) & 0xFFFFu;
    if (dcache_tag[h] == inst) return dcache[h];
    Decoded d{OP_UNKNOWN, AM_NONE, 0, 0, 0, 0};
    if ((inst & 0xFC00F800u) == 0xF000D000u) d = Decoded{OP_TB0, AM_VEC9, 0, 0, 0, 0};
    else if ((inst & 0xFC00F800u) == 0xF000D800u) d = Decoded{OP_TB1, AM_VEC9, 0, 0, 0, 0};
    else if ((inst & 0xFC00F800u) == 0xF000E800u) d = Decoded{OP_TCND, AM_VEC9, 0, 0, 0, 0};
    else if ((inst & 0xFC1F7E00u) == 0x84002200u) d = Decoded{OP_FLT_X, AM_FP, FK_FLT, (uint8_t)((inst >> 5) & 3), 0, 0};
    else {
        for (const auto &e : table) {
            if ((inst & e.mask) == e.value) { d = Decoded{e.op, e.mode, 0, 0, 0, 0}; break; }
        }
    }
    if (d.mode == AM_FP && d.op < OP_COUNT) {
        d.fk = fp_kind(op_names[d.op]);
        d.td = fp_sizes[d.op].td; d.t1 = fp_sizes[d.op].t1; d.t2 = fp_sizes[d.op].t2;
    }
    dcache_tag[h] = inst;
    dcache[h] = d;
    return d;
}

void M88110::rebuild_batc() {
    /* Data BATC entry = LBA[31:19] | PBA[18:6] | flags[5:0], bit0 = valid [inferred layout] */
    memset(batc_tab, 0, sizeof(batc_tab));
    for (int i = 7; i >= 0; i--) {
        const uint32_t e = batc_entry[i];
        if (!(e & 1u)) continue;
        batc_tab[e >> 19] = ((e << 13) & 0xFFF80000u) | 1u;
    }
    for (unsigned i = 0; i < 8192; i++)          /* identity entries need no translation */
        if (batc_tab[i] && (batc_tab[i] & 0xFFF80000u) == (i << 19)) batc_tab[i] = 0;
}

void M88110::branch_to(uint32_t target, bool delayed) {
    if (delayed) { pending_branch = true; pending_target = target; }
    else pc = target;
}

void M88110::trap(unsigned vec) {
    if (trap_hook) trap_hook(this, vec, trap_user);
    cr[2] = cr[1];            /* EPSR */
    cr[4] = pc;               /* EXIP: resume at the following instruction */
    cr[5] = pc + 4;
    cr[1] |= 0x80000000u | 0x2u;   /* supervisor, interrupts disabled */
    pc = cr[7] + vec * 8u;
}

uint32_t M88110::cmp_bits(uint32_t a, uint32_t b) const {
    const int32_t sa = (int32_t)a, sb = (int32_t)b;
    uint32_t v = 0;
    if (a == b) v |= 1u << 2; else v |= 1u << 3;
    if (sa > sb) v |= 1u << 4;
    if (sa <= sb) v |= 1u << 5;
    if (sa < sb) v |= 1u << 6;
    if (sa >= sb) v |= 1u << 7;
    if (a > b) v |= 1u << 8;
    if (a <= b) v |= 1u << 9;
    if (a < b) v |= 1u << 10;
    if (a >= b) v |= 1u << 11;
    return v;
}

uint32_t M88110::fcmp_bits(double a, double b) const {
    uint32_t v = 0;
    if (a != a || b != b) return 1u;   /* unordered */
    if (a == b) v |= 1u << 2; else v |= 1u << 3;
    if (a > b) v |= 1u << 4;
    if (a <= b) v |= 1u << 5;
    if (a < b) v |= 1u << 6;
    if (a >= b) v |= 1u << 7;
    if (a < 0 || a > b) v |= 1u << 8;              /* ou */
    if (a >= 0 && a <= b) v |= 1u << 9;            /* ib */
    if (a > 0 && a < b) v |= 1u << 10;             /* in */
    if (a <= 0 || a >= b) v |= 1u << 11;           /* ob */
    return v;
}

double M88110::fget(unsigned reg, unsigned size, bool xrf) const {
    if (xrf) {
        if (reg == 0) return 0.0;
        const double v = x[reg];
        return size == 0 ? (double)(float)v : v;
    }
    if (size == 0) return reg ? u2f(r[reg]) : 0.0f;
    const uint32_t hi = reg ? r[reg] : 0, lo = r[(reg + 1) & 31];
    return u2d(hi, lo);
}

void M88110::fset(unsigned reg, unsigned size, bool xrf, double v) {
    if (xrf) {
        if (reg) x[reg] = size == 0 ? (double)(float)v : v;
        return;
    }
    if (size == 0) { if (reg) r[reg] = f2u((float)v); return; }
    const uint64_t u = d2u(v);
    if (reg) r[reg] = (uint32_t)(u >> 32);
    if ((reg + 1) & 31) r[(reg + 1) & 31] = (uint32_t)u;
}

static inline uint32_t be32f(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }

uint64_t M88110::run(uint64_t n) {
    uint64_t done = 0;
    while (done < n && !halted) {
        const uint32_t ipc = pc;
        if ((ipc == bp[0] || ipc == bp[1] || ipc == bp[2] || ipc == bp[3]) && done && !pending_branch) break;
        const uint8_t *fp = bus->fast[ipc >> 22];
        const uint32_t inst = fp ? be32f(&fp[ipc & 0x3FFFFCu]) : bus->fetch(ipc);
        const bool had_pending = pending_branch;
        const uint32_t target = pending_target;
        pending_branch = false;
        pc = ipc + 4;
        Decoded d = decode(inst);
        if (trace) fprintf(stderr, "%s\n", disasm(inst, ipc).c_str());
        if (keep_hist) { hist_pc[hist_i & 63] = ipc; hist_inst[hist_i & 63] = inst; hist_i++; }
        if (guard_hi && (ipc < guard_lo || ipc >= guard_hi)) { guard_hit = true; guard_hi = 0; pc = ipc; break; }
        execute(inst, d);
        r[0] = 0;
        if (had_pending) pc = target;   /* the delay-slot instruction has executed */
        done++;
    }
    icount += done;
    return done;
}

#define FASTP(pa) (bus->fast[(pa) >> 22])
#define MW(pa, v, sz, w) do { if (mem_hook) mem_hook(mem_user, ipc, (pa), (v), (sz), (w)); } while (0)   /* MallVR watch */
static inline uint32_t be32p(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
#define RD8(pa)  (FASTP(pa) ? FASTP(pa)[(pa) & 0x3FFFFFu] : bus->rd8(pa))
#define RD16(pa) (FASTP(pa) ? (uint16_t)((FASTP(pa)[(pa) & 0x3FFFFFu] << 8) | FASTP(pa)[((pa) & 0x3FFFFFu) + 1]) : bus->rd16(pa))
#define RD32(pa) (FASTP(pa) ? be32p(&FASTP(pa)[(pa) & 0x3FFFFFu]) : bus->rd32(pa))
#define WR8(pa, v)  do { uint8_t *fp_ = FASTP(pa); if (fp_) fp_[(pa) & 0x3FFFFFu] = (uint8_t)(v); else bus->wr8(pa, (uint8_t)(v)); } while (0)
#define WR16(pa, v) do { uint8_t *fp_ = FASTP(pa); if (fp_) { uint8_t *q_ = &fp_[(pa) & 0x3FFFFFu]; q_[0] = (uint8_t)((v) >> 8); q_[1] = (uint8_t)(v); } else bus->wr16(pa, (uint16_t)(v)); } while (0)
#define WR32(pa, v) do { uint8_t *fp_ = FASTP(pa); const uint32_t v_ = (v); if (fp_) { uint8_t *q_ = &fp_[(pa) & 0x3FFFFFu]; q_[0] = (uint8_t)(v_ >> 24); q_[1] = (uint8_t)(v_ >> 16); q_[2] = (uint8_t)(v_ >> 8); q_[3] = (uint8_t)v_; } else bus->wr32(pa, v_); } while (0)

void M88110::execute(uint32_t inst, Decoded d) {
    const unsigned rd = (inst >> 21) & 31, rs1 = (inst >> 16) & 31, rs2 = inst & 31;
    const uint32_t imm = inst & 0xFFFFu;
    const int32_t simm = (int16_t)imm;
    const uint32_t S1 = r[rs1];
    const bool triadic = d.mode == AM_TRIADIC || d.mode == AM_SCALED;
    const uint32_t S2 = triadic || d.mode == AM_JUMP ? r[rs2] : d.mode == AM_SIMM16 ? (uint32_t)simm : imm;
    const uint32_t ipc = pc - 4;
    uint32_t &D = r[rd];

    /* effective address for load/store forms */
    auto ea = [&](unsigned size) -> uint32_t {
        if (d.mode == AM_SI16_GRF || d.mode == AM_SI16_XRF) return S1 + (uint32_t)simm;
        if (d.mode == AM_SCALED) return S1 + r[rs2] * size;
        return S1 + r[rs2];
    };
    const bool xrf_form = (d.mode == AM_SI16_XRF) || (triadic && !(inst & (1u << 26)) && (inst >> 27) == 0x1E);

    switch (d.op) {
    /* ---- loads ---- */
    case OP_LD_B: case OP_LD_B_USR: { uint32_t a = xlate(ea(1)); D = (uint32_t)(int32_t)(int8_t)RD8(a); MW(a, D, 1, false); break; }
    case OP_LD_BU: case OP_LD_BU_USR: { uint32_t a = xlate(ea(1)); D = RD8(a); MW(a, D, 1, false); break; }
    case OP_LD_H: case OP_LD_H_USR: { uint32_t a = xlate(ea(2)) & ~1u; D = (uint32_t)(int32_t)(int16_t)RD16(a); MW(a, D, 2, false); break; }
    case OP_LD_HU: case OP_LD_HU_USR: { uint32_t a = xlate(ea(2)) & ~1u; D = RD16(a); MW(a, D, 2, false); break; }
    case OP_LD: case OP_LD_USR: {
        uint32_t a = xlate(ea(4)) & ~3u;
        uint32_t v = RD32(a); MW(a, v, 4, false);
        if (xrf_form) { if (rd) x[rd] = u2f(v); } else D = v;
        break;
    }
    case OP_LD_D: case OP_LD_D_USR: {
        uint32_t a = xlate(ea(8)) & ~7u;
        uint32_t hi = RD32(a), lo = RD32(a + 4); MW(a, hi, 4, false); MW(a + 4, lo, 4, false);
        if (xrf_form) { if (rd) x[rd] = u2d(hi, lo); }
        else { D = hi; r[(rd + 1) & 31] = lo; }
        break;
    }
    case OP_LD_X: case OP_LD_X_USR: {
        /* 128-bit extended in memory; approximate via the double stored in the upper 64 bits [inferred] */
        uint32_t a = xlate(ea(16)) & ~15u;
        if (rd) x[rd] = u2d(RD32(a), RD32(a + 4));
        break;
    }
    /* ---- stores ---- */
    case OP_ST_B: case OP_ST_B_USR: case OP_ST_B_WT: case OP_ST_B_USR_WT: WR8(xlate(ea(1)), (uint8_t)D); MW(xlate(ea(1)), D & 0xFF, 1, true); break;
    case OP_ST_H: case OP_ST_H_USR: case OP_ST_H_WT: case OP_ST_H_USR_WT: WR16(xlate(ea(2)) & ~1u, (uint16_t)D); MW(xlate(ea(2)) & ~1u, D & 0xFFFF, 2, true); break;
    case OP_ST: case OP_ST_USR: case OP_ST_WT: case OP_ST_USR_WT: {
        uint32_t a = xlate(ea(4)) & ~3u;
        WR32(a, xrf_form ? f2u((float)(rd ? x[rd] : 0.0)) : D); MW(a, xrf_form ? f2u((float)(rd ? x[rd] : 0.0)) : D, 4, true);
        break;
    }
    case OP_ST_D: case OP_ST_D_USR: case OP_ST_D_WT: case OP_ST_D_USR_WT: {
        uint32_t a = xlate(ea(8)) & ~7u;
        if (xrf_form) { uint64_t u = d2u(rd ? x[rd] : 0.0); WR32(a, (uint32_t)(u >> 32)); WR32(a + 4, (uint32_t)u); }
        else { WR32(a, D); WR32(a + 4, r[(rd + 1) & 31]); }
        MW(a, RD32(a), 4, true); MW(a + 4, RD32(a + 4), 4, true);
        break;
    }
    case OP_ST_X: case OP_ST_X_USR: case OP_ST_X_WT: case OP_ST_X_USR_WT: {
        uint32_t a = xlate(ea(16)) & ~15u;
        uint64_t u = d2u(rd ? x[rd] : 0.0);
        WR32(a, (uint32_t)(u >> 32)); WR32(a + 4, (uint32_t)u); WR32(a + 8, 0); WR32(a + 12, 0);
        break;
    }
    case OP_XMEM: case OP_XMEM_USR: { uint32_t a = xlate(ea(4)) & ~3u; uint32_t t = RD32(a); WR32(a, D); D = t; break; }
    case OP_XMEM_BU: case OP_XMEM_BU_USR: { uint32_t a = xlate(ea(1)); uint32_t t = RD8(a); WR8(a, (uint8_t)D); D = t; break; }
    case OP_LDA: case OP_LDA_USR: D = S1 + r[rs2] * 4; break;
    case OP_LDA_D: case OP_LDA_D_USR: D = S1 + r[rs2] * 8; break;
    case OP_LDA_H: case OP_LDA_H_USR: D = S1 + r[rs2] * 2; break;
    case OP_LDA_X: case OP_LDA_X_USR: D = S1 + r[rs2] * 16; break;

    /* ---- logical ---- */
    case OP_AND: D = triadic ? (S1 & S2) : (S1 & (0xFFFF0000u | imm)); break;
    case OP_AND_C: D = S1 & ~S2; break;
    case OP_AND_U: D = S1 & ((imm << 16) | 0xFFFFu); break;
    case OP_MASK: D = S1 & imm; break;
    case OP_MASK_U: D = S1 & (imm << 16); break;
    case OP_XOR: D = S1 ^ S2; break;
    case OP_XOR_C: D = S1 ^ ~S2; break;
    case OP_XOR_U: D = S1 ^ (imm << 16); break;
    case OP_OR: D = S1 | S2; break;
    case OP_OR_C: D = S1 | ~S2; break;
    case OP_OR_U: D = S1 | (imm << 16); break;

    /* ---- arithmetic ---- */
    case OP_ADDU: case OP_ADD: D = S1 + S2; break;
    case OP_SUBU: case OP_SUB: D = S1 - S2; break;
    case OP_ADDU_CO: case OP_ADD_CO: { uint64_t t = (uint64_t)S1 + S2; D = (uint32_t)t; cr[1] = (cr[1] & ~(1u << 28)) | ((uint32_t)(t >> 32) << 28); break; }
    case OP_ADDU_CI: case OP_ADD_CI: D = S1 + S2 + ((cr[1] >> 28) & 1u); break;
    case OP_ADDU_CIO: case OP_ADD_CIO: { uint64_t t = (uint64_t)S1 + S2 + ((cr[1] >> 28) & 1u); D = (uint32_t)t; cr[1] = (cr[1] & ~(1u << 28)) | ((uint32_t)(t >> 32) << 28); break; }
    case OP_SUBU_CO: case OP_SUB_CO: { uint64_t t = (uint64_t)S1 + (uint32_t)~S2 + 1u; D = (uint32_t)t; cr[1] = (cr[1] & ~(1u << 28)) | ((uint32_t)(t >> 32) << 28); break; }
    case OP_SUBU_CI: case OP_SUB_CI: D = S1 + ~S2 + ((cr[1] >> 28) & 1u); break;
    case OP_SUBU_CIO: case OP_SUB_CIO: { uint64_t t = (uint64_t)S1 + (uint32_t)~S2 + ((cr[1] >> 28) & 1u); D = (uint32_t)t; cr[1] = (cr[1] & ~(1u << 28)) | ((uint32_t)(t >> 32) << 28); break; }
    case OP_MULU: D = S1 * S2; break;
    case OP_MULS: D = (uint32_t)((int32_t)S1 * (int32_t)S2); break;
    case OP_MULU_D: { uint64_t t = (uint64_t)S1 * S2; D = (uint32_t)(t >> 32); r[(rd + 1) & 31] = (uint32_t)t; break; }
    case OP_DIVU: D = S2 ? S1 / S2 : 0; break;
    case OP_DIVS: D = S2 ? (uint32_t)((int32_t)S1 / (int32_t)S2) : 0; break;
    case OP_DIVU_D: { uint64_t n = ((uint64_t)S1 << 32) | r[(rs1 + 1) & 31]; uint64_t q = S2 ? n / S2 : 0;
                      D = (uint32_t)(q >> 32); r[(rd + 1) & 31] = (uint32_t)q; break; }
    case OP_CMP: D = cmp_bits(S1, d.mode == AM_SIMM16 ? (uint32_t)simm : S2); break;

    /* ---- bit fields ---- */
    case OP_CLR: case OP_SET: case OP_EXT: case OP_EXTU: case OP_MAK: case OP_ROT: {
        const uint32_t spec = d.mode == AM_BITFIELD ? (inst & 0x3FFu) : (S2 & 0x3FFu);
        const unsigned w = (spec >> 5) & 31, o = spec & 31;
        const uint32_t m = bf_mask(w);
        switch (d.op) {
            case OP_CLR: D = S1 & ~(m << o); break;
            case OP_SET: D = S1 | (m << o); break;
            case OP_EXT: {
                int32_t v = (int32_t)S1 >> o;
                if (w) v = (int32_t)((uint32_t)v << (32 - w)) >> (32 - w);
                D = (uint32_t)v; break;
            }
            case OP_EXTU: D = (S1 >> o) & m; break;
            case OP_MAK: D = (S1 & m) << o; break;
            case OP_ROT: D = o ? (S1 >> o) | (S1 << (32 - o)) : S1; break;
            default: break;
        }
        break;
    }
    case OP_FF1: { uint32_t v = S2; int b = 31; while (b >= 0 && !(v & (1u << b))) b--; D = b < 0 ? 32 : (uint32_t)b; break; }
    case OP_FF0: { uint32_t v = ~S2; int b = 31; while (b >= 0 && !(v & (1u << b))) b--; D = b < 0 ? 32 : (uint32_t)b; break; }

    /* ---- flow ---- */
    case OP_BR: case OP_BR_N: case OP_BSR: case OP_BSR_N: {
        int32_t disp = (int32_t)((inst & 0x03FFFFFFu) << 6) >> 4;
        const bool n = d.op == OP_BR_N || d.op == OP_BSR_N;
        if (d.op == OP_BSR || d.op == OP_BSR_N) r[1] = ipc + (n ? 8 : 4);
        branch_to(ipc + (uint32_t)disp, n);
        break;
    }
    case OP_BB0: case OP_BB0_N: case OP_BB1: case OP_BB1_N: {
        const bool set = (S1 >> rd) & 1u;
        const bool take = (d.op == OP_BB1 || d.op == OP_BB1_N) ? set : !set;
        if (take) branch_to(ipc + (uint32_t)(simm * 4), d.op == OP_BB0_N || d.op == OP_BB1_N);
        break;
    }
    case OP_BCND: case OP_BCND_N: {
        const int32_t v = (int32_t)S1;
        unsigned c = v > 0 ? 1u : v == 0 ? 2u : (S1 == 0x80000000u ? 8u : 4u);
        if (rd & c) branch_to(ipc + (uint32_t)(simm * 4), d.op == OP_BCND_N);
        break;
    }
    case OP_JMP: case OP_JMP_N: branch_to(r[rs2] & ~3u, d.op == OP_JMP_N); break;
    case OP_JSR: case OP_JSR_N: {
        const uint32_t t = r[rs2] & ~3u;
        r[1] = ipc + (d.op == OP_JSR_N ? 8 : 4);
        branch_to(t, d.op == OP_JSR_N);
        break;
    }
    case OP_RTE: cr[1] = cr[2]; pc = cr[4] & ~3u; break;
    case OP_TB0: if (!((S1 >> rd) & 1u)) trap(inst & 0x1FFu); break;
    case OP_TB1: if ((S1 >> rd) & 1u) trap(inst & 0x1FFu); break;
    case OP_TCND: {
        const int32_t v = (int32_t)S1;
        unsigned c = v > 0 ? 1u : v == 0 ? 2u : (S1 == 0x80000000u ? 8u : 4u);
        if (rd & c) trap(inst & 0x1FFu);
        break;
    }
    case OP_TBND: if (S1 > S2) trap(5); break;

    /* ---- control registers ---- */
    case OP_LDCR: D = cr[(inst >> 5) & 63]; break;
    case OP_STCR: case OP_XCR: {
        const unsigned c = (inst >> 5) & 63;
        const uint32_t v = r[rs2], old = cr[c];
        cr[c] = v;
        if (c == 0) cr[0] = old;                  /* PID is read-only */
        if (c == 46) {                             /* dbp: write BATC entry selected by dir */
            const unsigned i = cr[45] & 7u;
            batc_entry[i] = v;
            rebuild_batc();
        }
        if (d.op == OP_XCR) D = old;
        break;
    }
    case OP_FLDCR: D = fcr[(inst >> 5) & 63]; break;
    case OP_FSTCR: fcr[(inst >> 5) & 63] = r[rs2]; break;
    case OP_FXCR: { const unsigned c = (inst >> 5) & 63; uint32_t old = fcr[c]; fcr[c] = r[rs2]; D = old; break; }

    /* ---- graphics unit (64-bit register pairs; see m88110_core.md) ---- */
    case OP_PADD: case OP_PADD_B: case OP_PADD_H: case OP_PSUB: case OP_PSUB_B: case OP_PSUB_H:
    case OP_PADDS_U: case OP_PADDS_U_B: case OP_PADDS_U_H: case OP_PADDS_US: case OP_PADDS_US_B: case OP_PADDS_US_H:
    case OP_PADDS_S: case OP_PADDS_S_B: case OP_PADDS_S_H:
    case OP_PSUBS_U: case OP_PSUBS_U_B: case OP_PSUBS_U_H: case OP_PSUBS_US: case OP_PSUBS_US_B: case OP_PSUBS_US_H:
    case OP_PSUBS: case OP_PSUBS_B: case OP_PSUBS_H: {
        const unsigned t = (inst >> 5) & 3;              /* 1 = byte, 2 = half, 3 = word */
        const unsigned sat = (inst >> 7) & 3;            /* 0 none, 1 unsigned, 2 unsigned+signed, 3 signed */
        const bool sub = ((inst >> 12) & 1u) != 0;
        const unsigned bits = t == 1 ? 8 : t == 2 ? 16 : 32;
        const uint64_t a = ((uint64_t)r[rs1] << 32) | r[(rs1 + 1) & 31];
        const uint64_t b = ((uint64_t)r[rs2] << 32) | r[(rs2 + 1) & 31];
        uint64_t out = 0;
        for (unsigned f = 0; f < 64; f += bits) {
            const uint64_t fm = bits == 64 ? ~0ull : ((1ull << bits) - 1);
            int64_t va = (int64_t)((a >> f) & fm), vb = (int64_t)((b >> f) & fm);
            if (sat == 3 || (sat == 2 && false)) {    /* signed fields */
                va = (va << (64 - bits)) >> (64 - bits);
                vb = (vb << (64 - bits)) >> (64 - bits);
            }
            if (sat == 2) vb = (vb << (64 - bits)) >> (64 - bits);   /* unsigned + signed */
            int64_t res = sub ? va - vb : va + vb;
            if (sat == 1 || sat == 2) { const int64_t mx = (int64_t)fm; if (res < 0) res = 0; if (res > mx) res = mx; }
            else if (sat == 3) {
                const int64_t mx = (int64_t)(fm >> 1), mn = -mx - 1;
                if (res < mn) res = mn; if (res > mx) res = mx;
            }
            out |= ((uint64_t)res & fm) << f;
        }
        D = (uint32_t)(out >> 32);
        r[(rd + 1) & 31] = (uint32_t)out;
        break;
    }
    case OP_PCMP: {
        /* compare two 64-bit values field-wise as 32-bit halves; give cmp-style bits for the low word [inferred] */
        D = cmp_bits(r[rs1], r[rs2]);
        break;
    }
    case OP_PMUL: {
        /* the 64-bit rS1 pair times the unsigned 32-bit rS2, low 64 bits. Not field-wise: the 16-bit Gouraud set-up (BOX
           draw16B 0xce04) multiplies two 32-bit fields of colour differences by 0x10000/width and keeps their high halves;
           the bitmap blend (byte fields x alpha <= 256) never carries between fields, so it is the same either way. */
        const uint64_t a = ((uint64_t)r[rs1] << 32) | r[(rs1 + 1) & 31];
        const uint64_t out = a * (uint64_t)r[rs2];
        D = (uint32_t)(out >> 32);
        r[(rd + 1) & 31] = (uint32_t)out;
        break;
    }
    case OP_PUNPK_N: case OP_PUNPK_B: case OP_PUNPK_H: {
        /* zero-extend each field of the 32-bit rS1 to double width (field in the low half) [inferred] */
        const unsigned bits = d.op == OP_PUNPK_N ? 4 : d.op == OP_PUNPK_B ? 8 : 16;
        uint64_t out = 0;
        for (unsigned i = 0; i * bits < 32; i++) out |= (uint64_t)((S1 >> (i * bits)) & ((1u << bits) - 1)) << (i * 2 * bits);
        D = (uint32_t)(out >> 32);
        r[(rd + 1) & 31] = (uint32_t)out;
        break;
    }
    case OP_PPACK_8: case OP_PPACK_16: case OP_PPACK_16_H: case OP_PPACK_32: case OP_PPACK_32_B: case OP_PPACK_32_H: {
        /* pack: the high-order (R / fields) bits of each T-sized field of the 64-bit rS2 pair form an R-bit value;
         * the 64-bit rS1 pair is shifted left by R and the packed value inserted below; result in the rD pair.
         * (Firmware always consumes rD+1, and chains two ppack.32 to fill a pair before st.d.) */
        const unsigned rbits = d.op == OP_PPACK_8 ? 8 : (d.op == OP_PPACK_16 || d.op == OP_PPACK_16_H) ? 16 : 32;
        const unsigned t = (inst >> 5) & 3;
        const unsigned fbits = t == 1 ? 8 : t == 2 ? 16 : 32;
        const uint64_t a = ((uint64_t)r[rs1] << 32) | r[(rs1 + 1) & 31];
        const uint64_t b = ((uint64_t)r[rs2] << 32) | r[(rs2 + 1) & 31];
        const unsigned nf = 64 / fbits;
        const unsigned per = rbits / nf;
        uint64_t packed = 0;
        for (unsigned i = 0; i < nf; i++) {
            const uint64_t field = (b >> (i * fbits)) & (fbits == 64 ? ~0ull : ((1ull << fbits) - 1));
            packed |= ((field >> (fbits - per)) & ((1ull << per) - 1)) << (i * per);
        }
        const uint64_t out = (a << rbits) | packed;
        D = (uint32_t)(out >> 32);
        r[(rd + 1) & 31] = (uint32_t)out;
        break;
    }
    case OP_PROT: {
        const unsigned o = d.mode == AM_IMM6 ? ((inst >> 5) & 63) : (r[rs2] & 63);
        const uint64_t a = ((uint64_t)r[rs1] << 32) | r[(rs1 + 1) & 31];
        const uint64_t out = o ? (a << o) | (a >> (64 - o)) : a;
        D = (uint32_t)(out >> 32);
        r[(rd + 1) & 31] = (uint32_t)out;
        break;
    }

    default: {
        /* floating point */
        if (d.mode == AM_FP) {
            const bool xrf = (inst >> 15) & 1u;
            switch (d.fk) {
            case FK_FLT:
                if (d.op == OP_FLT_X) { if (rd) x[rd] = (double)(int32_t)r[rs2]; }
                else fset(rd, d.td, d.op == OP_FLT_XS, (double)(int32_t)r[rs2]);
                return;
            case FK_INT: case FK_NINT: case FK_TRNC: {
                const double v = fget(rs2, d.t2, xrf || d.t2 == 2);
                D = (uint32_t)(d.fk == FK_TRNC ? (int32_t)v : round_nearest_even(v));
                return;
            }
            case FK_MOV: {
                const uint32_t grp = inst & 0xFC00FFE0u;
                if (grp == 0x84004200u) { if (rd) x[rd] = u2f(r[rs2]); }
                else if (grp == 0x84004280u) { if (rd) x[rd] = u2d(r[rs2], r[(rs2 + 1) & 31]); }
                else if (grp == 0x8400C000u) { D = f2u((float)x[rs2]); }
                else if (grp == 0x8400C080u) { uint64_t u = d2u(x[rs2]); D = (uint32_t)(u >> 32); r[(rd + 1) & 31] = (uint32_t)u; }
                else { if (rd) x[rd] = x[rs2]; }
                return;
            }
            case FK_CVT: fset(rd, d.td, xrf, fget(rs2, d.t2, xrf)); return;
            case FK_NONE: break;
            default: {
                const double a = fget(rs1, d.t1, xrf), b = fget(rs2, d.t2, xrf);
                double v = 0;
                switch (d.fk) {
                    case FK_CMP: D = fcmp_bits(a, b); return;
                    case FK_ADD: v = a + b; break;
                    case FK_SUB: v = a - b; break;
                    case FK_MUL: v = a * b; break;
                    case FK_DIV: v = a / b; break;
                    case FK_SQRT: v = sqrt(b); break;
                    default: break;
                }
                fset(rd, d.td, xrf, v);
                return;
            }
            }
        }
        unknown_count++;
        last_unknown_inst = inst;
        last_unknown_pc = ipc;   /* unimplemented: logged by the caller, executed as a no-op */
        break;
    }
    }
}
