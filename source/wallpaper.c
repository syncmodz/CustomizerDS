#include "wallpaper.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "lang.h"
#include "fs3ds.h"
#include "zip3ds.h"
#include "imgload.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

/* Mecanismo (spec anemone-3ds): um "entry" de wallpaper e uma pasta em
 * sdmc:/wallpapers/ com body_LZ.bin (LZ11 cru, obrigatorio), bgm.bcstm
 * (opcional) e preview.png (opcional, so display). Aplicar =
 *   tema-ext:  /BodyCache.bin  <- body_LZ.bin (com byte[5]=1 se com musica)
 *              /BgmCache.bin    <- remade BGM_MAX_SIZE + bytes reais (ou zerado)
 *              /ThemeManage.bin <- struct 0x800
 *   home-ext:  /SaveData.dat    <- patch theme_entry {type=3,index=0xFF}, shuffle=0
 * depois "aplicar agora": .3dsx -> APT_HardwareResetAsync(); .cia ->
 * srvPublishToSubscriber(0x202,0). SO no console real (Azahar nao aplica). */

#define WALL_DIR       "sdmc:/Themes"
#define WALL_MAX       64
#define BODY_CACHE_SIZE 0x150000u
#define BGM_MAX_SIZE    0x337000u

#define W_VISIBLE   4
#define W_MARGIN    16.0f
#define W_ROW_H     46.0f
#define W_ROW_GAP    6.0f
#define W_TOP       12.0f

typedef struct { char name[256]; char path[600]; bool isZip; bool hasMusic; } WallEntry;

static WallEntry s_entries[WALL_MAX];
static int s_count = 0;
static int s_selected = 0;
static int s_scrollTop = 0;
static float s_scrollAnim = 0.0f, s_scrollVel = 0.0f;
static PopupModal s_popup;
static bool s_wantMusic = true;     /* Y alterna: aplicar com/sem musica */
static float s_toastT = 0.0f;
static char s_toast[96] = "";
static int s_pendingReboot = 0;     /* frames ate APT_HardwareResetAsync */
static C2D_Image s_preview = {0};
static bool s_hasPreview = false;
static int s_previewFor = -2;

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
    snprintf(s_toast, sizeof(s_toast), "%s", msg);
    s_toastT = 2.6f;
}

void wallpaperInit(void) {
    s_count = 0; s_selected = 0; s_scrollTop = 0;
    s_scrollAnim = 0.0f; s_scrollVel = 0.0f;
    s_popup.active = false; s_toastT = 0.0f; s_pendingReboot = 0;
    freePreview(); s_previewFor = -2;

    DIR* d = opendir(WALL_DIR);
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d)) != NULL && s_count < WALL_MAX) {
        if (e->d_name[0] == '.') continue;
        char full[600];
        snprintf(full, sizeof(full), "%s/%s", WALL_DIR, e->d_name);
        bool hasBody = false, music = false, isZip = false;
        if (isDir(full)) {
            char sub[700];
            snprintf(sub, sizeof(sub), "%s/body_LZ.bin", full); hasBody = fileExists(sub);
            snprintf(sub, sizeof(sub), "%s/bgm.bcstm", full);   music = fileExists(sub);
        } else if (hasZipExt(e->d_name)) {
            isZip = true;
            hasBody = zipHas(full, "body_LZ.bin");
            music = zipHas(full, "bgm.bcstm");
        }
        if (!hasBody) continue;   /* nem pasta nem zip com corpo de tema */
        WallEntry* w = &s_entries[s_count++];
        snprintf(w->name, sizeof(w->name), "%s", e->d_name);
        if (isZip) { size_t l = strlen(w->name); if (l > 4) w->name[l-4] = '\0'; } /* tira .zip do display */
        snprintf(w->path, sizeof(w->path), "%s", full);
        w->isZip = isZip; w->hasMusic = music;
    }
    closedir(d);
    loadPreview(s_selected);
}

/* ---- leitura de arquivo do SD ---- */
static u8* readFile(const char* path, u32* outSize, u32 cap) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    u32 sz = (u32)n;
    if (cap && sz > cap) sz = cap;   /* clampa (ex: BGM_MAX_SIZE) */
    u8* b = (u8*)malloc(sz);
    if (!b) { fclose(f); return NULL; }
    size_t rd = fread(b, 1, sz, f); fclose(f);
    if (rd != sz) { free(b); return NULL; }
    *outSize = sz;
    return b;
}

static void put_u32(u8* p, u32 v) {
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

/* le um arquivo do tema (pasta extraida OU dentro do .zip). free() normal. */
static u8* readThemeFile(const WallEntry* w, const char* basename, u32* outSize) {
    if (w->isZip) {
        u8* b = NULL; u32 n = 0;
        if (!zipExtract(w->path, basename, &b, &n)) return NULL;
        *outSize = n;
        return b;
    }
    char p[700];
    snprintf(p, sizeof(p), "%s/%s", w->path, basename);
    return readFile(p, outSize, 0);
}

static void freePreview(void) {
    if (s_hasPreview) { imgFree(&s_preview); s_hasPreview = false; }
}

/* preview.png do tema i (pasta ou zip). Defensivo. */
static void loadPreview(int i) {
    freePreview();
    s_previewFor = i;
    if (i < 0 || i >= s_count) return;
    u32 sz = 0; u8* png = readThemeFile(&s_entries[i], "preview.png", &sz);
    if (png) { s_hasPreview = imgLoadPng(png, sz, &s_preview); free(png); }
}

/* Aplica o tema selecionado. Retorna string de resultado p/ toast. */
static const char* applyTheme(int idx, bool withMusic) {
    if (idx < 0 || idx >= s_count) return T(STR_WALL_FAIL);
    if (!fs3dsThemeReady()) return T(STR_WALL_NOEXT);

    const WallEntry* we = &s_entries[idx];
    bool music = withMusic && we->hasMusic;

    /* --- 1. body_LZ.bin --- */
    u32 bodySz = 0;
    u8* body = readThemeFile(we, "body_LZ.bin", &bodySz);
    if (!body) return T(STR_WALL_FAIL);

    /* com musica: garantir byte[5]=1 no body DESCOMPRIMIDO (flag "tem BGM").
     * so recomprime se ainda nao estiver setado (evita inchar via encoder
     * literal). */
    if (music) {
        u32 dsz = 0;
        u8* dec = lz11Decompress(body, bodySz, &dsz);
        if (dec && dsz > 5 && dec[5] != 1) {
            dec[5] = 1;
            u32 nsz = 0;
            u8* re = lz11CompressFast(dec, dsz, &nsz);
            if (re) { free(body); body = re; bodySz = nsz; }
        }
        if (dec) free(dec);
    }

    FS_Archive th = fs3dsThemeExt();
    FS_Archive hm = fs3dsHomeExt();
    bool ok = fs3dsWrite(th, "/BodyCache.bin", body, bodySz);
    free(body);
    if (!ok) return T(STR_WALL_FAIL);

    /* --- 2. BgmCache.bin --- com musica: remake BGM_MAX_SIZE + bytes reais no
     * offset 0 (igual remake_file+buf_to_file do Anemone). Sem musica: BgmCache
     * zerado de BGM_MAX_SIZE. bcstm maior que o limite = ERRO (nunca truncar). */
    u32 musicSz = 0;
    if (music) {
        u32 bgmSz = 0;
        u8* bgm = readThemeFile(we, "bgm.bcstm", &bgmSz);
        if (bgm && bgmSz > BGM_MAX_SIZE) { free(bgm); return T(STR_WALL_FAIL); }
        if (!fs3dsRemakeZeroed(th, "/BgmCache.bin", BGM_MAX_SIZE)) { if (bgm) free(bgm); return T(STR_WALL_FAIL); }
        if (bgm) {
            fs3dsWriteAt(th, "/BgmCache.bin", 0, bgm, bgmSz);
            musicSz = bgmSz;
            free(bgm);
        }
    } else {
        if (!fs3dsRemakeZeroed(th, "/BgmCache.bin", BGM_MAX_SIZE)) return T(STR_WALL_FAIL);
    }

    /* --- 3. ThemeManage.bin (0x800): read-modify-write igual ao Anemone.
     * music_size SO e escrito no modo com-musica (no sem-musica o Anemone
     * deixa o valor antigo -- inofensivo pq body[5]=0 diz "sem BGM"). --- */
    u8 tmp[0x800];
    u8* tm = NULL; u32 tmSz = 0;
    if (fs3dsReadAll(th, "/ThemeManage.bin", &tm, &tmSz) && tmSz >= 0x800) {
        /* usa o buffer existente (preserva campos nao tocados) */
    } else {
        if (tm) { free(tm); tm = NULL; }
        memset(tmp, 0, sizeof(tmp));
        tm = tmp;
    }
    put_u32(tm + 0x00, 1);              /* unk1 */
    put_u32(tm + 0x04, 0);              /* unk2 */
    put_u32(tm + 0x08, bodySz);         /* body_size */
    if (music) put_u32(tm + 0x0C, musicSz); /* music_size so no modo BGM */
    put_u32(tm + 0x10, 0xFF);           /* unk3 */
    put_u32(tm + 0x14, 1);              /* unk4 */
    put_u32(tm + 0x18, 0xFF);           /* dlc_theme_content_index */
    put_u32(tm + 0x1C, 0x0200);         /* use_theme_cache */
    bool tmOk = fs3dsWrite(th, "/ThemeManage.bin", tm, 0x800);
    if (tm != tmp) free(tm);
    if (!tmOk) return T(STR_WALL_FAIL);

    /* --- 4. SaveData.dat: patch theme_entry {type=3,index=0xFF}, shuffle=0 --- */
    u8* sd = NULL; u32 sdSz = 0;
    if (fs3dsReadAll(hm, "/SaveData.dat", &sd, &sdSz)) {
        const u32 TE = 0x13b8;              /* offset do theme_entry */
        if (sdSz >= TE + 100) {
            put_u32(sd + TE, 0xFF);         /* index = 0xFF */
            sd[TE + 4] = 0;                 /* dlc_low */
            sd[TE + 5] = 3;                 /* type = 3 (tema) */
            sd[TE + 6] = 0; sd[TE + 7] = 0; /* unk */
            memset(sd + TE + 8, 0, 80);     /* shuffle_themes[10] zerado */
            sd[TE + 99] = 0;                /* shuffle = 0 */
            fs3dsWrite(hm, "/SaveData.dat", sd, sdSz);
        }
        free(sd);
    }

    /* --- 5. aplicar agora --- */
    if (envIsHomebrew()) {
        s_pendingReboot = 2;                /* .3dsx: reboot em ~2 frames */
        return T(STR_WALL_REBOOT);
    } else {
        srvPublishToSubscriber(0x202, 0);   /* .cia: Home Menu recarrega ao vivo */
        return T(STR_WALL_DONE);
    }
}

static void wClampScroll(void) {
    if (s_selected < s_scrollTop) s_scrollTop = s_selected;
    if (s_selected >= s_scrollTop + W_VISIBLE) s_scrollTop = s_selected - W_VISIBLE + 1;
    int maxTop = s_count - W_VISIBLE; if (maxTop < 0) maxTop = 0;
    if (s_scrollTop > maxTop) s_scrollTop = maxTop;
    if (s_scrollTop < 0) s_scrollTop = 0;
}

static void rowRect(int i, float slideX, float* rx, float* ry, float* rw, float* rh) {
    *rx = W_MARGIN + slideX;
    *rw = (float)SCREEN_BOT_WIDTH - 2.0f * W_MARGIN;
    *ry = W_TOP + ((float)i - s_scrollAnim) * (W_ROW_H + W_ROW_GAP);
    *rh = W_ROW_H;
}

static void confirmApply(void) {
    bool music = s_wantMusic && s_count > 0 && s_entries[s_selected].hasMusic;
    popupShowConfirm(&s_popup, music ? T(STR_WALL_CONFIRM_MUSIC) : T(STR_WALL_CONFIRM));
}

void wallpaperUpdate(const AppInput* in, float dt, int* currentScreen) {
    if (s_pendingReboot > 0) {
        if (--s_pendingReboot == 0) { APT_HardwareResetAsync(); return; }
        return;   /* congela input durante o pre-reboot */
    }

    popupUpdate(&s_popup);
    if (s_toastT > 0.0f) s_toastT -= dt;

    if (popupActive(&s_popup)) {
        if (popupConfirmInput(&s_popup, in) && s_popup.result == 1)
            showToast(applyTheme(s_selected, s_wantMusic));
        return;
    }

    if (in->back) { freePreview(); *currentScreen = SCREEN_MAIN_MENU; return; }

    if (s_count > 0) {
        int prev = s_selected;
        if (in->downNav) s_selected = (s_selected + 1) % s_count;
        if (in->up)      s_selected = (s_selected - 1 + s_count) % s_count;
        if (s_selected != prev) { wClampScroll(); loadPreview(s_selected); }

        if (in->down & KEY_Y) s_wantMusic = !s_wantMusic;

        if (in->touchDown) {
            for (int i = 0; i < s_count; i++) {
                float x, y, w, h; rowRect(i, 0.0f, &x, &y, &w, &h);
                if (in->touchPX >= x && in->touchPX < x + w &&
                    in->touchPY >= y && in->touchPY < y + h) {
                    s_selected = i; wClampScroll();
                    confirmApply();
                    return;
                }
            }
        }
        if (in->confirm) confirmApply();
    }
}

void wallpaperRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_WALL));

    float cardX = 24.0f + slideX;
    UI_Elevation(cardX, 88.0f, 352.0f, 128.0f, 16.0f, 2, 1.0f);
    if (UI_AssetsReady()) UI_NineCard(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);
    else UI_Card(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);

    bool music = s_count > 0 && s_wantMusic && s_entries[s_selected].hasMusic;
    const char* mtag = (s_count > 0 && !s_entries[s_selected].hasMusic) ? T(STR_WALL_NOMUSIC)
                       : (music ? T(STR_WALL_HASMUSIC) : T(STR_WALL_NOMUSIC));
    if (s_count == 0) {
        UI_TextCenter(buf, NULL, T(STR_WALL_EMPTY), 200.0f + slideX, 140.0f, 0.28f, 0.28f, g_theme.textSecondary);
    } else if (s_hasPreview) {
        /* thumbnail a DIREITA e MAIOR, nome + musica a ESQUERDA. */
        const float ch = 128.0f, top = 88.0f, pad = 8.0f;
        float iw = (float)s_preview.subtex->width, ih = (float)s_preview.subtex->height;
        float sc = (ch - 2.0f * pad) / ih;
        float dw = iw * sc;
        if (dw > 224.0f) { sc = 224.0f / iw; dw = iw * sc; }
        float dh = ih * sc;
        float imgX = cardX + 352.0f - pad - dw;
        float dy = top + (ch - dh) * 0.5f;
        C2D_DrawImageAt(s_preview, imgX, dy, 0.6f, NULL, sc, sc);
        float lx = cardX + 16.0f, lw = imgX - lx - 12.0f;
        float lcx = lx + lw * 0.5f;
        UI_TextCenterFit(buf, NULL, s_entries[s_selected].name, lcx, top + 44.0f, 0.32f, 0.20f, lw, g_theme.textPrimary);
        UI_TextCenter(buf, NULL, mtag, lcx, top + 78.0f, 0.24f, 0.24f, UI_AccentAnim());
    } else {
        UI_Text(buf, NULL, T(STR_ITEM_WALL_SUB), 44.0f + slideX, 106.0f, 0.20f, 0.20f, g_theme.textHint);
        UI_TextCenterFit(buf, NULL, s_entries[s_selected].name, 200.0f + slideX, 134.0f,
                         0.44f, 0.24f, 320.0f, g_theme.textPrimary);
        UI_TextCenter(buf, NULL, mtag, 200.0f + slideX, 166.0f, 0.24f, 0.24f, UI_AccentAnim());
    }
    /* so avisa se a extdata do tema NAO abriu (emulador OU sem permissao). No
     * console com extdata ok, nao polui a tela com "so no 3DS real". */
    if (!fs3dsThemeReady())
        UI_TextCenter(buf, NULL, T(STR_WALL_NOEXT), 200.0f + slideX, 194.0f, 0.26f, 0.26f, g_theme.warning);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }
}

void wallpaperRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_BottomBackground();

    wClampScroll();
    {
        float d = uiFrameDt();
        float target = (float)s_scrollTop, diff = target - s_scrollAnim;
        s_scrollVel += (diff * 210.0f - s_scrollVel * 21.0f) * d;
        s_scrollAnim += s_scrollVel * d;
        if (fabsf(diff) < 0.001f && fabsf(s_scrollVel) < 0.01f) { s_scrollAnim = target; s_scrollVel = 0.0f; }
    }

    const float listTop = W_TOP, listBot = W_TOP + W_VISIBLE * (W_ROW_H + W_ROW_GAP);
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
        /* ponto de accent na direita = tema tem musica. */
        if (s_entries[i].hasMusic) {
            ColorRGBA mc = UI_AccentAnim(); mc.a = sel ? 255 : 150;
            UI_RoundRect(x + w - 22.0f, cy - 4.0f, 8.0f, 8.0f, 4.0f, mc);
        }
    }

    if (s_count > W_VISIBLE) {
        float sbX = SCREEN_BOT_WIDTH - 6.0f + slideX;
        float sbH = W_VISIBLE * (W_ROW_H + W_ROW_GAP) - W_ROW_GAP;
        ColorRGBA track = g_theme.textHint; track.a = 40;
        UI_RoundRect(sbX, listTop, 3.0f, sbH, 1.5f, track);
        float thumbH = sbH * (float)W_VISIBLE / (float)s_count;
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

    UI_HelpBar(buf, T(STR_WALL_HELP_L), T(STR_SAIR));
    popupRender(buf, &s_popup);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
