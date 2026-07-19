#ifndef LANG_H_
#define LANG_H_

#include <3ds.h>

/* i18n (PROMPT v9.1 §i18n): PT, EN, ES. O chrome usa a FONTE DE SISTEMA
 * (ui.c, font=NULL), entao acentos de PT/ES ( a~, c-cedilha, n~, ¿, ¡)
 * renderizam normalmente. Nomes proprios (CustomizerDS, nomes das fontes,
 * "HEX", "RGB") e amostras de fonte NAO sao traduzidos. */
typedef enum { LANG_PT = 0, LANG_EN, LANG_ES, LANG_COUNT } Lang;

typedef enum {
    /* abas / titulos */
    STR_TAB_HOME = 0,
    STR_TAB_FONTS,
    STR_TAB_THEME,
    STR_TAB_LED,
    STR_TAB_SPLASH,
    /* home */
    STR_HOME_SLOGAN,
    STR_HOME_HINT,
    STR_NAV_FONTS,
    STR_NAV_THEME,
    STR_NAV_LED,
    STR_ITEM_FONTS,
    STR_ITEM_THEME,
    STR_ITEM_LED,
    STR_ITEM_FONTS_SUB,
    STR_ITEM_THEME_SUB,
    STR_ITEM_LED_SUB,
    STR_ITEM_SPLASH,
    STR_ITEM_SPLASH_SUB,
    /* 2.0.0 splash */
    STR_SPLASH_EMPTY,
    STR_SPLASH_APPLY,
    STR_SPLASH_REMOVE,
    STR_SPLASH_CONFIRM,
    STR_SPLASH_DONE,
    STR_SPLASH_REMOVED,
    STR_SPLASH_CFG_WARN,
    STR_SPLASH_HELP_L,
    STR_SPLASH_CURRENT,
    /* 2.0.0 wallpaper/tema */
    STR_TAB_WALL,
    STR_ITEM_WALL,
    STR_ITEM_WALL_SUB,
    STR_WALL_EMPTY,
    STR_WALL_CONFIRM,
    STR_WALL_CONFIRM_MUSIC,
    STR_WALL_DONE,
    STR_WALL_FAIL,
    STR_WALL_NOEXT,
    STR_WALL_HWONLY,
    STR_WALL_HASMUSIC,
    STR_WALL_NOMUSIC,
    STR_WALL_TOGGLE_MUSIC,
    STR_WALL_HELP_L,
    STR_WALL_APPLYING,
    STR_WALL_REBOOT,
    /* 2.0.0 badges */
    STR_TAB_BADGE,
    STR_ITEM_BADGE,
    STR_ITEM_BADGE_SUB,
    STR_BADGE_EMPTY,
    STR_BADGE_CONFIRM,
    STR_BADGE_DONE,
    STR_BADGE_FAIL,
    STR_BADGE_NOEXT,
    STR_BADGE_HWONLY,
    STR_BADGE_COUNT,
    STR_BADGE_HELP_L,
    STR_BADGE_INSTALLING,
    /* 2.0.0 home ui (layeredfs) */
    STR_TAB_HOMEUI,
    STR_ITEM_HOMEUI,
    STR_ITEM_HOMEUI_SUB,
    STR_HOMEUI_EMPTY,
    STR_HOMEUI_CONFIRM,
    STR_HOMEUI_REMOVE,
    STR_HOMEUI_DONE,
    STR_HOMEUI_REMOVED,
    STR_HOMEUI_FAIL,
    STR_HOMEUI_WARN,
    STR_HOMEUI_REGION,
    STR_HOMEUI_HELP_L,
    STR_HOMEUI_REBOOT,
    /* editor de cor do HUD */
    STR_HUD_EDIT_ROW,
    STR_HUD_TITLE,
    STR_HUD_BATTERY,
    STR_HUD_CLOCK,
    STR_HUD_COINS,
    STR_HUD_STEPS,
    STR_HUD_WIFI,
    STR_HUD_APPLY,
    STR_HUD_RESET,
    STR_HUD_APPLIED,
    STR_HUD_NOBASE,
    STR_HUD_HELP,
    STR_HUD_PICK_HELP,
    STR_HUD_SEC_PRESETS,
    STR_HUD_SEC_ELEMENTS,
    STR_HUD_SEC_ACTIONS,
    STR_HUD_TAB_PRESETS,
    STR_HUD_TAB_ELEMENTS,
    STR_HUD_REBOOTING,
    STR_HUD_WARN_PATCHING,
    STR_HUD_WARN_NOCFG,
    STR_HUD_WRITE_FAIL,
    /* comuns */
    STR_PREVIEW,
    STR_SAIR,           /* "START sair" */
    STR_IN_USE,
    STR_TO_APPLY,
    STR_YES,
    STR_NO,
    /* help bars */
    STR_HELP_HOME_L,    /* "A abrir" */
    STR_HELP_FONTS_L,   /* "A aplicar" */
    STR_HELP_THEME1_L,  /* "cima: espectro  esq/dir valor" */
    STR_HELP_THEME1_R,  /* "A aplicar  B voltar" */
    STR_HELP_THEME2_L,  /* "<- -> alterna tema/escolhe accent" */
    STR_HELP_THEME2_R,  /* "A confirma  START sair" */
    STR_HELP_LED_L,     /* "A modo  B voltar  <- valor ->" */
    /* fontes */
    STR_CONFIRM_FONT,
    /* tema */
    STR_HEX_TITLE,
    STR_HEX_HINT,
    STR_APPEARANCE,
    STR_LIGHT,
    STR_DARK,
    STR_ACCENT,
    STR_CUSTOM,
    STR_CONFIRM_COLOR,
    STR_LANGUAGE,
    /* led */
    STR_LED_STATIC,
    STR_LED_PULSE,
    STR_LED_OFF,
    STR_LED_ACTIVE,
    STR_LED_SIM,
    STR_RED,
    STR_GREEN,
    STR_BLUE,
    STR_SPEED,
    STR_SPEED_SHORT,
    STR_DEPTH_SHORT,   /* 1.5.0: rotulo curto do slider de profundidade do pulso */
    STR_STATE,
    STR_REALTIME,
    STR_SIMULATED,
    /* fontes */
    STR_FONT_SYSTEM,
    STR_SYSFONT_CONFIRM,  /* popup A/B ao aplicar como fonte do sistema */
    STR_SYSFONT_RESTORE,  /* popup A/B ao restaurar a fonte original */
    STR_SYSFONT_DONE,     /* instalado, reiniciando */
    STR_SYSFONT_FAIL,     /* falhou (tamanho/escrita) */
    STR_SYSFONT_INSTALLING, /* tela de progresso do install */
    STR_SYSFONT_KEEPON,   /* 1.4.0: tela de baixo durante install ("nao desligue") */
    STR_SYSFONT_REBOOT_END, /* 1.4.0: subtitulo ("reiniciando ao concluir...") */
    STR_SYSFONT_EMU_DONE, /* 1.4.0 PART4: titulo do install simulado no Azahar */
    STR_SYSFONT_EMU_NOTE, /* 1.4.0 PART4: aviso honesto (so aplica no console) */
    STR_HELP_FONTS_X,     /* "X fonte do sistema" */
    /* startup */
    STR_STARTUP_SLOGAN,
    STR_COUNT
} StringId;

/* Inicializa o idioma: se savedLang for um valor valido (0..LANG_COUNT-1) usa
 * ele; senao (sentinela CFG_LANG_UNSET / ausencia de config) pega o idioma do
 * SISTEMA via CFGU, caindo em LANG_EN se nao for PT/EN/ES. */
void langInit(int savedLang);
void langSet(Lang l);
Lang langGet(void);
/* Nunca retorna NULL: cai pra EN, depois pra "?" (cobre o item de cobertura
 * i18n da §13). */
const char* T(StringId id);

#endif
