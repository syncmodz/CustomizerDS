#ifndef THEME_H
#define THEME_H

#include <3ds.h>
#include <stdbool.h>

typedef struct {
    u8 r, g, b, a;
} ColorRGBA;

typedef struct {
    ColorRGBA background;
    ColorRGBA backgroundTop;
    ColorRGBA surface;
    ColorRGBA surfaceAlt;
    ColorRGBA surfaceElevated;
    ColorRGBA primary;
    ColorRGBA primaryLight;
    ColorRGBA primaryDark;
    ColorRGBA accent;
    ColorRGBA success;
    ColorRGBA warning;
    ColorRGBA onPrimary;
    ColorRGBA textPrimary;
    ColorRGBA textSecondary;
    ColorRGBA textHint;
    ColorRGBA divider;
} ThemePalette;

extern ThemePalette g_theme;

/* Tokens v3 (PROMPT_CustomizerDS_v3 secao 2): raio e espacamento
 * centralizados aqui -- nenhuma tela deve usar numero solto pra isso.
 * "pill full" nao tem constante porque e sempre h/2, calculado por widget. */
static const float RADIUS_CARD = 24.0f;
static const float RADIUS_CHIP = 14.0f; /* chip/segment */
static const float SPACE_4  = 4.0f;
static const float SPACE_8  = 8.0f;
static const float SPACE_12 = 12.0f;
static const float SPACE_16 = 16.0f;
static const float SPACE_24 = 24.0f;
static const float SPACE_32 = 32.0f;

void themeInit(void);
void themeSetDark(bool dark);
bool themeIsDark(void);
void themeSetAccentIndex(int index);
int themeGetAccentIndex(void);
int themeAccentCount(void);
const char* themeAccentName(int index);
ColorRGBA themeAccentColor(int index);
u32 themeColor(ColorRGBA color);
ColorRGBA themeMix(ColorRGBA a, ColorRGBA b, float t);

/* Cor de texto com contraste seguro sobre qualquer fundo (formula de luminancia real,
 * nao a soma crua de canais) -- usar sempre que o fundo nao for diretamente uma cor
 * do tema (ex: fundo fixo, accent, ou cor escolhida pelo usuario). */
ColorRGBA themeContrastText(ColorRGBA bg);

/* accent_glow do contrato de design: accent @ 35% alpha, para halo/sombra
 * do elemento ativo (docs/design-tokens.md). */

/* Tokens v3: accent_soft = accent @ 14% (tint sutil Monet, halos contidos
 * dentro do raio do card) e bg_card_sel = mix(bg_card, accent, 8%) (card
 * selecionado ganha um "vazamento" sutil da cor de destaque, nao um fill
 * solido). Ver docs/design-tokens.md. */
ColorRGBA themeAccentSoft(void);
ColorRGBA themeCardSelBg(void);

/* Accent customizado via hexadecimal (#RRGGBB), independente dos presets */
bool themeAccentIsCustom(void);
ColorRGBA themeGetCustomAccent(void);
void themeSetCustomAccent(ColorRGBA c);

/* Transicao "yin-yang" (spec v5 6): ao alternar Claro/Escuro, o fundo das
 * duas telas faz um wipe radial saindo do icone que disparou a troca,
 * "engolindo" a cor antiga em vez de cortar abrupto -- UI_TopBackground/
 * UI_BottomBackground (ui.c) leem este estado e o icone sol/lua e desenhado
 * por cima (darkmode.c) girando + trocando de sprite na metade do giro.
 * Vive em theme.c (nao darkmode.c) porque UI_*Background e chamado por
 * QUALQUER tela, nao so a de Tema -- mas so darkmode.c pode disparar (e o
 * unico lugar que chama themeSetDark). */
#define THEME_WIPE_DUR 0.62f /* spec v5 6: 500-700ms */
typedef struct {
    bool active;
    float t; /* segundos acumulados, 0..THEME_WIPE_DUR */
    ColorRGBA oldBgTop;
    ColorRGBA oldBgBot;
    float originTopX, originTopY;
    float originBotX, originBotY;
    bool wasDark; /* tema ANTES da troca -- p/ telas de baixo recriarem as
                    * bandas touch-bar/conteudo antigas (nao fazem parte de
                    * ThemePalette, sao calculadas direto em ui.c). */
} ThemeWipe;

/* Chamar ANTES de themeSetDark() -- captura a cor de fundo ATUAL (que vai
 * ser "engolida") antes que g_theme mude. */
void themeWipeTrigger(float originTopX, float originTopY, float originBotX, float originBotY);
void themeWipeTick(float dt);
ThemeWipe* themeWipeGet(void);

#endif
