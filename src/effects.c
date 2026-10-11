/*
 * effects.c
 * implements the fixed-capacity timed effect registry. No dynamic allocation:
 * all storage lives inside the Effects struct.
 */

#include "raylib.h"
#include "effects.h"

/* True when an entry is live and matches the queried (target, paddle, stat). */
static bool EffectMatches(const Effect* e, EffectTarget t, int paddleIndex, EffectStat s) {
    return e->active
        && e->target == t
        && e->paddleIndex == paddleIndex
        && e->stat == s;
}

void EffectsClear(Effects* fx) {
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        fx->items[i] = (Effect){0};
    }
}

void EffectsInit(Effects* fx) {
    EffectsClear(fx);
}

void EffectsPush(Effects* fx, Effect e) {
    /* Refresh an existing entry with the same (target, paddle, stat, source)
     * instead of stacking a duplicate. The new effect fully replaces the slot
     * (including `op`), so re-casting an ability — even one that hits the same
     * stat with a different operation, e.g. Defender Wide Paddle (MUL) then
     * Iron Fortress (SET) — makes the latest activation authoritative. */
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        Effect* slot = &fx->items[i];
        if (slot->active
            && slot->target == e.target
            && slot->paddleIndex == e.paddleIndex
            && slot->stat == e.stat
            && slot->source == e.source) {
            *slot = e;
            return;
        }
    }

    /* Otherwise claim the first inactive slot. */
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        if (!fx->items[i].active) {
            fx->items[i] = e;
            return;
        }
    }

    /* Full: evict the entry closest to expiring. */
    int victim = 0;
    for (int i = 1; i < EFFECT_CAPACITY; i++) {
        if (fx->items[i].timeLeft < fx->items[victim].timeLeft) {
            victim = i;
        }
    }
    fx->items[victim] = e;
}

void EffectsUpdate(Effects* fx, float dt) {
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        Effect* e = &fx->items[i];
        if (!e->active) continue;
        e->timeLeft -= dt;
        if (e->timeLeft <= 0.0f) {
            e->active = false;
        }
    }
}

bool EffectsHas(const Effects* fx, EffectTarget t, int paddleIndex, EffectStat s) {
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        if (EffectMatches(&fx->items[i], t, paddleIndex, s)) {
            return true;
        }
    }
    return false;
}

float EffectsApply(const Effects* fx, EffectTarget t, int paddleIndex, EffectStat s, float base) {
    float value = base;

    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        const Effect* e = &fx->items[i];
        if (EffectMatches(e, t, paddleIndex, s) && e->op == OP_MUL) {
            value *= e->value;
        }
    }
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        const Effect* e = &fx->items[i];
        if (EffectMatches(e, t, paddleIndex, s) && e->op == OP_ADD) {
            value += e->value;
        }
    }
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        const Effect* e = &fx->items[i];
        if (EffectMatches(e, t, paddleIndex, s) && e->op == OP_SET) {
            value = e->value;
            break; /* lowest index wins */
        }
    }

    return value;
}

const Effect* EffectsList(const Effects* fx, int* countOut) {
    *countOut = EFFECT_CAPACITY;
    return fx->items;
}
