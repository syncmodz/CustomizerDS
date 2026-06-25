# Changing the 3DS system font

This is a manual job. CustomizerDS does **not** do it for you, on purpose:
the app can only build regular fonts (`mkbcfnt`), and the system font needs a
specific format (A4, 16 levels). A wrong format gives you garbled text on the
Home Menu. So this is only documented here, not automated.

It is recoverable (Boot9Strap + a NAND backup + GodMode9), but back up first.

## What you need

- Title: `0004009B00014002`. Inner file: `cbf_std.bcfnt.lz` (LZ-compressed BCFNT).
- Bit depth must be **A4 (16 levels)**. Max size **1.5 MiB** compressed.
- A **NAND backup** (GodMode9) before you start.

## Steps (PC)

1. Make a NAND backup in GodMode9.
2. (Optional) Merge your font with the official one in FontForge so system
   glyphs aren't missing.
3. Convert it with **CTR Font Converter** at bit depth **16 levels (A4)** →
   `SystemFont.bcfnt`.
4. Run **FontTool** (`python FontTool.py -font SystemFont.bcfnt`) to get
   `cbf_std.bcfnt.lz` and/or a `.cia`.
5. Install the `.cia` in GodMode9, or test it in an emulator by placing the
   file at `/load/mods/0004009B00014002/romfs/cbf_std.bcfnt.lz`.
6. Reboot. If text looks broken, restore (below).

## Restore

Install the original `SystemFont.cia`, or restore your NAND backup in GodMode9.
On an emulator, just delete the file from `/load/mods/...`.

## Why the app doesn't do it

The A4 conversion needs CTR Font Converter (Nintendo's SDK tool) and, for full
glyph coverage, your own dumped system font. There's no safe way to generate
that from inside the app, and shipping a wrong-format font would corrupt the
Home Menu text. So CustomizerDS sticks to the in-app font (100% safe) and
documents the manual path here.

References: aromakitsune (Custom System Fonts), GBAtemp (Customising a System
Font), astronautlevel2/FontTool.
