#include <math.h>
#include <stdlib.h>
#include "anim.h"

float EASE_OUT[256];
float EASE_IN_OUT[256];

void animInit(void) {
    for (int i = 0; i < 256; i++) {
        float t = i / 255.0f;
        EASE_OUT[i] = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
        if (t < 0.5f) {
            EASE_IN_OUT[i] = 4.0f * t * t * t;
        } else {
            float tn = -2.0f * t + 2.0f;
            EASE_IN_OUT[i] = 1.0f - (tn * tn * tn) / 2.0f;
        }
    }
}

void animStep(Anim* a) {
    if (a->finished) return;
    float diff = a->target - a->value;
    a->value += diff * a->speed;
    if (fabsf(diff) < 0.001f) {
        a->value = a->target;
        a->finished = true;
    }
}

float animEasedOut(Anim* a) {
    int idx = (int)(a->value * 255.0f);
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;
    return EASE_OUT[idx];
}

float animEasedInOut(Anim* a) {
    int idx = (int)(a->value * 255.0f);
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;
    return EASE_IN_OUT[idx];
}

void animTo(Anim* a, float target) {
    a->target = target;
    a->finished = false;
}

void animSet(Anim* a, float value, float speed) {
    a->value = value;
    a->target = value;
    a->speed = speed;
    a->finished = true;
}

float animApproach(float current, float target, float speed) {
    float diff = target - current;
    if (fabsf(diff) < 0.5f) return target;
    return current + diff * speed;
}

float animPulse(float time, float speed) {
    return (sinf(time * speed) + 1.0f) * 0.5f;
}

float easeOutCubic(float t) {
    if (t >= 1.0f) return 1.0f;
    if (t <= 0.0f) return 0.0f;
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

float easeInOutCubic(float t) {
    if (t >= 1.0f) return 1.0f;
    if (t <= 0.0f) return 0.0f;
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float tn = -2.0f * t + 2.0f;
        return 1.0f - (tn * tn * tn) / 2.0f;
    }
}
