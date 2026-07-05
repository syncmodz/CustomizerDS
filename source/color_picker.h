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

/* Geometria das celulas de digito hex -- centralizada aqui (em vez de
 * duplicada em colorPickerInput/colorPickerRender) para o hit-test do toque
 * nunca dessincronizar do que e desenhado. Reduzida ~20% e com raio 14-16
 * (spec v4 4.4: estavam grandes demais e quase quadradas). */
#define HEX_CELL_W 30.0f
#define HEX_CELL_H 35.0f
#define HEX_CELL_GAP 6.0f
#define HEX_CELL_RADIUS 15.0f

typedef struct {
    char hex_input[8];   /* 6 chars hex + '\0', sempre maiusculo */
    int cursor_pos;      /* 0..5 - qual digito esta selecionado */
    Color_RGB preview;   /* cor atual calculada do hex_input */
    bool valid;
    /* 1.6.0: cor EXIBIDA (float) que persegue preview -> cross-fade da cor +
     * contagem animada dos numeros R/G/B. Inicializada em colorPickerInit. */
    float dispR, dispG, dispB;
} ColorPicker;

void colorPickerInit(ColorPicker* cp);
/* y deve ser o mesmo valor passado para colorPickerRender, para a geometria
 * de toque bater exatamente com o que esta desenhado na tela. */
void colorPickerInput(ColorPicker* cp, const AppInput* in, float y);
Color_RGB hexToRGB(const char* hex_str);
u32 rgbToColor32(Color_RGB rgb);
void colorPickerRender(C2D_TextBuf buf, ColorPicker* cp, float y);

#endif
