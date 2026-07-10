#ifndef ZIP3DS_H
#define ZIP3DS_H

#include <3ds.h>
#include <stdbool.h>
#include <stddef.h>

/* 2.0.0: leitura de ZIP (miniz) -- igual ao Anemone, que le splashes/temas/
 * badges direto do .zip na SD. Casa por BASENAME (ultimo componente do path),
 * entao funciona com zip "plano" ou com uma pasta-raiz dentro do zip. */

/* extrai a 1a entrada cujo basename == wantBasename (case-insensitive) pra um
 * buffer malloc'd (free() normal). true em sucesso. */
bool zipExtract(const char* zipPath, const char* wantBasename, u8** out, u32* outSize);

/* true se o zip tem uma entrada com aquele basename. */
bool zipHas(const char* zipPath, const char* wantBasename);

/* callback por PNG dentro do zip (data = buffer temporario, valido so na call). */
typedef void (*ZipPngCb)(const char* basename, const u8* data, u32 size, void* ud);
/* itera todas as entradas .png do zip. Retorna quantas PNGs processou. */
int zipForEachPng(const char* zipPath, ZipPngCb cb, void* ud);

/* conta entradas .png no zip SEM extrair (barato, p/ display). */
int zipCountPng(const char* zipPath);

/* extrai a 1a entrada .png do zip (p/ thumbnail). free() normal. */
bool zipExtractFirstPng(const char* zipPath, u8** out, u32* outSize);

#endif
