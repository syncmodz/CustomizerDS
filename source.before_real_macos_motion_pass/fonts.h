#ifndef FONTS_H_
#define FONTS_H_

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

#define MAX_CUSTOM_FONTS 4

typedef struct {
    C2D_Font fonts[MAX_CUSTOM_FONTS];
    bool tried[MAX_CUSTOM_FONTS];
    const char* paths[MAX_CUSTOM_FONTS];
    C2D_Font current;
    int currentIndex;
    int count;
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
