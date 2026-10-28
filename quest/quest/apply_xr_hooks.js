// MallVR OpenXR hooks (VPIX_XR=1) for vpix_render.cpp, applied to a COPY of the master file:
//   node quest/apply_xr_hooks.js <in vpix_render.cpp> <out vpix_render.cpp>
// The master copy (dosbox_src/vpix_render.cpp) is edited daily by other work, so the ~25 hook lines live here as
// text replacements with exact anchors until they are merged; every anchor must match once, or the script stops.
// All OpenXR code itself is in dosbox_src/vpix_xr.h.
const fs = require('fs');
const [inp, out] = process.argv.slice(2);
let s = fs.readFileSync(inp, 'utf8');
const crlf = s.includes('\r\n'); if (crlf) s = s.replace(/\r\n/g, '\n');
function rep(from, to) {
    const n = s.split(from).length - 1;
    if (n !== 1) { console.error(`anchor found ${n} times (need 1):\n${from}`); process.exit(1); }
    s = s.replace(from, () => to);
}

// 0. OpenXR headers at file scope (the rest of the file is inside an anonymous namespace; vpix_xr.h's own includes are then no-ops)
rep(`#include "openvr.h"\n`, `#include "openvr.h"
#define XR_NO_PROTOTYPES
#ifdef __ANDROID__
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#else
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#include <unknwn.h>
#endif
#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"
`);

// 1. forward declarations used before vpix_xr.h is included (HUD scale, haptics)
rep(`static vr::IVRSystem *vrsys = NULL;    /* set when SteamVR starts */`,
    `static vr::IVRSystem *vrsys = NULL;    /* set when SteamVR starts */
static bool xr_active();                /* VPIX_XR: OpenXR is the VR layer (vpix_xr.h) */
static void xr_haptic(int hand, float secs, float amp);`);
rep(`if (!backdrop && vrsys && hud_sc > 0.f`, `if (!backdrop && (vrsys || xr_active()) && hud_sc > 0.f`);
rep(`    if (q.empty() || !acts.ok || !acts.haptic) return;`,
    `    if (!q.empty() && xr_active()) { for (const HapticReq &r : q) xr_haptic(r.hand, r.secs, r.amp); return; }
    if (q.empty() || !acts.ok || !acts.haptic) return;`);

// 2. the OpenXR layer, after vr_to_pose / mul34
rep(`            o.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + (j == 3 ? a.m[i][3] : 0.f);
    return o;
}
`, `            o.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + (j == 3 ? a.m[i][3] : 0.f);
    return o;
}
#include "vpix_xr.h"
static bool xr_active() { return xr.on; }
`);

// 3. start-up: OpenXR first when VPIX_XR=1, else SteamVR (OpenVR) exactly as before
rep(`    const char *ve = getenv("VPIX_VR");
    if (!(ve && atoi(ve) == 0) && vr::VR_IsHmdPresent() && load_fbo()) {`,
    `    const char *ve = getenv("VPIX_VR");
    bool xr_up = false;
    if (!(ve && atoi(ve) == 0) && load_fbo() && xr_init(dc, rc, ew, eh)) {
        xr_up = make_eye_fb(eyes[0], ew, eh) && make_eye_fb(eyes[1], ew, eh);
        if (!xr_up) { rl("XR eye FBOs FAILED -> OpenVR"); xr_shutdown(); }
        else SetWindowTextA(hw, "MallVR - Gainesphere Studios - OpenXR (left eye mirror)");
    }
    if (xr_up) {}
    else if (!(ve && atoi(ve) == 0) && vr::VR_IsHmdPresent() && load_fbo()) {`);
rep(`    if (swi) swi(sys ? 0 : 1);`, `    if (swi) swi((sys || xr_up) ? 0 : 1);`);

// 4. per frame: poses + actions
rep(`        VPIX_Pose eye_pose[2], head_now;
        if (sys) {
            vr::VREvent_t ev;
            while (sys->PollNextEvent(&ev, sizeof(ev))) if (ev.eventType == vr::VREvent_Quit) { rl("SteamVR quit"); sys->AcknowledgeQuit_Exiting(); }
            vr::VRCompositor()->WaitGetPoses(poses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
            /* the gun hand: right-hand controller, else any controller */
            vr::TrackedDeviceIndex_t ci = sys->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
            if (ci == vr::k_unTrackedDeviceIndexInvalid || !poses[ci].bPoseIsValid)
                for (vr::TrackedDeviceIndex_t i = 1; i < vr::k_unMaxTrackedDeviceCount; i++)
                    if (sys->GetTrackedDeviceClass(i) == vr::TrackedDeviceClass_Controller && poses[i].bPoseIsValid) { ci = i; break; }`,
    `        VPIX_Pose eye_pose[2], head_now;
        XrPoses xp;
        if (xr_up && !xr_begin(xp)) { Sleep(10); continue; }   /* OpenXR session not running yet / paused */
        if (sys || xr_up) {
            vr::TrackedDeviceIndex_t ci = vr::k_unTrackedDeviceIndexInvalid;
            if (sys) {
            vr::VREvent_t ev;
            while (sys->PollNextEvent(&ev, sizeof(ev))) if (ev.eventType == vr::VREvent_Quit) { rl("SteamVR quit"); sys->AcknowledgeQuit_Exiting(); }
            vr::VRCompositor()->WaitGetPoses(poses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
            /* the gun hand: right-hand controller, else any controller */
            ci = sys->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
            if (ci == vr::k_unTrackedDeviceIndexInvalid || !poses[ci].bPoseIsValid)
                for (vr::TrackedDeviceIndex_t i = 1; i < vr::k_unMaxTrackedDeviceCount; i++)
                    if (sys->GetTrackedDeviceClass(i) == vr::TrackedDeviceClass_Controller && poses[i].bPoseIsValid) { ci = i; break; }
            }`);
rep(`            if (acts.ok) {`, `            if (acts.ok || xr_up) {`);
rep(`                VPIX_Acts a; acts_read(a); vr_voice_mute = a.voice_mute;`,
    `                VPIX_Acts a; if (xr_up) xr_acts_read(a); else acts_read(a); vr_voice_mute = a.voice_mute;`);
rep(`            const vr::TrackedDevicePose_t &hp = poses[vr::k_unTrackedDevice_Hmd];
            if (hp.bPoseIsValid) {`.replace('k_unTrackedDevice_Hmd', 'k_unTrackedDeviceIndex_Hmd'),
    `            if (xr_up) {
                if (xp.head_ok) {
                    head_now = xp.head; head_xy_now[0] = head_now.p[0]; head_xy_now[1] = head_now.p[1];
                    eye_pose[0] = xp.eye[0]; eye_pose[1] = xp.eye[1];
                }
            } else {
            const vr::TrackedDevicePose_t &hp = poses[vr::k_unTrackedDeviceIndex_Hmd];
            if (hp.bPoseIsValid) {`);
rep(`                    eye_pose[e] = vr_to_pose(mul34(hp.mDeviceToAbsoluteTracking, sys->GetEyeToHeadTransform((vr::EVREye)e)));
            }
`, `                    eye_pose[e] = vr_to_pose(mul34(hp.mDeviceToAbsoluteTracking, sys->GetEyeToHeadTransform((vr::EVREye)e)));
            }
            }
`);
rep(`            if (ci != vr::k_unTrackedDeviceIndexInvalid && poses[ci].bPoseIsValid) input.hand = vr_to_pose(poses[ci].mDeviceToAbsoluteTracking);`,
    `            if (xr_up) { if (xp.hand_ok[0]) input.hand = xp.hand[0]; else if (xp.hand_ok[1]) input.hand = xp.hand[1]; }   /* gun hand: right, else left */
            else if (ci != vr::k_unTrackedDeviceIndexInvalid && poses[ci].bPoseIsValid) input.hand = vr_to_pose(poses[ci].mDeviceToAbsoluteTracking);`);
rep(`            {   /* the left controller as a second hand (VPIX_TWO_HANDS, Boxing: either hand punches) */
                vr::TrackedDeviceIndex_t li`,
    `            if (xr_up) input.hand2 = (xp.hand_ok[0] && xp.hand_ok[1]) ? xp.hand[1] : VPIX_Pose();
            else {   /* the left controller as a second hand (VPIX_TWO_HANDS, Boxing: either hand punches) */
                vr::TrackedDeviceIndex_t li`);

// 5. rendering: projection from the OpenXR view, copy into the swapchain, end the frame
rep(`        const VPIX_Frame &vf = (have && sys) ? glove_apply`, `        const VPIX_Frame &vf = (have && (sys || xr_up)) ? glove_apply`);
rep(`        if (sys) {
            for (int e = 0; e < 2; e++) {`, `        if (sys || xr_up) {
            for (int e = 0; e < 2; e++) {`);
rep(`                float P[16];
                vr::HmdMatrix44_t pm = sys->GetProjectionMatrix((vr::EVREye)e, z_near, 400000.f);
                for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) P[j * 4 + i] = pm.m[i][j];
                rev_proj(P);                                   /* VPIX_DEPTH_FIX: z row -> reversed infinite */`,
    `                float P[16];
                if (xr_up) xr_proj(e, P);
                else {
                vr::HmdMatrix44_t pm = sys->GetProjectionMatrix((vr::EVREye)e, z_near, 400000.f);
                for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) P[j * 4 + i] = pm.m[i][j];
                rev_proj(P);                                   /* VPIX_DEPTH_FIX: z row -> reversed infinite */
                }`);
rep(`                vr::Texture_t tx = {(void *)(uintptr_t)eyes[e].tex, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
                vr::EVRCompositorError ce = vr::VRCompositor()->Submit((vr::EVREye)e, &tx);
                if (ce != vr::VRCompositorError_None && vr_frames < 5) rl("Submit eye %d error %d", e, (int)ce);
            }`,
    `                if (xr_up) { xr_submit(e, eyes[e].fbo); continue; }
                vr::Texture_t tx = {(void *)(uintptr_t)eyes[e].tex, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
                vr::EVRCompositorError ce = vr::VRCompositor()->Submit((vr::EVREye)e, &tx);
                if (ce != vr::VRCompositorError_None && vr_frames < 5) rl("Submit eye %d error %d", e, (int)ce);
            }
            if (xr_up) xr_end();`);
rep(`        if (!swi && !sys) Sleep(10);`, `        if (!swi && !sys && !xr_up) Sleep(10);`);
rep(`    if (sys) vr::VR_Shutdown();`, `    if (sys) vr::VR_Shutdown();
    if (xr_up) xr_shutdown();`);

if (crlf) s = s.replace(/\n/g, '\r\n');
fs.writeFileSync(out, s);
console.log('OpenXR hooks applied ->', out);
