#include "darkmode.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include <string.h>

static int darkModeEnabled = 0;
static int selectedOption = 0;
static const char* options[] = {
    "Ativar Modo Escuro",
    "Desativar Modo Escuro",
    "Voltar"
};
static const int OPTION_COUNT = 3;
static UISwitch darkSwitch;
static Anim selectedAnim;

void darkmodeInit(void) {
    selectedOption = 0;
    uiSwitchInit(&darkSwitch, darkModeEnabled);
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
            darkModeEnabled = 1;
            uiSwitchToggle(&darkSwitch);
        } else if (selectedOption == 1) {
            darkModeEnabled = 0;
            uiSwitchToggle(&darkSwitch);
        } else {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        }
    }

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) {
        return;
    }

    // Header
    UI_Header(buf, "Modo Escuro", "Alternar tema escuro");

    // Status com UISwitch
    uiSwitchStep(&darkSwitch);
    C2D_Text text;
    C2D_TextParse(&text, buf, "Status:");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, 0.3f, 20.0f, 55.0f, 0.3f, 0.3f, g_theme.textPrimary);
    uiSwitchDraw(&darkSwitch, 120, 50);

    // Animar selected
    animTo(&selectedAnim, selectedOption * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

    // Opções
    for (int i = 0; i < OPTION_COUNT; i++) {
        bool selected = (i == selectedOption);
        float itemAnim = selected ? 1.0f : 0.0f;
        if (selected) {
            itemAnim = selectAnim - (int)selectAnim;
            if (itemAnim < 0) itemAnim += 1.0f;
        }
        UI_ListItem(buf, 10, 90 + i*40, 300, 35, options[i],
                    NULL, selected, itemAnim, NULL);
    }

    // Footer
    UI_Footer(buf, "Selecionar", "Voltar", NULL);

    C2D_TextBufDelete(buf);
}
