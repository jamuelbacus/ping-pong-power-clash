/* tests/test_effective_stats.c */
#include "ball.h"
#include "paddle.h"
#include "effects.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

int main(void) {
    Effects fx; EffectsInit(&fx);
    Ball b; InitBall(&b, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle p1, p2;
    InitPaddle(&p1, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, true, 4000, 500, 0.9f, KEY_W, KEY_S, SPEEDSTER);
    InitPaddle(&p2, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000, 500, 0.9f, KEY_UP, KEY_DOWN, SPEEDSTER);

    /* baseline: effective == base when no effects */
    UpdateBall(&b, &fx, 0.0f, 960, 540, &p1, &p2);
    CHECK(b.velocity.x == BALL_BASE_VELOCITY_X, "no-effect velocity == base");

    /* paddle speed effect */
    Effect e = { .active = true, .target = TARGET_PADDLE, .paddleIndex = 0,
                 .stat = STAT_PADDLE_SPEED, .op = OP_MUL, .value = 2.0f,
                 .timeLeft = 4.0f, .duration = 4.0f, .color = BLUE, .source = 0 };
    EffectsPush(&fx, e);
    UpdatePaddle(&p1, &fx, 0, 1.0f / 60.0f, 540, &b);
    CHECK(p1.maxSpeed == 1000.0f, "paddle speed x2");

    /* expiry restores base */
    EffectsUpdate(&fx, 4.5f);
    UpdatePaddle(&p1, &fx, 0, 1.0f / 60.0f, 540, &b);
    CHECK(p1.maxSpeed == 500.0f, "paddle speed restored after expiry");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
