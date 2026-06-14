#include <math.h>
#include "theme.h"

AppTheme g_theme;

static void themeGeneratePalette(u8 r, u8 g, u8 b) {
    g_theme.primary = C2D_Color32(r, g, b, 255);

    u8 rd = (u8)(r * 0.75f), gd = (u8)(g * 0.75f), bd = (u8)(b * 0.75f);
    g_theme.primaryDark = C2D_Color32(rd, gd, bd, 255);

    u8 rl = (u8)(r + (255 - r) * 0.35f), gl = (u8)(g + (255 - g) * 0.35f), bl = (u8)(b + (255 - b) * 0.35f);
    g_theme.primaryLight = C2D_Color32(rl, gl, bl, 255);

    g_theme.accent = C2D_Color32(b, g, r, 255);

    g_theme.surface = C2D_Color32(
        (u8)(r * 0.12f + 15), (u8)(g * 0.12f + 15), (u8)(b * 0.12f + 22), 255);
    g_theme.surfaceHigh = C2D_Color32(
        (u8)(r * 0.18f + 20), (u8)(g * 0.18f + 20), (u8)(b * 0.18f + 30), 255);

    g_theme.background = C2D_Color32(15, 15, 22, 255);
    g_theme.backgroundTop = C2D_Color32(18, 18, 28, 255);

    float luminance = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
    g_theme.onPrimary = (luminance > 0.5f) ? C2D_Color32(0, 0, 0, 255) : C2D_Color32(255, 255, 255, 255);

    g_theme.textPrimary = C2D_Color32(240, 240, 248, 255);
    g_theme.textSecondary = C2D_Color32(170, 168, 185, 255);
    g_theme.textHint = C2D_Color32(100, 98, 115, 255);
    g_theme.divider = C2D_Color32(45, 40, 70, 255);
    g_theme.ripple = C2D_Color32(r, g, b, 120);
    g_theme.isDark = (luminance < 0.5f);
}

void themeInit(void) {
    g_theme.background = C2D_Color32(15, 15, 22, 255);
    g_theme.backgroundTop = C2D_Color32(18, 18, 28, 255);
    themeGeneratePalette(103, 58, 183);
}
