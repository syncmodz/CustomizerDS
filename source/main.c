#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "menu.h"
#include "fonts.h"
#include "darkmode.h"
#include "led.h"
#include "config.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include "input.h"
#include "icons.h"

C3D_RenderTarget *topTarget, *botTarget;

int main() {
    gfxInitDefault();
    Result romfsRc = romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!topTarget) { C2D_Fini(); C3D_Fini(); if (!romfsRc) romfsExit(); gfxExit(); return 1; }
    botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!botTarget) { C2D_Fini(); C3D_Fini(); if (!romfsRc) romfsExit(); gfxExit(); return 1; }

    themeInit();
    fontsSystemInit();
    iconsInit();

    int currentScreen = SCREEN_MAIN_MENU;
    fontsInit();
    darkmodeInit();
    ledInit();
    uiScreenEnter();

    C2D_TextBuf buf = C2D_TextBufNew(4096);
    if (!buf) {
        iconsExit();
        fontsSystemCleanup();
        ledExit();
        if (!romfsRc) romfsExit();
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }

    bool startupDone = false;
    float startupT = 0.0f;
    const float STARTUP_DURATION = 1.30f;

    u64 lastTick = osGetTime();

    while (aptMainLoop()) {
        u64 nowTick = osGetTime();
        float dt = (nowTick - lastTick) / 1000.0f;
        lastTick = nowTick;
        if (dt <= 0.0f || dt > 0.25f) dt = 1.0f / 60.0f;

        AppInput in;
        inputRead(&in, dt);

        if (!startupDone) {
            if (in.down != 0 || in.held != 0 || in.touchHeld) {
                startupDone = true;
            } else {
                startupT += dt;
                if (startupT >= STARTUP_DURATION) startupDone = true;
            }
            if (startupDone) uiScreenEnter();
        } else {
            if (in.start) break;

            /* screen navigation with transitions */
            int oldScreen = currentScreen;
            switch (currentScreen) {
                case SCREEN_MAIN_MENU:
                    menuUpdate(&in, &currentScreen);
                    break;
                case SCREEN_FONTS:
                    fontsUpdate(&in, &currentScreen);
                    break;
                case SCREEN_DARKMODE:
                    darkmodeUpdate(&in, dt, &currentScreen);
                    break;
                case SCREEN_LED:
                    ledUpdate(&in, dt, &currentScreen);
                    break;
            }
            if (currentScreen != oldScreen) {
                transitionStart(&g_trans, TRANSITION_SLIDE_UP, oldScreen, currentScreen, 0.30f);
                uiScreenEnter();
                inputResetRepeat();
                switch (currentScreen) {
                    case SCREEN_MAIN_MENU: menuInit(); break;
                    case SCREEN_FONTS: fontsInit(); break;
                    case SCREEN_DARKMODE: darkmodeInit(); break;
                    case SCREEN_LED: ledEnter(); break;
                }
            }
        }

        uiFrameTick(dt);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(
            g_theme.backgroundTop.r, g_theme.backgroundTop.g,
            g_theme.backgroundTop.b, g_theme.backgroundTop.a));
        C2D_TargetClear(botTarget, C2D_Color32(
            g_theme.background.r, g_theme.background.g,
            g_theme.background.b, g_theme.background.a));

        C2D_TextBufClear(buf);

        C2D_SceneBegin(topTarget);
        if (startupDone) {
            float transVal = transitionActive(&g_trans) ? transitionEased(&g_trans, EASE_OUT_CUBIC) : 1.0f;
            switch (currentScreen) {
                case SCREEN_MAIN_MENU: menuRenderTop(buf, transVal); break;
                case SCREEN_FONTS: fontsRenderTop(buf, transVal); break;
                case SCREEN_DARKMODE: darkmodeRenderTop(buf, transVal); break;
                case SCREEN_LED: ledRenderTop(buf, transVal); break;
            }
        } else {
            UI_TopBackground();
            UI_StartupLogo(buf, startupT);
        }

        C2D_SceneBegin(botTarget);
        if (startupDone) {
            float transVal = transitionActive(&g_trans) ? transitionEased(&g_trans, EASE_OUT_CUBIC) : 1.0f;
            switch (currentScreen) {
                case SCREEN_MAIN_MENU: menuRenderBottom(buf, transVal); break;
                case SCREEN_FONTS: fontsRenderBottom(buf, transVal); break;
                case SCREEN_DARKMODE: darkmodeRenderBottom(buf, transVal); break;
                case SCREEN_LED: ledRenderBottom(buf, transVal); break;
            }
        } else {
            UI_TouchBarBackground();
        }

        C3D_FrameEnd(0);
    }

    C2D_TextBufDelete(buf);
    iconsExit();
    fontsSystemCleanup();
    ledExit();
    if (!romfsRc) romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
