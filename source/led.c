#include "led.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "config.h"
#include "fonts.h"
#include "lang.h"
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

/* Nome do modo traduzido (i18n). "RGB" e nome proprio -> nao traduz; os
 * outros vem do T(). A ordem segue o enum LedMode acima. */
static const char* modeNameI(int i) {
    switch (i) {
        case LED_RAINBOW:  return "RGB";
        case LED_SOLID:    return T(STR_LED_STATIC);
        case LED_PULSE:    return T(STR_LED_PULSE);
        case LED_MODE_OFF: return T(STR_LED_OFF);
        default:           return "RGB";
    }
}

static int s_mode = LED_RAINBOW;
static int s_selected = 0;
static int s_speed = 2;
static u8 s_r = 255;
static u8 s_g = 96;
static u8 s_b = 160;
static bool s_mcuReady = false;
static Result s_lastResult = 0;

/* Tween do segmentedLozenge (UI_TouchBarSegmented) -- substitui o antigo
 * s_segSlideT/morphT; o componente agora cuida do slide easeOutBack por
 * dentro, esta instancia so guarda o estado entre frames. */
static Tween s_segTween;
static int s_draggingSlider = -1;

/* Slider 3.5: thumb cresce 1.0->1.18 (easeOutBack) ao arrastar, e o numero
 * exibido conta animado (180ms easeOutCubic) em vez de saltar instantaneo.
 * Indices 1-4 = R/G/B/Vel (ou so 1=Vel nos modos sem RGB), 0 nao usado. */
static Tween s_thumbTween[5];
static Tween s_valTween[5];
static int s_lastDisplayed[5] = { -1, -1, -1, -1, -1 };

int ledSelected(void) { return s_selected; }
const char* ledModeName(void) { return modeNameI(s_mode); }

static u8 scaleU8(u8 value, float mul) {
    int out = (int)((float)value * mul);
    return (u8)clampi(out, 0, 255);
}

/* hsvToRgb (antes static aqui) foi extraido pra common.h como hsvToRgbF --
 * compartilhado com a barra de espectro do seletor hex em darkmode.c (v3 3.4). */

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
        hsvToRgbF((float)i * 360.0f / 32.0f, 1.0f, 1.0f, &r, &g, &b);
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

static void initSliderTweens(void) {
    for (int k = 0; k < 5; k++) {
        tweenStart(&s_thumbTween[k], 1.0f, 1.0f, 0.001f, EASE_LINEAR);
        tweenUpdate(&s_thumbTween[k], 1.0f);
        tweenStart(&s_valTween[k], 0.0f, 0.0f, 0.001f, EASE_LINEAR);
        tweenUpdate(&s_valTween[k], 1.0f);
        s_lastDisplayed[k] = -1;
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
    initSliderTweens();
}

void ledEnter(void) {
    s_selected = 0;
    s_draggingSlider = -1;
    initSliderTweens();
}

void ledExit(void) {
    if (s_mcuReady) {
        /* §14.3: restaura o LED ao sair -- limpa nosso padrao customizado
         * (apaga, estado ocioso) pra nao deixar a cor do usuario "presa". O
         * LED de notificacao e MCU, NAO NAND: nenhum risco de brick, e um
         * reboot ja reverteria de qualquer forma. */
        setPatternSolid(0, 0, 0);
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

/* Slider 3.5: thumb cresce 1.0->1.18 ao iniciar o arrasto (easeOutBack) e
 * volta a 1.0 ao soltar (easeOutCubic); o valor exibido conta animado
 * (180ms easeOutCubic) sempre que o valor real mudar -- chamado uma vez por
 * frame, incondicional, pra nunca perder um tick mesmo com os returns
 * antecipados do tratamento de toque abaixo. */
static void updateSliderTweens(float dt) {
    static int s_prevDragIdx = -1;
    if (s_draggingSlider != s_prevDragIdx) {
        if (s_prevDragIdx >= 0 && s_prevDragIdx < 5)
            tweenStart(&s_thumbTween[s_prevDragIdx], tweenValue(&s_thumbTween[s_prevDragIdx]), 1.0f, 0.18f, EASE_OUT_CUBIC);
        if (s_draggingSlider >= 0 && s_draggingSlider < 5)
            tweenStart(&s_thumbTween[s_draggingSlider], tweenValue(&s_thumbTween[s_draggingSlider]), 1.18f, 0.18f, EASE_OUT_BACK);
        s_prevDragIdx = s_draggingSlider;
    }
    for (int k = 0; k < 5; k++) tweenUpdate(&s_thumbTween[k], dt);

    int realVals[5] = {0, 0, 0, 0, 0};
    if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
        realVals[1] = s_r; realVals[2] = s_g; realVals[3] = s_b; realVals[4] = s_speed;
    } else {
        realVals[1] = s_speed;
    }
    for (int k = 1; k < 5; k++) {
        if (s_lastDisplayed[k] != realVals[k]) {
            float from = (s_lastDisplayed[k] < 0) ? (float)realVals[k] : tweenValue(&s_valTween[k]);
            tweenStart(&s_valTween[k], from, (float)realVals[k], 0.18f, EASE_OUT_CUBIC);
            s_lastDisplayed[k] = realVals[k];
        }
        tweenUpdate(&s_valTween[k], dt);
    }
}

void ledUpdate(const AppInput* in, float dt, int* currentScreen) {
    updateSliderTweens(dt);
    if (in->back) {
        s_draggingSlider = -1;
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    int totalItems = 1;
    if (s_mode == LED_SOLID || s_mode == LED_PULSE) totalItems = 5;
    else totalItems = 2;

    bool dragging = (s_draggingSlider >= 0 && in->touchHeld);

    /* esq/dir SO troca o modo quando o item de MODO (selected 0) esta focado.
     * Antes trocava o modo em qualquer item -> os sliders R/G/B/Vel ficavam
     * ajustaveis so por toque (esq/dir pulava o modo em vez de mexer no
     * valor). Com o guard, esq/dir num slider ajusta o valor (bloco abaixo),
     * deixando a tela LED 100% navegavel por D-pad. */
    bool modeChanged = false;
    if ((in->left || in->right) && s_selected == 0) {
        int dir = in->right ? 1 : -1;
        setMode((s_mode + dir + LED_MODE_COUNT) % LED_MODE_COUNT);
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
}

static ColorRGBA previewColor(void) {
    if (s_mode == LED_MODE_OFF) return (ColorRGBA){24, 24, 32, 255};
    if (s_mode == LED_SOLID) return (ColorRGBA){s_r, s_g, s_b, 255};
    if (s_mode == LED_PULSE) {
        /* Spec 3.5 literal: 0.5 + 0.5*sin(time*vel). */
        float wave = 0.5f + 0.5f * sinf(uiFrameTime() * (float)s_speed);
        return (ColorRGBA){scaleU8(s_r, wave), scaleU8(s_g, wave), scaleU8(s_b, wave), 255};
    }
    u8 r, g, b;
    float h = fmodf(uiFrameTime() * (45.0f + s_speed * 25.0f), 360.0f);
    hsvToRgbF(h, 1.0f, 1.0f, &r, &g, &b);
    return (ColorRGBA){r, g, b, 255};
}

ColorRGBA ledPreviewColor(void) { return previewColor(); }

void ledRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    /* Parallax 3 camadas (Travel Motion), igual as outras telas. */
    float offsetRaw = (1.0f - transVal);
    float offset = offsetRaw * 40.0f;
    float offsetFg = offsetRaw * 18.0f;
    UI_TopBackground();
    UI_TopMenuBar(T(STR_TAB_LED), buf);

    /* Blobs decorativos de fundo removidos (mesma causa do bug "bolas
     * sobrepostas" da Inicio): caiam dentro do retangulo do card, que tem
     * alpha 235 (nao 255) e deixava-os espiar por tras. */
    ColorRGBA c = previewColor();

    /* Transicao 3.2: card desliza/escala a partir do centro; fade real
     * (cobre tambem UI_StatChip/UI_Badge sem alpha proprio) vem do veu. */
    float cardBaseX = 16, cardBaseY = 30 + offset, cardBaseW = 368, cardBaseH = 196;
    float cardW = cardBaseW * scaleM, cardH = cardBaseH * scaleM;
    float cardX = cardBaseX + slideX + (cardBaseW - cardW) * 0.5f;
    float cardY = cardBaseY + (cardBaseH - cardH) * 0.5f;
    UI_Card(cardX, cardY, cardW, cardH, RADIUS_CARD, g_theme.surface);

    UI_Text(buf, NULL, modeNameI(s_mode), 32 + slideX, 50 + offsetFg, 0.40f, 0.40f, g_theme.textPrimary);
    UI_Badge(buf, 32 + slideX, 80 + offsetFg,
             s_mcuReady ? T(STR_LED_ACTIVE) : T(STR_LED_SIM),
             s_mcuReady ? g_theme.success : g_theme.warning);

    /* Swatch grande com a cor real que esta sendo aplicada agora. */
    float lx = 266.0f + slideX, ly = 50 + offsetFg;
    ColorRGBA previewBg = themeIsDark() ? (ColorRGBA){8, 8, 9, 255} : (ColorRGBA){225, 228, 238, 255};
    UI_RoundFrame(lx, ly, 88, 64, 16, previewBg, (ColorRGBA){255, 255, 255, 12});
    UI_RoundRect(lx + 4, ly + 4, 80, 56, 12, c);
    ColorRGBA glow = c;
    glow.a = 30;
    UI_RoundRect(lx + 2, ly + 2, 84, 60, 14, glow);

    /* Grade de stat chips preenchendo o card -- substitui a lista de texto
     * solto (Off / led ativo / RGB / instrucoes) por dados reais legiveis. */
    float chipY = 116.0f + offsetFg;
    float chipH = 64.0f;
    float chipGap = 8.0f;
    float chipW = (336.0f - chipGap * 2.0f) / 3.0f;
    char valBuf[3][12];
    const char* labels[3];
    ColorRGBA dots[3];
    int numChips;

    if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
        numChips = 3;
        labels[0] = T(STR_RED); labels[1] = T(STR_GREEN); labels[2] = T(STR_BLUE);
        snprintf(valBuf[0], sizeof(valBuf[0]), "%d", s_r);
        snprintf(valBuf[1], sizeof(valBuf[1]), "%d", s_g);
        snprintf(valBuf[2], sizeof(valBuf[2]), "%d", s_b);
        dots[0] = (ColorRGBA){255, 80, 80, 255};
        dots[1] = (ColorRGBA){80, 220, 110, 255};
        dots[2] = (ColorRGBA){90, 140, 255, 255};
    } else {
        numChips = 2;
        labels[0] = T(STR_SPEED); labels[1] = T(STR_STATE);
        snprintf(valBuf[0], sizeof(valBuf[0]), "%d / 5", s_speed);
        snprintf(valBuf[1], sizeof(valBuf[1]), "%s", s_mcuReady ? T(STR_REALTIME) : T(STR_SIMULATED));
        dots[0] = g_theme.accent;
        dots[1] = s_mcuReady ? g_theme.success : g_theme.warning;
    }

    for (int i = 0; i < numChips; i++) {
        /* Stagger 3.2 exato: 40ms entre chips, 260ms cada, easeOutCubic. */
        float ca = UI_StaggerT(i, 0.04f, 0.26f);
        if (ca <= 0.0f) continue;
        float slide = (1.0f - ca) * 10.0f;
        float cx = 32.0f + slideX + i * (chipW + chipGap);
        UI_StatChip(buf, cx, chipY + slide, chipW, chipH, labels[i], valBuf[i], dots[i]);
    }

    if (R_FAILED(s_lastResult)) {
        char err[32];
        snprintf(err, sizeof(err), "erro: 0x%08lX", (unsigned long)s_lastResult);
        UI_Text(buf, NULL, err, 32 + slideX, chipY + chipH + 6, 0.20f, 0.20f, g_theme.textHint);
    }

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }
}

void ledRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    float offset = (1.0f - transVal) * 30.0f;
    (void)scaleM;
    UI_BottomBackground();

    const char* modeLabels[LED_MODE_COUNT];
    for (int i = 0; i < LED_MODE_COUNT; i++) modeLabels[i] = modeNameI(i);
    UI_TouchBarSegmented(buf, 10 + slideX, 8 + offset, 300, 28, modeLabels, LED_MODE_COUNT, s_mode, &s_segTween);

    if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
        UI_TouchBarSliderDrag(buf, 10 + slideX, 56 + offset, 300, "R", s_r, 0, 255, s_selected == 1,
                              (ColorRGBA){s_r, 40, 40, 255}, tweenValue(&s_thumbTween[1]), tweenValue(&s_valTween[1]));
        UI_TouchBarSliderDrag(buf, 10 + slideX, 88 + offset, 300, "G", s_g, 0, 255, s_selected == 2,
                              (ColorRGBA){40, s_g, 40, 255}, tweenValue(&s_thumbTween[2]), tweenValue(&s_valTween[2]));
        UI_TouchBarSliderDrag(buf, 10 + slideX, 120 + offset, 300, "B", s_b, 0, 255, s_selected == 3,
                              (ColorRGBA){40, 40, s_b, 255}, tweenValue(&s_thumbTween[3]), tweenValue(&s_valTween[3]));
        UI_TouchBarSliderDrag(buf, 10 + slideX, 152 + offset, 300, T(STR_SPEED_SHORT), s_speed, 1, 5, s_selected == 4,
                              g_theme.accent, tweenValue(&s_thumbTween[4]), tweenValue(&s_valTween[4]));
    } else {
        UI_TouchBarSliderDrag(buf, 10 + slideX, 56 + offset, 300, T(STR_SPEED_SHORT), s_speed, 1, 5, s_selected == 1,
                              g_theme.accent, tweenValue(&s_thumbTween[1]), tweenValue(&s_valTween[1]));
    }

    UI_HelpBar(buf, T(STR_HELP_LED_L), T(STR_SAIR));

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
