#ifndef LED_H_
#define LED_H_

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include "input.h"

void ledInit(void);
void ledEnter(void);
void ledExit(void);
void ledUpdate(const AppInput* in, int* currentScreen);
void ledRenderTop(C2D_TextBuf buf);
void ledRenderBottom(C2D_TextBuf buf);
int ledSelected(void);

#endif
