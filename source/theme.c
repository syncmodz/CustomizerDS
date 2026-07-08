#include "theme.h"
#include "config.h"
#include "common.h"

ThemePalette g_theme;
static bool s_dark = true;
static int s_accentIndex = 0;
static bool s_customAccentActive = false;
static ColorRGBA s_customAccent = { 160, 110, 255, 255 };

typedef struct {
    const char* name;
    ColorRGBA color;
} AccentPreset;

/* Cores reais do seletor de accent do macOS (System Colors), nao "candy neon". */
static const AccentPreset ACCENTS[] = {
    { "Azul",    { 10, 132, 255, 255 } },
    { "Verde",   { 50, 215, 75, 255 } },
    { "Roxo",    { 191, 90, 242, 255 } },
    { "Rosa",    { 255, 55, 95, 255 } },
    { "Laranja", { 255, 159, 10, 255 } },
};

/* Tokens da spec v3 (docs/design-tokens.md): bg_base #0E0D12, bg_card
 * #17151C, stroke branco@7%, text_primary #F3F1F7, text_secondary
 * branco@52%. Nunca #000000 puro. (v2 tinha bg_base #0E0E10/bg_elevated
 * #1A1A1D -- v3 ajustou pro tom levemente roxo/Monet do end4.) */
static const ThemePalette DARK_BASE = {
    .background      = { 14, 13, 18, 255 },   /* bg_base #0E0D12 */
    .backgroundTop   = { 16, 15, 20, 255 },
    .surface         = { 23, 21, 28, 235 },   /* bg_card #17151C */
    .surfaceAlt      = { 20, 19, 25, 245 },
    .surfaceElevated = { 32, 29, 38, 245 },   /* elevado, mesmo tom roxo-escuro */
    .primary         = { 10, 132, 255, 255 },
    .primaryLight    = { 90, 180, 255, 255 },
    .primaryDark     = { 20, 20, 24, 255 },
    .accent          = { 10, 132, 255, 255 },
    .success         = { 50, 215, 75, 255 },
    .warning         = { 255, 159, 10, 255 },
    .onPrimary       = { 245, 245, 247, 255 },
    .textPrimary     = { 243, 241, 247, 255 },  /* text_primary #F3F1F7 */
    .textSecondary   = { 255, 255, 255, 133 },  /* text_secondary branco@52% */
    .textHint        = { 255, 255, 255, 90 },   /* terciario, branco@35% */
    .divider         = { 255, 255, 255, 18 },   /* stroke branco@7% */
};

static const ThemePalette LIGHT_BASE = {
    .background      = { 236, 236, 236, 255 },
    .backgroundTop   = { 242, 242, 242, 255 },
    .surface         = { 255, 255, 255, 255 },
    .surfaceAlt      = { 247, 247, 247, 255 },
    .surfaceElevated = { 255, 255, 255, 255 },
    .primary         = { 0, 122, 255, 255 },
    .primaryLight    = { 90, 170, 255, 255 },
    .primaryDark     = { 40, 40, 46, 255 },
    .accent          = { 0, 122, 255, 255 },
    .success         = { 40, 180, 80, 255 },
    .warning         = { 200, 120, 20, 255 },
    .onPrimary       = { 255, 255, 255, 255 },
    .textPrimary     = { 28, 28, 30, 255 },
    .textSecondary   = { 110, 110, 115, 255 },
    .textHint        = { 150, 150, 155, 255 },
    .divider         = { 0, 0, 0, 18 },
};

static void applyTheme(void) {
    g_theme = s_dark ? DARK_BASE : LIGHT_BASE;
    g_theme.accent = s_customAccentActive ? s_customAccent : themeAccentColor(s_accentIndex);
    g_theme.primaryLight = themeMix(g_theme.primary, g_theme.accent, 0.35f);
}

void themeInit(void) {
    ConfigData cfg;
    configLoad(&cfg);
    s_dark = cfg.darkMode != 0;
    s_accentIndex = clampi(cfg.accentIndex, 0, themeAccentCount() - 1);
    s_customAccentActive = (cfg.customAccentFlag != 0) &&
                            (cfg.customR || cfg.customG || cfg.customB);
    if (s_customAccentActive) {
        s_customAccent = (ColorRGBA){ cfg.customR, cfg.customG, cfg.customB, 255 };
    }
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
    s_customAccentActive = false;
    applyTheme();
}

bool themeAccentIsCustom(void) {
    return s_customAccentActive;
}

ColorRGBA themeGetCustomAccent(void) {
    return s_customAccent;
}

void themeSetCustomAccent(ColorRGBA c) {
    c.a = 255;
    s_customAccent = c;
    s_customAccentActive = true;
    applyTheme();
}

ColorRGBA themeAccentSoft(void) {
    ColorRGBA c = g_theme.accent;
    c.a = 36; /* 14% de 255 */
    return c;
}

ColorRGBA themeCardSelBg(void) {
    return themeMix(g_theme.surface, g_theme.accent, 0.08f);
}

ColorRGBA themeContrastText(ColorRGBA bg) {
    int luma = (int)bg.r * 299 + (int)bg.g * 587 + (int)bg.b * 114;
    return (luma > 150000) ? (ColorRGBA){ 10, 12, 16, 255 } : (ColorRGBA){ 250, 251, 253, 255 };
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

static ThemeWipe s_wipe;

void themeWipeTrigger(float originTopX, float originTopY, float originBotX, float originBotY) {
    /* Chamado ANTES de themeSetDark() pelo unico lugar que pode disparar a
     * troca (darkmode.c) -- g_theme ainda tem a cor ANTIGA aqui, que e
     * exatamente a que o wipe precisa "engolir". */
    s_wipe.active = true;
    s_wipe.t = 0.0f;
    s_wipe.oldBgTop = g_theme.backgroundTop;
    s_wipe.oldBgBot = g_theme.background;
    s_wipe.wasDark = s_dark; /* themeSetDark() ainda nao rodou neste ponto */
    s_wipe.originTopX = originTopX;
    s_wipe.originTopY = originTopY;
    s_wipe.originBotX = originBotX;
    s_wipe.originBotY = originBotY;
}

void themeWipeTick(float dt) {
    if (!s_wipe.active) return;
    s_wipe.t += dt;
    if (s_wipe.t >= THEME_WIPE_DUR) {
        s_wipe.t = THEME_WIPE_DUR;
        s_wipe.active = false;
    }
}

ThemeWipe* themeWipeGet(void) {
    return &s_wipe;
}
