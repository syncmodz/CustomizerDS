#include "lang.h"

static Lang s_lang = LANG_EN;

/* Tabela PT / EN / ES. Mantida curta o suficiente pras help bars/botoes nao
 * estourarem (§i18n.9); onde o ES/PT e mais longo, a string ja foi pensada
 * pra caber no mesmo espaco do EN. */
static const char* STRINGS[LANG_COUNT][STR_COUNT] = {
    /* ===================== LANG_PT ===================== */
    [LANG_PT] = {
        [STR_TAB_HOME] = "Inicio",
        [STR_TAB_FONTS] = "Fontes",
        [STR_TAB_THEME] = "Tema",
        [STR_TAB_LED] = "LED",
        [STR_HOME_SLOGAN] = "personalize seu console",
        [STR_HOME_HINT] = "Navegue com as setas ou toque na tela",
        [STR_NAV_FONTS] = "Fontes",
        [STR_NAV_THEME] = "Tema",
        [STR_NAV_LED] = "LED",
        [STR_ITEM_FONTS] = "Fontes",
        [STR_ITEM_THEME] = "Tema do app",
        [STR_ITEM_LED] = "LED RGB",
        [STR_ITEM_FONTS_SUB] = "personalizar",
        [STR_ITEM_THEME_SUB] = "modo e cor",
        [STR_ITEM_LED_SUB] = "cor e luz",
        [STR_PREVIEW] = "Preview",
        [STR_SAIR] = "START sair",
        [STR_IN_USE] = "em uso",
        [STR_TO_APPLY] = "A para aplicar",
        [STR_YES] = "Sim",
        [STR_NO] = "Nao",
        [STR_HELP_HOME_L] = "A abrir",
        [STR_HELP_FONTS_L] = "A previa",
        [STR_HELP_THEME1_L] = "cima: espectro  esq/dir valor",
        [STR_HELP_THEME1_R] = "A aplicar  B voltar",
        [STR_HELP_THEME2_L] = "<- -> tema/accent",
        [STR_HELP_THEME2_R] = "A confirma  START sair",
        [STR_HELP_LED_L] = "A modo  B voltar  <- valor ->",
        [STR_CONFIRM_FONT] = "Aplicar essa fonte no seu dispositivo?",
        [STR_HEX_TITLE] = "Cor customizada (hex)",
        [STR_HEX_HINT] = "toque/arraste no espectro ou digite o hex",
        [STR_APPEARANCE] = "Aparencia",
        [STR_LIGHT] = "Claro",
        [STR_DARK] = "Escuro",
        [STR_ACCENT] = "Accent",
        [STR_CUSTOM] = "Custom",
        [STR_CONFIRM_COLOR] = "Aplicar esta cor como tema?",
        [STR_LANGUAGE] = "Idioma",
        [STR_LED_STATIC] = "Fixo",
        [STR_LED_PULSE] = "Pulso",
        [STR_LED_OFF] = "Off",
        [STR_LED_ACTIVE] = "led ativo",
        [STR_LED_SIM] = "simulado (sem MCU)",
        [STR_RED] = "Vermelho",
        [STR_GREEN] = "Verde",
        [STR_BLUE] = "Azul",
        [STR_SPEED] = "Velocidade",
        [STR_SPEED_SHORT] = "Vel",
        [STR_STATE] = "Estado",
        [STR_REALTIME] = "Tempo real",
        [STR_SIMULATED] = "Simulado",
        [STR_FONT_SYSTEM] = "Padrao do Sistema",
        [STR_SYSFONT_CONFIRM] = "Aplicar fonte do sistema? Vai reiniciar (backup NAND).",
        [STR_SYSFONT_RESTORE] = "Restaurar a fonte ORIGINAL? O console vai REINICIAR.",
        [STR_SYSFONT_DONE] = "Fonte instalada. Reiniciando...",
        [STR_SYSFONT_FAIL] = "Falha ao instalar (so funciona no console real).",
        [STR_SYSFONT_INSTALLING] = "Instalando fonte...",
        [STR_SYSFONT_KEEPON] = "Nao desligue o console",
        [STR_SYSFONT_REBOOT_END] = "reiniciando ao concluir...",
        [STR_HELP_FONTS_X] = "X aplicar no sistema",
        [STR_STARTUP_SLOGAN] = "personalize seu console com estilo",
    },
    /* ===================== LANG_EN ===================== */
    [LANG_EN] = {
        [STR_TAB_HOME] = "Home",
        [STR_TAB_FONTS] = "Fonts",
        [STR_TAB_THEME] = "Theme",
        [STR_TAB_LED] = "LED",
        [STR_HOME_SLOGAN] = "personalize your console",
        [STR_HOME_HINT] = "Use the d-pad or tap the screen",
        [STR_NAV_FONTS] = "Fonts",
        [STR_NAV_THEME] = "Theme",
        [STR_NAV_LED] = "LED",
        [STR_ITEM_FONTS] = "Fonts",
        [STR_ITEM_THEME] = "App theme",
        [STR_ITEM_LED] = "RGB LED",
        [STR_ITEM_FONTS_SUB] = "customize",
        [STR_ITEM_THEME_SUB] = "mode & color",
        [STR_ITEM_LED_SUB] = "color & light",
        [STR_PREVIEW] = "Preview",
        [STR_SAIR] = "START quit",
        [STR_IN_USE] = "in use",
        [STR_TO_APPLY] = "A to apply",
        [STR_YES] = "Yes",
        [STR_NO] = "No",
        [STR_HELP_HOME_L] = "A open",
        [STR_HELP_FONTS_L] = "A preview",
        [STR_HELP_THEME1_L] = "up: spectrum  l/r value",
        [STR_HELP_THEME1_R] = "A apply  B back",
        [STR_HELP_THEME2_L] = "<- -> theme/accent",
        [STR_HELP_THEME2_R] = "A confirm  START quit",
        [STR_HELP_LED_L] = "A mode  B back  <- value ->",
        [STR_CONFIRM_FONT] = "Apply this font on your device?",
        [STR_HEX_TITLE] = "Custom color (hex)",
        [STR_HEX_HINT] = "tap/drag the spectrum or type the hex",
        [STR_APPEARANCE] = "Appearance",
        [STR_LIGHT] = "Light",
        [STR_DARK] = "Dark",
        [STR_ACCENT] = "Accent",
        [STR_CUSTOM] = "Custom",
        [STR_CONFIRM_COLOR] = "Apply this color as theme?",
        [STR_LANGUAGE] = "Language",
        [STR_LED_STATIC] = "Static",
        [STR_LED_PULSE] = "Pulse",
        [STR_LED_OFF] = "Off",
        [STR_LED_ACTIVE] = "led active",
        [STR_LED_SIM] = "simulated (no MCU)",
        [STR_RED] = "Red",
        [STR_GREEN] = "Green",
        [STR_BLUE] = "Blue",
        [STR_SPEED] = "Speed",
        [STR_SPEED_SHORT] = "Spd",
        [STR_STATE] = "State",
        [STR_REALTIME] = "Real time",
        [STR_SIMULATED] = "Simulated",
        [STR_FONT_SYSTEM] = "System Default",
        [STR_SYSFONT_CONFIRM] = "Apply as system font? It will reboot (back up NAND).",
        [STR_SYSFONT_RESTORE] = "Restore the ORIGINAL system font? It will REBOOT.",
        [STR_SYSFONT_DONE] = "Font installed. Rebooting...",
        [STR_SYSFONT_FAIL] = "Install failed (works on a real console only).",
        [STR_SYSFONT_INSTALLING] = "Installing font...",
        [STR_SYSFONT_KEEPON] = "Don't turn off the console",
        [STR_SYSFONT_REBOOT_END] = "rebooting when done...",
        [STR_HELP_FONTS_X] = "X apply to system",
        [STR_STARTUP_SLOGAN] = "personalize your console in style",
    },
    /* ===================== LANG_ES ===================== */
    [LANG_ES] = {
        [STR_TAB_HOME] = "Inicio",
        [STR_TAB_FONTS] = "Fuentes",
        [STR_TAB_THEME] = "Tema",
        [STR_TAB_LED] = "LED",
        [STR_HOME_SLOGAN] = "personaliza tu consola",
        [STR_HOME_HINT] = "Usa la cruceta o toca la pantalla",
        [STR_NAV_FONTS] = "Fuentes",
        [STR_NAV_THEME] = "Tema",
        [STR_NAV_LED] = "LED",
        [STR_ITEM_FONTS] = "Fuentes",
        [STR_ITEM_THEME] = "Tema del app",
        [STR_ITEM_LED] = "LED RGB",
        [STR_ITEM_FONTS_SUB] = "personalizar",
        [STR_ITEM_THEME_SUB] = "modo y color",
        [STR_ITEM_LED_SUB] = "color y luz",
        [STR_PREVIEW] = "Preview",
        [STR_SAIR] = "START salir",
        [STR_IN_USE] = "en uso",
        [STR_TO_APPLY] = "A aplicar",
        [STR_YES] = "Si",
        [STR_NO] = "No",
        [STR_HELP_HOME_L] = "A abrir",
        [STR_HELP_FONTS_L] = "A vista previa",
        [STR_HELP_THEME1_L] = "arriba: espectro  izq/der valor",
        [STR_HELP_THEME1_R] = "A aplicar  B volver",
        [STR_HELP_THEME2_L] = "<- -> tema/accent",
        [STR_HELP_THEME2_R] = "A confirmar  START salir",
        [STR_HELP_LED_L] = "A modo  B volver  <- valor ->",
        [STR_CONFIRM_FONT] = "Aplicar esta fuente en tu dispositivo?",
        [STR_HEX_TITLE] = "Color personalizado (hex)",
        [STR_HEX_HINT] = "toca/arrastra el espectro o escribe el hex",
        [STR_APPEARANCE] = "Apariencia",
        [STR_LIGHT] = "Claro",
        [STR_DARK] = "Oscuro",
        [STR_ACCENT] = "Accent",
        [STR_CUSTOM] = "Custom",
        [STR_CONFIRM_COLOR] = "Aplicar este color como tema?",
        [STR_LANGUAGE] = "Idioma",
        [STR_LED_STATIC] = "Fijo",
        [STR_LED_PULSE] = "Pulso",
        [STR_LED_OFF] = "Off",
        [STR_LED_ACTIVE] = "led activo",
        [STR_LED_SIM] = "simulado (sin MCU)",
        [STR_RED] = "Rojo",
        [STR_GREEN] = "Verde",
        [STR_BLUE] = "Azul",
        [STR_SPEED] = "Velocidad",
        [STR_SPEED_SHORT] = "Vel",
        [STR_STATE] = "Estado",
        [STR_REALTIME] = "Tiempo real",
        [STR_SIMULATED] = "Simulado",
        [STR_FONT_SYSTEM] = "Predeterminada",
        [STR_SYSFONT_CONFIRM] = "Aplicar fuente del sistema? Reiniciara (backup NAND).",
        [STR_SYSFONT_RESTORE] = "Restaurar la fuente ORIGINAL? Se REINICIARA.",
        [STR_SYSFONT_DONE] = "Fuente instalada. Reiniciando...",
        [STR_SYSFONT_FAIL] = "Fallo al instalar (solo en consola real).",
        [STR_SYSFONT_INSTALLING] = "Instalando fuente...",
        [STR_SYSFONT_KEEPON] = "No apagues la consola",
        [STR_SYSFONT_REBOOT_END] = "reiniciando al terminar...",
        [STR_HELP_FONTS_X] = "X aplicar al sistema",
        [STR_STARTUP_SLOGAN] = "personaliza tu consola con estilo",
    },
};

void langSet(Lang l) {
    if (l < 0 || l >= LANG_COUNT) l = LANG_EN;
    s_lang = l;
}

Lang langGet(void) { return s_lang; }

void langInit(int savedLang) {
    if (savedLang >= 0 && savedLang < LANG_COUNT) {
        s_lang = (Lang)savedLang;
        return;
    }
    /* Sem idioma salvo -> default do SISTEMA (§i18n.4). */
    s_lang = LANG_EN;
    if (R_SUCCEEDED(cfguInit())) {
        u8 sys = 0;
        if (R_SUCCEEDED(CFGU_GetSystemLanguage(&sys))) {
            if (sys == CFG_LANGUAGE_PT) s_lang = LANG_PT;
            else if (sys == CFG_LANGUAGE_ES) s_lang = LANG_ES;
            else s_lang = LANG_EN; /* EN para EN e qualquer outro idioma do sistema */
        }
        cfguExit();
    }
}

const char* T(StringId id) {
    if (id < 0 || id >= STR_COUNT) return "?";
    const char* s = STRINGS[s_lang][id];
    if (s) return s;
    s = STRINGS[LANG_EN][id];        /* fallback EN */
    return s ? s : "?";              /* depois a propria chave/"?" */
}
