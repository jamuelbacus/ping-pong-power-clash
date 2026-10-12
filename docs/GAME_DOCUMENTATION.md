# Ping Pong: Power Clash — Game Documentation

## 1. Overview

| Field | Value |
| --- | --- |
| Title | Ping Pong: Power Clash |
| Genre | Arcade sports / competitive Pong with fighting-game-style abilities |
| Platform | Desktop (cross-platform via raylib; a Windows build is committed, Linux builds from source) |
| Language | C (C17) |
| Framework | raylib 5.5 |
| Window | First opens at 1280 × 720, **resizable** (minimum 960 × 540), `F11` toggles fullscreen; the game renders to a fixed 960 × 540 virtual canvas that is letterboxed/scaled to the window (`FLAG_WINDOW_RESIZABLE`, `viewport.c`) |
| Players | 1 (vs CPU) or 2 (local PvP) |
| Win condition | First to **5** points (`WINNING_SCORE`) |
| Current state | Playable prototype — three characters fully implemented (Speedster, Grand Wizard, Defender); the rest are roster placeholders |

### Concept

Ping Pong: Power Clash takes classic Pong and layers a roster of 8 characters on top. Each
character has a distinct theme, playstyle, a skill 1, a skill 2, and an ultimate ability. The
character you pick changes how you bend the ball, move your paddle, or disrupt your opponent.
Matches are first-to-5, single arena, no power-ups on the field.

---

## 2. Core gameplay

### The ball

- Spawns at center (`480, 270`), radius `10` (`BALL_SIZE`), red (`BALL_COLOR`).
- The ball has an authoritative **`baseVelocity`** (starts `(300, 200)` px/s —
  `BALL_BASE_VELOCITY_X/Y`) and a derived effective **`velocity`**. Walls and paddles write to
  `baseVelocity`; the effective velocity is re-derived from it every frame.
- Hard speed cap of `800` px/s (`BALL_MAX_VELOCITY`); `CapVelocity` clamps the effective
  velocity after every update.
- Top/bottom wall bounces reflect `baseVelocity.y` and **clamp the position** back inside the
  wall (`radius` from the edge), so the ball can never stick or tunnel through a wall.
- On a paddle hit the bounce angle depends on **where** it struck the paddle: hit distance along
  the paddle face maps to an angle up to ±90°. Each non-shielded hit also multiplies
  `speedFactor` by `1.005` (capped at `2.0`) so rallies speed up over time. The new base magnitude
  is derived from `baseVelocity` (never from an effect-scaled velocity), so timed speed effects
  apply exactly once and are never baked into the base.
- A **point** is scored when the ball leaves the left or right edge:
  - right edge (`x > WINDOW_WIDTH`) → player 1 scores (`playerScore++`),
  - left edge (`x < 0`) → player 2 / CPU scores (`player2Score++`),
  - then `resetBall` re-serves by keeping the last horizontal direction, so the ball heads back
    toward the side that just conceded.

### Curve / lateral bend (Grand Wizard)

- `Curve Spell` and `Arcane Chaos` push a `STAT_BALL_CURVE` acceleration into the effects
  registry. While active, `UpdateBall` accumulates it into `Ball::lateralVelocity` each frame.
- The bend is applied to the effective `velocity` **only** — `baseVelocity` stays authoritative
  for wall bounces, so a timed effect can never leave permanent state behind.
- When the effect expires the bend **decays back to zero** at `CURVE_DECAY_RATE` (400 px/s per
  second). A paddle hit resets the bend instantly.
- Known limitation: the bend currently always accumulates toward `+X` (sign is hard-coded);
  sign control is a follow-up.

### The paddles

- Size `20 × 100` (`PADDLE_WIDTH × PADDLE_HEIGHT`), positioned left at `(50, 200)` and right at
  `(890, 200)`.
- Movement snaps to a max speed on key press and stops instantly on release: player 1 and
  player 2 / CPU share `maxSpeed` 500 px/s (`InitGame` passes `maxSpeed 500` to both paddles).
  The `speed` init argument (300 for player 1, 250 for player 2) is a dead field never read, as
  are the `accel` / `friction` tuning fields — movement snaps instantly (`UpdatePaddle` sets
  `velocityY = ±maxSpeed`). Speedster's **Overdrive** passive re-derives that cap every frame
  from the rally's intensity (see §3): 500 px/s at zero heat, up to 750 px/s at full heat.
- CPU paddles track the ball's Y position directly at `maxSpeed`.
- `UpdatePaddle` **derives effective stats from the effects registry every frame**:
  `maxSpeed` from any `STAT_PADDLE_SPEED` effect, paddle height from any `STAT_PADDLE_SIZE`
  effect (both relative to `originalMaxSpeed` / `originalSize`), and `immovable` from a live
  `STAT_PADDLE_IMMOVABLE` marker. Paddles are kept inside the arena (clamped to `screenHeight`).
- Paddle collisions **separate** the ball by an epsilon past the paddle face, so one contact
  produces exactly one bounce and one `speedFactor` step (no re-trigger on the next frame).

### The arena

- Dark neon theme (`theme.c`): deep indigo-black background (`THEME_BG`), a dim violet 40 px grid
  with a slow horizontal drift, a soft radial glow at center, and additive "glow" primitives for
  panels, lines, circles, and text.
- A bright center line at `x = 480`; scores are drawn at the top quarters, character names
  beneath them; each player's skill HUD panel sits at the bottom (`HUD_P1 {10, 430, 230, 100}`,
  `HUD_P2 {720, 430, 230, 100}`).
- Rendering is unpremultiplied in the 960 × 540 virtual canvas; the presenter (`viewport.c`)
  letterboxes it into the real window with black bars and maps the mouse back into virtual
  coordinates (`GetVirtualMouse`).

### Scoring & winning

- Ball off the **right** edge → player 1 scores; off the **left** edge → player 2 (or CPU) scores.
- First to **5** (`WINNING_SCORE`) wins. The frame after the winning point the match freezes:
  the win check at the top of `updateGamePlay` flips `gameState` to `GAME_OVER` and returns
  before physics. On the scoring frame itself the match is still live — `UpdatePaddle` ×2 and
  `CpuTickAbilities` run once after `resetBall`, so the CPU can fire one final ability whose
  effect (e.g. a HUD active dot) lingers on the frozen GAME_OVER frame.
- At `GAME_OVER` the current match scene (scoreboard, HUD, effect juice) stays drawn — the
  simulation is halted, so nothing moves — with a winner banner overlaid
  (PvP: "Player 1 Wins!" / "Player 2 Wins!"; CPU: "You Win!" / "CPU Wins!"):
  - `SPACE` → back to the main menu,
  - `R` → "rematch": `ResetGame` zeroes both scores, `resetBall` restores the ball, clears all
    effects and armed shield windows, and returns to the **menu** (a fresh match starts from there).
- `resetBall` awards exactly **one** point per miss, repositions the ball to center, resets
  `baseVelocity` (direction kept toward the conceding side), resets `speedFactor` to `1.0`, clears
  the full effects registry, and disarms both paddles' Defender shield windows (`shieldTimer = 0`)
  so **nothing lingers across a point** (R23).

---

## 3. Characters

Eight characters are defined in `src/character.c` and indexed by the `CharacterType` enum in
`include/character.h`. The table below mirrors the source data.

| # | Character | Theme | Playstyle | Skill 1 | Skill 2 | Ultimate | Strength | Weakness |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | Speedster | Cyber runner / ninja | Fast reactions, pressure play — passive **Overdrive**: the hotter the rally, the faster the paddle | Dash Return | Speed Boost | Afterimage Hit | Can save impossible shots | Low power, struggles against heavy shots |
| 1 | Power Striker | Boxer, gladiator, or mech | Slow but devastating | Smash Shot | Heavy Paddle | Meteor Strike | Scores fast points | Poor recovery, easy to outmaneuver |
| 2 | Grand Wizard | Mage, sorcerer | Ball manipulation | Curve Spell | Time Slow | Arcane Chaos | Confuses opponents | Requires timing & prediction |
| 3 | Trickster | Jester, thief, hacker | Deception | Fake Ball | Reverse Spin | Mirror Field | Psychological advantage | Low raw damage |
| 4 | Defender | Knight, guardian, robot shield | Defense & endurance | Shield Block | Wide Paddle | Iron Fortress | Extremely hard to score against | Limited offensive options |
| 5 | Elementalist | Elemental warrior | Status effects | Fire Ball | Ice Ball | Storm Surge | Adaptable | Requires switching elements wisely |
| 6 | Sniper | Sharpshooter, archer | Accuracy-based | Perfect Angle | Piercing Shot | Deadeye | Rewards skill | High skill ceiling |
| 7 | Chaos Bot | Glitched AI, broken machine | Unpredictable | Glitch Bounce | Lag Spike | System Error | Unpredictable fun | Unreliable control |

### Implemented abilities (Speedster, Grand Wizard, Defender)

Three characters have fully wired abilities through the effects registry (`src/abilities.c`).
`AbilitiesActivate` owns each ability's cooldown and pushes the effects that physics/rendering
consume. All values below are the literal constants from the code.

| Character | Ability (key) | Cooldown | Effect |
| --- | --- | --- | --- |
| **Speedster** | Dash Return (`Q`) | 5 s | Instantly snaps the paddle's Y onto the ball's Y and records the pre-snap position into the paddle history, which a `STAT_TRAIL` marker draws for 0.3 s as **true afterimages** (see below). **Flow:** also banks +0.35 heat for 1.5 s and trims Skill 2's cooldown by 1 s (never below 0) |
| | Speed Boost (`E`) | 7 s | `STAT_PADDLE_SPEED` × 2 for 4 s — it multiplies the *heat-scaled* base, so it stacks cleanly on top of Overdrive. **Flow:** same +0.35 heat for 1.5 s, trims Skill 1's cooldown by 1 s (never below 0) |
| | Afterimage Hit (`R`) | 15 s | `STAT_BALL_SPEED` × 1.35 for 5 s + **ball** `STAT_TRAIL` for 5 s. The trail is a true afterimage: ghosts sit at recorded positions and fade out over 0.5 s, so the echo traces the ball's real arc (including Grand Wizard curve bends) |
| **Grand Wizard** | Curve Spell (`Q`) | 5 s | `STAT_BALL_CURVE` SET 220 for 3 s — a lateral bend that accumulates into `ball.lateralVelocity` (+X), on top of the effective velocity only |
| | Time Slow (`E`) | 7 s | `STAT_BALL_SPEED` × 0.5 for 2 s |
| | Arcane Chaos (`R`) | 15 s | `STAT_BALL_CURVE` SET 400 for 5 s (stronger bend; same registry key as Curve Spell, so re-casting refreshes it) |
| **Defender** | Shield Block (`Q`) | 5 s | Arms a 5 s reflect window (`paddle->shieldTimer`). The next hit that strikes this paddle reflects at base speed × 1.3 (`DEFENDER_SHIELD_REFLECT_MULT`) and **skips** the ×1.005 `speedFactor` creep; the window is consumed on hit and decays over time if unused |
| | Wide Paddle (`E`) | 7 s | `STAT_PADDLE_SIZE` × 1.8 for 4 s (paddle height, derived through `UpdatePaddle`) |
| | Iron Fortress (`R`) | 15 s | `STAT_PADDLE_SIZE` SET `FORTRESS_PADDLE_HEIGHT` (540) + `STAT_PADDLE_IMMOVABLE` for 5 s — the paddle locks to full arena height and becomes immovable (`UpdatePaddle` derives `immovable` from the marker every frame) |

Notes:

- **Ball-targeted effects are keyed at `paddleIndex = -1`** — the index the ball reads
  regardless of which player activated the ability; ownership is tracked in the effect's
  `.source` so each player's HUD dot lights up correctly.
- `EffectsPush` **full-replaces** an existing slot that matches `(target, paddleIndex, stat,
  source)`, so re-casting an ability makes the latest activation authoritative (even if the
  operation differs, e.g. Wide Paddle then Iron Fortress).
- `EffectsApply` composes all matching MULs → all ADDs → the first SET; when the last effect
  of a kind expires, the stat returns to its base value **exactly**.
- Rendering is purely query-driven: ball tint/auras/trails and the defender shield barrier are
  drawn from the registry (see §5, `visuals.c`).

### Speedster passive — Overdrive

Overdrive is the game's only **passive**, and it is character-innate: whoever plays Speedster
gets it, including the CPU (no CPU-only gating). It drives one hidden **heat** scalar in `[0, 1]`
that widens the paddle's max speed every frame, before any `STAT_PADDLE_SPEED` multiplier runs:

```
maxSpeed = 500 × (1 + 0.5 × heat)      →  500 px/s at heat 0 … 750 px/s at heat 1
```

Three inputs feed that one scalar, so the fastest paddle in the match is the one that is already
under pressure:

| Input | Trigger | Heat contributed |
| --- | --- | --- |
| **Rally Heat** | Always on — every frame, from the ball's *current effective* speed | `clamp((ballSpeed − 360.5551) / 360.5551, 0, 1)` — the serve speed `\|(300, 200)\|` is the floor (heat 0), and at roughly twice that the meter is full. Afterimage Hit's ×1.35 raises heat; a Grand Wizard Time Slow cools it |
| **Flow** | Casting Dash Return (`Q`) or Speed Boost (`E`) | `+0.35` for 1.5 s, plus the cast **trims the other skill's cooldown by 1 s** (never below 0). Chaining `Q` → `E` is the intended rhythm; re-casting refreshes the window, it never stacks |
| **Clutch** | Returning the ball off the **outer 25 % of your paddle** (top or bottom edge band) | `SET` to `1.0` for 0.5 s — full heat — **and** the return launches with its own ball afterimage for 0.5 s |

Rules that fall out of the model:

- Rally + Flow are summed and then **clamped to 1**: full heat is full heat (750 px/s), never
  837 px/s or anything else in between.
- **Clutch overrides Flow**, it does not add to it — a clutch return sets heat to 1 for its
  shorter 0.5 s window. Because heat is keyed per paddle, the latest activation on that paddle's
  heat slot is the only one that counts.
- Heat is strictly per-paddle: player 1's rally can never push player 2's speed cap.
- The "danger band near your wall" is read as the **edge band of the paddle height**
  (`hitPos < 0.25 || hitPos > 0.75`). Both paddles sit at a fixed x (left / right), so every
  return happens near the end wall and a literal wall-distance reading would fire on every hit;
  the outer quarter is the honest version of the same gamble — a risky return that pays out.
  Centre returns stay calm.
- Balance is deliberately conservative and all of the numbers above are tunable constants.

### True afterimages

Trail ghosts are no longer a comet tail sliding behind the entity — they **never move**. While a
`STAT_TRAIL` marker is live, the entity's current position is recorded every frame into a fixed
30-entry ring (`src/trail.c`), and `DrawEffects` (`src/game.c`) draws each surviving record as a
ghost that fades out over 0.5 s, sitting exactly where the entity has *been*. All three Speedster
trails use it: the ball trail of Afterimage Hit / Clutch (and the paddle trail of Dash Return)
are decaying position echoes, not velocity offsets. Consequences:

- The ball echo traces the ball's real path, including Grand Wizard curve bends.
- Dash Return parks the paddle's **pre-snap** position before it teleports, so a dash leaves a
  footprint where it flashed *from* plus echoes along the destination.
- Scoring wipes all three rings, so nothing ghosts across a serve.

### CPU ability usage

The CPU (`CpuTickAbilities`, `src/abilities.c`) runs on a **decision interval**, not per-frame:

- A decision tick happens at most every `CPU_DECISION_INTERVAL` (1 s).
- Per tick **at most one** ability may fire, gated on (a) the ball actually approaching the CPU
  paddle, and (b) a single `CPU_DIFFICULTY` roll (`0.5` = 50% chance).
- Among ready abilities it prefers the strongest: ultimate (2) > skill 2 (1) > skill 1 (0).
- The decision timer resets to the interval after every tick (whether or not anything fired), so
  it never drifts negative or accumulates.

---

## 4. Controls

| Action | Player 1 (left) | Player 2 (right, PvP) |
| --- | --- | --- |
| Move paddle up / down | `W` / `S` | `↑` / `↓` |
| Skill 1 | `Q` | `Numpad 1` |
| Skill 2 | `E` | `Numpad 2` |
| Ultimate | `R` | `Numpad 3` |
| Menu / character select | Left mouse button | Left mouse button |
| Game over → back to menu | `SPACE` | `SPACE` |
| Game over → rematch (returns to menu) | `R` | `R` |
| Toggle fullscreen (any state) | `F11` | `F11` |

Cooldowns: skill 1 = 5 s, skill 2 = 7 s, ultimate = 15 s (`layout.h`). Durations are
per-ability as listed in §3.

> Player 2's ability keys are chosen at runtime by paddle X position (`paddle->position.x >
> 480` → numpad). Because the pads are always on the same halves this is stable in practice, but
> the numpad dependence is worth keeping in mind for keyboards without one.

---

## 5. Architecture

### State machine (`GameState` in `include/game.h`)

```
MENU ──click "Player vs CPU"/"Player vs Player"──▶ CHARACTER_SELECT
CHARACTER_SELECT ──pick character(s)──▶ PLAY_CPU  or  PLAY_PVP
PLAY_CPU / PLAY_PVP ──score reaches WINNING_SCORE──▶ GAME_OVER
GAME_OVER ──SPACE──▶ MENU
GAME_OVER ──R (ResetGame)──▶ MENU
```

| State | Behaviour |
| --- | --- |
| `MENU` | Logo + two mode buttons. Mouse click sets CPU/PvP mode and moves to `CHARACTER_SELECT`. |
| `CHARACTER_SELECT` | 4 × 2 grid of the 8 characters (icons from `assets/characters/*.png`, glow-circle fallback if missing). PvP asks both players ("PLAYER 1" then "PLAYER 2"); CPU mode picks a random CPU character after player 1 chooses and jumps straight to `PLAY_CPU`. |
| `PLAY_CPU` / `PLAY_PVP` | Live match: `EffectsUpdate` → win check → `UpdateBall` → scoring/`resetBall` → `UpdatePaddle` ×2 → `CpuTickAbilities` (CPU only). |
| `GAME_OVER` | Simulation frozen (`updateGamePlay` returns before physics); winner banner + prompt drawn over the last frame. |

### Main loop (`src/main.c`)

```c
SetConfigFlags(FLAG_WINDOW_RESIZABLE);
InitWindow(1280, 720, WINDOW_TITLE);
SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);   // 960 x 540
SetTargetFPS(60);
while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
    float dt = GetFrameTime();
    UpdateGame(&game, dt);
    BeginViewport(&viewport);          // 960 x 540 render target
    ClearBackground(THEME_BG);
    DrawGame(&game);
    EndViewport(&viewport);            // letterboxed presentation + mouse mapping
}
```

### Module responsibilities

| File | Responsibility |
| --- | --- |
| `src/main.c` | Window setup (resizable, min size, 60 FPS), main loop, F11 fullscreen, viewport present |
| `src/game.c` | State machine, scene update/draw, scoring + `resetBall`, `ResetGame`, `DrawEffects` juice rendering |
| `src/ball.c` | Ball init, physics (`UpdateBall`: curve accumulation/decay, wall/paddle collisions, velocity derivation + cap), drawing |
| `src/paddle.c` | Paddle init, movement, CPU tracking, cooldowns, shield decay, effective-stat derivation (`maxSpeed`/size/`immovable`), player ability input, drawing |
| `src/abilities.c` | `AbilitiesActivate` (cooldown + effect push per character/ability) and the CPU decision policy `CpuTickAbilities` |
| `src/effects.c` | Fixed 64-slot timed effect registry: push (refresh-first), update/expiry, `EffectsApply` (MUL→ADD→SET), queries |
| `src/trail.c` | Fixed-capacity true-afterimage ring (`TrailHistory`): record positions, age them, expire at `TRAIL_MAX_AGE` |
| `src/visuals.c` | Pure visual-state queries over the registry (`EffectTintColor`, `EffectTrailActive`, `PlayerHasActiveEffect`, `ShieldActive`) — no drawing, unit-testable |
| `src/character.c` | Roster data (`characters[8]`: names, themes, playstyles, skills, accents) |
| `src/theme.c` | Neon palette + glow drawing primitives + background grid |
| `src/viewport.c` | Fixed-resolution render target, letterboxed presentation, `GetVirtualMouse` |
| `src/assets.c` | Texture loading with graceful fallback (logo + 8 character icons) |
| `src/utils.c` | `UpdateTimer` helper |
| `include/layout.h` | Named UI layout constants (menu buttons, char-grid rects, HUD rects, cooldown maxima) shared by draw code and hit-tests |

### Key data structures

- `Game` (`include/game.h`): owns both paddles, the ball, both scores (`playerScore`,
  `player2Score`), `gameState`, selection state, assets, and the `Effects` registry.
- `Ball` (`include/ball.h`): `position`, `baseVelocity` (source of truth — collisions write
  here), `velocity` (derived effective value physics moves by), `radius`, `color`, `speedFactor`
  (rally creep, ×1.005 per non-shielded hit, cap 2.0), `lateralVelocity` (curve bend), and
  `trail` (a `TrailHistory` afterimage ring).
  `Effects` applies on top through `STAT_BALL_SPEED` / `STAT_BALL_VEL_X/Y` / `STAT_BALL_SIZE`
  / `STAT_BALL_CURVE` / `STAT_TRAIL`.
- `Effects` (`include/effects.h`): fixed 64-slot array of `Effect` entries; each entry keys
  `(target, paddleIndex, stat, op(eration), value, timeLeft/duration, color, source)`.
- `TrailHistory` (`include/trail.h`): fixed 30-entry ring of `{ pos, age, live }` samples
  (`TRAIL_MAX_GHOSTS` ghosts, `TRAIL_MAX_AGE` 0.5 s), embedded as `Ball::trail` and both
  `Paddle::trail` — the true-afterimage position history.
- `Paddle` (`include/paddle.h`): transform + movement basics, `maxSpeed` base, cooldowns,
  `cpuDecisionTimer`, `shieldTimer` (armed reflect window), `immovable` (derived per frame from
  `STAT_PADDLE_IMMOVABLE`), `originalSize`/`originalMaxSpeed` (the bases effects scale from),
  `trail` (afterimage history), plus legacy dead fields (see §6).
- `Character` (`include/character.h`): fixed-size `char` arrays for name/theme/playstyle/3
  skills/strength/weakness + neon `accent` color.

### Data flow

```
input (keyboard / mouse / CPU policy)
        │
        ▼
UpdateGame ──▶ EffectsUpdate (expire timed effects)
        │
        ├─▶ win check (≥5) ──▶ GAME_OVER (freeze)
        │
        ├─▶ UpdateBall ──▶ curve accumulate/decay ──▶ walls + paddle1 + paddle2
        │        │            (collisions write baseVelocity; shield paths skip creep;
        │        │             clutch edge returns spike heat + arm the ball trail)
        │        └─▶ out of bounds ──▶ score++ → resetBall (effects + shields cleared)
        │
        ├─▶ UpdatePaddle ×2 ──▶ cooldowns / shield decay / derived stats (Speedster: heat
        │        │              feeds maxSpeed) / input / move / trail history record
        │
        └─▶ CpuTickAbilities (decision interval, one roll, ult-first, approach-gated)

DrawGame ──▶ DrawMenu / DrawCharacterSelect / DrawGameplay
                 └─▶ DrawHUD + DrawEffects (registry-driven tints, trails, shield barrier, dots)
```

---

## 6. Current state & known issues

The game is a playable prototype: menu → select → match → game-over → menu flow works, three
characters' abilities are fully wired, and rendering is registry-driven. Issues fixed along the
way (now resolved and guarded by regression tests in `tests/`):

**Resolved**

1. **Scoring / `resetBall` bug** — the ball is recentered on every miss, exactly one point is
   awarded, effects and paddle shield windows are cleared across points. (`test_reset_ball`)
2. **No game-over state** — `GAME_OVER` freezes simulation; `SPACE` → menu, `R` → reset.
   (`test_game_over`)
3. **PvP win text** — winner banner derives from `GameWinner` + `isCpu`, so CPU/PvP wording is
   correct.
4. **Slow/lag effects never expire** — replaced by the timed effects registry over an
   authoritative `baseVelocity`; expiry restores base exactly.
5. **Skills as dead flags** — Speedster / Grand Wizard / Defender abilities push registry effects
   that physics and rendering consume.
6. **Wall/paddle collision artifacts** — wall bounces clamp position; paddle hits separate the
   ball (one contact = one bounce/creep step).
7. **`ResetGame` never called** — wired to `R` at game over.
8. **`cpuScore` naming** — renamed to `player2Score`. (`isCpu` stays.)
9. **Hard-coded UI coordinates** — moved to named `layout.h` constants.
10. **Per-frame CPU rolls** — replaced by the decision-interval policy.
11. **No `SetTargetFPS`** — fixed at 60; window is resizable with F11 fullscreen and a neon theme.
12. **Fake afterimages** — the old velocity-offset comet tail was replaced by the position-history
    ring in `src/trail.c`: ghosts sit where the entity has actually been and fade out by age, so
    the echo traces the real path (including curve bends). (`test_trail`)
13. **No Speedster passive** — Overdrive adds heat-driven paddle speed (Rally Heat / Flow / Clutch)
    for whoever plays Speedster, CPU included. (`test_speedster`)

**Remaining / follow-up**

- **Legacy dead fields** still live in the structs (placeholders for unimplemented characters).
  In `Ball`: `slowTimer`, `sizeMultiplier`, `unpredictableBounce`, `lagTimer`, `randomEffects` —
  only zeroed/initialized, never consumed by physics (rendering reads `sizeMultiplier`, which no
  live effect modifies). In `Paddle`: `speed`, `accel`, `friction`, `skill1Timer`,
  `skill2Timer`, `ultimateTimer`, `skill1Active`, `skill2Active`, `ultimateActive`,
  `speedMultiplier`, `sizeMultiplier`, `mirrorField`, `heavyPaddle`, `smashShotReady`,
  `piercingShot`, `widthMultiplier`, `deadeye`. Removing them is cleanup, not gameplay work.
  The `BALL_SPEED_INCREMENT` constant in `include/ball.h` (20 px/s) is likewise defined but never
  used.
- **Ability balance & feel** — tuning is deliberately conservative; numbers, durations, and the
  CPU difficulty (50%) are all constants that need play-testing.
- **Curve direction** — the lateral bend always accumulates toward `+X`; making the sign follow
  the caster or velocity is a candidate follow-up.
- **Other characters** — Power Striker, Trickster, Elementalist, Sniper, Chaos Bot are defined as
  data (names/themes/accents) but have **no ability bodies** yet; their selector cards render and
  play with no skills.
- **Cosmetic notes** — no audio yet (`assets/audio` is an empty placeholder); the logo and
  character icons load by relative path (run from the repo root); `accel` / `friction` / `speed`
  tuning fields are unused by movement.

---

## 7. Build & assets

See [`../README.md`](../README.md) for build commands and
[`TECHNICAL_ANALYSIS.md`](TECHNICAL_ANALYSIS.md) for environment/platform notes.
`tests/README.md` documents the headless regression suites and their exact build recipes.

### Game build (Linux/macOS, raylib via pkg-config)

```sh
gcc src/*.c -Iinclude $(pkg-config --cflags --libs raylib) -lm -o game
./game
```

`main.c` opens a **resizable** window (min 960 × 540), targets 60 FPS, toggles fullscreen on
`F11`, and presents the 960 × 540 virtual canvas letterboxed.

### Headless tests

All tests live in `tests/` and build outside `src/` (see `tests/README.md` for recipes and
expected `RESULT: PASS` output). The suites cover the effects registry, effective-stat
derivation, ball physics, reset/scoring, game-over behaviour, CPU policy, each implemented
character, the visual-state queries, and the afterimage position-history ring.

### Runtime assets

| File(s) | Used? |
| --- | --- |
| `pingpongpowerclashlogo.png` | Yes — menu logo, scaled to 250 px wide |
| `assets/characters/*.png` (8 icons) | Yes — character-select cards, tinted by accent; graceful glow-circle fallback if missing |
| `ref/pingpongpowerclashrefmenuscreen.png` | No — design reference only |
| `lib/raylib.dll` (+ import/static libs) | Windows build only |
| `assets/audio|backgrounds|fonts|ui/` | Placeholders (`.gitkeep`) — no audio/fonts loaded yet |

All other visuals are raylib primitives (neon glow shapes).
