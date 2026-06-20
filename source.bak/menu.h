#ifndef MENU_H
#define MENU_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include "input.h"

void menuInit(void);
void menuUpdate(const AppInput* in, int* currentScreen);
void menuRenderTop(C2D_TextBuf buf, float transVal);
void menuRenderBottom(C2D_TextBuf buf, float transVal);
int menuSelected(void);

#endif
