#include <math.h>
#include <stdlib.h>
#include "anim.h"

// Lookup tables (definidas aqui, não só extern)
float EASE_OUT[256];
float EASE_IN_OUT[256];

void animInit(void) {
    for (int i = 0; i < 256; i++) {
        float t = i / 255.0f;
        // ease-out-cubic: f(t) = 1-(1-t)³
        EASE_OUT[i] = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
        
        // ease-in-out: f(t) = t<0.5 ? 4t³ : 1-(-2t+2)³/2
        if (t < 0.5f) {
            float t2 = t * 2.0f;
            EASE_IN_OUT[i] = 4.0f * t2 * t2 * t2 / 2.0f; // 4t³ (t já é 2t)
            // Correção: a fórmula real é 4t³ para t<0.5, onde t é 0..0.5
            // Como t é 0..1, usamos t*2 para normalizar
            float tn = t * 2.0f;
            EASE_IN_OUT[i] = 4.0f * tn * tn * tn;
        } else {
            float tn = (-2.0f * t + 2.0f);
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
