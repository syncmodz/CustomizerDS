#ifndef COLOR_PICKER_H
#define COLOR_PICKER_H

#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include "theme.h"
#include "input.h"

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color_RGB;

typedef struct {
    char hex_input[8];   /* 6 chars hex + '\0', sempre maiusculo */
    int cursor_pos;      /* 0..5 - qual digito esta selecionado */
    Color_RGB preview;   /* cor atual calculada do hex_input */
    bool valid;
} ColorPicker;

void colorPickerInit(ColorPicker* cp);
/* y deve ser o mesmo valor passado para colorPickerRender, para a geometria
 * de toque bater exatamente com o que esta desenhado na tela. */
void colorPickerInput(ColorPicker* cp, const AppInput* in, float y);
Color_RGB hexToRGB(const char* hex_str);
u32 rgbToColor32(Color_RGB rgb);
void colorPickerRender(C2D_TextBuf buf, ColorPicker* cp, float x, float y);

#endif
