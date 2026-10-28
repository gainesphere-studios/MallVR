/* MallVR Quest proof 1: smallest OpenXR + EGL + GLES3 app (NativeActivity, Khronos loader).
   What you see: both eyes cleared to navy; right trigger pulls it to green, left trigger to red, A/X flash yellow.
   Logcat tag "MallVRxr": instance/session state, swapchain size, head + controller poses once a second. */
#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "MallVRxr", __VA_ARGS__)
#define CHK(x) do { XrResult r_ = (x); if (XR_FAILED(r_)) { LOG("%s failed: %d (line %d)", #x, (int)r_, __LINE__); return 0; } } while (0)

static XrInstance inst; static XrSystemId sys; static XrSession sess; static XrSpace stage, view_space;
static XrSessionState state = XR_SESSION_STATE_UNKNOWN; static int running = 0, resumed = 0;
static EGLDisplay dpy; static EGLContext ctx; static EGLSurface pbuf;
static XrSwapchain sc[2]; static uint32_t sc_n[2]; static XrSwapchainImageOpenGLESKHR img[2][4]; static int ew, eh;
static GLuint fbo;
static XrActionSet aset; static XrAction a_trig, a_btn, a_pose; static XrPath hand[2]; static XrSpace hand_sp[2];

static XrPath P(const char *s) { XrPath p; xrStringToPath(inst, s, &p); return p; }

static int egl_init(void) {
    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY); eglInitialize(dpy, NULL, NULL);
    EGLint ca[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0,
                    EGL_RENDERABLE_TYPE, 0x40 /* EGL_OPENGL_ES3_BIT */, EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE };
    EGLConfig cfg; EGLint n = 0; eglChooseConfig(dpy, ca, &cfg, 1, &n); if (!n) { LOG("no EGL config"); return 0; }
    EGLint xa[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE }; ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, xa);
    EGLint pa[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE }; pbuf = eglCreatePbufferSurface(dpy, cfg, pa);
    eglMakeCurrent(dpy, pbuf, pbuf, ctx);
    LOG("GL: %s / %s", glGetString(GL_RENDERER), glGetString(GL_VERSION));
    return ctx != EGL_NO_CONTEXT;
}

static int xr_init(struct android_app *app) {
    PFN_xrInitializeLoaderKHR il; xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction *)&il);
    XrLoaderInitInfoAndroidKHR li = { XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR }; li.applicationVM = app->activity->vm; li.applicationContext = app->activity->clazz;
    CHK(il((XrLoaderInitInfoBaseHeaderKHR *)&li));
    const char *ext[] = { XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME, XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME };
    XrInstanceCreateInfoAndroidKHR ia = { XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR }; ia.applicationVM = app->activity->vm; ia.applicationActivity = app->activity->clazz;
    XrInstanceCreateInfo ci = { XR_TYPE_INSTANCE_CREATE_INFO }; ci.next = &ia; ci.enabledExtensionCount = 2; ci.enabledExtensionNames = ext;
    strcpy(ci.applicationInfo.applicationName, "MallVR hello"); ci.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    CHK(xrCreateInstance(&ci, &inst));
    XrInstanceProperties ip = { XR_TYPE_INSTANCE_PROPERTIES }; xrGetInstanceProperties(inst, &ip); LOG("runtime: %s", ip.runtimeName);
    XrSystemGetInfo sg = { XR_TYPE_SYSTEM_GET_INFO }; sg.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY; CHK(xrGetSystem(inst, &sg, &sys));
    PFN_xrGetOpenGLESGraphicsRequirementsKHR gr; xrGetInstanceProcAddr(inst, "xrGetOpenGLESGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&gr);
    XrGraphicsRequirementsOpenGLESKHR req = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR }; CHK(gr(inst, sys, &req));
    if (!egl_init()) return 0;
    XrGraphicsBindingOpenGLESAndroidKHR gb = { XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR }; gb.display = dpy; gb.context = ctx;
    { EGLint id; eglQueryContext(dpy, ctx, EGL_CONFIG_ID, &id); EGLint a[] = { EGL_CONFIG_ID, id, EGL_NONE }; EGLint n; eglChooseConfig(dpy, a, &gb.config, 1, &n); }
    XrSessionCreateInfo sc_i = { XR_TYPE_SESSION_CREATE_INFO }; sc_i.next = &gb; sc_i.systemId = sys; CHK(xrCreateSession(inst, &sc_i, &sess));
    XrReferenceSpaceCreateInfo rs = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO }; rs.poseInReferenceSpace.orientation.w = 1;
    rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT; if (XR_FAILED(xrCreateReferenceSpace(sess, &rs, &stage))) { rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; CHK(xrCreateReferenceSpace(sess, &rs, &stage)); }
    rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW; CHK(xrCreateReferenceSpace(sess, &rs, &view_space));

    XrViewConfigurationView vv[2] = { { XR_TYPE_VIEW_CONFIGURATION_VIEW }, { XR_TYPE_VIEW_CONFIGURATION_VIEW } }; uint32_t nv;
    CHK(xrEnumerateViewConfigurationViews(inst, sys, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &nv, vv));
    ew = vv[0].recommendedImageRectWidth; eh = vv[0].recommendedImageRectHeight; LOG("eye size %dx%d", ew, eh);
    for (int e = 0; e < 2; e++) {
        XrSwapchainCreateInfo si = { XR_TYPE_SWAPCHAIN_CREATE_INFO }; si.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        si.format = GL_SRGB8_ALPHA8; si.sampleCount = 1; si.width = ew; si.height = eh; si.faceCount = 1; si.arraySize = 1; si.mipCount = 1;
        CHK(xrCreateSwapchain(sess, &si, &sc[e]));
        for (int i = 0; i < 4; i++) img[e][i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
        CHK(xrEnumerateSwapchainImages(sc[e], 4, &sc_n[e], (XrSwapchainImageBaseHeader *)img[e]));
    }
    glGenFramebuffers(1, &fbo);

    /* actions: trigger value, A/X click, grip pose (same Touch profile MallVR will use) */
    XrActionSetCreateInfo as = { XR_TYPE_ACTION_SET_CREATE_INFO }; strcpy(as.actionSetName, "main"); strcpy(as.localizedActionSetName, "Main");
    CHK(xrCreateActionSet(inst, &as, &aset));
    hand[0] = P("/user/hand/left"); hand[1] = P("/user/hand/right");
    XrActionCreateInfo ac = { XR_TYPE_ACTION_CREATE_INFO }; ac.countSubactionPaths = 2; ac.subactionPaths = hand;
    ac.actionType = XR_ACTION_TYPE_FLOAT_INPUT; strcpy(ac.actionName, "trigger"); strcpy(ac.localizedActionName, "Trigger"); CHK(xrCreateAction(aset, &ac, &a_trig));
    ac.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT; strcpy(ac.actionName, "button"); strcpy(ac.localizedActionName, "Button"); CHK(xrCreateAction(aset, &ac, &a_btn));
    ac.actionType = XR_ACTION_TYPE_POSE_INPUT; strcpy(ac.actionName, "grip"); strcpy(ac.localizedActionName, "Grip"); CHK(xrCreateAction(aset, &ac, &a_pose));
    XrActionSuggestedBinding b[] = {
        { a_trig, P("/user/hand/left/input/trigger/value") }, { a_trig, P("/user/hand/right/input/trigger/value") },
        { a_btn, P("/user/hand/left/input/x/click") }, { a_btn, P("/user/hand/right/input/a/click") },
        { a_pose, P("/user/hand/left/input/grip/pose") }, { a_pose, P("/user/hand/right/input/grip/pose") } };
    XrInteractionProfileSuggestedBinding sb = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    sb.interactionProfile = P("/interaction_profiles/oculus/touch_controller"); sb.suggestedBindings = b; sb.countSuggestedBindings = 6;
    CHK(xrSuggestInteractionProfileBindings(inst, &sb));
    for (int h = 0; h < 2; h++) { XrActionSpaceCreateInfo sp = { XR_TYPE_ACTION_SPACE_CREATE_INFO }; sp.action = a_pose; sp.subactionPath = hand[h]; sp.poseInActionSpace.orientation.w = 1; CHK(xrCreateActionSpace(sess, &sp, &hand_sp[h])); }
    XrSessionActionSetsAttachInfo at = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO }; at.countActionSets = 1; at.actionSets = &aset; CHK(xrAttachSessionActionSets(sess, &at));
    LOG("OpenXR ready");
    return 1;
}

static void poll_events(void) {
    XrEventDataBuffer ev = { XR_TYPE_EVENT_DATA_BUFFER };
    while (xrPollEvent(inst, &ev) == XR_SUCCESS) {
        if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            state = ((XrEventDataSessionStateChanged *)&ev)->state; LOG("session state %d", (int)state);
            if (state == XR_SESSION_STATE_READY) { XrSessionBeginInfo bi = { XR_TYPE_SESSION_BEGIN_INFO }; bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO; xrBeginSession(sess, &bi); running = 1; }
            if (state == XR_SESSION_STATE_STOPPING) { xrEndSession(sess); running = 0; }
        }
        ev.type = XR_TYPE_EVENT_DATA_BUFFER;
    }
}

static void frame(void) {
    XrFrameState fs = { XR_TYPE_FRAME_STATE }; XrFrameWaitInfo fw = { XR_TYPE_FRAME_WAIT_INFO }; if (XR_FAILED(xrWaitFrame(sess, &fw, &fs))) return;
    XrFrameBeginInfo fb = { XR_TYPE_FRAME_BEGIN_INFO }; xrBeginFrame(sess, &fb);
    XrActiveActionSet aa = { aset, XR_NULL_PATH }; XrActionsSyncInfo sy = { XR_TYPE_ACTIONS_SYNC_INFO }; sy.countActiveActionSets = 1; sy.activeActionSets = &aa; xrSyncActions(sess, &sy);
    float trig[2] = { 0, 0 }; int btn = 0;
    for (int h = 0; h < 2; h++) {
        XrActionStateGetInfo gi = { XR_TYPE_ACTION_STATE_GET_INFO }; gi.subactionPath = hand[h];
        XrActionStateFloat f = { XR_TYPE_ACTION_STATE_FLOAT }; gi.action = a_trig; xrGetActionStateFloat(sess, &gi, &f); trig[h] = f.currentState;
        XrActionStateBoolean bb = { XR_TYPE_ACTION_STATE_BOOLEAN }; gi.action = a_btn; xrGetActionStateBoolean(sess, &gi, &bb); btn |= bb.currentState;
    }
    XrView views[2] = { { XR_TYPE_VIEW }, { XR_TYPE_VIEW } }; XrViewState vs = { XR_TYPE_VIEW_STATE }; uint32_t nv = 0;
    XrViewLocateInfo vl = { XR_TYPE_VIEW_LOCATE_INFO }; vl.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO; vl.displayTime = fs.predictedDisplayTime; vl.space = stage;
    xrLocateViews(sess, &vl, &vs, 2, &nv, views);
    static time_t last; time_t now = time(NULL);
    if (now != last) {
        last = now; XrSpaceLocation hl[2] = { { XR_TYPE_SPACE_LOCATION }, { XR_TYPE_SPACE_LOCATION } };
        for (int h = 0; h < 2; h++) xrLocateSpace(hand_sp[h], stage, fs.predictedDisplayTime, &hl[h]);
        LOG("head %.2f %.2f %.2f | L %.2f %.2f %.2f trig %.2f | R %.2f %.2f %.2f trig %.2f | btn %d",
            views[0].pose.position.x, views[0].pose.position.y, views[0].pose.position.z,
            hl[0].pose.position.x, hl[0].pose.position.y, hl[0].pose.position.z, trig[0],
            hl[1].pose.position.x, hl[1].pose.position.y, hl[1].pose.position.z, trig[1], btn);
    }
    XrCompositionLayerProjectionView pv[2] = { { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW }, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW } };
    if (fs.shouldRender) for (int e = 0; e < 2; e++) {
        uint32_t i; XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO }; xrAcquireSwapchainImage(sc[e], &ai, &i);
        XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO }; wi.timeout = XR_INFINITE_DURATION; xrWaitSwapchainImage(sc[e], &wi);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo); glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, img[e][i].image, 0);
        glViewport(0, 0, ew, eh);
        float r = 0.05f + 0.9f * trig[0], g = 0.05f + 0.9f * trig[1], bl = 0.25f;   /* navy; left trigger red, right trigger green */
        if (btn) { r = g = 1; bl = 0; }
        glClearColor(r, g, bl, 1); glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST); glScissor(ew / 2 - ew / 40, eh / 2 - eh / 40, ew / 20, eh / 20); glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT); glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO }; xrReleaseSwapchainImage(sc[e], &ri);
        pv[e].pose = views[e].pose; pv[e].fov = views[e].fov; pv[e].subImage.swapchain = sc[e];
        pv[e].subImage.imageRect.extent.width = ew; pv[e].subImage.imageRect.extent.height = eh;
    }
    XrCompositionLayerProjection lp = { XR_TYPE_COMPOSITION_LAYER_PROJECTION }; lp.space = stage; lp.viewCount = 2; lp.views = pv;
    const XrCompositionLayerBaseHeader *layers[] = { (XrCompositionLayerBaseHeader *)&lp };
    XrFrameEndInfo fe = { XR_TYPE_FRAME_END_INFO }; fe.displayTime = fs.predictedDisplayTime; fe.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fe.layerCount = fs.shouldRender ? 1 : 0; fe.layers = layers; xrEndFrame(sess, &fe);
}

static void on_cmd(struct android_app *app, int32_t cmd) {
    if (cmd == APP_CMD_RESUME) resumed = 1;
    if (cmd == APP_CMD_PAUSE) resumed = 0;
}

void android_main(struct android_app *app) {
    app->onAppCmd = on_cmd;
    LOG("MallVR hello start");
    if (!xr_init(app)) { LOG("init failed"); ANativeActivity_finish(app->activity); }
    while (!app->destroyRequested) {
        int ev; struct android_poll_source *src;
        while (ALooper_pollOnce((resumed || running) ? 0 : 250, NULL, &ev, (void **)&src) >= 0) { if (src) src->process(app, src); if (app->destroyRequested) break; }
        if (!inst) continue;
        poll_events();
        if (running) frame();
    }
    if (sess) xrDestroySession(sess);
    if (inst) xrDestroyInstance(inst);
}
