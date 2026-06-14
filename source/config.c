#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static const ConfigData DEFAULT_CONFIG = {
    .magic = CONFIG_MAGIC,
    .darkMode = 1,
    .fontIndex = 0,
    .ledMode = 2,
    .ledSpeed = 1,
    .ledR = 255,
    .ledG = 100,
    .ledB = 50,
    .reserved = {0, 0},
};

Result configLoad(ConfigData* out) {
    FILE* f = fopen(CONFIG_PATH, "rb");
    if (!f) {
        *out = DEFAULT_CONFIG;
        return 0;
    }
    size_t n = fread(out, 1, sizeof(ConfigData), f);
    fclose(f);
    if (n != sizeof(ConfigData) || out->magic != CONFIG_MAGIC) {
        *out = DEFAULT_CONFIG;
        return 0;
    }
    return 0;
}

Result configSave(const ConfigData* in) {
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/CustomizerDS", 0777);
    FILE* f = fopen(CONFIG_PATH, "wb");
    if (!f) return -1;
    size_t n = fwrite(in, 1, sizeof(ConfigData), f);
    fclose(f);
    return (n == sizeof(ConfigData)) ? 0 : -1;
}
