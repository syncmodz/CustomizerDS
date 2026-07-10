#include "fs3ds.h"
#include <stdlib.h>
#include <string.h>

static bool s_init = false;
static u8 s_region = 1;
static FS_Archive s_theme, s_home, s_badge;
static bool s_haveTheme = false, s_haveHome = false, s_haveBadge = false;

static bool openExt(u32 lowid, FS_Archive* ar) {
    u32 bin[3] = { MEDIATYPE_SD, lowid, 0 };
    FS_Path p = { PATH_BINARY, 0xC, bin };
    return R_SUCCEEDED(FSUSER_OpenArchive(ar, ARCHIVE_EXTDATA, p));
}

bool fs3dsInit(void) {
    if (s_init) return s_haveTheme;
    cfguInit();
    u8 region = 1;
    if (R_FAILED(CFGU_SecureInfoGetRegion(&region))) region = 1;
    s_region = region;
    u32 themeId, homeId;
    switch (region) {
        case 0: themeId = 0x2cc; homeId = 0x82; break; /* JPN */
        case 2: themeId = 0x2ce; homeId = 0x98; break; /* EUR */
        case 5: themeId = 0x2cf; homeId = 0xa9; break; /* KOR */
        default: themeId = 0x2cd; homeId = 0x8f; break; /* USA/others */
    }
    s_haveTheme = openExt(themeId, &s_theme);
    s_haveHome  = openExt(homeId, &s_home);
    s_haveBadge = openExt(0x14d1, &s_badge); /* pode nao existir ainda (badges) */
    s_init = true;
    return s_haveTheme;
}

void fs3dsExit(void) {
    if (!s_init) return;
    if (s_haveTheme) FSUSER_CloseArchive(s_theme);
    if (s_haveHome)  FSUSER_CloseArchive(s_home);
    if (s_haveBadge) FSUSER_CloseArchive(s_badge);
    s_haveTheme = s_haveHome = s_haveBadge = false;
    cfguExit();
    s_init = false;
}

bool fs3dsThemeReady(void) { return s_init && s_haveTheme && s_haveHome; }
bool fs3dsBadgeReady(void) { return s_init && s_haveBadge; }
u8   fs3dsRegion(void)     { return s_region; }
FS_Archive fs3dsThemeExt(void) { return s_theme; }
FS_Archive fs3dsHomeExt(void)  { return s_home; }
FS_Archive fs3dsBadgeExt(void) { return s_badge; }

static FS_Path ap(const char* path) { return fsMakePath(PATH_ASCII, path); }

/* cria a extdata (raw CreateExtSaveData 0x08300182) -- igual ao Anemone. */
static Result createBadgeExtdata(u32 id) {
    static u8 null_smdh[0x36C0];
    memset(null_smdh, 0, sizeof(null_smdh));
    Handle* handle = fsGetSessionHandle();
    u32* cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x08300182;
    cmdbuf[1] = MEDIATYPE_SD;
    cmdbuf[2] = id;
    cmdbuf[3] = 0;
    cmdbuf[4] = 0x36C0;
    cmdbuf[5] = 1000;               /* directory limit */
    cmdbuf[6] = 1000;               /* file limit */
    cmdbuf[7] = (0x36C0 << 4) | 0xA;
    cmdbuf[8] = (u32)null_smdh;
    Result ret = svcSendSyncRequest(*handle);
    if (ret) return ret;
    return cmdbuf[1];
}

bool fs3dsBadgeEnsure(void) {
    if (!s_init) return false;
    if (!s_haveBadge) {
        if (!openExt(0x14d1, &s_badge)) {
            createBadgeExtdata(0x14d1);
            if (!openExt(0x14d1, &s_badge)) return false;
        }
        s_haveBadge = true;
    }
    /* garante os dois arquivos (cria se faltarem, como o Anemone). */
    Handle h;
    if (R_FAILED(FSUSER_OpenFile(&h, s_badge, ap("/BadgeData.dat"), FS_OPEN_READ, 0)))
        FSUSER_CreateFile(s_badge, ap("/BadgeData.dat"), 0, (u64)0xF4DF80);
    else FSFILE_Close(h);
    if (R_FAILED(FSUSER_OpenFile(&h, s_badge, ap("/BadgeMngFile.dat"), FS_OPEN_READ, 0)))
        fs3dsRemakeZeroed(s_badge, "/BadgeMngFile.dat", 0xD4A8);
    else FSFILE_Close(h);
    return true;
}

bool fs3dsReadAll(FS_Archive ar, const char* path, u8** out, u32* outSize) {
    Handle h;
    if (R_FAILED(FSUSER_OpenFile(&h, ar, ap(path), FS_OPEN_READ, 0))) return false;
    u64 sz = 0; FSFILE_GetSize(h, &sz);
    u8* buf = (u8*)malloc((size_t)sz);
    if (!buf) { FSFILE_Close(h); return false; }
    u32 rd = 0;
    Result r = FSFILE_Read(h, &rd, 0, buf, (u32)sz);
    FSFILE_Close(h);
    if (R_FAILED(r)) { free(buf); return false; }
    *out = buf; *outSize = rd;
    return true;
}

bool fs3dsWriteAt(FS_Archive ar, const char* path, u64 off, const void* buf, u32 size) {
    Handle h;
    if (R_FAILED(FSUSER_OpenFile(&h, ar, ap(path), FS_OPEN_WRITE, 0))) return false;
    u32 wr = 0;
    Result r = FSFILE_Write(h, &wr, off, buf, size, FS_WRITE_FLUSH);
    FSFILE_Close(h);
    return R_SUCCEEDED(r) && wr == size;
}

bool fs3dsRemakeZeroed(FS_Archive ar, const char* path, u32 size) {
    FSUSER_DeleteFile(ar, ap(path)); /* pode falhar se nao existe -- ok */
    if (R_FAILED(FSUSER_CreateFile(ar, ap(path), 0, (u64)size))) return false;
    /* zera em pedacos (CreateFile nem sempre garante conteudo zerado). */
    const u32 CH = 256 * 1024;
    u8* z = (u8*)calloc(1, CH);
    if (!z) return true; /* sem buffer: confia no create */
    Handle h;
    if (R_FAILED(FSUSER_OpenFile(&h, ar, ap(path), FS_OPEN_WRITE, 0))) { free(z); return false; }
    for (u32 off = 0; off < size; off += CH) {
        u32 n = (size - off < CH) ? (size - off) : CH;
        u32 wr = 0;
        if (R_FAILED(FSFILE_Write(h, &wr, off, z, n, FS_WRITE_FLUSH))) { FSFILE_Close(h); free(z); return false; }
    }
    FSFILE_Close(h); free(z);
    return true;
}

bool fs3dsWrite(FS_Archive ar, const char* path, const void* buf, u32 size) {
    FSUSER_DeleteFile(ar, ap(path));
    if (R_FAILED(FSUSER_CreateFile(ar, ap(path), 0, (u64)size))) return false;
    return fs3dsWriteAt(ar, path, 0, buf, size);
}

/* ---- LZ11 ---- formato publico (magic 0x11, size 24-bit LE, tokens). ---- */
u8* lz11Decompress(const u8* in, u32 inSize, u32* outSize) {
    if (inSize < 4 || in[0] != 0x11) return NULL;
    u32 osz = in[1] | (in[2] << 8) | (in[3] << 16);
    u8* out = (u8*)malloc(osz ? osz : 1);
    if (!out) return NULL;
    u32 ip = 4, op = 0;
    while (op < osz && ip < inSize) {
        u8 flags = in[ip++];
        for (int b = 0; b < 8 && op < osz; b++) {
            if (!(flags & (0x80 >> b))) {
                if (ip >= inSize) { free(out); return NULL; }
                out[op++] = in[ip++];
            } else {
                if (ip + 1 >= inSize) { free(out); return NULL; }
                u8 a = in[ip++], bb = in[ip++];
                u32 len, disp;
                int indicator = a >> 4;
                if (indicator == 0) {
                    if (ip >= inSize) { free(out); return NULL; }
                    u8 c = in[ip++];
                    len = (((a & 0xF) << 4) | (bb >> 4)) + 0x11;
                    disp = (((bb & 0xF) << 8) | c) + 1;
                } else if (indicator == 1) {
                    if (ip + 1 >= inSize) { free(out); return NULL; }
                    u8 c = in[ip++], d = in[ip++];
                    len = (((a & 0xF) << 12) | (bb << 4) | (c >> 4)) + 0x111;
                    disp = (((c & 0xF) << 8) | d) + 1;
                } else {
                    len = indicator + 1;
                    disp = (((a & 0xF) << 8) | bb) + 1;
                }
                for (u32 k = 0; k < len && op < osz; k++) {
                    if (disp > op) { free(out); return NULL; }
                    out[op] = out[op - disp]; op++;
                }
            }
        }
    }
    *outSize = op;
    return out;
}

/* Compressor "rapido": so literais (valido, nao comprime) -- igual ao caminho
 * fast do Anemone. Usado so p/ regravar o body apos setar o byte de BGM. */
u8* lz11CompressFast(const u8* in, u32 inSize, u32* outSize) {
    /* header (4) + p/ cada 8 literais 1 byte de flag(=0) -> ceil(n/8) flags. */
    u32 blocks = (inSize + 7) / 8;
    u32 cap = 4 + inSize + blocks;
    u8* out = (u8*)malloc(cap ? cap : 1);
    if (!out) return NULL;
    out[0] = 0x11;
    out[1] = inSize & 0xFF; out[2] = (inSize >> 8) & 0xFF; out[3] = (inSize >> 16) & 0xFF;
    u32 op = 4, ip = 0;
    while (ip < inSize) {
        out[op++] = 0x00; /* 8 literais */
        for (int b = 0; b < 8 && ip < inSize; b++) out[op++] = in[ip++];
    }
    *outSize = op;
    return out;
}
