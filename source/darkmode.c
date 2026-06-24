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
#define LANG_SEG_X 85.0f
#define LANG_SEG_Y 150.0f
#define LANG_SEG_W 150.0f
#define LANG_SEG_H 28.0f
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

static void triggerSwatchBounce(int idx) {
    s_swatchBounceIdx = idx;
    s_swatchBounceT = 0.0f;
}

/* Centraliza a fileira de swatches de accent na tela de baixo (320px) --
 * antes comecava num x fixo (18) que so por coincidencia "colava" no canto
 * direito do segmented Claro/Escuro acima (60..260), deixando a fileira
 * visivelmente puxada pra esquerda (margem 18 vs 60 do lado direito) --
 * spec v6 secao 3: "conteudo das telas de baixo centralizado". */
static float accentRowStartX(int accentTotal) {
    float swSize = 32.0f, gap = 10.0f;
    float totalW = (float)accentTotal * swSize + (float)(accentTotal - 1) * gap;
    return (SCREEN_BOT_WIDTH - totalW) * 0.5f;
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
    themeWipeTick(dt);

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

    if (in->downNav) s_selected = (s_selected + 1) % total;
    if (in->up) s_selected = (s_selected - 1 + total) % total;

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
        float segX = 60.0f, segY = 8.0f, segW = 100.0f, segH = 32.0f; /* deve bater com segH=32 do darkmodeRenderBottom */
        if (in->touchPY >= segY && in->touchPY < segY + segH &&
            in->touchPX >= segX && in->touchPX < segX + segW * 2) {
            bool isDark = (in->touchPX >= segX + segW);
            if (in->touchDown) {
                s_selected = isDark ? 1 : 0;
                applyThemeWithWipe(isDark);
            }
            return;
        }

        int accentTotal = themeAccentCount() + 1;
        float swSize = 32.0f, gap = 10.0f, startX = accentRowStartX(accentTotal), sy = 58.0f;
        for (int i = 0; i < accentTotal; i++) {
            float sx = startX + i * (swSize + gap);
            if (in->touchPY >= sy && in->touchPY < sy + swSize &&
                in->touchPX >= sx && in->touchPX < sx + swSize) {
                if (in->touchDown) {
                    s_selected = 2 + i;
                    triggerSwatchBounce(i);
                    if (i < themeAccentCount()) { themeSetAccentIndex(i); saveTheme(); }
                    else {
                        ColorRGBA cur = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
                        enterHexEdit(cur);
                        s_hexOriginX = sx;
                        s_hexOriginY = sy;
                        s_hexEditing = true;
                        s_hexEnterT = 0.0f;
                    }
                }
                return;
            }
        }

        /* §i18n.6: toque nos 3 segmentos PT|EN|ES. */
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
                /* Mesma origem usada pelo toque (3.3): posicao real do swatch
                 * HEX na grade, calculada com as mesmas constantes do render. */
                float swSize = 32.0f, gap = 10.0f, startX = accentRowStartX(themeAccentCount() + 1), sy0 = 58.0f;
                s_hexOriginX = startX + idx * (swSize + gap);
                s_hexOriginY = sy0;
                s_hexEditing = true;
                s_hexEnterT = 0.0f;
            }
        }
    }

    if (s_swatchBounceT < 1.0f) s_swatchBounceT += dt;
}

/* Icone sol/lua girando + trocando de sprite na metade do giro (spec v5 6),
 * desenhado por cima de tudo na origem do wipe capturada em
 * applyThemeWithWipe() -- mesmo helper serve pra tela de cima (origem fixa
 * no MiniWindow) e de baixo (origem no segmento de destino do toggle).
 * Giro+morph calibrados no prototipo web (spec v5 9.4): 360deg easeOutCubic,
 * pop de escala via spring(0.55) -- o "pico de dopamina" Apple-style mora
 * aqui, nao no raio do wipe (que e monotono, sem overshoot). */
/* PROMPT v8 secao 6: SO desenha na tela de cima (o caller de baixo nao chama
 * mais isto) e tem CICLO DE VIDA COM FADE -- mata o "travado"/some-seco. O
 * alpha do overlay celeste sobe 0->1 nos 1os 20% do timer, segura, e cai
 * 1->0 nos ultimos 25% -> entra, brilha e some sozinho, atado ao MESMO timer
 * (wp->t/THEME_WIPE_DUR) do wipe de ambiente. */
static void darkmodeRenderWipeIcon(bool isTopScreen) {
    ThemeWipe* wp = themeWipeGet();
    if (!wp->active) return;
    float t = clampf(wp->t / THEME_WIPE_DUR, 0.0f, 1.0f);

    float alpha;
    if (t < 0.20f) {
        alpha = easeFunc(t / 0.20f, EASE_OUT_CUBIC);                 /* sobe 0->1 */
    } else if (t > 0.75f) {
        alpha = 1.0f - easeFunc((t - 0.75f) / 0.25f, EASE_IN_CUBIC); /* cai 1->0 */
    } else {
        alpha = 1.0f;                                               /* segura */
    }
    if (alpha <= 0.001f) return;

    /* giro+morph por easeInOutCubic (secao 6.3); pop de escala via spring
     * mantido (e o "pico de dopamina" Apple-style). */
    float spinDeg = 360.0f * easeFunc(t, EASE_IN_OUT_CUBIC);
    float pop = easeSpringAmp(t, 0.55f);
    float iconScale = 0.85f + 0.3f * pop;
    bool showNew = t > 0.5f;
    IconID oldIcon = wp->wasDark ? ICON_THEME : ICON_SUN;
    IconID newIcon = wp->wasDark ? ICON_SUN : ICON_THEME;
    IconID ic = showNew ? newIcon : oldIcon;
    float ox = isTopScreen ? wp->originTopX : wp->originBotX;
    float oy = isTopScreen ? wp->originTopY : wp->originBotY;
    float baseSize = isTopScreen ? 40.0f : 24.0f;
    float ang = spinDeg * (M_PI / 180.0f);
    float sz = baseSize * iconScale;
    /* §3a: a lua (moon_fill, contorno escuro) gira TINTADA de prata pra nao
     * ficar preta no escuro; o sol gira com a cor propria (amarelo). */
    if (ic == ICON_THEME) {
        ColorRGBA silver = {232, 234, 240, 255}; /* #E8EAF0 */
        iconsDrawRotated(ic, ox, oy, sz, ang, silver, alpha);
    } else {
        iconsDrawFixedRotated(ic, ox, oy, sz, ang, alpha);
    }
}

void darkmodeRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    /* Parallax 3 camadas (Travel Motion), igual as outras telas. */
    float offsetRaw = (1.0f - transVal);
    float offset = offsetRaw * 40.0f;
    float offsetFg = offsetRaw * 18.0f;
    UI_TopBackground();
    UI_TopMenuBar(T(STR_TAB_THEME), buf);

    /* Blobs decorativos de fundo removidos (mesma causa do bug "bolas
     * sobrepostas" da Inicio): caiam dentro do retangulo do card, que tem
     * alpha 235 (nao 255) e deixava-os espiar por tras. */

    /* Transicao 3.2: card desliza/escala a partir do centro; fade real vem
     * do veu no final (cobre tambem o ramo s_hexEditing antes do return). */
    float cardBaseX = 16, cardBaseY = 30 + offset, cardBaseW = 368, cardBaseH = 196;
    float cardW = cardBaseW * scaleM, cardH = cardBaseH * scaleM;
    float cardX = cardBaseX + slideX + (cardBaseW - cardW) * 0.5f;
    float cardY = cardBaseY + (cardBaseH - cardH) * 0.5f;
    UI_Card(cardX, cardY, cardW, cardH, RADIUS_CARD, g_theme.surface);

    if (s_hexEditing) {
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
        float scrimMax = themeIsDark() ? 0.55f : 0.28f;
        ColorRGBA scrim = {0, 0, 0, (u8)(255 * scrimP * scrimMax)};
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, scrim);

        ColorRGBA textP = g_theme.textPrimary; textP.a = fa;
        ColorRGBA textH = g_theme.textHint; textH.a = fa;
        ColorRGBA textS = g_theme.textSecondary; textS.a = fa;

        UI_Text(buf, NULL, T(STR_HEX_TITLE), 32, 52 + offsetFg + hexSlide, 0.32f, 0.32f, textP);
        UI_Text(buf, NULL, T(STR_HEX_HINT),
                32, 76 + offsetFg + hexSlide, 0.22f, 0.22f, textH);
        Color_RGB rgb = s_hexPicker.preview;
        ColorRGBA previewC = {rgb.r, rgb.g, rgb.b, fa};
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

    UI_Text(buf, NULL, T(STR_APPEARANCE), 32 + slideX, 52 + offsetFg, 0.34f, 0.34f, g_theme.textPrimary);

    /* Esquerda: mini janela reconhecivel (titlebar+semaforos+conteudo na cor
     * real do tema) -- preview honesto, nao um mock de "skeleton loading". */
    float winX = 32.0f + slideX, winY = 84.0f + offsetFg, winW = 160.0f, winH = 116.0f;
    UI_MiniWindow(winX, winY, winW, winH, themeIsDark());
    UI_TextCenter(buf, NULL, themeIsDark() ? T(STR_DARK) : T(STR_LIGHT),
                  winX + winW * 0.5f, winY + winH + 6, 0.24f, 0.24f, g_theme.textSecondary);

    /* Direita: swatch de accent + nome + valor hex, centralizados na altura
     * do card (preenche o espaco que antes ficava vazio). */
    float colX = winX + winW + 24.0f;
    float colCenterY = winY + winH * 0.5f;
    ColorRGBA accentC = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
    UI_Text(buf, NULL, T(STR_ACCENT), colX, winY, 0.22f, 0.22f, g_theme.textHint);
    /* Halo contido: UI_RoundFrame com fill transparente NAO punch um buraco
     * (ja descoberto na barra de espectro) -- aqui o bug era visivel de
     * verdade, pois desenhava um retangulo translucido por CIMA do swatch
     * inteiro. Corrigido com o mesmo padrao de 3 camadas do UI_NavCard:
     * fundo solido do card, tint contido no proprio raio, swatch por cima. */
    UI_RoundRect(colX - 4, colCenterY - 28, 56, 56, 28, g_theme.surface);
    ColorRGBA halo = accentC; halo.a = 40;
    UI_RoundRect(colX - 4, colCenterY - 28, 56, 56, 28, halo);
    UI_RoundRect(colX, colCenterY - 24, 48, 48, 24, accentC);

    const char* accentLabel = themeAccentIsCustom() ? T(STR_CUSTOM) : themeAccentName(themeGetAccentIndex());
    UI_Text(buf, NULL, accentLabel, colX + 60, colCenterY - 18, 0.28f, 0.28f, g_theme.textPrimary);
    char hexStr[10];
    snprintf(hexStr, sizeof(hexStr), "#%02X%02X%02X", accentC.r, accentC.g, accentC.b);
    UI_Text(buf, NULL, hexStr, colX + 60, colCenterY + 4, 0.24f, 0.24f, g_theme.textHint);

    darkmodeRenderWipeIcon(true);

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

        /* §4: scrim mais suave no claro (igual a tela de cima). */
        float scrimMax = themeIsDark() ? 0.55f : 0.28f;
        ColorRGBA scrim = {0, 0, 0, (u8)(255 * scrimP * scrimMax)};
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, scrim);

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

    /* segmentedLozenge compartilhado (v3 3.3) -- mesma logica do LED, cada
     * tela com seu proprio Tween (s_segTween), sem mais codigo bespoke. */
    const char* modeLabels[] = { T(STR_LIGHT), T(STR_DARK) };
    UI_TouchBarSegmented(buf, 60.0f + slideX, 8.0f + offset, 200.0f, 32.0f,
                         modeLabels, 2, themeIsDark() ? 1 : 0, &s_segTween);

    int accentTotal = themeAccentCount() + 1;
    float swSize = 32.0f, gap = 10.0f, startX = accentRowStartX(accentTotal) + slideX, sy = 58.0f + offset;
    int activeIdx = themeAccentIsCustom() ? themeAccentCount() : themeGetAccentIndex();

    /* Anel de selecao desliza (v3 3.3) -- antes ele so "ligava/desligava" no
     * swatch ativo, sem animar a posicao; agora UMA instancia de anel desliza
     * em X de onde estava pro novo ativo, 180ms easeOutCubic (mesma curva da
     * spec v2 pro anel de selecao generico). */
    float ringTargetX = startX + activeIdx * (swSize + gap) + swSize * 0.5f;
    if (s_ringTween.duration <= 0.0001f) {
        tweenStart(&s_ringTween, ringTargetX, ringTargetX, 0.001f, EASE_LINEAR);
        tweenUpdate(&s_ringTween, 1.0f);
    } else if (fabsf(s_ringTween.to - ringTargetX) > 0.5f) {
        tweenStart(&s_ringTween, tweenValue(&s_ringTween), ringTargetX, 0.18f, EASE_OUT_CUBIC);
    }
    tweenUpdate(&s_ringTween, uiFrameDt());
    float ringX = tweenValue(&s_ringTween);
    float ringY = sy + swSize * 0.5f;

    /* Halo pulsante 3.4: opacidade 0.25<->0.4, 1.4s, loop, easeInOut, +
     * escala 1.0->1.03 (circulo perfeito w==h, caminho seguro de UI_RoundRect). */
    float haloPhase = fmodf(uiFrameTime(), 1.4f) / 1.4f;
    float haloTri = (haloPhase < 0.5f) ? (haloPhase * 2.0f) : (2.0f - haloPhase * 2.0f);
    float haloWave = easeFunc(haloTri, EASE_IN_OUT_CUBIC);
    float ringScale = lerpf(1.0f, 1.03f, haloWave);
    ColorRGBA ring = g_theme.accent;
    ring.a = (u8)(lerpf(0.25f, 0.4f, haloWave) * 255.0f);
    float ringSize = (swSize + 8) * ringScale;
    UI_RoundRect(ringX - ringSize * 0.5f, ringY - ringSize * 0.5f, ringSize, ringSize, ringSize * 0.5f, ring);

    for (int i = 0; i < accentTotal; i++) {
        float sx = startX + i * (swSize + gap);
        /* Stagger 3.2 exato: 40ms entre swatches, 260ms cada, easeOutCubic. */
        float sa = UI_StaggerT(i, 0.04f, 0.26f);
        ColorRGBA ac;
        if (i < themeAccentCount()) {
            ac = themeAccentColor(i);
        } else {
            ac = themeGetCustomAccent();
        }

        /* Bounce 3.4: 1.0->1.15->1.0, 300ms, primeira metade easeOutCubic
         * (cresce), segunda metade easeOutBack (acomoda com leve overshoot). */
        float bounceScale = 1.0f;
        if (i == s_swatchBounceIdx && s_swatchBounceT < 0.3f) {
            float bt = clampf(s_swatchBounceT / 0.3f, 0.0f, 1.0f);
            bounceScale = (bt < 0.5f)
                ? lerpf(1.0f, 1.15f, easeFunc(bt * 2.0f, EASE_OUT_CUBIC))
                : lerpf(1.15f, 1.0f, easeFunc((bt - 0.5f) * 2.0f, EASE_OUT_BACK));
        }
        float scx = sx + (1.0f - sa) * 6 + swSize * 0.5f;
        float scy = sy + (1.0f - sa) * 6 + swSize * 0.5f;

        /* §8: bolinha cakeOS via SPRITE (ICON_SWATCH_THIN, aro fino + miolo
         * translucido tintado na cor) em vez de retangulos/sombras empilhados
         * -- borda limpa, sem mancha. Desenhada ~86% do slot (32px) pra ficar
         * MENOR (§7c). O realce da selecionada e o anel accent atras
         * (s_ringTween), ja desenhado antes do loop. */
        float drawSz = swSize * bounceScale * 0.86f;
        iconsDraw(ICON_SWATCH_THIN, scx, scy, drawSz, ac, 1.0f);

        if (i == themeAccentCount()) {
            /* swatch HEX: rotulo "HEX" por cima pra distinguir dos presets.
             * Agora o swatch e um anel de miolo TRANSLUCIDO, entao o texto fica
             * sobre o fundo da tela (nao sobre a cor solida) -- por isso usamos
             * g_theme.textPrimary (alto contraste com o fundo) em vez de
             * themeContrastText(cor), que sumia. */
            UI_TextCenter(buf, NULL, "HEX", scx, scy - 6, 0.21f, 0.21f, g_theme.textPrimary);
        }
    }

    bool customActive = themeAccentIsCustom();
    float labelX = customActive
        ? (startX + themeAccentCount() * (swSize + gap) + swSize * 0.5f)
        : (startX + themeGetAccentIndex() * (swSize + gap) + swSize * 0.5f);
    const char* accentLabel = customActive ? T(STR_CUSTOM) : themeAccentName(themeGetAccentIndex());
    UI_TextCenter(buf, NULL, accentLabel, labelX, 100 + offset, 0.22f, 0.22f, g_theme.accent);

    ColorRGBA curC = customActive ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
    char rgbStr[24];
    snprintf(rgbStr, sizeof(rgbStr), "R%d G%d B%d", curC.r, curC.g, curC.b);
    UI_Text(buf, NULL, rgbStr, startX, 118 + offset, 0.24f, 0.24f, g_theme.textHint);

    /* §i18n.6: seletor de idioma PT|EN|ES (nomes proprios, nao traduzem). */
    const char* langLabels[LANG_COUNT] = { "PT", "EN", "ES" };
    UI_Text(buf, NULL, T(STR_LANGUAGE), LANG_SEG_X + slideX, LANG_SEG_Y - 16 + offset,
            0.22f, 0.22f, g_theme.textHint);
    UI_TouchBarSegmented(buf, LANG_SEG_X + slideX, LANG_SEG_Y + offset, LANG_SEG_W, LANG_SEG_H,
                         langLabels, LANG_COUNT, (int)langGet(), &s_langSegTween);

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
