#include "ui.h"
#include "common.h"
#include "anim.h"
#include <stdio.h>
#include <string.h>

static float s_frame = 0.0f;
float g_enterT = 1.0f;
float g_frame = 0.0f;

void uiFrameTick(void) {
    s_frame += 1.0f / 60.0f;
    if (s_frame > 1024.0f) s_frame = 0.0f;
    g_frame = s_frame;
    if (g_enterT < 1.0f) g_enterT = animApproach(g_enterT, 1.0f, 0.12f);
}

float uiFrameTime(void) {
    return s_frame;
}

void uiScreenEnter(void) {
    g_enterT = 0.0f;
}

static u32 toU32(ColorRGBA c) {
    return themeColor(c);
}

static int computeCornerFill(int row, int r) {
    if (row >= r) return 0;
    float dy = (float)(r - row);
    float offset = r - sqrtf((float)(r * r) - dy * dy);
    int fill = (int)(offset + 0.5f);
    if (fill < 0) fill = 0;
    if (fill > r) fill = r;
    return fill;
}

void UI_RoundRect(float x, float y, float w, float h, float r, ColorRGBA color) {
    if (w <= 1 || h <= 1) return;
    int ri = (int)fmaxf(r, 1.0f);
    if (ri * 2 >= w) ri = (int)(w * 0.5f);
    if (ri * 2 >= h) ri = (int)(h * 0.5f);
    if (ri < 1) { C2D_DrawRectSolid(x, y, 0.0f, w, h, toU32(color)); return; }
    if (ri > 16) ri = 16;

    u32 c = toU32(color);
    int maxRow = (ri < h / 2) ? ri : (int)(h / 2);

    for (int i = 0; i < maxRow; i++) {
        int fill = computeCornerFill(i, ri);
        int drawW = w - 2 * fill;
        if (drawW > 0) {
            C2D_DrawRectSolid(x + fill, y + i, 0.0f, drawW, 1, c);
            C2D_DrawRectSolid(x + fill, y + h - 1 - i, 0.0f, drawW, 1, c);
        }
    }
    int midH = h - 2 * ri;
    if (midH > 0) {
        C2D_DrawRectSolid(x, y + ri, 0.0f, w, midH, c);
    }
}

void UI_Fill(float x, float y, float w, float h, ColorRGBA color) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x, y, 0.0f, w, h, toU32(color));
}

void UI_Line(float x, float y, float w, float h, ColorRGBA color) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x, y, 0.0f, w, h, toU32(color));
}

void UI_Shadow(float x, float y, float w, float h) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x + 2, y + 2, 0.0f, w, h, toU32((ColorRGBA){0, 0, 0, 32}));
    C2D_DrawRectSolid(x + 3, y + 3, 0.0f, w, h, toU32((ColorRGBA){0, 0, 0, 12}));
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
    C2D_DrawText(&t, C2D_WithColor, x, y, 0.0f, sx, sy, toU32(color));
}

void UI_TextRight(C2D_TextBuf buf, C2D_Font font, const char* text,
                  float right, float y, float sx, float sy, ColorRGBA color) {
    C2D_Text t;
    parseText(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sx, sy, &tw, NULL);
    C2D_DrawText(&t, C2D_WithColor, right - tw, y, 0.0f, sx, sy, toU32(color));
}

void UI_TextCenter(C2D_TextBuf buf, C2D_Font font, const char* text,
                   float centerX, float y, float sx, float sy, ColorRGBA color) {
    C2D_Text t;
    parseText(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sx, sy, &tw, NULL);
    C2D_DrawText(&t, C2D_WithColor, centerX - tw * 0.5f, y, 0.0f, sx, sy, toU32(color));
}

void UI_BackgroundTop(void) {
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, g_theme.backgroundTop);
    ColorRGBA divider = {255, 255, 255, 10};
    UI_Line(0, SCREEN_TOP_HEIGHT - 1, SCREEN_TOP_WIDTH, 1, divider);
}

void UI_BackgroundBottom(void) {
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, g_theme.background);
    ColorRGBA topGlow = {255, 255, 255, 6};
    UI_Line(0, 0, SCREEN_BOT_WIDTH, 1, topGlow);
}

void UI_TouchBarBackground(void) {
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, (ColorRGBA){4, 5, 7, 255});
    ColorRGBA glow = {255, 255, 255, 10};
    UI_Line(0, 0, SCREEN_BOT_WIDTH, 1, glow);
}

void UI_GlassPanel(float x, float y, float w, float h, float r, ColorRGBA bg, ColorRGBA border, ColorRGBA highlight) {
    if (w <= 0 || h <= 0) return;
    UI_Shadow(x, y, w, h);
    UI_RoundRect(x, y, w, h, r, bg);
    if (border.a > 0) {
        UI_RoundRect(x, y, w, h, r, border);
    }
    if (highlight.a > 0) {
        float hh = h * 0.35f;
        if (hh > 40) hh = 40;
        UI_RoundRect(x + 1, y + 1, w - 2, hh, r, highlight);
    }
}

void UI_CapsuleButton(C2D_TextBuf buf, C2D_Font font, float x, float y, float w, float h,
                      const char* label, const char* prefix, const char* value,
                      UIButtonState state, float pressedT, float appearT) {
    float ap = (appearT < 1.0f) ? easeOutCubic(appearT) : 1.0f;
    float slideIn = (1.0f - ap) * 16.0f;
    float ox = x + slideIn;
    float oy = y + pressedT * 1.0f;

    ColorRGBA bg = {24, 28, 38, 245};
    ColorRGBA labelColor = g_theme.textPrimary;
    ColorRGBA borderCol = {255, 255, 255, 14};

    if (state == UI_BUTTON_SELECTED) {
        float pulse = sinf(s_frame * 3.5f) * 0.04f;
        float amt = 0.22f + pulse - pressedT * 0.08f;
        bg = themeMix((ColorRGBA){24, 28, 38, 245}, g_theme.accent, amt);
        labelColor = g_theme.onPrimary;
        borderCol = g_theme.accent;
        borderCol.a = 60;
    } else if (state == UI_BUTTON_ACTIVE) {
        bg = themeMix((ColorRGBA){24, 28, 38, 245}, g_theme.accent, 0.12f);
        borderCol.a = 30;
    }

    float radius = (h <= 34) ? h * 0.5f : 16.0f;

    if (ap < 1.0f) {
        float scaleW = lerpf(0.92f, 1.0f, ap);
        float scaleH = lerpf(0.88f, 1.0f, ap);
        float cw = w * scaleW;
        float ch = h * scaleH;
        float cx = ox + (w - cw) * 0.5f;
        float cy = oy + (h - ch) * 0.5f;
        UI_RoundRect(cx, cy, cw, ch, radius, bg);
        UI_RoundRect(cx, cy, cw, ch, radius, borderCol);
        UI_TextCenter(buf, font, label, cx + cw * 0.5f, cy + (ch - 16) * 0.5f, 0.34f, 0.34f, labelColor);
        return;
    }

    UI_Shadow(ox + 1, oy, w, h);
    UI_RoundRect(ox, oy, w, h, radius, bg);
    UI_RoundRect(ox, oy, w, h, radius, borderCol);

    if (prefix) {
        float ph = h - 8;
        float px = ox + 6;
        float py = oy + 4;
        UI_RoundRect(px, py, 36, ph, ph * 0.5f, themeMix(bg, g_theme.textPrimary, 0.18f));
        UI_TextCenter(buf, font, prefix, px + 18, py + (ph - 14) * 0.5f, 0.30f, 0.30f, g_theme.textSecondary);
        UI_Text(buf, font, label, ox + 50, oy + (h - 16) * 0.5f, 0.34f, 0.34f, labelColor);
    } else {
        UI_Text(buf, font, label, ox + 14, oy + (h - 16) * 0.5f, 0.34f, 0.34f, labelColor);
    }
    if (value) {
        UI_TextRight(buf, NULL, value, ox + w - 12, oy + (h - 16) * 0.5f, 0.26f, 0.26f, g_theme.textSecondary);
    }
}

void UI_TouchBarPill(float x, float y, float w, float h, ColorRGBA bg, const char* label,
                     C2D_TextBuf buf, C2D_Font font, float sx, float sy) {
    UI_RoundRect(x, y, w, h, h * 0.5f, bg);
    UI_TextCenter(buf, font, label, x + w * 0.5f, y + (h - sy * 16.0f) * 0.5f, sx, sy, g_theme.textPrimary);
}

void UI_Pill(float x, float y, float w, float h, ColorRGBA bg, const char* label,
             C2D_TextBuf buf, C2D_Font font, float sx, float sy) {
    UI_RoundRect(x, y, w, h, h * 0.5f, bg);
    UI_TextCenter(buf, font, label, x + w * 0.5f, y + (h - sy * 16.0f) * 0.5f, sx, sy, g_theme.onPrimary);
}

void UI_Button(C2D_TextBuf buf, C2D_Font font, float x, float y, float w, float h,
               const char* label, const char* value, UIButtonState state) {
    ColorRGBA bg = g_theme.surface;
    ColorRGBA border = g_theme.divider;
    ColorRGBA labelColor = g_theme.textPrimary;
    if (state == UI_BUTTON_SELECTED) {
        float pulse = sinf(s_frame * 4.0f) * 0.10f;
        bg = themeMix(g_theme.surface, g_theme.primary, 0.28f + pulse);
        border = g_theme.accent;
    } else if (state == UI_BUTTON_ACTIVE) {
        bg = themeMix(g_theme.surface, g_theme.accent, 0.34f);
        border = g_theme.accent;
    }
    UI_Shadow(x, y, w, h);
    UI_RoundRect(x, y, w, h, 9, bg);
    UI_RoundRect(x, y, w, h, 9, border);
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
        ColorRGBA border = g_theme.divider;
        UI_RoundRect(x - 1, y - 1, w + 2, h + 2, 5, border);
        UI_RoundRect(x, y, w, h, 4, color);
    }
}

void UI_KeyHelp(C2D_TextBuf buf, C2D_Font font, const char* left, const char* right) {
    UI_Fill(0, SCREEN_BOT_HEIGHT - 22, SCREEN_BOT_WIDTH, 22,
            (ColorRGBA){0, 0, 0, 60});
    if (left)
        UI_Text(buf, font, left, 9, SCREEN_BOT_HEIGHT - 16, 0.20f, 0.20f, g_theme.textHint);
    if (right)
        UI_TextRight(buf, font, right, SCREEN_BOT_WIDTH - 9, SCREEN_BOT_HEIGHT - 16, 0.20f, 0.20f, g_theme.textHint);
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
    float knobPulse = selected ? sinf(s_frame * 3.0f) * 2.0f : 0.0f;
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

void UI_MorphSelector(float x, float y, float w, float h, float targetX, float slideT) {
    float curX = targetX;
    if (slideT < 1.0f) {
        static float s_morphX = 0.0f;
        if (s_morphX == 0.0f) s_morphX = targetX;
        s_morphX = animApproach(s_morphX, targetX, 0.22f);
        curX = s_morphX;
    }
    ColorRGBA selBg = g_theme.accent;
    selBg.a = 35;
    UI_RoundRect(curX, y, w, h, h * 0.5f, selBg);
    ColorRGBA selBorder = g_theme.accent;
    selBorder.a = 80;
    UI_RoundRect(curX, y, w, h, h * 0.5f, selBorder);
}

void UI_PopupCard(float x, float y, float w, float h, float r, ColorRGBA bg, float scaleT) {
    float st = clampf(scaleT, 0.0f, 1.0f);
    float s = lerpf(0.92f, 1.0f, easeOutCubic(st));
    float alphaT = easeOutCubic(st);
    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;
    float sw = w * s;
    float sh = h * s;
    float sx2 = cx - sw * 0.5f;
    float sy2 = cy - sh * 0.5f;
    ColorRGBA dimBg = {0, 0, 0, (u8)(60 * alphaT)};
    UI_RoundRect(sx2, sy2, sw, sh, r, bg);
    ColorRGBA border = {255, 255, 255, (u8)(22 * alphaT)};
    UI_RoundRect(sx2, sy2, sw, sh, r, border);
}

void UI_StartupLogo(float t) {
    float logoAlpha = clampf(t * 2.0f, 0.0f, 1.0f);
    ColorRGBA bg = {0, 0, 0, (u8)(180 * logoAlpha)};
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, bg);
    if (t > 0.1f) {
        float logoIn = easeOutBack(clampf((t - 0.1f) / 0.3f, 0.0f, 1.0f));
        float lx = SCREEN_TOP_WIDTH * 0.5f;
        float ly = lerpf(120, 80, logoIn);
        UI_RoundRect(lx - 50, ly - 20, 100, 40, 20, g_theme.accent);
        UI_TextCenter(NULL, NULL, "CDS", lx, ly - 6, 0.60f, 0.60f, g_theme.onPrimary);
        float titleIn = easeOutCubic(clampf((t - 0.2f) / 0.3f, 0.0f, 1.0f));
        if (titleIn > 0.0f) {
            UI_TextCenter(NULL, NULL, "CustomizerDS", lx, ly + 30, 0.50f * titleIn, 0.50f * titleIn, g_theme.textPrimary);
        }
        float subIn = easeOutCubic(clampf((t - 0.35f) / 0.3f, 0.0f, 1.0f));
        if (subIn > 0.0f) {
            UI_TextCenter(NULL, NULL, "personalize seu 3DS com estilo", lx, ly + 50, 0.28f * subIn, 0.28f * subIn, g_theme.textSecondary);
        }
    }
}
