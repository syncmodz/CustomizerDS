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
static float s_previewT = 1.0f;

#define FONT_REAL_LOAD 0

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
#if FONT_REAL_LOAD
    if (g_fonts.fonts[index] != NULL) return g_fonts.fonts[index];
    if (g_fonts.tried[index]) return NULL;
    g_fonts.tried[index] = true;
    g_fonts.fonts[index] = C2D_FontLoad(g_fonts.paths[index]);
    return g_fonts.fonts[index];
#else
    g_fonts.tried[index] = true;
    return NULL;
#endif
}

static void applyFont(int index, bool save) {
    index = clampi(index, 0, fontsCount() - 1);
    g_fonts.currentIndex = index;
#if FONT_REAL_LOAD
    g_fonts.current = fontsGetFont(index);
#else
    g_fonts.current = NULL;
#endif
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
    g_fonts.current = NULL;
}

void fontsInit(void) {
    s_selected = clampi(g_fonts.currentIndex, 0, fontsCount() - 1);
    s_previewT = 0.0f;
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

    int oldSelected = s_selected;
    if (in->downNav) s_selected = (s_selected + 1) % fontsCount();
    if (in->up) s_selected = (s_selected - 1 + fontsCount()) % fontsCount();
    if (oldSelected != s_selected) s_previewT = 0.0f;

    if (in->touchDown || in->touchHeld) {
        for (int i = 0; i < fontsCount(); i++) {
            float by = 10.0f + i * 38.0f;
            if (in->touchPY >= by && in->touchPY < by + 36 &&
                in->touchPX >= 10 && in->touchPX < 310) {
                if (in->touchDown) {
                    oldSelected = s_selected;
                    s_selected = i;
                    if (oldSelected != s_selected) s_previewT = 0.0f;
                    applyFont(s_selected, true);
                }
                return;
            }
        }
    }

    if (in->confirm) {
        applyFont(s_selected, true);
    }

    s_previewT = fminf(s_previewT + 1.0f / 60.0f, 1.0f);
}

static const char* previewLine(int index) {
    switch (index) {
        case 0: return "round friendly interface";
        case 1: return "ROUND FRIENDLY BOLD";
        case 2: return "sleek future display";
        case 3: return "SLEEK FUTURE BOLD";
        default: return "system default preview";
    }
}

static const char* previewTitle(int index) {
    switch (index) {
        case 0: return "Comfortaa";
        case 1: return "Comfortaa Bold";
        case 2: return "MADE Evolve";
        case 3: return "MADE Evolve Bold";
        default: return "Padrao do Sistema";
    }
}

static void drawFontPreviewBars(C2D_TextBuf buf, int index, float x, float y, float t) {
    float a = easeOutCubic(t);
    ColorRGBA soft = themeMix(g_theme.surfaceElevated, g_theme.accent, 0.10f);
    ColorRGBA line = themeMix(g_theme.divider, g_theme.accent, 0.18f);
    float slide = (1.0f - a) * 10.0f;
    UI_RoundFrame(x, y + slide, 308, 78, 18, soft, (ColorRGBA){255, 255, 255, 18});
    UI_Text(buf, NULL, previewLine(index), x + 18, y + 18 + slide,
            (index == 2 || index == 3) ? 0.40f : 0.36f,
            (index == 1 || index == 3) ? 0.43f : 0.36f,
            g_theme.textPrimary);
    UI_Text(buf, NULL, "Aa Bb Cc 123", x + 18, y + 43 + slide,
            (index == 2 || index == 3) ? 0.48f : 0.44f,
            (index == 1 || index == 3) ? 0.52f : 0.44f,
            g_theme.accent);

    float chipW = (index == 2 || index == 3) ? 42.0f : 34.0f;
    for (int i = 0; i < 4; i++) {
        float cx = x + 188 + i * (chipW + 6);
        float cy = y + 18 + slide + (i % 2) * 18;
        UI_RoundRect(cx, cy, chipW, 10, 5, line);
    }
}

void fontsRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();

    float et = easeOutCubic(g_enterT);
    float slideUp = (1.0f - et) * 16.0f;

    UI_GlassPanel(18, 30 + slideUp, 364, 178, 18,
                  g_theme.surface,
                  (ColorRGBA){255, 255, 255, 22},
                  (ColorRGBA){255, 255, 255, 14});

    UI_Text(buf, NULL, "Preview de Fonte", 32, 48 + slideUp, 0.38f, 0.38f, g_theme.textSecondary);
    UI_Text(buf, NULL, previewTitle(s_selected), 32, 68 + slideUp, 0.58f, 0.58f, g_theme.textPrimary);

    char nameBuf[64];
    snprintf(nameBuf, sizeof(nameBuf), "%s", (s_selected == g_fonts.currentIndex) ? "aplicada no app" : "A para aplicar");
    UI_Badge(buf, NULL, 282, 48 + slideUp, nameBuf,
             (s_selected == g_fonts.currentIndex) ? g_theme.success : g_theme.accent);

    drawFontPreviewBars(buf, s_selected, 32, 106 + slideUp, s_previewT);

    float previewT = clampf(et * 2.0f, 0.0f, 1.0f);
    float py = lerpf(196 + slideUp, 190 + slideUp, previewT);
    UI_Text(buf, NULL, "preview seguro sem travar render; arquivos CFNT seguem no romfs",
            32, py, 0.23f, 0.23f, g_theme.textHint);
}

void fontsRenderBottom(C2D_TextBuf buf) {
    UI_TouchBarBackground();

    float et = easeOutCubic(g_enterT);

    for (int i = 0; i < fontsCount(); i++) {
        float by = 10.0f + i * 38.0f;
        float appearT = clampf((et * 2.5f - i * 0.08f), 0.0f, 1.0f);
        UIButtonState st = (i == s_selected) ? UI_BUTTON_SELECTED : UI_BUTTON_NORMAL;
        const char* value = (i == g_fonts.currentIndex) ? "USANDO" : NULL;
        UI_CapsuleButton(buf, NULL, 10, by, 300, 34,
                         fontsLabel(i),
                         i == g_fonts.currentIndex ? "OK" : NULL,
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
