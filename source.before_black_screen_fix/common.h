#ifndef COMMON_H
#define COMMON_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stdint.h>

#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240
#define SCREEN_BOT_WIDTH 320
#define SCREEN_BOT_HEIGHT 240

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

static inline bool touchIn(touchPosition* touch, float x, float y, float w, float h) {
    if (!touch) return false;
    return touch->px >= x && touch->px < x + w && touch->py >= y && touch->py < y + h;
}

#endif
