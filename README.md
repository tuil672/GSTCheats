# GSTgeode — source project

**Target:** Geometry Dash `2.2091` / Geode `v6.0.0-alpha.1`  
**Mod ID:** `gstgeode.gstgeode`

GSTgeode is a new Geode mod project. It does not reuse the old `thesillydoggo.qolmod` binary.

## Included in v1.0.0

- Custom dark/cyan creator panel with a player-tools and level-builder layout.
- Noclip-style death bypass toggle.
- Speedhack toggle with `1.0x`, `1.25x`, `1.5x`, `2.0x`, and `3.0x` rates.
- Layout generator with five palettes (Neon, Sunset, Forest, Gold, Ice), four obstacle densities (Easy, Normal, Hard, Demon), three lengths (Short, Medium, Long), and three decoration levels (Off, Light, Full).
- All settings are saved between sessions through `Mod::get()->setSavedValue`.
- Shortcuts in the main menu, editor UI, and pause menu.

## Generator behavior

The generator places standard block objects (`1`) and spike objects (`8`) through `EditorUI::createObject`. The selected theme colors the generated pieces.

- **Difficulty** changes spike/block frequency and adds double spikes or taller walls on Hard/Demon.
- **Length** changes the number of segments.
- **Decor** adds floating diamonds and block arches. All decoration sits at `y >= 255`, above the player's maximum jump height, so it cannot block the level.

Obstacles are placed on the real ground (object centre `y = 105`), so generated levels are immediately playable instead of starting with an unjumpable wall. It is deliberately a **starter layout**, not a fully decorated rated-level generator. Generate into a blank level and use the editor's Undo command to remove the generated pieces.

## Cheat behavior and limitations

- **Noclip:** suppresses the player-destruction call. It is a death bypass, not a full ghost mode that removes every solid-object collision.
- **Speedhack:** changes the delta-time passed to the active game layer. High speed values can make physics and timing less predictable.
- Cheats can invalidate legitimate runs and achievements. Turn them off for normal play or record attempts.

## Build

The project builds against the Geode `v6.0.0-alpha.1` SDK and bindings for GD `2.2091`.

### GitHub Actions

1. Push to `main` or run **Actions → Build GSTgeode → Run workflow**.
2. Download the artifact matching your platform (`Android64`, `Android32`, or `Win64`).
3. Install the resulting `.geode` file using Geode's mod installer / mods folder.

The workflow pins the requested SDK version. If that exact SDK tag is not available from the build action, update the `sdk:` input to the exact Geode 6 SDK ref used by your loader.

### Local build

With the matching Geode SDK and CLI installed:

```bash
geode build
```

A local Windows build with MSVC 19.44 was verified to produce `build/gstgeode.gstgeode.geode`.

## Compatibility note

Geode `v6.0.0-alpha.1` announces support for the 2.209x line, but it is an alpha release. Test in an empty level before relying on generated layouts, and keep a backup of your save data.
