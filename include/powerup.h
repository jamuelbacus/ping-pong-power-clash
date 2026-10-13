/*
 * powerup.h
 * Powerup system with fixed pool (no dynamic allocation).
 */

#ifndef POWERUP_H
#define POWERUP_H

#include <stdbool.h>
#include "raylib.h"
#include "effects.h"
#include "sfx.h"

typedef struct Ball Ball;
typedef struct Paddle Paddle;

#define POWERUP_POOL_SIZE 4

typedef enum {
    POWERUP_NONE = -1,
    POWERUP_SPEED_BOOST = 0,
    POWERUP_ENLARGE_PADDLE = 1,
    POWERUP_SHRINK_OPPONENT = 2,
    POWERUP_HEAT_PULSE = 3
} PowerupType;

typedef enum {
    POWERUP_STATE_INACTIVE = 0,
    POWERUP_STATE_ACTIVE = 1,
    POWERUP_STATE_SPAWNED = 2,
    POWERUP_STATE_COLLECTED = 3,
    POWERUP_STATE_EXPIRED = 4
} PowerupState;

typedef struct Powerup {
    PowerupType type;
    PowerupState state;
    Vector2 position;
    float timer;
    float duration;
    bool active;
} Powerup;

typedef struct PowerupPool {
    Powerup items[POWERUP_POOL_SIZE];
    float spawnTimer;
    Sfx* sfx;
} PowerupPool;

void PowerupPoolInit(PowerupPool* pool);
void PowerupPoolUpdate(PowerupPool* pool, float deltaTime, int screenWidth, int screenHeight, Ball* ball, Paddle* paddle1, Paddle* paddle2, Effects* fx, Sfx* sfx);
void PowerupPoolSpawn(PowerupPool* pool, PowerupType type, Vector2 pos);
void PowerupPoolClear(PowerupPool* pool);
void PowerupPoolDraw(const PowerupPool* pool);

#endif // POWERUP_H
