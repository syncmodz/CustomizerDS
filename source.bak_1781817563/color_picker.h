#ifndef COLOR_PICKER_H
#define COLOR_PICKER_H

#include <citro2d.h>
#include <stdbool.h>
#include "theme.h"
#include "input.h"

/* Editor de cor via hexadecimal (#RRGGBB), 6 dígitos editáveis com D-Pad.
 * Usado pelo slot "Custom" da tela de Tema (darkmode.c) para permitir
 * accent color totalmente livre, além dos 5 presets fixos. */
typedef struct {
    char hex[7];   /* 6 dígitos hex + '\0', sempre maiúsculo */
    int cursor;    /* 0..5, dígito atualmente selecionado */
    bool active;   /* true enquanto o editor está em uso */
} HexPicker;

void hexPickerStart(HexPicker* hp, ColorRGBA initial);

/* Processa input enquanto o editor está ativo.
 * Retorna true se ainda estiver editando (deve continuar capturando input).
 * *applied = true quando o usuário confirmou com A (cor deve ser aplicada).
 * Quando retorna false (por A ou B), hp->active já foi zerado. */
bool hexPickerHandleInput(HexPicker* hp, const AppInput* in, bool* applied);

ColorRGBA hexPickerColor(const HexPicker* hp);

void hexPickerRenderTop(C2D_TextBuf buf, const HexPicker* hp, float slideUp);
void hexPickerRenderBottom(C2D_TextBuf buf, const HexPicker* hp);

#endif
