#include "config.h"
#include "common.h"

static uint8_t configLedR = 255, configLedG = 0, configLedB = 0;

Result configLoad(void) {
    // Carregar configuracoes do SD card
    return 0;
}

void configGetLedColor(uint8_t* r, uint8_t* g, uint8_t* b) {
    *r = configLedR;
    *g = configLedG;
    *b = configLedB;
}

void configSetLedColor(uint8_t r, uint8_t g, uint8_t b) {
    configLedR = r;
    configLedG = g;
    configLedB = b;
}
