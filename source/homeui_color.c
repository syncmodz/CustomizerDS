#include "homeui_color.h"
#include "fs3ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#define THEME_SIZE 528

/* offsets (base,len) dos runs de cor na tabela de 528 bytes -- derivados por
 * diff dos 5 temas oficiais (Black/Blue/Red/Pink/Yellow). Fatos, nao asset. */
static const u16 RUNS[][2] = {
    {0x0d0,5},{0x0e0,12},{0x0f0,12},{0x100,13},{0x110,13},{0x120,9},
    {0x132,27},{0x154,21},{0x16a,3},{0x170,13},{0x180,13},{0x190,9},
    {0x1a0,13},{0x1b4,19},{0x1ca,3},{0x1d0,21},{0x1f0,12},{0x200,6},
};
#define NRUNS ((int)(sizeof(RUNS) / sizeof(RUNS[0])))

static void rgb2hsv(float r, float g, float b, float* h, float* s, float* v) {
    float mx = fmaxf(r, fmaxf(g, b)), mn = fminf(r, fminf(g, b)), d = mx - mn;
    *v = mx; *s = (mx <= 0.0f) ? 0.0f : d / mx;
    if (d <= 0.0f) { *h = 0.0f; return; }
    float hh;
    if (mx == r) hh = (g - b) / d + (g < b ? 6.0f : 0.0f);
    else if (mx == g) hh = (b - r) / d + 2.0f;
    else hh = (r - g) / d + 4.0f;
    *h = hh / 6.0f;
}

static void hsv2rgb(float h, float s, float v, float* r, float* g, float* b) {
    if (s <= 0.0f) { *r = *g = *b = v; return; }
    h *= 6.0f; int i = (int)h; float f = h - i;
    float p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
    switch (i % 6) {
        case 0: *r = v; *g = t; *b = p; break;
        case 1: *r = q; *g = v; *b = p; break;
        case 2: *r = p; *g = v; *b = t; break;
        case 3: *r = p; *g = q; *b = v; break;
        case 4: *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

bool homeuiColorAvailable(void) {
    FILE* f = fopen("romfs:/homeui/theme_base.bin", "rb");
    if (f) { fclose(f); return true; }
    return false;
}

static u8* loadBase(void) {
    FILE* f = fopen("romfs:/homeui/theme_base.bin", "rb");
    if (!f) return NULL;
    u8* b = (u8*)malloc(THEME_SIZE);
    if (!b) { fclose(f); return NULL; }
    size_t rd = fread(b, 1, THEME_SIZE, f); fclose(f);
    if (rd != (size_t)THEME_SIZE) { free(b); return NULL; }
    return b;
}

/* hue-shift dos triplos RGB dentro de cada run; cinza neutro (s baixo) fica. */
static void hueShift(u8* buf, float hue) {
    for (int i = 0; i < NRUNS; i++) {
        int s = RUNS[i][0], l = RUNS[i][1];
        for (int o = s; o + 2 < s + l; o += 3) {
            float r = buf[o] / 255.0f, g = buf[o+1] / 255.0f, b = buf[o+2] / 255.0f;
            float hh, ss, vv; rgb2hsv(r, g, b, &hh, &ss, &vv);
            if (ss < 0.06f) continue;   /* branco/preto/cinza inalterados */
            float nr, ng, nb; hsv2rgb(hue, ss, vv, &nr, &ng, &nb);
            buf[o]   = (u8)(nr * 255.0f + 0.5f);
            buf[o+1] = (u8)(ng * 255.0f + 0.5f);
            buf[o+2] = (u8)(nb * 255.0f + 0.5f);
        }
    }
}

static const char* homeMenuTID(u8 region) {
    switch (region) {
        case 0: return "0004003000008202"; /* JPN */
        case 2: return "0004003000009802"; /* EUR */
        case 3: return "0004003000009802"; /* AUS */
        case 4: return "000400300000A102"; /* CHN */
        case 5: return "000400300000A902"; /* KOR */
        case 6: return "000400300000B102"; /* TWN */
        default: return "0004003000008F02"; /* USA */
    }
}

/* cria a arvore de diretorios (sdmc:/luma/titles/<tid>/romfs/theme). */
static void mkdirp(const char* path) {
    char tmp[192];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char* p = tmp + 7; *p; p++) {   /* pula "sdmc:/" */
        if (*p == '/') { *p = '\0'; mkdir(tmp, 0777); *p = '/'; }
    }
    mkdir(tmp, 0777);
}

bool homeuiColorApply(u8 r, u8 g, u8 b) {
    u8* base = loadBase();
    if (!base) return false;
    float h, s, v; rgb2hsv(r / 255.0f, g / 255.0f, b / 255.0f, &h, &s, &v);
    hueShift(base, h);
    u32 csz = 0; u8* comp = lz11CompressFast(base, THEME_SIZE, &csz);
    free(base);
    if (!comp) return false;

    const char* tid = homeMenuTID(fs3dsRegion());
    char dir[192];
    snprintf(dir, sizeof(dir), "sdmc:/luma/titles/%s/romfs/theme", tid);
    mkdirp(dir);

    /* grava nos 5 slots de cor: assim, qualquer cor selecionada nas Config. do
     * Home Menu mostra o recolor. */
    static const char* names[5] = { "Black", "Blue", "Red", "Pink", "Yellow" };
    bool ok = false;
    for (int i = 0; i < 5; i++) {
        char p[256];
        snprintf(p, sizeof(p), "%s/%s_LZ.bin", dir, names[i]);
        FILE* o = fopen(p, "wb");
        if (o) { ok |= (fwrite(comp, 1, csz, o) == csz); fclose(o); }
    }
    free(comp);
    return ok;
}

bool homeuiColorRemove(void) {
    const char* tid = homeMenuTID(fs3dsRegion());
    static const char* names[5] = { "Black", "Blue", "Red", "Pink", "Yellow" };
    for (int i = 0; i < 5; i++) {
        char p[256];
        snprintf(p, sizeof(p), "sdmc:/luma/titles/%s/romfs/theme/%s_LZ.bin", tid, names[i]);
        remove(p);
    }
    return true;
}
