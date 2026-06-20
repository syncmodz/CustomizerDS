#include "darkmode.h"
#include "input.h"
#include "ui.h"
#include "theme.h"
#include "fonts.h"

void darkModeInit(DarkModeState* ds, MenuState* menu) {
    memset(ds, 0, sizeof(DarkModeState));
    ds->menu = menu;
    ds->selAccent = 1;
    for (int i = 0; i < 8; i++) animStateInit(&ds->accentAnim[i]);
}

void darkModeUpdate(DarkModeState* ds, InputState* in) {
    if (in->upDir || in->left) { ds->selAccent = (ds->selAccent - 1 + 8) % 8; animStatePopIn(&ds->accentAnim[ds->selAccent]); }
    if (in->downDir || in->right) { ds->selAccent = (ds->selAccent + 1) % 8; animStatePopIn(&ds->accentAnim[ds->selAccent]); }
    if (in->confirm || in->a) { g_accentColor = g_accentOptions[ds->selAccent]; themeRefresh(); }
    if (in->xIn) { g_darkMode = !g_darkMode; themeRefresh(); }
    if (in->b || in->cancel) ds->menu->current = SCREEN_MAIN;
    for (int i = 0; i < 8; i++) { if (ds->accentAnim[i].active) animStateUpdate(&ds->accentAnim[i], 1.0f/60.0f); }
}

void darkModeDraw(DarkModeState* ds) {
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "Theme Settings", SCREEN_BOT_W/2, 10, 0.5f, 0.5f, g_textColor);
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "X to toggle dark/light | A to apply accent", SCREEN_BOT_W/2, 26, 0.3f, 0.3f, g_secTextColor);

    drawToggle(SCREEN_BOT_W/2 - 25, 42, g_darkMode, accentForTheme(), 1);

    int dotY = 82;
    int ds2 = 22, dsp = 8;
    int totW = 8 * ds2 + 7 * dsp;
    int dx = (SCREEN_BOT_W - totW) / 2;

    for (int i = 0; i < 8; i++) {
        int cx = dx + i * (ds2 + dsp) + ds2/2;
        int cy = dotY + ds2/2;

        if (i == ds->selAccent) {
            C2D_DrawCircleSolid(cx, cy, 0.5f, ds2/2 + 3, g_borderColor);
        }

        AnimState* a = &ds->accentAnim[i];
        float s = a->active ? a->scale : 1;
        float r = (ds2/2 - 2) * s;

        C2D_DrawCircleSolid(cx, cy, 0.5f, r, g_accentOptions[i]);
        if (r > 4) C2D_DrawCircleSolid(cx, cy, 0.5f, r - 2, lightenColor(g_accentOptions[i], 0.3f));
    }

    drawTextCentered(fontsGetBuf(), fontsGetSystem(), g_accentNames[ds->selAccent], SCREEN_BOT_W/2, dotY + ds2 + 10, 0.4f, 0.4f, accentForTheme());
}

void darkModeDrawTop(DarkModeState* ds) {
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "Theme", SCREEN_TOP_W/2, 12, 0.5f, 0.5f, g_textColor);
    drawTextRight(fontsGetBuf(), fontsGetSystem(), g_darkMode ? "Dark" : "Light", SCREEN_TOP_W - 10, 12, 0.4f, 0.4f, g_secTextColor);

    int ph = 80;
    for (int i = 0; i < 8; i++) {
        int x = 10 + i * 48;
        if (x + 44 > SCREEN_TOP_W) break;
        u32 c = g_accentOptions[i];
        drawRoundedRect(x, 50, 44, ph, 6, c);
        if (i == ds->selAccent) drawRoundedRect(x, 50, 44, ph, 6, rgba8(255,255,255,40));
        drawTextCentered(fontsGetBuf(), fontsGetSystem(), g_accentNames[i], x + 22, 50 + ph + 6, 0.25f, 0.25f, g_secTextColor);
    }
}
