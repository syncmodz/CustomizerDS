#include "color_picker.h"
#include "ui.h"
#include "theme.h"
#include "fonts.h"

void cpInit(ColorPicker* cp, int x, int y, int w, int h, u32 init) {
    memset(cp, 0, sizeof(ColorPicker));
    cp->x = x; cp->y = y; cp->w = w; cp->h = h;
    cp->color = init;
    u8 cr=(init>>24)&0xFF, cg=(init>>16)&0xFF, cb=(init>>8)&0xFF;
    snprintf(cp->hex, 7, "%02X%02X%02X", cr, cg, cb);
}

void cpUpdate(ColorPicker* cp, InputState* in) {
    if (in->touchDown) {
        cp->active = touchHit(in->touch.px, in->touch.py, cp->x + 10, cp->y + 18, cp->w - 20, 30);
    }
    if (cp->active) {
        if (in->b || in->cancel) { cp->active = false; }
        if (in->anyPress) {
            char c = 0;
            if (in->a) c = 'a';
            else if (in->b) { int len = strlen(cp->hex); if (len > 0) { cp->hex[len-1] = 0; cp->cursor--; } }
            if (c && strlen(cp->hex) < 6) {
                int len = strlen(cp->hex);
                cp->hex[len] = c;
                cp->hex[len+1] = 0;
                cp->cursor = len + 1;
            }
        }
    }
}

void cpDraw(ColorPicker* cp) {
    drawPanel(cp->x, cp->y, cp->w, cp->h, 10, g_surfaceColor, 1);
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), "Color Picker", cp->x + cp->w/2, cp->y + 4, 0.4f, 0.4f, g_secTextColor);
    drawHexInput2(cp->x + 10, cp->y + 18, cp->w - 20, 30, cp->hex, cp->active, appGetTime(), g_surfaceColor, g_textColor, 1);

    int ps = 36;
    int px = cp->x + cp->w - ps - 14;
    drawRoundedRect(px, cp->y + 20, ps, ps, 6, cp->color);

    char buf[10];
    snprintf(buf, sizeof(buf), "#%s", cp->hex);
    drawTextCentered(fontsGetBuf(), fontsGetSystem(), buf, cp->x + cp->w/2, cp->y + cp->h - 16, 0.4f, 0.4f, g_textColor);
}
