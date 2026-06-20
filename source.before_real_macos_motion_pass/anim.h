#ifndef ANIM_H
#define ANIM_H

#include <stdbool.h>
#include <math.h>

extern float EASE_OUT[256];
extern float EASE_IN_OUT[256];

typedef struct {
    float value;
    float target;
    float speed;
    bool  finished;
} Anim;

void animInit(void);
void animStep(Anim* a);
float animEasedOut(Anim* a);
float animEasedInOut(Anim* a);
void animTo(Anim* a, float target);
void animSet(Anim* a, float value, float speed);

float animApproach(float current, float target, float speed);
float animPulse(float time, float speed);
float easeOutCubic(float t);
float easeInOutCubic(float t);

#endif
