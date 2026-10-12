# Fun Mode: Timing, Powerups, and Map Expansion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add hit-quality/timing + streak effects, powerups, map hazards (toggle), bigger map (expand playfield), and raise win score to 7 while keeping gameplay fun and tests passing.

**Architecture:** Extend ball-paddle collision to classify hit quality (positional); track streaks per player; apply streak effects (trail/speed) via existing effects/trail; add powerup system (fixed pool, no dynamic allocation); add hazards (toggle); increase court size and WINNING_SCORE.

**Tech Stack:** C, raylib 5.5, single-file headless test programs, bash test runner.

**Spec:** private/superpowers/specs/2026-10-13-fun-mode-timing-powerups-design.md

## Global Constraints

- raylib primitives only; no new art, audio, or fonts
- No dynamic allocation
- No new `-Wall -Wextra` warnings (only pre-existing `WINDOW_TITLE` unused-variable warning is exempt)
- All tests headless — binaries written to `/tmp/<name>`
- Full suite: `bash /tmp/opencode/run_suite.sh` — every recipe must print `RESULT: PASS`
- Never `git push` without explicit approval

## Review Focus

1. Hit quality classification must be deterministic (positional bands). Center = perfect; edge = weak.
2. Streak resets on point scored and on goal; thresholds trigger effects without breaking physics.
3. Map size change affects all geometry; HUD/positions must remain coherent.
4. Powerups don’t allocate dynamically and expire properly.
5. Hazards toggle works; when disabled, hazards never spawn/tick.

---
### Task 1: Expand map and raise win score

**Files:**
- Modify: `include/game.h` (`WINDOW_WIDTH`, `WINDOW_HEIGHT`, `WINNING_SCORE`)
- Modify: `include/layout.h` (HUD geometry if needed; verify against new canvas)
- Modify: `src/game.c`, `src/ball.c`, `src/paddle.c`, `src/decoy.c`, `src/main.c` as needed for positions
- Test: `tests/test_game_over.c` (win to 7), `tests/test_reset_ball.c`, `tests/test_ball_physics.c` (positions/coordinates)

**Interfaces:**
- Consumes: existing constants
- Produces: larger canvas and WINNING_SCORE=7

- [ ] **Step 1: Write failing tests/update expectations**
  - Update `tests/test_game_over.c` to expect win at 7 (e.g., reach 7). Keep other logic.
  - Review tests using hardcoded 960/540 (positions). Note what must change; don’t change unrelated assertions blindly.

- [ ] **Step 2: Change map size and win score**
  - Set `WINDOW_WIDTH=1024`, `WINDOW_HEIGHT=576` in `include/game.h` (bigger map).
  - Set `WINNING_SCORE=7` in `include/game.h`.
  - Adjust paddle initial positions if necessary: `PLAYER1_PADDLE_POSITION`/`PLAYER2_PADDLE_POSITION` in `include/paddle.h` remain reasonable; ball init in game.c uses canvas.
  - Update any hardcoded positions in tests that assume old canvas (e.g., test_ball_physics positions).

- [ ] **Step 3: Verify tests pass**
  - Run: `bash /tmp/opencode/run_suite.sh`
  Expected: all recipes print `RESULT: PASS`

- [ ] **Step 4: Commit**
```bash
git add include/game.h include/paddle.h include/layout.h src tests/test_game_over.c tests/test_ball_physics.c tests/test_reset_ball.c
git commit -m "feat(map): increase canvas to 1024x576; raise win to 7"
```

### Task 2: Hit quality classification + streak tracking

**Files:**
- Modify: `include/ball.h` (hit quality types/counters)
- Modify: `src/ball.c` (classify hit on paddle collision, store last quality)
- Modify: `include/game.h` / `src/game.c` (per-player streak, reset on score/goal)
- Test: add/update tests as needed (or extend test_speedster/ball tests)

**Interfaces:**
- Produces: `HitQuality` enum (PERFECT/GOOD/WEAK), `Ball.lastHitQuality`, `Game.playerStreak[2]`, `Game.streakEffectsActive[2]`

- [ ] **Step 1: Define types and counters**
  - Add HitQuality enum in ball.h or effects.h; track last hit quality on Ball.
  - Add streak arrays in Game.

- [ ] **Step 2: Classify hits**
  - On paddle hit, compute `hitPos` (0-1); classify: |hitPos-0.5| < 0.12 → PERFECT; < 0.25 → GOOD; else WEAK (deterministic).
  - Record quality; update streak for hitting player (increment if quality GOOD/PERFECT? define: weak doesn’t break streak? or any return increments; weak gives smaller effect). Define: any successful return counts as hit; weak gives no threshold progress toward “trail+faster” bonus beyond maybe not increasing bonus streak? Or require GOOD/PERFECT for streak bonus. Start simple: PERFECT/GOOD increment streak; WEAK resets or doesn’t increment? Better: streak increments on GOOD/PERFECT hits; WEAK doesn’t increment and maybe doesn’t decay immediately except on score.

- [ ] **Step 3: Reset streaks on score**
  - On goal (ball exits), reset the other player’s? Or reset both? Reset streak for players appropriately (scorer gets fresh rally context; convention: reset both on point).

- [ ] **Step 4: Tests + verify**
  - Run full suite: `bash /tmp/opencode/run_suite.sh` (all PASS)

- [ ] **Step 5: Commit**
```bash
git add include/ball.h src/ball.c include/game.h src/game.c
git commit -m "feat(hit): add hit quality classification and streak tracking"
```

### Task 3: Streak effects (trail + faster) on thresholds

**Files:**
- Modify: `src/ball.c` or `src/game.c` to apply effects on streak thresholds
- Modify: `include/overdrive.h` / `effects` usage as needed
- Test: headless tests still pass

**Interfaces:**
- Consumes: streaks from Task 2
- Produces: visual trail + speed boost when streak reaches thresholds (e.g., 3 PERFECT/GOOD in a row → trail + small speed boost; 5 → stronger)

- [ ] **Step 1: Define thresholds**
  - 3 hits of GOOD/PERFECT quality in a row → enable streak trail (push `STAT_TRAIL` effect on ball for short duration or continuous while active)
  - 5 → faster (boost `ball->speedFactor` modestly, e.g., +10-15% in a controlled way)

- [ ] **Step 2: Apply effects**
  - When streak crosses threshold, push appropriate effects. Don’t fight Overdrive clutch trail.
  - Ensure effects don’t break exact equality in tests (avoid changing base constants except speedFactor which is already multiplicative; tests that assert specific values may need updates if affected — but aim to be conservative)

- [ ] **Step 3: Verify**
  - `bash /tmp/opencode/run_suite.sh` all PASS

- [ ] **Step 4: Commit**
```bash
git add src/ball.c src/game.c
git commit -m "feat(streak): add trail+speed effects on hit streaks"
```

### Task 4: Powerups system (fixed pool, no dynamic alloc)

**Files:**
- Create: `include/powerup.h`, `src/powerup.c` (or put in existing modules)
- Modify: `include/game.h`, `src/game.c` (spawn/tick/collect)
- Modify: `src/paddle.c`, `src/ball.c` as needed

**Interfaces:**
- Fixed pool (e.g., 4 powerups max). States: inactive/active/spawned/collected/expired.
- Types: SPEED_BOOST, ENLARGE_PADDLE, SHRINK_OPPONENT, HEAT_PULSE (reusing effects).

- [ ] **Step 1: Define structs and pool**
  - No `malloc`; arrays of size 4. Fields: type, pos, timer, active.

- [ ] **Step 2: Spawning (deterministic or time-based)**
  - Spawn periodically or on rally start with simple rules. Avoid affecting headless tests.

- [ ] **Step 3: Collection and effects**
  - On collision with paddle or ball, apply effect (timed) and deactivate powerup. Reuse effects registry where possible.

- [ ] **Step 4: Tick and draw**
  - Expire after duration; don’t leak state.

- [ ] **Step 5: Verify suite**
  - `bash /tmp/opencode/run_suite.sh` all PASS

- [ ] **Step 6: Commit**
```bash
git add include/powerup.h src/powerup.c include/game.h src/game.c src/paddle.c
git commit -m "feat(powerups): add fixed-pool powerup system"
```

### Task 5: Map hazards (toggle)

**Files:**
- Modify: `include/game.h`, `src/game.h`? (game.h/c) add hazard state and toggle
- Modify: `src/ball.c`, `src/game.c`
- Modify: menu/character select if needed to toggle (check game flow)

**Interfaces:**
- `Game.hazardsEnabled` bool; toggle accessible from menu or config default
- Simple hazards: moving obstacle or speed zone that affects ball on contact

- [ ] **Step 1: Add toggle**
  - Add field and default (off) so existing behavior unchanged.

- [ ] **Step 2: Implement hazard**
  - Keep minimal: e.g., a vertical bar that moves; on collision with ball, reflect or apply small effect. Avoid allocation.

- [ ] **Step 3: Only active if enabled**
  - Gate all hazard logic behind toggle.

- [ ] **Step 4: Verify suite**
  - `bash /tmp/opencode/run_suite.sh` all PASS

- [ ] **Step 5: Commit**
```bash
git add include/game.h src/game.c src/ball.c
git commit -m "feat(hazards): add map hazards with toggle (default off)"
```

### Task 6: Final integration, docs, verification

**Files:**
- Update: `docs/GAME_DOCUMENTATION.md`, `docs/TECHNICAL_ANALYSIS.md`, `tests/README.md` if needed
- Verify: all tests updated for map size/win to 7

**Interfaces:**
- Ensure everything compiles cleanly and tests pass.

- [ ] **Step 1: Rebuild and run full suite**
  - `gcc src/*.c -Iinclude -Wall -Wextra $(pkg-config --cflags --libs raylib) -lm -o game 2>&1 | tail -5`
  - `bash /tmp/opencode/run_suite.sh` → all RESULT: PASS

- [ ] **Step 2: Update docs**
  - Reflect bigger map (1024x576), win to 7, hit quality/streaks, powerups, hazards toggle.

- [ ] **Step 3: Verify no new warnings**
  - Only pre-existing WINDOW_TITLE warning allowed.

- [ ] **Step 4: Commit**
```bash
git add docs/GAME_DOCUMENTATION.md docs/TECHNICAL_ANALYSIS.md tests
git commit -m "docs: update for fun mode features and tuning"
```
