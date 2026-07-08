#include "transitions.h"
#include "anim.h"
#include "common.h"
#include <stdlib.h>

/* Duracoes da tabela da secao 3 (todas 260-420ms). */
/* v1.2: durações um tico maiores nas mais "secas" pra dar um assentamento mais
 * suave/premium sem ficar lento (sweet spot ~320-380ms pra troca de tela). */
/* 1.9.0 FIX2: transicoes de aba mais CURTAS (0.30s) -> resposta imediata ao
 * botao, sem "respirar antes de andar". Wipes cenicos (6/7/9/10) seguem um tico
 * maiores (nao sao disparados por navegacao rapida). */
static const float DURATIONS[TRANS_COUNT] = {
    0.30f, /* 1  PUSH_H        */
    0.30f, /* 2  COVER_V */
    0.28f, /* 3  CROSSFADE     */
    0.30f, /* 4  ZOOM_THROUGH  */
    0.30f, /* 5  POP           */
    0.38f, /* 6  RADIAL_WIPE */
    0.36f, /* 7  DIAGONAL_WIPE */
    0.30f, /* 8  SLIDE_UP      */
    0.36f, /* 9  DEPTH_FLIP */
    0.34f, /* 10 DISSOLVE */
};

static const char* NAMES[TRANS_COUNT] = {
    "1 Push Horizontal", "2 Cover Vertical", "3 Cross-Fade Settle",
    "4 Zoom Through", "5 Pop", "6 Radial Wipe", "7 Diagonal Wipe",
    "8 Slide Up Parallax", "9 Depth Flip", "10 Dissolve Settle"
};

float transDuration(TransitionID id) {
    if (id < 0 || id >= TRANS_COUNT) return 0.30f;
    return DURATIONS[id];
}

static const char* EASES[TRANS_COUNT] = {
    "IN_OUT_CUBIC", "OUT_QUINT", "OUT_SINE", "OUT_QUINT", "OUT_BACK 0.95",
    "OUT_CUBIC wipe", "OUT_CUBIC wipe", "OUT_QUINT", "IN_OUT_CUBIC", "SPRING+SINE"
};

const char* transName(TransitionID id) {
    if (id < 0 || id >= TRANS_COUNT) return "?";
    return NAMES[id];
}

const char* transEaseName(TransitionID id) {
    if (id < 0 || id >= TRANS_COUNT) return "?";
    return EASES[id];
}

static TransLayer idLayer(void) {
    TransLayer l = { 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
    return l;
}

static void clearFrame(TransitionFrame* o, float W, float H) {
    o->oldL = idLayer();
    o->newL = idLayer();
    o->newAbove = true;
    o->scrimAlpha = 0.0f;
    o->reveal = REVEAL_NONE;
    o->revealT = 0.0f;
    o->originX = W * 0.5f;
    o->originY = H * 0.5f;
}

void transEval(TransitionID id, float t, int navDir, float W, float H, TransitionFrame* out) {
    clearFrame(out, W, H);
    float dur = transDuration(id);
    float t01 = clampf(t / dur, 0.0f, 1.0f);

    switch (id) {
        case TRANS_PUSH_H: {
            /* tabela 1: velha x=-W*p, nova x=W*(1-p). 1.9.0 FIX2: EMPH_DECEL --
             * a EMPHASIZED (2 seg) comeca quase parada nos 1os ~17% e, numa troca
             * de aba disparada por botao, isso lia como INPUT LAG. DECEL comeca
             * rapido (resposta no frame seguinte) e assenta suave. */
            float p = easeFunc(t01, EASE_EMPH_DECEL);
            out->oldL.x = -(float)navDir * W * p;
            out->newL.x =  (float)navDir * W * (1.0f - p);
            break;
        }
        case TRANS_COVER_V: {
            /* tabela 2: nova sobe cobrindo a velha (OUT_QUINT). Velha afunda
             * 8px e escurece 40%; nova vem de baixo (y=H*(1-p)). */
            float p = easeFunc(t01, EASE_OUT_QUINT);
            out->oldL.y = 8.0f * p;
            out->oldL.alpha = 1.0f - 0.4f * p;
            out->newL.y = H * (1.0f - p);
            out->newAbove = true;
            break;
        }
        case TRANS_CROSSFADE: {
            /* tabela 3: crossfade puro + settle de escala. 1.6.0: settle um tico
             * mais presente (0.975->1.0) com EMPH_DECEL -- entra "vindo pra
             * frente" com mais vida, sem deixar de ser manteiga. */
            float p = easeFunc(t01, EASE_EMPH_DECEL);
            out->oldL.alpha = 1.0f - easeFunc(t01, EASE_OUT_SINE);
            out->newL.alpha = p;
            float s = lerpf(0.975f, 1.0f, p);
            out->newL.scaleX = s; out->newL.scaleY = s;
            break;
        }
        case TRANS_ZOOM_THROUGH: {
            /* tabela 4: velha cresce 1.0->1.06 sumindo; nova 0.94->1.0 surgindo.
             * v1.2: OUT_QUINT (era OUT_CUBIC) -- desaceleracao mais suave/premium. */
            float p = easeFunc(t01, EASE_OUT_QUINT);
            float so = lerpf(1.0f, 1.06f, p);
            out->oldL.scaleX = so; out->oldL.scaleY = so;
            out->oldL.alpha = 1.0f - p;
            float sn = lerpf(0.94f, 1.0f, p);
            out->newL.scaleX = sn; out->newL.scaleY = sn;
            out->newL.alpha = p;
            break;
        }
        case TRANS_POP: {
            /* tabela 5: nova escala 0.9->1.0 com back DISCRETO (c1~0.95),
             * alpha nos 1os 60%; velha some nos 1os 40%. Ancora = centro
             * (sem rastrear toque numa troca de tela). */
            float eGrow = easeOutBackAmp(t01, 0.95f);
            float sn = lerpf(0.9f, 1.0f, eGrow);
            out->newL.scaleX = sn; out->newL.scaleY = sn;
            out->newL.alpha = easeFunc(clampf(t01 / 0.6f, 0.0f, 1.0f), EASE_OUT_CUBIC);
            out->oldL.alpha = 1.0f - easeFunc(clampf(t01 / 0.4f, 0.0f, 1.0f), EASE_OUT_CUBIC);
            break;
        }
        case TRANS_SLIDE_UP: {
            /* tabela 8: parallax de saida -- velha sobe 20px sumindo, nova
             * vem 28px de baixo. 1.6.0: nova entra com EMPH_DECEL (END4) +
             * micro-escala 0.98->1.0 pra dar profundidade ao parallax. */
            float p = easeFunc(t01, EASE_EMPH_DECEL);
            out->oldL.y = -20.0f * p;
            out->oldL.alpha = 1.0f - easeFunc(t01, EASE_OUT_QUINT);
            out->newL.y = 28.0f * (1.0f - p);
            out->newL.alpha = easeFunc(clampf(t01 / 0.5f, 0.0f, 1.0f), EASE_OUT_QUINT);
            float sn = lerpf(0.98f, 1.0f, p);
            out->newL.scaleX = sn; out->newL.scaleY = sn;
            break;
        }
        case TRANS_DISSOLVE: {
            /* tabela 10: crossfade + 1 mola gentil na escala da nova (1.03->1.0,
             * 1 oscilacao via easeSpringAmp, overshoot <=3%). */
            out->oldL.alpha = 1.0f - easeFunc(t01, EASE_OUT_CUBIC);
            out->newL.alpha = easeFunc(t01, EASE_OUT_SINE);
            float spring = easeSpringAmp(t01, 0.7f); /* 0..1, 1 oscilacao suave */
            float s = 1.0f + 0.03f * (1.0f - spring); /* 1.03 -> 1.0 */
            out->newL.scaleX = s; out->newL.scaleY = s;
            break;
        }
        case TRANS_DEPTH_FLIP: {
            /* tabela 9 (tier C): velha encolhe em X ate 0 (1a metade), nova
             * abre de 0 a 1 (2a metade), com escurecer leve na "dobra".
             * IN_OUT_CUBIC. Ancora centro. */
            if (t01 < 0.5f) {
                float e = easeFunc(t01 / 0.5f, EASE_IN_OUT_CUBIC);
                out->oldL.scaleX = 1.0f - e;
                out->newL.alpha = 0.0f;
                out->scrimAlpha = 0.25f * e;
                out->newAbove = false; /* velha visivel por cima */
            } else {
                float e = easeFunc((t01 - 0.5f) / 0.5f, EASE_IN_OUT_CUBIC);
                out->newL.scaleX = e;
                out->oldL.alpha = 0.0f;
                out->scrimAlpha = 0.25f * (1.0f - e);
                out->newAbove = true;
            }
            break;
        }
        case TRANS_RADIAL_WIPE: {
            /* tabela 6 (tier B): a nova deve ser revelada por circulo crescente
             * sobre a velha cheia. Sem o stencil (entra num commit proprio) cai
             * num crossfade seguro -- por isso so aparece no Laboratorio ate la.
             * O revealT ja vai preenchido pro compositor usar quando o stencil
             * existir. */
            float p = easeFunc(t01, EASE_OUT_CUBIC);
            out->reveal = REVEAL_RADIAL;
            out->revealT = p;
            out->oldL.alpha = 1.0f - p; /* fallback crossfade */
            out->newL.alpha = p;
            break;
        }
        case TRANS_DIAGONAL_WIPE: {
            /* tabela 7 (tier B): faixa diagonal varrendo. Mesma situacao do 6 --
             * fallback crossfade ate o stencil. */
            float p = easeFunc(t01, EASE_OUT_CUBIC);
            out->reveal = REVEAL_DIAGONAL;
            out->revealT = p;
            out->oldL.alpha = 1.0f - p;
            out->newL.alpha = p;
            break;
        }
        default: break;
    }
}

/* Pool polido (secao 3): so as transicoes "manteiga" entram no sorteio
 * aleatorio. 6,7,9 ficam de fora (acessiveis so pelo Laboratorio). */
static const TransitionID POOL[] = {
    TRANS_PUSH_H, TRANS_COVER_V, TRANS_CROSSFADE,
    TRANS_ZOOM_THROUGH, TRANS_SLIDE_UP, TRANS_DISSOLVE
};
#define POOL_COUNT ((int)(sizeof(POOL) / sizeof(POOL[0])))

static int s_lastPoolIdx = -1;

TransitionID transPickNext(void) {
    int idx = rand() % POOL_COUNT;
    if (idx == s_lastPoolIdx) idx = (idx + 1) % POOL_COUNT;
    s_lastPoolIdx = idx;
    return POOL[idx];
}
