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
    { "Fontes",      "Aa", "personalizar", SCREEN_FONTS },
    { "Tema do app", "\xe2\x97\x89", "modo e cor",   SCREEN_DARKMODE },
    { "LED RGB",     "\xe2\x97\x8b", "cor e luz",    SCREEN_LED },
};

static int s_selected = 0;
static float s_slideT = 1.0f;
static float s_pressedT[3] = {0, 0, 0};

static int itemCount(void) {
    return (int)(sizeof(ITEMS) / sizeof(ITEMS[0]));
}

void menuInit(void) {
    s_selected = 0;
    for (int i = 0; i < 3; i++) s_pressedT[i] = 0;
}

void menuUpdate(u32 kDown, touchPosition* touch, bool touchDown, int* currentScreen) {
    if (kDown & KEY_DOWN) {
        s_selected = (s_selected + 1) % itemCount();
        s_slideT = 0.0f;
    }
    if (kDown & KEY_UP) {
        s_selected = (s_selected - 1 + itemCount()) % itemCount();
        s_slideT = 0.0f;
    }

    for (int i = 0; i < itemCount(); i++) {
        s_pressedT[i] = animApproach(s_pressedT[i], 0.0f, 0.30f);
    }

    if (touchDown) {
        for (int i = 0; i < itemCount(); i++) {
            float by = 42.0f + i * 50.0f;
            if (touchIn(touch, 10, by, 300, 44)) {
                s_selected = i;
                s_slideT = 0.0f;
                s_pressedT[i] = 1.0f;
                *currentScreen = ITEMS[i].target;
                return;
            }
        }
    }

    if (kDown & KEY_A) {
        s_pressedT[s_selected] = 1.0f;
        *currentScreen = ITEMS[s_selected].target;
    }

    s_slideT = fminf(s_slideT + 1.0f/60.0f, 1.0f);
}

void menuRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();

    float et = easeOutCubic(g_enterT);
    float slideUp = (1.0f - et) * 10.0f;

    UI_GlassPanel(20, 40 + slideUp, 360, 160, 14,
                  g_theme.surface,
                  (ColorRGBA){255, 255, 255, 20},
                  (ColorRGBA){255, 255, 255, 12});

    float pulse = (sinf(uiFrameTime() * 2.8f) + 1.0f) * 0.5f;
    ColorRGBA logoBg = themeMix(g_theme.accent, g_theme.onPrimary, pulse * 0.15f);
    float logoS = 50;
    UI_RoundRect(34 + slideUp * 0.3f, 54 + slideUp, logoS, logoS, 12, logoBg);
    UI_TextCenter(buf, NULL, "CDS", 34 + slideUp * 0.3f + logoS * 0.5f, 66 + slideUp, 0.50f, 0.50f, g_theme.onPrimary);

    UI_Text(buf, fontsCurrent(), "CustomizerDS", 98, 56 + slideUp, 0.56f, 0.56f, g_theme.textPrimary);
    UI_Text(buf, NULL, "syncmaker edition", 98, 80 + slideUp, 0.24f, 0.24f, g_theme.accent);

    const char* tags[] = {"fontes", "led rgb", "tema"};
    float tagStartX = 34;
    for (int i = 0; i < 3; i++) {
        float tagX = tagStartX + i * 76;
        float tagAp = easeOutCubic(clampf((et * 3.0f - 0.5f - i * 0.15f), 0.0f, 1.0f));
        float ty = 158 + slideUp + (1.0f - tagAp) * 8.0f;
        UI_TouchBarPill(tagX, ty, 68, 22, (ColorRGBA){24, 28, 38, 200},
                        tags[i], buf, NULL, 0.24f, 0.24f);
    }

    UI_Text(buf, NULL, "toque para personalizar", 30, 132 + slideUp, 0.26f, 0.26f, g_theme.textHint);
}

void menuRenderBottom(C2D_TextBuf buf) {
    UI_TouchBarBackground();

    float et = easeOutCubic(g_enterT);

    for (int i = 0; i < itemCount(); i++) {
        float by = 26.0f + i * 52.0f;
        float appearT = clampf((et * 2.0f - i * 0.12f), 0.0f, 1.0f);
        UIButtonState st = (i == s_selected) ? UI_BUTTON_SELECTED : UI_BUTTON_NORMAL;
        UI_CapsuleButton(buf, fontsCurrent(), 10, by, 300, 44,
                         ITEMS[i].title, ITEMS[i].prefix, ITEMS[i].value,
                         st, s_pressedT[i], appearT);
    }

    UI_KeyHelp(buf, NULL, "A abrir  B sair", "START sair");
}
