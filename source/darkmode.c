#include "darkmode.h"
#include "common.h"
#include "theme.h"
#include "config.h"
#include "ui.h"
#include "anim.h"
#include "fonts.h"
#include "color_picker.h"
#include "lang.h"
#include <math.h>

static int s_selected = 0;
/* Tween do segmentedLozenge Claro/Escuro (UI_TouchBarSegmented). */
static Tween s_segTween;
/* §i18n.6: seletor de idioma PT|EN|ES na aba Tema (segmented proprio). */
static Tween s_langSegTween;
/* espec v20: card de idioma na linha de baixo (196,132,108,32). */
#define LANG_SEG_X 196.0f
#define LANG_SEG_Y 132.0f
#define LANG_SEG_W 108.0f
#define LANG_SEG_H 32.0f
/* Tween do anel de selecao dos swatches de accent (desliza em X, v3 3.3). */
static Tween s_ringTween;
static bool s_hexEditing = false;
static float s_hexEnterT = 0.0f;
/* Origem do toque que abriu o popup do hex (3.3: "nasce na posicao/escala
 * do elemento que o abriu") -- coordenadas da tela de baixo, onde o swatch
 * HEX realmente fica; a tela de cima nao tem um swatch equivalente (sao
 * paineis fisicos diferentes), entao so o popup de baixo usa esta origem. */
static float s_hexOriginX = 14.0f;
static float s_hexOriginY = 56.0f;
/* Fechar (3.3): caminho inverso, easeInCubic, 240ms -- so depois disso
 * s_hexEditing realmente vira false (sem isso o popup desaparecia instantaneo). */
static bool s_hexClosing = false;
static float s_hexCloseT = 0.0f;
/* Bounce 3.4 (swatches): 1.0->1.15->1.0, 300ms, disparado quando o accent
 * selecionado muda (toque, D-pad ou confirm) -- s_swatchBounceT alto = parado. */
static float s_swatchBounceT = 999.0f;
static int s_swatchBounceIdx = -1;
static ColorPicker s_hexPicker;
static PopupModal s_popup;
static float s_popupHoldT = 0.0f;

/* Touch Bar HSV (v3 3.4): barra de espectro continuo como SELECAO GROSSA,
 * os digitos hex continuam como entrada FINA -- as duas vias escrevem no
 * mesmo s_hexPicker, entao ficam sempre sincronizadas. s_hueFocused decide
 * se D-pad cima/baixo/esq/dir controla a barra ou os digitos. */
static float s_hueT = 0.0f;     /* 0..1, posicao na barra (0=vermelho) */
static bool s_hueFocused = false;
static bool s_hueDragging = false;
static Tween s_hueValTween[3]; /* contagem animada de R/G/B (3.4: 180ms easeOutCubic) */
static int s_hueLastRGB[3] = { -1, -1, -1 };

int darkmodeSelected(void) { return s_selected; }

static void saveTheme(void) {
    ConfigData cfg;
    configLoad(&cfg);
    cfg.darkMode = themeIsDark() ? 1 : 0;
    cfg.accentIndex = (u8)themeGetAccentIndex();
    cfg.customAccentFlag = themeAccentIsCustom() ? 1 : 0;
    ColorRGBA cc = themeGetCustomAccent();
    cfg.customR = cc.r;
    cfg.customG = cc.g;
    cfg.customB = cc.b;
    configSave(&cfg);
}

/* §i18n.6: troca o idioma ao vivo e persiste em reserved[0] (§12). */
static void setLang(Lang l) {
    langSet(l);
    ConfigData cfg;
    configLoad(&cfg);
    cfg.reserved[0] = (u8)langGet();
    configSave(&cfg);
}

/* 1.9.0 FIX5: PressFx (StateLayer) no swatch selecionado -- flash branco 10% +
 * fade, o "peso" do toque. Coords base (o swatch fica assentado ao selecionar). */
static PressFx s_swatchPress;
static void triggerSwatchBounce(int idx) {
    s_swatchBounceIdx = idx;
    s_swatchBounceT = 0.0f;
    float scx = 40.0f + idx * 44.0f, scy = 102.0f;
    pressFxTrigger(&s_swatchPress, scx - 17.0f, scy - 17.0f, 34.0f, 34.0f, 17.0f);
}

/* Centro do MiniWindow de preview na tela de cima (winX=32,winW=160,
 * winY=84,winH=116, ver darkmodeRenderTop) -- usado como origem fixa do
 * wipe/icone na tela de cima, que nao tem um "elemento tocado" equivalente
 * ao segmented da tela de baixo. */
#define THEME_WIPE_TOP_ORIGIN_X 112.0f
#define THEME_WIPE_TOP_ORIGIN_Y 142.0f

/* Unico ponto que de fato chama themeSetDark() (spec v5 6): dispara o wipe
 * yin-yang ANTES de themeSetDark() (pra capturar a cor antiga) com origem
 * no segmento de destino do toggle Claro/Escuro (segX=60,segW=100,segY=8,
 * segH=32 -- mesmas constantes de darkmodeRenderBottom). Guard de no-op
 * evita reativar o wipe se por algum motivo dark == estado atual. */
static void applyThemeWithWipe(bool dark) {
    if (themeIsDark() == dark) return;
    float segX = 60.0f, segW = 100.0f, segY = 8.0f, segH = 32.0f;
    float originBotX = segX + (dark ? 1 : 0) * segW + segW * 0.5f;
    float originBotY = segY + segH * 0.5f;
    themeWipeTrigger(THEME_WIPE_TOP_ORIGIN_X, THEME_WIPE_TOP_ORIGIN_Y, originBotX, originBotY);
    themeSetDark(dark);
    saveTheme();
}

static float rgbToHueDeg(u8 r, u8 g, u8 b) {
    float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
    float mx = fmaxf(rf, fmaxf(gf, bf));
    float mn = fminf(rf, fminf(gf, bf));
    float d = mx - mn;
    if (d < 0.0001f) return 0.0f;
    float h;
    if (mx == rf) h = fmodf((gf - bf) / d, 6.0f);
    else if (mx == gf) h = (bf - rf) / d + 2.0f;
    else h = (rf - gf) / d + 4.0f;
    h *= 60.0f;
    if (h < 0.0f) h += 360.0f;
    return h;
}

/* Dispara a contagem animada (3.4: 180ms easeOutCubic) pro novo RGB derivado
 * de s_hueT -- nao escreve em s_hexPicker direto, isso e feito por
 * updateHueValueTweens() enquanto a animacao estiver ativa. */
static void applyHueTarget(void) {
    u8 r, g, b;
    hsvToRgbF(s_hueT * 360.0f, 1.0f, 1.0f, &r, &g, &b);
    int target[3] = { r, g, b };
    for (int k = 0; k < 3; k++) {
        if (s_hueLastRGB[k] != target[k]) {
            float from = (s_hueLastRGB[k] < 0) ? (float)target[k] : tweenValue(&s_hueValTween[k]);
            tweenStart(&s_hueValTween[k], from, (float)target[k], 0.18f, EASE_OUT_CUBIC);
            s_hueLastRGB[k] = target[k];
        }
    }
}

/* Centraliza o que precisa acontecer toda vez que entra no editor de hex --
 * inicializa o picker E a barra de espectro (hueT derivado da cor atual,
 * foco comeca nos digitos, tweens de contagem zerados/inativos). */
static void enterHexEdit(ColorRGBA cur) {
    colorPickerInit(&s_hexPicker);
    s_hexPicker.preview = (Color_RGB){cur.r, cur.g, cur.b};
    s_hexPicker.dispR = cur.r; s_hexPicker.dispG = cur.g; s_hexPicker.dispB = cur.b; /* 1.6.0: sem flash ao abrir */
    snprintf(s_hexPicker.hex_input, sizeof(s_hexPicker.hex_input),
             "%02X%02X%02X", cur.r, cur.g, cur.b);
    s_hueT = rgbToHueDeg(cur.r, cur.g, cur.b) / 360.0f;
    s_hueFocused = false;
    s_hueDragging = false;
    for (int k = 0; k < 3; k++) {
        s_hueValTween[k].active = false;
        s_hueLastRGB[k] = -1;
    }
}

/* So escreve em s_hexPicker enquanto a contagem estiver em andamento --
 * depois que assenta, para de escrever, pra nao atropelar edicao fina por
 * digito (que mexe em s_hexPicker direto, ver color_picker.c::stepDigit). */
static void updateHueValueTweens(float dt) {
    bool animating = s_hueValTween[0].active || s_hueValTween[1].active || s_hueValTween[2].active;
    for (int k = 0; k < 3; k++) tweenUpdate(&s_hueValTween[k], dt);
    if (!animating) return;
    u8 r = (u8)clampi((int)(tweenValue(&s_hueValTween[0]) + 0.5f), 0, 255);
    u8 g = (u8)clampi((int)(tweenValue(&s_hueValTween[1]) + 0.5f), 0, 255);
    u8 b = (u8)clampi((int)(tweenValue(&s_hueValTween[2]) + 0.5f), 0, 255);
    snprintf(s_hexPicker.hex_input, sizeof(s_hexPicker.hex_input), "%02X%02X%02X", r, g, b);
    s_hexPicker.preview = (Color_RGB){ r, g, b };
}

void darkmodeInit(void) {
    s_selected = 0;
    s_hexEditing = false;
    s_hexClosing = false;
    s_popup.active = false;
    s_popupHoldT = 0.0f;
}

/* Popup estilo "Travel Agency" (card escala com easeOutBack + fundo escurece).
 * Em modo confirm (usado abaixo para "aplicar esta cor?") NUNCA some sozinho
 * -- so um toast comum (sem cancelLabel/confirmMode) usaria o auto-hide. */
static void updatePopup(void) {
    popupUpdate(&s_popup);
    if (s_popup.active && !s_popup.confirmMode && s_popup.animT >= 1.0f) {
        s_popupHoldT += uiFrameDt();
        if (s_popupHoldT > 0.9f) { popupHide(&s_popup); s_popupHoldT = 0.0f; }
    }
}

void darkmodeUpdate(const AppInput* in, float dt, int* currentScreen) {
    updatePopup();
    pressFxUpdate(&s_swatchPress, dt, in->touchHeld); /* 1.9.0 FIX5 */
    /* 1.5.0: themeWipeTick NAO roda mais aqui -- roda no laco principal (main.c)
     * TODO frame, em qualquer aba. Antes, sair da aba Tema no meio da animacao
     * de troca claro/escuro congelava o wipe (o timer so avancava aqui), e o
     * split de cor ficava travado nas outras telas ate voltar. */

    if (s_hexEditing) {
        if (s_hexClosing) {
            /* 240ms de "caminho inverso" (easeInCubic) antes de realmente
             * sair do modo de edicao -- sem isso o popup cortava instantaneo. */
            s_hexCloseT += dt;
            if (s_hexCloseT >= 0.24f) {
                s_hexEditing = false;
                s_hexClosing = false;
            }
            return;
        }

        /* Popup de confirmacao "Aplicar esta cor como tema?" (spec v4 4.4):
         * enquanto ele estiver de pe, bloqueia TODO o input do editor de hex
         * por baixo (espectro/digitos) -- so A/B/toque nos selos respondem. */
        if (popupActive(&s_popup)) {
            if (popupConfirmInput(&s_popup, in) && s_popup.result == 1) {
                Color_RGB rgb = hexToRGB(s_hexPicker.hex_input);
                themeSetCustomAccent((ColorRGBA){rgb.r, rgb.g, rgb.b, 255});
                saveTheme();
                s_hexClosing = true;
                s_hexCloseT = 0.0f;
            }
            return;
        }

        s_hexEnterT = fminf(s_hexEnterT + dt, 1.0f);

        /* Touch Bar HSV (3.4): barra de espectro como selecao grossa --
         * stylus/mouse arrastam direto sobre ela, D-pad cima entra em foco
         * vindo do digito 0 e cima/baixo alternam entre barra e digitos. */
        float hueBarX = 10.0f, hueBarY = 8.0f, hueBarW = 300.0f, hueBarH = 28.0f;
        bool overHueBar = in->touchPY >= hueBarY && in->touchPY < hueBarY + hueBarH &&
                          in->touchPX >= hueBarX && in->touchPX < hueBarX + hueBarW;
        if ((in->touchDown || (in->touchHeld && s_hueDragging)) && overHueBar) {
            s_hueFocused = true;
            s_hueDragging = true;
            s_hueT = clampf(((float)in->touchPX - hueBarX) / hueBarW, 0.0f, 1.0f);
            applyHueTarget();
        } else {
            if (!in->touchHeld) s_hueDragging = false;
            /* Tocar em qualquer coisa fora da barra devolve o foco pros
             * digitos -- senao left/right ficava preso ajustando o hue
             * depois de o usuario ja ter tocado num digito especifico. */
            if (in->touchDown) s_hueFocused = false;

            if (s_hueFocused) {
                if (in->down & KEY_DOWN) {
                    s_hueFocused = false;
                } else if (in->left || in->right) {
                    s_hueT = clampf(s_hueT + (in->right ? 1.0f : -1.0f) * (5.0f / 360.0f), 0.0f, 1.0f);
                    applyHueTarget();
                }
            } else if ((in->down & KEY_UP) && s_hexPicker.cursor_pos == 0) {
                s_hueFocused = true;
            } else {
                colorPickerInput(&s_hexPicker, in, 56.0f);
            }
        }
        updateHueValueTweens(dt);

        if (in->confirm) {
            popupShowConfirm(&s_popup, T(STR_CONFIRM_COLOR));
        }
        if (in->back) {
            s_hexClosing = true;
            s_hexCloseT = 0.0f;
        }
        return;
    }

    /* Slots de foco: 0/1 = Claro/Escuro, 2..(langSlot-1) = swatches de accent
     * (inclui HEX), langSlot = seletor de idioma PT|EN|ES (§i18n.6). */
    int langSlot = 2 + themeAccentCount() + 1;
    int total = langSlot + 1;

    if (in->back) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    /* §redesign: navegacao por SECAO. Cima/baixo troca de secao
     * (0=Aparencia, 1=Accent, 2=Idioma) pousando no valor ATIVO da secao;
     * esquerda/direita escolhe DENTRO da secao (blocos abaixo). Acaba com o
     * indice linear que percorria cor-por-cor misturado com os toggles. */
    {
        int curSec = (s_selected < 2) ? 0 : (s_selected < langSlot ? 1 : 2);
        int newSec = curSec;
        if (in->downNav) newSec = (curSec + 1) % 3;
        else if (in->up) newSec = (curSec + 2) % 3;
        if (newSec != curSec) {
            if (newSec == 0)      s_selected = themeIsDark() ? 1 : 0;
            else if (newSec == 1) s_selected = 2 + (themeAccentIsCustom() ? themeAccentCount() : themeGetAccentIndex());
            else                  s_selected = langSlot;
        }
    }
    (void)total;

    if (in->left && s_selected < 2) {
        s_selected = s_selected == 0 ? 1 : 0;
        applyThemeWithWipe(!themeIsDark());
    }
    if (in->right && s_selected < 2) {
        s_selected = s_selected == 0 ? 1 : 0;
        applyThemeWithWipe(!themeIsDark());
    }

    if (in->left && s_selected >= 2 && s_selected < langSlot) {
        int accentTotal = themeAccentCount() + 1;
        int idx = (s_selected - 2 - 1 + accentTotal) % accentTotal;
        s_selected = 2 + idx;
        if (idx < themeAccentCount()) { themeSetAccentIndex(idx); saveTheme(); }
        triggerSwatchBounce(idx);
    }
    if (in->right && s_selected >= 2 && s_selected < langSlot) {
        int accentTotal = themeAccentCount() + 1;
        int idx = (s_selected - 2 + 1) % accentTotal;
        s_selected = 2 + idx;
        if (idx < themeAccentCount()) { themeSetAccentIndex(idx); saveTheme(); }
        triggerSwatchBounce(idx);
    }

    /* Idioma: D-pad esq/dir cicla PT->EN->ES quando o slot de idioma esta focado. */
    if (in->left && s_selected == langSlot) {
        setLang((Lang)((langGet() - 1 + LANG_COUNT) % LANG_COUNT));
    }
    if (in->right && s_selected == langSlot) {
        setLang((Lang)((langGet() + 1) % LANG_COUNT));
    }

    if (in->touchDown || in->touchHeld) {
        /* segmented Claro/Escuro (16,18,288,38) -- Escuro = metade direita. */
        if (in->touchPY >= 18.0f && in->touchPY < 56.0f &&
            in->touchPX >= 16.0f && in->touchPX < 304.0f) {
            bool isDark = (in->touchPX >= 160.0f);
            if (in->touchDown) {
                s_selected = isDark ? 1 : 0;
                applyThemeWithWipe(isDark);
            }
            return;
        }

        /* swatches cakeOS (centros 40+i*44, 102). */
        int accentTotal = themeAccentCount() + 1;
        for (int i = 0; i < accentTotal; i++) {
            float scx = 40.0f + i * 44.0f, scy = 102.0f;
            if (in->touchPY >= scy - 18.0f && in->touchPY < scy + 18.0f &&
                in->touchPX >= scx - 20.0f && in->touchPX < scx + 20.0f) {
                if (in->touchDown) {
                    s_selected = 2 + i;
                    triggerSwatchBounce(i);
                    if (i < themeAccentCount()) { themeSetAccentIndex(i); saveTheme(); }
                    else {
                        ColorRGBA cur = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
                        enterHexEdit(cur);
                        s_hexOriginX = scx - 16.0f;
                        s_hexOriginY = scy - 16.0f;
                        s_hexEditing = true;
                        s_hexEnterT = 0.0f;
                    }
                }
                return;
            }
        }

        /* card HEX (16,132,168,32) -> abre o editor de cor custom. */
        if (in->touchDown && in->touchPY >= 132.0f && in->touchPY < 164.0f &&
            in->touchPX >= 16.0f && in->touchPX < 184.0f) {
            s_selected = 2 + themeAccentCount();
            ColorRGBA cur = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
            enterHexEdit(cur);
            s_hexOriginX = 16.0f; s_hexOriginY = 132.0f;
            s_hexEditing = true; s_hexEnterT = 0.0f;
            return;
        }

        /* card idioma PT|EN|ES (LANG_SEG_*). */
        if (in->touchDown &&
            in->touchPY >= LANG_SEG_Y && in->touchPY < LANG_SEG_Y + LANG_SEG_H &&
            in->touchPX >= LANG_SEG_X && in->touchPX < LANG_SEG_X + LANG_SEG_W) {
            int seg = (int)((in->touchPX - LANG_SEG_X) / (LANG_SEG_W / (float)LANG_COUNT));
            seg = clampi(seg, 0, LANG_COUNT - 1);
            s_selected = langSlot;
            setLang((Lang)seg);
            return;
        }
    }

    if (in->confirm) {
        if (s_selected == 0) { applyThemeWithWipe(false); }
        else if (s_selected == 1) { applyThemeWithWipe(true); }
        else {
            int idx = s_selected - 2;
            triggerSwatchBounce(idx);
            if (idx >= 0 && idx < themeAccentCount()) { themeSetAccentIndex(idx); saveTheme(); }
            else {
                ColorRGBA cur = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
                enterHexEdit(cur);
                /* origem da animacao = centro do swatch HEX na grade (40+idx*44, 102). */
                s_hexOriginX = (40.0f + idx * 44.0f) - 16.0f;
                s_hexOriginY = 102.0f - 16.0f;
                s_hexEditing = true;
                s_hexEnterT = 0.0f;
            }
        }
    }

    if (s_swatchBounceT < 1.0f) s_swatchBounceT += dt;
}

/* 1.5.0 §TEMA: a troca sol<->lua agora anima NO PROPRIO lugar do icone (lado
 * direito do card de previa), NAO mais como um overlay celeste gigante em cima
 * do mini-card a esquerda (o dono achou feio). Sem aro/aureola, sem fade-out
 * final: os dois sprites fazem crossfade girando de forma continua com um leve
 * mergulho de escala no ponto de troca -- entra girando e assenta. */
static void drawModeIconStyled(IconID ic, float cx, float cy, float size, float ang, float a) {
    if (a <= 0.003f) return;
    /* a lua (moon_fill, contorno escuro) gira TINTADA de prata pra nao ficar
     * preta; o sol gira com a cor propria (amarelo). */
    if (ic == ICON_THEME) {
        ColorRGBA silver = {232, 234, 240, 255}; /* #E8EAF0 */
        iconsDrawRotated(ic, cx, cy, size, ang, silver, a);
    } else {
        iconsDrawFixedRotated(ic, cx, cy, size, ang, a);
    }
}

static void darkmodeDrawModeIcon(float cx, float cy, float size) {
    ThemeWipe* wp = themeWipeGet();
    if (!wp->active) {
        drawModeIconStyled(themeIsDark() ? ICON_THEME : ICON_SUN, cx, cy, size, 0.0f, 1.0f);
        return;
    }
    float t = clampf(wp->t / THEME_WIPE_DUR, 0.0f, 1.0f);
    IconID oldI = wp->wasDark ? ICON_THEME : ICON_SUN;
    IconID newI = wp->wasDark ? ICON_SUN : ICON_THEME;
    float e = easeFunc(t, EASE_IN_OUT_CUBIC);
    float ang = e * (float)M_PI;                    /* 0..180 giro continuo */
    float oldA = 1.0f - clampf(t / 0.5f, 0.0f, 1.0f);
    float newA = clampf((t - 0.5f) / 0.5f, 0.0f, 1.0f);
    float dip = 1.0f - 0.14f * sinf(t * (float)M_PI); /* mergulho de escala no swap */
    drawModeIconStyled(oldI, cx, cy, size * dip, ang, oldA);
    drawModeIconStyled(newI, cx, cy, size * dip, ang - (float)M_PI, newA);
}

/* 1.8.0 CAELESTIA: o cross-fade de accent do preview agora usa o accent ANIMADO
 * GLOBAL (UI_AccentAnim, 300ms EFF_SLOW) -- mesma cor "escorrendo" que o resto
 * da UI, unificado (antes tinha um tween local so pro preview). */

void darkmodeRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    float offsetFg = 0.0f;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_THEME));

    if (s_hexEditing) {
        /* editor de HEX: painel largo cobrindo o header enquanto edita. 1.5.0:
         * SEM UI_Card aqui -- a sombra dura dele (halo retangular, alpha fixo)
         * ficava "mal colocada" e escurecia a beirada no tema claro. Agora e um
         * painel arredondado chapado, sem drop-shadow, cobrindo quase a tela. */
        UI_RoundRect(8 + slideX, 28, 384, 202, RADIUS_CARD, g_theme.surface);
        /* Popup 3.3 (Travel Agency): scrim 0->55% em 220ms easeOutCubic,
         * card cresce 0.6->1.0 + opacidade 0.4->1.0 com easeOutBack(1.5),
         * 360ms. A tela de cima nao tem um "elemento que abriu" equivalente
         * (e um painel fisico separado da tela de baixo, onde o swatch foi
         * tocado) -- aqui o crescimento e ancorado no proprio centro do
         * frame de preview, que e a unica origem que faz sentido aqui. */
        float scaleG, posE, scrimP;
        if (s_hexClosing) {
            float closeT = clampf(s_hexCloseT / 0.24f, 0.0f, 1.0f);
            float closeP = 1.0f - easeFunc(closeT, EASE_IN_CUBIC);
            scaleG = closeP; posE = closeP; scrimP = closeP;
        } else {
            float growT = clampf(s_hexEnterT / 0.36f, 0.0f, 1.0f);
            scaleG = easeOutBackAmp(growT, 1.5f);
            posE = easeFunc(growT, EASE_OUT_CUBIC);
            scrimP = easeFunc(clampf(s_hexEnterT / 0.22f, 0.0f, 1.0f), EASE_OUT_CUBIC);
        }
        u8 fa = (u8)(255 * clampf(lerpf(0.4f, 1.0f, clampf(scaleG, 0.0f, 1.0f)), 0.0f, 1.0f));
        float hexSlide = (1.0f - posE) * 16.0f;

        /* §4: scrim bem mais suave no CLARO -- 55% preto borrava/sujava a tela
         * toda no modo claro; no escuro continua forte pra focar o editor. */
        float scrimMax = themeIsDark() ? 0.55f : 0.10f;
        ColorRGBA scrim = {0, 0, 0, (u8)(255 * scrimP * scrimMax)};
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, scrim);

        ColorRGBA textP = g_theme.textPrimary; textP.a = fa;
        ColorRGBA textH = g_theme.textHint; textH.a = fa;
        ColorRGBA textS = g_theme.textSecondary; textS.a = fa;

        UI_Text(buf, NULL, T(STR_HEX_TITLE), 32, 52 + offsetFg + hexSlide, 0.32f, 0.32f, textP);
        UI_Text(buf, NULL, T(STR_HEX_HINT),
                32, 76 + offsetFg + hexSlide, 0.22f, 0.22f, textH);
        /* 1.6.0: usa a cor EXIBIDA (cross-fade) do picker, igual a tela de baixo. */
        ColorRGBA previewC = {(u8)(s_hexPicker.dispR + 0.5f), (u8)(s_hexPicker.dispG + 0.5f),
                              (u8)(s_hexPicker.dispB + 0.5f), fa};
        UI_TextCenter(buf, NULL, T(STR_PREVIEW), 308, 36 + offsetFg + hexSlide, 0.20f, 0.20f, textH);

        ColorRGBA frameC = themeIsDark() ? (ColorRGBA){10, 12, 18, 255} : (ColorRGBA){225, 228, 238, 255};
        frameC.a = fa;
        float baseFX = 260, baseFY = 50 + offsetFg, baseFW = 96, baseFH = 96;
        float fcx = baseFX + baseFW * 0.5f, fcy = baseFY + baseFH * 0.5f;
        float fw = baseFW * scaleG, fh = baseFH * scaleG;
        /* §4: sem UI_Card aqui (a sombra dura dele -- alpha 102 fixo -- borrava
         * no claro). Moldura simples com borda sutil, sem drop shadow. */
        ColorRGBA frameBorder = {255, 255, 255, themeIsDark() ? 12 : 30};
        UI_RoundFrame(fcx - fw * 0.5f, fcy - fh * 0.5f, fw, fh, 14 * clampf(scaleG, 0.0f, 1.0f), frameC, frameBorder);
        float iw = 80 * scaleG, ih = 80 * scaleG;
        UI_RoundRect(fcx - iw * 0.5f, fcy - ih * 0.5f, iw, ih, 10 * clampf(scaleG, 0.0f, 1.0f), previewC);

        char hexLabel[10];
        snprintf(hexLabel, sizeof(hexLabel), "#%s", s_hexPicker.hex_input);
        UI_TextCenter(buf, NULL, hexLabel, 308, 156 + offsetFg + hexSlide, 0.28f, 0.28f, textS);
        if (fadeA < 0.999f) {
            ColorRGBA veil = g_theme.backgroundTop;
            veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
            UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
        }
        return;
    }

    /* Card de previa (espec v20): x24 y88 w352 h128 r16. */
    float cardX2 = 24.0f + slideX;
    if (UI_AssetsReady()) UI_NineCard(cardX2, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);
    else UI_Card(cardX2, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);
    UI_Text(buf, NULL, T(STR_PREVIEW), 44.0f + slideX, 106.0f, 0.24f, 0.24f, g_theme.textHint);

    ColorRGBA accentC = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
    /* 1.6.0: a cor EXIBIDA no preview morfa suave (cross-fade ~220ms) ao trocar
     * de accent, em vez de saltar. So o desenho -- o valor real ja mudou. */
    ColorRGBA dAcc = UI_AccentAnim();

    /* mini-card de exemplo (44,136,150,76) r12: swatch accent r11 + nome + #hex
     * + pilula accent. 1.5.0: fundo THEME-AWARE (era #1F1E26 fixo -> escuro feio
     * no tema claro). */
    float mx = 44.0f + slideX, my = 136.0f;
    ColorRGBA innerCard = themeIsDark() ? (ColorRGBA){0x1F, 0x1E, 0x26, 255}
                                        : (ColorRGBA){0xEC, 0xEC, 0xF0, 255};
    if (UI_AssetsReady()) UI_NineCard(mx, my, 150.0f, 76.0f, 12.0f, innerCard);
    else UI_RoundRect(mx, my, 150.0f, 76.0f, 12.0f, innerCard);
    UI_RoundRect(72.0f - 11.0f + slideX, 164.0f - 11.0f, 22.0f, 22.0f, 11.0f, dAcc);
    const char* accentLabel = themeAccentIsCustom() ? T(STR_CUSTOM) : themeAccentName(themeGetAccentIndex());
    UI_Text(buf, NULL, accentLabel, 92.0f + slideX, 150.0f, 0.27f, 0.27f, g_theme.textPrimary);
    char hexStr[10];
    snprintf(hexStr, sizeof(hexStr), "#%02X%02X%02X", accentC.r, accentC.g, accentC.b);
    UI_Text(buf, NULL, hexStr, 92.0f + slideX, 170.0f, 0.24f, 0.24f, g_theme.textSecondary);
    if (UI_AssetsReady()) UI_NinePill(60.0f + slideX, 196.0f, 80.0f, 14.0f, dAcc);
    else UI_RoundRect(60.0f + slideX, 196.0f, 80.0f, 14.0f, 7.0f, dAcc);

    /* 1.5.0: icone de modo (sol/lua) SEM aro/aureola (o dono odiou o anel ambar).
     * A troca sol<->lua anima no proprio icone (fade+rotacao), sem overlay. */
    darkmodeDrawModeIcon(300.0f + slideX, 158.0f, 40.0f);
    UI_TextCenter(buf, NULL, themeIsDark() ? T(STR_DARK) : T(STR_LIGHT),
                  300.0f + slideX, 192.0f, 0.32f, 0.32f, g_theme.textSecondary);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }
}

void darkmodeRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    float offset = (1.0f - transVal) * 30.0f;
    (void)scaleM;
    UI_BottomBackground();

    if (s_hexEditing) {
        /* Popup 3.3: nasce na posicao real do swatch HEX tocado (s_hexOriginX/Y,
         * gravado no toque/confirm que abriu) e desliza/cresce ate o lugar
         * definitivo do teclado, com scrim e fade -- o teclado em si nao tem
         * parametro de escala (colorPickerRender so recebe x,y), entao a
         * "escala 0.6->1.0" e entregue via opacidade (veu) + a posicao real
         * faz o trabalho de "emergir do ponto tocado". */
        float scaleG, posE, scrimP;
        if (s_hexClosing) {
            float closeT = clampf(s_hexCloseT / 0.24f, 0.0f, 1.0f);
            float closeP = 1.0f - easeFunc(closeT, EASE_IN_CUBIC);
            scaleG = closeP; posE = closeP; scrimP = closeP;
        } else {
            float growT = clampf(s_hexEnterT / 0.36f, 0.0f, 1.0f);
            scaleG = easeOutBackAmp(growT, 1.5f);
            posE = easeFunc(growT, EASE_OUT_CUBIC);
            scrimP = easeFunc(clampf(s_hexEnterT / 0.22f, 0.0f, 1.0f), EASE_OUT_CUBIC);
        }
        float popupFade = clampf(lerpf(0.4f, 1.0f, clampf(scaleG, 0.0f, 1.0f)), 0.0f, 1.0f);

        /* §4: scrim mais suave no claro (igual a tela de cima). 1.5.0: cobre a
         * tela TODA (era ate H-26, deixava uma faixa clara embaixo "sem tampar"). */
        float scrimMax = themeIsDark() ? 0.55f : 0.10f;
        ColorRGBA scrim = {0, 0, 0, (u8)(255 * scrimP * scrimMax)};
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, scrim);

        /* Touch Bar HSV (3.4): selecao grossa, desliza/cresce junto com o
         * resto do popup a partir da mesma origem. */
        float hueX = lerpf(s_hexOriginX, 10.0f, posE);
        float hueY = lerpf(s_hexOriginY, 8.0f, posE) + offset;
        if (s_hueFocused) {
            /* Anel solido desenhado ANTES da barra (que cobre o miolo) --
             * mesma tecnica anti-blob ja usada no anel de selecao do Inicio:
             * UI_RoundFrame com fill transparente nao "fura" nada, so deixa
             * o retangulo opaco inteiro visivel. */
            UI_RoundRect(hueX - 3, hueY - 3, 306, 34, 17, g_theme.accent);
        }
        UI_HueSpectrumBar(hueX, hueY, 300.0f, 28.0f, s_hueT, s_hueDragging);

        float ky = lerpf(s_hexOriginY, 56.0f, posE) + offset;
        colorPickerRender(buf, &s_hexPicker, ky);
        UI_HelpBar(buf, T(STR_HELP_THEME1_L), T(STR_HELP_THEME1_R));

        if (popupFade < 0.999f) {
            ColorRGBA veil = g_theme.background;
            veil.a = (u8)(255 * (1.0f - popupFade));
            UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
        }
        if (fadeA < 0.999f) {
            ColorRGBA veil2 = g_theme.background;
            veil2.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
            UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil2);
        }
        popupRender(buf, &s_popup);
        return;
    }

    /* === espec v20: controles planos (segmented + swatches em anel + cards) === */
    int accentTotal = themeAccentCount() + 1; /* 5 presets + HEX/custom */

    /* 1) segmented Claro/Escuro (16,18,288,38) -- foco integrado no controle. */
    const char* modeLabels[] = { T(STR_LIGHT), T(STR_DARK) };
    UI_TouchBarSegmented(buf, 16.0f + slideX, 18.0f + offset, 288.0f, 38.0f,
                         modeLabels, 2, themeIsDark() ? 1 : 0, &s_segTween, s_selected < 2);

    /* 2) caption + 6 swatches cakeOS (centros y102, x=40..260 passo 44). */
    UI_Text(buf, NULL, T(STR_ACCENT), 24.0f + slideX, 70.0f + offset, 0.24f, 0.24f, g_theme.textHint);
    int activeIdx = themeAccentIsCustom() ? themeAccentCount() : themeGetAccentIndex();

    for (int i = 0; i < accentTotal; i++) {
        float scx = 40.0f + i * 44.0f + slideX, scy = 102.0f + offset;
        ColorRGBA ac = (i < themeAccentCount()) ? themeAccentColor(i) : themeGetCustomAccent();
        /* bounce 3.4 ao selecionar. */
        float bounce = 1.0f;
        if (i == s_swatchBounceIdx && s_swatchBounceT < 0.3f) {
            float bt = clampf(s_swatchBounceT / 0.3f, 0.0f, 1.0f);
            bounce = (bt < 0.5f) ? lerpf(1.0f, 1.15f, easeFunc(bt * 2.0f, EASE_OUT_CUBIC))
                                 : lerpf(1.15f, 1.0f, easeFunc((bt - 0.5f) * 2.0f, EASE_OUT_BACK));
        }
        /* 1.5.0: swatch no estilo GLASS cakeOS/reva -- borda GROSSA da cor +
         * miolo translucido furado (mesma linguagem das bolinhas do app), em
         * vez do disco chapado + aro fino de antes. */
        float r = 14.0f * bounce;
        float bw = fmaxf(3.0f, r * 0.32f);
        ColorRGBA border = ac; border.a = 255;
        UI_RoundRect(scx - r, scy - r, r * 2.0f, r * 2.0f, r, border);
        float hr = r - bw;
        if (hr > 0.5f) {
            UI_RoundRect(scx - hr, scy - hr, hr * 2.0f, hr * 2.0f, hr, g_theme.background);
            ColorRGBA fill = ac; fill.a = 70;
            UI_RoundRect(scx - hr, scy - hr, hr * 2.0f, hr * 2.0f, hr, fill);
        }
        if (i == themeAccentCount()) {
            /* 6o = HEX/custom: rotulo "HEX" legivel sobre a cor. */
            UI_RoundRect(scx - 12.0f, scy - 6.5f, 24.0f, 13.0f, 6.5f, (ColorRGBA){0, 0, 0, 165});
            UI_TextCenter(buf, NULL, "HEX", scx, scy - 5.0f, 0.16f, 0.16f, (ColorRGBA){255, 255, 255, 255});
        }
        /* 1.6.1: foco do swatch = anel accent NITIDO por FORA do circulo (nao
         * cobre a cor, ao contrario do chip preenchido). */
        if (s_selected - 2 == i) {
            ColorRGBA fa = UI_AccentAnim(); fa.a = 255;
            float rr = r + 5.0f;
            if (UI_AssetsReady()) UI_Ring(scx - rr, scy - rr, rr * 2.0f, rr * 2.0f, rr, fa);
            else UI_RoundFrame(scx - rr, scy - rr, rr * 2.0f, rr * 2.0f, rr, (ColorRGBA){0, 0, 0, 0}, fa);
        }
    }

    /* anel BRANCO nitido no swatch ativo, deslizando (END4) de um pro outro. */
    float ringTargetX = 40.0f + activeIdx * 44.0f + slideX;
    if (s_ringTween.duration <= 0.0001f) {
        tweenStart(&s_ringTween, ringTargetX, ringTargetX, 0.001f, EASE_LINEAR);
        tweenUpdate(&s_ringTween, 1.0f);
    } else if (fabsf(s_ringTween.to - ringTargetX) > 0.5f) {
        tweenStart(&s_ringTween, tweenValue(&s_ringTween), ringTargetX, DUR_SPATIAL_FAST, EASE_EXPR_FAST);
    }
    tweenUpdate(&s_ringTween, uiFrameDt());
    UI_RingCircle(tweenValue(&s_ringTween), 102.0f + offset, 38.0f, (ColorRGBA){255, 255, 255, 255});
    pressFxDraw(&s_swatchPress); /* 1.9.0 FIX5: flash de toque no swatch */

    /* 3) card HEX (16,132,168,32) + card idioma (196,132,108,32). */
    ColorRGBA curC = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
    if (UI_AssetsReady()) UI_NineCard(16.0f + slideX, 132.0f + offset, 168.0f, 32.0f, 10.0f, g_theme.surface);
    else UI_RoundRect(16.0f + slideX, 132.0f + offset, 168.0f, 32.0f, 10.0f, g_theme.surface);
    /* 1.5.0: o card HEX compartilha o slot de selecao com o 6o swatch (HEX). O
     * foco ja e desenhado NO SWATCH (loop acima) -- desenhar AQUI tambem fazia
     * UI_FocusRing ser chamado 2x no mesmo frame com alvos diferentes, e como o
     * anel tem estado global unico ele brigava consigo mesmo (o "flicker" que o
     * dono viu). Um indicador so, no swatch. */
    char hexStr[16];
    snprintf(hexStr, sizeof(hexStr), "HEX %02X%02X%02X", curC.r, curC.g, curC.b);
    UI_Text(buf, NULL, hexStr, 28.0f + slideX, 140.0f + offset, 0.27f, 0.27f, g_theme.textPrimary);

    const char* langLabels[LANG_COUNT] = { "PT", "EN", "ES" };
    UI_TouchBarSegmented(buf, LANG_SEG_X + slideX, LANG_SEG_Y + offset, LANG_SEG_W, LANG_SEG_H,
                         langLabels, LANG_COUNT, (int)langGet(), &s_langSegTween,
                         s_selected == 2 + accentTotal);

    UI_HelpBar(buf, T(STR_HELP_THEME2_L), T(STR_HELP_THEME2_R));

    /* secao 6.2: sol/lua SO na tela de cima. Aqui (tela de baixo) nao
     * desenhamos mais o icone -- o wipe radial de ambiente continua (via
     * UI_BottomBackground), so o astro girando fica top-only. */

    popupRender(buf, &s_popup);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
