#include "fonts.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include <string.h>

FontSystem g_fonts;

static int selectedFont = 0;
static const char* fonts[] = {
    "Comfortaa Regular",
    "Comfortaa Bold",
    "MADE Evolve Sans",
    "Padrão do Sistema",
    "Voltar"
};
static const int FONT_COUNT = 5;
static Anim selectedAnim;

void fontsInit(void) {
    selectedFont = 0;
    animSet(&selectedAnim, 0.0f, 0.12f);
}

void fontsSystemInit(void) {
    g_fonts.comfortaaRegular = C2D_FontLoad("romfs:/fonts/comfortaa_regular.bcfnt");
    g_fonts.comfortaaBold = C2D_FontLoad("romfs:/fonts/comfortaa_bold.bcfnt");
    g_fonts.madeEvolveRegular = C2D_FontLoad("romfs:/fonts/made_evolve_regular.bcfnt");
    g_fonts.madeEvolveBold = C2D_FontLoad("romfs:/fonts/made_evolve_bold.bcfnt");
    g_fonts.currentFont = 0;
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

    if (kDown & KEY_DOWN) {
        selectedFont = (selectedFont + 1) % FONT_COUNT;
    }
    if (kDown & KEY_UP) {
        selectedFont = (selectedFont - 1 + FONT_COUNT) % FONT_COUNT;
    }
    if (kDown & KEY_A) {
        if (selectedFont == FONT_COUNT - 1) {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        } else {
            // Set currentFont based on selection
            if (selectedFont == 0 || selectedFont == 1) {
                g_fonts.currentFont = 1; // comfortaa
            } else if (selectedFont == 2) {
                g_fonts.currentFont = 2; // made_evolve
            } else if (selectedFont == 3) {
                g_fonts.currentFont = 0; // system
            }
        }
    }

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) {
        return;
    }

    // Header
    UI_Header(buf, "Fontes do Sistema", "Escolha uma fonte");

    // Animar selected
    animTo(&selectedAnim, selectedFont * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

    // Fontes
    for (int i = 0; i < FONT_COUNT; i++) {
        bool selected = (i == selectedFont);
        float itemAnim = selected ? 1.0f : 0.0f;
        if (selected) {
            itemAnim = selectAnim - (int)selectAnim;
            if (itemAnim < 0) itemAnim += 1.0f;
        }
        UI_ListItem(buf, 10, 55 + i*35, 300, 30, fonts[i],
                    NULL, selected, itemAnim, selected ? ">" : NULL);
    }

    // Footer
    UI_Footer(buf, "Selecionar", "Voltar", NULL);

    C2D_TextBufDelete(buf);
}
