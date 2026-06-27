# System font

CustomizerDS can replace the **3DS system font** (the font the Home Menu and
most apps use) with one of its fonts — applied **in-app**, one tap, on real
hardware.

## How it works

In the Fonts tab, pick a font and press **X**. A confirmation popup warns the
console will reboot; on **A**, the app installs a `.cia` that replaces the font
title `0004009B00014002` in CTRNAND and reboots to apply it.

- The install goes through the **AM** service (`am:net`), exactly like **FBI**:
  `AM_DeleteTitle`/`AM_DeleteTicket` (remove the old font) →
  `AM_StartCiaInstall(MEDIATYPE_NAND)` → write the CIA → `AM_FinishCiaInstall`
  (atomic commit). The `am` sysmodule does the NAND write; the app only streams
  the bytes. See `source/sysfont.c::sysfontInstallCia`.
- The `.cia` files are **pre-built and embedded** in the app at
  `romfs:/sysfont/SystemFont_<font>.cia` (one per selectable font), plus
  `SystemFont_STOCK.cia` for restoring the original.
- **"System Default"** (the first list entry) installs `SystemFont_STOCK.cia` =
  your console's original font, so it's a one-tap restore.

This **only works on real hardware** — the Azahar/Citra emulator can't install
system titles, so there the install just fails with an error popup. (To preview
a font on the emulator, use its own `/load/mods` path; the app fonts render in
the in-app preview regardless.)

## Safety / brick recovery

Replacing the font writes to a **system title in CTRNAND**. This is recoverable:

- **Keep a NAND backup** (GM9). If the Home Menu ever fails to boot after a font
  change, restore the NAND backup with GM9 and you're back to normal.
- The `.cia` are validated at build time: **A4 glyph format (TGLP `0x0B`)** and
  **≤ 1.5 MiB** compressed — a malformed/oversized font (which could hang the
  Home Menu) never gets packaged.
- Each `.cia` was verified with `ctrtool` (correct title id, valid RomFS).
- The CIAs are **decrypted/NoCrypto** (the bundled header sets the flag), which
  is what installs and runs under Luma. makerom's "Failed to load ncch aes key"
  during build is the normal homebrew "won't be encrypted" notice.

> First time: make sure your GM9 NAND backup exists before applying a font.

## ⚠️ Custom system fonts MUST be built by MERGE (or they brick)

A font made with bare `mkbcfnt` has only ~95 glyphs (ASCII). The real system
font has ~7501 glyphs, including the **PUA button icons (U+E000+, A/B/X/Y/L/R)
that the Home Menu indexes** and a fallback glyph. Installing a 95-glyph font
makes NS dereference a missing glyph at boot → **Home Menu crash / soft-brick**
(recoverable with GM9 + the stock CIA, but still). Format (A4/`0x0B`) and size
(≤1.5 MiB) are necessary but **not** sufficient.

So custom system fonts are built by **merge** (`scripts/mk_sysfont_merge.py`,
needs freetype): start from the **stock** font (full glyph set) and re-render
only the Latin glyphs (U+0020–U+024F, ~285 incl. accents) from the custom
TTF/OTF into the stock's 24×30 cells, keeping cmap, widths, CJK, PUA and the
fallback. Result = complete, non-crashing font that just restyles the text.

## How the `.cia` are generated

`make` builds them automatically into `romfs/sysfont/` (they're prerequisites of
the app), or run `make sysfont`. Per custom font:
**stock + TTF → `mk_sysfont_merge.py` → merged `.bcfnt` → `mk_sysfont_cia.py`
(`3dstool` lzex → romfs → CFA with `ncchheader.bin` → `makerom`)**. The STOCK
restore is built straight from the original `.lz` (no merge). Inputs (all
gitignored): `scripts/sysfont/ttf/<font>.{ttf,otf}`, the stock
`scripts/sysfont/stock/cbf_std.bcfnt[.lz]`, and a freetype venv at
`scripts/sysfont/.venv`. Each merged `.cia` is ~1.45 MB (full font), so the app
`.cia` is ~21 MB. **Never** use bare `mkbcfnt` output as a system font.

- `ncchheader.bin` is the font title's real NCCH header (TID `0004009B00014002`,
  NoCrypto), bundled from **FontTool** (astronautlevel2 & ihaveamac, MIT — see
  `scripts/sysfont/FontTool-LICENSE.txt`). No console dump is needed for it.
- The **STOCK** font input (`scripts/sysfont/stock/cbf_std.bcfnt.lz`) is the
  original font extracted from the owner's own NAND backup (decrypted via
  `ninfs` with boot9 + OTP). It is **console/region specific**.

### Do not redistribute the font data

`scripts/sysfont/stock/` and `romfs/sysfont/` are **gitignored**: they contain
Nintendo font data (the stock font, and the embedded font CIAs). They're fine
for your own console, but **do not commit them or publish a release `.cia` that
contains them**. On a clean clone without the stock `.lz`, the build simply omits
the STOCK restore (everything else still builds).

## Fallback: manual install via FBI

If the in-app install ever misbehaves, the same `.cia` in `romfs/sysfont/` (or
regenerated via `make sysfont`) can be copied to the SD and installed with
**FBI**, then reboot. Same result, FBI does the NAND write.

## Notes

- The in-app font **preview** (`C2D_FontLoad`) is independent and never touches
  the system font.
- App fonts cover Latin (ASCII + accents). The STOCK font keeps full coverage
  (CJK etc.); custom fonts may show boxes for scripts they don't include —
  cosmetic, reversible by restoring System Default.
</content>
