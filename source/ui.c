#include "ui.h"
#include "common.h"
#include "icons.h"
#include "lang.h"
#include "ui9_gen.h" /* 1.4.0 §A1: indices da sheet 9-slice (gerado por tex3ds) */
#include <stdio.h>
#include <string.h>

static float s_frame = 0.0f;
static float s_dt = 1.0f / 60.0f;
float g_enterT = 1.0f;
float g_frame = 0.0f;
ScreenTransition g_trans;

/* v1.0.2 §2: fonte global do CHROME = Comfortaa Bold (gordinha, legivel no
 * console -- a fonte de sistema fina era ilegivel em 240px). Tem acentos
 * Latinos, entao PT/EN/ES seguem ok. So o chrome usa ela (font==NULL); o
 * preview da aba Fontes passa a fonte explicita e nao e afetado. Se nao
 * carregar, parse() cai na fonte de sistema (nunca trava). */
static C2D_Font g_uiFont = NULL;
/* UI_TEXT_SCALE vem de ui.h (publico). Aplicado so quando font==NULL (chrome),
 * nunca no preview de fonte. */

/* 1.4.0 §A1: sheet 9-slice (ui9.t3x). Carregada junto da fonte (romfs pronto).
 * Se nao carregar, os helpers caem no desenho vetorial antigo (nunca trava). */
static C2D_SpriteSheet s_ui9 = NULL;

void UI_FontInit(void) {
    /* 1.5.0: fonte do chrome = Coolvetica Regular (mais "gordinha"/display, casa
     * melhor com o app). Gerada com whitelist ASCII + Latin-1 (0x20-0x7E,
     * 0xA0-0xFF) a 40pt -> tem acentos PT/ES e fica nitida ate o large-title.
     * Se nao carregar, cai na Comfortaa Bold (fallback), e essa cai na fonte de
     * sistema (parse()) -- nunca trava. */
    if (!g_uiFont) g_uiFont = C2D_FontLoad("romfs:/fonts/ui_coolvetica.bcfnt");
    if (!g_uiFont) g_uiFont = C2D_FontLoad("romfs:/fonts/ui_comfortaa_bold.bcfnt");
    if (!s_ui9) s_ui9 = C2D_SpriteSheetLoad("romfs:/gfx/ui9.t3x");
}
void UI_FontExit(void) {
    if (s_ui9) { C2D_SpriteSheetFree(s_ui9); s_ui9 = NULL; }
    if (g_uiFont) { C2D_FontFree(g_uiFont); g_uiFont = NULL; }
}

/* 1.8.0 CAELESTIA: accent ANIMADO global (o "CAnim" -- toda mudanca de cor
 * escorre em 300ms). Os elementos accent (foco, pilula do segmented, sliders
 * de vel/prof, sublinha do titulo...) leem UI_AccentAnim() em vez de
 * g_theme.accent -> ao trocar de accent, a tela inteira se transforma suave.
 * A paleta de swatches em si continua com as cores fixas (correto). */
static ColorTween s_accTw = {0};
static bool s_accTwInit = false;

void uiFrameTick(float dt) {
    if (dt <= 0.0f || dt > 0.25f) dt = 1.0f / 60.0f;
    s_dt = dt;
    s_frame += dt;
    if (s_frame > 1024.0f) s_frame = 0.0f;
    g_frame = s_frame;
    if (g_enterT < 1.0f) g_enterT = approach(g_enterT, 1.0f, 2.1f * dt);
    if (g_trans.active) transitionUpdate(&g_trans, dt);
    if (!s_accTwInit) { colorTweenStart(&s_accTw, g_theme.accent, g_theme.accent, DUR_EFFECTS_SLOW, EASE_EFF_SLOW); s_accTw.active = false; s_accTwInit = true; }
    colorTweenTo(&s_accTw, g_theme.accent); /* auto-update; so reinicia se mudou */
}

/* accent animado (300ms) -- alpha 255. */
ColorRGBA UI_AccentAnim(void) {
    if (!s_accTwInit) return g_theme.accent;
    ColorRGBA c = colorTweenValue(&s_accTw); c.a = 255; return c;
}

float uiFrameTime(void) { return s_frame; }
float uiFrameDt(void) { return s_dt; }
void uiScreenEnter(void) { g_enterT = 0.0f; UI_FocusRingReset(); }
float UI_EnterProgress(void) { return easeOutCubic(g_enterT); }

/* Segundos reais desde uiScreenEnter() (g_enterT e um ramp linear 0->1 a
 * 2.1 unidades/s, ver uiFrameTick) -- usado pelo stagger 3.2 (40ms entre
 * elementos, 260ms cada, easeOutCubic), que precisa de tempo bruto, nao
 * do progresso ja "easeado" de UI_EnterProgress(). */
float UI_EnterSeconds(void) { return g_enterT / 2.1f; }

/* Stagger 3.2: elemento `index` comeca `stepSec*index` depois da entrada da
 * tela e leva `durSec` pra aparecer, sempre com easeOutCubic.
 *
 * spec v7 Parte B (Stagger Cascata, transitions.c::TRANS_STAGGER_CASCADE):
 * essa transicao especifica pede um timing/curva diferentes (60ms/300ms/
 * easeOutBack-1.3) do que cada tela ja chama aqui com suas proprias
 * constantes hardcoded. Em vez de editar os ~6 call sites por tela, main.c
 * liga um override global so enquanto essa transicao esta ativa -- todo
 * UI_StaggerT do app passa a usar o timing/curva do override, sem precisar
 * saber que existe. */
static bool s_staggerOverrideOn = false;
static float s_staggerOverrideStep = 0.0f;
static float s_staggerOverrideDur = 0.0f;

void UI_SetStaggerOverride(float stepSec, float durSec) {
    s_staggerOverrideOn = true;
    s_staggerOverrideStep = stepSec;
    s_staggerOverrideDur = durSec;
}

void UI_ClearStaggerOverride(void) {
    s_staggerOverrideOn = false;
}

float UI_StaggerT(int index, float stepSec, float durSec) {
    if (s_staggerOverrideOn) {
        stepSec = s_staggerOverrideStep;
        durSec = s_staggerOverrideDur;
    }
    float p = clampf((UI_EnterSeconds() - (float)index * stepSec) / durSec, 0.0f, 1.0f);
    return s_staggerOverrideOn ? easeOutBackAmp(p, 1.3f) : easeFunc(p, EASE_OUT_CUBIC);
}

float UI_EnterSlide(float maxOffset, EaseType ease) {
    float t = easeFunc(g_enterT, ease);
    return (1.0f - t) * maxOffset;
}

static u32 u32c(ColorRGBA c) { return C2D_Color32(c.r, c.g, c.b, c.a); }

/* ==================== 9-SLICE / SPRITES (1.4.0 §A1) ====================
 * Mata o serrilhado do UI_RoundRect (leque de triangulos SEM antialiasing nos
 * cantos): os assets PNG (ui9.t3x) ja tem o canto anti-aliased, entao em
 * qualquer tamanho o canto fica perfeito. Todos os assets sao brancos/neutros
 * -> desenhados TINTADOS (blend=1.0 substitui o RGB pela cor, o alpha da
 * textura preserva o AA; mesma tecnica dos icones monocromaticos). */

static C2D_Image ui9img(size_t idx) {
    C2D_Image img = {0};
    if (s_ui9) return C2D_SpriteSheetGetImage(s_ui9, idx);
    return img;
}
bool UI_AssetsReady(void) { return s_ui9 != NULL; }

/* Desenha UMA celula (sub-regiao em texcoords l/t/r/b) de um sprite de atlas,
 * esticada pro retangulo destino. width/height=1 no subtex temporario: o
 * tamanho desenhado vem so de scaleX/scaleY (= w/h destino), eliminando o erro
 * de arredondamento u16 da largura/altura nativa. */
static void ui9cell(C2D_Image base, float dx, float dy, float dw, float dh,
                    float l, float t, float r, float b, const C2D_ImageTint* tint) {
    if (dw <= 0.05f || dh <= 0.05f) return;
    Tex3DS_SubTexture sub = { 1, 1, l, t, r, b };
    C2D_Image cell = { base.tex, &sub };
    C2D_DrawImageAt(cell, dx, dy, 0.0f, tint, dw, dh);
}

/* 9-slice generico. srcInset = px do inset no asset-fonte (README_assets.txt);
 * dstCorner = tamanho do canto NO DESTINO (px) -- desacoplado do source pra o
 * mesmo asset virar card (canto fixo) ou pill/stadium (canto = h/2). Cantos
 * desenhados em tamanho fixo, bordas esticam em 1 eixo, centro em 2. */
void UI_NineSliceImg(C2D_Image img, float x, float y, float w, float h,
                     float srcInset, float dstCorner, ColorRGBA tint) {
    if (!img.tex || !img.subtex || w <= 0.0f || h <= 0.0f) return;
    const Tex3DS_SubTexture* st = img.subtex;
    float fu = fminf(srcInset / (float)st->width, 0.5f);
    float fv = fminf(srcInset / (float)st->height, 0.5f);
    float L = st->left, R = st->right, T = st->top, B = st->bottom;
    float uL = L + (R - L) * fu, uR = R - (R - L) * fu; /* U: esq->dir */
    float vT = T - (T - B) * fv, vB = B + (T - B) * fv; /* V desce (top>bottom) */
    float cx = fminf(dstCorner, w * 0.5f);
    float cy = fminf(dstCorner, h * 0.5f);
    float x0 = x, x1 = x + cx, x2 = x + w - cx, x3 = x + w;
    float y0 = y, y1 = y + cy, y2 = y + h - cy, y3 = y + h;
    C2D_ImageTint tn;
    C2D_PlainImageTint(&tn, C2D_Color32(tint.r, tint.g, tint.b, tint.a), 1.0f);
    ui9cell(img, x0, y0, x1 - x0, y1 - y0, L,  T,  uL, vT, &tn); /* TL */
    ui9cell(img, x1, y0, x2 - x1, y1 - y0, uL, T,  uR, vT, &tn); /* T  */
    ui9cell(img, x2, y0, x3 - x2, y1 - y0, uR, T,  R,  vT, &tn); /* TR */
    ui9cell(img, x0, y1, x1 - x0, y2 - y1, L,  vT, uL, vB, &tn); /* L  */
    ui9cell(img, x1, y1, x2 - x1, y2 - y1, uL, vT, uR, vB, &tn); /* C  */
    ui9cell(img, x2, y1, x3 - x2, y2 - y1, uR, vT, R,  vB, &tn); /* R  */
    ui9cell(img, x0, y2, x1 - x0, y3 - y2, L,  vB, uL, B,  &tn); /* BL */
    ui9cell(img, x1, y2, x2 - x1, y3 - y2, uL, vB, uR, B,  &tn); /* B  */
    ui9cell(img, x2, y2, x3 - x2, y3 - y2, uR, vB, R,  B,  &tn); /* BR */
}

/* Helpers por papel (insets do README). Cada um cai no fallback vetorial do
 * chamador quando a sheet nao carregou (UI_AssetsReady()==false). */
void UI_NineCard(float x, float y, float w, float h, float r, ColorRGBA tint) {
    UI_NineSliceImg(ui9img(ui9_gen_card9_3x_idx), x, y, w, h, 28.0f, r, tint);
}
void UI_NinePill(float x, float y, float w, float h, ColorRGBA tint) {
    UI_NineSliceImg(ui9img(ui9_gen_pill9_3x_idx), x, y, w, h, 22.0f, h * 0.5f, tint);
}
void UI_NineFocus(float x, float y, float w, float h, float r, ColorRGBA tint) {
    UI_NineSliceImg(ui9img(ui9_gen_focus9_3x_idx), x, y, w, h, 40.0f, r, tint);
}
void UI_NineShadow(float x, float y, float w, float h, float r, ColorRGBA tint) {
    UI_NineSliceImg(ui9img(ui9_gen_shadow9_3x_idx), x, y, w, h, 46.0f, r, tint);
}
/* 1.4.0 §SEM-GLOW: anel NÍTIDO (ring9 = contorno AA puro, centro transparente,
 * ZERO halo) tintado -- foco/seleção. Preenche (x,y,w,h) com a borda no
 * perimetro; o conteudo aparece pelo centro transparente. */
void UI_Ring(float x, float y, float w, float h, float r, ColorRGBA tint) {
    UI_NineSliceImg(ui9img(ui9_gen_ring9_3x_idx), x, y, w, h, 30.0f, r, tint);
}

/* Glow radial (substitui os circulos translucidos empilhados -- 1 sprite =
 * menos overdraw). (cx,cy)=centro; diameter = lado do quadrado; tint accent. */
void UI_Glow(float cx, float cy, float diameter, ColorRGBA tint) {
    C2D_Image img = ui9img(ui9_gen_glow_radial_3x_idx);
    if (!img.tex || !img.subtex || diameter <= 0.0f) return;
    C2D_ImageTint tn;
    C2D_PlainImageTint(&tn, C2D_Color32(tint.r, tint.g, tint.b, tint.a), 1.0f);
    float scale = diameter / (float)img.subtex->width;
    C2D_DrawImageAt(img, cx - diameter * 0.5f, cy - diameter * 0.5f, 0.0f, &tn, scale, scale);
}

/* Marca de sucesso (fim da instalacao de fonte). (cx,cy)=centro; size = lado. */
void UI_Check(float cx, float cy, float size, ColorRGBA tint) {
    C2D_Image img = ui9img(ui9_gen_check_3x_idx);
    if (!img.tex || !img.subtex || size <= 0.0f) return;
    C2D_ImageTint tn;
    C2D_PlainImageTint(&tn, C2D_Color32(tint.r, tint.g, tint.b, tint.a), 1.0f);
    float scale = size / (float)img.subtex->height;
    C2D_DrawImageAt(img, cx - size * 0.5f, cy - size * 0.5f, 0.0f, &tn, scale, scale);
}

/* 1.4.0 PART3: anel circular cakeOS (ring_circle = contorno, centro
 * transparente, sem glow) centrado e tintado -- swatches/LED/emblema. */
void UI_RingCircle(float cx, float cy, float diameter, ColorRGBA tint) {
    C2D_Image img = ui9img(ui9_gen_ring_circle_3x_idx);
    if (!img.tex || !img.subtex || diameter <= 0.0f) return;
    C2D_ImageTint tn;
    C2D_PlainImageTint(&tn, C2D_Color32(tint.r, tint.g, tint.b, tint.a), 1.0f);
    float scale = diameter / (float)img.subtex->width;
    C2D_DrawImageAt(img, cx - diameter * 0.5f, cy - diameter * 0.5f, 0.0f, &tn, scale, scale);
}

/* 1.4.0 PART3: header macOS large-title de TODA tela de cima (espec numerica
 * v20): mini-emblema centro (28,26) r9 + eyebrow "CustomizerDS" (x46) + o
 * large-title da tela (x24, ~26px, a esquerda). Cores via tema. */
void UI_ScreenHeader(C2D_TextBuf buf, const char* title) {
    /* 1.6.1: o mini-emblema do header tambem ganha vida (deriva 2D). */
    UI_Emblem(28.0f, 26.0f, 9.0f / 24.0f, uiFrameTime(), 1.0f);
    UI_Text(buf, NULL, "CustomizerDS", 46.0f, 16.0f, 0.30f, 0.30f, g_theme.textHint);
    if (title) {
        UI_Text(buf, NULL, title, 24.0f, 40.0f, 0.70f, 0.70f, g_theme.textPrimary);
        /* 1.6.1: DETALHE -- barra accent curta sob o titulo (cresce na entrada da
         * tela), a "aba" que amarra a identidade em toda tela. */
        float grow = UI_EnterProgress();
        ColorRGBA acc = UI_AccentAnim(); acc.a = 255;
        UI_RoundRect(24.0f, 66.0f, 34.0f * grow, 3.0f, 1.5f, acc);
    }
}

/* spec v7 Parte A (causa-raiz): a versao antiga montava o round-rect em 2
 * retangulos + 4 circulos SOBREPOSTOS -- os 2 retangulos (vertical cheio +
 * horizontal cheio) se cruzavam na regiao central, e cada circulo de canto
 * cobria o quadrante interno que ja pertencia aos retangulos adjacentes.
 * Para cor.a < 255 (quase todo overlay/halo/superficie do app), cada pixel
 * naquela area de overlap recebe o blend "over" 2-3x, ficando visivelmente
 * mais opaco/escuro que o resto da forma -- exatamente as "manchas/circulos-
 * fantasma" atras do icone nos cards selecionados e a "costura" nas pontas
 * dos sliders (onde a linha reto-curvo do retangulo encontra o circulo).
 *
 * Fix: UM polígono convexo so (leque de triangulos a partir do centro),
 * desenhado de uma vez, com a MESMA cor em todo vertice -- nenhum pixel e
 * coberto por mais de um triangulo (leque de poligono convexo nunca se
 * autointersecta), entao zero acumulo de alpha e zero costura. As bordas
 * arredondadas sao a propria geometria do poligono (8 segmentos por
 * quadrante = 32 no perimetro total), nao um circulo separado por cima --
 * "antialiasing" aqui e por densidade de segmento (citro2d/PICA200 nao tem
 * MSAA configurado e validado neste projeto pra arriscar sem hardware),
 * suave o bastante pros raios usados nesta resolucao (ate ~30px).
 * Funciona sem caso especial para os degenerados:
 *  - pill/stadium exato (r=h/2): os 4 cantos formam as 2 semicapsulas certas;
 *  - circulo perfeito (w=h=2r): os 4 centros de canto coincidem no meio. */
#define UI_ROUNDRECT_ARC_SEGS 16 /* 1.8.0 CAELESTIA: cantos vetoriais (fallback) mais lisos */

void UI_RoundRect(float x, float y, float w, float h, float r, ColorRGBA color) {
    if (w <= 0.0f || h <= 0.0f) return;
    float ri = fmaxf(0.0f, fminf(r, fminf(w, h) * 0.5f));
    u32 c = u32c(color);
    if (ri < 0.5f) { C2D_DrawRectSolid(x, y, 0.0f, w, h, c); return; }

    /* Ordem TL->TR->BR->BL percorre o perimetro inteiro numa volta so; cada
     * canto cobre seu proprio quadrante de 90 graus, conectando direto no
     * comeco do arco do proximo canto (a borda RETA entre dois cantos sai
     * de graca: e so o triangulo do leque entre os 2 ultimos vertices). */
    struct { float cx, cy, startDeg; } corners[4] = {
        { x + ri,     y + ri,     180.0f },
        { x + w - ri, y + ri,     270.0f },
        { x + w - ri, y + h - ri, 360.0f },
        { x + ri,     y + h - ri, 450.0f },
    };

    float vx[4 * (UI_ROUNDRECT_ARC_SEGS + 1)];
    float vy[4 * (UI_ROUNDRECT_ARC_SEGS + 1)];
    int n = 0;
    for (int k = 0; k < 4; k++) {
        for (int s = 0; s <= UI_ROUNDRECT_ARC_SEGS; s++) {
            float deg = corners[k].startDeg + 90.0f * (float)s / (float)UI_ROUNDRECT_ARC_SEGS;
            float rad = deg * ((float)M_PI / 180.0f);
            vx[n] = corners[k].cx + ri * cosf(rad);
            vy[n] = corners[k].cy + ri * sinf(rad);
            n++;
        }
    }

    float cx = x + w * 0.5f, cy = y + h * 0.5f;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        C2D_DrawTriangle(cx, cy, c, vx[i], vy[i], c, vx[j], vy[j], c, 0.0f);
    }
}

void UI_RoundFrame(float x, float y, float w, float h, float r, ColorRGBA fill, ColorRGBA border) {
    if (w <= 1 || h <= 1) return;
    if (border.a > 0) UI_RoundRect(x, y, w, h, r, border);
    float innerR = r > 1.0f ? r - 1.0f : r;
    UI_RoundRect(x + 1, y + 1, w - 2, h - 2, innerR, fill);
}

void UI_Fill(float x, float y, float w, float h, ColorRGBA color) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x, y, 0.0f, w, h, u32c(color));
}

/* Overlay extra do pool de transicoes (spec v6 secao 2, ampliado na v7 Parte
 * B): desenhado por main.c DEPOIS que a tela entrante terminou de se
 * desenhar (background+conteudo), pra nao precisar tocar nenhuma tela
 * individual. Cada campo vem zerado/neutro em qualquer transicao que nao o
 * usa, entao isto e um no-op barato fora delas:
 *  - scrimAlpha: veu preto (Pop-up Expansivo);
 *  - flashAlpha/flashRadiusFrac: circulo de accent crescendo (Wipe Radial) --
 *    flashRadiusFrac e fracao 0..1 do raio do CANTO mais distante do centro;
 *  - edgeShadowDir: sombra direcional na borda de "trás" do slide (Push
 *    Lateral) -- -1/+1 liga, 0 desliga;
 *  - maskBandAlpha/maskSweepT: faixa diagonal translucida varrendo a tela
 *    (Mask Reveal Diagonal) -- maskBandAlpha<=0 desliga. */
void UI_TransOverlay(float screenW, float screenH, float scrimAlpha, float flashAlpha, float flashRadiusFrac,
                      int edgeShadowDir, float maskBandAlpha, float maskSweepT) {
    if (scrimAlpha > 0.001f) {
        ColorRGBA s = {0, 0, 0, (u8)(255.0f * clampf(scrimAlpha, 0.0f, 1.0f))};
        UI_Fill(0, 0, screenW, screenH, s);
    }
    if (flashAlpha > 0.001f && flashRadiusFrac > 0.0f) {
        float cx = screenW * 0.5f, cy = screenH * 0.5f;
        float maxR = sqrtf(cx * cx + cy * cy);
        ColorRGBA c = g_theme.accent;
        c.a = (u8)(255.0f * clampf(flashAlpha, 0.0f, 1.0f));
        C2D_DrawCircleSolid(cx, cy, 0.0f, flashRadiusFrac * maxR, u32c(c));
    }
    if (edgeShadowDir != 0) {
        /* sombra na borda de "trás" do movimento: navDir>0 (avancando p/
         * frente) a tela entra da direita, trás = borda esquerda; navDir<0
         * o inverso. Largura fixa pequena, gradiente simulado em 4 faixas
         * de alpha decrescente (sem shader de gradiente real disponivel). */
        float bandW = 24.0f;
        float baseA = 90.0f;
        int steps = 4;
        for (int i = 0; i < steps; i++) {
            float frac = (float)i / (float)steps;
            float bx = (edgeShadowDir > 0) ? (frac * bandW) : (screenW - bandW + frac * bandW);
            ColorRGBA sh = {0, 0, 0, (u8)(baseA * (1.0f - frac))};
            UI_Fill(bx, 0, bandW / (float)steps + 1.0f, screenH, sh);
        }
    }
    if (maskBandAlpha > 0.001f) {
        /* faixa diagonal ~30 graus simulada com C2D_DrawTriangle (2
         * triangulos = paralelogramo), centro varrendo de -bandW a
         * screenW+bandW conforme maskSweepT; alpha cai pra 0 nas pontas do
         * sweep pra nao "cortar" abruptamente ao entrar/sair da tela. */
        float bandW = screenW * 0.22f;
        float skew = screenH * 0.55f; /* ~30 graus pra essa altura de tela */
        float travel = screenW + bandW * 2.0f;
        float centerX = -bandW + maskSweepT * travel;
        float edgeFade = fminf(1.0f, fminf(maskSweepT * 6.0f, (1.0f - maskSweepT) * 6.0f));
        ColorRGBA c = g_theme.accent;
        c.a = (u8)(110.0f * clampf(maskBandAlpha, 0.0f, 1.0f) * edgeFade);
        u32 cc = u32c(c);
        float x0 = centerX - bandW * 0.5f, x1 = centerX + bandW * 0.5f;
        C2D_DrawTriangle(x0, 0.0f, cc, x1, 0.0f, cc, x1 + skew, screenH, cc, 0.0f);
        C2D_DrawTriangle(x0, 0.0f, cc, x1 + skew, screenH, cc, x0 + skew, screenH, cc, 0.0f);
    }
}

/* Bolinha estilo "glass" do Reva UI (v3 secao 2C/3.1): borda solida 100% da
 * cor + interior translucido @25%. Como UI_RoundRect sempre preenche o disco
 * cheio (sem modo so-contorno), o anel e simulado em 3 camadas -- circulo de
 * borda opaco, depois um circulo do tamanho exato da cor de fundo conhecida
 * por baixo (pra "furar" um buraco), depois o fill translucido dentro desse
 * buraco. Os 3 sao circulos perfeitos (w==h), entao ficam no caminho seguro
 * de UI_RoundRect (sem o artefato de blobs em alpha parcial). bgBehind tem
 * que ser a cor solida exata do que esta atras (topbar, titlebar etc). */
void UI_GlassDot(float cx, float cy, float r, ColorRGBA color, ColorRGBA bgBehind) {
    ColorRGBA border = color;
    border.a = 255;
    UI_RoundRect(cx - r, cy - r, r * 2, r * 2, r, border);
    float holeR = r - 1.5f;
    if (holeR > 0.3f) {
        UI_RoundRect(cx - holeR, cy - holeR, holeR * 2, holeR * 2, holeR, bgBehind);
        ColorRGBA fill = color;
        fill.a = 64; /* ~25% */
        UI_RoundRect(cx - holeR, cy - holeR, holeR * 2, holeR * 2, holeR, fill);
    }
}

/* r deve bater com o raio do elemento que recebe a sombra -- senao a sombra
 * (2 camadas retangulares deslocadas, simulando blur sem blur real) sobra
 * como um canto quadrado duro por tras de swatches circulares e da janela
 * mock no modo claro, onde o fundo claro deixa a sombra retangular bem mais
 * visivel que no escuro. */
void UI_Shadow(float x, float y, float w, float h, float r, float alpha, float offset) {
    if (w <= 0 || h <= 0) return;
    u8 a = (u8)fminf(alpha, 255);
    if (a == 0) return;
    /* 1.4.0 §A1: sombra = sprite shadow9 (blur suave real), desenhada um pouco
     * maior que o card e deslocada -> halo macio que da profundidade, sem o
     * canto quadrado duro das 2 camadas retangulares antigas. */
    if (UI_AssetsReady()) {
        /* 1.9.0 FIX3: elementos FINOS/pequenos (campo HEX ~32px, chips) espremiam
         * o 9-slice e o halo virava MANCHA suja. Fator k encolhe halo+alpha
         * proporcional ao menor lado -> sombra sempre limpa, nunca borrao. */
        float k = fminf(1.0f, fminf(w, h) / 48.0f);
        float m = 8.0f * k;
        ColorRGBA sh = {0, 0, 0, (u8)(a * k)};
        UI_NineShadow(x - m, y - m + offset, w + 2.0f * m, h + 2.0f * m, r + m, sh);
        return;
    }
    UI_RoundRect(x + offset, y + offset, w, h, r, (ColorRGBA){0, 0, 0, a / 4});
    UI_RoundRect(x + offset * 1.5f, y + offset * 1.5f, w, h, r, (ColorRGBA){0, 0, 0, a / 8});
}

/* 1.9.0 FIX3: elevacao estilo Material -- KEY (direcional, deslocada p/ BAIXO,
 * luz vem de cima) + AMBIENT (difusa, quase concentrica). level 1=repouso(chip)
 * 2=card 3=flutuante(foco/knob) 4=popup. Auto-protecao p/ finos (k) -> nunca
 * mancha. Substitui as sombras avulsas por um sistema consistente e "rico". */
void UI_Elevation(float x, float y, float w, float h, float r, int level, float alphaMul) {
    if (level <= 0 || w <= 0.0f || h <= 0.0f) return;
    if (!UI_AssetsReady()) { UI_Shadow(x, y, w, h, r, 70.0f * alphaMul, 2.0f); return; }
    static const struct { float aExp, aOfs, aA; float kExp, kOfs, kA; } E[4] = {
        { 2.0f, 1.0f, 26.0f,   0.0f, 0.0f,  0.0f }, /* 1: so ambient      */
        { 3.0f, 1.0f, 26.0f,   5.0f, 3.0f, 42.0f }, /* 2: card            */
        { 4.0f, 1.0f, 30.0f,   9.0f, 5.0f, 55.0f }, /* 3: foco/knob       */
        { 6.0f, 2.0f, 34.0f,  14.0f, 8.0f, 70.0f }, /* 4: popup           */
    };
    int i = (level > 4 ? 4 : level) - 1;
    float k = fminf(1.0f, fminf(w, h) / 48.0f);
    ColorRGBA sh = {0, 0, 0, 0};
    sh.a = (u8)(E[i].aA * k * alphaMul);
    if (sh.a > 2) UI_NineShadow(x - E[i].aExp, y - E[i].aExp + E[i].aOfs,
                                w + 2.0f * E[i].aExp, h + 2.0f * E[i].aExp, r + E[i].aExp, sh);
    sh.a = (u8)(E[i].kA * k * alphaMul);
    if (E[i].kExp > 0.0f && sh.a > 2)
        UI_NineShadow(x - E[i].kExp, y - E[i].kExp + E[i].kOfs,
                      w + 2.0f * E[i].kExp, h + 2.0f * E[i].kExp, r + E[i].kExp, sh);
}

/* Gradiente horizontal (esquerda->direita) -- usado no preenchimento dos
 * sliders do LED (spec 3.5: "preenchimento em gradiente na cor do canal"). */
void UI_GradientH(float x, float y, float w, float h, ColorRGBA left, ColorRGBA right) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x, y, 0.0f, w, h, u32c(left));
    for (float i = 0; i < w; i += 2.0f) {
        float t = i / w;
        ColorRGBA c;
        c.r = (u8)((float)left.r * (1.0f - t) + (float)right.r * t);
        c.g = (u8)((float)left.g * (1.0f - t) + (float)right.g * t);
        c.b = (u8)((float)left.b * (1.0f - t) + (float)right.b * t);
        c.a = (u8)((float)left.a * (1.0f - t) + (float)right.a * t);
        C2D_DrawRectSolid(x + i, y, 0.0f, 2.0f, h, u32c(c));
    }
}

static C2D_Text parse(C2D_Text* out, C2D_TextBuf buf, C2D_Font font, const char* text) {
    if (!text || !text[0]) return *out;
    C2D_Font use = font ? font : g_uiFont; /* chrome usa Comfortaa Bold (g_uiFont) */
    if (use) C2D_TextFontParse(out, use, buf, text);
    else C2D_TextParse(out, buf, text);   /* fallback: fonte de sistema */
    C2D_TextOptimize(out);
    return *out;
}

/* §2: o chrome (font==NULL) ganha +25% de tamanho pra legibilidade no console;
 * o preview de fonte (font explicita) fica no tamanho original. */
static inline float uiChromeScale(C2D_Font font) { return font ? 1.0f : UI_TEXT_SCALE; }

/* §3: ANEL DE FOCO accent animado (pulsa), desenhado ATRAS do elemento focado
 * -- chamar ANTES de desenhar o elemento. Distinto da selecao/aplicado: e o
 * halo accent ao redor do que o D-pad esta apontando agora. */
/* 1.6.1: seleção = CHIP PREENCHIDO (fill accent suave + borda accent nítida),
 * no lugar do aro fino oco que o dono achou feio. O conteudo do elemento e
 * desenhado por cima e continua legivel. Le como um estado "selecionado"
 * premium (estilo iOS/cakeOS), nao um contorno vazio. */
static void focusRingDraw(float x, float y, float w, float h, float r) {
    ColorRGBA acc = UI_AccentAnim(); acc.a = 255;
    ColorRGBA soft = acc; soft.a = themeIsDark() ? 46 : 40; /* ~18% */
    UI_RoundRect(x, y, w, h, r, soft);         /* preenchimento suave */
    if (UI_AssetsReady()) UI_Ring(x, y, w, h, r, acc); /* borda AA nitida por cima */
    else UI_RoundFrame(x, y, w, h, r, soft, acc);
}

/* 1.5.0 §FOCO (revisao 2): o "liquido" anterior ficou LENTO/atrasado (0.26s +
 * overshoot forte da EXPR_FAST -> arrastava e balancava). Agora e RAPIDO (0.16s)
 * com stretch SUTIL: a borda lider usa EXPR_SPATIAL (molinha leve) e a de tras
 * EMPH_DECEL, entao a caixa alonga de leve no trajeto e assenta rapido --
 * responsivo, sem wobble e SEM o "respiro" parado (que lia como bug). Cada
 * aresta interpolada por eixo; salto grande ou transicao de tela = snap. */
/* 1.9.0 FIX1: o foco e um estado global que PERSISTE entre telas. Ao entrar numa
 * aba ele viajava da posicao da tela anterior ate o item novo (~0.5s de viagem
 * feia). uiScreenEnter()/troca-de-aba chamam UI_FocusRingReset() -> a proxima
 * chamada SNAPA no item (nasce parado). O morph bonito fica so no D-pad dentro
 * da tela. */
static bool s_focusForceSnap = false;
void UI_FocusRingReset(void) { s_focusForceSnap = true; }

void UI_FocusRing(float x, float y, float w, float h, float r) {
    static float fL, fT, fR, fB, fr_;   /* from (edges + raio) */
    static float tL, tT, tR, tB, tr_;   /* target */
    static float cL, cT, cR, cB, cr_;   /* atual desenhado (edges) */
    static Tween tw_ = {0};
    static bool init = false;

    float nL = x, nT = y, nR = x + w, nB = y + h; /* alvo novo em arestas */
    if (!init) {
        fL = tL = cL = nL; fT = tT = cT = nT; fR = tR = cR = nR; fB = tB = cB = nB;
        fr_ = tr_ = cr_ = r; init = true; tw_.active = false;
    }

    bool targetMoved = (fabsf(nL - tL) + fabsf(nT - tT) + fabsf(nR - tR) + fabsf(nB - tB)) > 0.5f;
    /* FIX1: reset pedido mas alvo coincide (nao "moveu") -> snapa e limpa aqui. */
    if (s_focusForceSnap && !targetMoved) {
        fL = cL = tL; fT = cT = tT; fR = cR = tR; fB = cB = tB; fr_ = cr_ = tr_;
        tw_.active = false; s_focusForceSnap = false;
    }
    if (targetMoved) {
        float jump = fabsf(nL - cL) + fabsf(nT - cT);
        fL = cL; fT = cT; fR = cR; fB = cB; fr_ = cr_;
        tL = nL; tT = nT; tR = nR; tB = nB; tr_ = r;
        if (s_focusForceSnap || jump > 300.0f || transitionActive(&g_trans)) {
            fL = nL; fT = nT; fR = nR; fB = nB; fr_ = r; /* snap (nasce parado) */
            tw_.active = false;
            s_focusForceSnap = false;
        } else {
            tweenStart(&tw_, 0.0f, 1.0f, DUR_SPATIAL_FAST, EASE_LINEAR); /* 1.8.0 CAELESTIA: 0.35s */
        }
    }
    tweenUpdate(&tw_, uiFrameDt());

    if (tw_.active) {
        float p = tweenValue(&tw_);
        float lead  = easeFunc(p, EASE_EXPR_FAST);  /* 1.8.0 CAELESTIA: lider FastSpatial (molinha) */
        float trail = easeFunc(p, EASE_EMPH_DECEL); /* aresta de tras */
        if (tL >= fL) { cL = lerpf(fL, tL, trail); cR = lerpf(fR, tR, lead); }
        else          { cL = lerpf(fL, tL, lead);  cR = lerpf(fR, tR, trail); }
        if (tT >= fT) { cT = lerpf(fT, tT, trail); cB = lerpf(fB, tB, lead); }
        else          { cT = lerpf(fT, tT, lead);  cB = lerpf(fB, tB, trail); }
        cr_ = lerpf(fr_, tr_, trail);
    } else {
        cL = tL; cT = tT; cR = tR; cB = tB; cr_ = tr_;
    }

    const float p3 = 3.0f;
    focusRingDraw(cL - p3, cT - p3, (cR - cL) + 2.0f * p3, (cB - cT) + 2.0f * p3, cr_ + p3);
}

/* ===== 1.8.0 motor CAELESTIA ===== */

/* pop-in caelestia: painel 0.80->1 (AppList/TrayMenu), item 0.90->1. Escala com
 * EXPR_SPATIAL (a molinha do overshoot), alpha com EFF_DEFAULT (sem overshoot). */
void UI_PopIn(float p, int type, float* outScale, float* outAlpha) {
    float pc = clampf(p, 0.0f, 1.0f);
    float from = (type == 0) ? 0.80f : 0.90f;
    float e = easeFunc(pc, EASE_EXPR_SPATIAL);
    if (outScale) *outScale = from + (1.0f - from) * e;
    if (outAlpha) *outAlpha = easeFunc(pc, EASE_EFF_DEFAULT);
}

/* PressFx (StateLayer): overlay branco 10% que surge no press e faz fade em
 * 300ms (EASE_EFF_SLOW) ao soltar. 1 card fino tintado -> barato. */
void pressFxTrigger(PressFx* p, float x, float y, float w, float h, float r) {
    p->t = 0.0f; p->held = true; p->x = x; p->y = y; p->w = w; p->h = h; p->r = r;
}
void pressFxUpdate(PressFx* p, float dt, bool stillHeld) {
    if (!stillHeld) p->held = false;
    p->t += dt;
}
void pressFxDraw(const PressFx* p) {
    if (p->w <= 0.0f) return;
    if (p->t > DUR_EFFECTS_SLOW && !p->held) return;
    float a = p->held ? 0.10f
                      : 0.10f * (1.0f - easeFunc(clampf(p->t / DUR_EFFECTS_SLOW, 0.0f, 1.0f), EASE_EFF_SLOW));
    if (a <= 0.004f) return;
    ColorRGBA white = {254, 254, 254, (u8)(a * 255.0f)}; /* 0xFE evita bug de blend do branco puro */
    if (UI_AssetsReady()) UI_NineCard(p->x, p->y, p->w, p->h, p->r, white);
    else UI_RoundRect(p->x, p->y, p->w, p->h, p->r, white);
}

/* Largura REAL do texto como sera desenhado (fonte/escala do chrome) -- pra
 * dimensionar caixas (badges) sem o texto transbordar. */
float UI_TextWidth(C2D_TextBuf buf, C2D_Font font, const char* text, float sx) {
    if (!text || !text[0]) return 0.0f;
    float k = uiChromeScale(font);
    C2D_Text t;
    parse(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sx * k, sx * k, &tw, NULL);
    return tw;
}

void UI_Text(C2D_TextBuf buf, C2D_Font font, const char* text,
             float x, float y, float sx, float sy, ColorRGBA color) {
    if (!text || !text[0]) return;
    float k = uiChromeScale(font); sx *= k; sy *= k;
    C2D_Text t;
    parse(&t, buf, font, text);
    C2D_DrawText(&t, C2D_WithColor, x, y, 0.0f, sx, sy, u32c(color));
}

void UI_TextRight(C2D_TextBuf buf, C2D_Font font, const char* text,
                  float right, float y, float sx, float sy, ColorRGBA color) {
    if (!text || !text[0]) return;
    float k = uiChromeScale(font); sx *= k; sy *= k;
    C2D_Text t;
    parse(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sx, sy, &tw, NULL);
    C2D_DrawText(&t, C2D_WithColor, right - tw, y, 0.0f, sx, sy, u32c(color));
}

void UI_TextCenter(C2D_TextBuf buf, C2D_Font font, const char* text,
                   float centerX, float y, float sx, float sy, ColorRGBA color) {
    if (!text || !text[0]) return;
    float k = uiChromeScale(font); sx *= k; sy *= k;
    C2D_Text t;
    parse(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sx, sy, &tw, NULL);
    C2D_DrawText(&t, C2D_WithColor, centerX - tw * 0.5f, y, 0.0f, sx, sy, u32c(color));
}

/* Centralizado com AUTO-SHRINK (v9 §6): mede o texto em sxMax e, se passar de
 * maxW, reduz a escala proporcionalmente ate caber (nunca abaixo de sxMin).
 * Usado pro nome COMPLETO da fonte na home ("Patterns & Dots", "Comic Sans
 * MS3"). Divisao protegida (tw>0) -- cobre o item de div-by-zero da §13. */
void UI_TextCenterFit(C2D_TextBuf buf, C2D_Font font, const char* text,
                      float centerX, float y, float sxMax, float sxMin,
                      float maxW, ColorRGBA color) {
    if (!text || !text[0]) return;
    float k = uiChromeScale(font); sxMax *= k; sxMin *= k; /* §2: chrome +25% */
    C2D_Text t;
    parse(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sxMax, sxMax, &tw, NULL);
    float sx = sxMax;
    if (tw > maxW && tw > 0.0f) {
        sx = sxMax * (maxW / tw);
        if (sx < sxMin) sx = sxMin;
    }
    float twFit = 0.0f;
    C2D_TextGetDimensions(&t, sx, sx, &twFit, NULL);
    C2D_DrawText(&t, C2D_WithColor, centerX - twFit * 0.5f, y, 0.0f, sx, sx, u32c(color));
}

/* ==================== TOP SCREEN (macOS) ==================== */

/* Raio que cobre o canto mais distante da origem, p/ o wipe radial (abaixo)
 * terminar garantidamente fora da tela em t=1 (senao sobraria uma borda da
 * cor antiga visivel num canto). */
static float wipeMaxRadius(float originX, float originY, float w, float h) {
    float dx = fmaxf(originX, w - originX);
    float dy = fmaxf(originY, h - originY);
    return sqrtf(dx * dx + dy * dy);
}

void UI_TopBackground(void) {
    ThemeWipe* wp = themeWipeGet();
    if (wp->active) {
        /* Transicao yin-yang (spec v5 6): a cor ANTIGA cobre a tela toda e
         * um disco da cor NOVA cresce a partir do icone que disparou a
         * troca, "engolindo" a cor antiga -- easeOutCubic, sem overshoot
         * (cobrir >100% nao tem leitura visual, mesma logica do prototipo
         * web calibrado em spec v5 9.4). */
        UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, wp->oldBgTop);
        float t = clampf(wp->t / THEME_WIPE_DUR, 0.0f, 1.0f);
        float radius = easeFunc(t, EASE_OUT_CUBIC) *
            wipeMaxRadius(wp->originTopX, wp->originTopY, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT);
        C2D_DrawCircleSolid(wp->originTopX, wp->originTopY, 0.0f, radius, themeColor(g_theme.backgroundTop));
    } else {
        UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, g_theme.backgroundTop);
    }
    ColorRGBA glow = {255, 255, 255, themeIsDark() ? 3 : 20};
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, 1, glow);
    ColorRGBA shade = {0, 0, 0, themeIsDark() ? 18 : 6};
    UI_Fill(0, SCREEN_TOP_HEIGHT - 1, SCREEN_TOP_WIDTH, 1, shade);
}

void UI_TopMenuBar(const char* title, C2D_TextBuf buf) {
    ColorRGBA barBg = themeIsDark()
        ? (ColorRGBA){10, 12, 18, 235}
        : (ColorRGBA){222, 225, 234, 240};
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, 24, barBg);
    ColorRGBA div = {255, 255, 255, themeIsDark() ? 6 : 16};
    UI_Fill(0, 24, SCREEN_TOP_WIDTH, 1, div);

    /* macOS traffic light dots, estilo "glass" Reva (v3 3.1): borda solida +
     * interior translucido, nao mais um disco chapado. */
    UI_GlassDot(12, 12, 4, (ColorRGBA){255, 95, 87, 255}, barBg);
    UI_GlassDot(24, 12, 4, (ColorRGBA){255, 189, 47, 255}, barBg);
    UI_GlassDot(36, 12, 4, (ColorRGBA){40, 200, 65, 255}, barBg);

    UI_TextCenter(buf, NULL, "CustomizerDS", 90, 5, 0.24f, 0.24f, g_theme.textSecondary);
    if (title)
        UI_TextCenter(buf, NULL, title, SCREEN_TOP_WIDTH / 2, 5, 0.26f, 0.26f, g_theme.textPrimary);
}

void UI_Card(float x, float y, float w, float h, float r, ColorRGBA bg) {
    /* Sombra mais proxima do token da spec (y=8 blur=24 #000@40%): UI_Shadow
     * nao tem blur real (2 retangulos deslocados, sem gaussian), entao o
     * "blur" e simulado por essas 2 camadas; alpha~102=40% de 255, offset 4px
     * (8px literal ficaria desproporcional num card de 196px numa tela de 240px). */
    UI_Shadow(x, y, w, h, r, 102, 4.0f);
    /* 1.4.0 §A1: superficie do card = sprite card9 tintado (canto AA perfeito),
     * substituindo o UI_RoundFrame (cantos serrilhados). */
    if (UI_AssetsReady()) {
        UI_NineCard(x, y, w, h, r, bg);
    } else {
        UI_RoundFrame(x, y, w, h, r, bg, (ColorRGBA){255, 255, 255, themeIsDark() ? 12 : 24});
    }
    ColorRGBA hl = {255, 255, 255, themeIsDark() ? 4 : 14};
    UI_Fill(x + r, y + 1, w - r * 2.0f, 1, hl);
}

void UI_FrostCard(float x, float y, float w, float h, float r) {
    ColorRGBA bg = themeIsDark()
        ? (ColorRGBA){14, 17, 23, 220}
        : (ColorRGBA){248, 249, 252, 220};
    UI_Card(x, y, w, h, r, bg);
}

/* ==================== POPUP MODAL (Travel Agency style) ==================== */

#define POPUP_W 260.0f
#define POPUP_H 120.0f
#define POPUP_BADGE_R 22.0f

static void popupGeometry(float* px, float* py) {
    *px = (SCREEN_BOT_WIDTH - POPUP_W) * 0.5f;
    *py = (SCREEN_BOT_HEIGHT - POPUP_H) * 0.5f - 10;
}

void popupShow(PopupModal* p, const char* msg, const char* confirm, const char* cancel) {
    p->active = true;
    p->confirmMode = false;
    p->closing = false;
    p->animT = 0.0f;
    p->closeT = 0.0f;
    p->bgAlpha = 0.0f;
    p->result = 0;
    strncpy(p->message, msg, sizeof(p->message) - 1);
    strncpy(p->confirmLabel, confirm ? confirm : "OK", sizeof(p->confirmLabel) - 1);
    strncpy(p->cancelLabel, cancel ? cancel : "", sizeof(p->cancelLabel) - 1);
}

void popupShowConfirm(PopupModal* p, const char* msg) {
    p->active = true;
    p->confirmMode = true;
    p->closing = false;
    p->animT = 0.0f;
    p->closeT = 0.0f;
    p->bgAlpha = 0.0f;
    p->result = 0;
    strncpy(p->message, msg, sizeof(p->message) - 1);
    p->confirmLabel[0] = 'A'; p->confirmLabel[1] = '\0';
    p->cancelLabel[0] = 'B'; p->cancelLabel[1] = '\0';
}

void popupHide(PopupModal* p) { p->active = false; p->closing = false; }
bool popupActive(PopupModal* p) { return p->active; }

void popupUpdate(PopupModal* p) {
    if (!p->active) return;
    if (p->closing) {
        /* Caminho inverso, easeInCubic 240ms (spec v4 4.2/4.4). */
        p->closeT = fminf(p->closeT + uiFrameDt() / 0.24f, 1.0f);
        if (p->closeT >= 1.0f) { p->active = false; p->closing = false; }
        return;
    }
    /* Entrada: scrim 0->55% easeOutCubic 220ms, card escala/opacidade
     * 0.6->1.0 com easeOutBack 360ms (popupRender aplica as curvas). */
    p->animT = fminf(p->animT + uiFrameDt() / 0.36f, 1.0f);
    p->bgAlpha = fminf(p->animT * (0.36f / 0.22f), 1.0f);
}

bool popupConfirmInput(PopupModal* p, const AppInput* in) {
    if (!p->active || !p->confirmMode || p->closing) return false;
    int decision = 0;
    if (in->confirm) decision = 1;
    else if (in->back) decision = -1;
    else if (in->touchDown) {
        float px, py;
        popupGeometry(&px, &py);
        float by = py + POPUP_H - 34;
        float bAx = SCREEN_BOT_WIDTH * 0.5f - 45.0f, bBx = SCREEN_BOT_WIDTH * 0.5f + 45.0f;
        float dxA = in->touchPX - bAx, dyA = in->touchPY - by;
        float dxB = in->touchPX - bBx, dyB = in->touchPY - by;
        if (dxA * dxA + dyA * dyA <= POPUP_BADGE_R * POPUP_BADGE_R) decision = 1;
        else if (dxB * dxB + dyB * dyB <= POPUP_BADGE_R * POPUP_BADGE_R) decision = -1;
    }
    if (decision == 0) return false;
    p->result = decision;
    p->closing = true;
    p->closeT = 0.0f;
    return true;
}

void popupRender(C2D_TextBuf buf, PopupModal* p) {
    if (!p->active) return;
    /* 1.8.0 CAELESTIA: pop-in de painel 0.80->1.00 (TrayMenu scale 0.8->1) com
     * EXPR_SPATIAL (molinha); alpha sem overshoot (EFF_DEFAULT). Fecha a 0.94 +
     * fade com emphasizedAccel. */
    float openScale, openAlpha;
    UI_PopIn(p->animT, 0, &openScale, &openAlpha);
    float closeE = easeFunc(p->closeT, EASE_EMPH_ACCEL);
    float scale = p->closing ? lerpf(1.0f, 0.94f, closeE) : openScale;
    float alpha = p->closing ? (1.0f - closeE) : openAlpha;
    float scrimT = p->closing ? alpha : fminf(p->bgAlpha, 1.0f);

    ColorRGBA bgDim = {0, 0, 0, (u8)(140 * scrimT)};
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, bgDim);

    float pw = POPUP_W * scale, ph = POPUP_H * scale;
    float px0, py0;
    popupGeometry(&px0, &py0);
    float cx0 = px0 + POPUP_W * 0.5f, cy0 = py0 + POPUP_H * 0.5f;
    float px = cx0 - pw * 0.5f, py = cy0 - ph * 0.5f;

    ColorRGBA cardBg = themeIsDark()
        ? (ColorRGBA){20, 24, 34, (u8)(245 * alpha)}
        : (ColorRGBA){248, 249, 252, (u8)(250 * alpha)};
    ColorRGBA cardBorder = {255, 255, 255, (u8)(themeIsDark() ? 20 * alpha : 30 * alpha)};

    UI_Elevation(px, py, pw, ph, 16 * scale, 4, alpha); /* 1.9.0 FIX3: popup flutua (level 4) */
    if (UI_AssetsReady()) UI_NineCard(px, py, pw, ph, 16 * scale, cardBg);
    else UI_RoundFrame(px, py, pw, ph, 16 * scale, cardBg, cardBorder);

    float textX = SCREEN_BOT_WIDTH * 0.5f;
    float textY = py + 20 * scale;
    ColorRGBA textC = g_theme.textPrimary;
    textC.a = (u8)(textC.a * alpha);
    UI_TextCenter(buf, NULL, p->message, textX, textY, 0.26f, 0.26f, textC);

    if (p->confirmMode) {
        /* Selos A/B estilo UniversalAddControl (anel + fill translucido +
         * glyph), com o rotulo Sim/Nao ao lado -- nao a letra solta. */
        float by = py0 + POPUP_H - 34;
        float bAx = SCREEN_BOT_WIDTH * 0.5f - 45.0f, bBx = SCREEN_BOT_WIDTH * 0.5f + 45.0f;
        UI_ABBadge(bAx, by, POPUP_BADGE_R * scale, true, alpha);
        UI_ABBadge(bBx, by, POPUP_BADGE_R * scale, false, alpha);
        ColorRGBA lblC = g_theme.textSecondary; lblC.a = (u8)(lblC.a * alpha);
        UI_TextCenter(buf, NULL, T(STR_YES), bAx, by + POPUP_BADGE_R * scale + 4, 0.22f, 0.22f, lblC);
        UI_TextCenter(buf, NULL, T(STR_NO), bBx, by + POPUP_BADGE_R * scale + 4, 0.22f, 0.22f, lblC);
        return;
    }

    if (p->confirmLabel[0]) {
        ColorRGBA btnBg = g_theme.accent;
        btnBg.a = (u8)(btnBg.a * alpha);
        float btnW = (p->cancelLabel[0]) ? 90.0f : 140.0f;
        float bx = (SCREEN_BOT_WIDTH - btnW) * 0.5f;
        float by = py + ph - 44 * scale;
        if (UI_AssetsReady()) UI_NinePill(bx, by, btnW, 32, btnBg);
        else UI_RoundRect(bx, by, btnW, 32, 16, btnBg);
        ColorRGBA btnText = themeContrastText(g_theme.accent);
        btnText.a = (u8)(btnText.a * alpha);
        UI_TextCenter(buf, NULL, p->confirmLabel, SCREEN_BOT_WIDTH * 0.5f, by + 7, 0.26f, 0.26f, btnText);
    }
}

/* Sprite real (spec v5 3.2): circulo Reva com contorno preto bold + fill
 * verde (A) ou vermelho (B) -- as mesmas cores do semaforo macOS do app
 * (UI_TopMenuBar) -- e letra ja desenhada no asset, gerado via nano-banana +
 * pos-processo de contorno. Substitui o UI_RoundRect+texto procedural
 * anterior (que nao tinha o contorno bold caracteristico do Reva). */
void UI_ABBadge(float cx, float cy, float r, bool isA, float alpha) {
    iconsDrawFixed(isA ? ICON_BADGE_A : ICON_BADGE_B, cx, cy, r * 2.0f, alpha);
}

/* ==================== BOTTOM SCREEN (Touch Bar) ==================== */

void UI_BottomBackground(void) {
    /* §2 (v10): fundo UNIFORME. Antes desenhava uma faixa "Touch Bar" quase-preta
     * (50px no topo) + um painel de conteudo de tom diferente embaixo -- a emenda
     * desses dois tons era o "bicolor" reclamado. Agora a tela de baixo inteira
     * usa g_theme.background como base e os controles (pilulas/segmented/cards)
     * ficam por cima com fundo proprio arredondado. O wipe radial de tema
     * continua. */
    ThemeWipe* wp = themeWipeGet();
    if (wp->active) {
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, wp->oldBgBot);
        float t = clampf(wp->t / THEME_WIPE_DUR, 0.0f, 1.0f);
        float radius = easeFunc(t, EASE_OUT_CUBIC) *
            wipeMaxRadius(wp->originBotX, wp->originBotY, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT);
        C2D_DrawCircleSolid(wp->originBotX, wp->originBotY, 0.0f, radius, themeColor(g_theme.background));
    } else {
        UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, g_theme.background);
    }
}

void UI_TouchBarBackground(void) {
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, g_theme.background);
}

void UI_TouchBarPill(C2D_TextBuf buf, float x, float y, float w, float h,
                     const char* label, const char* icon, bool selected, float appearT, float pulse) {
    float ap = (appearT < 1.0f) ? easeOutCubic(appearT) : 1.0f;
    if (ap <= 0.0f) return;
    float slide = (1.0f - ap) * 10.0f;
    float ox = x + slide;

    float r = h * 0.5f;
    ColorRGBA bg, border, textCol;

    if (selected) {
        bg = g_theme.accent;
        bg.r = (u8)clampi((int)bg.r + (int)(pulse * 15), 0, 255);
        bg.g = (u8)clampi((int)bg.g + (int)(pulse * 15), 0, 255);
        bg.b = (u8)clampi((int)bg.b + (int)(pulse * 15), 0, 255);
        border = (ColorRGBA){255, 255, 255, (u8)(30 + (int)(pulse * 20))};
        textCol = themeContrastText(g_theme.accent);
    } else {
        /* Cinza claramente mais claro que a faixa quase preta (UI_BottomBackground),
         * senao o botao nao-selecionado fica invisivel contra o fundo. */
        bg = themeIsDark()
            ? (ColorRGBA){58, 58, 60, 255}
            : (ColorRGBA){226, 226, 230, 255};
        border = (ColorRGBA){0, 0, 0, themeIsDark() ? 0 : 12};
        textCol = g_theme.textSecondary;
    }

    UI_Shadow(ox, y, w, h, r, 20, 1.0f);
    if (UI_AssetsReady()) {
        UI_NinePill(ox, y, w, h, bg);
    } else {
        UI_RoundRect(ox, y, w, h, r, bg);
        if (!selected && border.a > 0) UI_RoundFrame(ox, y, w, h, r, bg, border);
    }

    /* 1.4.0 §SEM-GLOW: removido o glow accent atras do segmento selecionado --
     * a pilula solida ja marca a selecao, sem halo. */

    float cx = ox + w * 0.5f;
    float cy = y + 6;
    if (icon) UI_Text(buf, NULL, icon, ox + 10, cy, 0.30f, 0.30f, textCol);
    UI_TextCenter(buf, NULL, label, cx, cy, 0.26f, 0.26f, textCol);
}

void UI_TouchBarRow(C2D_TextBuf buf, const char** labels, const char** icons,
                    int count, int selected, float baseY, float baseAppear) {
    float gap = 8.0f;
    float totalW = SCREEN_BOT_WIDTH - 20;
    float btnW = (totalW - gap * (count - 1)) / count;
    btnW = fmaxf(70.0f, fminf(btnW, 130.0f));
    float startX = (SCREEN_BOT_WIDTH - (btnW * count + gap * (count - 1))) * 0.5f;

    for (int i = 0; i < count; i++) {
        float bx = startX + i * (btnW + gap);
        float ap = clampf(baseAppear - i * 0.10f, 0.0f, 1.0f);
        float pulse = (i == selected) ? 0.5f + 0.5f * sinf(s_frame * 4.0f) : 0.0f;
        UI_TouchBarPill(buf, bx, baseY, btnW, 34,
                        labels[i], icons ? icons[i] : NULL,
                        i == selected, ap, pulse);
    }
}

/* Segmented control v3 3.3: lozenge elevada que desliza entre as opcoes com
 * easeOutBack (nao 2+ retangulos colados). morphTween e do CHAMADOR (static
 * Tween por tela) -- antes essa funcao tinha estado interno (`static float
 * s_morphX`) compartilhado entre TODAS as chamadas, o que faria o Tema e o
 * LED baterem de frente um no estado do outro ao usar o mesmo componente. */
void UI_TouchBarSegmented(C2D_TextBuf buf, float x, float y, float w, float h,
                          const char** labels, int count, int selected, Tween* morphTween,
                          bool focused) {
    /* Trilho cinza claramente visivel contra a faixa quase preta da Touch Bar. */
    ColorRGBA baseBg = themeIsDark()
        ? (ColorRGBA){44, 44, 46, 255}
        : (ColorRGBA){226, 226, 230, 255};
    ColorRGBA baseBorder = {0, 0, 0, themeIsDark() ? 0 : 14};
    float r = h * 0.5f;
    /* 1.6.1: FOCO integrado no controle (o aro fino externo era feio). Focado =
     * trilho TINGIDO de accent (sem nenhum contorno solto) -- indica foco de
     * dentro, sem cobrir a pilula selecionada nem repetir o aro que o dono odeia. */
    if (focused) baseBg = themeMix(baseBg, UI_AccentAnim(), themeIsDark() ? 0.22f : 0.18f);

    UI_Shadow(x, y, w, h, r, 15, 1.0f);
    if (UI_AssetsReady()) {
        UI_NinePill(x, y, w, h, baseBg);
    } else {
        UI_RoundRect(x, y, w, h, r, baseBg);
        if (baseBorder.a > 0) UI_RoundFrame(x, y, w, h, r, baseBg, baseBorder);
    }

    float segW = w / count;
    float targetX = x + selected * segW;
    if (morphTween->duration <= 0.0001f) {
        /* primeira chamada (nunca inicializado) -- comeca ja no lugar certo. */
        tweenStart(morphTween, targetX, targetX, 0.001f, EASE_LINEAR);
        tweenUpdate(morphTween, 1.0f);
    } else if (fabsf(morphTween->to - targetX) > 0.5f) {
        /* 1.8.0 CAELESTIA: a pilula desliza em FastSpatial (0.35s, molinha). */
        tweenStart(morphTween, tweenValue(morphTween), targetX, DUR_SPATIAL_FAST, EASE_EXPR_FAST);
    }
    tweenUpdate(morphTween, uiFrameDt());
    float curX = tweenValue(morphTween);

    /* Pilula do selecionado solida (nao mais um tingimento de baixa opacidade
     * que quase desaparecia contra o trilho) -- feedback de selecao inequivoco. */
    ColorRGBA selBg = UI_AccentAnim(); /* 1.8.0 CAELESTIA: cor escorre ao trocar accent */
    UI_Shadow(curX + 2, y + 2, segW - 4, h - 4, r - 2, 20, 1.0f);
    if (UI_AssetsReady()) UI_NinePill(curX + 2, y + 2, segW - 4, h - 4, selBg);
    else UI_RoundRect(curX + 2, y + 2, segW - 4, h - 4, r - 2, selBg);

    for (int i = 0; i < count; i++) {
        float cx = x + segW * i + segW * 0.5f;
        ColorRGBA tc = (i == selected) ? themeContrastText(selBg) : g_theme.textSecondary;
        UI_TextCenter(buf, NULL, labels[i], cx, y + (h - 14) * 0.5f, 0.26f, 0.26f, tc);
    }
}

/* Touch Bar colorida do macOS (v3 3.4) -- a UNICA parte do app que deve ser
 * visualmente fiel a barra original: espectro HSV continuo (vermelho ->
 * laranja -> amarelo -> verde -> ciano -> azul -> magenta -> vermelho) numa
 * capsula de fundo escuro translucido, com um thumb branco circular.
 * As faixas coloridas tem canto reto (citro2d nao recorta por mascara), mas
 * ficam afastadas dos cantos arredondados da capsula (insetX = r*0.7) pra
 * nunca aparecerem por fora da silhueta redonda do trilho. */
void UI_HueSpectrumBar(float x, float y, float w, float h, float hueT, bool dragging) {
    float r = h * 0.5f;
    ColorRGBA trough = {18, 16, 24, 215};
    UI_Shadow(x, y, w, h, r, 15, 1.0f);
    if (UI_AssetsReady()) UI_NinePill(x, y, w, h, trough);
    else UI_RoundRect(x, y, w, h, r, trough);

    float insetX = r * 0.7f;
    float ix = x + insetX, iw = w - insetX * 2;
    float iy = y + 3.0f, ih = h - 6.0f;
    for (float i = 0; i < iw; i += 2.0f) {
        float hue = (i / iw) * 360.0f;
        u8 cr, cg, cb;
        hsvToRgbF(hue, 1.0f, 1.0f, &cr, &cg, &cb);
        float stripW = fminf(2.0f, iw - i);
        UI_Fill(ix + i, iy, stripW, ih, (ColorRGBA){cr, cg, cb, 255});
    }

    float cursorX = ix + clampf(hueT, 0.0f, 1.0f) * iw;
    float cursorR = 7.0f * (dragging ? 1.15f : 1.0f);
    UI_Shadow(cursorX - cursorR, y + h * 0.5f - cursorR, cursorR * 2, cursorR * 2, cursorR, 35, 1.0f);
    UI_RoundRect(cursorX - cursorR, y + h * 0.5f - cursorR, cursorR * 2, cursorR * 2, cursorR,
                (ColorRGBA){255, 255, 255, 255});
}

void UI_TouchBarSlider(C2D_TextBuf buf, float x, float y, float w,
                       const char* label, int value, int min, int max,
                       bool selected, ColorRGBA swatch) {
    UI_TouchBarSliderDrag(buf, x, y, w, label, value, min, max, selected, swatch, 1.0f, (float)value);
}

/* Slider 3.5: capsula de ~10px (branco@10%), preenchimento em gradiente na
 * cor do canal, thumb circular branco que cresce 1.0->1.18 (easeOutBack) ao
 * arrastar com glow na cor do canal no trilho, valor exibido com contagem
 * animada. thumbScale e displayValue ja vem tweened do chamador (led.c
 * mantem o Tween -- mesmo padrao de UI_PillButtonPress/popup do hex). */
void UI_TouchBarSliderDrag(C2D_TextBuf buf, float x, float y, float w,
                           const char* label, int value, int min, int max,
                           bool selected, ColorRGBA swatch, float thumbScale, float displayValue) {
    float rh = 28.0f;
    ColorRGBA bg = selected
        ? themeMix((ColorRGBA){30, 30, 32, 250}, g_theme.accent, 0.10f)
        : (ColorRGBA){30, 30, 32, 250};
    if (!themeIsDark()) bg = selected
        ? themeMix((ColorRGBA){242, 242, 242, 250}, g_theme.accent, 0.08f)
        : (ColorRGBA){242, 242, 242, 250};
    ColorRGBA border = selected ? g_theme.accent
        : (ColorRGBA){255, 255, 255, themeIsDark() ? 12 : 16};
    if (selected) border.a = themeIsDark() ? 65 : 85;

    UI_Shadow(x, y, w, rh, rh * 0.5f, 10, 1.0f);
    UI_RoundFrame(x, y, w, rh, rh * 0.5f, bg, border);

    if (label) UI_Text(buf, NULL, label, x + 10, y + 6, 0.26f, 0.26f, g_theme.textPrimary);

    float barX = x + (label ? 36 : 10);
    float barY = y + 9;
    float barH = 10.0f; /* capsula ~10px, spec 3.5 */
    float barW = w - (label ? 74 : 44);
    float t = (max > min) ? (float)(value - min) / (float)(max - min) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);

    /* Trilho branco@10% (capsula real, raio = altura/2). 1.4.0 §SEM-GLOW:
     * removido o glow da cor do canal por baixo do trilho -- trilho solido. */
    ColorRGBA track = {255, 255, 255, 26};
    UI_RoundRect(barX, barY, barW, barH, barH * 0.5f, track);

    if (t > 0.0f) {
        /* Preenchimento em gradiente na cor do canal (mais escuro -> mais
         * vivo da esquerda pra direita, em vez de um tom solido plano). */
        ColorRGBA fillDark = themeMix(swatch, (ColorRGBA){0, 0, 0, 255}, 0.35f);
        fillDark.a = 230;
        ColorRGBA fillLight = swatch;
        fillLight.a = 230;
        float fillW = barW * t;
        /* Tampa arredondada na ponta esquerda (spec v6 3): UI_GradientH
         * desenha um rect de cantos retos, que colidia com o trilho em
         * capsula por baixo (raio barH/2) e sobrava um "cantinho" reto
         * saindo do silhueta arredondada. Circulo do raio exato do trilho,
         * na cor inicial do gradiente, cobre exatamente esse canto -- a
         * ponta direita do preenchimento continua reta de proposito (e a
         * borda "viva" de uma barra de progresso, no e bug). */
        UI_RoundRect(barX, barY, barH, barH, barH * 0.5f, fillDark);
        UI_GradientH(barX, barY, fillW, barH, fillDark, fillLight);
    }

    /* Thumb circular branco, escala 1.0->1.18 easeOutBack ao arrastar
     * (thumbScale ja vem com a curva aplicada pelo chamador). */
    float knobX = barX + barW * t;
    float knobR = 7.0f * thumbScale;
    float knobCy = barY + barH * 0.5f;
    /* §8: knob = sprite cakeOS de borda GROSSA (ICON_SWATCH_THICK) tintada com
     * a cor do canal -- substitui o circulo branco + glow empilhados. Halo
     * accent sutil so ao arrastar pra reforcar o "pegou". */
    UI_Shadow(knobX - knobR, knobCy - knobR, knobR * 2, knobR * 2, knobR, 35, 1.0f);
    /* 1.4.0 §SEM-GLOW: removido o halo accent ao arrastar -- thumb solido nitido. */
    ColorRGBA knobTint = swatch; knobTint.a = 255;
    iconsDraw(ICON_SWATCH_THICK, knobX, knobCy, knobR * 2.0f, knobTint, 1.0f);

    /* Valor com contagem animada (displayValue ja vem tweened de led.c). */
    char valStr[8];
    snprintf(valStr, sizeof(valStr), "%d", (int)(displayValue + 0.5f));
    UI_TextRight(buf, NULL, valStr, x + w - 8, y + 6, 0.24f, 0.24f, g_theme.textSecondary);
}

void UI_HelpBar(C2D_TextBuf buf, const char* left, const char* right) {
    ColorRGBA barBg = themeIsDark()
        ? (ColorRGBA){0, 0, 0, 60}
        : (ColorRGBA){175, 180, 194, 70};
    float hy = SCREEN_BOT_HEIGHT - 26;
    UI_Fill(0, hy, SCREEN_BOT_WIDTH, 26, barBg);
    ColorRGBA div = {255, 255, 255, themeIsDark() ? 4 : 6};
    UI_Fill(0, hy, SCREEN_BOT_WIDTH, 1, div);
    /* §2: com o chrome +25% e em Bold, esquerda+direita podiam colidir no meio.
     * Medimos as duas (no tamanho real) e, se nao couberem, encolhemos as duas
     * juntas ate caber -- legivel e sem invadir uma a outra. */
    float base = 0.22f;
    float lw = left ? UI_TextWidth(buf, NULL, left, base) : 0.0f;
    float rw = right ? UI_TextWidth(buf, NULL, right, base) : 0.0f;
    float avail = (float)SCREEN_BOT_WIDTH - 18.0f - 10.0f; /* margens 9+9 + folga */
    if (lw + rw > avail && (lw + rw) > 0.0f) base *= avail / (lw + rw);
    if (left) UI_Text(buf, NULL, left, 9, hy + 5, base, base, g_theme.textSecondary);
    if (right) UI_TextRight(buf, NULL, right, SCREEN_BOT_WIDTH - 9, hy + 5, base, base, g_theme.textSecondary);
}

void UI_Badge(C2D_TextBuf buf, float x, float y, const char* text, ColorRGBA bg) {
    if (!text || !text[0]) return;
    float tw = 0.0f;
    C2D_Text tmp;
    parse(&tmp, buf, NULL, text); /* mede com a fonte/escala REAL do chrome */
    /* §2: mede no tamanho DESENHADO (0.22 * UI_TEXT_SCALE) -- UI_TextCenter
     * abaixo aplica o mesmo fator, senao o texto saia maior que a pilula. */
    C2D_TextGetDimensions(&tmp, 0.22f * UI_TEXT_SCALE, 0.22f * UI_TEXT_SCALE, &tw, NULL);
    float minW = (float)strlen(text) * 7.5f * UI_TEXT_SCALE;
    float pw = fmaxf(tw, minW) + 18.0f;
    ColorRGBA textCol = ((int)bg.r + (int)bg.g + (int)bg.b > 430)
        ? (ColorRGBA){10, 12, 16, 255}
        : g_theme.onPrimary;
    if (UI_AssetsReady()) UI_NinePill(x, y, pw, 18, bg);
    else UI_RoundRect(x, y, pw, 18, 9, bg);
    UI_TextCenter(buf, NULL, text, x + pw * 0.5f, y + 2, 0.22f, 0.22f, textCol);
}

void UI_PillButton(C2D_TextBuf buf, float x, float y, float w, float h,
                   const char* label, const char* icon, int iconImg, bool selected, float appearT) {
    UI_PillButtonPress(buf, x, y, w, h, label, icon, iconImg, selected, appearT, 1.0f, 1.0f);
}

/* Microinteracoes 3.4: pressScale e fornecido pelo chamador (que mantem o
 * Tween de press/release -- 1.0->0.96 90ms easeOutCubic no toque, 0.96->1.0
 * easeOutBack ~220ms ao soltar; o c1=1.70158 padrao de EASE_OUT_BACK ja bate
 * com o overshoot "1.7" da spec).
 * focusScale (spec v7 Parte C, exemplo CSS do dono: .square:hover{scale(1.4)}
 * transition ease-in-out 150ms): tambem fornecido pelo chamador (Tween
 * proprio, 1.0<->~1.08 easeInOutCubic ~160ms ao ganhar/perder foco) --
 * SUBSTITUI o pulso ambiente continuo 1.0<->1.03 que existia aqui antes
 * (eram dois efeitos de escala competindo; o pedido especifico do dono e
 * uma TRANSICAO de estado, nao um loop). O halo de cor (0.25<->0.4 alpha)
 * continua ambiente -- isso e so feedback "vivo", nao concorre com a escala. */
void UI_PillButtonPress(C2D_TextBuf buf, float x, float y, float w, float h,
                        const char* label, const char* icon, int iconImg,
                        bool selected, float appearT, float pressScale, float focusScale) {
    float ap = (appearT < 1.0f) ? easeOutCubic(appearT) : 1.0f;
    if (ap <= 0.0f) return;
    float slide = (1.0f - ap) * 8.0f;

    float totalScale = pressScale * focusScale;

    float pw = w * totalScale, ph = h * totalScale;
    float ox = x + slide + (w - pw) * 0.5f;
    float oy = y + slide + (h - ph) * 0.5f;

    float r = ph * 0.5f;
    ColorRGBA bg, textCol;

    if (selected) {
        bg = g_theme.accent;
        textCol = themeContrastText(g_theme.accent);
    } else {
        bg = themeIsDark()
            ? (ColorRGBA){58, 58, 60, 255}
            : (ColorRGBA){226, 226, 230, 255};
        textCol = g_theme.textSecondary;
    }

    /* Press 3.4: escurece ao apertar (pressScale<1), proporcional ao quao
     * apertado esta (1.0=solto, 0.96=fundo do press). */
    float pressAmt = clampf((1.0f - pressScale) / 0.04f, 0.0f, 1.0f);
    if (pressAmt > 0.0f) bg = themeMix(bg, (ColorRGBA){0, 0, 0, 255}, pressAmt * 0.25f);

    bg.a = (u8)((float)bg.a * ap);
    textCol.a = (u8)((float)textCol.a * ap);

    UI_Shadow(ox, oy, pw, ph, r, 15, 1.0f);
    /* 1.4.0 §SEM-GLOW: removido o halo accent pulsante atras da pilula
     * selecionada -- a pilula solida accent ja marca a selecao, sem brilho. */
    if (UI_AssetsReady()) {
        UI_NinePill(ox, oy, pw, ph, bg);
    } else {
        UI_RoundRect(ox, oy, pw, ph, r, bg);
        if (!selected) {
            ColorRGBA border = (ColorRGBA){0, 0, 0, themeIsDark() ? 15 : 10};
            border.a = (u8)((float)border.a * ap);
            UI_RoundFrame(ox, oy, pw, ph, r, bg, border);
        }
    }

    bool hasIcon = (iconImg >= 0) || (icon != NULL);
    float cx = ox + pw * 0.5f;
    if (iconImg >= 0) {
        iconsDrawAuto((IconID)iconImg, cx, oy + 9 * totalScale, 14.0f * totalScale, textCol, ap);
    } else if (icon) {
        UI_TextCenter(buf, NULL, icon, cx, oy + 3 * totalScale, 0.28f * totalScale, 0.28f * totalScale, textCol);
    }
    UI_TextCenter(buf, NULL, label, cx, oy + (hasIcon ? 17 : 6) * totalScale, 0.26f * totalScale, 0.26f * totalScale, textCol);
}

/* Card de navegacao estilo end4-Monet (v3 3.1): raio 24, sombra macOS,
 * tint Monet (bg_card_sel + accent_soft) quando selecionado, icone Reva a
 * esquerda, label+valor a direita. Selecionado ganha borda accent + halo
 * CONTIDO -- a borda e desenhada como circulo opaco cheio e o
 * bg/tint e desenhado por cima inset 2px (mesma tecnica de UI_GlassDot),
 * entao nunca extrapola a silhueta do card (era o "vaza" visto nos prints). */
/* Variante com alpha (card inteiro, p/ o carrossel da Inicio esmaecer os
 * peeks) e contentReveal (0..1 -- stagger interno icone->titulo->valor
 * enquanto o card de centro assenta, spec v5 5: "stagger sutil entre o
 * conteudo interno do card"). contentReveal=1 e UI_NavCard puro (sem custo
 * extra fora do carrossel). */
void UI_NavCardFx(C2D_TextBuf buf, float x, float y, float w, float h,
               IconID icon, const char* label, const char* value,
               ColorRGBA dot, bool selected, float alpha, float contentReveal) {
    /* Fator de escala derivado de h (56px = referencia, o chipH original):
     * o carrossel da Inicio (spec v5 5) desenha os peeks com h menor, e tudo
     * dentro do card (icone, texto, raio) precisa encolher junto -- senao um
     * card menor com icone/texto do MESMO tamanho fica desproporcional. */
    float s = h / 56.0f;
    float radius = RADIUS_CARD * s;

    ColorRGBA bg = selected ? themeCardSelBg() : g_theme.surface;
    bg.a = (u8)((float)bg.a * alpha);
    UI_Shadow(x, y, w, h, radius, (selected ? 60 : 35) * alpha, 2.0f * s);

    if (selected) {
        ColorRGBA border = g_theme.accent; border.a = (u8)(255 * alpha);
        UI_RoundRect(x, y, w, h, radius, border);
        float inset = 2.0f * s;
        float ix = x + inset, iy = y + inset, iw = w - inset * 2.0f, ih = h - inset * 2.0f;
        float ir = radius - inset;
        UI_RoundRect(ix, iy, iw, ih, ir, bg);
        ColorRGBA soft = themeAccentSoft(); soft.a = (u8)((float)soft.a * alpha);
        UI_RoundRect(ix, iy, iw, ih, ir, soft);
    } else {
        UI_RoundRect(x, y, w, h, radius, bg);
    }

    /* Stagger interno: icone primeiro, titulo 60ms depois, valor 120ms
     * depois -- cada um com seu proprio easeOutCubic e leve slide-up. */
    float iconE = clampf(contentReveal / 0.6f, 0.0f, 1.0f);
    float labelE = clampf((contentReveal - 0.15f) / 0.6f, 0.0f, 1.0f);
    float valueE = clampf((contentReveal - 0.30f) / 0.6f, 0.0f, 1.0f);
    iconE = easeOutCubic(iconE); labelE = easeOutCubic(labelE); valueE = easeOutCubic(valueE);

    /* Zona do icone fixa e pequena (nao proporcional a h) -- h aqui e a
     * altura do card inteiro (~56px), nao a do icone; usar h*0.5 pro icone
     * comeria espaco demais do texto (label/valor) num card estreito.
     * 14px (era 17): spec v4 4.1 pediu ~15-20% menor, "grandes demais e
     * desarmonicos" -- 14/56 = 25% da altura do card, dentro do teto de 40%.
     * *s: nos peeks do carrossel (h menor) o icone encolhe na mesma
     * proporcao, em vez de ficar grande demais pro card reduzido. */
    float iconCx = x + 16.0f * s;
    float iconCy = y + h * 0.5f + (1.0f - iconE) * 6.0f * s;
    ColorRGBA iconTint = selected ? g_theme.accent : dot;
    iconsDrawAuto(icon, iconCx, iconCy, 14.0f * s, iconTint, alpha * iconE);

    float textX = x + 30.0f * s;
    ColorRGBA labelC = g_theme.textHint; labelC.a = (u8)((float)labelC.a * alpha * labelE);
    ColorRGBA valueC = selected ? g_theme.textPrimary : g_theme.textSecondary;
    valueC.a = (u8)((float)valueC.a * alpha * valueE);
    if (label) UI_Text(buf, NULL, label, textX, y + h * 0.24f + (1.0f - labelE) * 6.0f * s, 0.20f * s, 0.20f * s, labelC);
    if (value) UI_Text(buf, NULL, value, textX, y + h * 0.54f + (1.0f - valueE) * 6.0f * s, 0.24f * s, 0.24f * s, valueC);
}

void UI_StatChip(C2D_TextBuf buf, float x, float y, float w, float h,
                 const char* label, const char* value, ColorRGBA dot) {
    ColorRGBA bg = themeIsDark()
        ? (ColorRGBA){40, 40, 42, 255}
        : (ColorRGBA){240, 240, 240, 255};
    ColorRGBA border = {255, 255, 255, themeIsDark() ? 10 : 0};
    if (border.a > 0) UI_RoundFrame(x, y, w, h, 10, bg, border);
    else UI_RoundRect(x, y, w, h, 10, bg);

    /* Bolinha glass Reva (v3 3.5) -- bgBehind e o fundo solido do chip
     * desenhado acima, nao a cor de border (que pode ter alpha 0). */
    UI_GlassDot(x + 13.5f, y + 13.5f, 4.5f, dot, bg);
    if (label) UI_Text(buf, NULL, label, x + 22, y + 7, 0.20f, 0.20f, g_theme.textHint);
    if (value) UI_Text(buf, NULL, value, x + 10, y + h - 22, 0.26f, 0.26f, g_theme.textPrimary);
}

void UI_MiniWindow(float x, float y, float w, float h, bool dark) {
    ColorRGBA winBg = dark ? (ColorRGBA){30, 30, 32, 255} : (ColorRGBA){255, 255, 255, 255};
    ColorRGBA winBorder = {255, 255, 255, dark ? 14 : 0};
    ColorRGBA titleBg = dark ? (ColorRGBA){44, 44, 46, 255} : (ColorRGBA){234, 234, 234, 255};

    UI_Shadow(x, y, w, h, 10, 35, 1.5f);
    if (winBorder.a > 0) UI_RoundFrame(x, y, w, h, 10, winBg, winBorder);
    else UI_RoundRect(x, y, w, h, 10, winBg);

    float titleH = fminf(16.0f, h * 0.3f);
    UI_Fill(x + 1, y + 1, w - 2, titleH, titleBg);

    /* Semaforos estilo "glass" Reva (v3 3.1), igual ao UI_TopMenuBar. */
    float r = 2.6f;
    float cy = y + titleH * 0.5f;
    float cx0 = x + 7 + r;
    UI_GlassDot(cx0, cy, r, (ColorRGBA){255, 95, 87, 255}, titleBg);
    UI_GlassDot(cx0 + (r * 2 + 3), cy, r, (ColorRGBA){255, 189, 47, 255}, titleBg);
    UI_GlassDot(cx0 + (r * 2 + 3) * 2, cy, r, (ColorRGBA){40, 200, 65, 255}, titleBg);
}

/* Coreografia exata da spec v2 (docs/animation-spec.md 3.1):
 * fundo preto->bg_base 250ms easeOutCubic; logo escala 0.85->1.0 + fade,
 * easeOutBack, 420ms; titulo+tagline 100ms depois do logo, sobe 12px+fade,
 * easeOutCubic, 360ms. Os 3 cards (stagger 600/680/760ms) ficam em
 * menuRenderStartupPills, na tela de baixo. */
/* Duracao total da boot (deve casar com STARTUP_DURATION em main.c). */
#define BOOT_DUR 3.4f

/* Bolinha "glass" da boot: aro accent-ish + miolo translucido da cor (estilo
 * cakeOS), desenhada por cima do bg_base. */
static void bootGlassBall(float cx, float cy, float r, ColorRGBA col, float a) {
    if (a <= 0.003f || r <= 0.5f) return;
    /* 1.5.0: borda PROPORCIONAL e grossa (~30% do raio) -- casa a bolinha grande
     * do hero com a aparencia das bolinhas pequenas do canto (logo), que o dono
     * curtiu. Miolo translucido fininho. */
    float bw = fmaxf(2.0f, r * 0.30f);
    ColorRGBA border = col; border.a = (u8)(235.0f * a);
    C2D_DrawCircleSolid(cx, cy, 0.0f, r, u32c(border));
    ColorRGBA hole = g_theme.backgroundTop; hole.a = (u8)(255.0f * a);
    C2D_DrawCircleSolid(cx, cy, 0.0f, r - bw, u32c(hole));
    ColorRGBA fill = col; fill.a = (u8)(110.0f * a);
    C2D_DrawCircleSolid(cx, cy, 0.0f, r - bw, u32c(fill));
}

/* §1/§2: emblema compartilhado (3 bolinhas glass) usado na home (com float
 * idle + glow respirando) e no handoff boot->home (so reposicionado/escalado).
 * Reusa bootGlassBall; as posicoes de repouso em scale 1.0 batem EXATAMENTE
 * com os alvos das bolinhas da boot (UI_StartupLogo) -> emenda sem pulo.
 * Perf (apendice A): 1 so circulo de glow de raio modesto, sem rotacao. */
#define EMBLEM_TWO_PI 6.28318531f
void UI_Emblem(float cx, float cy, float scale, float idleT, float alpha) {
    if (alpha <= 0.003f || scale <= 0.01f) return;
    const float R = 24.0f * scale;

    /* 1.4.0 §SEM-GLOW: emblema FLAT (3 aneis cakeOS) -- removido o glow rosa
     * "respirando" atras. A marca sao os 3 aneis de vidro, sem halo. */

    /* 1.6.0: cada bolinha faz uma DERIVA 2D (Lissajous: X e Y com periodos
     * diferentes -> orbita pequena, "constelacao flutuante") em vez de so subir
     * e descer. Amplitudes pequenas (1.2-2.5px, escaladas) pra ficar vivo mas
     * discreto; fases/periodos distintos pra nunca sincronizar. */
    struct { float ox, oy, ampX, perX, phX, ampY, perY, phY; ColorRGBA col; } b[3] = {
        {  0.0f, -15.0f, 2.8f, 4.3f, 0.6f, 3.8f, 3.4f, 0.0f, {255,  95, 135, 255} }, /* rosa  */
        {-19.0f, +11.0f, 3.4f, 3.7f, 2.1f, 3.0f, 2.7f, 1.7f, { 90, 200, 230, 255} }, /* ciano */
        {+19.0f, +11.0f, 2.6f, 5.1f, 4.0f, 3.4f, 3.0f, 3.1f, { 95, 215, 130, 255} }, /* verde */
    };
    /* respiro global de escala do conjunto (~+-3%), bem lento. */
    float breath = 1.0f + 0.03f * sinf(idleT * (EMBLEM_TWO_PI / 4.6f));
    float Rb = R * breath;
    for (int i = 0; i < 3; i++) {
        float fx = b[i].ampX * scale * sinf(idleT * (EMBLEM_TWO_PI / b[i].perX) + b[i].phX);
        float fy = b[i].ampY * scale * sinf(idleT * (EMBLEM_TWO_PI / b[i].perY) + b[i].phY);
        bootGlassBall(cx + b[i].ox * scale + fx, cy + b[i].oy * scale + fy, Rb, b[i].col, alpha);
    }
}

/* §1 (1.0.2): boot animada "se construindo" -- fundo acende + wipe radial, 3
 * bolinhas glass voam de 3 pontos com spring e se encaixam formando o emblema
 * (glow no encaixe), wordmark se forma (Comfortaa Bold, fade+slide), slogan.
 * Os ultimos 0.35s tudo esmaece pro bg pra emendar suave na Home (a Home entra
 * com seu proprio stagger). Pulavel: main pula direto pra Home. 60fps (poucos
 * draws, sem particulas). */
void UI_StartupLogo(C2D_TextBuf buf, float t) {
    float lx = SCREEN_TOP_WIDTH * 0.5f; /* 200 */
    float cy = 86.0f;                   /* centro do emblema */

    /* §2: SEM esmaecimento de saida -- a boot agora entrega o emblema/wordmark
     * em opacidade cheia pro handoff shared-element (menuRenderTopHandoff), que
     * desliza o emblema pra pose da Home enquanto o resto da Home faz fade-in.
     * (Antes esmaecia tudo nos ultimos 0.35s pra emendar no stagger de aba, que
     * foi removido -- ver main.c.) */
    float gA = 1.0f;

    /* 1) fundo "acende" do preto pro bg em 0.4s. */
    float lit = easeFunc(clampf(t / 0.40f, 0.0f, 1.0f), EASE_OUT_CUBIC);
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, (ColorRGBA){0, 0, 0, (u8)(255 * (1.0f - lit))});

    /* wipe radial accent saindo do centro (0-0.7s), some sozinho. */
    float wT = clampf(t / 0.70f, 0.0f, 1.0f);
    if (wT < 1.0f) {
        float wr = easeFunc(wT, EASE_OUT_CUBIC) * 300.0f;
        ColorRGBA wc = g_theme.accent; wc.a = (u8)(55.0f * (1.0f - wT));
        C2D_DrawCircleSolid(lx, cy, 0.0f, wr, u32c(wc));
    }

    /* 1.4.0 §SEM-GLOW: removido o pulso de glow rosa no "encaixe" das bolinhas
     * (o boot fica limpo; o wipe de revelacao acima nao e halo, fica). */

    /* 2) 3 bolinhas glass voam de 3 pontos e se encaixam (spring, 1 oscilacao). */
    const float R = 24.0f;
    struct { float tx, ty, sx, sy, delay; ColorRGBA col; } balls[3] = {
        { lx,         cy - 15.0f, lx,                       -60.0f,                        0.25f, {255, 95, 135, 255} }, /* rosa, de cima */
        { lx - 19.0f, cy + 11.0f, -70.0f,                   SCREEN_TOP_HEIGHT + 60.0f,     0.33f, { 90, 200, 230, 255} }, /* ciano, baixo-esq */
        { lx + 19.0f, cy + 11.0f, SCREEN_TOP_WIDTH + 70.0f, SCREEN_TOP_HEIGHT + 60.0f,     0.41f, { 95, 215, 130, 255} }, /* verde, baixo-dir */
    };
    for (int i = 0; i < 3; i++) {
        float bt = clampf((t - balls[i].delay) / 0.78f, 0.0f, 1.0f);
        float e = easeSpringAmp(bt, 0.62f); /* spring suave: 1 oscilacao discreta */
        float bx = lerpf(balls[i].sx, balls[i].tx, e);
        float by = lerpf(balls[i].sy, balls[i].ty, e);
        bootGlassBall(bx, by, R, balls[i].col, gA);
    }

    /* 3) wordmark "CustomizerDS" se forma (fade + slide-da-esquerda), Comfortaa Bold. */
    float wmT = clampf((t - 1.10f) / 0.60f, 0.0f, 1.0f);
    if (wmT > 0.0f) {
        float e = easeFunc(wmT, EASE_OUT_QUINT);
        ColorRGBA wc = g_theme.textPrimary; wc.a = (u8)(255 * e * gA);
        /* bases compensam o UI_TEXT_SCALE do chrome (~*1.5) -> efetivo ~0.51. */
        UI_TextCenter(buf, NULL, "CustomizerDS", lx - (1.0f - e) * 16.0f, cy + 52.0f, 0.34f, 0.34f, wc);
    }
    /* slogan, 0.6s depois, fade. */
    float slT = clampf((t - 1.70f) / 0.45f, 0.0f, 1.0f);
    if (slT > 0.0f) {
        ColorRGBA sc = g_theme.accent; sc.a = (u8)(255 * easeFunc(slT, EASE_OUT_CUBIC) * gA);
        UI_TextCenter(buf, NULL, T(STR_STARTUP_SLOGAN), lx, cy + 84.0f, 0.19f, 0.19f, sc);
    }
}
