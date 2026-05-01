#ifndef LED_H_
#define LED_H_

#include <3ds.h>

void ledInit(void);
void ledRender(u32 kDown, u32 kHeld, int* currentScreen);
void ledDrawPreview(void);

#endif
