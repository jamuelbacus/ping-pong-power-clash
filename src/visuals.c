/*
 * visuals.c
 * Implements the pure visual-state queries declared in visuals.h. These
 * translate the Effects registry into the booleans/colors the renderer needs,
 * with no drawing (see header for the R22 rationale).
 */

#include "visuals.h"

static const Color NO_TINT = { 0, 0, 0, 0 };

Color EffectTintColor(const Effects* fx, EffectTarget target, int paddleIndex) {
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        const Effect* e = &fx->items[i];
        if (e->active && e->target == target && e->paddleIndex == paddleIndex) {
            return e->color;
        }
    }
    return NO_TINT;
}

bool EffectTrailActive(const Effects* fx, EffectTarget target, int paddleIndex) {
    return EffectsHas(fx, target, paddleIndex, STAT_TRAIL);
}

bool PlayerHasActiveEffect(const Effects* fx, int paddleIndex) {
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        const Effect* e = &fx->items[i];
        if (!e->active) continue;
        if ((e->target == TARGET_PADDLE && e->paddleIndex == paddleIndex)
            || (e->target == TARGET_BALL && e->source == paddleIndex)) {
            return true;
        }
    }
    return false;
}

bool ShieldActive(const Paddle* paddle) {
    return paddle->shieldTimer > 0.0f;
}

bool AfterburnerArmed(const Paddle* paddle) {
    return paddle->afterburnerTimer > 0.0f;
}

float SkillRecoverFraction(float cooldown, float maxCooldown) {
    if (maxCooldown <= 0.0f) return 1.0f;
    if (cooldown <= 0.0f) return 1.0f;
    float f = 1.0f - cooldown / maxCooldown;
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return f;
}