#include "homeui.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "lang.h"
#include "color_picker.h"
#include "homeui_color.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* 2.0.0: aba UI da Home = customizador de COR real. Pick uma cor -> hue-shift
 * das tabelas de cor do Home Menu -> LayeredFS. SD-only, sem brick. */

#define HOMEUI_PICKER_Y  92.0f

static ColorPicker s_picker;
static PopupModal s_popup;
static int s_pendingAction = 0;   /* 1=aplicar, 2=remover */
static float s_toastT = 0.0f;
static char s_toast[96] = "";

static void showToast(const char* m) { snprintf(s_toast, sizeof(s_toast), "%s", m); s_toastT = 3.0f; }

void homeuiInit(void) {
    colorPickerInit(&s_picker);
    s_popup.active = false; s_pendingAction = 0; s_toastT = 0.0f;
}

void homeuiUpdate(const AppInput* in, float dt, int* currentScreen) {
    popupUpdate(&s_popup);
    if (s_toastT > 0.0f) s_toastT -= dt;

    if (popupActive(&s_popup)) {
        if (popupConfirmInput(&s_popup, in) && s_popup.result == 1) {
            if (s_pendingAction == 1) {
                Color_RGB c = s_picker.preview;
                showToast(homeuiColorApply(c.r, c.g, c.b) ? T(STR_HOMEUI_DONE) : T(STR_HOMEUI_FAIL));
            } else if (s_pendingAction == 2) {
                homeuiColorRemove();
                showToast(T(STR_HOMEUI_REMOVED));
            }
            s_pendingAction = 0;
        }
        return;
    }

    if (in->back) { *currentScreen = SCREEN_MAIN_MENU; return; }

    colorPickerInput(&s_picker, in, HOMEUI_PICKER_Y);

    if (in->confirm && homeuiColorAvailable()) {
        s_pendingAction = 1;
        popupShowConfirm(&s_popup, T(STR_HOMEUI_CONFIRM));
    }
    if (in->down & KEY_X) {
        s_pendingAction = 2;
        popupShowConfirm(&s_popup, T(STR_HOMEUI_REMOVE));
    }
}

void homeuiRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_HOMEUI));

    float cardX = 24.0f + slideX;
    UI_Elevation(cardX, 88.0f, 352.0f, 128.0f, 16.0f, 2, 1.0f);
    if (UI_AssetsReady()) UI_NineCard(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);
    else UI_Card(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);

    if (!homeuiColorAvailable()) {
        UI_TextCenterFit(buf, NULL, T(STR_HOMEUI_EMPTY), 200.0f + slideX, 148.0f, 0.26f, 0.20f, 320.0f, g_theme.warning);
    } else {
        /* swatch grande da cor escolhida + mock simples da Home recolorida. */
        Color_RGB c = s_picker.preview;
        ColorRGBA col = { c.r, c.g, c.b, 255 };
        /* mock: barra HUD em cima + "grade" de apps tingida */
        float mx = cardX + 24.0f, my = 104.0f, mw = 180.0f, mh = 96.0f;
        ColorRGBA dark = { (u8)(c.r*0.25f), (u8)(c.g*0.25f), (u8)(c.b*0.25f), 255 };
        UI_RoundRect(mx, my, mw, mh, 10.0f, dark);
        ColorRGBA bar = { (u8)(c.r*0.6f), (u8)(c.g*0.6f), (u8)(c.b*0.6f), 255 };
        UI_RoundRect(mx + 6.0f, my + 6.0f, mw - 12.0f, 16.0f, 5.0f, bar);   /* HUD */
        for (int i = 0; i < 6; i++) {                                       /* apps */
            float ax = mx + 12.0f + (i % 3) * 56.0f, ay = my + 32.0f + (i / 3) * 30.0f;
            UI_RoundRect(ax, ay, 44.0f, 22.0f, 6.0f, col);
        }
        /* texto a direita */
        float rcx = cardX + 24.0f + 180.0f + 8.0f + (352.0f - 24.0f - 180.0f - 8.0f - 24.0f) * 0.5f;
        UI_TextCenterFit(buf, NULL, T(STR_ITEM_HOMEUI_SUB), rcx, 118.0f, 0.24f, 0.18f, 120.0f, g_theme.textSecondary);
        char hex[16]; snprintf(hex, sizeof(hex), "#%02X%02X%02X", c.r, c.g, c.b);
        UI_TextCenter(buf, NULL, hex, rcx, 150.0f, 0.30f, 0.30f, col);
        UI_TextCenterFit(buf, NULL, T(STR_HOMEUI_REBOOT), rcx, 182.0f, 0.20f, 0.16f, 120.0f, g_theme.textHint);
    }

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }
}

void homeuiRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)slideX; (void)scaleM;
    UI_BottomBackground();

    UI_TextCenter(buf, NULL, T(STR_HEX_TITLE), SCREEN_BOT_WIDTH * 0.5f, 62.0f, 0.26f, 0.26f, g_theme.textSecondary);
    colorPickerRender(buf, &s_picker, HOMEUI_PICKER_Y);

    if (s_toastT > 0.0f) {
        float a = clampf(s_toastT / 0.4f, 0.0f, 1.0f);
        ColorRGBA bg = g_theme.accent; bg.a = (u8)(230 * a);
        float tw = UI_TextWidth(buf, NULL, s_toast, 0.24f) + 28.0f;
        float tx = (SCREEN_BOT_WIDTH - tw) * 0.5f;
        if (UI_AssetsReady()) UI_NinePill(tx, 168.0f, tw, 24.0f, bg);
        else UI_RoundRect(tx, 168.0f, tw, 24.0f, 12.0f, bg);
        ColorRGBA tc = themeContrastText(g_theme.accent); tc.a = (u8)(255 * a);
        UI_TextCenter(buf, NULL, s_toast, SCREEN_BOT_WIDTH * 0.5f, 173.0f, 0.24f, 0.24f, tc);
    }

    UI_HelpBar(buf, T(STR_HOMEUI_HELP_L), T(STR_SAIR));
    popupRender(buf, &s_popup);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
