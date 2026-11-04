# Changelog

## v2.1.0
- Fixed the panel layout bug: every control now lives in the popup's centered main
  layer instead of being drawn from the bottom-left corner of the screen.
- Fixed row labels being positioned outside their buttons (titles and values were
  drawn off the row).
- Difficulty and length now produce real level lengths (160 / 280 / 420 cells
  instead of 32 / 48 / 72, which were only a few seconds long).
- Mode portals are stacked in three rows so a mode switch is triggered at any
  height after a flying section.
- Added safe zones around portals.
- Wave sections now have a proper corridor with pillars, and Swing is treated as
  a flying mode with its own layout.
- Cube/Robot/Spider walls are capped at two blocks so they are always jumpable.
- More sections in Mix mode (6-8 depending on length).

## v2.0.0
- Rebuilt the panel as a clean three-tab interface (Player / Builder / Visual).
- Added game-mode selection: Mix, Cube, Ship, Ball, UFO, Wave, Robot, Spider, Swing.
- Mix mode generates several sections with portals between different game modes.
- Added layout styles (Flow, Towers, Corridor) and four layout variants.
- Added real decoration: floating diamonds, arches, light beams and distant stars.
- Decoration is placed above jump height so it never blocks the player.
- Added Auto Restart and Force Practice.
- Added a HUD with percentage and FPS counters.
- Added a live player hitbox overlay.
- Added Randomize and Reset All.
- Replaced text entry buttons with proper GD button sprites.
- Settings are persisted between sessions.

## v1.1.0
- Added a decoration-level control (Off / Light / Full) with floating diamonds and block arches.
- Added a fifth palette (Ice).
- Generator now places obstacles on the ground so generated levels are playable from the start.
- Decorations are kept above jump height so they cannot block the player.
- Settings are now persisted between sessions.
- Local Windows build (MSVC) verified.

## v1.0.0
- Initial GSTgeode source project.
- Creator panel and editor/pause/main-menu shortcuts.
- Starter layout generator with theme, difficulty and length controls.
- Noclip-style death bypass and configurable speed multiplier.
