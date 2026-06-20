#include "input.h"

static int s_lrState = 0;
static int s_lrTimer = 0;
static bool s_lrLeftFired = false;
static bool s_lrRightFired = false;

#define KEY_CPAD_LEFT  (1 << 28)
#define KEY_CPAD_RIGHT (1 << 29)
#define KEY_CPAD_UP    (1 << 30)
#define KEY_CPAD_DOWN  (1 << 31)

void inputPoll(InputState* in) {
    memset(in, 0, sizeof(InputState));
    hidScanInput();

    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    u32 kUp = hidKeysUp();

    circlePosition cpad;
    hidCircleRead(&cpad);

    u32 cpadDown = 0, cpadHeld = 0;
    if (cpad.dx < -30) { cpadDown |= KEY_CPAD_LEFT; cpadHeld |= KEY_CPAD_LEFT; }
    if (cpad.dx > 30)  { cpadDown |= KEY_CPAD_RIGHT; cpadHeld |= KEY_CPAD_RIGHT; }
    if (cpad.dy < -30) { cpadDown |= KEY_CPAD_UP; cpadHeld |= KEY_CPAD_UP; }
    if (cpad.dy > 30)  { cpadDown |= KEY_CPAD_DOWN; cpadHeld |= KEY_CPAD_DOWN; }

    u32 allDown = kDown | cpadDown;
    u32 allHeld = kHeld | cpadHeld;

    in->down = allDown;
    in->held = allHeld;
    in->up = kUp;

    touchPosition tp;
    hidTouchRead(&tp);
    in->touch = tp;
    in->touchDown = (allDown & KEY_TOUCH) != 0;
    in->touchHeld = (allHeld & KEY_TOUCH) != 0;
    in->touchUp = (kUp & KEY_TOUCH) != 0;

    in->confirm = (allDown & KEY_A) != 0;
    in->cancel  = (allDown & KEY_B) != 0;
    in->a = (allDown & KEY_A) != 0;
    in->b = (allDown & KEY_B) != 0;
    in->xIn = (allDown & KEY_X) != 0;
    in->yIn = (allDown & KEY_Y) != 0;
    in->start = (allDown & KEY_START) != 0;
    in->select = (allDown & KEY_SELECT) != 0;

    in->cpadX = cpad.dx;
    in->cpadY = cpad.dy;

    in->anyPress = (allDown & (KEY_A|KEY_B|KEY_X|KEY_Y|KEY_START|KEY_SELECT|KEY_TOUCH|
                  KEY_CPAD_LEFT|KEY_CPAD_RIGHT|KEY_CPAD_UP|KEY_CPAD_DOWN|
                  KEY_DLEFT|KEY_DRIGHT|KEY_DUP|KEY_DDOWN)) != 0;

    bool lLD = (allDown & (KEY_DLEFT|KEY_CPAD_LEFT)) != 0;
    bool lRD = (allDown & (KEY_DRIGHT|KEY_CPAD_RIGHT)) != 0;
    bool lUD = (allDown & (KEY_DUP|KEY_CPAD_UP)) != 0;
    bool lDD = (allDown & (KEY_DDOWN|KEY_CPAD_DOWN)) != 0;
    bool lLH = (allHeld & (KEY_DLEFT|KEY_CPAD_LEFT)) != 0;
    bool lRH = (allHeld & (KEY_DRIGHT|KEY_CPAD_RIGHT)) != 0;
    bool lAnyH = lLH || lRH;

    bool lOut = false, rOut = false;

    if (lLD && !s_lrLeftFired) { lOut = true; s_lrLeftFired = true; }
    if (lRD && !s_lrRightFired) { rOut = true; s_lrRightFired = true; }
    if (!lLH) s_lrLeftFired = false;
    if (!lRH) s_lrRightFired = false;

    if (!lAnyH) { s_lrState = 0; s_lrTimer = 0; }
    else {
        if (s_lrState == 0) { s_lrState = 1; s_lrTimer = 18; }
        if (s_lrState == 1) {
            s_lrTimer--;
            if (s_lrTimer <= 0) { s_lrState = 2; s_lrTimer = 0; }
        }
        if (s_lrState == 2) {
            s_lrTimer--;
            if (s_lrTimer <= 0) { lOut = lLH; rOut = lRH; s_lrTimer = 8; }
        }
    }

    in->left = lOut;
    in->right = rOut;
    in->upDir = lUD;
    in->downDir = lDD;
}
