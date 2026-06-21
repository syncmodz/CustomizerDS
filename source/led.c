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
static int s_draggingSlider = -1;

int ledSelected(void) { return s_selected; }

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
        float wave = 0.20f + 0.80f * (sinf(t * M_TAU) * 0.5f + 0.5f);
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

static int sliderValueFromTouch(const AppInput* in, int index, float barX, float barY, float barW, int min, int max) {
    float t = clampf(((float)in->touchPX - barX) / barW, 0.0f, 1.0f);
    int val = min + (int)(t * (max - min) + 0.5f);
    val = clampi(val, min, max);
    s_draggingSlider = index;
    s_selected = index;
    return val;
}

static int s_savePending = 0;

static void commitSliderChange(void) {
    applyLed();
    s_savePending = SAVE_DEBOUNCE_FRAMES;
}

static void handleSliderDPad(int sel, u8* ch, int step) {
    if (ch) {
        *ch = (u8)clampi((int)*ch + step, 0, 255);
    }
    applyLed();
    if (s_savePending == 0) s_savePending = SAVE_DEBOUNCE_FRAMES;
}

void ledUpdate(const AppInput* in, float dt, int* currentScreen) {
    if (in->back) {
        s_draggingSlider = -1;
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    int totalItems = 1;
    if (s_mode == LED_SOLID || s_mode == LED_PULSE) totalItems = 5;
    else totalItems = 2;

    bool dragging = (s_draggingSlider >= 0 && in->touchHeld);

    bool modeChanged = false;
    if (in->left || in->right) {
        int dir = in->right ? 1 : -1;
        setMode((s_mode + dir + LED_MODE_COUNT) % LED_MODE_COUNT);
        s_segSlideT = 0.0f;
        modeChanged = true;
    }

    if (!dragging && !modeChanged) {
        if (in->downNav) { s_selected = (s_selected + 1) % totalItems; s_draggingSlider = -1; }
        if (in->up) { s_selected = (s_selected - 1 + totalItems) % totalItems; s_draggingSlider = -1; }

        if (s_selected == 0) {
            if (in->confirm) {
                setMode((s_mode + 1) % LED_MODE_COUNT);
            }
        } else if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
            if (s_selected >= 1 && s_selected <= 3) {
                u8* ch = (s_selected == 1) ? &s_r : (s_selected == 2) ? &s_g : &s_b;
                if (in->left) handleSliderDPad(s_selected, ch, -8);
                if (in->right) handleSliderDPad(s_selected, ch, 8);
            }
            if (s_selected == 4) {
                if (in->left) { s_speed = clampi(s_speed - 1, 1, 5); applyLed(); if (s_savePending == 0) s_savePending = SAVE_DEBOUNCE_FRAMES; }
                if (in->right) { s_speed = clampi(s_speed + 1, 1, 5); applyLed(); if (s_savePending == 0) s_savePending = SAVE_DEBOUNCE_FRAMES; }
            }
        } else {
            if (s_selected == 1) {
                if (in->left) { s_speed = clampi(s_speed - 1, 1, 5); applyLed(); if (s_savePending == 0) s_savePending = SAVE_DEBOUNCE_FRAMES; }
                if (in->right) { s_speed = clampi(s_speed + 1, 1, 5); applyLed(); if (s_savePending == 0) s_savePending = SAVE_DEBOUNCE_FRAMES; }
            }
        }
    }

    if (in->touchDown || (in->touchHeld && s_draggingSlider >= 0)) {
        if (!dragging && in->touchPY >= 8 && in->touchPY < 8 + 28 &&
            in->touchPX >= 10 && in->touchPX < 310) {
            float segItemW = 300.0f / (float)LED_MODE_COUNT;
            int newMode = (int)((in->touchPX - 10) / segItemW);
            newMode = clampi(newMode, 0, LED_MODE_COUNT - 1);
            if (in->touchDown) {
                s_segSlideT = 0.0f;
                s_selected = 0;
                setMode(newMode);
            }
            return;
        }

        if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
            /* y/x devem bater exatamente com UI_TouchBarSlider em ledRenderBottom:
             * linhas em y=56,88,120 (passo 32) e a barra interna comeca em x+36 (label ocupa 0-36). */
            float slY = 56.0f, rowH = 28.0f, barX = 46.0f, barW = 226.0f;
            for (int i = 0; i < 3; i++) {
                float sy = slY + i * 32.0f;
                if ((dragging && s_draggingSlider == 1 + i) ||
                    (!dragging && in->touchPY >= sy && in->touchPY < sy + rowH &&
                     in->touchPX >= barX && in->touchPX < barX + barW)) {
                    if (in->touchDown || (dragging && s_draggingSlider == 1 + i)) {
                        u8* ch = (i == 0) ? &s_r : (i == 1) ? &s_g : &s_b;
                        *ch = (u8)sliderValueFromTouch(in, 1 + i, barX, sy + 9, barW, 0, 255);
                        commitSliderChange();
                    }
                    return;
                }
            }
            float spY = 152.0f;
            if ((dragging && s_draggingSlider == 4) ||
                (!dragging && in->touchPY >= spY && in->touchPY < spY + rowH &&
                 in->touchPX >= barX && in->touchPX < barX + barW)) {
                if (in->touchDown || (dragging && s_draggingSlider == 4)) {
                    s_speed = sliderValueFromTouch(in, 4, barX, spY + 9, barW, 1, 5);
                    commitSliderChange();
                }
                return;
            }
        } else {
            float barX = 46.0f, barW = 226.0f, spY = 56.0f;
            if ((dragging && s_draggingSlider == 1) ||
                (!dragging && in->touchPY >= spY && in->touchPY < spY + 28 &&
                 in->touchPX >= barX && in->touchPX < barX + barW)) {
                if (in->touchDown || (dragging && s_draggingSlider == 1)) {
                    s_speed = sliderValueFromTouch(in, 1, barX, spY + 9, barW, 1, 5);
                    commitSliderChange();
                }
                return;
            }
        }
    }

    if (!in->touchHeld) s_draggingSlider = -1;

    if (s_savePending > 0) {
        s_savePending--;
        if (s_savePending == 0) saveLed();
    }

    s_segSlideT = fminf(s_segSlideT + dt, 1.0f);
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

void ledRenderTop(C2D_TextBuf buf, float transVal) {
    float offset = (1.0f - transVal) * 40.0f;
    UI_TopBackground();
    UI_TopMenuBar("LED", buf);

    UI_Card(16, 30 + offset, 368, 196, 16, g_theme.surface);

    UI_Text(buf, NULL, MODE_NAMES[s_mode], 32, 52 + offset, 0.44f, 0.44f, g_theme.textPrimary);
    UI_Text(buf, NULL, s_mcuReady ? "led ativo" : "MCU indisponivel — preview",
            32, 78 + offset, 0.22f, 0.22f, s_mcuReady ? g_theme.success : g_theme.warning);

    ColorRGBA c = previewColor();
    float lx = 266.0f, ly = 52 + offset;
    ColorRGBA previewBg = themeIsDark() ? (ColorRGBA){8, 10, 14, 255} : (ColorRGBA){225, 228, 238, 255};
    UI_RoundFrame(lx, ly, 88, 64, 16, previewBg, (ColorRGBA){255, 255, 255, 12});
    UI_RoundRect(lx + 4, ly + 4, 80, 56, 12, c);
    ColorRGBA glow = c;
    glow.a = 30;
    UI_RoundRect(lx + 2, ly + 2, 84, 60, 14, glow);

    char rgb[40];
    snprintf(rgb, sizeof(rgb), "R%d G%d B%d  vel %d", s_r, s_g, s_b, s_speed);
    UI_Text(buf, NULL, rgb, 32, 100 + offset, 0.26f, 0.26f, g_theme.textSecondary);

    if (R_FAILED(s_lastResult)) {
        char err[32];
        snprintf(err, sizeof(err), "erro: 0x%08lX", (unsigned long)s_lastResult);
        UI_Text(buf, NULL, err, 32, 126 + offset, 0.22f, 0.22f, g_theme.textHint);
    } else {
        UI_Text(buf, NULL, "ajuste com os controles abaixo", 32, 126 + offset, 0.22f, 0.22f, g_theme.textHint);
    }

    UI_Text(buf, NULL, s_mcuReady ? "tempo real" : "simulacao na tela",
            32, 150 + offset, 0.20f, 0.20f, g_theme.textHint);
}

void ledRenderBottom(C2D_TextBuf buf, float transVal) {
    float offset = (1.0f - transVal) * 30.0f;
    float et = UI_EnterProgress();
    UI_BottomBackground();

    const char* modeLabels[LED_MODE_COUNT];
    for (int i = 0; i < LED_MODE_COUNT; i++) modeLabels[i] = MODE_NAMES[i];
    UI_TouchBarSegmented(buf, 10, 8 + offset, 300, 28, modeLabels, LED_MODE_COUNT, s_mode, s_segSlideT);

    if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
        UI_TouchBarSlider(buf, 10, 56 + offset, 300, "R", s_r, 0, 255, s_selected == 1, (ColorRGBA){s_r, 40, 40, 255});
        UI_TouchBarSlider(buf, 10, 88 + offset, 300, "G", s_g, 0, 255, s_selected == 2, (ColorRGBA){40, s_g, 40, 255});
        UI_TouchBarSlider(buf, 10, 120 + offset, 300, "B", s_b, 0, 255, s_selected == 3, (ColorRGBA){40, 40, s_b, 255});
        UI_TouchBarSlider(buf, 10, 152 + offset, 300, "Vel", s_speed, 1, 5, s_selected == 4, g_theme.accent);
    } else {
        UI_TouchBarSlider(buf, 10, 56 + offset, 300, "Vel", s_speed, 1, 5, s_selected == 1, g_theme.accent);
    }

    UI_HelpBar(buf, "A modo  B voltar  <- valor ->", "START sair");
}
