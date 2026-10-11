/* tests/test_cpu_policy.c */
#include "abilities.h"
#include "effects.h"
#include "ball.h"
#include "paddle.h"
#include <stdio.h>
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

int main(void) {
    Effects fx; EffectsInit(&fx);
    Ball b; InitBall(&b, 800, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle cpu; InitPaddle(&cpu, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000,500,0.9f,KEY_UP,KEY_DOWN,GRAND_WIZARD);

    /* A single 1/60 tick must not fire everything at once: count the live
     * entries and assert at most one ability was activated. */
    CpuTickAbilities(&fx, &b, &cpu, 1, 1.0f/60.0f);
    int n = 0;
    const Effect* list = EffectsList(&fx, &n);
    int active = 0;
    for (int i = 0; i < n; i++) if (list[i].active) active++;
    CHECK(active <= 1, "one decision tick fires at most one ability");

    /* Advancing a full decision interval keeps the timer bounded and non-negative. */
    for (int i = 0; i < 120; i++) CpuTickAbilities(&fx, &b, &cpu, 1, 1.0f/60.0f);
    CHECK(cpu.cpuDecisionTimer >= 0.0f, "decision timer never negative");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
