/* MallVR Quest: winmm waveIn names so vpix_net.cpp builds; waveInOpen fails = "no microphone" (voice chat mic: AAudio, later) */
#pragma once
#include "windows.h"
typedef void *HWAVEIN; typedef UINT MMRESULT;
typedef struct { WORD wFormatTag, nChannels; DWORD nSamplesPerSec, nAvgBytesPerSec; WORD nBlockAlign, wBitsPerSample, cbSize; } WAVEFORMATEX;
typedef struct { LPSTR lpData; DWORD dwBufferLength, dwBytesRecorded; DWORD_PTR dwUser; DWORD dwFlags, dwLoops; void *lpNext; DWORD_PTR reserved; } WAVEHDR;
#define WAVE_FORMAT_PCM 1
#define WAVE_MAPPER ((UINT)-1)
#define CALLBACK_EVENT 0x00050000
#define MMSYSERR_NOERROR 0
#define WHDR_DONE 1
static inline MMRESULT waveInOpen(HWAVEIN *, UINT, const WAVEFORMATEX *, DWORD_PTR, DWORD_PTR, DWORD) { return 1; }
static inline MMRESULT waveInPrepareHeader(HWAVEIN, WAVEHDR *, UINT) { return 1; }
static inline MMRESULT waveInUnprepareHeader(HWAVEIN, WAVEHDR *, UINT) { return 1; }
static inline MMRESULT waveInAddBuffer(HWAVEIN, WAVEHDR *, UINT) { return 1; }
static inline MMRESULT waveInStart(HWAVEIN) { return 1; }
static inline MMRESULT waveInStop(HWAVEIN) { return 1; }
static inline MMRESULT waveInReset(HWAVEIN) { return 1; }
static inline MMRESULT waveInClose(HWAVEIN) { return 1; }
static inline MMRESULT timeBeginPeriod(UINT) { return 0; }
static inline MMRESULT timeEndPeriod(UINT) { return 0; }
