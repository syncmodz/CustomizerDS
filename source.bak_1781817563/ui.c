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

void UI_RoundRect(float x, float y, float w, float h, float r, ColorRGBA color) {
    if (w <= 1 || h <= 1) return;
    float ri = clampf(r, 1.0f, fminf(w, h) * 0.5f);
    u32 c = toU32(color);
    if (w <= ri * 2.0f + 1.0f || h <= ri * 2.0f + 1.0f) {
        C2D_DrawCircleSolid(x + w * 0.5f, y + h * 0.5f, 0.0f, fminf(w, h) * 0.5f, c);
        if (w > h) C2D_DrawRectSolid(x + h * 0.5f, y, 0.0f, w - h, h, c);
        if (h > w) C2D_DrawRectSolid(x, y + w * 0.5f, 0.0f, w, h - w, c);
        return;
    }
    C2D_DrawRectSolid(x + ri, y, 0.0f, w - ri * 2.0f, h, c);
    C2D_DrawRectSolid(x, y + ri, 0.0f, w, h - ri * 2.0f, c);
    C2D_DrawCircleSolid(x + ri, y + ri, 0.0f, ri, c);
    C2D_DrawCircleSolid(x + w - ri, y + ri, 0.0f, ri, c);
    C2D_DrawCircleSolid(x + ri, y + h - ri, 0.0f, ri, c);
    C2D_DrawCircleSolid(x + w - ri, y + h - ri, 0.0f, ri, c);
}

void UI_RoundFrame(float x, float y, float w, float h, float r, ColorRGBA fill, ColorRGBA border) {
    if (w <= 1 || h <= 1) return;
    if (border.a > 0) {
        UI_RoundRect(x, y, w, h, r, border);
    }
    if (w > 2 && h > 2) {
        float innerR = r > 1.0f ? r - 1.0f : r;
        UI_RoundRect(x + 1, y + 1, w - 2, h - 2, innerR, fill);
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
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, 46, (ColorRGBA){255, 255, 255, themeIsDark() ? 4 : 26});
    UI_Fill(0, SCREEN_TOP_HEIGHT - 58, SCREEN_TOP_WIDTH, 58, (ColorRGBA){0, 0, 0, themeIsDark() ? 18 : 8});
    ColorRGBA divider = {255, 255, 255, themeIsDark() ? 10 : 22};
    UI_Line(0, SCREEN_TOP_HEIGHT - 1, SCREEN_TOP_WIDTH, 1, divider);
}

void UI_BackgroundBottom(void) {
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, g_theme.background);
    ColorRGBA topGlow = {255, 255, 255, 6};
    UI_Line(0, 0, SCREEN_BOT_WIDTH, 1, topGlow);
}

void UI_TouchBarBackground(void) {
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, g_theme.background);
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, 42,
            themeIsDark() ? (ColorRGBA){255, 255, 255, 5} : (ColorRGBA){255, 255, 255, 46});
    UI_Fill(0, SCREEN_BOT_HEIGHT - 34, SCREEN_BOT_WIDTH, 34,
            themeIsDark() ? (ColorRGBA){0, 0, 0, 34} : (ColorRGBA){180, 184, 198, 40});
    UI_Line(0, 0, SCREEN_BOT_WIDTH, 1,
            themeIsDark() ? (ColorRGBA){255, 255, 255, 14} : (ColorRGBA){255, 255, 255, 80});
}

void UI_GlassPanel(float x, float y, float w, float h, float r, ColorRGBA bg, ColorRGBA border, ColorRGBA highlight) {
    if (w <= 0 || h <= 0) return;
    UI_Shadow(x, y, w, h);
    UI_RoundFrame(x, y, w, h, r, bg, border);
    if (highlight.a > 0) {
        UI_Line(x + r, y + 1, w - r * 2.0f, 1, highlight);
        UI_Line(x + 2, y + 10, 1, h - 20, (ColorRGBA){255, 255, 255, 7});
    }
}

void UI_CapsuleButton(C2D_TextBuf buf, C2D_Font font, float x, float y, float w, float h,
                      const char* label, const char* prefix, const char* value,
                      UIButtonState state, float pressedT, float appearT) {
    float ap = (appearT < 1.0f) ? easeOutCubic(appearT) : 1.0f;
    float slideIn = (1.0f - ap) * 16.0f;
    float ox = x + slideIn;
    float oy = y + pressedT * 1.5f;

    ColorRGBA base = themeIsDark() ? (ColorRGBA){18, 22, 31, 246} : (ColorRGBA){246, 248, 252, 248};
    ColorRGBA bg = base;
    ColorRGBA labelColor = g_theme.textPrimary;
    ColorRGBA borderCol = themeIsDark() ? (ColorRGBA){255, 255, 255, 18} : (ColorRGBA){20, 24, 34, 20};
    ColorRGBA iconBg = themeIsDark() ? (ColorRGBA){36, 45, 60, 235} : (ColorRGBA){228, 232, 242, 245};

    if (state == UI_BUTTON_SELECTED) {
        float amt = 0.13f - pressedT * 0.03f;
        bg = themeMix(base, g_theme.accent, amt);
        labelColor = g_theme.textPrimary;
        borderCol = g_theme.accent;
        borderCol.a = themeIsDark() ? 86 : 110;
        iconBg = themeMix(iconBg, g_theme.accent, 0.28f);
    } else if (state == UI_BUTTON_ACTIVE) {
        bg = themeMix(base, g_theme.accent, 0.08f);
        borderCol.a = 30;
    }

    float radius = h * 0.5f;

    if (ap < 1.0f) {
        float scaleW = lerpf(0.88f, 1.0f, ap);
        float scaleH = lerpf(0.84f, 1.0f, ap);
        float cw = w * scaleW;
        float ch = h * scaleH;
        float cx = ox + (w - cw) * 0.5f;
        float cy = oy + (h - ch) * 0.5f;
        UI_RoundFrame(cx, cy, cw, ch, radius, bg, borderCol);
        UI_TextCenter(buf, font, label, cx + cw * 0.5f, cy + (ch - 18) * 0.5f, 0.36f, 0.36f, labelColor);
        return;
    }

    UI_Shadow(ox + 1, oy + 1, w, h);
    UI_RoundFrame(ox, oy, w, h, radius, bg, borderCol);
    if (state == UI_BUTTON_SELECTED) {
        ColorRGBA rail = g_theme.accent;
        rail.a = themeIsDark() ? 210 : 235;
        UI_RoundRect(ox + 5, oy + 6, 4, h - 12, 2, rail);
        ColorRGBA glow = g_theme.accent;
        glow.a = themeIsDark() ? 18 : 26;
        UI_RoundRect(ox + 2, oy + 3, w - 4, h - 6, radius - 3, glow);
    }

    if (prefix) {
        float ph = h - 10;
        float px = ox + 13;
        float py = oy + 5;
        UI_RoundRect(px, py, ph, ph, ph * 0.5f, iconBg);
        UI_TextCenter(buf, font, prefix, px + ph * 0.5f, py + (ph - 15) * 0.5f, 0.31f, 0.31f, g_theme.textPrimary);
        UI_Text(buf, font, label, ox + 54, oy + (h - 18) * 0.5f, 0.36f, 0.36f, labelColor);
    } else {
        UI_Text(buf, font, label, ox + 16, oy + (h - 18) * 0.5f, 0.36f, 0.36f, labelColor);
    }
    if (value) {
        UI_TextRight(buf, NULL, value, ox + w - 12, oy + (h - 15) * 0.5f, 0.24f, 0.24f, g_theme.textSecondary);
    }
}

void UI_TouchBarPill(float x, float y, float w, float h, ColorRGBA bg, const char* label,
                     C2D_TextBuf buf, C2D_Font font, float sx, float sy) {
    UI_RoundRect(x, y, w, h, h * 0.5f, bg);
    UI_TextCenter(buf, font, label, x + w * 0.5f, y + (h - sy * 16.0f) * 0.5f, sx, sy, themeContrastText(bg));
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
    UI_RoundFrame(x, y, w, h, 9, bg, border);
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
    UI_Fill(0, SCREEN_BOT_HEIGHT - 25, SCREEN_BOT_WIDTH, 25,
            themeIsDark() ? (ColorRGBA){0, 0, 0, 78} : (ColorRGBA){210, 214, 226, 135});
    UI_Line(0, SCREEN_BOT_HEIGHT - 25, SCREEN_BOT_WIDTH, 1,
            (ColorRGBA){255, 255, 255, 10});
    if (left)
        UI_Text(buf, font, left, 9, SCREEN_BOT_HEIGHT - 18, 0.25f, 0.25f, g_theme.textSecondary);
    if (right)
        UI_TextRight(buf, font, right, SCREEN_BOT_WIDTH - 9, SCREEN_BOT_HEIGHT - 18, 0.25f, 0.25f, g_theme.textSecondary);
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
    ColorRGBA border = selected ? g_theme.accent : (ColorRGBA){255, 255, 255, 16};
    if (selected) border.a = 70;
    UI_RoundFrame(x, y, w, 36, 18, bg, border);
    UI_Text(buf, font, label, x + 12, y + 9, 0.29f, 0.29f, g_theme.textPrimary);
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
    C2D_TextGetDimensions(&tmp, 0.24f, 0.24f, &tw, NULL);
    float pw = tw + 16.0f;
    ColorRGBA badgeText = ((int)color.r + (int)color.g + (int)color.b > 430)
        ? (ColorRGBA){10, 12, 16, 255}
        : g_theme.onPrimary;
    UI_RoundRect(x, y, pw, 22, 11, color);
    UI_TextCenter(buf, NULL, text, x + pw * 0.5f, y + 4, 0.25f, 0.25f, badgeText);
}

void UI_MorphSelector(float x, float y, float w, float h, float targetX, float slideT) {
    (void)x;
    (void)slideT;
    float curX = targetX;
    ColorRGBA selBg = g_theme.accent;
    selBg.a = 35;
    ColorRGBA selBorder = g_theme.accent;
    selBorder.a = 80;
    UI_RoundFrame(curX, y, w, h, h * 0.5f, selBg, selBorder);
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
    ColorRGBA border = {255, 255, 255, (u8)(22 * alphaT)};
    UI_RoundFrame(sx2, sy2, sw, sh, r, bg, border);
}

void UI_StartupLogo(C2D_TextBuf buf, float t) {
    float logoAlpha = clampf(t * 2.0f, 0.0f, 1.0f);
    ColorRGBA bg = {0, 0, 0, (u8)(180 * logoAlpha)};
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, bg);
    if (t > 0.1f) {
        float logoIn = easeOutBack(clampf((t - 0.1f) / 0.3f, 0.0f, 1.0f));
        float lx = SCREEN_TOP_WIDTH * 0.5f;
        float ly = lerpf(120, 80, logoIn);
        ColorRGBA shell = themeMix((ColorRGBA){18, 22, 31, 245}, g_theme.accent, 0.18f);
        UI_RoundFrame(lx - 58, ly - 23, 116, 46, 23, shell, g_theme.accent);
        UI_TextCenter(buf, NULL, "CDS", lx, ly - 7, 0.62f, 0.62f, g_theme.textPrimary);
        float titleIn = easeOutCubic(clampf((t - 0.2f) / 0.3f, 0.0f, 1.0f));
        if (titleIn > 0.0f) {
            UI_TextCenter(buf, NULL, "CustomizerDS", lx, ly + 30, 0.50f * titleIn, 0.50f * titleIn, g_theme.textPrimary);
        }
        float subIn = easeOutCubic(clampf((t - 0.35f) / 0.3f, 0.0f, 1.0f));
        if (subIn > 0.0f) {
            UI_TextCenter(buf, NULL, "personalize seu 3DS com estilo", lx, ly + 50, 0.28f * subIn, 0.28f * subIn, g_theme.textSecondary);
        }
    }
}
