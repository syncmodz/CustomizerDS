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
    ColorRGBA sect;    /* cor da secao (icone) */
} MenuItem;

/* 2.0.0: home vira LISTA VERTICAL data-driven (era carrossel horizontal de 3).
 * Adicionar/remover itens = so mexer aqui; MENU_COUNT deriva do array. */
static const MenuItem ITEMS[] = {
    { STR_ITEM_FONTS, ICON_FONTS, STR_ITEM_FONTS_SUB, SCREEN_FONTS,    {255,  86, 120, 255} },
    { STR_ITEM_THEME, ICON_THEME, STR_ITEM_THEME_SUB, SCREEN_DARKMODE, { 86, 200, 235, 255} },
    { STR_ITEM_LED,   ICON_LED,   STR_ITEM_LED_SUB,   SCREEN_LED,      { 95, 215, 130, 255} },
    { STR_ITEM_SPLASH, ICON_SPLASH, STR_ITEM_SPLASH_SUB, SCREEN_SPLASH, {245, 190,  70, 255} },
    { STR_ITEM_WALL, ICON_WALL, STR_ITEM_WALL_SUB, SCREEN_WALLPAPER, {180, 130, 240, 255} },
    { STR_ITEM_BADGE, ICON_BADGES, STR_ITEM_BADGE_SUB, SCREEN_BADGE, {240, 120, 180, 255} },
    { STR_ITEM_HOMEUI, ICON_HOMEUI, STR_ITEM_HOMEUI_SUB, SCREEN_HOMEUI, {120, 200, 180, 255} },
};
#define MENU_COUNT ((int)(sizeof(ITEMS) / sizeof(ITEMS[0])))
#define MENU_MAX 12  /* teto p/ o array de tweens (itens futuros: splash/wallpaper/badge/home-ui) */

/* Geometria da lista vertical (compartilhada por update/render/cascata). */
#define MENU_MARGIN  16.0f
#define MENU_ROW_H   46.0f
#define MENU_ROW_GAP  6.0f
#define MENU_TOP     12.0f
#define MENU_VISIBLE  4    /* linhas visiveis antes de rolar */

static int s_selected = 0;
/* rolagem com mola (reaproveita o padrao da lista de Fontes). */
static int s_scrollTop = 0;
static float s_scrollAnim = 0.0f;
static float s_scrollVel = 0.0f;

/* §2: ligado durante o handoff boot->home -- enquanto true, menuRenderTop NAO
 * desenha emblema/wordmark (menuRenderTopHandoff os desenha deslizando). */
static bool s_emblemSuppressed = false;

/* micro-escala de foco por item (1.0<->1.06). Um Tween por item. */
static Tween s_pillFocusTween[MENU_MAX];
static bool s_pillFocusInit = false;

static void pillFocusInit(void) {
    for (int i = 0; i < MENU_COUNT; i++) {
        float v = (i == s_selected) ? 1.06f : 1.0f;
        tweenStart(&s_pillFocusTween[i], v, v, 0.001f, EASE_LINEAR);
        tweenUpdate(&s_pillFocusTween[i], 1.0f);
    }
    s_pillFocusInit = true;
}

static void pillFocusChange(int oldSel, int newSel) {
    if (oldSel == newSel) return;
    tweenStart(&s_pillFocusTween[oldSel], tweenValue(&s_pillFocusTween[oldSel]), 1.0f, DUR_SPATIAL_FAST, EASE_EXPR_SPATIAL);
    tweenStart(&s_pillFocusTween[newSel], tweenValue(&s_pillFocusTween[newSel]), 1.06f, DUR_SPATIAL_FAST, EASE_EXPR_SPATIAL);
}

/* Sol/lua dinamico: o item "Tema" mostra lua no Escuro e sol no Claro. */
static IconID themeIconCurrent(void) { return themeIsDark() ? ICON_THEME : ICON_SUN; }
static IconID resolveIcon(int idx) {
    return (ITEMS[idx].target == SCREEN_DARKMODE) ? themeIconCurrent() : (IconID)ITEMS[idx].iconImg;
}

/* rect da linha i (com rolagem animada). Compartilhado. */
static void menuRowRect(int i, float slideX, float* rx, float* ry, float* rw, float* rh) {
    *rx = MENU_MARGIN + slideX;
    *rw = (float)SCREEN_BOT_WIDTH - 2.0f * MENU_MARGIN;
    *ry = MENU_TOP + ((float)i - s_scrollAnim) * (MENU_ROW_H + MENU_ROW_GAP);
    *rh = MENU_ROW_H;
}

static void menuClampScroll(void) {
    if (s_selected < s_scrollTop) s_scrollTop = s_selected;
    if (s_selected >= s_scrollTop + MENU_VISIBLE) s_scrollTop = s_selected - MENU_VISIBLE + 1;
    int maxTop = MENU_COUNT - MENU_VISIBLE; if (maxTop < 0) maxTop = 0;
    if (s_scrollTop > maxTop) s_scrollTop = maxTop;
    if (s_scrollTop < 0) s_scrollTop = 0;
}

int menuSelected(void) { return s_selected; }
void menuInit(void) {
    s_selected = 0;
    s_scrollTop = 0;
    s_scrollAnim = 0.0f;
    s_scrollVel = 0.0f;
    pillFocusInit();
}

void menuUpdate(const AppInput* in, int* currentScreen) {
    if (!s_pillFocusInit) pillFocusInit();
    for (int i = 0; i < MENU_COUNT; i++) tweenUpdate(&s_pillFocusTween[i], uiFrameDt());

    int prevSelected = s_selected;
    if (in->back) return;

    /* 2.0.0: lista VERTICAL -- cima/baixo move o foco (com wrap); o anel accent
     * desliza e a micro-escala anima. */
    if (in->downNav) s_selected = (s_selected + 1) % MENU_COUNT;
    if (in->up)      s_selected = (s_selected - 1 + MENU_COUNT) % MENU_COUNT;
    if (s_selected != prevSelected) { pillFocusChange(prevSelected, s_selected); menuClampScroll(); }

    if (in->touchDown) {
        for (int i = 0; i < MENU_COUNT; i++) {
            float x, y, w, h; menuRowRect(i, 0.0f, &x, &y, &w, &h);
            if (in->touchPX >= x && in->touchPX < x + w &&
                in->touchPY >= y && in->touchPY < y + h) {
                if (i != s_selected) { pillFocusChange(s_selected, i); s_selected = i; menuClampScroll(); }
                *currentScreen = ITEMS[i].target; /* toca a linha = abre */
                return;
            }
        }
    }

    if (in->confirm) *currentScreen = ITEMS[s_selected].target;
}

/* desenha UMA linha da lista (usado pela home e pela cascata do boot). */
static void menuDrawRow(C2D_TextBuf buf, int i, float x, float y, float w, float h,
                        bool sel, float alpha, float fscale) {
    float cx = x + w * 0.5f, cy = y + h * 0.5f;
    /* micro-escala de foco centrada. */
    float micro = 1.0f + (fscale - 1.0f) * 0.5f;
    float dw = w * micro, dh = h * micro;
    float dx = cx - dw * 0.5f, dy = cy - dh * 0.5f;

    ColorRGBA bg = sel ? themeCardSelBg() : g_theme.surface;
    bg.a = (u8)((float)bg.a * alpha);
    float liftT = clampf((fscale - 1.0f) / 0.06f, 0.0f, 1.0f);
    UI_Elevation(dx, dy, dw, dh, 14.0f, 2, alpha);
    if (liftT > 0.01f) UI_Elevation(dx, dy, dw, dh, 14.0f, 3, liftT * alpha);
    if (UI_AssetsReady()) UI_NineCard(dx, dy, dw, dh, 14.0f, bg);
    else UI_RoundFrame(dx, dy, dw, dh, 14.0f, bg, (ColorRGBA){255, 255, 255, (u8)((themeIsDark() ? 12 : 24) * alpha)});
    if (sel) UI_FocusRing(dx, dy, dw, dh, 14.0f);

    /* icone da secao a esquerda. */
    ColorRGBA ic = ITEMS[i].sect;
    iconsDrawAuto(resolveIcon(i), dx + 26.0f, cy, 24.0f, ic, alpha);

    /* titulo + subtitulo a direita do icone. */
    ColorRGBA nameC = sel ? g_theme.textPrimary : g_theme.textSecondary;
    nameC.a = (u8)((float)nameC.a * alpha);
    ColorRGBA subC = g_theme.textHint; subC.a = (u8)((float)subC.a * alpha);
    UI_Text(buf, NULL, T(ITEMS[i].titleId), dx + 50.0f, cy - 13.0f, 0.28f, 0.28f, nameC);
    UI_Text(buf, NULL, T(ITEMS[i].subId), dx + 50.0f, cy + 4.0f, 0.19f, 0.19f, subC);

    /* chevron sutil a direita indicando "abre". */
    ColorRGBA chev = ITEMS[i].sect; chev.a = (u8)(150 * alpha);
    UI_TextRight(buf, NULL, ">", dx + dw - 14.0f, cy - 11.0f, 0.30f, 0.30f, chev);
}

/* §1: cascata da tela de baixo no fim da boot -- as linhas da lista sobem/aparecem
 * escalonadas, ja na pose final -> handoff sem pulo. */
void menuRenderStartupPills(C2D_TextBuf buf, float startupT) {
    UI_BottomBackground();
    for (int i = 0; i < MENU_COUNT; i++) {
        float p = clampf((startupT - (1.20f + i * 0.10f)) / 0.55f, 0.0f, 1.0f);
        if (p <= 0.0f) continue;
        float e = easeFunc(p, EASE_EMPH_DECEL);
        float x, y, w, h; menuRowRect(i, 0.0f, &x, &y, &w, &h);
        y += (1.0f - e) * 28.0f; /* sobe pra pose */
        menuDrawRow(buf, i, x, y, w, h, false, e, 1.0f);
    }
}

void menuRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)scaleM;
    float offsetFg = (1.0f - transVal) * 18.0f;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_HOME));

    if (!s_emblemSuppressed) {
        float bob = 2.5f * sinf(uiFrameTime() * (6.28318531f / 3.2f));
        float heroFloat = 2.0f * sinf(uiFrameTime() * (6.28318531f / 4.6f) + 1.6f);
        UI_Emblem(200.0f + slideX, 112.0f + offsetFg + bob, 26.0f / 24.0f, uiFrameTime(), 1.0f);
        UI_TextCenter(buf, NULL, "CustomizerDS", 200.0f + slideX, 162.0f + offsetFg + heroFloat,
                      0.54f, 0.54f, g_theme.textPrimary);
        UI_TextCenter(buf, NULL, T(STR_HOME_SLOGAN), 200.0f + slideX, 188.0f + offsetFg + heroFloat * 1.3f,
                      0.32f, 0.32f, g_theme.textSecondary);
    }

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, veil);
    }
}

/* §2: handoff shared-element boot->home (tela de cima; inalterado). */
void menuRenderTopHandoff(C2D_TextBuf buf, float h) {
    h = clampf(h, 0.0f, 1.0f);
    float e = easeFunc(h, EASE_IN_OUT_CUBIC);
    float a = easeOutCubic(h);

    s_emblemSuppressed = true;
    menuRenderTop(buf, 1.0f, 0.0f, 1.0f, 1.0f);

    if (a < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - a));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }

    UI_Emblem(200.0f, lerpf(86.0f, 112.0f, e),
              lerpf(1.0f, 26.0f / 24.0f, e), uiFrameTime(), 1.0f);
    float wmS = lerpf(0.34f, 0.54f, e);
    UI_TextCenter(buf, NULL, "CustomizerDS",
                  200.0f, lerpf(138.0f, 162.0f, e), wmS, wmS, g_theme.textPrimary);
    s_emblemSuppressed = false;
}

void menuRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_BottomBackground();

    menuClampScroll();
    /* rolagem com mola (igual a lista de Fontes). */
    {
        float dt = uiFrameDt();
        float target = (float)s_scrollTop;
        float diff = target - s_scrollAnim;
        const float k = 210.0f, damp = 21.0f;
        s_scrollVel += (diff * k - s_scrollVel * damp) * dt;
        s_scrollAnim += s_scrollVel * dt;
        if (fabsf(diff) < 0.001f && fabsf(s_scrollVel) < 0.01f) { s_scrollAnim = target; s_scrollVel = 0.0f; }
    }

    const float listTop = MENU_TOP, listBot = MENU_TOP + MENU_VISIBLE * (MENU_ROW_H + MENU_ROW_GAP);
    for (int i = 0; i < MENU_COUNT; i++) {
        float x, y, w, h; menuRowRect(i, slideX, &x, &y, &w, &h);
        if (y + h < listTop - 2.0f || y > listBot + 2.0f) continue; /* fora da janela */
        bool sel = (i == s_selected);
        /* pop-in escalonado ao entrar na aba. */
        float ap = easeFunc(clampf((UI_EnterSeconds() - i * 0.04f) / 0.26f, 0.0f, 1.0f), EASE_EMPH_DECEL);
        float fscale = (s_pillFocusInit) ? tweenValue(&s_pillFocusTween[i]) : (sel ? 1.06f : 1.0f);
        menuDrawRow(buf, i, x, y, w, h, sel, ap, fscale);
    }

    /* scrollbar fina se houver mais itens que cabem. */
    if (MENU_COUNT > MENU_VISIBLE) {
        float sbX = SCREEN_BOT_WIDTH - 6.0f + slideX;
        float sbH = MENU_VISIBLE * (MENU_ROW_H + MENU_ROW_GAP) - MENU_ROW_GAP;
        ColorRGBA track = g_theme.textHint; track.a = 40;
        UI_RoundRect(sbX, listTop, 3.0f, sbH, 1.5f, track);
        float thumbH = sbH * (float)MENU_VISIBLE / (float)MENU_COUNT;
        float thumbY = listTop + sbH * s_scrollAnim / (float)MENU_COUNT;
        UI_RoundRect(sbX, thumbY, 3.0f, thumbH, 1.5f, UI_AccentAnim());
    }

    UI_HelpBar(buf, T(STR_HELP_HOME_L), T(STR_SAIR));

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
