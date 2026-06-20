#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "theme.h"
#include "anim.h"
#include "input.h"
#include "fonts.h"
#include "ui.h"
#include "menu.h"
#include "darkmode.h"
#include "led.h"
#include "color_picker.h"

static C3D_RenderTarget* s_top = NULL;
static C3D_RenderTarget* s_bot = NULL;

int main() {
    gfxInitDefault();
    romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    s_top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    s_bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    fontsInit();
    themeRefresh();
    animInit();

    MenuState ms;
    menuInit(&ms);

    DarkModeState ds;
    darkModeInit(&ds, &ms);

    LEDState ls;
    ledInit(&ls, &ms);

    InputState in;
    memset(&in, 0, sizeof(in));

    u64 last = osGetTime();
    float dt = 1.0f/60.0f;

    g_bootProgress = 0;
    g_bootComplete = false;

    while (aptMainLoop()) {
        u64 now = osGetTime();
        dt = (now - last) / 1000.0f;
        if (dt > 0.05f) dt = 1.0f/60.0f;
        last = now;

        inputPoll(&in);
        if (in.start) break;

        if (!g_bootComplete) {
            animUpdate(dt);
            g_bootProgress += dt * 0.15f;
            if (g_bootProgress >= 1) g_bootComplete = true;
        } else {
            animUpdate(dt);
            switch (ms.current) {
                case SCREEN_MAIN: menuHandle(&ms, &in); break;
                case SCREEN_THEME: darkModeUpdate(&ds, &in); break;
                case SCREEN_LED: ledUpdate(&ls, &in); break;
                case SCREEN_FONTS: if (in.b || in.cancel) ms.current = SCREEN_MAIN; break;
                case SCREEN_ABOUT: if (in.b || in.cancel) ms.current = SCREEN_MAIN; break;
                default: break;
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // --- Top Screen ---
        C2D_TargetClear(s_top, g_bgColor);
        C2D_SceneBegin(s_top);
        C2D_TextBufClear(fontsGetBuf());

        if (!g_bootComplete) {
            drawTextCentered(fontsGetBuf(), fontsGetSystem(), "CustomizerDS", SCREEN_TOP_W/2, SCREEN_TOP_H/2 - 20, 0.6f, 0.6f, g_textColor);
            drawTextCentered(fontsGetBuf(), fontsGetSystem(), "for Nintendo 3DS", SCREEN_TOP_W/2, SCREEN_TOP_H/2 + 4, 0.35f, 0.35f, g_secTextColor);
            drawStartupScreen(g_bootProgress);
        } else {
            switch (ms.current) {
                case SCREEN_MAIN: menuDrawTop(&ms); break;
                case SCREEN_THEME: darkModeDrawTop(&ds); break;
                case SCREEN_LED: ledDrawTop(&ls); break;
                case SCREEN_FONTS:
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "Font Preview", SCREEN_TOP_W/2, 12, 0.5f, 0.5f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "ABCDEFGHIJKLMNOPQRSTUVWXYZ", SCREEN_TOP_W/2, 60, 0.45f, 0.45f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "abcdefghijklmnopqrstuvwxyz", SCREEN_TOP_W/2, 82, 0.45f, 0.45f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "0123456789 !@#$%^&*()", SCREEN_TOP_W/2, 104, 0.45f, 0.45f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "B to go back", SCREEN_TOP_W/2, SCREEN_TOP_H-20, 0.3f, 0.3f, g_secTextColor);
                    break;
                case SCREEN_ABOUT:
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "About", SCREEN_TOP_W/2, 12, 0.5f, 0.5f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "CustomizerDS v2.0", SCREEN_TOP_W/2, 60, 0.5f, 0.5f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "Customize your 3DS experience", SCREEN_TOP_W/2, 84, 0.35f, 0.35f, g_secTextColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "B to go back", SCREEN_TOP_W/2, SCREEN_TOP_H-20, 0.3f, 0.3f, g_secTextColor);
                    break;
                default: break;
            }
        }

        // --- Bottom Screen ---
        C2D_TargetClear(s_bot, g_bgColor);
        C2D_SceneBegin(s_bot);

        if (!g_bootComplete) {
            float p = g_bootProgress;
            drawTextCentered(fontsGetBuf(), fontsGetSystem(), "CustomizerDS", SCREEN_BOT_W/2, (int)(-50 + p * 160), 0.7f, 0.7f,
                colorWithAlpha(g_textColor, (u8)(p * 255)));
            drawTextCentered(fontsGetBuf(), fontsGetSystem(), "Loading...", SCREEN_BOT_W/2, (int)(140 + p * 20), 0.35f, 0.35f,
                colorWithAlpha(g_secTextColor, (u8)(p * 200)));
            drawProgress(SCREEN_BOT_W/2 - 100, 180, 200, 4, p, rgba8(58,58,60,255), accentForTheme());
        } else {
            switch (ms.current) {
                case SCREEN_MAIN: menuDraw(&ms); break;
                case SCREEN_THEME: darkModeDraw(&ds); break;
                case SCREEN_LED: ledDraw(&ls); break;
                case SCREEN_FONTS:
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "Font Preview", SCREEN_BOT_W/2, 20, 0.5f, 0.5f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "System font used throughout", SCREEN_BOT_W/2, 44, 0.35f, 0.35f, g_secTextColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "ABCDEFGHIJKLMNOPQRSTUVWXYZ", SCREEN_BOT_W/2, 80, 0.45f, 0.45f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "abcdefghijklmnopqrstuvwxyz", SCREEN_BOT_W/2, 104, 0.45f, 0.45f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "0123456789", SCREEN_BOT_W/2, 128, 0.45f, 0.45f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "B to go back", SCREEN_BOT_W/2, SCREEN_BOT_H-20, 0.3f, 0.3f, g_secTextColor);
                    break;
                case SCREEN_ABOUT:
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "About", SCREEN_BOT_W/2, 20, 0.5f, 0.5f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "CustomizerDS v2.0", SCREEN_BOT_W/2, 60, 0.5f, 0.5f, g_textColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "Built for CFW 3DS", SCREEN_BOT_W/2, 84, 0.35f, 0.35f, g_secTextColor);
                    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "B to go back", SCREEN_BOT_W/2, SCREEN_BOT_H-20, 0.3f, 0.3f, g_secTextColor);
                    break;
                default: break;
            }
        }

        C3D_FrameEnd(0);
    }

    fontsExit();

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
