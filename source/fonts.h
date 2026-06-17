#ifndef FONTS_H_
#define FONTS_H_

#include <3ds.h>
#include <citro2d.h>

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
void fontsRender(u32 kDown, u32 kHeld, int* currentScreen);
void fontsSystemCleanup(void);

#endif
