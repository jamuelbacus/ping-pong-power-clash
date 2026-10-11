# Ping Pong: Power Clash — Game Documentation

## 1. Overview

| Field | Value |
| --- | --- |
| Title | Ping Pong: Power Clash |
| Genre | Arcade sports / competitive Pong with fighting-game-style abilities |
| Platform | Desktop (Windows build committed; raylib is cross-platform) |
| Language | C (C17) |
| Framework | raylib 5.5 |
| Window | 960 × 540, non-resizable, title "Ping Pong: Power Clash" |
| Players | 1 (vs CPU) or 2 (local PvP) |
| Win condition | First to **5** points (`WINNING_SCORE`) |
| Current state | Playable prototype — core loop works, several systems are stubs/buggy |

### Concept

Ping Pong: Power Clash takes classic Pong and layers a roster of 8 characters on top. Each
character has a distinct theme, playstyle, two regular skills, and an ultimate ability. The
character you pick changes how you bend the ball, move your paddle, or disrupt your opponent.
Matches are first-to-5, single arena, no power-ups on the field.

---

## 2. Core gameplay

### The ball

- Spawns at center (`480, 270`), radius `10`, red.
- Base velocity `(300, 200)` px/s; hard speed cap of `800` px/s (`BALL_MAX_VELOCITY`).
- Bounces off the top/bottom walls using angle-based reflection.
- On a paddle hit, the bounce angle depends on **where** it struck the paddle (top edge →
  steep up, center → flat, bottom edge → steep down). Each successful hit also increases an
  internal `speedFactor` (×1.005 per hit, capped at 2.0) so rallies speed up over time.
- A point is scored when the ball leaves the left or right edge.

### The paddles

- Size `20 × 100`, positioned left (`x = 50`) and right (`x = 890`).
- Movement is **instant**: the paddle snaps to `maxSpeed` (500 px/s) on key press and stops
  immediately on release. Acceleration/friction fields exist but are not applied.
- CPU paddle tracks the ball's Y position directly at its own max speed (250 px/s).

### The arena

- Plain white background, black center line at `x = 480`.
- Scores drawn at the top quarters; character names and a skill HUD at the bottom.

### Scoring & winning (intended)

- Ball off the **right** edge → player 1 scores.
- Ball off the **left** edge → player 2 (or CPU) scores.
- First to 5 → win screen. *(Currently broken — see §6.)*

---

## 3. Characters

Eight characters are defined in `src/character.c` and indexed by the `CharacterType` enum in
`include/character.h`. The table below mirrors the source data.

| # | Character | Theme | Playstyle | Skill 1 | Skill 2 | Ultimate | Strength | Weakness |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | Speedster | Cyber runner / ninja | Fast reactions, pressure play | Dash Return | Speed Boost | Afterimage Hit | Can save impossible shots | Low power, struggles against heavy shots |
| 1 | Power Striker | Boxer, gladiator, or mech | Slow but devastating | Smash Shot | Heavy Paddle | Meteor Strike | Scores fast points | Poor recovery, easy to outmaneuver |
| 2 | Grand Wizard | Mage, sorcerer | Ball manipulation | Curve Spell | Time Slow | Arcane Chaos | Confuses opponents | Requires timing & prediction |
| 3 | Trickster | Jester, thief, hacker | Deception | Fake Ball | Reverse Spin | Mirror Field | Psychological advantage | Low raw damage |
| 4 | Defender | Knight, guardian, robot shield | Defense & endurance | Shield Block | Wide Paddle | Iron Fortress | Extremely hard to score against | Limited offensive options |
| 5 | Elementalist | Elemental warrior | Status effects | Fire Ball | Ice Ball | Storm Surge | Adaptable | Requires switching elements wisely |
| 6 | Sniper | Sharpshooter, archer | Accuracy-based | Perfect Angle | Piercing Shot | Deadeye | Rewards skill | High skill ceiling |
| 7 | Chaos Bot | Glitched AI, broken machine | Unpredictable | Glitch Bounce | Lag Spike | System Error | Unpredictable fun | Unreliable control |

### Skill implementation status

The skill *cooldown/timer plumbing* is implemented in `src/paddle.c`, but many individual
effects are **cosmetic only** — the field they set is never read by the physics or drawing code.

| Character | Skill 1 effect (code) | Skill 2 effect (code) | Ultimate effect (code) | Works? |
| --- | --- | --- | --- | --- |
| Speedster | teleport paddle to ball Y | `speedMultiplier = 2` | `ball.illusions = true` (drawn) | Partial |
| Power Striker | sets `smashShotReady` | sets `heavyPaddle` | adds `+200` to `velocity.y` | Partial (ult works; flags dead) |
| Grand Wizard | sets `ball.curveAngle = 100` | sets `ball.slowTimer` | sets `ball.randomEffects` | Partial (curve/slow buggy) |
| Trickster | fake ball spawned (drawn) | reverses `velocity.x` | sets `mirrorField` | Partial |
| Defender | sets `shieldBlock` | `sizeMultiplier = 2` | `immovable = true` (paddle spans height) | Partial |
| Elementalist | `velocity.x *= 1.5` | sets `ball.slowTimer` | `velocity.x *= 2` | Partial (slow buggy) |
| Sniper | (no-op) | sets `piercingShot` | sets `deadeye` | Mostly dead |
| Chaos Bot | sets `unpredictableBounce` | sets `ball.lagTimer` | sets `ball.randomEffects` | Partial (lag buggy) |

> Fields that are **written but never consumed** ("dead flags"): `ball.unpredictableBounce`,
> `ball.randomEffects`, `ball.sizeMultiplier`, `paddle.heavyPaddle`, `paddle.smashShotReady`,
> `paddle.shieldBlock`, `paddle.piercingShot`, `paddle.deadeye`, `paddle.mirrorField`,
> `paddle.widthMultiplier`.

### CPU ability usage

The CPU rolls a random chance each frame to fire a skill:
skill 1 = `GetRandomValue(0,1000) < 5`, skill 2 = `< 3`, ultimate = `< 1`.
Because this is a *per-frame* roll (~60×/s), even the "rare" ultimate fires often in practice.

---

## 4. Controls

| Action | Player 1 | Player 2 (PvP) |
| --- | --- | --- |
| Move up / down | `W` / `S` | `↑` / `↓` |
| Skill 1 (5 s cooldown) | `Q` | `Numpad 1` |
| Skill 2 (7 s cooldown) | `E` | `Numpad 2` |
| Ultimate (15 s cooldown) | `R` | `Numpad 3` |
| Menu / character select | Left mouse | Left mouse |

Skill durations: skill 1 = 3 s, skill 2 = 4 s, ultimate = 5 s.

> Player 2's keys are chosen at runtime by paddle X position (`x > 480` → numpad). This is
> fragile and keyboard-layout dependent.

---

## 5. Architecture

### State machine (`GameState` in `include/game.h`)

```
MENU ──click "Player vs CPU"/"Player vs Player"──▶ CHARACTER_SELECT
CHARACTER_SELECT ──pick character(s)──▶ PLAY_CPU  or  PLAY_PVP
```

- `MENU`: logo + two mode buttons.
- `CHARACTER_SELECT`: 4×2 grid of the 8 characters; PvP asks for both players, CPU mode picks
  a random CPU character after player 1 chooses.
- `PLAY_CPU` / `PLAY_PVP`: live match.

### Main loop (`src/main.c`)

```c
InitWindow(...);
InitGame(&game);
while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateGame(&game, dt);
    BeginDrawing();
    ClearBackground(WHITE);
    DrawGame(&game);
    EndDrawing();
}
UnloadTexture(game.logo);
CloseWindow();
```

### Module responsibilities

| File | Responsibility |
| --- | --- |
| `src/main.c` | Window setup, main loop, texture unload |
| `src/game.c` | Game state machine, scene update/draw, scoring, `resetBall`, `ResetGame` |
| `src/ball.c` | Ball init, physics (`UpdateBall`), rendering, effects, velocity cap |
| `src/paddle.c` | Paddle init, movement, CPU AI, skill/cooldown system, rendering |
| `src/character.c` | Roster data (`characters[8]`) |
| `src/utils.c` | `UpdateTimer` helper |
| `include/*.h` | Structs, enums, tuning constants, prototypes, vendored raylib headers |

### Key data structures

- `Game` (`game.h`): owns both paddles, the ball, both scores, current state, selection state, logo.
- `Ball` (`ball.h`): position/velocity/radius/color + effect state (`slowTimer`, `curveAngle`,
  `illusions`, `fakeBall*`, `sizeMultiplier`, `unpredictableBounce`, `lagTimer`, `randomEffects`).
- `Paddle` (`paddle.h`): transform + movement tuning + full skill/cooldown/effect state + `charType`.
- `Character` (`character.h`): fixed-size `char` arrays for name/theme/playstyle/skills/strength/weakness.

### Data flow

```
input (keyboard/mouse)
        │
        ▼
UpdateGame ──▶ UpdateBall ──▶ collisions (walls, paddle1, paddle2) ──▶ effects
        │                │
        │                └─▶ score check → resetBall
        ▼
UpdatePaddle ×2 ──▶ movement + cooldowns + skill triggers + timers
        │
        ▼
DrawGame ──▶ DrawMenu / DrawCharacterSelect / DrawGameplay + HUD
```

---

## 6. Current state & known issues

The game is an **early prototype**. The menu → select → match flow runs, paddles move, the ball
bounces, and the skill/cooldown framework is in place. However it is not yet a complete game.

### Critical

1. **Scoring never resets the ball.** `resetBall()` in `src/game.c` is a no-op — it declares a
   local `velX` and never assigns anything. Once the ball leaves the screen it stays out of
   bounds, so the score increments **every frame** until it instantly reaches the win threshold.
   This makes a real match impossible.
2. **No game-over state.** `UpdateGame` keeps simulating the match even after a score reaches
   `WINNING_SCORE`; only the *drawing* switches to the win text. The ball keeps ticking and the
   score keeps climbing behind it.
3. **Win text is wrong in PvP** — it always says "CPU Wins!" for player 2.

### High

4. **Slow/lag effects are permanent** (the WIP commit's known issue). `UpdateBall` multiplies
   `velocity` by `0.8` (slow) / `0.9` (lag) *every frame* while the timer runs and never restores
   the pre-effect velocity, so the ball never recovers its speed. No "base velocity" is stored.
5. **Skills are largely dead flags.** See §3 — most effects set fields nothing reads.
6. **Wall collision doesn't reposition the ball.** Velocity flips but position isn't clamped,
   allowing the ball to stick at a wall and flip repeatedly.
7. **Paddle collision doesn't separate the ball.** Overlapping frames can re-trigger the bounce
   and re-multiply `speedFactor`.

### Medium / low

8. `ResetGame` is defined but never called; no pause, rematch, or return-to-menu after a win.
9. Logo is loaded by relative path — fails (silently, width 0) if CWD isn't the repo root.
10. Dead tuning fields: `paddle.speed`, `accel`, `friction`; movement ignores them.
11. PvP player 2 is still named `cpuScore`/`isCpu` throughout the code.
12. Hard-coded UI coordinates instead of named layout constants.
13. CPU skill chance is a per-frame roll, making "rare" ultimates frequent.
14. No audio at all (no sounds/music loaded).
15. No `SetTargetFPS`; framerate is uncapped (physics are `deltaTime`-based, so it's stable,
    but CPU/power use is unbounded).

### Suggested fix order

1. Implement `resetBall` (center ball, restore base velocity, clear effect state, reset `speedFactor`).
2. Add a `GAME_OVER` / win state; stop simulation, allow rematch.
3. Store base ball velocity; make slow/lag multiplicative *relative to base* and restore on expiry.
4. Clamp ball position on wall/paddle collisions and separate on hit.
5. Wire up or remove the dead skill flags.
6. Fix PvP win text and naming.

---

## 7. Build & assets

See [`../README.md`](../README.md) for build commands and
[`TECHNICAL_ANALYSIS.md`](TECHNICAL_ANALYSIS.md) for the full list of files that are still
needed (build system, VS Code tasks, `.gitignore`, platform-appropriate raylib).

**Runtime assets**

| File | Size | Used? |
| --- | --- | --- |
| `pingpongpowerclashlogo.png` | 2048 × 2048 | Yes — menu logo, scaled to 250 px wide |
| `ref/pingpongpowerclashrefmenuscreen.png` | 1536 × 1024 | No — design reference only |
| `lib/raylib.dll` | — | Yes on Windows (dynamic link) |

No fonts, sounds, or sprites are loaded; all other visuals are raylib primitives.
