#include "fonts.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

FontSystem g_fonts;

static int s_selected = 0;

static const char* FONT_LABELS[] = {
    "Comfortaa Regular",
    "Comfortaa Bold",
    "MADE Evolve",
    "MADE Evolve Bold",
    "Padrao do Sistema",
};

static const char* FONT_PATHS[] = {
    "romfs:/fonts/comfortaa_regular.bcfnt",
    "romfs:/fonts/comfortaa_bold.bcfnt",
    "romfs:/fonts/made_evolve_regular.bcfnt",
    "romfs:/fonts/made_evolve_bold.bcfnt",
};

C2D_Font fontsGetFont(int index) {
    if (index < 0 || index >= MAX_CUSTOM_FONTS) return NULL;
    if (g_fonts.fonts[index] != NULL) return g_fonts.fonts[index];
    if (g_fonts.tried[index]) return NULL;
    g_fonts.tried[index] = true;
    g_fonts.fonts[index] = C2D_FontLoad(g_fonts.paths[index]);
    return g_fonts.fonts[index];
}

static void applyFont(int index, bool save) {
    index = clampi(index, 0, fontsCount() - 1);
    g_fonts.currentIndex = index;
    g_fonts.current = fontsGetFont(index);
    if (save) {
        ConfigData cfg;
        configLoad(&cfg);
        cfg.fontIndex = (u8)index;
        configSave(&cfg);
    }
}

void fontsSystemInit(void) {
    for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
        g_fonts.fonts[i] = NULL;
        g_fonts.tried[i] = false;
        g_fonts.paths[i] = FONT_PATHS[i];
    }
    g_fonts.current = NULL;
    g_fonts.currentIndex = 4;
    g_fonts.count = fontsCount();

    ConfigData cfg;
    configLoad(&cfg);
    int idx = clampi(cfg.fontIndex, 0, fontsCount() - 1);
    g_fonts.currentIndex = idx;
    g_fonts.current = fontsGetFont(idx);
}

void fontsInit(void) {
    s_selected = clampi(g_fonts.currentIndex, 0, fontsCount() - 1);
}

void fontsSystemCleanup(void) {
    for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
        if (g_fonts.fonts[i]) {
            C2D_FontFree(g_fonts.fonts[i]);
            g_fonts.fonts[i] = NULL;
        }
    }
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
            float by = 38.0f + i * 38.0f;
            if (touchIn(touch, 10, by, 300, 34)) {
                s_selected = i;
                applyFont(s_selected, true);
            }
        }
    }
    if (kDown & KEY_A) applyFont(s_selected, true);
}

void fontsRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();

    float slideUp = (1.0f - easeOutCubic(g_enterT)) * 8.0f;

    C2D_Font previewFont = fontsGetFont(s_selected);
    if (!previewFont) previewFont = NULL;

    UI_Card(16, 46 + slideUp, 368, 150, 12, g_theme.surface);

    UI_Text(buf, previewFont, "CustomizerDS", 34, 62 + slideUp, 0.76f, 0.76f, g_theme.textPrimary);
    UI_Text(buf, fontsCurrent(), "interface fluida, limpa e feita pro 3DS", 34, 96 + slideUp, 0.33f, 0.33f, g_theme.textSecondary);
    UI_Text(buf, previewFont, "Aa Bb Cc 123  #F060A0", 34, 126 + slideUp, 0.45f, 0.45f, g_theme.accent);

    bool loaded = fontsLoaded(s_selected);
    const char* status = loaded ? "carregada" : "fallback sistema";
    ColorRGBA badgeC = loaded ? g_theme.success : g_theme.warning;
    UI_Badge(buf, NULL, 34, 160 + slideUp, status, badgeC);

    UI_Text(buf, NULL, fontsLabel(s_selected), 34 + (loaded ? 90 : 110), 162 + slideUp, 0.22f, 0.22f, g_theme.textHint);
}

void fontsRenderBottom(C2D_TextBuf buf) {
    UI_BackgroundBottom();

    for (int i = 0; i < fontsCount(); i++) {
        float by = 38.0f + i * 38.0f;
        UIButtonState st = UI_BUTTON_NORMAL;
        if (i == s_selected) st = UI_BUTTON_SELECTED;
        const char* value = NULL;
        if (i == g_fonts.currentIndex) value = "USANDO";
        else if (!fontsLoaded(i)) value = "FALLBACK";
        UI_TouchButton(buf, fontsGetFont(i), 10, by, 300, 34,
                       fontsLabel(i), i == g_fonts.currentIndex ? "\xe2\x9c\x93" : NULL, value, st, 1.0f);
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

bool fontsLoaded(int index) {
    if (index >= MAX_CUSTOM_FONTS) return true;
    if (g_fonts.fonts[index] != NULL) return true;
    if (g_fonts.tried[index]) return false;
    fontsGetFont(index);
    return g_fonts.fonts[index] != NULL;
}
