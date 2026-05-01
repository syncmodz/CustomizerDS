#include "darkmode.h"
#include "common.h"
#include <string.h>

static int darkModeEnabled = 0;
static int selectedOption = 0;
static const char* options[] = {
    "Ativar Modo Escuro",
    "Desativar Modo Escuro",
    "Voltar"
};
static const int OPTION_COUNT = 3;

void darkmodeInit(void) {
    selectedOption = 0;
}

void darkmodeRender(u32 kDown, u32 kHeld, int* currentScreen) {
    if (kDown & KEY_B) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (kDown & KEY_DOWN) {
        selectedOption = (selectedOption + 1) % OPTION_COUNT;
    }
    if (kDown & KEY_UP) {
        selectedOption = (selectedOption - 1 + OPTION_COUNT) % OPTION_COUNT;
    }
    if (kDown & KEY_A) {
        if (selectedOption == 0) {
            darkModeEnabled = 1;
        } else if (selectedOption == 1) {
            darkModeEnabled = 0;
        } else {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        }
    }

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) {
        return;
    }
    C2D_Text text;
    C2D_TextParse(&text, buf, "Modulo Modo Escuro");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, 0.4f, 10.0f, 10.0f, 0.4f, 0.4f, C2D_Color32(200, 200, 50, 255));

    C2D_TextParse(&text, buf, darkModeEnabled ? "Status: ATIVADO" : "Status: DESATIVADO");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, 0.3f, 10.0f, 40.0f, 0.3f, 0.3f,
        darkModeEnabled ? C2D_Color32(0, 255, 0, 255) : C2D_Color32(255, 0, 0, 255));

    for (int i = 0; i < OPTION_COUNT; i++) {
        u32 color = (i == selectedOption) ? C2D_Color32(60, 100, 150, 255) : C2D_Color32(40, 40, 50, 255);
        C2D_DrawRectSolid(10, 80 + i*30, 0, 300, 25, color);

        C2D_TextParse(&text, buf, options[i]);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, 0.3f, 20.0f, 83 + i*30, 0.3f, 0.3f,
            (i == selectedOption) ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(200, 200, 200, 255));
    }

    C2D_TextBufDelete(buf);
}
