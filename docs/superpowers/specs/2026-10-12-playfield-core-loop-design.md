# WS1 — Playfield & Core Loop: Design Spec

**Date:** 2026-10-12
**Status:** Draft for review
**Issues:** #1 (ball occluded by HUD), #2 (flat rallies / no room for powers)
**Part of:** the six-issue "fix all" effort (see Workstreams below)

---

## 1. Context

The game works but is not fun to play. Two of the six filed issues are foundational
because everything else renders on top of the play field:

- **#1** The ball can pass behind the bottom-corner skill panels and become invisible.
  The panels (`HUD_P1`, `HUD_P2`) sit *inside* the court and are drawn *after* the ball,
  so they win the z-order.
- **#2** Rallies are flat and powers have no visible impact. Measured cause: the paddle is
  far too fast relative to the ball. At normal ball speed the paddle can sweep ~2.6× the
  field height while the ball crosses; even at the ball's maximum speed it still covers
  ~97% of the field. No shot is ever out of reach, so a powered return is just a slightly
  quicker return.

Both are consequences of the same thing — the court and the HUD fight for the same space,
and the entities are sized/specced so nothing is ever contested. This workstream fixes
both.

## 2. Goal

A match where:

1. the ball is never hidden by any HUD element, and
2. the base exchange is read-and-react — points are won by anticipating the ball and
   being in position, and a powered return opens a gap a prepared defender can still
   contest.

## 3. Non-goals

This workstream does **not** add audio (#3), in-match character portraits (#4), game
modes (#5), or power-ups (#6). Those are separate workstreams, designed and specced
individually after this one lands. No new subsystems, no new art, no new fonts.

## 4. Workstreams (for orientation)

| Workstream | Issues | Status |
|---|---|---|
| **WS1 — Playfield & core loop** | #1, #2 | this spec |
| WS2 — Audio | #3 | not started |
| WS3 — In-match character identity | #4 | not started |
| WS4 — Content layer (power-ups, modes) | #6, #5 | not started |

## 5. Constraints

Inherited from the project and still in force:

- raylib primitives only for rendering; no hand-made art assets.
- No dynamic allocation (`malloc`/`free`, growable containers).
- No new `-Wall -Wextra` warnings (the pre-existing `WINDOW_TITLE` unused-variable
  warning is exempt).
- All tests are headless and must not open a window.
- Never `git push` without explicit approval.

## 6. Design

### 6.1 HUD & occlusion (fixes #1)

**Chosen layout:** "scoreline only" — a single translucent plate centred at the top of
the court, replacing the two corner panels.

- Contents, per side: the score (large), the character name (small), and **three slim
  cooldown bars** — one per skill — ordered left-to-right by key order (P1: `Q E R`;
  P2: `1 2 3` — note P2 uses the numpad keys `KP_1/KP_2/KP_3`), each with a tiny key
  letter beneath.
- Bar semantics: the bar **fills as the cooldown recovers** (empty = just used, full =
  ready). A **full and bright** bar means ready; a **glowing** bar means that skill's
  effect is currently active. This replaces the standalone "active-effect" dot, which is
  removed.
- The plate is translucent (≈55% opacity) so play reads through it.

**Draw order (the actual occlusion fix):** `DrawGameplay` currently draws
entities first and HUD last. It is inverted:

1. grid background
2. centre line
3. **HUD scoreline (translucent)**
4. ball
5. Shadow Clone decoys
6. paddles
7. effect juice (trails, auras, shield/afterburner outlines)

Because the HUD is drawn first and translucently, the ball and its trail always render
on top; nothing in the play field can hide the ball.

**Layout constants (`include/layout.h`):**

- Remove `HUD_P1` and `HUD_P2`.
- Add scoreline geometry: plate rect, per-side score/name/bars anchor points, bar width,
  bar height, gap. Exact values are implementation detail but live here so the draw code
  and any hit-tests cannot drift.

**Pure, testable helper (`src/visuals.c`, `include/visuals.h`):**

```c
/* Fraction of a skill's cooldown that has recovered: 0 while fully cooling, 1 when
 * ready. Used for both the bar fill and the ready glow so the two can never disagree. */
float SkillRecoverFraction(float cooldown, float maxCooldown);
```

This mirrors the existing pattern of pure visual-state queries (`ShieldActive`,
`AfterburnerArmed`, `PlayerHasActiveEffect`) and keeps the HUD state headless-testable.

### 6.2 Core-loop tuning (fixes #2)

Adopt the "Moderate" preset agreed during design. All values are starting points and are
expected to be nudged after playtesting.

| Constant | File | From | To |
|---|---|---|---|
| `PADDLE_HEIGHT` | `include/paddle.h` | `100` | `76` |
| paddle `maxSpeed` (both paddles, in `InitGame`) | `src/game.c` | `500` | `320` |
| `BALL_BASE_VELOCITY_X` | `include/ball.h` | `300` | `400` |
| `BALL_BASE_VELOCITY_Y` | `include/ball.h` | `200` | `267` |
| `BALL_SPEED_BASE` | `include/overdrive.h` | `360.5551f` | `≈480.93f` |
| `BALL_MAX_VELOCITY` | `include/ball.h` | `800` | unchanged |

Notes:

- `BALL_SPEED_BASE` is the Overdrive "rally heat" reference and **must stay bit-identical
  to the float32 rounding of `sqrtf(400² + 267²)`** so that a serve-speed ball yields heat
  exactly 0. Derive the exact literal during implementation; do not hand-round.
- Paddle movement keeps its current instant response (`velocityY = ±maxSpeed` in one
  frame); no acceleration ramp is introduced in this workstream. The unused `accel` /
  `friction` fields on `Paddle` are left as they are.
- Speedster **Overdrive** is unchanged in formula: `maxSpeed = originalMaxSpeed ×
  (1 + 0.5 × heat)`. With the new base this becomes `320 → 480` at full heat, so the
  Speedster remains the fastest paddle (+50% over base) — its identity is preserved.
- Resulting reaction budget: paddle covers **~1.24× the field height** while the ball
  crosses at normal speed (comfortable if read), and **~0.62×** at powered/maximum speed
  (a real gap — the power matters).

### 6.3 Consequence: CPU difficulty

The CPU paddle also moves at `maxSpeed`, so lowering it makes the CPU weaker. This is
accepted for WS1 and recorded as a follow-up (a CPU difficulty scalar), not fixed here.

## 7. Data flow

No new state. The scoreline reads existing fields (`playerScore`, `player2Score`, both
paddles' `skill{1,2,3}Cooldown` / `ultimateCooldown`, and `charType`), and the cooldown
bars are computed through `SkillRecoverFraction`. Active-glow state reuses the existing
queries (`PlayerHasActiveEffect`, `ShieldActive`, `AfterburnerArmed`).

## 8. Testing

Headless regression tests only; no test may open a window.

- **Update** existing tests that hardcode the tuned values:
  - `test_effective_stats` (paddle speed `500`/`1000` guards).
  - `test_speedster` (serve-speed literal `360.5551f`, and the `500 → 750` Overdrive
    guards, which become the new base and its full-heat value).
  - `test_ball_physics`, `test_game_over`, `test_reset_ball`, `test_cpu_policy`,
    `test_visuals`, `test_wizard`, `test_defender` — their `InitPaddle(...)` call sites
    pass `maxSpeed` explicitly; update them to the new base so the suite exercises the
    real configuration.
- **Add** tests for `SkillRecoverFraction`: `0` at full cooldown just spent, `1` at/after
  zero, monotonic between, and safe for `maxCooldown <= 0`.
- **Add** a test for the scoreline state if any non-trivial mapping is introduced beyond
  `SkillRecoverFraction`.
- The full 12-recipe suite (`/tmp/opencode/run_suite.sh`) must end green
  (`RESULT: PASS` each).

## 9. Documentation

Update to match the new reality:

- `docs/GAME_DOCUMENTATION.md` — HUD description, control/score display, tuning table.
- `docs/TECHNICAL_ANALYSIS.md` — play-field/HUD layout, draw order, tuned constants.
- `tests/README.md` — any new recipe/test rows and the updated link sets.

## 10. Acceptance criteria

- [ ] No HUD element can occlude the ball at any point in a rally (HUD drawn first,
      translucent).
- [ ] The two corner skill panels are gone; a centred translucent scoreline shows both
      scores, both names, and per-skill cooldown bars with key letters.
- [ ] A cooldown bar fills as it recovers; a glowing bar indicates a live effect; the old
      standalone active dot is removed.
- [ ] Paddle height, paddle speed, ball base speed, and `BALL_SPEED_BASE` are tuned as in
      §6.2; Overdrive still yields the fastest paddle.
- [ ] The full 12-recipe headless suite passes; no new warnings; no dynamic allocation.
- [ ] `GAME_DOCUMENTATION.md`, `TECHNICAL_ANALYSIS.md`, and `tests/README.md` are updated.

## 11. Open items / follow-ups

- Exact tuning constants (`76 / 320 / 480 / cap 800`) are subject to playtest adjustment.
- Optional follow-ups recorded but not in scope: paddle acceleration ramp; raising
  `BALL_MAX_VELOCITY` to preserve power headroom above a faster serve; CPU difficulty
  scalar.
