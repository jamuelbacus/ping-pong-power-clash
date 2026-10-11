/* 
 * ball.c
 * implements functions to initialize, update, and draw the ball.
 */

#include "raylib.h"
#include "ball.h"
#include "paddle.h"
#include "theme.h"
#include "effects.h"

#include <math.h>

static void CapVelocity(Vector2* velocity, float maxVelocity) {
    float speed = sqrt(velocity->x * velocity->x + velocity->y * velocity->y);
    if (speed > maxVelocity) {
        velocity->x = (velocity->x / speed) * maxVelocity;
        velocity->y = (velocity->y / speed) * maxVelocity;
    }
}

void InitBall(Ball* ball, float x, float y, float radius, float velX, float velY, Color color) {
    *ball = (Ball){
        .position = {x, y},
        .velocity = {velX, velY},
        .baseVelocity = {velX, velY},
        .radius = radius,
        .color = color,
        .speedFactor = 1.0f,
        .slowTimer = 0.0f,
        .curveAngle = 0.0f,
        .illusions = false,
        .fakeBallPos = {0.0f, 0.0f},
        .fakeBallActive = false,
        .sizeMultiplier = 1.0f,
        .unpredictableBounce = false,
        .lagTimer = 0.0f,
        .randomEffects = false
    };
}

/* Derive the effective velocity from baseVelocity plus active ball effects.
 * STAT_BALL_SPEED scales the magnitude, then STAT_BALL_VEL_X/Y adjust each
 * component. Called after any base-velocity mutation so `velocity` always
 * reflects the current direction within the same frame. */
static void ApplyBallEffects(Ball* ball, Effects* fx) {
    ball->velocity = ball->baseVelocity;

    float baseSpeed = sqrt(ball->baseVelocity.x * ball->baseVelocity.x
                         + ball->baseVelocity.y * ball->baseVelocity.y);
    if (baseSpeed > 0.0f) {
        float speed = EffectsApply(fx, TARGET_BALL, -1, STAT_BALL_SPEED, baseSpeed);
        float scale = speed / baseSpeed;
        ball->velocity.x *= scale;
        ball->velocity.y *= scale;
    }

    ball->velocity.x = EffectsApply(fx, TARGET_BALL, -1, STAT_BALL_VEL_X, ball->velocity.x);
    ball->velocity.y = EffectsApply(fx, TARGET_BALL, -1, STAT_BALL_VEL_Y, ball->velocity.y);
}

void UpdateBall(Ball* ball, Effects* fx, float deltaTime, int screenWidth, int screenHeight, Paddle* paddle1, Paddle* paddle2) {
    // Curve is a lateral acceleration applied to the base (source of truth) direction.
    ball->baseVelocity.x += ball->curveAngle * deltaTime;

    // Derive the effective velocity from base + active effects.
    ApplyBallEffects(ball, fx);

    // Update position
    ball->position.x += ball->velocity.x * deltaTime;
    ball->position.y += ball->velocity.y * deltaTime;

    // Wall collision (mutate the base; effective velocity is re-derived below)
    if(ball->position.y - ball->radius <= 0 || ball->position.y + ball->radius >= screenHeight) {
        ball->baseVelocity.y *= -1;
    }

    if(CheckCollisionCircleRec(ball->position, ball->radius, (Rectangle){paddle1->position.x, paddle1->position.y, paddle1->size.x, paddle1->size.y})) {
        // Angle-based bounce
        float hitPos = (ball->position.y - paddle1->position.y) / paddle1->size.y;  // 0 (top) to 1 (bottom)
        float angle = (hitPos - 0.5f) * PI / 2;  // Max 90 degrees up/down
        float baseSpeed = sqrt(ball->baseVelocity.x * ball->baseVelocity.x + ball->baseVelocity.y * ball->baseVelocity.y);
        float speed = baseSpeed * ball->speedFactor;  // derive from base so effects apply exactly once
        ball->baseVelocity.x = speed * cos(angle) * (paddle1->position.x < screenWidth / 2 ? 1 : -1);
        ball->baseVelocity.y = speed * sin(angle);
        ball->speedFactor *= 1.005f;
        if (ball->speedFactor > 2.0f) ball->speedFactor = 2.0f;
    }
    if(CheckCollisionCircleRec(ball->position, ball->radius, (Rectangle){paddle2->position.x, paddle2->position.y, paddle2->size.x, paddle2->size.y})) {
        float hitPos = (ball->position.y - paddle2->position.y) / paddle2->size.y;
        float angle = (hitPos - 0.5f) * PI / 2;
        float baseSpeed = sqrt(ball->baseVelocity.x * ball->baseVelocity.x + ball->baseVelocity.y * ball->baseVelocity.y);
        float speed = baseSpeed * ball->speedFactor;  // derive from base so effects apply exactly once
        ball->baseVelocity.x = speed * cos(angle) * (paddle2->position.x < screenWidth / 2 ? 1 : -1);
        ball->baseVelocity.y = speed * sin(angle);
        ball->speedFactor *= 1.005f;
        if (ball->speedFactor > 2.0f) ball->speedFactor = 2.0f;
    }

    // Reflect any base-velocity mutation back into the effective velocity so
    // callers (and the next frame) observe the post-collision direction now.
    ApplyBallEffects(ball, fx);

    // Cap max speed
    CapVelocity(&ball->velocity, BALL_MAX_VELOCITY);
}

void DrawBall(const Ball* ball) {
    DrawGlowCircle(ball->position, ball->radius * ball->sizeMultiplier, ball->color, 8.0f);
    if (ball->fakeBallActive) {
        DrawGlowCircle(ball->fakeBallPos, ball->radius, Fade(ball->color, 0.6f), 4.0f);
    }
    if (ball->illusions) {
        DrawGlowCircle((Vector2){ball->position.x + 20, ball->position.y}, ball->radius, Fade(ball->color, 0.7f), 4.0f);
        DrawGlowCircle((Vector2){ball->position.x - 20, ball->position.y}, ball->radius, Fade(ball->color, 0.7f), 4.0f);
    }
}