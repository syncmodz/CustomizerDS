#include "ui.h"
#include "common.h"
#include "icons.h"
#include <stdio.h>
#include <string.h>

static float s_frame = 0.0f;
static float s_dt = 1.0f / 60.0f;
float g_enterT = 1.0f;
float g_frame = 0.0f;
ScreenTransition g_trans;

void uiFrameTick(float dt) {
    if (dt <= 0.0f || dt > 0.25f) dt = 1.0f / 60.0f;
    s_dt = dt;
    s_frame += dt;
    if (s_frame > 1024.0f) s_frame = 0.0f;
    g_frame = s_frame;
    if (g_enterT < 1.0f) g_enterT = approach(g_enterT, 1.0f, 2.1f * dt);
    if (g_trans.active) transitionUpdate(&g_trans, dt);
}

float uiFrameTime(void) { return s_frame; }
float uiFrameDt(void) { return s_dt; }
void uiScreenEnter(void) { g_enterT = 0.0f; }
float UI_EnterProgress(void) { return easeOutCubic(g_enterT); }

float UI_EnterSlide(float maxOffset, EaseType ease) {
    float t = easeFunc(g_enterT, ease);
    return (1.0f - t) * maxOffset;
}

static u32 u32c(ColorRGBA c) { return C2D_Color32(c.r, c.g, c.b, c.a); }

void UI_RoundRect(float x, float y, float w, float h, float r, ColorRGBA color) {
    if (w <= 0 || h <= 0) return;
    float ri = fmaxf(1.0f, fminf(r, fminf(w, h) * 0.5f));
    u32 c = u32c(color);
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
    if (border.a > 0) UI_RoundRect(x, y, w, h, r, border);
    float innerR = r > 1.0f ? r - 1.0f : r;
    UI_RoundRect(x + 1, y + 1, w - 2, h - 2, innerR, fill);
}

void UI_Fill(float x, float y, float w, float h, ColorRGBA color) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x, y, 0.0f, w, h, u32c(color));
}

void UI_Shadow(float x, float y, float w, float h, float alpha, float offset) {
    if (w <= 0 || h <= 0) return;
    u8 a = (u8)fminf(alpha, 255);
    if (a > 0) {
        C2D_DrawRectSolid(x + offset, y + offset, 0.0f, w, h, C2D_Color32(0, 0, 0, a / 4));
        C2D_DrawRectSolid(x + offset * 1.5f, y + offset * 1.5f, 0.0f, w, h, C2D_Color32(0, 0, 0, a / 8));
    }
}

void UI_GradientV(float x, float y, float w, float h, ColorRGBA top, ColorRGBA bot) {
    if (w <= 0 || h <= 0) return;
    C2D_DrawRectSolid(x, y, 0.0f, w, h, u32c(top));
    for (float i = 0; i < h; i += 2.0f) {
        float t = i / h;
        ColorRGBA c;
        c.r = (u8)((float)top.r * (1.0f - t) + (float)bot.r * t);
        c.g = (u8)((float)top.g * (1.0f - t) + (float)bot.g * t);
        c.b = (u8)((float)top.b * (1.0f - t) + (float)bot.b * t);
        c.a = (u8)((float)top.a * (1.0f - t) + (float)bot.a * t);
        C2D_DrawRectSolid(x, y + i, 0.0f, w, 2.0f, u32c(c));
    }
}

static C2D_Text parse(C2D_Text* out, C2D_TextBuf buf, C2D_Font font, const char* text) {
    if (!text || !text[0]) return *out;
    if (font) C2D_TextFontParse(out, font, buf, text);
    else C2D_TextParse(out, buf, text);
    C2D_TextOptimize(out);
    return *out;
}

void UI_Text(C2D_TextBuf buf, C2D_Font font, const char* text,
             float x, float y, float sx, float sy, ColorRGBA color) {
    if (!text || !text[0]) return;
    C2D_Text t;
    parse(&t, buf, font, text);
    C2D_DrawText(&t, C2D_WithColor, x, y, 0.0f, sx, sy, u32c(color));
}

void UI_TextRight(C2D_TextBuf buf, C2D_Font font, const char* text,
                  float right, float y, float sx, float sy, ColorRGBA color) {
    if (!text || !text[0]) return;
    C2D_Text t;
    parse(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sx, sy, &tw, NULL);
    C2D_DrawText(&t, C2D_WithColor, right - tw, y, 0.0f, sx, sy, u32c(color));
}

void UI_TextCenter(C2D_TextBuf buf, C2D_Font font, const char* text,
                   float centerX, float y, float sx, float sy, ColorRGBA color) {
    if (!text || !text[0]) return;
    C2D_Text t;
    parse(&t, buf, font, text);
    float tw = 0.0f;
    C2D_TextGetDimensions(&t, sx, sy, &tw, NULL);
    C2D_DrawText(&t, C2D_WithColor, centerX - tw * 0.5f, y, 0.0f, sx, sy, u32c(color));
}

/* ==================== TOP SCREEN (macOS) ==================== */

void UI_TopBackground(void) {
    ColorRGBA base = g_theme.backgroundTop;
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, base);
    ColorRGBA glow = {255, 255, 255, themeIsDark() ? 3 : 20};
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, 1, glow);
    ColorRGBA shade = {0, 0, 0, themeIsDark() ? 18 : 6};
    UI_Fill(0, SCREEN_TOP_HEIGHT - 1, SCREEN_TOP_WIDTH, 1, shade);
}

void UI_TopMenuBar(const char* title, C2D_TextBuf buf) {
    ColorRGBA barBg = themeIsDark()
        ? (ColorRGBA){10, 12, 18, 235}
        : (ColorRGBA){222, 225, 234, 240};
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, 24, barBg);
    ColorRGBA div = {255, 255, 255, themeIsDark() ? 6 : 16};
    UI_Fill(0, 24, SCREEN_TOP_WIDTH, 1, div);

    /* macOS traffic light dots */
    UI_RoundRect(8, 8, 8, 8, 4, (ColorRGBA){255, 95, 87, 255});
    UI_RoundRect(20, 8, 8, 8, 4, (ColorRGBA){255, 189, 47, 255});
    UI_RoundRect(32, 8, 8, 8, 4, (ColorRGBA){40, 200, 65, 255});

    UI_TextCenter(buf, NULL, "CustomizerDS", 90, 5, 0.24f, 0.24f, g_theme.textSecondary);
    if (title)
        UI_TextCenter(buf, NULL, title, SCREEN_TOP_WIDTH / 2, 5, 0.26f, 0.26f, g_theme.textPrimary);
}

void UI_Card(float x, float y, float w, float h, float r, ColorRGBA bg) {
    UI_Shadow(x, y, w, h, 50, 2.0f);
    UI_RoundFrame(x, y, w, h, r, bg, (ColorRGBA){255, 255, 255, themeIsDark() ? 12 : 24});
    ColorRGBA hl = {255, 255, 255, themeIsDark() ? 4 : 14};
    UI_Fill(x + r, y + 1, w - r * 2.0f, 1, hl);
}

void UI_FrostCard(float x, float y, float w, float h, float r) {
    ColorRGBA bg = themeIsDark()
        ? (ColorRGBA){14, 17, 23, 220}
        : (ColorRGBA){248, 249, 252, 220};
    UI_Card(x, y, w, h, r, bg);
}

/* ==================== POPUP MODAL (Travel Agency style) ==================== */

void popupShow(PopupModal* p, const char* msg, const char* confirm, const char* cancel) {
    p->active = true;
    p->animT = 0.0f;
    p->bgAlpha = 0.0f;
    p->result = 0;
    strncpy(p->message, msg, sizeof(p->message) - 1);
    strncpy(p->confirmLabel, confirm ? confirm : "OK", sizeof(p->confirmLabel) - 1);
    strncpy(p->cancelLabel, cancel ? cancel : "", sizeof(p->cancelLabel) - 1);
}

void popupHide(PopupModal* p) { p->active = false; }
bool popupActive(PopupModal* p) { return p->active; }

void popupUpdate(PopupModal* p) {
    if (!p->active) return;
    p->animT = fminf(p->animT + uiFrameDt(), 0.35f);
    p->bgAlpha = fminf(p->animT / 0.15f, 0.6f);
}

void popupRender(C2D_TextBuf buf, PopupModal* p) {
    if (!p->active) return;
    float t = easeFunc(p->animT / 0.35f, EASE_OUT_BACK);
    float scale = 0.88f + 0.12f * t;
    float alpha = t;

    ColorRGBA bgDim = {0, 0, 0, (u8)(180 * p->bgAlpha)};
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, bgDim);

    float pw = 260.0f, ph = 120.0f;
    float px = (SCREEN_BOT_WIDTH - pw * scale) * 0.5f;
    float py = (SCREEN_BOT_HEIGHT - ph * scale) * 0.5f - 10;

    ColorRGBA cardBg = themeIsDark()
        ? (ColorRGBA){20, 24, 34, (u8)(245 * alpha)}
        : (ColorRGBA){248, 249, 252, (u8)(250 * alpha)};
    ColorRGBA cardBorder = {255, 255, 255, (u8)(themeIsDark() ? 20 * alpha : 30 * alpha)};

    UI_Shadow(px, py, pw * scale, ph * scale, 80 * alpha, 3.0f);
    UI_RoundFrame(px, py, pw * scale, ph * scale, 16, cardBg, cardBorder);

    float textX = (SCREEN_BOT_WIDTH) * 0.5f;
    float textY = py + 20 * scale;
    ColorRGBA textC = g_theme.textPrimary;
    textC.a = (u8)(textC.a * alpha);
    UI_TextCenter(buf, NULL, p->message, textX, textY, 0.28f, 0.28f, textC);

    if (p->confirmLabel[0]) {
        ColorRGBA btnBg = g_theme.accent;
        btnBg.a = (u8)(btnBg.a * alpha);
        float btnW = (p->cancelLabel[0]) ? 90.0f : 140.0f;
        float bx = (SCREEN_BOT_WIDTH - btnW) * 0.5f;
        float by = py + ph * scale - 44;
        UI_RoundRect(bx, by, btnW, 32, 16, btnBg);
        ColorRGBA btnText = themeContrastText(g_theme.accent);
        btnText.a = (u8)(btnText.a * alpha);
        UI_TextCenter(buf, NULL, p->confirmLabel, SCREEN_BOT_WIDTH * 0.5f, by + 7, 0.26f, 0.26f, btnText);
    }

    if (p->cancelLabel[0]) {
        ColorRGBA cancelBg = themeIsDark()
            ? (ColorRGBA){40, 44, 60, (u8)(200 * alpha)}
            : (ColorRGBA){220, 222, 232, (u8)(220 * alpha)};
        float by = py + ph * scale - 44;
        UI_RoundRect(px + 30, by, 90, 32, 16, cancelBg);
        ColorRGBA cancelText = g_theme.textSecondary;
        cancelText.a = (u8)(cancelText.a * alpha);
        UI_TextCenter(buf, NULL, p->cancelLabel, px + 75, by + 7, 0.26f, 0.26f, cancelText);
    }
}

/* ==================== BOTTOM SCREEN (Touch Bar) ==================== */

void UI_BottomBackground(void) {
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, g_theme.background);

    ColorRGBA touchBarBg = themeIsDark()
        ? (ColorRGBA){44, 46, 54, 255}
        : (ColorRGBA){240, 242, 248, 255};
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, 50, touchBarBg);
    ColorRGBA stripDiv = {255, 255, 255, themeIsDark() ? 12 : 20};
    UI_Fill(0, 50, SCREEN_BOT_WIDTH, 1, stripDiv);

    ColorRGBA contentBg = themeIsDark()
        ? (ColorRGBA){28, 30, 38, 255}
        : (ColorRGBA){245, 247, 250, 255};
    UI_Fill(0, 50, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT - 50 - 26, contentBg);
}

void UI_TouchBarBackground(void) {
    UI_Fill(0, 0, SCREEN_BOT_WIDTH, SCREEN_BOT_HEIGHT, g_theme.background);
}

void UI_TouchBarPill(C2D_TextBuf buf, float x, float y, float w, float h,
                     const char* label, const char* icon, bool selected, float appearT, float pulse) {
    float ap = (appearT < 1.0f) ? easeOutCubic(appearT) : 1.0f;
    if (ap <= 0.0f) return;
    float slide = (1.0f - ap) * 10.0f;
    float ox = x + slide;

    float r = h * 0.5f;
    ColorRGBA bg, border, textCol;

    if (selected) {
        bg = g_theme.accent;
        bg.r = (u8)((int)bg.r + (int)(pulse * 15));
        bg.g = (u8)((int)bg.g + (int)(pulse * 15));
        bg.b = (u8)((int)bg.b + (int)(pulse * 15));
        border = (ColorRGBA){255, 255, 255, (u8)(30 + (int)(pulse * 20))};
        textCol = themeContrastText(g_theme.accent);
    } else {
        bg = themeIsDark()
            ? (ColorRGBA){34, 38, 52, 230}
            : (ColorRGBA){232, 235, 244, 240};
        border = (ColorRGBA){0, 0, 0, themeIsDark() ? 18 : 12};
        textCol = g_theme.textSecondary;
    }

    UI_Shadow(ox, y, w, h, 20, 1.0f);
    UI_RoundRect(ox, y, w, h, r, bg);
    if (!selected) UI_RoundFrame(ox, y, w, h, r, bg, border);

    if (selected) {
        ColorRGBA glow = g_theme.accent;
        glow.a = 30;
        UI_RoundRect(ox + 3, y + 3, w - 6, h - 6, r - 3, glow);
    }

    float cx = ox + w * 0.5f;
    float cy = y + 6;
    if (icon) UI_Text(buf, NULL, icon, ox + 10, cy, 0.30f, 0.30f, textCol);
    float lx = icon ? ox + 28 : cx;
    UI_TextCenter(buf, NULL, label, cx, cy, 0.26f, 0.26f, textCol);
}

void UI_TouchBarRow(C2D_TextBuf buf, const char** labels, const char** icons,
                    int count, int selected, float baseY, float baseAppear) {
    float gap = 8.0f;
    float totalW = SCREEN_BOT_WIDTH - 20;
    float btnW = (totalW - gap * (count - 1)) / count;
    btnW = fmaxf(70.0f, fminf(btnW, 130.0f));
    float startX = (SCREEN_BOT_WIDTH - (btnW * count + gap * (count - 1))) * 0.5f;

    for (int i = 0; i < count; i++) {
        float bx = startX + i * (btnW + gap);
        float ap = clampf(baseAppear - i * 0.10f, 0.0f, 1.0f);
        float pulse = (i == selected) ? 0.5f + 0.5f * sinf(s_frame * 4.0f) : 0.0f;
        UI_TouchBarPill(buf, bx, baseY, btnW, 34,
                        labels[i], icons ? icons[i] : NULL,
                        i == selected, ap, pulse);
    }
}

void UI_TouchBarSegmented(C2D_TextBuf buf, float x, float y, float w, float h,
                          const char** labels, int count, int selected, float morphT) {
    ColorRGBA baseBg = themeIsDark()
        ? (ColorRGBA){34, 38, 52, 230}
        : (ColorRGBA){232, 235, 244, 240};
    ColorRGBA baseBorder = {0, 0, 0, themeIsDark() ? 20 : 14};
    float r = h * 0.5f;

    UI_Shadow(x, y, w, h, 15, 1.0f);
    UI_RoundRect(x, y, w, h, r, baseBg);
    UI_RoundFrame(x, y, w, h, r, baseBg, baseBorder);

    float segW = w / count;
    float targetX = x + selected * segW;
    static float s_morphX = -9999.0f;
    static bool s_init = false;
    if (!s_init) { s_morphX = targetX; s_init = true; }
    if (morphT < 1.0f) {
        float targetV = targetX;
        s_morphX = approach(s_morphX, targetV, 240.0f * uiFrameDt());
    } else {
        s_morphX = targetX;
    }

    float curX = s_morphX;
    ColorRGBA selBg = g_theme.accent;
    selBg.a = themeIsDark() ? 45 : 55;
    ColorRGBA selBorder = g_theme.accent;
    selBorder.a = themeIsDark() ? 75 : 95;
    UI_RoundFrame(curX + 2, y + 2, segW - 4, h - 4, r - 2, selBg, selBorder);

    for (int i = 0; i < count; i++) {
        float cx = x + segW * i + segW * 0.5f;
        ColorRGBA tc = (i == selected) ? g_theme.textPrimary : g_theme.textSecondary;
        UI_TextCenter(buf, NULL, labels[i], cx, y + (h - 14) * 0.5f, 0.26f, 0.26f, tc);
    }
}

void UI_TouchBarSlider(C2D_TextBuf buf, float x, float y, float w,
                       const char* label, int value, int min, int max,
                       bool selected, ColorRGBA swatch) {
    float rh = 28.0f;
    ColorRGBA bg = selected
        ? themeMix((ColorRGBA){20, 24, 34, 250}, g_theme.accent, 0.08f)
        : (ColorRGBA){20, 24, 34, 250};
    if (!themeIsDark()) bg = selected
        ? themeMix((ColorRGBA){240, 242, 248, 250}, g_theme.accent, 0.06f)
        : (ColorRGBA){240, 242, 248, 250};
    ColorRGBA border = selected ? g_theme.accent
        : (ColorRGBA){255, 255, 255, themeIsDark() ? 12 : 16};
    if (selected) border.a = themeIsDark() ? 65 : 85;

    UI_Shadow(x, y, w, rh, 10, 1.0f);
    UI_RoundFrame(x, y, w, rh, rh * 0.5f, bg, border);

    if (label) UI_Text(buf, NULL, label, x + 10, y + 6, 0.26f, 0.26f, g_theme.textPrimary);

    float barX = x + (label ? 36 : 10);
    float barY = y + 11;
    float barW = w - (label ? 74 : 44);
    float t = (max > min) ? (float)(value - min) / (float)(max - min) : 0.0f;
    t = clampf(t, 0.0f, 1.0f);

    ColorRGBA track = themeIsDark()
        ? (ColorRGBA){55, 60, 78, 200}
        : (ColorRGBA){185, 190, 208, 200};
    UI_RoundRect(barX, barY, barW, 5, 2.5f, track);
    if (t > 0.0f) {
        ColorRGBA fill = swatch;
        fill.a = 200;
        float fillW = barW * t;
        UI_RoundRect(barX, barY, fillW, 5, 2.5f, fill);
    }

    float knobX = barX + barW * t;
    UI_Shadow(knobX - 4, barY - 3, 8, 11, 25, 1.0f);
    UI_RoundRect(knobX - 4, barY - 3, 8, 11, 4, selected ? g_theme.onPrimary : g_theme.textSecondary);

    char valStr[8];
    snprintf(valStr, sizeof(valStr), "%d", value);
    UI_TextRight(buf, NULL, valStr, x + w - 8, y + 6, 0.24f, 0.24f, g_theme.textSecondary);
}

void UI_HelpBar(C2D_TextBuf buf, const char* left, const char* right) {
    ColorRGBA barBg = themeIsDark()
        ? (ColorRGBA){0, 0, 0, 60}
        : (ColorRGBA){175, 180, 194, 70};
    float hy = SCREEN_BOT_HEIGHT - 26;
    UI_Fill(0, hy, SCREEN_BOT_WIDTH, 26, barBg);
    ColorRGBA div = {255, 255, 255, themeIsDark() ? 4 : 6};
    UI_Fill(0, hy, SCREEN_BOT_WIDTH, 1, div);
    if (left) UI_Text(buf, NULL, left, 9, hy + 5, 0.22f, 0.22f, g_theme.textSecondary);
    if (right) UI_TextRight(buf, NULL, right, SCREEN_BOT_WIDTH - 9, hy + 5, 0.22f, 0.22f, g_theme.textSecondary);
}

void UI_Badge(C2D_TextBuf buf, float x, float y, const char* text, ColorRGBA bg) {
    if (!text || !text[0]) return;
    float tw = 0.0f;
    C2D_Text tmp;
    C2D_TextParse(&tmp, buf, text);
    C2D_TextOptimize(&tmp);
    C2D_TextGetDimensions(&tmp, 0.22f, 0.22f, &tw, NULL);
    float pw = tw + 14.0f;
    ColorRGBA textCol = ((int)bg.r + (int)bg.g + (int)bg.b > 430)
        ? (ColorRGBA){10, 12, 16, 255}
        : g_theme.onPrimary;
    UI_RoundRect(x, y, pw, 18, 9, bg);
    UI_TextCenter(buf, NULL, text, x + pw * 0.5f, y + 2, 0.22f, 0.22f, textCol);
}

void UI_PillButton(C2D_TextBuf buf, float x, float y, float w, float h,
                   const char* label, const char* icon, int iconImg, bool selected, float appearT) {
    float ap = (appearT < 1.0f) ? easeOutCubic(appearT) : 1.0f;
    if (ap <= 0.0f) return;
    float slide = (1.0f - ap) * 8.0f;
    float ox = x + slide;
    float oy = y + slide;

    float r = h * 0.5f;
    ColorRGBA bg, textCol;

    if (selected) {
        bg = g_theme.accent;
        textCol = themeContrastText(g_theme.accent);
    } else {
        bg = themeIsDark()
            ? (ColorRGBA){44, 46, 54, 240}
            : (ColorRGBA){230, 232, 240, 240};
        textCol = g_theme.textSecondary;
    }

    bg.a = (u8)((float)bg.a * ap);
    textCol.a = (u8)((float)textCol.a * ap);

    UI_Shadow(ox, oy, w, h, 15, 1.0f);
    if (selected) {
        /* leve brilho pulsante no botao ativo (60fps.design: "pulse background") */
        float pulse = (sinf(uiFrameTime() * 3.2f) + 1.0f) * 0.5f;
        ColorRGBA glow = g_theme.accent;
        glow.a = (u8)((10.0f + pulse * 22.0f) * ap);
        UI_RoundRect(ox - 3, oy - 3, w + 6, h + 6, r + 3, glow);
    }
    UI_RoundRect(ox, oy, w, h, r, bg);

    if (!selected) {
        ColorRGBA border = (ColorRGBA){0, 0, 0, themeIsDark() ? 15 : 10};
        border.a = (u8)((float)border.a * ap);
        UI_RoundFrame(ox, oy, w, h, r, bg, border);
    }

    bool hasIcon = (iconImg >= 0) || (icon != NULL);
    float cx = ox + w * 0.5f;
    if (iconImg >= 0) {
        iconsDraw((IconID)iconImg, cx, oy + 9, 14.0f, textCol, ap);
    } else if (icon) {
        UI_TextCenter(buf, NULL, icon, cx, oy + 3, 0.28f, 0.28f, textCol);
    }
    UI_TextCenter(buf, NULL, label, cx, oy + (hasIcon ? 17 : 6), 0.26f, 0.26f, textCol);
}

void UI_StartupLogo(C2D_TextBuf buf, float t) {
    float alpha = clampf(t / 0.35f, 0.0f, 1.0f);
    ColorRGBA bg = {0, 0, 0, (u8)(180 * alpha)};
    UI_Fill(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, bg);

    if (t < 0.08f) return;

    /* Phase 1: CDS badge scales in with bounce (Herman Miller style) */
    float p1t = clampf((t - 0.08f) / 0.30f, 0.0f, 1.0f);
    float scale = easeFunc(p1t, EASE_OUT_BACK);
    float sy = lerpf(130.0f, 90.0f, easeOutCubic(p1t));
    float lx = SCREEN_TOP_WIDTH * 0.5f;

    ColorRGBA badge = themeMix((ColorRGBA){18, 22, 34, 245}, g_theme.accent, 0.12f);
    float bw = 80 * scale, bh = 40 * scale;
    UI_RoundFrame(lx - bw * 0.5f, sy - bh * 0.5f, bw, bh, 20 * scale, badge, g_theme.accent);
    UI_TextCenter(buf, NULL, "CDS", lx, sy - 5 * scale, 0.48f * scale, 0.48f * scale, g_theme.textPrimary);

    /* Phase 2: title fades in (Herman Miller staggered reveal) */
    float p2t = clampf((t - 0.28f) / 0.25f, 0.0f, 1.0f);
    if (p2t > 0.0f) {
        float ta = easeOutCubic(p2t);
        float ts = 0.44f * ta;
        ColorRGBA titleC = g_theme.textPrimary;
        titleC.a = (u8)(255 * ta);
        UI_TextCenter(buf, NULL, "CustomizerDS", lx, sy + 28, ts, ts, titleC);
    }

    /* Phase 3: subtitle */
    float p3t = clampf((t - 0.42f) / 0.25f, 0.0f, 1.0f);
    if (p3t > 0.0f) {
        float sa = easeOutCubic(p3t);
        ColorRGBA subC = g_theme.textSecondary;
        subC.a = (u8)(255 * sa);
        UI_TextCenter(buf, NULL, "personalize seu console com estilo", lx, sy + 48, 0.26f * sa, 0.26f * sa, subC);
    }

    /* Phase 4: exit (scale + fade out like Herman Miller) */
    float exitT = clampf((t - 1.00f) / 0.23f, 0.0f, 1.0f);
    if (exitT > 0.0f) {
        float exitScale = lerpf(1.0f, 1.12f, exitT);
        float exitAlpha = 1.0f - exitT;
        float cx = lx;
        float cy = sy;
        UI_RoundFrame(cx - 44 * exitScale, cy - 22 * exitScale,
                      88 * exitScale, 44 * exitScale,
                      22 * exitScale,
                      (ColorRGBA){18, 22, 34, (u8)(245 * exitAlpha)},
                      (ColorRGBA){g_theme.accent.r, g_theme.accent.g, g_theme.accent.b, (u8)(255 * exitAlpha)});
        ColorRGBA cdsC = g_theme.textPrimary;
        cdsC.a = (u8)(255 * exitAlpha);
        UI_TextCenter(buf, NULL, "CDS", cx, cy - 3 * exitScale, 0.48f * exitScale, 0.48f * exitScale, cdsC);
    }
}
