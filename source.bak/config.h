#ifndef CONFIG_H_
#define CONFIG_H_

#include <3ds.h>
#include <stdint.h>

#define CONFIG_MAGIC 0x43445332
#define CONFIG_PATH "sdmc:/3ds/CustomizerDS/config.bin"

typedef struct {
    u32 magic;
    u8 darkMode;
    u8 fontIndex;
    u8 ledMode;
    u8 ledSpeed;
    u8 ledR;
    u8 ledG;
    u8 ledB;
    u8 accentIndex;
    u8 appStyle;
    u8 customAccentFlag;
    u8 customR;
    u8 customG;
    u8 customB;
    u8 reserved[10];
} ConfigData;

const ConfigData* configDefaults(void);
Result configLoad(ConfigData* out);
Result configSave(const ConfigData* in);

#endif
