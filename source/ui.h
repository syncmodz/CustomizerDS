#ifndef UI_H
#define UI_H

#include "theme.h"
#include "anim.h"
#include <citro2d.h>
#include <stdbool.h>

// ── BLUR SIMULADO ──────────────────────────────//
// Desenha N camadas concêntricas com alpha decrescente
// criando ilusão de glow/blur (técnica: gaussian aproximado)
void UI_Glow(float x, float y, float w, float h,
             u32 color, int layers);

// ── CARD MATERIAL ──────────────────────────────//
// Card com fundo surface + borda sutil + glow se selecionado
void UI_Card(float x, float y, float w, float h,
             bool selected, float selectAnim);

// ── ITEM DE LISTA ──────────────────────────────//
// Card completo com texto, ícone e indicador de seleção
void UI_ListItem(C2D_TextBuf buf, float x, float y,
                 float w, float h, const char* label,
                 const char* sublabel,  // pode ser NULL
                 bool selected, float selectAnim,
                 const char* rightLabel); // ex: ">" ou "ON" ou NULL

// ── HEADER DO MÓDULO ───────────────────────────//
// Barra no topo: cor primary, título em branco, linha accent embaixo
void UI_Header(C2D_TextBuf buf, const char* title,
               const char* subtitle); // subtitle pode ser NULL

// ── RODAPÉ DA TELA INFERIOR ──────────────────//
// Barra no fundo: dicas de controles com ícones de botão
void UI_Footer(C2D_TextBuf buf,
               const char* aLabel,     // texto do botão A (ou NULL)
               const char* bLabel,     // texto do botão B (ou NULL)
               const char* startLabel); // texto do START (ou NULL)

// ── RIPPLE ─────────────────────────────────────//
// Efeito de onda circular ao pressionar (círculo expandindo)
typedef struct {
    float x, y;        // centro do ripple
    float radius;      // raio atual
    float maxRadius;   // raio máximo
    float alpha;       // opacidade atual (255 → 0)
    bool  active;
} Ripple;

void rippleStart(Ripple* r, float x, float y, float maxR);
void rippleStep(Ripple* r);   // chamar todo frame
void rippleDraw(Ripple* r, u32 color); // desenhar se active

// ── SWITCH / TOGGLE ──────────────────────────────//
// Toggle estilo iOS/macOS com animação de deslize
typedef struct {
    bool  value;
    Anim  thumbAnim;   // 0.0=esquerda(off), 1.0=direita(on)
    Anim  colorAnim;   // fade entre cor off e on
} UISwitch;

void uiSwitchInit(UISwitch* s, bool initialValue);
void uiSwitchToggle(UISwitch* s);
void uiSwitchStep(UISwitch* s);
void uiSwitchDraw(UISwitch* s, float x, float y);

// ── BARRA DE PROGRESSO / LOADING ───────────────//
// Shimmer animado com lerp de cor
typedef struct { float phase; } UILoader;

void uiLoaderStep(UILoader* l);
void uiLoaderDraw(UILoader* l, float x, float y, float w);

// ── SEPARADOR ──────────────────────────────────//
void UI_Divider(float x, float y, float w);

// ── TEXTO COM TRUNCAMENTO ──────────────────────//
// Imprime texto cortando com "..." se ultrapassar maxWidth
void UI_TextEllipsis(C2D_TextBuf buf, const char* text,
                     float x, float y, float scaleX, float scaleY,
                     u32 color, float maxWidth);

#endif // UI_H
