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

C3D_RenderTarget *topTarget, *botTarget;

static void debugOverlay(C2D_TextBuf buf, AppInput* in, int screen, bool startupDone, float startupT) {
    char line[6][64];
    int sel = 0;
    switch (screen) {
        case SCREEN_MAIN_MENU: sel = menuSelected(); break;
        case SCREEN_FONTS: sel = fontsSelected(); break;
        case SCREEN_DARKMODE: sel = darkmodeSelected(); break;
        case SCREEN_LED: sel = ledSelected(); break;
    }
    snprintf(line[0], sizeof(line[0]), "SCR:%d SEL:%d SD:%d ST:%.2f", screen, sel, startupDone, startupT);
    snprintf(line[1], sizeof(line[1]), "DN:0x%08lX HE:0x%08lX", (unsigned long)in->down, (unsigned long)in->held);
    snprintf(line[2], sizeof(line[2]), "TP:%d,%d d:%d h:%d", in->touchPX, in->touchPY, in->touchDown, in->touchHeld);
    snprintf(line[3], sizeof(line[3]), "UP:%d DN:%d LT:%d RT:%d", in->up, in->downNav, in->left, in->right);
    snprintf(line[4], sizeof(line[4]), "A:%d B:%d START:%d SEL:%d", in->confirm, in->back, in->start, in->debug);
    snprintf(line[5], sizeof(line[5]), "CPAD dx:%d dy:%d", 0, 0);
    float ly = SCREEN_BOT_HEIGHT - 120;
    ColorRGBA dbgBg = {0, 0, 0, 200};
    UI_Fill(0, ly, SCREEN_BOT_WIDTH, 120, dbgBg);
    for (int i = 0; i < 5; i++) {
        UI_Text(buf, NULL, line[i], 2, ly + 2 + i * 10, 0.22f, 0.22f, (ColorRGBA){0, 255, 0, 255});
    }
}

void onScreenEnter(int screen) {
    uiScreenEnter();
    switch (screen) {
        case SCREEN_MAIN_MENU: menuInit(); break;
        case SCREEN_FONTS: fontsInit(); break;
        case SCREEN_DARKMODE: darkmodeInit(); break;
        case SCREEN_LED: ledEnter(); break;
    }
}

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
    menuInit();
    fontsInit();
    darkmodeInit();
    ledInit();
    onScreenEnter(currentScreen);

    bool debugMode = false;
    bool startupDone = false;
    float startupT = 0.0f;

    while (aptMainLoop()) {
        AppInput in;
        inputRead(&in);

        if (!startupDone) {
            bool anyInput = (in.down & 0xFFFFF) != 0;
            if (anyInput) {
                startupDone = true;
                startupT = 1.0f;
            } else {
                startupT += 1.0f / 60.0f;
                if (startupT >= 1.0f) startupDone = true;
            }
        } else {
            if (in.debug) debugMode = !debugMode;

            if (in.start) break;

            switch (currentScreen) {
                case SCREEN_MAIN_MENU:
                    menuUpdate(&in, &currentScreen);
                    break;
                case SCREEN_FONTS:
                    fontsUpdate(&in, &currentScreen);
                    break;
                case SCREEN_DARKMODE:
                    darkmodeUpdate(&in, &currentScreen);
                    break;
                case SCREEN_LED:
                    ledUpdate(&in, &currentScreen);
                    break;
            }
        }

        uiFrameTick();

        if (currentScreen != prevScreen) {
            onScreenEnter(currentScreen);
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
                case SCREEN_MAIN_MENU: menuRenderTop(buf); break;
                case SCREEN_FONTS: fontsRenderTop(buf); break;
                case SCREEN_DARKMODE: darkmodeRenderTop(buf); break;
                case SCREEN_LED: ledRenderTop(buf); break;
            }
        } else {
            UI_BackgroundTop();
            UI_StartupLogo(startupT);
        }

        C2D_SceneBegin(botTarget);
        if (startupDone) {
            switch (currentScreen) {
                case SCREEN_MAIN_MENU: menuRenderBottom(buf); break;
                case SCREEN_FONTS: fontsRenderBottom(buf); break;
                case SCREEN_DARKMODE: darkmodeRenderBottom(buf); break;
                case SCREEN_LED: ledRenderBottom(buf); break;
            }
            if (debugMode) {
                debugOverlay(buf, &in, currentScreen, startupDone, startupT);
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
