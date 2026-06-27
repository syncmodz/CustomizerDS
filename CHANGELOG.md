# Changelog

## 1.3.0
- You can now apply a bundled font as the whole **3DS system font**: pick a font
  in the Fonts tab and press X. It shows an install screen and reboots to apply;
  "System Default" restores the original. Make a NAND backup first.
- The system fonts are built by merging the design into the original font, so the
  full glyph set is kept and the Home Menu doesn't crash.
- Home screen: the app emblem is now animated, and the boot screen flows into the
  home instead of popping in.
- Theme tab: navigate by section with the D-pad and a cleaner selection
  indicator (no more pulsing halo).
- Fixed the D-pad moving twice per press on the emulator (hardware was fine).
- Removed Minecraftia and Patterns & Dots (hard to read as a system font) — 7
  bundled fonts now.

## 1.2.0
- Smoother, faster tab transitions. The incoming screen is now drawn once and
  reused each frame instead of being redrawn every frame, so transitions hold
  60fps on hardware.
- Tweaked the transition timing and easing for a softer settle.

## 1.1.0
- Fixed: icons and fonts were missing when installed as a `.cia` (the romfs
  wasn't readable). makerom now builds the romfs the standard way.
- The LED no longer turns off when you close the app, and the saved color comes
  back when you reopen it.
- Added a 9th font (Super Mario 64) and brought back "System Default" as the
  first option.
- Theme/Fonts/Home cleanup: HEX label, single-tone bottom background,
  scrollable font list, centered layout.
- English README with a QR code to install the `.cia`.

## 1.0.0
- First release: 9 fonts, 3 languages (EN/PT/ES), light/dark theme with accent
  color and a HEX picker, RGB LED, animated transitions, full D-pad navigation.
