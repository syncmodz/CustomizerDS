#include "color_picker.h"
#include "theme.h"
#include "ui.h"
#include "common.h"
#include "lang.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

void colorPickerInit(ColorPicker* cp) {
    memset(cp, 0, sizeof(ColorPicker));
    strcpy(cp->hex_input, "0099FF");
    cp->cursor_pos = 0;
    cp->valid = true;
    cp->preview = hexToRGB(cp->hex_input);
    cp->dispR = cp->preview.r; cp->dispG = cp->preview.g; cp->dispB = cp->preview.b;
    cp->digitPopT = 999.0f; cp->digitPopPos = -1; /* sem pop espurio ao abrir */
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

static void stepDigit(ColorPicker* cp, int pos, int dir) {
    char c = cp->hex_input[pos];
    int val = (isdigit((unsigned char)c)) ? (c - '0') : (toupper((unsigned char)c) - 'A' + 10);
    val = (val + dir + 16) % 16;
    cp->hex_input[pos] = (val < 10) ? ('0' + val) : ('A' + val - 10);
    cp->preview = hexToRGB(cp->hex_input);
    cp->digitPopT = 0.0f; cp->digitPopPos = pos; /* 1.9.0 FIX4: dispara micro-pop */
}

/* Input via in->down ja com debounce do input.c -- left/right mudam valor do digito,
 * up/down movem o cursor de posicao. Chamado de darkmodeUpdate enquanto s_hexEditing.
 * Tambem aceita toque/mouse: tocar a metade esquerda de uma celula decrementa,
 * a direita incrementa, e qualquer toque na celula move o cursor pra ela --
 * sem isso o seletor de hex so funcionava com botoes fisicos. */
void colorPickerInput(ColorPicker* cp, const AppInput* in, float y) {
    if (in->down & KEY_UP) {
        cp->cursor_pos = (cp->cursor_pos + 5) % 6;
    }
    if (in->down & KEY_DOWN) {
        cp->cursor_pos = (cp->cursor_pos + 1) % 6;
    }
    if (in->down & KEY_RIGHT) stepDigit(cp, cp->cursor_pos, 1);
    if (in->down & KEY_LEFT) stepDigit(cp, cp->cursor_pos, -1);

    if (in->touchDown) {
        float cellW = HEX_CELL_W, cellH = HEX_CELL_H, gap = HEX_CELL_GAP;
        float totalW = cellW * 6 + gap * 5;
        float startX = (SCREEN_BOT_WIDTH - totalW) * 0.5f;
        for (int i = 0; i < 6; i++) {
            float cx = startX + i * (cellW + gap);
            if (in->touchPX >= cx && in->touchPX < cx + cellW &&
                in->touchPY >= y && in->touchPY < y + cellH) {
                cp->cursor_pos = i;
                if (in->touchPX < cx + cellW * 0.5f) stepDigit(cp, i, -1);
                else stepDigit(cp, i, 1);
                break;
            }
        }
    }
}

/* Renderiza o editor de hex na tela de baixo (320x240).
 * Layout: 6 celulas de digito hex centradas, + preview abaixo + valores RGB.
 * (Sem parametro x: a fileira e sempre centrada na largura da tela --
 * um "x" de origem do toque nunca foi usado aqui de verdade, era parametro
 * morto; quem anima a partir do ponto tocado e a barra de espectro acima,
 * que tem largura/posicao proprias.) */
void colorPickerRender(C2D_TextBuf buf, ColorPicker* cp, float y) {
    float cellW = HEX_CELL_W;
    float cellH = HEX_CELL_H;
    float gap = HEX_CELL_GAP;
    float radius = HEX_CELL_RADIUS;
    float totalW = cellW * 6 + gap * 5;
    float startX = (SCREEN_BOT_WIDTH - totalW) * 0.5f;
    float fy = y;

    UI_Text(buf, NULL, "# ", startX - 16, fy + 8, 0.32f, 0.32f, g_theme.textSecondary);

    cp->digitPopT += uiFrameDt();
    for (int i = 0; i < 6; i++) {
        float cx = startX + i * (cellW + gap);
        bool sel = (i == cp->cursor_pos);
        /* 1.9.0 FIX4: micro-pop 1.15->1 (EXPR_FAST 0.2s) na celula recem-editada. */
        float pop = 1.0f;
        if (i == cp->digitPopPos && cp->digitPopT < DUR_EFFECTS_DEF)
            pop = 1.15f - 0.15f * easeFunc(clampf(cp->digitPopT / DUR_EFFECTS_DEF, 0.0f, 1.0f), EASE_EXPR_FAST);
        float dcx = cx + cellW * 0.5f, dcy = fy + cellH * 0.5f;
        float pw = cellW * pop, ph = cellH * pop;
        float px = dcx - pw * 0.5f, py = dcy - ph * 0.5f;
        ColorRGBA digitCol;
        if (sel) {
            /* 1.6.1: digito selecionado = celula PREENCHIDA de accent solido +
             * digito em cor de contraste. 1.9.0: elevacao Material (foco/knob). */
            ColorRGBA acc = UI_AccentAnim(); acc.a = 255;
            UI_Elevation(px, py, pw, ph, radius, 3, 1.0f);
            UI_RoundRect(px, py, pw, ph, radius, acc);
            digitCol = themeContrastText(acc);
        } else {
            ColorRGBA bg = g_theme.surfaceElevated;
            ColorRGBA border = themeIsDark() ? (ColorRGBA){255, 255, 255, 18}
                                             : (ColorRGBA){20, 24, 34, 22};
            UI_RoundFrame(px, py, pw, ph, radius, bg, border);
            digitCol = g_theme.textSecondary;
        }
        char digit[2] = { cp->hex_input[i], '\0' };
        UI_TextCenter(buf, NULL, digit, dcx, dcy - 9.0f * pop, 0.38f * pop, 0.38f * pop, digitCol);
    }

    /* 1.6.0: cor exibida persegue preview (cross-fade ~150ms) -> a moldura de
     * preview MORFA a cor e os numeros R/G/B CONTAM em vez de saltar. */
    {
        float sp = fminf(1.0f, uiFrameDt() * 12.0f);
        cp->dispR += ((float)cp->preview.r - cp->dispR) * sp;
        cp->dispG += ((float)cp->preview.g - cp->dispG) * sp;
        cp->dispB += ((float)cp->preview.b - cp->dispB) * sp;
    }
    /* preview da cor -- somente leitura (sem hit-test em colorPickerInput),
     * por isso ganha rotulo + moldura para nao parecer um botao clicavel */
    Color_RGB rgb = { (u8)(cp->dispR + 0.5f), (u8)(cp->dispG + 0.5f), (u8)(cp->dispB + 0.5f) };
    ColorRGBA previewC = {rgb.r, rgb.g, rgb.b, 255};
    float py = fy + cellH + 16;
    float pw = 80.0f, ph = 36.0f;
    float px = (SCREEN_BOT_WIDTH - pw) * 0.5f;
    UI_TextCenter(buf, NULL, T(STR_PREVIEW), SCREEN_BOT_WIDTH * 0.5f, py - 13, 0.20f, 0.20f, g_theme.textHint);
    ColorRGBA frameBorder = themeIsDark() ? (ColorRGBA){255, 255, 255, 30} : (ColorRGBA){20, 24, 34, 30};
    UI_RoundFrame(px - 2, py - 2, pw + 4, ph + 4, 13, previewC, frameBorder);
    ColorRGBA textOnPreview = themeContrastText(previewC);
    char hexStr[10];
    snprintf(hexStr, sizeof(hexStr), "#%s", cp->hex_input);
    UI_TextCenter(buf, NULL, hexStr, SCREEN_BOT_WIDTH * 0.5f, py + 9, 0.30f, 0.30f, textOnPreview);

    char rgbStr[32];
    snprintf(rgbStr, sizeof(rgbStr), "R%d  G%d  B%d", rgb.r, rgb.g, rgb.b);
    UI_TextCenter(buf, NULL, rgbStr, SCREEN_BOT_WIDTH * 0.5f, py + ph + 8, 0.26f, 0.26f, g_theme.textSecondary);
}
