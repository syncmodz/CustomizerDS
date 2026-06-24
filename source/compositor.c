#include "compositor.h"
#include "common.h"
#include "ui.h"

static bool s_ready = false;

static C3D_Tex s_texTopA, s_texTopB, s_texBotA, s_texBotB;
static C3D_RenderTarget *s_rtTopA, *s_rtTopB, *s_rtBotA, *s_rtBotB;

/* Top e Bottom cabem em 512x256 (pow2 >= 400x240 e 320x240). A subtex usa V
 * INVERTIDO (top=1.0 > bottom) de proposito: ao amostrar um render-target como
 * textura a imagem sai espelhada na vertical, e inverter as coords V aqui
 * corrige isso sem precisar de scaleY=-1 + offset na hora de desenhar
 * (secao 2.3 gotcha 1). VALIDAR no Azahar -- se sair de cabeca pra baixo,
 * trocar top<->bottom. */
static const Tex3DS_SubTexture s_subTop = {
    400, 240, 0.0f, 1.0f, 400.0f / 512.0f, 1.0f - 240.0f / 256.0f
};
static const Tex3DS_SubTexture s_subBot = {
    320, 240, 0.0f, 1.0f, 320.0f / 512.0f, 1.0f - 240.0f / 256.0f
};

static bool makeTarget(C3D_Tex* tex, C3D_RenderTarget** rt) {
    if (!C3D_TexInitVRAM(tex, 512, 256, GPU_RGBA8)) return false;
    *rt = C3D_RenderTargetCreateFromTex(tex, GPU_TEXFACE_2D, 0, -1);
    if (!*rt) { C3D_TexDelete(tex); return false; }
    /* Blit 1:1 (sem mag/min real), entao linear e seguro e evita serrilhado
     * de borda quando uma transicao escala/desliza a imagem composta. */
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    return true;
}

bool compositorInit(void) {
    if (s_ready) return true;
    if (!makeTarget(&s_texTopA, &s_rtTopA)) return false;
    if (!makeTarget(&s_texTopB, &s_rtTopB)) { compositorExit(); return false; }
    if (!makeTarget(&s_texBotA, &s_rtBotA)) { compositorExit(); return false; }
    if (!makeTarget(&s_texBotB, &s_rtBotB)) { compositorExit(); return false; }
    s_ready = true;
    return true;
}

void compositorExit(void) {
    if (s_rtTopA) { C3D_RenderTargetDelete(s_rtTopA); s_rtTopA = NULL; C3D_TexDelete(&s_texTopA); }
    if (s_rtTopB) { C3D_RenderTargetDelete(s_rtTopB); s_rtTopB = NULL; C3D_TexDelete(&s_texTopB); }
    if (s_rtBotA) { C3D_RenderTargetDelete(s_rtBotA); s_rtBotA = NULL; C3D_TexDelete(&s_texBotA); }
    if (s_rtBotB) { C3D_RenderTargetDelete(s_rtBotB); s_rtBotB = NULL; C3D_TexDelete(&s_texBotB); }
    s_ready = false;
}

bool compositorReady(void) { return s_ready; }

C3D_RenderTarget* compositorTop(bool slotA) { return slotA ? s_rtTopA : s_rtTopB; }
C3D_RenderTarget* compositorBot(bool slotA) { return slotA ? s_rtBotA : s_rtBotB; }

C2D_Image compositorTopImage(bool slotA) {
    C2D_Image img = { slotA ? &s_texTopA : &s_texTopB, &s_subTop };
    return img;
}
C2D_Image compositorBotImage(bool slotA) {
    C2D_Image img = { slotA ? &s_texBotA : &s_texBotB, &s_subBot };
    return img;
}

/* Tint uniforme de alpha: cor branca (RGB inalterado, blend=0) com o alpha
 * pedido nas 4 pontas -- o citro2d multiplica o alpha da textura por este,
 * dando um fade limpo sem mexer na cor. */
static void alphaTint(C2D_ImageTint* tint, float a) {
    u32 c = C2D_Color32f(1.0f, 1.0f, 1.0f, clampf(a, 0.0f, 1.0f));
    for (int i = 0; i < 4; i++) {
        tint->corners[i].color = c;
        tint->corners[i].blend = 0.0f;
    }
}

/* Desenha uma camada de tela inteira com escala ancorada no CENTRO (para o
 * top-left esquerdo C2D_DrawImageAt escala a partir do canto, entao recuamos
 * o canto pra metade do que a escala "comeu"). */
static void drawLayer(C2D_Image img, float W, float H, const TransLayer* L) {
    if (L->alpha <= 0.001f) return;
    C2D_ImageTint tint;
    alphaTint(&tint, L->alpha);
    float sx = L->scaleX, sy = L->scaleY;
    float x = L->x + (W - W * sx) * 0.5f;
    float y = L->y + (H - H * sy) * 0.5f;
    C2D_DrawImageAt(img, x, y, 0.0f, &tint, sx, sy);
}

void compositorComposite(bool isTop, const TransitionFrame* tf,
                         C2D_Image imgOld, C2D_Image imgNew) {
    float W = isTop ? (float)SCREEN_TOP_WIDTH : (float)SCREEN_BOT_WIDTH;
    float H = (float)SCREEN_TOP_HEIGHT; /* ambas as telas tem 240 de altura */

    if (tf->newAbove) {
        drawLayer(imgOld, W, H, &tf->oldL);
        drawLayer(imgNew, W, H, &tf->newL);
    } else {
        drawLayer(imgNew, W, H, &tf->newL);
        drawLayer(imgOld, W, H, &tf->oldL);
    }

    if (tf->scrimAlpha > 0.001f) {
        u32 s = C2D_Color32(0, 0, 0, (u8)(255.0f * clampf(tf->scrimAlpha, 0.0f, 1.0f)));
        C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, W, H, s);
    }

    /* Tier B (6 radial / 7 diagonal): APROXIMACAO decorativa (permitida pela
     * secao 3, "aproxime com mascara circular"). O recorte REAL da tela nova
     * por circulo/faixa exige stencil com mascara de cor (C3D_StencilTest +
     * writemask), que mexe no estado GLOBAL de depth/stencil do citro2d --
     * sem o emulador a mao pra validar o restore, nao arriscamos corromper TODA
     * a UI por 2 transicoes que so aparecem no Laboratorio. Entao: as 2 telas
     * fazem crossfade (transEval) e por cima vai o flash radial / faixa
     * diagonal de accent ja prontos e testados em UI_TransOverlay -- da a
     * leitura de "wipe" sem stencil. TODO(hardware): trocar pelo recorte real
     * via stencil quando der pra validar no Azahar/console. */
    if (tf->reveal == REVEAL_RADIAL) {
        float flashA = 0.35f * (1.0f - clampf(tf->revealT, 0.0f, 1.0f));
        UI_TransOverlay(W, H, 0.0f, flashA, clampf(tf->revealT, 0.0f, 1.0f), 0, 0.0f, 0.0f);
    } else if (tf->reveal == REVEAL_DIAGONAL) {
        UI_TransOverlay(W, H, 0.0f, 0.0f, 0.0f, 0, 1.0f, clampf(tf->revealT, 0.0f, 1.0f));
    }
}

