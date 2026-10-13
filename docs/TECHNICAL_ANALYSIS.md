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
- The game is a **playable prototype**: match loop, first-to-7 scoring, game-over/rematch, with
  powerups and a hazards toggle (off by default). Hit quality and streak effects are tracked. 
  Three fully implemented characters (Speedster, Grand Wizard, Defender) driven by a timed
  effects registry. Speedster additionally carries the **Overdrive** heat passive (Rally Heat /
  Flow / Clutch → paddle max speed), and every trail — ball and dash — is a true position-history
  afterimage from the fixed-capacity ring in `src/trail.c`. Gameplay draw order: centre line →
  scoreline HUD → ball → decoys → paddles → effects, with the HUD drawn first and translucently so
  the ball always renders above it. `SkillRecoverFraction` computes per-skill cooldown recovery for the
  HUD bars. The remaining five characters are roster data only.

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

The regression suites in `tests/` build separately from `src/`. Full recipes for all **eleven**
test programs, with their exact source lists, are in [`tests/README.md`](../tests/README.md).

---

## 5. Codebase health

### 5.1 Structure

Clean, small, well-separated C modules in C17 with matching headers. Constants live in headers;
`Game` owns the world; `layout.h` centralizes UI geometry so draw code and hit-tests can't drift.
The architecture is a good foundation and has grown the effects/abilities layer without new
allocation, plus the pure, allocation-free position-history ring (`src/trail.c`) that backs all
afterimage rendering.

### 5.2 Current architecture: effects registry + abilities

All ability effects go through a **fixed 64-slot timed registry** (`include/effects.h`,
`src/effects.c`). There is no dynamic allocation.

- **`Effect` entry** — `{ active, target, paddleIndex, stat, op, value, timeLeft, duration,
  color, source }`. `target` is `TARGET_BALL` or `TARGET_PADDLE`. `stat` is one of
  `STAT_BALL_SPEED|VEL_X|VEL_Y|SIZE`, `STAT_PADDLE_SPEED|SIZE`, `STAT_TRAIL`,
  `STAT_BALL_CURVE`, `STAT_PADDLE_IMMOVABLE`, and (appended last) `STAT_PADDLE_HEAT`. `op` is
  `OP_MUL | OP_ADD | OP_SET`.
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
  `STAT_PADDLE_IMMOVABLE` marker) every frame — and, for a Speedster paddle only, derives
  `maxSpeed` from `STAT_PADDLE_HEAT` first (see §5.6). Rendering is **query-driven**: `visuals.c`
  exposes `EffectTintColor` / `EffectTrailActive` / `PlayerHasActiveEffect` / `ShieldActive`
  (pure, headless-testable), and `DrawEffects` in `game.c` draws ball tint/aura/trail, paddle
  aura + dash ghosts, and the defender shield barrier from those queries.
- **Activation & CPU** — `AbilitiesActivate` (in `src/abilities.c`) owns every ability's
  cooldown and pushes its effects. `CpuTickAbilities` implements the decision-interval policy
  (one roll per `CPU_DECISION_INTERVAL`, approach-gated, ult > skill2 > skill1, at most one
  ability per tick) — and because Speedster's passive is character-innate, the CPU benefits from
  Overdrive whenever it picks Speedster, with no separate gating (Ruling C2). Both are detailed in
  `GAME_DOCUMENTATION.md` §3.
- **Ruling C1 — clutch "wall band" is the paddle edge band.** The design called for a danger band
  near the returner's end wall, but both paddles sit at a fixed x (`50` / `890`), so every return
  happens within ~70 px of the wall and a literal wall-distance reading would fire on every hit.
  The band is therefore read off the **paddle height**: `hitPos < 0.25 || hitPos > 0.75`, the
  outer quarter of the face (`OVERDRIVE_CLUTCH_EDGE`). Concept name stays "Clutch band".
- **Ruling C3 — latest activation is authoritative.** All heat effects share the registry key
  `(TARGET_PADDLE, idx, STAT_PADDLE_HEAT, source=idx)`, so `EffectsPush` **refreshes** one slot
  instead of stacking. Intended consequences: re-using `Q`/`E` while Flow is live refreshes the
  window rather than adding; a Clutch `OP_SET` replaces a live Flow `OP_ADD` (and the SET pass
  beats the ADD pass); and a clutch ball-trail push refreshes the acting paddle's own ball
  `STAT_TRAIL` slot (same `source`) rather than stacking a second one.

### 5.3 Ball model (authoritative base + derived velocity)

- `Ball.baseVelocity` is the **source of truth**; wall/paddle collisions write here.
- `Ball.velocity` is derived each frame from `baseVelocity` + active registry effects
  (`ApplyBallEffects`), so a timed slow/speed/bend can never leave permanent state behind.
- `Ball.speedFactor` creeps ×1.005 per **unpowered** paddle hit (cap 2.0) and contributes to
  the base magnitude on each bounce. Two one-shot armed windows replace the creep with a fixed
  base-speed multiplier, each consumed by the contact: Defender's Shield Block (×1.3,
  `paddle->shieldTimer`) and Speedster's Afterburner (×1.5, `paddle->afterburnerTimer`, clamped
  to `BALL_MAX_VELOCITY`).
- `Ball.lateralVelocity` rides on the effective velocity only; `baseVelocity` stays pristine
  through the curve bend.
- Paddle collisions separate the ball past the paddle face by `PADDLE_SEPARATION_EPSILON`
  (0.5 px beyond tangency), so one contact produces exactly one bounce and one creep step.

### 5.4 Legacy dead fields (present but unused)

Confirmed against `include/ball.h` and `include/paddle.h`. These are placeholders for
unimplemented characters/skills — no live code reads them (except where noted):

**Live fields that must NOT be confused with dead ones:** `Ball::trail`, `Paddle::trail` (the
afterimage history rings, §5.6), `Ball::decoys` (the Shadow Clone swarm, §5.7),
`Paddle::shieldTimer` (Defender's armed window) and `Paddle::afterburnerTimer` (Speedster's
charged-return window, §5.8) are all
consumed. The dash and trail features carry **no dead flag of their own** — "is a dash trail
running?" is answered by a live `STAT_TRAIL` registry marker, and the ghost positions come from
recorded history, not from a boolean or timer field:

**`Ball`** — `slowTimer` (only zeroed in `InitBall`/`resetBall`; never read), `sizeMultiplier`
(read by ball drawing but always 1.0 — no live effect pushes `STAT_BALL_SIZE`),
`unpredictableBounce`, `lagTimer`, `randomEffects` (init/zeroed only).

**`Paddle`** — `speed`, `accel`, `friction` (set at init; movement snaps to `maxSpeed` instead
of using them), `skill1Timer`, `skill2Timer`, `ultimateTimer`, `skill1Active`, `skill2Active`,
`ultimateActive`, `speedMultiplier`, `sizeMultiplier`, `mirrorField`, `heavyPaddle`,
`smashShotReady`, `piercingShot`, `widthMultiplier`, `deadeye`.

**Also unused** — `BALL_SPEED_INCREMENT` (`include/ball.h`, 20 px/s): defined but never
referenced. Ball size is not effect-driven today (nothing pushes `STAT_BALL_SIZE`); baseline
rally speed growth comes from the per-bounce `speedFactor *= 1.005` creep, while live
speed/bend changes come from `STAT_BALL_SPEED` / `STAT_BALL_CURVE` registry effects.

**Removed entirely in this branch** (no longer in the structs): `cpuScore` (renamed to
`player2Score` — `isCpu` stays), `curveAngle` (replaced by the registry `STAT_BALL_CURVE` +
`Ball::lateralVelocity`), `shieldBlock` (replaced by `Paddle::shieldTimer`), and the fake-ball /
illusions fields — the trail is now a real data structure (`Ball::trail` / `Paddle::trail`,
`TrailHistory` in `include/trail.h`) that `DrawEffects` renders from, so `STAT_TRAIL` is no longer
a marker nothing reads; its ghosts come from recorded history, not from any `*Active` / `*Timer`
flag.

### 5.5 Compiler-warning checklist

`gcc -std=c17 -Wall -Wextra src/*.c ...` builds clean except one known, benign warning:
`include/game.h`: `WINDOW_TITLE` is a `static const` used only by `main.c`, so the declaration
in the header is unused in other translation units. No other warnings.

### 5.6 Speedster Overdrive (heat) and true afterimages

**Heat.** One scalar in `[0, 1]` per paddle, derived every frame for Speedster only; all constants
live in `include/overdrive.h`:

| Source | Rule | Where |
| --- | --- | --- |
| **Rally** (always on) | `rally = clamp((\|ball->velocity\| - BALL_SPEED_BASE) / BALL_SPEED_BASE, 0, 1)`, computed from the **effective** velocity, so a hard rally raises heat and Time Slow ×0.5 lowers it. `BALL_SPEED_BASE` is the float32 rounding of `\|(400,267)\|` (the serve speed), which is what keeps rally heat at zero at the tuned serve speed | `src/paddle.c` `UpdatePaddle` |
| **Flow** (`Q`/`E` use) | `OP_ADD` `OVERDRIVE_FLOW_HEAT` (0.35) for `OVERDRIVE_FLOW_DURATION` (1.5 s), plus trimming the **other** ability's cooldown by `OVERDRIVE_FLOW_TRIM` (1 s), clamped at 0 | `src/abilities.c`, SPEEDSTER cases 0 and 1 |
| **Clutch** (edge return) | `OP_SET` `OVERDRIVE_CLUTCH_HEAT` (1.0) for `OVERDRIVE_CLUTCH_DURATION` (0.5 s) **plus** a ball `OP_SET` `STAT_TRAIL` for `OVERDRIVE_CLUTCH_TRAIL` (0.5 s), keyed at the acting paddle's index (0 and 1 blocks mirror each other) | `src/ball.c`, inside both paddle-collision branches |

Derivation in `UpdatePaddle`:

```c
float heat = EffectsApply(fx, TARGET_PADDLE, idx, STAT_PADDLE_HEAT, rally);  /* rally + flow, clamped */
heat = clamp(heat, 0.0f, 1.0f);
float base = paddle->originalMaxSpeed * (1.0f + OVERDRIVE_SPEED_PER_HEAT * heat);  /* 320 .. 480 */
paddle->maxSpeed = EffectsApply(fx, TARGET_PADDLE, idx, STAT_PADDLE_SPEED, base);
```

Non-Speedster paddles keep the exact pre-existing derivation (`originalMaxSpeed` → `STAT_PADDLE_SPEED`).
Note `BALL_SPEED_BASE` (`include/overdrive.h`) is load-bearing beyond heat: it must stay
bit-identical to `sqrtf(400*400 + 267*267)` so a serve-speed ball yields heat exactly 0.

**True afterimages.** `include/trail.h` + `src/trail.c` add a pure, fixed-capacity position ring
(no allocation):

- `TRAIL_MAX_GHOSTS 30` (≈0.5 s at 60 fps), `TRAIL_MAX_AGE 0.5f`.
- `TrailGhost { Vector2 pos; float age; bool live; }` — one position sample; `TrailHistory {
  TrailGhost ghosts[30]; int head; }` — the ring.
- `TrailHistoryRecord` pushes at `head` and advances it modulo the capacity (oldest slot is
  overwritten); `TrailHistoryUpdate` ages every live ghost and expires it at
  `age >= TRAIL_MAX_AGE`; `TrailActiveGhosts(h, n, out)` copies the live ghosts **newest-first**
  (returning how many fit in `n`).
- Embedded as `Ball::trail` and `Paddle::trail` (both `Paddle`s get one), initialized in
  `InitBall` / `InitPaddle`.
- **Recording points:** `UpdateBall` calls `TrailHistoryUpdate(&ball->trail, dt)` and records
  `ball->position` only while a ball `STAT_TRAIL` marker is live; `UpdatePaddle` does the same for
  its own trail. Dash Return (`AbilitiesActivate`, SPEEDSTER case 0) records the **pre-snap**
  position before teleporting, so the dash leaves a footprint where it flashed from. `resetBall`
  re-inits all three rings so a mid-point trail never ghosts across a serve.
- **Rendering:** `DrawEffects` (`src/game.c`) gates on the registry markers (`EffectTrailActive`,
  R22 — the registry stays the only source of truth for *whether* a trail exists), then draws
  each live ghost as a fading accent circle/rect at its recorded position, alpha falling from
  `0.5` toward `0.1` as `age / TRAIL_MAX_AGE`. This replaced the old velocity-offset "comet tail"
  ghosts, which used to glide with the entity.

### 5.7 Speedster Shadow Clone (visual decoys)

The Speedster ultimate is a **pure illusion** ability: it spawns decoy balls that look and move
like the ball but are not physics entities. It lives entirely outside the effects registry.

- `include/decoy.h` + `src/decoy.c` define a fixed-capacity `DecoySwarm` (no allocation):
  `Decoy { Vector2 position; Vector2 velocity; float age; bool live; }`,
  `DECOY_MAX 2` / `DECOY_SPAWN_COUNT 2`, `DECOY_LIFETIME 1.2f`, `DECOY_FADE_TIME 0.4f`.
- `DecoySwarmSpawn` clears the swarm and re-emits `DECOY_SPAWN_COUNT` clones at the ball's
  position, each clone's heading rotated off the ball's velocity by `DECOY_SPREAD` (30°) so the
  pair fans out. Re-activation replaces rather than stacks, so the swarm never exceeds capacity.
- `DecoySwarmUpdate` advances each clone, reflects it off the **top/bottom walls only**
  (reflection radius `DECOY_RADIUS`, matching `BALL_SIZE`), and retires it at `DECOY_LIFETIME`.
  Clones never query the registry, collide with a paddle, or touch the score.
- Embedded as `Ball::decoys` (initialized in `InitBall` and wiped by `resetBall`);
  `updateGamePlay` ticks it right after `UpdateBall` (so it freezes with the sim at `GAME_OVER`),
  and `DrawGameplay` calls `DrawDecoys` to draw each live clone exactly like the real ball with
  `DecoyAlpha(age)` opacity (opaque → 0 over the final `DECOY_FADE_TIME`).
- Abilities: `AbilitiesActivate` SPEEDSTER case 2 calls `DecoySwarmSpawn`. It produces **no**
  registry effect, so the old Afterimage Hit `STAT_BALL_SPEED ×1.35` + ball-`STAT_TRAIL` buffs are
  gone with it.

### 5.8 Speedster Afterburner (charged power return)

The Speedster's Skill 2 (`E`) is a **charged return**: press `E` to arm a short window, and the
next ball that paddle returns launches at a fixed speed spike. It reuses the same one-shot
armed-window pattern as Defender's Shield Block, one field over.

- `include/abilities.h`: `AFTERBURNER_SPEED_MULT` (1.5), `AFTERBURNER_WINDOW` (2.0 s).
- `AbilitiesActivate` SPEEDSTER case 1 sets `paddle->afterburnerTimer = AFTERBURNER_WINDOW` (raw
  paddle state, like `shieldTimer`) and still pushes the Flow heat pulse + trims `Q`'s cooldown,
  so the `Q` → `E` rhythm survives the rework.
- `UpdatePaddle` decays the window each frame (floor 0), alongside `shieldTimer`.
- `UpdateBall` consumes it on the next paddle contact in **both** paddle branches: the return uses
  `baseSpeed × AFTERBURNER_SPEED_MULT` instead of `baseSpeed × speedFactor`, skips the creep step,
  and is clamped to `BALL_MAX_VELOCITY` (800). `resetBall` disarms it like the shield.
- `visuals.c` exposes the pure query `AfterburnerArmed(paddle)`; `DrawEffects` draws a charged
  outline in the Speedster accent while armed, and the HUD active-dot condition includes it (R22).
- The CPU Speedster gets it for free: `CpuTickAbilities` already prefers Skill 2 over Skill 1.

Replaces the old **Speed Boost** (`STAT_PADDLE_SPEED × 2` for 4 s), which overlapped both Dash
Return and Overdrive and left the kit with no offense.

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
   tuning fields, and any balance feedback from testing. The tuned gameplay values are now
   `PADDLE_HEIGHT 76`, paddle `maxSpeed 320`, `BALL_BASE_VELOCITY_(X,Y)` (400,267),
   `BALL_SPEED_BASE ~480.93` with the HUD using `SkillRecoverFraction` and inverted draw order
   (HUD first, ball above). The canvas is **1024 × 576**, match win is **first to 7** (`WINNING_SCORE`), window title is **"Ping Pong: Power Clash"** (`WINDOW_TITLE`). Fixed-pool **powerups** (Speed Boost, Enlarge Paddle, Shrink Opponent, Heat Pulse) spawn periodically, are collected on collision, and cleared on reset. **Hazards** toggle (off by default) controls a moving central obstacle. **Hit quality** (Perfect/Good/Weak) on paddle contact drives per-player **streaks** with streak effects at thresholds.
