#ifndef CONFIG_H_
#define CONFIG_H_

#include <3ds.h>
#include <stdint.h>

typedef struct {
    u32 magic;
    u8 darkMode;
    u8 fontIndex;
    u8 ledMode;
    u8 ledSpeed;
    u8 ledR, ledG, ledB;
    u8 reserved[2];
} ConfigData;

#define CONFIG_MAGIC 0x43445331
#define CONFIG_PATH "sdmc:/3ds/CustomizerDS/config.bin"

Result configLoad(ConfigData* out);
Result configSave(const ConfigData* in);

#endif
