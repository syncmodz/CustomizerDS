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

/* Sao presets de estilo (peso/tamanho) sobre a fonte de sistema, nao tipografias
 * separadas -- nao existe .bcfnt de Comfortaa/MADE Evolve no romfs, e carregar
 * um arquivo que nao existe e exatamente o que travava a tela de Fontes antes.
 * Os nomes refletem isso para nao prometer uma fonte que o app nao tem. */
static const char* FONT_LABELS[] = {
    "Redondo",
    "Redondo Negrito",
    "Moderno",
    "Moderno Negrito",
    "Padrao do Sistema",
};

int fontsSelected(void) { return s_selected; }

C2D_Font fontsGetFont(int index) {
    (void)index;
    return g_fonts.systemFont;
}

static void applyFont(int index, bool save) {
    index = clampi(index, 0, fontsCount() - 1);
    g_fonts.currentIndex = index;
    g_fonts.current = g_fonts.systemFont;
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
    }
    g_fonts.systemFont = C2D_FontLoadSystem(CFG_REGION_USA);
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
}

void fontsSystemCleanup(void) {
    if (g_fonts.systemFont) {
        C2D_FontFree(g_fonts.systemFont);
        g_fonts.systemFont = NULL;
    }
}

void fontsUpdate(const AppInput* in, int* currentScreen) {
    if (in->back) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (in->downNav) s_selected = (s_selected + 1) % fontsCount();
    if (in->up) s_selected = (s_selected - 1 + fontsCount()) % fontsCount();

    if (in->touchDown) {
        for (int i = 0; i < fontsCount(); i++) {
            float by = 56.0f + i * 34.0f;
            if (in->touchPY >= by && in->touchPY < by + 28 &&
                in->touchPX >= 10 && in->touchPX < 310) {
                s_selected = i;
                applyFont(s_selected, true);
                return;
            }
        }
    }

    if (in->confirm) {
        applyFont(s_selected, true);
    }
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
        case 0: return "Redondo";
        case 1: return "Redondo Negrito";
        case 2: return "Moderno";
        case 3: return "Moderno Negrito";
        default: return "Padrao do Sistema";
    }
}

void fontsRenderTop(C2D_TextBuf buf, float transVal) {
    float offset = (1.0f - transVal) * 40.0f;
    UI_TopBackground();
    UI_TopMenuBar("Fontes", buf);

    UI_Card(16, 30 + offset, 368, 196, 16, g_theme.surface);

    UI_Text(buf, NULL, "Preview", 32, 52 + offset, 0.28f, 0.28f, g_theme.textSecondary);
    UI_Text(buf, NULL, previewTitle(s_selected), 32, 70 + offset, 0.48f, 0.48f, g_theme.textPrimary);

    C2D_Font f = g_fonts.systemFont;
    if (!f) f = NULL;

    ColorRGBA previewBg = themeMix(g_theme.surfaceElevated, g_theme.accent, 0.08f);
    UI_RoundFrame(32, 112 + offset, 300, 70, 14, previewBg, (ColorRGBA){255, 255, 255, 12});

    UI_Text(buf, f, previewLine(s_selected), 48, 126 + offset,
            (s_selected == 2 || s_selected == 3) ? 0.38f : 0.34f,
            (s_selected == 1 || s_selected == 3) ? 0.42f : 0.34f,
            g_theme.textPrimary);
    UI_Text(buf, f, "Aa Bb Cc 123", 48, 152 + offset,
            (s_selected == 2 || s_selected == 3) ? 0.44f : 0.40f,
            (s_selected == 1 || s_selected == 3) ? 0.48f : 0.40f,
            g_theme.accent);

    char status[24];
    snprintf(status, sizeof(status), "%s",
             (s_selected == g_fonts.currentIndex) ? "em uso" : "A para aplicar");
    float tw = 0;
    C2D_Text tmp2;
    C2D_TextParse(&tmp2, buf, status);
    C2D_TextOptimize(&tmp2);
    C2D_TextGetDimensions(&tmp2, 0.22f, 0.22f, &tw, NULL);
    float pw = tw + 14.0f;
    float badgeX = (16 + 368) - pw - 8; /* relativo a borda direita do card (cima tem 400px, nao 320) */
    ColorRGBA badgeC = (s_selected == g_fonts.currentIndex) ? g_theme.success : g_theme.accent;
    UI_RoundRect(badgeX, 50 + offset, pw, 18, 9, badgeC);
    ColorRGBA badgeText = themeContrastText(badgeC);
    UI_TextCenter(buf, NULL, status, badgeX + pw * 0.5f, 52 + offset, 0.22f, 0.22f, badgeText);
}

void fontsRenderBottom(C2D_TextBuf buf, float transVal) {
    float offset = (1.0f - transVal) * 30.0f;
    float et = UI_EnterProgress();
    UI_BottomBackground();

    for (int i = 0; i < fontsCount(); i++) {
        float by = 56.0f + i * 34.0f + offset;
        float appearT = clampf((et * 2.5f - i * 0.08f), 0.0f, 1.0f);
        float ap = easeOutCubic(appearT);
        if (ap <= 0.0f) continue;
        float slide = (1.0f - ap) * 14.0f;

        bool selected = (i == s_selected);
        bool isCurrent = (i == g_fonts.currentIndex);

        ColorRGBA bg = selected
            ? themeMix(g_theme.surface, g_theme.accent, 0.12f)
            : (themeIsDark() ? (ColorRGBA){26, 28, 38, 240} : (ColorRGBA){238, 240, 248, 240});
        ColorRGBA border = selected
            ? g_theme.accent
            : (themeIsDark() ? (ColorRGBA){255, 255, 255, 8} : (ColorRGBA){0, 0, 0, 8});
        if (selected) border.a = 80;

        UI_RoundFrame(10 + slide, by + slide, 300, 28, 14, bg, border);

        ColorRGBA textCol = selected ? g_theme.textPrimary : g_theme.textSecondary;
        UI_Text(buf, NULL, FONT_LABELS[i], 20 + slide, by + 5 + slide, 0.26f, 0.26f, textCol);

        if (isCurrent) {
            UI_Badge(buf, 260, by + 4 + slide, "OK", g_theme.success);
        }

        if (i < fontsCount() - 1) {
            ColorRGBA div = {255, 255, 255, themeIsDark() ? 6 : 18};
            UI_Fill(20, by + 30 + slide, 280, 1, div);
        }
    }

    UI_HelpBar(buf, "A aplicar", "START sair");
}

C2D_Font fontsCurrent(void) {
    return g_fonts.systemFont;
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
    (void)index;
    return true;
}
