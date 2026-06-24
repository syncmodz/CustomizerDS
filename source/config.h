#ifndef CONFIG_H_
#define CONFIG_H_

#include <3ds.h>
#include <stdint.h>

#define CONFIG_MAGIC 0x43445332
#define CONFIG_PATH "sdmc:/3ds/CustomizerDS/config.bin"

/* i18n (v9.1 §i18n.5): o idioma e guardado em reserved[0]. 0xFF = "nao
 * definido" -> usa o idioma do SISTEMA. Mantem magic/tamanho do struct, entao
 * saves antigos continuam validos. */
#define CFG_LANG_UNSET 0xFF

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
