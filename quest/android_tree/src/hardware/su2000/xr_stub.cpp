/*
 *  MallVR: no-op stand-ins for the WebXR headset server (xrserver.cpp / h264enc.cpp) of LeonG-ZA's SU2000 card emulation
 *  (github.com/LeonG-ZA/dosbox-x, branch su2000, GPL). MallVR only uses his PIX 1000 card as the reference picture
 *  (VPIX_REF); the headset streaming and its libdatachannel / OpenH264 dependencies are left out.
 */
#include <stddef.h>
#include <stdint.h>
#include "su2000.h"

void XR_CardAudio(unsigned, unsigned, unsigned, const int16_t *) {}
void XR_RemoteVoice(const char *, size_t) {}
void XR_Configure(const char *, int, bool, bool, const char *) {}
void XR_ConfigureVideo(const char *, const char *, int) {}
void XR_Setup(int) {}
void XR_Shutdown(void) {}
bool XR_Active(void) { return false; }
void XR_PushFrame(const PixFrame *, unsigned, bool) {}
void XR_Poll(void) {}
