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
    "Comfortaa",
    "Comfortaa Negrito",
    "MADE Evolve Sans",
    "MADE Evolve Sans Negrito",
    "Padrao do Sistema",
};

/* Caminho dentro da romfs (arquivos .bcfnt em romfs/fonts, gerados com mkbcfnt
 * a partir dos .ttf/.otf reais). Indice correspondente em FONT_LABELS/g_fonts.fonts. */
static const char* FONT_PATHS[MAX_CUSTOM_FONTS] = {
    "romfs:/fonts/comfortaa_regular.bcfnt",
    "romfs:/fonts/comfortaa_bold.bcfnt",
    "romfs:/fonts/made_evolve_regular.bcfnt",
    "romfs:/fonts/made_evolve_bold.bcfnt",
};

int fontsSelected(void) { return s_selected; }

/* Nunca retorna NULL: se o .bcfnt de um indice nao carregou (arquivo ausente,
 * romfs nao montada etc.), cai pra fonte de sistema em vez de travar -- essa
 * falta de fallback era exatamente o que crashava a tela de Fontes antes. */
C2D_Font fontsGetFont(int index) {
    index = clampi(index, 0, fontsCount() - 1);
    if (index < MAX_CUSTOM_FONTS && g_fonts.fonts[index]) return g_fonts.fonts[index];
    return g_fonts.systemFont;
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
        g_fonts.fonts[i] = C2D_FontLoad(FONT_PATHS[i]);
    }
    g_fonts.systemFont = C2D_FontLoadSystem(CFG_REGION_USA);
    if (!g_fonts.systemFont) g_fonts.systemFont = C2D_FontLoadSystem(CFG_REGION_JPN);
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
        if (g_fonts.fonts[i]) { C2D_FontFree(g_fonts.fonts[i]); g_fonts.fonts[i] = NULL; }
    }
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
        case 0: return "Comfortaa";
        case 1: return "Comfortaa Negrito";
        case 2: return "MADE Evolve Sans";
        case 3: return "MADE Evolve Sans Negrito";
        default: return "Padrao do Sistema";
    }
}

void fontsRenderTop(C2D_TextBuf buf, float transVal) {
    /* Parallax 2 camadas: o card (midground) anda mais que o conteudo
     * dentro dele (foreground), que se acomoda primeiro -- igual ao
     * "Travel Motion" usado nas outras telas. */
    float offsetRaw = (1.0f - transVal);
    float offset = offsetRaw * 40.0f;
    float offsetFg = offsetRaw * 18.0f;
    UI_TopBackground();
    UI_TopMenuBar("Fontes", buf);

    UI_Card(16, 30 + offset, 368, 196, 16, g_theme.surface);

    UI_Text(buf, NULL, "Preview", 32, 52 + offsetFg, 0.28f, 0.28f, g_theme.textSecondary);
    UI_Text(buf, NULL, previewTitle(s_selected), 32, 70 + offsetFg, 0.48f, 0.48f, g_theme.textPrimary);

    C2D_Font f = fontsGetFont(s_selected);

    ColorRGBA previewBg = themeMix(g_theme.surfaceElevated, g_theme.accent, 0.08f);
    UI_RoundFrame(32, 112 + offsetFg, 300, 70, 14, previewBg, (ColorRGBA){255, 255, 255, 12});

    UI_Text(buf, f, previewLine(s_selected), 48, 126 + offsetFg,
            (s_selected == 2 || s_selected == 3) ? 0.38f : 0.34f,
            (s_selected == 1 || s_selected == 3) ? 0.42f : 0.34f,
            g_theme.textPrimary);
    UI_Text(buf, f, "Aa Bb Cc 123", 48, 152 + offsetFg,
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
    UI_RoundRect(badgeX, 50 + offsetFg, pw, 18, 9, badgeC);
    ColorRGBA badgeText = themeContrastText(badgeC);
    UI_TextCenter(buf, NULL, status, badgeX + pw * 0.5f, 52 + offsetFg, 0.22f, 0.22f, badgeText);
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
        UI_Text(buf, fontsGetFont(i), FONT_LABELS[i], 20 + slide, by + 5 + slide, 0.26f, 0.26f, textCol);

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
    return fontsGetFont(g_fonts.currentIndex);
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
    if (index < 0 || index >= MAX_CUSTOM_FONTS) return true; /* fonte de sistema */
    return g_fonts.fonts[index] != NULL;
}
