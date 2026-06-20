#include "ui.h"
#include "common.h"
#include "anim.h"
#include <math.h>
#include <stdio.h>

static float s_frame = 0.0f;
float g_enterT = 1.0f;

void uiFrameTick(void) {
    s_frame += 1.0f / 60.0f;
    if (s_frame > 1024.0f) s_frame = 0.0f;
    if (g_enterT < 1.0f) g_enterT = animApproach(g_enterT, 1.0f, 0.12f);
}

float uiFrameTime(void) {
    return s_frame;
}

void uiScreenEnter(void) {
    g_enterT = 0.0f;
}

void UI_Fill(float x, float y, float w, float h, ColorRGBA color) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x, y, 0.0f, w, h, themeColor(color));
}

void UI_Line(float x, float y, float w, float h, ColorRGBA color) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x, y, 0.0f, w, h, themeColor(color));
}

void UI_RoundRect(float x, float y, float w, float h, float r, ColorRGBA color) {
    if (w <= 0 || h <= 0) return;
    int ri = (int)(r + 0.5f);
    if (ri < 1) ri = 1;
    if (ri * 2 >= w || ri * 2 >= h) ri = (int)(fminf(w, h) * 0.25f);
    if (ri < 1) ri = 1;
    u32 c = themeColor(color);
    C2D_DrawRectSolid(x + ri, y, 0, w - 2*ri, h, c);
    C2D_DrawRectSolid(x, y + ri, 0, w, h - 2*ri, c);
    for (int i = 0; i < ri && i < h; i++) {
        int fill = ri - i;
        if (fill < 1) fill = 1;
        if (fill > w) fill = (int)w;
        C2D_DrawRectSolid(x, y + i, 0, fill, 1, c);
        C2D_DrawRectSolid(x + w - fill, y + i, 0, fill, 1, c);
        C2D_DrawRectSolid(x, y + h - 1 - i, 0, fill, 1, c);
        C2D_DrawRectSolid(x + w - fill, y + h - 1 - i, 0, fill, 1, c);
    }
}

void UI_RoundPanel(float x, float y, float w, float h, float r, ColorRGBA bg, ColorRGBA border) {
    if (w <= 0 || h <= 0) return;
    UI_RoundRect(x, y, w, h, r, bg);
    C2D_DrawRectSolid(x + r, y, 0, w - 2*r, 1, themeColor(border));
    C2D_DrawRectSolid(x + r, y + h - 1, 0, w - 2*r, 1, themeColor(border));
    C2D_DrawRectSolid(x, y + r, 0, 1, h - 2*r, themeColor(border));
    C2D_DrawRectSolid(x + w - 1, y + r, 0, 1, h - 2*r, themeColor(border));
}

void UI_Shadow(float x, float y, float w, float h) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x + 2, y + 2, 0, w, h, themeColor((ColorRGBA){0, 0, 0, 32}));
    C2D_DrawRectSolid(x + 3, y + 3, 0, w, h, themeColor((ColorRGBA){0, 0, 0, 12}));
}

void UI_Panel(float x, float y, float w, float h, ColorRGBA bg, ColorRGBA border) {
    UI_RoundPanel(x, y, w, h, 6, bg, border);
}

void UI_AccentBar(float x, float y, float w, float h) {
    UI_Fill(x, y, w, h, g_theme.accent);
}

static void parseText(C2D_Text* out, C2D_TextBuf buf, C2D_Font font, const char* text) {
    if (font) {
        C2D_TextFontParse(out, font, buf, text ? text : "");
    } else {
        C2D_TextParse(out, buf, text ? text : "");
    }
    C2D_TextOptimize(out);
}

void UI_Text(C2D_TextBuf buf, C2D_Font font, const char* text,
             float x, float y, float sx, float sy, ColorRGBA color) {
    C2D_Text t;
    parseText(&t, buf, font, text);
    C2D_DrawText(&t, C2D_WithColor, x, y, 0.0f, sx, sy, themeColor(color));
}

void UI_TextRight(C2D_TextBuf buf, C2D_Font font, const char* text,
                  float right, float y, float sx, float sy, ColorRGBA color) {
    C2D_Text t;
    parseText(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sx, sy, &tw, NULL);
    C2D_DrawText(&t, C2D_WithColor, right - tw, y, 0.0f, sx, sy, themeColor(color));
}

void UI_TextCenter(C2D_TextBuf buf, C2D_Font font, const char* text,
                   float centerX, float y, float sx, float sy, ColorRGBA color) {
    C2D_Text t;
    parseText(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sx, sy, &tw, NULL);
    C2D_DrawText(&t, C2D_WithColor, centerX - tw * 0.5f, y, 0.0f, sx, sy, themeColor(color));
}

void UI_HeaderTop(C2D_TextBuf buf, C2D_Font font, const char* title, const char* subtitle) {
    UI_BackgroundTop();
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, 38, g_theme.primaryDark);
    UI_RoundRect(190, 2, SCREEN_TOP_WIDTH - 194, 34, 6, g_theme.primary);
    UI_AccentBar(0, 37, SCREEN_TOP_WIDTH, 2);
    UI_Text(buf, font, title, 14, 6, 0.54f, 0.54f, g_theme.onPrimary);
    if (subtitle) {
        UI_Text(buf, NULL, subtitle, 16, 25, 0.22f, 0.22f, g_theme.textSecondary);
    }
}

void UI_TouchChrome(C2D_TextBuf buf, C2D_Font font, const char* title, const char* hint) {
    UI_BackgroundBottom();
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, 28, g_theme.surface);
    UI_AccentBar(0, 27, SCREEN_BOT_WIDTH, 2);
    UI_Text(buf, font, title, 10, 6, 0.34f, 0.34f, g_theme.textPrimary);
    if (hint) {
        UI_TextRight(buf, NULL, hint, 312, 9, 0.20f, 0.20f, g_theme.textHint);
    }
}

void UI_Button(C2D_TextBuf buf, C2D_Font font, float x, float y, float w, float h,
               const char* label, const char* value, UIButtonState state) {
    ColorRGBA bg = g_theme.surface;
    ColorRGBA border = g_theme.divider;
    ColorRGBA labelColor = g_theme.textPrimary;
    if (state == UI_BUTTON_SELECTED) {
        float pulse = sinf(uiFrameTime() * 4.0f) * 0.10f;
        bg = themeMix(g_theme.surface, g_theme.primary, 0.28f + pulse);
        border = g_theme.accent;
        labelColor = g_theme.onPrimary;
    } else if (state == UI_BUTTON_ACTIVE) {
        bg = themeMix(g_theme.surface, g_theme.accent, 0.34f);
        border = g_theme.accent;
    }
    UI_Shadow(x, y, w, h);
    UI_RoundPanel(x, y, w, h, 6, bg, border);
    if (state != UI_BUTTON_NORMAL) {
        UI_AccentBar(x + 2, y + 2, 2, h - 4);
    }
    UI_Text(buf, font, label, x + 10, y + 7, 0.31f, 0.31f, labelColor);
    if (value) {
        UI_TextRight(buf, NULL, value, x + w - 8, y + 10, 0.24f, 0.24f, g_theme.textSecondary);
    }
}

void UI_Slider(C2D_TextBuf buf, C2D_Font font, float x, float y, float w,
               const char* label, int value, int min, int max, bool selected) {
    char valueText[12];
    snprintf(valueText, sizeof(valueText), "%d", value);
    UI_Button(buf, font, x, y, w, 32, label, valueText, selected ? UI_BUTTON_SELECTED : UI_BUTTON_NORMAL);
    float barX = x + (w < 180.0f ? 62.0f : 90.0f);
    float barY = y + 23;
    float barW = w - (w < 180.0f ? 102.0f : 138.0f);
    float t = (max > min) ? (float)(value - min) / (float)(max - min) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);
    UI_Fill(barX, barY, barW, 3, g_theme.divider);
    UI_Fill(barX, barY, barW * t, 3, g_theme.accent);
    UI_Fill(barX + barW * t - 2, barY - 3, 5, 9, g_theme.onPrimary);
}

void UI_Swatch(float x, float y, float w, float h, ColorRGBA color, bool selected) {
    UI_RoundRect(x, y, w, h, 4, color);
    if (selected) {
        UI_RoundRect(x - 2, y - 2, w + 4, h + 4, 6, g_theme.accent);
        UI_RoundRect(x, y, w, h, 4, color);
    } else {
        UI_RoundPanel(x, y, w, h, 4, color, g_theme.divider);
    }
}

void UI_KeyHelp(C2D_TextBuf buf, C2D_Font font, const char* left, const char* right) {
    UI_Fill(0, SCREEN_BOT_HEIGHT - 22, SCREEN_BOT_WIDTH, 22, g_theme.surface);
    if (left) UI_Text(buf, font, left, 9, SCREEN_BOT_HEIGHT - 16, 0.20f, 0.20f, g_theme.textSecondary);
    if (right) UI_TextRight(buf, font, right, SCREEN_BOT_WIDTH - 9, SCREEN_BOT_HEIGHT - 16, 0.20f, 0.20f, g_theme.textSecondary);
}

void UI_DimmedGrid(float screenW, float screenH) {
    ColorRGBA line = g_theme.divider;
    line.a = 70;
    for (int x = 0; x < (int)screenW; x += 20) UI_Line((float)x, 0, 1, screenH, line);
    for (int y = 0; y < (int)screenH; y += 20) UI_Line(0, (float)y, screenW, 1, line);
}

void UI_BackgroundTop(void) {
    ColorRGBA c1 = g_theme.backgroundTop;
    ColorRGBA c2 = themeMix(g_theme.backgroundTop, g_theme.surface, 0.15f);
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, 60, c1);
    UI_Fill(0, 60, SCREEN_TOP_WIDTH, 90, c2);
    UI_Fill(0, 150, SCREEN_TOP_WIDTH, 90, g_theme.backgroundTop);
}

void UI_BackgroundBottom(void) {
    ColorRGBA c1 = g_theme.background;
    ColorRGBA c2 = themeMix(g_theme.background, g_theme.surface, 0.08f);
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, 60, c1);
    UI_Fill(0, 60, SCREEN_BOT_WIDTH, 180, c2);
}

void UI_Card(float x, float y, float w, float h, float r, ColorRGBA bg) {
    if (w <= 0 || h <= 0) return;
    UI_Shadow(x, y, w, h);
    UI_RoundRect(x, y, w, h, r, bg);
    ColorRGBA glass = {255, 255, 255, 18};
    UI_RoundRect(x + 1, y + 1, w - 2, h * 0.45f, r, glass);
}

void UI_Pill(float x, float y, float w, float h, ColorRGBA bg, ColorRGBA textColor,
             C2D_TextBuf buf, C2D_Font font, const char* label, float sx, float sy) {
    UI_RoundRect(x, y, w, h, h * 0.5f, bg);
    UI_TextCenter(buf, font, label, x + w * 0.5f, y + (h - sy * 16) * 0.5f, sx, sy, textColor);
}

void UI_TouchButton(C2D_TextBuf buf, C2D_Font font, float x, float y, float w, float h,
                    const char* label, const char* prefix, const char* value,
                    UIButtonState state, float animT) {
    ColorRGBA bg = g_theme.surfaceElevated;
    ColorRGBA labelColor = g_theme.textPrimary;
    float ofs = 0.0f;
    if (state == UI_BUTTON_SELECTED) {
        float pulse = sinf(uiFrameTime() * 3.5f) * 0.06f;
        bg = themeMix(g_theme.surfaceElevated, g_theme.accent, 0.20f + pulse);
        labelColor = g_theme.onPrimary;
        ofs = -1.0f;
    } else if (state == UI_BUTTON_ACTIVE) {
        bg = themeMix(g_theme.surfaceElevated, g_theme.accent, 0.15f);
    }
    float slideX = 0.0f;
    if (animT < 1.0f) slideX = (1.0f - easeOutCubic(animT)) * 12.0f;
    UI_Shadow(x + slideX, y + ofs, w, h);
    UI_RoundRect(x + slideX, y + ofs, w, h, 9, bg);
    ColorRGBA border = {255, 255, 255, 18};
    UI_RoundRect(x + slideX, y + ofs, w, h, 9, border);
    if (prefix) {
        UI_Pill(x + slideX + 8, y + ofs + (h - 22) * 0.5f, 34, 22,
                themeMix(bg, g_theme.textPrimary, 0.2f), g_theme.textSecondary,
                buf, font, prefix, 0.30f, 0.30f);
        UI_Text(buf, font, label, x + slideX + 50, y + ofs + (h - 16) * 0.5f - 2, 0.34f, 0.34f, labelColor);
    } else {
        UI_Text(buf, font, label, x + slideX + 14, y + ofs + (h - 16) * 0.5f - 2, 0.34f, 0.34f, labelColor);
    }
    if (value) {
        UI_TextRight(buf, NULL, value, x + slideX + w - 12, y + ofs + (h - 16) * 0.5f - 2, 0.26f, 0.26f, g_theme.textSecondary);
    }
}

void UI_SegmentedControl(C2D_TextBuf buf, C2D_Font font, float x, float y, float w, float h,
                         const char** labels, int count, int selected, float slideT) {
    float segW = w / (float)count;
    UI_RoundRect(x, y, w, h, h * 0.5f, g_theme.surface);
    float targetX = x + selected * segW + 2;
    float slideX = targetX;
    if (slideT < 1.0f) {
        static float s_currentX = 0.0f;
        if (s_currentX == 0.0f) { s_currentX = targetX; }
        s_currentX = animApproach(s_currentX, targetX, 0.18f);
        slideX = s_currentX;
    } else {
        slideX = targetX;
    }
    UI_RoundRect(slideX, y + 2, segW - 4, h - 4, h * 0.5f, g_theme.accent);
    for (int i = 0; i < count; i++) {
        ColorRGBA c = (i == selected) ? g_theme.onPrimary : g_theme.textPrimary;
        UI_TextCenter(buf, font, labels[i], x + segW * i + segW * 0.5f, y + (h - 16) * 0.5f, 0.28f, 0.28f, c);
    }
}

void UI_RoundSwatch(float x, float y, float r, ColorRGBA color, bool selected, float pulseT) {
    float s = r * 2.0f;
    if (selected) {
        float glow = pulseT * 0.3f;
        ColorRGBA ring = themeMix(color, g_theme.onPrimary, glow);
        UI_RoundRect(x - 3, y - 3, s + 6, s + 6, r + 3, ring);
    }
    UI_RoundRect(x, y, s, s, r, color);
    if (selected) {
        ColorRGBA shine = {255, 255, 255, 40};
        UI_RoundRect(x + 2, y + 2, s - 4, s * 0.4f, r - 2, shine);
    }
}

void UI_SliderModern(C2D_TextBuf buf, C2D_Font font, float x, float y, float w,
                     const char* label, int value, int min, int max, bool selected, float animT) {
    ColorRGBA bg = selected ? themeMix(g_theme.surface, g_theme.accent, 0.10f) : g_theme.surface;
    UI_RoundRect(x, y, w, 36, 8, bg);
    UI_Text(buf, font, label, x + 12, y + 9, 0.28f, 0.28f, g_theme.textPrimary);
    float barX = x + 76;
    float barY = y + 16;
    float barW = w - 92;
    float t = (max > min) ? (float)(value - min) / (float)(max - min) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);
    float knobX = barX + barW * t;
    UI_RoundRect(barX, barY, barW, 4, 2, g_theme.divider);
    UI_RoundRect(barX, barY, (knobX - barX), 4, 2, g_theme.accent);
    float knobPulse = selected ? sinf(uiFrameTime() * 3.0f) * 2.0f : 0.0f;
    UI_Shadow(knobX - 5, barY - 5 + knobPulse, 10, 10);
    UI_RoundRect(knobX - 5, barY - 5 + knobPulse, 10, 10, 5, selected ? g_theme.onPrimary : g_theme.textPrimary);
    if (selected) {
        UI_RoundRect(knobX - 3, barY - 3 + knobPulse, 6, 6, 3, g_theme.accent);
    }
    char valStr[8];
    snprintf(valStr, sizeof(valStr), "%d", value);
    UI_TextRight(buf, NULL, valStr, x + w - 10, y + 9, 0.24f, 0.24f, g_theme.textSecondary);
}

void UI_Badge(C2D_TextBuf buf, C2D_Font font, float x, float y, const char* text, ColorRGBA color) {
    float tw = 0.0f;
    C2D_Text tmp;
    parseText(&tmp, buf, NULL, text);
    C2D_TextGetDimensions(&tmp, 0.22f, 0.22f, &tw, NULL);
    float pw = tw + 14.0f;
    UI_RoundRect(x, y, pw, 18, 9, color);
    UI_TextCenter(buf, NULL, text, x + pw * 0.5f, y + 2, 0.22f, 0.22f, g_theme.onPrimary);
}
