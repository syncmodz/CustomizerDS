#include "led.h"
#include "input.h"
#include "ui.h"
#include "theme.h"
#include "fonts.h"

static const char* s_modes[LED_MODE_COUNT] = {"Off", "Solid", "Breathe", "Rainbow"};

void ledInit(LEDState* ls, MenuState* menu) {
    memset(ls, 0, sizeof(LEDState));
    ls->menu = menu;
    ls->mode = LED_MODE_SOLID;
    ls->brightness = 100;
    ls->speed = 5;
    ls->color = ACCENT_BLUE;
    ls->selMode = 1;
    for (int i = 0; i < LED_MODE_COUNT; i++) animStateInit(&ls->modeAnim[i]);
}

void ledUpdate(LEDState* ls, InputState* in) {
    if (in->left) ls->selMode = (ls->selMode - 1 + 4) % 4;
    if (in->right) ls->selMode = (ls->selMode + 1) % 4;
    if (in->confirm || in->a) { ls->mode = (LEDMode)ls->selMode; }
    if (in->upDir) { ls->brightness = clampi(ls->brightness + 10, 0, 100); }
    if (in->downDir) { ls->brightness = clampi(ls->brightness - 10, 0, 100); }
    if (in->b || in->cancel) ls->menu->current = SCREEN_MAIN;
    for (int i = 0; i < 4; i++) { if (ls->modeAnim[i].active) animStateUpdate(&ls->modeAnim[i], 1.0f/60.0f); }
}

void ledDraw(LEDState* ls) {
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "LED Settings", SCREEN_BOT_W/2, 8, 0.5f, 0.5f, g_textColor);

    int mw = 70, mh = 28, sp = 6;
    int tot = 4 * mw + 3 * sp;
    int sx = (SCREEN_BOT_W - tot) / 2;
    for (int i = 0; i < 4; i++) {
        int x = sx + i * (mw + sp);
        u32 mc = i == ls->selMode ? accentForTheme() : g_surfaceColor;
        u32 tc = i == ls->selMode ? rgba8(255,255,255,255) : g_textColor;
        drawRoundedRect(x, 38, mw, mh, 6, mc);
        drawTextCentered(fontsGetBuf(), fontsGetSystem(), s_modes[i], x + mw/2, 44, 0.35f, 0.35f, tc);
    }

    int py = 80, ps = 44;
    int pcx = SCREEN_BOT_W/2;
    u32 lc = ls->color;
    if (ls->mode == LED_MODE_OFF) lc = rgba8(28,28,30,255);
    if (ls->mode == LED_MODE_RAINBOW) {
        float t = appGetTime() * 0.5f;
        lc = rgba8(
            (u8)((sinf(t*0.3f)*0.5f+0.5f)*255),
            (u8)((sinf(t*0.3f+2.094f)*0.5f+0.5f)*255),
            (u8)((sinf(t*0.3f+4.188f)*0.5f+0.5f)*255),
            255
        );
    }
    if (ls->mode == LED_MODE_BREATHE) {
        float t = sinf(appGetTime() * ls->speed * 0.1f) * 0.5f + 0.5f;
        lc = colorWithAlpha(ls->color, (u8)(t * ls->brightness / 100.0f * 255));
    }
    C2D_DrawCircleSolid(pcx, py + ps/2, 0.5f, ps/2, rgba8(0,0,0,60));
    C2D_DrawCircleSolid(pcx, py + ps/2, 0.5f, ps/2 - 2, lc);
    C2D_DrawCircleSolid(pcx, py + ps/2, 0.5f, ps/2 - 4, lightenColor(lc, 0.2f));

    char buf[48];
    snprintf(buf, sizeof(buf), "Brightness: %d%%", ls->brightness);
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), buf, SCREEN_BOT_W/2, py + ps + 10, 0.35f, 0.35f, g_textColor);
    snprintf(buf, sizeof(buf), "Speed: %d", ls->speed);
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), buf, SCREEN_BOT_W/2, py + ps + 26, 0.35f, 0.35f, g_secTextColor);

    drawSlider2(40, py + ps + 40, SCREEN_BOT_W - 80, 20, ls->brightness/100.0f,
        rgba8(58,58,60,255), accentForTheme(), rgba8(255,255,255,255), 1);
}

void ledDrawTop(LEDState* ls) {
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "LED Control", SCREEN_TOP_W/2, 12, 0.5f, 0.5f, g_textColor);
    char buf[48];
    snprintf(buf, sizeof(buf), "Brightness: %d%% | Speed: %d", ls->brightness, ls->speed);
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), buf, SCREEN_TOP_W/2, 40, 0.35f, 0.35f, g_secTextColor);
    for (int i = 0; i < 4; i++) {
        int x = 10 + i * 80;
        u32 c = i == (int)ls->mode ? accentForTheme() : g_secTextColor;
        drawTextCentered(fontsGetBuf(), fontsGetSystem(), s_modes[i], x + 40, SCREEN_TOP_H - 20, 0.3f, 0.3f, c);
        if (i == (int)ls->mode) C2D_DrawRectSolid(x + 15, SCREEN_TOP_H - 24, 0.5f, 50, 2, accentForTheme());
    }
}
