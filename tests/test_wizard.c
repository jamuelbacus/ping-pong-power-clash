/* tests/test_wizard.c
 * Grand Wizard abilities:
 *   0 Curve Spell   -> a 3s lateral ball curve seeded via STAT_BALL_CURVE.
 *   1 Time Slow     -> TARGET_BALL STAT_BALL_SPEED MUL 0.5 for 2s.
 *   2 Arcane Chaos  -> a stronger 5s lateral ball curve.
 * Curve/Chaos accumulate in Ball::lateralVelocity, which is added to the
 * effective velocity only — baseVelocity stays authoritative for wall
 * bounces — and decay back to 0 when the effect expires (R18).
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
    Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,GRAND_WIZARD);

    AbilitiesActivate(&fx, &b, &pad, 0, GRAND_WIZARD, 1, NULL);   /* Time Slow */
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 150.0f, "time slow halves speed");
    CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_BALL_SPEED) == 2.0f, "time slow lasts 2s");
    EffectsUpdate(&fx, 2.5f);
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 300.0f, "time slow recovers");

    AbilitiesActivate(&fx, &b, &pad, 0, GRAND_WIZARD, 0, NULL);   /* Curve Spell */
    CHECK(EffectsHas(&fx, TARGET_BALL, -1, STAT_BALL_CURVE), "curve applies a curve modifier");
    CHECK(EffectValueOf(&fx, TARGET_BALL, -1, STAT_BALL_CURVE) == 220.0f, "curve spell strength 220");
    CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_BALL_CURVE) == 3.0f, "curve spell lasts 3s");

    AbilitiesActivate(&fx, &b, &pad, 0, GRAND_WIZARD, 2, NULL);   /* Arcane Chaos */
    CHECK(EffectsHas(&fx, TARGET_BALL, -1, STAT_BALL_CURVE), "chaos applies a curve modifier");
    CHECK(EffectValueOf(&fx, TARGET_BALL, -1, STAT_BALL_CURVE) == 400.0f, "arcane chaos is stronger (400)");
    CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_BALL_CURVE) == 5.0f, "arcane chaos lasts 5s");

    /* R18: the curve accumulates in lateralVelocity and is added to the
     * effective velocity only. baseVelocity must stay pristine (a timed
     * effect must never leave permanent state), and the bend must decay back
     * to 0 once the effect expires. */
    Effects cfx; EffectsInit(&cfx);
    Ball cb; InitBall(&cb, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle p1; InitPaddle(&p1, 50, 250, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,GRAND_WIZARD);
    Paddle p2; InitPaddle(&p2, 890, 250, PADDLE_WIDTH, PADDLE_HEIGHT, 300, RED, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,GRAND_WIZARD);
    AbilitiesActivate(&cfx, &cb, &p1, 0, GRAND_WIZARD, 0, NULL);   /* Curve Spell */
    float startX = cb.baseVelocity.x;
    float startY = cb.baseVelocity.y;
    UpdateBall(&cb, &cfx, 0.1f, 960, 540, &p1, &p2);
    CHECK(cb.baseVelocity.x == startX, "curve must not corrupt baseVelocity.x");
    CHECK(cb.baseVelocity.y == startY, "base y untouched");
    CHECK(cb.lateralVelocity > 0.0f, "lateral velocity accumulated");
    CHECK(cb.velocity.x > startX, "bend visible in effective velocity");

    /* The curve is a timed effect: once it expires the lateral bend must
     * decay away, leaving no permanent state behind. */
    EffectsUpdate(&cfx, 3.5f);
    for (int i = 0; i < 5; i++) UpdateBall(&cb, &cfx, 0.1f, 960, 540, &p1, &p2);
    CHECK(cb.lateralVelocity == 0.0f, "lateral velocity self-restores to 0");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
