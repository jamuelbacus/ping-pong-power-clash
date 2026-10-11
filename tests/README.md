# Tests

Small headless regression tests. They live outside `src/`, so the normal
`gcc src/*.c ...` build and the shipping game are unaffected.

## `test_reset_ball`

Reproduces and guards the scoring bug where a ball that left the screen kept
awarding a point every frame (because `resetBall()` never repositioned it).
Asserts that a single miss awards **exactly one** point and returns the ball
to the play area.

Build and run from the **project root**:

```sh
gcc tests/test_reset_ball.c \
    src/theme.c src/viewport.c src/assets.c \
    src/ball.c src/paddle.c src/character.c src/effects.c src/utils.c src/abilities.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o tests/test_reset_ball
./tests/test_reset_ball
```

Expected output: `RESULT: PASS ...` (exit code 0). It includes `src/game.c`
directly to reach the file-static `updateGamePlay()`/`resetBall()`.

The compiled `tests/test_reset_ball` binary is git-ignored.

## `test_cpu_policy`

Guards the CPU ability policy: a single decision tick fires at most one
ability, and `cpuDecisionTimer` stays non-negative across ticks.

```sh
gcc tests/test_cpu_policy.c \
    src/abilities.c src/effects.c src/ball.c src/paddle.c \
    src/character.c src/utils.c src/theme.c \
    -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_cpu_policy
/tmp/test_cpu_policy
```

Expected output: `RESULT: PASS` (exit code 0).
