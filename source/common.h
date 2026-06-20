#pragma once
#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SCREEN_TOP_W 400
#define SCREEN_TOP_H 240
#define SCREEN_BOT_W 320
#define SCREEN_BOT_H 240

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static inline float clampf(float v, float mn, float mx) {
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}

static inline int clampi(int v, int mn, int mx) {
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}

static inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline float smoothstepf(float t) {
    return t * t * (3.0f - 2.0f * t);
}

static inline float invlerpf(float a, float b, float v) {
    float d = b - a;
    if (fabsf(d) < 0.0001f) return 0;
    return (v - a) / d;
}

static inline u32 rgba8(u8 r, u8 g, u8 b, u8 a) {
    return C2D_Color32(r, g, b, a);
}

static inline bool touchHit(int tx, int ty, int x, int y, int w, int h) {
    return tx >= x && tx < x + w && ty >= y && ty < y + h;
}

static inline float easeOutCubic(float t) {
    if (t <= 0) return 0;
    if (t >= 1) return 1;
    return 1 - (1 - t) * (1 - t) * (1 - t);
}

static inline float easeInOutCubic(float t) {
    if (t <= 0) return 0;
    if (t >= 1) return 1;
    if (t < 0.5f) return 4 * t * t * t;
    float n = -2 * t + 2;
    return 1 - (n * n * n) / 2;
}

static inline float easeOutBack(float t) {
    if (t <= 0) return 0;
    if (t >= 1) return 1;
    float c1 = 1.70158f, c3 = c1 + 1;
    return 1 + c3 * (t - 1) * (t - 1) * (t - 1) + c1 * (t - 1) * (t - 1);
}

static inline float easeOutElastic(float t) {
    if (t <= 0) return 0;
    if (t >= 1) return 1;
    float c4 = (2 * M_PI) / 3;
    return powf(2, -10 * t) * sinf((t * 10 - 0.75f) * c4) + 1;
}

static inline float easeOutBounce(float t) {
    float n1 = 7.5625f, d1 = 2.75f;
    if (t < 1 / d1) return n1 * t * t;
    else if (t < 2 / d1) { t -= 1.5f / d1; return n1 * t * t + 0.75f; }
    else if (t < 2.5f / d1) { t -= 2.25f / d1; return n1 * t * t + 0.9375f; }
    else { t -= 2.625f / d1; return n1 * t * t + 0.984375f; }
}

static inline float animPulse(float time, float speed, float offset) {
    return (sinf((time + offset) * speed) + 1) * 0.5f;
}

static inline u32 blendColors(u32 a, u32 b, float t) {
    t = clampf(t, 0, 1);
    u8 ar = (a>>24)&0xFF, ag = (a>>16)&0xFF, ab = (a>>8)&0xFF, aa = a&0xFF;
    u8 br = (b>>24)&0xFF, bg = (b>>16)&0xFF, bb = (b>>8)&0xFF, ba = b&0xFF;
    return rgba8((u8)lerpf(ar,br,t), (u8)lerpf(ag,bg,t), (u8)lerpf(ab,bb,t), (u8)lerpf(aa,ba,t));
}

static inline u32 darkenColor(u32 color, float amount) {
    u8 r=(color>>24)&0xFF,g=(color>>16)&0xFF,b=(color>>8)&0xFF,a=color&0xFF;
    amount = clampf(amount, 0, 1);
    return rgba8((u8)(r*(1-amount)),(u8)(g*(1-amount)),(u8)(b*(1-amount)),a);
}

static inline u32 lightenColor(u32 color, float amount) {
    u8 r=(color>>24)&0xFF,g=(color>>16)&0xFF,b=(color>>8)&0xFF,a=color&0xFF;
    amount = clampf(amount, 0, 1);
    return rgba8((u8)(r+(255-r)*amount),(u8)(g+(255-g)*amount),(u8)(b+(255-b)*amount),a);
}

static inline u32 colorWithAlpha(u32 color, u8 alpha) {
    return (color & 0xFFFFFF00) | alpha;
}

static inline u32 pickContrastColor(u32 bg) {
    u8 r=(bg>>24)&0xFF,g=(bg>>16)&0xFF,b=(bg>>8)&0xFF;
    float l = 0.2126f*(r/255.0f)+0.7152f*(g/255.0f)+0.0722f*(b/255.0f);
    return l > 0.5f ? rgba8(29,29,31,255) : rgba8(245,245,247,255);
}

void drawRoundedRect(int x, int y, int w, int h, int r, u32 color);
void drawGradientRect(int x, int y, int w, int h, u32 c1, u32 c2, bool vert);
void drawText(C2D_TextBuf buf, C2D_Font font, const char* text, int x, int y, float sx, float sy, u32 color);
void drawTextCentered(C2D_TextBuf buf, C2D_Font font, const char* text, int x, int y, float sx, float sy, u32 color);
void drawTextRight(C2D_TextBuf buf, C2D_Font font, const char* text, int x, int y, float sx, float sy, u32 color);
float textGetWidth(C2D_Font font, const char* text, float scale);
float appGetTime(void);
