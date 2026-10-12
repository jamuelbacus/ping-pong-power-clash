/* 
 * ball.c
 * implements functions to initialize, update, and draw the ball.
 */

#include "raylib.h"
#include "ball.h"
#include "paddle.h"
#include "theme.h"
#include "effects.h"
#include "abilities.h"
#include "trail.h"
#include "overdrive.h"

#include <math.h>

/* Extra margin added when pushing the ball clear of a paddle. Placing the ball
 * center exactly at face +/- radius is a tangent position, which raylib still
 * classifies as colliding (with vertical overlap), risking a re-hit and
 * speedFactor creep on the next frame. Clear by epsilon past tangency instead. */
static const float PADDLE_SEPARATION_EPSILON = 0.5f;

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
        .lateralVelocity = 0.0f,
        .sizeMultiplier = 1.0f,
        .unpredictableBounce = false,
        .lagTimer = 0.0f,
        .randomEffects = false
    };

    /* The afterimage ring starts empty (no live ghosts) on every (re)init. */
    TrailHistoryInit(&ball->trail);
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
    // Curve Spell / Arcane Chaos accumulate in lateralVelocity while the
    // effect is active, and decay back to 0 when it ends. The bend never
    // touches baseVelocity, so a timed effect can't leave permanent state
    // behind and the base stays authoritative for wall bounces. This runs
    // before the effective velocity is derived so the bend shows this frame.
    if (EffectsHas(fx, TARGET_BALL, -1, STAT_BALL_CURVE)) {
        ball->lateralVelocity += EffectsApply(fx, TARGET_BALL, -1, STAT_BALL_CURVE, 0.0f) * deltaTime;
    } else if (ball->lateralVelocity != 0.0f) {
        // ...and decay back to 0 when the effect ends, so the bend is
        // self-restoring and never writes a permanent state into base.
        float decay = CURVE_DECAY_RATE * deltaTime;
        if (ball->lateralVelocity > 0.0f) ball->lateralVelocity -= fminf(ball->lateralVelocity, decay);
        else if (ball->lateralVelocity < 0.0f) ball->lateralVelocity += fminf(-ball->lateralVelocity, decay);
    }

    // Derive the effective velocity from base + active effects.
    ApplyBallEffects(ball, fx);

    // Update position
    ball->position.x += ball->velocity.x * deltaTime;
    ball->position.y += ball->velocity.y * deltaTime;

    // Wall collision (mutate the base; effective velocity is re-derived below).
    // Clamp the position to the wall so the ball can never sink into or tunnel
    // through it, and only reflect when actually travelling into the wall.
    if(ball->position.y - ball->radius <= 0) {
        if (ball->baseVelocity.y < 0) ball->baseVelocity.y *= -1;
        ball->position.y = ball->radius;
    } else if(ball->position.y + ball->radius >= screenHeight) {
        if (ball->baseVelocity.y > 0) ball->baseVelocity.y *= -1;
        ball->position.y = screenHeight - ball->radius;
    }

    if(CheckCollisionCircleRec(ball->position, ball->radius, (Rectangle){paddle1->position.x, paddle1->position.y, paddle1->size.x, paddle1->size.y})) {
        // Angle-based bounce
        float hitPos = (ball->position.y - paddle1->position.y) / paddle1->size.y;  // 0 (top) to 1 (bottom)
        float angle = (hitPos - 0.5f) * PI / 2;  // Max 90 degrees up/down
        // Defender Shield Block: consume the struck paddle's armed shield and
        // reflect at a fixed base-speed multiplier. A shielded hit must not
        // also apply the regular speedFactor creep.
        bool shielded = paddle1->shieldTimer > 0.0f;
        if (shielded) paddle1->shieldTimer = 0.0f;
        float baseSpeed = sqrt(ball->baseVelocity.x * ball->baseVelocity.x + ball->baseVelocity.y * ball->baseVelocity.y);
        float speed = shielded ? baseSpeed * DEFENDER_SHIELD_REFLECT_MULT : baseSpeed * ball->speedFactor;  // derive from base so effects apply exactly once
        float dirX = (paddle1->position.x < screenWidth / 2) ? 1.0f : -1.0f;
        ball->baseVelocity.x = speed * cos(angle) * dirX;
        ball->baseVelocity.y = speed * sin(angle);
        ball->lateralVelocity = 0.0f; /* a paddle hit resets the curve bend */
        if (!shielded) {
            ball->speedFactor *= 1.005f;
            if (ball->speedFactor > 2.0f) ball->speedFactor = 2.0f;
        }
        // Push the ball clear of the paddle on the outward face (the side is
        // derived from the paddle's screen half) so one contact produces exactly
        // one bounce and one speedFactor step.
        ball->position.x = (dirX > 0.0f)
            ? paddle1->position.x + paddle1->size.x + ball->radius + PADDLE_SEPARATION_EPSILON
            : paddle1->position.x - ball->radius - PADDLE_SEPARATION_EPSILON;
        /* Overdrive Clutch: returning the ball off the outer 25% of a Speedster
         * paddle spikes heat to full for a short window (a direct SET, so it
         * replaces any live Flow ADD on the same slot) and launches a ball
         * afterimage. Center returns stay calm. */
        if (paddle1->charType == SPEEDSTER
            && (hitPos < OVERDRIVE_CLUTCH_EDGE || hitPos > 1.0f - OVERDRIVE_CLUTCH_EDGE)) {
            EffectsPush(fx, (Effect){ .active=true, .target=TARGET_PADDLE, .paddleIndex=0,
                .stat=STAT_PADDLE_HEAT, .op=OP_SET, .value=OVERDRIVE_CLUTCH_HEAT,
                .timeLeft=OVERDRIVE_CLUTCH_DURATION, .duration=OVERDRIVE_CLUTCH_DURATION,
                .color=characters[SPEEDSTER].accent, .source=0 });
            EffectsPush(fx, (Effect){ .active=true, .target=TARGET_BALL, .paddleIndex=-1,
                .stat=STAT_TRAIL, .op=OP_SET, .value=1.0f,
                .timeLeft=OVERDRIVE_CLUTCH_TRAIL, .duration=OVERDRIVE_CLUTCH_TRAIL,
                .color=characters[SPEEDSTER].accent, .source=0 });
        }
    }
    if(CheckCollisionCircleRec(ball->position, ball->radius, (Rectangle){paddle2->position.x, paddle2->position.y, paddle2->size.x, paddle2->size.y})) {
        float hitPos = (ball->position.y - paddle2->position.y) / paddle2->size.y;
        float angle = (hitPos - 0.5f) * PI / 2;
        // Defender Shield Block: consume the struck paddle's armed shield and
        // reflect at a fixed base-speed multiplier (no speedFactor creep).
        bool shielded = paddle2->shieldTimer > 0.0f;
        if (shielded) paddle2->shieldTimer = 0.0f;
        float baseSpeed = sqrt(ball->baseVelocity.x * ball->baseVelocity.x + ball->baseVelocity.y * ball->baseVelocity.y);
        float speed = shielded ? baseSpeed * DEFENDER_SHIELD_REFLECT_MULT : baseSpeed * ball->speedFactor;  // derive from base so effects apply exactly once
        float dirX = (paddle2->position.x < screenWidth / 2) ? 1.0f : -1.0f;
        ball->baseVelocity.x = speed * cos(angle) * dirX;
        ball->baseVelocity.y = speed * sin(angle);
        ball->lateralVelocity = 0.0f; /* a paddle hit resets the curve bend */
        if (!shielded) {
            ball->speedFactor *= 1.005f;
            if (ball->speedFactor > 2.0f) ball->speedFactor = 2.0f;
        }
        // Push the ball clear of the paddle on the outward face (the side is
        // derived from the paddle's screen half) so one contact produces exactly
        // one bounce and one speedFactor step.
        ball->position.x = (dirX > 0.0f)
            ? paddle2->position.x + paddle2->size.x + ball->radius + PADDLE_SEPARATION_EPSILON
            : paddle2->position.x - ball->radius - PADDLE_SEPARATION_EPSILON;
        /* Overdrive Clutch (right side, index 1): mirrors the paddle1 block so
         * both paddles' heat slots and shared ball trail key off their own
         * paddle index. */
        if (paddle2->charType == SPEEDSTER
            && (hitPos < OVERDRIVE_CLUTCH_EDGE || hitPos > 1.0f - OVERDRIVE_CLUTCH_EDGE)) {
            EffectsPush(fx, (Effect){ .active=true, .target=TARGET_PADDLE, .paddleIndex=1,
                .stat=STAT_PADDLE_HEAT, .op=OP_SET, .value=OVERDRIVE_CLUTCH_HEAT,
                .timeLeft=OVERDRIVE_CLUTCH_DURATION, .duration=OVERDRIVE_CLUTCH_DURATION,
                .color=characters[SPEEDSTER].accent, .source=1 });
            EffectsPush(fx, (Effect){ .active=true, .target=TARGET_BALL, .paddleIndex=-1,
                .stat=STAT_TRAIL, .op=OP_SET, .value=1.0f,
                .timeLeft=OVERDRIVE_CLUTCH_TRAIL, .duration=OVERDRIVE_CLUTCH_TRAIL,
                .color=characters[SPEEDSTER].accent, .source=1 });
        }
    }

    // Reflect any base-velocity mutation back into the effective velocity so
    // callers (and the next frame) observe the post-collision direction now.
    ApplyBallEffects(ball, fx);

    // Lateral bend rides on the effective velocity only; base stays authoritative.
    ball->velocity.x += ball->lateralVelocity;

    // Cap max speed
    CapVelocity(&ball->velocity, BALL_MAX_VELOCITY);

    // "True afterimage" history: ghosts always age out, and positions are only
    // recorded while the STAT_TRAIL marker is live, so the ring never carries
    // footprints from before the effect started.
    TrailHistoryUpdate(&ball->trail, deltaTime);
    if (EffectsHas(fx, TARGET_BALL, -1, STAT_TRAIL))
        TrailHistoryRecord(&ball->trail, ball->position);
}

void DrawBall(const Ball* ball) {
    /* Trail ghosts are drawn by DrawEffects() from the STAT_TRAIL registry
     * marker (R22) — the old fakeBallActive/illusions clutter is gone. */
    DrawGlowCircle(ball->position, ball->radius * ball->sizeMultiplier, ball->color, 8.0f);
}
