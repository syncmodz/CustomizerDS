#pragma once
#include "common.h"
#include "input.h"

typedef struct {
    int x, y, w, h;
    u32 color;
    char hex[8];
    int cursor;
    bool active;
} ColorPicker;

void cpInit(ColorPicker* cp, int x, int y, int w, int h, u32 init);
void cpUpdate(ColorPicker* cp, InputState* in);
void cpDraw(ColorPicker* cp);
