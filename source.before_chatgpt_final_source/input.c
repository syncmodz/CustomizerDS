#include "input.h"
#include <string.h>

static circlePosition s_prevCpad = {0, 0};
static bool s_cpadUpWas = false;
static bool s_cpadDownWas = false;
static bool s_cpadLeftWas = false;
static bool s_cpadRightWas = false;

void inputRead(AppInput* in) {
    memset(in, 0, sizeof(AppInput));

    hidScanInput();

    u32 rawDown = hidKeysDown();
    u32 rawHeld = hidKeysHeld();

    circlePosition cpad;
    hidCircleRead(&cpad);

    u32 cpadDownBits = 0;
    u32 cpadHeldBits = 0;

    bool cpadUp = cpad.dy > INPUT_CPAD_THRESHOLD;
    bool cpadDown = cpad.dy < -INPUT_CPAD_THRESHOLD;
    bool cpadLeft = cpad.dx < -INPUT_CPAD_THRESHOLD;
    bool cpadRight = cpad.dx > INPUT_CPAD_THRESHOLD;

    if (cpadUp && !s_cpadUpWas) cpadDownBits |= KEY_CPAD_UP;
    if (cpadDown && !s_cpadDownWas) cpadDownBits |= KEY_CPAD_DOWN;
    if (cpadLeft && !s_cpadLeftWas) cpadDownBits |= KEY_CPAD_LEFT;
    if (cpadRight && !s_cpadRightWas) cpadDownBits |= KEY_CPAD_RIGHT;

    if (cpadUp) cpadHeldBits |= KEY_CPAD_UP;
    if (cpadDown) cpadHeldBits |= KEY_CPAD_DOWN;
    if (cpadLeft) cpadHeldBits |= KEY_CPAD_LEFT;
    if (cpadRight) cpadHeldBits |= KEY_CPAD_RIGHT;

    s_cpadUpWas = cpadUp;
    s_cpadDownWas = cpadDown;
    s_cpadLeftWas = cpadLeft;
    s_cpadRightWas = cpadRight;
    s_prevCpad = cpad;

    in->down = rawDown | cpadDownBits;
    in->held = rawHeld | cpadHeldBits;

    hidTouchRead(&in->touch);
    in->touchDown = (in->down & KEY_TOUCH) != 0;
    in->touchHeld = (in->held & KEY_TOUCH) != 0;
    in->touchPX = in->touch.px;
    in->touchPY = in->touch.py;

    in->up = (in->down & KEY_DUP) != 0 || cpadDown;
    in->downNav = (in->down & KEY_DDOWN) != 0 || cpadUp;
    in->left = (in->down & KEY_DLEFT) != 0 || cpadLeft;
    in->right = (in->down & KEY_DRIGHT) != 0 || cpadRight;
    in->confirm = (in->down & KEY_A) != 0;
    in->back = (in->down & KEY_B) != 0;
    in->start = (in->down & KEY_START) != 0;
    in->debug = (in->down & KEY_SELECT) != 0;
}
