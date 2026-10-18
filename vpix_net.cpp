/*
 *  Virtuality SU2000 - the pod network card, emulated so two (or more) MallVR copies link up.
 *
 *  The SU2000 pods talked over a Western Digital / SMC 8003-class Ethernet card (NS 8390 core,
 *  shared-memory buffer) set up in CONFIG.VPC as [NET] IO 0x280, MEM 0xC8000, IRQ 5. Every game
 *  links the same NET library (SMC "lower MAC" code: NETI_CardGetBoardId / CardSetup /
 *  CardHandleReceive / NET_SendPacket). It polls the 8390's ISR from NET_ProcessPackets, so no
 *  interrupt is needed. We emulate the card faithfully enough that the game's own library runs
 *  unchanged (it does all the atom/handler work), and carry whole Ethernet frames over UDP.
 *
 *  The card we present takes the library's simplest detection path (traced in SFL.EXE):
 *    base+0x0E board type 0x02 (rev 1) -> NETI_CardGetRamSize; base+7 is read/write, so the
 *    library sets "interface chip present" (bid 0x10); ICR bit 3 clear -> 8 KB RAM;
 *    MSR (base+0) bits 0-5 = 0x24 -> RAM at 0xC8000; LAN address at base+8..0xD with the SMC
 *    prefix 00:00:C0 (the library checks it) and base+0xF = checksum (bytes 8..F sum to 0xFF).
 *  The 8390 sees the 8 KB RAM at every 8 KB of its 64 KB address space (TPSR is written as
 *  (ram base >> 8) & 0xFF = 0x80 -> offset 0).
 *
 *  Transport (the "packet in/out" seam): VPIX_Net_SetSender() replaces UDP with any host
 *  transport (an online relay, a matchmaking service), VPIX_Net_Receive() hands in a frame.
 *
 *  Env:
 *    VPIX_NET=<pod>          enable, pod number 1..8 (also the last byte of the card's MAC)
 *    VPIX_NET_BIND=<port>    UDP port to listen on (default 27100 + pod)
 *    VPIX_NET_PEERS=h:p,...  where to send frames (default 127.0.0.1:27101..27104, minus our own)
 *    VPIX_NET_WAIT_MS=<ms>   real-time budget per game frame for lockstep waits (default 60)
 *    VPIX_NET_LOG=1          one log line per frame (debugging)
 */

#include "dosbox.h"
#include "mem.h"
#include "paging.h"
#include "inout.h"
#include "setup.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <chrono>

#include "vpix_net.h"

static const Bitu NET_IO = 0x280;
static const PhysPt NET_RAM = 0xC8000;
static const uint32_t RAM_SIZE = 0x2000;              /* 8 KB, what the board-ID path reports */

static bool net_on = false, net_log = false;
static int pod = 1;
static uint8_t ram[RAM_SIZE];
static uint8_t board[16];                             /* base+0..0xF: WD/SMC registers + LAN address ROM */
static FILE *nlog = NULL;

/* ---- NS 8390 ------------------------------------------------------------------- */
static uint8_t cr = 0x21, isr = 0x80, imr, pstart = 0x06, pstop = 0x20, bnry = 0x06, curr = 0x07, tpsr;
static uint8_t tbcr[2], rsar[2], rbcr[2], rcr, tcr, dcr, tsr, rsr, ncr, par[6], mar[8];
static unsigned long tx_frames = 0, rx_frames = 0, rx_dropped = 0;
static unsigned empty_polls = 0;                      /* lockstep wait state, see maybe_wait_for_frame */
static double wait_used_ms = 0, wait_budget_ms = 60;
static bool running = false;                          /* only STP stops the chip; drivers write CR 0x20/0x60 while it runs */

static void nl(const char *fmt, ...) {
    if (!nlog) return;
    va_list ap; va_start(ap, fmt); vfprintf(nlog, fmt, ap); va_end(ap);
    fputc('\n', nlog); fflush(nlog);
}

/* ---- transport -------------------------------------------------------------------- */
static SOCKET sock = INVALID_SOCKET;
static std::vector<sockaddr_in> peers;
static std::mutex rxq_lock;
static std::condition_variable rxq_cv;
static std::deque<std::vector<uint8_t>> rxq;
static std::thread rx_thread;
static std::atomic<bool> rx_run(false);
static VPIX_NetSender custom_sender = NULL;

void VPIX_Net_SetSender(VPIX_NetSender s) { custom_sender = s; }

void VPIX_Net_Receive(const uint8_t *frame, size_t len) {
    if (!net_on || len < 14 || len > 1514) return;
    std::lock_guard<std::mutex> g(rxq_lock);
    if (rxq.size() >= 4096) { rxq.pop_front(); rx_dropped++; }  /* receiver gone for good: drop the oldest */
    rxq.emplace_back(frame, frame + len);
    rxq_cv.notify_one();
}

static void udp_send(const uint8_t *f, size_t len) {
    if (custom_sender) { custom_sender(f, len); return; }
    if (sock == INVALID_SOCKET) return;
    for (auto &p : peers) sendto(sock, (const char *)f, (int)len, 0, (const sockaddr *)&p, sizeof(p));
}

static void rx_loop() {
    std::vector<uint8_t> buf(2048);
    while (rx_run) {
        int n = recv(sock, (char *)buf.data(), (int)buf.size(), 0);
        if (n <= 0) { if (!rx_run) break; Sleep(1); continue; }
        VPIX_Net_Receive(buf.data(), (size_t)n);
    }
}

/* ---- receive: frame -> ring buffer in card RAM ------------------------------------ */
static uint32_t crc32_le(const uint8_t *d, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) { c ^= d[i]; for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1))); }
    return ~c;
}

static bool accept_frame(const uint8_t *f) {
    if (rcr & 0x10) return true;                                  /* promiscuous */
    if (f[0] & 1) {
        bool bc = true; for (int i = 0; i < 6; i++) if (f[i] != 0xFF) bc = false;
        if (bc) return (rcr & 0x04) != 0;
        if (!(rcr & 0x08)) return false;
        /* 8390 multicast hash: top 6 bits of the big-endian CRC of the destination */
        uint32_t c = 0xFFFFFFFFu;
        for (int i = 0; i < 6; i++) { uint8_t b = f[i]; for (int k = 0; k < 8; k++, b >>= 1) { uint32_t bit = ((c >> 31) ^ b) & 1; c <<= 1; if (bit) c ^= 0x04C11DB7u; } }
        uint32_t h = c >> 26;
        return (mar[h >> 3] >> (h & 7)) & 1;
    }
    return memcmp(f, par, 6) == 0;
}

static inline uint8_t &rambyte(uint32_t a8390) { return ram[a8390 & (RAM_SIZE - 1)]; }

/* Frames wait in the host queue until the ring has room: the games run uncapped (hundreds of
   frames a second) where the pods ran at ~30, so a burst would otherwise overflow the 6.5 KB
   ring and make the driver run its overflow-reset path. Returns false = no room yet. */
static bool ring_put(const std::vector<uint8_t> &fr) {
    if (!running || (tcr & 0x06)) { rx_dropped++; return true; }  /* stopped or in loopback: lost */
    if (pstop <= pstart || curr < pstart || curr >= pstop) { rx_dropped++; return true; }
    if (!accept_frame(fr.data())) return true;
    std::vector<uint8_t> f(fr);
    if (f.size() < 60) f.resize(60, 0);
    uint32_t c = crc32_le(f.data(), f.size());                    /* the count includes the FCS */
    for (int i = 0; i < 4; i++) f.push_back((uint8_t)(c >> (8 * i)));
    uint32_t total = (uint32_t)f.size() + 4, pages = (total + 255) / 256, ring = (uint32_t)(pstop - pstart);
    uint32_t free_pages = ((uint32_t)bnry + ring - curr) % ring;
    if (free_pages == 0) free_pages = ring;                       /* BNRY == CURR right after a reset */
    if (pages >= free_pages) return false;
    uint8_t next = (uint8_t)(curr + pages);
    if (next >= pstop) next = (uint8_t)(next - ring);
    rsr = (uint8_t)(0x01 | ((f[0] & 1) ? 0x20 : 0));
    uint32_t a = (uint32_t)curr << 8;
    uint8_t hdr[4] = {rsr, next, (uint8_t)total, (uint8_t)(total >> 8)};   /* status, next page, count incl. this header */
    auto put = [&](uint8_t v) {
        rambyte(a) = v; a++;
        if ((a >> 8) >= pstop && (a & 0xFF) == 0) a = (uint32_t)pstart << 8;
    };
    for (uint8_t v : hdr) put(v);
    for (uint8_t v : f) put(v);
    if (net_log) nl("RX %u bytes from %02X:%02X:%02X:%02X:%02X:%02X -> page %02X, next %02X (bnry %02X)",
                    (unsigned)fr.size(), fr[6], fr[7], fr[8], fr[9], fr[10], fr[11], curr, next, bnry);
    curr = next;
    isr |= 0x01;
    rx_frames++;
    return true;
}

static void drain_rx() {
    std::lock_guard<std::mutex> g(rxq_lock);
    while (!rxq.empty() && ring_put(rxq.front())) rxq.pop_front();
}

/* ---- transmit ----------------------------------------------------------------------- */
static void transmit() {
    uint32_t len = (uint32_t)tbcr[0] | ((uint32_t)tbcr[1] << 8);
    if (len > 1514) len = 1514;
    std::vector<uint8_t> f(len);
    for (uint32_t i = 0; i < len; i++) f[i] = rambyte(((uint32_t)tpsr << 8) + i);
    tsr = 0x01; isr |= 0x02;
    empty_polls = 0; wait_used_ms = 0;                           /* new game frame: fresh wait budget */
    if (tcr & 0x06) { if (net_log) nl("TX %u bytes (loopback, not sent)", len); return; }
    tx_frames++;
    if (net_log && len >= 14) nl("TX %u bytes to %02X:%02X:%02X:%02X:%02X:%02X", len, f[0], f[1], f[2], f[3], f[4], f[5]);
    udp_send(f.data(), f.size());
}

/* ---- I/O ---------------------------------------------------------------------------- */
/* Lockstep waits: games poll NET_ProcessPackets in a loop until the other pod's packet is in
   (DAC's attract sync), timed in game frames that took ~1/30 s on the pods but microseconds
   here, so the wait ran out before a UDP frame could cross. After a transmit, repeated empty
   ISR polls wait in real time for the next frame (up to wait_budget_ms per transmit, so a
   missing pod costs at most that per game frame). A single poll per frame never waits. */
static void maybe_wait_for_frame() {
    if (++empty_polls < 4 || wait_used_ms >= wait_budget_ms) return;
    auto t0 = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> g(rxq_lock);
    rxq_cv.wait_for(g, std::chrono::microseconds(1000), [] { return !rxq.empty(); });
    wait_used_ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

static Bitu net_read(Bitu port, Bitu /*iolen*/) {
    Bitu r = port - NET_IO;
    if (r < 0x10) return board[r];
    drain_rx();
    if ((r & 0xF) == 7 && (cr >> 6) == 0 && running && !(isr & 0x01)) { maybe_wait_for_frame(); drain_rx(); }
    if (isr & 0x01) empty_polls = 0;
    unsigned reg = (unsigned)(r & 0xF), page = cr >> 6;
    if (reg == 0) return (cr & ~0x03) | (running ? 0x02 : 0x01);
    if (page == 0) {
        switch (reg) {
        case 3: return bnry;
        case 4: return tsr;
        case 5: return ncr;
        case 7: return isr;
        case 0xC: return rsr;
        case 0xD: case 0xE: case 0xF: return 0;               /* tally counters */
        default: return 0;
        }
    }
    if (page == 1) {
        if (reg >= 1 && reg <= 6) return par[reg - 1];
        if (reg == 7) return curr;
        return mar[reg - 8];
    }
    if (page == 2) {
        switch (reg) {
        case 1: return pstart; case 2: return pstop; case 4: return tpsr;
        case 0xC: return rcr; case 0xD: return tcr; case 0xE: return dcr; case 0xF: return imr;
        }
    }
    return 0;
}

static void net_write(Bitu port, Bitu val, Bitu /*iolen*/) {
    Bitu r = port - NET_IO; uint8_t v = (uint8_t)val;
    if (r < 0x10) {
        if (r < 8) board[r] = (r == 0) ? (uint8_t)(v & 0x7F) : v;   /* MSR bit 7 = reset pulse */
        return;
    }
    unsigned reg = (unsigned)(r & 0xF), page = cr >> 6;
    if (reg == 0) {
        cr = (uint8_t)(v & ~0x04);
        if (v & 0x01) { running = false; isr |= 0x80; }                 /* STOP */
        else if (v & 0x02) { running = true; isr &= (uint8_t)~0x80; }   /* START */
        if ((v & 0x04) && running) transmit();                          /* TXP: sent at once */
        return;
    }
    if (page == 0) {
        switch (reg) {
        case 1: pstart = v; break;
        case 2: pstop = v; break;
        case 3: bnry = v; break;
        case 4: tpsr = v; break;
        case 5: tbcr[0] = v; break;
        case 6: tbcr[1] = v; break;
        case 7: isr &= (uint8_t)~v; break;                              /* write 1 to clear */
        case 8: rsar[0] = v; break;
        case 9: rsar[1] = v; break;
        case 0xA: rbcr[0] = v; break;
        case 0xB: rbcr[1] = v; break;
        case 0xC: rcr = v; break;
        case 0xD: tcr = v; break;
        case 0xE: dcr = v; break;
        case 0xF: imr = v; break;
        }
        return;
    }
    if (page == 1) {
        if (reg >= 1 && reg <= 6) par[reg - 1] = v;
        else if (reg == 7) curr = v;
        else mar[reg - 8] = v;
    }
}

/* ---- shared RAM at 0xC8000 -------------------------------------------------------------- */
class VPIX_NetRam : public PageHandler {
public:
    VPIX_NetRam() : PageHandler(PFLAG_NOCODE) {}
    uint8_t readb(PhysPt a) override { if (!(board[0] & 0x40)) return 0xFF; return ram[(a - NET_RAM) & (RAM_SIZE - 1)]; }
    void writeb(PhysPt a, uint8_t v) override { if (board[0] & 0x40) ram[(a - NET_RAM) & (RAM_SIZE - 1)] = v; }
    uint16_t readw(PhysPt a) override { return (uint16_t)(readb(a) | (readb(a + 1) << 8)); }
    uint32_t readd(PhysPt a) override { return (uint32_t)readw(a) | ((uint32_t)readw(a + 2) << 16); }
    void writew(PhysPt a, uint16_t v) override { writeb(a, (uint8_t)v); writeb(a + 1, (uint8_t)(v >> 8)); }
    void writed(PhysPt a, uint32_t v) override { writew(a, (uint16_t)v); writew(a + 2, (uint16_t)(v >> 16)); }
};
static VPIX_NetRam *ram_handler = NULL;

void VPIX_NET_RemapMemory(void) {
    if (ram_handler) MEM_RegisterHandler(NET_RAM >> 12, ram_handler, RAM_SIZE >> 12);
}

int VPIX_NET_Pod(void) { return pod; }
bool VPIX_NET_Enabled(void) { return net_on; }

void VPIX_NET_Shutdown(void) {
    if (!net_on) return;
    nl("== shutdown: %lu frames sent, %lu received, %lu dropped", tx_frames, rx_frames, rx_dropped);
    rx_run = false;
    if (sock != INVALID_SOCKET) { closesocket(sock); sock = INVALID_SOCKET; }
    if (rx_thread.joinable()) rx_thread.join();
    if (nlog) { fclose(nlog); nlog = NULL; }
}

void VPIX_NET_Init(const char *logbase) {
    const char *e = getenv("VPIX_NET");
    if (!e || !*e || atoi(e) <= 0) return;
    pod = atoi(e); if (pod > 254) pod = 254;
    net_on = true;
    net_log = getenv("VPIX_NET_LOG") != NULL;
    if (logbase && *logbase) nlog = fopen((std::string(logbase) + ".net").c_str(), "w");

    /* the card: MSR = RAM at 0xC8000, enabled; LAN address 00:00:C0:56:50:<pod> ("VP") */
    memset(board, 0, sizeof(board));
    board[0] = 0x40 | ((NET_RAM >> 13) & 0x3F);
    board[4] = 0x40;                                          /* IRR: IRQ select bits (unused, polled) */
    const uint8_t mac[6] = {0x00, 0x00, 0xC0, 0x56, 0x50, (uint8_t)pod};
    memcpy(board + 8, mac, 6);
    board[0xE] = 0x02;                                        /* board type: rev 1, no 16-bit features */
    uint8_t sum = 0; for (int i = 8; i < 0xF; i++) sum = (uint8_t)(sum + board[i]);
    board[0xF] = (uint8_t)(0xFF - sum);
    memset(ram, 0, sizeof(ram));

    IO_RegisterReadHandler(NET_IO, net_read, IO_MB, 0x20);
    IO_RegisterWriteHandler(NET_IO, net_write, IO_MB, 0x20);
    ram_handler = new VPIX_NetRam();
    VPIX_NET_RemapMemory();

    /* UDP: one socket, receive on our port, send to every peer */
    WSADATA wd; WSAStartup(MAKEWORD(2, 2), &wd);
    int bind_port = 27100 + pod;
    if ((e = getenv("VPIX_NET_BIND")) != NULL && atoi(e) > 0) bind_port = atoi(e);
    if ((e = getenv("VPIX_NET_WAIT_MS")) != NULL && atof(e) >= 0) wait_budget_ms = atof(e);
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in me = {}; me.sin_family = AF_INET; me.sin_port = htons((u_short)bind_port); me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (sock == INVALID_SOCKET || bind(sock, (sockaddr *)&me, sizeof(me)) != 0) {
        nl("NET: cannot bind UDP port %d (error %d) - frames will be sent but none received", bind_port, WSAGetLastError());
    } else {
        DWORD to = 200; setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&to, sizeof(to));
        rx_run = true;
        rx_thread = std::thread(rx_loop);
    }
    std::string list;
    if ((e = getenv("VPIX_NET_PEERS")) != NULL && *e) list = e;
    else for (int p = 1; p <= 4; p++) if (p != pod) { if (!list.empty()) list += ","; list += "127.0.0.1:" + std::to_string(27100 + p); }
    size_t s = 0;
    while (s < list.size()) {
        size_t c = list.find(',', s); if (c == std::string::npos) c = list.size();
        std::string hp = list.substr(s, c - s); s = c + 1;
        size_t k = hp.rfind(':'); if (k == std::string::npos) continue;
        addrinfo hints = {}, *res = NULL; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
        if (getaddrinfo(hp.substr(0, k).c_str(), hp.substr(k + 1).c_str(), &hints, &res) == 0 && res) {
            peers.push_back(*(sockaddr_in *)res->ai_addr); freeaddrinfo(res);
        }
    }
    nl("NET: pod %d, MAC 00:00:C0:56:50:%02X, card I/O %03X RAM %05X (%u KB), UDP port %d, %u peer(s): %s",
       pod, pod, (unsigned)NET_IO, (unsigned)NET_RAM, RAM_SIZE >> 10, bind_port, (unsigned)peers.size(), list.c_str());
}
