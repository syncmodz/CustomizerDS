#ifndef COMMON_H
#define COMMON_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stdint.h>

// Screen dimensions
#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240
#define SCREEN_BOT_WIDTH 320
#define SCREEN_BOT_HEIGHT 240

// Screen types
typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_ASSETS,
    SCREEN_FONTS,
    SCREEN_DARKMODE,
    SCREEN_LED
} ScreenType;

// Utility functions
static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

#endif
