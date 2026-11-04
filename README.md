# GSTgeode

**Target:** Geometry Dash `2.2091` / Geode `v6.0.0-alpha.1`  
**Mod ID:** `gstgeode.gstgeode`

GSTgeode is a creator/practice toolkit for Geometry Dash. It does not reuse the old
`thesillydoggo.qolmod` binary.

## Panel

The panel has three tabs and opens from the main menu, the editor, or the pause menu.

### PLAYER

| Feature | Description |
| --- | --- |
| Noclip | Suppresses the player-destruction call (death bypass). |
| Speedhack | Multiplies gameplay delta time. |
| Speed rate | `1.0x`, `1.25x`, `1.5x`, `2.0x`, `3.0x`. |
| Auto restart | Restarts the level shortly after a death (disabled in practice mode). |
| Force practice | Automatically enables practice mode when a level starts. |
| Reset all | Restores every setting to its default value. |

### BUILDER

| Feature | Options |
| --- | --- |
| Mode | `MIX`, `CUBE`, `SHIP`, `BALL`, `UFO`, `WAVE`, `ROBOT`, `SPIDER`, `SWING` |
| Theme | `NEON`, `SUNSET`, `FOREST`, `GOLD`, `ICE` |
| Difficulty | `EASY`, `NORMAL`, `HARD`, `DEMON` |
| Length | `SHORT` (160 cells), `MEDIUM` (280), `LONG` (420) |
| Decor | `OFF`, `LIGHT`, `FULL` |
| Style | `FLOW`, `TOWERS`, `CORRIDOR` |
| Variant | `V1`–`V4` (changes the obstacle phase) |
| Generate | Builds the layout in the open editor level. |
| Randomize | Randomizes theme, style, variant and difficulty. |

`MIX` splits the level into 4–8 sections and cycles through game modes, inserting the
matching portals (cube, ship, ball, UFO, wave, robot, spider, swing). Selecting a single
mode keeps the whole generated section in that mode.

### VISUAL

| Feature | Description |
| --- | --- |
| Hitboxes | Draws live player hitbox rectangles during a run. |
| HUD | Shows a compact status line in the top-left corner. |
| Show FPS | FPS counter inside the HUD. |
| Show percent | Current level percentage inside the HUD. |

## Generator details

- Objects are placed with `EditorUI::createObject` using standard object IDs:
  blocks (`1`), spikes (`8`) and mode portals.
- Obstacles sit on the real ground (`y = 105`), so the layout is playable immediately
  instead of starting with an unjumpable wall.
- Flying sections get a ceiling; wave sections get pillars to weave through, and
  swing sections use their own gravity-flip layout.
- Mode portals are stacked in three rows (60..330) so a mode change is triggered at
  any height, and cells around each portal are left empty.
- Walls are capped at two blocks so every cube/robot/spider section is jumpable.
- Decoration (floating diamonds, arch structures, light beams, distant stars) is placed
  at `y >= 315`, above the player's jump height, so it cannot block the level.
- Settings are persisted between sessions with `Mod::get()->setSavedValue`.

Generated content is a starter layout, not a fully decorated rated level. Generate into
an empty level and remove unwanted pieces with the editor.

## Cheat limitations

- Noclip is a death bypass, not a full ghost mode that removes every solid collision.
- Speedhack changes delta time; high values make physics and timing less predictable.
- Cheats can invalidate legitimate runs and achievements.

## Build

The project builds against the Geode `v6.0.0-alpha.1` SDK and bindings for GD `2.2091`.

```bash
geode build
```

A local Windows build with MSVC 19.44 was verified to produce
`build/gstgeode.gstgeode.geode`. The GitHub Actions workflow builds the Win64 artifact.
