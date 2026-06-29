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
    /* §9: reaplica o LED salvo JA no boot do app (antes so carregava a config
     * e esperava o usuario interagir). Assim a cor/padrao escolhidos voltam
     * sempre que o app abre -- persistencia "a nivel de app". A persistencia
     * REAL atraves de reboot do console e via patch da Luma (docs/LED_PERSIST.md),
     * que e dependente de versao e nao geramos as cegas. */
    applyLed();
}

void ledEnter(void) {
    s_selected = 0;
    s_draggingSlider = -1;
    initSliderTweens();
}

/* §4.1: re-aplica o padrao atual a cada ~2s. O sistema (PTM) sobrescreve o LED
 * em sleep/notificacao/carga -- por isso ele "apagava depois de uns minutos".
 * Chamado TODO frame por main.c (independente da aba), pra o LED escolhido
 * sobreviver enquanto o app roda. */
static float s_reassertT = 0.0f;
void ledTick(float dt) {
    if (!s_mcuReady) return;
    s_reassertT += dt;
    if (s_reassertT >= 2.0f) {
        s_reassertT = 0.0f;
        applyLed();
    }
}

void ledExit(void) {
    if (s_mcuReady) {
        /* v1.1: NAO limpamos mais o LED ao sair. O dono quer que a cor/padrao
         * CONTINUE depois de fechar o app -- o MCU mantem o ultimo padrao
         * ate um reboot ou uma notificacao do sistema. (Antes a gente apagava
         * de proposito; era isso que fazia "o led sumir ao sair do app".)
         * Persistencia ATRAVES de reboot e via patch da Luma -- ver
         * docs/LED_PERSIST.md. */
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
        /* segmented de modo (16,18,288,38) -- deve bater com ledRenderBottom. */
        if (!dragging && in->touchPY >= 18 && in->touchPY < 56 &&
            in->touchPX >= 16 && in->touchPX < 304) {
            float segItemW = 288.0f / (float)LED_MODE_COUNT;
            int newMode = (int)((in->touchPX - 16) / segItemW);
            newMode = clampi(newMode, 0, LED_MODE_COUNT - 1);
            if (in->touchDown) {
                s_selected = 0;
                setMode(newMode);
            }
            return;
        }

        float barX = 44.0f, barW = 236.0f, rowH = 30.0f;
        if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
            /* R/G/B em y=78,118,158; velocidade em y=198 (bate com ledRenderBottom). */
            const float ys[4] = { 78.0f, 118.0f, 158.0f, 198.0f };
            for (int i = 0; i < 3; i++) {
                float sy = ys[i];
                if ((dragging && s_draggingSlider == 1 + i) ||
                    (!dragging && in->touchPY >= sy - 6 && in->touchPY < sy + rowH &&
                     in->touchPX >= barX && in->touchPX < barX + barW)) {
                    if (in->touchDown || (dragging && s_draggingSlider == 1 + i)) {
                        u8* ch = (i == 0) ? &s_r : (i == 1) ? &s_g : &s_b;
                        *ch = (u8)sliderValueFromTouch(in, 1 + i, barX, sy + 8, barW, 0, 255);
                        commitSliderChange();
                    }
                    return;
                }
            }
            float spY = ys[3];
            if ((dragging && s_draggingSlider == 4) ||
                (!dragging && in->touchPY >= spY - 6 && in->touchPY < spY + rowH &&
                 in->touchPX >= barX && in->touchPX < barX + barW)) {
                if (in->touchDown || (dragging && s_draggingSlider == 4)) {
                    s_speed = sliderValueFromTouch(in, 4, barX, spY + 8, barW, 1, 5);
                    commitSliderChange();
                }
                return;
            }
        } else {
            float spY = 78.0f;
            if ((dragging && s_draggingSlider == 1) ||
                (!dragging && in->touchPY >= spY - 6 && in->touchPY < spY + rowH &&
                 in->touchPX >= barX && in->touchPX < barX + barW)) {
                if (in->touchDown || (dragging && s_draggingSlider == 1)) {
                    s_speed = sliderValueFromTouch(in, 1, barX, spY + 8, barW, 1, 5);
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
    (void)transVal; (void)scaleM;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_LED));

    /* Espec v20: anel grande (200,128) r42, contorno AA + fill cor@40%, SEM
     * glow + label de modo (200,~182). */
    ColorRGBA c = previewColor();
    float cx = 200.0f + slideX, cy = 128.0f;
    ColorRGBA fill = c; fill.a = 102;                 /* ~40% */
    UI_RoundRect(cx - 40.0f, cy - 40.0f, 80.0f, 80.0f, 40.0f, fill);
    ColorRGBA ring = c; ring.a = 255;
    UI_RingCircle(cx, cy, 86.0f, ring);

    UI_TextCenter(buf, NULL, modeNameI(s_mode), cx, 178.0f, 0.38f, 0.38f, g_theme.textPrimary);
    ColorRGBA st = s_mcuReady ? g_theme.success : g_theme.warning;
    UI_TextCenter(buf, NULL, s_mcuReady ? T(STR_LED_ACTIVE) : T(STR_LED_SIM),
                  cx, 200.0f, 0.24f, 0.24f, st);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, veil);
    }
}

/* Slider PLANO (espec v20, SEM glow): label (x24) + trilho fino x44..280 h12
 * + preenchimento na cor do canal + thumb circulo solido r8. focado = anel
 * accent no trilho; numero animado a direita. */
static void drawFlatSlider(C2D_TextBuf buf, const char* label, int displayValue,
                           int value, int min, int max, float y, ColorRGBA chan,
                           bool focused, float slideX) {
    ColorRGBA lblC = focused ? g_theme.textPrimary : g_theme.textSecondary;
    UI_Text(buf, NULL, label, 24.0f + slideX, y - 5.0f, 0.27f, 0.27f, lblC);

    float barX = 44.0f + slideX, barW = 236.0f, barY = y + 2.0f, barH = 12.0f;
    if (focused) UI_FocusRing(barX, barY, barW, barH, barH * 0.5f);
    ColorRGBA track = {255, 255, 255, 18}; /* ~7% */
    if (UI_AssetsReady()) UI_NinePill(barX, barY, barW, barH, track);
    else UI_RoundRect(barX, barY, barW, barH, barH * 0.5f, track);

    float t = (max > min) ? (float)(value - min) / (float)(max - min) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);
    if (t > 0.001f) {
        if (UI_AssetsReady()) UI_NinePill(barX, barY, fmaxf(barH, barW * t), barH, chan);
        else UI_RoundRect(barX, barY, fmaxf(barH, barW * t), barH, barH * 0.5f, chan);
    }
    float knobX = barX + barW * t, knobCy = barY + barH * 0.5f;
    UI_RoundRect(knobX - 8.0f, knobCy - 8.0f, 16.0f, 16.0f, 8.0f, chan); /* thumb solido */

    char v[8]; snprintf(v, sizeof(v), "%d", displayValue);
    UI_TextRight(buf, NULL, v, barX + barW, y - 5.0f, 0.22f, 0.22f, g_theme.textHint);
}

void ledRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_BottomBackground();

    /* segmented de modo (16,18,288,38). */
    const char* modeLabels[LED_MODE_COUNT];
    for (int i = 0; i < LED_MODE_COUNT; i++) modeLabels[i] = modeNameI(i);
    if (s_selected == 0) UI_FocusRing(16.0f + slideX, 18.0f, 288.0f, 38.0f, 19.0f);
    UI_TouchBarSegmented(buf, 16.0f + slideX, 18.0f, 288.0f, 38.0f, modeLabels, LED_MODE_COUNT, s_mode, &s_segTween);

    /* sliders nas cores dos canais (R rosa / G verde / B azul). */
    ColorRGBA chR = {255, 86, 120, 255}, chG = {95, 215, 130, 255}, chB = {10, 132, 255, 255};
    if (s_mode == LED_SOLID || s_mode == LED_PULSE) {
        drawFlatSlider(buf, "R", (int)(tweenValue(&s_valTween[1]) + 0.5f), s_r, 0, 255, 78.0f,  chR, s_selected == 1, slideX);
        drawFlatSlider(buf, "G", (int)(tweenValue(&s_valTween[2]) + 0.5f), s_g, 0, 255, 118.0f, chG, s_selected == 2, slideX);
        drawFlatSlider(buf, "B", (int)(tweenValue(&s_valTween[3]) + 0.5f), s_b, 0, 255, 158.0f, chB, s_selected == 3, slideX);
        drawFlatSlider(buf, T(STR_SPEED_SHORT), (int)(tweenValue(&s_valTween[4]) + 0.5f), s_speed, 1, 5, 198.0f, g_theme.accent, s_selected == 4, slideX);
    } else {
        drawFlatSlider(buf, T(STR_SPEED_SHORT), (int)(tweenValue(&s_valTween[1]) + 0.5f), s_speed, 1, 5, 78.0f, g_theme.accent, s_selected == 1, slideX);
    }

    UI_HelpBar(buf, T(STR_HELP_LED_L), T(STR_SAIR));

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, veil);
    }
}
