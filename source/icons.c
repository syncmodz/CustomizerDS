#include "icons.h"
#include <citro2d.h>

/* A ordem do enum IconID (icons.h) precisa bater com a ordem dos arquivos
 * passados ao tex3ds (ver romfs/gfx/icons.t3x e source/icons_gen.h):
 * 0=icon_fonts (textformat), 1=icon_theme (moon.fill), 2=icon_led (bolt.fill),
 * 3=icon_sun (sun.fill), 4=icon_badge_a, 5=icon_badge_b -- estes 3 ultimos
 * sao sprites "sticker" Reva (contorno preto bold + fill colorido proprio,
 * spec v5), desenhados SEM tint (ver iconsDrawFixed). */
static C2D_SpriteSheet s_sheet = NULL; /* icons.t3x: ICON_FONTS..ICON_BADGE_B */
static C2D_SpriteSheet s_extra = NULL; /* extra.t3x:  ICON_SWATCH_THICK..ICON_APPICON */

/* Roteia o IconID pra sheet certa por faixa. Fora de faixa, ou sheet nao
 * carregada, devolve imagem vazia -- os draws checam .tex/.subtex e nao
 * desenham nada (nunca trava o app). */
static C2D_Image getImage(IconID id) {
    C2D_Image img = {0};
    if (id >= ICON_FONTS && id <= ICON_BADGE_B) {
        if (s_sheet) return C2D_SpriteSheetGetImage(s_sheet, (size_t)id);
    } else if (id >= ICON_SWATCH_THICK && id <= ICON_PACK) {
        if (s_extra) return C2D_SpriteSheetGetImage(s_extra, (size_t)(id - ICON_SWATCH_THICK));
    }
    return img;
}

void iconsInit(void) {
    s_sheet = C2D_SpriteSheetLoad("romfs:/gfx/icons.t3x");
    s_extra = C2D_SpriteSheetLoad("romfs:/gfx/extra.t3x");
}

void iconsExit(void) {
    if (s_sheet) { C2D_SpriteSheetFree(s_sheet); s_sheet = NULL; }
    if (s_extra) { C2D_SpriteSheetFree(s_extra); s_extra = NULL; }
}

void iconsDraw(IconID id, float x, float y, float size, ColorRGBA tint, float alpha) {
    C2D_Image img = getImage(id);
    if (!img.tex || !img.subtex) return;

    float nh = (float)img.subtex->height;
    float nw = (float)img.subtex->width;
    if (nh <= 0.0f) return;
    float scale = size / nh;

    C2D_ImageTint t;
    C2D_PlainImageTint(&t, C2D_Color32(tint.r, tint.g, tint.b, (u8)(tint.a * alpha)), 1.0f);
    C2D_DrawImageAt(img, x - (nw * scale) * 0.5f, y - (nh * scale) * 0.5f, 0.5f, &t, scale, scale);
}

/* blend=0.0: o combiner do citro2d ignora a cor do tint e so usa a cor
 * original do sprite, modulada pelo alpha do tint -- e como expomos so o
 * fade (alpha) sem lavar o contorno preto + fill colorido do asset. */
void iconsDrawFixed(IconID id, float x, float y, float size, float alpha) {
    C2D_Image img = getImage(id);
    if (!img.tex || !img.subtex) return;

    float nh = (float)img.subtex->height;
    float nw = (float)img.subtex->width;
    if (nh <= 0.0f) return;
    float scale = size / nh;

    C2D_ImageTint t;
    C2D_PlainImageTint(&t, C2D_Color32(255, 255, 255, (u8)(255 * alpha)), 0.0f);
    C2D_DrawImageAt(img, x - (nw * scale) * 0.5f, y - (nh * scale) * 0.5f, 0.5f, &t, scale, scale);
}

/* Variante rotacionada de iconsDrawFixed (spec v5 6: icone sol/lua gira
 * durante a transicao yin-yang) -- C2D_DrawImageAtRotated ja recebe x,y
 * como CENTRO da imagem (nao top-left), entao aqui nao precisa do offset
 * manual que iconsDraw/iconsDrawFixed fazem. */
void iconsDrawFixedRotated(IconID id, float x, float y, float size, float angleRad, float alpha) {
    C2D_Image img = getImage(id);
    if (!img.tex || !img.subtex) return;

    float nh = (float)img.subtex->height;
    if (nh <= 0.0f) return;
    float scale = size / nh;

    C2D_ImageTint t;
    C2D_PlainImageTint(&t, C2D_Color32(255, 255, 255, (u8)(255 * alpha)), 0.0f);
    C2D_DrawImageAtRotated(img, x, y, 0.5f, angleRad, &t, scale, scale);
}

/* Variante TINTADA e rotacionada (v9 §3a): igual iconsDrawFixedRotated mas com
 * blend=1.0, pra girar a LUA original do Reva pintada de prata (o moon_fill e
 * uma crescente com contorno escuro -- como sticker fica preta no escuro). O
 * SOL continua no ...FixedRotated (cor propria amarela). */
void iconsDrawRotated(IconID id, float x, float y, float size, float angleRad, ColorRGBA tint, float alpha) {
    C2D_Image img = getImage(id);
    if (!img.tex || !img.subtex) return;

    float nh = (float)img.subtex->height;
    if (nh <= 0.0f) return;
    float scale = size / nh;

    C2D_ImageTint t;
    C2D_PlainImageTint(&t, C2D_Color32(tint.r, tint.g, tint.b, (u8)(tint.a * alpha)), 1.0f);
    C2D_DrawImageAtRotated(img, x, y, 0.5f, angleRad, &t, scale, scale);
}

bool iconIsSticker(IconID id) {
    return id == ICON_SUN || id == ICON_BADGE_A || id == ICON_BADGE_B || id == ICON_APPICON;
}

/* Despacha pro caminho certo (fixed p/ sticker, tintado p/ glifo monocromatico)
 * -- os chamadores genericos (cards/pilulas/listas que desenham "o icone do
 * item") nao precisam saber qual e qual, so chamam isto sempre. */
void iconsDrawAuto(IconID id, float x, float y, float size, ColorRGBA tint, float alpha) {
    if (iconIsSticker(id)) iconsDrawFixed(id, x, y, size, alpha);
    else iconsDraw(id, x, y, size, tint, alpha);
}
