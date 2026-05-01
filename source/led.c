#include "led.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include <math.h>
#include <string.h>

static uint8_t ledR = 255, ledG = 0, ledB = 0;
static float hue = 0.0f;
static int selectedMode = 0;
static const char* modes[] = {
    "Arco-iris (Automático)",
    "Vermelho Fixo",
    "Verde Fixo",
    "Azul Fixo",
    "Amarelo Fixo",
    "Roxo Fixo",
    "Voltar"
};
static const int MODE_COUNT = 7;
static Anim selectedAnim;
static Ripple ledRipple;

void ledInit(void) {
    ledR = 255; ledG = 0; ledB = 0;
    hue = 0.0f;
    selectedMode = 0;
    animSet(&selectedAnim, 0.0f, 0.12f);
    ledRipple.active = false;
}

void updateLEDColor(void) {
    switch (selectedMode) {
        case 0: // Rainbow
            hue += 1.0f;
            if (hue >= 360.0f) hue -= 360.0f;
            {
                float h = hue / 60.0f;
                int i = (int)h;
                float f = h - i;
                float q = 1.0f - f;
                float t = f;
                switch (i) {
                    case 0: ledR = 255; ledG = (uint8_t)(t * 255); ledB = 0; break;
                    case 1: ledR = (uint8_t)(q * 255); ledG = 255; ledB = 0; break;
                    case 2: ledR = 0; ledG = 255; ledB = (uint8_t)(t * 255); break;
                    case 3: ledR = 0; ledG = (uint8_t)(q * 255); ledB = 255; break;
                    case 4: ledR = (uint8_t)(t * 255); ledG = 0; ledB = 255; break;
                    case 5: ledR = 255; ledG = 0; ledB = (uint8_t)(q * 255); break;
                }
            }
            break;
        case 1: ledR = 255; ledG = 0; ledB = 0; break;
        case 2: ledR = 0; ledG = 255; ledB = 0; break;
        case 3: ledR = 0; ledG = 0; ledB = 255; break;
        case 4: ledR = 255; ledG = 255; ledB = 0; break;
        case 5: ledR = 128; ledG = 0; ledB = 128; break;
    }
}

void ledRender(u32 kDown, u32 kHeld, int* currentScreen) {
    if (kDown & KEY_B) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (kDown & KEY_DOWN) {
        selectedMode = (selectedMode + 1) % MODE_COUNT;
    }
    if (kDown & KEY_UP) {
        selectedMode = (selectedMode - 1 + MODE_COUNT) % MODE_COUNT;
    }
    if (kDown & KEY_A && selectedMode == MODE_COUNT - 1) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    updateLEDColor();

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) {
        return;
    }

    // Header
    UI_Header(buf, "LED RGB", "Personalizar cor do LED");

    // Preview do LED com ripple
    rippleStep(&ledRipple);
    u32 ledColor = C2D_Color32(ledR, ledG, ledB, 255);
    UI_Card(50, 50, 100, 100, true, 0.0f);
    C2D_DrawRectSolid(60, 60, 0, 80, 80, ledColor);
    rippleDraw(&ledRipple, ledColor);

    // Paleta de cores (mini retângulos)
    C2D_DrawRectSolid(170, 60, 0, 20, 20, C2D_Color32(255,0,0,255));
    C2D_DrawRectSolid(195, 60, 0, 20, 20, C2D_Color32(0,255,0,255));
    C2D_DrawRectSolid(220, 60, 0, 20, 20, C2D_Color32(0,0,255,255));
    C2D_DrawRectSolid(170, 85, 0, 20, 20, C2D_Color32(255,255,0,255));
    C2D_DrawRectSolid(195, 85, 0, 20, 20, C2D_Color32(128,0,128,255));
    C2D_DrawRectSolid(220, 85, 0, 20, 20, ledColor);

    // Animar selected
    animTo(&selectedAnim, selectedMode * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

    // Modos
    for (int i = 0; i < MODE_COUNT; i++) {
        bool selected = (i == selectedMode);
        float itemAnim = selected ? 1.0f : 0.0f;
        if (selected) {
            itemAnim = selectAnim - (int)selectAnim;
            if (itemAnim < 0) itemAnim += 1.0f;
        }
        UI_ListItem(buf, 10, 160 + i*28, 300, 25, modes[i],
                    NULL, selected, itemAnim, NULL);
    }

    // Footer
    UI_Footer(buf, "Selecionar", "Voltar", NULL);

    C2D_TextBufDelete(buf);
}

void ledDrawPreview(void) {
    C2D_DrawRectSolid(300, 20, 0, 80, 80, C2D_Color32(ledR, ledG, ledB, 255));
}
