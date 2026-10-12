/* tests/test_ball_physics.c — includes ../src/game.c to reach resetBall */
#include "../src/game.c"
#include "trail.h"
#include "effects.h"
#include <stdio.h>
#include <math.h>
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

int main(void) {
    Game g;
    InitBall(&g.ball, 480, 5, BALL_SIZE, 0, -400, BALL_COLOR);         /* heading into the top wall */
    InitPaddle(&g.paddle, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, true, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);
    InitPaddle(&g.player2Paddle, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
    EffectsInit(&g.effects);
    g.playerScore = g.player2Score = 0;

    UpdateBall(&g.ball, &g.effects, 1.0f/60.0f, 960, 540, &g.paddle, &g.player2Paddle);
    CHECK(g.ball.position.y >= g.ball.radius, "ball clamped inside the top wall");
    CHECK(g.ball.velocity.y > 0, "ball bounced off the top wall");

    /* resetBall clears effects and restores base speed exactly */
    EffectsPush(&g.effects, (Effect){ .active=true, .target=TARGET_BALL, .paddleIndex=-1,
        .stat=STAT_BALL_SPEED, .op=OP_MUL, .value=0.5f, .timeLeft=5, .duration=5,
        .color=WHITE, .source=0 });
    g.ball.position.x = -50.0f;
    resetBall(&g);
    CHECK(!EffectsHas(&g.effects, TARGET_BALL, -1, STAT_BALL_SPEED), "resetBall clears effects");
    CHECK(g.ball.position.x == BALL_POSITION.x, "ball recentered");

    /* timed effects must expire through the game loop tick */
    EffectsPush(&g.effects, (Effect){ .active=true, .target=TARGET_BALL, .paddleIndex=-1,
        .stat=STAT_BALL_SPEED, .op=OP_MUL, .value=0.5f, .timeLeft=0.01f, .duration=0.01f,
        .color=WHITE, .source=1 });
    g.ball.position = BALL_POSITION;
    g.ball.baseVelocity = (Vector2){0.0f, 0.0f};   /* stays put so no point is scored */
    g.ball.velocity = (Vector2){0.0f, 0.0f};
    updateGamePlay(&g, 1.0f/60.0f);
    CHECK(!EffectsHas(&g.effects, TARGET_BALL, -1, STAT_BALL_SPEED), "effects expire through updateGamePlay");

    /* R-trail records while live and resetBall wipes it (no cross-point ghosts). */
    {
        EffectsPush(&g.effects, (Effect){ .active=true, .target=TARGET_BALL, .paddleIndex=-1,
            .stat=STAT_TRAIL, .op=OP_SET, .value=1.0f, .timeLeft=5.0f, .duration=5.0f,
            .color=BALL_COLOR, .source=0 });
        for (int i = 0; i < 6; i++) updateGamePlay(&g, 1.0f/60.0f);
        TrailGhost ghosts[TRAIL_MAX_GHOSTS];
        CHECK(TrailActiveGhosts(&g.ball.trail, TRAIL_MAX_GHOSTS, ghosts) > 0,
              "ball afterimage records ghost positions while its marker is live");
        resetBall(&g);
        CHECK(TrailActiveGhosts(&g.ball.trail, TRAIL_MAX_GHOSTS, ghosts) == 0,
              "resetBall clears the ball afterimage");
    }

    /* paddle separation: one contact == exactly one bounce / speedFactor step.
     * dt = 0 keeps the ball at its cleared position so a tangency-only "clear"
     * would be re-detected as a collision on the second call. */
    Ball sep;
    InitBall(&sep, 75.0f, 250.0f, BALL_SIZE, -300.0f, 0.0f, BALL_COLOR);
    Paddle lp, rp;
    InitPaddle(&lp, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, true, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);
    InitPaddle(&rp, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
    EffectsInit(&g.effects);
    float f0 = sep.speedFactor;
    UpdateBall(&sep, &g.effects, 0.0f, 960, 540, &lp, &rp);
    UpdateBall(&sep, &g.effects, 0.0f, 960, 540, &lp, &rp);
    CHECK(fabsf(sep.speedFactor - f0 * 1.005f) < 1e-6f,
          "one paddle contact advances speedFactor exactly one step");
    CHECK(sep.position.x > lp.position.x + lp.size.x + sep.radius,
          "cleared ball is strictly past the paddle face, not tangent");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
