/*
 * decoy.h
 * Speedster ultimate "Shadow Clone": a fixed-capacity burst of visual-only
 * decoy balls. Decoys move and bounce off the top/bottom walls so they read
 * as real, but they are pure illusion — they never touch a paddle, the effects
 * registry, or the score. No dynamic allocation.
 */

#ifndef DECOY_H
#define DECOY_H

#include <stdbool.h>
#include "raylib.h"

/* Clones created per activation. */
#define DECOY_SPAWN_COUNT 2

/* Fixed capacity. Re-activating replaces the live burst rather than stacking,
 * so a swarm is never wider than a single activation. */
#define DECOY_MAX 2

/* How long a clone lives (seconds), and how much of the tail end it spends
 * fading out. */
#define DECOY_LIFETIME 1.2f
#define DECOY_FADE_TIME 0.4f

/* Reflection radius. Matches BALL_SIZE so clones bounce at the same offset the
 * real ball does (decoy.h cannot include ball.h: ball.h includes this file). */
#define DECOY_RADIUS 10.0f

/* Angle (radians) each clone is rotated off the ball's heading. */
#define DECOY_SPREAD (30.0f * PI / 180.0f)

typedef struct Decoy {
    Vector2 position;
    Vector2 velocity;
    float age;      /* seconds alive; retires at DECOY_LIFETIME */
    bool live;
} Decoy;

typedef struct DecoySwarm {
    Decoy items[DECOY_MAX];
} DecoySwarm;

void DecoySwarmInit(DecoySwarm* swarm);
void DecoySwarmSpawn(DecoySwarm* swarm, Vector2 origin, Vector2 velocity);
void DecoySwarmUpdate(DecoySwarm* swarm, float deltaTime, int screenHeight);
int DecoySwarmActiveCount(const DecoySwarm* swarm);

/* Draw-time alpha: opaque while fresh, ramping to 0 over the final
 * DECOY_FADE_TIME so the clones dissolve rather than pop. */
float DecoyAlpha(float age);

#endif // DECOY_H
