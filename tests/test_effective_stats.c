/* tests/test_effective_stats.c */
#include "ball.h"
#include "paddle.h"
#include "effects.h"
#include <stdio.h>
#include <math.h>

static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

int main(void) {
    Effects fx; EffectsInit(&fx);
    Ball b; InitBall(&b, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle p1, p2;
    InitPaddle(&p1, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, true, 4000, 320, 0.9f, KEY_W, KEY_S, SPEEDSTER);
    InitPaddle(&p2, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000, 320, 0.9f, KEY_UP, KEY_DOWN, SPEEDSTER);

    /* baseline: effective == base when no effects */
    UpdateBall(&b, &fx, 0.0f, 960, 540, &p1, &p2);
    CHECK(b.velocity.x == BALL_BASE_VELOCITY_X, "no-effect velocity == base");

    /* regression: a paddle bounce must derive its new base magnitude from
     * baseVelocity, not from the effect-scaled effective velocity, so a
     * temporary STAT_BALL_SPEED effect is applied exactly once and never
     * baked into baseVelocity. */
    Effects fxBall; EffectsInit(&fxBall);
    EffectsPush(&fxBall, (Effect){ .active = true, .target = TARGET_BALL, .paddleIndex = -1,
                 .stat = STAT_BALL_SPEED, .op = OP_MUL, .value = 2.0f,
                 .timeLeft = 4.0f, .duration = 4.0f, .color = BLUE, .source = 0 });
    Paddle pLeft, pRight;
    InitPaddle(&pLeft, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, true, 4000, 320, 0.9f, KEY_W, KEY_S, SPEEDSTER);
    InitPaddle(&pRight, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000, 320, 0.9f, KEY_UP, KEY_DOWN, SPEEDSTER);
    Ball b2; InitBall(&b2, 60, 250, BALL_SIZE, -300.0f, 0.0f, BALL_COLOR);
    float baseSpeedBefore = 300.0f;              /* |(-300, 0)| */
    float expectedBaseMag = baseSpeedBefore * b2.speedFactor;

    UpdateBall(&b2, &fxBall, 0.0f, 960, 540, &pLeft, &pRight);

    CHECK(b2.baseVelocity.x > 0.0f, "paddle bounce redirects base velocity away from the paddle");
    float baseMag = sqrtf(b2.baseVelocity.x * b2.baseVelocity.x + b2.baseVelocity.y * b2.baseVelocity.y);
    CHECK(fabsf(baseMag - expectedBaseMag) < 0.01f,
          "paddle bounce base magnitude excludes the STAT_BALL_SPEED multiplier");
    CHECK(fabsf(baseMag - expectedBaseMag * 2.0f) > 0.01f,
          "paddle bounce did not bake the x2 effect into baseVelocity");
    float effMag = sqrtf(b2.velocity.x * b2.velocity.x + b2.velocity.y * b2.velocity.y);
    CHECK(fabsf(effMag - expectedBaseMag * 2.0f) < 0.01f,
          "effective velocity still applies the x2 effect exactly once");

    /* paddle speed effect */
    Effect e = { .active = true, .target = TARGET_PADDLE, .paddleIndex = 0,
                 .stat = STAT_PADDLE_SPEED, .op = OP_MUL, .value = 2.0f,
                 .timeLeft = 4.0f, .duration = 4.0f, .color = BLUE, .source = 0 };
    EffectsPush(&fx, e);
    UpdatePaddle(&p1, &fx, 0, 1.0f / 60.0f, 540, &b, NULL);
    CHECK(p1.maxSpeed == 640.0f, "paddle speed x2");

    /* expiry restores base */
    EffectsUpdate(&fx, 4.5f);
    UpdatePaddle(&p1, &fx, 0, 1.0f / 60.0f, 540, &b, NULL);
    CHECK(p1.maxSpeed == 320.0f, "paddle speed restored after expiry");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
