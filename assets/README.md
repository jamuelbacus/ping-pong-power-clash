# Assets

Art, audio, and fonts for *Ping Pong: Power Clash*. Loaded at runtime relative to the
**project root**, so run the game from there (e.g. `./game`, not from inside `assets/`).

## Layout

```
assets/
├── characters/     # one emblem/portrait per character (see naming below)
├── backgrounds/    # arena / menu backdrops
├── ui/             # buttons, skill icons, cursors
├── fonts/          # custom .ttf/.otf fonts
└── audio/          # sounds (.wav/.ogg) and music (.ogg)
```

## Character icon naming

`assets/characters/<type>.png` — the filename must match the `CharacterType` enum in
`include/character.h`, lowercased:

| Character | File |
| --- | --- |
| Speedster | `speedster.png` |
| Power Striker | `power_striker.png` |
| Grand Wizard | `grand_wizard.png` |
| Trickster | `trickster.png` |
| Defender | `defender.png` |
| Elementalist | `elementalist.png` |
| Sniper | `sniper.png` |
| Chaos Bot | `chaos_bot.png` |

## Fallback behavior

The loader in `src/assets.c` is **tolerant of missing files**: if a texture is absent it logs a
warning and the draw code falls back to a procedural placeholder (a glowing accent circle for
characters). That's why the game looks fine today with no art — drop a PNG in and it upgrades
automatically.

Transparency: use PNGs with an alpha channel. Icons are drawn tinted by each character's accent
color, so **white or near-white art tints best**.

**Always record new assets in [`../CREDITS.md`](../CREDITS.md).**
