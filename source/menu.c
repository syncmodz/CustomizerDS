#include "menu.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "fonts.h"
#include "led.h"
#include "anim.h"
#include "icons.h"
#include "lang.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    StringId titleId;  /* i18n: titulo via T() em tempo de render */
    int iconImg;       /* IconID -- glifo real do Reva UI, ver icons.h */
    StringId subId;    /* i18n: subtitulo via T() */
    ScreenType target;
} MenuItem;

static const MenuItem ITEMS[] = {
    { STR_ITEM_FONTS, ICON_FONTS, STR_ITEM_FONTS_SUB, SCREEN_FONTS },
    { STR_ITEM_THEME, ICON_THEME, STR_ITEM_THEME_SUB, SCREEN_DARKMODE },
    { STR_ITEM_LED,   ICON_LED,   STR_ITEM_LED_SUB,   SCREEN_LED },
};

static int s_selected = 0;

/* §1: o emblema da Home virou as 3 bolinhas glass da boot (UI_Emblem). Em
 * scale 1.0 o conjunto tem ~74px de altura; 0.70 -> ~52px, o mesmo footprint
 * do antigo appicon (iconsDrawFixed 52px). */
/* §2: ligado durante o handoff boot->home -- enquanto true, menuRenderTop NAO
 * desenha emblema/wordmark (menuRenderTopHandoff os desenha deslizando). */
static bool s_emblemSuppressed = false;

/* spec v7 Parte C: foco do seletor Fontes/Tema/LED escala 1.0<->~1.08 ao
 * trocar de selecionado (nao e mais um loop ambiente -- ver comentario em
 * UI_PillButtonPress/ui.c). Um Tween por pilula (igual ao padrao de
 * s_thumbTween em led.c) porque a pilula que perde o foco anima de volta a
 * 1.0 ao MESMO tempo que a que ganha anima pra 1.08 -- duas animacoes
 * concorrentes, nao uma so. Press (A/toque) usa um Tween separado, so 1
 * de cada vez (so existe 1 dedo/botao). */
static Tween s_pillFocusTween[3];
static bool s_pillFocusInit = false;
static Tween s_pillPressTween;
static int s_pillPressIdx = -1;

static void pillFocusInit(void) {
    for (int i = 0; i < 3; i++) {
        float v = (i == s_selected) ? 1.08f : 1.0f;
        tweenStart(&s_pillFocusTween[i], v, v, 0.001f, EASE_LINEAR);
        tweenUpdate(&s_pillFocusTween[i], 1.0f);
    }
    tweenStart(&s_pillPressTween, 1.0f, 1.0f, 0.001f, EASE_LINEAR);
    tweenUpdate(&s_pillPressTween, 1.0f);
    s_pillFocusInit = true;
}

/* Calibrado em proto/home-microinteraction.html: easeInOutCubic, 160ms
 * (dentro da janela 150-180ms pedida -- 1.4 do exemplo CSS colado pelo dono
 * era demais pro tamanho do pill na home, por isso 1.08). */
static void pillFocusChange(int oldSel, int newSel) {
    if (oldSel == newSel) return;
    tweenStart(&s_pillFocusTween[oldSel], tweenValue(&s_pillFocusTween[oldSel]), 1.0f, 0.16f, EASE_IN_OUT_CUBIC);
    tweenStart(&s_pillFocusTween[newSel], tweenValue(&s_pillFocusTween[newSel]), 1.08f, 0.16f, EASE_IN_OUT_CUBIC);
}

/* Sol/lua dinamico: o item "Tema" mostra lua no Escuro e sol no Claro. O
 * crossfade do icone do carrossel saiu junto com o carrossel (secao 5); a
 * animacao "grande" de troca de tema (sol girando/morfando) e separada e vive
 * em darkmode.c/theme.c (secao 6). */
static IconID themeIconCurrent(void) { return themeIsDark() ? ICON_THEME : ICON_SUN; }

static IconID resolveIcon(int idx) {
    return (idx == 1) ? themeIconCurrent() : (IconID)ITEMS[idx].iconImg;
}

int menuSelected(void) { return s_selected; }
void menuInit(void) {
    s_selected = 0;
    pillFocusInit();
}

void menuUpdate(const AppInput* in, int* currentScreen) {
    if (!s_pillFocusInit) pillFocusInit();
    for (int i = 0; i < 3; i++) tweenUpdate(&s_pillFocusTween[i], uiFrameDt());
    tweenUpdate(&s_pillPressTween, uiFrameDt());
    if (s_pillPressIdx >= 0 && !in->touchHeld) {
        /* soltou (ou tela trocou) -- volta com easeOutBack leve, como no
         * proto (mousedown 0.96 imediato / mouseup volta com overshoot). */
        tweenStart(&s_pillPressTween, tweenValue(&s_pillPressTween), 1.0f, 0.22f, EASE_OUT_BACK);
        s_pillPressIdx = -1;
    }

    int prevSelected = s_selected;
    if (in->back) return;
    /* Spec v5 4: os 3 cards sao uma fileira HORIZONTAL (carrossel) -- D-pad
     * esquerda/direita navega entre eles, igual aos segmented de Tema/LED.
     * Cima/baixo nao tem mais efeito aqui: nao ha um segundo eixo de foco
     * nesta tela (so a fileira de cards), entao decidimos deixa-los sem
     * acao em vez de inventar um significado artificial pra eles. */
    /* Espec v20: 3 tiles numa fileira -- esquerda/direita move o foco; o anel
     * accent desliza entre eles (UI_FocusRing/§FLUIDEZ) e a micro-escala anima
     * via pillFocusChange. */
    if (in->right) s_selected = (s_selected + 1) % 3;
    if (in->left)  s_selected = (s_selected - 1 + 3) % 3;
    if (s_selected != prevSelected) pillFocusChange(prevSelected, s_selected);

    if (in->touchDown) {
        /* tiles 92x150, y=30, x=22/128/234 (deve bater com menuRenderBottom). */
        const float tileW = 92.0f, tileH = 150.0f, tileY = 30.0f;
        const float tileX[3] = { 22.0f, 128.0f, 234.0f };
        for (int i = 0; i < 3; i++) {
            if (in->touchPX >= tileX[i] && in->touchPX < tileX[i] + tileW &&
                in->touchPY >= tileY && in->touchPY < tileY + tileH) {
                if (i != s_selected) pillFocusChange(s_selected, i);
                s_selected = i;
                *currentScreen = ITEMS[i].target; /* toca o tile = abre */
                return;
            }
        }
    }

    if (in->confirm) {
        *currentScreen = ITEMS[s_selected].target;
    }
}

/* §1 (1.0.2): cascata da tela de baixo, SINCRONIZADA com o emblema de cima.
 * Quando o topo "assenta" (~1.2s) as 3 pilulas deslizam de baixo pra posicao
 * com spring leve, escalonadas; slogan aparece embaixo. As pilulas pousam
 * exatamente onde a Home as desenha (y=60) -> emenda suave com a Home. */
void menuRenderStartupPills(C2D_TextBuf buf, float startupT) {
    UI_BottomBackground();
    /* §2 + espec v20: os 3 TILES da Home sobem/aparecem em cascata no fim da
     * boot, ja na pose final (y=30, x=22/128/234) -> o handoff entrega a tela
     * de baixo sem pulo. */
    const float tileW = 92.0f, tileH = 150.0f, tileY = 30.0f;
    const float tileX[3] = { 22.0f, 128.0f, 234.0f };
    const ColorRGBA sect[3] = {
        {255, 86, 120, 255}, {86, 200, 235, 255}, {95, 215, 130, 255}
    };

    for (int i = 0; i < 3; i++) {
        float p = clampf((startupT - (1.20f + i * 0.10f)) / 0.55f, 0.0f, 1.0f);
        if (p <= 0.0f) continue;
        float e = easeFunc(p, EASE_EMPH_DECEL);
        float slideUp = (1.0f - e) * 40.0f;
        u8 au = (u8)(255 * e);

        float x = tileX[i], y = tileY + slideUp;
        float cx = x + tileW * 0.5f;
        ColorRGBA bg = g_theme.surface; bg.a = au;
        if (UI_AssetsReady()) UI_NineCard(x, y, tileW, tileH, 16.0f, bg);
        else UI_RoundRect(x, y, tileW, tileH, 16.0f, bg);

        ColorRGBA rc = sect[i]; rc.a = au;
        UI_RingCircle(cx, y + 50.0f, 40.0f, rc);
        iconsDrawAuto(resolveIcon(i), cx, y + 50.0f, 18.0f, rc, e);
        ColorRGBA nc = g_theme.textSecondary; nc.a = (u8)((float)nc.a * e);
        UI_TextCenter(buf, NULL, T(ITEMS[i].titleId), cx, y + 102.0f, 0.35f, 0.35f, nc);
    }
}

void menuRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)scaleM;
    float offsetFg = (1.0f - transVal) * 18.0f;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_HOME));

    /* Hero centrado (espec v20): emblema grande centro (200,116) r34 com bob
     * idle +-2px (seno 3.2s), wordmark (200,~168) e subtitulo (200,~194).
     * Suprimido durante o handoff boot->home (os shared-elements deslizam em
     * menuRenderTopHandoff). */
    if (!s_emblemSuppressed) {
        float bob = 2.0f * sinf(uiFrameTime() * (6.28318531f / 3.2f));
        UI_Emblem(200.0f + slideX, 116.0f + offsetFg + bob, 34.0f / 24.0f, uiFrameTime(), 1.0f);
        UI_TextCenter(buf, NULL, "CustomizerDS", 200.0f + slideX, 168.0f + offsetFg,
                      0.54f, 0.54f, g_theme.textPrimary);
        UI_TextCenter(buf, NULL, T(STR_HOME_SLOGAN), 200.0f + slideX, 194.0f + offsetFg,
                      0.32f, 0.32f, g_theme.textSecondary);
    }

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, veil);
    }
}

/* §2: handoff shared-element boot->home (~0.45s, relogio em main.c). Em vez do
 * antigo stagger de "abrir aba" (uiScreenEnter foi removido do fim da boot), a
 * Home se monta enquanto o emblema/wordmark deslizam das poses da BOOT pras da
 * HOME -- continuacao, sem emenda. h = 0..1. */
void menuRenderTopHandoff(C2D_TextBuf buf, float h) {
    h = clampf(h, 0.0f, 1.0f);
    float e = easeFunc(h, EASE_IN_OUT_CUBIC); /* movimento dos shared-elements */
    float a = easeOutCubic(h);                /* fade-in do resto da Home */

    /* 1) Home assentada, com emblema+wordmark SUPRIMIDOS (nos os desenhamos
     *    deslizando no passo 3, por cima do veu). */
    s_emblemSuppressed = true;
    menuRenderTop(buf, 1.0f, 0.0f, 1.0f, 1.0f);

    /* 2) veu que faz o resto da Home (card, vitrine, slogan) emergir do bg. */
    if (a < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - a));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }

    /* 3) emblema: BOOT (centro 200,86 scale 1.0; ver UI_StartupLogo) -> HOME
     *    (centro 200,116 scale 34/24). Ambos centrados em x=200 (espec v20). */
    UI_Emblem(200.0f, lerpf(86.0f, 116.0f, e),
              lerpf(1.0f, 34.0f / 24.0f, e), uiFrameTime(), 1.0f);

    /* 4) wordmark: BOOT centrado (200,138) base 0.34 -> HOME centrado (200,168)
     *    base 0.54 -- aterrissa EXATO no que a Home desenha depois (sem pulo). */
    float wmS = lerpf(0.34f, 0.54f, e);
    UI_TextCenter(buf, NULL, "CustomizerDS",
                  200.0f, lerpf(138.0f, 168.0f, e), wmS, wmS, g_theme.textPrimary);

    /* libera a supressao -> quando o handoff acabar, o render normal volta a
     * desenhar emblema/wordmark na MESMA pose final (continuidade). */
    s_emblemSuppressed = false;
}

void menuRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)fadeA; (void)scaleM;
    UI_BottomBackground();

    /* Espec v20: 3 tiles 92x150, y=30, x=22/128/234. Cada tile = card (foco =
     * card-foco #1F1E26 + anel accent + micro-escala) com anel cakeOS na cor da
     * secao (cx,80) r20 + icone + nome (cx,140). */
    const float tileW = 92.0f, tileH = 150.0f, tileY = 30.0f;
    const float tileX[3] = { 22.0f, 128.0f, 234.0f };
    const ColorRGBA sect[3] = {
        {255, 86, 120, 255},  /* Fontes = rosa  */
        { 86, 200, 235, 255}, /* Tema   = ciano */
        { 95, 215, 130, 255}, /* LED    = verde */
    };

    for (int i = 0; i < 3; i++) {
        bool sel = (i == s_selected);
        /* pop-in escalonado (EZ_DECEL 92->100%) ao entrar na aba. */
        float ap = easeFunc(clampf((UI_EnterSeconds() - i * 0.04f) / 0.26f, 0.0f, 1.0f), EASE_EMPH_DECEL);
        /* micro-escala de foco ~1.03 (do Tween 1.0<->1.08 suavizado). */
        float fscale = (s_pillFocusInit) ? tweenValue(&s_pillFocusTween[i]) : (sel ? 1.08f : 1.0f);
        float micro = 1.0f + (fscale - 1.0f) * 0.4f;
        float popS = lerpf(0.92f, 1.0f, ap) * micro;

        float cx = tileX[i] + tileW * 0.5f + slideX;
        float cy = tileY + tileH * 0.5f;
        float w = tileW * popS, h = tileH * popS;
        float x = cx - w * 0.5f, y = cy - h * 0.5f;

        ColorRGBA bg = sel ? (ColorRGBA){0x1F, 0x1E, 0x26, 255} : g_theme.surface;
        UI_Shadow(x, y, w, h, 16.0f, sel ? 45 : 24, 2.0f);
        if (UI_AssetsReady()) UI_NineCard(x, y, w, h, 16.0f, bg);
        else UI_RoundFrame(x, y, w, h, 16.0f, bg, (ColorRGBA){255, 255, 255, themeIsDark() ? 12 : 24});
        if (sel) UI_FocusRing(x, y, w, h, 16.0f); /* anel accent nitido (desliza) */

        float ringCy = cy + (80.0f - cy) * (h / tileH);
        float nameCy = cy + (140.0f - cy) * (h / tileH);
        UI_RingCircle(cx, ringCy, 40.0f * popS, sect[i]);
        iconsDrawAuto(resolveIcon(i), cx, ringCy, 18.0f, sect[i], 1.0f);
        ColorRGBA nameC = sel ? g_theme.textPrimary : g_theme.textSecondary;
        UI_TextCenter(buf, NULL, T(ITEMS[i].titleId), cx, nameCy - 8.0f, 0.35f, 0.35f, nameC);
    }

    UI_HelpBar(buf, T(STR_HELP_HOME_L), T(STR_SAIR));

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
