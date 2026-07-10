#ifndef IMGLOAD_H
#define IMGLOAD_H

#include <citro2d.h>
#include <stdbool.h>
#include <stddef.h>

/* 2.0.0: carrega um PNG (buffer) em runtime pra um C2D_Image desenhavel
 * (textura tiled RGBA8). Pra previews de splash/wallpaper. Defensivo: qualquer
 * falha -> retorna false e nao desenha nada (nunca crasha). */
bool imgLoadPng(const unsigned char* png, size_t size, C2D_Image* out);
void imgFree(C2D_Image* img);

#endif
