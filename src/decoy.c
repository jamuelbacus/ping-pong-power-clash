/*
 * decoy.c
 * Speedster "Shadow Clone" decoy swarm. Pure visual illusion: fixed capacity,
 * no dynamic allocation, no collision/scoring — clones only move and age.
 */

#include "decoy.h"
#include <math.h>

static Vector2 Rotate(Vector2 v, float angle) {
    float c = cosf(angle), s = sinf(angle);
    return (Vector2){ v.x * c - v.y * s, v.x * s + v.y * c };
}

void DecoySwarmInit(DecoySwarm* swarm) {
    for (int i = 0; i < DECOY_MAX; i++) {
        swarm->items[i].position = (Vector2){ 0.0f, 0.0f };
        swarm->items[i].velocity = (Vector2){ 0.0f, 0.0f };
        swarm->items[i].age = 0.0f;
        swarm->items[i].live = false;
    }
}

void DecoySwarmSpawn(DecoySwarm* swarm, Vector2 origin, Vector2 velocity) {
    /* Re-activation replaces the current burst rather than stacking. */
    DecoySwarmInit(swarm);

    int n = DECOY_SPAWN_COUNT;
    if (n > DECOY_MAX) n = DECOY_MAX;

    for (int i = 0; i < n; i++) {
        /* Fan evenly around the ball's heading: -spread .. +spread. */
        float angle = DECOY_SPREAD * (2.0f * (float)i - (float)(n - 1));
        swarm->items[i].position = origin;
        swarm->items[i].velocity = Rotate(velocity, angle);
        swarm->items[i].age = 0.0f;
        swarm->items[i].live = true;
    }
}

void DecoySwarmUpdate(DecoySwarm* swarm, float deltaTime, int screenHeight) {
    for (int i = 0; i < DECOY_MAX; i++) {
        Decoy* d = &swarm->items[i];
        if (!d->live) continue;

        d->age += deltaTime;
        if (d->age >= DECOY_LIFETIME) {
            d->live = false;
            continue;
        }

        d->position.x += d->velocity.x * deltaTime;
        d->position.y += d->velocity.y * deltaTime;

        /* Top/bottom reflection only: clones read as balls, but they are not
         * physics entities and never interact with paddles or the goal lines. */
        if (d->position.y - DECOY_RADIUS <= 0.0f) {
            if (d->velocity.y < 0.0f) d->velocity.y = -d->velocity.y;
            d->position.y = DECOY_RADIUS;
        } else if (d->position.y + DECOY_RADIUS >= (float)screenHeight) {
            if (d->velocity.y > 0.0f) d->velocity.y = -d->velocity.y;
            d->position.y = (float)screenHeight - DECOY_RADIUS;
        }
    }
}

int DecoySwarmActiveCount(const DecoySwarm* swarm) {
    int count = 0;
    for (int i = 0; i < DECOY_MAX; i++)
        if (swarm->items[i].live) count++;
    return count;
}

float DecoyAlpha(float age) {
    if (age >= DECOY_LIFETIME) return 0.0f;
    float remaining = DECOY_LIFETIME - age;
    if (remaining >= DECOY_FADE_TIME) return 1.0f;
    return remaining / DECOY_FADE_TIME;
}
