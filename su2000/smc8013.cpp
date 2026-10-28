/*
 *  SMC / Western Digital 8013 Ethernet card (Virtuality pod linking) and a UDP link between emulators.
 *
 *  The games' NET library drives the card directly (see su2000/docs/findings/network.md, DAC.EXE NETI_*):
 *    base+0x00..0x0F  SMC ASIC: 0 MSR (bit 7 reset, bit 6 memory enable, bits 5..0 address 18..13), 1 ICR (bit 0 =
 *                     16-bit card, read only), 2..7 general registers (7 = GP2, read/write: the 83C583 test writes
 *                     0x35 / 0x3A), 8..13 station address, 14 board id, 15 checksum (bytes 8..15 sum to 0xFF)
 *    base+0x10..0x1F  National 8390 in shared-memory mode: frames are sent from and received into the card RAM
 *                     (16 KB at [su2000] network card memory), pages of 256 bytes; receive ring PSTART..PSTOP with
 *                     a 4-byte header per frame (status, next page, byte count including the 4 CRC bytes)
 *  Frames are raw Ethernet (multicast / broadcast destination, type 0x0000).
 *
 *  Link: [su2000] network = relay:<udp port>   this emulator also relays every frame to all the others
 *                           <host>:<udp port>  connect to the emulator running the relay
 *  Datagrams: "SU2N" + Ethernet frame, "SU2K" keep-alive (so the relay learns the client's address), "SU2V" + a
 *  headset microphone packet (the pods' analogue MICNET line).
 */

#include "dosbox.h"
#include "inout.h"
#include "logging.h"
#include "mem.h"
#include "paging.h"
#include "pic.h"
#include "su2000.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
#define SOCK_BAD INVALID_SOCKET
#define sock_close closesocket
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
typedef int sock_t;
#define SOCK_BAD (-1)
#define sock_close close
#endif

namespace {

/* ---- card state ---- */
const uint32_t RAM_SIZE = 0x4000;   /* 16 KB */

struct Smc {
    uint32_t io = 0x280, mem = 0xC8000;
    unsigned irq = 5;
    uint8_t asic[8] = { 0 };
    uint8_t prom[8] = { 0 };          /* station address, board id, checksum */
    uint8_t ram[RAM_SIZE];
    /* 8390 */
    uint8_t cr = 0x21, isr = 0x80, imr = 0, rcr = 0, tcr = 0, dcr = 0, tsr = 0, rsr = 0;
    uint8_t pstart = 0, pstop = 0, bnry = 0, tpsr = 0, curr = 0;
    uint16_t tbcr = 0, rsar = 0, rbcr = 0;
    uint8_t par[6] = { 0 }, mar[8] = { 0 };
    uint8_t cntr[3] = { 0 };
    bool irq_raised = false;
    IO_ReadHandleObject rd;
    IO_WriteHandleObject wr;
    unsigned long sent = 0, received = 0, dropped = 0;
    unsigned long drop_stopped = 0, drop_filter = 0, drop_full = 0;
};
Smc card;
bool installed = false;

/* ---- UDP link ---- */
std::mutex link_mtx;
std::deque<std::vector<uint8_t>> rx_queue;          /* frames from the network, delivered on the emulation thread */
std::deque<std::vector<uint8_t>> rx_wait;           /* frames waiting for room in the receive ring (emulation thread) */
sock_t sock = SOCK_BAD;
bool relay = false;
sockaddr_in server;                                  /* client mode */
struct Peer { sockaddr_in addr; std::chrono::steady_clock::time_point seen; };
std::vector<Peer> peers;                             /* relay mode: clients that have sent something */
std::thread link_thread;
std::atomic<bool> link_running(false);

bool same_addr(const sockaddr_in &a, const sockaddr_in &b) {
    return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

void send_datagram(const std::vector<uint8_t> &d, const sockaddr_in *except) {
    if (sock == SOCK_BAD) return;
    if (!relay) {
        sendto(sock, (const char *)d.data(), (int)d.size(), 0, (const sockaddr *)&server, sizeof(server));
        return;
    }
    std::lock_guard<std::mutex> lk(link_mtx);
    for (const Peer &p : peers)
        if (!except || !same_addr(p.addr, *except))
            sendto(sock, (const char *)d.data(), (int)d.size(), 0, (const sockaddr *)&p.addr, sizeof(p.addr));
}

void link_loop(void) {
    auto last_keepalive = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    std::vector<uint8_t> buf(2048);
    while (link_running) {
        const auto now = std::chrono::steady_clock::now();
        if (!relay && now - last_keepalive > std::chrono::seconds(1)) {
            static const char ka[4] = { 'S', 'U', '2', 'K' };
            sendto(sock, ka, 4, 0, (const sockaddr *)&server, sizeof(server));
            last_keepalive = now;
        }
        fd_set rs;
        FD_ZERO(&rs);
        FD_SET(sock, &rs);
        timeval tv = { 0, 50000 };
        if (select((int)sock + 1, &rs, NULL, NULL, &tv) <= 0) continue;
        sockaddr_in from;
        socklen_t fl = sizeof(from);
        const int n = recvfrom(sock, (char *)buf.data(), (int)buf.size(), 0, (sockaddr *)&from, &fl);
        if (n < 4 || buf[0] != 'S' || buf[1] != 'U' || buf[2] != '2') continue;
        if (relay) {
            std::lock_guard<std::mutex> lk(link_mtx);
            bool known = false;
            for (Peer &p : peers) if (same_addr(p.addr, from)) { p.seen = now; known = true; }
            if (!known) {
                peers.push_back({ from, now });
                char ip[32];
                const uint8_t *a = (const uint8_t *)&from.sin_addr;
                snprintf(ip, sizeof(ip), "%u.%u.%u.%u", a[0], a[1], a[2], a[3]);
                LOG_MSG("SU2000: network relay: pod at %s:%u joined", ip, (unsigned)ntohs(from.sin_port));
            }
            /* forget clients silent for 10 s */
            for (size_t i = 0; i < peers.size();)
                if (now - peers[i].seen > std::chrono::seconds(10)) peers.erase(peers.begin() + (ptrdiff_t)i); else i++;
        }
        if (buf[3] == 'V' && n > 4) {                        /* microphone audio from another pod (MICNET line) */
            if (relay) send_datagram(std::vector<uint8_t>(buf.begin(), buf.begin() + n), &from);
            XR_RemoteVoice((const char *)buf.data() + 4, (size_t)n - 4);
            continue;
        }
        if (buf[3] != 'N' || n < 4 + 14) continue;
        std::vector<uint8_t> frame(buf.begin() + 4, buf.begin() + n);
        if (relay) send_datagram(std::vector<uint8_t>(buf.begin(), buf.begin() + n), &from);
        std::lock_guard<std::mutex> lk(link_mtx);
        rx_queue.push_back(std::move(frame));
        while (rx_queue.size() > 4096) { rx_queue.pop_front(); card.dropped++; }
    }
}

bool link_open(const std::string &spec) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == SOCK_BAD) return false;
    sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    if (spec.compare(0, 6, "relay:") == 0) {
        relay = true;
        a.sin_port = htons((uint16_t)atoi(spec.c_str() + 6));
        if (bind(sock, (sockaddr *)&a, sizeof(a)) != 0) {
            LOG_MSG("SU2000: network relay cannot use UDP port %s", spec.c_str() + 6);
            sock_close(sock); sock = SOCK_BAD;
            return false;
        }
        LOG_MSG("SU2000: network relay on UDP port %s", spec.c_str() + 6);
    } else {
        relay = false;
        const size_t c = spec.rfind(':');
        if (c == std::string::npos) return false;
        const std::string host = spec.substr(0, c);
        addrinfo hints, *res = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if (getaddrinfo(host.c_str(), spec.c_str() + c + 1, &hints, &res) != 0 || !res) {
            LOG_MSG("SU2000: network: cannot resolve %s", host.c_str());
            sock_close(sock); sock = SOCK_BAD;
            return false;
        }
        memcpy(&server, res->ai_addr, sizeof(server));
        freeaddrinfo(res);
        bind(sock, (sockaddr *)&a, sizeof(a));
        LOG_MSG("SU2000: network: pod linked through the relay at %s", spec.c_str());
    }
    link_running = true;
    link_thread = std::thread(link_loop);
    return true;
}

void link_close(void) {
    if (link_running) {
        link_running = false;
        if (link_thread.joinable()) link_thread.join();
    }
    if (sock != SOCK_BAD) { sock_close(sock); sock = SOCK_BAD; }
    std::lock_guard<std::mutex> lk(link_mtx);
    peers.clear();
    rx_queue.clear();
    rx_wait.clear();
}

/* ---- 8390 ---- */
void update_irq(void) {
    const bool on = (card.isr & card.imr & 0x7F) != 0;
    if (on && !card.irq_raised) PIC_ActivateIRQ(card.irq);
    else if (!on && card.irq_raised) PIC_DeActivateIRQ(card.irq);
    card.irq_raised = on;
}

void reset_8390(void) {
    card.cr = 0x21;
    card.isr = 0x80;
    card.imr = 0;
    update_irq();
}

bool started(void) { return (card.cr & 0x03) == 0x02; }

void transmit(void) {
    const uint32_t start = (uint32_t)card.tpsr << 8;
    uint32_t len = card.tbcr;
    if (start >= RAM_SIZE) len = 0;
    if (start + len > RAM_SIZE) len = RAM_SIZE - start;
    if ((card.tcr & 0x06) == 0 && len >= 14 && sock != SOCK_BAD) {    /* not in loopback */
        std::vector<uint8_t> d(4 + len);
        memcpy(d.data(), "SU2N", 4);
        memcpy(d.data() + 4, &card.ram[start], len);
        send_datagram(d, NULL);
        if (card.sent++ < 5) LOG_MSG("SU2000: SMC 8013 sent %u bytes to %02x:%02x:%02x:%02x:%02x:%02x", len, d[4], d[5], d[6], d[7], d[8], d[9]);
    }
    card.tsr = 0x01;                 /* PTX */
    card.isr |= 0x02;
    card.cr &= (uint8_t)~0x04;
    update_irq();
}

bool accept(const uint8_t *f) {
    if (card.rcr & 0x10) return true;                               /* promiscuous */
    if (f[0] & 1) {
        const bool bcast = memcmp(f, "\xff\xff\xff\xff\xff\xff", 6) == 0;
        if (bcast) return (card.rcr & 0x04) != 0;
        return (card.rcr & 0x08) != 0;                              /* multicast: every group (the library filters) */
    }
    return memcmp(f, card.par, 6) == 0;
}

/* Frames that did not fit in the receive ring, kept in order until the program frees pages. Two emulated pods do not
   run in step like real ones: a faster pod overran the slower one's ring (27% dropped), and DAC lost the reply that
   starts a linked game ("waiting for opponent"). */

/* false: the ring is full now (frame not taken; try again later) */
bool receive(const std::vector<uint8_t> &frame) {
    if (!started() || card.pstop <= card.pstart || card.pstop > RAM_SIZE / 256) { card.drop_stopped++; return true; }
    if (frame.size() < 14 || !accept(frame.data())) { card.drop_filter++; return true; }
    size_t len = frame.size() < 60 ? 60 : frame.size();
    if (len > 1514) len = 1514;
    const unsigned count = (unsigned)len + 4;                       /* with the CRC */
    const unsigned pages = (count + 4 + 255) / 256;
    const unsigned ring = (unsigned)card.pstop - card.pstart;
    /* free pages between CURR and BNRY */
    unsigned avail = (card.bnry > card.curr) ? (unsigned)(card.bnry - card.curr) : ring - (unsigned)(card.curr - card.bnry);
    if (card.bnry == card.curr) avail = ring;
    if (pages >= avail) return false;
    unsigned next = card.curr + pages;
    if (next >= card.pstop) next -= ring;
    std::vector<uint8_t> data(count + 4, 0);
    data[0] = (uint8_t)(0x01 | ((frame[0] & 1) ? 0x20 : 0));        /* PRX, PHY = multicast / broadcast */
    data[1] = (uint8_t)next;
    data[2] = (uint8_t)count; data[3] = (uint8_t)(count >> 8);
    memcpy(&data[4], frame.data(), frame.size() < len ? frame.size() : len);
    uint32_t a = (uint32_t)card.curr << 8;
    for (size_t i = 0; i < data.size(); i++) {
        card.ram[a] = data[i];
        if (++a >= (uint32_t)card.pstop << 8) a = (uint32_t)card.pstart << 8;
    }
    card.curr = (uint8_t)next;
    card.rsr = data[0];
    card.isr |= 0x01;
    if (card.received++ < 5) LOG_MSG("SU2000: SMC 8013 received %u bytes from %02x:%02x:%02x:%02x:%02x:%02x", (unsigned)frame.size(), frame[6], frame[7], frame[8], frame[9], frame[10], frame[11]);
    update_irq();
    return true;
}

void deliver_event(Bitu) {
    std::deque<std::vector<uint8_t>> q;
    {
        std::lock_guard<std::mutex> lk(link_mtx);
        q.swap(rx_queue);
    }
    for (auto &f : q) rx_wait.push_back(std::move(f));
    while (!rx_wait.empty() && receive(rx_wait.front())) rx_wait.pop_front();
    while (rx_wait.size() > 4096) { rx_wait.pop_front(); card.dropped++; card.drop_full++; }   /* the program stopped reading */
    static double last_stats = 0;
    const double now = PIC_FullIndex();
    if (now - last_stats >= 10000.0) {
        last_stats = now;
        LOG_MSG("SU2000: SMC 8013: %lu frames sent, %lu received, %lu dropped (ring full %lu, card stopped %lu, filtered %lu); cr %02x isr %02x imr %02x rcr %02x bnry %02x curr %02x ring %02x..%02x",
                card.sent, card.received, card.dropped, card.drop_full, card.drop_stopped, card.drop_filter, card.cr, card.isr, card.imr,
                card.rcr, card.bnry, card.curr, card.pstart, card.pstop);
    }
    if (installed) PIC_AddEvent(deliver_event, 1.0);
}

Bitu reg_read(unsigned r) {
    if (r < 0x10) {
        if (r >= 8) return card.prom[r - 8];
        if (r == 1) return card.asic[1] | 0x01;                     /* 16-bit card, read only */
        return card.asic[r];
    }
    r -= 0x10;
    if (r == 0) return card.cr;
    const unsigned page = card.cr >> 6;
    if (page == 0) {
        switch (r) {
            case 0x03: return card.bnry;
            case 0x04: return card.tsr;
            case 0x07: return card.isr;
            case 0x0C: return card.rsr;
            case 0x0D: return card.cntr[0];
            case 0x0E: return card.cntr[1];
            case 0x0F: return card.cntr[2];
            default: return 0;
        }
    }
    if (page == 1) {
        if (r >= 1 && r <= 6) return card.par[r - 1];
        if (r == 7) return card.curr;
        if (r >= 8) return card.mar[r - 8];
        return 0;
    }
    switch (r) {
        case 0x01: return card.pstart;
        case 0x02: return card.pstop;
        case 0x04: return card.tpsr;
        case 0x0C: return card.rcr;
        case 0x0D: return card.tcr;
        case 0x0E: return card.dcr;
        case 0x0F: return card.imr;
        default: return 0;
    }
}

void reg_write(unsigned r, uint8_t v) {
    if (r < 0x10) {
        if (r == 0 && (v & 0x80)) reset_8390();
        if (r < 8) card.asic[r] = v;
        return;
    }
    r -= 0x10;
    if (r == 0) {
        if ((v & 0x03) == 0x02 && !started()) LOG_MSG("SU2000: SMC 8013 started: ring %02x..%02x, receive config %02x, transmit config %02x", card.pstart, card.pstop, card.rcr, card.tcr);
        /* STP / STA are commands: a write with neither (a page switch, 0x20 / 0x60) keeps the card running or stopped */
        const uint8_t run = card.cr & 0x03;
        card.cr = (uint8_t)((v & ~0x07) | run);
        if (v & 0x01) { card.isr |= 0x80; card.cr = (uint8_t)((card.cr & ~0x03) | 0x01); }
        else if (v & 0x02) { card.isr &= 0x7F; card.cr = (uint8_t)((card.cr & ~0x03) | 0x02); }
        if ((v & 0x04) && started()) transmit();
        update_irq();
        return;
    }
    const unsigned page = card.cr >> 6;
    if (page == 0) {
        switch (r) {
            case 0x01: card.pstart = v; break;
            case 0x02: card.pstop = v; break;
            case 0x03: card.bnry = v; break;
            case 0x04: card.tpsr = v; break;
            case 0x05: card.tbcr = (uint16_t)((card.tbcr & 0xFF00) | v); break;
            case 0x06: card.tbcr = (uint16_t)((card.tbcr & 0x00FF) | (v << 8)); break;
            case 0x07: card.isr &= (uint8_t)~(v & 0x7F); update_irq(); break;   /* write 1 to clear */
            case 0x08: card.rsar = (uint16_t)((card.rsar & 0xFF00) | v); break;
            case 0x09: card.rsar = (uint16_t)((card.rsar & 0x00FF) | (v << 8)); break;
            case 0x0A: card.rbcr = (uint16_t)((card.rbcr & 0xFF00) | v); break;
            case 0x0B: card.rbcr = (uint16_t)((card.rbcr & 0x00FF) | (v << 8)); break;
            case 0x0C: card.rcr = v; break;
            case 0x0D: card.tcr = v; break;
            case 0x0E: card.dcr = v; break;
            case 0x0F: card.imr = v; update_irq(); break;
        }
        return;
    }
    if (page == 1) {
        if (r >= 1 && r <= 6) card.par[r - 1] = v;
        else if (r == 7) card.curr = v;
        else if (r >= 8) card.mar[r - 8] = v;
    }
}

Bitu io_read(Bitu port, Bitu iolen) {
    (void)iolen;
    const Bitu v = reg_read((unsigned)(port - card.io) & 0x1F) & 0xFF;
    SU2K_Log(SU2K_IO_READ, 1, (uint32_t)port, (uint32_t)v);
    return v;
}

void io_write(Bitu port, Bitu val, Bitu iolen) {
    (void)iolen;
    SU2K_Log(SU2K_IO_WRITE, 1, (uint32_t)port, (uint32_t)val);
    reg_write((unsigned)(port - card.io) & 0x1F, (uint8_t)val);
}

class RamWindow : public PageHandler {
public:
    RamWindow() : PageHandler(PFLAG_NOCODE) {}
    uint8_t readb(PhysPt a) override { const uint32_t o = (uint32_t)a - card.mem; return o < RAM_SIZE ? card.ram[o] : 0xFF; }
    void writeb(PhysPt a, uint8_t v) override { const uint32_t o = (uint32_t)a - card.mem; if (o < RAM_SIZE) card.ram[o] = v; }
    uint16_t readw(PhysPt a) override { return (uint16_t)(readb(a) | (readb(a + 1) << 8)); }
    uint32_t readd(PhysPt a) override { return (uint32_t)readw(a) | ((uint32_t)readw(a + 2) << 16); }
    void writew(PhysPt a, uint16_t v) override { writeb(a, (uint8_t)v); writeb(a + 1, (uint8_t)(v >> 8)); }
    void writed(PhysPt a, uint32_t v) override { writew(a, (uint16_t)v); writew(a + 2, (uint16_t)(v >> 16)); }
};
RamWindow window;

} // namespace

/* card: "io:irq:memory" (CONFIG.VPC [NET]); link: "relay:<port>" or "<host>:<port>"; mac: "" = random */
void SMC_Setup(const char *card_spec, const char *link, const char *mac) {
    SMC_Shutdown();
    if (link && !strcmp(link, "none")) return;
    /* "off": the card is fitted but not linked (a Solo pod has one; DAC's adapter set-up crashes on an empty slot) */
    const bool linked = link && *link && strcmp(link, "off") && strcmp(link, "false");
    unsigned long io = 0x280, irq = 5, memaddr = 0xC8000;
    if (card_spec && *card_spec) {
        char *e;
        io = strtoul(card_spec, &e, 0);
        if (*e == ':') irq = strtoul(e + 1, &e, 0);
        if (*e == ':') memaddr = strtoul(e + 1, &e, 0);
    }
    card.io = (uint32_t)io; card.irq = (unsigned)irq; card.mem = (uint32_t)memaddr;
    memset(card.ram, 0, sizeof(card.ram));
    memset(card.asic, 0, sizeof(card.asic));
    /* station address: SMC prefix 00:00:C0, the rest from the setting or random */
    unsigned m[6] = { 0x00, 0x00, 0xC0, 0, 0, 0 };
    if (!mac || sscanf(mac, "%x:%x:%x:%x:%x:%x", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) != 6) {
        /* pods started in the same second must still differ: DAC ignores packets from its own address */
        std::random_device rd;
        const unsigned r = rd() ^ (unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int i = 3; i < 6; i++) m[i] = (r >> (8 * (i - 3))) & 0xFFu;
    }
    for (int i = 0; i < 6; i++) card.prom[i] = (uint8_t)m[i];
    card.prom[6] = 0x04;               /* board id: revision 2 (bits 4..1), no interface-chip extras */
    uint8_t sum = 0;
    for (int i = 0; i < 7; i++) sum = (uint8_t)(sum + card.prom[i]);
    card.prom[7] = (uint8_t)(0xFF - sum);
    card.sent = card.received = card.dropped = 0;
    reset_8390();
    card.rd.Install(card.io, io_read, IO_MA, 0x20);
    card.wr.Install(card.io, io_write, IO_MA, 0x20);
    MEM_SetPageHandler(card.mem >> 12, RAM_SIZE >> 12, &window);
    PAGING_ClearTLB();
    installed = true;
    LOG_MSG("SU2000: SMC 8013 at %03lxh IRQ %lu RAM %05lxh, station %02x:%02x:%02x:%02x:%02x:%02x", io, irq, memaddr,
            m[0], m[1], m[2], m[3], m[4], m[5]);
    if (linked) link_open(link);
    PIC_AddEvent(deliver_event, 1.0);
}

void SMC_Shutdown(void) {
    if (!installed) return;
    installed = false;
    PIC_RemoveEvents(deliver_event);
    link_close();
    card.rd.Uninstall();
    card.wr.Uninstall();
    MEM_ResetPageHandler_Unmapped(card.mem >> 12, RAM_SIZE >> 12);
    PAGING_ClearTLB();
    if (card.irq_raised) { PIC_DeActivateIRQ(card.irq); card.irq_raised = false; }
    LOG_MSG("SU2000: SMC 8013: %lu frames sent, %lu received, %lu dropped", card.sent, card.received, card.dropped);
}

bool SMC_Installed(void) { return installed; }

/* The pods' MICNET line: microphone packets (xrserver "SUM1" format) to all other pods. */
void SMC_SendVoice(const char *data, size_t n) {
    if (!installed || sock == SOCK_BAD || n > 1400) return;
    std::vector<uint8_t> d(4 + n);
    memcpy(d.data(), "SU2V", 4);
    memcpy(d.data() + 4, data, n);
    send_datagram(d, NULL);
}
