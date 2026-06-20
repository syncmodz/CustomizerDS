#include "color_picker.h"
#include "common.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

static int hexDigitValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static char hexDigitChar(int v) {
    v = clampi(v, 0, 15);
    return (v < 10) ? (char)('0' + v) : (char)('A' + v - 10);
}

void hexPickerStart(HexPicker* hp, ColorRGBA initial) {
    snprintf(hp->hex, sizeof(hp->hex), "%02X%02X%02X", initial.r, initial.g, initial.b);
    hp->cursor = 0;
    hp->active = true;
}

bool hexPickerHandleInput(HexPicker* hp, const AppInput* in, bool* applied) {
    *applied = false;
    if (!hp->active) return false;

    if (in->up) hp->cursor = (hp->cursor + 5) % 6;
    if (in->downNav) hp->cursor = (hp->cursor + 1) % 6;

    if (in->left) {
        int v = hexDigitValue(hp->hex[hp->cursor]);
        hp->hex[hp->cursor] = hexDigitChar((v + 15) % 16);
    }
    if (in->right) {
        int v = hexDigitValue(hp->hex[hp->cursor]);
        hp->hex[hp->cursor] = hexDigitChar((v + 1) % 16);
    }

    if (in->confirm) {
        hp->active = false;
        *applied = true;
        return false;
    }
    if (in->back) {
        hp->active = false;
        return false;
    }
    return true;
}

ColorRGBA hexPickerColor(const HexPicker* hp) {
    ColorRGBA out = { 0, 0, 0, 255 };
    out.r = (u8)(hexDigitValue(hp->hex[0]) * 16 + hexDigitValue(hp->hex[1]));
    out.g = (u8)(hexDigitValue(hp->hex[2]) * 16 + hexDigitValue(hp->hex[3]));
    out.b = (u8)(hexDigitValue(hp->hex[4]) * 16 + hexDigitValue(hp->hex[5]));
    return out;
}

void hexPickerRenderTop(C2D_TextBuf buf, const HexPicker* hp, float slideUp) {
    UI_Text(buf, NULL, "Cor customizada", 32, 48 + slideUp, 0.44f, 0.44f, g_theme.textPrimary);
    UI_Text(buf, NULL, "setas: cima/baixo escolhem o digito, esquerda/direita mudam o valor",
            32, 74 + slideUp, 0.23f, 0.23f, g_theme.textHint);

    ColorRGBA preview = hexPickerColor(hp);
    float swX = 262.0f, swY = 44 + slideUp, swW = 92, swH = 92;
    UI_RoundFrame(swX, swY, swW, swH, 16, (ColorRGBA){8, 10, 14, 255}, (ColorRGBA){255, 255, 255, 18});
    UI_RoundRect(swX + 6, swY + 6, swW - 12, swH - 12, 10, preview);

    char hexLabel[8];
    snprintf(hexLabel, sizeof(hexLabel), "#%s", hp->hex);
    UI_TextCenter(buf, NULL, hexLabel, swX + swW * 0.5f, swY + swH + 6, 0.30f, 0.30f, g_theme.textSecondary);
}

void hexPickerRenderBottom(C2D_TextBuf buf, const HexPicker* hp) {
    float cellW = 40.0f, cellH = 48.0f, gap = 6.0f;
    float totalW = cellW * 6 + gap * 5;
    float startX = (SCREEN_BOT_WIDTH - totalW) * 0.5f;
    float y = 40.0f;

    for (int i = 0; i < 6; i++) {
        float x = startX + i * (cellW + gap);
        bool sel = (i == hp->cursor);
        ColorRGBA bg = sel ? themeMix(g_theme.surfaceElevated, g_theme.accent, 0.30f) : g_theme.surfaceElevated;
        ColorRGBA border = sel ? g_theme.accent : g_theme.divider;
        if (sel) border.a = themeIsDark() ? 200 : 220;
        UI_RoundFrame(x, y, cellW, cellH, 10, bg, border);
        char digit[2] = { hp->hex[i], '\0' };
        UI_TextCenter(buf, NULL, digit, x + cellW * 0.5f, y + (cellH - 26) * 0.5f, 0.40f, 0.40f,
                      sel ? g_theme.textPrimary : g_theme.textSecondary);
    }

    ColorRGBA c = hexPickerColor(hp);
    char rgbLine[40];
    snprintf(rgbLine, sizeof(rgbLine), "R%d  G%d  B%d", c.r, c.g, c.b);
    UI_TextCenter(buf, NULL, rgbLine, SCREEN_BOT_WIDTH * 0.5f, y + cellH + 16, 0.27f, 0.27f, g_theme.textSecondary);

    UI_KeyHelp(buf, NULL, "cima/baixo digito  esq/dir valor", "A aplicar  B cancelar");
}
