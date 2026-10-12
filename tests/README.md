# Tests

Small headless regression tests. They live outside `src/`, so the normal
`gcc src/*.c ...` build and the shipping game are unaffected. None of them opens a window —
they exercise the pure logic and the pure visual-state queries directly.

Each recipe below was verified from the **project root**; every binary is written to
`/tmp/<name>` and exits `0` with `RESULT: PASS` when the suite passes.

## Summary table

| Test | What it guards | Link set (sources besides `tests/<name>.c`) |
| --- | --- | --- |
| `test_effects` | Registry semantics: MUL/ADD/SET order, same-source refresh vs distinct-source stack, expiry restores base | `src/effects.c` |
| `test_effective_stats` | Base-vs-derived velocity; paddle speed ×2 and restore; effects applied exactly once | `abilities effects ball paddle character theme trail` |
| `test_ball_physics` | Wall clamp/bounce, `resetBall` (clears effects, recenters), timed-effect expiry, one contact = one bounce/creep | `visuals theme viewport assets ball paddle character effects utils abilities trail` |
| `test_game_over` | Reaching 5 enters `GAME_OVER`; simulation frozen there | `visuals theme viewport assets ball paddle character effects utils abilities trail` |
| `test_reset_ball` | A single miss awards exactly one point and returns the ball to play; `resetBall` disarms shields | `visuals theme viewport assets ball paddle character effects utils abilities trail` |
| `test_cpu_policy` | One decision tick fires at most one ability; decision timer never negative | `abilities effects ball paddle character utils theme trail` |
| `test_speedster` | Dash snap + pre-snap footprint, Speed Boost ×2, Afterimage Hit ×1.35 + ball trail at index −1; Overdrive heat (rally floor/cap, Flow push + cooldown trim, Clutch edge spike) | `abilities effects ball paddle character theme trail` |
| `test_wizard` | Curve Spell/Arcane Chaos strengths + durations, self-restoring bend, base untouched | `abilities effects ball paddle character theme trail` |
| `test_defender` | Shield arm/consume/reflect (×1.3, no creep), Wide Paddle ×1.8, Iron Fortress full height + immovable | `abilities effects ball paddle character theme trail` |
| `test_visuals` | Pure visual queries match the registry: tint, trail, active-dot ownership, shield window | `visuals abilities effects ball paddle character theme trail` |
| `test_trail` | Pure position-history ring: record/age/expire order, newest-first readout, fixed capacity, ghosts never move | `trail` |

> `src/theme.c` is needed even by "logic-only" suites because `paddle.c` / `ball.c` draw
> functions reference `DrawGlowRect` / `DrawGlowCircle` (`theme.c`) and `characters[]`
> (`character.c`) in the same translation units. `src/abilities.c` is needed whenever
> `paddle.c` is linked — its skill-input block references `AbilitiesActivate`.
> `src/trail.c` is needed whenever `ball.c` / `paddle.c` / `abilities.c` / `game.c` is
> linked — every one of them records into a `TrailHistory` ring.

---

## `test_effects`

Registry behaviour: empty registry is identity, `OP_MUL` → `OP_ADD` → first `OP_SET`
application order, same `(target,index,stat,source)` **refreshes** rather than stacks, distinct
sources stack, and expiry returns the base value exactly.

```sh
gcc tests/test_effects.c src/effects.c -Iinclude \
    $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_effects
/tmp/test_effects
```

Expected output: `RESULT: PASS` (exit code 0).

## `test_effective_stats`

Guards the base-vs-derived ball model: `velocity == baseVelocity` with no effects; a paddle
bounce derives its new base magnitude from `baseVelocity` (so a temporary `STAT_BALL_SPEED`
effect applies exactly once and is never baked into the base); a ×2 speed effect yields a ×2
effective magnitude; paddle `STAT_PADDLE_SPEED` ×2 doubles `maxSpeed` and expiry restores it.

```sh
gcc tests/test_effective_stats.c src/abilities.c src/effects.c src/ball.c src/paddle.c \
    src/character.c src/theme.c src/trail.c -Iinclude \
    $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_effective_stats
/tmp/test_effective_stats
```

Expected output: `RESULT: PASS` (exit code 0).

## `test_ball_physics`

Includes `../src/game.c` directly (like `test_reset_ball`) to reach the file-static
`resetBall()`/`updateGamePlay()`, so the recipe carries `game.c`'s full link set:
`visuals.c theme.c viewport.c assets.c` are present because `game.c`'s `DrawEffects`/HUD-dot
code calls the pure visual queries (R22) and `InitGame` loads assets.

Checks: top-wall clamp + bounce, `resetBall` clears effects and recenters, timed effects expire
through a game-loop tick, and one paddle contact produces exactly one bounce / one
`speedFactor` step with clean separation past the paddle face.

```sh
gcc tests/test_ball_physics.c \
    src/visuals.c src/theme.c src/viewport.c src/assets.c \
    src/ball.c src/paddle.c src/character.c src/effects.c src/utils.c src/abilities.c \
    src/trail.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_ball_physics
/tmp/test_ball_physics
```

Expected output: `RESULT: PASS` (exit code 0).

## `test_game_over`

Includes `../src/game.c` (same link set as `test_ball_physics`/`test_reset_ball`). Guards the
lifecycle rule: reaching `WINNING_SCORE` (5) flips the state to `GAME_OVER`, `GameWinner`
reports player 1, and the simulation is **frozen** — the ball stops moving while `GAME_OVER`.

```sh
gcc tests/test_game_over.c \
    src/visuals.c src/theme.c src/viewport.c src/assets.c \
    src/ball.c src/paddle.c src/character.c src/effects.c src/utils.c src/abilities.c \
    src/trail.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_game_over
/tmp/test_game_over
```

Expected output: `RESULT: PASS` (exit code 0).

## `test_reset_ball`

Reproduces and guards the old scoring bug where a ball that left the screen kept awarding a
point every frame (because `resetBall` never repositioned it). Asserts that a single miss awards
**exactly one** point, returns the ball to the play area, and re-serves toward the conceding
side. Also guards R23: `resetBall` disarms any armed Defender `shieldTimer` so a powered
reflect can't carry across a point.

```sh
gcc tests/test_reset_ball.c \
    src/visuals.c src/theme.c src/viewport.c src/assets.c \
    src/ball.c src/paddle.c src/character.c src/effects.c src/utils.c src/abilities.c \
    src/trail.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_reset_ball
/tmp/test_reset_ball
```

Expected output: `RESULT: PASS (one point for the miss; ball reset to the play area)`
(exit code 0). It includes `src/game.c` directly to reach the file-static `resetBall()`.

## `test_cpu_policy`

Guards the CPU ability policy: a single decision tick fires **at most one** ability (exactly one
cooldown armed per tick), and `cpuDecisionTimer` stays bounded and non-negative across many
ticks.

```sh
gcc tests/test_cpu_policy.c \
    src/abilities.c src/effects.c src/ball.c src/paddle.c \
    src/character.c src/utils.c src/theme.c src/trail.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_cpu_policy
/tmp/test_cpu_policy
```

Expected output: `RESULT: PASS` (exit code 0).

## `test_speedster`

Speedster abilities through `AbilitiesActivate`: Dash Return snaps the paddle to the ball's Y
and arms a 0.3 s paddle `STAT_TRAIL` — parking the **pre-snap** position first, so the ring
starts with a footprint where the dash flashed from and `UpdatePaddle` records the destination
on the next frame; Speed Boost is `STAT_PADDLE_SPEED` ×2; Afterimage Hit is
`STAT_BALL_SPEED` ×1.35 plus a 5 s ball `STAT_TRAIL`, landing at the ball's read index `-1`
regardless of which paddle activated it.

The **Overdrive** passive (heat model) is guarded here too: a serve-speed ball (`|(300,200)| =
BALL_SPEED_BASE`) holds heat at 0 with `maxSpeed` at 500; a ball at twice that speed caps heat at
1 → 750; Rally + Flow clamp **together** at full heat (750, never 837.5); `Q` banks +0.35 heat
and trims `E`'s cooldown by 1 s while `E` mirrors it for `Q`'s, clamping at 0 and never going
negative; Heat ×0.5 + Speed Boost ×2 stacks to 1175; Flow heat expires on schedule; and a
non-Speedster paddle ignores heat entirely.

**Clutch** runs through the real `UpdateBall` collision path: a return off the outer 25 % of the
paddle face (`hitPos < 0.25 || hitPos > 0.75`) `OP_SET`s heat to 1.0 for 0.5 s and arms a 0.5 s
ball `STAT_TRAIL` — for **both** paddle indexes, each keyed to its own slot — while a centre
return does nothing, a non-Speedster paddle never clutches, and the SET slot replaces a live Flow
`OP_ADD` (latest activation authoritative, ruling C3).

```sh
gcc tests/test_speedster.c \
    src/abilities.c src/effects.c src/ball.c src/paddle.c src/character.c src/theme.c \
    src/trail.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_speedster
/tmp/test_speedster
```

Expected output: `RESULT: PASS` (exit code 0).

## `test_wizard`

Grand Wizard abilities: Time Slow halves ball speed for 2 s then recovers; Curve Spell is
`STAT_BALL_CURVE` = 220 for 3 s; Arcane Chaos is 400 for 5 s (same key — re-casting refreshes).
Guards the self-restoring bend: the curve accumulates in `Ball::lateralVelocity`, never touches
`baseVelocity`, shows up in the effective velocity, and decays back to 0 after the effect
expires.

```sh
gcc tests/test_wizard.c \
    src/abilities.c src/effects.c src/ball.c src/paddle.c src/character.c src/theme.c \
    src/trail.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_wizard
/tmp/test_wizard
```

Expected output: `RESULT: PASS` (exit code 0).

## `test_defender`

Defender abilities: Shield Block arms a 5 s reflect window (`shieldTimer`), consumes it on hit,
reflects at `DEFENDER_SHIELD_REFLECT_MULT` (×1.3) with **no** `speedFactor` creep, and the
window decays in `UpdatePaddle`; Wide Paddle is `STAT_PADDLE_SIZE` ×1.8 through `UpdatePaddle`
and expires back; Iron Fortress is the full-height `STAT_PADDLE_SIZE` SET (540) plus an
immovable marker that `UpdatePaddle` maps to `paddle->immovable`, releasing cleanly on expiry.

```sh
gcc tests/test_defender.c \
    src/abilities.c src/effects.c src/ball.c src/paddle.c src/character.c src/theme.c \
    src/trail.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_defender
/tmp/test_defender
```

Expected output: `RESULT: PASS` (exit code 0).

## `test_visuals`

Headless tests for the pure visual-state queries in `src/visuals.c` (R22) that drive ball tint,
paddle aura, dash trail, and the HUD "active" dot: empty registry ⇒ blank tint and no active
player; Time Slow tints the ball with the owner's accent and lights only the owner's dot; a
paddle `STAT_TRAIL` belongs to its paddle only and expires; the Defender shield window is armed
by `ShieldActive` and closes via the real decay path; Iron Fortress tints only the owning
paddle. No window is opened — these are the exact queries `DrawEffects` composes into glow
shapes.

```sh
gcc tests/test_visuals.c src/visuals.c src/abilities.c src/effects.c src/ball.c \
    src/paddle.c src/character.c src/theme.c src/trail.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_visuals
/tmp/test_visuals
```

Expected output: `RESULT: PASS` (exit code 0).

## `test_trail`

The pure position-history ring behind the "true afterimage" (`TrailHistory`). No window, no
entities: it records positions, reads them back newest-first, ages them out at
`TRAIL_MAX_AGE`, and evicts the oldest past `TRAIL_MAX_GHOSTS` — with no dynamic
allocation anywhere.

```sh
gcc tests/test_trail.c src/trail.c -Iinclude \
    $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_trail
/tmp/test_trail
```

Expected output: `RESULT: PASS` (exit code 0).

---

## Notes

- The compiled `tests/test_reset_ball` binary (the old in-tree artifact) is git-ignored; all
  current outputs go to `/tmp/<name>`.
- `test_ball_physics.c`, `test_game_over.c`, and `test_reset_ball.c` `#include "../src/game.c"`
  to reach file-static functions; everything else links public APIs.
- All recipes assume raylib 5.5 on `pkg-config`. On a machine without `pkg-config`, replace it
  with the platform flags from `README.md`.