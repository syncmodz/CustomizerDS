#include "fonts.h"
#include "common.h"
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

void fontsInit(void) {
    selectedFont = 0;
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
    C2D_Text text;
    C2D_TextParse(&text, buf, "Modulo Fontes");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, 0.4f, 10.0f, 10.0f, 0.4f, 0.4f, C2D_Color32(200, 200, 50, 255));

    for (int i = 0; i < FONT_COUNT; i++) {
        u32 color = (i == selectedFont) ? C2D_Color32(60, 100, 150, 255) : C2D_Color32(40, 40, 50, 255);
        C2D_DrawRectSolid(10, 50 + i*30, 0, 300, 25, color);

        C2D_TextParse(&text, buf, fonts[i]);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, 0.3f, 20.0f, 53 + i*30, 0.3f, 0.3f,
            (i == selectedFont) ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(200, 200, 200, 255));
    }

    C2D_TextBufDelete(buf);
}
