#ifndef LED_H_
#define LED_H_

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

void ledInit(void);
void ledExit(void);
void ledUpdate(u32 kDown, u32 kHeld, touchPosition* touch, bool touchDown, int* currentScreen);
void ledRenderTop(C2D_TextBuf buf);
void ledRenderBottom(C2D_TextBuf buf);

#endif
