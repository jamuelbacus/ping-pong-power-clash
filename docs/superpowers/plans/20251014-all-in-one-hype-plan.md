# Plan: All-in-One Hype Pass (Audio + Visuals + Feel)

## Goal
Deliver the "hype + smug" feeling when landing big plays. Elevate audio (menu + game BGM, unique per-skill sounds), unify UI/art direction, and improve readability/juice so it's easier to catch the ball and rallies feel rewarding. Keep C + Raylib, no dynamic allocation, pass all 12 headless tests with -Wall -Wextra (only pre-existing warnings exempt).

## Known Issues to Address
- No sound in main menu; game BGM exists but feels flat/noisy, lacks excitement
- Skills don't have unique sounds (only generic skill_cast)
- UI generic, art direction inconsistent (icons liked)
- Hard to catch ball, gets boring fast

## Approach
Tackle in parallel where independent, but ensure cohesion (audio + visuals reinforce same energy). Focus on readability first (helps "catch ball"), then juice/sfx, then UI polish.

## Task 1: Audio System Expansion (Menu + Game + Unique Skills)
**Files**: `include/sfx.h`, `src/sfx.c`, `src/main.c`, `src/abilities.c`, `assets/audio/`

- [ ] Add menu BGM (Music) separate from game BGM; load in Sfx, play based on game state (menu vs in-game). Add `PlayMenuBgm`, `StopMenuBgm`, `PlayGameBgm` helpers.
- [ ] Improve game BGM - make more energetic/looping cleanly (higher energy, less "noise", build tension subtly). Keep volume reasonable.
- [ ] Create unique skill sounds per character/ability (or at least per ability type). Generate short punchy casts for each of 3 abilities per character? Or categorize by ability index and character accent. Alternatively, have per-ability SFX slots (up to reasonable count) or vary pitch by character.
- [ ] Add menu select/confirm sounds (optional but cheap). Also ensure audio initializes before any play.
- [ ] Update CREDITS.md for new audio.

## Task 2: Gameplay Feel - Readability + Juice
**Files**: `src/ball.c`, `src/game.c`, `src/visuals.c`, `src/effects.c`, `include/layout.h`

- [ ] Improve ball readability: add subtle trail clarity, stronger outline/glow, or higher contrast. Consider slight speed ramp feel feedback, but preserve physics determinism for tests.
- [ ] Make hits feel weightier: enhance paddle hit flash, screen shake on perfect/streak hits, stronger impact particles. Keep cartoony and non-distracting.
- [ ] "Hard to catch" - tweak ball speed curve slightly, increase paddle hit forgiveness/readability, add predictive ghost/trajectory hint toggle? Or improve VFX on incoming. Prefer non-cheaty visual clarity.
- [ ] Streaks already trigger SFX; amp up VFX for streak5 to feel smug/hype.

## Task 3: UI/Art Direction Cohesion
**Files**: `src/theme.c`, `src/main.c`, `src/visuals.c`, `include/theme.h`, assets

- [ ] Unify color palette, font usage, spacing, shapes. Define primary/secondary/accent per character consistently.
- [ ] Menu polish: title, character select, UI panels with consistent borders/shadows, animated accents.
- [ ] HUD: score, timers, cooldowns with clearer readability and juice on score/streak.
- [ ] Lean into "power clash" energy (bold shapes, glows, motion) while keeping simple web-game vibe.

## Task 4: Wire It All Up
- [ ] Hook menu/game state to BGM switching in main loop.
- [ ] Hook unique skill sounds in abilities (pass character/ability context).
- [ ] Ensure all SFX play without blocking; respect raylib audio.

## Verification
- [ ] Build: `gcc ... -Wall -Wextra` clean (game binary)
- [ ] Tests: all 12 headless tests pass
- [ ] Manual smoke test (menu + gameplay) for audio feel
- [ ] No git push without explicit approval

## Priority
Do in order: 1 (Audio foundation + menu bgm), 2 (Gameplay feel/readability), 3 (UI cohesion), 4 (Polish). Many subtasks independent.
