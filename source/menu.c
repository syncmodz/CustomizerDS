#include "menu.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "fonts.h"
#include "anim.h"
#include "icons.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    const char* title;
    int iconImg; /* IconID -- glifo real do Reva UI, ver icons.h */
    const char* subtitle;
    ScreenType target;
} MenuItem;

static const MenuItem ITEMS[] = {
    { "Fontes",      ICON_FONTS, "personalizar", SCREEN_FONTS },
    { "Tema do app", ICON_THEME, "modo e cor",   SCREEN_DARKMODE },
    { "LED RGB",     ICON_LED,   "cor e luz",    SCREEN_LED },
};

static int s_selected = 0;

int menuSelected(void) { return s_selected; }
void menuInit(void) { s_selected = 0; }

void menuUpdate(const AppInput* in, int* currentScreen) {
    if (in->back) return;
    if (in->downNav) s_selected = (s_selected + 1) % 3;
    if (in->up) s_selected = (s_selected - 1 + 3) % 3;

    if (in->touchDown) {
        float tbY = 12.0f;
        float btnW = 90.0f;
        float btnH = 32.0f;
        float gap = 12.0f;
        float totalW = btnW * 3 + gap * 2;
        float startX = (SCREEN_BOT_WIDTH - totalW) * 0.5f;
        for (int i = 0; i < 3; i++) {
            float bx = startX + i * (btnW + gap);
            if (in->touchPY >= tbY && in->touchPY < tbY + btnH &&
                in->touchPX >= bx && in->touchPX < bx + btnW) {
                s_selected = i;
                *currentScreen = ITEMS[i].target;
                return;
            }
        }
        float descY = 56.0f;
        if (in->touchPY >= descY && in->touchPY < descY + 70 &&
            in->touchPX >= 20 && in->touchPX < 300) {
            *currentScreen = ITEMS[s_selected].target;
            return;
        }
    }

    if (in->confirm) {
        *currentScreen = ITEMS[s_selected].target;
    }
}

void menuRenderTop(C2D_TextBuf buf, float transVal) {
    /* Parallax de 3 camadas (Travel Motion): o fundo "atrasa" mais que o
     * card, e o conteudo dentro do card se acomoda primeiro que o card. */
    float offsetRaw = (1.0f - transVal);
    float offsetBg = offsetRaw * 70.0f;
    float offset = offsetRaw * 40.0f;
    float offsetFg = offsetRaw * 18.0f;
    UI_TopBackground();
    UI_TopMenuBar("Inicio", buf);

    float et = UI_EnterProgress();

    float px = sinf(uiFrameTime() * 0.3f) * 6.0f;
    float py = cosf(uiFrameTime() * 0.2f) * 4.0f;
    ColorRGBA dot1 = themeIsDark()
        ? (ColorRGBA){50, 60, 90, 18}
        : (ColorRGBA){180, 190, 220, 20};
    ColorRGBA dot2 = themeIsDark()
        ? (ColorRGBA){70, 50, 100, 12}
        : (ColorRGBA){200, 180, 220, 18};
    UI_RoundRect(60 + px, 80 + py + offsetBg, 70, 70, 35, dot1);
    UI_RoundRect(280 - px, 170 - py + offsetBg, 50, 50, 25, dot2);
    UI_RoundRect(320 + px, 60 - py + offsetBg, 40, 40, 20, dot1);

    float cascadeT = clampf((et * 2.0f - 0.0f), 0.0f, 1.0f);
    float cascadeOffset = (1.0f - easeOutCubic(cascadeT)) * 20.0f;

    UI_Card(16, 30 + offset + cascadeOffset, 368, 196, 16, g_theme.surface);

    float pulse = (sinf(uiFrameTime() * 2.8f) + 1.0f) * 0.5f;
    ColorRGBA logoBg = themeMix(g_theme.surfaceElevated, g_theme.accent, 0.08f + pulse * 0.06f);
    UI_RoundFrame(36, 52 + offsetFg + cascadeOffset, 52, 52, 16, logoBg, (ColorRGBA){255, 255, 255, 24});
    UI_TextCenter(buf, NULL, "CDS", 62, 66 + offsetFg + cascadeOffset, 0.46f, 0.46f, g_theme.textPrimary);

    UI_Text(buf, NULL, "CustomizerDS", 104, 50 + offsetFg + cascadeOffset, 0.54f, 0.54f, g_theme.textPrimary);
    UI_Text(buf, NULL, "personalize seu console", 104, 76 + offsetFg + cascadeOffset, 0.24f, 0.24f, g_theme.accent);

    ColorRGBA itemBg = themeMix(g_theme.surfaceElevated, g_theme.accent, 0.10f);
    UI_RoundRect(36, 104 + offsetFg + cascadeOffset, 42, 42, 12, itemBg);
    iconsDraw((IconID)ITEMS[s_selected].iconImg, 57, 125 + offsetFg + cascadeOffset, 20.0f, g_theme.textPrimary, 1.0f);
    UI_Text(buf, NULL, ITEMS[s_selected].title, 92, 110 + offsetFg + cascadeOffset, 0.36f, 0.36f, g_theme.textPrimary);
    UI_Text(buf, NULL, ITEMS[s_selected].subtitle, 92, 132 + offsetFg + cascadeOffset, 0.22f, 0.22f, g_theme.textSecondary);

    for (int i = 0; i < 3; i++) {
        float dotT = clampf((et * 2.5f - i * 0.10f), 0.0f, 1.0f);
        float ds = easeOutBack(dotT);
        float cx = 130 + i * 22;
        float cy = 168 + offsetFg + cascadeOffset;
        ColorRGBA dotC = (i == s_selected)
            ? g_theme.accent
            : (themeIsDark() ? (ColorRGBA){60, 65, 85, 200} : (ColorRGBA){190, 194, 210, 200});
        UI_RoundRect(cx - 5 * (1.0f - ds), cy - 5 * (1.0f - ds), 10 * ds, 10 * ds, 5 * ds, dotC);
        if (i == s_selected) {
            ColorRGBA glow = {255, 255, 255, 24};
            UI_RoundRect(cx - 7 * (1.0f - ds), cy - 7 * (1.0f - ds), 14 * ds, 14 * ds, 7 * ds, glow);
        }
    }
}

void menuRenderBottom(C2D_TextBuf buf, float transVal) {
    float offset = (1.0f - transVal) * 30.0f;
    float et = UI_EnterProgress();
    UI_BottomBackground();

    float tbY = 12.0f + offset;
    float btnW = 90.0f;
    float btnH = 32.0f;
    float gap = 12.0f;
    float totalW = btnW * 3 + gap * 2;
    float startX = (SCREEN_BOT_WIDTH - totalW) * 0.5f;

    for (int i = 0; i < 3; i++) {
        float bx = startX + i * (btnW + gap);
        float appearT = clampf((et * 2.5f - i * 0.10f), 0.0f, 1.0f);
        UI_PillButton(buf, bx, tbY, btnW, btnH,
                      ITEMS[i].title, NULL, ITEMS[i].iconImg,
                      i == s_selected, appearT);
    }

    float descY = 56.0f + offset;
    float descAp = clampf((et * 2.0f - 0.15f), 0.0f, 1.0f);
    float da = easeOutCubic(descAp);
    ColorRGBA descBg = themeIsDark()
        ? (ColorRGBA){20, 22, 30, 240}
        : (ColorRGBA){240, 242, 248, 240};
    descBg.a = (u8)((float)descBg.a * da);
    UI_RoundFrame(20, descY, 280, 70, 14, descBg, (ColorRGBA){255, 255, 255, (u8)(8 * da)});

    UI_RoundRect(32, descY + 8, 36, 36, 10, g_theme.accent);
    ColorRGBA iconText = themeContrastText(g_theme.accent);
    iconText.a = (u8)(255 * da);
    iconsDraw((IconID)ITEMS[s_selected].iconImg, 50, descY + 26, 18.0f, iconText, da);

    ColorRGBA titleC = g_theme.textPrimary;
    titleC.a = (u8)(255 * da);
    UI_Text(buf, NULL, ITEMS[s_selected].title, 80, descY + 8, 0.34f, 0.34f, titleC);
    ColorRGBA subC = g_theme.textSecondary;
    subC.a = (u8)(255 * da);
    UI_Text(buf, NULL, ITEMS[s_selected].subtitle, 80, descY + 30, 0.24f, 0.24f, subC);

    ColorRGBA hintC = g_theme.textHint;
    hintC.a = (u8)(180 * da);
    UI_TextCenter(buf, NULL, "Navegue com as setas ou toque na tela",
                  160, descY + 54, 0.22f, 0.22f, hintC);

    UI_HelpBar(buf, "A abrir", "START sair");
}
