#ifndef DARKMODE_H_
#define DARKMODE_H_

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include "input.h"

void darkmodeInit(void);
void darkmodeUpdate(const AppInput* in, int* currentScreen);
void darkmodeRenderTop(C2D_TextBuf buf);
void darkmodeRenderBottom(C2D_TextBuf buf);
int darkmodeSelected(void);

#endif
