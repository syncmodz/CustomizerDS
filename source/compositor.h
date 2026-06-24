#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <citro3d.h>
#include <citro2d.h>
#include <stdbool.h>
#include "transitions.h"

/* Compositor de texturas offscreen (PROMPT_CustomizerDS_v8 secao 2).
 *
 * Em vez de cada transicao espalhar "offsets" dentro de cada Render*, cada
 * tela e desenhada INTEIRA numa textura offscreen em VRAM e a transicao vira
 * composicao de 2 imagens: A = snapshot da tela VELHA (estatica, capturada 1x
 * no inicio da troca) e B = tela NOVA (desenhada ao vivo a cada frame). Assim
 * qualquer transicao manipula 2 imagens, o fade-out passa a existir
 * naturalmente (a tela velha some/desliza de verdade) e a tela se move como
 * BLOCO unico (acaba o "emborrachado").
 *
 * Custo VRAM: 4 alvos 512x256 RGBA8 ~= 2 MB (VRAM total do 3DS = 6 MB).
 * Fora de transicao o main.c desenha direto no alvo real -- sem custo
 * offscreen quando ocioso. */

bool compositorInit(void);
void compositorExit(void);
bool compositorReady(void);

/* Alvos offscreen. slotA=true devolve o alvo A (snapshot velha), false o B
 * (nova ao vivo). O caller faz C2D_SceneBegin(...) nesse alvo e desenha a tela
 * inteira (neutra, sem offset de transicao). */
C3D_RenderTarget* compositorTop(bool slotA);
C3D_RenderTarget* compositorBot(bool slotA);

/* Imagens que amostram esses alvos para compor no alvo real, com a subtex ja
 * corrigida pro flip vertical do render-target amostrado como textura
 * (secao 2.3 gotcha 1 -- VALIDAR visualmente no Azahar). */
C2D_Image compositorTopImage(bool slotA);
C2D_Image compositorBotImage(bool slotA);

/* Compoe a tela velha (imgOld=A) e a nova (imgNew=B) no alvo real ATUAL
 * (caller ja fez C2D_SceneBegin no alvo da tela), aplicando as 2 camadas do
 * TransitionFrame (translacao/escala/alpha, ordem de desenho e scrim).
 * isTop seleciona as dimensoes (400x240 / 320x240). */
void compositorComposite(bool isTop, const TransitionFrame* tf,
                         C2D_Image imgOld, C2D_Image imgNew);

#endif
