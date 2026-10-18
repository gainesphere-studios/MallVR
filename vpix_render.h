/* Virtuality SU2000 HLE — interface between the PIX hooks (vpix_hle.cpp) and the renderer. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>

struct VPIX_Instance {
    uint32_t handle;          /* model handle returned by our PIX_LoadModel */
    float R[3][3], T[3], S[3];
    uint32_t pal[16];         /* model palette, 0xRRGGBB */
    uint32_t palw[16];        /* the same entries as raw dwords: type 0x78 polygons take their texture number here */
    uint8_t own_hand = 0;     /* VPIX_HOLD_VIEW games: the player's own gun (1) / gun forearm (2), moved by the controller while the game camera is frozen;
                                 3 = rest of the own body, 4 = own shot: all of them hidden while the view is held */
    int32_t prio = 0;         /* model slot +0x54: the card's draw priority */
    uint16_t slot = 0;        /* model slot (32 * window + index) */
    uint8_t scared = 0;       /* Pac-Man ghost eyes / mouth while the ghost is blue or flashing: drawn with the scared-face textures */
    uint8_t near_bias = 0;    /* VPIX_NEAR_MODELS: drawn a few decal layers toward the eye (Zone's doors in their wall opening) */
    bool ymove = false; float ymove_from = 0, ymove_to = 0;   /* VPIX_MOVE_Y: model-space y 'from' drawn at 'to' (Zone pill sign) */
    float ymin = -1e30f, ymax = 1e30f;      /* VPIX_CLAMP_Y models: model-space y below this is moved up to it (trims a room's end) */
    uint8_t seal = 0;         /* VPIX_SEAL models: outline pass over base polygons (fills T-junction cracks) */
    uint8_t poly_select = 0;  /* VPIX_POLY_SELECT models: draw a polygon with select bits only if they meet 'select' (PIXMODEL +0x50) */
    uint32_t select = 0;
    uint8_t one_sided = 0;    /* VPIX_ONE_SIDED models: type 0x22 polygons back-face culled (Zone's VR logo); 2 = every polygon; 3 = every polygon's back side drawn flat grey */
    uint8_t on_top = 0;       /* VPIX_ON_TOP: drawn last, depth test off (Missile Command skip sign over the gun housing) */
    uint8_t headlock = 0;     /* PEARL sell messages over the fade screen (VPIX_PANEL_HEADLOCK): drawn relative to the game camera = locked to the head */
};

/* A tracked pose in the game's tracker space: X right, Y forward, Z up, millimetres.
   R rows = local axes (right, forward, up) in tracker space — same layout as the PIX matrices. */
struct VPIX_Pose {
    bool valid = false;
    float R[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float p[3] = {0, 0, 0};
};

/* One 2D bitmap object (PIXBITMAP) on screen. Screen space is VPIX_SCREEN_W x _H, origin top-left. */
struct VPIX_Blit {
    uint32_t handle;          /* our PIX_LoadBitmap handle */
    int sx, sy, sw, sh;       /* source rectangle in the bitmap */
    int x, y;                 /* screen position */
    float dw, dh;             /* drawn size */
    int key;                  /* flag 4: transparent colour, 12-bit 0BGR like the texels; -1 = opaque */
    float alpha;              /* flag 8: translucency (+0x2A / 255), else 1 */
    bool world;               /* flag 0x20: a 3D sprite at the model slot's position, dw x dh world units */
    float pos[3];             /* world position (model slot T) */
    float pivot[2];           /* +0x1E/+0x22: pivot within the sprite (from its left / bottom) */
    float tint[3] = {1, 1, 1}; /* VPIX_GLYPH_COLOUR: flag 0x800 colour at +0x2C for 1 bpp glyphs (Ghost Train letters + shadows) */
    int32_t prio = 0;         /* model slot +0x54: the card's draw priority (higher = in front); VPIX_BLIT_PRIORITY games use it */
    bool backdrop = false;    /* VPIX_BLIT_PRIORITY: priority below every 3D model in the view -> drawn behind the 3D scene */
    bool mid = false;         /* PEARL: a 3D model has a higher priority -> drawn between the models (those above it in front) */
};

struct VPIX_Frame {
    unsigned long frame = 0;
    float cam_R[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, cam_T[3] = {0, 0, 0};
    float fov_deg = 70.f;
    uint8_t clear[3] = {0, 0, 0};
    bool in_head = false;     /* game camera is the player's own head (tracker-driven) */
    VPIX_Pose head_fed;       /* head pose the game was given for this frame */
    float player_height = 0;  /* mm, calibrated by the game at the "focus visette" step (0 = not yet) */
    float crouch_mm = 0;      /* VPIX_CROUCH games: how far head_fed was lowered by the crouch button (the eye drops the same) */
    int game_mode = -1;       /* game_mode of local slot 0 when the game has the symbol (DAC: 0 play, 3 GAME OVER, 7 initials, 2 attract), else -1 */
    bool hold = false;        /* renderer only (VPIX_HOLD_VIEW): game camera frozen, head_fed = the pose that built it */
    std::vector<VPIX_Instance> inst;
    std::vector<VPIX_Blit> blits;
};

/* Controller/headset state, written by the render thread (OpenVR or desktop fallback),
   read by the HLE thread when the game asks for tracker records or the joystick. */
enum { VPIX_BTN_TRIGGER = 1, VPIX_BTN_WALK = 2, VPIX_BTN_3 = 4, VPIX_BTN_4 = 8 };
struct VPIX_Input {
    VPIX_Pose head, hand;     /* tracker space, after the player's yaw offset */
    VPIX_Pose hand2;          /* the other (left) controller, invalid when there is none; used by VPIX_TWO_HANDS (Boxing) */
    uint32_t buttons = 0;     /* CTRLJOYDATA button bits */
    int coins = 0;            /* coin presses since start */
    bool start = false;       /* front-panel start button */
    bool vr = false;          /* a headset is driving the poses */
    float gun_pitch = -20.f;  /* degrees added to the gun's pitch (Vive wand vs pod joystick); left pad up/down adjusts */
    float hand_zr = 0.f;      /* Boxing: mm the RIGHT hand is fed lower (left pad up/down adjusts, hand_zr_BOX in the settings) */
    float hand_reach = 1.f;   /* this game's multiplier on the hand's distance from the head; left pad left/right adjusts */
    float loco[2] = {0, 0};   /* stick movement (back/strafe), mm in tracker space, added to head and hand positions */
    float walk_yaw = 0;       /* VPIX_WALK_TURN games (Pac-Man): radians the fed head is turned while back/strafe is held */
    float joy[2] = {0, 0};    /* VPIX_JOYAXES games: the pod joystick's x / y (-1..1) from the thumbsticks */
    float crouch_mm = 0;      /* VPIX_CROUCH games: crouch button held -> head and hand fed this many mm lower */
};

bool VPIX_Render_AddModel(uint32_t handle, const uint8_t *data, size_t len);
/* model file name, as loaded by PIX_LoadModel (used by dev-only tooling; no-op otherwise) */
void VPIX_Render_ModelName(uint32_t handle, const char *file);
bool VPIX_Render_AddTexture(uint32_t handle, const uint8_t *data, size_t len);     /* .TEX file */
bool VPIX_Render_AddBitmap(uint32_t handle, const uint8_t *data, size_t len);      /* .VST (TGA) file */
/* PIX_SetTexture: texture number n (polygon flags low bits) -> handle + planar projection.
   flags 0x20000 = XY plane, 0x40000 = XZ, 0x80000 = YZ; u = (a - o) * su, v = (b - o) * sv in texels.
   tflags/mask = PIXTEXTURE +0x20 flags and +0x24 maskcolor (PIXCOL red, green, blue, flags). */
void VPIX_Render_SetTexture(uint32_t n, uint32_t flags, uint32_t handle, const float o[3], float su, float sv, uint32_t tflags, uint32_t mask);
/* PIX_SetMaterial (VPIX_MATERIALS games): material n's texture number (PIXMATERIAL +0xC), -1 = none.
   Type 0x68 polygons then take their texture from material (low bits) instead of using them directly. */
void VPIX_Render_SetMaterial(uint32_t n, int tex, float alpha = 1.f);   /* alpha < 1: translucent (type 0x68 polygons blended) */
/* PEARL (older PIX): material n's colour (0xRRGGBB); from the first call on, type 0x28 polygons are material colours
   (low bits = material, blended when its opacity < 1) instead of model palette entries */
void VPIX_Render_SetMaterialColour(uint32_t n, uint32_t rgb);
void VPIX_Render_Submit(const VPIX_Frame &f);
void VPIX_Render_Start(void);
void VPIX_Render_Stop(void);
void VPIX_Input_Get(VPIX_Input &out);
/* controller rumble (SteamVR action /actions/main/out/haptic): hand 0 = right, 1 = left, 2 = both; queued, played by the render thread */
void VPIX_Haptic(int hand, float seconds, float amplitude);
/* FLIC layer: one video frame (RGBA, black = transparent) at screen x/y (352x288 space), or off */
void VPIX_Render_SetFlic(const uint32_t *rgba, int w, int h, int x, int y, bool on);
/* lowest world z of a model's vertices for an instance transform (Boxing KO lift); 1e9 if the model is unknown */
float VPIX_Render_ModelMinZ(uint32_t handle, const float R[3][3], const float S[3], const float T[3]);
