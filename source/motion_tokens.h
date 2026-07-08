#ifndef MOTION_TOKENS_H
#define MOTION_TOKENS_H
/* ===== Tokens de movimento/forma extraidos do caelestia-dots/shell =====
 * Fonte: plugin/src/Caelestia/Config/tokens.hpp (Material 3 expressive),
 * clonado e conferido (curvas e duracoes batem 1:1). Regra de uso (Anim.qml):
 *  - ESPACIAL (posicao/tamanho/escala/morph): curvas *Spatial (tem overshoot).
 *  - EFEITO  (cor/alpha):                     curvas *Effects (sem overshoot).
 *  - Transicoes grandes de tela:              EMPHASIZED (2 segmentos).
 * 1.8.0 (motor CAELESTIA). */

/* -------- duracoes (segundos; caelestia usa ms) -------- */
#define DUR_SMALL          0.200f
#define DUR_NORMAL         0.400f
#define DUR_LARGE          0.600f
#define DUR_SPATIAL_FAST   0.350f  /* foco/morph entre itens        */
#define DUR_SPATIAL_DEF    0.500f  /* paineis aparecendo/movendo    */
#define DUR_SPATIAL_SLOW   0.650f
#define DUR_EFFECTS_FAST   0.150f
#define DUR_EFFECTS_DEF    0.200f  /* alpha/opacity                 */
#define DUR_EFFECTS_SLOW   0.300f  /* COR (CAnim do caelestia)      */

/* -------- rounding (px; escala M3 do caelestia, mapeado p/ 3DS) -------- */
#define ROUND_XS    4.0f
#define ROUND_SM    8.0f
#define ROUND_MD   12.0f   /* chip/segment interno   */
#define ROUND_LG   16.0f   /* card interno/preview   */
#define ROUND_LGI  20.0f
#define ROUND_XL   24.0f   /* painel grande (= RADIUS_CARD do 3DS) */
#define ROUND_FULL 9999.0f /* pilula: raio = h/2      */

/* -------- spacing/padding (px) -------- */
#define SPACE_XS 4.0f
#define SPACE_SM 8.0f
#define SPACE_MD 12.0f
#define SPACE_LG 16.0f

#endif
