#include "menu.h"
#include "common.h"
#include <string.h>

static int selectedIndex = 0;
static const char* menuItems[] = {
    "Assets/Icones",
    "Fontes do Sistema",
    "Modo Escuro/Tema",
    "LED RGB",
    "Sair"
};
static const int MENU_COUNT = 5;

void menuInit(void) {
    selectedIndex = 0;
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
            case 0: *currentScreen = SCREEN_ASSETS; break;
            case 1: *currentScreen = SCREEN_FONTS; break;
            case 2: *currentScreen = SCREEN_DARKMODE; break;
            case 3: *currentScreen = SCREEN_LED; break;
            case 4: *currentScreen = SCREEN_MAIN_MENU; break;
        }
    }

    C2D_TextBuf buf = C2D_TextBufNew(4096);
    if (!buf) {
        return;
    }
    C2D_Text title, item;
    C2D_TextParse(&title, buf, "CustomizerDS - Menu");
    C2D_TextOptimize(&title);
    C2D_DrawText(&title, 0.4f, 10.0f, 10.0f, 0.4f, 0.4f, C2D_Color32(200, 200, 50, 255));

    for (int i = 0; i < MENU_COUNT; i++) {
        u32 color = (i == selectedIndex) ? C2D_Color32(60, 100, 150, 255) : C2D_Color32(40, 40, 50, 255);
        C2D_DrawRectSolid(10, 30 + i*35, 0, 300, 30, color);

        C2D_TextParse(&item, buf, menuItems[i]);
        C2D_TextOptimize(&item);
        C2D_DrawText(&item, 0.3f, 20.0f, 35 + i*35, 0.3f, 0.3f,
            (i == selectedIndex) ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(200, 200, 200, 255));
    }

    C2D_TextBufDelete(buf);
}
