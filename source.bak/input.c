#include "input.h"
#include <string.h>

static bool s_cpadUpWas = false;
static bool s_cpadDownWas = false;
static bool s_cpadLeftWas = false;
static bool s_cpadRightWas = false;
static int s_navCooldown = 0;
static int s_lrCooldown = 0;

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
    in->down = rawDown | cpadDownBits;
    in->held = rawHeld | cpadHeldBits;

    hidTouchRead(&in->touch);
    in->touchDown = (in->down & KEY_TOUCH) != 0;
    in->touchHeld = (in->held & KEY_TOUCH) != 0;
    in->touchPX = in->touch.px;
    in->touchPY = in->touch.py;

    bool navUp = (in->down & (KEY_DUP | KEY_CPAD_UP)) != 0;
    bool navDown = (in->down & (KEY_DDOWN | KEY_CPAD_DOWN)) != 0;
    bool navHeld = (in->held & (KEY_DUP | KEY_DDOWN | KEY_CPAD_UP | KEY_CPAD_DOWN)) != 0;
    if (!navHeld) {
        s_navCooldown = 0;
    } else if (s_navCooldown > 0) {
        s_navCooldown--;
        navUp = false;
        navDown = false;
    } else if (navUp || navDown) {
        s_navCooldown = 7;
    }

    in->up = navUp;
    in->downNav = navDown;

    /* mesma logica de debounce/repeat do up/down, agora aplicada a esquerda/direita.
     * Antes: in->left/right vinham direto de 'held', disparando a cada frame (60x/seg)
     * enquanto o botao ficasse pressionado -- isso fazia accent color, sliders de LED,
     * canal R/G/B e o editor de hex avancarem dezenas de passos por segundo de forma
     * descontrolada, e cada passo podia disparar save em disco (ver led.c/darkmode.c),
     * o que travava o frame. Resultado visivel: "stutter" generalizado em qualquer tela
     * com setas. Agora dispara uma vez, espera, e repete a um ritmo controlado. */
    bool lrHeld = (in->held & (KEY_DLEFT | KEY_DRIGHT | KEY_CPAD_LEFT | KEY_CPAD_RIGHT)) != 0;
    bool lrLeftNow = (in->down & (KEY_DLEFT | KEY_CPAD_LEFT)) != 0;
    bool lrRightNow = (in->down & (KEY_DRIGHT | KEY_CPAD_RIGHT)) != 0;
    bool leftOut, rightOut;
    if (!lrHeld) {
        s_lrCooldown = 0;
        leftOut = false;
        rightOut = false;
    } else if (s_lrCooldown > 0) {
        s_lrCooldown--;
        leftOut = false;
        rightOut = false;
    } else {
        leftOut = (in->held & (KEY_DLEFT | KEY_CPAD_LEFT)) != 0;
        rightOut = (in->held & (KEY_DRIGHT | KEY_CPAD_RIGHT)) != 0;
        if (leftOut || rightOut) s_lrCooldown = 7;
    }
    /* primeiro toque sempre dispara imediato, mesmo sem ainda ter passado pelo cooldown */
    if (lrLeftNow) { leftOut = true; }
    if (lrRightNow) { rightOut = true; }

    in->left = leftOut;
    in->right = rightOut;
    in->confirm = (in->down & KEY_A) != 0;
    in->back = (in->down & KEY_B) != 0;
    in->start = (in->down & KEY_START) != 0;
    in->debug = (in->down & KEY_SELECT) != 0;
}
