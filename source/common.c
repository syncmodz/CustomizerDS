#include "common.h"

void drawRoundedRect(int x, int y, int w, int h, int r, u32 color) {
    if (r <= 0) { C2D_DrawRectSolid(x, y, 0.5f, w, h, color); return; }
    C2D_DrawRectSolid(x + r, y, 0.5f, w - 2*r, h, color);
    C2D_DrawRectSolid(x, y + r, 0.5f, w, h - 2*r, color);
    C2D_DrawCircleSolid(x + r, y + r, 0.5f, r, color);
    C2D_DrawCircleSolid(x + w - r - 1, y + r, 0.5f, r, color);
    C2D_DrawCircleSolid(x + r, y + h - r - 1, 0.5f, r, color);
    C2D_DrawCircleSolid(x + w - r - 1, y + h - r - 1, 0.5f, r, color);
}

void drawGradientRect(int x, int y, int w, int h, u32 c1, u32 c2, bool vert) {
    for (int i = 0; i < (vert ? h : w); i++) {
        float t = i / (float)((vert ? h : w) - 1);
        u32 c = blendColors(c1, c2, t);
        if (vert) C2D_DrawRectSolid(x, y + i, 0.5f, w, 1, c);
        else C2D_DrawRectSolid(x + i, y, 0.5f, 1, h, c);
    }
}

void drawText(C2D_TextBuf buf, C2D_Font font, const char* text, int x, int y, float sx, float sy, u32 color) {
    if (!text || !buf) return;
    C2D_Text c2dText;
    C2D_TextParse(&c2dText, buf, text);
    C2D_TextOptimize(&c2dText);
    C2D_DrawText(&c2dText, C2D_WithColor, x, y, 0.5f, sx, sy, color);
}

void drawTextCentered(C2D_TextBuf buf, C2D_Font font, const char* text, int x, int y, float sx, float sy, u32 color) {
    if (!text || !buf) return;
    C2D_Text c2dText;
    C2D_TextParse(&c2dText, buf, text);
    C2D_TextOptimize(&c2dText);
    float w = 0;
    C2D_TextGetDimensions(&c2dText, sx, sy, &w, NULL);
    C2D_DrawText(&c2dText, C2D_WithColor, x - w/2, y, 0.5f, sx, sy, color);
}

void drawTextRight(C2D_TextBuf buf, C2D_Font font, const char* text, int x, int y, float sx, float sy, u32 color) {
    if (!text || !buf) return;
    C2D_Text c2dText;
    C2D_TextParse(&c2dText, buf, text);
    C2D_TextOptimize(&c2dText);
    float w = 0;
    C2D_TextGetDimensions(&c2dText, sx, sy, &w, NULL);
    C2D_DrawText(&c2dText, C2D_WithColor, x - w, y, 0.5f, sx, sy, color);
}

float textGetWidth(C2D_Font font, const char* text, float scale) {
    (void)font;
    if (!text) return 0;
    return strlen(text) * 10 * scale;
}

float appGetTime(void) { return osGetTime() / 1000.0f; }
