#ifndef ICONS_H_
#define ICONS_H_

#include "common.h"
#include "theme.h"

typedef enum {
    ICON_FONTS = 0,
    ICON_THEME,
    ICON_LED,
    ICON_COUNT
} IconID;

/* Glifos reais do pacote Reva UI (textformat.abc, moon.fill, bolt.fill),
 * convertidos com tex3ds para romfs/gfx/icons.t3x. Se a sheet nao carregar
 * (romfs ausente etc.) iconsDraw nao desenha nada -- nunca trava o app. */
void iconsInit(void);
void iconsExit(void);
void iconsDraw(IconID id, float x, float y, float size, ColorRGBA tint, float alpha);

#endif
