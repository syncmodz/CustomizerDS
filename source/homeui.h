#ifndef HOMEUI_H
#define HOMEUI_H

#include <citro2d.h>
#include "input.h"

/* 2.0.0: aba UI da Home -- instala packs de tema da interface do Home Menu via
 * LayeredFS do Luma (mecanismo cooolgamer/aromakitsune: copiar a arvore do pack
 * pra sdmc:/luma/titles/<title-id-da-home-por-regiao>/). SD-only, NAO mexe em
 * NAND/extdata -> nao tem risco de brick (remover a pasta desfaz). Precisa de
 * 'game patching' ligado no Luma + reboot pra ver. */
void homeuiInit(void);
void homeuiUpdate(const AppInput* in, float dt, int* currentScreen);
void homeuiRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
void homeuiRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);

#endif
