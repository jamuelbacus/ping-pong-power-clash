# Fun Mode: Timing, Powerups, and Map Expansion (Big Spec)

## 1. Problem statement recap
The user wants: introduce a skill and timing system where you can score a perfect hit, good hit, weak hit, miss; if you get good hits like 3 perfect hits etc and so on, it will leave a trail and such and faster; player can also choose if map hazards are introduced; also powerups; also increase points to win to 7 (for longer time); also bigger map.

## 2. Goals (non-binding)
Make it fun without breaking the "read-and-react" feel. Preserve raylib primitives, no dynamic allocation, no new -Wall -Wextra warnings (pre-existing WINDOW_TITLE exempt), headless tests must remain green unless we intentionally update them with reason.

## 3. Constraints
- Raylib primitives only; no new art/audio/fonts
- No dynamic allocation
- No new `-Wall -Wextra` warnings (only pre-existing `WINDOW_TITLE` exempt)
- All tests headless; binaries to `/tmp`; `bash /tmp/opencode/run_suite.sh` must print `RESULT: PASS`
- Never `git push` without explicit approval
- Fit with existing architecture (effects registry, trail/history, decoys, Overdrive)
- Don’t break existing tests unless behavior changes are explicit

## 4. Proposed architecture (phased in design)
Large scope; split conceptually into subsystems:

### 4.1 Hit Quality (timing) system
Hit quality based on `hitPos` (how centered: 0-1) and timing? Current angle depends on hitPos (center = 0 angle). “Perfect/good/weak/miss”:
- Define bands: center band → perfect; middle → good; edges near edges? Also timing-based (e.g., was paddle moving toward/meeting ball optimally)? Or mainly positional: 
  - Perfect: |hitPos - 0.5| < ε_p (e.g. 0.15) → gives small boost (speed slightly, or trail)
  - Good: next band → moderate
  - Weak: far edge → reduced effect or just no bonus
  - Miss: already handled by collision

Extend `UpdateBall` on paddle hit to classify quality and record it on the returning player (or in `Ball`/`Game` state). Store a hit counter (consecutive perfect/good hits) for the rally side? Or just count recent hits.

### 4.2 Streak → trail/faster
“if you get good hits like 3 perfect hits etc and so on, it will leave a trail and such and faster”
- Maintain `streak` per player (consecutive successful returns/hits). Reset on point scored.
- On reaching thresholds (e.g., 3 perfect → start trail on ball, small speed boost; 5 → stronger). Use effects registry (e.g., push `STAT_TRAIL` with duration, or just boost `ball->speedFactor` slightly in a controlled way to avoid breaking tests).
- Trail from streak is additive to existing trail (Overdrive clutch already pushes trail). Need to ensure no conflict.

### 4.3 Bigger map
Increase `WINDOW_WIDTH/HEIGHT` or playfield? Current `WINDOW_WIDTH=960, WINDOW_HEIGHT=540`. “Bigger map” could mean larger virtual court; need to adjust wall positions, paddle positions relative to edges, HUD placement (we already have scoreline at top), and ensure viewport/scaling still works. Tests may assume 960x540 — changing constants affects tests that use hardcoded positions.

### 4.4 Win to 7
Change `WINNING_SCORE` in `game.h` from 5 to 7. Tests like `test_game_over.c` likely check win at 5; need to update.

### 4.5 Map hazards (optional, toggle)
Player can choose if map hazards introduced. Add menu option/toggle (e.g., in character select or pre-game). Hazards: moving obstacles, shrinking gaps, or bounce modifiers. Keep headless-test friendly (tests can run with hazards disabled).

### 4.6 Powerups (pickup)
Timed powerups spawn on field (no dynamic allocation — pre-allocate pool of max powerups, e.g., 4). Types: speed boost, enlarge paddle, shrink opponent, etc. Collision with ball or paddle? “Powerups” generally collected; affects next hit or timed. Need to model pickup/expiration.

## 5. Integration points
- `ball.c` UpdateBall: hit classification on paddle contact
- `game.c`: track streaks, win condition, powerup spawns/ticks, hazards
- `paddle.c`: effects on paddle size/speed from powerups
- `effects.h/c`: new effect types if needed (or reuse)
- `layout.h/game.h`: map size, win score
- Menu/UI: toggle for hazards

## 6. Backwards compatibility
Changing map size/win score will break existing tests that assume 5/960x540. That’s expected but must be updated explicitly with reasoning. Also full suite must pass after updates.

## 7. Acceptance criteria
- Hit quality classified and affects streak
- Streak thresholds produce visual trail and/or speed increase
- Map larger, win to 7, hazards toggle, powerups functional
- All tests updated if behavior changes; suite passes

## 8. Next step
This is a large architectural change. I should write the full spec (this file) and commit it, then invoke writing-plans to create detailed implementation plan with tasks. The spec is already written above as draft; let me commit it properly and then proceed to planning.