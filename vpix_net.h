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
#endif
