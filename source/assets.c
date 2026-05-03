#include "assets.h"
#include "common.h"
#include "theme.h"
#include "anim.h"
#include "ui.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <3ds/services/cfgu.h>

static int selectedAsset = 0;
static const char* assets[] = {
    "Reva UI - Icon Pack 1",
    "Reva UI - Icon Pack 2",
    "Reva UI - Icon Pack 3",
    "Aplicar RevaUI (LayeredFS)",
    "Corrigir Anemone",
    "Voltar"
};
static const int ASSET_COUNT = 6;
static Anim selectedAnim;

static bool showFixMessage = false;
static float fixMessageTimer = 0.0f;
static const float FIX_MESSAGE_DURATION = 3.0f;

// Remove recursivamente um diretório, profundidade máxima 8 níveis
static void removeDir(const char* path, int depth) {
    if (depth > 8) return;
    DIR* d = opendir(path);
    if (!d) return;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        char child[512];
        snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
        struct stat st;
        if (stat(child, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            removeDir(child, depth + 1);
            rmdir(child);
        } else {
            unlink(child);
        }
    }
    closedir(d);
    rmdir(path);
}

static void fixAnemone() {
    const char* anemoneDir = "sdmc:/3ds/Anemone3DS";
    struct stat st;

    // Tenta deletar a pasta corrompida (nunca toca em /Themes/)
    if (stat(anemoneDir, &st) == 0 && S_ISDIR(st.st_mode)) {
        removeDir(anemoneDir, 0);
    }

    // Se a deleção funcionou, retorna sem criar o arquivo de instrução
    if (stat(anemoneDir, &st) != 0) {
        return;
    }

    // Fallback: criar arquivo de instrução no SD para o usuário fazer no PC
    FILE* f = fopen("sdmc:/FIX_ANEMONE.txt", "w");
    if (!f) return;
    fprintf(f, "Para corrigir o Anemone sem perder seus temas:\n");
    fprintf(f, "1. No computador, acesse o SD card\n");
    fprintf(f, "2. DELETE a pasta: /3ds/Anemone3DS/ (so essa pasta, nao /Themes/)\n");
    fprintf(f, "3. Baixe o Anemone3DS.cia mais recente do GitHub:\n");
    fprintf(f, "   https://github.com/astronautlevel2/Anemone3DS/releases\n");
    fprintf(f, "4. Coloque o .cia na pasta /cias/ do SD\n");
    fprintf(f, "5. Instale via FBI\n");
    fprintf(f, "Seus temas em /Themes/ estao seguros e intactos.\n");
    fclose(f);
}

bool assetsShowFixMessage(void) {
    return showFixMessage;
}

static bool createDir(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) return true;
    return mkdir(path, 0777) == 0;
}

static void applyRevaUILayeredFS() {
    cfguInit();
    u8 region;
    Result res = CFGU_SecureInfoGetRegion(&region);
    cfguExit();

    if (R_FAILED(res)) {
        region = 2; // Fallback EUR
    }

    const char* titleID;
    switch(region) {
        case 0: titleID = "0004003000008202"; break; // JPN
        case 1: titleID = "0004003000008F02"; break; // USA
        default: titleID = "0004003000009802"; break; // EUR
    }

    char basePath[256];
    snprintf(basePath, sizeof(basePath), "sdmc:/luma/titles/%s/romfs/timg", titleID);

    char tempPath[256];
    snprintf(tempPath, sizeof(tempPath), "sdmc:/luma");
    createDir(tempPath);
    snprintf(tempPath, sizeof(tempPath), "sdmc:/luma/titles");
    createDir(tempPath);
    snprintf(tempPath, sizeof(tempPath), "sdmc:/luma/titles/%s", titleID);
    createDir(tempPath);
    snprintf(tempPath, sizeof(tempPath), "sdmc:/luma/titles/%s/romfs", titleID);
    createDir(tempPath);
    snprintf(tempPath, sizeof(tempPath), "sdmc:/luma/titles/%s/romfs/timg", titleID);
    createDir(tempPath);

    const char* bclimFiles[] = {
        "wifi_0.bclim",
        "bt_0.bclim",
        "airplane_0.bclim",
        "alarm_0.bclim",
        "mute_0.bclim",
        "orientation_0.bclim"
    };

    for (int i = 0; i < 6; i++) {
        char srcPath[256], dstPath[256];
        snprintf(srcPath, sizeof(srcPath), "romfs:/revaui/%s", bclimFiles[i]);
        snprintf(dstPath, sizeof(dstPath), "%s/%s", basePath, bclimFiles[i]);

        FILE* src = fopen(srcPath, "rb");
        if (!src) continue;

        FILE* dst = fopen(dstPath, "wb");
        if (!dst) { fclose(src); continue; }

        fseek(src, 0, SEEK_END);
        long size = ftell(src);
        fseek(src, 0, SEEK_SET);

        void* buf = malloc(size);
        if (buf) {
            fread(buf, 1, size, src);
            fwrite(buf, 1, size, dst);
            free(buf);
        }

        fclose(src);
        fclose(dst);
    }
}

void assetsInit(void) {
    selectedAsset = 0;
    animSet(&selectedAnim, 0.0f, 0.12f);
    showFixMessage = false;
    fixMessageTimer = 0.0f;
}

void assetsRender(u32 kDown, u32 kHeld, int* currentScreen) {
    if (kDown & KEY_B) {
        *currentScreen = SCREEN_MAIN_MENU;
        return;
    }

    if (kDown & KEY_DOWN) {
        selectedAsset = (selectedAsset + 1) % ASSET_COUNT;
    }
    if (kDown & KEY_UP) {
        selectedAsset = (selectedAsset - 1 + ASSET_COUNT) % ASSET_COUNT;
    }
    if (kDown & KEY_A) {
        if (selectedAsset == ASSET_COUNT - 1) {
            *currentScreen = SCREEN_MAIN_MENU;
            return;
        } else if (selectedAsset == 3) {
            applyRevaUILayeredFS();
        } else if (selectedAsset == 4) {
            fixAnemone();
            showFixMessage = true;
            fixMessageTimer = FIX_MESSAGE_DURATION;
        }
    }

    if (showFixMessage) {
        fixMessageTimer -= 1.0f / 60.0f;
        if (fixMessageTimer <= 0.0f) showFixMessage = false;
    }

    C2D_TextBuf buf = C2D_TextBufNew(1024);
    if (!buf) return;

    UI_Header(buf, "Assets/Icones", "Escolha um pacote de icones");

    animTo(&selectedAnim, selectedAsset * 1.0f);
    animStep(&selectedAnim);
    float selectAnim = animEasedOut(&selectedAnim);

    for (int i = 0; i < ASSET_COUNT; i++) {
        bool selected = (i == selectedAsset);
        float itemAnim = selected ? 1.0f : 0.0f;
        if (selected) {
            itemAnim = selectAnim - (int)selectAnim;
            if (itemAnim < 0) itemAnim += 1.0f;
        }
        UI_ListItem(buf, 10, 55 + i*35, 300, 30, assets[i],
                    NULL, selected, itemAnim, selected ? ">" : NULL);
    }

    if (selectedAsset == 3) {
        C2D_Text warn;
        C2D_TextParse(&warn, buf, "Requer Luma: Enable game patching ativo");
        C2D_TextOptimize(&warn);
        C2D_DrawText(&warn, 10.0f, 220.0f, 0.0f, 0.22f, 0.22f, g_theme.accent);
    }

    UI_Footer(buf, "Selecionar", "Voltar", NULL);

    C2D_TextBufDelete(buf);
}
