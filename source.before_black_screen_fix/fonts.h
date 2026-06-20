#ifndef FONTS_H_
#define FONTS_H_

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

typedef struct {
    C2D_Font comfortaaRegular;
    C2D_Font comfortaaBold;
    C2D_Font madeEvolveRegular;
    C2D_Font madeEvolveBold;
    C2D_Font current;
    int currentIndex;
} FontSystem;

extern FontSystem g_fonts;

void fontsInit(void);
void fontsSystemInit(void);
void fontsSystemCleanup(void);
void fontsUpdate(u32 kDown, touchPosition* touch, bool touchDown, int* currentScreen);
void fontsRenderTop(C2D_TextBuf buf);
void fontsRenderBottom(C2D_TextBuf buf);
C2D_Font fontsCurrent(void);
C2D_Font fontsGetFont(int index);
const char* fontsCurrentName(void);
const char* fontsLabel(int index);
int fontsCount(void);
bool fontsLoaded(int index);

#endif
