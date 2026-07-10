#ifndef ICONS_H_
#define ICONS_H_

#include "common.h"
#include "theme.h"

typedef enum {
    /* sheet romfs/gfx/icons.t3x (idx 0..5, ver icons_gen.h) */
    ICON_FONTS = 0,
    ICON_THEME,
    ICON_LED,
    ICON_SUN,
    ICON_BADGE_A,
    ICON_BADGE_B,
    /* sheet separado romfs/gfx/extra.t3x (v9 §2.1, idx 0..2 -- ver
     * extra_gen.h e o roteamento por faixa em icons.c::getImage). SWATCH_* sao
     * bolinhas brancas monocromaticas (desenhar com iconsDraw/tint = cor);
     * APPICON e sticker (cor propria, iconsDrawFixed). */
    ICON_SWATCH_THICK,
    ICON_SWATCH_THIN,
    ICON_APPICON,
    /* 2.0.0: icones das abas novas (extra.t3x idx 3..5, ver extra.t3s).
     * Monocromaticos (glifos Reva SF): desenhar TINTADO (iconsDraw) com a cor
     * do item -- NAO sao sticker. */
    ICON_SPLASH,
    ICON_WALL,
    ICON_BADGES,
    ICON_HOMEUI,
    ICON_COUNT
} IconID;

/* Glifos reais do pacote Reva UI (textformat, moon.fill, bolt.fill) + sol e
 * selos A/B (gerados via nano-banana + pos-processo de contorno bold preto,
 * spec v5 secao 3 -- a Reva UI nao tem sol nem selos A/B prontos, so o
 * template plus.circle.fill como referencia de estilo), convertidos com
 * tex3ds para romfs/gfx/icons.t3x. Se a sheet nao carregar (romfs ausente
 * etc.) iconsDraw/iconsDrawFixed nao desenham nada -- nunca trava o app. */
void iconsInit(void);
void iconsExit(void);
/* Tintado (blend=1.0, substitui a cor do glifo pelo tint) -- usar SO para
 * icones monocromaticos sem cor propria (Aa, bolt). NAO usar em moon/sun/
 * badge_a/badge_b: eles tem cor + contorno preto bold ja desenhados no
 * sprite, e o full-tint do citro2d apagaria essa cor (ver iconsDrawFixed). */
void iconsDraw(IconID id, float x, float y, float size, ColorRGBA tint, float alpha);
/* Sem tint (blend=0.0): desenha o sprite com as cores originais dele,
 * so modulando alpha (p/ fade in/out). Usar para moon/sun/badge_a/badge_b
 * -- qualquer asset "sticker" Reva com cor e contorno proprios. */
void iconsDrawFixed(IconID id, float x, float y, float size, float alpha);
/* Igual iconsDrawFixed, mas rotacionado (radianos, sentido anti-horario) --
 * usado pelo giro sol/lua da transicao yin-yang (spec v5 6, ver darkmode.c). */
void iconsDrawFixedRotated(IconID id, float x, float y, float size, float angleRad, float alpha);
/* Igual a iconsDrawFixedRotated mas TINTADO (blend=1.0) -- p/ girar a lua
 * original do Reva pintada de prata (v9 §3a); o sol fica no ...FixedRotated. */
void iconsDrawRotated(IconID id, float x, float y, float size, float angleRad, ColorRGBA tint, float alpha);
/* true p/ ICON_THEME/ICON_SUN/ICON_BADGE_A/ICON_BADGE_B (sticker, cor propria). */
bool iconIsSticker(IconID id);
/* Escolhe iconsDraw ou iconsDrawFixed automaticamente -- usar em qualquer
 * lugar generico que desenha "o icone de um item" sem saber de antemao se
 * e sticker ou monocromatico. */
void iconsDrawAuto(IconID id, float x, float y, float size, ColorRGBA tint, float alpha);

#endif
