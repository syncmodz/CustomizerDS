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
#include "transitions.h"
#include "compositor.h"
#include "lang.h"

/* secao 7: Laboratorio de transicoes. 1 = liga (atalho L+R+SELECT cicla as 10
 * e re-dispara uma troca "dummy" pra assistir); 0 = desliga no release. */
#ifndef DEV_TRANSITION_LAB
#define DEV_TRANSITION_LAB 1
#endif

C3D_RenderTarget *topTarget, *botTarget;

#if DEV_TRANSITION_LAB
/* HUD do Laboratorio: barra no rodape da tela de cima com nome/easing/duracao
 * da transicao atual, pra calibrar por sensacao (secao 7). */
static void drawLabHud(C2D_TextBuf buf, TransitionID id) {
    UI_Fill(0.0f, 200.0f, SCREEN_TOP_WIDTH, 40.0f, (ColorRGBA){0, 0, 0, 190});
    UI_Fill(0.0f, 200.0f, SCREEN_TOP_WIDTH, 1.0f, (ColorRGBA){255, 255, 255, 40});
    UI_Text(buf, NULL, transName(id), 8.0f, 204.0f, 0.42f, 0.42f, (ColorRGBA){255, 255, 255, 255});
    char line[96];
    snprintf(line, sizeof(line), "%s - %dms", transEaseName(id), (int)(transDuration(id) * 1000.0f));
    UI_Text(buf, NULL, line, 8.0f, 224.0f, 0.32f, 0.32f, (ColorRGBA){190, 200, 215, 255});
    UI_TextRight(buf, NULL, "botao: proxima  |  L+R+SELECT: sair",
                 SCREEN_TOP_WIDTH - 8.0f, 224.0f, 0.30f, 0.30f, (ColorRGBA){150, 160, 175, 255});
}
#endif

/* Dispatch de render por tela -- extraido do switch inline pra poder
 * desenhar a MESMA tela tanto no alvo offscreen (compositor) quanto direto no
 * alvo real, sem duplicar o switch em cada caminho. */
static void renderTopScreen(int screen, C2D_TextBuf buf, float transVal,
                            float slideX, float fadeA, float scaleM) {
    switch (screen) {
        case SCREEN_MAIN_MENU: menuRenderTop(buf, transVal, slideX, fadeA, scaleM); break;
        case SCREEN_FONTS:     fontsRenderTop(buf, transVal, slideX, fadeA, scaleM); break;
        case SCREEN_DARKMODE:  darkmodeRenderTop(buf, transVal, slideX, fadeA, scaleM); break;
        case SCREEN_LED:       ledRenderTop(buf, transVal, slideX, fadeA, scaleM); break;
    }
}

static void renderBottomScreen(int screen, C2D_TextBuf buf, float transVal,
                               float slideX, float fadeA, float scaleM) {
    switch (screen) {
        case SCREEN_MAIN_MENU: menuRenderBottom(buf, transVal, slideX, fadeA, scaleM); break;
        case SCREEN_FONTS:     fontsRenderBottom(buf, transVal, slideX, fadeA, scaleM); break;
        case SCREEN_DARKMODE:  darkmodeRenderBottom(buf, transVal, slideX, fadeA, scaleM); break;
        case SCREEN_LED:       ledRenderBottom(buf, transVal, slideX, fadeA, scaleM); break;
    }
}

/* Desenha uma tela INTEIRA e NEUTRA (sem offset de transicao) num alvo
 * offscreen do compositor -- o movimento/fade fica todo por conta da
 * composicao A/B depois. Args 1,0,1,1 = "assentada" (secao 4: o stagger por
 * elemento nao dispara durante a troca de tela). */
static void drawScreenToTarget(C3D_RenderTarget* t, bool isTop, int screen,
                               C2D_TextBuf buf, u32 bg) {
    /* TODO(hardware): clear offscreen pode vazar no console real; UI_*Background
     * ja cobre 100% opaco, entao la trocar por confiar nesse fill (secao 2.3). */
    C2D_TargetClear(t, bg);
    C2D_SceneBegin(t);
    C2D_ViewReset();
    if (isTop) renderTopScreen(screen, buf, 1.0f, 0.0f, 1.0f, 1.0f);
    else       renderBottomScreen(screen, buf, 1.0f, 0.0f, 1.0f, 1.0f);
}

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

    /* Compositor offscreen (secao 2). Se a alocacao de VRAM falhar, seguimos
     * com o caminho de render direto (compositorOK=false) -- degrada sem
     * crashar em vez de abortar o app. */
    bool compositorOK = compositorInit();

    themeInit();
    fontsSystemInit();
    iconsInit();

    /* i18n (§i18n.4/5): idioma salvo (reserved[0]) tem prioridade; se for o
     * sentinela CFG_LANG_UNSET, langInit pega o idioma do sistema via CFGU. */
    ConfigData cfgLang;
    configLoad(&cfgLang);
    langInit(cfgLang.reserved[0] == CFG_LANG_UNSET ? -1 : (int)cfgLang.reserved[0]);

    int currentScreen = SCREEN_MAIN_MENU;
    fontsInit();
    darkmodeInit();
    ledInit();
    uiScreenEnter();

    /* 8192 (era 4096): no 1o frame de uma troca o compositor desenha ate 4
     * telas no mesmo buf (velha top+bottom em A + nova top+bottom em B). */
    C2D_TextBuf buf = C2D_TextBufNew(8192);
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
    const float STARTUP_DURATION = 1.15f; /* spec 3.1: ultimo card em 760ms + 340ms = 1100ms + folga */

    /* Pool de transicoes (spec v6 secao 2): navDir +1 = abrir funcao
     * (direita), -1 = voltar ao menu (esquerda). transClock e um relogio
     * simples desde a ultima navegacao; currentTrans e sorteada do pool
     * (transPickNext, nunca repete a anterior) a cada troca de tela.
     * transEval() devolve, por tela, as 2 camadas (velha A saindo + nova B
     * entrando) que o compositor (secao 2) compoe a partir das texturas
     * offscreen. transSnapPending marca o 1o frame da troca, quando a tela
     * velha e capturada uma unica vez no alvo A. */
    float transClock = 0.0f;
    bool transActive = false;
    bool transSnapPending = false;
    int transOldScreen = SCREEN_MAIN_MENU;
    int navDir = 1;
    TransitionID currentTrans = TRANS_CROSSFADE;
    srand((unsigned)osGetTime());

#if DEV_TRANSITION_LAB
    bool labMode = false;
    bool labComboWas = false;
    int  labIdx = 0;
#endif

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

#if DEV_TRANSITION_LAB
            /* Laboratorio (secao 7): L+R+SELECT (edge) entra/sai. No modo lab,
             * qualquer botao cicla pra proxima das 10 e re-dispara uma troca
             * "dummy" (mesma tela em A e B) pra assistir a curva; a navegacao
             * normal fica suspensa. */
            bool comboHeld = (in.held & KEY_L) && (in.held & KEY_R) && (in.held & KEY_SELECT);
            if (comboHeld && !labComboWas) {
                labMode = !labMode;
                if (labMode) {
                    labIdx = 0;
                    currentTrans = (TransitionID)labIdx;
                    transClock = 0.0f; transActive = true; transSnapPending = true;
                    transOldScreen = currentScreen; navDir = 1;
                }
            }
            labComboWas = comboHeld;
            if (labMode) {
                u32 cycleKeys = KEY_A | KEY_B | KEY_X | KEY_Y |
                                KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT | KEY_TOUCH;
                if (!comboHeld && (in.down & cycleKeys)) {
                    labIdx = (labIdx + 1) % TRANS_COUNT;
                    currentTrans = (TransitionID)labIdx;
                    transClock = 0.0f; transActive = true; transSnapPending = true;
                    transOldScreen = currentScreen;
                    navDir = (navDir > 0) ? -1 : 1; /* alterna pra ver os 2 sentidos */
                }
            }
            if (!labMode)
#endif
            {
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
                /* Direcao: abrir funcao (menu -> tela) = direita; voltar
                 * (tela -> menu, via B) = esquerda. */
                navDir = (currentScreen == SCREEN_MAIN_MENU) ? -1 : 1;
                transClock = 0.0f;
                transActive = true;
                transSnapPending = true;      /* capturar a tela velha (A) no 1o frame */
                transOldScreen = oldScreen;
                currentTrans = transPickNext();
                /* NAO chamamos uiScreenEnter() aqui: o compositor move a tela
                 * NOVA como bloco unico, entao ela deve aparecer ASSENTADA. O
                 * stagger por elemento reintroduziria "borracha" durante a
                 * troca (secao 4) -- ele fica so na entrada inicial (startup). */
                inputResetRepeat();
                switch (currentScreen) {
                    case SCREEN_MAIN_MENU: menuInit(); break;
                    case SCREEN_FONTS: fontsInit(); break;
                    case SCREEN_DARKMODE: darkmodeInit(); break;
                    case SCREEN_LED: ledEnter(); break;
                }
            }
            } /* fecha o bloco de navegacao normal (so roda fora do modo lab) */
        }

        uiFrameTick(dt);

        if (transActive) {
            transClock += dt;
            if (transClock >= transDuration(currentTrans)) transActive = false;
        }

        u32 bgTop = C2D_Color32(g_theme.backgroundTop.r, g_theme.backgroundTop.g,
                                g_theme.backgroundTop.b, g_theme.backgroundTop.a);
        u32 bgBot = C2D_Color32(g_theme.background.r, g_theme.background.g,
                                g_theme.background.b, g_theme.background.a);

        /* Compositor (secao 2/3): durante a troca, a tela VELHA (A) e a NOVA
         * (B) sao desenhadas em alvos offscreen e compostas no alvo real --
         * dai sai o fade-out de verdade e o movimento em bloco. Ocioso desenha
         * direto na tela real (sem custo offscreen, secao 8). */
        bool useComposite = startupDone && compositorOK;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TextBufClear(buf);

        if (useComposite && transActive) {
            /* 1) snapshot da tela velha -> A, so no 1o frame (a velha e
             *    estatica durante a transicao -- secao 2.2). */
            if (transSnapPending) {
                drawScreenToTarget(compositorTop(true), true, transOldScreen, buf, bgTop);
                drawScreenToTarget(compositorBot(true), false, transOldScreen, buf, bgBot);
                transSnapPending = false;
            }

            /* 2) tela nova -> B, todo frame (neutra). */
            drawScreenToTarget(compositorTop(false), true, currentScreen, buf, bgTop);
            drawScreenToTarget(compositorBot(false), false, currentScreen, buf, bgBot);

            /* 3) avalia a transicao por tela (W difere: 400 topo / 320 baixo). */
            TransitionFrame tfTop, tfBot;
            transEval(currentTrans, transClock, navDir, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, &tfTop);
            transEval(currentTrans, transClock, navDir, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, &tfBot);

            /* 4) compoe A+B nos alvos reais. */
            C2D_SceneBegin(topTarget);
            C2D_TargetClear(topTarget, bgTop);
            C2D_ViewReset();
            compositorComposite(true, &tfTop, compositorTopImage(true), compositorTopImage(false));

            C2D_SceneBegin(botTarget);
            C2D_TargetClear(botTarget, bgBot);
            C2D_ViewReset();
            compositorComposite(false, &tfBot, compositorBotImage(true), compositorBotImage(false));
        } else {
            /* Ocioso (sem transicao) ou startup/compositor indisponivel:
             * desenha direto no alvo real. As telas recebem args neutros
             * (1,0,1,1) -- a animacao de troca agora e so do compositor. */
            C2D_TargetClear(topTarget, bgTop);
            C2D_TargetClear(botTarget, bgBot);

            C2D_SceneBegin(topTarget);
            C2D_ViewReset();
            if (startupDone) {
                renderTopScreen(currentScreen, buf, 1.0f, 0.0f, 1.0f, 1.0f);
            } else {
                UI_TopBackground();
                UI_StartupLogo(buf, startupT);
            }

            C2D_SceneBegin(botTarget);
            C2D_ViewReset();
            if (startupDone) {
                renderBottomScreen(currentScreen, buf, 1.0f, 0.0f, 1.0f, 1.0f);
            } else {
                menuRenderStartupPills(buf, startupT);
            }
        }

#if DEV_TRANSITION_LAB
        if (labMode && startupDone) {
            C2D_SceneBegin(topTarget);
            C2D_ViewReset();
            drawLabHud(buf, currentTrans);
        }
#endif

        C3D_FrameEnd(0);
    }

    C2D_TextBufDelete(buf);
    compositorExit();
    iconsExit();
    fontsSystemCleanup();
    ledExit();
    if (!romfsRc) romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
