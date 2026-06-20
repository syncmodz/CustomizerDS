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
    "RGB",
    "Fixo",
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
static float s_pressedT = 0.0f;

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

    s_pressedT = animApproach(s_pressedT, 0.0f, 0.30f);

    if (touchDown) {
        if (touchIn(touch, 10, 34, 300, 26)) {
            int segW = 300 / LED_MODE_COUNT;
            int newMode = ((int)touch->px - 10) / segW;
            newMode = clampi(newMode, 0, LED_MODE_COUNT - 1);
            s_segSlideT = 0.0f;
            s_selected = 0;
            s_pressedT = 1.0f;
            setMode(newMode);
            return;
        }

        if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
            float swY = 72.0f;
            float swSize = 28.0f;
            float gap = 10.0f;
            float startX = 16.0f;
            for (int i = 0; i < 3; i++) {
                float sx = startX + i * (swSize + gap);
                if (touchIn(touch, sx, swY, swSize, swSize)) {
                    s_selected = 1 + i;
                    s_activeChannel = i;
                    return;
                }
            }

            for (int i = 0; i < 3; i++) {
                float slY = 112.0f + i * 30.0f;
                float barX = 90.0f;
                float barW = 220.0f;
                if (touchIn(touch, barX, slY, barW, 22)) {
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

            float spY = 198.0f;
            if (touchIn(touch, 90, spY, 220, 22)) {
                s_selected = 4;
                float t = clampf(((float)touch->px - 90.0f) / 220.0f, 0.0f, 1.0f);
                s_speed = clampi((int)(1 + t * 4.0f + 0.5f), 1, 5);
                applyLed();
                saveLed();
                return;
            }
        } else {
            float spY = 106.0f;
            if (touchIn(touch, 90, spY, 220, 22)) {
                s_selected = 4;
                float t = clampf(((float)touch->px - 90.0f) / 220.0f, 0.0f, 1.0f);
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

    float et = easeOutCubic(g_enterT);
    float slideUp = (1.0f - et) * 10.0f;

    UI_GlassPanel(20, 40 + slideUp, 360, 160, 14,
                  g_theme.surface,
                  (ColorRGBA){255, 255, 255, 20},
                  (ColorRGBA){255, 255, 255, 12});

    UI_Text(buf, fontsCurrent(), MODE_NAMES[s_mode], 32, 54 + slideUp, 0.50f, 0.50f, g_theme.textPrimary);

    UI_Text(buf, NULL, s_mcuReady ? "MCU pronto - led ao vivo" : "MCU indisponivel - preview apenas",
            32, 78 + slideUp, 0.24f, 0.24f, s_mcuReady ? g_theme.success : g_theme.warning);

    ColorRGBA c = previewColor();

    float lx = 270.0f;
    float ly = 54 + slideUp;
    float lw = 82;
    float lh = 60;

    UI_RoundRect(lx, ly, lw, lh, 12, (ColorRGBA){8, 10, 14, 255});
    UI_RoundRect(lx + 6, ly + 6, lw - 12, lh - 12, 8, c);
    ColorRGBA glow = c;
    glow.a = 40;
    UI_RoundRect(lx + 2, ly + 2, lw - 4, lh - 4, 10, glow);

    char rgb[40];
    snprintf(rgb, sizeof(rgb), "R%d G%d B%d  vel %d", s_r, s_g, s_b, s_speed);
    UI_Text(buf, NULL, rgb, 32, 104 + slideUp, 0.28f, 0.28f, g_theme.textSecondary);

    if (R_FAILED(s_lastResult)) {
        char err[32];
        snprintf(err, sizeof(err), "erro: 0x%08lX", (unsigned long)s_lastResult);
        UI_Text(buf, NULL, err, 32, 128 + slideUp, 0.22f, 0.22f, g_theme.textHint);
    } else {
        UI_Text(buf, NULL, "toque nos controles abaixo para ajustar", 32, 128 + slideUp, 0.24f, 0.24f, g_theme.textHint);
    }

    float previewT = clampf(et * 2.0f, 0.0f, 1.0f);
    float py = lerpf(150 + slideUp, 146 + slideUp, previewT);
    UI_Text(buf, NULL, s_mcuReady ? "led atualizando em tempo real" : "simulacao de led na tela",
            32, py, 0.22f, 0.22f, g_theme.textHint);
}

static void ledSliderRow(C2D_TextBuf buf, const char* label, float x, float y, float w,
                         int value, int min, int max, bool selected, ColorRGBA swatchColor) {
    ColorRGBA bg = (ColorRGBA){24, 28, 38, 245};
    float radius = 8;
    UI_RoundRect(x, y, w, 26, radius, bg);

    UI_Text(buf, NULL, label, x + 10, y + 5, 0.28f, 0.28f, g_theme.textPrimary);

    float barX = x + 32;
    float barY = y + 10;
    float barW = w - 80;
    float t = (max > min) ? (float)(value - min) / (float)(max - min) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);

    UI_RoundRect(barX, barY, barW, 6, 3, (ColorRGBA){60, 64, 80, 255});

    float fillW = barW * t;
    if (fillW > 0) {
        UI_RoundRect(barX, barY, fillW, 6, 3, swatchColor);
    }

    float knobX = barX + fillW;
    UI_RoundRect(knobX - 5, barY - 4, 10, 14, 5, selected ? g_theme.onPrimary : g_theme.textSecondary);

    char valStr[8];
    snprintf(valStr, sizeof(valStr), "%d", value);
    UI_TextRight(buf, NULL, valStr, x + w - 6, y + 5, 0.24f, 0.24f, g_theme.textSecondary);
}

void ledRenderBottom(C2D_TextBuf buf) {
    UI_TouchBarBackground();

    float et = easeOutCubic(g_enterT);
    float ap = clampf(et * 2.0f, 0.0f, 1.0f);

    float segH = 26.0f;
    float segY = 30.0f;
    float segW = 300.0f;

    UI_RoundRect(10, segY, segW, segH, segH * 0.5f, (ColorRGBA){24, 28, 38, 245});

    float segItemW = segW / (float)LED_MODE_COUNT;
    float targetX = 10 + s_mode * segItemW + 3;
    float curX = targetX;
    if (s_segSlideT < 1.0f) {
        static float s_morphX = 0.0f;
        if (s_morphX == 0.0f) s_morphX = targetX;
        s_morphX = animApproach(s_morphX, targetX, 0.22f);
        curX = s_morphX;
    }

    ColorRGBA selBg = g_theme.accent;
    selBg.a = 40;
    UI_RoundRect(curX, segY + 2, segItemW - 6, segH - 4, (segH - 4) * 0.5f, selBg);
    ColorRGBA selBorder = g_theme.accent;
    selBorder.a = 70;
    UI_RoundRect(curX, segY + 2, segItemW - 6, segH - 4, (segH - 4) * 0.5f, selBorder);

    for (int i = 0; i < LED_MODE_COUNT; i++) {
        ColorRGBA tc = (i == s_mode) ? g_theme.onPrimary : g_theme.textPrimary;
        UI_TextCenter(buf, NULL, MODE_NAMES[i], 10 + segItemW * i + segItemW * 0.5f, segY + 5, 0.26f, 0.26f, tc);
    }

    if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
        float swY = 66.0f;
        float swSize = 28.0f;
        float gap = 10.0f;
        float startX = 16.0f;
        ColorRGBA colors[] = {
            {s_r, 0, 0, 255},
            {0, s_g, 0, 255},
            {0, 0, s_b, 255},
        };
        for (int i = 0; i < 3; i++) {
            float sx = startX + i * (swSize + gap);
            float appearT = clampf((ap * 2.0f - i * 0.1f), 0.0f, 1.0f);
            float sa = easeOutCubic(appearT);
            bool sel = (s_selected == 1 + i);
            float pulse = sel ? 0.05f * sinf(uiFrameTime() * 4.0f) + 0.05f : 0.0f;

            if (sel) {
                ColorRGBA ring = themeMix(colors[i], g_theme.onPrimary, pulse * 0.3f);
                UI_RoundRect(sx - 3 + (1.0f - sa) * 6, swY - 3 + (1.0f - sa) * 6,
                             swSize + 6, swSize + 6, (swSize + 6) * 0.5f, ring);
            }
            UI_RoundRect(sx + (1.0f - sa) * 6, swY + (1.0f - sa) * 6,
                         swSize, swSize, swSize * 0.5f, colors[i]);
        }

        ColorRGBA pc = previewColor();
        UI_RoundRect(110, swY, 56, 28, 14, pc);
        ColorRGBA pcGlow = pc;
        pcGlow.a = 50;
        UI_RoundRect(108, swY - 2, 60, 32, 16, pcGlow);

        ledSliderRow(buf, "R", 10, 100, 300, s_r, 0, 255, s_selected == 1, (ColorRGBA){s_r, 64, 64, 255});
        ledSliderRow(buf, "G", 10, 130, 300, s_g, 0, 255, s_selected == 2, (ColorRGBA){64, s_g, 64, 255});
        ledSliderRow(buf, "B", 10, 160, 300, s_b, 0, 255, s_selected == 3, (ColorRGBA){64, 64, s_b, 255});

        ledSliderRow(buf, "Vel", 10, 192, 300, s_speed, 1, 5, s_selected == 4, g_theme.accent);
    } else {
        ledSliderRow(buf, "Velocidade", 10, 96, 300, s_speed, 1, 5, s_selected == 4, g_theme.accent);
    }

    UI_KeyHelp(buf, NULL, "A aplicar  B voltar", "START sair");
}
