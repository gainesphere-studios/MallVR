/* MallVR Quest: Android side of the fake Windows layer (compat/windows.h).
   - wglCreateContext / wglMakeCurrent: one EGL context (OpenGL ES 3, 16x16 pbuffer: the headset gets its pictures through OpenXR
     swapchains, never a window), then gl4es (ptitSeb, MIT) so vpix_render.cpp's OpenGL 1.x calls run on GLES.
   - wglGetProcAddress: gl4es's GL entry points (libGL.so), e.g. glGenFramebuffers / glBlitFramebuffer for the eye FBOs.
   - the Java VM + activity (from SDL) and the app folder, for OpenXR on Android.
   - OpenVR exports: no SteamVR on the headset, so VR_IsHmdPresent() = false and the renderer never takes the OpenVR path. */
#include "windows.h"
#include <EGL/egl.h>
#include <android/log.h>
#include <jni.h>
#include <stdlib.h>
#include "SDL.h"

#define ALOG(...) __android_log_print(ANDROID_LOG_INFO, "MallVR", __VA_ARGS__)

static EGLDisplay g_dpy = EGL_NO_DISPLAY; static EGLConfig g_cfg = 0; static EGLContext g_ctx = EGL_NO_CONTEXT; static EGLSurface g_pb = EGL_NO_SURFACE;
extern "C" void initialize_gl4es(void);
extern "C" void *gl4es_GetProcAddress(const char *name);

HGLRC wglCreateContext(HDC) {
    g_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(g_dpy, NULL, NULL)) { ALOG("eglInitialize failed"); return NULL; }
    const EGLint ca[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 24,
                          EGL_RENDERABLE_TYPE, 0x40 /* EGL_OPENGL_ES3_BIT */, EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE };
    EGLint n = 0;
    if (!eglChooseConfig(g_dpy, ca, &g_cfg, 1, &n) || n < 1) { ALOG("no EGL config"); return NULL; }
    const EGLint xa[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    g_ctx = eglCreateContext(g_dpy, g_cfg, EGL_NO_CONTEXT, xa);
    const EGLint pa[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
    g_pb = eglCreatePbufferSurface(g_dpy, g_cfg, pa);
    if (g_ctx == EGL_NO_CONTEXT || g_pb == EGL_NO_SURFACE) { ALOG("EGL context/pbuffer failed 0x%X", eglGetError()); return NULL; }
    return (HGLRC)g_ctx;
}
BOOL wglMakeCurrent(HDC, HGLRC rc) {
    if (!rc) return eglMakeCurrent(g_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!eglMakeCurrent(g_dpy, g_pb, g_pb, g_ctx)) { ALOG("eglMakeCurrent failed 0x%X", eglGetError()); return FALSE; }
    static bool gl4es_up = false;
    if (!gl4es_up) { initialize_gl4es(); gl4es_up = true; ALOG("EGL GLES3 context + gl4es up"); }
    return TRUE;
}
BOOL wglDeleteContext(HGLRC) {
    if (g_dpy != EGL_NO_DISPLAY) { eglMakeCurrent(g_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); eglDestroySurface(g_dpy, g_pb); eglDestroyContext(g_dpy, g_ctx); }
    g_ctx = EGL_NO_CONTEXT; g_pb = EGL_NO_SURFACE; return TRUE;
}
PROC wglGetProcAddress(LPCSTR name) {
    if (!strcmp(name, "glClipControl") || !strcmp(name, "glClipControlARB") || !strncmp(name, "wgl", 3)) return NULL;   /* depth fix falls back to near 60 */
    return (PROC)gl4es_GetProcAddress(name);
}
void *VPIX_EGL_Display(void) { return g_dpy; }
void *VPIX_EGL_Config(void) { return g_cfg; }
void *VPIX_EGL_Context(void) { return g_ctx; }

void *VPIX_Android_VM(void) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv(); JavaVM *vm = NULL;
    if (env) env->GetJavaVM(&vm);
    return vm;
}
void *VPIX_Android_Activity(void) {
    static jobject act = NULL;
    if (!act) { JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv(); jobject a = (jobject)SDL_AndroidGetActivity(); if (env && a) { act = env->NewGlobalRef(a); env->DeleteLocalRef(a); } }
    return act;
}
const char *VPIX_Android_Dir(void) { const char *d = getenv("MALLVR_DIR"); return d && *d ? d : "/sdcard/MallVR"; }

/* OpenVR runtime exports (openvr.h's inline wrappers call these): never a headset through OpenVR on Android */
namespace vr { enum EVRInitError : int; enum EVRApplicationType : int; }
extern "C" {
bool VR_IsHmdPresent() { return false; }
bool VR_IsRuntimeInstalled() { return false; }
const char *VR_GetVRInitErrorAsSymbol(vr::EVRInitError) { return "VRInitError_Init_NotSupported"; }
const char *VR_GetVRInitErrorAsEnglishDescription(vr::EVRInitError) { return "no OpenVR on Android (MallVR uses OpenXR here)"; }
void *VR_GetGenericInterface(const char *, vr::EVRInitError *e) { if (e) *e = (vr::EVRInitError)108; return NULL; }
bool VR_IsInterfaceVersionValid(const char *) { return false; }
uint32_t VR_GetInitToken() { return 0; }
uint32_t VR_InitInternal2(vr::EVRInitError *e, vr::EVRApplicationType, const char *) { if (e) *e = (vr::EVRInitError)108; return 0; }
void VR_ShutdownInternal() {}
}
