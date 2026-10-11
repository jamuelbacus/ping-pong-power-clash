/* 
 * paddle.c
 * implements functions to initialize, update, and draw the paddle.
 */

#include "raylib.h"
#include "paddle.h"
#include "ball.h"
#include "abilities.h"
#include "theme.h"
#include "trail.h"
#include "overdrive.h"

#include <math.h>

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
    paddle->cpuDecisionTimer = 0.0f;
    paddle->skill1Timer = 0.0f;
    paddle->skill2Timer = 0.0f;
    paddle->ultimateTimer = 0.0f;
    paddle->skill1Active = false;
    paddle->skill2Active = false;
    paddle->ultimateActive = false;
    paddle->speedMultiplier = 1.0f;
    paddle->sizeMultiplier = 1.0f;
    paddle->immovable = false;
    paddle->shieldTimer = 0.0f;
    paddle->mirrorField = false;
    paddle->heavyPaddle = false;
    paddle->smashShotReady = false;
    paddle->piercingShot = false;
    paddle->originalSize = paddle->size;
    paddle->originalMaxSpeed = paddle->maxSpeed;
    paddle->widthMultiplier = 1.0f;
    paddle->deadeye = false;
    TrailHistoryInit(&paddle->trail);
}

void UpdatePaddle(Paddle* paddle, Effects* fx, int paddleIndex, float deltaTime, int screenHeight, Ball* ball) {
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

    // Defender Shield Block: decay the armed reflect window each frame.
    if (paddle->shieldTimer > 0.0f) {
        paddle->shieldTimer -= deltaTime;
        if (paddle->shieldTimer < 0.0f) paddle->shieldTimer = 0.0f;
    }

    // Derive effective stats from base + active effects each frame.
    if (paddle->charType == SPEEDSTER) {
        // Speedster passive "Overdrive": the ball's effective speed this frame
        // feeds one heat scalar in [0,1] (rally), which Flow stacks onto. Heat
        // widens maxSpeed before the STAT_PADDLE_SPEED chain runs, so the
        // existing multipliers still apply to the boosted base.
        float ballSpeed = sqrtf(ball->velocity.x * ball->velocity.x
                              + ball->velocity.y * ball->velocity.y);
        float rally = (ballSpeed - BALL_SPEED_BASE) / BALL_SPEED_BASE;
        if (rally < 0.0f) rally = 0.0f;
        if (rally > 1.0f) rally = 1.0f;
        float heat = EffectsApply(fx, TARGET_PADDLE, paddleIndex, STAT_PADDLE_HEAT, rally);
        if (heat < 0.0f) heat = 0.0f;
        if (heat > 1.0f) heat = 1.0f;
        float base = paddle->originalMaxSpeed * (1.0f + OVERDRIVE_SPEED_PER_HEAT * heat);
        paddle->maxSpeed = EffectsApply(fx, TARGET_PADDLE, paddleIndex, STAT_PADDLE_SPEED, base);
    } else {
        paddle->maxSpeed = EffectsApply(fx, TARGET_PADDLE, paddleIndex, STAT_PADDLE_SPEED, paddle->originalMaxSpeed);
    }
    float effectiveHeight = EffectsApply(fx, TARGET_PADDLE, paddleIndex, STAT_PADDLE_SIZE, paddle->originalSize.y);
    // Defender Iron Fortress maps STAT_PADDLE_IMMOVABLE onto the paddle here so
    // the immovable branches below always read the live effect state.
    paddle->immovable = EffectsHas(fx, TARGET_PADDLE, paddleIndex, STAT_PADDLE_IMMOVABLE);
    if (paddle->immovable) {
        paddle->size = (Vector2){paddle->originalSize.x, (float)screenHeight};
        paddle->position.y = 0;
    } else {
        paddle->size = (Vector2){paddle->originalSize.x, effectiveHeight};
    }

    // Skill activation: player input only. CPU decisions are driven by
    // CpuTickAbilities() from the game loop.
    if (!paddle->isCpu) {
        int qKey = (paddle->position.x > 480) ? KEY_KP_1 : KEY_Q;
        int eKey = (paddle->position.x > 480) ? KEY_KP_2 : KEY_E;
        int rKey = (paddle->position.x > 480) ? KEY_KP_3 : KEY_R;
        if (IsKeyPressed(qKey) && paddle->skill1Cooldown <= 0.0f)
            AbilitiesActivate(fx, ball, paddle, paddleIndex, paddle->charType, 0);
        if (IsKeyPressed(eKey) && paddle->skill2Cooldown <= 0.0f)
            AbilitiesActivate(fx, ball, paddle, paddleIndex, paddle->charType, 1);
        if (IsKeyPressed(rKey) && paddle->ultimateCooldown <= 0.0f)
            AbilitiesActivate(fx, ball, paddle, paddleIndex, paddle->charType, 2);
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

    // "True afterimage" history: ghosts always age out, and positions are only
    // recorded while this paddle's STAT_TRAIL marker is live, so the ring never
    // carries footprints from before the effect started.
    TrailHistoryUpdate(&paddle->trail, deltaTime);
    if (EffectsHas(fx, TARGET_PADDLE, paddleIndex, STAT_TRAIL))
        TrailHistoryRecord(&paddle->trail, paddle->position);
}

void DrawPaddle(Paddle paddle) {
    Color accent = characters[paddle.charType].accent;
    Rectangle rec = { paddle.position.x, paddle.position.y, paddle.size.x, paddle.size.y };
    DrawGlowRect(rec, accent, 10.0f);
}