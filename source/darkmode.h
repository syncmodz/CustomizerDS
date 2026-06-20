#pragma once
#include "common.h"
#include "menu.h"

typedef struct {
    MenuState* menu;
    int selAccent;
    AnimState accentAnim[8];
} DarkModeState;

void darkModeInit(DarkModeState* ds, MenuState* menu);
void darkModeUpdate(DarkModeState* ds, InputState* in);
void darkModeDraw(DarkModeState* ds);
void darkModeDrawTop(DarkModeState* ds);
