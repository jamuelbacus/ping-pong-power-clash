/*
 * ball.h
 * defines the Ball struct and declares functions to initialize, update, and draw the ball.
 */

#ifndef BALL_H
#define BALL_H

#include <stdbool.h>
#include "raylib.h"
#include "effects.h"

static const float BALL_SIZE = 10.0f;
static const float BALL_BASE_VELOCITY_X = 300.0f;
static const float BALL_BASE_VELOCITY_Y = 200.0f;
static const float BALL_MAX_VELOCITY = 800.0f;
static const float BALL_SPEED_INCREMENT = 20.0f;
static const float CURVE_DECAY_RATE = 400.0f; /* px/s of lateral bend shed per second once the curve effect ends */
static const Color BALL_COLOR = RED;
static const Vector2 BALL_POSITION = {480, 270};

typedef struct Paddle Paddle; // forward declaration

// Ball structure
typedef struct Ball {
    Vector2 position;
    Vector2 velocity;      // effective velocity used by physics (derived from baseVelocity)
    Vector2 baseVelocity;  // source of truth for direction/speed; collisions write here
    float radius;
    Color color;
    float speedFactor;

    // Skill effect variables
    float slowTimer;
    float lateralVelocity; // curve bend: accumulates while STAT_BALL_CURVE is active, decays on expiry
    bool illusions;
    Vector2 fakeBallPos;
    bool fakeBallActive;
    float sizeMultiplier;
    bool unpredictableBounce;
    float lagTimer;
    bool randomEffects;
} Ball;

void InitBall(Ball* ball, float x, float y, float radius, float velX, float velY, Color color);
void UpdateBall(Ball* ball, Effects* fx, float deltaTime, int screenWidth, int screenHeight, Paddle* paddle1, Paddle* paddle2);
void DrawBall(const Ball* ball);

#endif // BALL_H