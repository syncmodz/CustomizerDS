#include "led.h"
#include "common.h"
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

void ledInit(void) {
    ledR = 255; ledG = 0; ledB = 0;
    hue = 0.0f;
    selectedMode = 0;
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
    C2D_Text text;
    C2D_TextParse(&text, buf, "Modulo LED RGB");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, 0.4f, 10.0f, 10.0f, 0.4f, 0.4f, C2D_Color32(200, 200, 50, 255));

    C2D_DrawRectSolid(50, 40, 0, 100, 100, C2D_Color32(ledR, ledG, ledB, 255));
    C2D_DrawRectSolid(160, 40, 0, 50, 100, C2D_Color32(ledR, ledG, ledB, 255));

    for (int i = 0; i < MODE_COUNT; i++) {
        u32 color = (i == selectedMode) ? C2D_Color32(60, 100, 150, 255) : C2D_Color32(40, 40, 50, 255);
        C2D_DrawRectSolid(10, 150 + i*20, 0, 300, 18, color);

        C2D_TextParse(&text, buf, modes[i]);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, 0.3f, 20.0f, 152 + i*20, 0.25f, 0.25f,
            (i == selectedMode) ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(200, 200, 200, 255));
    }

    C2D_TextBufDelete(buf);
}

void ledDrawPreview(void) {
    C2D_DrawRectSolid(300, 20, 0, 80, 80, C2D_Color32(ledR, ledG, ledB, 255));
}
