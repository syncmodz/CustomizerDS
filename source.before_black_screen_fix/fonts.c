#include "fonts.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "config.h"
#include <stdio.h>

FontSystem g_fonts;

static int s_selected = 0;
static float s_selY = 0.0f;

static const char* FONT_LABELS[] = {
    "Comfortaa Regular",
    "Comfortaa Bold",
    "MADE Evolve",
    "MADE Evolve Bold",
    "Padrao do Sistema",
};

static C2D_Font fontByIndex(int index) {
    switch (index) {
        case 0: return g_fonts.comfortaaRegular;
        case 1: return g_fonts.comfortaaBold;
        case 2: return g_fonts.madeEvolveRegular;
        case 3: return g_fonts.madeEvolveBold;
        default: return NULL;
    }
}

static void applyFont(int index, bool save) {
    index = clampi(index, 0, fontsCount() - 1);
    g_fonts.currentIndex = index;
    g_fonts.current = fontByIndex(index);

    if (save) {
        ConfigData cfg;
        configLoad(&cfg);
        cfg.fontIndex = (u8)index;
        configSave(&cfg);
    }
}

void fontsSystemInit(void) {
    g_fonts.comfortaaRegular = C2D_FontLoad("romfs:/fonts/comfortaa_regular.bcfnt");
    g_fonts.comfortaaBold = C2D_FontLoad("romfs:/fonts/comfortaa_bold.bcfnt");
    g_fonts.madeEvolveRegular = C2D_FontLoad("romfs:/fonts/made_evolve_regular.bcfnt");
    g_fonts.madeEvolveBold = C2D_FontLoad("romfs:/fonts/made_evolve_bold.bcfnt");

    ConfigData cfg;
    configLoad(&cfg);
    applyFont(cfg.fontIndex, false);
}

void fontsInit(void) {
    s_selected = clampi(g_fonts.currentIndex, 0, fontsCount() - 1);
}

void fontsSystemCleanup(void) {
    if (g_fonts.comfortaaRegular) C2D_FontFree(g_fonts.comfortaaRegular);
    if (g_fonts.comfortaaBold) C2D_FontFree(g_fonts.comfortaaBold);
    if (g_fonts.madeEvolveRegular) C2D_FontFree(g_fonts.madeEvolveRegular);
    if (g_fonts.madeEvolveBold) C2D_FontFree(g_fonts.madeEvolveBold);
}

void fontsUpdate(u32 kDown, touchPosition* touch, bool touchDown, int* currentScreen) {
    if (kDown & KEY_B) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (kDown & KEY_DOWN) s_selected = (s_selected + 1) % fontsCount();
    if (kDown & KEY_UP) s_selected = (s_selected - 1 + fontsCount()) % fontsCount();

    if (touchDown) {
        for (int i = 0; i < fontsCount(); i++) {
            if (touchIn(touch, 10, 38 + i * 30, 300, 26)) {
                s_selected = i;
                applyFont(s_selected, true);
            }
        }
        if (touchIn(touch, 8, 206, 78, 26)) {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        }
    }

    if (kDown & KEY_A) applyFont(s_selected, true);
}

void fontsRenderTop(C2D_TextBuf buf) {
    UI_HeaderTop(buf, fontsCurrent(), "Fontes", "preview real do texto antes de aplicar");

    C2D_Font previewFont = fontsGetFont(s_selected);

    UI_RoundPanel(16, 58, 368, 130, 6, g_theme.surface, g_theme.divider);
    UI_AccentBar(16, 58, 4, 130);

    UI_Text(buf, previewFont, "CustomizerDS", 34, 75, 0.76f, 0.76f, g_theme.textPrimary);
    UI_Text(buf, fontsCurrent(), "interface fluida, limpa e feita pro 3DS", 34, 110, 0.33f, 0.33f, g_theme.textSecondary);
    UI_Text(buf, previewFont, "Aa Bb Cc 123  #F060A0", 34, 141, 0.45f, 0.45f, g_theme.accent);

    const char* status = fontsLoaded(s_selected) ? "fonte carregada" : "fallback sistema";
    UI_Text(buf, previewFont, status, 34, 169, 0.24f, 0.24f, g_theme.textHint);
}

void fontsRenderBottom(C2D_TextBuf buf) {
    UI_TouchChrome(buf, fontsCurrent(), "fontes", "toque ou setas");

    float targetY = 38.0f + s_selected * 30.0f;
    s_selY += (targetY - s_selY) * 0.18f;
    UI_RoundRect(8, s_selY, 304, 26, 4, themeMix(g_theme.surface, g_theme.primary, 0.12f));

    for (int i = 0; i < fontsCount(); i++) {
        UIButtonState state = UI_BUTTON_NORMAL;
        if (i == g_fonts.currentIndex) state = UI_BUTTON_ACTIVE;
        if (i == s_selected) state = UI_BUTTON_SELECTED;

        const char* value = NULL;
        if (i == g_fonts.currentIndex) value = "USANDO";
        if (!fontsLoaded(i)) value = "FALLBACK";

        UI_Button(buf, fontByIndex(i), 10, 38 + i * 30, 300, 26,
                  fontsLabel(i), value, state);
    }

    UI_KeyHelp(buf, NULL, "A aplicar  B voltar", "START sair");
}

C2D_Font fontsCurrent(void) {
    return g_fonts.current;
}

const char* fontsCurrentName(void) {
    return fontsLabel(g_fonts.currentIndex);
}

const char* fontsLabel(int index) {
    index = clampi(index, 0, fontsCount() - 1);
    return FONT_LABELS[index];
}

int fontsCount(void) {
    return (int)(sizeof(FONT_LABELS) / sizeof(FONT_LABELS[0]));
}

C2D_Font fontsGetFont(int index) {
    return fontByIndex(index);
}

bool fontsLoaded(int index) {
    if (index == 4) return true;
    return fontByIndex(index) != NULL;
}
