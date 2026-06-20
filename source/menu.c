#include "menu.h"
#include "input.h"
#include "ui.h"
#include "theme.h"
#include "fonts.h"

const MenuItem g_menuItems[5] = {
    {"Theme", "Customize colors & appearance", ACCENT_BLUE, SCREEN_THEME},
    {"LED", "Controller LED configuration", ACCENT_PURPLE, SCREEN_LED},
    {"Fonts", "System font preview", ACCENT_ORANGE, SCREEN_FONTS},
    {"About", "App info & credits", ACCENT_GREEN, SCREEN_ABOUT},
    {"Close", "Return to Homebrew Launcher", ACCENT_RED, SCREEN_MAIN},
};

void menuInit(MenuState* ms) {
    memset(ms, 0, sizeof(MenuState));
    ms->current = SCREEN_MAIN;
    animStateInit(&ms->trans);
}

void menuNav(MenuState* ms, ScreenID t) {
    ms->previous = ms->current;
    ms->current = t;
    animStateInit(&ms->trans);
    animStateFadeIn(&ms->trans);
}

void menuDraw(MenuState* ms) {
    if (ms->trans.active) animStateUpdate(&ms->trans, 1.0f/60.0f);
    float alpha = ms->trans.active ? ms->trans.opacity : 1;

    int startY = 10;
    int cw = SCREEN_BOT_W - 40, ch = 38, sp = 6;
    float glow = ms->trans.active ? ms->trans.opacity : 0;

    if (glow > 0) {
        C2D_DrawRectSolid(0, 0, 0.5f, SCREEN_BOT_W, SCREEN_BOT_H, colorWithAlpha(g_bgColor, (u8)(glow * 80)));
    }
    if (glow > 0.5f) {
        C2D_DrawRectSolid(0, 0, 0.5f, SCREEN_BOT_W, 3, colorWithAlpha(accentForTheme(), (u8)((glow-0.5f)*2*200)));
    }

    for (int i = 0; i < 5; i++) {
        const MenuItem* item = &g_menuItems[i];
        int cy = startY + i * (ch + sp);
        float par = g_parallaxOffset * (0.3f + i * 0.12f);

        int ex = i == ms->selected ? 1 : 0;
        int ecw = cw + ex * 6, ech = ch + ex * 6;
        int exOff = (ecw - cw) / 2;
        int ecx = (SCREEN_BOT_W - ecw) / 2;

        drawCard(ecx, cy + (int)par, ecw, ech,
            i == ms->selected ? blendColors(g_surfaceColor, item->color, 0.08f) : g_surfaceColor,
            item->title, item->subtitle, par, alpha);

        if (i == ms->selected && alpha > 0.5f) {
            C2D_DrawRectSolid(ecx, cy + ech - 2 + (int)par, 0.5f, ecw, 2,
                colorWithAlpha(item->color, (u8)((alpha-0.5f)*2*200)));
        }
    }

    drawMacOSTouchBar2(startY + 5 * (ch + sp) + 8, alpha);
}

void menuDrawTop(MenuState* ms) {
    char t[48];
    snprintf(t, sizeof(t), "CustomizerDS");
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), t, SCREEN_TOP_W/2, 12, 0.5f, 0.5f, g_textColor);

    int bw = 240, bh = 4;
    float p = 0;
    switch (ms->current) {
        case SCREEN_MAIN: p = 0; break;
        case SCREEN_THEME: p = 0.25f; break;
        case SCREEN_LED: p = 0.5f; break;
        case SCREEN_FONTS: p = 0.75f; break;
        case SCREEN_ABOUT: p = 1; break;
        default: p = 0; break;
    }
    drawProgress(SCREEN_TOP_W/2 - bw/2, 100, bw, bh, p, rgba8(58,58,60,255), accentForTheme());

    const char* steps[] = {"Home", "Theme", "LED", "Fonts", "About"};
    for (int i = 0; i < 5; i++) {
        u32 c = i == ms->current ? accentForTheme() : g_secTextColor;
        drawTextCentered(fontsGetBuf(), fontsGetSystem(), steps[i], SCREEN_TOP_W/2 - 120 + i * 60, 112, 0.3f, 0.3f, c);
    }
}

void menuHandle(MenuState* ms, InputState* in) {
    if (in->downDir) ms->selected = (ms->selected + 1) % 5;
    if (in->upDir) ms->selected = (ms->selected - 1 + 5) % 5;
    if (in->confirm || in->a) {
        ScreenID t = g_menuItems[ms->selected].target;
        if (t == SCREEN_MAIN && ms->current == SCREEN_MAIN) return;
        if (t == SCREEN_MAIN) return;
        menuNav(ms, t);
    }
    if (in->b || in->cancel) {
        if (ms->current != SCREEN_MAIN) ms->current = SCREEN_MAIN;
    }
}
