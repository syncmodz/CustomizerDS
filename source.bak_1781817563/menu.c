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
    { "Tema do app", "UI", "modo e cor",   SCREEN_DARKMODE },
    { "LED RGB",     "L",  "cor e luz",    SCREEN_LED },
};

static int s_selected = 0;
static float s_slideT = 1.0f;
static float s_pressedT[3] = {0, 0, 0};

static int itemCount(void) {
    return (int)(sizeof(ITEMS) / sizeof(ITEMS[0]));
}

int menuSelected(void) { return s_selected; }

void menuInit(void) {
    s_selected = 0;
    for (int i = 0; i < 3; i++) s_pressedT[i] = 0;
}

void menuUpdate(const AppInput* in, int* currentScreen) {
    for (int i = 0; i < itemCount(); i++) {
        s_pressedT[i] = animApproach(s_pressedT[i], 0.0f, 0.30f);
    }

    if (in->downNav) {
        s_selected = (s_selected + 1) % itemCount();
        s_slideT = 0.0f;
    }
    if (in->up) {
        s_selected = (s_selected - 1 + itemCount()) % itemCount();
        s_slideT = 0.0f;
    }

    if (in->touchDown || in->touchHeld) {
        for (int i = 0; i < itemCount(); i++) {
            float by = 24.0f + i * 52.0f;
            if (in->touchPY >= by && in->touchPY < by + 44 &&
                in->touchPX >= 10 && in->touchPX < 310) {
                if (in->touchDown) {
                    s_selected = i;
                    s_slideT = 0.0f;
                    s_pressedT[i] = 1.0f;
                    *currentScreen = ITEMS[i].target;
                }
                return;
            }
        }
    }

    if (in->confirm) {
        s_pressedT[s_selected] = 1.0f;
        *currentScreen = ITEMS[s_selected].target;
    }

    s_slideT = fminf(s_slideT + 1.0f/60.0f, 1.0f);
}

void menuRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();

    float et = easeOutCubic(g_enterT);
    float slideUp = (1.0f - et) * 16.0f;

    UI_GlassPanel(18, 34 + slideUp, 364, 168, 18,
                  g_theme.surface,
                  (ColorRGBA){255, 255, 255, 22},
                  (ColorRGBA){255, 255, 255, 14});

    float pulse = (sinf(uiFrameTime() * 2.8f) + 1.0f) * 0.5f;
    ColorRGBA logoBg = themeMix(g_theme.accent, g_theme.onPrimary, pulse * 0.15f);
    float logoS = 56;
    UI_RoundFrame(32 + slideUp * 0.25f, 52 + slideUp, logoS, logoS, 18, logoBg,
                  (ColorRGBA){255, 255, 255, 34});
    UI_TextCenter(buf, NULL, "CDS", 32 + slideUp * 0.25f + logoS * 0.5f, 68 + slideUp, 0.50f, 0.50f, themeContrastText(logoBg));

    UI_Text(buf, NULL, "CustomizerDS", 104, 54 + slideUp, 0.58f, 0.58f, g_theme.textPrimary);
    UI_Text(buf, NULL, "console personalization studio", 104, 82 + slideUp, 0.25f, 0.25f, g_theme.accent);

    UI_Text(buf, NULL, ITEMS[s_selected].title, 104, 112 + slideUp, 0.34f, 0.34f, g_theme.textPrimary);
    UI_Text(buf, NULL, ITEMS[s_selected].value, 104, 130 + slideUp, 0.25f, 0.25f, g_theme.textSecondary);

    const char* tags[] = {"fontes", "led rgb", "tema"};
    float tagStartX = 34;
    for (int i = 0; i < 3; i++) {
        float tagX = tagStartX + i * 76;
        float tagAp = easeOutCubic(clampf((et * 3.0f - 0.5f - i * 0.15f), 0.0f, 1.0f));
        float ty = 166 + slideUp + (1.0f - tagAp) * 8.0f;
        UI_TouchBarPill(tagX, ty, 68, 22, (ColorRGBA){22, 27, 38, 218},
                        tags[i], buf, NULL, 0.24f, 0.24f);
    }
}

void menuRenderBottom(C2D_TextBuf buf) {
    UI_TouchBarBackground();

    float et = easeOutCubic(g_enterT);

    for (int i = 0; i < itemCount(); i++) {
        float by = 24.0f + i * 52.0f;
        float appearT = clampf((et * 2.0f - i * 0.12f), 0.0f, 1.0f);
        UIButtonState st = (i == s_selected) ? UI_BUTTON_SELECTED : UI_BUTTON_NORMAL;
        UI_CapsuleButton(buf, NULL, 10, by, 300, 44,
                         ITEMS[i].title, ITEMS[i].prefix, ITEMS[i].value,
                         st, s_pressedT[i], appearT);
    }

    UI_KeyHelp(buf, NULL, "A abrir  B sair", "START sair");
}
