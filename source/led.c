#include "led.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "config.h"
#include "fonts.h"
#include "icons.h"
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
/* 1.5.0: PROFUNDIDADE do pulso (1..100%). Controla o quanto o LED escurece no
 * vale da onda: no 3DS real o pulso so escurecia ate ~metade -- agora da pra
 * ir do "quase constante" ao "apaga total". floor = 1 - depth/100. */
static int s_pulseDepth = 80;
static bool s_mcuReady = false;
static Result s_lastResult = 0;

/* 1.5.0: cada modo mostra sliders DIFERENTES (o dono reclamou do slider de
 * Speed aparecer em Off/Static, onde nao ha nada pra ajustar):
 *   RGB    -> [Vel]
 *   Fixo   -> [R,G,B]            (sem Vel: cor parada nao tem velocidade)
 *   Pulso  -> [R,G,B,Vel,Prof]  (Prof = profundidade nova)
 *   Off    -> (nenhum)
 * Os tweens de thumb/valor sao indexados por KIND (nao por posicao) pra
 * persistirem quando o modo muda. */
typedef enum { CH_R = 0, CH_G, CH_B, CH_SPEED, CH_DEPTH, CH_COUNT } LedChan;

static int ledSliderKinds(int* kinds) {
    switch (s_mode) {
        case LED_RAINBOW: kinds[0] = CH_SPEED; return 1;
        case LED_SOLID:   kinds[0] = CH_R; kinds[1] = CH_G; kinds[2] = CH_B; return 3;
        case LED_PULSE:   kinds[0] = CH_R; kinds[1] = CH_G; kinds[2] = CH_B;
                          kinds[3] = CH_SPEED; kinds[4] = CH_DEPTH; return 5;
        default:          return 0; /* OFF: nada */
    }
}

/* Tween do segmentedLozenge (UI_TouchBarSegmented) -- substitui o antigo
 * s_segSlideT/morphT; o componente agora cuida do slide easeOutBack por
 * dentro, esta instancia so guarda o estado entre frames. */
static Tween s_segTween;

/* Slider 3.5: thumb cresce 1.0->1.18 (easeOutBack) ao arrastar, e o numero
 * exibido conta animado (180ms easeOutCubic) em vez de saltar instantaneo.
 * 1.5.0: indexados por CANAL (LedChan: R/G/B/Speed/Depth) e nao mais por
 * posicao, pra o estado sobreviver a troca de modo. */
static Tween s_thumbTween[CH_COUNT];
static Tween s_valTween[CH_COUNT];
static int s_lastDisplayed[CH_COUNT] = { -1, -1, -1, -1, -1 };
static int s_dragChan = -1; /* canal (LedChan) sendo arrastado, ou -1 */

/* Ponteiro/limites/cor/label de um canal. */
static void chanInfo(int chan, int** ip, int* mn, int* mx, ColorRGBA* col, const char** label) {
    static int rr, gg, bb, sp, dp;
    switch (chan) {
        case CH_R: rr = s_r; *ip = &rr; *mn = 0; *mx = 255; *col = (ColorRGBA){255, 86, 120, 255}; *label = "R"; break;
        case CH_G: gg = s_g; *ip = &gg; *mn = 0; *mx = 255; *col = (ColorRGBA){95, 215, 130, 255}; *label = "G"; break;
        case CH_B: bb = s_b; *ip = &bb; *mn = 0; *mx = 255; *col = (ColorRGBA){10, 132, 255, 255}; *label = "B"; break;
        case CH_SPEED: sp = s_speed; *ip = &sp; *mn = 1; *mx = 5; *col = UI_AccentAnim(); *label = T(STR_SPEED_SHORT); break;
        default: dp = s_pulseDepth; *ip = &dp; *mn = 1; *mx = 100; *col = UI_AccentAnim(); *label = T(STR_DEPTH_SHORT); break;
    }
}
/* Escreve o valor de volta na variavel do canal. */
static void chanSet(int chan, int v) {
    switch (chan) {
        case CH_R: s_r = (u8)clampi(v, 0, 255); break;
        case CH_G: s_g = (u8)clampi(v, 0, 255); break;
        case CH_B: s_b = (u8)clampi(v, 0, 255); break;
        case CH_SPEED: s_speed = clampi(v, 1, 5); break;
        default: s_pulseDepth = clampi(v, 1, 100); break;
    }
}
static int chanGet(int chan) {
    switch (chan) {
        case CH_R: return s_r; case CH_G: return s_g; case CH_B: return s_b;
        case CH_SPEED: return s_speed; default: return s_pulseDepth;
    }
}

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
    /* 1.9.5: idem pulse -- estava rapido demais; faixa de delay bem maior. */
    pattern.delay = (u8)clampi(0x90 - s_speed * 0x1A, 0x10, 0x90);
    pattern.smoothing = 0x01;
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
    /* 1.9.5: o LED fisico estava rapido DEMAIS (delay 10..26). O MCU segura cada
     * um dos 32 frames por 'delay' ticks -> delay maior = pulso mais lento. Faixa
     * bem mais alta agora: speed 1 = bem lento, speed 5 = rapido. */
    pattern.delay = (u8)clampi(0xC0 - s_speed * 0x24, 0x18, 0xC0);
    pattern.smoothing = 0x01;
    pattern.loopDelay = 0x00;
    pattern.blinkSpeed = 0x00;
    /* 1.5.0: profundidade -> piso da onda. depth 100% = piso 0 (apaga total);
     * depth baixo = piso alto (quase constante). */
    float floorB = clampf(1.0f - (float)s_pulseDepth / 100.0f, 0.0f, 1.0f);
    for (int i = 0; i < 32; i++) {
        float t = (float)i / 31.0f;
        float wave = floorB + (1.0f - floorB) * (sinf(t * M_TAU) * 0.5f + 0.5f);
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
    cfg.reserved[1] = (u8)clampi(s_pulseDepth, 1, 100); /* 1.5.0: profundidade do pulso */
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
    for (int k = 0; k < CH_COUNT; k++) {
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
    s_pulseDepth = cfg.reserved[1] ? clampi(cfg.reserved[1], 1, 100) : 80; /* 0 = save antigo -> default 80 */
    s_selected = 0;
    s_dragChan = -1;
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
    s_dragChan = -1;
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

static int s_savePending = 0;

/* Geometria das linhas de slider, COMPARTILHADA por render e toque: o grupo de
 * N sliders fica centralizado verticalmente na area de controle (~80..200). */
static float ledRowStep(int count) { return (count >= 5) ? 27.0f : 34.0f; }
static float ledRowY(int slot, int count) {
    if (count <= 0) return 120.0f;
    float step = ledRowStep(count);
    float total = (float)(count - 1) * step;
    float y0 = 80.0f + (120.0f - total) * 0.5f;
    return y0 + (float)slot * step;
}

static int chanStepSize(int chan) {
    return (chan == CH_SPEED) ? 1 : (chan == CH_DEPTH) ? 5 : 8;
}

static void commitSliderChange(void) {
    applyLed();
    s_savePending = SAVE_DEBOUNCE_FRAMES;
}

/* Ajusta um canal por D-pad (passo proprio) e agenda o save. */
static void chanStep(int chan, int dir) {
    chanSet(chan, chanGet(chan) + dir * chanStepSize(chan));
    applyLed();
    if (s_savePending == 0) s_savePending = SAVE_DEBOUNCE_FRAMES;
}

/* Valor do canal a partir do X do toque na barra; marca drag + selecao. */
static void chanFromTouch(const AppInput* in, int chan, int slot, float barX, float barW) {
    int mn, mx; int* ip; ColorRGBA col; const char* lbl;
    chanInfo(chan, &ip, &mn, &mx, &col, &lbl);
    float t = clampf(((float)in->touchPX - barX) / barW, 0.0f, 1.0f);
    chanSet(chan, clampi(mn + (int)(t * (mx - mn) + 0.5f), mn, mx));
    s_dragChan = chan;
    s_selected = slot + 1;
}

/* Slider 3.5: thumb cresce 1.0->1.18 ao iniciar o arrasto (easeOutBack) e
 * volta a 1.0 ao soltar (easeOutCubic); o valor exibido conta animado
 * (180ms easeOutCubic) sempre que o valor real mudar -- chamado uma vez por
 * frame, incondicional, pra nunca perder um tick mesmo com os returns
 * antecipados do tratamento de toque abaixo. */
static void updateSliderTweens(float dt) {
    static int s_prevDragChan = -1;
    if (s_dragChan != s_prevDragChan) {
        if (s_prevDragChan >= 0 && s_prevDragChan < CH_COUNT)
            tweenStart(&s_thumbTween[s_prevDragChan], tweenValue(&s_thumbTween[s_prevDragChan]), 1.0f, 0.18f, EASE_OUT_CUBIC);
        if (s_dragChan >= 0 && s_dragChan < CH_COUNT)
            tweenStart(&s_thumbTween[s_dragChan], tweenValue(&s_thumbTween[s_dragChan]), 1.18f, 0.18f, EASE_OUT_BACK);
        s_prevDragChan = s_dragChan;
    }
    for (int k = 0; k < CH_COUNT; k++) tweenUpdate(&s_thumbTween[k], dt);

    /* contagem animada por canal (indexado por LedChan, persiste entre modos). */
    for (int k = 0; k < CH_COUNT; k++) {
        int rv = chanGet(k);
        if (s_lastDisplayed[k] != rv) {
            float from = (s_lastDisplayed[k] < 0) ? (float)rv : tweenValue(&s_valTween[k]);
            tweenStart(&s_valTween[k], from, (float)rv, 0.18f, EASE_OUT_CUBIC);
            s_lastDisplayed[k] = rv;
        }
        tweenUpdate(&s_valTween[k], dt);
    }
}

void ledUpdate(const AppInput* in, float dt, int* currentScreen) {
    updateSliderTweens(dt);
    if (in->back) {
        s_dragChan = -1;
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    int kinds[CH_COUNT];
    int nsl = ledSliderKinds(kinds);
    int totalItems = 1 + nsl; /* item 0 = modo; 1..nsl = sliders do modo atual */

    bool dragging = (s_dragChan >= 0 && in->touchHeld);

    /* esq/dir SO troca o modo quando o item de MODO (selected 0) esta focado;
     * num slider, esq/dir ajusta o valor (bloco abaixo). */
    bool modeChanged = false;
    if ((in->left || in->right) && s_selected == 0) {
        int dir = in->right ? 1 : -1;
        setMode((s_mode + dir + LED_MODE_COUNT) % LED_MODE_COUNT);
        modeChanged = true;
    }

    if (!dragging && !modeChanged) {
        if (in->downNav) { s_selected = (s_selected + 1) % totalItems; s_dragChan = -1; }
        if (in->up) { s_selected = (s_selected - 1 + totalItems) % totalItems; s_dragChan = -1; }

        if (s_selected == 0) {
            if (in->confirm) setMode((s_mode + 1) % LED_MODE_COUNT);
        } else if (s_selected - 1 < nsl) {
            int chan = kinds[s_selected - 1];
            if (in->left) chanStep(chan, -1);
            if (in->right) chanStep(chan, +1);
        }
    }

    if (in->touchDown || (in->touchHeld && s_dragChan >= 0)) {
        /* segmented de modo (16,18,288,38) -- deve bater com ledRenderBottom. */
        if (!dragging && in->touchPY >= 18 && in->touchPY < 56 &&
            in->touchPX >= 16 && in->touchPX < 304) {
            float segItemW = 288.0f / (float)LED_MODE_COUNT;
            int newMode = clampi((int)((in->touchPX - 16) / segItemW), 0, LED_MODE_COUNT - 1);
            if (in->touchDown) { s_selected = 0; setMode(newMode); }
            return;
        }

        float barX = 44.0f, barW = 236.0f, rowH = 30.0f;
        for (int slot = 0; slot < nsl; slot++) {
            int chan = kinds[slot];
            float sy = ledRowY(slot, nsl);
            if ((dragging && s_dragChan == chan) ||
                (!dragging && in->touchPY >= sy - 6 && in->touchPY < sy + rowH &&
                 in->touchPX >= barX && in->touchPX < barX + barW)) {
                if (in->touchDown || (dragging && s_dragChan == chan)) {
                    chanFromTouch(in, chan, slot, barX, barW);
                    commitSliderChange();
                }
                return;
            }
        }
    }

    if (!in->touchHeld) s_dragChan = -1;

    /* seguranca: se o total de itens encolheu (troca de modo), nao deixa a
     * selecao "presa" num slot que nao existe mais. */
    if (s_selected >= totalItems) s_selected = 0;

    if (s_savePending > 0) {
        s_savePending--;
        if (s_savePending == 0) saveLed();
    }
}

static ColorRGBA previewColor(void) {
    if (s_mode == LED_MODE_OFF) return (ColorRGBA){24, 24, 32, 255};
    if (s_mode == LED_SOLID) return (ColorRGBA){s_r, s_g, s_b, 255};
    if (s_mode == LED_PULSE) {
        /* 1.5.0: preview respeita a PROFUNDIDADE (piso da onda) -- assim o dono
         * ve na bolinha exatamente o quanto vai escurecer no console. */
        float floorB = clampf(1.0f - (float)s_pulseDepth / 100.0f, 0.0f, 1.0f);
        float wave = floorB + (1.0f - floorB) * (0.5f + 0.5f * sinf(uiFrameTime() * (float)s_speed));
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
    /* 1.8.0 CAELESTIA §idle: no modo Pulso o anel "respira" 1.0->1.04 na
     * frequencia do LED (calmo, so nesse modo). */
    float ringS = 1.0f;
    if (s_mode == LED_PULSE)
        ringS = 1.0f + 0.04f * (0.5f + 0.5f * sinf(uiFrameTime() * (float)s_speed));
    float fr = 40.0f * ringS;
    ColorRGBA fill = c; fill.a = 102;                 /* ~40% */
    UI_RoundRect(cx - fr, cy - fr, fr * 2.0f, fr * 2.0f, fr, fill);
    ColorRGBA ring = c; ring.a = 255;
    UI_RingCircle(cx, cy, 86.0f * ringS, ring);

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

/* 1.5.0 slider cakeOS/reva: label a esquerda + trilho pill + preenchimento na
 * cor do canal + KNOB reva (base branca + anel GROSSO na cor, cresce ao
 * arrastar) + numero numa COLUNA fixa a direita, tudo centralizado na vertical
 * da barra (o numero antes flutuava solto em cima, desalinhado). */
static void drawFlatSlider(C2D_TextBuf buf, const char* label, int displayValue,
                           int value, int min, int max, float y, ColorRGBA chan,
                           bool focused, float slideX, float thumbScale) {
    ColorRGBA lblC = focused ? UI_AccentAnim() : g_theme.textSecondary;
    float barX = 42.0f + slideX, barW = 224.0f, barH = 12.0f, barY = y + 2.0f;
    float cy = barY + barH * 0.5f;

    /* 1.6.1: foco do slider = FAIXA preenchida (accent suave) atras da linha
     * inteira + label em accent, no lugar do aro fino no trilho. */
    if (focused) {
        ColorRGBA soft = UI_AccentAnim(); soft.a = themeIsDark() ? 34 : 30;
        UI_RoundRect(12.0f + slideX, y - 4.0f, SCREEN_BOT_WIDTH - 24.0f, barH + 12.0f, 10.0f, soft);
    }

    UI_Text(buf, NULL, label, 18.0f + slideX, cy - 8.0f, 0.26f, 0.26f, lblC);

    ColorRGBA track = {255, 255, 255, (u8)(themeIsDark() ? 20 : 32)};
    if (UI_AssetsReady()) UI_NinePill(barX, barY, barW, barH, track);
    else UI_RoundRect(barX, barY, barW, barH, barH * 0.5f, track);

    /* 1.9.0 FIX4: o PREENCHIMENTO e o KNOB seguem o valor ANIMADO (displayValue,
     * ja tweened), nao o valor cru -> a barra escorre ate o novo valor em vez de
     * saltar (antes so o numero animava). */
    float t = (max > min) ? ((float)displayValue - (float)min) / (float)(max - min) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);
    if (t > 0.001f) {
        float fw = fmaxf(barH, barW * t);
        if (UI_AssetsReady()) UI_NinePill(barX, barY, fw, barH, chan);
        else UI_RoundRect(barX, barY, fw, barH, barH * 0.5f, chan);
    }

    /* knob reva: base branca + anel grosso (ICON_SWATCH_THICK) na cor do canal.
     * elevacao 1->2 no foco (lift do knob). */
    float knobX = barX + barW * t;
    float kr = 9.0f * thumbScale;
    UI_Elevation(knobX - kr, cy - kr, kr * 2.0f, kr * 2.0f, kr, focused ? 2 : 1, 1.0f);
    UI_RoundRect(knobX - kr, cy - kr, kr * 2.0f, kr * 2.0f, kr, (ColorRGBA){255, 255, 255, 255});
    ColorRGBA ring = chan; ring.a = 255;
    iconsDraw(ICON_SWATCH_THICK, knobX, cy, kr * 2.0f, ring, 1.0f);

    char v[8]; snprintf(v, sizeof(v), "%d", displayValue);
    UI_TextRight(buf, NULL, v, SCREEN_BOT_WIDTH - 14.0f + slideX, cy - 8.0f, 0.24f, 0.24f, g_theme.textSecondary);
}

void ledRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_BottomBackground();

    /* segmented de modo (16,18,288,38). */
    const char* modeLabels[LED_MODE_COUNT];
    for (int i = 0; i < LED_MODE_COUNT; i++) modeLabels[i] = modeNameI(i);
    UI_TouchBarSegmented(buf, 16.0f + slideX, 18.0f, 288.0f, 38.0f, modeLabels, LED_MODE_COUNT, s_mode, &s_segTween, s_selected == 0);

    /* 1.5.0: sliders DO MODO ATUAL (Off nao tem nenhum; Fixo so R/G/B; Pulso
     * ganha Profundidade), centralizados verticalmente. */
    int kinds[CH_COUNT];
    int nsl = ledSliderKinds(kinds);
    for (int slot = 0; slot < nsl; slot++) {
        int chan = kinds[slot];
        int mn, mx; int* ip; ColorRGBA col; const char* lbl;
        chanInfo(chan, &ip, &mn, &mx, &col, &lbl);
        float y = ledRowY(slot, nsl);
        drawFlatSlider(buf, lbl, (int)(tweenValue(&s_valTween[chan]) + 0.5f),
                       chanGet(chan), mn, mx, y, col, s_selected == slot + 1, slideX,
                       tweenValue(&s_thumbTween[chan]));
    }

    UI_HelpBar(buf, T(STR_HELP_LED_L), T(STR_SAIR));

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, veil);
    }
}
