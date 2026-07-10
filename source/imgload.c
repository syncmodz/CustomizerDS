#include "imgload.h"
#include "lodepng.h"
#include <citro3d.h>
#include <tex3ds.h>
#include <stdlib.h>
#include <string.h>

/* menor potencia de 2 >= v (min 8, max 1024 pro GPU do 3DS). */
static u32 npot(u32 v) {
    u32 p = 8;
    while (p < v) p <<= 1;
    return p;
}

bool imgLoadPng(const unsigned char* png, size_t size, C2D_Image* out) {
    if (!png || !out) return false;
    unsigned w = 0, h = 0; unsigned char* rgba = NULL;
    if (lodepng_decode32(&rgba, &w, &h, png, size)) { if (rgba) free(rgba); return false; }
    if (w == 0 || h == 0 || w > 1024 || h > 1024) { free(rgba); return false; }

    u32 pw = npot(w), ph = npot(h);
    if (pw > 1024 || ph > 1024) { free(rgba); return false; }

    C3D_Tex* tex = (C3D_Tex*)malloc(sizeof(C3D_Tex));
    Tex3DS_SubTexture* sub = (Tex3DS_SubTexture*)malloc(sizeof(Tex3DS_SubTexture));
    if (!tex || !sub) { free(rgba); free(tex); free(sub); return false; }

    if (!C3D_TexInit(tex, (u16)pw, (u16)ph, GPU_RGBA8)) {
        free(rgba); free(tex); free(sub); return false;
    }
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
    memset(tex->data, 0, tex->size);

    /* RGBA linear -> textura tiled do 3DS (blocos 8x8, ordem Z/morton; texel
     * GPU_RGBA8 = bytes A,B,G,R na memoria). */
    u32* dst = (u32*)tex->data;
    for (u32 y = 0; y < h; y++) {
        for (u32 x = 0; x < w; x++) {
            const unsigned char* px = &rgba[(y * w + x) * 4];
            u32 r = px[0], g = px[1], b = px[2], a = px[3];
            u32 tileX = x >> 3, tileY = y >> 3;
            u32 inTile = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1)
                       | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
            u32 pos = (tileY * (pw >> 3) + tileX) * 64 + inTile;
            dst[pos] = (r << 24) | (g << 16) | (b << 8) | a;
        }
    }
    free(rgba);
    C3D_TexFlush(tex);

    sub->width = (u16)w;
    sub->height = (u16)h;
    sub->left = 0.0f;
    sub->top = 1.0f;
    sub->right = (float)w / (float)pw;
    sub->bottom = 1.0f - (float)h / (float)ph;

    out->tex = tex;
    out->subtex = sub;
    return true;
}

void imgFree(C2D_Image* img) {
    if (!img || !img->tex) return;
    C3D_TexDelete((C3D_Tex*)img->tex);
    free((void*)img->tex);
    free((void*)img->subtex);
    img->tex = NULL;
    img->subtex = NULL;
}
