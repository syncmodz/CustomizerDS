#ifndef UI_H
#define UI_H

#include "theme.h"
#include "anim.h"
#include "icons.h"
#include "input.h"
#include <citro2d.h>
#include <stdbool.h>

extern float g_enterT;
extern float g_frame;
extern ScreenTransition g_trans;

/* §2 (1.0.2): carrega/descarrega a fonte global do chrome (Comfortaa Bold).
 * Chamar UI_FontInit apos romfs/C2D prontos e UI_FontExit no cleanup. */
void UI_FontInit(void);
void UI_FontExit(void);

/* §2: multiplicador de tamanho aplicado ao texto do CHROME (font==NULL) pra
 * legibilidade no console. Publico pra quem MEDE texto pra dimensionar uma
 * caixa (badges) medir no mesmo tamanho que sera desenhado. */
#define UI_TEXT_SCALE 1.5f
/* Largura do texto como sera desenhado (fonte+escala do chrome). */
float UI_TextWidth(C2D_TextBuf buf, C2D_Font font, const char* text, float sx);
/* §3: anel de foco accent (chamar antes de desenhar o elemento focado). */
void UI_FocusRing(float x, float y, float w, float h, float r);

void uiFrameTick(float dt);
float uiFrameTime(void);
float uiFrameDt(void);
void uiScreenEnter(void);
float UI_EnterProgress(void);
float UI_EnterSlide(float maxOffset, EaseType ease);
float UI_EnterSeconds(void);
/* Stagger 3.2: 40ms entre elementos, 260ms cada, easeOutCubic (docs/animation-spec.md 3.2).
 * v7 Parte B: enquanto o override (abaixo) estiver ligado, ignora
 * stepSec/durSec recebidos e usa os do override + easeOutBackAmp(1.3) --
 * usado so pela transicao Stagger Cascata (main.c liga/desliga). */
float UI_StaggerT(int index, float stepSec, float durSec);
void UI_SetStaggerOverride(float stepSec, float durSec);
void UI_ClearStaggerOverride(void);

void UI_Fill(float x, float y, float w, float h, ColorRGBA color);
/* Overlay extra do pool de transicoes (spec v6/v7 -- ver transitions.c e
 * UI_TransOverlay em ui.c): scrim preto, flash circular de accent, sombra de
 * borda direcional (Push Lateral) e faixa diagonal (Mask Reveal Diagonal),
 * desenhados por main.c depois da tela entrante. No-op por campo quando o
 * valor correspondente vem zerado/neutro (qualquer transicao que nao usa
 * aquele canal). */
void UI_TransOverlay(float screenW, float screenH, float scrimAlpha, float flashAlpha, float flashRadiusFrac,
                      int edgeShadowDir, float maskBandAlpha, float maskSweepT);
/* Bolinha "glass" Reva UI: borda solida + interior @25%. bgBehind precisa
 * ser a cor solida exata por baixo do ponto onde a bolinha e desenhada. */
void UI_GlassDot(float cx, float cy, float r, ColorRGBA color, ColorRGBA bgBehind);
/* 1.4.0 §A1: 9-slice por sprite (canto anti-aliased em qualquer tamanho, mata o
 * serrilhado do UI_RoundRect). UI_AssetsReady()==false (sheet ui9.t3x ausente)
 * -> chamador deve cair no desenho vetorial. srcInset = px do inset no asset;
 * dstCorner = tamanho do canto no destino (desacoplado: card=raio fixo,
 * pill=h/2). Ver source/ui9_gen.h e README_assets.txt. */
bool UI_AssetsReady(void);
void UI_NineSliceImg(C2D_Image img, float x, float y, float w, float h,
                     float srcInset, float dstCorner, ColorRGBA tint);
void UI_NineCard(float x, float y, float w, float h, float r, ColorRGBA tint);
void UI_NinePill(float x, float y, float w, float h, ColorRGBA tint);
void UI_NineFocus(float x, float y, float w, float h, float r, ColorRGBA tint);
void UI_NineShadow(float x, float y, float w, float h, float r, ColorRGBA tint);
/* Anel nitido (contorno AA, sem halo) -- foco/seleção. Preenche o rect com a
 * borda; centro transparente. */
void UI_Ring(float x, float y, float w, float h, float r, ColorRGBA tint);
/* Glow radial 1-sprite (substitui circulos translucidos empilhados) e marca de
 * sucesso, ambos tintados/centrados. */
void UI_Glow(float cx, float cy, float diameter, ColorRGBA tint);
void UI_Check(float cx, float cy, float size, ColorRGBA tint);

void UI_RoundRect(float x, float y, float w, float h, float r, ColorRGBA color);
void UI_RoundFrame(float x, float y, float w, float h, float r, ColorRGBA fill, ColorRGBA border);
/* r deve bater com o raio do elemento que recebe a sombra, senao sobra canto
 * quadrado duro por tras de formas redondas (ver comentario na definicao). */
void UI_Shadow(float x, float y, float w, float h, float r, float alpha, float offset);
void UI_GradientH(float x, float y, float w, float h, ColorRGBA left, ColorRGBA right);

void UI_Text(C2D_TextBuf buf, C2D_Font font, const char* text,
             float x, float y, float sx, float sy, ColorRGBA color);
void UI_TextRight(C2D_TextBuf buf, C2D_Font font, const char* text,
                  float right, float y, float sx, float sy, ColorRGBA color);
void UI_TextCenter(C2D_TextBuf buf, C2D_Font font, const char* text,
                   float centerX, float y, float sx, float sy, ColorRGBA color);
/* Centralizado com auto-shrink ate caber em maxW (v9 §6). */
void UI_TextCenterFit(C2D_TextBuf buf, C2D_Font font, const char* text,
                      float centerX, float y, float sxMax, float sxMin,
                      float maxW, ColorRGBA color);

/* === Top Screen (macOS desktop) === */
void UI_TopBackground(void);
void UI_TopMenuBar(const char* title, C2D_TextBuf buf);
void UI_Card(float x, float y, float w, float h, float r, ColorRGBA bg);
void UI_FrostCard(float x, float y, float w, float h, float r);

/* Popup modal (Travel Agency style). Dois modos:
 *  - toast: popupShow(), some sozinho depois de um tempo (sem cancelLabel).
 *  - confirm: popupShowConfirm(), BLOQUEIA ate o usuario responder A/B (toque
 *    ou botao fisico) -- usado pelos popups de "tem certeza?" de Fontes/HEX
 *    (spec v4 4.2/4.4). Selos A/B = sprites Reva reais (ver UI_ABBadge). */
typedef struct {
    bool active;
    bool confirmMode;
    bool closing;
    float animT;     /* 0..1 cru, progresso de entrada (220/360ms, ver popupRender) */
    float closeT;     /* 0..1 cru, progresso de saida (easeInCubic 240ms) */
    float bgAlpha;
    int result;        /* 0 pendente, 1 confirmado (A), -1 cancelado (B) */
    char message[64];
    char confirmLabel[16];
    char cancelLabel[16];
} PopupModal;

void popupShow(PopupModal* p, const char* msg, const char* confirm, const char* cancel);
/* Abre em modo confirm: "msg" + selos A=Sim / B=Nao. Bloqueia at o usuario
 * decidir -- ver popupConfirmInput(). */
void popupShowConfirm(PopupModal* p, const char* msg);
void popupHide(PopupModal* p);
bool popupActive(PopupModal* p);
void popupUpdate(PopupModal* p);
/* So tem efeito em modo confirm, enquanto ainda nao fechou. Retorna true UMA
 * vez, no frame em que o usuario decide (p->result fica 1 ou -1 a partir
 * dai) -- o caller deve agir no resultado nesse mesmo frame; o popup cuida
 * de fechar a propria animacao sozinho depois (popupUpdate). */
bool popupConfirmInput(PopupModal* p, const AppInput* in);
void popupRender(C2D_TextBuf buf, PopupModal* p);

/* Selo A/B Reva real (sprite ICON_BADGE_A/B: contorno preto bold + fill
 * verde/vermelho do semaforo macOS, spec v5 3.2) -- nao mais procedural. */
void UI_ABBadge(float cx, float cy, float r, bool isA, float alpha);

/* === Bottom Screen (Touch Bar) === */
void UI_BottomBackground(void);
void UI_TouchBarBackground(void);

typedef struct {
    const char* label;
    const char* icon;
    float appearT;
    bool hover;
    float pulsePhase;
} TouchBarButtonState;

void UI_TouchBarPill(C2D_TextBuf buf, float x, float y, float w, float h,
                     const char* label, const char* icon, bool selected, float appearT, float pulse);
void UI_TouchBarRow(C2D_TextBuf buf, const char** labels, const char** icons,
                    int count, int selected, float baseY, float baseAppear);
/* morphTween: Tween estatico do CHAMADOR (uma instancia por tela que usa o
 * componente -- nao compartilhar a mesma entre Tema e LED). */
void UI_TouchBarSegmented(C2D_TextBuf buf, float x, float y, float w, float h,
                          const char** labels, int count, int selected, Tween* morphTween);
/* Touch Bar colorida do macOS (v3 3.4) -- espectro HSV continuo + thumb.
 * hueT 0..1 (0=vermelho, mapeado linear pros 360 graus). */
void UI_HueSpectrumBar(float x, float y, float w, float h, float hueT, bool dragging);
void UI_TouchBarSlider(C2D_TextBuf buf, float x, float y, float w,
                       const char* label, int value, int min, int max,
                       bool selected, ColorRGBA swatch);
/* thumbScale: 1.0 solto, ate ~1.18 arrastando (curva easeOutBack mantida
 * pelo chamador). displayValue: valor ja tweened para a contagem animada
 * (180ms easeOutCubic, spec 3.5). Ver led.c. */
void UI_TouchBarSliderDrag(C2D_TextBuf buf, float x, float y, float w,
                           const char* label, int value, int min, int max,
                           bool selected, ColorRGBA swatch, float thumbScale, float displayValue);
void UI_HelpBar(C2D_TextBuf buf, const char* left, const char* right);

/* Badge, pill, shared */
void UI_Badge(C2D_TextBuf buf, float x, float y, const char* text, ColorRGBA bg);
/* iconImg: indice IconID (icons.h) para usar um glifo real, ou -1 para usar
 * o fallback de texto em "icon" (ou nenhum icone, se ambos forem nulos/-1). */
void UI_PillButton(C2D_TextBuf buf, float x, float y, float w, float h,
                   const char* label, const char* icon, int iconImg, bool selected, float appearT);
/* Microinteracoes 3.4 + spec v7 Parte C: pressScale 1.0=solto, 0.96=fundo do
 * press; focusScale 1.0=sem foco, ~1.08=focado (ambos Tweens do chamador). */
void UI_PillButtonPress(C2D_TextBuf buf, float x, float y, float w, float h,
                        const char* label, const char* icon, int iconImg,
                        bool selected, float appearT, float pressScale, float focusScale);
void UI_StartupLogo(C2D_TextBuf buf, float t);

/* §1/§2: emblema compartilhado das 3 bolinhas glass (boot, home e handoff),
 * extraido do bootGlassBall. (cx,cy) = centro; scale 1.0 = R24 com offsets
 * rosa(0,-15) ciano(-19,+11) verde(+19,+11); idleT alimenta o float idle
 * (seno lento, fase/amp diferentes) e o glow rosa que respira atras; alpha
 * global. Passe um idleT crescente (uiFrameTime()) na home; o mesmo emblema
 * e usado na boot/handoff so reposicionado/escalado. */
void UI_Emblem(float cx, float cy, float scale, float idleT, float alpha);

/* Chip pequeno "rotulo + valor" com um ponto colorido indicador -- usado para
 * preencher os cards de cima com dados reais em vez de espaco vazio. */
/* Card de navegacao end4-Monet (v3 3.1): icone Reva + label/valor, raio
 * RADIUS_CARD, halo contido quando selecionado (nunca extrapola o card). */
/* Card de navegacao do carrossel da Inicio (spec v5 5): alpha esmaece os
 * peeks fora de foco; contentReveal (0..1) faz o stagger icone->titulo->
 * valor assentar enquanto o card de centro entra (1.0 = revelado, sem
 * stagger -- use isso pra um card "estatico" fora de uma transicao). */
void UI_NavCardFx(C2D_TextBuf buf, float x, float y, float w, float h,
               IconID icon, const char* label, const char* value,
               ColorRGBA dot, bool selected, float alpha, float contentReveal);
void UI_StatChip(C2D_TextBuf buf, float x, float y, float w, float h,
                 const char* label, const char* value, ColorRGBA dot);

/* Mini "janela" reconhecivel (barra de titulo com semaforos + area de
 * conteudo na cor do tema) -- usada como preview do tema claro/escuro. */
void UI_MiniWindow(float x, float y, float w, float h, bool dark);

#endif
