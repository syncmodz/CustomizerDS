#pragma once
#include "common.h"
#include "anim.h"
#include "theme.h"

#define PANEL_R 10
#define ITEM_R 8
#define BTN_R 6

void drawPanel(int x, int y, int w, int h, int r, u32 color, float alpha);
void drawShadow(int x, int y, int w, int h, int r, float size, u32 color);
void drawCard(int x, int y, int w, int h, u32 color, const char* title, const char* subtitle, float parallax, float alpha);
void drawToggle(int x, int y, bool on, u32 accent, float alpha);
void drawSlider2(int x, int y, int w, int h, float pct, u32 track, u32 fill, u32 thumb, float alpha);
void drawProgress(int x, int y, int w, int h, float pct, u32 track, u32 fill);
void drawHexInput2(int x, int y, int w, int h, const char* text, bool active, float blink, u32 bg, u32 fg, float alpha);
void drawHueWheel(int cx, int cy, int r);
void drawStartupScreen(float progress);
void drawMacOSTouchBar2(int y, float alpha);
void drawBadge2(int x, int y, const char* label, u32 color);
