#include "color_picker.h"
#include "theme.h"
#include "ui.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

void colorPickerInit(ColorPicker* cp) {
    memset(cp, 0, sizeof(ColorPicker));
    strcpy(cp->hex_input, "0099FF");
    cp->cursor_pos = 0;
    cp->valid = true;
    cp->preview = hexToRGB(cp->hex_input);
}

Color_RGB hexToRGB(const char* hex_str) {
    Color_RGB out = {0, 0, 0};
    if (strlen(hex_str) != 6) return out;
    for (int i = 0; i < 6; i++) {
        if (!isxdigit((unsigned char)hex_str[i])) return out;
    }
    sscanf(hex_str, "%02hhx%02hhx%02hhx", &out.r, &out.g, &out.b);
    return out;
}

u32 rgbToColor32(Color_RGB rgb) {
    return C2D_Color32(rgb.r, rgb.g, rgb.b, 255);
}

/* Input via in->down ja com debounce do input.c -- left/right mudam valor do digito,
 * up/down movem o cursor de posicao. Chamado de darkmodeUpdate enquanto s_hexEditing. */
void colorPickerInput(ColorPicker* cp, u32 kDown) {
    if (kDown & KEY_UP) {
        cp->cursor_pos = (cp->cursor_pos + 5) % 6;
    }
    if (kDown & KEY_DOWN) {
        cp->cursor_pos = (cp->cursor_pos + 1) % 6;
    }
    if (kDown & KEY_RIGHT) {
        char c = cp->hex_input[cp->cursor_pos];
        int val = (isdigit((unsigned char)c)) ? (c - '0') : (toupper((unsigned char)c) - 'A' + 10);
        val = (val + 1) % 16;
        cp->hex_input[cp->cursor_pos] = (val < 10) ? ('0' + val) : ('A' + val - 10);
        cp->preview = hexToRGB(cp->hex_input);
    }
    if (kDown & KEY_LEFT) {
        char c = cp->hex_input[cp->cursor_pos];
        int val = (isdigit((unsigned char)c)) ? (c - '0') : (toupper((unsigned char)c) - 'A' + 10);
        val = (val - 1 + 16) % 16;
        cp->hex_input[cp->cursor_pos] = (val < 10) ? ('0' + val) : ('A' + val - 10);
        cp->preview = hexToRGB(cp->hex_input);
    }
}

/* Renderiza o editor de hex na tela de baixo (320x240).
 * Layout: 6 celulas de digito hex centradas, + preview abaixo + valores RGB */
void colorPickerRender(C2D_TextBuf buf, ColorPicker* cp, float x, float y) {
    float cellW = 38.0f;
    float cellH = 44.0f;
    float gap = 5.0f;
    float totalW = cellW * 6 + gap * 5;
    float startX = (SCREEN_BOT_WIDTH - totalW) * 0.5f;
    float fy = y;

    UI_Text(buf, NULL, "# ", startX - 18, fy + 12, 0.36f, 0.36f, g_theme.textSecondary);

    for (int i = 0; i < 6; i++) {
        float cx = startX + i * (cellW + gap);
        bool sel = (i == cp->cursor_pos);
        ColorRGBA bg = sel ? themeMix(g_theme.surfaceElevated, g_theme.accent, 0.30f)
                           : g_theme.surfaceElevated;
        ColorRGBA border = sel ? g_theme.accent
                               : (themeIsDark() ? (ColorRGBA){255, 255, 255, 18}
                                                : (ColorRGBA){20, 24, 34, 22});
        if (sel) border.a = 120;
        UI_RoundFrame(cx, fy, cellW, cellH, 10, bg, border);
        if (sel) {
            /* pulsacao suave na borda selecionada */
            float pulse = 0.06f * sinf(uiFrameTime() * 5.0f);
            ColorRGBA glow = g_theme.accent;
            glow.a = (u8)(40 + (int)(30 * pulse));
            UI_RoundRect(cx - 2, fy - 2, cellW + 4, cellH + 4, 12, glow);
            UI_RoundFrame(cx, fy, cellW, cellH, 10, bg, border);
        }
        char digit[2] = { cp->hex_input[i], '\0' };
        UI_TextCenter(buf, NULL, digit, cx + cellW * 0.5f, fy + (cellH - 20) * 0.5f,
                      0.42f, 0.42f, sel ? g_theme.textPrimary : g_theme.textSecondary);
    }

    /* preview da cor */
    Color_RGB rgb = cp->preview;
    ColorRGBA previewC = {rgb.r, rgb.g, rgb.b, 255};
    float py = fy + cellH + 12;
    float pw = 80.0f, ph = 36.0f;
    float px = (SCREEN_BOT_WIDTH - pw) * 0.5f;
    UI_RoundRect(px, py, pw, ph, 12, previewC);
    ColorRGBA textOnPreview = themeContrastText(previewC);
    char hexStr[10];
    snprintf(hexStr, sizeof(hexStr), "#%s", cp->hex_input);
    UI_TextCenter(buf, NULL, hexStr, SCREEN_BOT_WIDTH * 0.5f, py + 9, 0.30f, 0.30f, textOnPreview);

    char rgbStr[32];
    snprintf(rgbStr, sizeof(rgbStr), "R%d  G%d  B%d", rgb.r, rgb.g, rgb.b);
    UI_TextCenter(buf, NULL, rgbStr, SCREEN_BOT_WIDTH * 0.5f, py + ph + 8, 0.26f, 0.26f, g_theme.textSecondary);
}
