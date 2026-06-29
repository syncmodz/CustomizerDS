#include "fonts.h"
#include "lang.h"
#include "sysfont.h"
#include "common.h"
#include "theme.h"
#include "ui.h"
#include "anim.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

FontSystem g_fonts;

static int s_selected = 0;
static int s_scrollTop = 0; /* §5/§7: lista rolavel (9 fontes nao cabem todas) */
static bool s_sysfontPending = false; /* §5: o popup atual e de FONTE DE SISTEMA */

/* Geometria da lista de fontes (render + toque devem usar as MESMAS). */
#define FL_TOP      8.0f
#define FL_ROWH     32.0f
#define FL_ROWINNER 28.0f
#define FL_X        10.0f
#define FL_W        300.0f
#define FL_VISIBLE  6

/* Mantem s_scrollTop de forma que s_selected fique sempre visivel na janela. */
static void fontsClampScroll(void) {
    int n = fontsCount();
    if (s_selected < s_scrollTop) s_scrollTop = s_selected;
    if (s_selected >= s_scrollTop + FL_VISIBLE) s_scrollTop = s_selected - FL_VISIBLE + 1;
    int maxTop = (n > FL_VISIBLE) ? (n - FL_VISIBLE) : 0;
    s_scrollTop = clampi(s_scrollTop, 0, maxTop);
}
/* Popup de confirmacao "tem certeza?" (spec v4 4.2) -- aplicar uma fonte so
 * acontece depois do usuario confirmar A no popup, nunca direto no toque. */
static PopupModal s_popup;

/* Tela de instalacao animada (fonte de sistema -> CTRNAND, in-app). O install e
 * feito INCREMENTAL (um pedaco por frame) pra a barra encher suave; ao chegar a
 * 100% segura um instante e o console reinicia sozinho (sysfontReboot). */
static SysfontInstall s_inst;
static bool  s_installing = false;
static float s_instT = 0.0f;
static float s_instProg = 0.0f;
static bool  s_instDone = false;
static float s_instDoneT = 0.0f;
static char  s_instName[40] = {0};
static int   s_instIndex = 0; /* 1.4.0 §B1: fonte em instalacao (p/ marcar "em uso" so no sucesso real) */

/* v9 §10: 8 fontes (nomes proprios -- NAO traduzem no i18n). A fonte de
 * sistema continua existindo como FALLBACK seguro (fontsGetFont cai nela se um
 * .bcfnt nao carregar), mas nao e mais uma linha selecionavel da lista. */
static const char* FONT_LABELS[] = {
    "Comfortaa",
    "Comfortaa Negrito",
    "MADE Evolve Sans",
    "MADE Evolve Sans Negrito",
    "Love House",
    "Comic Sans MS3",
    "Super Mario 64",
};

/* Caminho dentro da romfs (arquivos .bcfnt em romfs/fonts, gerados com mkbcfnt
 * a partir dos .ttf/.otf reais). Indice correspondente em FONT_LABELS/g_fonts.fonts. */
static const char* FONT_PATHS[MAX_CUSTOM_FONTS] = {
    "romfs:/fonts/comfortaa_regular.bcfnt",
    "romfs:/fonts/comfortaa_bold.bcfnt",
    "romfs:/fonts/made_evolve_regular.bcfnt",
    "romfs:/fonts/made_evolve_bold.bcfnt",
    "romfs:/fonts/love_house.bcfnt",
    "romfs:/fonts/comic_sans_ms3.bcfnt",
    "romfs:/fonts/super_mario_64.bcfnt",
};

/* v14 §4 (hardware): .cia de fonte de sistema embutidos (1 por fonte custom,
 * MESMA ordem de FONT_PATHS), gerados por `make sysfont` em romfs/sysfont/
 * (ver Makefile/scripts/mk_sysfont_cia.py). Instalados na CTRNAND via AM
 * (sysfontInstallCia). SYSFONT_STOCK_CIA = fonte original do console (extraida
 * da NAND), usada pelo item 0 "Padrao do Sistema" pra RESTAURAR. */
static const char* FONT_CIA_PATHS[MAX_CUSTOM_FONTS] = {
    "romfs:/sysfont/SystemFont_comfortaa_regular.cia",
    "romfs:/sysfont/SystemFont_comfortaa_bold.cia",
    "romfs:/sysfont/SystemFont_made_evolve_regular.cia",
    "romfs:/sysfont/SystemFont_made_evolve_bold.cia",
    "romfs:/sysfont/SystemFont_love_house.cia",
    "romfs:/sysfont/SystemFont_comic_sans_ms3.cia",
    "romfs:/sysfont/SystemFont_super_mario_64.cia",
};
#define SYSFONT_STOCK_CIA "romfs:/sysfont/SystemFont_STOCK.cia"

int fontsSelected(void) { return s_selected; }

/* Nunca retorna NULL: se o .bcfnt de um indice nao carregou (arquivo ausente,
 * romfs nao montada etc.), cai pra fonte de sistema em vez de travar -- essa
 * falta de fallback era exatamente o que crashava a tela de Fontes antes. */
/* index 0 = FONTE DO SISTEMA (sempre disponivel); 1..N = fontes custom em
 * g_fonts.fonts[index-1]. Cai pra system font se um .bcfnt nao carregar. */
C2D_Font fontsGetFont(int index) {
    index = clampi(index, 0, fontsCount() - 1);
    if (index == 0) return g_fonts.systemFont;
    int ci = index - 1;
    if (ci < MAX_CUSTOM_FONTS && g_fonts.fonts[ci]) return g_fonts.fonts[ci];
    return g_fonts.systemFont;
}

static void applyFont(int index, bool save) {
    index = clampi(index, 0, fontsCount() - 1);
    g_fonts.currentIndex = index;
    g_fonts.current = fontsGetFont(index);
    if (save) {
        ConfigData cfg;
        configLoad(&cfg);
        cfg.fontIndex = (u8)index;
        configSave(&cfg);
    }
}

void fontsSystemInit(void) {
    for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
        g_fonts.fonts[i] = C2D_FontLoad(FONT_PATHS[i]);
    }
    g_fonts.systemFont = C2D_FontLoadSystem(CFG_REGION_USA);
    if (!g_fonts.systemFont) g_fonts.systemFont = C2D_FontLoadSystem(CFG_REGION_JPN);
    g_fonts.count = fontsCount();

    ConfigData cfg;
    configLoad(&cfg);
    int idx = clampi(cfg.fontIndex, 0, fontsCount() - 1);
    g_fonts.currentIndex = idx;
    g_fonts.current = fontsGetFont(idx);
}

void fontsInit(void) {
    s_selected = clampi(g_fonts.currentIndex, 0, fontsCount() - 1);
    s_popup.active = false;
}

void fontsSystemCleanup(void) {
    for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
        if (g_fonts.fonts[i]) { C2D_FontFree(g_fonts.fonts[i]); g_fonts.fonts[i] = NULL; }
    }
    if (g_fonts.systemFont) {
        C2D_FontFree(g_fonts.systemFont);
        g_fonts.systemFont = NULL;
    }
}

void fontsUpdate(const AppInput* in, int* currentScreen) {
    /* Tela de instalacao: dirige o install em fatias (1 pedaco/frame -> barra
     * anima) e BLOQUEIA toda a navegacao. Ao terminar, segura 100% um instante
     * e reinicia o console pra aplicar. */
    if (s_installing) {
        float dt = uiFrameDt();
        s_instT += dt;
        if (!s_instDone) {
            int r = sysfontInstallStep(&s_inst);
            s_instProg = s_inst.size ? (float)((double)s_inst.off / (double)s_inst.size) : 1.0f;
            if (r < 0) {
                s_installing = false;
                popupShow(&s_popup, T(STR_SYSFONT_FAIL), NULL, NULL);
            } else if (r == 0) {
                s_instDone = true; s_instDoneT = 0.0f; s_instProg = 1.0f;
            }
        } else {
            s_instDoneT += dt;
            /* 1.4.0 §B2: segura no 100% pro flourish (check + glow) respirar,
             * depois fade -> reboot. Persiste a fonte como "em uso" SO agora
             * (sucesso real -- nao finge sucesso se um Step falhar antes). */
            if (s_instDoneT >= 1.1f) {
                applyFont(s_instIndex, true);
                sysfontInstallEnd(&s_inst);
                sysfontReboot();
            }
        }
        return;
    }

    popupUpdate(&s_popup);

    /* Popup de confirmacao em pe: bloqueia o resto do input da tela (so
     * A/B/toque nos selos respondem), igual ao editor de hex em darkmode.c. */
    if (popupActive(&s_popup)) {
        if (popupConfirmInput(&s_popup, in) && s_popup.result == 1) {
            /* v14 §4 (hardware): instala o .cia de fonte embutido na CTRNAND
             * via AM (rota da FBI) e REINICIA pra aplicar. Item 0 "Padrao do
             * Sistema" instala a fonte STOCK (extraida da NAND do dono) =
             * restaurar. So funciona no hardware (no Azahar o AM install
             * falha -> popup de erro honesto). NAND backup recomendado. */
            s_sysfontPending = false;
            const char* cia = (s_selected == 0)
                ? SYSFONT_STOCK_CIA
                : FONT_CIA_PATHS[s_selected - 1];
            if (sysfontInstallBegin(&s_inst, cia) == SYSFONT_OK) {
                /* abre a tela de instalacao animada (o reboot vem no fim). */
                s_installing = true; s_instT = 0.0f; s_instProg = 0.0f;
                s_instDone = false; s_instDoneT = 0.0f;
                s_instIndex = s_selected;
                snprintf(s_instName, sizeof(s_instName), "%s", fontsLabel(s_selected));
            } else {
                popupShow(&s_popup, T(STR_SYSFONT_FAIL), NULL, NULL);
            }
        } else if (s_popup.result == -1) {
            s_sysfontPending = false;
        }
        return;
    }

    if (in->back) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (in->downNav) s_selected = (s_selected + 1) % fontsCount();
    if (in->up) s_selected = (s_selected - 1 + fontsCount()) % fontsCount();

    if (in->touchDown) {
        /* mesma janela rolavel do render (FL_*), mapeando o toque ao item visivel. */
        fontsClampScroll();
        int n = fontsCount();
        for (int v = 0; v < FL_VISIBLE && (s_scrollTop + v) < n; v++) {
            float by = FL_TOP + v * FL_ROWH;
            if (in->touchPY >= by && in->touchPY < by + FL_ROWINNER &&
                in->touchPX >= FL_X && in->touchPX < FL_X + FL_W) {
                /* Toque so seleciona/previa -- aplicar sempre passa pelo
                 * popup de confirmacao (A), nunca direto no toque. */
                s_selected = s_scrollTop + v;
                return;
            }
        }
    }

    /* 1.4.0 §FONTES (norte v19): acao PRIMARIA = X (aplicar como fonte do
     * SISTEMA, com confirmacao clara: reboot + backup NAND; em "Padrao do
     * Sistema" (0) = restaurar a original). A = previa no app (aplica a fonte
     * so no app, sem reiniciar). O preview da tela de cima ja acompanha a
     * navegacao; o A persiste essa escolha como fonte do app. */
    if (in->confirm) {
        applyFont(s_selected, true);
    }
    if (in->down & KEY_X) {
        s_sysfontPending = true;
        popupShowConfirm(&s_popup, s_selected == 0 ? T(STR_SYSFONT_RESTORE) : T(STR_SYSFONT_CONFIRM));
    }
}

static const char* previewLine(int index) {
    /* index 0 = sistema; 1..7 = fontes custom (na ordem de FONT_LABELS). */
    switch (index) {
        case 0: return "system default preview";
        case 1: return "round friendly interface";
        case 2: return "ROUND FRIENDLY BOLD";
        case 3: return "sleek future display";
        case 4: return "SLEEK FUTURE BOLD";
        case 5: return "lovely handwritten style";
        case 6: return "casual comic lettering";
        case 7: return "ITS A ME MARIO 123";       /* charset de jogo, limitado */
        default: return "system default preview";
    }
}

static const char* previewTitle(int index) {
    return fontsLabel(index); /* o titulo do preview e o proprio nome da fonte */
}

/* 1.4.0 §B2: instalacao MIRABOLANTE (norte = northstar_fonts_1.4.0.png painel
 * 3). Sem card -- takeover de tela: fundo accent-tingido que entra suave +
 * emblema das 3 bolinhas com GLOW respirando (1 sprite glow_radial, pulsa no
 * ritmo do progresso) + barra 9-slice (pill9) enchendo com easing + % subindo;
 * ao 100% um FLOURISH (emblema da pulso final + check verde com brilho) antes
 * do fade -> reboot. Os assets sao os do pacote 1.4.0 (UI_Glow/UI_NinePill/
 * UI_Check); fora dos cantos AA, nada de retangulo serrilhado. */
static void drawInstallOverlay(C2D_TextBuf buf) {
    float t = s_instT;
    float scrimA = clampf(t / 0.30f, 0.0f, 1.0f);   /* fade de entrada */
    bool done = s_instDone;
    float doneK = done ? clampf(s_instDoneT / 0.35f, 0.0f, 1.0f) : 0.0f;

    /* Fundo: scrim levemente tingido de accent (ambiencia do painel 3). */
    ColorRGBA scrim = themeMix(g_theme.backgroundTop, g_theme.accent, 0.10f);
    scrim.a = (u8)(232 * scrimA);
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, scrim);

    float mid = SCREEN_TOP_WIDTH * 0.5f;
    float emY = SCREEN_TOP_HEIGHT * 0.40f;
    float p = clampf(s_instProg, 0.0f, 1.0f);
    float pe = easeFunc(p, EASE_OUT_CUBIC);

    /* 1.4.0 §SEM-GLOW: instalacao LIMPA -- sem glow respirando atras do
     * emblema. So o emblema flat (3 aneis), com um leve pulso de escala no
     * fim (flourish) em vez de brilho. */
    UI_Emblem(mid, emY, 0.74f + doneK * 0.08f, uiFrameTime(), scrimA);

    UI_TextCenter(buf, NULL, done ? T(STR_SYSFONT_DONE) : T(STR_SYSFONT_INSTALLING),
                  mid, emY + 54, 0.26f, 0.26f, g_theme.textPrimary);
    UI_TextCenterFit(buf, NULL, s_instName, mid, emY + 76, 0.32f, 0.20f, 260.0f,
                     done ? g_theme.success : g_theme.accent);

    float by = emY + 106;
    if (!done) {
        float bw = 252.0f, bx = mid - bw * 0.5f, bh = 12.0f;
        ColorRGBA track = g_theme.textHint; track.a = (u8)(55 * scrimA);
        UI_NinePill(bx, by, bw, bh, track);
        if (pe > 0.001f) {
            ColorRGBA fill = g_theme.accent; fill.a = (u8)(255 * scrimA);
            UI_NinePill(bx, by, fmaxf(bh, bw * pe), bh, fill);
        }
        char pct[8]; snprintf(pct, sizeof(pct), "%d%%", (int)(p * 100.0f + 0.5f));
        UI_TextCenter(buf, NULL, pct, mid, by + 18, 0.22f, 0.22f, g_theme.textSecondary);
    } else {
        /* 1.4.0 §SEM-GLOW: check verde nitido (escala easeOutBack), sem glow atras. */
        float sc = easeOutBackAmp(doneK, 1.7f);
        UI_Check(mid, by + 8, 46.0f * sc, g_theme.success);
    }
}

void fontsRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    /* Parallax 3 camadas (Travel Motion), igual ao Menu: fundo decorativo
     * anda mais, o cartao no meio, o conteudo de dentro se acomoda primeiro. */
    float offsetRaw = (1.0f - transVal);
    float offset = offsetRaw * 40.0f;
    float offsetFg = offsetRaw * 18.0f;
    UI_TopBackground();
    UI_TopMenuBar(T(STR_TAB_FONTS), buf);

    /* Blobs decorativos de fundo removidos (mesma causa do bug "bolas
     * sobrepostas" da Inicio): caiam dentro do retangulo do card, que tem
     * alpha 235 (nao 255) e deixava-os espiar por tras. */

    /* Transicao 3.2: card desliza/escala a partir do centro; o fade real
     * (cobre tudo, mesmo widgets sem alpha proprio) vem do veu no final. */
    float cardBaseX = 16, cardBaseY = 30 + offset, cardBaseW = 368, cardBaseH = 196;
    float cardW = cardBaseW * scaleM, cardH = cardBaseH * scaleM;
    float cardX = cardBaseX + slideX + (cardBaseW - cardW) * 0.5f;
    float cardY = cardBaseY + (cardBaseH - cardH) * 0.5f;
    UI_Card(cardX, cardY, cardW, cardH, RADIUS_CARD, g_theme.surface);

    UI_Text(buf, NULL, T(STR_PREVIEW), 32 + slideX, 50 + offsetFg, 0.26f, 0.26f, g_theme.textSecondary);
    UI_Text(buf, NULL, previewTitle(s_selected), 32 + slideX, 68 + offsetFg, 0.42f, 0.42f, g_theme.textPrimary);

    char status[24];
    snprintf(status, sizeof(status), "%s",
             (s_selected == g_fonts.currentIndex) ? T(STR_IN_USE) : T(STR_TO_APPLY));
    /* §2: mede com a fonte/escala REAL do chrome (UI_TextWidth) -- senao a
     * pilula fica menor que o texto agora que o chrome e +25% e em Bold. */
    float tw = UI_TextWidth(buf, NULL, status, 0.22f);
    float minW = (float)strlen(status) * 7.5f * UI_TEXT_SCALE;
    float pw = fmaxf(tw, minW) + 18.0f;
    float badgeX = (16 + 368) - pw - 8 + slideX; /* relativo a borda direita do card (cima tem 400px, nao 320) */
    ColorRGBA badgeC = (s_selected == g_fonts.currentIndex) ? g_theme.success : g_theme.accent;
    if (UI_AssetsReady()) UI_NinePill(badgeX, 50 + offsetFg, pw, 18, badgeC);
    else UI_RoundRect(badgeX, 50 + offsetFg, pw, 18, 9, badgeC);
    ColorRGBA badgeText = themeContrastText(badgeC);
    UI_TextCenter(buf, NULL, status, badgeX + pw * 0.5f, 52 + offsetFg, 0.22f, 0.22f, badgeText);

    /* Bloco de preview ocupa o resto do card (sem vazio embaixo) -- mostra
     * a fonte real ja carregada, em duas linhas de tamanhos diferentes. */
    C2D_Font f = fontsGetFont(s_selected);
    float boxY = 96 + offsetFg;
    float boxH = 122;
    ColorRGBA previewBg = themeMix(g_theme.surfaceElevated, g_theme.accent, 0.07f);
    /* 336 (era 304): a caixa terminava 32px antes da borda direita do card
     * (margem 16 esq / 48 dir, assimetrico) -- spec v6 3: "conteudo
     * centralizado, espacamentos iguais". Card vai de 16 a 384 (368 largo);
     * caixa em 32..368 fecha com margem 16 simetrica dos dois lados. */
    UI_RoundFrame(32 + slideX, boxY, 336, boxH, 14, previewBg, (ColorRGBA){255, 255, 255, 12});

    /* Tamanhos uniformes (o "bold/alt" por indice fixo saiu -- com a fonte do
     * sistema no indice 0 os indices mudaram e aquilo viraria bug). */
    UI_Text(buf, f, previewLine(s_selected), 48 + slideX, boxY + 14, 0.36f, 0.36f, g_theme.textPrimary);
    UI_Text(buf, f, "Aa Bb Cc 123", 48 + slideX, boxY + 46, 0.44f, 0.44f, g_theme.accent);
    UI_Text(buf, f, "0123456789 !?", 48 + slideX, boxY + 84, 0.28f, 0.28f, g_theme.textSecondary);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 25, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT - 25, veil);
    }

    if (s_installing) drawInstallOverlay(buf);
}

void fontsRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    float offset = (1.0f - transVal) * 30.0f;
    (void)scaleM;
    /* Fundo UNIFORME (nao o bicolor de UI_BottomBackground): a aba Fontes nao
     * tem uma "Touch Bar" de controles no topo -- a lista ocupa tudo --, entao
     * a faixa escura de 50px em cima so criava aquele "retangulo preto + quadrado"
     * bicolor reclamado. Aqui a tela toda fica numa cor so. */
    UI_TouchBarBackground();

    /* §5/§7: lista ROLAVEL das 9 fontes (mostra FL_VISIBLE por vez). Nomes
     * MAIORES (0.30, na propria fonte) e a fonte EM USO marcada por uma
     * bolinha Reva (ICON_SWATCH_THIN tint success) em vez do selo de texto. */
    fontsClampScroll();
    int n = fontsCount();
    for (int v = 0; v < FL_VISIBLE && (s_scrollTop + v) < n; v++) {
        int i = s_scrollTop + v;
        float by = FL_TOP + v * FL_ROWH + offset;
        float ap = UI_StaggerT(v, 0.04f, 0.26f);
        if (ap <= 0.0f) continue;
        float slide = (1.0f - ap) * 10.0f;

        bool selected = (i == s_selected);   /* foco (D-pad) */
        bool isCurrent = (i == g_fonts.currentIndex); /* em uso */
        float rx = FL_X + slide + slideX;

        ColorRGBA bg = selected
            ? themeCardSelBg()
            : (themeIsDark() ? (ColorRGBA){26, 28, 38, 240} : (ColorRGBA){238, 240, 248, 240});
        ColorRGBA border = selected
            ? g_theme.accent
            : (themeIsDark() ? (ColorRGBA){255, 255, 255, 8} : (ColorRGBA){0, 0, 0, 8});
        if (selected) border.a = 110; /* realce de FOCO accent visivel (§6) */

        /* §3: anel de foco accent na linha focada (alem da borda accent). */
        if (selected) UI_FocusRing(rx, by + slide, FL_W, FL_ROWINNER, 14);
        if (selected) UI_Shadow(rx, by + slide, FL_W, FL_ROWINNER, 14, 30, 1.5f);
        /* 1.4.0 §A1: fundo da linha = pilula 9-slice (canto AA), com fallback. */
        if (UI_AssetsReady()) UI_NinePill(rx, by + slide, FL_W, FL_ROWINNER, bg);
        else UI_RoundFrame(rx, by + slide, FL_W, FL_ROWINNER, 14, bg, border);

        ColorRGBA textCol = selected ? g_theme.textPrimary : g_theme.textSecondary;
        UI_Text(buf, fontsGetFont(i), fontsLabel(i), rx + 14, by + 5 + slide, 0.30f, 0.30f, textCol);

        if (isCurrent) {
            /* bolinha Reva "em uso" (success) na ponta direita do item. */
            iconsDraw(ICON_SWATCH_THIN, rx + FL_W - 18, by + FL_ROWINNER * 0.5f + slide, 18.0f, g_theme.success, 1.0f);
        }
    }

    /* scrollbar arredondada na borda direita (font-safe, sem glifos unicode). */
    if (n > FL_VISIBLE) {
        float sbX = SCREEN_BOT_WIDTH - 6.0f, sbTop = FL_TOP + offset, sbH = FL_VISIBLE * FL_ROWH;
        ColorRGBA track = g_theme.textHint; track.a = 40;
        UI_RoundRect(sbX, sbTop, 3.0f, sbH, 1.5f, track);
        float thumbH = sbH * (float)FL_VISIBLE / (float)n;
        float thumbY = sbTop + sbH * (float)s_scrollTop / (float)n;
        UI_RoundRect(sbX, thumbY, 3.0f, thumbH, 1.5f, g_theme.accent);
    }

    char fhelp[64];
    snprintf(fhelp, sizeof(fhelp), "%s   %s", T(STR_HELP_FONTS_X), T(STR_HELP_FONTS_L));
    UI_HelpBar(buf, fhelp, T(STR_SAIR));

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 26, veil);
    }
    /* durante a instalacao a tela de baixo avisa pra nao desligar (norte painel
     * 3): scrim + "Nao desligue o console" / "reiniciando ao concluir...". */
    if (s_installing) {
        ColorRGBA scrim = g_theme.background; scrim.a = 228;
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, scrim);
        float mid = SCREEN_BOT_WIDTH * 0.5f, cy = SCREEN_BOT_HEIGHT * 0.5f;
        UI_TextCenter(buf, NULL, T(STR_SYSFONT_KEEPON), mid, cy - 16, 0.30f, 0.30f, g_theme.textPrimary);
        UI_TextCenter(buf, NULL, T(STR_SYSFONT_REBOOT_END), mid, cy + 14, 0.22f, 0.22f, g_theme.textSecondary);
    }
    popupRender(buf, &s_popup);
}

C2D_Font fontsCurrent(void) {
    return fontsGetFont(g_fonts.currentIndex);
}

const char* fontsCurrentName(void) {
    return fontsLabel(g_fonts.currentIndex);
}

const char* fontsLabel(int index) {
    index = clampi(index, 0, fontsCount() - 1);
    if (index == 0) return T(STR_FONT_SYSTEM); /* §A v10: 1a opcao = sistema */
    return FONT_LABELS[index - 1];
}

int fontsCount(void) {
    /* 1 (Padrao do Sistema) + N fontes custom. */
    return 1 + (int)(sizeof(FONT_LABELS) / sizeof(FONT_LABELS[0]));
}

bool fontsLoaded(int index) {
    if (index == 0) return true; /* sistema sempre disponivel */
    int ci = index - 1;
    if (ci < 0 || ci >= MAX_CUSTOM_FONTS) return true;
    return g_fonts.fonts[ci] != NULL;
}
