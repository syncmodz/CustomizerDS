#include "input.h"
#include <string.h>

/* v1.0.2 §3 -- modelo simples e confiavel: 1 toque (edge cru de hidKeysDown) =
 * 1 movimento IMEDIATO; repeticao SO ao manter pressionado, depois de um delay
 * inicial. O debounce antigo (EDGE_DEBOUNCE 0.09s) foi feito pra filtrar
 * key-repeat do emulador, mas no console real ele ENGOLIA toques rapidos
 * ("tem que apertar 2x"). hidKeysDown ja e um edge limpo (1 por aperto fisico)
 * do D-pad; pro circle-pad usamos HISTERESE (engata em HI, solta em LO) pra ele
 * nao oscilar perto do limiar e virar repeticao fantasma. Tempos em segundos
 * (multiplicados por dt) porque o loop so e 60Hz garantido no hardware. */

#define REPEAT_DELAY 0.32f /* segurar antes de comecar a repetir */
#define REPEAT_GAP   0.12f /* intervalo de repeticao depois do delay */

/* §3 (PROMPT v14): no Azahar o MESMO edge de hidKeysDown chega em 2 frames
 * seguidos -> 1 toque andava 2x. Guard temporal: ignora um edge que venha a
 * menos de MIN_EDGE_GAP de um disparo anterior. ~45ms < toque humano real
 * (>100ms) e < REPEAT_GAP (120ms), entao NAO afeta o hardware (que ja e 1:1)
 * nem o hold. Substitui o antigo EDGE_DEBOUNCE de 90ms, que engolia toques
 * rapidos no console. */
#define MIN_EDGE_GAP 0.045f

/* 0=up 1=down 2=left 3=right */
static float s_repTimer[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static bool  s_repActive[4] = {false, false, false, false};
static bool  s_cpadWas[4] = {false, false, false, false};
/* tempo desde o ultimo disparo por direcao; inicia "alto" pra liberar o 1o edge. */
static float s_sinceFire[4] = {1.0f, 1.0f, 1.0f, 1.0f};

void inputResetRepeat(void) {
    for (int i = 0; i < 4; i++) {
        s_repTimer[i] = 0.0f; s_repActive[i] = false; s_sinceFire[i] = 1.0f;
    }
}

/* Edge (kDown) -> dispara na hora e arma o timer no delay; enquanto segura,
 * depois do delay, dispara a cada REPEAT_GAP. Soltar zera. O guard de
 * MIN_EDGE_GAP (s_sinceFire, acumulado por dt todo frame) descarta o edge-duplo
 * do Azahar sem reintroduzir debounce que comeria toque no hardware. */
static bool stepRepeat(int i, bool edge, bool held, float dt) {
    s_sinceFire[i] += dt;
    if (edge) {
        if (s_sinceFire[i] < MIN_EDGE_GAP) return false; /* §3: edge-duplo do Azahar */
        s_sinceFire[i] = 0.0f;
        s_repActive[i] = true; s_repTimer[i] = REPEAT_DELAY; return true;
    }
    if (held && s_repActive[i]) {
        s_repTimer[i] -= dt;
        if (s_repTimer[i] <= 0.0f) { s_repTimer[i] = REPEAT_GAP; return true; }
        return false;
    }
    if (!held) { s_repActive[i] = false; s_repTimer[i] = 0.0f; }
    return false;
}

void inputRead(AppInput* in, float dt) {
    memset(in, 0, sizeof(AppInput));
    if (dt <= 0.0f || dt > 0.25f) dt = 1.0f / 60.0f;

    hidScanInput();
    u32 rawDown = hidKeysDown();
    u32 rawHeld = hidKeysHeld();

    circlePosition cpad;
    hidCircleRead(&cpad);

    /* circle-pad com histerese: engata no limiar cheio, so solta na metade. */
    const float HI = (float)INPUT_CPAD_THRESHOLD;
    const float LO = (float)INPUT_CPAD_THRESHOLD * 0.5f;
    bool cUp = s_cpadWas[0] ? (cpad.dy >  LO) : (cpad.dy >  HI);
    bool cDn = s_cpadWas[1] ? (cpad.dy < -LO) : (cpad.dy < -HI);
    bool cLf = s_cpadWas[2] ? (cpad.dx < -LO) : (cpad.dx < -HI);
    bool cRt = s_cpadWas[3] ? (cpad.dx >  LO) : (cpad.dx >  HI);

    u32 cDownBits = 0, cHeldBits = 0;
    if (cUp && !s_cpadWas[0]) cDownBits |= KEY_CPAD_UP;
    if (cDn && !s_cpadWas[1]) cDownBits |= KEY_CPAD_DOWN;
    if (cLf && !s_cpadWas[2]) cDownBits |= KEY_CPAD_LEFT;
    if (cRt && !s_cpadWas[3]) cDownBits |= KEY_CPAD_RIGHT;
    if (cUp) cHeldBits |= KEY_CPAD_UP;
    if (cDn) cHeldBits |= KEY_CPAD_DOWN;
    if (cLf) cHeldBits |= KEY_CPAD_LEFT;
    if (cRt) cHeldBits |= KEY_CPAD_RIGHT;
    s_cpadWas[0] = cUp; s_cpadWas[1] = cDn; s_cpadWas[2] = cLf; s_cpadWas[3] = cRt;

    in->down = rawDown | cDownBits;
    in->held = rawHeld | cHeldBits;

    hidTouchRead(&in->touch);
    in->touchDown = (in->down & KEY_TOUCH) != 0;
    in->touchHeld = (in->held & KEY_TOUCH) != 0;
    in->touchPX = in->touch.px;
    in->touchPY = in->touch.py;

    bool upEdge = (in->down & (KEY_DUP | KEY_CPAD_UP)) != 0;
    bool upHeld = (in->held & (KEY_DUP | KEY_CPAD_UP)) != 0;
    bool dnEdge = (in->down & (KEY_DDOWN | KEY_CPAD_DOWN)) != 0;
    bool dnHeld = (in->held & (KEY_DDOWN | KEY_CPAD_DOWN)) != 0;
    bool lfEdge = (in->down & (KEY_DLEFT | KEY_CPAD_LEFT)) != 0;
    bool lfHeld = (in->held & (KEY_DLEFT | KEY_CPAD_LEFT)) != 0;
    bool rtEdge = (in->down & (KEY_DRIGHT | KEY_CPAD_RIGHT)) != 0;
    bool rtHeld = (in->held & (KEY_DRIGHT | KEY_CPAD_RIGHT)) != 0;

    in->up      = stepRepeat(0, upEdge, upHeld, dt);
    in->downNav = stepRepeat(1, dnEdge, dnHeld, dt);
    in->left    = stepRepeat(2, lfEdge, lfHeld, dt);
    in->right   = stepRepeat(3, rtEdge, rtHeld, dt);

    in->confirm = (in->down & KEY_A) != 0;
    in->back    = (in->down & KEY_B) != 0;
    in->start   = (in->down & KEY_START) != 0;
    in->debug   = (in->down & KEY_SELECT) != 0;
}
