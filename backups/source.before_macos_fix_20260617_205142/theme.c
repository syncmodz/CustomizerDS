#include "theme.h"
#include "config.h"
#include "common.h"

ThemePalette g_theme;
static bool s_dark = true;
static int s_accentIndex = 0;

typedef struct {
    const char* name;
    ColorRGBA color;
} AccentPreset;

static const AccentPreset ACCENTS[] = {
    { "Touchbar Cyan", { 70, 210, 255, 255 } },
    { "Neon Green",    { 70, 245, 155, 255 } },
    { "Mac Purple",    { 160, 110, 255, 255 } },
    { "Rose",          { 255, 95, 145, 255 } },
    { "Amber",         { 255, 185, 75, 255 } },
};

static const ThemePalette DARK_BASE = {
    .background      = { 4, 5, 7, 255 },
    .backgroundTop   = { 8, 10, 14, 255 },
    .surface         = { 14, 17, 23, 235 },
    .surfaceAlt      = { 20, 24, 32, 245 },
    .surfaceElevated = { 24, 28, 38, 245 },
    .primary         = { 70, 210, 255, 255 },
    .primaryLight    = { 130, 230, 255, 255 },
    .primaryDark     = { 20, 30, 45, 255 },
    .accent          = { 70, 210, 255, 255 },
    .success         = { 70, 245, 155, 255 },
    .warning         = { 255, 185, 75, 255 },
    .onPrimary       = { 245, 247, 252, 255 },
    .textPrimary     = { 245, 247, 252, 255 },
    .textSecondary   = { 158, 166, 182, 255 },
    .textHint        = { 126, 134, 150, 255 },
    .divider         = { 255, 255, 255, 22 },
};

static const ThemePalette LIGHT_BASE = {
    .background      = { 220, 222, 228, 255 },
    .backgroundTop   = { 232, 234, 240, 255 },
    .surface         = { 248, 249, 252, 255 },
    .surfaceAlt      = { 240, 242, 248, 255 },
    .surfaceElevated = { 255, 255, 255, 255 },
    .primary         = { 70, 210, 255, 255 },
    .primaryLight    = { 130, 230, 255, 255 },
    .primaryDark     = { 30, 40, 60, 255 },
    .accent          = { 70, 210, 255, 255 },
    .success         = { 40, 200, 110, 255 },
    .warning         = { 220, 140, 45, 255 },
    .onPrimary       = { 255, 255, 255, 255 },
    .textPrimary     = { 22, 24, 30, 255 },
    .textSecondary   = { 90, 94, 108, 255 },
    .textHint        = { 110, 116, 132, 255 },
    .divider         = { 0, 0, 0, 18 },
};

static void applyTheme(void) {
    g_theme = s_dark ? DARK_BASE : LIGHT_BASE;
    g_theme.accent = themeAccentColor(s_accentIndex);
    g_theme.primaryLight = themeMix(g_theme.primary, g_theme.accent, 0.35f);
}

void themeInit(void) {
    ConfigData cfg;
    configLoad(&cfg);
    s_dark = cfg.darkMode != 0;
    s_accentIndex = clampi(cfg.accentIndex, 0, themeAccentCount() - 1);
    applyTheme();
}

void themeSetDark(bool dark) {
    s_dark = dark;
    applyTheme();
}

bool themeIsDark(void) {
    return s_dark;
}

void themeSetAccentIndex(int index) {
    s_accentIndex = clampi(index, 0, themeAccentCount() - 1);
    applyTheme();
}

int themeGetAccentIndex(void) {
    return s_accentIndex;
}

int themeAccentCount(void) {
    return (int)(sizeof(ACCENTS) / sizeof(ACCENTS[0]));
}

const char* themeAccentName(int index) {
    index = clampi(index, 0, themeAccentCount() - 1);
    return ACCENTS[index].name;
}

ColorRGBA themeAccentColor(int index) {
    index = clampi(index, 0, themeAccentCount() - 1);
    return ACCENTS[index].color;
}

u32 themeColor(ColorRGBA color) {
    return C2D_Color32(color.r, color.g, color.b, color.a);
}

ColorRGBA themeMix(ColorRGBA a, ColorRGBA b, float t) {
    t = clampf(t, 0.0f, 1.0f);
    ColorRGBA out;
    out.r = (u8)lerpf(a.r, b.r, t);
    out.g = (u8)lerpf(a.g, b.g, t);
    out.b = (u8)lerpf(a.b, b.b, t);
    out.a = (u8)lerpf(a.a, b.a, t);
    return out;
}
