#pragma once
#include "common.h"

extern bool g_darkMode;
extern u32 g_accentColor;
extern u32 g_bgColor;
extern u32 g_surfaceColor;
extern u32 g_textColor;
extern u32 g_secTextColor;
extern u32 g_borderColor;

// C2D_Color32: 0xRRGGBBAA
#define ACCENT_GRAPHITE 0x8E8E93FF
#define ACCENT_BLUE    0x007AFFFF
#define ACCENT_PURPLE  0xAF52DEFF
#define ACCENT_PINK    0xFF2D55FF
#define ACCENT_RED     0xFF3B30FF
#define ACCENT_ORANGE  0xFF9500FF
#define ACCENT_YELLOW  0xFFCC00FF
#define ACCENT_GREEN   0x34C759FF

#define LIGHT_BG      0xF5F5F7FF
#define LIGHT_SURFACE 0xFFFFFFFF
#define LIGHT_TEXT    0x1D1D1FFF
#define LIGHT_SEC     0x86868BFF
#define LIGHT_BORDER  0xD2D2D7FF

#define DARK_BG       0x1C1C1EFF
#define DARK_SURFACE  0x2C2C2EFF
#define DARK_TEXT     0xF5F5F7FF
#define DARK_SEC      0x98989DFF
#define DARK_BORDER   0x444446FF

extern const u32 g_accentOptions[8];
extern const char* g_accentNames[8];

u32 accentForTheme(void);
void themeRefresh(void);
