#ifndef THEME_H
#define THEME_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

// Estrutura da paleta Material Design extraída do wallpaper
typedef struct {
    u32 primary;        // cor principal do wallpaper
    u32 primaryDark;    // primary com V -25% no HSV
    u32 primaryLight;   // primary com V +15% no HSV
    u32 accent;         // hue +150° da primary (complementar)
    u32 surface;        // fundo de cards: R,G,B da primary * 0.12 + base
    u32 surfaceHigh;    // surface um pouco mais claro para hover
    u32 background;     // fundo geral: muito escuro
    u32 backgroundTop;  // fundo tela superior (ligeiramente diferente)
    u32 onPrimary;      // branco ou preto dependendo do contraste
    u32 textPrimary;    // branco puro ou quase
    u32 textSecondary;  // textPrimary com alpha ~60%
    u32 textHint;       // textPrimary com alpha ~35%
    u32 divider;        // linha divisória sutil
    u32 ripple;         // cor do efeito ripple ao toque
    bool isDark;        // luminância da primary < 0.5
} AppTheme;

// Tema global acessível por todos os módulos
extern AppTheme g_theme;

// Inicializa o sistema de tema com paleta padrão
void themeInit(void);

#endif // THEME_H
