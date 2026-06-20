#include "menu.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "fonts.h"
#include "anim.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    const char* title;
    const char* prefix;
    const char* value;
    ScreenType target;
} MenuItem;

static const MenuItem ITEMS[] = {
    { "Fontes",      "Aa", "trocar fonte",  SCREEN_FONTS },
    { "Tema do app", "\xe2\x97\x89", "modo e cor", SCREEN_DARKMODE },
    { "LED RGB",     "\xe2\x97\x8b", "cor e luz",  SCREEN_LED },
};

static int s_selected = 0;
static float s_slideT = 1.0f;

static int itemCount(void) {
    return (int)(sizeof(ITEMS) / sizeof(ITEMS[0]));
}

void menuInit(void) {
    s_selected = 0;
}

void menuUpdate(u32 kDown, touchPosition* touch, bool touchDown, int* currentScreen) {
    if (kDown & KEY_DOWN) { s_selected = (s_selected + 1) % itemCount(); s_slideT = 0.0f; }
    if (kDown & KEY_UP)   { s_selected = (s_selected - 1 + itemCount()) % itemCount(); s_slideT = 0.0f; }

    if (touchDown) {
        for (int i = 0; i < itemCount(); i++) {
            float by = 42.0f + i * 48.0f;
            if (touchIn(touch, 10, by, 300, 42)) {
                s_selected = i;
                s_slideT = 0.0f;
                *currentScreen = ITEMS[i].target;
                return;
            }
        }
    }

    if (kDown & KEY_A) {
        *currentScreen = ITEMS[s_selected].target;
    }

    s_slideT = fminf(s_slideT + 1.0f/60.0f, 1.0f);
}

void menuRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();

    float slideUp = (1.0f - easeOutCubic(g_enterT)) * 8.0f;

    UI_Card(20, 50 + slideUp, 360, 148, 12, g_theme.surface);

    float pulse = (sinf(uiFrameTime() * 2.8f) + 1.0f) * 0.5f;
    ColorRGBA logoBg = themeMix(g_theme.accent, g_theme.onPrimary, pulse * 0.18f);
    UI_RoundRect(36, 66 + slideUp, 48, 48, 10, logoBg);
    UI_TextCenter(buf, NULL, "CDS", 60, 78 + slideUp, 0.48f, 0.48f, g_theme.onPrimary);

    UI_Text(buf, fontsCurrent(), "CustomizerDS", 96, 68 + slideUp, 0.54f, 0.54f, g_theme.textPrimary);
    UI_Text(buf, NULL, "syncmaker edition", 96, 90 + slideUp, 0.24f, 0.24f, g_theme.accent);

    UI_Text(buf, NULL, "fontes, tema escuro/claro e LED RGB", 36, 118 + slideUp, 0.28f, 0.28f, g_theme.textSecondary);
    UI_Text(buf, NULL, "tudo num toque, fluido ate no Old 3DS", 36, 140 + slideUp, 0.24f, 0.24f, g_theme.textHint);

    UI_Pill(300, 165 + slideUp, 80, 22, g_theme.primary, g_theme.onPrimary, buf, NULL, "v2.0", 0.30f, 0.30f);
}

void menuRenderBottom(C2D_TextBuf buf) {
    UI_BackgroundBottom();

    for (int i = 0; i < itemCount(); i++) {
        float by = 42.0f + i * 48.0f;
        float animT = (i == s_selected) ? s_slideT : 1.0f;
        UIButtonState st = (i == s_selected) ? UI_BUTTON_SELECTED : UI_BUTTON_NORMAL;
        UI_TouchButton(buf, fontsCurrent(), 10, by, 300, 42,
                       ITEMS[i].title, ITEMS[i].prefix, ITEMS[i].value, st, animT);
    }

    UI_KeyHelp(buf, NULL, "A abrir  B sair", "START sair");
}
