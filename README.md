# GSTgeode — source project

**Target:** Geometry Dash `2.2091` / Geode `v6.0.0-alpha.1`  
**Mod ID:** `gstgeode.gstgeode`

GSTgeode is a new Geode mod project. It does not reuse the old `thesillydoggo.qolmod` binary.

## Included in v1.0.0

- Custom dark/cyan creator panel with a player-tools and level-builder layout.
- Noclip-style death bypass toggle.
- Speedhack toggle with `1.0x`, `1.25x`, `1.5x`, `2.0x`, and `3.0x` rates.
- Starter-layout generator with four palettes (Neon, Sunset, Forest, Gold), four obstacle densities (Easy, Normal, Hard, Demon), and three lengths (Short, Medium, Long).
- Shortcuts in the main menu, editor UI, and pause menu.

## Generator behavior

The generator places standard block objects (`1`) and spike objects (`8`) through `EditorUI::createObject`. The selected theme colors the generated pieces. Difficulty changes spike frequency; length changes the number of segments. It is deliberately a **starter layout**, not a fully decorated rated-level generator. Generate into a blank level and use the editor's Undo command to remove the generated pieces.

## Cheat behavior and limitations

- **Noclip:** this first version suppresses the player-destruction call. It is a death bypass, not a full ghost mode that removes every solid-object collision.
- **Speedhack:** changes the delta-time passed to the active game layer. High speed values can make physics and timing less predictable.
- Cheats can invalidate legitimate runs and achievements. Turn them off for normal play or record attempts.

## Build

The source tree is ready for a Geode SDK build, but this archive is **not a compiled `.geode` installer**. The build runner must have the Geode `v6.0.0-alpha.1` SDK and bindings for GD `2.2091` available.

### GitHub Actions

1. Create an empty GitHub repository and upload the contents of this folder.
2. Open **Actions → Build GSTgeode → Run workflow** (or push to `main`).
3. Download the artifact matching your platform (`Android64`, `Android32`, or `Win64`).
4. Install the resulting `.geode` file using Geode's mod installer / mods folder.

The workflow pins the requested SDK version. If that exact SDK tag is not available from the build action, the workflow will fail before compilation; in that case, build locally with the matching SDK or update the `sdk:` input to the exact Geode 6 SDK ref that your loader was built from.

### Local build

With the matching Geode SDK and CLI installed:

```bash
geode build
```

## Compatibility note

Geode `v6.0.0-alpha.1` announces support for the 2.209x line, but it is an alpha release. The public bindings repository currently documents 2.209 separately; a bindings issue specifically notes that 2.2091 is a later hotfix and does not yet have its own exact bindings directory. The editor/game APIs used here are therefore a starting point, not proof of a successful 2.2091 build or runtime test. Compile against the exact SDK/bindings used by your installation and test in an empty level before relying on it.
