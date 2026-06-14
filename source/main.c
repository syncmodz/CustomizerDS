#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds/services/mcuhwc.h>
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
    hidInit();
    Result romfsRc = romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!topTarget) { C2D_Fini(); C3D_Fini(); if (!romfsRc) romfsExit(); gfxExit(); return 1; }
    botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!botTarget) { C2D_Fini(); C3D_Fini(); if (!romfsRc) romfsExit(); gfxExit(); return 1; }

    animInit();
    themeInit();
    fontsSystemInit();

    int currentScreen = SCREEN_MAIN_MENU;
    menuInit();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, g_theme.backgroundTop);
        C2D_TargetClear(botTarget, g_theme.background);

        C2D_SceneBegin(topTarget);
        switch (currentScreen) {
            case SCREEN_MAIN_MENU:
                menuRender(kDown, kHeld, &currentScreen);
                break;
            case SCREEN_FONTS:
                fontsRender(kDown, kHeld, &currentScreen);
                break;
            case SCREEN_DARKMODE:
                darkmodeRender(kDown, kHeld, &currentScreen);
                break;
            case SCREEN_LED:
                ledRender(kDown, kHeld, &currentScreen);
                break;
        }

        C2D_SceneBegin(botTarget);
        C2D_TextBuf buf = C2D_TextBufNew(1024);
        if (buf) {
            UI_Footer(buf, NULL, "START para sair", NULL);
            C2D_TextBufDelete(buf);
        }

        C3D_FrameEnd(0);
    }

    fontsSystemCleanup();
    mcuHwcExit();
    if (!romfsRc) romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
