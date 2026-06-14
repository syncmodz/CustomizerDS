#ifndef THEME_H
#define THEME_H

#include <3ds.h>
#include <stdbool.h>

typedef struct {
    u8 r, g, b, a;
} ColorRGBA;

typedef struct {
    ColorRGBA background;
    ColorRGBA backgroundTop;
    ColorRGBA surface;
    ColorRGBA primary;
    ColorRGBA primaryLight;
    ColorRGBA primaryDark;
    ColorRGBA accent;
    ColorRGBA onPrimary;
    ColorRGBA textPrimary;
    ColorRGBA textSecondary;
    ColorRGBA textHint;
    ColorRGBA divider;
} ThemePalette;

extern ThemePalette g_theme;

void themeInit(void);
void themeSetDark(bool dark);
bool themeIsDark(void);

#endif
