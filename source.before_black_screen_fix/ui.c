#include "ui.h"
#include "common.h"
#include <math.h>
#include <stdio.h>

static float s_frame = 0.0f;

void uiFrameTick(void) {
    s_frame += 1.0f / 60.0f;
    if (s_frame > 1024.0f) s_frame = 0.0f;
}

float uiFrameTime(void) {
    return s_frame;
}

void UI_Fill(float x, float y, float w, float h, ColorRGBA color) {
    C2D_DrawRectSolid(x, y, 0.0f, w, h, themeColor(color));
}

void UI_Line(float x, float y, float w, float h, ColorRGBA color) {
    C2D_DrawRectSolid(x, y, 0.0f, w, h, themeColor(color));
}

void UI_RoundRect(float x, float y, float w, float h, float r, ColorRGBA color) {
    int ri = (int)(r + 0.5f);
    if (ri < 1) ri = 1;
    if (ri * 2 > w || ri * 2 > h) ri = (int)(fminf(w, h) * 0.25f);
    if (ri < 1) ri = 1;
    u32 c = themeColor(color);
    C2D_DrawRectSolid(x + ri, y, 0, w - 2*ri, h, c);
    C2D_DrawRectSolid(x, y + ri, 0, w, h - 2*ri, c);
    for (int i = 0; i < ri; i++) {
        int fill = ri - i;
        C2D_DrawRectSolid(x, y + i, 0, fill, 1, c);
        C2D_DrawRectSolid(x + w - fill, y + i, 0, fill, 1, c);
        C2D_DrawRectSolid(x, y + h - 1 - i, 0, fill, 1, c);
        C2D_DrawRectSolid(x + w - fill, y + h - 1 - i, 0, fill, 1, c);
    }
}

void UI_RoundPanel(float x, float y, float w, float h, float r, ColorRGBA bg, ColorRGBA border) {
    UI_RoundRect(x, y, w, h, r, bg);
    C2D_DrawRectSolid(x + r, y, 0, w - 2*r, 1, themeColor(border));
    C2D_DrawRectSolid(x + r, y + h - 1, 0, w - 2*r, 1, themeColor(border));
    C2D_DrawRectSolid(x, y + r, 0, 1, h - 2*r, themeColor(border));
    C2D_DrawRectSolid(x + w - 1, y + r, 0, 1, h - 2*r, themeColor(border));
}

void UI_Shadow(float x, float y, float w, float h) {
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
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, 42, g_theme.primaryDark);
    UI_RoundRect(196, 2, SCREEN_TOP_WIDTH - 198, 38, 6, g_theme.primary);
    UI_AccentBar(0, 41, SCREEN_TOP_WIDTH, 3);
    UI_Text(buf, font, title, 14, 7, 0.58f, 0.58f, g_theme.onPrimary);
    if (subtitle) {
        UI_Text(buf, NULL, subtitle, 16, 29, 0.25f, 0.25f, g_theme.textSecondary);
    }
}

void UI_TouchChrome(C2D_TextBuf buf, C2D_Font font, const char* title, const char* hint) {
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, 30, g_theme.surface);
    UI_AccentBar(0, 29, SCREEN_BOT_WIDTH, 2);
    UI_Text(buf, font, title, 10, 7, 0.36f, 0.36f, g_theme.textPrimary);
    if (hint) {
        UI_TextRight(buf, NULL, hint, 312, 10, 0.22f, 0.22f, g_theme.textHint);
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
    UI_Panel(x, y, w, h, color, selected ? g_theme.onPrimary : g_theme.divider);
    if (selected) {
        UI_AccentBar(x + 2, y + h - 4, w - 4, 2);
    }
}

void UI_KeyHelp(C2D_TextBuf buf, C2D_Font font, const char* left, const char* right) {
    UI_Fill(0, SCREEN_BOT_HEIGHT - 24, SCREEN_BOT_WIDTH, 24, g_theme.surface);
    if (left) UI_Text(buf, font, left, 9, SCREEN_BOT_HEIGHT - 18, 0.23f, 0.23f, g_theme.textSecondary);
    if (right) UI_TextRight(buf, font, right, SCREEN_BOT_WIDTH - 9, SCREEN_BOT_HEIGHT - 18, 0.23f, 0.23f, g_theme.textSecondary);
}

void UI_DimmedGrid(float screenW, float screenH) {
    ColorRGBA line = g_theme.divider;
    line.a = 70;
    for (int x = 0; x < (int)screenW; x += 20) UI_Line((float)x, 0, 1, screenH, line);
    for (int y = 0; y < (int)screenH; y += 20) UI_Line(0, (float)y, screenW, 1, line);
}
