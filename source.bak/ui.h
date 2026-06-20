#ifndef UI_H
#define UI_H

#include "theme.h"
#include "anim.h"
#include <citro2d.h>
#include <stdbool.h>

extern float g_enterT;
extern float g_frame;
extern ScreenTransition g_trans;

void uiFrameTick(void);
float uiFrameTime(void);
void uiScreenEnter(void);
float UI_EnterProgress(void);
float UI_EnterSlide(float maxOffset, EaseType ease);

void UI_Fill(float x, float y, float w, float h, ColorRGBA color);
void UI_RoundRect(float x, float y, float w, float h, float r, ColorRGBA color);
void UI_RoundFrame(float x, float y, float w, float h, float r, ColorRGBA fill, ColorRGBA border);
void UI_Shadow(float x, float y, float w, float h, float alpha, float offset);
void UI_GradientV(float x, float y, float w, float h, ColorRGBA top, ColorRGBA bot);

void UI_Text(C2D_TextBuf buf, C2D_Font font, const char* text,
             float x, float y, float sx, float sy, ColorRGBA color);
void UI_TextRight(C2D_TextBuf buf, C2D_Font font, const char* text,
                  float right, float y, float sx, float sy, ColorRGBA color);
void UI_TextCenter(C2D_TextBuf buf, C2D_Font font, const char* text,
                   float centerX, float y, float sx, float sy, ColorRGBA color);

/* === Top Screen (macOS desktop) === */
void UI_TopBackground(void);
void UI_TopMenuBar(const char* title, C2D_TextBuf buf);
void UI_Card(float x, float y, float w, float h, float r, ColorRGBA bg);
void UI_FrostCard(float x, float y, float w, float h, float r);

/* Popup modal (Travel Agency style) */
typedef struct {
    bool active;
    float animT;
    float bgAlpha;
    int result;
    char message[64];
    char confirmLabel[16];
    char cancelLabel[16];
} PopupModal;

void popupShow(PopupModal* p, const char* msg, const char* confirm, const char* cancel);
void popupHide(PopupModal* p);
bool popupActive(PopupModal* p);
void popupUpdate(PopupModal* p);
void popupRender(C2D_TextBuf buf, PopupModal* p);

/* === Bottom Screen (Touch Bar) === */
void UI_BottomBackground(void);
void UI_TouchBarBackground(void);

typedef struct {
    const char* label;
    const char* icon;
    float appearT;
    bool hover;
    float pulsePhase;
} TouchBarButtonState;

void UI_TouchBarPill(C2D_TextBuf buf, float x, float y, float w, float h,
                     const char* label, const char* icon, bool selected, float appearT, float pulse);
void UI_TouchBarRow(C2D_TextBuf buf, const char** labels, const char** icons,
                    int count, int selected, float baseY, float baseAppear);
void UI_TouchBarSegmented(C2D_TextBuf buf, float x, float y, float w, float h,
                          const char** labels, int count, int selected, float morphT);
void UI_TouchBarSlider(C2D_TextBuf buf, float x, float y, float w,
                       const char* label, int value, int min, int max,
                       bool selected, ColorRGBA swatch);
void UI_HelpBar(C2D_TextBuf buf, const char* left, const char* right);

/* Badge, pill, shared */
void UI_Badge(C2D_TextBuf buf, float x, float y, const char* text, ColorRGBA bg);
void UI_PillButton(C2D_TextBuf buf, float x, float y, float w, float h,
                   const char* label, const char* icon, bool selected, float appearT);
void UI_StartupLogo(C2D_TextBuf buf, float t);

#endif
