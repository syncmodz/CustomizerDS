#include "ui.h"
#include "fonts.h"

void drawPanel(int x, int y, int w, int h, int r, u32 color, float alpha) {
    u32 c = colorWithAlpha(color, (u8)(alpha * (color & 0xFF)));
    drawRoundedRect(x, y, w, h, r, c);
}

void drawShadow(int x, int y, int w, int h, int r, float size, u32 color) {
    u8 a = color & 0xFF;
    if (a == 0) return;
    int steps = (int)fminf(size * 4, 16);
    for (int i = steps - 1; i >= 0; i--) {
        float t = (i + 1) / (float)steps;
        int s = (int)(size * t);
        u8 al = (u8)(a * (1 - t) * t);
        u32 c = (color & 0xFFFFFF00) | al;
        drawRoundedRect(x - s, y - s, w + s * 2, h + s * 2, (int)(r + s * 0.5f), c);
    }
}

void drawCard(int x, int y, int w, int h, u32 color, const char* title, const char* subtitle, float parallax, float alpha) {
    int dx = (int)(parallax * 2);
    drawShadow(x + dx, y, w, h, ITEM_R, 4, rgba8(0,0,0,(u8)(alpha * 60)));
    drawRoundedRect(x + dx, y, w, h, ITEM_R, colorWithAlpha(color, (u8)(alpha * 255)));
    if (title) {
        drawTextCentered(fontsGetBuf(), fontsGetSystem(), title, x + dx + w/2, y + 12, 0.45f, 0.45f,
            colorWithAlpha(pickContrastColor(color), (u8)(alpha * 255)));
    }
    if (subtitle) {
        drawTextCentered(fontsGetBuf(), fontsGetSystem(), subtitle, x + dx + w/2, y + h - 20, 0.3f, 0.3f,
            colorWithAlpha(pickContrastColor(color), (u8)(alpha * 180)));
    }
}

void drawToggle(int x, int y, bool on, u32 accent, float alpha) {
    int w = 50, h = 28;
    u32 bg = on ? colorWithAlpha(accent, (u8)(alpha * 255)) : rgba8(58,58,60,(u8)(alpha * 255));
    drawRoundedRect(x, y, w, h, h/2, bg);
    int tx = on ? x + w - h + 4 : x + 4;
    u32 tc = on ? rgba8(255,255,255,(u8)(alpha * 255)) : rgba8(99,99,102,(u8)(alpha * 255));
    C2D_DrawCircleSolid(tx + h/2 - 4, y + h/2, 0.5f, h/2 - 4, tc);
    C2D_DrawCircleSolid(tx + h/2 - 4, y + h/2, 0.5f, h/2 - 6, lightenColor(tc, 0.3f));
}

void drawSlider2(int x, int y, int w, int h, float pct, u32 track, u32 fill, u32 thumb, float alpha) {
    int trY = y + h/2 - 3;
    drawRoundedRect(x, trY, w, 6, 3, colorWithAlpha(track, (u8)(alpha * 255)));
    int fw = (int)(w * pct);
    if (fw > 0) drawRoundedRect(x, trY, fw, 6, 3, colorWithAlpha(fill, (u8)(alpha * 255)));
    int tx = x + fw;
    C2D_DrawCircleSolid(tx, y + h/2, 0.5f, 8, colorWithAlpha(darkenColor(thumb, 0.2f), (u8)(alpha * 255)));
    C2D_DrawCircleSolid(tx, y + h/2, 0.5f, 6, colorWithAlpha(thumb, (u8)(alpha * 255)));
    C2D_DrawCircleSolid(tx, y + h/2, 0.5f, 4, colorWithAlpha(lightenColor(thumb, 0.3f), (u8)(alpha * 255)));
}

void drawProgress(int x, int y, int w, int h, float pct, u32 track, u32 fill) {
    drawRoundedRect(x, y, w, h, h/2, track);
    int fw = (int)(w * pct);
    if (fw > 0) drawRoundedRect(x, y, fw, h, h/2, fill);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)(pct * 100));
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), buf, x + w/2, y + (h-10)/2, 0.35f, 0.35f, rgba8(255,255,255,255));
}

void drawHexInput2(int x, int y, int w, int h, const char* text, bool active, float blink, u32 bg, u32 fg, float alpha) {
    drawRoundedRect(x, y, w, h, 4, colorWithAlpha(bg, (u8)(alpha * 255)));
    C2D_DrawRectSolid(x + 8, y + h - 2, 0.5f, w - 16, 1, colorWithAlpha(g_secTextColor, (u8)(alpha * 200)));
    char buf[10];
    snprintf(buf, sizeof(buf), "#%.6s", text ? text : "");
    drawText(fontsGetBuf(), fontsGetSystem(), buf, x + 10, y + (h-12)/2, 0.45f, 0.45f, colorWithAlpha(fg, (u8)(alpha * 255)));
    if (active && (int)(blink * 2) % 2 == 0) {
        float tw = textGetWidth(fontsGetSystem(), buf, 0.45f);
        C2D_DrawRectSolid(x + 10 + tw + 4, y + 6, 0.5f, 1, h - 12, colorWithAlpha(fg, (u8)(alpha * 255)));
    }
}

void drawHueWheel(int cx, int cy, int r) {
    for (int a = 0; a < 360; a += 2) {
        float rad = a * M_PI / 180.0f;
        int x = cx + (int)(r * cosf(rad));
        int y = cy + (int)(r * sinf(rad));
        float s = sinf(rad) * 0.5f + 0.5f;
        C2D_DrawCircleSolid(x, y, 0.5f, 3, rgba8(
            (u8)(((1-s)*255 + s*255)/2),
            (u8)(((1-s)*0 + s*255)/2),
            (u8)(((1-s)*255 + s*0)/2),
            255
        ));
    }
}

void drawStartupScreen(float progress) {
    float p = clampf(progress, 0, 1);
    int bw = 200, bh = 4;
    int bx = (SCREEN_TOP_W - bw) / 2;
    int by = SCREEN_TOP_H / 2 + 30;
    drawRoundedRect(bx, by, bw, bh, 2, rgba8(58,58,60,255));
    if (p > 0) drawRoundedRect(bx, by, (int)(bw * p), bh, 2, accentForTheme());
    if (p < 1) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Starting up... %d%%", (int)(p * 100));
        drawTextCentered(fontsGetBuf(), fontsGetSystem(), buf, SCREEN_TOP_W/2, by - 18, 0.35f, 0.35f, g_secTextColor);
    }
}

void drawMacOSTouchBar2(int y, float alpha) {
    u32 bar = g_darkMode ? rgba8(44,44,46,(u8)(alpha*255)) : rgba8(232,232,237,(u8)(alpha*255));
    drawRoundedRect(20, y, SCREEN_BOT_W - 40, 32, 16, bar);
    u32 sep = g_darkMode ? rgba8(58,58,60,(u8)(alpha*200)) : rgba8(210,210,215,(u8)(alpha*200));
    const char* items[] = {"|", "||", "\xE2\x96\xB6", "\xE2\x97\x8F", "\xE2\x96\xA0", "\xE2\x96\xB2", "\xE2\x96\xBC"};
    for (int i = 0; i < 7; i++) {
        int sp = (SCREEN_BOT_W - 40) / 8;
        int ix = 20 + sp * (i + 1);
        drawTextCentered(fontsGetBuf(), fontsGetSystem(), items[i], ix, y + 6, 0.35f, 0.35f,
            colorWithAlpha(g_secTextColor, (u8)(alpha*200)));
        if (i < 6) C2D_DrawRectSolid(ix + sp/2, y + 6, 0.5f, 1, 20, sep);
    }
}

void drawBadge2(int x, int y, const char* label, u32 color) {
    float tw = textGetWidth(fontsGetSystem(), label, 0.35f);
    int bw = (int)tw + 12, bh = 16;
    drawRoundedRect(x - bw/2, y - bh/2, bw, bh, 8, color);
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), label, x, y - 4, 0.35f, 0.35f, rgba8(255,255,255,255));
}
