/* VPIX_XR=1: OpenXR instead of OpenVR (the Quest build's VR layer, tried first on the PC through SteamVR's OpenXR runtime).
   Included in vpix_render.cpp after vr_to_pose / VPIX_Acts / the FBO entry points. The game still renders each eye into
   our own EyeFB (so depth fix, mirror and spectator stay as they are); xr_submit copies it into the OpenXR swapchain.
   openxr_loader.dll (Khronos, next to the exe) is loaded at run time: without it, or without a runtime, MallVR falls back
   to OpenVR. Same actions as mallvr_actions.json, with bindings for Index, Vive wands and Touch (Quest / Rift).
   Dev switches: VPIX_XR_POSE=aim (controller aim pose instead of grip; LeonG's WebXR page uses aim), VPIX_XR_LINEAR=1
   (plain RGBA8 swapchain instead of sRGB, if the picture looks too dark / too bright vs OpenVR). Log lines start "XR". */
/* needs openxr/openxr.h + openxr_platform.h included at file scope first (XR_NO_PROTOTYPES, XR_USE_PLATFORM_WIN32,
   XR_USE_GRAPHICS_API_OPENGL): vpix_render.cpp's body is an anonymous namespace */

#define XR_FUNCS(F) F(xrDestroyInstance) F(xrGetInstanceProperties) F(xrGetSystem) F(xrGetSystemProperties) F(xrCreateSession) \
    F(xrDestroySession) F(xrBeginSession) F(xrEndSession) F(xrRequestExitSession) F(xrPollEvent) F(xrCreateReferenceSpace) \
    F(xrLocateSpace) F(xrEnumerateViewConfigurationViews) F(xrEnumerateSwapchainFormats) F(xrCreateSwapchain) \
    F(xrEnumerateSwapchainImages) F(xrAcquireSwapchainImage) F(xrWaitSwapchainImage) F(xrReleaseSwapchainImage) \
    F(xrWaitFrame) F(xrBeginFrame) F(xrEndFrame) F(xrLocateViews) F(xrStringToPath) F(xrCreateActionSet) F(xrCreateAction) \
    F(xrSuggestInteractionProfileBindings) F(xrCreateActionSpace) F(xrAttachSessionActionSets) F(xrSyncActions) \
    F(xrGetActionStateBoolean) F(xrGetActionStateFloat) F(xrGetActionStateVector2f) F(xrApplyHapticFeedback) \
    F(xrGetCurrentInteractionProfile) F(xrPathToString) F(xrResultToString)
static struct XrState {
    bool on = false, running = false, frame_open = false, linear = false, aim_pose = false;
    HMODULE dll = NULL; PFN_xrGetInstanceProcAddr gipa = NULL;
#define XR_PFN(n) PFN_##n n = NULL;
    XR_FUNCS(XR_PFN)
#undef XR_PFN
    XrInstance inst = XR_NULL_HANDLE; XrSystemId sys = XR_NULL_SYSTEM_ID; XrSession sess = XR_NULL_HANDLE;
    XrSpace stage = XR_NULL_HANDLE, view = XR_NULL_HANDLE, hand_sp[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrSessionState state = XR_SESSION_STATE_UNKNOWN;
    XrSwapchain sc[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE}; 
#ifdef __ANDROID__
    std::vector<XrSwapchainImageOpenGLESKHR> img[2];
#else
    std::vector<XrSwapchainImageOpenGLKHR> img[2];
#endif

    GLuint sc_fbo = 0; int ew = 0, eh = 0;
    XrFrameState fs = {XR_TYPE_FRAME_STATE}; XrView views[2] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};
    XrCompositionLayerProjectionView pv[2] = {{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}};
    bool eye_done[2] = {false, false};
    XrActionSet aset = XR_NULL_HANDLE; XrPath hand[2] = {0, 0};   /* 0 = right (the gun hand, like acts.hand_src), 1 = left */
    XrAction fire, menu, walk, top, rmove, lmove, pad, trim, turnl, turnr, aimpad, aim, reachpad, reach, crouch, mode, voice_mute, thr, brk, pose, haptic;
} xr;

#ifdef __ANDROID__
#include <EGL/egl.h>
static struct { PBindBuf BindFramebuffer; PGenBuf GenFramebuffers; PFbTex FramebufferTexture2D; PBlit BlitFramebuffer; void (APIENTRY *Disable)(GLenum); } xr_gles;
static void *xr_gles_sym(const char *n) {   /* core GLES entry points: EGL, else the driver library */
    void *p = (void *)eglGetProcAddress(n); if (p) return p;
    static void *lib = dlopen("libGLESv3.so", RTLD_NOW); return lib ? dlsym(lib, n) : NULL;
}
static void xr_gles_load() {
    xr_gles.BindFramebuffer = (PBindBuf)xr_gles_sym("glBindFramebuffer"); xr_gles.GenFramebuffers = (PGenBuf)xr_gles_sym("glGenFramebuffers");
    xr_gles.FramebufferTexture2D = (PFbTex)xr_gles_sym("glFramebufferTexture2D"); xr_gles.BlitFramebuffer = (PBlit)xr_gles_sym("glBlitFramebuffer");
    xr_gles.Disable = (void (APIENTRY *)(GLenum))xr_gles_sym("glDisable");
    const char *ex = (const char *)((const unsigned char *(APIENTRY *)(GLenum))xr_gles_sym("glGetString"))(0x1F03 /* GL_EXTENSIONS */);
    rl("XR GLES: sRGB write control %s, clip control %s", ex && strstr(ex, "GL_EXT_sRGB_write_control") ? "yes" : "NO", ex && strstr(ex, "GL_EXT_clip_control") ? "yes" : "no");
}
#define XR_GL(f) xr_gles.f
#else
#define XR_GL(f) XR_GL_W_##f
#define XR_GL_W_GenFramebuffers glGenFramebuffers_
#define XR_GL_W_BindFramebuffer glBindFramebuffer_
#define XR_GL_W_FramebufferTexture2D glFramebufferTexture2D_
#define XR_GL_W_BlitFramebuffer glBlitFramebuffer_
#define XR_GL_W_Disable glDisable
#endif

static bool xr_ok(XrResult r, const char *what) {
    if (XR_SUCCEEDED(r)) return true;
    char s[XR_MAX_RESULT_STRING_SIZE] = "?"; if (xr.xrResultToString && xr.inst) xr.xrResultToString(xr.inst, r, s);
    rl("XR %s failed: %s (%d)", what, s, (int)r);
    return false;
}
static XrPath xr_path(const char *s) { XrPath p = 0; xr.xrStringToPath(xr.inst, s, &p); return p; }

static void xr_make_actions() {
    XrActionSetCreateInfo as = {XR_TYPE_ACTION_SET_CREATE_INFO}; strcpy(as.actionSetName, "main"); strcpy(as.localizedActionSetName, "MallVR");
    if (!xr_ok(xr.xrCreateActionSet(xr.inst, &as, &xr.aset), "xrCreateActionSet")) return;
    xr.hand[0] = xr_path("/user/hand/right"); xr.hand[1] = xr_path("/user/hand/left");
    auto mk = [&](XrAction &a, const char *name, XrActionType t, bool per_hand) {
        XrActionCreateInfo ci = {XR_TYPE_ACTION_CREATE_INFO}; ci.actionType = t;
        strcpy(ci.actionName, name); strcpy(ci.localizedActionName, name);
        if (per_hand) { ci.countSubactionPaths = 2; ci.subactionPaths = xr.hand; }
        a = XR_NULL_HANDLE; xr_ok(xr.xrCreateAction(xr.aset, &ci, &a), name);
    };
    const XrActionType B = XR_ACTION_TYPE_BOOLEAN_INPUT, V = XR_ACTION_TYPE_VECTOR2F_INPUT, F = XR_ACTION_TYPE_FLOAT_INPUT;
    mk(xr.fire, "fire", B, false); mk(xr.menu, "menu", B, false); mk(xr.walk, "walk", B, false); mk(xr.top, "top", B, false);
    mk(xr.rmove, "move_right", V, false); mk(xr.lmove, "move_left", V, false); mk(xr.pad, "trim_pad", V, false); mk(xr.trim, "trim_press", B, false);
    mk(xr.turnl, "turn_left", B, false); mk(xr.turnr, "turn_right", B, false); mk(xr.aimpad, "aim_pad", V, false); mk(xr.aim, "aim_press", B, false);
    mk(xr.reachpad, "reach_pad", V, false); mk(xr.reach, "reach_press", B, false); mk(xr.crouch, "crouch", B, false); mk(xr.mode, "mode", B, false);
    mk(xr.voice_mute, "voice_mute", B, false); mk(xr.thr, "throttle", F, false); mk(xr.brk, "brake", F, false);
    mk(xr.pose, "hand_pose", XR_ACTION_TYPE_POSE_INPUT, true); mk(xr.haptic, "haptic", XR_ACTION_TYPE_VIBRATION_OUTPUT, true);
    const char *pose = xr.aim_pose ? "aim" : "grip";
    /* the same layout as mallvr_bindings_*.json (SteamVR Input) */
    struct B_ { XrAction *a; const char *p; };
    auto suggest = [&](const char *profile, std::vector<B_> bl) {
        char rp[64], lp[64]; snprintf(rp, sizeof(rp), "/user/hand/right/input/%s/pose", pose); snprintf(lp, sizeof(lp), "/user/hand/left/input/%s/pose", pose);
        bl.push_back({&xr.pose, rp}); bl.push_back({&xr.pose, lp});
        bl.push_back({&xr.haptic, "/user/hand/right/output/haptic"}); bl.push_back({&xr.haptic, "/user/hand/left/output/haptic"});
        std::vector<XrActionSuggestedBinding> sb;
        for (const B_ &b : bl) if (*b.a) sb.push_back({*b.a, xr_path(b.p)});
        XrInteractionProfileSuggestedBinding ip = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        ip.interactionProfile = xr_path(profile); ip.suggestedBindings = sb.data(); ip.countSuggestedBindings = (uint32_t)sb.size();
        xr_ok(xr.xrSuggestInteractionProfileBindings(xr.inst, &ip), profile);
    };
    suggest("/interaction_profiles/valve/index_controller", {
        {&xr.fire, "/user/hand/right/input/trigger/click"}, {&xr.thr, "/user/hand/right/input/trigger/value"},
        {&xr.rmove, "/user/hand/right/input/thumbstick"}, {&xr.mode, "/user/hand/right/input/thumbstick/click"},
        {&xr.lmove, "/user/hand/left/input/thumbstick"}, {&xr.voice_mute, "/user/hand/left/input/thumbstick/click"},
        {&xr.aimpad, "/user/hand/left/input/trackpad"}, {&xr.aim, "/user/hand/left/input/trackpad/touch"},
        {&xr.reachpad, "/user/hand/right/input/trackpad"}, {&xr.reach, "/user/hand/right/input/trackpad/touch"},
        {&xr.crouch, "/user/hand/right/input/a/click"}, {&xr.crouch, "/user/hand/left/input/a/click"},
        {&xr.menu, "/user/hand/right/input/b/click"}, {&xr.menu, "/user/hand/left/input/b/click"},
        {&xr.top, "/user/hand/right/input/squeeze/value"},
        {&xr.mode, "/user/hand/left/input/trigger/click"}, {&xr.brk, "/user/hand/left/input/trigger/value"} });
    suggest("/interaction_profiles/htc/vive_controller", {
        {&xr.fire, "/user/hand/right/input/trigger/click"}, {&xr.thr, "/user/hand/right/input/trigger/value"},
        {&xr.rmove, "/user/hand/right/input/trackpad"}, {&xr.walk, "/user/hand/right/input/trackpad/click"},
        {&xr.pad, "/user/hand/left/input/trackpad"}, {&xr.trim, "/user/hand/left/input/trackpad/click"},
        {&xr.turnl, "/user/hand/left/input/squeeze/click"}, {&xr.turnr, "/user/hand/right/input/squeeze/click"},
        {&xr.menu, "/user/hand/right/input/menu/click"}, {&xr.crouch, "/user/hand/left/input/menu/click"},
        {&xr.mode, "/user/hand/left/input/trigger/click"}, {&xr.brk, "/user/hand/left/input/trigger/value"} });
    suggest("/interaction_profiles/oculus/touch_controller", {
        {&xr.fire, "/user/hand/right/input/trigger/value"}, {&xr.thr, "/user/hand/right/input/trigger/value"},
        {&xr.mode, "/user/hand/left/input/trigger/value"}, {&xr.brk, "/user/hand/left/input/trigger/value"},
        {&xr.rmove, "/user/hand/right/input/thumbstick"}, {&xr.mode, "/user/hand/right/input/thumbstick/click"},
        {&xr.lmove, "/user/hand/left/input/thumbstick"}, {&xr.voice_mute, "/user/hand/left/input/thumbstick/click"},
        {&xr.crouch, "/user/hand/right/input/a/click"}, {&xr.menu, "/user/hand/right/input/b/click"},
        {&xr.crouch, "/user/hand/left/input/x/click"}, {&xr.aim, "/user/hand/left/input/y/click"},
        {&xr.top, "/user/hand/right/input/squeeze/value"} });
    suggest("/interaction_profiles/khr/simple_controller", {
        {&xr.fire, "/user/hand/right/input/select/click"}, {&xr.menu, "/user/hand/right/input/menu/click"},
        {&xr.mode, "/user/hand/left/input/select/click"}, {&xr.crouch, "/user/hand/left/input/menu/click"} });
    for (int h = 0; h < 2; h++) {
        XrActionSpaceCreateInfo sp = {XR_TYPE_ACTION_SPACE_CREATE_INFO}; sp.action = xr.pose; sp.subactionPath = xr.hand[h]; sp.poseInActionSpace.orientation.w = 1;
        xr_ok(xr.xrCreateActionSpace(xr.sess, &sp, &xr.hand_sp[h]), "xrCreateActionSpace");
    }
    XrSessionActionSetsAttachInfo at = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO}; at.countActionSets = 1; at.actionSets = &xr.aset;
    xr_ok(xr.xrAttachSessionActionSets(xr.sess, &at), "xrAttachSessionActionSets");
    rl("XR actions ready (Index / Vive / Touch / simple bindings, %s pose)", pose);
}

/* true = OpenXR is up and eye size is known; false = use OpenVR (or the desktop window) as before */
static bool xr_init(HDC dc, HGLRC rc, uint32_t &ew, uint32_t &eh) {
    if (!(getenv("VPIX_XR") && atoi(getenv("VPIX_XR")))) return false;
    xr.linear = getenv("VPIX_XR_LINEAR") && atoi(getenv("VPIX_XR_LINEAR"));
    xr.aim_pose = getenv("VPIX_XR_POSE") && !strcmp(getenv("VPIX_XR_POSE"), "aim");
    xr.dll = LoadLibraryA("openxr_loader.dll");
    if (!xr.dll) { rl("XR: openxr_loader.dll not found next to the exe -> OpenVR"); return false; }
    xr.gipa = (PFN_xrGetInstanceProcAddr)GetProcAddress(xr.dll, "xrGetInstanceProcAddr");
    PFN_xrCreateInstance ci_fn = NULL;
#ifdef __ANDROID__
    /* Android: the loader needs the Java VM + activity before anything else (Khronos loader, Quest OS v62+) */
    PFN_xrInitializeLoaderKHR il = NULL;
    if (xr.gipa) xr.gipa(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction *)&il);
    XrLoaderInitInfoAndroidKHR li = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR}; li.applicationVM = VPIX_Android_VM(); li.applicationContext = VPIX_Android_Activity();
    if (!il || !li.applicationVM || !li.applicationContext || XR_FAILED(il((XrLoaderInitInfoBaseHeaderKHR *)&li))) { rl("XR: loader init failed (VM %p, activity %p)", li.applicationVM, li.applicationContext); return false; }
#endif
    if (!xr.gipa || XR_FAILED(xr.gipa(XR_NULL_HANDLE, "xrCreateInstance", (PFN_xrVoidFunction *)&ci_fn)) || !ci_fn) { rl("XR: bad loader"); return false; }
#ifdef __ANDROID__
    const char *ext[] = {XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME, XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
    XrInstanceCreateInfoAndroidKHR ia = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR}; ia.applicationVM = VPIX_Android_VM(); ia.applicationActivity = VPIX_Android_Activity();
    XrInstanceCreateInfo ci = {XR_TYPE_INSTANCE_CREATE_INFO}; ci.next = &ia; ci.enabledExtensionCount = 2; ci.enabledExtensionNames = ext;
#else
    const char *ext[] = {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};
    XrInstanceCreateInfo ci = {XR_TYPE_INSTANCE_CREATE_INFO}; ci.enabledExtensionCount = 1; ci.enabledExtensionNames = ext;
#endif
    strcpy(ci.applicationInfo.applicationName, "MallVR"); strcpy(ci.applicationInfo.engineName, "DOSBox-X VPIX");
    ci.applicationInfo.applicationVersion = 1; ci.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    XrResult r = ci_fn(&ci, &xr.inst);
    if (XR_FAILED(r)) { rl("XR: xrCreateInstance failed (%d): no OpenXR runtime? -> OpenVR", (int)r); return false; }
#define XR_GET(n) xr.gipa(xr.inst, #n, (PFN_xrVoidFunction *)&xr.n);
    XR_FUNCS(XR_GET)
#undef XR_GET
    XrInstanceProperties ip = {XR_TYPE_INSTANCE_PROPERTIES}; xr.xrGetInstanceProperties(xr.inst, &ip);
    rl("XR runtime: %s %u.%u.%u", ip.runtimeName, XR_VERSION_MAJOR(ip.runtimeVersion), XR_VERSION_MINOR(ip.runtimeVersion), XR_VERSION_PATCH(ip.runtimeVersion));
    XrSystemGetInfo sg = {XR_TYPE_SYSTEM_GET_INFO}; sg.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (!xr_ok(xr.xrGetSystem(xr.inst, &sg, &xr.sys), "xrGetSystem (headset on?)")) return false;
    XrSystemProperties sp = {XR_TYPE_SYSTEM_PROPERTIES}; xr.xrGetSystemProperties(xr.inst, xr.sys, &sp); rl("XR system: %s", sp.systemName);
#ifdef __ANDROID__
    PFN_xrGetOpenGLESGraphicsRequirementsKHR greq = NULL; xr.gipa(xr.inst, "xrGetOpenGLESGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&greq);
    XrGraphicsRequirementsOpenGLESKHR gr = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
    if (!greq || !xr_ok(greq(xr.inst, xr.sys, &gr), "xrGetOpenGLESGraphicsRequirementsKHR")) return false;
    XrGraphicsBindingOpenGLESAndroidKHR gb = {XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
    gb.display = (EGLDisplay)VPIX_EGL_Display(); gb.config = (EGLConfig)VPIX_EGL_Config(); gb.context = (EGLContext)VPIX_EGL_Context(); (void)dc; (void)rc;
    xr_gles_load();
#else
    PFN_xrGetOpenGLGraphicsRequirementsKHR greq = NULL; xr.gipa(xr.inst, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&greq);
    XrGraphicsRequirementsOpenGLKHR gr = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
    if (!greq || !xr_ok(greq(xr.inst, xr.sys, &gr), "xrGetOpenGLGraphicsRequirementsKHR")) return false;
    XrGraphicsBindingOpenGLWin32KHR gb = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR}; gb.hDC = dc; gb.hGLRC = rc;
#endif
    XrSessionCreateInfo sci = {XR_TYPE_SESSION_CREATE_INFO}; sci.next = &gb; sci.systemId = xr.sys;
    if (!xr_ok(xr.xrCreateSession(xr.inst, &sci, &xr.sess), "xrCreateSession")) return false;
    XrReferenceSpaceCreateInfo rs = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO}; rs.poseInReferenceSpace.orientation.w = 1;
    rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;     /* = SteamVR's standing universe, like OpenVR's poses */
    if (XR_FAILED(xr.xrCreateReferenceSpace(xr.sess, &rs, &xr.stage))) {
        rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; rl("XR: no STAGE space, LOCAL (seated origin)");
        if (!xr_ok(xr.xrCreateReferenceSpace(xr.sess, &rs, &xr.stage), "LOCAL space")) return false;
    }
    rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW; xr_ok(xr.xrCreateReferenceSpace(xr.sess, &rs, &xr.view), "VIEW space");
    XrViewConfigurationView vv[2] = {{XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW}}; uint32_t nv = 0;
    if (!xr_ok(xr.xrEnumerateViewConfigurationViews(xr.inst, xr.sys, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &nv, vv), "views")) return false;
    xr.ew = (int)vv[0].recommendedImageRectWidth; xr.eh = (int)vv[0].recommendedImageRectHeight;
    if (getenv("VPIX_XR_SCALE") && atof(getenv("VPIX_XR_SCALE")) > 0) { float s = (float)atof(getenv("VPIX_XR_SCALE")); xr.ew = (int)(xr.ew * s); xr.eh = (int)(xr.eh * s); }
    uint32_t nf = 0; xr.xrEnumerateSwapchainFormats(xr.sess, 0, &nf, NULL);
    std::vector<int64_t> fmts(nf); xr.xrEnumerateSwapchainFormats(xr.sess, nf, &nf, fmts.data());
    /* our pixels are gamma-encoded (OpenVR got ColorSpace_Gamma): an sRGB swapchain filled with no conversion shows them the same */
    int64_t want = xr.linear ? 0x8058 /* GL_RGBA8 */ : 0x8C43 /* GL_SRGB8_ALPHA8 */, fmt = 0;
    for (int64_t f : fmts) if (f == want) fmt = f;
    if (!fmt && !fmts.empty()) fmt = fmts[0];
    for (int e = 0; e < 2; e++) {
        XrSwapchainCreateInfo si = {XR_TYPE_SWAPCHAIN_CREATE_INFO}; si.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        si.format = fmt; si.sampleCount = 1; si.width = xr.ew; si.height = xr.eh; si.faceCount = 1; si.arraySize = 1; si.mipCount = 1;
        if (!xr_ok(xr.xrCreateSwapchain(xr.sess, &si, &xr.sc[e]), "xrCreateSwapchain")) return false;
        uint32_t ni = 0; xr.xrEnumerateSwapchainImages(xr.sc[e], 0, &ni, NULL);
#ifdef __ANDROID__
        xr.img[e].assign(ni, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
#else
        xr.img[e].assign(ni, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
#endif
        xr.xrEnumerateSwapchainImages(xr.sc[e], ni, &ni, (XrSwapchainImageBaseHeader *)xr.img[e].data());
    }
    XR_GL(GenFramebuffers)(1, &xr.sc_fbo);
    xr_make_actions();
    ew = xr.ew; eh = xr.eh; xr.on = true;
    rl("XR up: eye %dx%d, swapchain format 0x%llX (%s), %u formats offered", xr.ew, xr.eh, (unsigned long long)fmt, fmt == 0x8C43 ? "sRGB" : "linear", nf);
    return true;
}

static void xr_poll() {
    XrEventDataBuffer ev = {XR_TYPE_EVENT_DATA_BUFFER};
    while (xr.xrPollEvent(xr.inst, &ev) == XR_SUCCESS) {
        if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            xr.state = ((XrEventDataSessionStateChanged *)&ev)->state;
            rl("XR session state %d", (int)xr.state);
            if (xr.state == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo bi = {XR_TYPE_SESSION_BEGIN_INFO}; bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                xr.running = xr_ok(xr.xrBeginSession(xr.sess, &bi), "xrBeginSession");
            } else if (xr.state == XR_SESSION_STATE_STOPPING) { xr.xrEndSession(xr.sess); xr.running = false; }
            else if (xr.state == XR_SESSION_STATE_EXITING || xr.state == XR_SESSION_STATE_LOSS_PENDING) { rl("XR: runtime is closing the session"); xr.running = false; }
        } else if (ev.type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED) {
            for (int h = 0; h < 2; h++) {
                XrInteractionProfileState ps = {XR_TYPE_INTERACTION_PROFILE_STATE}; char s[256] = "none"; uint32_t n = 0;
                if (XR_SUCCEEDED(xr.xrGetCurrentInteractionProfile(xr.sess, xr.hand[h], &ps)) && ps.interactionProfile) xr.xrPathToString(xr.inst, ps.interactionProfile, sizeof(s), &n, s);
                rl("XR %s hand controller: %s", h ? "left" : "right", s);
            }
        }
        ev = {XR_TYPE_EVENT_DATA_BUFFER};
    }
}

/* OpenXR pose (stage space, metres, y up, -z forward: the same frame as OpenVR's standing universe) as an OpenVR 3x4 */
static vr::HmdMatrix34_t xr_mat(const XrPosef &p) {
    float x = p.orientation.x, y = p.orientation.y, z = p.orientation.z, w = p.orientation.w;
    vr::HmdMatrix34_t m;
    m.m[0][0] = 1 - 2 * (y * y + z * z); m.m[0][1] = 2 * (x * y - z * w);     m.m[0][2] = 2 * (x * z + y * w);     m.m[0][3] = p.position.x;
    m.m[1][0] = 2 * (x * y + z * w);     m.m[1][1] = 1 - 2 * (x * x + z * z); m.m[1][2] = 2 * (y * z - x * w);     m.m[1][3] = p.position.y;
    m.m[2][0] = 2 * (x * z - y * w);     m.m[2][1] = 2 * (y * z + x * w);     m.m[2][2] = 1 - 2 * (x * x + y * y); m.m[2][3] = p.position.z;
    return m;
}
static bool xr_space_pose(XrSpace s, VPIX_Pose &out) {
    XrSpaceLocation l = {XR_TYPE_SPACE_LOCATION};
    if (!s || XR_FAILED(xr.xrLocateSpace(s, xr.stage, xr.fs.predictedDisplayTime, &l))) return false;
    const XrSpaceLocationFlags need = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
    if ((l.locationFlags & need) != need) return false;
    out = vr_to_pose(xr_mat(l.pose)); return true;
}

/* start a frame: false = no frame this time (session not running / runtime says don't render); the caller waits a bit */
struct XrPoses { VPIX_Pose head, eye[2], hand[2]; bool head_ok = false, hand_ok[2] = {false, false}; };
static bool xr_begin(XrPoses &o) {
    xr_poll();
    if (!xr.running) return false;
    xr.fs = {XR_TYPE_FRAME_STATE}; XrFrameWaitInfo wi = {XR_TYPE_FRAME_WAIT_INFO};
    if (!xr_ok(xr.xrWaitFrame(xr.sess, &wi, &xr.fs), "xrWaitFrame")) return false;
    XrFrameBeginInfo bi = {XR_TYPE_FRAME_BEGIN_INFO};
    if (XR_FAILED(xr.xrBeginFrame(xr.sess, &bi))) return false;
    xr.frame_open = true; xr.eye_done[0] = xr.eye_done[1] = false;
    XrActiveActionSet aa = {xr.aset, XR_NULL_PATH}; XrActionsSyncInfo sy = {XR_TYPE_ACTIONS_SYNC_INFO}; sy.countActiveActionSets = 1; sy.activeActionSets = &aa;
    if (xr.aset) xr.xrSyncActions(xr.sess, &sy);
    XrViewLocateInfo vl = {XR_TYPE_VIEW_LOCATE_INFO}; vl.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    vl.displayTime = xr.fs.predictedDisplayTime; vl.space = xr.stage;
    XrViewState vs = {XR_TYPE_VIEW_STATE}; uint32_t nv = 0; xr.views[0] = {XR_TYPE_VIEW}; xr.views[1] = {XR_TYPE_VIEW};
    xr.xrLocateViews(xr.sess, &vl, &vs, 2, &nv, xr.views);
    o.head_ok = xr_space_pose(xr.view, o.head);
    for (int e = 0; e < 2; e++) o.eye[e] = vr_to_pose(xr_mat(xr.views[e].pose));
    for (int h = 0; h < 2; h++) o.hand_ok[h] = xr_space_pose(xr.hand_sp[h], o.hand[h]);
    return true;
}
/* GL projection for eye e (same layout as the OpenVR path builds: column-major, then rev_proj) */
static void xr_proj(int e, float P[16]) {
    const XrFovf &f = xr.views[e].fov; float n = z_near, fa = 400000.f;
    float l = tanf(f.angleLeft) * n, r = tanf(f.angleRight) * n, t = tanf(f.angleUp) * n, b = tanf(f.angleDown) * n;
    memset(P, 0, 64);
    P[0] = 2 * n / (r - l); P[8] = (r + l) / (r - l); P[5] = 2 * n / (t - b); P[9] = (t + b) / (t - b);
    P[10] = -(fa + n) / (fa - n); P[11] = -1; P[14] = -2 * fa * n / (fa - n);
    rev_proj(P);
}
/* copy our eye FBO into this eye's swapchain image */
static void xr_submit(int e, GLuint src_fbo) {
    if (!xr.frame_open || !xr.fs.shouldRender) return;
    uint32_t i = 0; XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if (XR_FAILED(xr.xrAcquireSwapchainImage(xr.sc[e], &ai, &i))) return;
    XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO}; wi.timeout = XR_INFINITE_DURATION; xr.xrWaitSwapchainImage(xr.sc[e], &wi);
    XR_GL(BindFramebuffer)(GL_READ_FRAMEBUFFER, src_fbo); XR_GL(BindFramebuffer)(GL_DRAW_FRAMEBUFFER, xr.sc_fbo);
    XR_GL(FramebufferTexture2D)(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, xr.img[e][i].image, 0);
    XR_GL(Disable)(0x8DB9 /* GL_FRAMEBUFFER_SRGB(_EXT): copy the bytes as they are */);
    XR_GL(BlitFramebuffer)(0, 0, xr.ew, xr.eh, 0, 0, xr.ew, xr.eh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    XR_GL(BindFramebuffer)(GL_FRAMEBUFFER, 0);
    XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO}; xr.xrReleaseSwapchainImage(xr.sc[e], &ri);
    XrCompositionLayerProjectionView &v = xr.pv[e]; v = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
    v.pose = xr.views[e].pose; v.fov = xr.views[e].fov; v.subImage.swapchain = xr.sc[e];
    v.subImage.imageRect.extent.width = xr.ew; v.subImage.imageRect.extent.height = xr.eh;
    xr.eye_done[e] = true;
}
static void xr_end() {
    if (!xr.frame_open) return;
    XrCompositionLayerProjection lp = {XR_TYPE_COMPOSITION_LAYER_PROJECTION}; lp.space = xr.stage; lp.viewCount = 2; lp.views = xr.pv;
    const XrCompositionLayerBaseHeader *layers[] = {(XrCompositionLayerBaseHeader *)&lp};
    XrFrameEndInfo fe = {XR_TYPE_FRAME_END_INFO}; fe.displayTime = xr.fs.predictedDisplayTime; fe.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fe.layerCount = (xr.fs.shouldRender && xr.eye_done[0] && xr.eye_done[1]) ? 1 : 0; fe.layers = layers;
    static int nerr = 0; XrResult r = xr.xrEndFrame(xr.sess, &fe);
    if (XR_FAILED(r) && nerr++ < 5) xr_ok(r, "xrEndFrame");
    xr.frame_open = false;
}
/* the same VPIX_Acts the SteamVR Input path fills */
static void xr_acts_read(VPIX_Acts &a) {
    auto b = [&](XrAction act) {
        XrActionStateGetInfo gi = {XR_TYPE_ACTION_STATE_GET_INFO}; gi.action = act; XrActionStateBoolean s = {XR_TYPE_ACTION_STATE_BOOLEAN};
        return act && XR_SUCCEEDED(xr.xrGetActionStateBoolean(xr.sess, &gi, &s)) && s.isActive && s.currentState;
    };
    auto v = [&](XrAction act, float o[2]) {
        XrActionStateGetInfo gi = {XR_TYPE_ACTION_STATE_GET_INFO}; gi.action = act; XrActionStateVector2f s = {XR_TYPE_ACTION_STATE_VECTOR2F};
        if (act && XR_SUCCEEDED(xr.xrGetActionStateVector2f(xr.sess, &gi, &s)) && s.isActive) { o[0] = s.currentState.x; o[1] = s.currentState.y; }
    };
    auto f = [&](XrAction act) {
        XrActionStateGetInfo gi = {XR_TYPE_ACTION_STATE_GET_INFO}; gi.action = act; XrActionStateFloat s = {XR_TYPE_ACTION_STATE_FLOAT};
        return (act && XR_SUCCEEDED(xr.xrGetActionStateFloat(xr.sess, &gi, &s)) && s.isActive) ? s.currentState : 0.f;
    };
    a.fire = b(xr.fire); a.menu = b(xr.menu); a.walk = b(xr.walk); a.top = b(xr.top); a.trim = b(xr.trim);
    a.turnl = b(xr.turnl); a.turnr = b(xr.turnr);
    v(xr.rmove, a.rmove); v(xr.lmove, a.lmove); v(xr.pad, a.pad);
    a.aim = b(xr.aim); a.reach = b(xr.reach); v(xr.aimpad, a.aimpad); v(xr.reachpad, a.reachpad);
    a.crouch = b(xr.crouch); a.mode = b(xr.mode); a.voice_mute = b(xr.voice_mute);
    a.thr = f(xr.thr); a.brk = f(xr.brk);
}
static void xr_haptic(int hand, float secs, float amp) {
    if (!xr.haptic) return;
    for (int h = 0; h < 2; h++) if (hand == 2 || hand == h) {
        XrHapticVibration hv = {XR_TYPE_HAPTIC_VIBRATION}; hv.duration = (XrDuration)(secs * 1e9); hv.frequency = XR_FREQUENCY_UNSPECIFIED; hv.amplitude = amp > 1.f ? 1.f : amp;
        XrHapticActionInfo hi = {XR_TYPE_HAPTIC_ACTION_INFO}; hi.action = xr.haptic; hi.subactionPath = xr.hand[h];
        xr.xrApplyHapticFeedback(xr.sess, &hi, (XrHapticBaseHeader *)&hv);
    }
}
static void xr_shutdown() {
    if (xr.sess) { if (xr.running) xr.xrEndSession(xr.sess); xr.xrDestroySession(xr.sess); xr.sess = XR_NULL_HANDLE; }
    if (xr.inst) { xr.xrDestroyInstance(xr.inst); xr.inst = XR_NULL_HANDLE; }
    xr.on = xr.running = false;
}
