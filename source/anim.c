#include "anim.h"
#include "ui.h"  /* uiFrameDt() p/ colorTweenTo auto-update */

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
        /* 1.8.0 motor CAELESTIA (tokens.hpp). */
        case EASE_EXPR_SLOW_SPATIAL: return cubicBezier(0.39f, 1.29f, 0.35f, 0.98f, t);
        case EASE_EFF_FAST:     return cubicBezier(0.31f, 0.94f, 0.34f, 1.00f, t);
        case EASE_EFF_DEFAULT:  return cubicBezier(0.34f, 0.80f, 0.34f, 1.00f, t);
        case EASE_EFF_SLOW:     return cubicBezier(0.34f, 0.88f, 0.34f, 1.00f, t);
        case EASE_STANDARD:     return cubicBezier(0.20f, 0.00f, 0.00f, 1.00f, t);
        case EASE_EMPHASIZED:   return easeEmphasized(t);
        default: return t;
    }
}

/* M3 "emphasized" real do caelestia (tokens.hpp m_emphasized):
 * seg1: P0(0,0) C1(0.05,0) C2(2/15,0.06) P3(1/6,0.4);
 * seg2: P0(1/6,0.4) C1(5/24,0.82) C2(0.25,1) P3(1,1). Cada segmento
 * re-escalado p/ [0,1]x[0,1] e avaliado por cubicBezier. */
float easeEmphasized(float x) {
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;
    if (x < 1.0f / 6.0f) {
        float u = x / (1.0f / 6.0f);
        return 0.4f * cubicBezier(0.30f, 0.00f, 0.80f, 0.15f, u);
    } else {
        float u = (x - 1.0f / 6.0f) / (5.0f / 6.0f);
        return 0.4f + 0.6f * cubicBezier(0.05f, 0.70f, 0.10f, 1.00f, u);
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

/* ===== 1.8.0 motor CAELESTIA ===== */

void colorTweenStart(ColorTween* ct, ColorRGBA from, ColorRGBA to, float duration, EaseType ease) {
    ct->from = from; ct->to = to; ct->t = 0.0f;
    ct->duration = (duration > 0.0001f) ? duration : 0.0001f;
    ct->ease = ease; ct->active = true;
}
void colorTweenUpdate(ColorTween* ct, float dt) {
    if (!ct->active) return;
    ct->t += dt;
    if (ct->t >= ct->duration) { ct->t = ct->duration; ct->active = false; }
}
ColorRGBA colorTweenValue(const ColorTween* ct) {
    float p = easeFunc(clampf(ct->t / ct->duration, 0.0f, 1.0f), ct->ease);
    return themeMix(ct->from, ct->to, p);
}
static bool colEq(ColorRGBA a, ColorRGBA b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
void colorTweenTo(ColorTween* ct, ColorRGBA to) {
    /* nao inicializado (duration 0) -> assenta direto na cor. */
    if (ct->duration <= 0.0001f) { colorTweenStart(ct, to, to, DUR_EFFECTS_SLOW, EASE_EFF_SLOW); ct->active = false; return; }
    if (!colEq(ct->to, to)) colorTweenStart(ct, colorTweenValue(ct), to, DUR_EFFECTS_SLOW, EASE_EFF_SLOW);
    else colorTweenUpdate(ct, uiFrameDt());
}

void rectMorphSnap(RectMorph* m, float x, float y, float w, float h) {
    tweenStart(&m->x, x, x, 0.0001f, EASE_LINEAR); tweenStart(&m->y, y, y, 0.0001f, EASE_LINEAR);
    tweenStart(&m->w, w, w, 0.0001f, EASE_LINEAR); tweenStart(&m->h, h, h, 0.0001f, EASE_LINEAR);
    tweenUpdate(&m->x, 1.0f); tweenUpdate(&m->y, 1.0f); tweenUpdate(&m->w, 1.0f); tweenUpdate(&m->h, 1.0f);
    m->init = true;
}
void rectMorphTo(RectMorph* m, float x, float y, float w, float h, float dur, EaseType ease) {
    if (!m->init) { rectMorphSnap(m, x, y, w, h); return; }
    /* retarget: parte sempre do valor ATUAL (input rapido nao trava/salta). */
    if (fabsf(m->x.to - x) > 0.5f || fabsf(m->y.to - y) > 0.5f ||
        fabsf(m->w.to - w) > 0.5f || fabsf(m->h.to - h) > 0.5f) {
        tweenStart(&m->x, tweenValue(&m->x), x, dur, ease);
        tweenStart(&m->y, tweenValue(&m->y), y, dur, ease);
        tweenStart(&m->w, tweenValue(&m->w), w, dur, ease);
        tweenStart(&m->h, tweenValue(&m->h), h, dur, ease);
    }
}
void rectMorphUpdate(RectMorph* m, float dt) {
    tweenUpdate(&m->x, dt); tweenUpdate(&m->y, dt); tweenUpdate(&m->w, dt); tweenUpdate(&m->h, dt);
}
