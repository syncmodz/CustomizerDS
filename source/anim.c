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
        default: return t;
    }
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
