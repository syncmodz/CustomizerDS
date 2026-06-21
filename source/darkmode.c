#include "darkmode.h"
#include "common.h"
#include "theme.h"
#include "config.h"
#include "ui.h"
#include "anim.h"
#include "fonts.h"
#include "color_picker.h"
#include <math.h>

static int s_selected = 0;
static float s_segSlideT = 1.0f;
static bool s_hexEditing = false;
static ColorPicker s_hexPicker;
static PopupModal s_popup;
static float s_popupHoldT = 0.0f;

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

void darkmodeInit(void) {
    s_selected = 0;
    s_segSlideT = 1.0f;
    s_hexEditing = false;
    s_popup.active = false;
    s_popupHoldT = 0.0f;
}

/* Popup estilo "Travel Agency" (card escala com easeOutBack + fundo escurece) ja
 * existia em ui.c sem nenhum uso real -- aqui ele confirma visualmente que a cor
 * customizada foi aplicada, em vez de so sair da tela de hex em silencio. */
static void updatePopup(void) {
    popupUpdate(&s_popup);
    if (s_popup.active && s_popup.animT >= 0.35f) {
        s_popupHoldT += uiFrameDt();
        if (s_popupHoldT > 0.9f) { popupHide(&s_popup); s_popupHoldT = 0.0f; }
    }
}

void darkmodeUpdate(const AppInput* in, float dt, int* currentScreen) {
    updatePopup();

    if (s_hexEditing) {
        colorPickerInput(&s_hexPicker, in, 56.0f);
        if (in->confirm) {
            Color_RGB rgb = hexToRGB(s_hexPicker.hex_input);
            themeSetCustomAccent((ColorRGBA){rgb.r, rgb.g, rgb.b, 255});
            saveTheme();
            s_hexEditing = false;
            popupShow(&s_popup, "Cor customizada aplicada", "OK", NULL);
            s_popupHoldT = 0.0f;
        }
        if (in->back) {
            s_hexEditing = false;
        }
        return;
    }

    int total = 2 + themeAccentCount() + 1;

    if (in->back) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (in->downNav) s_selected = (s_selected + 1) % total;
    if (in->up) s_selected = (s_selected - 1 + total) % total;

    if (in->left && s_selected < 2) {
        s_selected = s_selected == 0 ? 1 : 0;
        themeSetDark(!themeIsDark());
        s_segSlideT = 0.0f;
        saveTheme();
    }
    if (in->right && s_selected < 2) {
        s_selected = s_selected == 0 ? 1 : 0;
        themeSetDark(!themeIsDark());
        s_segSlideT = 0.0f;
        saveTheme();
    }

    if (in->left && s_selected >= 2) {
        int accentTotal = themeAccentCount() + 1;
        int idx = (s_selected - 2 - 1 + accentTotal) % accentTotal;
        s_selected = 2 + idx;
        if (idx < themeAccentCount()) { themeSetAccentIndex(idx); saveTheme(); }
    }
    if (in->right && s_selected >= 2) {
        int accentTotal = themeAccentCount() + 1;
        int idx = (s_selected - 2 + 1) % accentTotal;
        s_selected = 2 + idx;
        if (idx < themeAccentCount()) { themeSetAccentIndex(idx); saveTheme(); }
    }

    if (in->touchDown || in->touchHeld) {
        float segX = 60.0f, segY = 8.0f, segW = 100.0f, segH = 32.0f; /* deve bater com segH=32 do darkmodeRenderBottom */
        if (in->touchPY >= segY && in->touchPY < segY + segH &&
            in->touchPX >= segX && in->touchPX < segX + segW * 2) {
            bool isDark = (in->touchPX >= segX + segW);
            if (in->touchDown) {
                s_selected = isDark ? 1 : 0;
                s_segSlideT = 0.0f;
                themeSetDark(isDark);
                saveTheme();
            }
            return;
        }

        float swSize = 32.0f, gap = 10.0f, startX = 18.0f, sy = 58.0f;
        int accentTotal = themeAccentCount() + 1;
        for (int i = 0; i < accentTotal; i++) {
            float sx = startX + i * (swSize + gap);
            if (in->touchPY >= sy && in->touchPY < sy + swSize &&
                in->touchPX >= sx && in->touchPX < sx + swSize) {
                if (in->touchDown) {
                    s_selected = 2 + i;
                    if (i < themeAccentCount()) { themeSetAccentIndex(i); saveTheme(); }
                    else {
                        ColorRGBA cur = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
                        colorPickerInit(&s_hexPicker);
                        s_hexPicker.preview = (Color_RGB){cur.r, cur.g, cur.b};
                        snprintf(s_hexPicker.hex_input, sizeof(s_hexPicker.hex_input),
                                 "%02X%02X%02X", cur.r, cur.g, cur.b);
                        s_hexEditing = true;
                    }
                }
                return;
            }
        }
    }

    if (in->confirm) {
        if (s_selected == 0) { themeSetDark(false); s_segSlideT = 0.0f; saveTheme(); }
        else if (s_selected == 1) { themeSetDark(true); s_segSlideT = 0.0f; saveTheme(); }
        else {
            int idx = s_selected - 2;
            if (idx >= 0 && idx < themeAccentCount()) { themeSetAccentIndex(idx); saveTheme(); }
            else {
                ColorRGBA cur = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
                colorPickerInit(&s_hexPicker);
                s_hexPicker.preview = (Color_RGB){cur.r, cur.g, cur.b};
                snprintf(s_hexPicker.hex_input, sizeof(s_hexPicker.hex_input),
                         "%02X%02X%02X", cur.r, cur.g, cur.b);
                s_hexEditing = true;
            }
        }
    }

    s_segSlideT = fminf(s_segSlideT + dt, 1.0f);
}

void darkmodeRenderTop(C2D_TextBuf buf, float transVal) {
    /* Parallax 2 camadas: card anda mais, conteudo de dentro se acomoda primeiro. */
    float offsetRaw = (1.0f - transVal);
    float offset = offsetRaw * 40.0f;
    float offsetFg = offsetRaw * 18.0f;
    UI_TopBackground();
    UI_TopMenuBar("Tema", buf);

    UI_Card(16, 30 + offset, 368, 196, 16, g_theme.surface);

    if (s_hexEditing) {
        UI_Text(buf, NULL, "Cor customizada (hex)", 32, 52 + offsetFg, 0.32f, 0.32f, g_theme.textPrimary);
        UI_Text(buf, NULL, "esq/dir: valor, cima/baixo: posicao",
                32, 76 + offsetFg, 0.22f, 0.22f, g_theme.textHint);
        Color_RGB rgb = s_hexPicker.preview;
        ColorRGBA previewC = {rgb.r, rgb.g, rgb.b, 255};
        UI_TextCenter(buf, NULL, "Preview", 308, 36 + offsetFg, 0.20f, 0.20f, g_theme.textHint);
        UI_Card(260, 50 + offsetFg, 96, 96, 14,
                themeIsDark() ? (ColorRGBA){10, 12, 18, 255} : (ColorRGBA){225, 228, 238, 255});
        UI_RoundRect(268, 58 + offsetFg, 80, 80, 10, previewC);
        char hexLabel[10];
        snprintf(hexLabel, sizeof(hexLabel), "#%s", s_hexPicker.hex_input);
        UI_TextCenter(buf, NULL, hexLabel, 308, 156 + offsetFg, 0.28f, 0.28f, g_theme.textSecondary);
        return;
    }

    UI_Text(buf, NULL, "Aparencia", 32, 52 + offsetFg, 0.34f, 0.34f, g_theme.textPrimary);

    float px = 36.0f, py = 78.0f + offsetFg;
    ColorRGBA cardBg = themeIsDark() ? (ColorRGBA){20, 24, 34, 255} : (ColorRGBA){235, 238, 246, 255};
    UI_RoundFrame(px, py, 150, 72, 12, cardBg, (ColorRGBA){255, 255, 255, 12});
    UI_RoundRect(px + 10, py + 8, 130, 5, 2.5f, g_theme.accent);
    ColorRGBA mockLine = themeIsDark() ? (ColorRGBA){60, 65, 85, 200} : (ColorRGBA){190, 192, 205, 200};
    for (int i = 0; i < 3; i++)
        UI_RoundRect(px + 14, py + 22 + i * 8, 70 + i * 20, 3, 1.5f, mockLine);
    UI_RoundRect(px + 14, py + 50, 60, 12, 6, g_theme.accent);
    UI_TextCenter(buf, NULL, themeIsDark() ? "Escuro" : "Claro", px + 75, py + 60, 0.24f, 0.24f, g_theme.textSecondary);

    ColorRGBA accentC = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
    UI_RoundRect(px + 180, py, 24, 24, 12, accentC);
    const char* accentLabel = themeAccentIsCustom() ? "Custom" : themeAccentName(themeGetAccentIndex());
    UI_Text(buf, NULL, accentLabel, px + 212, py + 2, 0.28f, 0.28f, g_theme.accent);

    UI_RoundRect(px + 180, py + 36, 120, 8, 4, cardBg);
    UI_RoundRect(px + 180, py + 48, 100, 8, 4, cardBg);
    UI_RoundRect(px + 180, py + 60, 80, 8, 4, cardBg);
}

void darkmodeRenderBottom(C2D_TextBuf buf, float transVal) {
    float offset = (1.0f - transVal) * 30.0f;
    UI_BottomBackground();

    if (s_hexEditing) {
        colorPickerRender(buf, &s_hexPicker, 14.0f, 56.0f + offset);
        UI_HelpBar(buf, "esq/dir valor  cima/baixo posicao", "A aplicar  B voltar");
        return;
    }

    float et = UI_EnterProgress();

    const char* modeLabels[] = { "Claro", "Escuro" };
    float segX = 60.0f, segY = 8.0f + offset, segW = 200.0f, segH = 32.0f;
    ColorRGBA segBase = themeIsDark()
        ? (ColorRGBA){44, 46, 54, 240}
        : (ColorRGBA){230, 232, 240, 240};
    ColorRGBA segBorder = {0, 0, 0, themeIsDark() ? 15 : 10};
    float rr = segH * 0.5f;
    UI_Shadow(segX, segY, segW, segH, 15, 1.0f);
    UI_RoundRect(segX, segY, segW, segH, rr, segBase);
    UI_RoundFrame(segX, segY, segW, segH, rr, segBase, segBorder);
    float segItemW = segW / 2;
    float targetX = segX + (themeIsDark() ? 1 : 0) * segItemW;
    static float s_morphX = -9999.0f;
    if (s_morphX < -9000) s_morphX = targetX;
    if (s_segSlideT < 1.0f) {
        s_morphX = approach(s_morphX, targetX, 240.0f * uiFrameDt());
    } else {
        s_morphX = targetX;
    }
    ColorRGBA selBg = themeIsDark()
        ? (ColorRGBA){99, 102, 116, 220}
        : (ColorRGBA){52, 199, 89, 255};
    UI_RoundRect(s_morphX + 2, segY + 2, segItemW - 4, segH - 4, rr - 2, selBg);
    for (int i = 0; i < 2; i++) {
        float cx = segX + segItemW * i + segItemW * 0.5f;
        ColorRGBA tc = (themeIsDark() ? (i == 1) : (i == 0))
            ? g_theme.textPrimary : g_theme.textSecondary;
        UI_TextCenter(buf, NULL, modeLabels[i], cx, segY + (segH - 14) * 0.5f, 0.26f, 0.26f, tc);
    }

    float swSize = 32.0f, gap = 10.0f, startX = 18.0f, sy = 58.0f + offset;
    int accentTotal = themeAccentCount() + 1;
    for (int i = 0; i < accentTotal; i++) {
        float sx = startX + i * (swSize + gap);
        float appearT = clampf((et * 2.5f - i * 0.08f), 0.0f, 1.0f);
        float sa = easeOutCubic(appearT);
        ColorRGBA ac;
        bool active;
        if (i < themeAccentCount()) {
            active = !themeAccentIsCustom() && (themeGetAccentIndex() == i);
            ac = themeAccentColor(i);
        } else {
            active = themeAccentIsCustom();
            ac = themeGetCustomAccent();
        }

        if (active) {
            ColorRGBA ring = g_theme.accent;
            ring.a = 120;
            float ringR = swSize / 2 + 4;
            UI_RoundRect(sx - 4 + (1.0f - sa) * 6, sy - 4 + (1.0f - sa) * 6,
                         swSize + 8, swSize + 8, ringR, ring);
        }
        float r2 = swSize / 2;
        UI_RoundRect(sx + (1.0f - sa) * 6, sy + (1.0f - sa) * 6,
                     swSize, swSize, r2, ac);

        if (i == themeAccentCount()) {
            ColorRGBA hexCol = themeContrastText(ac);
            UI_TextCenter(buf, NULL, "HEX", sx + swSize / 2, sy + 9, 0.20f, 0.20f, hexCol);
        }
    }

    bool customActive = themeAccentIsCustom();
    float labelX = customActive
        ? (startX + themeAccentCount() * (swSize + gap) + swSize * 0.5f)
        : (startX + themeGetAccentIndex() * (swSize + gap) + swSize * 0.5f);
    const char* accentLabel = customActive ? "Custom (hex)" : themeAccentName(themeGetAccentIndex());
    UI_TextCenter(buf, NULL, accentLabel, labelX, 100 + offset, 0.22f, 0.22f, g_theme.accent);

    ColorRGBA curC = customActive ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
    char rgbStr[24];
    snprintf(rgbStr, sizeof(rgbStr), "R%d G%d B%d", curC.r, curC.g, curC.b);
    UI_Text(buf, NULL, rgbStr, 18, 118 + offset, 0.24f, 0.24f, g_theme.textHint);

    UI_HelpBar(buf, "<- -> alterna tema/escolhe accent", "A confirma  START sair");

    popupRender(buf, &s_popup);
}
