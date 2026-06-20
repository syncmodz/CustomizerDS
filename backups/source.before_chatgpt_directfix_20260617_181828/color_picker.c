#include "color_picker.h"
#include <stdio.h>

void colorPickerInit(ColorPicker* cp) {
    memset(cp, 0, sizeof(ColorPicker));
    strcpy(cp->hex_input, "0099FF");
    cp->cursor_pos = 0;
    cp->valid = true;
    cp->preview = hexToRGB(cp->hex_input);
}

Color_RGB hexToRGB(const char* hex_str) {
    Color_RGB out = {0, 0, 0};

    if (strlen(hex_str) != 6) {
        return out;
    }

    for (int i = 0; i < 6; i++) {
        if (!isxdigit((unsigned char)hex_str[i])) {
            return out;
        }
    }

    sscanf(hex_str, "%02hhx%02hhx%02hhx", &out.r, &out.g, &out.b);
    return out;
}

u32 rgbToColor32(Color_RGB rgb) {
    return C2D_Color32(rgb.r, rgb.g, rgb.b, 255);
}

void colorPickerInput(ColorPicker* cp, u32 kDown) {
    if (kDown & KEY_UP) {
        cp->cursor_pos--;
        if (cp->cursor_pos < 0) cp->cursor_pos = 5;
    }
    if (kDown & KEY_DOWN) {
        cp->cursor_pos++;
        if (cp->cursor_pos > 5) cp->cursor_pos = 0;
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

void colorPickerRender(C2D_TextBuf buf, ColorPicker* cp, float x, float y) {
    C2D_Text text;
    u32 preview_color = rgbToColor32(cp->preview);

    C2D_DrawRectSolid(x, y, 0, 100, 100, preview_color);

    C2D_TextParse(&text, buf, "Hex: #");
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor | C2D_AlignLeft, x, y + 110, 0.0f, 0.3f, 0.3f, 0xFFFFFFFF);

    for (int i = 0; i < 6; i++) {
        char hex_char[2] = {cp->hex_input[i], '\0'};
        u32 color = (i == cp->cursor_pos) ? C2D_Color32(0, 255, 255, 255) : 0xFFFFFFFF;
        C2D_TextParse(&text, buf, hex_char);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor | C2D_AlignLeft, x + 50 + (i * 15), y + 110, 0.0f, 0.3f, 0.3f, color);
    }
}
