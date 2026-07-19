#include "homeui.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "lang.h"
#include "fs3ds.h"
#include "hudcolor.h"
#include "colorwheel.h"
#include "icons.h"
#include "sysfont.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* 2.0.0: aba UI da Home = INSTALADOR DE PACKS via LayeredFS (jeito cooolgamer/
 * aromakitsune). Voce poe um pack em sdmc:/homeui/<nome>/ (a arvore luma do
 * cooolgamer serve direto) e o app acha o titles/<tid-da-tua-regiao> dentro
 * dele e copia pra sdmc:/luma/titles/<tid>/. SD-only -> sem risco de brick;
 * remover desfaz. Precisa game-patching no Luma + reboot. */

#define HOMEUI_DIR   "sdmc:/homeui"
#define HOMEUI_MAX   64
#define H_VISIBLE    4
#define H_MARGIN     16.0f
#define H_ROW_H      46.0f
#define H_ROW_GAP     6.0f
#define H_TOP        12.0f

typedef struct { char name[256]; } UiPack;

static UiPack s_packs[HOMEUI_MAX];
static int s_count = 0, s_selected = 0, s_scrollTop = 0;
static float s_scrollAnim = 0.0f, s_scrollVel = 0.0f;
static PopupModal s_popup;
static int s_pendingAction = 0;   /* 1=instalar, 2=remover */
static float s_toastT = 0.0f;
static char s_toast[96] = "";
static char s_tid[20] = "0004003000008F02";

/* ---- editor de cor do HUD ---- */
#define HMODE_EDITOR 0   /* tela principal: abas Presets | Elementos */
#define HMODE_PICKER 1   /* roda HSV */
#define HMODE_LIST   2   /* instalador de packs (avancado, via botao Packs) */

#define TAB_PRESETS  0
#define TAB_ELEMENTS 1

/* presets prontos (nossos): cada um pinta TODOS os elementos do HUD. */
typedef struct { const char* name; u8 r, g, b; } HudPreset;
static const HudPreset PRESETS[] = {
    { "Vermelho", 0xE6, 0x39, 0x46 },
    { "Verde",    0x06, 0xD6, 0xA0 },
    { "Ciano",    0x00, 0xB4, 0xD8 },
    { "Azul",     0x43, 0x61, 0xEE },
    { "Roxo",     0x9B, 0x5D, 0xE5 },
    { "Rosa",     0xFF, 0x5D, 0xA2 },
};
#define PRESET_COUNT ((int)(sizeof(PRESETS) / sizeof(PRESETS[0])))
#define PRESET_COLS  3

static int s_mode = HMODE_EDITOR;
static int s_edTab = TAB_PRESETS;
static int s_pSel = 0;               /* selecao na grade de presets */
static int s_eSel = 0;               /* selecao na lista de elementos */
static Tween s_tabTween;             /* morph do segmented */
static HudElement s_pickEl = HUD_EL_BATTERY;
static ColorWheel s_wheel;
static u8 s_region = 1;
static bool s_rebooting = false;     /* aplicou -> vai reiniciar (estilo Anemone) */
static float s_rebootT = 0.0f;
static HudEnv s_env = HUD_ENV_OK;    /* cache do diagnostico (ler config.ini toda
                                      * frame no render custaria acesso ao SD 60x/s) */

static IconID elemIcon(HudElement el) {
    switch (el) {
        case HUD_EL_BATTERY: return ICON_BATTERY;
        case HUD_EL_CLOCK:   return ICON_CLOCK;
        case HUD_EL_COINS:   return ICON_COIN;
        case HUD_EL_STEPS:   return ICON_STEPS;
        case HUD_EL_WIFI:    return ICON_WIFI;
        default:             return ICON_COUNT;
    }
}

/* total de linhas do instalador de packs */
static int listTotal(void) { return s_count > 0 ? s_count : 0; }

static bool isDir(const char* p) {
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static void showToast(const char* m) { snprintf(s_toast, sizeof(s_toast), "%s", m); s_toastT = 3.0f; }

void homeuiInit(void) {
    s_count = 0; s_selected = 0; s_scrollTop = 0;
    s_scrollAnim = 0.0f; s_scrollVel = 0.0f;
    s_popup.active = false; s_pendingAction = 0; s_toastT = 0.0f;
    s_mode = HMODE_EDITOR; s_edTab = TAB_PRESETS; s_pSel = 0; s_eSel = 0;
    s_rebooting = false; s_rebootT = 0.0f;
    s_region = fs3dsRegion();
    snprintf(s_tid, sizeof(s_tid), "%s", fs3dsHomeMenuTID(fs3dsRegion()));
    hudColorLoad(s_region);   /* carrega base + cores atuais (USA tem template) */
    s_env = hudColorEnv();    /* diagnostico 1x na entrada (nao por frame) */

    DIR* d = opendir(HOMEUI_DIR);
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d)) != NULL && s_count < HOMEUI_MAX) {
        if (e->d_name[0] == '.') continue;
        char sub[512];
        snprintf(sub, sizeof(sub), "%s/%s", HOMEUI_DIR, e->d_name);
        if (!isDir(sub)) continue;
        snprintf(s_packs[s_count].name, sizeof(s_packs[s_count].name), "%s", e->d_name);
        s_count++;
    }
    closedir(d);
}

/* ---- copia SD->SD em CHUNKS (64KB) -- nunca carrega o arquivo inteiro em
 * heap (packs podem ter body/banners grandes). ---- */
#define COPY_CHUNK (64 * 1024)
static bool copyFile(const char* src, const char* dst) {
    FILE* f = fopen(src, "rb");
    if (!f) return false;
    FILE* o = fopen(dst, "wb");
    if (!o) { fclose(f); return false; }
    static u8 buf[COPY_CHUNK];
    bool ok = true;
    size_t n;
    while ((n = fread(buf, 1, COPY_CHUNK, f)) > 0) {
        if (fwrite(buf, 1, n, o) != n) { ok = false; break; }
    }
    if (ferror(f)) ok = false;
    fclose(f); fclose(o);
    if (!ok) remove(dst);   /* nao deixa arquivo pela metade */
    return ok;
}

/* ---- manifesto do pack instalado: lista dos arquivos QUE NOS copiamos p/
 * /luma/titles/<tid>/. doRemove() apaga SO eles -- nunca a arvore inteira
 * (que pode ter mods de outras ferramentas). ---- */
#define MANIFEST_PATH "sdmc:/3ds/CustomizerDS/installed_pack.txt"

static void manifestAdd(FILE* m, const char* dstPath) {
    if (m) fprintf(m, "%s\n", dstPath);
}

/* copia recursiva com manifesto; false = falhou (caller faz rollback). */
static bool copyTreeM(const char* src, const char* dst, FILE* m) {
    mkdir(dst, 0777);
    DIR* d = opendir(src);
    if (!d) return false;
    bool ok = true;
    struct dirent* e;
    while (ok && (e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.' && (e->d_name[1] == '\0' || (e->d_name[1] == '.' && e->d_name[2] == '\0'))) continue;
        char s[768], t[768];
        snprintf(s, sizeof(s), "%s/%s", src, e->d_name);
        snprintf(t, sizeof(t), "%s/%s", dst, e->d_name);
        struct stat st;
        if (stat(s, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            ok = copyTreeM(s, t, m);
        } else {
            ok = copyFile(s, t);
            if (ok) manifestAdd(m, t);
        }
    }
    closedir(d);
    return ok;
}

/* remove os arquivos listados no manifesto (e tenta rmdir dirs que vazarem). */
static int removeManifested(void) {
    FILE* m = fopen(MANIFEST_PATH, "rb");
    if (!m) return -1;   /* sem manifesto */
    char line[768];
    int n = 0;
    while (fgets(line, sizeof(line), m)) {
        size_t len = strlen(line);
        while (len && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        if (!len) continue;
        if (remove(line) == 0) n++;
        /* tenta limpar diretorios-pai vazios (rmdir falha se nao vazio -- ok) */
        char* slash = strrchr(line, '/');
        while (slash && slash > line + 5) {
            *slash = '\0';
            if (rmdir(line) != 0) break;
            slash = strrchr(line, '/');
        }
    }
    fclose(m);
    remove(MANIFEST_PATH);
    return n;
}

/* acha (recursivo) a pasta chamada exatamente <tid> dentro do pack -- cobre a
 * arvore do cooolgamer (For Luma/<REGIAO>/luma/titles/<tid>/). */
static bool findTitleDir(const char* dir, const char* tid, char* out, size_t outsz, int depth) {
    if (depth > 8) return false;
    DIR* d = opendir(dir);
    if (!d) return false;
    bool found = false;
    struct dirent* e;
    while ((e = readdir(d)) != NULL && !found) {
        if (e->d_name[0] == '.') continue;
        char p[768];
        snprintf(p, sizeof(p), "%s/%s", dir, e->d_name);
        if (!isDir(p)) continue;
        if (strcasecmp(e->d_name, tid) == 0) { snprintf(out, outsz, "%s", p); found = true; break; }
        if (findTitleDir(p, tid, out, outsz, depth + 1)) { found = true; break; }
    }
    closedir(d);
    return found;
}

static void doInstall(int i) {
    if (i < 0 || i >= s_count) { showToast(T(STR_HOMEUI_FAIL)); return; }
    char pack[512];
    snprintf(pack, sizeof(pack), "%s/%s", HOMEUI_DIR, s_packs[i].name);
    char dst[128];
    snprintf(dst, sizeof(dst), "sdmc:/luma/titles/%s", s_tid);
    mkdir("sdmc:/luma", 0777);
    mkdir("sdmc:/luma/titles", 0777);
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/CustomizerDS", 0777);

    /* manifesto: registra cada arquivo copiado. Falhou no meio -> rollback
     * (remove o que copiou) pra nunca deixar a Home com mistura de pack. */
    FILE* m = fopen(MANIFEST_PATH, "wb");

    bool ok = false;
    char titleSrc[768];
    if (findTitleDir(pack, s_tid, titleSrc, sizeof(titleSrc), 0)) {
        /* pack tipo cooolgamer: copia o conteudo do titles/<tid> achado. */
        ok = copyTreeM(titleSrc, dst, m);
    } else {
        /* pack simples: <pack>/romfs -> /luma/titles/<tid>/romfs. */
        char romfsSrc[600];
        snprintf(romfsSrc, sizeof(romfsSrc), "%s/romfs", pack);
        if (isDir(romfsSrc)) {
            mkdir(dst, 0777);
            char rdst[160];
            snprintf(rdst, sizeof(rdst), "%s/romfs", dst);
            ok = copyTreeM(romfsSrc, rdst, m);
        }
    }
    if (m) fclose(m);
    if (!ok) removeManifested();   /* rollback: apaga o que chegou a copiar */
    showToast(ok ? T(STR_HOMEUI_DONE) : T(STR_HOMEUI_FAIL));
}

static void doRemove(void) {
    /* NUNCA apagar titles/<tid> inteiro: pode ter mods de OUTRAS ferramentas.
     * Remove so o que o manifesto diz que NOS instalamos; sem manifesto,
     * remove apenas o hud_LZ.bin (o unico arquivo que o editor de cor gera). */
    int n = removeManifested();
    if (n < 0) hudColorRemoveLayered(s_region);
    showToast(T(STR_HOMEUI_REMOVED));
}

static void hClampScroll(void) {
    if (s_selected < s_scrollTop) s_scrollTop = s_selected;
    if (s_selected >= s_scrollTop + H_VISIBLE) s_scrollTop = s_selected - H_VISIBLE + 1;
    int maxTop = listTotal() - H_VISIBLE; if (maxTop < 0) maxTop = 0;
    if (s_scrollTop > maxTop) s_scrollTop = maxTop;
    if (s_scrollTop < 0) s_scrollTop = 0;
}

static void rowRect(int i, float slideX, float* rx, float* ry, float* rw, float* rh) {
    *rx = H_MARGIN + slideX;
    *rw = (float)SCREEN_BOT_WIDTH - 2.0f * H_MARGIN;
    *ry = H_TOP + ((float)i - s_scrollAnim) * (H_ROW_H + H_ROW_GAP);
    *rh = H_ROW_H;
}

/* ===== geometria do editor (tela de baixo 320x240) ===== */
#define SEG_X   12.0f
#define SEG_Y   8.0f
#define SEG_W   232.0f
#define SEG_H   30.0f
#define PKS_X   252.0f
#define PKS_W   56.0f
#define CONT_Y  46.0f        /* topo da area de conteudo */
#define ACT_Y   188.0f       /* barra de acao (Aplicar/Padrao) */
#define ACT_H   26.0f

static void chipRect(int i, float slideX, float* x, float* y, float* w, float* h) {
    int col = i % PRESET_COLS, row = i / PRESET_COLS;
    float gap = 8.0f, m = 12.0f;
    float cw = (SCREEN_BOT_WIDTH - 2.0f * m - (PRESET_COLS - 1) * gap) / PRESET_COLS;
    float ch = (ACT_Y - 8.0f - CONT_Y - gap) / 2.0f;
    *x = m + col * (cw + gap) + slideX;
    *y = CONT_Y + row * (ch + gap);
    *w = cw; *h = ch;
}

static void elemRect(int i, float slideX, float* x, float* y, float* w, float* h) {
    float rh = 26.0f, gap = 2.0f;
    *x = 12.0f + slideX;
    *w = (float)SCREEN_BOT_WIDTH - 24.0f;
    *y = CONT_Y + (float)i * (rh + gap);
    *h = rh;
}

static bool hitRect(const AppInput* in, float x, float y, float w, float h) {
    return in->touchPX >= x && in->touchPX < x + w && in->touchPY >= y && in->touchPY < y + h;
}

/* aplica as cores atuais na Home e REINICIA (igual Anemone). */
static void doApplyReboot(void) {
    if (!hudColorReady()) { showToast(T(STR_HUD_NOBASE)); return; }
    if (!hudColorApply(s_region)) { showToast(T(STR_HUD_WRITE_FAIL)); return; }
    /* gravou e revalidou. Se o Luma nao tiver game patching, avisa e NAO reinicia
     * (reiniciar nao adiantaria) -- o dono precisa ligar no Luma primeiro. */
    s_env = hudColorEnv();
    if (s_env == HUD_ENV_NO_PATCHING) { showToast(T(STR_HUD_WARN_PATCHING)); return; }
    showToast(T(STR_HUD_APPLIED));
    s_rebooting = true; s_rebootT = 0.9f;
}

/* Restaurar padrao: apaga o override -> Home volta ao HUD stock -> REINICIA. */
static void doRestoreReboot(void) {
    hudColorRemoveLayered(s_region);       /* apaga o hud_LZ do LayeredFS */
    hudColorResetDefaults(s_region);       /* recarrega base p/ o editor mostrar padrao */
    showToast(T(STR_HUD_RESET));
    s_rebooting = true; s_rebootT = 0.9f;
}

/* preset = pinta todos os elementos + aplica + reinicia (1 toque, estilo Anemone). */
static void applyPreset(int p) {
    if (p < 0 || p >= PRESET_COUNT) return;
    if (!hudColorReady()) { showToast(T(STR_HUD_NOBASE)); return; }
    for (int e = 0; e < HUD_EL_COUNT; e++)
        hudColorSet((HudElement)e, PRESETS[p].r, PRESETS[p].g, PRESETS[p].b);
    doApplyReboot();
}

static u8 s_pickOrigR, s_pickOrigG, s_pickOrigB;  /* p/ B = cancelar de verdade */

static void openPicker(HudElement el) {
    if (!hudColorReady()) { showToast(T(STR_HUD_NOBASE)); return; }
    s_pickEl = el;
    u8 r, g, b; hudColorGet(el, &r, &g, &b);
    s_pickOrigR = r; s_pickOrigG = g; s_pickOrigB = b;
    colorWheelInit(&s_wheel);
    colorWheelSetRGB(&s_wheel, r, g, b);
    s_mode = HMODE_PICKER;
}

/* ---- modo PICKER: edita a cor de um elemento (roda HSV) ---- */
static void updatePicker(const AppInput* in, float dt) {
    colorWheelInput(&s_wheel, in, dt);
    u8 r, g, b; colorWheelGetRGB(&s_wheel, &r, &g, &b);
    hudColorSet(s_pickEl, r, g, b);                    /* preview ao vivo */
    if (in->confirm) { s_mode = HMODE_EDITOR; return; } /* A = confirma */
    if (in->back) {                                     /* B = CANCELA (reverte) */
        hudColorSet(s_pickEl, s_pickOrigR, s_pickOrigG, s_pickOrigB);
        s_mode = HMODE_EDITOR;
    }
}

/* ---- modo EDITOR: abas Presets(grade) | Elementos(lista) ---- */
static void updateEditor(const AppInput* in, int* currentScreen) {
    if (in->back) { *currentScreen = SCREEN_MAIN_MENU; return; }

    /* trocar de aba: ombros L/R ou toque no segmented */
    if (in->down & KEY_L) { s_edTab = TAB_PRESETS;  }
    if (in->down & KEY_R) { s_edTab = TAB_ELEMENTS; }
    if (in->touchDown && hitRect(in, SEG_X, SEG_Y, SEG_W, SEG_H))
        s_edTab = (in->touchPX < SEG_X + SEG_W * 0.5f) ? TAB_PRESETS : TAB_ELEMENTS;
    /* botao Packs (avancado) */
    if (in->touchDown && hitRect(in, PKS_X, SEG_Y, PKS_W, SEG_H)) {
        s_mode = HMODE_LIST; s_selected = 0; s_scrollTop = 0; return;
    }

    /* barra de acao: Aplicar (reinicia) / Padrao (reinicia) */
    if ((in->down & KEY_Y)) { doApplyReboot(); return; }
    if ((in->down & KEY_X)) { doRestoreReboot(); return; }
    if (in->touchDown && hitRect(in, SEG_X, ACT_Y, SEG_W, ACT_H)) { doApplyReboot(); return; }
    if (in->touchDown && hitRect(in, PKS_X, ACT_Y, PKS_W, ACT_H)) { doRestoreReboot(); return; }

    if (s_edTab == TAB_PRESETS) {
        if (in->right)   s_pSel = (s_pSel + 1) % PRESET_COUNT;
        if (in->left)    s_pSel = (s_pSel - 1 + PRESET_COUNT) % PRESET_COUNT;
        if (in->downNav) s_pSel = (s_pSel + PRESET_COLS) % PRESET_COUNT;
        if (in->up)      s_pSel = (s_pSel - PRESET_COLS + PRESET_COUNT) % PRESET_COUNT;
        if (in->touchDown) {
            for (int i = 0; i < PRESET_COUNT; i++) {
                float x, y, w, h; chipRect(i, 0.0f, &x, &y, &w, &h);
                if (hitRect(in, x, y, w, h)) { s_pSel = i; applyPreset(i); return; }
            }
        }
        if (in->confirm) applyPreset(s_pSel);
    } else {
        if (in->downNav) s_eSel = (s_eSel + 1) % HUD_EL_COUNT;
        if (in->up)      s_eSel = (s_eSel - 1 + HUD_EL_COUNT) % HUD_EL_COUNT;
        if (in->touchDown) {
            for (int i = 0; i < HUD_EL_COUNT; i++) {
                float x, y, w, h; elemRect(i, 0.0f, &x, &y, &w, &h);
                if (hitRect(in, x, y, w, h)) { s_eSel = i; openPicker((HudElement)i); return; }
            }
        }
        if (in->confirm) openPicker((HudElement)s_eSel);
    }
}

void homeuiUpdate(const AppInput* in, float dt, int* currentScreen) {
    popupUpdate(&s_popup);
    if (s_toastT > 0.0f) s_toastT -= dt;

    /* aplicou -> segura um instante mostrando "Reiniciando..." e reinicia
     * (igual Anemone: clicou, aplicou, console reinicia sozinho). */
    if (s_rebooting) {
        s_rebootT -= dt;
        if (s_rebootT <= 0.0f) sysfontReboot();
        return;
    }

    if (popupActive(&s_popup)) {
        if (popupConfirmInput(&s_popup, in) && s_popup.result == 1) {
            if (s_pendingAction == 1) doInstall(s_selected);
            else if (s_pendingAction == 2) doRemove();
            s_pendingAction = 0;
        }
        return;
    }

    if (s_mode == HMODE_PICKER) { updatePicker(in, dt); return; }
    if (s_mode == HMODE_EDITOR) { updateEditor(in, currentScreen); return; }

    /* ---- modo LISTA (instalador de packs, avancado) ---- */
    if (in->back) { s_mode = HMODE_EDITOR; return; }

    int total = listTotal();
    if (total > 0) {
        int prev = s_selected;
        if (in->downNav) s_selected = (s_selected + 1) % total;
        if (in->up)      s_selected = (s_selected - 1 + total) % total;
        if (s_selected != prev) hClampScroll();

        if (in->touchDown) {
            for (int i = 0; i < total; i++) {
                float x, y, w, h; rowRect(i, 0.0f, &x, &y, &w, &h);
                if (in->touchPX >= x && in->touchPX < x + w &&
                    in->touchPY >= y && in->touchPY < y + h) {
                    s_selected = i; hClampScroll();
                    s_pendingAction = 1;
                    popupShowConfirm(&s_popup, T(STR_HOMEUI_CONFIRM));
                    return;
                }
            }
        }
        if (in->confirm) {
            s_pendingAction = 1;
            popupShowConfirm(&s_popup, T(STR_HOMEUI_CONFIRM));
        }
        if (in->down & KEY_X) {
            s_pendingAction = 2;
            popupShowConfirm(&s_popup, T(STR_HOMEUI_REMOVE));
        }
    }
}

/* card do editor no top: PREVIEW do HUD ao vivo (5 icones nas cores atuais) +
 * nome/cor do item em foco. */
static void renderEditorTop(C2D_TextBuf buf, float slideX) {
    float cardX = 24.0f + slideX;
    UI_Elevation(cardX, 82.0f, 352.0f, 134.0f, 16.0f, 2, 1.0f);
    if (UI_AssetsReady()) UI_NineCard(cardX, 82.0f, 352.0f, 134.0f, 16.0f, g_theme.surface);
    else UI_Card(cardX, 82.0f, 352.0f, 134.0f, 16.0f, g_theme.surface);

    UI_Text(buf, NULL, T(STR_HUD_TITLE), 44.0f + slideX, 96.0f, 0.19f, 0.19f, g_theme.textHint);

    /* aviso de ambiente: se o LayeredFS nao for aplicar, diz na cara (fim do
     * "apliquei e ficou igual" silencioso). */
    if (s_env != HUD_ENV_OK) {
        const char* warn = (s_env == HUD_ENV_NO_PATCHING) ? T(STR_HUD_WARN_PATCHING) : T(STR_HUD_WARN_NOCFG);
        ColorRGBA wc = { 0xE6, 0x9A, 0x2E, 255 };  /* ambar de aviso */
        UI_TextCenterFit(buf, NULL, warn, 200.0f + slideX, 205.0f, 0.19f, 0.17f, 344.0f, wc);
    }

    /* ---- preview do HUD ao vivo: barra com os 5 icones nas cores escolhidas ---- */
    float stripY = 128.0f;
    ColorRGBA stripBg = themeIsDark() ? (ColorRGBA){255,255,255,10} : (ColorRGBA){20,24,34,12};
    UI_RoundRect(96.0f + slideX, stripY - 18.0f, 208.0f, 40.0f, 12.0f, stripBg);
    for (int i = 0; i < HUD_EL_COUNT; i++) {
        u8 r, g, b; hudColorGet((HudElement)i, &r, &g, &b);
        ColorRGBA c = { r, g, b, 255 };
        float ix = 200.0f + slideX + (float)(i - (HUD_EL_COUNT - 1) / 2.0f) * 42.0f;
        iconsDraw(elemIcon((HudElement)i), ix, stripY, 26.0f, c, 1.0f);
    }

    /* ---- item em foco: nome + swatch + hex ---- */
    const char* label; u8 r = 0, g = 0, b = 0;
    if (s_mode == HMODE_PICKER) { label = hudColorName(s_pickEl); colorWheelGetRGB(&s_wheel, &r, &g, &b); }
    else if (s_edTab == TAB_PRESETS) { label = PRESETS[s_pSel].name; r = PRESETS[s_pSel].r; g = PRESETS[s_pSel].g; b = PRESETS[s_pSel].b; }
    else { label = hudColorName((HudElement)s_eSel); hudColorGet((HudElement)s_eSel, &r, &g, &b); }

    ColorRGBA sw = { r, g, b, 255 };
    float bx = 44.0f + slideX, by = 176.0f, bw = 26.0f, bh = 26.0f;
    ColorRGBA border = themeIsDark() ? (ColorRGBA){255,255,255,30} : (ColorRGBA){20,24,34,30};
    UI_RoundFrame(bx, by, bw, bh, 8.0f, sw, border);
    UI_Text(buf, NULL, label, bx + 36.0f, by - 1.0f, 0.34f, 0.34f, g_theme.textPrimary);
    char hx[16]; snprintf(hx, sizeof(hx), "#%02X%02X%02X", r, g, b);
    UI_Text(buf, NULL, hx, bx + 36.0f, by + 14.0f, 0.20f, 0.20f, g_theme.textSecondary);
}

void homeuiRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_HOMEUI));

    if (s_mode != HMODE_LIST) {
        renderEditorTop(buf, slideX);
        if (fadeA < 0.999f) {
            ColorRGBA veil = g_theme.backgroundTop;
            veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
            UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
        }
        return;
    }

    float cardX = 24.0f + slideX;
    UI_Elevation(cardX, 88.0f, 352.0f, 128.0f, 16.0f, 2, 1.0f);
    if (UI_AssetsReady()) UI_NineCard(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);
    else UI_Card(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);

    UI_Text(buf, NULL, T(STR_ITEM_HOMEUI_SUB), 44.0f + slideX, 106.0f, 0.20f, 0.20f, g_theme.textHint);
    if (s_count == 0) {
        UI_TextCenterFit(buf, NULL, T(STR_HOMEUI_EMPTY), 200.0f + slideX, 138.0f, 0.26f, 0.20f, 320.0f, g_theme.textSecondary);
    } else {
        UI_TextCenterFit(buf, NULL, s_packs[s_selected].name, 200.0f + slideX, 134.0f, 0.44f, 0.24f, 320.0f, g_theme.textPrimary);
        char reg[64];
        snprintf(reg, sizeof(reg), "%s: %s", T(STR_HOMEUI_REGION), s_tid);
        UI_TextCenter(buf, NULL, reg, 200.0f + slideX, 166.0f, 0.20f, 0.20f, UI_AccentAnim());
    }
    UI_TextCenter(buf, NULL, T(STR_HOMEUI_REBOOT), 200.0f + slideX, 196.0f, 0.22f, 0.22f, g_theme.textHint);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }
}

/* uma linha de card generica (usada pela lista e pelo editor). */
/* linha de card: icone SF a esquerda (opcional, tintado) + label + swatch de
 * cor. Se ha icone e swatch, o swatch vai pra DIREITA; se so swatch, fica a
 * esquerda. icon = ICON_COUNT desliga o icone. */
static void drawRow(C2D_TextBuf buf, float x, float y, float w, float h, bool sel,
                    const char* label, ColorRGBA labelC, IconID icon, const u8* swatch) {
    float cy = y + h * 0.5f;
    ColorRGBA bg = sel ? themeCardSelBg() : g_theme.surface;
    UI_Elevation(x, y, w, h, 14.0f, sel ? 2 : 1, 1.0f);
    if (UI_AssetsReady()) UI_NineCard(x, y, w, h, 14.0f, bg);
    else UI_RoundRect(x, y, w, h, 14.0f, bg);
    if (sel) UI_FocusRing(x, y, w, h, 14.0f);

    float tx = x + 16.0f;
    bool hasIcon = ((unsigned)icon < (unsigned)ICON_COUNT);
    if (hasIcon) {
        iconsDraw(icon, x + 28.0f, cy, 22.0f, labelC, 1.0f);
        tx = x + 48.0f;
    }
    if (swatch) {
        ColorRGBA sw = { swatch[0], swatch[1], swatch[2], 255 };
        ColorRGBA bd = themeIsDark() ? (ColorRGBA){255,255,255,40} : (ColorRGBA){20,24,34,40};
        if (hasIcon) {
            /* swatch a direita */
            UI_RoundFrame(x + w - 34.0f, cy - 11.0f, 22.0f, 22.0f, 6.0f, sw, bd);
        } else {
            UI_RoundFrame(x + 14.0f, cy - 11.0f, 22.0f, 22.0f, 6.0f, sw, bd);
            tx = x + 46.0f;
        }
    }
    UI_Text(buf, NULL, label, tx, cy - 8.0f, 0.26f, 0.26f, labelC);
}

static void renderEditorBottom(C2D_TextBuf buf, float slideX) {
    /* ---- abas segmentadas + botao Packs ---- */
    const char* tabs[2] = { T(STR_HUD_TAB_PRESETS), T(STR_HUD_TAB_ELEMENTS) };
    UI_TouchBarSegmented(buf, SEG_X + slideX, SEG_Y, SEG_W, SEG_H, tabs, 2, s_edTab, &s_tabTween, true);
    UI_PillButton(buf, PKS_X + slideX, SEG_Y, PKS_W, SEG_H, NULL, NULL, ICON_PACK, false, 1.0f);

    if (s_edTab == TAB_PRESETS) {
        /* grade de chips coloridos */
        for (int i = 0; i < PRESET_COUNT; i++) {
            float x, y, w, h; chipRect(i, slideX, &x, &y, &w, &h);
            bool sel = (i == s_pSel);
            ColorRGBA c = { PRESETS[i].r, PRESETS[i].g, PRESETS[i].b, 255 };
            UI_Elevation(x, y, w, h, 12.0f, sel ? 3 : 1, 1.0f);
            UI_RoundRect(x, y, w, h, 12.0f, c);
            if (sel) UI_FocusRing(x, y, w, h, 12.0f);
            ColorRGBA tc = themeContrastText(c);
            UI_TextCenterFit(buf, NULL, PRESETS[i].name, x + w * 0.5f, y + h * 0.5f - 8.0f, 0.24f, 0.20f, w - 8.0f, tc);
        }
    } else {
        /* lista de elementos com icone + swatch */
        for (int i = 0; i < HUD_EL_COUNT; i++) {
            float x, y, w, h; elemRect(i, slideX, &x, &y, &w, &h);
            bool sel = (i == s_eSel);
            HudElement el = (HudElement)i;
            u8 r, g, b; hudColorGet(el, &r, &g, &b);
            u8 sw[3] = { r, g, b };
            ColorRGBA nameC = sel ? g_theme.textPrimary : g_theme.textSecondary;
            drawRow(buf, x, y, w, h, sel, hudColorName(el), nameC, elemIcon(el), sw);
        }
    }

    /* ---- barra de acao: Aplicar (reinicia) + Padrao ---- */
    UI_PillButton(buf, SEG_X + slideX, ACT_Y, SEG_W, ACT_H, T(STR_HUD_APPLY), NULL, ICON_APPLY, true, 1.0f);
    UI_PillButton(buf, PKS_X + slideX, ACT_Y, PKS_W, ACT_H, NULL, NULL, ICON_RESET, false, 1.0f);

    UI_HelpBar(buf, T(STR_HUD_HELP), T(STR_SAIR));
}

void homeuiRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_BottomBackground();

    /* aplicou -> tela de "Reiniciando..." antes do reboot (estilo Anemone) */
    if (s_rebooting) {
        ColorRGBA acc = UI_AccentAnim(); acc.a = 255;
        UI_TextCenter(buf, NULL, T(STR_HUD_REBOOTING), SCREEN_BOT_WIDTH * 0.5f, 108.0f, 0.30f, 0.30f, acc);
        return;
    }

    if (s_mode == HMODE_PICKER) {
        colorWheelRender(buf, &s_wheel);
        UI_HelpBar(buf, T(STR_HUD_PICK_HELP), T(STR_SAIR));
        if (fadeA < 0.999f) {
            ColorRGBA veil = g_theme.background;
            veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
            UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
        }
        return;
    }
    if (s_mode == HMODE_EDITOR) {
        renderEditorBottom(buf, slideX);
        if (s_toastT > 0.0f) {
            float a = clampf(s_toastT / 0.4f, 0.0f, 1.0f);
            ColorRGBA bg = g_theme.accent; bg.a = (u8)(230 * a);
            float tw = UI_TextWidth(buf, NULL, s_toast, 0.24f) + 28.0f;
            float tx = (SCREEN_BOT_WIDTH - tw) * 0.5f;
            if (UI_AssetsReady()) UI_NinePill(tx, 158.0f, tw, 24.0f, bg);
            else UI_RoundRect(tx, 158.0f, tw, 24.0f, 12.0f, bg);
            ColorRGBA tc = themeContrastText(g_theme.accent); tc.a = (u8)(255 * a);
            UI_TextCenter(buf, NULL, s_toast, SCREEN_BOT_WIDTH * 0.5f, 163.0f, 0.24f, 0.24f, tc);
        }
        if (fadeA < 0.999f) {
            ColorRGBA veil = g_theme.background;
            veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
            UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
        }
        return;
    }

    hClampScroll();
    {
        float dd = uiFrameDt();
        float target = (float)s_scrollTop, diff = target - s_scrollAnim;
        s_scrollVel += (diff * 210.0f - s_scrollVel * 21.0f) * dd;
        s_scrollAnim += s_scrollVel * dd;
        if (fabsf(diff) < 0.001f && fabsf(s_scrollVel) < 0.01f) { s_scrollAnim = target; s_scrollVel = 0.0f; }
    }

    const float listTop = H_TOP, listBot = H_TOP + H_VISIBLE * (H_ROW_H + H_ROW_GAP);
    int total = listTotal();
    if (total == 0)
        UI_TextCenter(buf, NULL, T(STR_HOMEUI_EMPTY), SCREEN_BOT_WIDTH * 0.5f + slideX, 100.0f, 0.24f, 0.20f, g_theme.textSecondary);
    for (int i = 0; i < total; i++) {
        float x, y, w, h; rowRect(i, slideX, &x, &y, &w, &h);
        if (y + h < listTop - 2.0f || y > listBot + 2.0f) continue;
        bool sel = (i == s_selected);
        ColorRGBA nameC = sel ? g_theme.textPrimary : g_theme.textSecondary;
        drawRow(buf, x, y, w, h, sel, s_packs[i].name, nameC, ICON_PACK, NULL);
    }

    if (total > H_VISIBLE) {
        float sbX = SCREEN_BOT_WIDTH - 6.0f + slideX;
        float sbH = H_VISIBLE * (H_ROW_H + H_ROW_GAP) - H_ROW_GAP;
        ColorRGBA track = g_theme.textHint; track.a = 40;
        UI_RoundRect(sbX, listTop, 3.0f, sbH, 1.5f, track);
        float thumbH = sbH * (float)H_VISIBLE / (float)total;
        float thumbY = listTop + sbH * s_scrollAnim / (float)total;
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

    UI_HelpBar(buf, T(STR_HOMEUI_HELP_L), T(STR_SAIR));
    popupRender(buf, &s_popup);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
}
