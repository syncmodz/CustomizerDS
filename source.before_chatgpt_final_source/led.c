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

static const char* MODE_NAMES[] = { "RGB", "Fixo", "Pulso", "Off" };

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
static int s_activeChannel = 0;
static int s_draggingSlider = -1;

int ledSelected(void) { return s_selected; }
//static int ledModeCount(void) { return LED_MODE_COUNT; }

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
    s_draggingSlider = -1;
    s_lastResult = mcuHwcInit();
    s_mcuReady = R_SUCCEEDED(s_lastResult);
}

void ledEnter(void) {
    s_selected = 0;
    s_draggingSlider = -1;
    s_pressedT = 0.0f;
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

static void handleSliderTouch(const AppInput* in, int index, float barX, float barY, float barW, int* outVal, int min, int max) {
    float t = clampf(((float)in->touchPX - barX) / barW, 0.0f, 1.0f);
    int val = min + (int)(t * (max - min) + 0.5f);
    val = clampi(val, min, max);
    *outVal = val;
    s_draggingSlider = index;
    s_selected = index;
    applyLed();
    saveLed();
}

void ledUpdate(const AppInput* in, int* currentScreen) {
    if (in->back) {
        s_draggingSlider = -1;
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    int totalItems = 1;
    if (s_mode == LED_SOLID || s_mode == LED_PULSE) totalItems = 5;
    else totalItems = 2;

    s_pressedT = animApproach(s_pressedT, 0.0f, 0.30f);

    bool dragging = (s_draggingSlider >= 0 && in->touchHeld);

    if (!dragging) {
        if (in->downNav) { s_selected = (s_selected + 1) % totalItems; s_draggingSlider = -1; }
        if (in->up) { s_selected = (s_selected - 1 + totalItems) % totalItems; s_draggingSlider = -1; }

        if (s_selected == 0) {
            if (in->left) { setMode((s_mode - 1 + LED_MODE_COUNT) % LED_MODE_COUNT); s_segSlideT = 0.0f; }
            if (in->right) { setMode((s_mode + 1) % LED_MODE_COUNT); s_segSlideT = 0.0f; }
        }

        if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
            if (s_selected >= 1 && s_selected <= 3) {
                u8* ch = (s_selected == 1) ? &s_r : (s_selected == 2) ? &s_g : &s_b;
                if (in->left) { *ch = (u8)clampi((int)*ch - 8, 0, 255); applyLed(); saveLed(); }
                if (in->right) { *ch = (u8)clampi((int)*ch + 8, 0, 255); applyLed(); saveLed(); }
            }
            if (s_selected == 4) {
                if (in->left) { s_speed = clampi(s_speed - 1, 1, 5); applyLed(); saveLed(); }
                if (in->right) { s_speed = clampi(s_speed + 1, 1, 5); applyLed(); saveLed(); }
            }
        } else {
            if (s_selected == 1) {
                if (in->left) { s_speed = clampi(s_speed - 1, 1, 5); applyLed(); saveLed(); }
                if (in->right) { s_speed = clampi(s_speed + 1, 1, 5); applyLed(); saveLed(); }
            }
        }
    }

    if (in->touchDown || (in->touchHeld && s_draggingSlider >= 0)) {
        float segX = 10, segY = 30, segW = 300, segH = 26;
        float segItemW = segW / (float)LED_MODE_COUNT;

        if (!dragging && in->touchPY >= segY && in->touchPY < segY + segH &&
            in->touchPX >= segX && in->touchPX < segX + segW) {
            int newMode = (int)((in->touchPX - segX) / segItemW);
            newMode = clampi(newMode, 0, LED_MODE_COUNT - 1);
            if (in->touchDown) {
                s_segSlideT = 0.0f;
                s_selected = 0;
                s_pressedT = 1.0f;
                setMode(newMode);
            }
            return;
        }

        if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
            float swY = 66.0f, swSize = 28.0f, gap = 10.0f, startX = 16.0f;
            for (int i = 0; i < 3; i++) {
                float sx = startX + i * (swSize + gap);
                if (!dragging && in->touchPY >= swY && in->touchPY < swY + swSize &&
                    in->touchPX >= sx && in->touchPX < sx + swSize) {
                    if (in->touchDown) {
                        s_selected = 1 + i;
                        s_activeChannel = i;
                    }
                    return;
                }
            }

            float slY = 100.0f;
            float barX = 42.0f;
            float barW = 248.0f;
            for (int i = 0; i < 3; i++) {
                float sy = slY + i * 30.0f;
                if ((dragging && s_draggingSlider == 1 + i) ||
                    (!dragging && in->touchPY >= sy && in->touchPY < sy + 26 &&
                     in->touchPX >= barX && in->touchPX < barX + barW)) {
                    if (in->touchDown || (dragging && s_draggingSlider == 1 + i)) {
                        u8* ch = (i == 0) ? &s_r : (i == 1) ? &s_g : &s_b;
                        handleSliderTouch(in, 1 + i, barX, sy + 8, barW, (int*)ch, 0, 255);
                    }
                    return;
                }
            }

            float spY = 192.0f;
            if ((dragging && s_draggingSlider == 4) ||
                (!dragging && in->touchPY >= spY && in->touchPY < spY + 26 &&
                 in->touchPX >= barX && in->touchPX < barX + barW)) {
                if (in->touchDown || (dragging && s_draggingSlider == 4)) {
                    handleSliderTouch(in, 4, barX, spY + 8, barW, &s_speed, 1, 5);
                }
                return;
            }
        } else {
            float spY = 96.0f;
            float barX = 42.0f;
            float barW = 248.0f;
            if ((dragging && s_draggingSlider == 1) ||
                (!dragging && in->touchPY >= spY && in->touchPY < spY + 26 &&
                 in->touchPX >= barX && in->touchPX < barX + barW)) {
                if (in->touchDown || (dragging && s_draggingSlider == 1)) {
                    handleSliderTouch(in, 1, barX, spY + 8, barW, &s_speed, 1, 5);
                }
                return;
            }
        }
    }

    if (in->confirm && s_selected == 0) {
        setMode((s_mode + 1) % LED_MODE_COUNT);
    }

    if (!in->touchHeld) s_draggingSlider = -1;

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
    float lx = 270.0f, ly = 54 + slideUp, lw = 82, lh = 60;
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
    UI_RoundRect(x, y, w, 26, 8, bg);
    UI_Text(buf, NULL, label, x + 10, y + 5, 0.28f, 0.28f, g_theme.textPrimary);
    float barX = x + 32;
    float barY = y + 10;
    float barW = w - 80;
    float t = (max > min) ? (float)(value - min) / (float)(max - min) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);
    UI_RoundRect(barX, barY, barW, 6, 3, (ColorRGBA){60, 64, 80, 255});
    float fillW = barW * t;
    if (fillW > 0) UI_RoundRect(barX, barY, fillW, 6, 3, swatchColor);
    float knobX = barX + fillW;
    UI_RoundRect(knobX - 5, barY - 4, 10, 14, 5, selected ? g_theme.onPrimary : g_theme.textSecondary);
    char valStr[8];
    snprintf(valStr, sizeof(valStr), "%d", value);
    UI_TextRight(buf, NULL, valStr, x + w - 6, y + 5, 0.24f, 0.24f, g_theme.textSecondary);
}

void ledRenderBottom(C2D_TextBuf buf) {
    UI_TouchBarBackground();
    float et = easeOutCubic(g_enterT);

    float segH = 26.0f, segY = 30.0f, segW = 300.0f;
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
        float swY = 66.0f, swSize = 28.0f, gap = 10.0f, startX = 16.0f;
        ColorRGBA colors[] = { {s_r, 0, 0, 255}, {0, s_g, 0, 255}, {0, 0, s_b, 255} };
        for (int i = 0; i < 3; i++) {
            float sx = startX + i * (swSize + gap);
            float appearT = clampf((et * 2.0f - i * 0.1f), 0.0f, 1.0f);
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
        ledSliderRow(buf, "Velocidade", 10, 96, 300, s_speed, 1, 5, s_selected == 1, g_theme.accent);
    }
    UI_KeyHelp(buf, NULL, "A modo  B voltar  <- valor ->", "START sair");
}
