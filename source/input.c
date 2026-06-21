#include "input.h"
#include <string.h>

/* Tudo aqui e medido em tempo real (segundos), nao em "frames", porque o loop
 * principal so e garantido a 60Hz em hardware real. Em emulador (Azahar no PC)
 * o vsync pode estar atado ao monitor do host (ex: 144Hz), entao um contador
 * "decrementa 1 por chamada" dispara ~2-3x mais rapido que o pretendido e um
 * unico toque vira repeticao fantasma. Por isso os limiares abaixo sao em
 * segundos e multiplicados por dt, nunca por "1 frame". */

#define NAV_REPEAT_GAP    0.12f   /* intervalo minimo entre repeticoes de up/down ao manter pressionado */
#define LR_INITIAL_DELAY  0.30f   /* tempo mantendo esq/dir antes de comecar a repetir */
#define LR_REPEAT_INTERVAL 0.1333f /* intervalo de repeticao depois do delay inicial */
#define EDGE_DEBOUNCE     0.09f   /* tempo minimo entre dois "toques" aceitos do mesmo botao -
                                     filtra key-repeat duplicado do host/emulador num toque so */

static bool s_cpadUpWas = false;
static bool s_cpadDownWas = false;
static bool s_cpadLeftWas = false;
static bool s_cpadRightWas = false;

static float s_navCooldown = 0.0f;
static float s_lrTimer = 0.0f;
static int s_lrState = 0;

static float s_lastNavUpEdge = -1000.0f;
static float s_lastNavDownEdge = -1000.0f;
static float s_lastLrLeftEdge = -1000.0f;
static float s_lastLrRightEdge = -1000.0f;
static float s_clock = 0.0f;

static bool debounceEdge(bool rawEdge, float* lastAcceptedAt) {
    if (!rawEdge) return false;
    if (s_clock - *lastAcceptedAt < EDGE_DEBOUNCE) return false;
    *lastAcceptedAt = s_clock;
    return true;
}

/* Chamado ao trocar de tela: sem isso, um repeat de esq/dir ou cima/baixo que
 * estava "no meio" da tela anterior (ex: usuario soltou o botao bem na hora
 * da troca) deixa o estado vazando para a tela nova e o primeiro toque la
 * pode nao disparar ou disparar fora de hora. */
void inputResetRepeat(void) {
    s_navCooldown = 0.0f;
    s_lrTimer = 0.0f;
    s_lrState = 0;
}

void inputRead(AppInput* in, float dt) {
    memset(in, 0, sizeof(AppInput));
    if (dt <= 0.0f || dt > 0.25f) dt = 1.0f / 60.0f;
    s_clock += dt;

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

    bool navUpEdge = (in->down & (KEY_DUP | KEY_CPAD_UP)) != 0;
    bool navDownEdge = (in->down & (KEY_DDOWN | KEY_CPAD_DOWN)) != 0;
    bool navHeld = (in->held & (KEY_DUP | KEY_DDOWN | KEY_CPAD_UP | KEY_CPAD_DOWN)) != 0;

    bool navUp = debounceEdge(navUpEdge, &s_lastNavUpEdge);
    bool navDown = debounceEdge(navDownEdge, &s_lastNavDownEdge);

    if (!navHeld) {
        s_navCooldown = 0.0f;
    } else if (s_navCooldown > 0.0f) {
        s_navCooldown -= dt;
        navUp = false;
        navDown = false;
    } else if (navUp || navDown) {
        s_navCooldown = NAV_REPEAT_GAP;
    }

    in->up = navUp;
    in->downNav = navDown;

    bool lrLeftEdge = (in->down & (KEY_DLEFT | KEY_CPAD_LEFT)) != 0;
    bool lrRightEdge = (in->down & (KEY_DRIGHT | KEY_CPAD_RIGHT)) != 0;
    bool lrLeftHeld = (in->held & (KEY_DLEFT | KEY_CPAD_LEFT)) != 0;
    bool lrRightHeld = (in->held & (KEY_DRIGHT | KEY_CPAD_RIGHT)) != 0;
    bool lrAnyHeld = lrLeftHeld || lrRightHeld;

    bool lrLeftDown = debounceEdge(lrLeftEdge, &s_lastLrLeftEdge);
    bool lrRightDown = debounceEdge(lrRightEdge, &s_lastLrRightEdge);

    bool leftOut = lrLeftDown;
    bool rightOut = lrRightDown;

    if (!lrAnyHeld) {
        s_lrState = 0;
        s_lrTimer = 0.0f;
    } else {
        if (s_lrState == 0) {
            s_lrState = 1;
            s_lrTimer = LR_INITIAL_DELAY;
        }

        if (s_lrState == 1) {
            s_lrTimer -= dt;
            if (s_lrTimer <= 0.0f) {
                s_lrState = 2;
                s_lrTimer = 0.0f;
            }
        }

        if (s_lrState == 2) {
            s_lrTimer -= dt;
            if (s_lrTimer <= 0.0f) {
                leftOut = lrLeftHeld;
                rightOut = lrRightHeld;
                s_lrTimer = LR_REPEAT_INTERVAL;
            }
        }
    }

    in->left = leftOut;
    in->right = rightOut;
    in->confirm = (in->down & KEY_A) != 0;
    in->back = (in->down & KEY_B) != 0;
    in->start = (in->down & KEY_START) != 0;
    in->debug = (in->down & KEY_SELECT) != 0;
}
