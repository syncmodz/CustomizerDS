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

void darkmodeUpdate(u32 kDown, touchPosition* touch, bool touchDown, int* currentScreen) {
    int total = 2 + themeAccentCount();

    if (kDown & KEY_B) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (kDown & KEY_DOWN) s_selected = (s_selected + 1) % total;
    if (kDown & KEY_UP) s_selected = (s_selected - 1 + total) % total;

    if (touchDown) {
        if (touchIn(touch, 10, 36, 145, 26)) {
            s_selected = 0;
            s_segSlideT = 0.0f;
            themeSetDark(false);
            saveTheme();
            return;
        }
        if (touchIn(touch, 165, 36, 145, 26)) {
            s_selected = 1;
            s_segSlideT = 0.0f;
            themeSetDark(true);
            saveTheme();
            return;
        }

        float swSize = 32.0f;
        float gap = 16.0f;
        float startX = 18.0f;
        for (int i = 0; i < themeAccentCount(); i++) {
            int row = i / 3;
            int col = i % 3;
            float sx = startX + col * (swSize + gap);
            float sy = 82.0f + row * (swSize + gap);
            if (touchIn(touch, sx, sy, swSize, swSize)) {
                s_selected = 2 + i;
                themeSetAccentIndex(i);
                saveTheme();
                return;
            }
        }
    }

    if (kDown & KEY_A) {
        if (s_selected == 0) { themeSetDark(false); saveTheme(); }
        else if (s_selected == 1) { themeSetDark(true); saveTheme(); }
        else { themeSetAccentIndex(s_selected - 2); saveTheme(); }
    }

    s_segSlideT = fminf(s_segSlideT + 1.0f/60.0f, 1.0f);
}

static void drawMockupBar(float x, float y, float w, float h,
                           ColorRGBA bg, ColorRGBA accent) {
    UI_RoundRect(x, y, w, h, 4, bg);
    UI_RoundRect(x + 6, y + (h - 8) * 0.5f, w * 0.4f, 8, 4, accent);
}

static void drawMockupLine(float x, float y, float w, ColorRGBA color) {
    UI_RoundRect(x, y, w, 3, 1.5f, color);
}

void darkmodeRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();

    float slideUp = (1.0f - easeOutCubic(g_enterT)) * 8.0f;

    UI_Card(20, 46 + slideUp, 360, 150, 12, g_theme.surface);

    UI_Text(buf, fontsCurrent(), "Visualizacao do Tema", 32, 60 + slideUp, 0.36f, 0.36f, g_theme.textPrimary);

    float mx = 32.0f;
    float my = 80.0f + slideUp;
    drawMockupBar(mx, my, 120, 18, g_theme.surfaceAlt, g_theme.accent);
    drawMockupBar(mx + 128, my, 120, 18, g_theme.surfaceAlt, g_theme.divider);

    ColorRGBA lineColor = g_theme.divider;
    lineColor.a = 180;
    float ly = my + 30.0f;
    drawMockupLine(mx, ly, 260, lineColor);
    drawMockupLine(mx, ly + 8, 180, lineColor);
    drawMockupLine(mx + 190, ly + 8, 70, lineColor);
    drawMockupLine(mx, ly + 16, 220, lineColor);

    UI_RoundRect(mx + 240, ly - 2, 20, 20, 4, g_theme.accent);
    UI_TextCenter(buf, NULL, "A", mx + 250, ly + 1, 0.28f, 0.28f, g_theme.onPrimary);

    ly += 30;
    drawMockupLine(mx, ly, 260, lineColor);
    drawMockupLine(mx, ly + 8, 200, lineColor);
    drawMockupLine(mx + 210, ly + 8, 50, lineColor);

    ColorRGBA modeColor = themeIsDark() ? (ColorRGBA){80, 80, 140, 255} : (ColorRGBA){200, 200, 220, 255};
    ColorRGBA dotColor = themeIsDark() ? (ColorRGBA){100, 100, 200, 255} : (ColorRGBA){180, 180, 200, 255};
    UI_RoundRect(mx + 240, my + 32, 20, 20, 10, modeColor);
    UI_RoundRect(mx + 244, my + 36, 12, 12, 6, dotColor);

    UI_Text(buf, NULL, themeIsDark() ? "Escuro" : "Claro", 32, 162 + slideUp, 0.28f, 0.28f, g_theme.textSecondary);
    UI_Text(buf, NULL, themeAccentName(themeGetAccentIndex()), 120, 162 + slideUp, 0.28f, 0.28f, g_theme.accent);

    UI_Pill(260, 164 + slideUp, 92, 22, g_theme.primary, g_theme.onPrimary, buf, NULL,
            themeIsDark() ? "escuro" : "claro", 0.30f, 0.30f);
}

void darkmodeRenderBottom(C2D_TextBuf buf) {
    UI_BackgroundBottom();

    const char* modeLabels[] = { "Claro", "Escuro" };
    int modeSel = themeIsDark() ? 1 : 0;
    UI_SegmentedControl(buf, NULL, 30, 36, 260, 26, modeLabels, 2, modeSel, s_segSlideT);

    float swSize = 32.0f;
    float gap = 16.0f;
    float startX = 18.0f;

    for (int i = 0; i < themeAccentCount(); i++) {
        int row = i / 3;
        int col = i % 3;
        float sx = startX + col * (swSize + gap);
        float sy = 82.0f + row * (swSize + gap);
        bool selected = (s_selected == 2 + i);
        bool active = (themeGetAccentIndex() == i);
        float pulse = selected ? 0.04f * sinf(uiFrameTime() * 4.0f) + 0.04f : 0.0f;
        UI_RoundSwatch(sx, sy, swSize * 0.5f, themeAccentColor(i), selected || active, pulse);
    }

    int activeIdx = themeGetAccentIndex();
    int activeRow = activeIdx / 3;
    int activeCol = activeIdx % 3;
    float labelY = 82.0f + activeRow * (swSize + gap) + swSize + 12;
    UI_TextCenter(buf, NULL, themeAccentName(activeIdx), startX + activeCol * (swSize + gap) + swSize * 0.5f,
                  labelY, 0.24f, 0.24f, g_theme.accent);

    UI_KeyHelp(buf, NULL, "A aplicar  B voltar", "START sair");
}
