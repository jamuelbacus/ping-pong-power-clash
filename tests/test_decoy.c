/*
 * tests/test_decoy.c
 * Headless tests for the Speedster "Shadow Clone" decoy swarm: a fixed-capacity
 * burst of visual-only balls. No window, no entities — it drives the pure
 * DecoySwarm API directly (spawn/divergence, wall reflection, age fade,
 * expiry, replace-not-stack, clear).
 */
#include "decoy.h"
#include <stdio.h>
#include <math.h>

static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

static float Speed(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }

int main(void) {
    /* Empty swarm. */
    DecoySwarm s; DecoySwarmInit(&s);
    CHECK(DecoySwarmActiveCount(&s) == 0, "init has no decoys");

    /* Spawn from a horizontal heading: the clones fan to opposite sides at the
     * same speed, starting together at the origin, age 0. */
    DecoySwarmSpawn(&s, (Vector2){480.0f, 270.0f}, (Vector2){300.0f, 0.0f});
    CHECK(DecoySwarmActiveCount(&s) == DECOY_SPAWN_COUNT, "spawn creates SPAWN_COUNT decoys");
    CHECK(s.items[0].position.x == 480.0f && s.items[0].position.y == 270.0f,
          "decoy starts at the origin");
    CHECK(s.items[0].age == 0.0f && s.items[1].age == 0.0f, "decoys start at age 0");
    CHECK(s.items[0].velocity.y < 0.0f && s.items[1].velocity.y > 0.0f,
          "clones fan to opposite sides of the heading");
    CHECK(fabsf(Speed(s.items[0].velocity) - 300.0f) < 0.05f &&
          fabsf(Speed(s.items[1].velocity) - 300.0f) < 0.05f,
          "clones keep the ball's speed");

    /* Integration: positions advance by velocity * dt. */
    {
        Vector2 p0 = s.items[0].position;
        Vector2 v0 = s.items[0].velocity;
        DecoySwarmUpdate(&s, 0.1f, 540);
        CHECK(fabsf(s.items[0].position.x - (p0.x + v0.x * 0.1f)) < 0.001f &&
              fabsf(s.items[0].position.y - (p0.y + v0.y * 0.1f)) < 0.001f,
              "decoy integrates its velocity");
        CHECK(fabsf(s.items[0].age - 0.1f) < 0.001f, "decoy ages by dt");
    }

    /* Top wall: an upward clone reflects and clamps to the wall face. */
    {
        DecoySwarm top; DecoySwarmInit(&top);
        DecoySwarmSpawn(&top, (Vector2){480.0f, 5.0f}, (Vector2){300.0f, 0.0f});
        CHECK(top.items[0].velocity.y < 0.0f, "spawned near the top heading up");
        DecoySwarmUpdate(&top, 1.0f / 60.0f, 540);
        CHECK(top.items[0].velocity.y > 0.0f, "top wall reflects an upward clone");
        CHECK(fabsf(top.items[0].position.y - DECOY_RADIUS) < 0.001f,
              "top wall clamps the clone to the wall face");
    }

    /* Bottom wall: a downward clone reflects and clamps. */
    {
        DecoySwarm bot; DecoySwarmInit(&bot);
        DecoySwarmSpawn(&bot, (Vector2){480.0f, 535.0f}, (Vector2){300.0f, 0.0f});
        CHECK(bot.items[1].velocity.y > 0.0f, "spawned near the bottom heading down");
        DecoySwarmUpdate(&bot, 1.0f / 60.0f, 540);
        CHECK(bot.items[1].velocity.y < 0.0f, "bottom wall reflects a downward clone");
        CHECK(fabsf(bot.items[1].position.y - (540.0f - DECOY_RADIUS)) < 0.001f,
              "bottom wall clamps the clone to the wall face");
    }

    /* Alpha: opaque while fresh, fading only over the final DECOY_FADE_TIME,
     * fully transparent at (and past) DECOY_LIFETIME. */
    CHECK(DecoyAlpha(0.0f) == 1.0f, "fresh decoy is fully opaque");
    CHECK(DecoyAlpha(DECOY_LIFETIME - DECOY_FADE_TIME * 0.5f) > 0.0f &&
          DecoyAlpha(DECOY_LIFETIME - DECOY_FADE_TIME * 0.5f) < 1.0f,
          "decoy fades near end of life");
    CHECK(DecoyAlpha(DECOY_LIFETIME) == 0.0f, "expired decoy is fully transparent");

    /* Expiry: one full-lifetime step retires every clone. */
    {
        DecoySwarm e; DecoySwarmInit(&e);
        DecoySwarmSpawn(&e, (Vector2){480.0f, 270.0f}, (Vector2){300.0f, 0.0f});
        DecoySwarmUpdate(&e, DECOY_LIFETIME, 540);
        CHECK(DecoySwarmActiveCount(&e) == 0, "decoys expire at DECOY_LIFETIME");
    }

    /* Replace, never stack: re-spawning resets the swarm and can never exceed
     * the fixed capacity. */
    {
        DecoySwarm r; DecoySwarmInit(&r);
        for (int i = 0; i < 5; i++)
            DecoySwarmSpawn(&r, (Vector2){100.0f + i, 100.0f + i}, (Vector2){300.0f, 0.0f});
        CHECK(DecoySwarmActiveCount(&r) == DECOY_SPAWN_COUNT,
              "re-spawn replaces the swarm rather than stacking");
        CHECK(DecoySwarmActiveCount(&r) <= DECOY_MAX, "swarm never exceeds DECOY_MAX");

        DecoySwarmInit(&r);
        CHECK(DecoySwarmActiveCount(&r) == 0, "init clears a live swarm");
    }

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
