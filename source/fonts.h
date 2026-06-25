#ifndef FONTS_H_
#define FONTS_H_

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include "input.h"

#define MAX_CUSTOM_FONTS 9

typedef struct {
    C2D_Font fonts[MAX_CUSTOM_FONTS];
    C2D_Font systemFont;
    C2D_Font current;
    int currentIndex;
    int count;
} FontSystem;

extern FontSystem g_fonts;

void fontsInit(void);
void fontsSystemInit(void);
void fontsSystemCleanup(void);
void fontsUpdate(const AppInput* in, int* currentScreen);
void fontsRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
void fontsRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
C2D_Font fontsCurrent(void);
C2D_Font fontsGetFont(int index);
const char* fontsCurrentName(void);
const char* fontsLabel(int index);
int fontsCount(void);
bool fontsLoaded(int index);
int fontsSelected(void);

#endif
