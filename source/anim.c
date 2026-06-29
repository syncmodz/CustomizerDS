#include "anim.h"

float easeFunc(float t, EaseType type) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    switch (type) {
        case EASE_LINEAR: return t;
        case EASE_OUT_SINE: return sinf(t * M_PI / 2.0f);
        case EASE_OUT_CUBIC: return 1.0f - powf(1.0f - t, 3.0f);
        case EASE_OUT_QUINT: return 1.0f - powf(1.0f - t, 5.0f);
        case EASE_OUT_BACK: {
            float c1 = 1.70158f;
            float c3 = c1 + 1.0f;
            return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
        }
        case EASE_OUT_EXPO: return (t >= 1.0f) ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
        case EASE_IN_OUT_CUBIC:
            return (t < 0.5f) ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        case EASE_IN_OUT_BACK: {
            float c1 = 1.70158f;
            float c2 = c1 * 1.525f;
            return (t < 0.5f)
                ? (powf(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
                : (powf(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
        }
        case EASE_OUT_ELASTIC: {
            if (t == 0.0f || t == 1.0f) return t;
            float c4 = 2.0f * M_PI / 3.0f;
            return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
        }
        case EASE_IN_CUBIC: return t * t * t;
        case EASE_SPRING: return easeSpringAmp(t, 0.75f);
        /* 1.4.0 §FLUIDEZ: curvas END4 (valores reais do general.conf). */
        case EASE_EMPH_DECEL:   return cubicBezier(0.05f, 0.70f, 0.10f, 1.00f, t);
        case EASE_EMPH_ACCEL:   return cubicBezier(0.30f, 0.00f, 0.80f, 0.15f, t);
        case EASE_EXPR_SPATIAL: return cubicBezier(0.38f, 1.21f, 0.22f, 1.00f, t);
        case EASE_EXPR_FAST:    return cubicBezier(0.42f, 1.67f, 0.21f, 0.90f, t);
        case EASE_MENU_DECEL:   return cubicBezier(0.10f, 1.00f, 0.00f, 1.00f, t);
        default: return t;
    }
}

/* B(s) de um cubic-bezier 1D com P0=0, P3=1 e controles p1,p2. */
static float bezAxis(float p1, float p2, float s) {
    float u = 1.0f - s;
    return 3.0f * u * u * s * p1 + 3.0f * u * s * s * p2 + s * s * s;
}
/* dB/ds do mesmo (p/ Newton). */
static float bezAxisD(float p1, float p2, float s) {
    float u = 1.0f - s;
    return 3.0f * u * u * p1 + 6.0f * u * s * (p2 - p1) + 3.0f * s * s * (1.0f - p2);
}
float cubicBezier(float p1x, float p1y, float p2x, float p2y, float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    /* acha s tal que X(s)=t por Newton-Raphson (x e monotonico pra p?x em
     * [0,1]); 8 iteracoes convergem bem e e barato por frame. */
    float s = t;
    for (int i = 0; i < 8; i++) {
        float x = bezAxis(p1x, p2x, s) - t;
        if (fabsf(x) < 1e-4f) break;
        float d = bezAxisD(p1x, p2x, s);
        if (fabsf(d) < 1e-6f) break;
        s -= x / d;
    }
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    return bezAxis(p1y, p2y, s); /* Y(s) -- pode passar de 1 (overshoot) */
}

/* Oscilador harmonico amortecido fechado (nao uma simulacao por dt como
 * springTo()) -- da uma curva 0..1 deterministica que pode ser amostrada em
 * qualquer t, exatamente como as demais curvas, entao funciona dentro do
 * Tween existente (tweenStart/Update/Value) sem precisar de estado extra.
 * w fixo (~1.1 ciclo dentro da janela 0..1) calibrado no prototipo web. */
float easeSpringAmp(float t, float dampingRatio) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    float damp = dampingRatio;
    if (damp < 0.05f) damp = 0.05f;
    if (damp > 0.999f) damp = 0.999f;
    float w = 2.0f * M_PI * 1.1f;
    float decay = expf(-damp * w * t);
    float wd = w * sqrtf(1.0f - damp * damp);
    return 1.0f - decay * cosf(wd * t);
}

/* Variante de EASE_OUT_BACK com amplitude de overshoot customizavel (a spec
 * 3.3 pede "overshoot ~1.5" para o popup do hex, diferente do c1=1.70158
 * padrao usado em easeFunc -- aqui o parametro e literal, nao aproximado). */
float easeOutBackAmp(float t, float overshoot) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    float c1 = overshoot;
    float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}

void transitionStart(ScreenTransition* t, TransitionType type, int from, int to, float duration) {
    t->type = type;
    t->fromScreen = from;
    t->toScreen = to;
    t->duration = duration;
    t->progress = 0.0f;
    t->active = true;
}

void transitionUpdate(ScreenTransition* t, float dt) {
    if (!t->active) return;
    t->progress += dt;
    if (t->progress >= t->duration) {
        t->progress = t->duration;
        t->active = false;
    }
}

bool transitionActive(ScreenTransition* t) {
    return t->active;
}

float transitionValue(ScreenTransition* t) {
    if (!t->active) return 1.0f;
    return fminf(t->progress / t->duration, 1.0f);
}

float transitionEased(ScreenTransition* t, EaseType ease) {
    return easeFunc(transitionValue(t), ease);
}

float springTo(float current, float target, float* velocity, float stiffness, float damping, float dt) {
    float diff = target - current;
    float springForce = diff * stiffness;
    float dampForce = *velocity * damping;
    float acceleration = springForce - dampForce;
    *velocity += acceleration * dt;
    current += *velocity * dt;
    if (fabsf(diff) < 0.5f && fabsf(*velocity) < 0.5f) {
        current = target;
        *velocity = 0.0f;
    }
    return current;
}

float approach(float current, float target, float maxStep) {
    float diff = target - current;
    if (fabsf(diff) < maxStep) return target;
    return current + copysignf(maxStep, diff);
}

void tweenStart(Tween* tw, float from, float to, float duration, EaseType ease) {
    tw->from = from;
    tw->to = to;
    tw->t = 0.0f;
    tw->duration = (duration > 0.0001f) ? duration : 0.0001f;
    tw->ease = ease;
    tw->active = true;
}

void tweenUpdate(Tween* tw, float dt) {
    if (!tw->active) return;
    tw->t += dt;
    if (tw->t >= tw->duration) {
        tw->t = tw->duration;
        tw->active = false;
    }
}

float tweenValue(Tween* tw) {
    float p = easeFunc(tw->t / tw->duration, tw->ease);
    return tw->from + (tw->to - tw->from) * p;
}

bool tweenDone(Tween* tw) {
    return !tw->active;
}
