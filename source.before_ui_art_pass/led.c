#include "led.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
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
    "Cor solida",
    "Pulso",
    "Desligado",
};

static int s_mode = LED_RAINBOW;
static int s_selected = 0;
static float s_selY = 0.0f;
static int s_speed = 2;
static u8 s_r = 255;
static u8 s_g = 96;
static u8 s_b = 160;
static bool s_mcuReady = false;
static Result s_lastResult = 0;

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

static void setValueFromTouch(int* value, int min, int max, touchPosition* touch, float barX, float barW) {
    float x = clampf((float)touch->px, barX, barX + barW);
    float t = (x - barX) / barW;
    *value = clampi((int)(min + (max - min) * t + 0.5f), min, max);
}

void ledUpdate(u32 kDown, u32 kHeld, touchPosition* touch, bool touchDown, int* currentScreen) {
    (void)kHeld;
    const int total = LED_MODE_COUNT + 4;

    if (kDown & KEY_B) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (kDown & KEY_DOWN) s_selected = (s_selected + 1) % total;
    if (kDown & KEY_UP) s_selected = (s_selected - 1 + total) % total;

    if (touchDown) {
        for (int i = 0; i < LED_MODE_COUNT; i++) {
            if (touchIn(touch, 10, 38 + i * 28, 145, 24)) {
                s_selected = i;
                setMode(i);
            }
        }
        for (int i = 0; i < 3; i++) {
            if (touchIn(touch, 10, 146 + i * 28, 300, 24)) {
                s_selected = LED_MODE_COUNT + i;
                int value = (i == 0) ? s_r : (i == 1) ? s_g : (i == 2) ? s_b : s_speed;
                setValueFromTouch(&value, 0, 255, touch, 100.0f, 162.0f);
                if (i == 0) s_r = (u8)value;
                if (i == 1) s_g = (u8)value;
                if (i == 2) s_b = (u8)value;
                applyLed();
                saveLed();
            }
        }
        if (touchIn(touch, 164, 122, 146, 24)) {
            s_selected = LED_MODE_COUNT + 3;
            int value = s_speed;
            setValueFromTouch(&value, 1, 5, touch, 226.0f, 44.0f);
            s_speed = value;
            applyLed();
            saveLed();
        }
        if (touchIn(touch, 8, 206, 78, 26)) {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        }
    }

    if (kDown & KEY_A && s_selected < LED_MODE_COUNT) {
        setMode(s_selected);
    }

    int delta = 0;
    if (kDown & KEY_RIGHT) delta = 1;
    if (kDown & KEY_LEFT) delta = -1;
    if (delta != 0 && s_selected >= LED_MODE_COUNT) {
        int step = (s_selected == LED_MODE_COUNT + 3) ? 1 : 8;
        int* value = NULL;
        int tmp = 0;
        int max = 255;
        if (s_selected == LED_MODE_COUNT + 0) { tmp = s_r; value = &tmp; }
        if (s_selected == LED_MODE_COUNT + 1) { tmp = s_g; value = &tmp; }
        if (s_selected == LED_MODE_COUNT + 2) { tmp = s_b; value = &tmp; }
        if (s_selected == LED_MODE_COUNT + 3) { tmp = s_speed; value = &tmp; max = 5; }
        if (value) {
            *value = clampi(*value + delta * step, s_selected == LED_MODE_COUNT + 3 ? 1 : 0, max);
            if (s_selected == LED_MODE_COUNT + 0) s_r = (u8)*value;
            if (s_selected == LED_MODE_COUNT + 1) s_g = (u8)*value;
            if (s_selected == LED_MODE_COUNT + 2) s_b = (u8)*value;
            if (s_selected == LED_MODE_COUNT + 3) s_speed = *value;
            applyLed();
            saveLed();
        }
    }
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
    UI_HeaderTop(buf, fontsCurrent(), "LED RGB", "preview do efeito antes de aplicar");

    UI_Panel(22, 56, 356, 132, g_theme.surface, g_theme.divider);
    UI_Text(buf, fontsCurrent(), MODE_NAMES[s_mode], 40, 74, 0.50f, 0.50f, g_theme.textPrimary);
    UI_Text(buf, NULL, s_mcuReady ? "MCU pronto para aplicar no LED real" : "MCU indisponivel aqui, preview visual ativo",
            40, 104, 0.24f, 0.24f, s_mcuReady ? g_theme.success : g_theme.warning);

    ColorRGBA c = previewColor();
    UI_Panel(266, 78, 64, 64, g_theme.backgroundTop, g_theme.divider);
    UI_Fill(278, 90, 40, 40, c);
    UI_Fill(288, 100, 20, 20, themeMix(c, g_theme.onPrimary, 0.28f));

    char rgb[40];
    snprintf(rgb, sizeof(rgb), "rgb(%d, %d, %d)  speed %d", s_r, s_g, s_b, s_speed);
    UI_Text(buf, NULL, rgb, 40, 150, 0.25f, 0.25f, g_theme.textSecondary);

    if (R_FAILED(s_lastResult)) {
        char err[32];
        snprintf(err, sizeof(err), "ultimo erro: 0x%08lX", (unsigned long)s_lastResult);
        UI_Text(buf, NULL, err, 40, 170, 0.22f, 0.22f, g_theme.textHint);
    }
}

void ledRenderBottom(C2D_TextBuf buf) {
    UI_TouchChrome(buf, fontsCurrent(), "led", "modos e sliders");

    float targetY = (s_selected < LED_MODE_COUNT)
        ? 38.0f + s_selected * 28.0f
        : 146.0f + (s_selected - LED_MODE_COUNT) * 28.0f;
    s_selY += (targetY - s_selY) * 0.18f;
    float selH = 24.0f;
    if (s_selY + selH > SCREEN_BOT_HEIGHT - 24) s_selY = (float)(SCREEN_BOT_HEIGHT - 24 - (int)selH);
    if (s_selY < 38.0f) s_selY = 38.0f;
    UI_RoundRect(8, s_selY, (s_selected < LED_MODE_COUNT) ? 149 : 304, selH, 4,
                 themeMix(g_theme.surface, g_theme.primary, 0.12f));

    for (int i = 0; i < LED_MODE_COUNT; i++) {
        UI_Button(buf, NULL, 10, 38 + i * 28, 145, 24, MODE_NAMES[i],
                  s_mode == i ? "ON" : NULL,
                  s_selected == i ? UI_BUTTON_SELECTED : (s_mode == i ? UI_BUTTON_ACTIVE : UI_BUTTON_NORMAL));
    }

    UI_Swatch(178, 40, 34, 34, (ColorRGBA){s_r, 0, 0, 255}, s_selected == LED_MODE_COUNT);
    UI_Swatch(220, 40, 34, 34, (ColorRGBA){0, s_g, 0, 255}, s_selected == LED_MODE_COUNT + 1);
    UI_Swatch(262, 40, 34, 34, (ColorRGBA){0, 0, s_b, 255}, s_selected == LED_MODE_COUNT + 2);
    UI_Swatch(220, 82, 34, 34, previewColor(), false);

    UI_Slider(buf, NULL, 10, 146, 300, "R", s_r, 0, 255, s_selected == LED_MODE_COUNT);
    UI_Slider(buf, NULL, 10, 174, 300, "G", s_g, 0, 255, s_selected == LED_MODE_COUNT + 1);
    UI_Slider(buf, NULL, 10, 202, 300, "B", s_b, 0, 255, s_selected == LED_MODE_COUNT + 2);

    UI_Slider(buf, NULL, 164, 122, 146, "Vel", s_speed, 1, 5, s_selected == LED_MODE_COUNT + 3);
}
