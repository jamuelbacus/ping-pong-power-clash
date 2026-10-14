/* tests/test_effects.c */
#include "effects.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); fails = 1; } } while (0)

static Effect mk(EffectStat s, EffectOp op, float v, float t, int source) {
    Effect e = { .active = true, .target = TARGET_BALL, .paddleIndex = -1,
                 .stat = s, .op = op, .value = v, .timeLeft = t, .duration = t,
                 .color = (Color){255,255,255,255}, .source = source };
    return e;
}

int main(void) {
    Effects fx; EffectsInit(&fx);
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 300.0f, "empty registry is identity");

    EffectsPush(&fx, mk(STAT_BALL_SPEED, OP_MUL, 2.0f, 1.0f, 0));
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 600.0f, "MUL applies");

    EffectsPush(&fx, mk(STAT_BALL_SPEED, OP_ADD, 50.0f, 1.0f, 1));
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 650.0f, "MUL then ADD");

    EffectsPush(&fx, mk(STAT_BALL_SPEED, OP_SET, 400.0f, 1.0f, 2));
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 400.0f, "SET overrides");

    /* same (target,index,stat,source) refreshes, does not stack */
    EffectsPush(&fx, mk(STAT_BALL_SIZE, OP_MUL, 2.0f, 1.0f, 5));
    EffectsPush(&fx, mk(STAT_BALL_SIZE, OP_MUL, 2.0f, 3.0f, 5));
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SIZE, 10.0f) == 20.0f, "same-source refresh (not 40)");

    /* distinct sources DO stack */
    EffectsPush(&fx, mk(STAT_BALL_SIZE, OP_MUL, 2.0f, 3.0f, 6));
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SIZE, 10.0f) == 40.0f, "distinct sources stack");

    /* expiry restores base exactly; slow example */
    Effects fx2; EffectsInit(&fx2);
    EffectsPush(&fx2, mk(STAT_BALL_SPEED, OP_MUL, 0.5f, 2.0f, 0));
    CHECK(EffectsApply(&fx2, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 150.0f, "slow halves speed");
    EffectsUpdate(&fx2, 2.5f);
    CHECK(EffectsApply(&fx2, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 300.0f, "slow expires back to base");
    CHECK(!EffectsHas(&fx2, TARGET_BALL, -1, STAT_BALL_SPEED), "expired effect gone");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
