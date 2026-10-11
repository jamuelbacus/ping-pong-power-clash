# Ping Pong: Power Clash — Technical Analysis

Scope: what files are needed for the project to build and run with raylib, what is missing, and
the current technical health of the codebase. Companion to
[`GAME_DOCUMENTATION.md`](GAME_DOCUMENTATION.md).

---

## 1. TL;DR

- The repo **does not contain a build system** — no `Makefile`, `CMakeLists.txt`, or VS Code build
  task. The committed `game.exe` was built some other way.
- The vendored raylib libraries and the committed binary are **Windows x64 only**. This cannot be
  built or linked on Linux/macOS as-is.
- There is **no `.gitignore`**, so the 1.4 MB `.exe` and 5.2 MB of Windows libraries are committed.
- The game **runs** (Windows), but the scoring loop is broken and many skill effects are stubs.

**Minimum additional files needed to work on this machine:** a compiler + raylib for your OS, and
a build script. Details below.

---

## 2. Environment findings

Checked on this machine (Linux):

| Tool | Status |
| --- | --- |
| `gcc` / `cc` / `clang` | **not found** |
| `make` | **not found** |
| `cmake` | **not found** |
| `pkg-config` | present |
| system raylib | **not found** (`pkg-config --exists raylib` fails; no `/usr/**/raylib.h`) |

Binary formats present in the repo:

| File | Format |
| --- | --- |
| `game.exe` | PE32+ executable (console), x86-64, MS Windows |
| `lib/raylib.dll` | PE32+ DLL, x86-64, MS Windows |
| `lib/libraylib.a` | `ar` archive of Windows AMD64 COFF objects (MinGW) |
| `lib/libraylibdll.a` | Windows import library (for `raylib.dll`) |
| `include/raylib.h` | raylib **5.5** headers |

### What this means

- **On Windows:** everything is present. You need MinGW-w64 (`gcc`) and the build command in the
  README. Dynamic linking needs `raylib.dll` beside the executable (or on `PATH`).
- **On Linux/macOS:** you must supply your own raylib (system package or built from source) and a
  compiler. The vendored `.a`/`.dll`/`.exe` are unusable. The raylib **headers** (`include/`) are
  platform-independent and can be kept or replaced with matching-version headers.

> raylib 5.5 must match your library version to avoid ABI/API surprises. Building raylib from the
> `5.5` tag (or `make install` from source) is the safest route if a distro package isn't available.

---

## 3. Missing files

### 3.1 Required to build / run

| Missing file | Why it's needed |
| --- | --- |
| **Build system** (`Makefile`, `CMakeLists.txt`, or `build.sh`/`build.bat`) | Nothing currently drives compilation. `game.exe` is a checked-in artifact. |
| **`.vscode/tasks.json`** | `.vscode/` contains only `c_cpp_properties.json`; no build task is defined. |
| **`.vscode/launch.json`** | No debug configuration. |
| **Platform-appropriate raylib** | The vendored libs are Windows-only; Linux/macOS need their own. |
| **C toolchain** | None is installed in this environment. |

### 3.2 Strongly recommended

| Missing file | Why |
| --- | --- |
| **`.gitignore`** | Missing entirely. Should ignore `game.exe`, `*.o`, `*.exe`, `build/`, `raylib.dll` (or keep it intentionally), editor cruft. |
| **`README`** | Was absent; now added. |
| **Asset folder + path handling** | Logo is loaded via bare relative path. Copy assets next to the binary or use a resolved path. |
| **`assets/` (sounds, fonts, sprites)** | Not required yet — no audio/fonts are loaded — but needed as the game grows. |

### 3.3 Note on `.vscode/c_cpp_properties.json`

Currently configured with `"intelliSenseMode": "windows-gcc-x64"` and `_DEBUG`/`UNICODE` defines.
If you develop on Linux, set `intelliSenseMode` to a Linux GCC/Clang value; otherwise IntelliSense
resolves the wrong platform headers.

---

## 4. Suggested build files

These are proposals — they were **not** added to the repo (no compiler is available here to verify
them, and the target platform needs confirmation first).

### 4.1 `Makefile` (POSIX / MinGW-aware)

```make
CC      ?= gcc
SRC     := $(wildcard src/*.c)
INC     := -Iinclude

# --- Linux/macOS (raylib via pkg-config) ---
CFLAGS  := -std=c17 -Wall -Wextra -O2
LDLIBS  := $(shell pkg-config --libs raylib 2>/dev/null || echo -lraylib -lm -lpthread -ldl -lrt -lX11)

# --- Windows/MinGW alternative (comment the block above, use: ) ---
# LDLIBS := -Llib -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32

game: $(SRC)
	$(CC) $(CFLAGS) $(INC) $(SRC) $(LDLIBS) -o $@

.PHONY: clean run
clean:
	rm -f game game.exe
run: game
	./game
```

### 4.2 `.gitignore`

```gitignore
# build output
build/
*.o
*.obj
game
game.exe

# pulled/copied runtime libs
raylib.dll

# editors / OS
.vscode/*
!.vscode/c_cpp_properties.json
!.vscode/tasks.json
!.vscode/launch.json
*.swp
.DS_Store
```

### 4.3 `.vscode/tasks.json` (build task)

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "build",
      "type": "shell",
      "command": "make",
      "group": { "kind": "build", "isDefault": true },
      "problemMatcher": ["$gcc"]
    }
  ]
}
```

---

## 5. Codebase health

### 5.1 Structure

Clean, small, well-separated C modules (ball / paddle / character / game / utils) with matching
headers and a sensible state machine. Constants live in headers; `Game` owns the world. The
architecture is a good foundation.

### 5.2 Defects

Ordered by severity. Line references are to the current source.

**A. `resetBall()` is a no-op — `src/game.c:17-19`**
```c
static void resetBall(Game* game) {
    float velX = (game->ball.velocity.x > 0) ? BALL_BASE_VELOCITY_X : -BALL_BASE_VELOCITY_X;
}   // velX unused; nothing is written back
```
Consequence: after a point, the ball is never recentered and keeps moving off-screen. Because the
out-of-bounds check runs every frame, the score increments every frame until it hits
`WINNING_SCORE` almost instantly. **A real match is currently impossible.** It should reset
position to `BALL_POSITION`, velocity to base (sign toward the player who was scored on), and clear
effect state (`speedFactor`, `slowTimer`, `lagTimer`, `curveAngle`, illusions/fake-ball flags).

**B. No game-over state — `src/game.c:71-111`, `119-142`**
`UpdateGame` continues simulating in `PLAY_CPU`/`PLAY_PVP` regardless of score; `DrawGame` merely
draws the win text instead. Add a `GAME_OVER` state that halts simulation and offers a rematch
(and call the existing, currently-unused `ResetGame`).

**C. PvP win text — `src/game.c:122`** says `"CPU Wins!"` even in player-vs-player.

**D. Slow/lag effects never expire — `src/ball.c:46-74`**
`velocity *= 0.8` / `*= 0.9` is applied **per frame** while the timer runs, and nothing restores the
velocity when the timer reaches 0 — so the ball is permanently slowed after the first use. This is
the bug called out in the WIP commit. Fix: store a base velocity and derive the effective velocity
each frame (e.g. `velocity = base * factor`), or restore the cached pre-effect velocity on expiry.

**E. Wall collision doesn't clamp — `src/ball.c:81-83`**
Velocity flips but position isn't pushed back inside, so the ball can oscillate/stick at a wall.

**F. Paddle collision doesn't separate — `src/ball.c:85-103`**
No positional correction on hit, so an overlapping ball can re-trigger the bounce on the next
frame, re-applying `speedFactor *= 1.005` repeatedly.

**G. Dead skill flags — `src/paddle.c` / `src/ball.c`**
`heavyPaddle`, `smashShotReady`, `shieldBlock`, `piercingShot`, `deadeye`, `mirrorField`,
`widthMultiplier`, `ball.unpredictableBounce`, `ball.randomEffects`, `ball.sizeMultiplier` are set
but never read by physics or rendering. Several "skills" therefore do nothing.

**H. Unused tuning fields — `src/paddle.c`**
`speed`, `accel`, `friction` are stored but ignored; movement snaps directly to `maxSpeed`.

**I. CPU skill rolls are per-frame — `src/paddle.c:225,256,287`**
`GetRandomValue(0,1000) < 5` etc. evaluated every frame (~60×/s) makes even the `< 1` "rare"
ultimate fire often. Use a cooldown-gated chance or time-based roll.

**J. Asset path — `src/game.c:5`**
`LoadTexture("pingpongpowerclashlogo.png")` depends on the process working directory. Running the
binary from anywhere else silently yields a 0-width texture (the menu guards for it, so no crash).

**K. Miscellaneous**
`updateGamePlay` checks `ball.position.x > WINDOW_WIDTH` (right edge) — fine, but the boundary
constants are duplicated rather than derived from paddle/ball geometry. `ResetGame` is never
called. No `SetTargetFPS`. No audio init. PvP shares `cpuScore`/`isCpu` naming.

### 5.3 Compiler-warning checklist

Expect warnings (once compiled with `-Wall -Wextra`) from: unused `velX` in `resetBall`, unused
parameters, and possibly comparing `int`/`float`. Worth a clean `-Wall -Wextra` build pass.

---

## 6. Recommended next steps

1. **Decide the target platform** (Windows only, or cross-platform). This determines which raylib
   to vendor and which build system to use.
2. **Add a build system** (`Makefile` and/or `CMakeLists.txt`) and a `.vscode/tasks.json`.
3. **Add `.gitignore`** and stop committing the binary + libs (or document why they're kept).
4. **Fix `resetBall` and add a game-over state** — this is what makes the game actually playable.
5. **Fix slow/lag** by tracking base vs. effective ball velocity.
6. **Clamp/separate collisions** to remove wall-stick and paddle re-trigger artifacts.
7. **Wire up or delete dead skill flags** so every listed skill has an observable effect.
8. **Add audio + asset folder** and load assets by a resolved path.
