#ifndef COMMON_H
#define COMMON_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240
#define SCREEN_BOT_WIDTH 320
#define SCREEN_BOT_HEIGHT 240

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_TAU
#define M_TAU (M_PI * 2.0f)
#endif

#define MOTION_FAST 10
#define MOTION_MED 18
#define MOTION_SLOW 28

#define SAVE_DEBOUNCE_FRAMES 18

typedef enum {
    SCREEN_MAIN_MENU = 0,
    SCREEN_FONTS,
    SCREEN_DARKMODE,
    SCREEN_LED
} ScreenType;

static inline float clampf(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static inline int clampi(int v, int min, int max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline float invlerpf(float a, float b, float v) {
    float denom = b - a;
    if (fabsf(denom) < 0.0001f) return 0.0f;
    return (v - a) / denom;
}

static inline bool touchIn(touchPosition* touch, float x, float y, float w, float h) {
    if (!touch) return false;
    return touch->px >= x && touch->px < x + w && touch->py >= y && touch->py < y + h;
}

static inline float easeOutCubic(float t) {
    if (t >= 1.0f) return 1.0f;
    if (t <= 0.0f) return 0.0f;
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

static inline float easeInOutCubic(float t) {
    if (t >= 1.0f) return 1.0f;
    if (t <= 0.0f) return 0.0f;
    if (t < 0.5f) return 4.0f * t * t * t;
    float tn = -2.0f * t + 2.0f;
    return 1.0f - (tn * tn * tn) / 2.0f;
}

static inline float easeOutBack(float t) {
    if (t >= 1.0f) return 1.0f;
    if (t <= 0.0f) return 0.0f;
    float c1 = 1.70158f;
    float c3 = c1 + 1.0f;
    return 1.0f + c3 * (t - 1.0f) * (t - 1.0f) * (t - 1.0f) + c1 * (t - 1.0f) * (t - 1.0f);
}

static inline float animApproach(float current, float target, float speed) {
    if (speed <= 0.0f) return target;
    float diff = target - current;
    if (fabsf(diff) < 0.5f) return target;
    return current + diff * speed;
}

static inline float animPulse(float frame, float speed, float offset) {
    return (sinf((frame + offset) * speed) + 1.0f) * 0.5f;
}

#endif
