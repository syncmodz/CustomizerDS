#include "fonts.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include <string.h>

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
    if (kDown & KEY_A && selectedFont == FONT_COUNT - 1) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
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
