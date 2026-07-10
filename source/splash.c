#include "splash.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "lang.h"
#include "zip3ds.h"
#include "imgload.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

/* Mecanismo (spec anemone-3ds): um "entry" de splash e uma pasta em
 * sdmc:/splashes/ contendo splash.bin (top 240x400x3=288000) e/ou
 * splashbottom.bin (320x240x3=230400). Aplicar = copiar VERBATIM pra
 * /luma/splash.bin e /luma/splashbottom.bin. Remover = apagar os dois.
 * Nenhuma escrita em NAND/extdata -> so SD, seguro, sem reboot. */

#define SPLASH_DIR   "sdmc:/Splashes"
#define LUMA_TOP     "sdmc:/luma/splash.bin"
#define LUMA_BOTTOM  "sdmc:/luma/splashbottom.bin"
#define SPLASH_MAX   64
#define SP_VISIBLE   4
#define SP_MARGIN    16.0f
#define SP_ROW_H     46.0f
#define SP_ROW_GAP    6.0f
#define SP_TOP       12.0f

typedef struct { char name[256]; char path[600]; bool isZip; bool hasTop, hasBottom; } SplashEntry;

static SplashEntry s_entries[SPLASH_MAX];
static int s_count = 0;
static int s_selected = 0;
static int s_scrollTop = 0;
static float s_scrollAnim = 0.0f, s_scrollVel = 0.0f;
static PopupModal s_popup;
static int s_pendingAction = 0;    /* 1=aplicar, 2=remover */
static float s_toastT = 0.0f;
static char s_toast[96] = "";
static C2D_Image s_preview = {0};
static bool s_hasPreview = false;
static int s_previewFor = -2;      /* indice cujo preview esta carregado */

static void freePreview(void);
static void loadPreview(int i);

static bool fileExists(const char* p) {
    FILE* f = fopen(p, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

static bool isDir(const char* p) {
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool hasZipExt(const char* n) {
    size_t l = strlen(n);
    return l > 4 && n[l-4] == '.' &&
           (n[l-3]|32) == 'z' && (n[l-2]|32) == 'i' && (n[l-1]|32) == 'p';
}

static void showToast(const char* msg) {
    strncpy(s_toast, msg, sizeof(s_toast) - 1);
    s_toast[sizeof(s_toast) - 1] = '\0';
    s_toastT = 2.2f;
}

void splashInit(void) {
    s_count = 0; s_selected = 0; s_scrollTop = 0;
    s_scrollAnim = 0.0f; s_scrollVel = 0.0f;
    s_popup.active = false; s_pendingAction = 0; s_toastT = 0.0f;
    freePreview(); s_previewFor = -2;

    DIR* d = opendir(SPLASH_DIR);
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d)) != NULL && s_count < SPLASH_MAX) {
        if (e->d_name[0] == '.') continue;
        char full[600];
        snprintf(full, sizeof(full), "%s/%s", SPLASH_DIR, e->d_name);
        bool top = false, bot = false, isZip = false;
        if (isDir(full)) {
            char sub[700];
            snprintf(sub, sizeof(sub), "%s/splash.bin", full);       top = fileExists(sub);
            snprintf(sub, sizeof(sub), "%s/splashbottom.bin", full); bot = fileExists(sub);
        } else if (hasZipExt(e->d_name)) {
            isZip = true;
            top = zipHas(full, "splash.bin");
            bot = zipHas(full, "splashbottom.bin");
        }
        if (!top && !bot) continue; /* nem pasta nem zip com splash */
        SplashEntry* se = &s_entries[s_count++];
        snprintf(se->name, sizeof(se->name), "%s", e->d_name);
        if (isZip) { size_t l = strlen(se->name); if (l > 4) se->name[l-4] = '\0'; } /* tira .zip do display */
        snprintf(se->path, sizeof(se->path), "%s", full);
        se->isZip = isZip; se->hasTop = top; se->hasBottom = bot;
    }
    closedir(d);
    loadPreview(s_selected);
}

static bool copyFileRaw(const char* src, const char* dst) {
    FILE* f = fopen(src, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return false; }
    u8* b = (u8*)malloc((size_t)n);
    if (!b) { fclose(f); return false; }
    size_t rd = fread(b, 1, (size_t)n, f); fclose(f);
    if (rd != (size_t)n) { free(b); return false; }
    mkdir("sdmc:/luma", 0777);
    FILE* o = fopen(dst, "wb");
    if (!o) { free(b); return false; }
    size_t wr = fwrite(b, 1, (size_t)n, o); fclose(o); free(b);
    return wr == (size_t)n;
}

static bool writeBufToFile(const u8* buf, u32 n, const char* dst) {
    mkdir("sdmc:/luma", 0777);
    FILE* o = fopen(dst, "wb");
    if (!o) return false;
    size_t wr = fwrite(buf, 1, n, o);
    fclose(o);
    return wr == (size_t)n;
}

/* le um arquivo do entry (pasta OU zip) pra memoria (free normal). */
static u8* readEntryFile(const SplashEntry* se, const char* basename, u32* outSize) {
    if (se->isZip) {
        u8* b = NULL; u32 n = 0;
        if (!zipExtract(se->path, basename, &b, &n)) return NULL;
        *outSize = n; return b;
    }
    char p[700];
    snprintf(p, sizeof(p), "%s/%s", se->path, basename);
    FILE* f = fopen(p, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    u8* b = (u8*)malloc((size_t)n);
    if (!b) { fclose(f); return NULL; }
    size_t rd = fread(b, 1, (size_t)n, f); fclose(f);
    if (rd != (size_t)n) { free(b); return NULL; }
    *outSize = (u32)n; return b;
}

static void freePreview(void) {
    if (s_hasPreview) { imgFree(&s_preview); s_hasPreview = false; }
}

/* carrega preview.png do entry i (defensivo: falha -> sem preview, cai no texto). */
static void loadPreview(int i) {
    freePreview();
    s_previewFor = i;
    if (i < 0 || i >= s_count) return;
    u32 sz = 0; u8* png = readEntryFile(&s_entries[i], "preview.png", &sz);
    if (png) { s_hasPreview = imgLoadPng(png, sz, &s_preview); free(png); }
}

static void doApply(int i) {
    if (i < 0 || i >= s_count) return;
    SplashEntry* se = &s_entries[i];
    bool ok = false;
    if (se->isZip) {
        u8* b = NULL; u32 n = 0;
        if (se->hasTop && zipExtract(se->path, "splash.bin", &b, &n)) { ok |= writeBufToFile(b, n, LUMA_TOP); free(b); }
        if (se->hasBottom && zipExtract(se->path, "splashbottom.bin", &b, &n)) { ok |= writeBufToFile(b, n, LUMA_BOTTOM); free(b); }
    } else {
        char src[700];
        if (se->hasTop)    { snprintf(src, sizeof(src), "%s/splash.bin", se->path);       ok |= copyFileRaw(src, LUMA_TOP); }
        if (se->hasBottom) { snprintf(src, sizeof(src), "%s/splashbottom.bin", se->path); ok |= copyFileRaw(src, LUMA_BOTTOM); }
    }
    showToast(ok ? T(STR_SPLASH_DONE) : "ERRO");
}

static void doRemove(void) {
    remove(LUMA_TOP);
    remove(LUMA_BOTTOM);
    showToast(T(STR_SPLASH_REMOVED));
}

static void spClampScroll(void) {
    if (s_selected < s_scrollTop) s_scrollTop = s_selected;
    if (s_selected >= s_scrollTop + SP_VISIBLE) s_scrollTop = s_selected - SP_VISIBLE + 1;
    int maxTop = s_count - SP_VISIBLE; if (maxTop < 0) maxTop = 0;
    if (s_scrollTop > maxTop) s_scrollTop = maxTop;
    if (s_scrollTop < 0) s_scrollTop = 0;
}

static void rowRect(int i, float slideX, float* rx, float* ry, float* rw, float* rh) {
    *rx = SP_MARGIN + slideX;
    *rw = (float)SCREEN_BOT_WIDTH - 2.0f * SP_MARGIN;
    *ry = SP_TOP + ((float)i - s_scrollAnim) * (SP_ROW_H + SP_ROW_GAP);
    *rh = SP_ROW_H;
}

void splashUpdate(const AppInput* in, float dt, int* currentScreen) {
    popupUpdate(&s_popup);
    if (s_toastT > 0.0f) s_toastT -= dt;

    if (popupActive(&s_popup)) {
        if (popupConfirmInput(&s_popup, in) && s_popup.result == 1) {
            if (s_pendingAction == 1) doApply(s_selected);
            else if (s_pendingAction == 2) doRemove();
            s_pendingAction = 0;
        }
        return;
    }

    if (in->back) { freePreview(); *currentScreen = SCREEN_MAIN_MENU; return; }

    if (s_count > 0) {
        int prev = s_selected;
        if (in->downNav) s_selected = (s_selected + 1) % s_count;
        if (in->up)      s_selected = (s_selected - 1 + s_count) % s_count;
        if (s_selected != prev) { spClampScroll(); loadPreview(s_selected); }

        if (in->touchDown) {
            for (int i = 0; i < s_count; i++) {
                float x, y, w, h; rowRect(i, 0.0f, &x, &y, &w, &h);
                if (in->touchPX >= x && in->touchPX < x + w &&
                    in->touchPY >= y && in->touchPY < y + h) {
                    s_selected = i; spClampScroll();
                    s_pendingAction = 1;
                    popupShowConfirm(&s_popup, T(STR_SPLASH_CONFIRM));
                    return;
                }
            }
        }
        if (in->confirm) {
            s_pendingAction = 1;
            popupShowConfirm(&s_popup, T(STR_SPLASH_CONFIRM));
        }
    }
    /* X = remover splash aplicado (independe de haver entries). */
    if (in->down & KEY_X) {
        s_pendingAction = 2;
        popupShowConfirm(&s_popup, T(STR_SPLASH_REMOVE));
    }
}

void splashRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_SPLASH));

    float cardX = 24.0f + slideX;
    UI_Elevation(cardX, 88.0f, 352.0f, 128.0f, 16.0f, 2, 1.0f);
    if (UI_AssetsReady()) UI_NineCard(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);
    else UI_Card(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);

    if (s_count == 0) {
        UI_TextCenter(buf, NULL, T(STR_SPLASH_EMPTY), 200.0f + slideX, 140.0f, 0.28f, 0.28f, g_theme.textSecondary);
    } else if (s_hasPreview) {
        /* thumbnail a DIREITA e MAIOR (encaixa na altura do card), info a
         * ESQUERDA. */
        const float ch = 128.0f, top = 88.0f, pad = 8.0f;
        float iw = (float)s_preview.subtex->width, ih = (float)s_preview.subtex->height;
        float sc = (ch - 2.0f * pad) / ih;              /* enche a altura */
        float dw = iw * sc;
        if (dw > 224.0f) { sc = 224.0f / iw; dw = iw * sc; }
        float dh = ih * sc;
        float imgX = cardX + 352.0f - pad - dw;          /* encostado a direita */
        float dy = top + (ch - dh) * 0.5f;
        C2D_DrawImageAt(s_preview, imgX, dy, 0.6f, NULL, sc, sc);
        float lx = cardX + 16.0f, lw = imgX - lx - 12.0f;
        float lcx = lx + lw * 0.5f;
        UI_TextCenterFit(buf, NULL, s_entries[s_selected].name, lcx, top + 44.0f, 0.32f, 0.22f, lw, g_theme.textPrimary);
        char avail[48];
        snprintf(avail, sizeof(avail), "%s%s",
                 s_entries[s_selected].hasTop ? "TOP " : "",
                 s_entries[s_selected].hasBottom ? "BOTTOM" : "");
        UI_TextCenter(buf, NULL, avail, lcx, top + 78.0f, 0.24f, 0.24f, UI_AccentAnim());
    } else {
        UI_Text(buf, NULL, T(STR_ITEM_SPLASH_SUB), 44.0f + slideX, 106.0f, 0.22f, 0.22f, g_theme.textHint);
        UI_TextCenterFit(buf, NULL, s_entries[s_selected].name, 200.0f + slideX, 134.0f,
                         0.44f, 0.24f, 320.0f, g_theme.textPrimary);
        char avail[48];
        snprintf(avail, sizeof(avail), "%s%s",
                 s_entries[s_selected].hasTop ? "TOP " : "",
                 s_entries[s_selected].hasBottom ? "BOTTOM" : "");
        UI_TextCenter(buf, NULL, avail, 200.0f + slideX, 166.0f, 0.24f, 0.24f, UI_AccentAnim());
    }
    (void)0; /* aviso de "ative splash no Luma" removido (Luma do dono ja configurado) */

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }
}

void splashRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_BottomBackground();

    spClampScroll();
    {
        float d = uiFrameDt();
        float target = (float)s_scrollTop, diff = target - s_scrollAnim;
        s_scrollVel += (diff * 210.0f - s_scrollVel * 21.0f) * d;
        s_scrollAnim += s_scrollVel * d;
        if (fabsf(diff) < 0.001f && fabsf(s_scrollVel) < 0.01f) { s_scrollAnim = target; s_scrollVel = 0.0f; }
    }

    const float listTop = SP_TOP, listBot = SP_TOP + SP_VISIBLE * (SP_ROW_H + SP_ROW_GAP);
    for (int i = 0; i < s_count; i++) {
        float x, y, w, h; rowRect(i, slideX, &x, &y, &w, &h);
        if (y + h < listTop - 2.0f || y > listBot + 2.0f) continue;
        bool sel = (i == s_selected);
        float cy = y + h * 0.5f;
        ColorRGBA bg = sel ? themeCardSelBg() : g_theme.surface;
        UI_Elevation(x, y, w, h, 14.0f, sel ? 2 : 1, 1.0f);
        if (UI_AssetsReady()) UI_NineCard(x, y, w, h, 14.0f, bg);
        else UI_RoundRect(x, y, w, h, 14.0f, bg);
        if (sel) UI_FocusRing(x, y, w, h, 14.0f);
        ColorRGBA nameC = sel ? g_theme.textPrimary : g_theme.textSecondary;
        UI_Text(buf, NULL, s_entries[i].name, x + 16.0f, cy - 8.0f, 0.26f, 0.26f, nameC);
    }

    if (s_count > SP_VISIBLE) {
        float sbX = SCREEN_BOT_WIDTH - 6.0f + slideX;
        float sbH = SP_VISIBLE * (SP_ROW_H + SP_ROW_GAP) - SP_ROW_GAP;
        ColorRGBA track = g_theme.textHint; track.a = 40;
        UI_RoundRect(sbX, listTop, 3.0f, sbH, 1.5f, track);
        float thumbH = sbH * (float)SP_VISIBLE / (float)s_count;
        float thumbY = listTop + sbH * s_scrollAnim / (float)s_count;
        UI_RoundRect(sbX, thumbY, 3.0f, thumbH, 1.5f, UI_AccentAnim());
    }

    /* toast de feedback. */
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

    UI_HelpBar(buf, T(STR_SPLASH_HELP_L), T(STR_SAIR));
    popupRender(buf, &s_popup);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
