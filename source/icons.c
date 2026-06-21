#include "icons.h"
#include <citro2d.h>

/* A ordem do enum IconID (icons.h) precisa bater com a ordem dos arquivos
 * passados ao tex3ds (ver romfs/gfx/icons.t3x e source/icons_gen.h):
 * 0=icon_fonts (textformat.abc), 1=icon_theme (moon.fill), 2=icon_led (bolt.fill). */
static C2D_SpriteSheet s_sheet = NULL;

void iconsInit(void) {
    s_sheet = C2D_SpriteSheetLoad("romfs:/gfx/icons.t3x");
}

void iconsExit(void) {
    if (s_sheet) { C2D_SpriteSheetFree(s_sheet); s_sheet = NULL; }
}

void iconsDraw(IconID id, float x, float y, float size, ColorRGBA tint, float alpha) {
    if (!s_sheet || id < 0 || id >= ICON_COUNT) return;
    C2D_Image img = C2D_SpriteSheetGetImage(s_sheet, (size_t)id);
    if (!img.tex || !img.subtex) return;

    float nh = (float)img.subtex->height;
    float nw = (float)img.subtex->width;
    if (nh <= 0.0f) return;
    float scale = size / nh;

    C2D_ImageTint t;
    C2D_PlainImageTint(&t, C2D_Color32(tint.r, tint.g, tint.b, (u8)(tint.a * alpha)), 1.0f);
    C2D_DrawImageAt(img, x - (nw * scale) * 0.5f, y - (nh * scale) * 0.5f, 0.5f, &t, scale, scale);
}
