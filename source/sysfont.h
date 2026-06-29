#ifndef SYSFONT_H_
#define SYSFONT_H_

#include <stdbool.h>
#include <3ds.h>

/* §5 / PROMPT v14 §4: aplicar uma fonte do app como FONTE DE SISTEMA do 3DS.
 *
 * O mkbcfnt ja gera o glyph sheet em A4 (o que a fonte de sistema exige), entao
 * a gente comprime o .bcfnt em LZ11 -> cbf_std.bcfnt.lz (nome interno do titulo
 * padrao 0004009B00014002, USA/EUR/JPN) e exporta pra
 * sdmc:/3ds/CustomizerDS/sysfont/ . Limite: <= 1.5 MiB comprimido (validado).
 *
 * ATENCAO (corrigido na v14): a rota LayeredFS da Luma
 * (/luma/titles/0004009B00014002/romfs/) **NAO funciona** pra fonte de sistema
 * no HARDWARE -- o NS carrega a fonte na memoria compartilhada no boot, antes do
 * LayeredFS, entao o override e ignorado no console (por isso nunca aplicava).
 * Caminhos que realmente funcionam (ver docs/SYSTEM_FONT.md):
 *   - AZAHAR: copiar o export pra <Azahar>/load/mods/0004009B00014002/romfs/ .
 *   - HARDWARE: instalar um .cia que substitui o titulo da fonte na CTRNAND
 *     (FBI/GM9) + reboot. Esse .cia e pre-gerado pelo build: `make sysfont` ->
 *     build/sysfont/SystemFont_<fonte>.cia (3dstool+makerom+ncchheader.bin
 *     vendorizado do FontTool). Ver docs/SYSTEM_FONT.md. */

typedef enum {
    SYSFONT_OK = 0,
    SYSFONT_ERR_READ,    /* nao leu o arquivo (romfs) */
    SYSFONT_ERR_SIZE,    /* comprimido passou de 1.5 MiB */
    SYSFONT_ERR_WRITE,   /* nao escreveu no SD */
    SYSFONT_ERR_INSTALL, /* falhou ao instalar o .cia na NAND (AM) */
} SysfontResult;

/* v14 §4 (hardware, in-app): instala um .cia de fonte (embutido na romfs em
 * romfs:/sysfont/) substituindo o titulo da fonte de sistema 0004009B00014002
 * na CTRNAND, via servico AM -- MESMA rota da FBI: amInit -> AM_DeleteTitle/
 * AM_DeleteTicket (limpa o antigo) -> AM_StartCiaInstall(MEDIATYPE_NAND) ->
 * FSFILE_Write em blocos -> AM_FinishCiaInstall (atomico). Em qualquer erro de
 * escrita chama AM_CancelCIAInstall e aborta. NAO reinicia (caller chama
 * sysfontReboot). Requer servico am:net no exheader (ver Makefile APP_RSF).
 * SO funciona no HARDWARE (nao no Azahar). NAND backup recomendado. */
SysfontResult sysfontInstallCia(const char* ciaRomfsPath);

/* Versao INCREMENTAL do install (pra mostrar uma tela animada de progresso em
 * vez de travar): Begin abre tudo; Step escreve UM pedaco por chamada (1 frame)
 * -> 1=falta mais, 0=tudo escrito (chamar End), -1=erro (ja abortou); End faz o
 * commit (AM_FinishCiaInstall). off/size dao o progresso (0..1). */
typedef struct {
    void* _f;                  /* FILE* opaco */
    Handle h;                  /* handle do AM_StartCiaInstall */
    u64 off, size;             /* progresso */
    bool active;
} SysfontInstall;
SysfontResult sysfontInstallBegin(SysfontInstall* ctx, const char* ciaRomfsPath);
int sysfontInstallStep(SysfontInstall* ctx);
SysfontResult sysfontInstallEnd(SysfontInstall* ctx);

/* (Legado / Azahar) Gera cbf_std.bcfnt.lz a partir do .bcfnt e exporta pro SD
 * (sdmc:/3ds/CustomizerDS/sysfont/) p/ testar no emulador via /load/mods.
 * NAO escreve NAND, NAO reinicia. */
SysfontResult sysfontApply(const char* fontRomfsPath);

/* Remove o export do SD (e override antigo da Luma). */
bool sysfontRestore(void);

/* Reinicia o console pra aplicar (APT_HardwareResetAsync). */
void sysfontReboot(void);

/* 1.4.0 PART 4.1: copia romfsPath -> sdPath SE sdPath ainda nao existe (nao
 * recopia todo boot). true = ja existia ou copiou ok; false = falha. Nao trava
 * o app (caller ignora o retorno). */
bool sysfontCopyToSDIfAbsent(const char* romfsPath, const char* sdPath);

#endif
