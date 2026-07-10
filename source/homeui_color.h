#ifndef HOMEUI_COLOR_H
#define HOMEUI_COLOR_H

#include <3ds.h>
#include <stdbool.h>

/* 2.0.0: recolor REAL da UI do Home Menu. Mecanismo (RE do romfs da Home do
 * dono): os theme/<Cor>_LZ.bin sao tabelas de 528 bytes de gradientes RGB (LZ11).
 * Pega uma base (romfs:/homeui/theme_base.bin -- dado da Nintendo, gitignored),
 * faz hue-shift dos triplos RGB pra cor escolhida, LZ11-comprime e grava nos 5
 * slots de cor via LayeredFS (/luma/titles/<home-tid>/romfs/theme/). O usuario
 * ve o recolor no boot. SD-only -> sem risco de brick (remover desfaz). */

bool homeuiColorAvailable(void);            /* base embutida presente? */
bool homeuiColorApply(u8 r, u8 g, u8 b);    /* recolore a Home com essa cor */
bool homeuiColorRemove(void);               /* remove o recolor (volta ao padrao) */

#endif
