#include "anim.h"

float g_parallaxOffset = 0;
float g_bootProgress = 0;
bool g_bootComplete = false;

void animInit(void) {}
void animUpdate(float dt) {
    g_parallaxOffset = sinf(appGetTime() * 0.3f) * 2;
    if (!g_bootComplete) {
        g_bootProgress = fminf(1, g_bootProgress + dt * 0.15f);
        if (g_bootProgress >= 1) g_bootComplete = true;
    }
}

void springInit(Spring* s, float target, float stiff, float damp, float mass) {
    s->pos = target; s->vel = 0; s->target = target;
    s->stiff = stiff; s->damp = damp; s->mass = mass;
}

void springSetTarget(Spring* s, float t) { s->target = t; }

bool springStep(Spring* s, float dt) {
    float f = -(s->stiff * (s->pos - s->target));
    float d = -s->damp * s->vel;
    float a = (f + d) / s->mass;
    s->vel += a * dt;
    s->pos += s->vel * dt;
    return fabsf(s->pos - s->target) < 0.5f && fabsf(s->vel) < 0.5f;
}

Transition transStart(float duration, float delay) {
    Transition t = {0, duration, delay, 0, true, false, easeOutCubic};
    return t;
}

float transGet(Transition* t) {
    return t->active ? t->progress : 1;
}

float transUpdate(Transition* t, float dt) {
    if (!t->active || t->finished) return 1;
    t->elapsed += dt;
    if (t->elapsed < t->delay) return 0;
    t->progress = clampf((t->elapsed - t->delay) / t->duration, 0, 1);
    if (t->progress >= 1) { t->finished = true; t->active = false; }
    return t->easeFunc(t->progress);
}

void animStateInit(AnimState* a) {
    memset(a, 0, sizeof(AnimState));
    a->opacity = 0; a->scale = 0.8f;
    springInit(&a->opacityS, 1, 100, 10, 1);
    springInit(&a->scaleS, 1, 300, 20, 1);
    springInit(&a->slideXS, 0, 100, 10, 1);
    springInit(&a->slideYS, 0, 100, 10, 1);
}

void animStateFadeIn(AnimState* a) { a->active = true; a->opacity = 0; springSetTarget(&a->opacityS, 1); }
void animStateFadeOut(AnimState* a) { a->active = true; springSetTarget(&a->opacityS, 0); }
void animStateSlideIn(AnimState* a, float fx, float fy) {
    a->active = true; a->slideX = fx; a->slideY = fy;
    springSetTarget(&a->slideXS, 0); springSetTarget(&a->slideYS, 0);
}
void animStatePopIn(AnimState* a) {
    a->active = true; a->scale = 0.7f; a->opacity = 0;
    springSetTarget(&a->scaleS, 1); springSetTarget(&a->opacityS, 1);
}

void animStateUpdate(AnimState* a, float dt) {
    if (!a->active) return;
    springStep(&a->opacityS, dt); springStep(&a->scaleS, dt);
    springStep(&a->slideXS, dt); springStep(&a->slideYS, dt);
    a->opacity = a->opacityS.pos; a->scale = a->scaleS.pos;
    a->slideX = a->slideXS.pos; a->slideY = a->slideYS.pos;
    if (fabsf(a->opacity - a->opacityS.target) < 0.5f && fabsf(a->scale - a->scaleS.target) < 0.5f) {
        if (a->opacityS.target <= 0) memset(a, 0, sizeof(AnimState));
    }
}
