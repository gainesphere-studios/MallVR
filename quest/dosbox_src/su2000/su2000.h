/*
 *  Virtuality SU2000 hardware emulation (research build).
 *
 *  Shared declarations for the SU2000 device stubs: the binary bus-trace logger and the
 *  per-device init entry points. See su2000/docs/PROTOCOL.md for the hardware being modelled.
 */
#ifndef DOSBOX_SU2000_H
#define DOSBOX_SU2000_H

#include <stdint.h>
#include <string>

/* Record kinds for the binary trace (.su2k files). Keep in sync with su2000/tools/su2klog.py. */
enum SU2K_Kind : uint8_t {
    SU2K_IO_READ   = 1,
    SU2K_IO_WRITE  = 2,
    SU2K_MEM_READ  = 3,  /* addr = ISA physical address, aux = board address (PIX window) */
    SU2K_MEM_WRITE = 4,
    SU2K_FIFO      = 5,  /* PIX broadcast FIFO word */
    SU2K_EVENT     = 6,  /* emulator-side event, value = SU2K_Event */
};

enum SU2K_Event : uint32_t {
    SU2K_EV_RUN_A      = 1,  /* aux = card index */
    SU2K_EV_RUN_B      = 2,
    SU2K_EV_RESET      = 3,
    SU2K_EV_FAKE_WRITE = 4,  /* addr = board address, aux = card, value written by the stub */
};

void SU2K_Log(SU2K_Kind kind, uint8_t width, uint32_t addr, uint32_t value, uint32_t aux = 0);
bool SU2K_LogEnabled(void);

/* Device init/teardown, called from su2000.cpp */
void PIX1000_Setup(uint32_t fifo_port, const uint32_t *proc_ports, unsigned int nproc,
                   uint32_t video_port, uint32_t procmem, bool fake_boot, unsigned int fake_cpu_rev);
void PIX1000_Shutdown(void);
struct PixFrame;
bool PIX1000_GetFrame(unsigned i, PixFrame &out, uint32_t &seq);
bool PIX1000_GetFrameSeq(unsigned i, uint32_t seq, PixFrame &out);
unsigned PIX1000_NumCards(void);
bool PIX1000_Stereo(void);
void PIX1000_SetStereo(bool on, float separation);
void PIX1000_SetMode(bool emulation, bool hle_b);
void PIX1000_Tick(void);
void TRACKER_Setup(const uint32_t *ports, unsigned int n);
void TRACKER_Shutdown(void);
void TRACKER_SetPose(const char *s);
void TRACKER_SetCalibrate(bool on);
void TRACKER_SetHandTarget(const char *s);
void TRACKER_MouseDelta(double dx, double dy, bool head);
void FCARD_Setup(const uint32_t *io, const uint32_t *mem, unsigned n);
void FCARD_Shutdown(void);
void SSCAPE_Setup(const char *spec);
unsigned SSCAPE_Cards(void);
void XR_CardAudio(unsigned card, unsigned rate, unsigned frames, const int16_t *lr);
void SSCAPE_Shutdown(void);
void SMC_Setup(const char *card, const char *link, const char *mac);
void SMC_Shutdown(void);
bool SMC_Installed(void);
void SMC_SendVoice(const char *data, size_t n);
void XR_RemoteVoice(const char *data, size_t n);
void FCARD_AddMapperKeys(void);
void FCARD_GetStick(int &x, int &y);
void FCARD_SetButton(unsigned bit, bool pressed);
uint16_t FCARD_GetButtons(void);
void FCARD_SetMicLevel(unsigned card, uint8_t level);
bool FCARD_GetMicnet(unsigned card, float &left, float &right);
void TRACKER_SetMouse(bool on);
void TRACKER_XRPose(int player, const double *head, const double *hand, bool recenter);
void FCARD_SetPlayerButton(unsigned card, unsigned bit, bool pressed);
void TRACKER_DebugTick(void);
void XR_Configure(const char *format, int jpeg_quality, bool audio, bool webrtc, const char *certificate);
void XR_ConfigureVideo(const char *mode, const char *library, int kbps);
void XR_Setup(int port);
void XR_Shutdown(void);
bool XR_Active(void);
void XR_PushFrame(const PixFrame *ch, unsigned n, bool stereo);
void XR_Poll(void);

#endif
