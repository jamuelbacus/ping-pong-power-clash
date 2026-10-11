# Ping Pong: Power Clash — Technical Analysis

Scope: what files the project needs to build and run with raylib, what is missing, the current
technical health of the codebase, and how the effects/abilities architecture works. Companion to
[`GAME_DOCUMENTATION.md`](GAME_DOCUMENTATION.md).

---

## 1. TL;DR

- The repo still has **no build system file** — no `Makefile`, `CMakeLists.txt`, or
  `build.sh`/`build.bat`. Builds are one-line `gcc` commands (Linux) or `gcc` + the vendored
  MinGW libraries (Windows), documented in `README.md`.
- The vendored raylib libraries and the committed binary are **Windows x64 only**. On
  Linux/macOS you must supply your own raylib (system package or built from source) and a
  compiler; the raylib **headers** (`include/`) are platform-independent.
- `.gitignore` now ignores the local Linux `game` binary, `*.o`/`*.obj`, and test binaries;
  `game.exe` and the Windows libs are intentionally tracked.
- The game is a **playable prototype**: match loop, first-to-5 scoring, game-over/rematch, and
  three fully implemented characters (Speedster, Grand Wizard, Defender) driven by a timed
  effects registry. The remaining five characters are roster data only.

**To build on this machine:** a compiler + raylib for your OS, then the one-liner in §4.

---

## 2. Environment findings

Checked on this machine (Linux):

| Tool | Status |
| --- | --- |
| `gcc` | installed and working (C17) |
| `make` / `cmake` | not present |
| `pkg-config` | present |
| system raylib | present (`pkg-config --cflags --libs raylib`) |

Binary formats present in the repo:

| File | Format |
| --- | --- |
| `game.exe` | PE32+ executable (console), x86-64, MS Windows |
| `lib/raylib.dll` | PE32+ DLL, x86-64, MS Windows |
| `lib/libraylib.a` | `ar` archive of Windows AMD64 COFF objects (MinGW) |
| `lib/libraylibdll.a` | Windows import library (for `raylib.dll`) |
| `include/raylib.h` | raylib **5.5** headers |

### What this means

- **On Windows:** everything is present. MinGW-w64 `gcc` plus the README build command; dynamic
  linking needs `raylib.dll` beside the executable (or on `PATH`).
- **On Linux/macOS:** supply your own raylib and compiler. The vendored `.a`/`.dll`/`.exe` are
  unusable; the headers can be kept or replaced with matching-version headers.

> raylib 5.5 must match your library version to avoid ABI/API surprises. Building raylib from the
> `5.5` tag (or `make install` from source) is safest if a distro package isn't available.

---

## 3. Missing files

### 3.1 Required to build / run

| Missing file | Why it's needed |
| --- | --- |
| **Build system** (`Makefile`, `CMakeLists.txt`, or `build.sh`/`build.bat`) | Nothing currently drives compilation; `game.exe` is a checked-in artifact and Linux builds are manual one-liners. |
| **`.vscode/tasks.json` / `launch.json`** | `.vscode/` contains only `c_cpp_properties.json`; no build or debug configuration. |
| **Platform-appropriate raylib** | The vendored libs are Windows-only; Linux/macOS need their own. |

### 3.2 Strongly recommended

| Missing file | Why |
| --- | --- |
| **Asset path handling** | Logo + character icons load by relative path from the working directory; run the binary from the repo root (existing, noted as cosmetic). |
| **Audio/fonts** | `assets/audio|fonts|ui|backgrounds/` are placeholder folders (`.gitkeep`); nothing plays or renders from them yet. |

> `.gitignore` **exists** now (`/game`, `*.o`, `*.obj`, `tests/test_reset_ball`, `private/`).
> The old analysis that it was "missing entirely" is outdated.

### 3.3 Note on `.vscode/c_cpp_properties.json`

Configured with `"intelliSenseMode": "windows-gcc-x64"` and `_DEBUG`/`UNICODE` defines. If you
develop on Linux, set `intelliSenseMode` to a Linux GCC/Clang value so IntelliSense resolves the
right platform headers. `cStandard` is `c17`.

---

## 4. Building

There is no build system file; the working commands are documented in `README.md`.

### 4.1 Linux/macOS (system raylib via pkg-config)

```sh
gcc src/*.c -Iinclude $(pkg-config --cflags --libs raylib) -lm -o game
./game
```

### 4.2 Windows (MinGW-w64, vendored raylib 5.5)

```bat
gcc src\*.c -Iinclude -Llib -lraylibdll -o game.exe -lopengl32 -lgdi32 -lwinmm -lshell32
copy lib\raylib.dll .
```

### 4.3 Suggested Makefile (POSIX / MinGW-aware) — not yet added

```make
CC      ?= gcc
SRC     := $(wildcard src/*.c)
INC     := -Iinclude

# --- Linux/macOS (raylib via pkg-config) ---
CFLAGS  := -std=c17 -Wall -Wextra -O2
LDLIBS  := $(shell pkg-config --libs raylib 2>/dev/null || echo -lraylib -lm -lpthread -ldl -lrt -lX11)

game: $(SRC)
	$(CC) $(CFLAGS) $(INC) $(SRC) $(LDLIBS) -o $@

.PHONY: clean run
clean:
	rm -f game game.exe
run: game
	./game
```

### 4.4 Headless tests

The regression suites in `tests/` build separately from `src/`. Full recipes for all **ten**
test programs, with their exact source lists, are in [`tests/README.md`](../tests/README.md).

---

## 5. Codebase health

### 5.1 Structure

Clean, small, well-separated C modules in C17 with matching headers. Constants live in headers;
`Game` owns the world; `layout.h` centralizes UI geometry so draw code and hit-tests can't drift.
The architecture is a good foundation and has grown the effects/abilities layer without new
allocation.

### 5.2 Current architecture: effects registry + abilities

All ability effects go through a **fixed 64-slot timed registry** (`include/effects.h`,
`src/effects.c`). There is no dynamic allocation.

- **`Effect` entry** — `{ active, target, paddleIndex, stat, op, value, timeLeft, duration,
  color, source }`. `target` is `TARGET_BALL` or `TARGET_PADDLE`. `stat` is one of
  `STAT_BALL_SPEED|VEL_X|VEL_Y|SIZE`, `STAT_PADDLE_SPEED|SIZE`, `STAT_TRAIL`,
  `STAT_BALL_CURVE`, `STAT_PADDLE_IMMOVABLE`. `op` is `OP_MUL | OP_ADD | OP_SET`.
- **Keying & refresh (R21)** — a slot's key is `(target, paddleIndex, stat, source)`.
  `EffectsPush` **full-replaces** a matching live slot (including `op` and value), so the latest
  activation is authoritative — re-casting an ability, even one hitting the same stat with a
  different operation, refreshes rather than stacks. Distinct `source`s (both players) stack.
  When full, the slot nearest expiry is evicted.
- **Application order (R21)** — `EffectsApply` composes: all matching `OP_MUL`s → all matching
  `OP_ADD`s → the **first** `OP_SET`. Worst case is exact; when every effect of a kind expires,
  the stat returns to its base value exactly.
- **Ball vs paddle targeting** — ball effects are keyed at `paddleIndex = -1`, the index
  `UpdateBall` always reads, so either player's ability lands on the ball; the owner is carried
  in `.source`. Paddle effects are keyed at the owning paddle's index.
- **Lifecycle** — `EffectsUpdate(dt)` ticks every active entry and deactivates it at
  `timeLeft <= 0`, and is called before physics each frame so effects that end that frame are
  gone before `UpdateBall`/`UpdatePaddle` query them. `resetBall`/`ResetGame` call `EffectsClear`.
- **Consumers** — `UpdateBall` derives effective velocity from `baseVelocity` via
  `STAT_BALL_SPEED` then `STAT_BALL_VEL_X/Y`, accumulates `STAT_BALL_CURVE` into
  `Ball::lateralVelocity` (decays at `CURVE_DECAY_RATE` on expiry, reset on paddle hit), and
  caps the result at `BALL_MAX_VELOCITY`. `UpdatePaddle` derives `maxSpeed`, paddle height
  (`STAT_PADDLE_SIZE`, bases `originalMaxSpeed`/`originalSize`) and `immovable` (a live
  `STAT_PADDLE_IMMOVABLE` marker) every frame. Rendering is **query-driven**: `visuals.c`
  exposes `EffectTintColor` / `EffectTrailActive` / `PlayerHasActiveEffect` / `ShieldActive`
  (pure, headless-testable), and `DrawEffects` in `game.c` draws ball tint/aura/trail, paddle
  aura + dash ghosts, and the defender shield barrier from those queries.
- **Activation & CPU** — `AbilitiesActivate` (in `src/abilities.c`) owns every ability's
  cooldown and pushes its effects. `CpuTickAbilities` implements the decision-interval policy
  (one roll per `CPU_DECISION_INTERVAL`, approach-gated, ult > skill2 > skill1, at most one
  ability per tick). Both are detailed in `GAME_DOCUMENTATION.md` §3.

### 5.3 Ball model (authoritative base + derived velocity)

- `Ball.baseVelocity` is the **source of truth**; wall/paddle collisions write here.
- `Ball.velocity` is derived each frame from `baseVelocity` + active registry effects
  (`ApplyBallEffects`), so a timed slow/speed/bend can never leave permanent state behind.
- `Ball.speedFactor` creeps ×1.005 per **non-shielded** paddle hit (cap 2.0) and contributes to
  the base magnitude on each bounce. Defender's Shield Block replaces the creep with a fixed
  ×1.3 reflect and consumes the paddle's `shieldTimer` window.
- `Ball.lateralVelocity` rides on the effective velocity only; `baseVelocity` stays pristine
  through the curve bend.
- Paddle collisions separate the ball past the paddle face by `PADDLE_SEPARATION_EPSILON`
  (0.5 px beyond tangency), so one contact produces exactly one bounce and one creep step.

### 5.4 Legacy dead fields (present but unused)

Confirmed against `include/ball.h` and `include/paddle.h`. These are placeholders for
unimplemented characters/skills — no live code reads them (except where noted):

**`Ball`** — `slowTimer` (only zeroed in `InitBall`/`resetBall`; never read), `sizeMultiplier`
(read by ball drawing but always 1.0 — no live effect pushes `STAT_BALL_SIZE`),
`unpredictableBounce`, `lagTimer`, `randomEffects` (init/zeroed only).

**`Paddle`** — `speed`, `accel`, `friction` (set at init; movement snaps to `maxSpeed` instead
of using them), `skill1Timer`, `skill2Timer`, `ultimateTimer`, `skill1Active`, `skill2Active`,
`ultimateActive`, `speedMultiplier`, `sizeMultiplier`, `mirrorField`, `heavyPaddle`,
`smashShotReady`, `piercingShot`, `widthMultiplier`, `deadeye`.

**Removed entirely in this branch** (no longer in the structs): `cpuScore` (renamed to
`player2Score` — `isCpu` stays), `curveAngle` (replaced by the registry `STAT_BALL_CURVE` +
`Ball::lateralVelocity`), `shieldBlock` (replaced by `Paddle::shieldTimer`), and the fake-ball /
illusions fields (ball trail now comes from the registry's `STAT_TRAIL` markers).

### 5.5 Compiler-warning checklist

`gcc -std=c17 -Wall -Wextra src/*.c ...` builds clean except one known, benign warning:
`include/game.h`: `WINDOW_TITLE` is a `static const` used only by `main.c`, so the declaration
in the header is unused in other translation units. No other warnings.

---

## 6. Recommended next steps

1. **Add a build system** (`Makefile` and/or `.vscode/tasks.json`) so the Linux and Windows
   builds stop being manual one-liners.
2. **Remove the legacy dead struct fields** (§5.4) once the unimplemented characters land or are
   cut — pure cleanup, no behavior change.
3. **Implement the remaining five characters' ability bodies** (Power Striker, Trickster,
   Elementalist, Sniper, Chaos Bot) as effects-registry pushes — their roster data and accents
   already exist.
4. **Play-test and tune the ability slate** (§5.2 values and the CPU difficulty are constants
   chosen conservatively).
5. **Curve sign control** — the lateral bend currently always accumulates toward `+X`;
   sign-following-caster or velocity-based direction would be a good follow-up.
6. **Audio** (placeholders exist in `assets/audio`), then polish: asset-path handling, dead
   tuning fields, and any balance feedback from testing.