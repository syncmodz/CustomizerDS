#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define LERP(a, b, t) ((a) + ((b) - (a)) * (t))

// ── BLUR SIMULADO ──────────────────────────────//
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

// ── CARD MATERIAL ──────────────────────────────//
void UI_Card(float x, float y, float w, float h,
             bool selected, float selectAnim) {
    // Glow se selecionado
    if (selectAnim > 0.05f) {
        UI_Glow(x, y, w, h, g_theme.accent, 3);
    }

    // Extração correta de canais RGBA8888 (formato AABBGGRR)
    u8 sr = g_theme.surface & 0xFF;           // R de surface (bits 7-0)
    u8 sg = (g_theme.surface >> 8) & 0xFF;   // G de surface (bits 15-8)
    u8 sb = (g_theme.surface >> 16) & 0xFF;  // B de surface (bits 23-16)
    u8 sa = (g_theme.surface >> 24) & 0xFF;  // A de surface (bits 31-24)

    u8 pr = g_theme.primary & 0xFF;           // R de primary (bits 7-0)
    u8 pg = (g_theme.primary >> 8) & 0xFF;   // G de primary (bits 15-8)
    u8 pb = (g_theme.primary >> 16) & 0xFF;  // B de primary (bits 23-16)
    u8 pa = (g_theme.primary >> 24) & 0xFF;  // A de primary (bits 31-24)

    // Interpolação com LERP
    u8 r = (u8)LERP(sr, pr, selectAnim);
    u8 g = (u8)LERP(sg, pg, selectAnim);
    u8 b = (u8)LERP(sb, pb, selectAnim);
    u8 a = (u8)LERP(sa, pa, selectAnim);

    u32 bgColor = C2D_Color32(r, g, b, a);
    C2D_DrawRectSolid(x, y, 0, w, h, bgColor);

    // Borda accent se selecionado
    if (selected) {
        u32 borderColor = (g_theme.accent & 0x00FFFFFF) | ((u8)(selectAnim * 255) << 24);
        C2D_DrawRectSolid(x, y, 0, w, 1, borderColor);          // Top
        C2D_DrawRectSolid(x, y + h - 1, 0, w, 1, borderColor); // Bottom
        C2D_DrawRectSolid(x, y, 0, 1, h, borderColor);          // Left
        C2D_DrawRectSolid(x + w - 1, y, 0, 1, h, borderColor); // Right
    }
}

// ── ITEM DE LISTA ──────────────────────────────//
void UI_ListItem(C2D_TextBuf buf, float x, float y,
                 float w, float h, const char* label,
                 const char* sublabel,
                 bool selected, float selectAnim,
                 const char* rightLabel) {
    UI_Card(x, y, w, h, selected, selectAnim);
    // Label
    C2D_Text text;
    C2D_TextParse(&text, buf, label);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, 0.3f, x + 10.0f, y + 8.0f, 0.3f, 0.3f, g_theme.textPrimary);
    // Sublabel
    if (sublabel != NULL) {
        C2D_Text sub;
        C2D_TextParse(&sub, buf, sublabel);
        C2D_TextOptimize(&sub);
        C2D_DrawText(&sub, 0.25f, x + 10.0f, y + 28.0f, 0.25f, 0.25f, g_theme.textSecondary);
    }
    // Right label
    if (rightLabel != NULL) {
        C2D_Text rlabel;
        C2D_TextParse(&rlabel, buf, rightLabel);
        C2D_TextOptimize(&rlabel);
        float tw = 0, th = 0;
        C2D_TextGetDimensions(&rlabel, 0.25f, 0.25f, &tw, &th);
        C2D_DrawText(&rlabel, 0.25f, x + w - tw - 10.0f, y + (h - th) / 2, 0.25f, 0.25f, g_theme.textSecondary);
    }
}

// ── HEADER DO MÓDULO ───────────────────────────//
void UI_Header(C2D_TextBuf buf, const char* title,
               const char* subtitle) {
    // Fundo: gradient de primaryDark → primary (simulado com 2 retângulos)
    C2D_DrawRectSolid(0, 0, 0, 200, 44, g_theme.primaryDark);
    C2D_DrawRectSolid(200, 0, 0, 200, 44, g_theme.primary);
    // Ícone "●" (bolinha accent)
    C2D_Text icon;
    C2D_TextParse(&icon, buf, "●");
    C2D_TextOptimize(&icon);
    C2D_DrawText(&icon, 0.4f, 10.0f, 14.0f, 0.4f, 0.4f, g_theme.accent);
    // Título
    C2D_Text t;
    C2D_TextParse(&t, buf, title);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, 0.5f, 30.0f, 8.0f, 0.5f, 0.5f, g_theme.onPrimary);
    // Subtítulo
    if (subtitle != NULL) {
        C2D_Text s;
        C2D_TextParse(&s, buf, subtitle);
        C2D_TextOptimize(&s);
        C2D_DrawText(&s, 0.28f, 30.0f, 32.0f, 0.28f, 0.28f, g_theme.textSecondary);
    }
    // Linha accent
    C2D_DrawRectSolid(0, 42, 0, 400, 2, g_theme.accent);
}

// ── RODAPÉ DA TELA INFERIOR ──────────────────//
void UI_Footer(C2D_TextBuf buf,
               const char* aLabel,
               const char* bLabel,
               const char* startLabel) {
    // Fundo surface
    C2D_DrawRectSolid(0, 190, 0, 320, 50, (g_theme.surface & 0x00FFFFFF) | (220 << 24));
    // Botões
    float bx = 10.0f;
    if (aLabel != NULL) {
        C2D_DrawRectSolid(bx, 200, 0, 10, 10, g_theme.primary);
        C2D_Text a;
        C2D_TextParse(&a, buf, "A");
        C2D_TextOptimize(&a);
        C2D_DrawText(&a, 0.28f, bx + 2.0f, 198.0f, 0.28f, 0.28f, g_theme.onPrimary);
        C2D_Text al;
        C2D_TextParse(&al, buf, aLabel);
        C2D_TextOptimize(&al);
        C2D_DrawText(&al, 0.26f, bx + 14.0f, 207.0f, 0.26f, 0.26f, g_theme.textSecondary);
        bx += 100.0f;
    }
    if (bLabel != NULL) {
        C2D_DrawRectSolid(bx, 200, 0, 10, 10, g_theme.primary);
        C2D_Text b;
        C2D_TextParse(&b, buf, "B");
        C2D_TextOptimize(&b);
        C2D_DrawText(&b, 0.28f, bx + 2.0f, 198.0f, 0.28f, 0.28f, g_theme.onPrimary);
        C2D_Text bl;
        C2D_TextParse(&bl, buf, bLabel);
        C2D_TextOptimize(&bl);
        C2D_DrawText(&bl, 0.26f, bx + 14.0f, 207.0f, 0.26f, 0.26f, g_theme.textSecondary);
        bx += 100.0f;
    }
    if (startLabel != NULL) {
        C2D_Text sl;
        C2D_TextParse(&sl, buf, startLabel);
        C2D_TextOptimize(&sl);
        C2D_DrawText(&sl, 0.26f, 260.0f, 207.0f, 0.26f, 0.26f, g_theme.textSecondary);
    }
}

// ── RIPPLE ─────────────────────────────────────//
void rippleStart(Ripple* r, float x, float y, float maxR) {
    r->x = x; r->y = y; r->radius = 0; r->maxRadius = maxR;
    r->alpha = 220.0f; r->active = true;
}
void rippleStep(Ripple* r) {
    if (!r->active) return;
    r->radius += r->maxRadius * 0.08f;
    r->alpha -= 14.0f;
    if (r->alpha <= 0.0f || r->radius >= r->maxRadius) { r->active = false; }
}
void rippleDraw(Ripple* r, u32 color) {
    if (!r->active) return;
    u32 c = (color & 0x00FFFFFF) | ((u8)r->alpha << 24);
    // Aproximação de círculo com retângulo centralizado
    C2D_DrawRectSolid(r->x - r->radius, r->y - r->radius, 0,
                          r->radius * 2, r->radius * 2, c);
}

// ── SWITCH / TOGGLE ──────────────────────────────//
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
    // Extração correta de canais RGBA8888 (formato AABBGGRR) para textHint e primary
    u8 thr = g_theme.textHint & 0xFF;           // R de textHint (bits 7-0)
    u8 thg = (g_theme.textHint >> 8) & 0xFF;   // G de textHint (bits 15-8)
    u8 thb = (g_theme.textHint >> 16) & 0xFF;  // B de textHint (bits 23-16)
    u8 tha = (g_theme.textHint >> 24) & 0xFF;  // A de textHint (bits 31-24)

    u8 pr = g_theme.primary & 0xFF;           // R de primary (bits 7-0)
    u8 pg = (g_theme.primary >> 8) & 0xFF;   // G de primary (bits 15-8)
    u8 pb = (g_theme.primary >> 16) & 0xFF;  // B de primary (bits 23-16)
    u8 pa = (g_theme.primary >> 24) & 0xFF;  // A de primary (bits 31-24)

    u8 r = (u8)LERP(thr, pr, s->colorAnim.value);
    u8 g = (u8)LERP(thg, pg, s->colorAnim.value);
    u8 b = (u8)LERP(thb, pb, s->colorAnim.value);
    u8 a = (u8)LERP(tha, pa, s->colorAnim.value);

    u32 trackColor = C2D_Color32(r, g, b, a);
    C2D_DrawRectSolid(x, y, 0, 36, 20, trackColor);

    // Thumb: 16x16 (squish ao mover)
    float squish = 1.0f + 0.15f * (1.0f - fabsf(s->thumbAnim.value * 2.0f - 1.0f));
    float tx = LERP(x + 2.0f, x + 18.0f, s->thumbAnim.value);
    C2D_DrawRectSolid(tx, y + 2.0f, 0, 16.0f * squish, 16, g_theme.onPrimary);
}

// ── BARRA DE PROGRESSO / LOADING ───────────────//
void uiLoaderStep(UILoader* l) {
    l->phase += 0.04f;
    if (l->phase > 1.0f) l->phase -= 1.0f;
}
void uiLoaderDraw(UILoader* l, float x, float y, float w) {
    float segW = w / 3.0f;
    for (int i = 0; i < 3; i++) {
        float segX = x + (i * segW) + (l->phase * segW * 3.0f);
        if (segX > x + w) segX -= w;

        // Extração correta de canais RGBA8888
        u8 sr = (g_theme.surface >> 24) & 0xFF;
        u8 sg = (g_theme.surface >> 16) & 0xFF;
        u8 sb = (g_theme.surface >> 8)  & 0xFF;

        u8 plr = (g_theme.primaryLight >> 24) & 0xFF;
        u8 plg = (g_theme.primaryLight >> 16) & 0xFF;
        u8 plb = (g_theme.primaryLight >> 8)  & 0xFF;

        u8 r = (u8)LERP(sr, plr, l->phase);
        u8 g = (u8)LERP(sg, plg, l->phase);
        u8 b = (u8)LERP(sb, plb, l->phase);

        u32 c = C2D_Color32(r, g, b, 255);
        C2D_DrawRectSolid(segX, y, 0, segW, 4, c);
    }
}

// ── SEPARADOR ──────────────────────────────────//
void UI_Divider(float x, float y, float w) {
    C2D_DrawRectSolid(x, y, 0, w, 1, g_theme.divider);
}

// ── TEXTO COM TRUNCAMENTO ──────────────────────//
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
    // Cortar texto e adicionar "..."
    char* ellipsisText = malloc(strlen(text) + 4); // text + "..."
    if (!ellipsisText) return;
    strcpy(ellipsisText, text);
    while (strlen(ellipsisText) > 0) {
        ellipsisText[strlen(ellipsisText) - 1] = '\0';
        C2D_Text et;
        C2D_TextParse(&et, buf, ellipsisText);
        C2D_TextOptimize(&et);
        float etw = 0;
        C2D_TextGetDimensions(&et, scaleX, scaleY, &etw, NULL);
        if (etw + 3 * scaleX * 8 < maxWidth) break; // ~3 chars for "..."
    }
    strcat(ellipsisText, "...");
    C2D_Text et;
    C2D_TextParse(&et, buf, ellipsisText);
    C2D_TextOptimize(&et);
    C2D_DrawText(&et, scaleX, x, y, scaleX, scaleY, color);
    free(ellipsisText);
}
