#include "fonts.h"

static C2D_Font s_systemFont = NULL;
static C2D_TextBuf s_textBuf = NULL;

void fontsInit(void) {
    s_systemFont = C2D_FontLoadSystem(CFG_REGION_USA);
    if (!s_systemFont) s_systemFont = C2D_FontLoadSystem(CFG_REGION_JPN);
    s_textBuf = C2D_TextBufNew(4096);
}

void fontsExit(void) {
    if (s_textBuf) { C2D_TextBufDelete(s_textBuf); s_textBuf = NULL; }
    if (s_systemFont) { C2D_FontFree(s_systemFont); s_systemFont = NULL; }
}

C2D_Font fontsGetSystem(void) { return s_systemFont; }
C2D_TextBuf fontsGetBuf(void) { return s_textBuf; }
