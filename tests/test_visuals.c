/* tests/test_visuals.c
 * Headless tests for the pure visual-state queries in visuals.c (R22):
 * these drive ball tint, paddle aura, dash trail, and the HUD "active" dot,
 * so they must agree with the Effects registry bit-for-bit.
 *
 * Build & run (from the project root):
 *   gcc tests/test_visuals.c src/visuals.c src/abilities.c src/effects.c \
 *       src/ball.c src/paddle.c src/character.c src/utils.c src/theme.c \
 *       src/trail.c src/decoy.c \
 *       -Iinclude $(pkg-config --cflags --libs raylib) -lm -o /tmp/test_visuals
 *   /tmp/test_visuals
 */
#include "visuals.h"
#include "abilities.h"
#include "effects.h"
#include "ball.h"
#include "paddle.h"
#include "character.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

static bool ColorEq(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

int main(void) {
    InitCharacters();

    Effects fx; EffectsInit(&fx);
    Ball b; InitBall(&b, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,GRAND_WIZARD);

    /* Empty registry: no ball tint and no player has an active effect. */
    CHECK(EffectTintColor(&fx, TARGET_BALL, -1).a == 0, "empty registry ball tint is BLANK");
    CHECK(!PlayerHasActiveEffect(&fx, 0), "no active effects for player 0");

    /* Time Slow pushes a ball-keyed STAT_BALL_SPEED owned by source 0. */
    AbilitiesActivate(&fx, &b, &pad, 0, GRAND_WIZARD, 1);
    CHECK(ColorEq(EffectTintColor(&fx, TARGET_BALL, -1), characters[GRAND_WIZARD].accent),
          "ball tint is the wizard accent");
    CHECK(PlayerHasActiveEffect(&fx, 0), "ball effect counted for its source (0)");
    CHECK(!PlayerHasActiveEffect(&fx, 1), "ball effect NOT counted for the other player");
    EffectsUpdate(&fx, 3.0f);
    CHECK(EffectTintColor(&fx, TARGET_BALL, -1).a == 0, "expired ball tint is BLANK again");
    CHECK(!PlayerHasActiveEffect(&fx, 0), "expired effect hands back the active dot");

    /* Dash Return pushes a paddle-keyed STAT_TRAIL (0.3s). */
    Effects fx2; EffectsInit(&fx2);
    AbilitiesActivate(&fx2, &b, &pad, 0, SPEEDSTER, 0);
    CHECK(EffectTrailActive(&fx2, TARGET_PADDLE, 0), "dash trail active on paddle 0");
    CHECK(!EffectTrailActive(&fx2, TARGET_PADDLE, 1), "dash trail belongs to paddle 0 only");
    EffectsUpdate(&fx2, 0.5f);
    CHECK(!EffectTrailActive(&fx2, TARGET_PADDLE, 0), "dash trail expires");

    /* Defender Shield Block arms the paddle's timed reflect window. */
    Effects fx3; EffectsInit(&fx3);
    AbilitiesActivate(&fx3, &b, &pad, 0, DEFENDER, 0);
    CHECK(ShieldActive(&pad), "shield window armed");
    pad.shieldTimer = 1.0f;
    UpdatePaddle(&pad, &fx3, 0, 1.5f, 540, &b);   /* real decay path, clamped to 0 */
    CHECK(!ShieldActive(&pad), "shield window expired/consumed");

    /* Speedster Afterburner arms a timed charge window on the paddle. */
    Effects fx4b; EffectsInit(&fx4b);
    Paddle apad; InitPaddle(&apad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,SPEEDSTER);
    AbilitiesActivate(&fx4b, &b, &apad, 0, SPEEDSTER, 1);
    CHECK(AfterburnerArmed(&apad), "afterburner window armed");
    apad.afterburnerTimer = 1.0f;
    UpdatePaddle(&apad, &fx4b, 0, 1.5f, 540, &b);   /* real decay path, clamped to 0 */
    CHECK(!AfterburnerArmed(&apad), "afterburner window expired/consumed");

    /* Iron Fortress tints the owning paddle with the defender accent. */
    Effects fx4; EffectsInit(&fx4);
    AbilitiesActivate(&fx4, &b, &pad, 0, DEFENDER, 2);
    CHECK(ColorEq(EffectTintColor(&fx4, TARGET_PADDLE, 0), characters[DEFENDER].accent),
          "fortress paddle tint is defender accent");
    CHECK(EffectTintColor(&fx4, TARGET_PADDLE, 1).a == 0, "fortress tint is BLANK for the other paddle");
    CHECK(!PlayerHasActiveEffect(&fx4, 1), "fortress effect not counted for the other player");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}