#ifndef CONFIG_H_
#define CONFIG_H_

#include <3ds.h>
#include <stdint.h>

Result configLoad(void);
void configGetLedColor(uint8_t* r, uint8_t* g, uint8_t* b);
void configSetLedColor(uint8_t r, uint8_t g, uint8_t b);

#endif
