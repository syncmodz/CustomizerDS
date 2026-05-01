#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "menu.h"
#include "assets.h"
#include "fonts.h"
#include "darkmode.h"
#include "led.h"
#include "config.h"

C3D_RenderTarget *topTarget, *botTarget;

int main() {
    gfxInitDefault();
    hidInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    int currentScreen = SCREEN_MAIN_MENU;
    menuInit();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32(20, 20, 30, 255));
        C2D_TargetClear(botTarget, C2D_Color32(20, 20, 30, 255));

        C2D_SceneBegin(topTarget);
        switch (currentScreen) {
            case SCREEN_MAIN_MENU:
                menuRender(kDown, kHeld, &currentScreen);
                break;
            case SCREEN_ASSETS:
                assetsRender(kDown, kHeld, &currentScreen);
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
        if (!buf) {
            // Apenas pular a renderização da tela inferior
            // C3D_FrameEnd será chamado normalmente abaixo
        } else {
            C2D_Text info;
            C2D_TextParse(&info, buf, "Pressione START para sair");
            C2D_TextOptimize(&info);
            C2D_DrawText(&info, 0.3f, 10.0f, 200.0f, 0.3f, 0.3f, C2D_Color32(200, 200, 200, 255));
            C2D_TextBufDelete(buf);
        }

        C3D_FrameEnd(0);
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
