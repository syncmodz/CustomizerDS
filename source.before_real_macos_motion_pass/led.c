#include "led.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "config.h"
#include "fonts.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <3ds/services/mcuhwc.h>

typedef enum {
    LED_RAINBOW = 0,
    LED_SOLID,
    LED_PULSE,
    LED_MODE_OFF,
    LED_MODE_COUNT
} LedMode;

static const char* MODE_NAMES[] = {
    "Arco-iris",
    "Solida",
    "Pulso",
    "Off",
};

static int s_mode = LED_RAINBOW;
static int s_selected = 0;
static int s_speed = 2;
static u8 s_r = 255;
static u8 s_g = 96;
static u8 s_b = 160;
static bool s_mcuReady = false;
static Result s_lastResult = 0;

static float s_segSlideT = 1.0f;

typedef enum {
    PART_MODE = 0,
    PART_SELECTED_CHANNEL,
    PART_SPEED,
} BottomPart;

static int s_activeChannel = 0;

static u8 scaleU8(u8 value, float mul) {
    int out = (int)((float)value * mul);
    return (u8)clampi(out, 0, 255);
}

static void hsvToRgb(float h, float s, float v, u8* r, u8* g, u8* b) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rr = 0, gg = 0, bb = 0;

    if (h < 60) { rr = c; gg = x; bb = 0; }
    else if (h < 120) { rr = x; gg = c; bb = 0; }
    else if (h < 180) { rr = 0; gg = c; bb = x; }
    else if (h < 240) { rr = 0; gg = x; bb = c; }
    else if (h < 300) { rr = x; gg = 0; bb = c; }
    else { rr = c; gg = 0; bb = x; }

    *r = (u8)((rr + m) * 255.0f);
    *g = (u8)((gg + m) * 255.0f);
    *b = (u8)((bb + m) * 255.0f);
}

static Result setPatternSolid(u8 r, u8 g, u8 b) {
    InfoLedPattern pattern;
    memset(&pattern, 0, sizeof(pattern));
    pattern.delay = 0x10;
    pattern.smoothing = 0x00;
    pattern.loopDelay = 0xFF;
    pattern.blinkSpeed = 0x00;
    memset(pattern.redPattern, r, 32);
    memset(pattern.greenPattern, g, 32);
    memset(pattern.bluePattern, b, 32);
    return MCUHWC_SetInfoLedPattern(&pattern);
}

static Result setPatternRainbow(void) {
    InfoLedPattern pattern;
    memset(&pattern, 0, sizeof(pattern));
    pattern.delay = (u8)clampi(0x1C - s_speed * 4, 4, 28);
    pattern.smoothing = 0x00;
    pattern.loopDelay = 0x00;
    pattern.blinkSpeed = 0x00;

    for (int i = 0; i < 32; i++) {
        u8 r, g, b;
        hsvToRgb((float)i * 360.0f / 32.0f, 1.0f, 1.0f, &r, &g, &b);
        pattern.redPattern[i] = r;
        pattern.greenPattern[i] = g;
        pattern.bluePattern[i] = b;
    }

    return MCUHWC_SetInfoLedPattern(&pattern);
}

static Result setPatternPulse(void) {
    InfoLedPattern pattern;
    memset(&pattern, 0, sizeof(pattern));
    pattern.delay = (u8)clampi(0x1E - s_speed * 4, 4, 30);
    pattern.smoothing = 0x01;
    pattern.loopDelay = 0x00;
    pattern.blinkSpeed = 0x00;

    for (int i = 0; i < 32; i++) {
        float t = (float)i / 31.0f;
        float wave = 0.20f + 0.80f * (sinf(t * 6.2831853f) * 0.5f + 0.5f);
        pattern.redPattern[i] = scaleU8(s_r, wave);
        pattern.greenPattern[i] = scaleU8(s_g, wave);
        pattern.bluePattern[i] = scaleU8(s_b, wave);
    }

    return MCUHWC_SetInfoLedPattern(&pattern);
}

static void saveLed(void) {
    ConfigData cfg;
    configLoad(&cfg);
    cfg.ledMode = (u8)s_mode;
    cfg.ledSpeed = (u8)s_speed;
    cfg.ledR = s_r;
    cfg.ledG = s_g;
    cfg.ledB = s_b;
    configSave(&cfg);
}

static void applyLed(void) {
    if (!s_mcuReady) return;

    switch (s_mode) {
        case LED_RAINBOW: s_lastResult = setPatternRainbow(); break;
        case LED_SOLID: s_lastResult = setPatternSolid(s_r, s_g, s_b); break;
        case LED_PULSE: s_lastResult = setPatternPulse(); break;
        case LED_MODE_OFF: s_lastResult = setPatternSolid(0, 0, 0); break;
        default: break;
    }
}

void ledInit(void) {
    ConfigData cfg;
    configLoad(&cfg);
    s_mode = clampi(cfg.ledMode, 0, LED_MODE_COUNT - 1);
    s_speed = clampi(cfg.ledSpeed, 1, 5);
    s_r = cfg.ledR;
    s_g = cfg.ledG;
    s_b = cfg.ledB;
    s_selected = 0;
    s_lastResult = mcuHwcInit();
    s_mcuReady = R_SUCCEEDED(s_lastResult);
}

void ledExit(void) {
    if (s_mcuReady) {
        mcuHwcExit();
        s_mcuReady = false;
    }
}

static void setMode(int mode) {
    s_mode = clampi(mode, 0, LED_MODE_COUNT - 1);
    applyLed();
    saveLed();
}

void ledUpdate(u32 kDown, u32 kHeld, touchPosition* touch, bool touchDown, int* currentScreen) {
    (void)kHeld;

    if (kDown & KEY_B) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    int total = 2 + (s_mode == LED_SOLID || s_mode == LED_PULSE ? 3 : 0);

    if (kDown & KEY_DOWN) s_selected = (s_selected + 1) % (total > 0 ? total : 1);
    if (kDown & KEY_UP) s_selected = (s_selected - 1 + (total > 0 ? total : 1)) % (total > 0 ? total : 1);

    if (touchDown) {
        if (touchIn(touch, 10, 36, 300, 24)) {
            int segW = 300 / LED_MODE_COUNT;
            int newMode = ((int)touch->px - 10) / segW;
            newMode = clampi(newMode, 0, LED_MODE_COUNT - 1);
            s_segSlideT = 0.0f;
            s_selected = 0;
            setMode(newMode);
            return;
        }

        if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
            float swY = 70.0f;
            float swSize = 24.0f;
            float gap = 8.0f;
            float startX = 12.0f;
            for (int i = 0; i < 3; i++) {
                float sx = startX + i * (swSize + gap);
                if (touchIn(touch, sx, swY, swSize, swSize)) {
                    s_selected = 1 + i;
                    s_activeChannel = i;
                    return;
                }
            }

            for (int i = 0; i < 3; i++) {
                float slY = 112.0f + i * 28.0f;
                float barX = 108.0f;
                float barW = 200.0f;
                if (touchIn(touch, barX, slY, barW, 20)) {
                    s_selected = 1 + i;
                    s_activeChannel = i;
                    float t = clampf(((float)touch->px - barX) / barW, 0.0f, 1.0f);
                    int val = (int)(t * 255.0f + 0.5f);
                    val = clampi(val, 0, 255);
                    if (i == 0) s_r = (u8)val;
                    if (i == 1) s_g = (u8)val;
                    if (i == 2) s_b = (u8)val;
                    applyLed();
                    saveLed();
                    return;
                }
            }

            float spY = 200.0f;
            if (touchIn(touch, 108, spY, 200, 20)) {
                s_selected = 4;
                float t = clampf(((float)touch->px - 108.0f) / 200.0f, 0.0f, 1.0f);
                s_speed = clampi((int)(1 + t * 4.0f + 0.5f), 1, 5);
                applyLed();
                saveLed();
                return;
            }
        } else {
            float spY = 112.0f;
            if (touchIn(touch, 108, spY, 200, 20)) {
                s_selected = 4;
                float t = clampf(((float)touch->px - 108.0f) / 200.0f, 0.0f, 1.0f);
                s_speed = clampi((int)(1 + t * 4.0f + 0.5f), 1, 5);
                applyLed();
                saveLed();
                return;
            }
        }
    }

    if (kDown & KEY_A) {
        if (s_selected == 0) { setMode(s_mode == LED_MODE_COUNT - 1 ? 0 : s_mode + 1); }
        else if (s_selected >= 1 && s_selected <= 3 && (s_mode == LED_SOLID || s_mode == LED_PULSE)) {
            u8* ch = (s_selected == 1) ? &s_r : (s_selected == 2) ? &s_g : &s_b;
            *ch = (u8)((*ch + 16) % 256);
            applyLed();
            saveLed();
        }
    }

    s_segSlideT = fminf(s_segSlideT + 1.0f/60.0f, 1.0f);
}

static ColorRGBA previewColor(void) {
    if (s_mode == LED_MODE_OFF) return (ColorRGBA){24, 24, 32, 255};
    if (s_mode == LED_SOLID) return (ColorRGBA){s_r, s_g, s_b, 255};
    if (s_mode == LED_PULSE) {
        float wave = 0.25f + 0.75f * (sinf(uiFrameTime() * (1.2f + s_speed)) * 0.5f + 0.5f);
        return (ColorRGBA){scaleU8(s_r, wave), scaleU8(s_g, wave), scaleU8(s_b, wave), 255};
    }
    u8 r, g, b;
    float h = fmodf(uiFrameTime() * (45.0f + s_speed * 25.0f), 360.0f);
    hsvToRgb(h, 1.0f, 1.0f, &r, &g, &b);
    return (ColorRGBA){r, g, b, 255};
}

void ledRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();

    float slideUp = (1.0f - easeOutCubic(g_enterT)) * 8.0f;

    UI_Card(20, 46 + slideUp, 360, 152, 12, g_theme.surface);

    UI_Text(buf, fontsCurrent(), MODE_NAMES[s_mode], 32, 62 + slideUp, 0.52f, 0.52f, g_theme.textPrimary);

    UI_Text(buf, NULL, s_mcuReady ? "MCU pronto" : "MCU indisponivel",
            32, 86 + slideUp, 0.24f, 0.24f, s_mcuReady ? g_theme.success : g_theme.warning);

    ColorRGBA c = previewColor();
    UI_RoundRect(272, 62 + slideUp, 72, 72, 12, g_theme.backgroundTop);
    UI_RoundRect(280, 70 + slideUp, 56, 56, 10, c);
    UI_RoundRect(288, 78 + slideUp, 40, 40, 8, themeMix(c, g_theme.onPrimary, 0.20f));

    char rgb[40];
    snprintf(rgb, sizeof(rgb), "R%d G%d B%d  vel %d", s_r, s_g, s_b, s_speed);
    UI_Text(buf, NULL, rgb, 32, 114 + slideUp, 0.28f, 0.28f, g_theme.textSecondary);

    if (R_FAILED(s_lastResult)) {
        char err[32];
        snprintf(err, sizeof(err), "erro: 0x%08lX", (unsigned long)s_lastResult);
        UI_Text(buf, NULL, err, 32, 138 + slideUp, 0.22f, 0.22f, g_theme.textHint);
    } else {
        UI_Text(buf, NULL, "toque nos controles abaixo para ajustar", 32, 138 + slideUp, 0.24f, 0.24f, g_theme.textHint);
    }

    UI_Pill(260, 166 + slideUp, 86, 22, g_theme.primary, g_theme.onPrimary, buf, NULL, s_mcuReady ? "ao vivo" : "preview", 0.30f, 0.30f);
}

static void ledSlider(C2D_TextBuf buf, const char* label, float x, float y, float w,
                       int value, int min, int max, bool selected, ColorRGBA swatchColor) {
    ColorRGBA bg = selected ? themeMix(g_theme.surface, swatchColor, 0.08f) : g_theme.surface;
    UI_RoundRect(x, y, w, 22, 6, bg);

    UI_RoundRect(x + 36, y + 3, w - 96, 16, 8, g_theme.divider);

    float t = (max > min) ? (float)(value - min) / (float)(max - min) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);
    float barW = (w - 96) * t;
    if (barW > 0) {
        UI_RoundRect(x + 36, y + 3, barW, 16, 8, swatchColor);
    }

    UI_RoundRect(x + 34 + (w - 96) * t, y - 1, 10, 24, 5, selected ? g_theme.onPrimary : g_theme.textPrimary);

    char valStr[8];
    snprintf(valStr, sizeof(valStr), "%d", value);
    UI_TextRight(buf, NULL, valStr, x + w - 4, y + 3, 0.24f, 0.24f, g_theme.textSecondary);
    UI_Text(buf, NULL, label, x + 6, y + 3, 0.26f, 0.26f, g_theme.textPrimary);
}

void ledRenderBottom(C2D_TextBuf buf) {
    UI_BackgroundBottom();

    UI_SegmentedControl(buf, NULL, 10, 36, 300, 24, MODE_NAMES, LED_MODE_COUNT, s_mode, s_segSlideT);

    if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
        float swY = 70.0f;
        float swSize = 22.0f;
        float gap = 8.0f;
        float startX = 12.0f;
        ColorRGBA colors[] = {
            {s_r, 0, 0, 255},
            {0, s_g, 0, 255},
            {0, 0, s_b, 255},
        };
        for (int i = 0; i < 3; i++) {
            float sx = startX + i * (swSize + gap);
            bool sel = (s_selected == 1 + i);
            float pulse = sel ? 0.05f * sinf(uiFrameTime() * 4.0f) + 0.05f : 0.0f;
            UI_RoundSwatch(sx + 2, swY + 2, swSize * 0.5f, colors[i], sel, pulse);
        }

        UI_RoundRect(88, swY + 2, 22, 22, 6, previewColor());

        ledSlider(buf, "R", 10, 106, 300, s_r, 0, 255, s_selected == 1, (ColorRGBA){s_r, 0, 0, 255});
        ledSlider(buf, "G", 10, 134, 300, s_g, 0, 255, s_selected == 2, (ColorRGBA){0, s_g, 0, 255});
        ledSlider(buf, "B", 10, 162, 300, s_b, 0, 255, s_selected == 3, (ColorRGBA){0, 0, s_b, 255});

        ledSlider(buf, "Vel", 10, 192, 300, s_speed, 1, 5, s_selected == 4, g_theme.accent);
    } else {
        ledSlider(buf, "Velocidade", 10, 106, 300, s_speed, 1, 5, s_selected == 4, g_theme.accent);
    }
}
