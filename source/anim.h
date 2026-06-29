#ifndef ANIM_H
#define ANIM_H

#include <3ds.h>
#include <stdbool.h>
#include <math.h>

typedef enum {
    EASE_LINEAR = 0,
    EASE_OUT_SINE,
    EASE_OUT_CUBIC,
    EASE_OUT_QUINT,
    EASE_OUT_BACK,
    EASE_OUT_EXPO,
    EASE_IN_OUT_CUBIC,
    EASE_IN_OUT_BACK,
    EASE_OUT_ELASTIC,
    EASE_IN_CUBIC, /* spec 3.3: fechar popup usa easeInCubic, nao IN_OUT */
    EASE_SPRING,   /* spec v5 7: spring suave (damping 0.75 default), settle
                     * com leve overshoot organico -- ver easeSpringAmp p/
                     * controlar o damping diretamente (carrossel, lozenge
                     * do LED, pop do icone sol/lua). */
    /* 1.4.0 §FLUIDEZ: curvas EXATAS do END4 (Material 3 expressive, do
     * general.conf do dots-hyprland), portadas via cubicBezier 4-pontos. As
     * "expressive" tem Y>1 no meio (overshoot = a "molinha"). Padrao p/
     * entrar/mover/focar = EMPH_DECEL; sair/fechar = EMPH_ACCEL; deslizamento
     * espacial com molinha = EXPR_SPATIAL. */
    EASE_EMPH_DECEL,     /* cubic-bezier(0.05,0.70,0.10,1.00) */
    EASE_EMPH_ACCEL,     /* cubic-bezier(0.30,0.00,0.80,0.15) */
    EASE_EXPR_SPATIAL,   /* cubic-bezier(0.38,1.21,0.22,1.00) overshoot leve */
    EASE_EXPR_FAST,      /* cubic-bezier(0.42,1.67,0.21,0.90) overshoot forte */
    EASE_MENU_DECEL,     /* cubic-bezier(0.10,1.00,0.00,1.00) abrir painel */
} EaseType;

/* Avaliador de cubic-bezier 4-pontos com P0=(0,0) e P3=(1,1) (igual CSS/
 * Hyprland): recebe t=tempo (eixo x, 0..1), resolve o parametro e devolve y.
 * y pode passar de 1 (overshoot das curvas expressive) -- proposital. */
float cubicBezier(float p1x, float p1y, float p2x, float p2y, float t);

float easeFunc(float t, EaseType type);
float easeOutBackAmp(float t, float overshoot);
/* Curva de spring fechada (oscilador harmonico amortecido), parametrizada
 * por dampingRatio (0..1 -- menor = mais "boing", ~0.55; maior = mais
 * comportado, ~0.85). Calibrado no prototipo web (spec v5 9.4) antes de
 * portar pro C: w fixo em 2*pi*1.1 da uma cadencia de assentamento boa em
 * durações de 250-700ms tipicas do app. */
float easeSpringAmp(float t, float dampingRatio);

typedef enum {
    TRANSITION_NONE = 0,
    TRANSITION_SLIDE_UP,      /* Travel Motion: content slides up with parallax */
    TRANSITION_SCALE_FADE,    /* Travel Agency: card scales+fades in as popup */
    TRANSITION_FADE,          /* simple crossfade */
    TRANSITION_SLIDE_RIGHT,   /* content slides in from right */
} TransitionType;

typedef struct {
    float progress;
    TransitionType type;
    bool active;
    int fromScreen;
    int toScreen;
    float duration;
} ScreenTransition;

void transitionStart(ScreenTransition* t, TransitionType type, int from, int to, float duration);
void transitionUpdate(ScreenTransition* t, float dt);
bool transitionActive(ScreenTransition* t);
float transitionValue(ScreenTransition* t);
float transitionEased(ScreenTransition* t, EaseType ease);

float springTo(float current, float target, float* velocity, float stiffness, float damping, float dt);
float approach(float current, float target, float maxStep);

/* Engine de tween central (docs/animation-spec.md): toda animacao pontual
 * (nao continua, tipo press/release, pop de popup, contagem de valor) deve
 * instanciar um Tween em vez de inventar um contador float solto -- garante
 * que toda curva passa por easeFunc (nunca linear) de forma uniforme. */
typedef struct {
    float from, to;
    float t;        /* segundos acumulados */
    float duration; /* segundos */
    EaseType ease;
    bool active;
} Tween;

void tweenStart(Tween* tw, float from, float to, float duration, EaseType ease);
void tweenUpdate(Tween* tw, float dt);
float tweenValue(Tween* tw);
bool tweenDone(Tween* tw);

#endif
