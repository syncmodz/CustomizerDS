#include "sysfont.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SYSFONT_TID_DIR  "sdmc:/luma/titles/0004009B00014002"
#define SYSFONT_LUMA_LZ  SYSFONT_TID_DIR "/romfs/cbf_std.bcfnt.lz"
#define SYSFONT_EXPORT_DIR "sdmc:/3ds/CustomizerDS/sysfont"
#define SYSFONT_EXPORT_LZ  SYSFONT_EXPORT_DIR "/cbf_std.bcfnt.lz"
#define SYSFONT_MAX_LZ   (1536u * 1024u) /* 1.5 MiB */

/* LZ11 (tipo 0x11) "tudo literal": NAO comprime (output um pouco maior que o
 * input), mas e um stream LZ11 VALIDO que o 3DS descompacta corretamente -- e
 * o jeito mais seguro (sem risco de bug de compressao corromper a fonte). Um
 * .bcfnt de fonte (~525KB) vira ~590KB, bem abaixo do limite de 1.5 MiB.
 *
 * Formato: [0x11][size:24 LE] depois grupos de [flag][ate 8 bytes]. flag=0x00
 * = os 8 sao literais; o descompactador para ao produzir 'size' bytes. */
static unsigned char* lz11_store(const unsigned char* in, unsigned int n, unsigned int* outLen) {
    unsigned int groups = (n + 7) / 8;
    unsigned int len = 4 + n + groups; /* header + literais + 1 flag por grupo */
    unsigned char* out = (unsigned char*)malloc(len);
    if (!out) return NULL;
    out[0] = 0x11;
    out[1] = (unsigned char)(n & 0xFF);
    out[2] = (unsigned char)((n >> 8) & 0xFF);
    out[3] = (unsigned char)((n >> 16) & 0xFF);
    unsigned int o = 4, i = 0;
    while (i < n) {
        out[o++] = 0x00; /* flag: 8 literais */
        for (int b = 0; b < 8 && i < n; b++) out[o++] = in[i++];
    }
    *outLen = o;
    return out;
}

static unsigned char* readFile(const char* path, unsigned int* size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    unsigned char* buf = (unsigned char*)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (rd != (size_t)sz) { free(buf); return NULL; }
    *size = (unsigned int)sz;
    return buf;
}

static bool writeFile(const char* path, const unsigned char* data, unsigned int size) {
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    size_t wr = fwrite(data, 1, size, f);
    fclose(f);
    return wr == size;
}

SysfontResult sysfontApply(const char* fontRomfsPath) {
    unsigned int srcSize = 0;
    unsigned char* src = readFile(fontRomfsPath, &srcSize);
    if (!src) return SYSFONT_ERR_READ;

    unsigned int lzSize = 0;
    unsigned char* lz = lz11_store(src, srcSize, &lzSize);
    free(src);
    if (!lz) return SYSFONT_ERR_WRITE;

    if (lzSize > SYSFONT_MAX_LZ) { free(lz); return SYSFONT_ERR_SIZE; }

    /* v14 §4: escreve SO no export do SD. A rota LayeredFS da Luma foi
     * APOSENTADA -- nao funciona pra fonte de sistema no hardware (o NS carrega
     * a fonte no boot, antes do LayeredFS). Ver sysfont.h / docs/SYSTEM_FONT.md.
     *   - Azahar: copiar este arquivo pra <Azahar>/load/mods/0004009B00014002/romfs/.
     *   - Hardware: instalar um .cia da fonte na CTRNAND (FBI/GM9) -- o app NAO
     *     escreve na NAND. */
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/CustomizerDS", 0777);
    mkdir(SYSFONT_EXPORT_DIR, 0777);

    bool okExp = writeFile(SYSFONT_EXPORT_LZ, lz, lzSize);
    free(lz);

    /* limpa overrides quebrados que versoes antigas deixavam na Luma. */
    remove(SYSFONT_LUMA_LZ);

    return okExp ? SYSFONT_OK : SYSFONT_ERR_WRITE;
}

bool sysfontRestore(void) {
    /* v14 §4: remove o export do SD e qualquer override antigo da Luma. No
     * HARDWARE a fonte real so volta reinstalando o .cia stock ou via NAND
     * backup (GM9) -- o app nao escreve na NAND (ver docs/SYSTEM_FONT.md). */
    remove(SYSFONT_LUMA_LZ);
    int r = remove(SYSFONT_EXPORT_LZ);
    return r == 0 || r == -1; /* -1 = ja nao existia, tudo bem */
}

void sysfontReboot(void) {
    APT_HardwareResetAsync();
}

/* v14 §4 (hardware): instala um .cia de fonte na CTRNAND via AM, igual a FBI.
 * O sysmodule 'am' e quem escreve na NAND; o app so precisa do servico am:net
 * (declarado no exheader/RSF) e passar os bytes do .cia pelo handle de install.
 *
 * Sequencia (espelha source/fbi/action/installcias.c da FBI p/ um titulo NAND):
 *   amInit -> AM_DeleteTitle(NAND,tid) + AM_DeleteTicket(tid) [ignora erro] ->
 *   AM_StartCiaInstall(MEDIATYPE_NAND,&h) -> FSFILE_Write(h,...) em blocos ->
 *   AM_FinishCiaInstall(h) [commit atomico]. Erro de escrita -> AM_CancelCIAInstall.
 * A fonte e um titulo normal de NAND (tid & 0xFFFFFFF != 0x2), entao NAO ha
 * AM_InstallFirm -- so a instalacao do titulo. */
#define SYSFONT_FONT_TID  0x0004009B00014002ULL
#define SYSFONT_CHUNK     (256u * 1024u)

SysfontResult sysfontInstallCia(const char* ciaRomfsPath) {
    FILE* f = fopen(ciaRomfsPath, "rb");
    if (!f) return SYSFONT_ERR_READ;

    if (R_FAILED(amInit())) { fclose(f); return SYSFONT_ERR_INSTALL; }

    /* limpa o titulo/ticket antigos da fonte (ignora erro: pode nao existir). */
    AM_DeleteTitle(MEDIATYPE_NAND, SYSFONT_FONT_TID);
    AM_DeleteTicket(SYSFONT_FONT_TID);

    Handle ciaH = 0;
    if (R_FAILED(AM_StartCiaInstall(MEDIATYPE_NAND, &ciaH))) {
        amExit(); fclose(f);
        return SYSFONT_ERR_INSTALL;
    }

    unsigned char* buf = (unsigned char*)malloc(SYSFONT_CHUNK);
    if (!buf) {
        AM_CancelCIAInstall(ciaH); amExit(); fclose(f);
        return SYSFONT_ERR_INSTALL;
    }

    bool ok = true;
    u64 offset = 0;
    size_t n;
    while ((n = fread(buf, 1, SYSFONT_CHUNK, f)) > 0) {
        u32 written = 0;
        if (R_FAILED(FSFILE_Write(ciaH, &written, offset, buf, (u32)n, 0)) || written != (u32)n) {
            ok = false;
            break;
        }
        offset += n;
    }
    if (!ok && ferror(f)) ok = false;
    free(buf);
    fclose(f);

    if (!ok || offset == 0) {
        AM_CancelCIAInstall(ciaH);
        amExit();
        return SYSFONT_ERR_INSTALL;
    }

    Result fin = AM_FinishCiaInstall(ciaH);
    amExit();
    return R_SUCCEEDED(fin) ? SYSFONT_OK : SYSFONT_ERR_INSTALL;
}

/* ---- install INCREMENTAL (mesma rota da FBI, fatiado por frame) ---- */
static void sysfontInstallAbort(SysfontInstall* ctx) {
    if (ctx->h) AM_CancelCIAInstall(ctx->h);
    amExit();
    if (ctx->_f) fclose((FILE*)ctx->_f);
    ctx->_f = NULL; ctx->h = 0; ctx->active = false;
}

SysfontResult sysfontInstallBegin(SysfontInstall* ctx, const char* ciaRomfsPath) {
    memset(ctx, 0, sizeof(*ctx));
    FILE* f = fopen(ciaRomfsPath, "rb");
    if (!f) return SYSFONT_ERR_READ;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return SYSFONT_ERR_READ; }
    if (R_FAILED(amInit())) { fclose(f); return SYSFONT_ERR_INSTALL; }
    AM_DeleteTitle(MEDIATYPE_NAND, SYSFONT_FONT_TID);
    AM_DeleteTicket(SYSFONT_FONT_TID);
    Handle h = 0;
    if (R_FAILED(AM_StartCiaInstall(MEDIATYPE_NAND, &h))) {
        amExit(); fclose(f); return SYSFONT_ERR_INSTALL;
    }
    ctx->_f = f; ctx->h = h; ctx->off = 0; ctx->size = (u64)sz; ctx->active = true;
    return SYSFONT_OK;
}

/* escreve ~1/110 do arquivo por chamada (barra enche suave em ~2s). */
int sysfontInstallStep(SysfontInstall* ctx) {
    static unsigned char buf[256 * 1024];
    if (!ctx->active) return 0;
    FILE* f = (FILE*)ctx->_f;
    u32 chunk = (u32)(ctx->size / 110);
    if (chunk < 8192) chunk = 8192;
    if (chunk > sizeof(buf)) chunk = sizeof(buf);
    u64 remain = ctx->size - ctx->off;
    u32 want = (remain < chunk) ? (u32)remain : chunk;
    if (fread(buf, 1, want, f) != want) { sysfontInstallAbort(ctx); return -1; }
    u32 written = 0;
    if (R_FAILED(FSFILE_Write(ctx->h, &written, ctx->off, buf, want, 0)) || written != want) {
        sysfontInstallAbort(ctx); return -1;
    }
    ctx->off += want;
    return (ctx->off >= ctx->size) ? 0 : 1;
}

SysfontResult sysfontInstallEnd(SysfontInstall* ctx) {
    Result fin = AM_FinishCiaInstall(ctx->h);
    amExit();
    if (ctx->_f) fclose((FILE*)ctx->_f);
    ctx->_f = NULL; ctx->h = 0; ctx->active = false;
    return R_SUCCEEDED(fin) ? SYSFONT_OK : SYSFONT_ERR_INSTALL;
}
