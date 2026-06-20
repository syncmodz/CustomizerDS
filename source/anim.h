#pragma once
#include "common.h"

typedef struct {
    float pos, vel, target;
    float stiff, damp, mass;
} Spring;

void springInit(Spring* s, float target, float stiff, float damp, float mass);
void springSetTarget(Spring* s, float t);
bool springStep(Spring* s, float dt);

typedef struct {
    float progress, duration, delay, elapsed;
    bool active, finished;
    float (*easeFunc)(float);
} Transition;

Transition transStart(float duration, float delay);
float transGet(Transition* t);
float transUpdate(Transition* t, float dt);

typedef struct {
    float opacity, scale, slideX, slideY;
    Spring opacityS, scaleS, slideXS, slideYS;
    bool active;
} AnimState;

void animStateInit(AnimState* a);
void animStateFadeIn(AnimState* a);
void animStateFadeOut(AnimState* a);
void animStateSlideIn(AnimState* a, float fx, float fy);
void animStatePopIn(AnimState* a);
void animStateUpdate(AnimState* a, float dt);

void animInit(void);
void animUpdate(float dt);

extern float g_parallaxOffset;
extern float g_bootProgress;
extern bool g_bootComplete;
