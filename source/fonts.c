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
#include <sys/stat.h> /* mkdir p/ auto-copia das fontes pro SD (PART 4.1) */

FontSystem g_fonts;

static int s_selected = 0;
static int s_scrollTop = 0; /* §5/§7: lista rolavel (9 fontes nao cabem todas) */
/* 1.5.0: rolagem FLUIDA -- s_scrollTop e o alvo (inteiro), s_scrollAnim segue
 * suave em "linhas" fracionarias, entao a lista desliza em vez de saltar. */
static float s_scrollAnim = 0.0f;
static bool s_sysfontPending = false; /* §5: o popup atual e de FONTE DE SISTEMA */

/* Geometria da lista AGRUPADA de fontes (espec v20; render + toque iguais):
 * 1 card x16 y18 w288 h168 r15; linhas h32 a partir de y20; texto em x32;
 * 5 linhas visiveis (rola se houver mais). */
#define FL_CARD_X   16.0f
#define FL_CARD_Y   18.0f
#define FL_CARD_W   288.0f
#define FL_CARD_H   168.0f
#define FL_ROW0     20.0f
#define FL_ROWH     32.0f
#define FL_VISIBLE  5
#define FL_TEXT_X   32.0f

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
static bool  s_instSim = false; /* 1.4.0 PART4: install SIMULADO (Azahar/sem AM) -- anima sem reboot, aviso honesto */

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

/* 1.4.0 PART 4.1: no 1o boot, copia os .cia de fonte embutidos pra
 * sdmc:/3ds/CustomizerDS/fonts/ (pro dono ter as fontes no SD p/ FBI/backup),
 * sem recopiar se ja existirem. Nao trava o app se o SD falhar. */
static void fontsCopyToSD(void) {
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/CustomizerDS", 0777);
    mkdir("sdmc:/3ds/CustomizerDS/fonts", 0777);
    char sd[96];
    int customs = fontsCount() - 1; /* indices 0..customs-1 em FONT_CIA_PATHS */
    for (int i = 0; i < customs && i < MAX_CUSTOM_FONTS; i++) {
        const char* rp = FONT_CIA_PATHS[i];
        if (!rp) continue;
        const char* base = strrchr(rp, '/'); base = base ? base + 1 : rp;
        snprintf(sd, sizeof(sd), "sdmc:/3ds/CustomizerDS/fonts/%s", base);
        sysfontCopyToSDIfAbsent(rp, sd);
    }
    sysfontCopyToSDIfAbsent(SYSFONT_STOCK_CIA,
                            "sdmc:/3ds/CustomizerDS/fonts/SystemFont_STOCK.cia");
}

void fontsInit(void) {
    s_selected = clampi(g_fonts.currentIndex, 0, fontsCount() - 1);
    s_popup.active = false;
    fontsCopyToSD(); /* PART 4.1: garante as fontes no SD (1x; pula as ja copiadas) */
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
            if (s_instSim) {
                /* PART4: Azahar/sem AM -- progresso por TEMPO (~2.5s), sem NAND. */
                s_instProg = fmaxf(s_instProg, clampf(s_instT / 2.5f, 0.0f, 1.0f));
                if (s_instProg >= 1.0f) { s_instDone = true; s_instDoneT = 0.0f; s_instProg = 1.0f; }
            } else {
                int r = sysfontInstallStep(&s_inst);
                s_instProg = s_inst.size ? (float)((double)s_inst.off / (double)s_inst.size) : 1.0f;
                if (r < 0) {
                    /* PART4: Step falhou (ex.: Azahar) -> vira SIMULADO (anima ate
                     * 100% por tempo + aviso honesto), em vez de FAIL seco. */
                    s_instSim = true;
                } else if (r == 0) {
                    s_instDone = true; s_instDoneT = 0.0f; s_instProg = 1.0f;
                }
            }
        } else {
            s_instDoneT += dt;
            if (s_instSim) {
                /* Simulado: NAO reinicia. Mostra o aviso honesto e fecha quando o
                 * dono aperta A/B/X (depois de uma pausinha pro check aparecer). */
                if (s_instDoneT > 0.5f && (in->confirm || in->back || (in->down & KEY_X))) {
                    s_installing = false;
                }
            } else {
                /* 1.4.0 §B2: segura no 100% pro check aparecer, depois fade ->
                 * reboot. Persiste a fonte como "em uso" SO agora (sucesso real). */
                if (s_instDoneT >= 1.25f) {
                    applyFont(s_instIndex, true);
                    sysfontInstallEnd(&s_inst);
                    sysfontReboot();
                }
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
            /* PART4: abre a tela de instalacao animada SEMPRE. Se o AM iniciar
             * (hardware) = install REAL -> reboot ao fim. Se falhar (Azahar/sem
             * AM) = modo SIMULADO -> mesma animacao por tempo + aviso honesto,
             * sem reboot. Assim o dono ve a tela bonita tambem no emulador. */
            s_installing = true; s_instT = 0.0f; s_instProg = 0.0f;
            s_instDone = false; s_instDoneT = 0.0f;
            s_instIndex = s_selected;
            s_instSim = (sysfontInstallBegin(&s_inst, cia) != SYSFONT_OK);
            snprintf(s_instName, sizeof(s_instName), "%s", fontsLabel(s_selected));
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
            float by = FL_ROW0 + v * FL_ROWH;
            if (in->touchPY >= by && in->touchPY < by + FL_ROWH &&
                in->touchPX >= FL_CARD_X && in->touchPX < FL_CARD_X + FL_CARD_W) {
                /* Toque so seleciona/previa -- aplicar sempre passa pelo
                 * popup de confirmacao (A), nunca direto no toque. */
                s_selected = s_scrollTop + v;
                return;
            }
        }
    }

    /* 1.5.0 §FONTES: acao PRIMARIA = A -> aplicar como fonte do SISTEMA direto
     * (o dono achava o fluxo A=previa / X=aplicar nada pratico). O preview da
     * tela de cima ja acompanha a navegacao, entao o A nao precisa mais
     * "prever" -- ele confirma (popup A/B: reboot + backup NAND; item 0 "Padrao
     * do Sistema" = restaurar a original). */
    if (in->confirm) {
        s_sysfontPending = true;
        popupShowConfirm(&s_popup, s_selected == 0 ? T(STR_SYSFONT_RESTORE) : T(STR_SYSFONT_CONFIRM));
    }
}

static const char* previewTitle(int index) {
    return fontsLabel(index); /* o titulo do preview e o proprio nome da fonte */
}

/* 1.4.0 PART 4.3: TELA DE INSTALACAO (espec. NUMERICA v20, SEM GLOW). Topo
 * 400x240. Emblema flat (200,92) r16 com respiro de escala; titulo (200,150);
 * nome (200,174); barra x90..310 y196 h10 (fill animado EZ_DECEL); % (200,218).
 * Ao 100%: barra vira success + check verde (pop-in EZ_SPATIAL); REAL = titulo
 * "reiniciando" -> espera -> fade (EZ_ACCEL) -> reboot. SIMULADO (Azahar) =
 * aviso honesto, sem reboot (fecha no A/B). */
static void drawInstallOverlay(C2D_TextBuf buf) {
    const float mid = 200.0f;
    float t = s_instT;
    bool done = s_instDone;
    float introA = clampf(t / 0.26f, 0.0f, 1.0f);
    /* fade-out final so no install REAL (antes do reboot). */
    float outA = 1.0f;
    if (done && !s_instSim) {
        float fo = (s_instDoneT - 0.9f) / 0.35f;
        if (fo > 0.0f) outA = 1.0f - easeFunc(clampf(fo, 0.0f, 1.0f), EASE_EMPH_ACCEL);
    }
    float A = introA * outA;

    ColorRGBA scrim = g_theme.backgroundTop; scrim.a = (u8)(255 * A);
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, scrim);

    /* emblema flat (200,92) r16, respiro de escala 1.0->1.04->1.0 (~1.2s). */
    float breath = 1.0f + 0.04f * (0.5f + 0.5f * sinf(t * (6.28318531f / 1.2f)));
    UI_Emblem(mid, 92.0f, (16.0f / 24.0f) * breath, uiFrameTime(), A);

    /* check verde sobre o emblema ao concluir (pop-in EZ_SPATIAL 200ms). */
    if (done) {
        float sc = easeFunc(clampf(s_instDoneT / 0.20f, 0.0f, 1.0f), EASE_EXPR_SPATIAL);
        ColorRGBA succ = g_theme.success; succ.a = (u8)(255 * A);
        UI_Check(mid, 92.0f, 34.0f * sc, succ);
    }

    /* titulo (200,150) + nome/aviso (200,174). */
    const char* title = !done ? T(STR_SYSFONT_INSTALLING)
                              : (s_instSim ? T(STR_SYSFONT_EMU_DONE) : T(STR_SYSFONT_DONE));
    ColorRGBA tc = g_theme.textPrimary; tc.a = (u8)(tc.a * A);
    ColorRGBA c2 = g_theme.textSecondary; c2.a = (u8)(c2.a * A);
    UI_TextCenter(buf, NULL, title, mid, 142.0f, 0.30f, 0.30f, tc);
    const char* sub = (done && s_instSim) ? T(STR_SYSFONT_EMU_NOTE) : s_instName;
    UI_TextCenterFit(buf, NULL, sub, mid, 166.0f, 0.24f, 0.18f, 300.0f, c2);

    /* barra de progresso x90..310 (w=220) y196 h10 r5. fill EZ_DECEL. */
    float p = clampf(s_instProg, 0.0f, 1.0f);
    float pe = easeFunc(p, EASE_EMPH_DECEL);
    ColorRGBA track = {255, 255, 255, (u8)(30 * A)};
    UI_NinePill(90.0f, 196.0f, 220.0f, 10.0f, track);
    if (pe > 0.001f) {
        ColorRGBA fill = done ? g_theme.success : g_theme.accent;
        fill.a = (u8)(255 * A);
        UI_NinePill(90.0f, 196.0f, fmaxf(10.0f, 220.0f * pe), 10.0f, fill);
    }
    char pct[8]; snprintf(pct, sizeof(pct), "%d%%", (int)(p * 100.0f + 0.5f));
    ColorRGBA hint = g_theme.textHint; hint.a = (u8)(hint.a * A);
    UI_TextCenter(buf, NULL, pct, mid, 212.0f, 0.22f, 0.22f, hint);
}

void fontsRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_TopBackground();
    UI_ScreenHeader(buf, T(STR_TAB_FONTS));

    /* Card de previa (espec v20): x24 y88 w352 h128 r16. */
    float cardX = 24.0f + slideX;
    if (UI_AssetsReady()) UI_NineCard(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);
    else UI_Card(cardX, 88.0f, 352.0f, 128.0f, 16.0f, g_theme.surface);

    /* caption "PREVIA" (44,114) f9 hint. */
    UI_Text(buf, NULL, T(STR_PREVIEW), 44.0f + slideX, 106.0f, 0.24f, 0.24f, g_theme.textHint);

    /* nome / amostra / pangrama -- na PROPRIA fonte selecionada. */
    C2D_Font f = fontsGetFont(s_selected);
    UI_Text(buf, f, previewTitle(s_selected), 44.0f + slideX, 124.0f, 0.81f, 0.81f, g_theme.textPrimary);
    UI_Text(buf, f, "Aa Bb Cc 123", 44.0f + slideX, 160.0f, 1.13f, 1.13f, g_theme.accent);
    UI_Text(buf, f, "O rato roeu a roupa do rei", 44.0f + slideX, 200.0f, 0.53f, 0.53f, g_theme.textSecondary);

    /* badge "em uso" (pilula verde, canto dir x262 y104 w98 h24) -- so se aplicada. */
    if (s_selected == g_fonts.currentIndex) {
        ColorRGBA green = g_theme.success;
        if (UI_AssetsReady()) UI_NinePill(262.0f + slideX, 104.0f, 98.0f, 24.0f, green);
        else UI_RoundRect(262.0f + slideX, 104.0f, 98.0f, 24.0f, 12.0f, green);
        UI_TextCenter(buf, NULL, T(STR_IN_USE), 262.0f + 49.0f + slideX, 109.0f, 0.27f, 0.27f,
                      (ColorRGBA){0x0E, 0x0D, 0x12, 255});
    }

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.backgroundTop;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, veil);
    }

    if (s_installing) drawInstallOverlay(buf);
}

void fontsRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM) {
    (void)transVal; (void)scaleM;
    UI_BottomBackground();

    fontsClampScroll();
    int n = fontsCount();

    /* lista AGRUPADA: 1 card unico (espec v20). */
    float cardX = FL_CARD_X + slideX;
    if (UI_AssetsReady()) UI_NineCard(cardX, FL_CARD_Y, FL_CARD_W, FL_CARD_H, 15.0f, g_theme.surface);
    else UI_RoundFrame(cardX, FL_CARD_Y, FL_CARD_W, FL_CARD_H, 15.0f, g_theme.surface,
                       (ColorRGBA){255, 255, 255, (u8)(themeIsDark() ? 8 : 24)});

    /* rolagem fluida: s_scrollAnim persegue s_scrollTop suave (glide ~exponencial). */
    s_scrollAnim += ((float)s_scrollTop - s_scrollAnim) * fminf(1.0f, uiFrameDt() * 14.0f);
    if (fabsf((float)s_scrollTop - s_scrollAnim) < 0.002f) s_scrollAnim = (float)s_scrollTop;

    const float listTop = FL_ROW0;
    const float listBot = FL_ROW0 + FL_VISIBLE * FL_ROWH;

    /* desenha 1 linha extra em cima/baixo pra cobrir as parciais que entram/saem
     * durante o deslize; cada linha some por FADE nas bordas (sem scissor, que e
     * arriscado na tela girada do 3DS). */
    int first = (int)floorf(s_scrollAnim) - 1; if (first < 0) first = 0;
    int last  = s_scrollTop + FL_VISIBLE; if (last >= n) last = n - 1;
    bool prevDrawn = false;
    for (int i = first; i <= last; i++) {
        float rowY = FL_ROW0 + ((float)i - s_scrollAnim) * FL_ROWH;
        float rowCy = rowY + FL_ROWH * 0.5f;
        /* fracao da linha DENTRO da janela (fade nas pontas). */
        float vis = fminf(rowY + FL_ROWH, listBot) - fmaxf(rowY, listTop);
        float ra = clampf(vis / FL_ROWH, 0.0f, 1.0f);
        if (ra <= 0.01f) { prevDrawn = false; continue; }
        bool selected = (i == s_selected);
        bool isCurrent = (i == g_fonts.currentIndex);

        /* divisoria 1px entre linhas (nao na 1a linha visivel). */
        if (prevDrawn) {
            ColorRGBA div = {255, 255, 255, (u8)((themeIsDark() ? 26 : 30) * ra)};
            UI_Fill(FL_TEXT_X + slideX, rowY, (FL_CARD_X + FL_CARD_W) - FL_TEXT_X, 1.0f, div);
        }
        prevDrawn = true;

        /* item focado: fundo THEME-AWARE (era #1F1E26 fixo -> escuro no claro)
         * + anel accent liquido (desliza). */
        if (selected) {
            float fx = cardX + 4.0f, fw = FL_CARD_W - 8.0f, fy = rowY + 1.0f, fh = FL_ROWH - 2.0f;
            ColorRGBA selBg = themeCardSelBg(); selBg.a = (u8)((float)selBg.a * ra);
            if (UI_AssetsReady()) UI_NineCard(fx, fy, fw, fh, 12.0f, selBg);
            else UI_RoundRect(fx, fy, fw, fh, 12.0f, selBg);
            /* o anel acompanha a linha deslizando (chamado todo frame com fy
             * animado); so evita desenhar quando a linha ja esta quase saindo. */
            if (ra > 0.5f) UI_FocusRing(fx, fy, fw, fh, 12.0f);
        }

        ColorRGBA textCol = selected ? g_theme.textPrimary : g_theme.textSecondary;
        textCol.a = (u8)((float)textCol.a * ra);
        UI_Text(buf, fontsGetFont(i), fontsLabel(i), FL_TEXT_X + slideX, rowCy - 9.0f, 0.34f, 0.34f, textCol);

        if (isCurrent) {
            /* bolinha verde solida "em uso" (290, rowCy) r6. */
            ColorRGBA su = g_theme.success; su.a = (u8)((float)su.a * ra);
            UI_RoundRect(FL_CARD_X + FL_CARD_W - 16.0f + slideX, rowCy - 6.0f, 12.0f, 12.0f, 6.0f, su);
        }
    }

    /* scrollbar fina (se houver mais fontes que cabem). */
    if (n > FL_VISIBLE) {
        float sbX = FL_CARD_X + FL_CARD_W - 4.0f + slideX, sbTop = FL_ROW0, sbH = FL_VISIBLE * FL_ROWH;
        ColorRGBA track = g_theme.textHint; track.a = 40;
        UI_RoundRect(sbX, sbTop, 3.0f, sbH, 1.5f, track);
        float thumbH = sbH * (float)FL_VISIBLE / (float)n;
        float thumbY = sbTop + sbH * (float)s_scrollTop / (float)n;
        UI_RoundRect(sbX, thumbY, 3.0f, thumbH, 1.5f, g_theme.accent);
    }

    /* rodape (espec v20): pilula accent "X aplicar no sistema" (16,202,150,28)
     * + "A previa" ao lado. */
    ColorRGBA pillC = g_theme.accent;
    if (UI_AssetsReady()) UI_NinePill(16.0f + slideX, 202.0f, 150.0f, 28.0f, pillC);
    else UI_RoundRect(16.0f + slideX, 202.0f, 150.0f, 28.0f, 14.0f, pillC);
    UI_TextCenter(buf, NULL, T(STR_HELP_FONTS_X), 16.0f + 75.0f + slideX, 208.0f, 0.24f, 0.24f, themeContrastText(pillC));
    UI_Text(buf, NULL, T(STR_HELP_FONTS_L), 182.0f + slideX, 210.0f, 0.24f, 0.24f, g_theme.textHint);

    if (fadeA < 0.999f) {
        ColorRGBA veil = g_theme.background;
        veil.a = (u8)(255 * (1.0f - clampf(fadeA, 0.0f, 1.0f)));
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, veil);
    }
    /* durante a instalacao a tela de baixo escurece; aviso "nao desligue" so no
     * install REAL (no simulado o topo ja explica que so aplica no console). */
    if (s_installing) {
        ColorRGBA scrim = g_theme.background; scrim.a = 228;
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, scrim);
        if (!s_instSim) {
            float mid = SCREEN_BOT_WIDTH * 0.5f, cy = SCREEN_BOT_HEIGHT * 0.5f;
            UI_TextCenter(buf, NULL, T(STR_SYSFONT_KEEPON), mid, cy - 16, 0.30f, 0.30f, g_theme.textPrimary);
            UI_TextCenter(buf, NULL, T(STR_SYSFONT_REBOOT_END), mid, cy + 14, 0.22f, 0.22f, g_theme.textSecondary);
        }
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
