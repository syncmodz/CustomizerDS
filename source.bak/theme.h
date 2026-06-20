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
    ColorRGBA surfaceAlt;
    ColorRGBA surfaceElevated;
    ColorRGBA primary;
    ColorRGBA primaryLight;
    ColorRGBA primaryDark;
    ColorRGBA accent;
    ColorRGBA success;
    ColorRGBA warning;
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
void themeSetAccentIndex(int index);
int themeGetAccentIndex(void);
int themeAccentCount(void);
const char* themeAccentName(int index);
ColorRGBA themeAccentColor(int index);
u32 themeColor(ColorRGBA color);
ColorRGBA themeMix(ColorRGBA a, ColorRGBA b, float t);

/* Cor de texto com contraste seguro sobre qualquer fundo (formula de luminancia real,
 * nao a soma crua de canais) -- usar sempre que o fundo nao for diretamente uma cor
 * do tema (ex: fundo fixo, accent, ou cor escolhida pelo usuario). */
ColorRGBA themeContrastText(ColorRGBA bg);

/* Accent customizado via hexadecimal (#RRGGBB), independente dos presets */
bool themeAccentIsCustom(void);
ColorRGBA themeGetCustomAccent(void);
void themeSetCustomAccent(ColorRGBA c);

#endif
