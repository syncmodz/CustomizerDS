#include "theme.h"
#include "config.h"

ThemePalette g_theme;
static bool s_dark = true;

static const ThemePalette THEME_DARK = {
    .background     = { 15, 15, 22, 255 },
    .backgroundTop  = { 18, 18, 28, 255 },
    .surface        = { 30, 30, 45, 255 },
    .primary        = { 103, 58, 183, 255 },
    .primaryLight   = { 152, 112, 210, 255 },
    .primaryDark    = { 75, 30, 130, 255 },
    .accent         = { 255, 100, 150, 255 },
    .onPrimary      = { 255, 255, 255, 255 },
    .textPrimary    = { 240, 240, 248, 255 },
    .textSecondary  = { 170, 168, 185, 255 },
    .textHint       = { 130, 128, 145, 255 },
    .divider        = { 50, 50, 65, 255 },
};

static const ThemePalette THEME_LIGHT = {
    .background     = { 245, 245, 250, 255 },
    .backgroundTop  = { 235, 235, 245, 255 },
    .surface        = { 255, 255, 255, 255 },
    .primary        = { 103, 58, 183, 255 },
    .primaryLight   = { 152, 112, 210, 255 },
    .primaryDark    = { 75, 30, 130, 255 },
    .accent         = { 255, 100, 150, 255 },
    .onPrimary      = { 255, 255, 255, 255 },
    .textPrimary    = { 25, 25, 35, 255 },
    .textSecondary  = { 100, 98, 115, 255 },
    .textHint       = { 160, 158, 175, 255 },
    .divider        = { 220, 218, 230, 255 },
};

void themeInit(void) {
    ConfigData cfg;
    configLoad(&cfg);
    s_dark = cfg.darkMode;
    g_theme = s_dark ? THEME_DARK : THEME_LIGHT;
}

void themeSetDark(bool dark) {
    s_dark = dark;
    g_theme = dark ? THEME_DARK : THEME_LIGHT;
}

bool themeIsDark(void) {
    return s_dark;
}
