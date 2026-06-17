#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <3ds.h>
#include <stdint.h>

typedef enum {
    LED_PATTERN_CONSTANT = 0,
    LED_PATTERN_BREATHING = 1,
    LED_PATTERN_PULSING = 2,
    LED_PATTERN_RAINBOW = 3,
} LEDPattern;

typedef struct {
    uint8_t r, g, b;
} LEDColor;

typedef struct {
    LEDPattern pattern;
    LEDColor color;
    uint8_t speed;
    uint8_t smoothing;
    bool enabled;
} LEDConfig_t;

Result ledSystemInit(void);
void ledSystemExit(void);
Result ledSetColor(uint8_t r, uint8_t g, uint8_t b);
Result ledSetPattern(LEDPattern pattern, uint8_t speed, uint8_t smoothing);
void ledConfigSave_t(LEDConfig_t* config);
LEDConfig_t ledConfigLoad_t(void);

#endif
