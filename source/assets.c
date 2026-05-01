#include "assets.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include <string.h>

static int selectedAsset = 0;
static const char* assets[] = {
    "Reva UI - Icon Pack 1",
    "Reva UI - Icon Pack 2",
    "Reva UI - Icon Pack 3",
    "Voltar"
};
static const int ASSET_COUNT = 4;
static Anim selectedAnim;

void assetsInit(void) {
    selectedAsset = 0;
    animSet(&selectedAnim, 0.0f, 0.12f);
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

    // Header
    UI_Header(buf, "Assets/Icones", "Escolha um pacote de icones");

    // Animar selected
    animTo(&selectedAnim, selectedAsset * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

    // Assets
    for (int i = 0; i < ASSET_COUNT; i++) {
        bool selected = (i == selectedAsset);
        float itemAnim = selected ? 1.0f : 0.0f;
        if (selected) {
            itemAnim = selectAnim - (int)selectAnim;
            if (itemAnim < 0) itemAnim += 1.0f;
        }
        UI_ListItem(buf, 10, 55 + i*35, 300, 30, assets[i],
                    NULL, selected, itemAnim, selected ? ">" : NULL);
    }

    // Footer
    UI_Footer(buf, "Selecionar", "Voltar", NULL);

    C2D_TextBufDelete(buf);
}
