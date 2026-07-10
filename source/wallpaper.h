#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <citro2d.h>
#include "input.h"

/* 2.0.0: aba Wallpapers/Temas -- aplica tema do Home Menu na extdata
 * (mecanismo do Anemone3DS: BodyCache/BgmCache/ThemeManage + patch de
 * SaveData.dat na home-save). Com ou sem musica (byte[5]=1 + BgmCache).
 * PRECISA de RSF FileSystemAccess (Extdata) + roda SO no console real. */
void wallpaperInit(void);
void wallpaperUpdate(const AppInput* in, float dt, int* currentScreen);
void wallpaperRenderTop(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);
void wallpaperRenderBottom(C2D_TextBuf buf, float transVal, float slideX, float fadeA, float scaleM);

#endif
