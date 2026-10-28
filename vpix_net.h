/* MallVR pod network: emulated SMC/WD 8003 card (8390) at I/O 0x280, RAM 0xC8000.
   Whole Ethernet frames in and out; UDP between MallVR copies by default. */
#ifndef VPIX_NET_H
#define VPIX_NET_H
#include <stddef.h>
#include <stdint.h>

void VPIX_NET_Init(const char *logbase);      /* env VPIX_NET=<pod> turns it on */
void VPIX_NET_Shutdown(void);
void VPIX_NET_RemapMemory(void);
bool VPIX_NET_Enabled(void);
int VPIX_NET_Pod(void);                       /* 1 = master pod, 2.. = slaves */

/* Transport seam for a host (online relay, matchmaking, a frontend like TeknoParrot):
   a sender replaces the built-in UDP; frames from the network come in through Receive
   (any thread). Frames are raw Ethernet II, 14-byte header, 60..1514 bytes, no FCS. */
typedef void (*VPIX_NetSender)(const uint8_t *frame, size_t len);
void VPIX_Net_SetSender(VPIX_NetSender sender);
void VPIX_Net_Receive(const uint8_t *frame, size_t len);

/* Native games (VPIX_NATIVE, no DOS NET library on the card): send a whole Ethernet frame to the other pods, and take the
   next frame that came in (returns its length, 0 = none waiting). Same transport as the card (UDP or the host's sender). */
void VPIX_Net_SendFrame(const uint8_t *frame, size_t len);
size_t VPIX_Net_PopFrame(uint8_t *buf, size_t cap);

/* Plain UDP side channel (remote seat, vpix_seat.h): non-blocking socket bound to port (0 = any), send to host:port,
   receive one datagram (returns its length, -1 = none waiting; from = sender "a.b.c.d", port). Handle < 0 = failed. */
int VPIX_Udp_Open(int port);
bool VPIX_Udp_SendTo(int h, const char *host, int port, const void *data, size_t len);
int VPIX_Udp_Recv(int h, void *buf, size_t cap, char *from, size_t from_cap, int *from_port);

/* Voice chat (vpix_voice.h): VPIX_VOICE=1 opens the default recording device (16 kHz mono) and cuts it into 20 ms packets of
   320 mu-law bytes, gated by loudness. PopMic hands out the next packet (1 = got one). On linked pods voice rides the pod network
   as its own Ethernet type (0x88B6, never reaches the game's card); the sink gets other pods' packets (src = their pod number). */
void VPIX_Voice_Start(void);
/* the mic's mute state, shared by the voice code (vpix_voice.h: the stick click) and the in-game menu (vpix_menu.h: MIC from the
   launcher's VOICE CHAT page) */
void VPIX_Voice_SetMuted(int muted);
int VPIX_Voice_Muted(void);
int VPIX_Voice_PopMic(uint8_t *ulaw320);
void VPIX_Voice_SendNet(const uint8_t *ulaw, size_t n);
typedef void (*VPIX_VoiceSink)(int src, const uint8_t *ulaw, size_t n);
void VPIX_Voice_SetSink(VPIX_VoiceSink sink);
#endif
