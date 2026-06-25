# Changelog

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
