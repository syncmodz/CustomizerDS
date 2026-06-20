#include "theme.h"

bool g_darkMode = true;
u32 g_accentColor = ACCENT_BLUE;
u32 g_bgColor = DARK_BG;
u32 g_surfaceColor = DARK_SURFACE;
u32 g_textColor = DARK_TEXT;
u32 g_secTextColor = DARK_SEC;
u32 g_borderColor = DARK_BORDER;

const u32 g_accentOptions[8] = {
    ACCENT_GRAPHITE, ACCENT_BLUE, ACCENT_PURPLE, ACCENT_PINK,
    ACCENT_RED, ACCENT_ORANGE, ACCENT_YELLOW, ACCENT_GREEN,
};

const char* g_accentNames[8] = {
    "Graphite", "Blue", "Purple", "Pink",
    "Red", "Orange", "Yellow", "Green",
};

u32 accentForTheme(void) {
    if (g_accentColor == ACCENT_GRAPHITE) {
        return g_darkMode ? rgba8(174,174,178,255) : rgba8(142,142,147,255);
    }
    return g_accentColor;
}

void themeRefresh(void) {
    if (g_darkMode) {
        g_bgColor = DARK_BG; g_surfaceColor = DARK_SURFACE;
        g_textColor = DARK_TEXT; g_secTextColor = DARK_SEC;
        g_borderColor = DARK_BORDER;
    } else {
        g_bgColor = LIGHT_BG; g_surfaceColor = LIGHT_SURFACE;
        g_textColor = LIGHT_TEXT; g_secTextColor = LIGHT_SEC;
        g_borderColor = LIGHT_BORDER;
    }
}
