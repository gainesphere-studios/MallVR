/*
 *  Virtuality "format card" (CTRL library: buttons, joystick, credits, lights, Visette brightness, audio) - first model.
 *
 *  Derived from the CTRLI_FCD_* functions in SFL.EXE (symbols from SFL.SYM); see su2000/docs/findings/format_card.md.
 *
 *  Shared memory (CONFIG.VPC [CTRL] MEM_ADDRESS, 2 KB per card, e.g. 0xE0000 / 0xE0800), byte offsets:
 *    0x00        firmware version, BCD (version = hi*100 + lo*10)              CTRLI_FCD_Open
 *    0x02..0x05  free-running counter, little-endian; must advance over 300 ms   CTRLI_FCD_Open ("card alive")
 *    0x06        card type: 0x81 or 0x82 (anything else: "Format C not found")   CTRLI_FCD_Open
 *    0x0A..0x0D  joystick axes (two sticks, x/y)                                 CTRLI_FCD_ReadJoystick
 *    0x12, 0x13  buttons 4..11, 12..19                                           CTRLI_FCD_ReadButton
 *    0x14        bits 0..3 buttons 0..3, bit 4 = credit (coin)                   CTRLI_FCD_ReadButton/GetCredit
 *    0x17..0x19  registers 0x14..0x16 of the register map
 *    0x400       command mailbox: the host waits for 0, then writes a command      CTRLI_FCD_SendCommand
 *    0x401..0x408 command parameters / outputs (lights, brightness, volume ...)
 *  I/O ports (CONFIG.VPC [CTRL] IO_ADDRESS, e.g. 0x210):
 *    +0..+3      8254-style timer. Counters 1 and 2 (mode 2, reload 0xFFFF) form a 32-bit down-counter at
 *                1 MHz; 0xDC at +3 latches both, then +1 lo/hi and +2 lo/hi are read   CTRLI_FCD_InitTimers/GetSystemTime
 *    +4, +5      written during open (0x18/0x10, 0x83/6/0x84/6): accepted, meaning unknown
 *  What is faked: the card's microcontroller is not emulated; commands are acknowledged at once and the
 *  outputs (lights, brightness, volume) are only stored.
 */

#include "dosbox.h"
#include <stdlib.h>
#include "inout.h"
#include "mem.h"
#include "paging.h"
#include "pic.h"
#include "logging.h"
#include "mapper.h"
#include "su2000.h"

#include <string.h>

namespace {

struct FormatCard {
    uint32_t io = 0, mem = 0;
    uint8_t ram[0x800];
    double timer_t0 = 0;          /* emulated ms when the 8254 counters were (re)programmed */
    uint32_t latched = 0xFFFFFFFFu;
    unsigned latch_reads[3] = {0, 0, 0};
    uint8_t mix[32][2] = {};      /* mixer values per register, left / right */
    uint8_t mix_reg = 0;
    bool mix_have_reg = false;
    bool micnet_used = false;    /* the game has set the MICNET (other players' microphones) level */
    IO_ReadHandleObject rd;
    IO_WriteHandleObject wr;
};

FormatCard cards[2];
unsigned ncards = 0;
uint32_t page_base = 0;
bool installed = false;

/* Inputs from the mapper, shared by all cards (one player per card is a later refinement). */
int joy_x = 0, joy_y = 0;    /* -1, 0, +1 from Ctrl+arrow keys */
uint16_t in_buttons = 0;     /* bits 0..3 -> byte 0x14 bits 0..3, bits 4..11 -> byte 0x13 */
uint16_t card_buttons[2] = { 0, 0 };   /* one card only: a headset player's buttons (player n = card n) */
bool in_coin = false;

FormatCard *card_for_mem(uint32_t a) {
    for (unsigned i = 0; i < ncards; i++)
        if (a >= cards[i].mem && a < cards[i].mem + 0x800) return &cards[i];
    return NULL;
}

void refresh(FormatCard &c) {
    /* counter at 0x02..0x05: milliseconds of emulated time */
    const uint32_t ms = (uint32_t)PIC_FullIndex();
    c.ram[2] = (uint8_t)ms; c.ram[3] = (uint8_t)(ms >> 8); c.ram[4] = (uint8_t)(ms >> 16); c.ram[5] = (uint8_t)(ms >> 24);
    /* Buttons are active-high. DAC (CTRL v01.03.51) reads player triggers from 0x13 (bits 0..3 player 1,
       4..7 player 2) and the start button from the button word bit 0/1 (= 0x14 bits 0/1). The player's
       first trigger also drives the start bit. */
    /* Ctrl+1..4 = player 1 buttons (0x13 bits 0..3), Ctrl+5..8 = player 2 buttons (bits 4..7). Driving both from one
       key starts a two-player game with an untracked second player. */
    const uint16_t btn = (uint16_t)(in_buttons | card_buttons[&c - cards < 2 ? &c - cards : 0]);
    const uint8_t p13 = (uint8_t)(btn >> 4);
    c.ram[0x14] = (uint8_t)((c.ram[0x14] & 0xE0u) | (btn & 0x0Fu) | ((p13 & 0x01u) ? 1u : 0u) |
                            ((p13 & 0x10u) ? 2u : 0u) | (in_coin ? 0x10u : 0u));
    c.ram[0x13] = p13;
    c.ram[0x12] = 0;
    /* joystick: the game uses 0x7F - raw, so 0x7F is centre */
    c.ram[0x0A] = (uint8_t)(0x7F + joy_x * 0x60);
    c.ram[0x0B] = (uint8_t)(0x7F + joy_y * 0x60);
    c.ram[0x0C] = c.ram[0x0A];
    c.ram[0x0D] = c.ram[0x0B];
}

class FcardWindow : public PageHandler {
public:
    FcardWindow() : PageHandler(PFLAG_NOCODE) {}
    uint8_t readb(PhysPt addr) override {
        FormatCard *c = card_for_mem((uint32_t)addr);
        if (!c) return 0xFF;
        const uint32_t o = (uint32_t)addr - c->mem;
        if (o <= 0x14) refresh(*c);
        const uint8_t v = c->ram[o];
        SU2K_Log(SU2K_MEM_READ, 1, (uint32_t)addr, v, o);
        return v;
    }
    void writeb(PhysPt addr, uint8_t val) override {
        FormatCard *c = card_for_mem((uint32_t)addr);
        if (!c) return;
        const uint32_t o = (uint32_t)addr - c->mem;
        SU2K_Log(SU2K_MEM_WRITE, 1, (uint32_t)addr, val, o);
        if (o == 0x400) {
            /* command mailbox: no microcontroller - acknowledge at once */
            static unsigned logged = 0;
            if (val && logged++ < 16) LOG_MSG("SU2000: format card command %02x (params %02x %02x %02x %02x)", val,
                                              c->ram[0x401], c->ram[0x402], c->ram[0x403], c->ram[0x404]);
            c->ram[o] = 0;
            return;
        }
        c->ram[o] = val;
    }
    uint16_t readw(PhysPt addr) override { return (uint16_t)(readb(addr) | (readb(addr + 1) << 8)); }
    uint32_t readd(PhysPt addr) override { return (uint32_t)readw(addr) | ((uint32_t)readw(addr + 2) << 16); }
    void writew(PhysPt addr, uint16_t v) override { writeb(addr, (uint8_t)v); writeb(addr + 1, (uint8_t)(v >> 8)); }
    void writed(PhysPt addr, uint32_t v) override { writew(addr, (uint16_t)v); writew(addr + 2, (uint16_t)(v >> 16)); }
};

FcardWindow window;

FormatCard *card_for_port(Bitu port) {
    for (unsigned i = 0; i < ncards; i++)
        if (port >= cards[i].io && port < cards[i].io + 8) return &cards[i];
    return NULL;
}

/* 32-bit 1 MHz down-counter from cascaded 8254 counters 1 and 2 */
uint32_t timer_value(FormatCard &c) {
    const double us = (PIC_FullIndex() - c.timer_t0) * 1000.0;
    return 0xFFFFFFFFu - (uint32_t)(uint64_t)us;
}

Bitu io_read(Bitu port, Bitu iolen) {
    FormatCard *c = card_for_port(port);
    Bitu v = 0xFF;
    if (c) {
        const unsigned r = (unsigned)(port - c->io);
        if (r == 1 || r == 2) {
            /* counter 1 = low 16 bits, counter 2 = high 16 bits; low byte first */
            const uint32_t word = r == 1 ? (c->latched & 0xFFFFu) : (c->latched >> 16);
            v = (c->latch_reads[r]++ & 1) ? (word >> 8) & 0xFF : word & 0xFF;
        } else v = 0;
    }
    SU2K_Log(SU2K_IO_READ, (uint8_t)iolen, (uint32_t)port, (uint32_t)v);
    return v;
}

void io_write(Bitu port, Bitu val, Bitu iolen) {
    SU2K_Log(SU2K_IO_WRITE, (uint8_t)iolen, (uint32_t)port, (uint32_t)val);
    FormatCard *c = card_for_port(port);
    if (!c) return;
    const unsigned r = (unsigned)(port - c->io);
    if (r == 5) {
        /* mixer: register byte, then value byte (CTRLI_FCD_UpdateMixer); register | 0x20 = left only, | 0x40 = right only */
        if (!c->mix_have_reg) { c->mix_reg = (uint8_t)val; c->mix_have_reg = true; }
        else {
            c->mix_have_reg = false;
            const unsigned reg = c->mix_reg & 0x1Fu, side = c->mix_reg & 0x60u;
            if (reg == 0x14 && (val & 0x1Fu)) c->micnet_used = true;
            if (reg == 0x14) {
                static unsigned n = 0;
                if (n++ < 40) LOG_MSG("SU2000: format card %u MICNET %s = %u", (unsigned)(c - cards),
                                      side == 0x20 ? "left" : side == 0x40 ? "right" : "both", (unsigned)(val & 0x1Fu));
            }
            if (side != 0x40) c->mix[reg][0] = (uint8_t)val;
            if (side != 0x20) c->mix[reg][1] = (uint8_t)val;
            static unsigned logged = 0;
            if (logged++ < 32) LOG_MSG("SU2000: format card %u mixer reg %02x = %02x", (unsigned)(c - cards), c->mix_reg, (unsigned)val);
        }
    }
    if (r == 3) {
        if ((val & 0xC0) == 0xC0) {            /* read-back command: latch the counters */
            c->latched = timer_value(*c);
            c->latch_reads[1] = c->latch_reads[2] = 0;
        } else if ((val & 0x30) == 0x30) {     /* counter programmed (lo/hi): restart */
            c->timer_t0 = PIC_FullIndex();
            c->latched = 0xFFFFFFFFu;
        }
    }
}

void map_button(unsigned bit, bool pressed) {
    if (pressed) in_buttons |= (uint16_t)(1u << bit); else in_buttons &= (uint16_t)~(1u << bit);
}
void key_coin(bool pressed) { in_coin = pressed; }
void key_b0(bool pressed) { map_button(0, pressed); }
void key_b1(bool pressed) { map_button(1, pressed); }
void key_b2(bool pressed) { map_button(2, pressed); }
void key_b3(bool pressed) { map_button(3, pressed); }
void key_left(bool p)  { joy_x = p ? -1 : (joy_x < 0 ? 0 : joy_x); }
void key_right(bool p) { joy_x = p ? 1 : (joy_x > 0 ? 0 : joy_x); }
void key_up(bool p)    { joy_y = p ? -1 : (joy_y < 0 ? 0 : joy_y); }
void key_down(bool p)  { joy_y = p ? 1 : (joy_y > 0 ? 0 : joy_y); }
void key_b4(bool p) { map_button(4, p); }
void key_b5(bool p) { map_button(5, p); }
void key_b6(bool p) { map_button(6, p); }
void key_b7(bool p) { map_button(7, p); }
void key_b8(bool p) { map_button(8, p); }
void key_b9(bool p) { map_button(9, p); }
void key_b10(bool p) { map_button(10, p); }
void key_b11(bool p) { map_button(11, p); }

} // namespace

void FCARD_Setup(const uint32_t *io, const uint32_t *mem, unsigned n) {
    FCARD_Shutdown();
    ncards = n > 2 ? 2 : n;
    page_base = 0;
    for (unsigned i = 0; i < ncards; i++) {
        FormatCard &c = cards[i];
        c.io = io[i];
        c.mem = mem[i];
        memset(c.ram, 0, sizeof(c.ram));
        c.ram[0x00] = 0x13;          /* firmware "1.3" [faked] */
        c.ram[0x06] = 0x82;          /* card type: CTRLI_FCD_Open takes 0x81 or 0x82, but with 0x81 DN2 (two cards) only opens card 2 */
        c.ram[0x0A] = c.ram[0x0B] = c.ram[0x0C] = c.ram[0x0D] = 0x7F;   /* joysticks centred (game uses 0x7F - raw) */
        c.timer_t0 = PIC_FullIndex();
        c.rd.Install(c.io, io_read, IO_MA, 8);
        c.wr.Install(c.io, io_write, IO_MA, 8);
        const uint32_t page = c.mem & ~0xFFFu;
        if (page != page_base) { MEM_SetPageHandler(page >> 12, 1, &window); page_base = page; }
    }
    PAGING_ClearTLB();
    installed = true;
    if (ncards) LOG_MSG("SU2000: format card(s): %u at %03xh/%05xh%s", ncards, cards[0].io, cards[0].mem, ncards > 1 ? " (+1)" : "");
}

void FCARD_Shutdown(void) {
    if (!installed) return;
    for (unsigned i = 0; i < ncards; i++) {
        cards[i].rd.Uninstall();
        cards[i].wr.Uninstall();
        MEM_ResetPageHandler_Unmapped((cards[i].mem & ~0xFFFu) >> 12, 1);
    }
    PAGING_ClearTLB();
    ncards = 0;
    installed = false;
}

void FCARD_GetStick(int &x, int &y) { x = joy_x; y = joy_y; }
void FCARD_SetButton(unsigned bit, bool pressed) { map_button(bit, pressed); }
uint16_t FCARD_GetButtons(void) { return in_buttons | card_buttons[0]; }
void FCARD_SetPlayerButton(unsigned card, unsigned bit, bool pressed) {
    if (card > 1) return;
    if (pressed) card_buttons[card] |= (uint16_t)(1u << bit); else card_buttons[card] &= (uint16_t)~(1u << bit);
}
/* Visette microphone level of one card (player), 0..255: shared-memory byte 0x17, read by CTRL_GetMic */
void FCARD_SetMicLevel(unsigned card, uint8_t level) { if (card < ncards) cards[card].ram[0x17] = level; }

/* Left / right gain (0..1) the game gives the MICNET input of a card: signal 0x20, mixer register 0x14 (DAC.EXE signal
   table at file offset 0xf1210), 5-bit levels. DAC sets it from the opponent's direction (SOUND_handle -> CTRL_SetFade).
   Returns false while the game has not set it. */
bool FCARD_GetMicnet(unsigned card, float &left, float &right) {
    if (card >= ncards || !cards[card].micnet_used) return false;
    left = (float)(cards[card].mix[0x14][0] & 0x1Fu) / 31.0f;
    right = (float)(cards[card].mix[0x14][1] & 0x1Fu) / 31.0f;
    return true;
}

void FCARD_AddMapperKeys(void) {
    MAPPER_AddHandler(key_coin, MK_9, MMOD1, "su2k_coin", "SU2000 coin");
    MAPPER_AddHandler(key_b0, MK_f6, MMOD1, "su2k_btn0", "SU2000 button 0");
    MAPPER_AddHandler(key_b1, MK_f7, MMOD1, "su2k_btn1", "SU2000 button 1");
    MAPPER_AddHandler(key_b2, MK_f8, MMOD1, "su2k_btn2", "SU2000 button 2");
    MAPPER_AddHandler(key_b3, MK_0, MMOD1, "su2k_btn3", "SU2000 button 3");
    MAPPER_AddHandler(key_left, MK_leftarrow, MMOD1, "su2k_left", "SU2000 stick left");
    MAPPER_AddHandler(key_right, MK_rightarrow, MMOD1, "su2k_right", "SU2000 stick right");
    MAPPER_AddHandler(key_up, MK_uparrow, MMOD1, "su2k_up", "SU2000 stick up");
    MAPPER_AddHandler(key_down, MK_downarrow, MMOD1, "su2k_down", "SU2000 stick down");
    /* byte 0x13 bits 0..7: Ctrl+1..8 */
    MAPPER_AddHandler(key_b4, MK_1, MMOD1, "su2k_btn4", "SU2000 button 4");
    MAPPER_AddHandler(key_b5, MK_2, MMOD1, "su2k_btn5", "SU2000 button 5");
    MAPPER_AddHandler(key_b6, MK_3, MMOD1, "su2k_btn6", "SU2000 button 6");
    MAPPER_AddHandler(key_b7, MK_4, MMOD1, "su2k_btn7", "SU2000 button 7");
    MAPPER_AddHandler(key_b8, MK_5, MMOD1, "su2k_btn8", "SU2000 button 8");
    MAPPER_AddHandler(key_b9, MK_6, MMOD1, "su2k_btn9", "SU2000 button 9");
    MAPPER_AddHandler(key_b10, MK_7, MMOD1, "su2k_btn10", "SU2000 button 10");
    MAPPER_AddHandler(key_b11, MK_8, MMOD1, "su2k_btn11", "SU2000 button 11");
}
