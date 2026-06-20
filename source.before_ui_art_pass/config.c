#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const ConfigData DEFAULT_CONFIG = {
    .magic = CONFIG_MAGIC,
    .darkMode = 1,
    .fontIndex = 0,
    .ledMode = 0,
    .ledSpeed = 2,
    .ledR = 255,
    .ledG = 96,
    .ledB = 160,
    .accentIndex = 0,
    .appStyle = 0,
    .reserved = {0},
};

const ConfigData* configDefaults(void) {
    return &DEFAULT_CONFIG;
}

Result configLoad(ConfigData* out) {
    if (!out) return -1;

    FILE* f = fopen(CONFIG_PATH, "rb");
    if (!f) {
        *out = DEFAULT_CONFIG;
        return 0;
    }

    ConfigData tmp;
    size_t n = fread(&tmp, 1, sizeof(ConfigData), f);
    fclose(f);

    if (n != sizeof(ConfigData) || tmp.magic != CONFIG_MAGIC) {
        *out = DEFAULT_CONFIG;
        return 0;
    }

    *out = tmp;
    return 0;
}

Result configSave(const ConfigData* in) {
    if (!in) return -1;

    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/CustomizerDS", 0777);

    ConfigData tmp = *in;
    tmp.magic = CONFIG_MAGIC;

    FILE* f = fopen(CONFIG_PATH, "wb");
    if (!f) return -1;

    size_t n = fwrite(&tmp, 1, sizeof(ConfigData), f);
    fclose(f);
    return (n == sizeof(ConfigData)) ? 0 : -1;
}
