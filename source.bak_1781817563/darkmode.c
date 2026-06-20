#include "darkmode.h"
#include "common.h"
#include "theme.h"
#include "config.h"
#include "ui.h"
#include "anim.h"
#include "fonts.h"
#include "color_picker.h"
#include <math.h>

static int s_selected = 0;
static float s_segSlideT = 1.0f;
static float s_segFromX = 20.0f;
static HexPicker s_hex;
static bool s_hexEditing = false;

#define ACCENT_SLOT_COUNT (themeAccentCount() + 1)

#define MODE_SEG_X 20.0f
#define MODE_SEG_Y 28.0f
#define MODE_SEG_W 140.0f
#define MODE_SEG_H 32.0f

static float modeTargetX(int modeSel) {
    return MODE_SEG_X + modeSel * MODE_SEG_W + 3.0f;
}

int darkmodeSelected(void) { return s_selected; }

static void saveTheme(void) {
    ConfigData cfg;
    configLoad(&cfg);
    cfg.darkMode = themeIsDark() ? 1 : 0;
    cfg.accentIndex = (u8)themeGetAccentIndex();
    cfg.customAccentFlag = themeAccentIsCustom() ? 1 : 0;
    ColorRGBA cc = themeGetCustomAccent();
    cfg.customR = cc.r;
    cfg.customG = cc.g;
    cfg.customB = cc.b;
    configSave(&cfg);
}

void darkmodeInit(void) {
    s_selected = 0;
    s_segFromX = modeTargetX(themeIsDark() ? 1 : 0);
    s_segSlideT = 1.0f;
    s_hexEditing = false;
}

void darkmodeUpdate(const AppInput* in, int* currentScreen) {
    if (s_hexEditing) {
        bool applied = false;
        bool stillActive = hexPickerHandleInput(&s_hex, in, &applied);
        if (!stillActive) {
            s_hexEditing = false;
            if (applied) {
                themeSetCustomAccent(hexPickerColor(&s_hex));
                saveTheme();
            }
        }
        return;
    }

    int total = 2 + ACCENT_SLOT_COUNT;

    if (in->back) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (in->downNav) s_selected = (s_selected + 1) % total;
    if (in->up) s_selected = (s_selected - 1 + total) % total;

    if (in->left && s_selected >= 2) {
        int idx = (s_selected - 2 - 1 + ACCENT_SLOT_COUNT) % ACCENT_SLOT_COUNT;
        s_selected = 2 + idx;
        if (idx < themeAccentCount()) {
            themeSetAccentIndex(idx);
            saveTheme();
        }
    }
    if (in->right && s_selected >= 2) {
        int idx = (s_selected - 2 + 1) % ACCENT_SLOT_COUNT;
        s_selected = 2 + idx;
        if (idx < themeAccentCount()) {
            themeSetAccentIndex(idx);
            saveTheme();
        }
    }

    if (in->touchDown || in->touchHeld) {
        float segX = MODE_SEG_X, segY = MODE_SEG_Y, segW = MODE_SEG_W, segH = MODE_SEG_H;
        if (in->touchPY >= segY && in->touchPY < segY + segH) {
            if (in->touchPX >= segX && in->touchPX < segX + segW) {
                if (in->touchDown) {
                    s_selected = 0;
                    s_segFromX = modeTargetX(themeIsDark() ? 1 : 0);
                    s_segSlideT = 0.0f;
                    themeSetDark(false);
                    saveTheme();
                }
                return;
            }
            if (in->touchPX >= segX + segW && in->touchPX < segX + segW * 2) {
                if (in->touchDown) {
                    s_selected = 1;
                    s_segFromX = modeTargetX(themeIsDark() ? 1 : 0);
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
        for (int i = 0; i <= themeAccentCount(); i++) {
            float sx = startX + i * (swSize + gap);
            if (in->touchPY >= sy && in->touchPY < sy + swSize &&
                in->touchPX >= sx && in->touchPX < sx + swSize) {
                if (in->touchDown) {
                    s_selected = 2 + i;
                    if (i < themeAccentCount()) {
                        themeSetAccentIndex(i);
                        saveTheme();
                    } else {
                        ColorRGBA start = themeAccentIsCustom() ? themeGetCustomAccent()
                                                                 : themeAccentColor(themeGetAccentIndex());
                        hexPickerStart(&s_hex, start);
                        s_hexEditing = true;
                    }
                }
                return;
            }
        }
    }

    if (in->confirm) {
        if (s_selected == 0) {
            s_segFromX = modeTargetX(themeIsDark() ? 1 : 0);
            s_segSlideT = 0.0f;
            themeSetDark(false);
            saveTheme();
        }
        else if (s_selected == 1) {
            s_segFromX = modeTargetX(themeIsDark() ? 1 : 0);
            s_segSlideT = 0.0f;
            themeSetDark(true);
            saveTheme();
        }
        else {
            int idx = s_selected - 2;
            if (idx >= 0 && idx < themeAccentCount()) {
                themeSetAccentIndex(idx);
                saveTheme();
            } else {
                ColorRGBA start = themeAccentIsCustom() ? themeGetCustomAccent()
                                                         : themeAccentColor(themeGetAccentIndex());
                hexPickerStart(&s_hex, start);
                s_hexEditing = true;
            }
        }
    }

    s_segSlideT = fminf(s_segSlideT + 1.0f/60.0f, 1.0f);
}

void darkmodeRenderTop(C2D_TextBuf buf) {
    UI_BackgroundTop();
    float et = easeOutCubic(g_enterT);
    float slideUp = (1.0f - et) * 16.0f;

    UI_GlassPanel(18, 34 + slideUp, 364, 168, 18,
                  g_theme.surface,
                  (ColorRGBA){255, 255, 255, 22},
                  (ColorRGBA){255, 255, 255, 14});

    if (s_hexEditing) {
        hexPickerRenderTop(buf, &s_hex, slideUp);
        return;
    }

    UI_Text(buf, NULL, "Visual do Tema", 32, 52 + slideUp, 0.46f, 0.46f, g_theme.textPrimary);

    float mx = 32.0f;
    float my = 76.0f + slideUp;
    ColorRGBA modeBg = themeIsDark() ? (ColorRGBA){30, 35, 50, 255} : (ColorRGBA){220, 222, 235, 255};
    ColorRGBA accentLine = g_theme.accent;
    UI_RoundFrame(mx, my, 120, 50, 12, modeBg, (ColorRGBA){255, 255, 255, 16});
    UI_RoundRect(mx + 8, my + 10, 104, 6, 3, accentLine);
    float ly = my + 26;
    ColorRGBA mockLine = themeIsDark() ? (ColorRGBA){60, 65, 85, 200} : (ColorRGBA){190, 192, 205, 200};
    for (int i = 0; i < 3; i++) {
        UI_RoundRect(mx + 12, ly + i * 8, 60 + i * 15, 4, 2, mockLine);
    }
    UI_RoundFrame(mx + 140, my, 120, 50, 12, modeBg, (ColorRGBA){255, 255, 255, 16});
    UI_RoundRect(mx + 148, my + 10, 104, 6, 3, accentLine);
    for (int i = 0; i < 3; i++) {
        UI_RoundRect(mx + 152, ly + i * 8, 40 + i * 10, 4, 2, mockLine);
    }
    UI_TextCenter(buf, NULL, themeIsDark() ? "Escuro" : "Claro", 80, my + 58, 0.28f, 0.28f, g_theme.textSecondary);
    const char* accentLabel = themeAccentIsCustom() ? "Custom (hex)" : themeAccentName(themeGetAccentIndex());
    UI_Text(buf, NULL, accentLabel, 180, my + 58, 0.28f, 0.28f, g_theme.accent);
    ColorRGBA dotC = themeAccentIsCustom() ? themeGetCustomAccent() : themeAccentColor(themeGetAccentIndex());
    float dpulse = (sinf(uiFrameTime() * 2.0f) + 1.0f) * 0.5f;
    dotC.a = (u8)(80 + (u8)(80 * dpulse));
    UI_RoundRect(mx + 280, my + 8, 36, 36, 18, dotC);
}

void darkmodeRenderBottom(C2D_TextBuf buf) {
    UI_TouchBarBackground();

    if (s_hexEditing) {
        hexPickerRenderBottom(buf, &s_hex);
        return;
    }

    float et = easeOutCubic(g_enterT);
    const char* modeLabels[] = { "Claro", "Escuro" };
    int modeSel = themeIsDark() ? 1 : 0;
    float segW = MODE_SEG_W;
    float segH = MODE_SEG_H;
    float segX = MODE_SEG_X;
    float segY = MODE_SEG_Y;

    UI_RoundFrame(segX, segY, segW * 2, segH, segH * 0.5f,
                  g_theme.surfaceElevated, g_theme.divider);
    float targetX = modeTargetX(modeSel);
    float curX = lerpf(s_segFromX, targetX, easeOutCubic(s_segSlideT));
    ColorRGBA selBg = g_theme.accent;
    selBg.a = themeIsDark() ? 46 : 64;
    ColorRGBA selBorder = g_theme.accent;
    selBorder.a = themeIsDark() ? 82 : 120;
    UI_RoundFrame(curX, segY + 2, segW - 6, segH - 4, (segH - 4) * 0.5f, selBg, selBorder);
    for (int i = 0; i < 2; i++) {
        ColorRGBA tc = (i == modeSel) ? g_theme.textPrimary : g_theme.textSecondary;
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
        bool active = !themeAccentIsCustom() && (themeGetAccentIndex() == i);
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

    {
        int i = themeAccentCount();
        float sx = startX + i * (swSize + gap);
        float sy = 78.0f;
        float appearT = clampf((et * 2.5f - i * 0.08f), 0.0f, 1.0f);
        float sa = easeOutCubic(appearT);
        bool active = themeAccentIsCustom();
        ColorRGBA ac = themeGetCustomAccent();
        float pulse = active ? 0.04f * sinf(uiFrameTime() * 4.0f) + 0.04f : 0.0f;
        if (active) {
            ColorRGBA ring = themeMix(ac, g_theme.onPrimary, pulse * 0.3f);
            UI_RoundRect(sx - 4 + (1.0f - sa) * 8, sy - 4 + (1.0f - sa) * 8,
                         swSize + 8, swSize + 8, (swSize + 8) * 0.5f, ring);
        }
        UI_RoundRect(sx + (1.0f - sa) * 8, sy + (1.0f - sa) * 8,
                     swSize, swSize, swSize * 0.5f, ac);
        ColorRGBA hexTextColor = themeContrastText(ac);
        UI_TextCenter(buf, NULL, "HEX", sx + swSize * 0.5f, sy + (swSize - 14) * 0.5f, 0.20f, 0.20f, hexTextColor);
    }

    bool customActive = themeAccentIsCustom();
    float labelX = customActive
        ? (startX + themeAccentCount() * (swSize + gap) + swSize * 0.5f)
        : (startX + themeGetAccentIndex() * (swSize + gap) + swSize * 0.5f);
    UI_TextCenter(buf, NULL, customActive ? "Custom (hex)" : themeAccentName(themeGetAccentIndex()),
                  labelX, 132, 0.24f, 0.24f, g_theme.accent);
    UI_KeyHelp(buf, NULL, "A aplicar  B voltar  <- accent ->", "START sair");
}
