#include "darkmode.h"
#include "common.h"
#include "theme.h"
#include "config.h"
#include "ui.h"
#include <string.h>

static int selectedOption = 0;
static const char* options[] = {
    "Tema Claro",
    "Tema Escuro",
    "Voltar"
};
static const int OPTION_COUNT = 3;
static Anim selectedAnim;

void darkmodeInit(void) {
    selectedOption = themeIsDark() ? 1 : 0;
    animSet(&selectedAnim, 0.0f, 0.12f);
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
            themeSetDark(false);
            ConfigData cfg;
            configLoad(&cfg);
            cfg.darkMode = 0;
            configSave(&cfg);
        } else if (selectedOption == 1) {
            themeSetDark(true);
            ConfigData cfg;
            configLoad(&cfg);
            cfg.darkMode = 1;
            configSave(&cfg);
        } else {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        }
    }

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) return;

    UI_Header(buf, "Tema", themeIsDark() ? "Modo Escuro" : "Modo Claro");

    animTo(&selectedAnim, selectedOption * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

    for (int i = 0; i < OPTION_COUNT; i++) {
        bool selected = (i == selectedOption);
        float itemAnim = selected ? 1.0f : 0.0f;
        if (selected) {
            itemAnim = selectAnim - (int)selectAnim;
            if (itemAnim < 0) itemAnim += 1.0f;
        }
        const char* indicator = NULL;
        if (i == 0 && !themeIsDark()) indicator = "OK";
        if (i == 1 && themeIsDark()) indicator = "OK";
        UI_ListItem(buf, 10, 90 + i * 40, 300, 35, options[i],
                    NULL, selected, itemAnim, indicator);
    }

    UI_Footer(buf, "Selecionar", "Voltar", NULL);
    C2D_TextBufDelete(buf);
}
