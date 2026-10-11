/* tests/test_speedster.c */
#include "abilities.h"
#include "effects.h"
#include "ball.h"
#include "paddle.h"
#include "character.h"
#include <stdio.h>
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

/* Duration of the first active effect matching (target, paddleIndex, stat),
 * or -1 when no such effect exists. */
static float EffectDurationOf(const Effects* fx, EffectTarget t, int paddleIndex, EffectStat s) {
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        const Effect* e = &fx->items[i];
        if (e->active && e->target == t && e->paddleIndex == paddleIndex && e->stat == s)
            return e->duration;
    }
    return -1.0f;
}

int main(void) {
    InitCharacters();
    Effects fx; EffectsInit(&fx);
    Ball b; InitBall(&b, 480, 400, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,SPEEDSTER);

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 0);   /* Dash Return */
    CHECK(pad.position.y == 400.0f - PADDLE_HEIGHT/2, "dash snaps paddle to ball Y");
    CHECK(EffectsHas(&fx, TARGET_PADDLE, 0, STAT_TRAIL), "dash arms a paddle trail marker");
    CHECK(EffectDurationOf(&fx, TARGET_PADDLE, 0, STAT_TRAIL) == 0.3f, "dash trail lasts 0.3s");

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 1);   /* Speed Boost */
    CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_SPEED, 300.0f) == 600.0f, "speed boost x2");

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 2);   /* Afterimage Hit */
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 405.0f, "afterimage speed x1.35");
    CHECK(EffectsHas(&fx, TARGET_BALL, -1, STAT_TRAIL), "afterimage arms a ball trail marker");
    CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_TRAIL) == 5.0f, "afterimage ball trail lasts 5s");

    /* Ball effects must land at the index the ball actually reads (-1),
     * independent of which paddle activated the ability. */
    Effects fx2; EffectsInit(&fx2);
    Ball b2; InitBall(&b2, 480, 400, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle pad2; InitPaddle(&pad2, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000,500,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);

    AbilitiesActivate(&fx2, &b2, &pad2, 1, SPEEDSTER, 2);   /* Afterimage Hit from paddle index 1 */
    CHECK(EffectsApply(&fx2, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 405.0f,
          "afterimage speed lands at the ball's read index for paddleIndex=1");
    CHECK(EffectsHas(&fx2, TARGET_BALL, -1, STAT_TRAIL),
          "afterimage ball trail lands at the ball's read index for paddleIndex=1");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
