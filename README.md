# Ping Pong: Power Clash

A character-based, 2D Pong-inspired game built in C with [raylib](https://www.raylib.com/) 5.5.
Each player picks one of 8 unique characters, each with 2 skills and an ultimate, and battles
on a single-screen arena. First to 5 points wins.

> **Status:** early prototype / work-in-progress. See
> [`docs/TECHNICAL_ANALYSIS.md`](docs/TECHNICAL_ANALYSIS.md) for the current state and known bugs.

---

## Documentation

| Doc | Contents |
| --- | --- |
| [`docs/GAME_DOCUMENTATION.md`](docs/GAME_DOCUMENTATION.md) | Game design, characters, controls, architecture, file map |
| [`docs/TECHNICAL_ANALYSIS.md`](docs/TECHNICAL_ANALYSIS.md) | Build requirements, missing files, bugs, roadmap |

---

## Quick start

### Windows (MinGW-w64)

The repo ships Windows x64 raylib 5.5 (`lib/raylib.dll`, `lib/libraylibdll.a`, `lib/libraylib.a`).

Run the pre-built `game.exe` from the repo root (so the logo asset is found), or rebuild:

```bat
:: Dynamic (uses raylib.dll)
gcc src\*.c -Iinclude -Llib -lraylibdll -o game.exe ^
    -lopengl32 -lgdi32 -lwinmm -lshell32
copy lib\raylib.dll .

:: Static (no DLL needed)
gcc src\*.c -Iinclude -Llib -lraylib -o game.exe ^
    -lopengl32 -lgdi32 -lwinmm -lshell32
```

### Linux

The vendored libraries are **Windows-only** — they will not link on Linux. Install or build a
Linux raylib first, then:

```sh
gcc src/*.c -Iinclude $(pkg-config --cflags --libs raylib) -lm -o game
./game
```

> The current environment has no C compiler, `make`, or `cmake` installed, and no system raylib.
> See the "Missing files" section of the technical analysis before attempting a build.

### Run requirements

- The working directory must contain `pingpongpowerclashlogo.png` (loaded by relative path).
  Preferred fix: copy the asset next to the executable and run from there.

---

## Controls

| Action | Player 1 (left) | Player 2 (right, PvP only) |
| --- | --- | --- |
| Move paddle | `W` / `S` | `↑` / `↓` |
| Skill 1 | `Q` | `Numpad 1` |
| Skill 2 | `E` | `Numpad 2` |
| Ultimate | `R` | `Numpad 3` |
| Menu / select | Left mouse button | — |

---

## Project layout

```
ping-pong-power-clash/
├── game.exe                        # committed Windows x64 build
├── pingpongpowerclashlogo.png      # runtime asset (2048x2048)
├── ref/                            # design reference images (not loaded at runtime)
├── include/                        # raylib 5.5 headers + game headers
├── lib/                            # Windows x64 raylib (static lib, import lib, DLL)
├── src/                            # game source
└── docs/                           # documentation (this folder)
```
