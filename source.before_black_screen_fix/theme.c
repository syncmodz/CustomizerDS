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
    { "Rosa neon",     {255, 96, 160, 255} },
    { "Roxo DS",       {139, 92, 246, 255} },
    { "Azul cristal",  { 72, 168, 255, 255} },
    { "Verde mint",    { 64, 220, 160, 255} },
    { "Laranja",       {255, 155,  72, 255} },
};

static const ThemePalette DARK_BASE = {
    .background     = { 13, 13, 20, 255 },
    .backgroundTop  = { 10, 10, 18, 255 },
    .surface        = { 27, 27, 40, 255 },
    .surfaceAlt     = { 35, 35, 52, 255 },
    .primary        = { 92, 56, 178, 255 },
    .primaryLight   = {132, 92, 220, 255 },
    .primaryDark    = { 52, 29, 101, 255 },
    .accent         = {255, 96, 160, 255 },
    .success        = { 72, 220, 150, 255 },
    .warning        = {255, 190,  88, 255 },
    .onPrimary      = {255, 255, 255, 255 },
    .textPrimary    = {244, 244, 250, 255 },
    .textSecondary  = {178, 176, 196, 255 },
    .textHint       = {115, 112, 135, 255 },
    .divider        = { 55, 55,  76, 255 },
};

static const ThemePalette LIGHT_BASE = {
    .background     = {240, 240, 248, 255 },
    .backgroundTop  = {232, 232, 244, 255 },
    .surface        = {255, 255, 255, 255 },
    .surfaceAlt     = {246, 246, 252, 255 },
    .primary        = { 92, 56, 178, 255 },
    .primaryLight   = {132, 92, 220, 255 },
    .primaryDark    = { 52, 29, 101, 255 },
    .accent         = {255, 96, 160, 255 },
    .success        = { 40, 178, 118, 255 },
    .warning        = {220, 132,  42, 255 },
    .onPrimary      = {255, 255, 255, 255 },
    .textPrimary    = { 22, 22,  32, 255 },
    .textSecondary  = { 88,  86, 108, 255 },
    .textHint       = {146, 144, 164, 255 },
    .divider        = {218, 216, 230, 255 },
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
