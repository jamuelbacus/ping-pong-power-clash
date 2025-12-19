/* 
 * paddle.c
 * implements functions to initialize, update, and draw the paddle.
 */

#include "raylib.h"
#include "paddle.h"
#include "ball.h"

void InitPaddle(Paddle* paddle, float x, float y, float width, float height, float speed, Color color, bool isCpu, float accel, float maxSpeed, float friction, int upKey, int downKey, CharacterType charType) {
    paddle->position = (Vector2){x, y};
    paddle->size = (Vector2){width, height};
    paddle->speed = speed;
    paddle->color = color;
    paddle->isCpu = isCpu;
    paddle->velocityY = 0.0f;
    paddle->accel = accel;
    paddle->maxSpeed = maxSpeed;
    paddle->friction = friction;
    paddle->upKey = upKey;
    paddle->downKey = downKey;
    paddle->charType = charType;
    paddle->skill1Cooldown = 0.0f;
    paddle->skill2Cooldown = 0.0f;
    paddle->ultimateCooldown = 0.0f;
    paddle->skill1Timer = 0.0f;
    paddle->skill2Timer = 0.0f;
    paddle->ultimateTimer = 0.0f;
    paddle->skill1Active = false;
    paddle->skill2Active = false;
    paddle->ultimateActive = false;
    paddle->speedMultiplier = 1.0f;
    paddle->sizeMultiplier = 1.0f;
    paddle->immovable = false;
    paddle->mirrorField = false;
    paddle->heavyPaddle = false;
    paddle->smashShotReady = false;
    paddle->shieldBlock = false;
    paddle->piercingShot = false;
    paddle->originalSize = paddle->size;
    paddle->originalMaxSpeed = paddle->maxSpeed;
    paddle->widthMultiplier = 1.0f;
    paddle->deadeye = false;
}

void UpdatePaddle(Paddle* paddle, float deltaTime, int screenHeight, Ball* ball) {
    if (paddle->isCpu) {
        // CPU controls: instant acceleration to max speed towards ball
        float targetY = ball->position.y;
        if (paddle->position.y < targetY) {
            paddle->velocityY = paddle->maxSpeed;
        } else if (paddle->position.y > targetY) {
            paddle->velocityY = -paddle->maxSpeed;
        } else {
            paddle->velocityY = 0.0f;
        }
    } else {
        // Player controls: instant acceleration to max speed
        if (IsKeyDown(paddle->upKey)) {
            paddle->velocityY = -paddle->maxSpeed;
        } else if (IsKeyDown(paddle->downKey)) {
            paddle->velocityY = paddle->maxSpeed;
        } else {
            paddle->velocityY = 0.0f;  // Stop instantly
        }
    }

    // Update cooldowns
    paddle->skill1Cooldown -= deltaTime;
    if (paddle->skill1Cooldown < 0) paddle->skill1Cooldown = 0;
    paddle->skill2Cooldown -= deltaTime;
    if (paddle->skill2Cooldown < 0) paddle->skill2Cooldown = 0;
    paddle->ultimateCooldown -= deltaTime;
    if (paddle->ultimateCooldown < 0) paddle->ultimateCooldown = 0;

    // Update timers
    if (paddle->skill1Timer > 0) {
        paddle->skill1Timer -= deltaTime;
        if (paddle->skill1Timer <= 0) {
            paddle->skill1Active = false;
            // reset effects
            switch (paddle->charType) {
                case SPEEDSTER:
                    paddle->speedMultiplier = 1.0f;
                    break;
                case DEFENDER:
                    paddle->sizeMultiplier = 1.0f;
                    break;
                // add more as needed
            }
        }
    }
    if (paddle->skill2Timer > 0) {
        paddle->skill2Timer -= deltaTime;
        if (paddle->skill2Timer <= 0) {
            paddle->skill2Active = false;
            paddle->heavyPaddle = false;
            paddle->sizeMultiplier = 1.0f;
        }
    }
    if (paddle->ultimateTimer > 0) {
        paddle->ultimateTimer -= deltaTime;
        if (paddle->ultimateTimer <= 0) {
            paddle->ultimateActive = false;
            paddle->immovable = false;
            paddle->mirrorField = false;
            ball->illusions = false;
            ball->unpredictableBounce = false;
            ball->randomEffects = false;
        }
    }

    // Apply multipliers
    paddle->maxSpeed = paddle->originalMaxSpeed * paddle->speedMultiplier;
    if (paddle->immovable) {
        paddle->size = (Vector2){paddle->originalSize.x, (float)screenHeight};
        paddle->position.y = 0;
    } else {
        paddle->size = (Vector2){paddle->originalSize.x * paddle->widthMultiplier, paddle->originalSize.y * paddle->sizeMultiplier};
    }

    // Skill activation
    if (!paddle->isCpu) {
        int qKey = (paddle->position.x > 480) ? KEY_KP_1 : KEY_Q;
        int eKey = (paddle->position.x > 480) ? KEY_KP_2 : KEY_E;
        int rKey = (paddle->position.x > 480) ? KEY_KP_3 : KEY_R;
        if (IsKeyPressed(qKey) && paddle->skill1Cooldown <= 0) {
            paddle->skill1Cooldown = 5.0f;
            paddle->skill1Active = true;
            paddle->skill1Timer = 3.0f;
            switch (paddle->charType) {
                case SPEEDSTER:
                    // Dash Return
                    paddle->position.y = ball->position.y - paddle->size.y / 2;
                    break;
                case POWER_STRIKER:
                    paddle->smashShotReady = true;
                    break;
                case GRAND_WIZARD:
                    ball->curveAngle = 100.0f; // bend right
                    break;
                case TRICKSTER:
                    ball->fakeBallActive = true;
                    ball->fakeBallPos = (Vector2){GetRandomValue(0,960), GetRandomValue(0,540)};
                    break;
                case DEFENDER:
                    paddle->shieldBlock = true;
                    break;
                case ELEMENTALIST:
                    ball->velocity.x *= 1.5f; // Fire Ball
                    break;
                case SNIPER:
                    // Perfect Angle: show trajectory, but for now, do nothing
                    break;
                case CHAOS_BOT:
                    ball->unpredictableBounce = true;
                    break;
            }
        }
        if (IsKeyPressed(eKey) && paddle->skill2Cooldown <= 0) {
            paddle->skill2Cooldown = 7.0f;
            paddle->skill2Active = true;
            paddle->skill2Timer = 4.0f;
            switch (paddle->charType) {
                case SPEEDSTER:
                    paddle->speedMultiplier = 2.0f;
                    break;
                case POWER_STRIKER:
                    paddle->heavyPaddle = true;
                    break;
                case GRAND_WIZARD:
                    ball->slowTimer = 2.0f;
                    break;
                case TRICKSTER:
                    ball->velocity.x *= -1.1f; // Reverse Spin
                    break;
                case DEFENDER:
                    paddle->sizeMultiplier = 2.0f;
                    break;
                case ELEMENTALIST:
                    ball->slowTimer = 3.0f;
                    break;
                case SNIPER:
                    paddle->piercingShot = true;
                    break;
                case CHAOS_BOT:
                    ball->lagTimer = 0.5f;
                    break;
            }
        }
        if (IsKeyPressed(rKey) && paddle->ultimateCooldown <= 0) {
            paddle->ultimateCooldown = 15.0f;
            paddle->ultimateActive = true;
            paddle->ultimateTimer = 5.0f;
            switch (paddle->charType) {
                case SPEEDSTER:
                    ball->illusions = true;
                    break;
                case POWER_STRIKER:
                    ball->velocity.y += 200; // Meteor Strike
                    ball->unpredictableBounce = true;
                    break;
                case GRAND_WIZARD:
                    ball->randomEffects = true;
                    break;
                case TRICKSTER:
                    paddle->mirrorField = true;
                    break;
                case DEFENDER:
                    paddle->immovable = true;
                    break;
                case ELEMENTALIST:
                    ball->velocity.x *= 2; // Storm Surge
                    break;
                case SNIPER:
                    paddle->deadeye = true;
                    break;
                case CHAOS_BOT:
                    ball->randomEffects = true;
                    break;
            }
        }
    } else {
        // CPU skill activation
        if (paddle->skill1Cooldown <= 0 && GetRandomValue(0, 1000) < 5) { // 0.5% chance
            paddle->skill1Cooldown = 5.0f;
            paddle->skill1Active = true;
            paddle->skill1Timer = 3.0f;
            switch (paddle->charType) {
                case SPEEDSTER:
                    paddle->position.y = ball->position.y - paddle->size.y / 2;
                    break;
                case POWER_STRIKER:
                    paddle->smashShotReady = true;
                    break;
                case GRAND_WIZARD:
                    ball->curveAngle = 100.0f;
                    break;
                case TRICKSTER:
                    ball->fakeBallActive = true;
                    ball->fakeBallPos = (Vector2){GetRandomValue(0,960), GetRandomValue(0,540)};
                    break;
                case DEFENDER:
                    paddle->shieldBlock = true;
                    break;
                case ELEMENTALIST:
                    ball->velocity.x *= 1.5f;
                    break;
                case SNIPER:
                    break;
                case CHAOS_BOT:
                    ball->unpredictableBounce = true;
                    break;
            }
        }
        if (paddle->skill2Cooldown <= 0 && GetRandomValue(0, 1000) < 3) {
            paddle->skill2Cooldown = 7.0f;
            paddle->skill2Active = true;
            paddle->skill2Timer = 4.0f;
            switch (paddle->charType) {
                case SPEEDSTER:
                    paddle->speedMultiplier = 2.0f;
                    break;
                case POWER_STRIKER:
                    paddle->heavyPaddle = true;
                    break;
                case GRAND_WIZARD:
                    ball->slowTimer = 2.0f;
                    break;
                case TRICKSTER:
                    ball->velocity.x *= -1.1f;
                    break;
                case DEFENDER:
                    paddle->sizeMultiplier = 2.0f;
                    break;
                case ELEMENTALIST:
                    ball->slowTimer = 3.0f;
                    break;
                case SNIPER:
                    paddle->piercingShot = true;
                    break;
                case CHAOS_BOT:
                    ball->lagTimer = 0.5f;
                    break;
            }
        }
        if (paddle->ultimateCooldown <= 0 && GetRandomValue(0, 1000) < 1) { // rare
            paddle->ultimateCooldown = 15.0f;
            paddle->ultimateActive = true;
            paddle->ultimateTimer = 5.0f;
            switch (paddle->charType) {
                case SPEEDSTER:
                    ball->illusions = true;
                    break;
                case POWER_STRIKER:
                    ball->velocity.y += 200;
                    ball->unpredictableBounce = true;
                    break;
                case GRAND_WIZARD:
                    ball->randomEffects = true;
                    break;
                case TRICKSTER:
                    paddle->mirrorField = true;
                    break;
                case DEFENDER:
                    paddle->immovable = true;
                    break;
                case ELEMENTALIST:
                    ball->velocity.x *= 2;
                    break;
                case SNIPER:
                    paddle->deadeye = true;
                    break;
                case CHAOS_BOT:
                    ball->randomEffects = true;
                    break;
            }
        }
    }

    // Update position
    if (!paddle->immovable) {
        paddle->position.y += paddle->velocityY * deltaTime;
    }

    // Keep within screen
    if (paddle->position.y < 0) {
        paddle->position.y = 0;
        paddle->velocityY = 0;
    }
    if (paddle->position.y + paddle->size.y > screenHeight) {
        paddle->position.y = screenHeight - paddle->size.y;
        paddle->velocityY = 0;
    }
}

void DrawPaddle(Paddle paddle) {
    DrawRectangleV(paddle.position, paddle.size, paddle.color);
}