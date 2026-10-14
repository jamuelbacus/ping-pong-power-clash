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
    Paddle cpu; InitPaddle(&cpu, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,GRAND_WIZARD);

    /* A single 1/60 tick must not fire everything at once: each activation
     * commits exactly one cooldown, so at most one cooldown may be armed.
     * (A single ability may push more than one effect, so counting effect
     * entries would be wrong.) */
    CpuTickAbilities(&fx, &b, &cpu, 1, 1.0f/60.0f, NULL);
    int activated = (cpu.skill1Cooldown > 0.0f)
                  + (cpu.skill2Cooldown > 0.0f)
                  + (cpu.ultimateCooldown > 0.0f);
    CHECK(activated <= 1, "one decision tick fires at most one ability");

    /* Advancing a full decision interval keeps the timer bounded and non-negative. */
    for (int i = 0; i < 120; i++) CpuTickAbilities(&fx, &b, &cpu, 1, 1.0f/60.0f, NULL);
    CHECK(cpu.cpuDecisionTimer >= 0.0f, "decision timer never negative");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
