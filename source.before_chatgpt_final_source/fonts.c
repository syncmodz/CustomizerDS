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

int fontsSelected(void) { return s_selected; }

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
    if (idx < MAX_CUSTOM_FONTS) {
        g_fonts.current = fontsGetFont(idx);
    }
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

void fontsUpdate(const AppInput* in, int* currentScreen) {
    if (in->back) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (in->downNav) s_selected = (s_selected + 1) % fontsCount();
    if (in->up) s_selected = (s_selected - 1 + fontsCount()) % fontsCount();

    if (in->touchDown || in->touchHeld) {
        for (int i = 0; i < fontsCount(); i++) {
            float by = 16.0f + i * 40.0f;
            if (in->touchPY >= by && in->touchPY < by + 36 &&
                in->touchPX >= 10 && in->touchPX < 310) {
                if (in->touchDown) {
                    s_selected = i;
                    applyFont(s_selected, true);
                }
                return;
            }
        }
    }

    if (in->confirm) {
        applyFont(s_selected, true);
    }
}

void fontsRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();

    float et = easeOutCubic(g_enterT);
    float slideUp = (1.0f - et) * 10.0f;

    C2D_Font previewFont = NULL;
    if (s_selected < MAX_CUSTOM_FONTS) {
        if (g_fonts.fonts[s_selected] != NULL || !g_fonts.tried[s_selected]) {
            previewFont = fontsGetFont(s_selected);
        }
    }

    UI_GlassPanel(16, 40 + slideUp, 368, 160, 14,
                  g_theme.surface,
                  (ColorRGBA){255, 255, 255, 20},
                  (ColorRGBA){255, 255, 255, 12});

    UI_Text(buf, previewFont, "CustomizerDS", 32, 54 + slideUp, 0.72f, 0.72f, g_theme.textPrimary);
    UI_Text(buf, fontsCurrent(), "Aa Bb Cc 123  #F060A0", 32, 90 + slideUp, 0.42f, 0.42f, g_theme.accent);

    bool loaded = (s_selected >= MAX_CUSTOM_FONTS) || (g_fonts.fonts[s_selected] != NULL);

    const char* status = loaded ? "fonte carregada" : "fallback sistema";
    ColorRGBA badgeC = loaded ? g_theme.success : g_theme.warning;
    UI_Badge(buf, NULL, 32, 120 + slideUp, status, badgeC);

    char nameBuf[64];
    snprintf(nameBuf, sizeof(nameBuf), "%s", fontsLabel(s_selected));
    float nameX = 32 + (loaded ? 110 : 120);
    UI_Text(buf, NULL, nameBuf, nameX, 122 + slideUp, 0.22f, 0.22f, g_theme.textHint);

    float previewT = clampf(et * 2.0f, 0.0f, 1.0f);
    float py = lerpf(148 + slideUp, 144 + slideUp, previewT);
    UI_Text(buf, NULL, loaded ? "fonte real carregada via romfs" : "usando fonte padrao do sistema",
            32, py, 0.24f, 0.24f, g_theme.textSecondary);
}

void fontsRenderBottom(C2D_TextBuf buf) {
    UI_TouchBarBackground();

    float et = easeOutCubic(g_enterT);

    for (int i = 0; i < fontsCount(); i++) {
        float by = 16.0f + i * 40.0f;
        float appearT = clampf((et * 2.5f - i * 0.08f), 0.0f, 1.0f);
        UIButtonState st = (i == s_selected) ? UI_BUTTON_SELECTED : UI_BUTTON_NORMAL;
        const char* value = (i == g_fonts.currentIndex) ? "USANDO" : NULL;
        UI_CapsuleButton(buf, fontsGetFont(i), 10, by, 300, 36,
                         fontsLabel(i),
                         i == g_fonts.currentIndex ? "\xe2\x9c\x93" : NULL,
                         value, st, 0.0f, appearT);
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
    return g_fonts.fonts[index] != NULL;
}
