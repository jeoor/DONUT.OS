/*
 * DONUT.OS for M5Stack Cardputer / Cardputer-ADV
 * Final polished build: unified overlays, readable boot log, system clock, RGB+ style, stopwatch/timer tool.
 *
 * References:
 * - a1k0n donut math: https://www.a1k0n.net/2011/07/20/donut-math.html
 * - a1k0n obfuscated donut: https://www.a1k0n.net/2006/09/15/obfuscated-c-donut.html
 * - a1k0n enhanced/header-bg: https://www.a1k0n.net/2006/09/20/obfuscated-c-donut-2.html
 * - a1k0n no-math donut: https://www.a1k0n.net/2021/01/13/optimizing-donut.html
 * - donut.js: https://www.a1k0n.net/js/donut.js
 * - TheDonutProject: https://github.com/EvanZhouDev/TheDonutProject
 * - Scratchapixel rasterization: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-practical-implementation.html
 * - Scratchapixel z-buffer: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation.html
 * - TinyRenderer: https://github.com/ssloy/tinyrenderer
 * - ESP32-S3 arcade 3D: https://github.com/davidmonterocrespo24/esp32s3-arcade-3d
 * - ESP32 3D engine: https://github.com/andresragot/esp32_3d_engine
 * - TGX: https://github.com/vindar/tgx
 * - TGX docs: https://vindar.github.io/tgx/html/index.html
 * - M5Cardputer docs: https://docs.m5stack.com/en/core/Cardputer
 * - M5Cardputer-ADV docs: https://docs.m5stack.com/en/core/Cardputer-Adv
 * - M5Cardputer Keyboard API: https://docs.m5stack.com/en/arduino/m5cardputer/keyboard
 * - M5Canvas docs: https://docs.m5stack.com/en/arduino/m5gfx/m5gfx_canvas
 * - M5GFX docs: https://docs.m5stack.com/en/arduino/m5gfx/m5gfx_functions
 * - LovyanGFX: https://github.com/lovyan03/LovyanGFX
 */

#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5GFX.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <Preferences.h>

// -----------------------------------------------------------------------------
// Constants

static constexpr int SCREEN_W = 240;
static constexpr int SCREEN_H = 135;
static constexpr int TOP_BAR_H = 13;
static constexpr int RENDER_START = SCREEN_W * TOP_BAR_H;
static constexpr int RENDER_H = SCREEN_H - TOP_BAR_H;
static constexpr int RENDER_PIXELS = SCREEN_W * RENDER_H;
static constexpr int SCREEN_PIXELS = SCREEN_W * SCREEN_H;

static constexpr int BG_W = 120;
static constexpr int BG_H = 61;

static constexpr float PI_F = 3.14159265358979323846f;
static constexpr float TAU_F = PI_F * 2.0f;
static constexpr float TORUS_R1 = 1.0f;
static constexpr float TORUS_R2 = 2.0f;
static constexpr float CAMERA_Z = 5.0f;
static constexpr float Z_SCALE = 120000.0f;
static constexpr float TRI_AREA_MIN = 0.40f;
static constexpr float BACKFACE_EPS = 0.06f;

static constexpr int STD_THETA = 18;
static constexpr int STD_PHI = 48;
static constexpr int HQ_THETA = 24;
static constexpr int HQ_PHI = 64;

static constexpr uint8_t LUM_DARK_MIN = 10;
static constexpr uint8_t LUM_BRIGHT_MAX = 210;
static constexpr uint32_t REPEAT_MS = 80;

static constexpr uint8_t FPS_TARGETS[] = {20, 25, 30, 0};
static constexpr size_t FPS_TARGET_COUNT = sizeof(FPS_TARGETS) / sizeof(FPS_TARGETS[0]);

static constexpr float VIEW_STEP = 2.0f;
static constexpr float VIEW_LIMIT_X = 48.0f;
static constexpr float VIEW_LIMIT_Y = 28.0f;
static constexpr uint32_t IDLE_ORBIT_MS = 8000;
static constexpr uint16_t TOAST_MS = 900;
static constexpr uint16_t BOOT_OVERLAY_MS = 5200;
static constexpr uint16_t BOOT_STAGE1_MS = 1200;
static constexpr uint16_t BOOT_STAGE2_MS = 2600;
// M5Canvas direct buffer writes need the Cardputer LCD byte order.
// bufFromDraw()/drawFromBuf() are the only conversion gates; pushSprite keeps setSwapBytes(false).
static constexpr bool CANVAS_BUFFER_SWAP_BYTES = true;
static constexpr uint32_t SETTINGS_SAVE_DELAY_MS = 1800UL;
static constexpr uint32_t SETTINGS_VERSION = 9UL;
static constexpr uint32_t FACTORY_RESET_HOLD_MS = 2000UL;
static constexpr uint32_t TIMER_DEFAULT_MS = 5UL * 60UL * 1000UL;
static constexpr uint32_t TIMER_STEP_MS = 60UL * 1000UL;
static constexpr uint32_t TIMER_FLASH_MS = 3000UL;

static constexpr uint8_t GLOW_SRC_MIN = 120;
static constexpr uint8_t GLOW_SRC_SCALE = 3;
static constexpr uint8_t GLOW_MAX = 220;
static constexpr uint8_t GLOW_BLEND_DIV_BG = 160;
static constexpr uint8_t GLOW_BLEND_DIV_DONUT = 220;
static constexpr int GLOW_RADIUS = 2;

static constexpr uint8_t BG_REBUILD_INTERVAL_CALM = 1;
static constexpr uint8_t BG_REBUILD_INTERVAL_ACTIVE = 1;
static constexpr int BG_FP_SHIFT = 8;
static constexpr int BG_FP_ONE = 1 << BG_FP_SHIFT;
static constexpr float BG_X_WOBBLE_AMPLITUDE = 0.48f;
static constexpr float BG_Y_SCROLL_UNITS = 4.0f;
static constexpr float BG_DEPTH_X_SCALE_SHALLOW = 0.82f;
static constexpr float BG_DEPTH_X_SCALE_DEEP = 1.12f;
static constexpr int FLOW_LINE_SPACING = 42;
static constexpr int FLOW_LINE_DX = 70;
static constexpr int FLOW_LINE_Y1 = TOP_BAR_H + 18;

#define ENABLE_HOT_PIXEL_SUPPRESS 0
#define ENABLE_SMOOTH_SHADE 0
#define USE_DEBUG_CONSERVATIVE_RASTER 0
#define ENABLE_BACKFACE_CULL 1
#define RASTER_CRACK_FIX_PAD 0

// -----------------------------------------------------------------------------
// Types and settings

struct RGB8 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Theme {
    const char* name;
    RGB8 rgb;
};

static constexpr Theme THEMES[] = {
    {"CYAN",    {  0, 180, 255}},
    {"BLUE",    {  0,  85, 255}},
    {"VIOLET",  {115,  45, 255}},
    {"MAGENTA", {255,  40, 180}},
    {"GREEN",   {  0, 240, 115}},
    {"AMBER",   {255, 145,  20}},
    {"WHITE",   {235, 245, 255}},
};
static constexpr int THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

struct RuntimeSettings {
    uint8_t themeIndex = 0;
    uint8_t backgroundMode = 1;
    bool rgbPlusEnabled = false;
    uint8_t floatingTextMode = 0;  // 0=OFF, 1=OS, 2=SIGN, 3=TECH
    bool showKeyHints = false;
    bool showSystemPanel = false;
    bool showStopwatchPanel = false;
    bool glowEnabled = false;
    bool highQuality = false;
    bool paused = false;
    bool idleOrbitEnabled = true;
    bool bgFxEnabled = true;
    uint8_t frameTargetIndex = 2;
    uint8_t motionProfile = 1;
    uint8_t bgDepthLevel = 1;
    float rotationSpeed = 1.0f;
    float donutScale = 94.0f;
    uint8_t brightness = 105;
};

struct DrawPalette {
    uint16_t topBar;
    uint16_t topLine;
    uint16_t text;
    uint16_t textDim;
    uint16_t hudBg;
    uint16_t hint;
    uint16_t flow;
};

struct BufferPalette {
    uint16_t plainBg;
    uint16_t checkerTone[5];
    uint16_t flow;
    uint16_t gridLine;
    uint16_t gridLineNear;
    uint16_t fxDim;
    uint16_t fxBright;
    uint16_t donut[256];
};

struct MeshVertex {
    float x;
    float y;
    uint16_t z;  // Same 0..65535 domain as zBuf.
    float shade;
    float nz;
};

struct EdgeValue {
    float x;
    float z;     // Interpolated depth; kept float between vertices.
    float shade;
};

enum ActionId {
    ACT_THEME,
    ACT_BACKGROUND,
    ACT_RGB_PLUS,
    ACT_FLOAT,
    ACT_KEYS,
    ACT_GLOW,
    ACT_QUALITY,
    ACT_PAUSE,
    ACT_FPS,
    ACT_SPEED_UP,
    ACT_SPEED_DOWN,
    ACT_SCALE_UP,
    ACT_SCALE_DOWN,
    ACT_BRIGHT_DOWN,
    ACT_BRIGHT_UP,
    ACT_RESET,
    ACT_RESTART,
    ACT_VIEW_UP,
    ACT_VIEW_LEFT,
    ACT_VIEW_DOWN,
    ACT_VIEW_RIGHT,
    ACT_VIEW_RESET,
    ACT_BG_DEPTH_DOWN,
    ACT_BG_DEPTH_UP,
    ACT_MOTION,
    ACT_IDLE_ORBIT,
    ACT_PANEL,
    ACT_STOPWATCH,
    ACT_STOPWATCH_RUN,
    ACT_STOPWATCH_RESET,
    ACT_COUNT
};

struct KeySpec {
    char a;
    char b;
    char c;
};

struct ToastState {
    char text[24];
    uint32_t startMs;
    uint16_t durationMs;
    bool active;
};

struct StopwatchState {
    bool running;
    uint32_t baseMs;
    uint32_t startMs;
    uint8_t mode;          // 0 = stopwatch, 1 = timer
    uint32_t timerTargetMs;
    bool timerDone;
};

static constexpr KeySpec KEY_SPECS[ACT_COUNT] = {
    {' ', 0, 0},
    {'b', 'B', 0},
    {'c', 'C', 0},
    {'t', 'T', 0},
    {'k', 'K', 0},
    {'g', 'G', 0},
    {'h', 'H', 0},
    {'p', 'P', 0},
    {'f', 'F', 0},
    {';', ':', 0},
    {'.', '>', 0},
    {'=', '+', 0},
    {'-', '_', 0},
    {'[', '{', 0},
    {']', '}', 0},
    {'r', 'R', 0},
    {'`', '~', 0},
    {'w', 'W', 0},
    {'a', 'A', 0},
    {'s', 'S', 0},
    {'d', 'D', 0},
    {'0', ')', 0},
    {'q', 'Q', 0},
    {'e', 'E', 0},
    {'m', 'M', 0},
    {'o', 'O', 0},
    {'y', 'Y', 0},
    {'n', 'N', 0},
    {'x', 'X', 0},
    {'z', 'Z', 0},
};

// -----------------------------------------------------------------------------
// Buffers and runtime state

static M5Canvas canvas(&M5Cardputer.Display);
static uint16_t* canvasBuf = nullptr;

static uint16_t zBuf[SCREEN_PIXELS];
static uint8_t shadeBuf[SCREEN_PIXELS];
static uint8_t glowBuf[SCREEN_PIXELS];
static uint16_t bgCache[SCREEN_PIXELS];

static MeshVertex rowFirst[HQ_PHI];
static MeshVertex rowA[HQ_PHI];
static MeshVertex rowB[HQ_PHI];

static float sinThetaStd[STD_THETA];
static float cosThetaStd[STD_THETA];
static float sinPhiStd[STD_PHI];
static float cosPhiStd[STD_PHI];
static float sinThetaHq[HQ_THETA];
static float cosThetaHq[HQ_THETA];
static float sinPhiHq[HQ_PHI];
static float cosPhiHq[HQ_PHI];

static RuntimeSettings settings;
static DrawPalette drawPalette;
static BufferPalette bufPalette;
static ToastState toastState;
static StopwatchState stopwatchState;
static Preferences prefs;
static bool prefsReady = false;
static bool prefsDirty = false;
static uint32_t prefsDirtyMs = 0;
static uint8_t systemPanelPage = 0;
static constexpr uint8_t SYSTEM_PANEL_PAGE_COUNT = 3;
static uint32_t timerFlashUntilMs = 0;
static uint32_t factoryResetHoldMs = 0;
static bool factoryResetTriggered = false;

static bool paletteDirty = true;
static bool bgCacheDirty = true;
static bool actionWasDown[ACT_COUNT];
static uint32_t actionLastRepeat[ACT_COUNT];

static int dirtyMinX = SCREEN_W;
static int dirtyMinY = SCREEN_H;
static int dirtyMaxX = -1;
static int dirtyMaxY = -1;

static constexpr float INITIAL_A = 0.72f;
static constexpr float INITIAL_B = 0.18f;
static constexpr float ROT_STEP_A = 0.040f;
static constexpr float ROT_STEP_B = 0.022f;

static float rotCA = 1.0f;
static float rotSA = 0.0f;
static float rotCB = 1.0f;
static float rotSB = 0.0f;
static float bgScroll = 0.0f;
static uint32_t bgMotionFx = 0;
static uint32_t bgXPhaseFx = 0;
static uint32_t orbitPhaseFx = 0;
static uint8_t orbitBlendQ8 = 0;
static float manualOffsetX = 0.0f;
static float manualOffsetY = 0.0f;
static float orbitOffsetX = 0.0f;
static float orbitOffsetY = 0.0f;
static float orbitScaleAdd = 0.0f;
static uint8_t bgAnimCounter = 0;
static uint8_t helpPage = 0;
static constexpr uint8_t HELP_PAGE_COUNT = 3;
static uint32_t frameNumber = 0;
static uint16_t bgToneLut[BG_H][5];
static uint32_t startMs = 0;
static uint32_t lastInputMs = 0;
static uint32_t fpsLastMs = 0;
static uint16_t fpsFrames = 0;
static float measuredFps = 0.0f;

static uint32_t bgBuildUs = 0;
static uint32_t bgCopyUs = 0;
static uint32_t bgFxUs = 0;
static uint32_t donutTimeUs = 0;
static uint32_t composeTimeUs = 0;
static uint32_t pushTimeUs = 0;
static uint32_t glowUs = 0;
static uint32_t uiUs = 0;
static uint32_t inputUs = 0;
static uint32_t motionUs = 0;
static uint32_t primitiveCount = 0;
static uint32_t culledCount = 0;
static uint32_t rasterizedCount = 0;
static uint32_t fillPixelCount = 0;

// -----------------------------------------------------------------------------
// Color and math helpers

static inline uint8_t clampU8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static inline float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline float clampFloat(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline uint16_t swap16(uint16_t v) {
    return (uint16_t)((v << 8) | (v >> 8));
}

static inline uint16_t draw565(uint8_t r, uint8_t g, uint8_t b) {
    return M5Cardputer.Display.color565(r, g, b);
}

// M5GFX returns colors in draw-format.  Direct canvas-buffer writes use one
// compile-time byte-order gate so UI drawing and framebuffer writes stay
// visually consistent on Cardputer.
static inline uint16_t bufFromDraw(uint16_t c) {
    return CANVAS_BUFFER_SWAP_BYTES ? swap16(c) : c;
}

static inline uint16_t drawFromBuf(uint16_t c) {
    return CANVAS_BUFFER_SWAP_BYTES ? swap16(c) : c;
}

static inline RGB8 scaleRgb(RGB8 c, uint8_t num, uint8_t den) {
    return {
        (uint8_t)((uint16_t)c.r * num / den),
        (uint8_t)((uint16_t)c.g * num / den),
        (uint8_t)((uint16_t)c.b * num / den)
    };
}

static inline RGB8 addRgb(RGB8 a, RGB8 b) {
    return {
        clampU8((int)a.r + b.r),
        clampU8((int)a.g + b.g),
        clampU8((int)a.b + b.b)
    };
}

static inline uint8_t expand5To8(uint8_t v) {
    return (uint8_t)((v << 3) | (v >> 2));
}

static inline uint8_t expand6To8(uint8_t v) {
    return (uint8_t)((v << 2) | (v >> 4));
}

static inline RGB8 rgb565SwapLook(RGB8 c) {
    uint16_t raw = (uint16_t)(((uint16_t)(c.r >> 3) << 11)
                            | ((uint16_t)(c.g >> 2) << 5)
                            |  (uint16_t)(c.b >> 3));
    uint16_t sw = swap16(raw);
    return {
        expand5To8((uint8_t)((sw >> 11) & 0x1F)),
        expand6To8((uint8_t)((sw >> 5) & 0x3F)),
        expand5To8((uint8_t)(sw & 0x1F))
    };
}

static inline RGB8 rgbBoost(RGB8 c, int r, int g, int b) {
    return { clampU8((int)c.r + r), clampU8((int)c.g + g), clampU8((int)c.b + b) };
}

static inline uint16_t addGlowToBuf(uint16_t baseBuf, RGB8 theme, uint8_t intensity, bool onDonut) {
    uint16_t base = drawFromBuf(baseBuf);
    int r = (int)(((base >> 11) & 0x1F) << 3);
    int g = (int)(((base >> 5) & 0x3F) << 2);
    int b = (int)((base & 0x1F) << 3);
    int div = onDonut ? GLOW_BLEND_DIV_DONUT : GLOW_BLEND_DIV_BG;
    r = clampU8(r + ((int)theme.r * intensity) / div);
    g = clampU8(g + ((int)theme.g * intensity) / div);
    b = clampU8(b + ((int)theme.b * intensity) / div);
    return bufFromDraw(draw565(r, g, b));
}

static inline uint16_t depthToU16(float invZ) {
    float v = invZ * Z_SCALE;
    if (v <= 0.0f) return 0;
    if (v >= 65535.0f) return 65535u;
    return (uint16_t)v;
}

static void clampPersistentSettings() {
    if (settings.themeIndex >= THEME_COUNT) settings.themeIndex = 0;
    settings.backgroundMode &= 3;
    if (settings.frameTargetIndex >= FPS_TARGET_COUNT) settings.frameTargetIndex = 2;
    if (settings.motionProfile > 2) settings.motionProfile = 1;
    if (settings.bgDepthLevel > 2) settings.bgDepthLevel = 1;
    if (settings.floatingTextMode > 3) settings.floatingTextMode = 0;
    settings.rotationSpeed = clampFloat(settings.rotationSpeed, 0.20f, 4.0f);
    settings.donutScale = clampFloat(settings.donutScale, 70.0f, 112.0f);
    if (settings.brightness < 1) settings.brightness = 1;
}

static void markSettingsDirty() {
    if (!prefsReady) return;
    prefsDirty = true;
    prefsDirtyMs = millis();
}

static void loadPersistentSettings() {
    prefsReady = prefs.begin("donutos", false);
    if (!prefsReady) return;

    uint32_t savedVersion = prefs.getUInt("version", 0);
    if (savedVersion != SETTINGS_VERSION) {
        prefs.clear();
        prefs.putUInt("version", SETTINGS_VERSION);
        settings = RuntimeSettings();
        clampPersistentSettings();
        prefsDirty = true;      // Save a full clean schema after boot settles.
        prefsDirtyMs = millis();
        return;
    }

    settings.themeIndex = prefs.getUChar("theme", settings.themeIndex);
    settings.backgroundMode = prefs.getUChar("bg", settings.backgroundMode);
    settings.rgbPlusEnabled = prefs.getBool("rgbplus", settings.rgbPlusEnabled);
    settings.floatingTextMode = prefs.getUChar("txtmode", settings.floatingTextMode);
    settings.glowEnabled = prefs.getBool("glow", settings.glowEnabled);
    settings.highQuality = prefs.getBool("hq", settings.highQuality);
    settings.frameTargetIndex = prefs.getUChar("fps", settings.frameTargetIndex);
    settings.motionProfile = prefs.getUChar("motion", settings.motionProfile);
    settings.bgDepthLevel = prefs.getUChar("depth", settings.bgDepthLevel);
    settings.idleOrbitEnabled = prefs.getBool("orbit", settings.idleOrbitEnabled);
    settings.rotationSpeed = prefs.getFloat("speed", settings.rotationSpeed);
    settings.donutScale = prefs.getFloat("scale", settings.donutScale);
    settings.brightness = prefs.getUChar("bright", settings.brightness);
    clampPersistentSettings();
    settings.bgFxEnabled = settings.motionProfile != 0;
}

static void savePersistentSettingsNow() {
    if (!prefsReady) return;
    prefs.putUInt("version", SETTINGS_VERSION);
    prefs.putUChar("theme", settings.themeIndex);
    prefs.putUChar("bg", settings.backgroundMode);
    prefs.putBool("rgbplus", settings.rgbPlusEnabled);
    prefs.putUChar("txtmode", settings.floatingTextMode);
    prefs.putBool("glow", settings.glowEnabled);
    prefs.putBool("hq", settings.highQuality);
    prefs.putUChar("fps", settings.frameTargetIndex);
    prefs.putUChar("motion", settings.motionProfile);
    prefs.putUChar("depth", settings.bgDepthLevel);
    prefs.putBool("orbit", settings.idleOrbitEnabled);
    prefs.putFloat("speed", settings.rotationSpeed);
    prefs.putFloat("scale", settings.donutScale);
    prefs.putUChar("bright", settings.brightness);
    prefsDirty = false;
}

static void servicePersistentSettings() {
    if (!prefsDirty) return;
    if ((uint32_t)(millis() - prefsDirtyMs) >= SETTINGS_SAVE_DELAY_MS) savePersistentSettingsNow();
}

// -----------------------------------------------------------------------------
// Palette

static void rebuildPalettes() {
    const RGB8 theme = THEMES[settings.themeIndex % THEME_COUNT].rgb;
    const bool rgbPlus = settings.rgbPlusEnabled;
    RGB8 tint = scaleRgb(theme, 1, 48);
    RGB8 bg0 = addRgb({1, 3, 8}, tint);
    if (rgbPlus) bg0 = rgbBoost(rgb565SwapLook(bg0), 5, 2, 10);
    bufPalette.plainBg = bufFromDraw(draw565(bg0.r, bg0.g, bg0.b));

    // a1k0n-style filled perspective checkerboard, with a correct depth fade.
    // The checker still comes from the author's inverse-projection + floor/XOR
    // model, but the color is now theme-tinted and row-LUT based:
    //   near field  = slightly brighter, stronger checker contrast
    //   far field   = slightly darker, lower checker contrast
    // This keeps the nice front/back gradient without using unrelated near/far hues.
    uint8_t baseR = clampU8(20 + theme.r / 9);
    uint8_t baseG = clampU8(30 + theme.g / 9);
    uint8_t baseB = clampU8(42 + theme.b / 8);

    // The old far-field fade reached ~0.76 brightness over the upper rows.
    // On the Cardputer LCD that looked like a black veil over the top half.
    // Keep the depth cue, but raise the far floor and reduce checker contrast
    // smoothly toward the horizon.  This is still a projection-tied gradient,
    // not a screen-space black overlay.
    for (int by = 0; by < BG_H; ++by) {
        float sy = (float)(by * 2) + 0.5f;
        float fragY = (float)(RENDER_H - 1) - sy;
        float uvy = (fragY - (float)RENDER_H) * (1.0f / (float)RENDER_H);
        float denom = uvy - 0.3f;
        float worldY = fabsf(2.0f / denom);
        float farT = (worldY - 1.55f) * (1.0f / (6.50f - 1.55f));
        if (farT < 0.0f) farT = 0.0f;
        if (farT > 1.0f) farT = 1.0f;
        farT = farT * farT * (3.0f - 2.0f * farT);

        float depthBrightness = 1.08f + (0.91f - 1.08f) * farT;
        float checkerContrast = 0.20f + (0.10f - 0.20f) * farT;

        for (int mix = 0; mix < 5; ++mix) {
            float factor = depthBrightness * (1.0f + checkerContrast * (float)mix);
            int r = (int)((float)baseR * factor + 0.5f);
            int g = (int)((float)baseG * factor + 0.5f);
            int b = (int)((float)baseB * factor + 0.5f);
            RGB8 tone = {clampU8(r), clampU8(g), clampU8(b)};
            if (rgbPlus) tone = rgbBoost(rgb565SwapLook(tone), mix * 7, mix * 3, (4 - mix) * 4);
            bgToneLut[by][mix] = bufFromDraw(draw565(tone.r, tone.g, tone.b));
        }
    }

    for (int mix = 0; mix < 5; ++mix) {
        bufPalette.checkerTone[mix] = bgToneLut[BG_H - 1][mix];
    }
    drawPalette.topBar = draw565(theme.r / 36 + 2, theme.g / 36 + 4, theme.b / 36 + 8);
    drawPalette.topLine = draw565(clampU8(theme.r / 3 + 18),
                                  clampU8(theme.g / 3 + 24),
                                  clampU8(theme.b / 3 + 34));

    // Theme-tinted UI text with a fixed brightness floor.
    // Do not use raw theme color directly: CYAN/BLUE/VIOLET can become too dark
    // on the moving checkerboard. These mixes keep the OS style unified while
    // preserving small-screen readability.
    RGB8 uiText = addRgb({170, 180, 195}, scaleRgb(theme, 1, 3));
    RGB8 uiDim  = addRgb({ 92, 106, 122}, scaleRgb(theme, 1, 5));
    RGB8 uiHint = addRgb({120, 134, 150}, scaleRgb(theme, 1, 2));

    drawPalette.text = draw565(uiText.r, uiText.g, uiText.b);
    drawPalette.textDim = draw565(uiDim.r, uiDim.g, uiDim.b);
    drawPalette.hudBg = draw565(theme.r / 56 + 2, theme.g / 56 + 4, theme.b / 56 + 8);
    drawPalette.hint = draw565(uiHint.r, uiHint.g, uiHint.b);
    RGB8 flowRgb = {clampU8(theme.r / 7 + 4), clampU8(theme.g / 7 + 8), clampU8(theme.b / 7 + 12)};
    RGB8 fxDimRgb = {clampU8(theme.r / 18 + 3), clampU8(theme.g / 18 + 12), clampU8(theme.b / 18 + 18)};
    RGB8 fxBrightRgb = {clampU8(theme.r / 10 + 4), clampU8(theme.g / 10 + 20), clampU8(theme.b / 10 + 30)};
    if (rgbPlus) {
        flowRgb = rgbBoost(rgb565SwapLook(flowRgb), 45, 12, 0);
        fxDimRgb = rgbBoost(rgb565SwapLook(fxDimRgb), 20, 0, 22);
        fxBrightRgb = rgbBoost(rgb565SwapLook(fxBrightRgb), 60, 18, 24);
    }
    drawPalette.flow = draw565(flowRgb.r, flowRgb.g, flowRgb.b);
    bufPalette.flow = bufFromDraw(drawPalette.flow);
    bufPalette.gridLine = bufFromDraw(draw565(theme.r / 32 + 2, theme.g / 32 + 9, theme.b / 32 + 14));
    bufPalette.gridLineNear = bufFromDraw(draw565(theme.r / 24 + 3, theme.g / 24 + 13, theme.b / 24 + 22));
    bufPalette.fxDim = bufFromDraw(draw565(fxDimRgb.r, fxDimRgb.g, fxDimRgb.b));
    bufPalette.fxBright = bufFromDraw(draw565(fxBrightRgb.r, fxBrightRgb.g, fxBrightRgb.b));

    for (int s = 0; s < 256; ++s) {
        int hi = s > 205 ? (s - 205) : 0;
        int r = 3 + (theme.r * (34 + s)) / 292 + hi * 3;
        int g = 6 + (theme.g * (40 + s)) / 292 + hi * 2;
        int b = 10 + (theme.b * (54 + s)) / 292 + hi * 2;
        RGB8 donutRgb = {clampU8(r), clampU8(g), clampU8(b)};
        if (rgbPlus) donutRgb = rgbBoost(rgb565SwapLook(donutRgb), hi * 2 + s / 30, hi / 3, hi + s / 36);
        bufPalette.donut[s] = bufFromDraw(draw565(donutRgb.r, donutRgb.g, donutRgb.b));
    }
    bufPalette.donut[0] = 0;
    paletteDirty = false;
}

// -----------------------------------------------------------------------------
// Trig tables

static void buildTrigTable(int count, float* sinTable, float* cosTable) {
    for (int i = 0; i < count; ++i) {
        float a = (float)i * TAU_F / (float)count;
        sinTable[i] = sinf(a);
        cosTable[i] = cosf(a);
    }
}

static inline void rotateUnitFast(float& c, float& s, float t) {
    float oldC = c;
    c -= t * s;
    s += t * oldC;
    float f = (3.0f - c * c - s * s) * 0.5f;
    c *= f;
    s *= f;
}

static void resetRotationState() {
    rotCA = cosf(INITIAL_A);
    rotSA = sinf(INITIAL_A);
    rotCB = cosf(INITIAL_B);
    rotSB = sinf(INITIAL_B);
    bgScroll = 0.0f;
    bgMotionFx = 0;
    bgXPhaseFx = 0;
}

// -----------------------------------------------------------------------------
// Dirty rect and render buffers

static inline void resetDirtyRect() {
    dirtyMinX = SCREEN_W;
    dirtyMinY = SCREEN_H;
    dirtyMaxX = -1;
    dirtyMaxY = -1;
}

static inline bool dirtyValid() {
    return dirtyMaxX >= dirtyMinX && dirtyMaxY >= dirtyMinY;
}

static inline void touchDirty(int x, int y) {
    if (x < dirtyMinX) dirtyMinX = x;
    if (x > dirtyMaxX) dirtyMaxX = x;
    if (y < dirtyMinY) dirtyMinY = y;
    if (y > dirtyMaxY) dirtyMaxY = y;
}

static inline void expandedDirtyRect(int pad, int& x0, int& y0, int& x1, int& y1) {
    x0 = dirtyMinX - pad;
    y0 = dirtyMinY - pad;
    x1 = dirtyMaxX + pad;
    y1 = dirtyMaxY + pad;
    if (x0 < 0) x0 = 0;
    if (y0 < TOP_BAR_H) y0 = TOP_BAR_H;
    if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
    if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;
}

static void clearRenderBuffers() {
    resetDirtyRect();
    memset(zBuf + RENDER_START, 0, RENDER_PIXELS * sizeof(uint16_t));
    memset(shadeBuf + RENDER_START, 0, RENDER_PIXELS * sizeof(uint8_t));
    if (settings.glowEnabled) {
        memset(glowBuf + RENDER_START, 0, RENDER_PIXELS * sizeof(uint8_t));
    }
}

// -----------------------------------------------------------------------------
// Background

static inline void advanceBackgroundPhases() {
    uint32_t yStep = settings.motionProfile == 2 ? 780UL : 360UL;
    uint32_t xStep = settings.motionProfile == 2 ? 310UL : 170UL;
    bgMotionFx += yStep;  // 32-bit unbounded phase; avoids visible 16-bit wrap jumps.
    bgXPhaseFx += xStep;
}

static void updateBackgroundAnimationState() {
    if (settings.paused) return;

    // Advance background motion only when the cached background will actually be
    // rebuilt. This prevents cache-frame stutter.  The y phase is 32-bit and
    // unbounded, so it does not visibly snap at the old 16-bit wrap point.
    if (settings.backgroundMode == 1 || settings.backgroundMode == 3) {
        if (settings.motionProfile == 0) return;
        uint8_t interval = settings.motionProfile == 2 ? BG_REBUILD_INTERVAL_ACTIVE : BG_REBUILD_INTERVAL_CALM;
        if (++bgAnimCounter >= interval) {
            bgAnimCounter = 0;
            advanceBackgroundPhases();
            bgCacheDirty = true;
        }
        return;
    }

    if (settings.backgroundMode == 2) {
        if (settings.motionProfile == 0) return;
        if (++bgAnimCounter >= 3) {
            bgAnimCounter = 0;
            advanceBackgroundPhases();
            bgCacheDirty = true;
        }
    }
}

static void fillPlainBackground(uint16_t* out) {
    for (int y = TOP_BAR_H; y < SCREEN_H; ++y) {
        uint16_t* row = out + y * SCREEN_W;
        for (int x = 0; x < SCREEN_W; ++x) row[x] = bufPalette.plainBg;
    }
}

static inline void putBgPixel(uint16_t* out, int x, int y, uint16_t c) {
    if ((unsigned)x >= SCREEN_W || y < TOP_BAR_H || y >= SCREEN_H) return;
    out[y * SCREEN_W + x] = c;
}

static void drawBgLine(uint16_t* out, int x0, int y0, int x1, int y1, uint16_t c) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        putBgPixel(out, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err << 1;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void renderPerspectiveCheckerHalf(uint16_t* out, uint32_t scrollFx) {
    // Fast Cardputer port of the a1k0n header-bg shader.
    // It keeps the author's screen-space inverse projection and checker rule:
    //   gx = xoffset + uv.x / (uv.y - 0.3)
    //   gy = yoffset + 2.0 / (uv.y - 0.3)
    //   checker = (floor(gx) ^ floor(gy)) & 1
    // The per-row denominator is computed once; horizontal gx is advanced by a
    // fixed float step. Colors come from bgToneLut[by][mix], so the near/far
    // gradient is tied to projection depth, not arbitrary screen bands.

    static constexpr float invH = 1.0f / (float)RENDER_H;
    // y scroll is intentionally unbounded.  Checker parity remains continuous,
    // while avoiding a visible snap when the old 16-bit phase wrapped.
    float yPhase = (float)scrollFx * (1.0f / 65536.0f);
    float xPhase = (float)(bgXPhaseFx & 0xFFFFUL) * (1.0f / 65536.0f);

    // Same motion model as the reference: lateral sine wobble + monotonic y scroll.
    // y scroll is no longer clamped to one 16-bit cycle, so the checker keeps
    // moving forward without a short periodic reset.
    float xoffset = sinf(xPhase * TAU_F) * BG_X_WOBBLE_AMPLITUDE;
    float yoffset = -BG_Y_SCROLL_UNITS * yPhase;

    // Depth affects only lateral perspective wobble, not the forward scroll.
    // This keeps the floor stable when changing Q/E depth.
    if (settings.bgDepthLevel == 0) {
        xoffset *= BG_DEPTH_X_SCALE_SHALLOW;
    } else if (settings.bgDepthLevel == 2) {
        xoffset *= BG_DEPTH_X_SCALE_DEEP;
    }

    const float centerX = (float)SCREEN_W * 0.5f;

    for (int by = 0; by < BG_H; ++by) {
        int y0 = TOP_BAR_H + by * 2;
        int y1 = y0 + 1;
        if (y0 >= SCREEN_H) break;
        uint16_t* row0 = out + y0 * SCREEN_W;
        uint16_t* row1 = y1 < SCREEN_H ? out + y1 * SCREEN_W : row0;

        float sy = (float)(by * 2) + 0.5f;
        float fragY = (float)(RENDER_H - 1) - sy;
        float uvy = (fragY - (float)RENDER_H) * invH;
        float denom = uvy - 0.3f;
        float invDen = 1.0f / denom;

        float syUp = sy - 1.0f;
        float fragYUp = (float)(RENDER_H - 1) - syUp;
        float uvyUp = (fragYUp - (float)RENDER_H) * invH;
        float denomUp = uvyUp - 0.3f;
        float invDenUp = 1.0f / denomUp;

        int iy = (int)floorf(yoffset + 2.0f * invDen);
        int iyUp = (int)floorf(yoffset + 2.0f * invDenUp);

        float sx0 = 0.5f;
        float gx = xoffset + ((sx0 - centerX) * invH) * invDen;
        float gxStep = (2.0f * invH) * invDen;
        float gxRightDelta = invH * invDen;

        float gxUp = xoffset + ((sx0 - centerX) * invH) * invDenUp;
        float gxUpStep = (2.0f * invH) * invDenUp;

        for (int bx = 0; bx < BG_W; ++bx) {
            int x = bx * 2;

            int checker0 = (((int)floorf(gx)) ^ iy) & 1;
            int checker1 = (((int)floorf(gx + gxRightDelta)) ^ iy) & 1;
            int checker2 = (((int)floorf(gxUp)) ^ iyUp) & 1;
            int mix = 2 * checker0 + checker1 + checker2;

            uint16_t c = bgToneLut[by][mix];
            row0[x] = c;
            row0[x + 1] = c;
            row1[x] = c;
            row1[x + 1] = c;

            gx += gxStep;
            gxUp += gxUpStep;
        }
    }
}

static void drawCachedFlow(uint16_t* out) {
    if (settings.backgroundMode != 2 && settings.backgroundMode != 3) return;
    int offset = (int)((bgMotionFx >> 8) % FLOW_LINE_SPACING);
    for (int x = -SCREEN_H; x < SCREEN_W + SCREEN_H; x += FLOW_LINE_SPACING) {
        int x0 = x + offset;
        int x1 = x0 + FLOW_LINE_DX;
        if ((x0 < 0 && x1 < 0) || (x0 >= SCREEN_W && x1 >= SCREEN_W)) continue;
        drawBgLine(out, x0, SCREEN_H - 1, x1, FLOW_LINE_Y1, bufPalette.flow);
    }
}

static void buildBackgroundCache(uint16_t* out) {
    memset(out, 0, SCREEN_PIXELS * sizeof(uint16_t));

    if (settings.backgroundMode == 1 || settings.backgroundMode == 3) {
        renderPerspectiveCheckerHalf(out, bgMotionFx);
    } else {
        fillPlainBackground(out);
    }

    if (settings.backgroundMode == 2 || settings.backgroundMode == 3) {
        drawCachedFlow(out);
    }
}

static void updateBackgroundCacheIfNeeded() {
    if (!bgCacheDirty) {
        bgBuildUs = 0;
        return;
    }

    uint32_t t0 = micros();
    buildBackgroundCache(bgCache);
    bgBuildUs = micros() - t0;
    bgCacheDirty = false;
}

static void copyBackgroundCacheToCanvas() {
    uint32_t t0 = micros();
    memcpy(canvasBuf + RENDER_START, bgCache + RENDER_START, RENDER_PIXELS * sizeof(uint16_t));
    bgCopyUs = micros() - t0;
}

static void drawBackgroundFxOverlay(uint16_t* out) {
    if (!settings.bgFxEnabled || settings.motionProfile == 0 || settings.backgroundMode != 2) return;

    // Particles are only for FLOW mode.  Checker and RGB+ checker stay clean:
    // no extra rail/sweep line layer, no byte-order trick.
    uint32_t t = bgMotionFx;
    int particles = settings.motionProfile == 2 ? 32 : 14;
    for (int i = 0; i < particles; ++i) {
        uint16_t h = (uint16_t)(i * 1103u + (t >> 1));
        int x = (int)((h * 37u + i * 19u) % SCREEN_W);
        int y = TOP_BAR_H + 8 + (int)((h * 13u + i * 29u) % (RENDER_H - 12));
        uint16_t c = (i & 7) == 0 ? bufPalette.fxBright : bufPalette.fxDim;
        putBgPixel(out, x, y, c);
        if (settings.motionProfile == 2 && (i & 5) == 0) putBgPixel(out, x + 1, y, c);
    }
}

// -----------------------------------------------------------------------------
// Filled triangle rasterizer

#if USE_DEBUG_CONSERVATIVE_RASTER
// Debug fallback rasterizer.  Kept out of release builds by default.
static bool edgeScanIntersect(const MeshVertex& a, const MeshVertex& b, float fy, EdgeValue& out) {
    float dy = b.y - a.y;
    if (fabsf(dy) < 0.0001f) return false;

    float minY = fminf(a.y, b.y);
    float maxY = fmaxf(a.y, b.y);
    if (fy < minY - 0.65f || fy > maxY + 0.65f) return false;

    float t = (fy - a.y) / dy;
    if (t < -0.08f || t > 1.08f) return false;
    t = clamp01(t);

    out.x = a.x + (b.x - a.x) * t;
    out.z = a.z + (b.z - a.z) * t;
    out.shade = a.shade + (b.shade - a.shade) * t;
    return true;
}

static void drawSpanConservative(int y, EdgeValue l, EdgeValue r) {
    if (y < TOP_BAR_H || y >= SCREEN_H) return;
    if (l.x > r.x) {
        EdgeValue t = l;
        l = r;
        r = t;
    }

    int x0 = (int)floorf(l.x) - 1;
    int x1 = (int)ceilf(r.x) + 1;
    if (x1 < 0 || x0 >= SCREEN_W || x0 > x1) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;

    float width = r.x - l.x;
    float z = l.z;
    float shade = l.shade;
    float dz = 0.0f;
    float ds = 0.0f;
    if (fabsf(width) > 0.001f) {
        dz = (r.z - l.z) / width;
        ds = (r.shade - l.shade) / width;
        float advance = ((float)x0 + 0.5f) - l.x;
        z += dz * advance;
        shade += ds * advance;
    } else {
        z = (l.z + r.z) * 0.5f;
        shade = (l.shade + r.shade) * 0.5f;
    }

    int32_t zFx = (int32_t)(z * 256.0f);
    int32_t sFx = (int32_t)(shade * 256.0f);
    int32_t dzFx = (int32_t)(dz * 256.0f);
    int32_t dsFx = (int32_t)(ds * 256.0f);
    int idx = y * SCREEN_W + x0;

    for (int x = x0; x <= x1; ++x, ++idx) {
        int zi = zFx >> 8;
        if (zi < 0) zi = 0;
        if (zi > 65535) zi = 65535;
        uint16_t depth = (uint16_t)zi;
        if (depth > zBuf[idx]) {
            int si = sFx >> 8;
            if (si < LUM_DARK_MIN) si = LUM_DARK_MIN;
            if (si > LUM_BRIGHT_MAX) si = LUM_BRIGHT_MAX;
            zBuf[idx] = depth;
            shadeBuf[idx] = (uint8_t)si;
            touchDirty(x, y);
            ++fillPixelCount;
        }
        zFx += dzFx;
        sFx += dsFx;
    }
}

static void rasterTriangle(MeshVertex a, MeshVertex b, MeshVertex c) {
    float minX = fminf(a.x, fminf(b.x, c.x));
    float maxX = fmaxf(a.x, fmaxf(b.x, c.x));
    float minY = fminf(a.y, fminf(b.y, c.y));
    float maxY = fmaxf(a.y, fmaxf(b.y, c.y));
    if (maxX < 0.0f || minX >= (float)SCREEN_W || maxY < (float)TOP_BAR_H || minY >= (float)SCREEN_H) return;

    float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (fabsf(area) < TRI_AREA_MIN) return;

    ++rasterizedCount;

    int ys = (int)floorf(minY) - 1;
    int ye = (int)ceilf(maxY) + 1;
    if (ys < TOP_BAR_H) ys = TOP_BAR_H;
    if (ye >= SCREEN_H) ye = SCREEN_H - 1;

    for (int y = ys; y <= ye; ++y) {
        float fy = (float)y + 0.5f;
        EdgeValue hits[3];
        int count = 0;

        if (edgeScanIntersect(a, b, fy, hits[count])) ++count;
        if (edgeScanIntersect(b, c, fy, hits[count])) ++count;
        if (edgeScanIntersect(c, a, fy, hits[count])) ++count;

        if (count < 2) continue;

        EdgeValue left = hits[0];
        EdgeValue right = hits[0];
        for (int i = 1; i < count; ++i) {
            if (hits[i].x < left.x) left = hits[i];
            if (hits[i].x > right.x) right = hits[i];
        }

        drawSpanConservative(y, left, right);
    }
}
#endif

static inline float quadSignedArea2(const MeshVertex& a, const MeshVertex& b,
                                    const MeshVertex& c, const MeshVertex& d) {
    return (a.x * b.y - a.y * b.x)
         + (b.x * c.y - b.y * c.x)
         + (c.x * d.y - c.y * d.x)
         + (d.x * a.y - d.y * a.x);
}

static inline bool quadOffscreen(const MeshVertex& a, const MeshVertex& b,
                                 const MeshVertex& c, const MeshVertex& d) {
    float minX = a.x;
    if (b.x < minX) minX = b.x;
    if (c.x < minX) minX = c.x;
    if (d.x < minX) minX = d.x;
    float maxX = a.x;
    if (b.x > maxX) maxX = b.x;
    if (c.x > maxX) maxX = c.x;
    if (d.x > maxX) maxX = d.x;
    float minY = a.y;
    if (b.y < minY) minY = b.y;
    if (c.y < minY) minY = c.y;
    if (d.y < minY) minY = d.y;
    float maxY = a.y;
    if (b.y > maxY) maxY = b.y;
    if (c.y > maxY) maxY = c.y;
    if (d.y > maxY) maxY = d.y;
    return maxX < 0.0f || minX >= (float)SCREEN_W || maxY < (float)TOP_BAR_H || minY >= (float)SCREEN_H;
}

static inline bool quadBackFacing(const MeshVertex& a, const MeshVertex& b,
                                  const MeshVertex& c, const MeshVertex& d) {
#if ENABLE_BACKFACE_CULL
    float nz = (a.nz + b.nz + c.nz + d.nz) * 0.25f;
    return nz >= BACKFACE_EPS;
#else
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    return false;
#endif
}

static inline bool edgeScanIntersectFast(const MeshVertex& a, const MeshVertex& b,
                                         float fy, EdgeValue& out) {
    float dy = b.y - a.y;
    if (fabsf(dy) < 0.0001f) return false;
    if ((fy < a.y && fy < b.y) || (fy > a.y && fy > b.y)) return false;

    float t = (fy - a.y) / dy;
    if (t < 0.0f || t > 1.0f) return false;

    out.x = a.x + (b.x - a.x) * t;
    out.z = a.z + (b.z - a.z) * t;
#if ENABLE_SMOOTH_SHADE
    out.shade = a.shade + (b.shade - a.shade) * t;
#else
    out.shade = 0.0f;
#endif
    return true;
}

static void drawSpanFast(int y, EdgeValue l, EdgeValue r, uint8_t flatShade) {
    if (y < TOP_BAR_H || y >= SCREEN_H) return;
    if (l.x > r.x) {
        EdgeValue t = l;
        l = r;
        r = t;
    }

    int x0 = (int)floorf(l.x) - RASTER_CRACK_FIX_PAD;
    int x1 = (int)ceilf(r.x) + RASTER_CRACK_FIX_PAD;
    if (x1 < 0 || x0 >= SCREEN_W || x0 > x1) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;

    float width = r.x - l.x;
    float z = l.z;
    float dz = 0.0f;
#if ENABLE_SMOOTH_SHADE
    float shade = l.shade;
    float ds = 0.0f;
#endif
    if (fabsf(width) > 0.001f) {
        dz = (r.z - l.z) / width;
        float advance = ((float)x0 + 0.5f) - l.x;
        z += dz * advance;
#if ENABLE_SMOOTH_SHADE
        ds = (r.shade - l.shade) / width;
        shade += ds * advance;
#endif
    } else {
        z = (l.z + r.z) * 0.5f;
#if ENABLE_SMOOTH_SHADE
        shade = (l.shade + r.shade) * 0.5f;
#endif
    }

    int32_t zFx = (int32_t)(z * 256.0f);
    int32_t dzFx = (int32_t)(dz * 256.0f);
#if ENABLE_SMOOTH_SHADE
    int32_t sFx = (int32_t)(shade * 256.0f);
    int32_t dsFx = (int32_t)(ds * 256.0f);
#endif
    int idx = y * SCREEN_W + x0;
    int firstWriteX = -1;
    int lastWriteX = -1;

    for (int x = x0; x <= x1; ++x, ++idx) {
        int zi = zFx >> 8;
        if (zi < 0) zi = 0;
        if (zi > 65535) zi = 65535;
        uint16_t depth = (uint16_t)zi;
        if (depth > zBuf[idx]) {
#if ENABLE_SMOOTH_SHADE
            int si = sFx >> 8;
            if (si < LUM_DARK_MIN) si = LUM_DARK_MIN;
            if (si > LUM_BRIGHT_MAX) si = LUM_BRIGHT_MAX;
            shadeBuf[idx] = (uint8_t)si;
#else
            shadeBuf[idx] = flatShade;
#endif
            zBuf[idx] = depth;
            if (firstWriteX < 0) firstWriteX = x;
            lastWriteX = x;
            ++fillPixelCount;
        }
        zFx += dzFx;
#if ENABLE_SMOOTH_SHADE
        sFx += dsFx;
#endif
    }

    if (firstWriteX >= 0) {
        if (firstWriteX < dirtyMinX) dirtyMinX = firstWriteX;
        if (lastWriteX > dirtyMaxX) dirtyMaxX = lastWriteX;
        if (y < dirtyMinY) dirtyMinY = y;
        if (y > dirtyMaxY) dirtyMaxY = y;
    }
}

static void rasterQuadSolid(const MeshVertex& a, const MeshVertex& b,
                            const MeshVertex& c, const MeshVertex& d) {
    ++primitiveCount;

    if (quadOffscreen(a, b, c, d)) return;

    float area2 = quadSignedArea2(a, b, c, d);
    if (fabsf(area2) < TRI_AREA_MIN) {
        ++culledCount;
        return;
    }

    if (quadBackFacing(a, b, c, d)) {
        ++culledCount;
        return;
    }

    ++rasterizedCount;

    float minYf = a.y;
    if (b.y < minYf) minYf = b.y;
    if (c.y < minYf) minYf = c.y;
    if (d.y < minYf) minYf = d.y;
    float maxYf = a.y;
    if (b.y > maxYf) maxYf = b.y;
    if (c.y > maxYf) maxYf = c.y;
    if (d.y > maxYf) maxYf = d.y;
    int ys = (int)floorf(minYf);
    int ye = (int)ceilf(maxYf);
    if (ys < TOP_BAR_H) ys = TOP_BAR_H;
    if (ye >= SCREEN_H) ye = SCREEN_H - 1;

    int shade = (int)((a.shade + b.shade + c.shade + d.shade) * 0.25f + 0.5f);
    if (shade < LUM_DARK_MIN) shade = LUM_DARK_MIN;
    if (shade > LUM_BRIGHT_MAX) shade = LUM_BRIGHT_MAX;
    uint8_t flatShade = (uint8_t)shade;

    for (int y = ys; y <= ye; ++y) {
        float fy = (float)y + 0.5f;
        EdgeValue hits[4];
        int count = 0;

        if (edgeScanIntersectFast(a, b, fy, hits[count])) ++count;
        if (edgeScanIntersectFast(b, c, fy, hits[count])) ++count;
        if (edgeScanIntersectFast(c, d, fy, hits[count])) ++count;
        if (edgeScanIntersectFast(d, a, fy, hits[count])) ++count;

        if (count < 2) continue;

        EdgeValue left = hits[0];
        EdgeValue right = hits[0];
        for (int i = 1; i < count; ++i) {
            if (hits[i].x < left.x) left = hits[i];
            if (hits[i].x > right.x) right = hits[i];
        }

        drawSpanFast(y, left, right, flatShade);
    }
}

// -----------------------------------------------------------------------------
// Solid torus mesh

static void buildMeshRow(int thetaIndex, int thetaCount, int phiCount,
                         const float* sinTheta, const float* cosTheta,
                         const float* sinPhi, const float* cosPhi,
                         float cA, float sA, float cB, float sB,
                         MeshVertex* row) {
    float ct = cosTheta[thetaIndex];
    float st = sinTheta[thetaIndex];
    float circleX = TORUS_R2 + TORUS_R1 * ct;
    float circleY = TORUS_R1 * st;
    float centerX = (float)SCREEN_W * 0.5f + manualOffsetX + orbitOffsetX;
    float centerY = (float)TOP_BAR_H + (float)RENDER_H * 0.50f + manualOffsetY + orbitOffsetY;
    float scale = clampFloat(settings.donutScale + orbitScaleAdd, 70.0f, 112.0f);
    (void)thetaCount;

    float xCp = circleX * cB;
    float xSp = circleX * sA * sB;
    float xConst = -circleY * cA * sB;
    float yCp = circleX * sB;
    float ySp = -circleX * sA * cB;
    float yConst = circleY * cA * cB;
    float zSp = cA * circleX;
    float zConst = sA * circleY;
    float nzSp = cA * ct;
    float nzConst = sA * st;
    float lumCp = ct * sB;
    float lumSp = -cA * ct - cB * ct * sA;
    float lumConst = -sA * st + cB * cA * st;

    for (int pi = 0; pi < phiCount; ++pi) {
        float cp = cosPhi[pi];
        float sp = sinPhi[pi];

        float x = xCp * cp + xSp * sp + xConst;
        float y = yCp * cp + ySp * sp + yConst;
        float z = zSp * sp + zConst;
        float invZ = 1.0f / (CAMERA_Z + z);

        float lum = lumCp * cp + lumSp * sp + lumConst;
        float lumNorm = lum <= 0.0f ? 0.0f : clamp01(lum * 0.65f + 0.08f);
        float shade = (float)LUM_DARK_MIN + lumNorm * (float)(LUM_BRIGHT_MAX - LUM_DARK_MIN);

        row[pi].x = centerX + scale * invZ * x;
        row[pi].y = centerY - scale * invZ * y;
        row[pi].z = depthToU16(invZ);
        row[pi].shade = shade;
        row[pi].nz = nzSp * sp + nzConst;
    }
}

static void rasterRows(const MeshVertex* row0, const MeshVertex* row1, int phiCount) {
    for (int pi = 0; pi < phiCount; ++pi) {
        int pn = pi + 1;
        if (pn >= phiCount) pn = 0;
        const MeshVertex& a = row0[pi];
        const MeshVertex& b = row1[pi];
        const MeshVertex& c = row1[pn];
        const MeshVertex& d = row0[pn];
#if USE_DEBUG_CONSERVATIVE_RASTER
        ++primitiveCount;
        rasterTriangle(a, b, c);
        ++primitiveCount;
        rasterTriangle(a, c, d);
#else
        rasterQuadSolid(a, b, c, d);
#endif
    }
}

static void renderDonutSolidMesh() {
    const bool hq = settings.highQuality;
    const int thetaCount = hq ? HQ_THETA : STD_THETA;
    const int phiCount = hq ? HQ_PHI : STD_PHI;
    const float* sinTheta = hq ? sinThetaHq : sinThetaStd;
    const float* cosTheta = hq ? cosThetaHq : cosThetaStd;
    const float* sinPhi = hq ? sinPhiHq : sinPhiStd;
    const float* cosPhi = hq ? cosPhiHq : cosPhiStd;

    float cA = rotCA;
    float sA = rotSA;
    float cB = rotCB;
    float sB = rotSB;

    MeshVertex* curr = rowA;
    MeshVertex* next = rowB;

    buildMeshRow(0, thetaCount, phiCount, sinTheta, cosTheta, sinPhi, cosPhi, cA, sA, cB, sB, curr);
    memcpy(rowFirst, curr, sizeof(MeshVertex) * phiCount);

    for (int ti = 1; ti < thetaCount; ++ti) {
        buildMeshRow(ti, thetaCount, phiCount, sinTheta, cosTheta, sinPhi, cosPhi, cA, sA, cB, sB, next);
        rasterRows(curr, next, phiCount);
        MeshVertex* t = curr;
        curr = next;
        next = t;
    }

    rasterRows(curr, rowFirst, phiCount);
}

// -----------------------------------------------------------------------------
// Post process and composition

#if ENABLE_HOT_PIXEL_SUPPRESS
static void suppressHotPixelsDirtyOnly() {
    if (!dirtyValid()) return;

    int x0, y0, x1, y1;
    expandedDirtyRect(1, x0, y0, x1, y1);
    if (x0 < 1) x0 = 1;
    if (x1 > SCREEN_W - 2) x1 = SCREEN_W - 2;
    if (y0 < TOP_BAR_H + 1) y0 = TOP_BAR_H + 1;
    if (y1 > SCREEN_H - 2) y1 = SCREEN_H - 2;

    for (int y = y0; y <= y1; ++y) {
        int idx = y * SCREEN_W + x0;
        for (int x = x0; x <= x1; ++x, ++idx) {
            uint8_t s = shadeBuf[idx];
            if (s < 206) continue;
            uint16_t n = (uint16_t)shadeBuf[idx - 1]
                       + (uint16_t)shadeBuf[idx + 1]
                       + (uint16_t)shadeBuf[idx - SCREEN_W]
                       + (uint16_t)shadeBuf[idx + SCREEN_W];
            if (n < 96) shadeBuf[idx] = 188;
        }
    }
}
#endif

static inline void glowMax(int x, int y, uint8_t amount) {
    if ((unsigned)x >= SCREEN_W || y < TOP_BAR_H || y >= SCREEN_H || amount == 0) return;
    int idx = y * SCREEN_W + x;
    if (glowBuf[idx] < amount) glowBuf[idx] = amount;
}

static void buildGlowDirtyOnly() {
    if (!settings.glowEnabled || !dirtyValid()) return;
    int x0, y0, x1, y1;
    expandedDirtyRect(1, x0, y0, x1, y1);
    if (x0 < 1) x0 = 1;
    if (x1 > SCREEN_W - 2) x1 = SCREEN_W - 2;
    if (y0 < TOP_BAR_H + 1) y0 = TOP_BAR_H + 1;
    if (y1 > SCREEN_H - 2) y1 = SCREEN_H - 2;

    for (int y = y0; y <= y1; ++y) {
        int idx = y * SCREEN_W + x0;
        for (int x = x0; x <= x1; ++x, ++idx) {
            uint8_t s = shadeBuf[idx];
            if (s < GLOW_SRC_MIN) continue;
            bool edge = shadeBuf[idx - 1] == 0 ||
                        shadeBuf[idx + 1] == 0 ||
                        shadeBuf[idx - SCREEN_W] == 0 ||
                        shadeBuf[idx + SCREEN_W] == 0;
            if (!edge && s < 185) continue;

            int g = ((int)s - GLOW_SRC_MIN) * GLOW_SRC_SCALE;
            if (g > GLOW_MAX) g = GLOW_MAX;
            if (edge) {
                g += 80;
                if (g > 255) g = 255;
            } else {
                g >>= 1;
            }

            uint8_t c0 = (uint8_t)g;
            uint8_t c1 = (uint8_t)((g * 3) >> 2);
            uint8_t c2 = (uint8_t)(g >> 1);
            uint8_t c3 = (uint8_t)(g / 3);

            glowMax(x,     y,     c0);
            glowMax(x - 1, y,     c1);
            glowMax(x + 1, y,     c1);
            glowMax(x,     y - 1, c1);
            glowMax(x,     y + 1, c1);
            glowMax(x - 1, y - 1, c2);
            glowMax(x + 1, y - 1, c2);
            glowMax(x - 1, y + 1, c2);
            glowMax(x + 1, y + 1, c2);
            glowMax(x - 2, y,     c3);
            glowMax(x + 2, y,     c3);
            glowMax(x,     y - 2, c3);
            glowMax(x,     y + 2, c3);
        }
    }
}

static void composeDonutDirtyOnly(uint16_t* out) {
    if (!dirtyValid()) return;

    const RGB8 theme = THEMES[settings.themeIndex % THEME_COUNT].rgb;
    int x0, y0, x1, y1;
    expandedDirtyRect(settings.glowEnabled ? GLOW_RADIUS + 2 : 0, x0, y0, x1, y1);

    for (int y = y0; y <= y1; ++y) {
        int idx = y * SCREEN_W + x0;
        for (int x = x0; x <= x1; ++x, ++idx) {
            uint8_t s = shadeBuf[idx];
            if (s != 0) {
                uint16_t c = bufPalette.donut[s];
                if (settings.glowEnabled && glowBuf[idx]) c = addGlowToBuf(c, theme, glowBuf[idx], true);
                out[idx] = c;
            } else if (settings.glowEnabled && glowBuf[idx]) {
                out[idx] = addGlowToBuf(out[idx], theme, glowBuf[idx], false);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// UI

static inline void printShadowText(int x, int y, const char* text, uint16_t fg) {
    canvas.setTextColor(drawPalette.hudBg);
    canvas.setCursor(x + 1, y + 1);
    canvas.print(text);
    canvas.setTextColor(fg);
    canvas.setCursor(x, y);
    canvas.print(text);
}

static inline void printShadowChar(int x, int y, char ch, uint16_t fg) {
    canvas.setTextColor(drawPalette.hudBg);
    canvas.setCursor(x + 1, y + 1);
    canvas.print(ch);
    canvas.setTextColor(fg);
    canvas.setCursor(x, y);
    canvas.print(ch);
}

static void drawBgTextRibbonRow(const char* text, int y, int phasePx, uint16_t color) {
    int len = (int)strlen(text);
    if (len <= 0) return;
    int period = len * 6;
    if (period <= 0) return;
    int x = -phasePx;
    while (x > 0) x -= period;
    canvas.setTextColor(color);
    for (; x < SCREEN_W; x += period) {
        canvas.setCursor(x, y);
        canvas.print(text);
    }
}

static const char* floatingTextModeName() {
    if (settings.floatingTextMode == 1) return "OS";
    if (settings.floatingTextMode == 2) return "SIGN";
    if (settings.floatingTextMode == 3) return "TECH";
    return "OFF";
}

static void drawFloatingBackgroundText() {
    if (settings.floatingTextMode == 0) return;

    // Inspired by a1k0n's enhanced donut: text is stamped into the background
    // before the torus is rendered, so the donut naturally covers it.
    // Modes keep the small screen readable: OS / SIGN / TECH.
    static const char* OS0 = "  DONUT.OS  ";
    static const char* OS1 = "  K HELP  Y SYS  N WATCH  ";
    static const char* OS2 = "  STOPWATCH  TIMER  SETTINGS  ";

    static const char* SG0 = "  kayro.cn  ";
    static const char* SG1 = "  i@kayro.cn  ";
    static const char* SG2 = "  DONUT.OS by KAYRO  ";

    static const char* TC0 = "  M5CARDPUTER  Z-BUFFER  ";
    static const char* TC1 = "  A1K0N  RASTER  SPI  ";
    static const char* TC2 = "  ESP32-S3  M5GFX  ";

    canvas.setTextSize(1);
    canvas.setTextWrap(false, false);

    uint16_t dim = drawPalette.textDim;
    uint16_t hint = drawPalette.hint;
    uint32_t t = frameNumber;

    if (settings.floatingTextMode == 1) {
        drawBgTextRibbonRow(OS0, TOP_BAR_H + 28, (int)((t / 3 + 11) % (strlen(OS0) * 6)), hint);
        drawBgTextRibbonRow(OS1, TOP_BAR_H + 58, (int)((t / 2 + 37) % (strlen(OS1) * 6)), dim);
        drawBgTextRibbonRow(OS2, TOP_BAR_H + 96, (int)((t / 3 + 71) % (strlen(OS2) * 6)), hint);
    } else if (settings.floatingTextMode == 2) {
        drawBgTextRibbonRow(SG0, TOP_BAR_H + 32, (int)((t / 3 + 17) % (strlen(SG0) * 6)), hint);
        drawBgTextRibbonRow(SG1, TOP_BAR_H + 60, (int)((t / 2 + 43) % (strlen(SG1) * 6)), hint);
        drawBgTextRibbonRow(SG2, TOP_BAR_H + 91, (int)((t / 3 + 79) % (strlen(SG2) * 6)), dim);
    } else {
        drawBgTextRibbonRow(TC0, TOP_BAR_H + 18, (int)((t / 2 +  7) % (strlen(TC0) * 6)), dim);
        drawBgTextRibbonRow(TC1, TOP_BAR_H + 52, (int)((t / 2 + 47) % (strlen(TC1) * 6)), hint);
        drawBgTextRibbonRow(TC2, TOP_BAR_H + 92, (int)((t / 3 + 71) % (strlen(TC2) * 6)), dim);
    }
}

static void drawBehindTextAndHints() {
    drawFloatingBackgroundText();
}

static void drawPanelFrame(int x, int y, int w, int h);
static void printPanelLine(int x, int y, const char* text, uint16_t color);

static void pushToast(const char* msg) {
    strncpy(toastState.text, msg, sizeof(toastState.text) - 1);
    toastState.text[sizeof(toastState.text) - 1] = '\0';
    toastState.startMs = millis();
    toastState.durationMs = TOAST_MS;
    toastState.active = true;
}

static void drawToast() {
    if (!toastState.active) return;
    uint32_t age = millis() - toastState.startMs;
    if (age > toastState.durationMs) {
        toastState.active = false;
        return;
    }

    int len = (int)strlen(toastState.text);
    int w = len * 6 + 14;
    if (w > SCREEN_W - 8) w = SCREEN_W - 8;
    const int h = 15;
    const int x = SCREEN_W - w - 4;
    const int y = SCREEN_H - h - 4;
    drawPanelFrame(x, y, w, h);
    canvas.setTextSize(1);
    canvas.setTextWrap(false, false);
    printPanelLine(x + 6, y + 4, toastState.text, drawPalette.text);
}

static void drawBootOverlay() {
    uint32_t age = millis() - startMs;
    if (age > BOOT_OVERLAY_MS) return;

    const int x = 14;
    const int y = TOP_BAR_H + 8;
    const int w = 170;
    const int h = 54;
    drawPanelFrame(x, y, w, h);
    canvas.setTextSize(1);
    canvas.setTextWrap(false, false);

    printPanelLine(x + 7, y + 5, "DONUT.OS", drawPalette.text);
    printPanelLine(x + w - 45, y + 5, "BOOT", drawPalette.hint);

    uint16_t c0 = drawPalette.hint;
    uint16_t c1 = (age >= BOOT_STAGE1_MS) ? drawPalette.hint : drawPalette.textDim;
    uint16_t c2 = (age >= BOOT_STAGE2_MS) ? drawPalette.hint : drawPalette.textDim;

    printPanelLine(x + 7, y + 18, "GFX CORE READY", c0);
    printPanelLine(x + 7, y + 29, "RASTER FAST", c1);
    printPanelLine(x + 7, y + 40, "PREF V9  Y SYS  N WATCH", c2);
}

static void formatRuntimeClock(char* out, size_t n) {
    uint32_t seconds = (millis() - startMs) / 1000UL;
    uint32_t hh = (seconds / 3600UL) % 100UL;
    uint32_t mm = (seconds / 60UL) % 60UL;
    uint32_t ss = seconds % 60UL;
    snprintf(out, n, "%02lu:%02lu:%02lu", (unsigned long)hh, (unsigned long)mm, (unsigned long)ss);
}

static void formatRuntimeShort(char* out, size_t n) {
    uint32_t seconds = (millis() - startMs) / 1000UL;
    uint32_t hh = (seconds / 3600UL) % 100UL;
    uint32_t mm = (seconds / 60UL) % 60UL;
    if (hh > 0) snprintf(out, n, "%02luh%02lum", (unsigned long)hh, (unsigned long)mm);
    else snprintf(out, n, "%02lum%02lus", (unsigned long)mm, (unsigned long)(seconds % 60UL));
}

static void formatRuntimeTopbar(char* out, size_t n) {
    uint32_t seconds = (millis() - startMs) / 1000UL;
    uint32_t hh = (seconds / 3600UL) % 100UL;
    uint32_t mm = (seconds / 60UL) % 60UL;
    snprintf(out, n, "%02lu:%02lu", (unsigned long)hh, (unsigned long)mm);
}

static uint32_t getStopwatchElapsedMs() {
    if (!stopwatchState.running) return stopwatchState.baseMs;
    return stopwatchState.baseMs + (uint32_t)(millis() - stopwatchState.startMs);
}

static uint32_t getTimerRemainingMs() {
    uint32_t elapsed = getStopwatchElapsedMs();
    if (elapsed >= stopwatchState.timerTargetMs) return 0;
    return stopwatchState.timerTargetMs - elapsed;
}

static void formatDurationMs(uint32_t ms, char* out, size_t n, bool centiseconds) {
    uint32_t seconds = ms / 1000UL;
    uint32_t hh = (seconds / 3600UL) % 100UL;
    uint32_t mm = (seconds / 60UL) % 60UL;
    uint32_t ss = seconds % 60UL;
    if (centiseconds) {
        uint32_t cs = (ms / 10UL) % 100UL;
        snprintf(out, n, "%02lu:%02lu:%02lu.%02lu",
                 (unsigned long)hh, (unsigned long)mm,
                 (unsigned long)ss, (unsigned long)cs);
    } else {
        snprintf(out, n, "%02lu:%02lu:%02lu",
                 (unsigned long)hh, (unsigned long)mm,
                 (unsigned long)ss);
    }
}

static void formatStopwatch(char* out, size_t n) {
    if (stopwatchState.mode == 1) formatDurationMs(getTimerRemainingMs(), out, n, false);
    else formatDurationMs(getStopwatchElapsedMs(), out, n, true);
}

static void toggleStopwatchRun() {
    uint32_t now = millis();
    if (stopwatchState.running) {
        stopwatchState.baseMs += (uint32_t)(now - stopwatchState.startMs);
        stopwatchState.running = false;
    } else {
        if (stopwatchState.mode == 1 && stopwatchState.timerDone) {
            // X after DONE restarts the countdown from its preset.
            stopwatchState.baseMs = 0;
            stopwatchState.timerDone = false;
            timerFlashUntilMs = 0;
        }
        stopwatchState.startMs = now;
        stopwatchState.running = true;
    }
}

static void resetStopwatch() {
    stopwatchState.running = false;
    stopwatchState.baseMs = 0;
    stopwatchState.startMs = millis();
    stopwatchState.timerDone = false;
    timerFlashUntilMs = 0;
}

static void setWatchMode(uint8_t mode) {
    if (mode > 1) mode = 0;
    if (stopwatchState.mode != mode) {
        stopwatchState.mode = mode;
        resetStopwatch();
    }
}

static bool adjustTimerTarget(int32_t deltaMs) {
    if (stopwatchState.mode != 1 || stopwatchState.running) return false;
    int32_t next = (int32_t)stopwatchState.timerTargetMs + deltaMs;
    if (next < (int32_t)TIMER_STEP_MS) next = (int32_t)TIMER_STEP_MS;
    if (next > (int32_t)(99UL * 60UL * 1000UL)) next = (int32_t)(99UL * 60UL * 1000UL);
    if ((uint32_t)next == stopwatchState.timerTargetMs) return false;
    stopwatchState.timerTargetMs = (uint32_t)next;
    stopwatchState.baseMs = 0;
    stopwatchState.timerDone = false;
    return true;
}

static void updateWatchState() {
    if (stopwatchState.mode != 1 || !stopwatchState.running) return;
    if (getStopwatchElapsedMs() >= stopwatchState.timerTargetMs) {
        stopwatchState.baseMs = stopwatchState.timerTargetMs;
        stopwatchState.running = false;
        stopwatchState.timerDone = true;
        timerFlashUntilMs = millis() + TIMER_FLASH_MS;
        settings.showStopwatchPanel = true;
        pushToast("TIMER DONE");
    }
}

static void printPanelLine(int x, int y, const char* text, uint16_t color) {
    canvas.setTextColor(color, drawPalette.hudBg);
    canvas.setCursor(x, y);
    canvas.print(text);
}

static void drawPanelFrame(int x, int y, int w, int h) {
    canvas.fillRect(x + 1, y + 1, w, h, drawPalette.topBar);
    canvas.fillRect(x, y, w, h, drawPalette.hudBg);
    canvas.drawRect(x, y, w, h, drawPalette.topLine);
}

static void drawHelpPanel() {
    if (!settings.showKeyHints) return;

    const int x = 4;
    const int y = SCREEN_H - 47;
    const int w = SCREEN_W - 8;
    const int h = 43;
    drawPanelFrame(x, y, w, h);

    canvas.setTextSize(1);
    canvas.setTextWrap(false, false);

    char title[22];
    snprintf(title, sizeof(title), "HELP %u/3", (unsigned)(helpPage + 1));
    printPanelLine(x + 5, y + 4, title, drawPalette.text);
    printPanelLine(x + w - 47, y + 4, "K NEXT", drawPalette.hint);

    if (helpPage == 0) {
        printPanelLine(x + 5, y + 15, "SPC Theme B BG C RGB+", drawPalette.textDim);
        printPanelLine(x + 5, y + 26, "T RIB O Orb G Glow H HQ", drawPalette.hint);
    } else if (helpPage == 1) {
        printPanelLine(x + 5, y + 15, "WASD Pan  +/- Scale  0 View", drawPalette.textDim);
        printPanelLine(x + 5, y + 26, ";/. Speed  [/] Bright  QE Depth", drawPalette.hint);
    } else {
        printPanelLine(x + 5, y + 15, "Y Sys  N Watch/Timer  X Run", drawPalette.textDim);
        printPanelLine(x + 5, y + 26, "Z Reset  R+~ Factory  ~ Boot", drawPalette.hint);
    }
}

static void drawStopwatchPanel() {
    if (!settings.showStopwatchPanel) return;

    const int x = 18;
    const int y = TOP_BAR_H + 54;
    const int w = SCREEN_W - 36;
    const int h = 43;
    drawPanelFrame(x, y, w, h);
    canvas.setTextSize(1);
    canvas.setTextWrap(false, false);

    char timeText[18];
    formatStopwatch(timeText, sizeof(timeText));

    const bool timer = stopwatchState.mode == 1;
    printPanelLine(x + 6, y + 4, timer ? "TIMER" : "STOPWATCH", drawPalette.text);
    printPanelLine(x + w - 46, y + 4,
                   stopwatchState.timerDone ? "DONE" : (stopwatchState.running ? "RUN" : "STOP"),
                   drawPalette.hint);
    printPanelLine(x + 30, y + 17, timeText, drawPalette.hint);
    if (timer) printPanelLine(x + 6, y + 30, "X RUN  Z RESET  +/- SET  N NEXT", drawPalette.textDim);
    else printPanelLine(x + 6, y + 30, "X START/STOP   Z RESET   N TIMER", drawPalette.textDim);
}

static void drawSystemPanel() {
    const int x = SCREEN_W - 104;
    const int y = TOP_BAR_H + 3;
    const int w = 100;
    const int h = 82;
    drawPanelFrame(x, y, w, h);
    canvas.setTextSize(1);
    canvas.setTextWrap(false, false);

    char line[28];
    char clk[12];
    char shortUp[12];
    formatRuntimeClock(clk, sizeof(clk));
    formatRuntimeShort(shortUp, sizeof(shortUp));

    if (systemPanelPage == 0) {
        printPanelLine(x + 4, y + 4, "SYS 1/3", drawPalette.text);
        printPanelLine(x + w - 39, y + 4, "Y NEXT", drawPalette.hint);
        snprintf(line, sizeof(line), "TIME %s", clk);
        printPanelLine(x + 4, y + 15, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "UP %s", shortUp);
        printPanelLine(x + 4, y + 26, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "CPU %uMHz", (unsigned)ESP.getCpuFreqMHz());
        printPanelLine(x + 4, y + 37, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "HEAP %luK", (unsigned long)(ESP.getFreeHeap() / 1024UL));
        printPanelLine(x + 4, y + 48, line, drawPalette.textDim);
        uint32_t psFree = ESP.getFreePsram();
        uint32_t psTotal = ESP.getPsramSize();
        if (psTotal > 0) snprintf(line, sizeof(line), "PS %lu/%luK", (unsigned long)(psFree / 1024UL), (unsigned long)(psTotal / 1024UL));
        else snprintf(line, sizeof(line), "PS --");
        printPanelLine(x + 4, y + 59, line, drawPalette.hint);
        snprintf(line, sizeof(line), "PREF %s V%lu", prefsDirty ? "PEND" : "OK", (unsigned long)SETTINGS_VERSION);
        printPanelLine(x + 4, y + 70, line, drawPalette.hint);
    } else if (systemPanelPage == 1) {
        printPanelLine(x + 4, y + 4, "RND 2/3", drawPalette.text);
        printPanelLine(x + w - 39, y + 4, "Y NEXT", drawPalette.hint);
        uint8_t target = FPS_TARGETS[settings.frameTargetIndex];
        if (target) snprintf(line, sizeof(line), "FPS %02d/%02u", (int)(measuredFps + 0.5f), (unsigned)target);
        else snprintf(line, sizeof(line), "FPS %02d/MAX", (int)(measuredFps + 0.5f));
        printPanelLine(x + 4, y + 15, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "PUSH %lu", (unsigned long)pushTimeUs);
        printPanelLine(x + 4, y + 26, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "BG %lu/%lu", (unsigned long)bgBuildUs, (unsigned long)bgCopyUs);
        printPanelLine(x + 4, y + 37, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "D/C %lu/%lu", (unsigned long)donutTimeUs, (unsigned long)composeTimeUs);
        printPanelLine(x + 4, y + 48, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "PX %lu", (unsigned long)fillPixelCount);
        printPanelLine(x + 4, y + 59, line, drawPalette.hint);
        snprintf(line, sizeof(line), "ORB%u ID%u", (unsigned)orbitBlendQ8, (unsigned)((millis() - lastInputMs) > IDLE_ORBIT_MS));
        printPanelLine(x + 4, y + 70, line, drawPalette.hint);
    } else {
        printPanelLine(x + 4, y + 4, "SET 3/3", drawPalette.text);
        printPanelLine(x + w - 34, y + 4, "Y OFF", drawPalette.hint);
        snprintf(line, sizeof(line), "THEME %s", THEMES[settings.themeIndex % THEME_COUNT].name);
        printPanelLine(x + 4, y + 15, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "BG%u M%u D%u", (unsigned)settings.backgroundMode, (unsigned)settings.motionProfile, (unsigned)settings.bgDepthLevel);
        printPanelLine(x + 4, y + 26, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "TXT %s", floatingTextModeName());
        printPanelLine(x + 4, y + 37, line, drawPalette.textDim);
        snprintf(line, sizeof(line), "RGB%u G%u H%u O%u", (unsigned)(settings.rgbPlusEnabled ? 1 : 0), (unsigned)(settings.glowEnabled ? 1 : 0), (unsigned)(settings.highQuality ? 1 : 0), (unsigned)(settings.idleOrbitEnabled ? 1 : 0));
        printPanelLine(x + 4, y + 48, line, drawPalette.textDim);
        {
            int spd10 = (int)(settings.rotationSpeed * 10.0f + 0.5f);
            snprintf(line, sizeof(line), "SPD %u.%u", (unsigned)(spd10 / 10), (unsigned)(spd10 % 10));
        }
        printPanelLine(x + 4, y + 59, line, drawPalette.hint);
        snprintf(line, sizeof(line), "S%u BR%u", (unsigned)settings.donutScale, (unsigned)settings.brightness);
        printPanelLine(x + 4, y + 70, line, drawPalette.hint);
    }
}

static void drawTopBar() {
    uint32_t now = millis();
    // Signed deadline delta is safe for TIMER_FLASH_MS because the interval is far below 2^31 ms.
    bool timerFlash = timerFlashUntilMs != 0 && (int32_t)(timerFlashUntilMs - now) > 0;
    if (timerFlashUntilMs != 0 && !timerFlash) timerFlashUntilMs = 0;

    uint16_t topBg = drawPalette.topBar;
    if (timerFlash && ((now >> 7) & 1)) {
        topBg = draw565(180, 30, 30);
    }

    canvas.fillRect(0, 0, SCREEN_W, TOP_BAR_H, topBg);
    canvas.drawFastHLine(0, TOP_BAR_H - 1, SCREEN_W, drawPalette.topLine);
    canvas.setTextSize(1);
    canvas.setTextWrap(false, false);

    uint8_t target = FPS_TARGETS[settings.frameTargetIndex];

    char left[20];
    char mid[20];
    char clk[8];
    snprintf(left, sizeof(left), "D.OS %s", THEMES[settings.themeIndex % THEME_COUNT].name);
    if (target == 0) snprintf(mid, sizeof(mid), "%02d/MAX B%uM%u", (int)(measuredFps + 0.5f), (unsigned)settings.backgroundMode, (unsigned)settings.motionProfile);
    else snprintf(mid, sizeof(mid), "%02d/%02u B%uM%u", (int)(measuredFps + 0.5f), (unsigned)target, (unsigned)settings.backgroundMode, (unsigned)settings.motionProfile);
    formatRuntimeTopbar(clk, sizeof(clk));

    canvas.setTextColor(drawPalette.text, topBg);
    canvas.setCursor(2, 2);
    canvas.print(left);

    canvas.setTextColor(drawPalette.textDim, topBg);
    canvas.setCursor(86, 2);
    canvas.print(mid);

    int clockX = SCREEN_W - 2 - 5 * 6;
    canvas.setTextColor(drawPalette.hint, topBg);
    canvas.setCursor(clockX, 2);
    canvas.print(clk);
}

// -----------------------------------------------------------------------------
// Input

static bool isKeySpecPressed(const KeySpec& spec) {
    if (spec.a && M5Cardputer.Keyboard.isKeyPressed(spec.a)) return true;
    if (spec.b && M5Cardputer.Keyboard.isKeyPressed(spec.b)) return true;
    if (spec.c && M5Cardputer.Keyboard.isKeyPressed(spec.c)) return true;
    return false;
}

static bool actionEdge(ActionId id, bool down) {
    bool was = actionWasDown[(int)id];
    actionWasDown[(int)id] = down;
    return down && !was;
}

static bool actionRepeat(ActionId id, bool down, uint32_t now) {
    if (!down) return false;
    if (!actionWasDown[(int)id]) {
        actionLastRepeat[(int)id] = now;
        return true;
    }
    if ((uint32_t)(now - actionLastRepeat[(int)id]) >= REPEAT_MS) {
        actionLastRepeat[(int)id] = now;
        return true;
    }
    return false;
}

static void resetViewState() {
    manualOffsetX = 0.0f;
    manualOffsetY = 0.0f;
    orbitOffsetX = 0.0f;
    orbitOffsetY = 0.0f;
    orbitScaleAdd = 0.0f;
    settings.donutScale = 94.0f;
}

static void resetRuntimeSettings() {
    settings = RuntimeSettings();
    resetViewState();
    resetRotationState();
    M5Cardputer.Display.setBrightness(settings.brightness);
    paletteDirty = true;
    bgCacheDirty = true;
    bgAnimCounter = 0;
    bgMotionFx = 0;
    bgXPhaseFx = 0;
    orbitPhaseFx = 0;
    orbitBlendQ8 = 0;
    helpPage = 0;
    systemPanelPage = 0;
    toastState.active = false;
    stopwatchState.running = false;
    stopwatchState.baseMs = 0;
    stopwatchState.startMs = 0;
    stopwatchState.mode = 0;
    stopwatchState.timerTargetMs = TIMER_DEFAULT_MS;
    stopwatchState.timerDone = false;
    timerFlashUntilMs = 0;
    markSettingsDirty();
}

static void handleInput() {
    bool down[ACT_COUNT];
    for (int i = 0; i < ACT_COUNT; ++i) down[i] = isKeySpecPressed(KEY_SPECS[i]);
    uint32_t now = millis();
    bool used = false;
    bool persist = false;
    char msg[24];

    bool factoryChord = down[ACT_RESET] && down[ACT_RESTART];
    if (factoryChord) {
        if (factoryResetHoldMs == 0) {
            factoryResetHoldMs = now;
            pushToast("HOLD FACTORY");
        } else if (!factoryResetTriggered && (uint32_t)(now - factoryResetHoldMs) >= FACTORY_RESET_HOLD_MS) {
            factoryResetTriggered = true;
            if (prefsReady) prefs.clear();
            resetRuntimeSettings();
            savePersistentSettingsNow();
            pushToast("FACTORY RESET");
            delay(250);
            ESP.restart();
        }
        lastInputMs = now;
        for (int i = 0; i < ACT_COUNT; ++i) actionWasDown[i] = down[i];
        actionWasDown[ACT_RESET] = true;
        actionWasDown[ACT_RESTART] = true;
        return;
    }
    if (factoryResetHoldMs != 0) {
        actionWasDown[ACT_RESET] = down[ACT_RESET];
        actionWasDown[ACT_RESTART] = down[ACT_RESTART];
    }
    factoryResetHoldMs = 0;
    factoryResetTriggered = false;

    if (actionEdge(ACT_THEME, down[ACT_THEME])) {
        settings.themeIndex = (settings.themeIndex + 1) % THEME_COUNT;
        paletteDirty = true;
        bgCacheDirty = true;
        persist = true;
        snprintf(msg, sizeof(msg), "THEME %s", THEMES[settings.themeIndex % THEME_COUNT].name);
        pushToast(msg);
        used = true;
    }
    if (actionEdge(ACT_BACKGROUND, down[ACT_BACKGROUND])) {
        settings.backgroundMode = (settings.backgroundMode + 1) & 3;
        bgCacheDirty = true;
        bgAnimCounter = 0;
        persist = true;
        if (settings.backgroundMode == 0) snprintf(msg, sizeof(msg), "BG PLAIN");
        else if (settings.backgroundMode == 1) snprintf(msg, sizeof(msg), "BG CHECKER");
        else if (settings.backgroundMode == 2) snprintf(msg, sizeof(msg), "BG FLOW");
        else snprintf(msg, sizeof(msg), "BG CHECK+FLOW");
        pushToast(msg);
        used = true;
    }
    if (actionEdge(ACT_RGB_PLUS, down[ACT_RGB_PLUS])) {
        settings.rgbPlusEnabled = !settings.rgbPlusEnabled;
        paletteDirty = true;
        bgCacheDirty = true;
        persist = true;
        pushToast(settings.rgbPlusEnabled ? "RGB+ ON" : "RGB+ OFF");
        used = true;
    }
    if (actionEdge(ACT_FLOAT, down[ACT_FLOAT])) {
        settings.floatingTextMode = (settings.floatingTextMode + 1) & 3;
        bgCacheDirty = true;
        persist = true;
        snprintf(msg, sizeof(msg), "TEXT %s", floatingTextModeName());
        pushToast(msg);
        used = true;
    }
    if (actionEdge(ACT_KEYS, down[ACT_KEYS])) {
        if (!settings.showKeyHints) {
            settings.showKeyHints = true;
            helpPage = 0;
            pushToast("HELP 1/3");
        } else if (helpPage + 1 < HELP_PAGE_COUNT) {
            ++helpPage;
            snprintf(msg, sizeof(msg), "HELP %u/3", (unsigned)(helpPage + 1));
            pushToast(msg);
        } else {
            settings.showKeyHints = false;
            helpPage = 0;
            pushToast("HELP OFF");
        }
        used = true;
    }
    if (actionEdge(ACT_GLOW, down[ACT_GLOW])) {
        settings.glowEnabled = !settings.glowEnabled;
        memset(glowBuf + RENDER_START, 0, RENDER_PIXELS * sizeof(uint8_t));
        persist = true;
        pushToast(settings.glowEnabled ? "GLOW ON" : "GLOW OFF");
        used = true;
    }
    if (actionEdge(ACT_QUALITY, down[ACT_QUALITY])) {
        settings.highQuality = !settings.highQuality;
        persist = true;
        pushToast(settings.highQuality ? "QUALITY HQ" : "QUALITY STD");
        used = true;
    }
    if (actionEdge(ACT_PAUSE, down[ACT_PAUSE])) {
        settings.paused = !settings.paused;
        pushToast(settings.paused ? "PAUSE" : "RUN");
        used = true;
    }
    if (actionEdge(ACT_FPS, down[ACT_FPS])) {
        settings.frameTargetIndex = (settings.frameTargetIndex + 1) & 3;
        uint8_t target = FPS_TARGETS[settings.frameTargetIndex];
        if (target) snprintf(msg, sizeof(msg), "FPS %u", (unsigned)target);
        else snprintf(msg, sizeof(msg), "FPS MAX");
        persist = true;
        pushToast(msg);
        used = true;
    }
    if (actionEdge(ACT_RESET, down[ACT_RESET])) {
        resetRuntimeSettings();
        pushToast("SYSTEM RESET");
        used = true;
    }
    if (actionEdge(ACT_RESTART, down[ACT_RESTART])) ESP.restart();

    if (actionEdge(ACT_VIEW_RESET, down[ACT_VIEW_RESET])) {
        resetViewState();
        pushToast("VIEW RESET");
        used = true;
    }
    if (actionEdge(ACT_MOTION, down[ACT_MOTION])) {
        settings.motionProfile = (settings.motionProfile + 1) % 3;
        settings.bgFxEnabled = settings.motionProfile != 0;
        persist = true;
        if (settings.motionProfile == 0) pushToast("MOTION STILL");
        else if (settings.motionProfile == 1) pushToast("MOTION CALM");
        else pushToast("MOTION ACTIVE");
        used = true;
    }
    if (actionEdge(ACT_IDLE_ORBIT, down[ACT_IDLE_ORBIT])) {
        settings.idleOrbitEnabled = !settings.idleOrbitEnabled;
        persist = true;
        orbitOffsetX = orbitOffsetY = orbitScaleAdd = 0.0f;
        pushToast(settings.idleOrbitEnabled ? "ORBIT ON" : "ORBIT OFF");
        used = true;
    }
    if (actionEdge(ACT_PANEL, down[ACT_PANEL])) {
        if (!settings.showSystemPanel) {
            settings.showSystemPanel = true;
            systemPanelPage = 0;
            pushToast("SYS 1/3");
        } else if (systemPanelPage + 1 < SYSTEM_PANEL_PAGE_COUNT) {
            ++systemPanelPage;
            if (systemPanelPage == 1) pushToast("RND 2/3");
            else pushToast("SET 3/3");
        } else {
            settings.showSystemPanel = false;
            systemPanelPage = 0;
            pushToast("SYS OFF");
        }
        used = true;
    }
    if (actionEdge(ACT_STOPWATCH, down[ACT_STOPWATCH])) {
        if (!settings.showStopwatchPanel) {
            settings.showStopwatchPanel = true;
            setWatchMode(0);
            pushToast("STOPWATCH");
        } else if (stopwatchState.mode == 0) {
            setWatchMode(1);
            settings.showStopwatchPanel = true;
            pushToast("TIMER");
        } else {
            settings.showStopwatchPanel = false;
            pushToast("WATCH OFF");
        }
        used = true;
    }
    if (actionEdge(ACT_STOPWATCH_RUN, down[ACT_STOPWATCH_RUN])) {
        toggleStopwatchRun();
        settings.showStopwatchPanel = true;
        pushToast(stopwatchState.running ? "WATCH RUN" : "WATCH STOP");
        used = true;
    }
    if (actionEdge(ACT_STOPWATCH_RESET, down[ACT_STOPWATCH_RESET])) {
        resetStopwatch();
        settings.showStopwatchPanel = true;
        pushToast(stopwatchState.mode == 1 ? "TIMER RESET" : "WATCH RESET");
        used = true;
    }

    if (actionRepeat(ACT_SPEED_UP, down[ACT_SPEED_UP], now)) {
        settings.rotationSpeed += 0.10f;
        if (settings.rotationSpeed > 4.0f) settings.rotationSpeed = 4.0f;
        persist = true;
        pushToast("SPEED UP");
        used = true;
    }
    if (actionRepeat(ACT_SPEED_DOWN, down[ACT_SPEED_DOWN], now)) {
        settings.rotationSpeed -= 0.10f;
        if (settings.rotationSpeed < 0.20f) settings.rotationSpeed = 0.20f;
        persist = true;
        pushToast("SPEED DOWN");
        used = true;
    }
    if (settings.showStopwatchPanel && stopwatchState.mode == 1) {
        if (actionRepeat(ACT_SCALE_UP, down[ACT_SCALE_UP], now)) {
            if (adjustTimerTarget((int32_t)TIMER_STEP_MS)) pushToast("TIMER +1M");
            used = true;
        }
        if (actionRepeat(ACT_SCALE_DOWN, down[ACT_SCALE_DOWN], now)) {
            if (adjustTimerTarget(-(int32_t)TIMER_STEP_MS)) pushToast("TIMER -1M");
            used = true;
        }
    } else {
        if (actionRepeat(ACT_SCALE_UP, down[ACT_SCALE_UP], now)) {
            settings.donutScale += 2.0f;
            if (settings.donutScale > 112.0f) settings.donutScale = 112.0f;
            snprintf(msg, sizeof(msg), "SCALE %u", (unsigned)settings.donutScale);
            persist = true;
            pushToast(msg);
            used = true;
        }
        if (actionRepeat(ACT_SCALE_DOWN, down[ACT_SCALE_DOWN], now)) {
            settings.donutScale -= 2.0f;
            if (settings.donutScale < 70.0f) settings.donutScale = 70.0f;
            snprintf(msg, sizeof(msg), "SCALE %u", (unsigned)settings.donutScale);
            persist = true;
            pushToast(msg);
            used = true;
        }
    }
    if (actionRepeat(ACT_BRIGHT_DOWN, down[ACT_BRIGHT_DOWN], now)) {
        settings.brightness = settings.brightness > 10 ? (uint8_t)(settings.brightness - 10) : 1;
        M5Cardputer.Display.setBrightness(settings.brightness);
        persist = true;
        pushToast("BRIGHT DOWN");
        used = true;
    }
    if (actionRepeat(ACT_BRIGHT_UP, down[ACT_BRIGHT_UP], now)) {
        settings.brightness = settings.brightness < 245 ? (uint8_t)(settings.brightness + 10) : 255;
        M5Cardputer.Display.setBrightness(settings.brightness);
        persist = true;
        pushToast("BRIGHT UP");
        used = true;
    }
    if (actionRepeat(ACT_VIEW_UP, down[ACT_VIEW_UP], now)) {
        manualOffsetY = clampFloat(manualOffsetY - VIEW_STEP, -VIEW_LIMIT_Y, VIEW_LIMIT_Y);
        pushToast("VIEW UP");
        used = true;
    }
    if (actionRepeat(ACT_VIEW_LEFT, down[ACT_VIEW_LEFT], now)) {
        manualOffsetX = clampFloat(manualOffsetX - VIEW_STEP, -VIEW_LIMIT_X, VIEW_LIMIT_X);
        pushToast("VIEW LEFT");
        used = true;
    }
    if (actionRepeat(ACT_VIEW_DOWN, down[ACT_VIEW_DOWN], now)) {
        manualOffsetY = clampFloat(manualOffsetY + VIEW_STEP, -VIEW_LIMIT_Y, VIEW_LIMIT_Y);
        pushToast("VIEW DOWN");
        used = true;
    }
    if (actionRepeat(ACT_VIEW_RIGHT, down[ACT_VIEW_RIGHT], now)) {
        manualOffsetX = clampFloat(manualOffsetX + VIEW_STEP, -VIEW_LIMIT_X, VIEW_LIMIT_X);
        pushToast("VIEW RIGHT");
        used = true;
    }
    if (actionRepeat(ACT_BG_DEPTH_DOWN, down[ACT_BG_DEPTH_DOWN], now)) {
        if (settings.bgDepthLevel > 0) {
            --settings.bgDepthLevel;
            bgCacheDirty = true;
            persist = true;
            snprintf(msg, sizeof(msg), "BG DEPTH %u", (unsigned)settings.bgDepthLevel);
            pushToast(msg);
        }
        used = true;
    }
    if (actionRepeat(ACT_BG_DEPTH_UP, down[ACT_BG_DEPTH_UP], now)) {
        if (settings.bgDepthLevel < 2) {
            ++settings.bgDepthLevel;
            bgCacheDirty = true;
            persist = true;
            snprintf(msg, sizeof(msg), "BG DEPTH %u", (unsigned)settings.bgDepthLevel);
            pushToast(msg);
        }
        used = true;
    }

    if (persist) markSettingsDirty();
    if (used) lastInputMs = now;
    for (int i = 0; i < ACT_COUNT; ++i) actionWasDown[i] = down[i];
}

// -----------------------------------------------------------------------------
// Timing

static void updateAngles() {
    if (settings.paused) return;
    rotateUnitFast(rotCA, rotSA, ROT_STEP_A * settings.rotationSpeed);
    rotateUnitFast(rotCB, rotSB, ROT_STEP_B * settings.rotationSpeed);
}

static void updateMotionState() {
    if (settings.paused) return;

    if (settings.motionProfile == 0 || !settings.idleOrbitEnabled) {
        orbitBlendQ8 = 0;
        orbitOffsetX = 0.0f;
        orbitOffsetY = 0.0f;
        orbitScaleAdd = 0.0f;
        return;
    }

    bool idle = (millis() - lastInputMs > IDLE_ORBIT_MS);

    // Smooth the idle-orbit transition.  Earlier builds jumped when the device
    // crossed IDLE_ORBIT_MS because orbitOffsetX/Y were enabled in a single frame.
    // Use a tiny Q8 fade so the torus glides into/out of the idle orbit.
    if (idle) {
        uint16_t next = (uint16_t)orbitBlendQ8 + (settings.motionProfile == 2 ? 10 : 7);
        orbitBlendQ8 = next > 255 ? 255 : (uint8_t)next;
        uint32_t step = settings.motionProfile == 2 ? 430UL : 230UL;
        orbitPhaseFx += step;  // 32-bit phase prevents periodic idle-orbit snaps.
    } else {
        if (orbitBlendQ8 > 0) {
            orbitBlendQ8 = orbitBlendQ8 > 18 ? (uint8_t)(orbitBlendQ8 - 18) : 0;
        }
        if (orbitBlendQ8 == 0) {
            orbitOffsetX = 0.0f;
            orbitOffsetY = 0.0f;
            orbitScaleAdd = 0.0f;
            return;
        }
    }

    int idx = (int)((orbitPhaseFx >> 10) % STD_PHI);
    int idx2 = (idx + STD_PHI / 4) % STD_PHI;
    float blend = (float)orbitBlendQ8 * (1.0f / 255.0f);
    float amp = settings.motionProfile == 2 ? 7.0f : 3.5f;
    orbitOffsetX = sinPhiStd[idx] * amp * blend;
    orbitOffsetY = cosPhiStd[idx2] * (amp * 0.45f) * blend;
    orbitScaleAdd = sinPhiStd[(idx + STD_PHI / 2) % STD_PHI] * (settings.motionProfile == 2 ? 2.0f : 1.0f) * blend;
}

static void updateFpsCounter() {
    ++fpsFrames;
    uint32_t now = millis();
    uint32_t elapsed = now - fpsLastMs;
    if (elapsed >= 600) {
        measuredFps = (float)fpsFrames * 1000.0f / (float)elapsed;
        fpsFrames = 0;
        fpsLastMs = now;
    }
}

static void limitFrameRate(uint32_t frameStartUs) {
    uint8_t target = FPS_TARGETS[settings.frameTargetIndex];
    if (target == 0) return;

    uint32_t targetUs = 1000000UL / (uint32_t)target;
    while ((uint32_t)(micros() - frameStartUs) + 1200UL < targetUs) delay(1);
    uint32_t elapsed = (uint32_t)(micros() - frameStartUs);
    if (elapsed < targetUs) delayMicroseconds(targetUs - elapsed);
}

// -----------------------------------------------------------------------------
// Setup and loop

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    setCpuFrequencyMhz(240);
    loadPersistentSettings();

    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(settings.brightness);

    canvas.setColorDepth(16);
    canvas.createSprite(SCREEN_W, SCREEN_H);
    canvasBuf = (uint16_t*)canvas.getBuffer();

    if (!canvasBuf) {
        M5Cardputer.Display.fillScreen(M5Cardputer.Display.color565(80, 0, 12));
        M5Cardputer.Display.setTextColor(M5Cardputer.Display.color565(255, 235, 235));
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(8, 58);
        M5Cardputer.Display.print("DONUT.OS canvas alloc failed");
        while (true) delay(1000);
    }

    buildTrigTable(STD_THETA, sinThetaStd, cosThetaStd);
    buildTrigTable(STD_PHI, sinPhiStd, cosPhiStd);
    buildTrigTable(HQ_THETA, sinThetaHq, cosThetaHq);
    buildTrigTable(HQ_PHI, sinPhiHq, cosPhiHq);
    resetRotationState();

    startMs = millis();
    stopwatchState.startMs = startMs;
    stopwatchState.timerTargetMs = TIMER_DEFAULT_MS;
    lastInputMs = startMs;
    fpsLastMs = startMs;
    rebuildPalettes();
    bgCacheDirty = true;

    canvas.fillScreen(0);
    canvas.setSwapBytes(false);
    canvas.pushSprite(0, 0);
}

void loop() {
    uint32_t frameStartUs = micros();

    uint32_t in0 = micros();
    M5Cardputer.update();
    handleInput();
    updateWatchState();
    servicePersistentSettings();
    inputUs = micros() - in0;

    if (paletteDirty) {
        rebuildPalettes();
        bgCacheDirty = true;
    }

    uint32_t mo0 = micros();
    updateAngles();
    updateMotionState();
    updateBackgroundAnimationState();
    motionUs = micros() - mo0;

    updateBackgroundCacheIfNeeded();
    copyBackgroundCacheToCanvas();

    uint32_t fx0 = micros();
    drawBackgroundFxOverlay(canvasBuf);
    bgFxUs = micros() - fx0;

    if (settings.floatingTextMode != 0) drawBehindTextAndHints();

    clearRenderBuffers();
    primitiveCount = 0;
    culledCount = 0;
    rasterizedCount = 0;
    fillPixelCount = 0;
    uint32_t t1 = micros();
    renderDonutSolidMesh();
    uint32_t t2 = micros();

#if ENABLE_HOT_PIXEL_SUPPRESS
    suppressHotPixelsDirtyOnly();
#endif
    if (settings.glowEnabled) {
        uint32_t g0 = micros();
        buildGlowDirtyOnly();
        glowUs = micros() - g0;
    } else {
        glowUs = 0;
    }
    composeDonutDirtyOnly(canvasBuf);
    uint32_t t3 = micros();

    donutTimeUs = t2 - t1;
    composeTimeUs = t3 - t2;

    uint32_t ui0 = micros();
    drawBootOverlay();
    if (settings.showSystemPanel) drawSystemPanel();
    drawStopwatchPanel();
    drawHelpPanel();
    drawToast();
    drawTopBar();
    uiUs = micros() - ui0;

    canvas.setSwapBytes(false);
    uint32_t p0 = micros();
    canvas.pushSprite(0, 0);
    pushTimeUs = micros() - p0;

    ++frameNumber;
    updateFpsCounter();
    limitFrameRate(frameStartUs);
}
