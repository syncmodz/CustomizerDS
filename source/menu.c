#include "menu.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include <string.h>

static int selectedIndex = 0;
static const char* menuItems[] = {
    "Fontes do Sistema",
    "Modo Escuro/Tema",
    "LED RGB"
};
static const int MENU_COUNT = 3;
static Anim selectedAnim;

void menuInit(void) {
    selectedIndex = 0;
    animSet(&selectedAnim, 0.0f, 0.12f);
}

void menuRender(u32 kDown, u32 kHeld, int* currentScreen) {
    if (kDown & KEY_DOWN) {
        selectedIndex = (selectedIndex + 1) % MENU_COUNT;
    }
    if (kDown & KEY_UP) {
        selectedIndex = (selectedIndex - 1 + MENU_COUNT) % MENU_COUNT;
    }
    if (kDown & KEY_A) {
        switch (selectedIndex) {
            case 0: *currentScreen = SCREEN_FONTS; break;
            case 1: *currentScreen = SCREEN_DARKMODE; break;
            case 2: *currentScreen = SCREEN_LED; break;
        }
    }

    C2D_TextBuf buf = C2D_TextBufNew(4096);
    if (!buf) {
        return;
    }

    // Header
    UI_Header(buf, "CustomizerDS", "Menu Principal");

    // Animar selected
    animTo(&selectedAnim, selectedIndex * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

    // Itens do menu
    for (int i = 0; i < MENU_COUNT; i++) {
        bool selected = (i == selectedIndex);
        float itemAnim = selected ? 1.0f : 0.0f;
        if (selected) {
            itemAnim = selectAnim - (int)selectAnim;
            if (itemAnim < 0) itemAnim += 1.0f;
        }
        UI_ListItem(buf, 10, 50 + i*40, 300, 35, menuItems[i],
                    NULL, selected, itemAnim, selected ? ">" : NULL);
    }

    // Footer
    UI_Footer(buf, "Selecionar", "Voltar", NULL);

    C2D_TextBufDelete(buf);
}
