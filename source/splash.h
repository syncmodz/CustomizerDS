#ifndef SPLASH_H
#define SPLASH_H

#include <citro2d.h>
#include "input.h"

/* 2.0.0: aba Splashes -- aplica imagens de boot do Luma (mecanismo do Anemone3DS:
 * copiar splash.bin/splashbottom.bin -> /luma/, verbatim). SD-only, sem reboot. */
void splashInit(void);
void splashUpdate(const AppInput* in, float dt, int* currentScreen);
void splashRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
void splashRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);

#endif
