#include "assets.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include <string.h>
#include <stdio.h>

static int selectedAsset = 0;
static const char* assets[] = {
    "Reva UI - Icon Pack 1",
    "Reva UI - Icon Pack 2",
    "Reva UI - Icon Pack 3",
    "Corrigir Anemone",
    "Voltar"
};
static const int ASSET_COUNT = 5;
static Anim selectedAnim;

// Variáveis para mensagem de fix Anemone
static bool showFixMessage = false;
static float fixMessageTimer = 0.0f;
static const float FIX_MESSAGE_DURATION = 3.0f;

static void fixAnemone() {
    FILE* f = fopen("/mnt/sd/FIX_ANEMONE.txt", "w");
    if (!f) return;
    fprintf(f, "Para corrigir o Anemone sem perder seus temas:\n");
    fprintf(f, "1. No computador, acesse o SD card\n");
    fprintf(f, "2. DELETE a pasta: /3ds/Anemone3DS/ (só essa pasta, não /Themes/)\n");
    fprintf(f, "3. Baixe o Anemone3DS.cia mais recente do GitHub:\n");
    fprintf(f, "   https://github.com/astronautlevel2/Anemone3DS/releases\n");
    fprintf(f, "4. Coloque o .cia na pasta /cias/ do SD\n");
    fprintf(f, "5. Instale via FBI\n");
    fprintf(f, "Seus temas em /Themes/ estão seguros e intactos.\n");
    fclose(f);
}

bool assetsShowFixMessage(void) {
    return showFixMessage;
}

void assetsInit(void) {
    selectedAsset = 0;
    animSet(&selectedAnim, 0.0f, 0.12f);
    showFixMessage = false;
    fixMessageTimer = 0.0f;
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
    if (kDown & KEY_A) {
        if (selectedAsset == ASSET_COUNT - 1) {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        } else if (selectedAsset == 3) {
            fixAnemone();
            showFixMessage = true;
            fixMessageTimer = FIX_MESSAGE_DURATION;
        }
    }

    if (showFixMessage) {
        fixMessageTimer -= 1.0f / 60.0f;
        if (fixMessageTimer <= 0.0f) showFixMessage = false;
    }

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) return;

    UI_Header(buf, "Assets/Icones", "Escolha um pacote de icones");

    animTo(&selectedAnim, selectedAsset * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

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

    UI_Footer(buf, "Selecionar", "Voltar", NULL);

    C2D_TextBufDelete(buf);
}
