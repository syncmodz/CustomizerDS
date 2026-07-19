#include "colorwheel.h"
#include "ui.h"
#include "theme.h"
#include "common.h"
#include "anim.h"
#include <math.h>
#include <stdio.h>

/* ---- geometria (tela de baixo 320x240) ---- */
#define CX      104.0f
#define CY      122.0f
#define R_OUT    84.0f
#define R_IN     64.0f
#define R_TRI    58.0f
#define RING_SEG 96

#define DEG2RAD (3.14159265f / 180.0f)

/* ---- HSV <-> RGB ---- */
static void hsv2rgb(float h, float s, float v, u8* R, u8* G, u8* B) {
    h = fmodf(h, 360.0f);
    if (h < 0.0f) h += 360.0f;
    s = fmaxf(0.0f, fminf(1.0f, s));
    v = fmaxf(0.0f, fminf(1.0f, v));
    float c = v * s;
    float hh = h / 60.0f;
    float x = c * (1.0f - fabsf(fmodf(hh, 2.0f) - 1.0f));
    float m = v - c;
    float r, g, b;
    if (hh < 1)      { r = c; g = x; b = 0; }
    else if (hh < 2) { r = x; g = c; b = 0; }
    else if (hh < 3) { r = 0; g = c; b = x; }
    else if (hh < 4) { r = 0; g = x; b = c; }
    else if (hh < 5) { r = x; g = 0; b = c; }
    else             { r = c; g = 0; b = x; }
    *R = (u8)((r + m) * 255.0f + 0.5f);
    *G = (u8)((g + m) * 255.0f + 0.5f);
    *B = (u8)((b + m) * 255.0f + 0.5f);
}

static void rgb2hsv(u8 R, u8 G, u8 B, float* h, float* s, float* v) {
    float rf = R / 255.0f, gf = G / 255.0f, bf = B / 255.0f;
    float mx = fmaxf(rf, fmaxf(gf, bf));
    float mn = fminf(rf, fminf(gf, bf));
    float d = mx - mn;
    *v = mx;
    *s = (mx <= 0.0f) ? 0.0f : d / mx;
    if (d <= 0.0001f) { *h = 0.0f; return; }
    float hh;
    if (mx == rf)      hh = fmodf((gf - bf) / d, 6.0f);
    else if (mx == gf) hh = (bf - rf) / d + 2.0f;
    else               hh = (rf - gf) / d + 4.0f;
    hh *= 60.0f;
    if (hh < 0) hh += 360.0f;
    *h = hh;
}

void colorWheelInit(ColorWheel* w) {
    w->h = 0; w->s = 1; w->v = 1;
    w->dr = 255; w->dg = 0; w->db = 0;
}

void colorWheelSetRGB(ColorWheel* w, u8 r, u8 g, u8 b) {
    rgb2hsv(r, g, b, &w->h, &w->s, &w->v);
    w->dr = r; w->dg = g; w->db = b;
}

void colorWheelGetRGB(const ColorWheel* w, u8* r, u8* g, u8* b) {
    hsv2rgb(w->h, w->s, w->v, r, g, b);
}

/* cantos do triangulo SV -- ESTATICO (nao gira com o hue): P0=hue puro no topo,
 * P1=branco embaixo-direita, P2=preto embaixo-esquerda. So a COR do canto P0
 * muda com o hue; a geometria fica parada. */
static const float TRI_ANG[3] = { -90.0f, 30.0f, 150.0f };
static void triVerts(float* x, float* y) {
    for (int i = 0; i < 3; i++) {
        float a = TRI_ANG[i] * DEG2RAD;
        x[i] = CX + R_TRI * cosf(a);
        y[i] = CY + R_TRI * sinf(a);
    }
}

void colorWheelInput(ColorWheel* w, const AppInput* in, float dt) {
    /* ---- toque (primario) ---- */
    if (in->touchDown || in->touchHeld) {
        float tx = (float)in->touchPX, ty = (float)in->touchPY;
        float dx = tx - CX, dy = ty - CY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d >= R_IN - 6.0f && d <= R_OUT + 8.0f) {
            /* anel -> matiz */
            float a = atan2f(dy, dx) / DEG2RAD;
            if (a < 0) a += 360.0f;
            w->h = a;
        } else {
            /* triangulo -> saturacao/brilho (baricentrica) */
            float px[3], py[3]; triVerts(px, py);
            float det = (py[1] - py[2]) * (px[0] - px[2]) + (px[2] - px[1]) * (py[0] - py[2]);
            if (fabsf(det) > 0.0001f) {
                float a = ((py[1] - py[2]) * (tx - px[2]) + (px[2] - px[1]) * (ty - py[2])) / det;
                float b = ((py[2] - py[0]) * (tx - px[2]) + (px[0] - px[2]) * (ty - py[2])) / det;
                a = fmaxf(0.0f, a);
                b = fmaxf(0.0f, b);
                if (a + b > 1.0f) { float t = a + b; a /= t; b /= t; }
                float vv = a + b;
                float ss = (vv > 0.0001f) ? a / vv : 0.0f;
                w->v = vv; w->s = ss;
            }
        }
    }

    /* ---- D-pad / ombros (fallback) ---- */
    if (in->held & KEY_LEFT)  w->h -= 120.0f * dt;
    if (in->held & KEY_RIGHT) w->h += 120.0f * dt;
    if (in->held & KEY_UP)    w->v += 1.1f * dt;
    if (in->held & KEY_DOWN)  w->v -= 1.1f * dt;
    if (in->held & KEY_L)     w->s -= 1.1f * dt;
    if (in->held & KEY_R)     w->s += 1.1f * dt;
    w->h = fmodf(w->h, 360.0f);
    if (w->h < 0) w->h += 360.0f;
    w->v = fmaxf(0.0f, fminf(1.0f, w->v));
    w->s = fmaxf(0.0f, fminf(1.0f, w->s));
}

static u32 c32(u8 r, u8 g, u8 b) { return C2D_Color32(r, g, b, 255); }

void colorWheelRender(C2D_TextBuf buf, ColorWheel* w) {
    /* ---- anel de matiz ---- */
    for (int i = 0; i < RING_SEG; i++) {
        float a0 = (float)i / RING_SEG * 360.0f;
        float a1 = (float)(i + 1) / RING_SEG * 360.0f;
        float r0 = a0 * DEG2RAD, r1 = a1 * DEG2RAD;
        u8 cr0, cg0, cb0, cr1, cg1, cb1;
        hsv2rgb(a0, 1, 1, &cr0, &cg0, &cb0);
        hsv2rgb(a1, 1, 1, &cr1, &cg1, &cb1);
        u32 col0 = c32(cr0, cg0, cb0), col1 = c32(cr1, cg1, cb1);
        float o0x = CX + R_OUT * cosf(r0), o0y = CY + R_OUT * sinf(r0);
        float i0x = CX + R_IN  * cosf(r0), i0y = CY + R_IN  * sinf(r0);
        float o1x = CX + R_OUT * cosf(r1), o1y = CY + R_OUT * sinf(r1);
        float i1x = CX + R_IN  * cosf(r1), i1y = CY + R_IN  * sinf(r1);
        C2D_DrawTriangle(o0x, o0y, col0, i0x, i0y, col0, o1x, o1y, col1, 0.0f);
        C2D_DrawTriangle(i0x, i0y, col0, i1x, i1y, col1, o1x, o1y, col1, 0.0f);
    }

    /* ---- triangulo SV ---- */
    float px[3], py[3]; triVerts(px, py);
    u8 hr, hg, hb; hsv2rgb(w->h, 1, 1, &hr, &hg, &hb);
    C2D_DrawTriangle(px[0], py[0], c32(hr, hg, hb),
                     px[1], py[1], c32(255, 255, 255),
                     px[2], py[2], c32(0, 0, 0), 0.0f);

    /* ---- knob do matiz ---- */
    float ka = w->h * DEG2RAD;
    float kmr = (R_IN + R_OUT) * 0.5f;
    float kx = CX + kmr * cosf(ka), ky = CY + kmr * sinf(ka);
    C2D_DrawCircleSolid(kx, ky, 0.0f, 8.0f, c32(255, 255, 255));
    C2D_DrawCircleSolid(kx, ky, 0.0f, 6.0f, c32(20, 20, 24));
    C2D_DrawCircleSolid(kx, ky, 0.0f, 4.5f, c32(hr, hg, hb));

    /* ---- knob SV (baricentrica a=vs P0, b=v(1-s) P1, c=1-v P2) ---- */
    float a = w->v * w->s, b = w->v * (1.0f - w->s), c = 1.0f - w->v;
    float sx = a * px[0] + b * px[1] + c * px[2];
    float sy = a * py[0] + b * py[1] + c * py[2];
    u8 cr, cg, cb; hsv2rgb(w->h, w->s, w->v, &cr, &cg, &cb);
    C2D_DrawCircleSolid(sx, sy, 0.0f, 7.0f, c32(255, 255, 255));
    C2D_DrawCircleSolid(sx, sy, 0.0f, 5.0f, c32(20, 20, 24));
    C2D_DrawCircleSolid(sx, sy, 0.0f, 3.5f, c32(cr, cg, cb));

    /* ---- painel direito: preview + hex + rgb ---- */
    float sp = fminf(1.0f, uiFrameDt() * 12.0f);
    w->dr += ((float)cr - w->dr) * sp;
    w->dg += ((float)cg - w->dg) * sp;
    w->db += ((float)cb - w->db) * sp;
    u8 pr = (u8)(w->dr + 0.5f), pg = (u8)(w->dg + 0.5f), pb = (u8)(w->db + 0.5f);

    float panelX = 206.0f;
    ColorRGBA prev = { pr, pg, pb, 255 };
    ColorRGBA border = themeIsDark() ? (ColorRGBA){255,255,255,30} : (ColorRGBA){20,24,34,30};
    UI_RoundFrame(panelX, 40.0f, 100.0f, 52.0f, 12.0f, prev, border);
    ColorRGBA onPrev = themeContrastText(prev);
    char hex[16]; snprintf(hex, sizeof(hex), "#%02X%02X%02X", cr, cg, cb);
    UI_TextCenter(buf, NULL, hex, panelX + 50.0f, 58.0f, 0.30f, 0.30f, onPrev);

    char line[24];
    snprintf(line, sizeof(line), "R  %d", cr);
    UI_Text(buf, NULL, line, panelX + 6.0f, 104.0f, 0.24f, 0.24f, g_theme.textSecondary);
    snprintf(line, sizeof(line), "G  %d", cg);
    UI_Text(buf, NULL, line, panelX + 6.0f, 128.0f, 0.24f, 0.24f, g_theme.textSecondary);
    snprintf(line, sizeof(line), "B  %d", cb);
    UI_Text(buf, NULL, line, panelX + 6.0f, 152.0f, 0.24f, 0.24f, g_theme.textSecondary);
}
