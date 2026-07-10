#include "badge.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "lang.h"
#include "fs3ds.h"
#include "lodepng.h"
#include "zip3ds.h"
#include "imgload.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

/* Mecanismo (spec anemone-3ds / badges.c): le sdmc:/Badges/ (PNGs soltos = set
 * "Other Badges"; subpasta = 1 set). Cada PNG (W,H multiplos de 64, ate 12x6
 * badges) vira RGB565 + plano A4, com tiling Z-order 8x8 (morton), + mip 32x32.
 * Escreve em /BadgeData.dat (0xF4DF80) e monta /BadgeMngFile.dat (0xD4A8) na
 * extdata 0x14d1. NNID via act:u. Sem reboot. SO no console real. */

#define BADGES_DIR      "sdmc:/Badges"
#define BADGE_DATA_SIZE 0xF4DF80u
#define BADGE_MNG_SIZE  0xD4A8u
#define BADGE_LIST_MAX  256

#define B_VISIBLE   4
#define B_MARGIN    16.0f
#define B_ROW_H     40.0f
#define B_ROW_GAP    5.0f
#define B_TOP       12.0f

/* kind: 0=png solto, 1=zip, 2=pasta */
typedef struct { char label[300]; char path[512]; int kind; } BadgeListItem;

static BadgeListItem s_list[BADGE_LIST_MAX];
static int s_count = 0;         /* itens de display */
static int s_pngCount = 0;      /* total de PNGs (badges-imagem) achados */
static int s_selected = 0;
static int s_scrollTop = 0;
static float s_scrollAnim = 0.0f, s_scrollVel = 0.0f;
static PopupModal s_popup;
static float s_toastT = 0.0f;
static char s_toast[96] = "";
static int s_pendingInstall = 0;
static C2D_Image s_preview = {0};
static bool s_hasPreview = false;
static int s_previewFor = -2;

static void freePreview(void);
static void loadPreview(int i);

/* buffers de conversao (uma imagem 12x6 badges no maximo), alocados no install */
static u16* s_rgb64 = NULL;
static u8*  s_a64   = NULL;
static u16* s_rgb32 = NULL;
static u8*  s_a32   = NULL;
static Handle s_dataHandle = 0;
static u8* s_mng = NULL;

static void showToast(const char* msg) {
    snprintf(s_toast, sizeof(s_toast), "%s", msg);
    s_toastT = 2.8f;
}

static bool hasPngExt(const char* n) {
    size_t l = strlen(n);
    return l > 4 && (n[l-4]=='.') &&
           (n[l-3]=='p'||n[l-3]=='P') && (n[l-2]=='n'||n[l-2]=='N') && (n[l-1]=='g'||n[l-1]=='G');
}

static bool isDir(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool hasZipExt(const char* n) {
    size_t l = strlen(n);
    return l > 4 && n[l-4] == '.' &&
           (n[l-3]|32) == 'z' && (n[l-2]|32) == 'i' && (n[l-1]|32) == 'p';
}

/* conta badges (PNGs) recursivamente: soltos + dentro de .zip + subpastas. */
static int countBadgesIn(const char* dirPath) {
    DIR* d = opendir(dirPath);
    if (!d) return 0;
    int total = 0;
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char full[768];
        snprintf(full, sizeof(full), "%s/%s", dirPath, e->d_name);
        if (isDir(full)) total += countBadgesIn(full);
        else if (hasZipExt(e->d_name)) total += zipCountPng(full);
        else if (hasPngExt(e->d_name)) total += 1;
    }
    closedir(d);
    return total;
}

/* -------- scan p/ display (conta PNGs soltos + dentro de subpastas) -------- */
void badgeInit(void) {
    s_count = 0; s_pngCount = 0; s_selected = 0; s_scrollTop = 0;
    s_scrollAnim = 0.0f; s_scrollVel = 0.0f;
    s_popup.active = false; s_toastT = 0.0f; s_pendingInstall = 0;
    freePreview(); s_previewFor = -2;

    DIR* d = opendir(BADGES_DIR);
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d)) != NULL && s_count < BADGE_LIST_MAX) {
        if (e->d_name[0] == '.') continue;
        char full[512];
        snprintf(full, sizeof(full), "%s/%s", BADGES_DIR, e->d_name);
        int n = 0, kind = -1;
        char label[300];
        if (isDir(full)) {
            n = countBadgesIn(full);   /* recursivo: pega zips dentro (ThemePlaza) */
            kind = 2;
            snprintf(label, sizeof(label), "[%s] (%d)", e->d_name, n);
        } else if (hasZipExt(e->d_name)) {
            n = zipCountPng(full);
            kind = 1;
            snprintf(label, sizeof(label), "%s (%d)", e->d_name, n);
        } else if (hasPngExt(e->d_name)) {
            n = 1;
            kind = 0;
            snprintf(label, sizeof(label), "%s", e->d_name);
        }
        if (n > 0) {
            s_pngCount += n;
            snprintf(s_list[s_count].label, sizeof(s_list[s_count].label), "%s", label);
            snprintf(s_list[s_count].path, sizeof(s_list[s_count].path), "%s", full);
            s_list[s_count].kind = kind;
            s_count++;
        }
    }
    closedir(d);
    loadPreview(s_selected);
}

/* -------- conversao PNG -> RGB565 + A4 (Z-order 8x8) + mip 32x32 --------
 * Reimplementa pngToRGB565 (facts: math de indexacao morton do GYTB). */
static int badgeConvert(const u8* png, size_t pngSize, bool setIcon) {
    unsigned w = 0, h = 0; u8* img = NULL;
    if (lodepng_decode32(&img, &w, &h, png, pngSize)) { if (img) free(img); return 0; }
    if (setIcon) {
        if (w != 48 || h != 48) { free(img); return 0; }
    } else {
        if (w < 64 || h < 64 || (w % 64) || (h % 64) || w > 12*64 || h > 6*64) { free(img); return 0; }
    }

    memset(s_a64, 0, 12*6*64*64/2);
    memset(s_a32, 0, 12*6*32*32/2);

    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            const u8* px = &img[(y*w + x) * 4];
            unsigned r = px[0] >> 3, g = px[1] >> 2, b = px[2] >> 3, a = px[3] >> 4;
            int idx = 8*64*((y/8)%8) | 64*((x/8)%8) | 32*((y/4)%2) | 16*((x/4)%2)
                    | 8*((y/2)%2) | 4*((x/2)%2) | 2*(y%2) | (x%2);
            idx |= 64*64*(h/64)*(x/64) + 64*64*(y/64);
            s_rgb64[idx] = (u16)((r << 11) | (g << 5) | b);
            s_a64[idx/2] |= (u8)(a << (4 * (x % 2)));
        }
    }
    for (unsigned y = 0; y < h; y += 2) {
        for (unsigned x = 0; x < w; x += 2) {
            const u8* p1 = &img[(y*w + x) * 4];
            const u8* p2 = &img[((y+1)*w + x) * 4];
            const u8* p3 = &img[(y*w + (x+1)) * 4];
            const u8* p4 = &img[((y+1)*w + (x+1)) * 4];
            unsigned r = (p1[0]+p2[0]+p3[0]+p4[0]) >> 5;
            unsigned g = (p1[1]+p2[1]+p3[1]+p4[1]) >> 4;
            unsigned b = (p1[2]+p2[2]+p3[2]+p4[2]) >> 5;
            unsigned a = (p1[3]+p2[3]+p3[3]+p4[3]) >> 6;
            int x2 = x/2, y2 = y/2;
            int idx = 4*64*((y2/8)%4) | 64*((x2/8)%4) | 32*((y2/4)%2) | 16*((x2/4)%2)
                    | 8*((y2/2)%2) | 4*((x2/2)%2) | 2*(y2%2) | (x2%2);
            idx |= 32*32*(h/64)*(x/64) + 32*32*(y/64);
            s_rgb32[idx] = (u16)((r << 11) | (g << 5) | b);
            s_a32[idx/2] |= (u8)(a << (4 * (x2 % 2)));
        }
    }
    free(img);
    return setIcon ? (int)((h/48)*(w/48)) : (int)((h/64)*(w/64));
}

static u8* readSDFile(const char* path, u32* outSize) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    u8* b = (u8*)malloc((size_t)n);
    if (!b) { fclose(f); return NULL; }
    size_t rd = fread(b, 1, (size_t)n, f); fclose(f);
    if (rd != (size_t)n) { free(b); return NULL; }
    *outSize = (u32)n;
    return b;
}

/* pega os bytes do 1o PNG (thumbnail) de um entry, por tipo. */
static u8* badgeFirstPng(const BadgeListItem* it, u32* outSize) {
    if (it->kind == 0) {                 /* png solto */
        return readSDFile(it->path, outSize);
    } else if (it->kind == 1) {          /* zip */
        u8* b = NULL; u32 n = 0;
        if (zipExtractFirstPng(it->path, &b, &n)) { *outSize = n; return b; }
        return NULL;
    } else if (it->kind == 2) {          /* pasta: 1o png solto, senao 1o png do 1o zip */
        DIR* d = opendir(it->path);
        if (!d) return NULL;
        char zipPath[832] = {0};
        u8* out = NULL;
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.') continue;
            if (hasPngExt(e->d_name)) {
                char pp[832]; snprintf(pp, sizeof(pp), "%s/%s", it->path, e->d_name);
                out = readSDFile(pp, outSize);
                if (out) break;
            } else if (hasZipExt(e->d_name) && zipPath[0] == '\0') {
                snprintf(zipPath, sizeof(zipPath), "%s/%s", it->path, e->d_name);
            }
        }
        closedir(d);
        if (!out && zipPath[0]) {
            u8* b = NULL; u32 n = 0;
            if (zipExtractFirstPng(zipPath, &b, &n)) { *outSize = n; out = b; }
        }
        return out;
    }
    return NULL;
}

static void freePreview(void) {
    if (s_hasPreview) { imgFree(&s_preview); s_hasPreview = false; }
}

static void loadPreview(int i) {
    freePreview();
    s_previewFor = i;
    if (i < 0 || i >= s_count) return;
    u32 sz = 0; u8* png = badgeFirstPng(&s_list[i], &sz);
    if (png) { s_hasPreview = imgLoadPng(png, sz, &s_preview); free(png); }
}

static void put_u16(u8* p, u16 v) { p[0]=v&0xFF; p[1]=(v>>8)&0xFF; }
static void put_u32(u8* p, u32 v) { p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF; }

/* shortcut: Anemone deriva de utf8 do nome (getShortcut). Simplificamos p/ 0
 * (sem atalho de titulo -- campo inofensivo). */
static u64 getShortcut(void) { return 0; }

/* instala 1 imagem PNG (1..72 badges) no set dado. Retorna badges instalados. */
static int installBadgeImage(const u8* png, u32 pngSize, const char* name8,
                             int* badgeCount, int setId) {
    int inImage = badgeConvert(png, pngSize, false);
    if (inImage <= 0) return 0;

    /* nome do badge sem extensao (.png), como remove_exten do Anemone. */
    char nm[256];
    snprintf(nm, sizeof(nm), "%s", name8);
    { size_t l = strlen(nm); if (l > 4 && nm[l-4] == '.') nm[l-4] = '\0'; }

    /* nome do badge (16 idiomas) em UTF-16, 0x8A bytes por idioma. */
    u16 name16[0x8A/2]; memset(name16, 0, sizeof(name16));
    ssize_t nlen = utf8_to_utf16(name16, (const u8*)nm, 0x8A/2 - 1);
    if (nlen < 0) nlen = 0;
    u64 shortcut = getShortcut();

    int installed = 0;
    for (int badge = 0; badge < inImage && *badgeCount < 1000; ++badge) {
        int bc = *badgeCount;
        for (int j = 0; j < 16; ++j)
            FSFILE_Write(s_dataHandle, NULL, 0x35E80 + (u64)bc*16*0x8A + j*0x8A, name16, 0x8A, 0);
        FSFILE_Write(s_dataHandle, NULL, 0x318F80 + (u64)bc*0x2800, s_rgb64 + badge*64*64, 64*64*2, 0);
        FSFILE_Write(s_dataHandle, NULL, 0x31AF80 + (u64)bc*0x2800, s_a64 + badge*64*64/2, 64*64/2, 0);
        FSFILE_Write(s_dataHandle, NULL, 0xCDCF80 + (u64)bc*0xA00, s_rgb32 + badge*32*32, 32*32*2, 0);
        FSFILE_Write(s_dataHandle, NULL, 0xCDD780 + (u64)bc*0xA00, s_a32 + badge*32*32/2, 32*32/2, 0);

        u8* m = s_mng + 0x3E8 + (u64)bc*0x28;
        put_u32(m + 0x04, (u32)(bc + 1));  /* badge id */
        put_u32(m + 0x08, (u32)setId);
        put_u16(m + 0x0C, (u16)bc);
        m[0x12] = 255; m[0x13] = 255;      /* quantidade */
        put_u32(m + 0x18, (u32)(shortcut & 0xFFFFFFFF));
        put_u32(m + 0x1C, (u32)(shortcut >> 32));
        put_u32(m + 0x20, (u32)(shortcut & 0xFFFFFFFF));
        put_u32(m + 0x24, (u32)(shortcut >> 32));

        s_mng[0x358 + bc/8] |= (u8)(1 << (bc % 8));  /* bit enabled */
        installed++;
        *badgeCount += 1;
    }
    return installed;
}

/* preenche o struct de 1 set em 0xA028. */
static void writeSetStruct(int setIndex, int setId, int badgesInSet, int startIdx) {
    u8* s = s_mng + 0xA028 + (u64)setIndex*0x30;
    memset(s, 0xFF, 8);
    s[0x0C] = 0x10; s[0x0D] = 0x27;     /* 0x2710 */
    put_u32(s + 0x10, (u32)setId);
    put_u32(s + 0x14, (u32)setIndex);
    memset(s + 0x18, 0xFF, 4);
    put_u32(s + 0x1C, (u32)badgesInSet);
    put_u32(s + 0x20, (u32)(0xFFFF * badgesInSet));
    put_u32(s + 0x24, (u32)startIdx);
    s_mng[0x3D8 + setIndex/8] |= (u8)(1 << (setIndex % 8));  /* set usado */
}

/* escreve o nome do set (16 idiomas) em BadgeData 0x0 + set*0x8A0. */
static void writeSetName(int setIndex, const char* name8) {
    u16 name16[0x8A/2]; memset(name16, 0, sizeof(name16));
    ssize_t nlen = utf8_to_utf16(name16, (const u8*)name8, 0x45 - 1);
    if (nlen < 0) nlen = 0;
    for (int i = 0; i < 16; ++i)
        FSFILE_Write(s_dataHandle, NULL, (u64)setIndex*0x8A0 + i*0x8A, name16, ((u32)nlen+1)*2, 0);
}

/* set icon opcional: le _seticon.png (48x48) e escreve em 0x250F80 + idx*0x2000. */
static void writeSetIcon(const char* pngPath, int setIndex) {
    u32 sz = 0; u8* png = readSDFile(pngPath, &sz);
    if (!png) return;
    if (badgeConvert(png, sz, true) > 0)
        FSFILE_Write(s_dataHandle, NULL, 0x250F80 + (u64)setIndex*0x2000, s_rgb64, 64*64*2, 0);
    free(png);
}

/* copia um arquivo da extdata -> SD em pedacos (sem carregar 16MB na RAM). */
static bool copyExtToSD(FS_Archive ar, const char* extPath, const char* sdPath) {
    Handle h;
    if (R_FAILED(FSUSER_OpenFile(&h, ar, fsMakePath(PATH_ASCII, extPath), FS_OPEN_READ, 0))) return false;
    u64 sz = 0; FSFILE_GetSize(h, &sz);
    FILE* o = fopen(sdPath, "wb");
    if (!o) { FSFILE_Close(h); return false; }
    const u32 CH = 256*1024;
    u8* b = (u8*)malloc(CH);
    if (!b) { fclose(o); FSFILE_Close(h); return false; }
    bool ok = true;
    for (u64 off = 0; off < sz; ) {
        u32 want = (sz - off < CH) ? (u32)(sz - off) : CH;
        u32 rd = 0;
        if (R_FAILED(FSFILE_Read(h, &rd, off, b, want)) || rd == 0) { ok = false; break; }
        if (fwrite(b, 1, rd, o) != rd) { ok = false; break; }
        off += rd;
    }
    free(b); fclose(o); FSFILE_Close(h);
    return ok;
}

/* backup dos badges ORIGINAIS pra SD, so na 1a vez (como backup_badges_fast do
 * Anemone). Assim o dono nao perde os badges que ja tinha. */
static void backupBadgesIfNeeded(FS_Archive bx) {
    FILE* t = fopen("sdmc:/3ds/CustomizerDS/BadgeData.dat", "rb");
    if (t) { fclose(t); return; }   /* ja tem backup */
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/CustomizerDS", 0777);
    copyExtToSD(bx, "/BadgeMngFile.dat", "sdmc:/3ds/CustomizerDS/BadgeMngFile.dat");
    copyExtToSD(bx, "/BadgeData.dat", "sdmc:/3ds/CustomizerDS/BadgeData.dat");
}

/* zera BadgeData.dat inteiro (como zero_handle do Anemone). */
static void zeroBadgeData(void) {
    const u32 CH = 256*1024;
    u8* z = (u8*)calloc(1, CH);
    if (!z) return;
    for (u32 off = 0; off < BADGE_DATA_SIZE; off += CH) {
        u32 n = (BADGE_DATA_SIZE - off < CH) ? (BADGE_DATA_SIZE - off) : CH;
        FSFILE_Write(s_dataHandle, NULL, off, z, n, 0);
    }
    free(z);
}

/* ---- walk de instalacao (pastas + .zip como set, recursivo) ---- */
typedef struct {
    int badgeCount, setCount;
    int defaultSet, defaultSetCount, defaultIdx;
} InstallWalk;

typedef struct { InstallWalk* w; int setId; int installed; } ZipSetCtx;

static void zipBadgeCb(const char* bn, const u8* data, u32 size, void* ud) {
    ZipSetCtx* c = (ZipSetCtx*)ud;
    if (c->w->badgeCount >= 1000) return;
    c->installed += installBadgeImage(data, size, bn, &c->w->badgeCount, c->setId);
}

/* 1 .zip = 1 set (nome = basename sem .zip). */
static void installZipAsSet(const char* zipPath, const char* zipBasename, InstallWalk* w) {
    if (w->setCount >= 100 || w->badgeCount >= 1000) return;
    int thisSet = w->setCount + 1;
    int startIdx = w->badgeCount;
    char nm[256];
    snprintf(nm, sizeof(nm), "%s", zipBasename);
    { size_t l = strlen(nm); if (l > 4 && nm[l-4] == '.') nm[l-4] = '\0'; } /* tira .zip */
    ZipSetCtx ctx = { w, thisSet, 0 };
    zipForEachPng(zipPath, zipBadgeCb, &ctx);
    if (ctx.installed > 0) {
        w->setCount = thisSet;
        writeSetName(thisSet - 1, nm);
        writeSetStruct(thisSet - 1, thisSet, ctx.installed, startIdx);
    }
}

/* varre um dir: .zip -> set proprio; subpasta -> recursa; .png solto -> set
 * default (raiz) ou set com nome da pasta (subpasta). */
static void installDir(const char* dirPath, const char* dirName, bool isRoot, InstallWalk* w) {
    DIR* d = opendir(dirPath);
    if (!d) return;
    int dirSet = 0, dirStart = w->badgeCount, dirCount = 0;
    struct dirent* e;
    while ((e = readdir(d)) != NULL && w->badgeCount < 1000) {
        if (e->d_name[0] == '.') continue;
        char full[832];
        snprintf(full, sizeof(full), "%s/%s", dirPath, e->d_name);
        if (isDir(full)) {
            installDir(full, e->d_name, false, w);
        } else if (hasZipExt(e->d_name)) {
            installZipAsSet(full, e->d_name, w);
        } else if (hasPngExt(e->d_name)) {
            u32 sz = 0; u8* png = readSDFile(full, &sz);
            if (!png) continue;
            if (isRoot) {
                if (w->defaultSet == 0) { w->setCount += 1; w->defaultSet = w->setCount; w->defaultIdx = w->badgeCount; }
                w->defaultSetCount += installBadgeImage(png, sz, e->d_name, &w->badgeCount, w->defaultSet);
            } else {
                if (dirSet == 0 && w->setCount < 100) { w->setCount += 1; dirSet = w->setCount; dirStart = w->badgeCount; }
                if (dirSet != 0) dirCount += installBadgeImage(png, sz, e->d_name, &w->badgeCount, dirSet);
            }
            free(png);
        }
    }
    closedir(d);
    /* set de PNGs soltos de uma SUBpasta (a raiz e finalizada no doInstall). */
    if (!isRoot && dirSet != 0 && dirCount > 0) {
        writeSetName(dirSet - 1, dirName);
        writeSetStruct(dirSet - 1, dirSet, dirCount, dirStart);
        char iconp[900];
        snprintf(iconp, sizeof(iconp), "%s/_seticon.png", dirPath);
        writeSetIcon(iconp, dirSet - 1);
    }
}

static const char* doInstall(void) {
    if (s_pngCount == 0) return T(STR_BADGE_EMPTY);
    if (!fs3dsBadgeEnsure()) return T(STR_BADGE_NOEXT);

    /* NNID (act:u). Falha = 0xFFFFFFFF (offline / sem NNID). */
    u32 nnid = 0xFFFFFFFF;
    if (R_SUCCEEDED(actInit(true))) {
        ACT_Initialize(0xB0502C8, 0, 0);
        ACT_GetAccountInfo(&nnid, sizeof(nnid), 0xFE, 12);
        actExit();
    }

    FS_Archive bx = fs3dsBadgeExt();
    /* backup dos badges originais (1a vez) ANTES de qualquer escrita. */
    backupBadgesIfNeeded(bx);

    if (R_FAILED(FSUSER_OpenFile(&s_dataHandle, bx, fsMakePath(PATH_ASCII, "/BadgeData.dat"), FS_OPEN_WRITE, 0)))
        return T(STR_BADGE_NOEXT);

    s_rgb64 = (u16*)malloc(12*6*64*64*2);
    s_a64   = (u8*) malloc(12*6*64*64/2);
    s_rgb32 = (u16*)malloc(12*6*32*32*2);
    s_a32   = (u8*) malloc(12*6*32*32/2);
    s_mng   = (u8*) calloc(1, BADGE_MNG_SIZE);
    if (!s_rgb64 || !s_a64 || !s_rgb32 || !s_a32 || !s_mng) goto fail;

    zeroBadgeData();

    /* varre /Badges/ recursivo: .zip = set, subpasta = set/recursa, .png solto
     * na raiz = set default. Pega os zips em Badges/ThemePlaza badges/ etc. */
    InstallWalk w = {0};
    installDir(BADGES_DIR, NULL, true, &w);
    int badgeCount = w.badgeCount;
    int setCount = w.setCount;

    /* metadata do set default (PNGs soltos na raiz de /Badges). */
    if (w.defaultSet != 0) {
        int di = w.defaultSet - 1;
        writeSetName(di, "Other Badges");
        writeSetStruct(di, w.defaultSet, w.defaultSetCount, w.defaultIdx);
    }
    FSFILE_Flush(s_dataHandle);

    /* header do BadgeMngFile. */
    s_mng[0x4] = (u8)setCount;
    put_u32(s_mng + 0x08, (u32)badgeCount);
    s_mng[0x10] = 0xFF; s_mng[0x11] = 0xFF; s_mng[0x12] = 0xFF; s_mng[0x13] = 0xFF; /* selected = all */
    put_u32(s_mng + 0x18, (u32)(0xFFFF * badgeCount));
    put_u32(s_mng + 0x1C, nnid);

    /* sets restantes (setCount..100) = valores default. */
    for (int i = setCount; i < 100; ++i) {
        u8* s = s_mng + 0xA028 + (u64)i*0x30;
        memset(s, 0xFF, 8);
        memset(s + 0x08, 0x00, 4);
        s[0x0C] = 0x10; s[0x0D] = 0x27;
        memset(s + 0x0E, 0x00, 2);
        memset(s + 0x10, 0xFF, 0xC);
        memset(s + 0x1C, 0x00, 8);
        memset(s + 0x24, 0xFF, 4);
        memset(s + 0x28, 0x00, 8);
    }

    /* preserva a regiao 0xB2E8 (360*0x18) do mng vivo. */
    Handle mh;
    if (R_SUCCEEDED(FSUSER_OpenFile(&mh, bx, fsMakePath(PATH_ASCII, "/BadgeMngFile.dat"), FS_OPEN_READ, 0))) {
        FSFILE_Read(mh, NULL, 0xB2E8, s_mng + 0xB2E8, 360*0x18);
        FSFILE_Close(mh);
    }

    bool okW = fs3dsWrite(bx, "/BadgeMngFile.dat", s_mng, BADGE_MNG_SIZE);

    FSFILE_Close(s_dataHandle); s_dataHandle = 0;
    free(s_rgb64); free(s_a64); free(s_rgb32); free(s_a32); free(s_mng);
    s_rgb64 = NULL; s_a64 = NULL; s_rgb32 = NULL; s_a32 = NULL; s_mng = NULL;
    return okW ? T(STR_BADGE_DONE) : T(STR_BADGE_FAIL);

fail:
    if (s_dataHandle) { FSFILE_Close(s_dataHandle); s_dataHandle = 0; }
    free(s_rgb64); free(s_a64); free(s_rgb32); free(s_a32); free(s_mng);
    s_rgb64 = NULL; s_a64 = NULL; s_rgb32 = NULL; s_a32 = NULL; s_mng = NULL;
    return T(STR_BADGE_FAIL);
}

static void bClampScroll(void) {
    if (s_selected < s_scrollTop) s_scrollTop = s_selected;
    if (s_selected >= s_scrollTop + B_VISIBLE) s_scrollTop = s_selected - B_VISIBLE + 1;
    int maxTop = s_count - B_VISIBLE; if (maxTop < 0) maxTop = 0;
    if (s_scrollTop > maxTop) s_scrollTop = maxTop;
    if (s_scrollTop < 0) s_scrollTop = 0;
}

void badgeUpdate(const AppInput* in, float dt, int* currentScreen) {
    if (s_pendingInstall > 0) {
        if (--s_pendingInstall == 0) showToast(doInstall());
        return;
    }
    popupUpdate(&s_popup);
    if (s_toastT > 0.0f) s_toastT -= dt;

    if (popupActive(&s_popup)) {
        if (popupConfirmInput(&s_popup, in) && s_popup.result == 1) {
            s_pendingInstall = 2;              /* renderiza "Instalando..." antes de travar */
            showToast(T(STR_BADGE_INSTALLING));
        }
        return;
    }

    if (in->back) { freePreview(); *currentScreen = SCREEN_MAIN_MENU; return; }

    if (s_count > 0) {
        int prev = s_selected;
        if (in->downNav) s_selected = (s_selected + 1) % s_count;
        if (in->up)      s_selected = (s_selected - 1 + s_count) % s_count;
        if (s_selected != prev) { bClampScroll(); loadPreview(s_selected); }
    }
    if (in->confirm && s_pngCount > 0)
        popupShowConfirm(&s_popup, T(STR_BADGE_CONFIRM));
}

void badgeRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_BADGE));

    float cardX = 24.0f + slideX;
    UI_Elevation(cardX, 88.0f, 352.0f, 128.0f, 16.0f, 2, 1.0f);
    if (UI_AssetsReady()) UI_NineCard(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);
    else UI_Card(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);

    if (s_pngCount == 0) {
        UI_TextCenter(buf, NULL, T(STR_BADGE_EMPTY), 200.0f + slideX, 140.0f, 0.28f, 0.28f, g_theme.textSecondary);
    } else if (s_hasPreview) {
        /* thumbnail do 1o badge do set selecionado, a esquerda; nome+total a direita. */
        float iw = (float)s_preview.subtex->width, ih = (float)s_preview.subtex->height;
        float box = 92.0f, sc = fminf(box / iw, box / ih);
        float dx = cardX + 26.0f, dy = 88.0f + (128.0f - ih * sc) * 0.5f;
        C2D_DrawImageAt(s_preview, dx, dy, 0.6f, NULL, sc, sc);
        UI_TextCenterFit(buf, NULL, s_list[s_selected].label, cardX + 246.0f, 128.0f, 0.28f, 0.20f, 196.0f, g_theme.textPrimary);
        char cnt[64];
        snprintf(cnt, sizeof(cnt), "%d %s", s_pngCount, T(STR_BADGE_COUNT));
        UI_TextCenter(buf, NULL, cnt, cardX + 246.0f, 160.0f, 0.22f, 0.22f, UI_AccentAnim());
    } else {
        UI_Text(buf, NULL, T(STR_ITEM_BADGE_SUB), 44.0f + slideX, 106.0f, 0.20f, 0.20f, g_theme.textHint);
        char cnt[64];
        snprintf(cnt, sizeof(cnt), "%d %s", s_pngCount, T(STR_BADGE_COUNT));
        UI_TextCenter(buf, NULL, cnt, 200.0f + slideX, 138.0f, 0.44f, 0.44f, g_theme.textPrimary);
    }
    /* so avisa se a extdata NAO esta disponivel (emulador/sem permissao). */
    if (!fs3dsThemeReady())
        UI_TextCenter(buf, NULL, T(STR_BADGE_NOEXT), 200.0f + slideX, 194.0f, 0.26f, 0.26f, g_theme.warning);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }
}

void badgeRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_BottomBackground();

    bClampScroll();
    {
        float dd = uiFrameDt();
        float target = (float)s_scrollTop, diff = target - s_scrollAnim;
        s_scrollVel += (diff * 210.0f - s_scrollVel * 21.0f) * dd;
        s_scrollAnim += s_scrollVel * dd;
        if (fabsf(diff) < 0.001f && fabsf(s_scrollVel) < 0.01f) { s_scrollAnim = target; s_scrollVel = 0.0f; }
    }

    const float listTop = B_TOP, listBot = B_TOP + B_VISIBLE * (B_ROW_H + B_ROW_GAP);
    for (int i = 0; i < s_count; i++) {
        float x = B_MARGIN + slideX;
        float w = (float)SCREEN_BOT_WIDTH - 2.0f * B_MARGIN;
        float y = B_TOP + ((float)i - s_scrollAnim) * (B_ROW_H + B_ROW_GAP);
        if (y + B_ROW_H < listTop - 2.0f || y > listBot + 2.0f) continue;
        float cy = y + B_ROW_H * 0.5f;
        bool sel = (i == s_selected);
        ColorRGBA bg = sel ? themeCardSelBg() : g_theme.surface;
        UI_Elevation(x, y, w, B_ROW_H, 12.0f, sel ? 2 : 1, 1.0f);
        if (UI_AssetsReady()) UI_NineCard(x, y, w, B_ROW_H, 12.0f, bg);
        else UI_RoundRect(x, y, w, B_ROW_H, 12.0f, bg);
        if (sel) UI_FocusRing(x, y, w, B_ROW_H, 12.0f);
        ColorRGBA tc = sel ? g_theme.textPrimary : g_theme.textSecondary;
        UI_Text(buf, NULL, s_list[i].label, x + 14.0f, cy - 8.0f, 0.24f, 0.24f, tc);
    }

    if (s_count > B_VISIBLE) {
        float sbX = SCREEN_BOT_WIDTH - 6.0f + slideX;
        float sbH = B_VISIBLE * (B_ROW_H + B_ROW_GAP) - B_ROW_GAP;
        ColorRGBA track = g_theme.textHint; track.a = 40;
        UI_RoundRect(sbX, listTop, 3.0f, sbH, 1.5f, track);
        float thumbH = sbH * (float)B_VISIBLE / (float)s_count;
        float thumbY = listTop + sbH * s_scrollAnim / (float)s_count;
        UI_RoundRect(sbX, thumbY, 3.0f, thumbH, 1.5f, UI_AccentAnim());
    }

    if (s_toastT > 0.0f) {
        float a = clampf(s_toastT / 0.4f, 0.0f, 1.0f);
        ColorRGBA bg = g_theme.accent; bg.a = (u8)(230 * a);
        float tw = UI_TextWidth(buf, NULL, s_toast, 0.24f) + 28.0f;
        float tx = (SCREEN_BOT_WIDTH - tw) * 0.5f;
        if (UI_AssetsReady()) UI_NinePill(tx, 186.0f, tw, 24.0f, bg);
        else UI_RoundRect(tx, 186.0f, tw, 24.0f, 12.0f, bg);
        ColorRGBA tc = themeContrastText(g_theme.accent); tc.a = (u8)(255 * a);
        UI_TextCenter(buf, NULL, s_toast, SCREEN_BOT_WIDTH * 0.5f, 191.0f, 0.24f, 0.24f, tc);
    }

    UI_HelpBar(buf, T(STR_BADGE_HELP_L), T(STR_SAIR));
    popupRender(buf, &s_popup);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
