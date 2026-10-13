/*
#include <stddef.h>
 * powerup.c
 * Powerup system implementation with fixed pool (no dynamic allocation).
 */

#include "powerup.h"
#include "game.h"
#include "abilities.h"
#include "overdrive.h"
#include <math.h>

#define POWERUP_SPAWN_INTERVAL 8.0f
#define POWERUP_LIFETIME 5.0f
#define POWERUP_EFFECT_DURATION 3.0f
#define POWERUP_RADIUS 12.0f

void PowerupPoolInit(PowerupPool* pool) {
    for (int i = 0; i < POWERUP_POOL_SIZE; i++) {
        pool->items[i].type = POWERUP_NONE;
        pool->items[i].state = POWERUP_STATE_INACTIVE;
        pool->items[i].position = (Vector2){0, 0};
        pool->items[i].timer = 0.0f;
        pool->items[i].duration = 0.0f;
        pool->items[i].active = false;
    }
    pool->spawnTimer = POWERUP_SPAWN_INTERVAL;
    pool->sfx = 0;
}

void PowerupPoolClear(PowerupPool* pool) {
    PowerupPoolInit(pool);
}

static void ApplyEffectToCollector(Powerup* p, Ball* ball, Paddle* paddle1, Paddle* paddle2, Effects* fx, Sfx* sfx, int collectorIndex) {
    (void)ball;
    (void)paddle1;
    (void)paddle2;
    if (collectorIndex < 0 || collectorIndex > 1) return;
    int src = collectorIndex;

    switch (p->type) {
        case POWERUP_SPEED_BOOST:
            EffectsPush(fx, (Effect){ .active = true, .target = TARGET_PADDLE, .paddleIndex = src,
                .stat = STAT_PADDLE_SPEED, .op = OP_MUL, .value = 1.4f,
                .timeLeft = POWERUP_EFFECT_DURATION, .duration = POWERUP_EFFECT_DURATION,
                .color = (Color){100, 200, 255, 255}, .source = 200 + src });
            break;
        case POWERUP_ENLARGE_PADDLE:
            EffectsPush(fx, (Effect){ .active = true, .target = TARGET_PADDLE, .paddleIndex = src,
                .stat = STAT_PADDLE_SIZE, .op = OP_MUL, .value = 1.5f,
                .timeLeft = POWERUP_EFFECT_DURATION, .duration = POWERUP_EFFECT_DURATION,
                .color = (Color){100, 255, 100, 255}, .source = 201 + src });
            break;
        case POWERUP_SHRINK_OPPONENT:
            EffectsPush(fx, (Effect){ .active = true, .target = TARGET_PADDLE, .paddleIndex = 1 - src,
                .stat = STAT_PADDLE_SIZE, .op = OP_MUL, .value = 0.7f,
                .timeLeft = POWERUP_EFFECT_DURATION, .duration = POWERUP_EFFECT_DURATION,
                .color = (Color){255, 100, 100, 255}, .source = 202 + src });
            break;
        case POWERUP_HEAT_PULSE:
            EffectsPush(fx, (Effect){ .active = true, .target = TARGET_PADDLE, .paddleIndex = src,
                .stat = STAT_PADDLE_HEAT, .op = OP_ADD, .value = OVERDRIVE_FLOW_HEAT * 1.5f,
                .timeLeft = POWERUP_EFFECT_DURATION, .duration = POWERUP_EFFECT_DURATION,
                .color = (Color){255, 150, 50, 255}, .source = 203 + src });
            break;
        default:
            break;
    }
    p->state = POWERUP_STATE_COLLECTED;
    p->timer = 0.0f;
    p->active = false;
    if (sfx) PlayPowerupCollect(sfx);
}

void PowerupPoolSpawn(PowerupPool* pool, PowerupType type, Vector2 pos) {
    for (int i = 0; i < POWERUP_POOL_SIZE; i++) {
        if (pool->items[i].state == POWERUP_STATE_INACTIVE || pool->items[i].state == POWERUP_STATE_EXPIRED || pool->items[i].state == POWERUP_STATE_COLLECTED) {
            pool->items[i].type = type;
            pool->items[i].state = POWERUP_STATE_ACTIVE;
            pool->items[i].position = pos;
            pool->items[i].timer = POWERUP_LIFETIME;
            pool->items[i].duration = POWERUP_LIFETIME;
            pool->items[i].active = true;
            return;
        }
    }
    int victim = 0;
    float minTime = pool->items[0].timer;
    for (int i = 1; i < POWERUP_POOL_SIZE; i++) {
        if (pool->items[i].timer < minTime) {
            minTime = pool->items[i].timer;
            victim = i;
        }
    }
    pool->items[victim].type = type;
    pool->items[victim].state = POWERUP_STATE_ACTIVE;
    pool->items[victim].position = pos;
    pool->items[victim].timer = POWERUP_LIFETIME;
    pool->items[victim].duration = POWERUP_LIFETIME;
    pool->items[victim].active = true;
}

void PowerupPoolUpdate(PowerupPool* pool, float deltaTime, int screenWidth, int screenHeight, Ball* ball, Paddle* paddle1, Paddle* paddle2, Effects* fx, Sfx* sfx) {
    (void)screenWidth;
    (void)screenHeight;
    for (int i = 0; i < POWERUP_POOL_SIZE; i++) {
        Powerup* p = &pool->items[i];
        if (p->state != POWERUP_STATE_ACTIVE) continue;
        p->timer -= deltaTime;
        if (p->timer <= 0.0f) {
            p->state = POWERUP_STATE_EXPIRED;
            p->active = false;
            p->timer = 0.0f;
            continue;
        }
        float pr = POWERUP_RADIUS;
        // Check collision with paddle 1
        if (CheckCollisionCircleRec(p->position, pr, (Rectangle){paddle1->position.x, paddle1->position.y, paddle1->size.x, paddle1->size.y})) {
            ApplyEffectToCollector(p, ball, paddle1, paddle2, fx, pool->sfx, 0);
            continue;
        }
        // Check collision with paddle 2
        if (CheckCollisionCircleRec(p->position, pr, (Rectangle){paddle2->position.x, paddle2->position.y, paddle2->size.x, paddle2->size.y})) {
            ApplyEffectToCollector(p, ball, paddle1, paddle2, fx, pool->sfx, 1);
            continue;
        }
        // Check collision with ball
        if (CheckCollisionCircles(p->position, pr, ball->position, ball->radius * ball->sizeMultiplier)) {
            // Determine collector - last hitter or random-ish; prefer last hitter if set
            if (ball->lastHitter == 0 || ball->lastHitter == 1) {
                ApplyEffectToCollector(p, ball, paddle1, paddle2, fx, pool->sfx, ball->lastHitter);
            } else {
                // fallback: side where ball is moving towards
                ApplyEffectToCollector(p, ball, paddle1, paddle2, fx, pool->sfx, ball->velocity.x < 0 ? 0 : 1);
            }
            continue;
        }
    }

    // Simple deterministic spawn - every 8 seconds spawn a random-ish powerup
    pool->spawnTimer -= deltaTime;
    if (pool->spawnTimer <= 0.0f) {
        pool->spawnTimer = POWERUP_SPAWN_INTERVAL;
    pool->sfx = 0;
        // Avoid affecting headless tests by only spawning if in play state? But we don't know game state here
        // Spawn at center-ish area
        Vector2 pos = (Vector2){ WINDOW_WIDTH / 2.0f + (GetRandomValue(-60, 60)), WINDOW_HEIGHT / 2.0f + (GetRandomValue(-60, 60)) };
        PowerupType types[] = { POWERUP_SPEED_BOOST, POWERUP_ENLARGE_PADDLE, POWERUP_SHRINK_OPPONENT, POWERUP_HEAT_PULSE };
        PowerupType t = types[GetRandomValue(0, 3)];
        PowerupPoolSpawn(pool, t, pos);
    }
}

void PowerupPoolDraw(const PowerupPool* pool) {
    for (int i = 0; i < POWERUP_POOL_SIZE; i++) {
        const Powerup* p = &pool->items[i];
        if (p->state != POWERUP_STATE_ACTIVE) continue;
        Color c = WHITE;
        switch (p->type) {
            case POWERUP_SPEED_BOOST: c = (Color){80, 180, 255, 255}; break;
            case POWERUP_ENLARGE_PADDLE: c = (Color){80, 255, 80, 255}; break;
            case POWERUP_SHRINK_OPPONENT: c = (Color){255, 80, 80, 255}; break;
            case POWERUP_HEAT_PULSE: c = (Color){255, 140, 40, 255}; break;
            default: c = WHITE; break;
        }
        DrawCircleV(p->position, POWERUP_RADIUS, c);
        DrawCircleLines((int)p->position.x, (int)p->position.y, POWERUP_RADIUS, BLACK);
    }
}
