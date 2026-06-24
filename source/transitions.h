#ifndef TRANSITIONS_H
#define TRANSITIONS_H

#include <stdbool.h>
#include "anim.h"

/* Pool de transicoes de troca de tela (PROMPT_CustomizerDS_v8 secao 3).
 *
 * REESCRITO pro modelo do compositor (secao 2): cada transicao agora descreve
 * DUAS camadas -- a tela VELHA (A) saindo e a tela NOVA (B) entrando -- que
 * main.c/compositor compoem a partir de 2 texturas offscreen. Por isso toda
 * transicao tem ENTRADA e SAIDA de verdade (acabou o "some seco") e a tela se
 * move como BLOCO unico (acaba o "emborrachado").
 *
 * Os 10 ids batem 1:1 com a tabela da secao 3 (numero ao lado). Tiers:
 *   A (polidas, no pool aleatorio): 1,2,3,4,5,8,10
 *   B (wipe radial/diagonal, precisa mascara): 6,7  -- so no Laboratorio ate
 *      o stencil entrar; por ora caem num crossfade seguro.
 *   C (flip): 9 -- so no Laboratorio. */
typedef enum {
    TRANS_PUSH_H = 0,    /* 1  Push Horizontal */
    TRANS_COVER_V,       /* 2  Cover Vertical */
    TRANS_CROSSFADE,     /* 3  Cross-Fade Settle */
    TRANS_ZOOM_THROUGH,  /* 4  Zoom Through */
    TRANS_POP,           /* 5  Pop (sutil) */
    TRANS_RADIAL_WIPE,   /* 6  Radial Wipe (tier B) */
    TRANS_DIAGONAL_WIPE, /* 7  Diagonal Wipe (tier B) */
    TRANS_SLIDE_UP,      /* 8  Slide Up Parallax */
    TRANS_DEPTH_FLIP,    /* 9  Depth Flip (tier C) */
    TRANS_DISSOLVE,      /* 10 Dissolve Settle */
    TRANS_COUNT
} TransitionID;

/* Transformacao de UMA camada de tela inteira (origem de escala = centro). */
typedef struct {
    float x, y;            /* translacao em px */
    float scaleX, scaleY;  /* 1.0 = sem escala */
    float alpha;           /* 0..1 */
} TransLayer;

/* Tipo de revelacao por mascara (tier B). NONE = composicao normal por
 * transform/alpha das 2 camadas. */
typedef enum {
    REVEAL_NONE = 0,
    REVEAL_RADIAL,
    REVEAL_DIAGONAL
} RevealKind;

typedef struct {
    TransLayer oldL;       /* tela velha (A) saindo */
    TransLayer newL;       /* tela nova (B) entrando */
    bool newAbove;         /* ordem de desenho: true = velha embaixo, nova por cima */
    float scrimAlpha;      /* 0..1 veu preto por cima das 2 camadas (escurecer na dobra) */
    RevealKind reveal;     /* tier B: nova revelada por mascara sobre a velha */
    float revealT;         /* 0..1 progresso da mascara */
    float originX, originY;/* origem do reveal (centro por padrao) */
} TransitionFrame;

/* Duracao total (segundos) -- 260-420ms, nunca linear (secao 3). */
float transDuration(TransitionID id);

/* Avalia a transicao em t (segundos desde o disparo) para uma tela de WxH,
 * na direcao navDir (-1 voltar / +1 abrir), preenchendo *out. */
void transEval(TransitionID id, float t, int navDir, float W, float H, TransitionFrame* out);

/* Sorteia a proxima do POOL POLIDO (1,2,3,4,8,10), nunca repetindo a anterior
 * (secao 3: "pool aleatorio padrao = so as polidas"). As demais (6,7,9) so
 * pelo Laboratorio. */
TransitionID transPickNext(void);

/* Nome legivel e easing principal em texto -- usados pelo Laboratorio (secao 7). */
const char* transName(TransitionID id);
const char* transEaseName(TransitionID id);

#endif
