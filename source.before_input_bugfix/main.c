#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "menu.h"
#include "fonts.h"
#include "darkmode.h"
#include "led.h"
#include "config.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"

C3D_RenderTarget *topTarget, *botTarget;

int main() {
    gfxInitDefault();
    Result romfsRc = romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!topTarget) { C2D_Fini(); C3D_Fini(); if (!romfsRc) romfsExit(); gfxExit(); return 1; }
    botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!botTarget) { C2D_Fini(); C3D_Fini(); if (!romfsRc) romfsExit(); gfxExit(); return 1; }

    C2D_Prepare();
    animInit();
    themeInit();
    fontsSystemInit();

    int currentScreen = SCREEN_MAIN_MENU;
    int prevScreen = currentScreen;
    int targetScreen = currentScreen;
    menuInit();
    fontsInit();
    darkmodeInit();
    ledInit();
    uiScreenEnter();

    bool startupDone = false;
    float startupT = 0.0f;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        touchPosition touch;
        hidTouchRead(&touch);
        bool touchDown = (kDown & KEY_TOUCH) != 0;

        if (kDown & KEY_START) break;

        if (!startupDone) {
            startupT += 1.0f / 60.0f;
            if (startupT >= 1.0f || kDown) {
                startupDone = true;
                startupT = 1.0f;
            }
        } else {
            switch (currentScreen) {
                case SCREEN_MAIN_MENU:
                    menuUpdate(kDown, &touch, touchDown, &currentScreen);
                    break;
                case SCREEN_FONTS:
                    fontsUpdate(kDown, &touch, touchDown, &currentScreen);
                    break;
                case SCREEN_DARKMODE:
                    darkmodeUpdate(kDown, &touch, touchDown, &currentScreen);
                    break;
                case SCREEN_LED:
                    ledUpdate(kDown, kHeld, &touch, touchDown, &currentScreen);
                    break;
            }
        }

        uiFrameTick();

        if (currentScreen != prevScreen) {
            uiScreenEnter();
            prevScreen = currentScreen;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(
            g_theme.backgroundTop.r, g_theme.backgroundTop.g,
            g_theme.backgroundTop.b, g_theme.backgroundTop.a));
        C2D_TargetClear(botTarget, C2D_Color32(
            g_theme.background.r, g_theme.background.g,
            g_theme.background.b, g_theme.background.a));

        C2D_TextBuf buf = C2D_TextBufNew(4096);
        if (!buf) {
            C3D_FrameEnd(0);
            continue;
        }

        C2D_SceneBegin(topTarget);
        if (startupDone) {
            switch (currentScreen) {
                case SCREEN_MAIN_MENU:
                    menuRenderTop(buf);
                    break;
                case SCREEN_FONTS:
                    fontsRenderTop(buf);
                    break;
                case SCREEN_DARKMODE:
                    darkmodeRenderTop(buf);
                    break;
                case SCREEN_LED:
                    ledRenderTop(buf);
                    break;
            }
        } else {
            UI_BackgroundTop();
            UI_StartupLogo(startupT);
        }

        C2D_SceneBegin(botTarget);
        if (startupDone) {
            switch (currentScreen) {
                case SCREEN_MAIN_MENU:
                    menuRenderBottom(buf);
                    break;
                case SCREEN_FONTS:
                    fontsRenderBottom(buf);
                    break;
                case SCREEN_DARKMODE:
                    darkmodeRenderBottom(buf);
                    break;
                case SCREEN_LED:
                    ledRenderBottom(buf);
                    break;
            }
        } else {
            UI_TouchBarBackground();
        }

        C2D_TextBufDelete(buf);
        C3D_FrameEnd(0);
    }

    fontsSystemCleanup();
    ledExit();
    if (!romfsRc) romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
