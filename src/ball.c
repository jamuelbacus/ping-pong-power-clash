/* 
 * ball.c
 * implements functions to initialize, update, and draw the ball.
 */

#include "raylib.h"
#include "ball.h"
#include "paddle.h"
#include "utils.h"

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

void UpdateBall(Ball* ball, float deltaTime, int screenWidth, int screenHeight, Paddle* paddle1, Paddle* paddle2) {
    // Update effect timers
    UpdateTimer(&ball->slowTimer, deltaTime);
    UpdateTimer(&ball->lagTimer, deltaTime);

    // Apply effects
    if (ball->slowTimer > 0) {
        ball->velocity.x *= 0.8f;
        ball->velocity.y *= 0.8f;
    }

    // TODO: Fix lag effect not working properly
    // TODO: Fix slow effect not working properly
    // PROBLEM: Lag and slow effect staying even after timer ends
    /* SOLUTION: 
    if (ball->TIMERHERE <= 0) { // when timer ends
        set ball->velocity to normal speed, (from where it left of) 
    }

    and 

    if(ball->TIMERHERE > 0) { // when timer is active
        use updatetimer to decrease timer
        set ball->velocity to no speed or slowed speed.
    }
    */

    // Apply curve effect
    ball->velocity.x += ball->curveAngle * deltaTime;

    // Handle lag effect
    if (ball->lagTimer > 0) {
        ball->velocity.x *= 0.9f;
        ball->velocity.y *= 0.9f;
    }

    // Update position
    ball->position.x += ball->velocity.x * deltaTime;
    ball->position.y += ball->velocity.y * deltaTime;

    // Wall collision
    if(ball->position.y - ball->radius <= 0 || ball->position.y + ball->radius >= screenHeight) {
        ball->velocity.y *= -1;
    }

    if(CheckCollisionCircleRec(ball->position, ball->radius, (Rectangle){paddle1->position.x, paddle1->position.y, paddle1->size.x, paddle1->size.y})) {
        // Angle-based bounce
        float hitPos = (ball->position.y - paddle1->position.y) / paddle1->size.y;  // 0 (top) to 1 (bottom)
        float angle = (hitPos - 0.5f) * PI / 2;  // Max 90 degrees up/down
        float speed = sqrt(ball->velocity.x * ball->velocity.x + ball->velocity.y * ball->velocity.y) * ball->speedFactor;
        ball->velocity.x = speed * cos(angle) * (paddle1->position.x < screenWidth / 2 ? 1 : -1);
        ball->velocity.y = speed * sin(angle);
        ball->speedFactor *= 1.005f;
        if (ball->speedFactor > 2.0f) ball->speedFactor = 2.0f;
    }
    if(CheckCollisionCircleRec(ball->position, ball->radius, (Rectangle){paddle2->position.x, paddle2->position.y, paddle2->size.x, paddle2->size.y})) {
        float hitPos = (ball->position.y - paddle2->position.y) / paddle2->size.y;
        float angle = (hitPos - 0.5f) * PI / 2;
        float speed = sqrt(ball->velocity.x * ball->velocity.x + ball->velocity.y * ball->velocity.y) * ball->speedFactor;
        ball->velocity.x = speed * cos(angle) * (paddle2->position.x < screenWidth / 2 ? 1 : -1);
        ball->velocity.y = speed * sin(angle);
        ball->speedFactor *= 1.005f;
        if (ball->speedFactor > 2.0f) ball->speedFactor = 2.0f;
    }

    // Cap max speed
    CapVelocity(&ball->velocity, BALL_MAX_VELOCITY);
}

void DrawBall(const Ball* ball) {
    DrawCircleV(ball->position, ball->radius * ball->sizeMultiplier, ball->color);
    if (ball->fakeBallActive) {
        DrawCircleV(ball->fakeBallPos, ball->radius, Fade(ball->color, 0.5f));
    }
    if (ball->illusions) {
        DrawCircleV((Vector2){ball->position.x + 20, ball->position.y}, ball->radius, Fade(ball->color, 0.7f));
        DrawCircleV((Vector2){ball->position.x - 20, ball->position.y}, ball->radius, Fade(ball->color, 0.7f));
    }
}