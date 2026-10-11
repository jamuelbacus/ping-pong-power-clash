/* tests/test_wizard.c
 * Grand Wizard abilities:
 *   0 Curve Spell   -> a 3s lateral ball curve seeded via STAT_BALL_CURVE (R17).
 *   1 Time Slow     -> TARGET_BALL STAT_BALL_SPEED MUL 0.5 for 2s.
 *   2 Arcane Chaos  -> a stronger 5s lateral ball curve.
 * Curve/Chaos are modelled as a real per-frame lateral acceleration on the
 * ball's authoritative baseVelocity, so Pong wall bouncing is never corrupted.
 */
#include "abilities.h"
#include "effects.h"
#include "ball.h"
#include "paddle.h"
#include "character.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

/* Value of the first active effect matching (target, paddleIndex, stat), or -1. */
static float EffectValueOf(const Effects* fx, EffectTarget t, int paddleIndex, EffectStat s) {
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        const Effect* e = &fx->items[i];
        if (e->active && e->target == t && e->paddleIndex == paddleIndex && e->stat == s)
            return e->value;
    }
    return -1.0f;
}

/* Duration of the first active effect matching (target, paddleIndex, stat), or -1. */
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
    Ball b; InitBall(&b, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,GRAND_WIZARD);

    AbilitiesActivate(&fx, &b, &pad, 0, GRAND_WIZARD, 1);   /* Time Slow */
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 150.0f, "time slow halves speed");
    CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_BALL_SPEED) == 2.0f, "time slow lasts 2s");
    EffectsUpdate(&fx, 2.5f);
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 300.0f, "time slow recovers");

    AbilitiesActivate(&fx, &b, &pad, 0, GRAND_WIZARD, 0);   /* Curve Spell */
    CHECK(EffectsHas(&fx, TARGET_BALL, -1, STAT_BALL_CURVE), "curve applies a curve modifier");
    CHECK(EffectValueOf(&fx, TARGET_BALL, -1, STAT_BALL_CURVE) == 220.0f, "curve spell strength 220");
    CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_BALL_CURVE) == 3.0f, "curve spell lasts 3s");

    AbilitiesActivate(&fx, &b, &pad, 0, GRAND_WIZARD, 2);   /* Arcane Chaos */
    CHECK(EffectsHas(&fx, TARGET_BALL, -1, STAT_BALL_CURVE), "chaos applies a curve modifier");
    CHECK(EffectValueOf(&fx, TARGET_BALL, -1, STAT_BALL_CURVE) == 400.0f, "arcane chaos is stronger (400)");
    CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_BALL_CURVE) == 5.0f, "arcane chaos lasts 5s");

    /* R17: the curve must bend the authoritative baseVelocity.x over time,
     * applied before the effective velocity is derived, without touching y. */
    Effects cfx; EffectsInit(&cfx);
    Ball cb; InitBall(&cb, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle p1; InitPaddle(&p1, 50, 250, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,GRAND_WIZARD);
    Paddle p2; InitPaddle(&p2, 890, 250, PADDLE_WIDTH, PADDLE_HEIGHT, 300, RED, true, 4000,500,0.9f,KEY_UP,KEY_DOWN,GRAND_WIZARD);
    AbilitiesActivate(&cfx, &cb, &p1, 0, GRAND_WIZARD, 0);   /* Curve Spell */
    float startX = cb.baseVelocity.x;
    float startY = cb.baseVelocity.y;
    UpdateBall(&cb, &cfx, 0.1f, 960, 540, &p1, &p2);
    CHECK(cb.baseVelocity.x > startX, "curve accelerates baseVelocity.x forward");
    CHECK(cb.baseVelocity.y == startY, "curve leaves baseVelocity.y untouched");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
