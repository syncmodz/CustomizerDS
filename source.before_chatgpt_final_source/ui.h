#ifndef UI_H
#define UI_H

#include "theme.h"
#include <citro2d.h>
#include <stdbool.h>

typedef enum {
    UI_BUTTON_NORMAL = 0,
    UI_BUTTON_SELECTED,
    UI_BUTTON_ACTIVE,
} UIButtonState;

extern float g_enterT;
extern float g_frame;

void uiFrameTick(void);
float uiFrameTime(void);
void uiScreenEnter(void);

void UI_Fill(float x, float y, float w, float h, ColorRGBA color);
void UI_Line(float x, float y, float w, float h, ColorRGBA color);
void UI_RoundRect(float x, float y, float w, float h, float r, ColorRGBA color);
void UI_Shadow(float x, float y, float w, float h);
void UI_Text(C2D_TextBuf buf, C2D_Font font, const char* text,
             float x, float y, float sx, float sy, ColorRGBA color);
void UI_TextRight(C2D_TextBuf buf, C2D_Font font, const char* text,
                  float right, float y, float sx, float sy, ColorRGBA color);
void UI_TextCenter(C2D_TextBuf buf, C2D_Font font, const char* text,
                   float centerX, float y, float sx, float sy, ColorRGBA color);

void UI_TouchBarBackground(void);
void UI_GlassPanel(float x, float y, float w, float h, float r, ColorRGBA bg, ColorRGBA border, ColorRGBA highlight);
void UI_CapsuleButton(C2D_TextBuf buf, C2D_Font font, float x, float y, float w, float h,
                      const char* label, const char* prefix, const char* value,
                      UIButtonState state, float pressedT, float appearT);
void UI_TouchBarPill(float x, float y, float w, float h, ColorRGBA bg, const char* label,
                     C2D_TextBuf buf, C2D_Font font, float sx, float sy);
void UI_MorphSelector(float x, float y, float w, float h, float targetX, float slideT);
void UI_PopupCard(float x, float y, float w, float h, float r, ColorRGBA bg, float scaleT);
void UI_Pill(float x, float y, float w, float h, ColorRGBA bg, const char* label,
             C2D_TextBuf buf, C2D_Font font, float sx, float sy);
void UI_Button(C2D_TextBuf buf, C2D_Font font, float x, float y, float w, float h,
               const char* label, const char* value, UIButtonState state);
void UI_Slider(C2D_TextBuf buf, C2D_Font font, float x, float y, float w,
               const char* label, int value, int min, int max, bool selected);
void UI_Swatch(float x, float y, float w, float h, ColorRGBA color, bool selected);
void UI_KeyHelp(C2D_TextBuf buf, C2D_Font font, const char* left, const char* right);
void UI_RoundSwatch(float x, float y, float r, ColorRGBA color, bool selected, float pulseT);
void UI_SliderModern(C2D_TextBuf buf, C2D_Font font, float x, float y, float w,
                     const char* label, int value, int min, int max, bool selected, float animT);
void UI_Badge(C2D_TextBuf buf, C2D_Font font, float x, float y, const char* text, ColorRGBA color);
void UI_BackgroundTop(void);
void UI_BackgroundBottom(void);
void UI_StartupLogo(float t);

#endif
