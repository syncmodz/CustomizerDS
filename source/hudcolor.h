#ifndef HUDCOLOR_H
#define HUDCOLOR_H

#include <3ds.h>
#include <stdbool.h>

/* Editor de COR por-elemento do HUD da Home Menu (bateria, relogio, coins,
 * passos, wifi). Trabalha no hud_LZ.bin: descomprime LZ11 -> acha o mat1 do
 * BCLYT -> casa materiais por nome -> troca a cor -> re-encoda LZ11 (stored)
 * -> grava /luma/titles/<tid>/romfs/hud_LZ.bin (LayeredFS, SD-only, sem brick).
 * Base = arquivo LayeredFS ja instalado, senao o template USA bundled. */

typedef enum {
    HUD_EL_BATTERY = 0,
    HUD_EL_CLOCK,
    HUD_EL_COINS,
    HUD_EL_STEPS,
    HUD_EL_WIFI,
    HUD_EL_COUNT
} HudElement;

/* Carrega a base (LayeredFS existente ou template) e le a cor atual de cada
 * elemento. Retorna false se nao houver base disponivel (regiao sem template
 * e sem pack instalado). */
bool hudColorLoad(u8 region);

/* true se hudColorLoad conseguiu uma base editavel. */
bool hudColorReady(void);

/* Cor atual (RGB) de um elemento -> para inicializar o picker / desenhar swatch. */
void hudColorGet(HudElement el, u8* r, u8* g, u8* b);

/* Define a cor de um elemento no buffer em memoria (nao grava ainda). */
void hudColorSet(HudElement el, u8 r, u8 g, u8 b);

/* Grava o hud_LZ.bin patchado em /luma/titles/<tid>/romfs/. true = sucesso. */
bool hudColorApply(u8 region);

/* Volta todas as cores para o padrao (recarrega do template bundled). */
bool hudColorResetDefaults(u8 region);

/* Apaga o override LayeredFS (/luma/titles/<tid>/romfs/hud_LZ.bin) -> a Home
 * volta a usar o HUD stock embutido. Usar no "Restaurar padrao". */
bool hudColorRemoveLayered(u8 region);

/* Nome amigavel do elemento (via lang). */
const char* hudColorName(HudElement el);

/* Diagnostico de ambiente: le sdmc:/luma/config.ini pra saber se o LayeredFS vai
 * ser aplicado. O sintoma "apliquei e ficou igual" quase sempre e aqui. */
typedef enum {
    HUD_ENV_OK = 0,        /* config.ini existe e enable_game_patching = 1 */
    HUD_ENV_NO_PATCHING,   /* config.ini existe mas enable_game_patching = 0 */
    HUD_ENV_NO_CONFIG,     /* config.ini nao existe (emulador/sem Luma no SD) */
} HudEnv;
HudEnv hudColorEnv(void);

/* true se o ULTIMO hudColorApply gravou E revalidou (roundtrip) o arquivo no SD. */
bool hudColorLastWriteValid(void);

#endif
