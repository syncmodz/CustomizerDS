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

/* Home (PROMPT v8 secao 5): a tela de CIMA mostra UMA funcao por vez,
 * deslizando suave ao navegar item<->item -- o carrossel estilo Xbox (3
 * camadas com peek/parallax) saiu. s_dispSelected e o item exibido (assenta
 * em s_selected ao fim do slide); s_slideT 0->1; s_slideDir +1 (novo entra
 * pela direita) / -1 (esquerda); s_slideFrom e o item saindo. A tela de BAIXO
 * (menuRenderBottom) fica IGUAL. A entrada NA aba Home (vinda de outra tela) e
 * do compositor (secao 2/3) -- este slide e so navegacao interna da Home. */
static int s_dispSelected = 0;
static int s_slideFrom = 0;
static int s_slideDir = 0;
static float s_slideT = 1.0f;   /* 1 = assentado */
#define HOME_SLIDE_DUR 0.26f    /* secao 5: dt/0.26 */
#define HOME_SLIDE_W 368.0f     /* largura do conteudo = distancia do slide */

/* Sol/lua dinamico: o item "Tema" mostra lua no Escuro e sol no Claro. O
 * crossfade do icone do carrossel saiu junto com o carrossel (secao 5); a
 * animacao "grande" de troca de tema (sol girando/morfando) e separada e vive
 * em darkmode.c/theme.c (secao 6). */
static IconID themeIconCurrent(void) { return themeIsDark() ? ICON_THEME : ICON_SUN; }

static IconID resolveIcon(int idx) {
    return (idx == 1) ? themeIconCurrent() : (IconID)ITEMS[idx].iconImg;
}

/* Conteudo "vitrine" de UM item da Home (secao 5): icone grande + titulo +
 * valor, centrado em (cx,cy). alpha 0..1 modula tudo (usado durante o slide). */
static void drawHomeShowcase(C2D_TextBuf buf, IconID icon, const char* title,
                             const char* value, float cx, float cy, float alpha) {
    float a = clampf(alpha, 0.0f, 1.0f);
    ColorRGBA tc = g_theme.textPrimary; tc.a = (u8)((float)tc.a * a);
    ColorRGBA vc = g_theme.accent;       vc.a = (u8)((float)vc.a * a);
    /* §3b: lua = prata #E8EAF0 (nao-sticker aqui, senao fica preta no escuro);
     * sol/appicon = cor propria (sticker); Aa/raio = ACCENT. Resolve a tela de
     * cima nos 2 modos. */
    ColorRGBA silver = {232, 234, 240, 255};
    float cyIcon = cy - 24.0f, sz = 32.0f;
    if (icon == ICON_THEME)       iconsDraw(icon, cx, cyIcon, sz, silver, a);
    else if (iconIsSticker(icon)) iconsDrawFixed(icon, cx, cyIcon, sz, a);
    else                          iconsDraw(icon, cx, cyIcon, sz, g_theme.accent, a);
    UI_TextCenter(buf, NULL, title, cx, cy - 2.0f, 0.5f, 0.5f, tc);
    /* §6: valor (nome da fonte pode ser longo) com auto-shrink ate caber no card. */
    UI_TextCenterFit(buf, NULL, value, cx, cy + 26.0f, 0.30f, 0.22f, 330.0f, vc);
}

int menuSelected(void) { return s_selected; }
void menuInit(void) {
    s_selected = 0;
    s_dispSelected = 0;
    s_slideFrom = 0;
    s_slideDir = 0;
    s_slideT = 1.0f;
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
    /* Travado durante a animacao (igual ao prototipo web: "if (animating)
     * return") -- evita reentrar o slot-permutation a meio de uma troca. */
    /* So aceita nova navegacao quando o slide anterior assentou (s_slideT>=1),
     * igual ao "if (animating) return" do proto -- evita reentrar no meio. */
    if (in->right && s_slideT >= 1.0f) {
        s_slideFrom = s_dispSelected;
        s_slideDir = 1;
        s_slideT = 0.0f;
        s_selected = (s_selected + 1) % 3;
    }
    if (in->left && s_slideT >= 1.0f) {
        s_slideFrom = s_dispSelected;
        s_slideDir = -1;
        s_slideT = 0.0f;
        s_selected = (s_selected - 1 + 3) % 3;
    }
    if (s_selected != prevSelected) pillFocusChange(prevSelected, s_selected);

    if (in->touchDown) {
        float tbY = 60.0f; /* deve bater com menuRenderBottom */
        float btnW = 90.0f;
        float btnH = 32.0f;
        float gap = 12.0f;
        float totalW = btnW * 3 + gap * 2;
        float startX = (SCREEN_BOT_WIDTH - totalW) * 0.5f;
        for (int i = 0; i < 3; i++) {
            float bx = startX + i * (btnW + gap);
            if (in->touchPY >= tbY && in->touchPY < tbY + btnH &&
                in->touchPX >= bx && in->touchPX < bx + btnW) {
                if (i != s_selected) pillFocusChange(s_selected, i);
                s_selected = i;
                s_pillPressIdx = i;
                tweenStart(&s_pillPressTween, tweenValue(&s_pillPressTween), 0.96f, 0.09f, EASE_OUT_CUBIC);
                *currentScreen = ITEMS[i].target;
                return;
            }
        }
        float descY = 110.0f; /* deve bater com menuRenderBottom */
        if (in->touchPY >= descY && in->touchPY < descY + 70 &&
            in->touchPX >= 20 && in->touchPX < 300) {
            *currentScreen = ITEMS[s_selected].target;
            return;
        }
    }

    if (in->confirm) {
        *currentScreen = ITEMS[s_selected].target;
    }
}

void menuRenderStartupPills(C2D_TextBuf buf, float startupT) {
    UI_TouchBarBackground();

    float tbY = 12.0f;
    float btnW = 90.0f;
    float btnH = 32.0f;
    float gap = 12.0f;
    float totalW = btnW * 3 + gap * 2;
    float startX = (SCREEN_BOT_WIDTH - totalW) * 0.5f;

    /* Spec v2 3.1: stagger em t=600/680/760ms, sobe 16px + fade + escala
     * 0.96->1.0, easeOutCubic, 340ms cada -- timings exatos, nao aproximados. */
    static const float START_T[3] = { 0.60f, 0.68f, 0.76f };
    const float DUR = 0.34f;

    for (int i = 0; i < 3; i++) {
        float p = clampf((startupT - START_T[i]) / DUR, 0.0f, 1.0f);
        if (p <= 0.0f) continue;
        float e = easeFunc(p, EASE_OUT_CUBIC);
        float s = lerpf(0.96f, 1.0f, e);
        float slide = (1.0f - e) * 16.0f;
        u8 a = (u8)(255 * e);

        float cx = startX + i * (btnW + gap) + btnW * 0.5f;
        float cy = tbY + btnH * 0.5f + slide;
        float w = btnW * s, h = btnH * s;

        bool selected = false;
        ColorRGBA bg = themeIsDark() ? (ColorRGBA){58, 58, 60, a} : (ColorRGBA){226, 226, 230, a};
        UI_RoundRect(cx - w * 0.5f, cy - h * 0.5f, w, h, 14 * s, bg);

        ColorRGBA textCol = g_theme.textSecondary;
        textCol.a = (u8)(((float)textCol.a / 255.0f) * a);
        if (ITEMS[i].iconImg >= 0) {
            ColorRGBA iconCol = textCol; iconCol.a = a;
            iconsDrawAuto(resolveIcon(i), cx, cy - 5 * s, 14.0f * s, iconCol, e);
        }
        UI_TextCenter(buf, NULL, T(ITEMS[i].titleId), cx, cy + 3 * s, 0.26f * s, 0.26f * s, textCol);
        (void)selected;
    }
}

void menuRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    /* Parallax de 3 camadas (Travel Motion): o fundo "atrasa" mais que o
     * card, e o conteudo dentro do card se acomoda primeiro que o card. */
    float offsetRaw = (1.0f - transVal);
    float offset = offsetRaw * 40.0f;
    float offsetFg = offsetRaw * 18.0f;
    UI_TopBackground();
    UI_TopMenuBar(T(STR_TAB_HOME), buf);

    float et = UI_EnterProgress();

    /* Os 3 blobs ambiente decorativos que ficavam aqui (desenhados no fundo
     * ANTES do card) foram removidos: g_theme.surface tem alpha 235 (nao
     * 255), entao o card por cima deixava esses blobs translucidos
     * espiarem por tras -- exatamente as "3-4 bolas sobrepostas" reportadas
     * dentro do card da vitrine (confirmado no screenshot da Inicio). Como
     * ficavam quase inteiramente cobertos pelo card mesmo quando "corretos",
     * nao tinham valor visual que justificasse o risco de ghosting. */
    float cascadeT = clampf((et * 2.0f - 0.0f), 0.0f, 1.0f);
    float cascadeOffset = (1.0f - easeOutCubic(cascadeT)) * 20.0f;

    /* Transicao 3.2: card desliza/escala a partir do ponto central -- o
     * fade real (cobre tudo que ja foi desenhado, ate widgets sem alpha
     * proprio como UI_StatChip) e aplicado por um veu no final da funcao. */
    float cardBaseX = 16, cardBaseY = 30 + offset + cascadeOffset, cardBaseW = 368, cardBaseH = 196;
    float cardW = cardBaseW * scaleM, cardH = cardBaseH * scaleM;
    float cardX = cardBaseX + slideX + (cardBaseW - cardW) * 0.5f;
    float cardY = cardBaseY + (cardBaseH - cardH) * 0.5f;
    UI_Card(cardX, cardY, cardW, cardH, RADIUS_CARD, g_theme.surface);

    /* §11: o placeholder "CDS" (moldura + texto) virou o ICONE real do app
     * (sprite icon_256 do extra.t3x), no mesmo lugar/tamanho, alinhado ao
     * titulo. Sticker (cor propria), entao iconsDrawFixed. */
    iconsDrawFixed(ICON_APPICON, 62 + slideX, 78 + offsetFg + cascadeOffset, 52.0f, 1.0f);

    UI_Text(buf, NULL, "CustomizerDS", 104 + slideX, 50 + offsetFg + cascadeOffset, 0.54f, 0.54f, g_theme.textPrimary);
    UI_Text(buf, NULL, T(STR_HOME_SLOGAN), 104 + slideX, 76 + offsetFg + cascadeOffset, 0.24f, 0.24f, g_theme.accent);

    /* 3 cards de navegacao estilo end4-Monet (v3 3.1): icone Reva + label
     * curto + valor atual, raio RADIUS_CARD, halo contido no selecionado
     * (substitui o antigo UI_StatChip + anel solido por fora). */
    ColorRGBA accentC = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
    const char* NAV_SHORT[3] = { T(STR_NAV_FONTS), T(STR_NAV_THEME), T(STR_NAV_LED) };
    /* §6: nome COMPLETO da fonte (auto-shrink na vitrine, ver drawHomeShowcase). */
    const char* chipValues[3] = { fontsCurrentName(), themeIsDark() ? T(STR_DARK) : T(STR_LIGHT), ledModeName() };
    ColorRGBA chipDots[3] = { g_theme.accent, accentC, ledPreviewColor() };

    /* +24px (era 112): desce a vitrine (icone+nome da opcao) pra nao encostar
     * no icone do app la em cima (feedback do dono). */
    float chipY = 136.0f + offsetFg + cascadeOffset;
    float chipH = 56.0f;
    /* Vitrine de UMA funcao por vez (secao 5): o conteudo (icone+titulo+valor)
     * e desenhado UMA vez quando assentado; durante o slide e desenhado DUAS
     * vezes -- o que sai (s_slideFrom) e o que entra (s_selected) -- como um
     * unico bloco coeso deslizando (sem parallax/cascade, sem 3 camadas). A
     * moldura/header acima fica parada; so o conteudo desliza. */
    if (s_slideT < 1.0f) {
        s_slideT += uiFrameDt() / HOME_SLIDE_DUR;
        if (s_slideT >= 1.0f) { s_slideT = 1.0f; s_dispSelected = s_selected; }
    }

    float rowCx = 200.0f + slideX;
    float showcaseY = chipY + chipH * 0.5f; /* mesmo centro vertical de antes */

    if (s_slideT >= 1.0f) {
        drawHomeShowcase(buf, resolveIcon(s_dispSelected), NAV_SHORT[s_dispSelected],
                         chipValues[s_dispSelected], rowCx, showcaseY, 1.0f);
    } else {
        float e = easeOutCubic(s_slideT);
        float dir = (float)s_slideDir;
        float xExit = rowCx - HOME_SLIDE_W * e * dir;        /* sai p/ o lado oposto */
        float xEnter = rowCx + HOME_SLIDE_W * (1.0f - e) * dir; /* entra do lado de dir */
        drawHomeShowcase(buf, resolveIcon(s_slideFrom), NAV_SHORT[s_slideFrom],
                         chipValues[s_slideFrom], xExit, showcaseY, 1.0f);
        drawHomeShowcase(buf, resolveIcon(s_selected), NAV_SHORT[s_selected],
                         chipValues[s_selected], xEnter, showcaseY, 1.0f);
    }

    /* §5: os 3 pontinhos indicadores da home (top) foram removidos -- a
     * vitrine de 1 item ja deixa claro onde voce esta, e eles poluiam. */

    /* Veu de fade real (3.2): cobre TUDO que foi desenhado no card, mesmo
     * widgets sem alpha proprio (UI_StatChip), sem precisar repassar fadeA
     * por dentro de cada um -- em fadeA=1 o veu e invisivel (alpha 0). */
    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }
}

void menuRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    float offset = (1.0f - transVal) * 30.0f;
    float et = UI_EnterProgress();
    UI_BottomBackground();

    /* As 3 pilulas (Fontes/Tema/LED) e o card de info DESCEM juntos pro
     * centro-baixo da tela de baixo (feedback: descer "font/app theme/rgb led",
     * nao so o card). Antes as pilulas ficavam coladas no topo (y=12). */
    float tbY = 60.0f + offset;
    float btnW = 90.0f * scaleM;
    float btnH = 32.0f;
    float gap = 12.0f;
    float totalW = btnW * 3 + gap * 2;
    float startX = (SCREEN_BOT_WIDTH - totalW) * 0.5f + slideX;

    for (int i = 0; i < 3; i++) {
        float bx = startX + i * (btnW + gap);
        /* Stagger 3.2: 40ms entre pilulas, 260ms cada (UI_PillButton aplica
         * easeOutCubic por dentro, entao passamos o progresso linear bruto). */
        float appearT = clampf((UI_EnterSeconds() - i * 0.04f) / 0.26f, 0.0f, 1.0f);
        /* spec v7 Parte C: focusScale por pilula (Tween 1.0<->1.08, ver
         * pillFocusChange acima); pressScale so a pilula sendo apertada
         * (s_pillPressIdx) usa o Tween de press, as outras ficam em 1.0. */
        float focusScale = (s_pillFocusInit) ? tweenValue(&s_pillFocusTween[i]) : ((i == s_selected) ? 1.08f : 1.0f);
        float pressScale = (i == s_pillPressIdx) ? tweenValue(&s_pillPressTween) : 1.0f;
        UI_PillButtonPress(buf, bx, tbY, btnW, btnH,
                            T(ITEMS[i].titleId), NULL, resolveIcon(i),
                            i == s_selected, appearT, pressScale, focusScale);
    }

    /* Card de info logo abaixo das pilulas (que agora estao em y=60). */
    float descY = 110.0f + offset;
    float descAp = clampf((et * 2.0f - 0.15f), 0.0f, 1.0f);
    float da = easeOutCubic(descAp);
    ColorRGBA descBg = themeIsDark()
        ? (ColorRGBA){20, 22, 30, 240}
        : (ColorRGBA){240, 242, 248, 240};
    descBg.a = (u8)((float)descBg.a * da);
    UI_RoundFrame(20 + slideX, descY, 280, 70, 14, descBg, (ColorRGBA){255, 255, 255, (u8)(8 * da)});

    UI_RoundRect(32 + slideX, descY + 8, 36, 36, 10, g_theme.accent);
    ColorRGBA iconText = themeContrastText(g_theme.accent);
    iconText.a = (u8)(255 * da);
    iconsDrawAuto(resolveIcon(s_selected), 50 + slideX, descY + 26, 18.0f, iconText, da);

    ColorRGBA titleC = g_theme.textPrimary;
    titleC.a = (u8)(255 * da);
    UI_Text(buf, NULL, T(ITEMS[s_selected].titleId), 80 + slideX, descY + 8, 0.34f, 0.34f, titleC);
    ColorRGBA subC = g_theme.textSecondary;
    subC.a = (u8)(255 * da);
    UI_Text(buf, NULL, T(ITEMS[s_selected].subId), 80 + slideX, descY + 30, 0.24f, 0.24f, subC);

    ColorRGBA hintC = g_theme.textHint;
    hintC.a = (u8)(180 * da);
    UI_TextCenter(buf, NULL, T(STR_HOME_HINT),
                  160 + slideX, descY + 54, 0.22f, 0.22f, hintC);

    UI_HelpBar(buf, T(STR_HELP_HOME_L), T(STR_SAIR));

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
