/*
 *  Ensoniq Soundscape (Virtuality SU2000 sound) - high-level emulation.
 *
 *  Port map (Soundscape host interface; matches the transport in the SU2000 SND library, DAC.EXE 0xb0dce):
 *    base+0/1  MIDI UART (MPU-401 compatible data/status)
 *    base+2    host port control/status: bit0 = byte waiting for the PC, bit1 = card ready, bit2 = byte is a control byte
 *    base+3    host port data
 *    base+4/5  ODIE gate array: index (4 bits) / data
 *  The SND library sends each byte as  out base+2, channel code (0x81 host commands, 0x85 MIDI);  out base+3, byte.
 *
 *  Not emulated: the 68000 and its firmware (SNDSCAPE.COD is taken by DMA and dropped). Instead the host commands the SND
 *  library uses are interpreted (see su2000/docs/findings/sound.md):
 *    0x80 download sample (id, length, format, loop, pitch), data follows by ODIE DMA B
 *    0x86 patch (id, sample id, ...)       0x87 program (id, patch id, ...)
 *  and the MIDI stream (program change, note on/off, volume CC7, pan CC10, pitch bend) plays the samples.
 */

#include "dosbox.h"
#include "inout.h"
#include "logging.h"
#include "su2000.h"
#include "dma.h"
#include "pic.h"
#include "mixer.h"

#include <deque>
#include <map>
#include <vector>
#include <string>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>

namespace {

const unsigned MIX_RATE = 44100;

struct Sample {
    std::vector<int16_t> pcm;
    uint32_t length = 0;          /* bytes announced by the download command */
    bool eight_bit = true;
    double rate = 22050.0;
    bool loop = false;
};

struct Voice {
    bool on = false;
    unsigned ch = 0, note = 0;
    const Sample *s = nullptr;
    double pos = 0.0, step = 0.0;
    float vel = 1.0f;
};

struct Channel {
    unsigned program = 0;
    float volume = 1.0f, pan = 0.5f, bend = 0.0f;   /* bend in semitones */
};

struct Soundscape {
    uint32_t base = 0;
    uint8_t odie_index = 0;
    uint8_t odie[32] = { 0 };
    uint8_t host_ctrl = 0;
    std::deque<uint16_t> to_pc;               /* bytes for the PC; bit 8 = control byte (host status bit 2) */
    unsigned irq = 7;
    int dma = -1;                             /* 8-bit DMA channel, -1 = the first unmasked one */
    unsigned index = 0;
    IO_ReadHandleObject rd;
    IO_WriteHandleObject wr;
    unsigned logged = 0;
    bool dma_done[2] = { false, false };      /* ODIE DMA A/B: bit 0 of reg 2/3 reads 1 when a transfer finished */
    bool firmware_done = false;

    std::vector<uint8_t> cmd;                 /* host command being received */
    std::map<unsigned, Sample> samples;
    std::map<unsigned, unsigned> patch_sample, program_patch;
    int loading = -1;                         /* sample id waiting for its DMA data */

    uint8_t midi_status = 0;
    std::vector<uint8_t> midi;
    Channel chan[16];
    Voice voices[32];
    MixerChannel *mix = nullptr;
};

/* Up to two cards (the two-player cabinet has one per player: CONFIG.VPC sound1 / sound2). The handlers select the card
   they work on through ssp; the code below uses `ss` for it. */
Soundscape cards[2];
unsigned ncards = 0;
Soundscape *ssp = &cards[0];
#define ss (*ssp)
bool installed = false;

void trace(const char *what, Bitu port, Bitu val) {
    if (ss.logged < 120) {
        ss.logged++;
        LOG_MSG("SU2000: sscape %s %03x = %02x", what, (unsigned)port, (unsigned)val);
    }
}

/* 7-bit packed little-endian fields (SND_HostWrite2/3/4) */
uint32_t field(const std::vector<uint8_t> &c, size_t at, unsigned n) {
    uint32_t v = 0;
    for (unsigned i = 0; i < n && at + i < c.size(); i++) v |= (uint32_t)(c[at + i] & 0x7Fu) << (7 * i);
    return v;
}

void send_to_pc(uint8_t b, bool control);

double wall(void) {
    static const auto t0 = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

void handle_command(void) {
    const std::vector<uint8_t> &c = ss.cmd;
    if (c.empty()) return;
    if (c[0] != 0x80 && c[0] != 0x86 && c[0] != 0x87) {
        static unsigned logged = 0;
        if (logged++ < 60) {
            std::string h;
            char b[4];
            for (size_t i = 0; i < c.size() && i < 24; i++) { snprintf(b, sizeof(b), "%02x ", c[i]); h += b; }
            LOG_MSG("SU2000: sscape cmd %s(wall %.1f s)", h.c_str(), wall());
        }
    }
    switch (c[0]) {
        case 0x80: {                          /* SNDI_DownloadSample */
            if (c.size() < 29) break;
            const unsigned id = field(c, 1, 2);
            Sample &s = ss.samples[id];
            s.length = field(c, 3, 4);
            s.eight_bit = c[7] == 0x60;
            const double pitch = (double)field(c, 24, 3);   /* (log2(44100 / rate) + 5) * 2048 */
            s.rate = 44100.0 / pow(2.0, pitch / 2048.0 - 5.0);
            s.loop = field(c, 27, 2) != 0;
            s.pcm.clear();
            ss.loading = (int)id;
            static unsigned n = 0;
            if (n++ % 10 == 0) LOG_MSG("SU2000: sscape sample %u (%u bytes, %.0f Hz) at wall %.1f s, emu %.1f s", id, s.length, s.rate, wall(), PIC_FullIndex() / 1000.0);
            break;
        }
        /* Queries: the SND library reads the answer as control bytes (7-bit values) from its reply ring (DAC.EXE 0xb10ae). */
        case 0x9F: {                          /* SND_GetFirmwareVersion: 1 + 1 + 4 x HostRead2 */
            static const uint8_t reply[10] = { 1, 5, 0, 0, 0, 0, 0, 0, 0, 0 };
            for (uint8_t b : reply) send_to_pc(b, true);
            break;
        }
        case 0x85: {                          /* SND_ReturnFreeMem: 6 x HostRead4; report 1 MB each */
            for (int i = 0; i < 6; i++) { send_to_pc(0, true); send_to_pc(0, true); send_to_pc(0x40, true); send_to_pc(0, true); }
            break;
        }
        case 0x89: case 0x99: case 0x9E: send_to_pc(0, true); break;           /* control value, MIDI emulation, synth gate */
        case 0x9B: send_to_pc(0x7F, true); send_to_pc(0x7F, true); send_to_pc(0x03, true); break;   /* MIDI channels enabled (16 bits) */
        case 0x86: if (c.size() >= 15) ss.patch_sample[field(c, 1, 2)] = field(c, 13, 2); break;
        case 0x87: if (c.size() >= 6) ss.program_patch[field(c, 1, 2)] = field(c, 4, 2); break;
        default: break;
    }
}

const Sample *program_sample(unsigned program) {
    auto p = ss.program_patch.find(program);
    const unsigned patch = p != ss.program_patch.end() ? p->second : program;
    auto q = ss.patch_sample.find(patch);
    const unsigned sample = q != ss.patch_sample.end() ? q->second : patch;
    auto s = ss.samples.find(sample);
    return (s != ss.samples.end() && !s->second.pcm.empty()) ? &s->second : nullptr;
}

double voice_step(const Voice &v) {
    return v.s->rate / MIX_RATE * pow(2.0, ((double)v.note - 60.0 + ss.chan[v.ch].bend) / 12.0);
}

void note_on(unsigned ch, unsigned note, unsigned vel) {
    static unsigned logged = 0;
    if (vel && logged++ < 300) LOG_MSG("SU2000: sscape note ch %u prog %u note %u vel %u -> %s", ch, ss.chan[ch].program, note, vel, program_sample(ss.chan[ch].program) ? "sample" : "NO SAMPLE");
    for (Voice &v : ss.voices)
        if (v.on && v.ch == ch && v.note == note) v.on = false;
    if (!vel) return;
    const Sample *s = program_sample(ss.chan[ch].program);
    if (!s) return;
    Voice *slot = &ss.voices[0];
    for (Voice &v : ss.voices) {
        if (!v.on) { slot = &v; break; }
        if (v.pos > slot->pos) slot = &v;        /* steal the oldest */
    }
    slot->on = true; slot->ch = ch; slot->note = note; slot->s = s; slot->pos = 0.0; slot->vel = vel / 127.0f;
    slot->step = voice_step(*slot);
}

void midi_message(const std::vector<uint8_t> &m) {
    const unsigned ch = m[0] & 15u;
    switch (m[0] & 0xF0u) {
        case 0x80: note_on(ch, m[1], 0); break;
        case 0x90: note_on(ch, m[1], m[2]); break;
        case 0xB0:
            if (m[1] == 7) ss.chan[ch].volume = m[2] / 127.0f;
            else if (m[1] == 10) ss.chan[ch].pan = m[2] / 127.0f;
            else if (m[1] == 120 || m[1] == 123) for (Voice &v : ss.voices) if (v.ch == ch) v.on = false;
            break;
        case 0xC0: ss.chan[ch].program = m[1]; break;
        case 0xE0:
            ss.chan[ch].bend = (float)((int)((m[2] << 7) | m[1]) - 8192) / 8192.0f * 2.0f;
            for (Voice &v : ss.voices) if (v.on && v.ch == ch) v.step = voice_step(v);
            break;
        default: break;
    }
}

void midi_byte(uint8_t b) {
    if (b >= 0xF8) return;                              /* real time */
    if (b & 0x80u) { ss.midi_status = b; ss.midi.assign(1, b); return; }
    if (!ss.midi_status) return;
    if (ss.midi.empty()) ss.midi.assign(1, ss.midi_status);   /* running status */
    ss.midi.push_back(b);
    const unsigned hi = ss.midi_status & 0xF0u;
    const size_t need = (hi == 0xC0 || hi == 0xD0) ? 2 : 3;
    if (ss.midi_status < 0xF0 && ss.midi.size() == need) { midi_message(ss.midi); ss.midi.clear(); }
}

void mix_handler(Bitu len) {
    std::vector<int16_t> out(len * 2);
    for (Bitu i = 0; i < len; i++) {
        float l = 0, r = 0;
        for (Voice &v : ss.voices) {
            if (!v.on) continue;
            const size_t n = v.s->pcm.size();
            size_t p = (size_t)v.pos;
            if (p >= n) {
                if (v.s->loop && n) { v.pos = 0; p = 0; }
                else { v.on = false; continue; }
            }
            const Channel &c = ss.chan[v.ch];
            const float x = v.s->pcm[p] * v.vel * c.volume;
            l += x * (1.0f - c.pan);
            r += x * c.pan;
            v.pos += v.step;
        }
        out[2 * i] = (int16_t)(l > 32767 ? 32767 : (l < -32768 ? -32768 : l));
        out[2 * i + 1] = (int16_t)(r > 32767 ? 32767 : (r < -32768 ? -32768 : r));
    }
    ss.mix->AddSamples_s16(len, out.data());
    /* VR link: with two cards each player's headset gets its own card */
    if (ncards > 1) XR_CardAudio(ss.index, MIX_RATE, (unsigned)len, out.data());
    /* debug: raw 16-bit stereo 44.1 kHz dump of card 1 when SU2K_SSCAPE_RAW names a file */
    static FILE *dump = nullptr;
    static bool tried = false;
    if (!tried) { tried = true; const char *p = getenv("SU2K_SSCAPE_RAW"); if (p && *p) dump = fopen(p, "wb"); }
    if (dump && ss.index == 0) fwrite(out.data(), sizeof(int16_t), out.size(), dump);
}

void mix_card0(Bitu len) { ssp = &cards[0]; mix_handler(len); }
void mix_card1(Bitu len) { ssp = &cards[1]; mix_handler(len); }

/* Card -> PC: queue a byte and interrupt. The SND library's IRQ handler (DAC.EXE 0xb13e2) reads ODIE reg 0, sees bit 1
   (host port), and takes control bytes (host status bit 2) as acknowledgements (0x80) or replies. */
void send_to_pc(uint8_t b, bool control) {
    ss.to_pc.push_back((uint16_t)(b | (control ? 0x100u : 0u)));
    PIC_ActivateIRQ(ss.irq);
}

/* Every command the library sends waits for an acknowledgement; act on it and answer once the PC has stopped writing. */
void ack_event(Bitu card) {
    ssp = &cards[card & 1];
    handle_command();
    ss.cmd.clear();
    send_to_pc(0x80, true);
}

/* A start pulse on ODIE DMA A/B: take everything the PC's DMA controller has set up on an unmasked 8-bit channel.
   DMA A carries the firmware (dropped); DMA B carries sample data for the last download command. */
void start_dma(unsigned which) {
    for (uint8_t c = 0; c < 4; c++) {
        if (ss.dma >= 0 && c != (uint8_t)ss.dma) continue;
        DmaChannel *ch = GetDMAChannel(c);
        if (!ch || ch->masked) continue;
        const Bitu n = (Bitu)ch->currcnt + 1u;
        std::vector<uint8_t> buf(n);
        const Bitu got = ch->Read(n, buf.data());
        if (which == 1 && ss.loading >= 0) {
            Sample &s = ss.samples[(unsigned)ss.loading];
            if (s.eight_bit) {
                /* 8-bit WAV data is unsigned */
                for (Bitu i = 0; i < got && s.pcm.size() < s.length; i++) s.pcm.push_back((int16_t)(((int)buf[i] - 128) << 8));
            } else {
                for (Bitu i = 0; i + 1 < got && s.pcm.size() * 2 < s.length; i += 2) s.pcm.push_back((int16_t)(buf[i] | (buf[i + 1] << 8)));
            }
            if (s.pcm.size() * (s.eight_bit ? 1u : 2u) >= s.length) ss.loading = -1;
        }
        break;
    }
    ss.dma_done[which] = true;
}

void select_card(Bitu port) {
    ssp = (ncards > 1 && port >= cards[1].base && port < cards[1].base + 8) ? &cards[1] : &cards[0];
}

Bitu ss_read(Bitu port, Bitu iolen) {
    (void)iolen;
    select_card(port);
    Bitu v = 0xFF;
    switch (port - ss.base) {
        case 1: v = 0x80; break;                                          /* MPU status: no data, can write */
        case 2:                                                           /* host port status */
            v = 0x02u;
            if (!ss.to_pc.empty()) v |= 0x01u | ((ss.to_pc.front() & 0x100u) ? 0x04u : 0u);
            break;
        case 3:
            v = 0;
            if (!ss.to_pc.empty()) { v = ss.to_pc.front() & 0xFFu; ss.to_pc.pop_front(); }
            break;
        case 4: v = ss.odie_index & 0x0Fu; break;                         /* 4-bit index (SND_GetHardwareConfig: 0xFF -> 0x0F) */
        case 5:
            v = ss.odie[ss.odie_index & 31];
            if ((ss.odie_index == 2 || ss.odie_index == 3) && ss.dma_done[ss.odie_index - 2]) v |= 0x01u;
            if (ss.odie_index == 0) v = ss.to_pc.empty() ? 0u : 0x82u;    /* interrupt status: host port has data; bit 7 keeps the IRQ handler looping */
            break;
        default: v = 0xFF; break;
    }
    SU2K_Log(SU2K_IO_READ, (uint8_t)iolen, (uint32_t)port, (uint32_t)v);
    return v;
}

void ss_write(Bitu port, Bitu val, Bitu iolen) {
    SU2K_Log(SU2K_IO_WRITE, (uint8_t)iolen, (uint32_t)port, (uint32_t)val);
    select_card(port);
    switch (port - ss.base) {
        case 0: midi_byte((uint8_t)val); break;                           /* MIDI UART */
        case 2: ss.host_ctrl = (uint8_t)val; break;
        case 3:
            if (ss.host_ctrl == 0x85) { midi_byte((uint8_t)val); break; }
            if ((val & 0x80u) && !ss.cmd.empty()) { handle_command(); ss.cmd.clear(); }
            ss.cmd.push_back((uint8_t)val);
            PIC_RemoveSpecificEvents(ack_event, ss.index);
            PIC_AddEvent(ack_event, 0.2, ss.index);
            break;
        case 4: ss.odie_index = (uint8_t)(val & 0x0Fu); break;
        case 5:
            if (ss.odie_index == 9 && (val & 0xC0u) == 0xC0u && (ss.odie[9] & 0xC0u) != 0xC0u) {
                /* host master control: 68000 released from reset after the firmware download -> it reports 0xFE */
                ss.to_pc.clear();
                ss.to_pc.push_back(0xFEu);
                if (!ss.firmware_done) LOG_MSG("SU2000: sscape firmware started at wall %.1f s, emu %.1f s", wall(), PIC_FullIndex() / 1000.0);
                ss.firmware_done = true;
            }
            if ((ss.odie_index == 2 || ss.odie_index == 3) && (val & 0x01u)) start_dma(ss.odie_index - 2u);
            ss.odie[ss.odie_index & 31] = (uint8_t)val;
            break;
        default: trace("wr", port, val); break;
    }
}

} // namespace

/* spec: "port[:irq[:dma]] ..." for up to two cards, e.g. "0x330:7" (Solo) or "0x330:12:1 0x350:7:3" (two-player
   cabinet, CONFIG.VPC sound1 / sound2); "0" = none. */
void SSCAPE_Setup(const char *spec) {
    SSCAPE_Shutdown();
    ncards = 0;
    while (spec && *spec && ncards < 2) {
        char *e;
        const unsigned long base = strtoul(spec, &e, 0);
        if (e == spec) { spec++; continue; }
        unsigned long irq = 7;
        long dma = -1;
        if (*e == ':') { irq = strtoul(e + 1, &e, 0); if (*e == ':') dma = strtol(e + 1, &e, 0); }
        spec = e;
        if (!base) continue;
        Soundscape &c = cards[ncards];
        c.base = (uint32_t)base;
        c.irq = (unsigned)irq;
        c.dma = (int)(dma >= 0 && dma < 4 ? dma : -1);
        c.index = ncards;
        c.to_pc.clear();
        c.cmd.clear();
        c.samples.clear();
        c.patch_sample.clear();
        c.program_patch.clear();
        c.loading = -1;
        c.logged = 0;
        c.firmware_done = false;
        c.dma_done[0] = c.dma_done[1] = false;
        c.odie_index = 0;
        memset(c.odie, 0, sizeof(c.odie));
        for (Voice &v : c.voices) v.on = false;
        c.rd.Install(c.base, ss_read, IO_MA, 8);
        c.wr.Install(c.base, ss_write, IO_MA, 8);
        c.mix = MIXER_AddChannel(ncards ? mix_card1 : mix_card0, MIX_RATE, ncards ? "SSCAPE2" : "SSCAPE");
        c.mix->Enable(true);
        LOG_MSG("SU2000: Soundscape %u (HLE) at %03xh IRQ %u%s, wall %.1f s", ncards + 1, c.base, c.irq,
                c.dma >= 0 ? (c.dma == 1 ? " DMA 1" : c.dma == 3 ? " DMA 3" : c.dma == 0 ? " DMA 0" : " DMA 2") : "", wall());
        ncards++;
    }
    ssp = &cards[0];
    installed = ncards > 0;
}

unsigned SSCAPE_Cards(void) { return ncards; }

void SSCAPE_Shutdown(void) {
    if (!installed) return;
    PIC_RemoveEvents(ack_event);
    for (unsigned i = 0; i < ncards; i++) {
        Soundscape &c = cards[i];
        c.rd.Uninstall();
        c.wr.Uninstall();
        if (c.mix) { MIXER_DelChannel(c.mix); c.mix = nullptr; }
        for (Voice &v : c.voices) v.on = false;
    }
    ncards = 0;
    installed = false;
}
