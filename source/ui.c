#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static inline u32 RGBA32(u8 r, u8 g, u8 b, u8 a) {
    return C2D_Color32(r, g, b, a);
}

static inline u32 colorToU32(ColorRGBA c) {
    return C2D_Color32(c.r, c.g, c.b, c.a);
}

static inline ColorRGBA lerpColor(ColorRGBA a, ColorRGBA b, float t) {
    ColorRGBA r;
    r.r = (u8)(a.r + (b.r - a.r) * t);
    r.g = (u8)(a.g + (b.g - a.g) * t);
    r.b = (u8)(a.b + (b.b - a.b) * t);
    r.a = (u8)(a.a + (b.a - a.a) * t);
    return r;
}

void UI_Glow(float x, float y, float w, float h,
             u32 color, int layers) {
    if (layers <= 0) return;
    for (int i = layers - 1; i >= 0; i--) {
        float inset = (layers - i) * 3.0f;
        u8 alpha = (u8)(i * (220.0f / layers));
        u32 layerColor = (color & 0x00FFFFFF) | ((u32)alpha << 24);
        C2D_DrawRectSolid(x - inset, y - inset, 0,
                          w + inset * 2, h + inset * 2, layerColor);
    }
}

void UI_Card(float x, float y, float w, float h,
             bool selected, float selectAnim) {
    if (selectAnim > 0.05f) {
        UI_Glow(x, y, w, h, colorToU32(g_theme.accent), 3);
    }

    ColorRGBA bg = lerpColor(g_theme.surface, g_theme.primary, selectAnim);
    u32 bgColor = colorToU32(bg);
    C2D_DrawRectSolid(x, y, 0, w, h, bgColor);

    if (selected) {
        u32 borderColor = (colorToU32(g_theme.accent) & 0x00FFFFFF) | ((u8)(selectAnim * 255) << 24);
        C2D_DrawRectSolid(x, y, 0, w, 1, borderColor);
        C2D_DrawRectSolid(x, y + h - 1, 0, w, 1, borderColor);
        C2D_DrawRectSolid(x, y, 0, 1, h, borderColor);
        C2D_DrawRectSolid(x + w - 1, y, 0, 1, h, borderColor);
    }
}

void UI_ListItem(C2D_TextBuf buf, float x, float y,
                 float w, float h, const char* label,
                 const char* sublabel,
                 bool selected, float selectAnim,
                 const char* rightLabel) {
    UI_Card(x, y, w, h, selected, selectAnim);
    C2D_Text text;
    C2D_TextParse(&text, buf, label);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, 0.3f, x + 10.0f, y + 8.0f, 0.3f, 0.3f, colorToU32(g_theme.textPrimary));
    if (sublabel) {
        C2D_Text sub;
        C2D_TextParse(&sub, buf, sublabel);
        C2D_TextOptimize(&sub);
        C2D_DrawText(&sub, 0.25f, x + 10.0f, y + 28.0f, 0.25f, 0.25f, colorToU32(g_theme.textSecondary));
    }
    if (rightLabel) {
        C2D_Text rlabel;
        C2D_TextParse(&rlabel, buf, rightLabel);
        C2D_TextOptimize(&rlabel);
        float tw = 0, th = 0;
        C2D_TextGetDimensions(&rlabel, 0.25f, 0.25f, &tw, &th);
        C2D_DrawText(&rlabel, 0.25f, x + w - tw - 10.0f, y + (h - th) / 2, 0.25f, 0.25f, colorToU32(g_theme.textSecondary));
    }
}

void UI_Header(C2D_TextBuf buf, const char* title,
               const char* subtitle) {
    C2D_DrawRectSolid(0, 0, 0, 200, 44, colorToU32(g_theme.primaryDark));
    C2D_DrawRectSolid(200, 0, 0, 200, 44, colorToU32(g_theme.primary));
    C2D_Text icon;
    C2D_TextParse(&icon, buf, "\xe2\x97\x8f");
    C2D_TextOptimize(&icon);
    C2D_DrawText(&icon, 0.4f, 10.0f, 14.0f, 0.4f, 0.4f, colorToU32(g_theme.accent));
    C2D_Text t;
    C2D_TextParse(&t, buf, title);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, 0.5f, 30.0f, 8.0f, 0.5f, 0.5f, colorToU32(g_theme.onPrimary));
    if (subtitle) {
        C2D_Text s;
        C2D_TextParse(&s, buf, subtitle);
        C2D_TextOptimize(&s);
        C2D_DrawText(&s, 0.28f, 30.0f, 32.0f, 0.28f, 0.28f, colorToU32(g_theme.textSecondary));
    }
    C2D_DrawRectSolid(0, 42, 0, 400, 2, colorToU32(g_theme.accent));
}

void UI_Footer(C2D_TextBuf buf,
               const char* aLabel,
               const char* bLabel,
               const char* startLabel) {
    ColorRGBA bg = g_theme.surface;
    bg.a = 220;
    C2D_DrawRectSolid(0, 190, 0, 320, 50, colorToU32(bg));
    float bx = 10.0f;
    if (aLabel) {
        C2D_DrawRectSolid(bx, 200, 0, 10, 10, colorToU32(g_theme.primary));
        C2D_Text a;
        C2D_TextParse(&a, buf, "A");
        C2D_TextOptimize(&a);
        C2D_DrawText(&a, 0.28f, bx + 2.0f, 198.0f, 0.28f, 0.28f, colorToU32(g_theme.onPrimary));
        C2D_Text al;
        C2D_TextParse(&al, buf, aLabel);
        C2D_TextOptimize(&al);
        C2D_DrawText(&al, 0.26f, bx + 14.0f, 207.0f, 0.26f, 0.26f, colorToU32(g_theme.textSecondary));
        bx += 100.0f;
    }
    if (bLabel) {
        C2D_DrawRectSolid(bx, 200, 0, 10, 10, colorToU32(g_theme.primary));
        C2D_Text b;
        C2D_TextParse(&b, buf, "B");
        C2D_TextOptimize(&b);
        C2D_DrawText(&b, 0.28f, bx + 2.0f, 198.0f, 0.28f, 0.28f, colorToU32(g_theme.onPrimary));
        C2D_Text bl;
        C2D_TextParse(&bl, buf, bLabel);
        C2D_TextOptimize(&bl);
        C2D_DrawText(&bl, 0.26f, bx + 14.0f, 207.0f, 0.26f, 0.26f, colorToU32(g_theme.textSecondary));
        bx += 100.0f;
    }
    if (startLabel) {
        C2D_Text sl;
        C2D_TextParse(&sl, buf, startLabel);
        C2D_TextOptimize(&sl);
        C2D_DrawText(&sl, 0.26f, 260.0f, 207.0f, 0.26f, 0.26f, colorToU32(g_theme.textSecondary));
    }
}

void rippleStart(Ripple* r, float x, float y, float maxR) {
    r->x = x; r->y = y; r->radius = 0; r->maxRadius = maxR;
    r->alpha = 220.0f; r->active = true;
}

void rippleStep(Ripple* r) {
    if (!r->active) return;
    r->radius += r->maxRadius * 0.08f;
    r->alpha -= 14.0f;
    if (r->alpha <= 0.0f || r->radius >= r->maxRadius) r->active = false;
}

void rippleDraw(Ripple* r, u32 color) {
    if (!r->active) return;
    u32 c = (color & 0x00FFFFFF) | ((u8)r->alpha << 24);
    C2D_DrawRectSolid(r->x - r->radius, r->y - r->radius, 0,
                      r->radius * 2, r->radius * 2, c);
}

void uiSwitchInit(UISwitch* s, bool initialValue) {
    s->value = initialValue;
    animSet(&s->thumbAnim, initialValue ? 1.0f : 0.0f, 0.18f);
    animSet(&s->colorAnim, initialValue ? 1.0f : 0.0f, 0.18f);
}

void uiSwitchToggle(UISwitch* s) {
    s->value = !s->value;
    animTo(&s->thumbAnim, s->value ? 1.0f : 0.0f);
    animTo(&s->colorAnim, s->value ? 1.0f : 0.0f);
}

void uiSwitchStep(UISwitch* s) {
    animStep(&s->thumbAnim);
    animStep(&s->colorAnim);
}

void uiSwitchDraw(UISwitch* s, float x, float y) {
    ColorRGBA track = lerpColor(g_theme.textHint, g_theme.primary, s->colorAnim.value);
    u32 trackColor = colorToU32(track);
    C2D_DrawRectSolid(x, y, 0, 36, 20, trackColor);

    float squish = 1.0f + 0.15f * (1.0f - fabsf(s->thumbAnim.value * 2.0f - 1.0f));
    float tx = (x + 2.0f) + (x + 18.0f - (x + 2.0f)) * s->thumbAnim.value;
    C2D_DrawRectSolid(tx, y + 2.0f, 0, 16.0f * squish, 16, colorToU32(g_theme.onPrimary));
}

void uiLoaderStep(UILoader* l) {
    l->phase += 0.04f;
    if (l->phase > 1.0f) l->phase -= 1.0f;
}

void uiLoaderDraw(UILoader* l, float x, float y, float w) {
    float segW = w / 3.0f;
    for (int i = 0; i < 3; i++) {
        float segX = x + (i * segW) + (l->phase * segW * 3.0f);
        if (segX > x + w) segX -= w;
        ColorRGBA c = lerpColor(g_theme.surface, g_theme.primaryLight, l->phase);
        C2D_DrawRectSolid(segX, y, 0, segW, 4, colorToU32(c));
    }
}

void UI_Divider(float x, float y, float w) {
    C2D_DrawRectSolid(x, y, 0, w, 1, colorToU32(g_theme.divider));
}

void UI_TextEllipsis(C2D_TextBuf buf, const char* text,
                     float x, float y, float scaleX, float scaleY,
                     u32 color, float maxWidth) {
    C2D_Text t;
    C2D_TextParse(&t, buf, text);
    C2D_TextOptimize(&t);
    float tw = 0, th = 0;
    C2D_TextGetDimensions(&t, scaleX, scaleY, &tw, &th);
    if (tw <= maxWidth) {
        C2D_DrawText(&t, scaleX, x, y, scaleX, scaleY, color);
        return;
    }
    char* buf2 = malloc(strlen(text) + 4);
    if (!buf2) return;
    strcpy(buf2, text);
    while (strlen(buf2) > 0) {
        buf2[strlen(buf2) - 1] = '\0';
        C2D_Text et;
        C2D_TextParse(&et, buf, buf2);
        C2D_TextOptimize(&et);
        float etw = 0;
        C2D_TextGetDimensions(&et, scaleX, scaleY, &etw, NULL);
        if (etw + 3 * scaleX * 8 < maxWidth) break;
    }
    strcat(buf2, "...");
    C2D_Text et;
    C2D_TextParse(&et, buf, buf2);
    C2D_TextOptimize(&et);
    C2D_DrawText(&et, scaleX, x, y, scaleX, scaleY, color);
    free(buf2);
}
