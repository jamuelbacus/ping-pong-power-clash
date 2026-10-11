/* tests/test_speedster.c */
#include "abilities.h"
#include "effects.h"
#include "ball.h"
#include "paddle.h"
#include "character.h"
#include <stdio.h>
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

int main(void) {
    InitCharacters();
    Effects fx; EffectsInit(&fx);
    Ball b; InitBall(&b, 480, 400, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,SPEEDSTER);

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 0);   /* Dash Return */
    CHECK(pad.position.y == 400.0f - PADDLE_HEIGHT/2, "dash snaps paddle to ball Y");
    CHECK(EffectsHas(&fx, TARGET_PADDLE, 0, STAT_TRAIL), "dash arms a paddle trail marker");

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 1);   /* Speed Boost */
    CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_SPEED, 300.0f) == 600.0f, "speed boost x2");

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 2);   /* Afterimage Hit */
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 405.0f, "afterimage speed x1.35");
    CHECK(EffectsHas(&fx, TARGET_BALL, -1, STAT_TRAIL), "afterimage arms a ball trail marker");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
