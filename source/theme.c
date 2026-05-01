#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <math.h>
#include "theme.h"

// Variável global (definida aqui, não só extern)
AppTheme g_theme;

// Conversão RGB -> HSV
static void rgbToHsv(u8 r, u8 g, u8 b, float* h, float* s, float* v) {
    float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
    float max = rf, min = rf;
    if (gf > max) max = gf; if (bf > max) max = bf;
    if (gf < min) min = gf; if (bf < min) min = bf;
    *v = max;
    float delta = max - min;
    *s = (max == 0.0f) ? 0.0f : delta / max;
    if (delta == 0.0f) { *h = 0.0f; return; }
    if (max == rf) *h = 60.0f * fmodf((gf - bf) / delta, 6.0f);
    else if (max == gf) *h = 60.0f * ((bf - rf) / delta + 2.0f);
    else *h = 60.0f * ((rf - gf) / delta + 4.0f);
    if (*h < 0.0f) *h += 360.0f;
}

// Conversão HSV -> RGB
static void hsvToRgb(float h, float s, float v, u8* r, u8* g, u8* b) {
    float c = v * s, x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f);
    float m = v - c;
    float r1 = 0, g1 = 0, b1 = 0;
    if (h < 60) { r1 = c; g1 = x; b1 = 0; }
    else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
    else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
    else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
    else { r1 = c; g1 = 0; b1 = x; }
    *r = (u8)((r1 + m) * 255); *g = (u8)((g1 + m) * 255); *b = (u8)((b1 + m) * 255);
}

// Gerar paleta a partir de uma cor primary
static void themeGeneratePalette(u8 r, u8 g, u8 b) {
    g_theme.primary = C2D_Color32(r, g, b, 255);
    
    // Conversão para HSV para variações
    float h, s, v;
    rgbToHsv(r, g, b, &h, &s, &v);
    
    // primaryDark: V - 25%
    float vDark = v > 0.25f ? v - 0.25f : v * 0.75f;
    u8 rd, gd, bd;
    hsvToRgb(h, s, vDark, &rd, &gd, &bd);
    g_theme.primaryDark = C2D_Color32(rd, gd, bd, 255);
    
    // primaryLight: V + 15% (limitado a 1.0)
    float vLight = v + 0.15f; if (vLight > 1.0f) vLight = 1.0f;
    u8 rl, gl, bl;
    hsvToRgb(h, s, vLight, &rl, &gl, &bl);
    g_theme.primaryLight = C2D_Color32(rl, gl, bl, 255);
    
    // accent: hue + 150°
    float hAccent = fmodf(h + 150.0f, 360.0f);
    u8 ra, ga, ba;
    hsvToRgb(hAccent, s, v, &ra, &ga, &ba);
    g_theme.accent = C2D_Color32(ra, ga, ba, 255);
    
    // surface: R,G,B da primary * 0.12 + base (15,15,22)
    g_theme.surface = C2D_Color32(
        (u8)(r * 0.12f + 15), (u8)(g * 0.12f + 15), (u8)(b * 0.12f + 22), 255);
    
    // surfaceHigh: surface um pouco mais claro
    g_theme.surfaceHigh = C2D_Color32(
        (u8)(r * 0.18f + 20), (u8)(g * 0.18f + 20), (u8)(b * 0.18f + 30), 255);
    
    // background e backgroundTop (já definidos no themeInit, mas recalculamos)
    g_theme.background = C2D_Color32(15, 15, 22, 255);
    g_theme.backgroundTop = C2D_Color32(18, 18, 28, 255);
    
    // onPrimary: branco ou preto (luminância da primary)
    float luminance = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
    g_theme.onPrimary = (luminance > 0.5f) ? C2D_Color32(0, 0, 0, 255) : C2D_Color32(255, 255, 255, 255);
    
    // textPrimary: branco puro ou quase
    g_theme.textPrimary = C2D_Color32(240, 240, 248, 255);
    
    // textSecondary: textPrimary com alpha ~60%
    g_theme.textSecondary = C2D_Color32(170, 168, 185, 255);
    
    // textHint: textPrimary com alpha ~35%
    g_theme.textHint = C2D_Color32(100, 98, 115, 255);
    
    // divider: linha divisória sutil
    g_theme.divider = C2D_Color32(45, 40, 70, 255);
    
    // ripple: cor do efeito ripple (primary com alpha ~120)
    g_theme.ripple = C2D_Color32(r, g, b, 120);
    
    // isDark: luminância < 0.5
    g_theme.isDark = (luminance < 0.5f);
}

void themeInit(void) {
    // Tema padrão (roxo Material 500)
    g_theme.background = C2D_Color32(15, 15, 22, 255);
    g_theme.backgroundTop = C2D_Color32(18, 18, 28, 255);
    themeGeneratePalette(103, 58, 183); // Roxo Material 500
}

void themeLoadFromAnemone(void) {
    // Tenta encontrar BodyCache.bin na pasta extdata
    const char* regionPaths[] = {
        "sdmc:/Nintendo 3DS/%s/%s/extdata/00000000/000002cd/BodyCache.bin", // USA
        "sdmc:/Nintendo 3DS/%s/%s/extdata/00000000/000002ce/BodyCache.bin", // EUR
        "sdmc:/Nintendo 3DS/%s/%s/extdata/00000000/000002cc/BodyCache.bin"  // JPN
    };
    
    // Tenta abrir usando opendir para encontrar ID0/ID1
    DIR* dir = opendir("sdmc:/Nintendo 3DS/");
    if (!dir) { themeInit(); return; }
    
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        char id0Path[512];
        snprintf(id0Path, sizeof(id0Path), "sdmc:/Nintendo 3DS/%s", ent->d_name);
        
        DIR* dir2 = opendir(id0Path);
        if (!dir2) continue;
        
        struct dirent* ent2;
        while ((ent2 = readdir(dir2)) != NULL) {
            if (ent2->d_name[0] == '.') continue;
            
            // Testa cada região
            for (int i = 0; i < 3; i++) {
                char path[1024];
                snprintf(path, sizeof(path), regionPaths[i], ent->d_name, ent2->d_name);
                FILE* f = fopen(path, "rb");
                if (!f) continue;
                
                // Ler draw type no offset 0x48
                uint8_t drawType;
                fseek(f, 0x48, SEEK_SET);
                if (fread(&drawType, 1, 1, f) != 1) { fclose(f); continue; }
                
                if (drawType == 1) {
                    // Cor sólida: ler 4 bytes RGBA no offset 0xD4
                    uint8_t rgba[4];
                    fseek(f, 0xD4, SEEK_SET);
                    if (fread(rgba, 1, 4, f) == 4) {
                        themeGeneratePalette(rgba[0], rgba[1], rgba[2]);
                    }
                    fclose(f);
                    closedir(dir2);
                    closedir(dir);
                    return;
                } else if (drawType == 3) {
                    // Textura: ler offset da textura no header
                    uint32_t texOffset;
                    fseek(f, 0xD0, SEEK_SET);
                    if (fread(&texOffset, 4, 1, f) == 1) {
                        fseek(f, texOffset + 0xD0, SEEK_SET);
                        // Ler 32x32 pixels (RGB565) e calcular média
                        uint64_t totalR = 0, totalG = 0, totalB = 0;
                        int count = 0;
                        for (int y = 0; y < 32; y++) {
                            for (int x = 0; x < 32; x++) {
                                uint16_t pixel;
                                if (fread(&pixel, 2, 1, f) != 1) break;
                                // RGB565: R5:G6:B5
                                u8 r = (pixel >> 11) & 0x1F;
                                u8 g = (pixel >> 5) & 0x3F;
                                u8 b = pixel & 0x1F;
                                totalR += (r << 3) | (r >> 2); // Expandir para 8-bit
                                totalG += (g << 2) | (g >> 4);
                                totalB += (b << 3) | (b >> 2);
                                count++;
                            }
                        }
                        if (count > 0) {
                            themeGeneratePalette(totalR / count, totalG / count, totalB / count);
                        }
                    }
                    fclose(f);
                    closedir(dir2);
                    closedir(dir);
                    return;
                }
                
                // Se drawType não for 1 nem 3 (ex: 0 = sem wallpaper)
                // Fecha o arquivo e encerra a busca com fallback
                fclose(f);
                closedir(dir2);
                closedir(dir);
                themeInit(); // Fallback para tema padrão
                return;
            }
        }
        closedir(dir2);
    }
    closedir(dir);
    
    // Fallback silencioso: usar tema padrão
    themeInit();
}
