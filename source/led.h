#pragma once
#include "common.h"
#include "menu.h"

typedef enum {
    LED_MODE_OFF, LED_MODE_SOLID, LED_MODE_BREATHE, LED_MODE_RAINBOW, LED_MODE_COUNT
} LEDMode;

typedef struct {
    MenuState* menu;
    LEDMode mode;
    int brightness, speed, selMode;
    u32 color;
    AnimState modeAnim[4];
} LEDState;

void ledInit(LEDState* ls, MenuState* menu);
void ledUpdate(LEDState* ls, InputState* in);
void ledDraw(LEDState* ls);
void ledDrawTop(LEDState* ls);
