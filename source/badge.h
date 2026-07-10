#ifndef BADGE_H
#define BADGE_H

#include <citro2d.h>
#include "input.h"

/* 2.0.0: aba Badges -- instala emblemas do Home Menu na extdata 0x14d1
 * (mecanismo do Anemone3DS: PNG -> RGB565 + A4 com tiling Z-order 8x8,
 * BadgeData.dat + BadgeMngFile.dat). Le sdmc:/Badges/ (PNGs soltos = set
 * "Other Badges"; subpasta = 1 set). SO no console real. */
void badgeInit(void);
void badgeUpdate(const AppInput* in, float dt, int* currentScreen);
void badgeRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
void badgeRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);

#endif
