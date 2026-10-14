/* tests/test_defender.c
 * Defender abilities:
 *   0 Shield Block   -> arms a 5s reflect window on the paddle; a struck
 *                       paddle consumes it and reflects the ball at
 *                       DEFENDER_SHIELD_REFLECT_MULT instead of applying the
 *                       usual speedFactor step (R20).
 *   1 Wide Paddle    -> STAT_PADDLE_SIZE MUL 1.8 for 4s (R19: size is derived
 *                       through UpdatePaddle like every other stat).
 *   2 Iron Fortress  -> STAT_PADDLE_SIZE SET FORTRESS_PADDLE_HEIGHT + a
 *                       STAT_PADDLE_IMMOVABLE marker for 5s; UpdatePaddle maps
 *                       that marker onto paddle->immovable each frame (R19).
 */
#include "abilities.h"
#include "effects.h"
#include "ball.h"
#include "paddle.h"
#include "character.h"
#include <stdio.h>
#include <math.h>

static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

int main(void) {
    InitCharacters();

    /* --- Plan test: registry-level behaviour --- */
    Effects fx; EffectsInit(&fx);
    Ball b; InitBall(&b, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,DEFENDER);

    /* Shield Block arms a timed reflect window on the paddle itself. */
    AbilitiesActivate(&fx, &b, &pad, 0, DEFENDER, 0, NULL);
    CHECK(pad.shieldTimer > 0.0f, "shield is armed");
    CHECK(pad.shieldTimer == 5.0f, "shield arms for the full window");

    /* Wide Paddle: STAT_PADDLE_SIZE MUL 1.8, 4s, at the source paddle index. */
    AbilitiesActivate(&fx, &b, &pad, 0, DEFENDER, 1, NULL);
    CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_SIZE, PADDLE_HEIGHT) == PADDLE_HEIGHT*1.8f, "wide paddle x1.8");

    /* Iron Fortress: full-height SET + immovable marker, 5s, at source index. */
    AbilitiesActivate(&fx, &b, &pad, 0, DEFENDER, 2, NULL);
    CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_SIZE, PADDLE_HEIGHT) == FORTRESS_PADDLE_HEIGHT, "iron fortress full height");
    CHECK(EffectsHas(&fx, TARGET_PADDLE, 0, STAT_PADDLE_IMMOVABLE), "iron fortress marks the paddle immovable");

    /* --- R20: shield consume + powered reflect through UpdateBall ---
     * Ball starts clearly overlapping the Defender paddle at x=50 and moves
     * toward it (velocity.x < 0), so the first step is a shielded contact. */
    Effects cfx; EffectsInit(&cfx);
    Ball cb; InitBall(&cb, 62, 250, BALL_SIZE, -BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle cpad; InitPaddle(&cpad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,DEFENDER);
    Paddle dummy2; InitPaddle(&dummy2, 890, 250, PADDLE_WIDTH, PADDLE_HEIGHT, 300, RED, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,DEFENDER);
    AbilitiesActivate(&cfx, &cb, &cpad, 0, DEFENDER, 0, NULL);
    float pre = sqrtf(cb.baseVelocity.x * cb.baseVelocity.x + cb.baseVelocity.y * cb.baseVelocity.y);
    float preFactor = cb.speedFactor;
    UpdateBall(&cb, &cfx, 1.0f/60.0f, 960, 540, &cpad, &dummy2);
    CHECK(cpad.shieldTimer == 0.0f, "shield consumed on hit");
    CHECK(cb.speedFactor == preFactor, "shielded bounce does not creep speedFactor");
    float post = sqrtf(cb.baseVelocity.x * cb.baseVelocity.x + cb.baseVelocity.y * cb.baseVelocity.y);
    CHECK(fabsf(post - pre * DEFENDER_SHIELD_REFLECT_MULT) < 0.01f, "shielded reflect is base speed x DEFENDER_SHIELD_REFLECT_MULT");

    /* --- R19/R20: Wide Paddle derives size through UpdatePaddle --- */
    Effects wfx; EffectsInit(&wfx);
    Ball wb; InitBall(&wb, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle wpad; InitPaddle(&wpad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,DEFENDER);
    AbilitiesActivate(&wfx, &wb, &wpad, 0, DEFENDER, 1, NULL);
    UpdatePaddle(&wpad, &wfx, 0, 1.0f/60.0f, 540, &wb, NULL);
    CHECK(wpad.size.y == PADDLE_HEIGHT * 1.8f, "wide paddle stretches through UpdatePaddle");
    EffectsUpdate(&wfx, 4.5f);
    UpdatePaddle(&wpad, &wfx, 0, 1.0f/60.0f, 540, &wb, NULL);
    CHECK(wpad.size.y == PADDLE_HEIGHT, "wide paddle expires back to normal height");

    /* --- R19: Iron Fortress drives immovable + full height via UpdatePaddle --- */
    Effects ffx; EffectsInit(&ffx);
    Ball fb; InitBall(&fb, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle fpad; InitPaddle(&fpad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,DEFENDER);
    AbilitiesActivate(&ffx, &fb, &fpad, 0, DEFENDER, 2, NULL);
    UpdatePaddle(&fpad, &ffx, 0, 1.0f/60.0f, 540, &fb, NULL);
    CHECK(fpad.immovable == true, "iron fortress locks the paddle still");
    CHECK(fpad.size.y == FORTRESS_PADDLE_HEIGHT, "iron fortress stretches to full height");
    EffectsUpdate(&ffx, 5.5f);
    UpdatePaddle(&fpad, &ffx, 0, 1.0f/60.0f, 540, &fb, NULL);
    CHECK(fpad.immovable == false, "iron fortress releases the paddle");
    CHECK(fpad.size.y == PADDLE_HEIGHT, "iron fortress paddle returns to normal height");

    /* --- R20: the armed reflect window decays in UpdatePaddle, floor 0 --- */
    Effects sfx; EffectsInit(&sfx);
    Ball sb; InitBall(&sb, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle spad; InitPaddle(&spad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,DEFENDER);
    AbilitiesActivate(&sfx, &sb, &spad, 0, DEFENDER, 0, NULL);
    UpdatePaddle(&spad, &sfx, 0, 1.0f/60.0f, 540, &sb, NULL);
    CHECK(spad.shieldTimer < 5.0f, "shield window decays over time");
    for (int i = 0; i < 320; i++) UpdatePaddle(&spad, &sfx, 0, 1.0f/60.0f, 540, &sb, NULL);
    CHECK(spad.shieldTimer == 0.0f, "shield window expires at zero");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}