#include "darkmode.h"
#include "common.h"
#include "theme.h"
#include "config.h"
#include "ui.h"
#include "anim.h"
#include "fonts.h"
#include <math.h>

static int s_selected = 0;
static float s_segSlideT = 1.0f;

int darkmodeSelected(void) { return s_selected; }

static void saveTheme(void) {
    ConfigData cfg;
    configLoad(&cfg);
    cfg.darkMode = themeIsDark() ? 1 : 0;
    cfg.accentIndex = (u8)themeGetAccentIndex();
    configSave(&cfg);
}

void darkmodeInit(void) {
    s_selected = 0;
}

void darkmodeUpdate(const AppInput* in, int* currentScreen) {
    int total = 2 + themeAccentCount();

    if (in->back) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (in->downNav) s_selected = (s_selected + 1) % total;
    if (in->up) s_selected = (s_selected - 1 + total) % total;

    if (in->left && s_selected >= 2) {
        int idx = (s_selected - 2 - 1 + themeAccentCount()) % themeAccentCount();
        s_selected = 2 + idx;
        themeSetAccentIndex(idx);
        saveTheme();
    }
    if (in->right && s_selected >= 2) {
        int idx = (s_selected - 2 + 1) % themeAccentCount();
        s_selected = 2 + idx;
        themeSetAccentIndex(idx);
        saveTheme();
    }

    if (in->touchDown || in->touchHeld) {
        float segX = 95.0f, segY = 30.0f, segW = 130.0f, segH = 30.0f;
        if (in->touchPY >= segY && in->touchPY < segY + segH) {
            if (in->touchPX >= segX && in->touchPX < segX + segW) {
                if (in->touchDown) {
                    s_selected = 0;
                    s_segSlideT = 0.0f;
                    themeSetDark(false);
                    saveTheme();
                }
                return;
            }
            if (in->touchPX >= segX + segW && in->touchPX < segX + segW * 2) {
                if (in->touchDown) {
                    s_selected = 1;
                    s_segSlideT = 0.0f;
                    themeSetDark(true);
                    saveTheme();
                }
                return;
            }
        }

        float swSize = 44.0f;
        float gap = 16.0f;
        float startX = 16.0f;
        float sy = 78.0f;
        for (int i = 0; i < themeAccentCount(); i++) {
            float sx = startX + i * (swSize + gap);
            if (in->touchPY >= sy && in->touchPY < sy + swSize &&
                in->touchPX >= sx && in->touchPX < sx + swSize) {
                if (in->touchDown) {
                    s_selected = 2 + i;
                    themeSetAccentIndex(i);
                    saveTheme();
                }
                return;
            }
        }
    }

    if (in->confirm) {
        if (s_selected == 0) { themeSetDark(false); saveTheme(); }
        else if (s_selected == 1) { themeSetDark(true); saveTheme(); }
        else {
            int idx = s_selected - 2;
            if (idx >= 0 && idx < themeAccentCount()) {
                themeSetAccentIndex(idx);
                saveTheme();
            }
        }
    }

    s_segSlideT = fminf(s_segSlideT + 1.0f/60.0f, 1.0f);
}

void darkmodeRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();
    float et = easeOutCubic(g_enterT);
    float slideUp = (1.0f - et) * 10.0f;

    UI_GlassPanel(20, 40 + slideUp, 360, 160, 14,
                  g_theme.surface,
                  (ColorRGBA){255, 255, 255, 20},
                  (ColorRGBA){255, 255, 255, 12});

    UI_Text(buf, fontsCurrent(), "Visual do Tema", 32, 54 + slideUp, 0.40f, 0.40f, g_theme.textPrimary);

    float mx = 32.0f;
    float my = 70.0f + slideUp;
    ColorRGBA modeBg = themeIsDark() ? (ColorRGBA){30, 35, 50, 255} : (ColorRGBA){220, 222, 235, 255};
    ColorRGBA accentLine = g_theme.accent;
    UI_RoundRect(mx, my, 120, 50, 8, modeBg);
    UI_RoundRect(mx + 8, my + 10, 104, 6, 3, accentLine);
    float ly = my + 26;
    ColorRGBA mockLine = themeIsDark() ? (ColorRGBA){60, 65, 85, 200} : (ColorRGBA){190, 192, 205, 200};
    for (int i = 0; i < 3; i++) {
        UI_RoundRect(mx + 12, ly + i * 8, 60 + i * 15, 4, 2, mockLine);
    }
    UI_RoundRect(mx + 140, my, 120, 50, 8, modeBg);
    UI_RoundRect(mx + 148, my + 10, 104, 6, 3, accentLine);
    for (int i = 0; i < 3; i++) {
        UI_RoundRect(mx + 152, ly + i * 8, 40 + i * 10, 4, 2, mockLine);
    }
    UI_TextCenter(buf, NULL, themeIsDark() ? "Escuro" : "Claro", 80, my + 58, 0.28f, 0.28f, g_theme.textSecondary);
    UI_Text(buf, NULL, themeAccentName(themeGetAccentIndex()), 180, my + 58, 0.28f, 0.28f, g_theme.accent);
    ColorRGBA dotC = themeAccentColor(themeGetAccentIndex());
    float dpulse = (sinf(uiFrameTime() * 2.0f) + 1.0f) * 0.5f;
    dotC.a = (u8)(80 + (u8)(80 * dpulse));
    UI_RoundRect(mx + 280, my + 8, 36, 36, 10, dotC);
}

void darkmodeRenderBottom(C2D_TextBuf buf) {
    UI_TouchBarBackground();
    float et = easeOutCubic(g_enterT);
    const char* modeLabels[] = { "Claro", "Escuro" };
    int modeSel = themeIsDark() ? 1 : 0;
    float segW = 130.0f;
    float segH = 30.0f;
    float segX = 95.0f;
    float segY = 30.0f;

    UI_RoundRect(segX, segY, segW * 2, segH, segH * 0.5f, (ColorRGBA){24, 28, 38, 245});
    float targetX = segX + modeSel * segW + 3;
    float curX = targetX;
    if (s_segSlideT < 1.0f) {
        static float s_morphX = 0.0f;
        if (s_morphX == 0.0f) s_morphX = targetX;
        s_morphX = animApproach(s_morphX, targetX, 0.22f);
        curX = s_morphX;
    }
    ColorRGBA selBg = g_theme.accent;
    selBg.a = 40;
    UI_RoundRect(curX, segY + 2, segW - 6, segH - 4, (segH - 4) * 0.5f, selBg);
    ColorRGBA selBorder = g_theme.accent;
    selBorder.a = 70;
    UI_RoundRect(curX, segY + 2, segW - 6, segH - 4, (segH - 4) * 0.5f, selBorder);
    for (int i = 0; i < 2; i++) {
        ColorRGBA tc = (i == modeSel) ? g_theme.onPrimary : g_theme.textPrimary;
        UI_TextCenter(buf, NULL, modeLabels[i], segX + segW * i + segW * 0.5f, segY + 7, 0.28f, 0.28f, tc);
    }

    float swSize = 44.0f;
    float gap = 16.0f;
    float startX = 16.0f;
    for (int i = 0; i < themeAccentCount(); i++) {
        float sx = startX + i * (swSize + gap);
        float sy = 78.0f;
        float appearT = clampf((et * 2.5f - i * 0.08f), 0.0f, 1.0f);
        float sa = easeOutCubic(appearT);
        bool active = (themeGetAccentIndex() == i);
        ColorRGBA ac = themeAccentColor(i);
        float pulse = active ? 0.04f * sinf(uiFrameTime() * 4.0f) + 0.04f : 0.0f;
        if (active) {
            ColorRGBA ring = themeMix(ac, g_theme.onPrimary, pulse * 0.3f);
            UI_RoundRect(sx - 4 + (1.0f - sa) * 8, sy - 4 + (1.0f - sa) * 8,
                         swSize + 8, swSize + 8, (swSize + 8) * 0.5f, ring);
        }
        UI_RoundRect(sx + (1.0f - sa) * 8, sy + (1.0f - sa) * 8,
                     swSize, swSize, swSize * 0.5f, ac);
        if (active) {
            ColorRGBA shine = {255, 255, 255, 35};
            UI_RoundRect(sx + (1.0f - sa) * 8 + 3, sy + (1.0f - sa) * 8 + 3,
                         swSize - 6, swSize * 0.35f, 6, shine);
        }
    }
    UI_TextCenter(buf, NULL, themeAccentName(themeGetAccentIndex()),
                  startX + themeGetAccentIndex() * (swSize + gap) + swSize * 0.5f,
                  132, 0.24f, 0.24f, g_theme.accent);
    UI_KeyHelp(buf, NULL, "A aplicar  B voltar  <- accent ->", "START sair");
}
