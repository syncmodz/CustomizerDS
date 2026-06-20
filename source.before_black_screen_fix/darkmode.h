#ifndef DARKMODE_H_
#define DARKMODE_H_

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

void darkmodeInit(void);
void darkmodeUpdate(u32 kDown, touchPosition* touch, bool touchDown, int* currentScreen);
void darkmodeRenderTop(C2D_TextBuf buf);
void darkmodeRenderBottom(C2D_TextBuf buf);

#endif
