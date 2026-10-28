/* MallVR Quest: just enough "Windows" for vpix_render.cpp / vpix_net.cpp to build on Android unchanged.
   vpix_render.cpp is compiled with _WIN32 defined (its own module) so its Windows code paths are used; this header maps them:
   window + WGL -> one EGL pbuffer context (GLES 3 + gl4es, made in vpix_android.cpp), messages / keys / mouse -> nothing,
   timers -> clock_gettime, files -> stat, LoadLibrary -> dlopen, child processes (ffmpeg) -> fail. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/stat.h>

typedef int BOOL; typedef unsigned char BYTE; typedef uint16_t WORD; typedef uint32_t DWORD; typedef unsigned int UINT; typedef int32_t LONG;
typedef uintptr_t DWORD_PTR, WPARAM, UINT_PTR; typedef intptr_t LPARAM, LRESULT, LONG_PTR;
typedef void *HANDLE, *HWND, *HDC, *HGLRC, *HINSTANCE, *HMODULE, *HCURSOR, *HICON, *HBRUSH, *HMENU, *LPVOID;
typedef void *HBITMAP, *HPALETTE; typedef unsigned long long ULONGLONG; typedef long long LONGLONG;
typedef char *LPSTR; typedef const char *LPCSTR;
typedef void (*FARPROC)(void); typedef FARPROC PROC;
#define WINAPI
#define APIENTRY
#define CALLBACK
#define TRUE 1
#define FALSE 0
#define MAX_PATH 260
#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xFFFF))
#define HIWORD(l) ((WORD)(((DWORD_PTR)(l) >> 16) & 0xFFFF))
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _popen popen
#define _pclose pclose

typedef union { struct { DWORD LowPart; LONG HighPart; } u; long long QuadPart; } LARGE_INTEGER;
typedef struct { LONG left, top, right, bottom; } RECT;
typedef struct { LONG x, y; } POINT;
typedef struct { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; } MSG;
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef struct { UINT style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor;
                 HBRUSH hbrBackground; LPCSTR lpszMenuName, lpszClassName; } WNDCLASSA;
typedef struct { WORD nSize, nVersion; DWORD dwFlags; BYTE iPixelType, cColorBits, cRedBits, cRedShift, cGreenBits, cGreenShift,
                 cBlueBits, cBlueShift, cAlphaBits, cAlphaShift, cAccumBits, cAccumRedBits, cAccumGreenBits, cAccumBlueBits,
                 cAccumAlphaBits, cDepthBits, cStencilBits, cAuxBuffers, iLayerType, bReserved; DWORD dwLayerMask, dwVisibleMask, dwDamageMask; } PIXELFORMATDESCRIPTOR;
typedef struct { DWORD nLength; LPVOID lpSecurityDescriptor; BOOL bInheritHandle; } SECURITY_ATTRIBUTES;
typedef struct { DWORD cb; LPSTR lpReserved, lpDesktop, lpTitle; DWORD dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;
                 WORD wShowWindow, cbReserved2; BYTE *lpReserved2; HANDLE hStdInput, hStdOutput, hStdError; } STARTUPINFOA;
typedef struct { HANDLE hProcess, hThread; DWORD dwProcessId, dwThreadId; } PROCESS_INFORMATION;

#define WM_SIZE 0x0005
#define WM_CLOSE 0x0010
#define SW_MINIMIZE 6
#define IDC_ARROW ((LPCSTR)32512)
#define WS_OVERLAPPEDWINDOW 0x00CF0000
#define WS_VISIBLE 0x10000000
#define WS_POPUP 0x80000000
#define WS_EX_TOPMOST 0x00000008
#define PFD_DRAW_TO_WINDOW 4
#define PFD_SUPPORT_OPENGL 32
#define PFD_DOUBLEBUFFER 1
#define PFD_TYPE_RGBA 0
#define PM_REMOVE 1
#define HANDLE_FLAG_INHERIT 1
#define STARTF_USESTDHANDLES 0x100
#define STD_ERROR_HANDLE ((DWORD)-12)
#define CREATE_NO_WINDOW 0x08000000
#define INFINITE 0xFFFFFFFF
#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_MBUTTON 0x04
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_ADD 0x6B
#define VK_SUBTRACT 0x6D
#define VK_OEM_PLUS 0xBB
#define VK_OEM_MINUS 0xBD
#define VK_F1 0x70
#define VK_OEM_1 0xBA
#define VK_OEM_2 0xBF
#define VK_OEM_3 0xC0
#define VK_OEM_4 0xDB
#define VK_OEM_5 0xDC
#define VK_OEM_6 0xDD
#define VK_OEM_7 0xDE
#define VK_OEM_COMMA 0xBC
#define VK_OEM_PERIOD 0xBE

/* window / message / input: there is no desktop window on the headset */
static inline HMODULE GetModuleHandle(LPCSTR) { return (HMODULE)1; }
#define GetModuleHandleA GetModuleHandle
static inline HCURSOR LoadCursor(HINSTANCE, LPCSTR) { return NULL; }
static inline WORD RegisterClassA(const WNDCLASSA *) { return 1; }
static inline BOOL AdjustWindowRect(RECT *, DWORD, BOOL) { return TRUE; }
static inline HWND CreateWindowA(LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID) { return (HWND)1; }
static inline HWND CreateWindowExA(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID) { return (HWND)1; }
static inline BOOL SetWindowTextA(HWND, LPCSTR) { return TRUE; }
static inline BOOL ShowWindow(HWND, int) { return TRUE; }
static inline LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
static inline BOOL PeekMessage(MSG *, HWND, UINT, UINT, UINT) { return FALSE; }
static inline BOOL TranslateMessage(const MSG *) { return FALSE; }
static inline LRESULT DispatchMessage(const MSG *) { return 0; }
static inline HDC GetDC(HWND) { return (HDC)1; }
static inline int ReleaseDC(HWND, HDC) { return 1; }
static inline BOOL DestroyWindow(HWND) { return TRUE; }
static inline void ExitProcess(UINT c) { exit((int)c); }
static inline int ChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR *) { return 1; }
static inline BOOL SetPixelFormat(HDC, int, const PIXELFORMATDESCRIPTOR *) { return TRUE; }
static inline BOOL SwapBuffers(HDC) { return TRUE; }
static inline short GetAsyncKeyState(int) { return 0; }
static inline HWND GetForegroundWindow(void) { return NULL; }
static inline DWORD GetWindowThreadProcessId(HWND, DWORD *pid) { if (pid) *pid = 0; return 0; }
static inline DWORD GetCurrentProcessId(void) { return (DWORD)getpid(); }
static inline BOOL GetClientRect(HWND, RECT *r) { if (r) memset(r, 0, sizeof(*r)); return FALSE; }
static inline BOOL ClientToScreen(HWND, POINT *) { return TRUE; }
static inline BOOL GetCursorPos(POINT *p) { if (p) p->x = p->y = 0; return TRUE; }
static inline BOOL SetCursorPos(int, int) { return TRUE; }
static inline int ShowCursor(BOOL) { return 0; }

/* WGL = one EGL pbuffer context with gl4es on top (vpix_android.cpp) */
HGLRC wglCreateContext(HDC);
BOOL wglMakeCurrent(HDC, HGLRC);
BOOL wglDeleteContext(HGLRC);
PROC wglGetProcAddress(LPCSTR);
/* the EGL objects OpenXR needs for its GLES binding (vpix_xr.h, Android) */
void *VPIX_EGL_Display(void); void *VPIX_EGL_Config(void); void *VPIX_EGL_Context(void);
void *VPIX_Android_VM(void); void *VPIX_Android_Activity(void);
const char *VPIX_Android_Dir(void);   /* app folder: /sdcard/MallVR (MALLVR_DIR) */

/* time */
static inline long long vpix_now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (long long)t.tv_sec * 1000000000LL + t.tv_nsec; }
static inline BOOL QueryPerformanceFrequency(LARGE_INTEGER *f) { f->QuadPart = 1000000000LL; return TRUE; }
static inline BOOL QueryPerformanceCounter(LARGE_INTEGER *c) { c->QuadPart = vpix_now_ns(); return TRUE; }
static inline unsigned long long GetTickCount64(void) { return (unsigned long long)(vpix_now_ns() / 1000000LL); }
static inline DWORD GetTickCount(void) { return (DWORD)GetTickCount64(); }
static inline DWORD timeGetTime(void) { return (DWORD)GetTickCount64(); }
static inline void Sleep(DWORD ms) { usleep((useconds_t)ms * 1000); }

/* files / modules */
static inline DWORD GetModuleFileNameA(HMODULE, char *buf, DWORD n) {   /* "exe" = <app dir>/engine/dosbox-x: files next to it live in engine/ */
    snprintf(buf, n, "%s/engine/dosbox-x", VPIX_Android_Dir()); return (DWORD)strlen(buf);
}
static inline DWORD GetFileAttributesA(LPCSTR p) { struct stat st; return stat(p, &st) == 0 ? (S_ISDIR(st.st_mode) ? 0x10 : 0x80) : INVALID_FILE_ATTRIBUTES; }
static inline HMODULE LoadLibraryA(LPCSTR n) {
    if (!strcmp(n, "openxr_loader.dll")) n = "libopenxr_loader.so";
    return (HMODULE)dlopen(n, RTLD_NOW);
}
static inline FARPROC GetProcAddress(HMODULE m, LPCSTR n) { return (FARPROC)dlsym(m, n); }

/* child processes / pipes / events: not on the headset (SU3000 movies need pl_mpeg here, see the plan) */
static inline BOOL CreatePipe(HANDLE *, HANDLE *, SECURITY_ATTRIBUTES *, DWORD) { return FALSE; }
static inline BOOL SetHandleInformation(HANDLE, DWORD, DWORD) { return FALSE; }
static inline HANDLE GetStdHandle(DWORD) { return NULL; }
static inline BOOL CreateProcessA(LPCSTR, LPSTR, SECURITY_ATTRIBUTES *, SECURITY_ATTRIBUTES *, BOOL, DWORD, LPVOID, LPCSTR, STARTUPINFOA *, PROCESS_INFORMATION *) { return FALSE; }
static inline BOOL ReadFile(HANDLE, LPVOID, DWORD, DWORD *r, LPVOID) { if (r) *r = 0; return FALSE; }
static inline BOOL TerminateProcess(HANDLE, UINT) { return FALSE; }
static inline BOOL CloseHandle(HANDLE) { return TRUE; }
static inline HANDLE CreateEvent(LPVOID, BOOL, BOOL, LPCSTR) { return (HANDLE)1; }
static inline DWORD WaitForSingleObject(HANDLE, DWORD ms) { if (ms != INFINITE) Sleep(ms); return 0x102; }
