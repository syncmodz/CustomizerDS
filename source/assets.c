#include "assets.h"
#include "common.h"
#include <string.h>

static int selectedAsset = 0;
static const char* assets[] = {
    "Reva UI - Icon Pack 1",
    "Reva UI - Icon Pack 2",
    "Reva UI - Icon Pack 3",
    "Voltar"
};
static const int ASSET_COUNT = 4;

void assetsInit(void) {
    selectedAsset = 0;
}

void assetsRender(u32 kDown, u32 kHeld, int* currentScreen) {
    if (kDown & KEY_B) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (kDown & KEY_DOWN) {
        selectedAsset = (selectedAsset + 1) % ASSET_COUNT;
    }
    if (kDown & KEY_UP) {
        selectedAsset = (selectedAsset - 1 + ASSET_COUNT) % ASSET_COUNT;
    }
    if (kDown & KEY_A && selectedAsset == ASSET_COUNT - 1) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) {
        return;
    }
    C2D_Text text;
    C2D_TextParse(&text, buf, "Modulo Assets/Icones");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, 0.4f, 10.0f, 10.0f, 0.4f, 0.4f, C2D_Color32(200, 200, 50, 255));

    for (int i = 0; i < ASSET_COUNT; i++) {
        u32 color = (i == selectedAsset) ? C2D_Color32(60, 100, 150, 255) : C2D_Color32(40, 40, 50, 255);
        C2D_DrawRectSolid(10, 50 + i*30, 0, 300, 25, color);

        C2D_TextParse(&text, buf, assets[i]);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, 0.3f, 20.0f, 53 + i*30, 0.3f, 0.3f,
            (i == selectedAsset) ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(200, 200, 200, 255));
    }

    C2D_TextBufDelete(buf);
}
