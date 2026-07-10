#ifndef FS3DS_H
#define FS3DS_H

#include <3ds.h>
#include <stdbool.h>

/* 2.0.0: fundacao FS pra extdata do Home Menu (temas/wallpapers e badges).
 * Mecanismo = fatos do Anemone (ids de extdata por regiao, LZ11); implementacao
 * PROPRIA. So funciona no console real -- Azahar nao aplica extdata. */

bool fs3dsInit(void);   /* cfgu + regiao + abre archives extdata (idempotente) */
void fs3dsExit(void);
bool fs3dsThemeReady(void);   /* archive do tema abriu? */
bool fs3dsBadgeReady(void);   /* archive dos badges abriu/criou? */
bool fs3dsBadgeEnsure(void);  /* abre/cria a extdata 0x14d1 + BadgeData/BadgeMngFile (igual open_badge_extdata do Anemone) */
u8   fs3dsRegion(void);

FS_Archive fs3dsThemeExt(void);   /* BodyCache/BgmCache/ThemeManage */
FS_Archive fs3dsHomeExt(void);    /* SaveData.dat */
FS_Archive fs3dsBadgeExt(void);   /* BadgeData/BadgeMngFile */

/* I/O num archive extdata (path ascii tipo "/BodyCache.bin"). */
bool fs3dsReadAll(FS_Archive ar, const char* path, u8** out, u32* outSize);
bool fs3dsWrite(FS_Archive ar, const char* path, const void* buf, u32 size);       /* recria e escreve */
bool fs3dsWriteAt(FS_Archive ar, const char* path, u64 off, const void* buf, u32 size); /* patch em arquivo existente */
bool fs3dsRemakeZeroed(FS_Archive ar, const char* path, u32 size);                 /* delete+create+zera */

/* LZ11 (body dos temas). malloc na saida; NULL em falha. */
u8* lz11Decompress(const u8* in, u32 inSize, u32* outSize);
u8* lz11CompressFast(const u8* in, u32 inSize, u32* outSize); /* literais (como o fast do Anemone) */

#endif
