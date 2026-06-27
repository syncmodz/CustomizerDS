#ifndef MENU_H
#define MENU_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include "input.h"

void menuInit(void);
void menuUpdate(const AppInput* in, int* currentScreen);
/* slideX/fadeA/scaleM: transicao direcional 3.2 (push entre telas) -- 0/1/1
 * quando nao ha transicao em curso (nenhuma mudanca de comportamento). */
void menuRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
/* §2: render do handoff shared-element boot->home (h = 0..1). Substitui o
 * render normal da tela de cima durante a janela de handoff (ver main.c). */
void menuRenderTopHandoff(C2D_TextBuf buf, float h);
void menuRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
int menuSelected(void);

/* Previa das 3 pilulas de funcao na tela de baixo durante o splash (Herman
 * Miller: varios elementos da cena entram em sequencia, nao so o logo). */
void menuRenderStartupPills(C2D_TextBuf buf, float startupT);

#endif
