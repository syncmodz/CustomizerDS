#include "hudcolor.h"
#include "lang.h"
#include "fs3ds.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ---------- LZ11 ---------- */
static u8* hudLz11Decompress(const u8* in, size_t inLen, size_t* outLen) {
    if (inLen < 4 || in[0] != 0x11) return NULL;
    size_t size = in[1] | (in[2] << 8) | (in[3] << 16);
    size_t pos = 4;
    if (size == 0) {
        if (inLen < 8) return NULL;
        size = in[4] | (in[5] << 8) | (in[6] << 16) | ((size_t)in[7] << 24);
        pos = 8;
    }
    u8* out = (u8*)malloc(size ? size : 1);
    if (!out) return NULL;
    size_t o = 0;
    while (o < size && pos < inLen) {
        u8 flags = in[pos++];
        for (int i = 0; i < 8 && o < size; i++) {
            if (flags & (0x80 >> i)) {
                if (pos + 1 >= inLen) { free(out); return NULL; }
                u8 b0 = in[pos++], b1 = in[pos++];
                int ind = b0 >> 4;
                size_t count, disp;
                if (ind == 0) {
                    if (pos >= inLen) { free(out); return NULL; }
                    u8 b2 = in[pos++];
                    count = (((b0 & 0xF) << 4) | (b1 >> 4)) + 0x11;
                    disp = (((size_t)(b1 & 0xF) << 8) | b2) + 1;
                } else if (ind == 1) {
                    if (pos + 1 >= inLen) { free(out); return NULL; }
                    u8 b2 = in[pos++], b3 = in[pos++];
                    count = (((size_t)(b0 & 0xF) << 12) | ((size_t)b1 << 4) | (b2 >> 4)) + 0x111;
                    disp = (((size_t)(b2 & 0xF) << 8) | b3) + 1;
                } else {
                    count = ind + 1;
                    disp = (((size_t)(b0 & 0xF) << 8) | b1) + 1;
                }
                if (disp > o) { free(out); return NULL; }
                for (size_t k = 0; k < count && o < size; k++) { out[o] = out[o - disp]; o++; }
            } else {
                if (pos >= inLen) { free(out); return NULL; }
                out[o++] = in[pos++];
            }
        }
    }
    if (o < size) { free(out); return NULL; }  /* stream truncado = falha, nao lixo */
    if (outLen) *outLen = size;
    return out;
}

/* LZ11 REAL (greedy + hash-chain), igual formato do que o Home Menu espera --
 * o "stored" (todos literais) gerava arquivo 5x maior e, na pratica, o Home
 * Menu nao aplicava. Este comprime de verdade (~10KB, igual cooolgamer).
 * Validado por roundtrip contra o descompressor acima. */
#define LZ_WIN     4096
#define LZ_MINM    3
#define LZ_MAXM    0x10110
#define LZ_HBITS   15
#define LZ_HSZ     (1 << LZ_HBITS)
#define LZ_CHAIN   128

static u8* hudLz11Compress(const u8* s, size_t n, size_t* outLen) {
    size_t cap = n + n / 8 + 64;
    u8* out = (u8*)malloc(cap);
    if (!out) return NULL;
    int* head = (int*)malloc(LZ_HSZ * sizeof(int));
    int* prevc = (int*)malloc((n ? n : 1) * sizeof(int));
    if (!head || !prevc) { free(head); free(prevc); free(out); return NULL; }
    for (int k = 0; k < LZ_HSZ; k++) head[k] = -1;

    out[0] = 0x11;
    out[1] = n & 0xFF; out[2] = (n >> 8) & 0xFF; out[3] = (n >> 16) & 0xFF;
    size_t op = 4, i = 0;

    #define LZ_H3(p) ((((u32)s[p] << 10) ^ ((u32)s[(p)+1] << 5) ^ (u32)s[(p)+2]) & (LZ_HSZ - 1))
    #define LZ_INS(p) do { if ((p) + 2 < n) { u32 hh = LZ_H3(p); prevc[p] = head[hh]; head[hh] = (int)(p); } } while (0)

    while (i < n) {
        size_t flagIdx = op++;
        u8 flag = 0;
        for (int b = 0; b < 8 && i < n; b++) {
            int bestLen = 0, bestDist = 0;
            if (i + LZ_MINM <= n) {
                int cand = head[LZ_H3(i)];
                int chain = LZ_CHAIN;
                int minPos = (int)i - LZ_WIN; if (minPos < 0) minPos = 0;
                size_t maxl = n - i; if (maxl > LZ_MAXM) maxl = LZ_MAXM;
                while (cand >= minPos && chain-- > 0) {
                    /* VRAM-safe: o decoder do Home Menu NAO suporta distancia 1
                     * (o encoder oficial nunca emite; dist=1 CRASHA a Home --
                     * confirmado no hardware do dono, 110 tokens dist=1 = crash). */
                    if ((int)i - cand >= 2) {
                        size_t l = 0;
                        while (l < maxl && s[cand + l] == s[i + l]) l++;
                        if ((int)l > bestLen) { bestLen = (int)l; bestDist = (int)i - cand; if (l >= maxl) break; }
                    }
                    cand = prevc[cand];
                }
            }
            if (bestLen >= LZ_MINM) {
                flag |= (0x80 >> b);
                int L = bestLen, disp = bestDist - 1;
                if (L <= 0x10) {
                    out[op++] = (u8)(((L - 1) << 4) | ((disp >> 8) & 0xF));
                    out[op++] = (u8)(disp & 0xFF);
                } else if (L <= 0x110) {
                    int l2 = L - 0x11;
                    out[op++] = (u8)((l2 >> 4) & 0xF);
                    out[op++] = (u8)(((l2 & 0xF) << 4) | ((disp >> 8) & 0xF));
                    out[op++] = (u8)(disp & 0xFF);
                } else {
                    int l2 = L - 0x111;
                    out[op++] = (u8)(0x10 | ((l2 >> 12) & 0xF));
                    out[op++] = (u8)((l2 >> 4) & 0xFF);
                    out[op++] = (u8)(((l2 & 0xF) << 4) | ((disp >> 8) & 0xF));
                    out[op++] = (u8)(disp & 0xFF);
                }
                for (int k = 0; k < L && i < n; k++) { LZ_INS(i); i++; }
            } else {
                out[op++] = s[i];
                LZ_INS(i);
                i++;
            }
        }
        out[flagIdx] = flag;
    }
    free(head); free(prevc);
    if (outLen) *outLen = op;
    return out;
}

/* ---------- mapa elemento -> material/slot ---------- */
typedef struct { HudElement el; const char* mat; int slot; } MatMap;
static const MatMap MAP[] = {
    { HUD_EL_BATTERY, "P_BatF_00",    1 },
    { HUD_EL_BATTERY, "P_BatF_01",    1 },
    { HUD_EL_BATTERY, "P_BatF_03",    1 },
    { HUD_EL_CLOCK,   "T_Date_00",    1 },
    { HUD_EL_CLOCK,   "T_TimeL_00",   1 },
    { HUD_EL_CLOCK,   "T_TimeR_00",   1 },
    { HUD_EL_CLOCK,   "T_TimeC_00",   1 },
    { HUD_EL_COINS,   "T_Coin_00",    1 },
    { HUD_EL_COINS,   "P_Coin_00",    1 },
    { HUD_EL_STEPS,   "T_Walk_00",    1 },
    { HUD_EL_STEPS,   "P_Walk_00",    1 },
    { HUD_EL_WIFI,    "P_NetAntR_00", 0 },
    { HUD_EL_WIFI,    "P_NetAntL_00", 0 },
};
#define MAP_N ((int)(sizeof(MAP) / sizeof(MAP[0])))
#define MAX_OFF 8

typedef struct { int off[MAX_OFF]; int n; u8 r, g, b; } ElState;

static u8* s_darc = NULL;
static size_t s_darcLen = 0;
static bool s_ready = false;
static ElState s_el[HUD_EL_COUNT];

static u32 rdU32(const u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24); }

/* localiza o mat1 e resolve os offsets absolutos de cor de cada elemento. */
static bool buildOffsets(void) {
    for (int e = 0; e < HUD_EL_COUNT; e++) { s_el[e].n = 0; s_el[e].r = s_el[e].g = s_el[e].b = 0xFF; }
    if (!s_darc || s_darcLen < 32) return false;

    /* acha o "mat1" */
    size_t m = 0; bool found = false;
    for (size_t i = 0; i + 12 < s_darcLen; i++) {
        if (s_darc[i] == 'm' && s_darc[i+1] == 'a' && s_darc[i+2] == 't' && s_darc[i+3] == '1') {
            u32 cnt = rdU32(s_darc + i + 8);
            if (cnt > 0 && cnt < 256) { m = i; found = true; break; }
        }
    }
    if (!found) return false;

    u32 count = rdU32(s_darc + m + 8);
    size_t tableEnd = m + 12 + (size_t)count * 4;
    if (tableEnd > s_darcLen) return false;

    for (u32 j = 0; j < count; j++) {
        u32 o = rdU32(s_darc + m + 12 + j * 4);
        size_t matAbs = m + o;
        if (matAbs + 28 > s_darcLen) continue;
        char name[24];
        memcpy(name, s_darc + matAbs, 20); name[20] = '\0';
        size_t body = matAbs + 20;
        for (int k = 0; k < MAP_N; k++) {
            if (strcmp(name, MAP[k].mat) != 0) continue;
            size_t coff = body + (size_t)MAP[k].slot * 4;
            if (coff + 3 >= s_darcLen) continue;
            ElState* es = &s_el[MAP[k].el];
            if (es->n < MAX_OFF) es->off[es->n++] = (int)coff;
        }
    }
    /* le a cor atual (do 1o offset de cada elemento) */
    bool any = false;
    for (int e = 0; e < HUD_EL_COUNT; e++) {
        if (s_el[e].n > 0) {
            const u8* c = s_darc + s_el[e].off[0];
            s_el[e].r = c[0]; s_el[e].g = c[1]; s_el[e].b = c[2];
            any = true;
        }
    }
    return any;
}

static u8* readAll(const char* path, size_t* len) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    u8* b = (u8*)malloc((size_t)n);
    if (!b) { fclose(f); return NULL; }
    size_t rd = fread(b, 1, (size_t)n, f);
    fclose(f);
    if (rd != (size_t)n) { free(b); return NULL; }
    if (len) *len = (size_t)n;
    return b;
}

static bool loadBase(u8 region, bool forceTemplate) {
    if (s_darc) { free(s_darc); s_darc = NULL; s_darcLen = 0; }
    s_ready = false;

    char lfs[128];
    snprintf(lfs, sizeof(lfs), "sdmc:/luma/titles/%s/romfs/hud_LZ.bin", fs3dsHomeMenuTID(region));

    /* base preferida: o LayeredFS ja instalado (preserva packs, so recolore). */
    if (!forceTemplate) {
        u8* raw = NULL; size_t rawLen = 0;
        raw = readAll(lfs, &rawLen);
        if (raw) {
            s_darc = hudLz11Decompress(raw, rawLen, &s_darcLen);
            free(raw);
            if (s_darc) {
                s_ready = buildOffsets();
                if (s_ready) return true;
                /* base existe mas nao tem mat1 casavel -> cai pro template */
                free(s_darc); s_darc = NULL; s_darcLen = 0;
            }
            /* base corrompida/indecodificavel -> cai pro template (nunca ficar
             * "sem base" so porque um arquivo velho/quebrado esta no SD). */
        }
    }

    /* template bundled (so USA; outras regioes caem no USA como aproximacao).
     * RECONSTRUIDO 2026-07-11: o dump original veio de uma Home ja modificada
     * (bateria verde 00EE00, mod da Kitsune); revertido pros valores stock da
     * tabela AromaKitsune (normal 23AAE6, low F57D41, charging 23AFE6,
     * charger FF732E) via scripts da skill 3ds-homemenu-layeredfs. */
    {
        u8* raw = NULL; size_t rawLen = 0;
        raw = readAll("romfs:/homeui/hud_usa.bin", &rawLen);
        if (!raw) return false;
        s_darc = hudLz11Decompress(raw, rawLen, &s_darcLen);
        free(raw);
        if (!s_darc) return false;
    }
    s_ready = buildOffsets();
    return s_ready;
}

bool hudColorLoad(u8 region) { return loadBase(region, false); }
bool hudColorResetDefaults(u8 region) { return loadBase(region, true); }
bool hudColorReady(void) { return s_ready; }

bool hudColorRemoveLayered(u8 region) {
    char path[128];
    snprintf(path, sizeof(path), "sdmc:/luma/titles/%s/romfs/hud_LZ.bin", fs3dsHomeMenuTID(region));
    return remove(path) == 0;
}

void hudColorGet(HudElement el, u8* r, u8* g, u8* b) {
    if ((unsigned)el >= (unsigned)HUD_EL_COUNT) { if (r) *r = 255; if (g) *g = 255; if (b) *b = 255; return; }
    if (r) *r = s_el[el].r;
    if (g) *g = s_el[el].g;
    if (b) *b = s_el[el].b;
}

void hudColorSet(HudElement el, u8 r, u8 g, u8 b) {
    if ((unsigned)el >= (unsigned)HUD_EL_COUNT || !s_darc) return;
    s_el[el].r = r; s_el[el].g = g; s_el[el].b = b;
    for (int i = 0; i < s_el[el].n; i++) {
        u8* c = s_darc + s_el[el].off[i];
        c[0] = r; c[1] = g; c[2] = b; /* mantem alpha em c[3] */
    }
}

static void ensureDir(const char* p) { mkdir(p, 0777); }

static bool s_lastWriteValid = false;
bool hudColorLastWriteValid(void) { return s_lastWriteValid; }

/* re-le o arquivo gravado e confere: byte0==0x11 e descomprime pra magic 'darc'. */
static bool validateWritten(const char* path, size_t expectLen) {
    size_t len = 0;
    u8* raw = readAll(path, &len);
    if (!raw) return false;
    bool ok = (len == expectLen && raw[0] == 0x11);
    if (ok) {
        size_t dl = 0;
        u8* d = hudLz11Decompress(raw, len, &dl);
        ok = (d && dl >= 4 && d[0] == 'd' && d[1] == 'a' && d[2] == 'r' && d[3] == 'c');
        free(d);
    }
    free(raw);
    return ok;
}

bool hudColorApply(u8 region) {
    s_lastWriteValid = false;
    if (!s_ready || !s_darc) return false;
    size_t encLen = 0;
    u8* enc = hudLz11Compress(s_darc, s_darcLen, &encLen);
    if (!enc) return false;

    const char* tid = fs3dsHomeMenuTID(region);
    char dir[128];
    ensureDir("sdmc:/luma");
    ensureDir("sdmc:/luma/titles");
    snprintf(dir, sizeof(dir), "sdmc:/luma/titles/%s", tid); ensureDir(dir);
    snprintf(dir, sizeof(dir), "sdmc:/luma/titles/%s/romfs", tid); ensureDir(dir);

    char path[160];
    snprintf(path, sizeof(path), "sdmc:/luma/titles/%s/romfs/hud_LZ.bin", tid);
    FILE* f = fopen(path, "wb");
    if (!f) { free(enc); return false; }
    size_t wr = fwrite(enc, 1, encLen, f);
    fflush(f); fclose(f); free(enc);
    if (wr != encLen) return false;
    /* revalida no SD (prova que gravou de verdade e e um LZ11/darc valido). */
    s_lastWriteValid = validateWritten(path, encLen);
    return s_lastWriteValid;
}

/* le sdmc:/luma/config.ini e verifica enable_game_patching. */
HudEnv hudColorEnv(void) {
    FILE* f = fopen("sdmc:/luma/config.ini", "rb");
    if (!f) return HUD_ENV_NO_CONFIG;
    char line[256];
    HudEnv res = HUD_ENV_NO_PATCHING;
    while (fgets(line, sizeof(line), f)) {
        /* pula comentario/espaco */
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == ';' || *p == '#') continue;
        if (strncmp(p, "enable_game_patching", 20) == 0) {
            /* acha o '=' e o primeiro digito depois */
            char* eq = strchr(p, '=');
            if (eq) {
                eq++;
                while (*eq == ' ' || *eq == '\t') eq++;
                res = (*eq == '1') ? HUD_ENV_OK : HUD_ENV_NO_PATCHING;
            }
            break;
        }
    }
    fclose(f);
    return res;
}

const char* hudColorName(HudElement el) {
    switch (el) {
        case HUD_EL_BATTERY: return T(STR_HUD_BATTERY);
        case HUD_EL_CLOCK:   return T(STR_HUD_CLOCK);
        case HUD_EL_COINS:   return T(STR_HUD_COINS);
        case HUD_EL_STEPS:   return T(STR_HUD_STEPS);
        case HUD_EL_WIFI:    return T(STR_HUD_WIFI);
        default: return "?";
    }
}
