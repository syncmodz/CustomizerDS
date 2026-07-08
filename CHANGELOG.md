# Changelog

## 1.8.0
Caelestia motion engine — the shape/feel language from caelestia-dots/shell
ported from its source (tokens.hpp / Anim / StateLayer / StyledRect), no logic
touched.
- **Real M3 curves**: added the full Material-3 expressive family from
  caelestia's tokens (the 2-segment `emphasized`, plus the Effects family for
  color/alpha vs the Spatial family for motion).
- **Focus is now morphing at caelestia speed** (0.35s FastSpatial with the
  spring), with retarget so fast D-pad input never sticks — across every screen.
- **Color never jumps anymore**: a global animated accent (300ms, caelestia's
  "CAnim") drives the focus, the segmented pills, the LED speed/depth sliders,
  the title underline and the fonts button — change the accent and the whole UI
  flows into the new color.
- **Pop-in** on dialogs (0.80→1.0 with the expressive spring, like caelestia's
  panels) and the tab transition now rides the M3 `emphasized` curve.
- **Install screen**: the progress bar now flows accent→green at 100% (animated,
  not a hard switch) and the check pops in on FastSpatial.
- **Smoother corners** everywhere (fallback vector rounding bumped to 16 segs;
  surfaces already use the 9-slice AA sheet).
- **Calm idle**: the LED ring breathes in Pulse mode.

## 1.7.0
Selection indicator redesigned + more visible detail across every screen.
- **New selection indicator** — the thin hollow accent ring is gone. Selection
  now reads as a **filled state**, tuned per control so it's never an empty
  outline floating around things:
  - Lists, tiles and cards: a filled accent "chip" (soft accent fill + crisp
    edge) that slides between items.
  - Segmented toggles (Light/Dark, LED mode, language): the focus tints the
    track from the inside instead of drawing a redundant outer ring on top of
    the already-solid selected pill.
  - Accent swatches: a crisp accent ring **outside** the circle (never covering
    the color).
  - HEX digit cells: the selected digit is now a **solid accent cell** with a
    contrast-colored digit.
  - LED sliders: the focused row gets a soft accent band + accent label.
- **New detail**: every screen title now has a short accent underline that grows
  in on entry, and the header emblem is alive on all screens.
- **Stronger ambient motion**: the emblem drift, list depth and hero parallax
  from 1.6.0 were dialed up so they actually read.

## 1.6.0
Motion & detail pass — the interface is richer and more alive, no functions
changed. Everything is pure per-frame motion (no extra fill/overdraw), so it
still holds 60fps and keeps the clean, glow-free cakeOS look.
- **Living emblem**: the three glass balls now drift in small 2D orbits (a
  floating constellation) with a slow breathing scale, on every screen's header
  and the home hero — not just a vertical bob.
- **Home hero parallax**: the wordmark and slogan float gently against the
  emblem for a sense of depth.
- **Theme accent cross-fades**: changing the accent now morphs the preview color
  smoothly instead of snapping.
- **Fonts list depth**: rows recede (dim + micro-scale) toward the top/bottom
  edges like a carousel, and scrolling now has a springy, weighted settle.
- **HEX editor detail**: the preview color cross-fades and the R/G/B numbers
  count up/down smoothly instead of jumping.
- **Silkier transitions**: cross-fade and slide-up now use the Material-3
  "emphasized" curve with a subtle scale settle for a more premium feel.

## 1.5.1
- Fonts tab: the **"A" badge** in the footer is now the real reva/cakeOS badge
  sprite (the green "A" used in the confirm popups), not a hand-drawn circle.
- Fonts tab: **"System Default" now previews the true original stock font**
  again. It used to show the live system font, which reads as Coolvetica once
  the system font has been modified — now it always shows the bundled stock font
  (which is what "System Default" restores).

## 1.5.0
- **Coolvetica is now a real installable system font**: it ships as a proper
  `.cia` merged into the original font (full glyph set kept), so applying it
  won't brick the Home Menu — exactly like the other bundled fonts. It's the 8th
  font in the list.
- Fonts tab: cleaner footer — a single centered accent pill with an **"A" badge**
  reading "apply to system" (dropped the stray "preview on browse" text).
- Theme tab: the accent swatches now use the **cakeOS/reva glass style** (thick
  colored ring + translucent center) instead of flat discs.
- Theme tab: fixed the light↔dark animation **freezing** if you left the tab
  mid-swap — the transition now finishes no matter which screen you're on.
- Theme tab: fixed the HEX editor backdrop in the **light theme** — no more
  misplaced hard shadow darkening the popup; the dim now covers the whole screen.
- LED tab: redesigned the sliders in the **cakeOS/reva style** (white knob with a
  thick colored ring) and aligned the value numbers in a tidy right-hand column.
- Focus indicator: **much snappier** — faster and smoother, with a subtle stretch
  as it travels, no more laggy/wobbly feel.

## 1.4.0
- New chrome font: **Coolvetica Regular** — a rounder, friendlier display face
  that fits the app better than the old bold.
- Reworked the focus ring: instead of a stiff slide it now **stretches like
  liquid** as it travels between items and gently breathes when it sits still.
- Home: the bottom icons are properly centered, the **ugly rings around them
  are gone** (just the icon now), the selected tile no longer turns dark in the
  light theme, and the floating balls up top are smaller with a thicker outline,
  matching the little logo balls — still animated.
- Fonts tab: **A applies the font to the system directly** (no more A-preview /
  X-apply two-step), and the list now **scrolls smoothly** with a soft fade at
  the edges instead of jumping line by line.
- Theme tab: removed the ugly ring around the sun/moon icon; the light-dark swap
  now animates **on the icon itself** (a fade + spin, no leftover ghost fade and
  no big icon over the preview). Fixed the selection indicator **flickering**
  when moving onto the HEX swatch. Cleaned up the light theme (selection and the
  HEX editor no longer look muddy/dark).
- LED tab: each mode now shows **only the sliders that make sense** — Off has
  none, Static shows just R/G/B (no pointless Speed), and **Pulse gains a Depth
  slider** to control how far the light dims in the valley of the pulse.
- Smoother, more expressive screen transitions (Material-3 "emphasized" curve on
  the horizontal slide).
- Build: the packed romfs now rebuilds whenever any bundled file changes, so a
  new font can't silently get left out of the `.3dsx`.
- Redesigned interface on all four screens (Home, Fonts, Theme, LED): a large
  macOS-style title on each screen, flat cakeOS cards, a grouped font list,
  ring swatches, and a clean LED ring.
- Removed the purple glow/halo behind everything. Focus and selection are now a
  crisp accent ring (it slides between items) or a solid pill — no more bloom.
- Smooth anti-aliased rounded corners — nothing looks jagged anymore.
- Softer, springier motion (pop-in and a sliding focus ring) like a desktop UI.
- New system-font install screen: a clean progress bar with the app emblem and
  a check, then reboot. It now also plays in the emulator (simulated, with an
  honest "real console only" note) instead of just failing.
- The bundled fonts are copied to sdmc:/3ds/CustomizerDS/fonts/ on first run.

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
