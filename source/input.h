#pragma once
#include "common.h"

typedef struct {
    u32 down, held, up;
    bool left, right, upDir, downDir;
    bool confirm, cancel, a, b, xIn, yIn, start, select;
    bool touchDown, touchHeld, touchUp;
    touchPosition touch;
    int cpadX, cpadY;
    bool anyPress;
} InputState;

void inputPoll(InputState* in);
