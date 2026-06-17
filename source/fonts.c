#include "fonts.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include "config.h"
#include <string.h>

FontSystem g_fonts;

static int selectedFont = 0;
static const char* fontLabels[] = {
    "Comfortaa Regular",
    "Comfortaa Bold",
    "MADE Evolve Sans",
    "MADE Evolve Bold",
    "Padrao do Sistema",
    "Voltar"
};
static const int FONT_COUNT = 6;
static Anim selectedAnim;

void fontsInit(void) {
    ConfigData cfg;
    configLoad(&cfg);
    selectedFont = cfg.fontIndex;
    if (selectedFont >= FONT_COUNT - 1) selectedFont = 0;
    animSet(&selectedAnim, 0.0f, 0.12f);
}

void fontsSystemInit(void) {
    g_fonts.comfortaaRegular = C2D_FontLoad("romfs:/fonts/comfortaa_regular.bcfnt");
    g_fonts.comfortaaBold = C2D_FontLoad("romfs:/fonts/comfortaa_bold.bcfnt");
    g_fonts.madeEvolveRegular = C2D_FontLoad("romfs:/fonts/made_evolve_regular.bcfnt");
    g_fonts.madeEvolveBold = C2D_FontLoad("romfs:/fonts/made_evolve_bold.bcfnt");
    g_fonts.current = NULL;
    g_fonts.currentIndex = 0;
}

void fontsSystemCleanup(void) {
    if (g_fonts.comfortaaRegular) C2D_FontFree(g_fonts.comfortaaRegular);
    if (g_fonts.comfortaaBold) C2D_FontFree(g_fonts.comfortaaBold);
    if (g_fonts.madeEvolveRegular) C2D_FontFree(g_fonts.madeEvolveRegular);
    if (g_fonts.madeEvolveBold) C2D_FontFree(g_fonts.madeEvolveBold);
}

void fontsRender(u32 kDown, u32 kHeld, int* currentScreen) {
    if (kDown & KEY_B) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (kDown & KEY_DOWN) selectedFont = (selectedFont + 1) % FONT_COUNT;
    if (kDown & KEY_UP) selectedFont = (selectedFont - 1 + FONT_COUNT) % FONT_COUNT;

    if (kDown & KEY_A) {
        if (selectedFont == FONT_COUNT - 1) {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        }
        switch (selectedFont) {
            case 0: g_fonts.current = g_fonts.comfortaaRegular; break;
            case 1: g_fonts.current = g_fonts.comfortaaBold; break;
            case 2: g_fonts.current = g_fonts.madeEvolveRegular; break;
            case 3: g_fonts.current = g_fonts.madeEvolveBold; break;
            default: g_fonts.current = NULL; break;
        }
        g_fonts.currentIndex = selectedFont;
        ConfigData cfg;
        configLoad(&cfg);
        cfg.fontIndex = selectedFont;
        configSave(&cfg);
    }

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) return;

    UI_Header(buf, "Fontes do Sistema", "Escolha uma fonte");

    animTo(&selectedAnim, selectedFont * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

    for (int i = 0; i < FONT_COUNT; i++) {
        bool selected = (i == selectedFont);
        float itemAnim = selected ? 1.0f : 0.0f;
        if (selected) {
            itemAnim = selectAnim - (int)selectAnim;
            if (itemAnim < 0) itemAnim += 1.0f;
        }
        UI_ListItem(buf, 10, 55 + i * 35, 300, 30, fontLabels[i],
                    NULL, selected, itemAnim, selected ? ">" : NULL,
                    g_fonts.current);
    }

    UI_Footer(buf, "Selecionar", "Voltar", NULL);
    C2D_TextBufDelete(buf);
}
