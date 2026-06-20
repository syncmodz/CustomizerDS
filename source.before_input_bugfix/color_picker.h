#ifndef COLOR_PICKER_H
#define COLOR_PICKER_H

#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color_RGB;

typedef struct {
    char hex_input[8];
    int cursor_pos;
    Color_RGB preview;
    bool valid;
} ColorPicker;

void colorPickerInit(ColorPicker* cp);
void colorPickerInput(ColorPicker* cp, u32 kDown);
Color_RGB hexToRGB(const char* hex_str);
u32 rgbToColor32(Color_RGB rgb);
void colorPickerRender(C2D_TextBuf buf, ColorPicker* cp, float x, float y);

#endif
