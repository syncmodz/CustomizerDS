#ifndef MENU_H
#define MENU_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

void menuInit(void);
void menuUpdate(u32 kDown, touchPosition* touch, bool touchDown, int* currentScreen);
void menuRenderTop(C2D_TextBuf buf);
void menuRenderBottom(C2D_TextBuf buf);

#endif
