#ifndef COLORWHEEL_H
#define COLORWHEEL_H

#include <3ds.h>
#include <citro2d.h>
#include "input.h"

/* Seletor de cor tipo "roda HSV": anel de matiz (hue) por fora + triangulo de
 * saturacao/brilho por dentro. Toque na tela de baixo (primario) + D-pad
 * (L/R = matiz, cima/baixo = brilho, botoes L/R de ombro = saturacao).
 * Desenhado na tela de baixo (320x240). */

typedef struct {
    float h;      /* matiz 0..360 */
    float s;      /* saturacao 0..1 */
    float v;      /* brilho 0..1 */
    /* animacao suave do preview (persegue a cor atual) */
    float dr, dg, db;
} ColorWheel;

void colorWheelInit(ColorWheel* w);
void colorWheelSetRGB(ColorWheel* w, u8 r, u8 g, u8 b);
void colorWheelGetRGB(const ColorWheel* w, u8* r, u8* g, u8* b);
void colorWheelInput(ColorWheel* w, const AppInput* in, float dt);
void colorWheelRender(C2D_TextBuf buf, ColorWheel* w);

#endif
