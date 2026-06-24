#ifndef LED_H_
#define LED_H_

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include "input.h"
#include "theme.h"

void ledInit(void);
void ledEnter(void);
void ledExit(void);
void ledUpdate(const AppInput* in, float dt, int* currentScreen);
void ledRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
void ledRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
int ledSelected(void);
const char* ledModeName(void);
ColorRGBA ledPreviewColor(void);

#endif
