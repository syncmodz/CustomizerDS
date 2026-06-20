#pragma once
#include "common.h"
#include "input.h"
#include "anim.h"

typedef enum {
    SCREEN_MAIN,
    SCREEN_THEME,
    SCREEN_LED,
    SCREEN_FONTS,
    SCREEN_ABOUT,
    SCREEN_COUNT,
} ScreenID;

typedef struct {
    const char* title;
    const char* subtitle;
    u32 color;
    ScreenID target;
} MenuItem;

extern const MenuItem g_menuItems[5];

typedef struct {
    ScreenID current, previous;
    AnimState trans;
    int selected;
} MenuState;

void menuInit(MenuState* ms);
void menuDraw(MenuState* ms);
void menuDrawTop(MenuState* ms);
void menuHandle(MenuState* ms, InputState* in);
void menuNav(MenuState* ms, ScreenID t);
