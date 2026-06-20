#include "darkmode.h"
#include "common.h"
#include "theme.h"
#include "config.h"
#include "ui.h"
#include "fonts.h"

static int s_selected = 0;
static float s_selY = 0.0f;

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
        if (touchIn(touch, 10, 42, 145, 34)) {
            s_selected = 0;
            themeSetDark(false);
            saveTheme();
        } else if (touchIn(touch, 165, 42, 145, 34)) {
            s_selected = 1;
            themeSetDark(true);
            saveTheme();
        } else {
            for (int i = 0; i < themeAccentCount(); i++) {
                int row = i / 2;
                int col = i % 2;
                if (touchIn(touch, 10 + col * 155, 92 + row * 36, 145, 30)) {
                    s_selected = 2 + i;
                    themeSetAccentIndex(i);
                    saveTheme();
                }
            }
        }

        if (touchIn(touch, 8, 206, 78, 26)) {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        }
    }

    if (kDown & KEY_A) {
        if (s_selected == 0) themeSetDark(false);
        else if (s_selected == 1) themeSetDark(true);
        else themeSetAccentIndex(s_selected - 2);
        saveTheme();
    }
}

void darkmodeRenderTop(C2D_TextBuf buf) {
    UI_HeaderTop(buf, fontsCurrent(), "Tema do app", "visual limpo e personalizavel");

    UI_Panel(18, 56, 364, 136, g_theme.surface, g_theme.divider);
    UI_Text(buf, fontsCurrent(), themeIsDark() ? "modo escuro ativo" : "modo claro ativo",
            34, 74, 0.46f, 0.46f, g_theme.textPrimary);
    UI_Text(buf, NULL, "cor de destaque", 34, 105, 0.25f, 0.25f, g_theme.textSecondary);

    for (int i = 0; i < themeAccentCount(); i++) {
        UI_Swatch(34 + i * 46, 130, 34, 34, themeAccentColor(i), i == themeGetAccentIndex());
    }

    UI_Text(buf, NULL, themeAccentName(themeGetAccentIndex()), 34, 174, 0.25f, 0.25f, g_theme.textHint);
}

void darkmodeRenderBottom(C2D_TextBuf buf) {
    UI_TouchChrome(buf, fontsCurrent(), "tema", "toque para aplicar");

    float targetY = (s_selected < 2) ? 42.0f : 92.0f + ((s_selected - 2) / 2) * 36.0f;
    s_selY += (targetY - s_selY) * 0.18f;
    float selH = (s_selected < 2) ? 34.0f : 30.0f;
    if (s_selY + selH > SCREEN_BOT_HEIGHT - 24) s_selY = (float)(SCREEN_BOT_HEIGHT - 24 - (int)selH);
    if (s_selY < 42.0f) s_selY = 42.0f;
    UI_RoundRect(8, s_selY, 304, selH, 4, themeMix(g_theme.surface, g_theme.primary, 0.12f));

    UI_Button(buf, fontsCurrent(), 10, 42, 145, 34, "Claro",
              !themeIsDark() ? "ON" : NULL,
              s_selected == 0 ? UI_BUTTON_SELECTED : (!themeIsDark() ? UI_BUTTON_ACTIVE : UI_BUTTON_NORMAL));
    UI_Button(buf, fontsCurrent(), 165, 42, 145, 34, "Escuro",
              themeIsDark() ? "ON" : NULL,
              s_selected == 1 ? UI_BUTTON_SELECTED : (themeIsDark() ? UI_BUTTON_ACTIVE : UI_BUTTON_NORMAL));

    for (int i = 0; i < themeAccentCount(); i++) {
        int row = i / 2;
        int col = i % 2;
        int selected = (s_selected == 2 + i);
        int active = (themeGetAccentIndex() == i);
        UI_Button(buf, NULL, 10 + col * 155, 92 + row * 36, 145, 30,
                  themeAccentName(i), active ? "ON" : NULL,
                  selected ? UI_BUTTON_SELECTED : (active ? UI_BUTTON_ACTIVE : UI_BUTTON_NORMAL));
    }

    UI_KeyHelp(buf, NULL, "A aplicar  B voltar", "START sair");
}
