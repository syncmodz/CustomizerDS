#include "led.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <3ds/services/mcuhwc.h>

static u8 curR, curG, curB;
static int curSpeed;
static int selectedItem;
static bool editMode;
static Anim selectedAnim;
static Ripple ledRipple;

typedef enum {
    MODE_RAINBOW,
    MODE_SOLID,
    MODE_OFF,
    MODE_COUNT
} LedMode;

static int currentMode;
static ConfigData ledCfg;

static const char* modeLabels[] = {
    "Arco-iris",
    "Cor Solida",
    "Desligado",
    "Voltar"
};

void ledInit(void) {
    mcuHwcInit();
    configLoad(&ledCfg);
    currentMode = ledCfg.ledMode;
    curR = ledCfg.ledR;
    curG = ledCfg.ledG;
    curB = ledCfg.ledB;
    curSpeed = ledCfg.ledSpeed;
    selectedItem = 0;
    editMode = false;
    animSet(&selectedAnim, 0.0f, 0.12f);
    ledRipple.active = false;
}

static void setLEDColor(u8 r, u8 g, u8 b) {
    InfoLedPattern pattern;
    memset(&pattern, 0, sizeof(pattern));
    pattern.delay = 0x10;
    pattern.smoothing = 0x00;
    pattern.loopDelay = 0xFF;
    pattern.blinkSpeed = 0x00;
    memset(pattern.redPattern, r, 32);
    memset(pattern.greenPattern, g, 32);
    memset(pattern.bluePattern, b, 32);
    MCUHWC_SetInfoLedPattern(&pattern);
}

static void setLEDRainbow(void) {
    InfoLedPattern pattern;
    memset(&pattern, 0, sizeof(pattern));
    pattern.delay = 0x10;
    pattern.smoothing = 0x00;
    pattern.loopDelay = 0x00;
    pattern.blinkSpeed = 0x00;
    for (int i = 0; i < 32; i++) {
        float h = (i / 32.0f) * 360.0f;
        float s = 1.0f, v = 1.0f;
        float c = v * s;
        float x = c * (1.0f - fabs(fmodf(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        float r1, g1, b1;
        if (h < 60)       { r1 = c; g1 = x; b1 = 0; }
        else if (h < 120)  { r1 = x; g1 = c; b1 = 0; }
        else if (h < 180)  { r1 = 0; g1 = c; b1 = x; }
        else if (h < 240)  { r1 = 0; g1 = x; b1 = c; }
        else if (h < 300)  { r1 = x; g1 = 0; b1 = c; }
        else                { r1 = c; g1 = 0; b1 = x; }
        pattern.redPattern[i]   = (u8)((r1 + m) * 255);
        pattern.greenPattern[i] = (u8)((g1 + m) * 255);
        pattern.bluePattern[i]  = (u8)((b1 + m) * 255);
    }
    MCUHWC_SetInfoLedPattern(&pattern);
}

static void setLEDOff(void) {
    setLEDColor(0, 0, 0);
}

static void applyLED(void) {
    switch (currentMode) {
        case MODE_RAINBOW: setLEDRainbow(); break;
        case MODE_SOLID:   setLEDColor(curR, curG, curB); break;
        case MODE_OFF:     setLEDOff(); break;
    }
}

static void saveConfig(void) {
    ledCfg.ledMode = currentMode;
    ledCfg.ledR = curR;
    ledCfg.ledG = curG;
    ledCfg.ledB = curB;
    ledCfg.ledSpeed = curSpeed;
    configSave(&ledCfg);
}

static int getItemCount(void) {
    if (currentMode == MODE_RAINBOW) return 4;
    if (currentMode == MODE_SOLID) return 6;
    return 4;
}

static const char* getItemLabel(int i) {
    int baseCount = 4;
    if (i < baseCount) return modeLabels[i];
    if (currentMode == MODE_RAINBOW) {
        if (i == baseCount) return "Velocidade";
    }
    if (currentMode == MODE_SOLID) {
        if (i == baseCount + 0) return "Vermelho";
        if (i == baseCount + 1) return "Verde";
        if (i == baseCount + 2) return "Azul";
    }
    return "";
}

static const char* getItemRight(int i) {
    int baseCount = 4;
    if (i < baseCount) return NULL;
    if (currentMode == MODE_RAINBOW && i == baseCount) {
        static char s[16];
        snprintf(s, 16, "%d", curSpeed);
        return s;
    }
    if (currentMode == MODE_SOLID) {
        static char s[16];
        if (i == baseCount + 0) { snprintf(s, 16, "%d", curR); return s; }
        if (i == baseCount + 1) { snprintf(s, 16, "%d", curG); return s; }
        if (i == baseCount + 2) { snprintf(s, 16, "%d", curB); return s; }
    }
    return NULL;
}

void ledRender(u32 kDown, u32 kHeld, int* currentScreen) {
    if (kDown & KEY_B) {
        setLEDColor(255, 255, 255);
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    int itemCount = getItemCount();

    if (!editMode) {
        if (kDown & KEY_DOWN) selectedItem = (selectedItem + 1) % itemCount;
        if (kDown & KEY_UP) selectedItem = (selectedItem - 1 + itemCount) % itemCount;
    }

    if (kDown & KEY_A) {
        if (selectedItem >= 4) {
            editMode = !editMode;
        } else if (selectedItem != 3) {
            currentMode = selectedItem;
            applyLED();
            saveConfig();
        } else {
            setLEDColor(255, 255, 255);
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        }
    }

    if (editMode) {
        if (currentMode == MODE_RAINBOW && selectedItem == 4) {
            if (kDown & KEY_RIGHT) { if (curSpeed < 5) curSpeed++; applyLED(); saveConfig(); }
            if (kDown & KEY_LEFT)  { if (curSpeed > 0) curSpeed--; applyLED(); saveConfig(); }
        }
        if (currentMode == MODE_SOLID) {
            if (selectedItem == 4) {
                if (kDown & KEY_RIGHT) { if (curR < 255) curR += 15; if (curR > 255) curR = 255; applyLED(); saveConfig(); }
                if (kDown & KEY_LEFT)  { if (curR > 15) curR -= 15; else curR = 0; applyLED(); saveConfig(); }
            }
            if (selectedItem == 5) {
                if (kDown & KEY_RIGHT) { if (curG < 255) curG += 15; if (curG > 255) curG = 255; applyLED(); saveConfig(); }
                if (kDown & KEY_LEFT)  { if (curG > 15) curG -= 15; else curG = 0; applyLED(); saveConfig(); }
            }
            if (selectedItem == 6) {
                if (kDown & KEY_RIGHT) { if (curB < 255) curB += 15; if (curB > 255) curB = 255; applyLED(); saveConfig(); }
                if (kDown & KEY_LEFT)  { if (curB > 15) curB -= 15; else curB = 0; applyLED(); saveConfig(); }
            }
        }
        if (kDown & KEY_A) editMode = false;
    }

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) return;

    UI_Header(buf, "LED RGB", "Personalizar cor do LED");

    u32 ledColor = C2D_Color32(curR, curG, curB, 255);
    UI_Card(50, 50, 100, 100, true, 0.0f);
    C2D_DrawRectSolid(60, 60, 0, 80, 80, ledColor);

    C2D_DrawRectSolid(170, 60, 0, 20, 20, C2D_Color32(255,0,0,255));
    C2D_DrawRectSolid(195, 60, 0, 20, 20, C2D_Color32(0,255,0,255));
    C2D_DrawRectSolid(220, 60, 0, 20, 20, C2D_Color32(0,0,255,255));
    C2D_DrawRectSolid(170, 85, 0, 20, 20, C2D_Color32(255,255,0,255));
    C2D_DrawRectSolid(195, 85, 0, 20, 20, C2D_Color32(128,0,128,255));
    C2D_DrawRectSolid(220, 85, 0, 20, 20, C2D_Color32(curR,curG,curB,255));

    animTo(&selectedAnim, selectedItem * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

    for (int i = 0; i < itemCount; i++) {
        bool selected = (i == selectedItem);
        float itemAnim = selected ? 1.0f : 0.0f;
        if (selected) {
            itemAnim = selectAnim - (int)selectAnim;
            if (itemAnim < 0) itemAnim += 1.0f;
        }
        UI_ListItem(buf, 10, 160 + i * 28, 300, 25,
                    getItemLabel(i),
                    NULL, selected, itemAnim,
                    getItemRight(i));
    }

    UI_Footer(buf, editMode ? "Parar" : "Selecionar", "Voltar", NULL);
    C2D_TextBufDelete(buf);
}

void ledDrawPreview(void) {
    C2D_DrawRectSolid(300, 20, 0, 80, 80, C2D_Color32(curR, curG, curB, 255));
}
