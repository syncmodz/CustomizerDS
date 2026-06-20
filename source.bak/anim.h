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
} EaseType;

float easeFunc(float t, EaseType type);

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
void transitionUpdate(ScreenTransition* t);
bool transitionActive(ScreenTransition* t);
float transitionValue(ScreenTransition* t);
float transitionEased(ScreenTransition* t, EaseType ease);

float springTo(float current, float target, float* velocity, float stiffness, float damping, float dt);
float approach(float current, float target, float maxStep);

#endif
