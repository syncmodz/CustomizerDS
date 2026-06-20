#include "menu.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "fonts.h"
#include <stdio.h>
#include <math.h>

static int s_selected = 0;
static float s_selY = 0.0f;

typedef struct {
    const char* title;
    const char* subtitle;
    ScreenType target;
} MenuItem;

static const MenuItem ITEMS[] = {
    { "Fontes", "trocar fonte do app e ver preview", SCREEN_FONTS },
    { "Tema do app", "modo claro, escuro e cor destaque", SCREEN_DARKMODE },
    { "LED RGB", "cor solida, pulso e arco-iris", SCREEN_LED },
};

static int itemCount(void) {
    return (int)(sizeof(ITEMS) / sizeof(ITEMS[0]));
}

void menuInit(void) {
    s_selected = 0;
}

void menuUpdate(u32 kDown, touchPosition* touch, bool touchDown, int* currentScreen) {
    if (kDown & KEY_DOWN) s_selected = (s_selected + 1) % itemCount();
    if (kDown & KEY_UP) s_selected = (s_selected - 1 + itemCount()) % itemCount();

    if (touchDown) {
        for (int i = 0; i < itemCount(); i++) {
            if (touchIn(touch, 10, 42 + i * 48, 300, 40)) {
                s_selected = i;
                *currentScreen = ITEMS[i].target;
                return;
            }
        }
    }

    if (kDown & KEY_A) {
        *currentScreen = ITEMS[s_selected].target;
    }
}

void menuRenderTop(C2D_TextBuf buf) {
    UI_HeaderTop(buf, fontsCurrent(), "CustomizerDS", "feito pra personalizar teu 3DS");

    UI_DimmedGrid(SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT);

    UI_Panel(18, 60, 364, 116, g_theme.surface, g_theme.divider);
    UI_AccentBar(18, 60, 5, 116);
    UI_Text(buf, fontsCurrent(), "syncmaker edition", 36, 78, 0.44f, 0.44f, g_theme.accent);
    UI_Text(buf, fontsCurrent(), "fontes, tema e LED em um painel touch", 36, 103, 0.33f, 0.33f, g_theme.textPrimary);
    UI_Text(buf, NULL, "leve no Old 3DS: sem textura pesada, sem alocacao absurda, tudo desenhado em 2D simples.", 36, 132, 0.23f, 0.23f, g_theme.textSecondary);

    ColorRGBA c = g_theme.accent;
    float pulse = (sinf(uiFrameTime() * 3.0f) + 1.0f) * 0.5f;
    c = themeMix(c, g_theme.onPrimary, pulse * 0.25f);
    UI_Fill(315, 82, 36, 36, c);
    UI_Fill(323, 90, 20, 20, g_theme.backgroundTop);
}

void menuRenderBottom(C2D_TextBuf buf) {
    UI_TouchChrome(buf, fontsCurrent(), "menu", "toque para abrir");

    float targetY = 42.0f + s_selected * 48.0f;
    s_selY += (targetY - s_selY) * 0.18f;
    UI_RoundRect(8, s_selY, 304, 40, 4, themeMix(g_theme.surface, g_theme.primary, 0.12f));

    for (int i = 0; i < itemCount(); i++) {
        UI_Button(buf, fontsCurrent(), 10, 42 + i * 48, 300, 40,
                  ITEMS[i].title, (i == s_selected) ? ">" : NULL,
                  i == s_selected ? UI_BUTTON_SELECTED : UI_BUTTON_NORMAL);
        UI_Text(buf, NULL, ITEMS[i].subtitle, 20, 42 + i * 48 + 24, 0.22f, 0.22f, g_theme.textSecondary);
    }

    UI_KeyHelp(buf, NULL, "A abrir", "START sair");
}
