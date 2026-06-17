#include "led_control.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <3ds/services/mcuhwc.h>

Result ledSystemInit(void) {
    return mcuHwcInit();
}

void ledSystemExit(void) {
    mcuHwcExit();
}

Result ledSetColor(uint8_t r, uint8_t g, uint8_t b) {
    InfoLedPattern pattern;
    memset(&pattern, 0, sizeof(pattern));
    pattern.delay = 0x10;
    pattern.smoothing = 0x00;
    pattern.loopDelay = 0xFF;
    pattern.blinkSpeed = 0x00;
    memset(pattern.redPattern, r, 32);
    memset(pattern.greenPattern, g, 32);
    memset(pattern.bluePattern, b, 32);
    return MCUHWC_SetInfoLedPattern(&pattern);
}

Result ledSetPattern(LEDPattern pattern, uint8_t speed, uint8_t smoothing) {
    (void)pattern;
    (void)speed;
    (void)smoothing;
    return 0;
}

void ledConfigSave_t(LEDConfig_t* config) {
    FILE* f = fopen("sdmc:/CustomizerDS/led_config.bin", "wb");
    if (f) {
        fwrite(config, sizeof(LEDConfig_t), 1, f);
        fclose(f);
    }
}

LEDConfig_t ledConfigLoad_t(void) {
    LEDConfig_t config = {0};
    FILE* f = fopen("sdmc:/CustomizerDS/led_config.bin", "rb");
    if (f) {
        fread(&config, sizeof(LEDConfig_t), 1, f);
        fclose(f);
    }
    return config;
}
