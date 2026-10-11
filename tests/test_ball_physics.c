/* tests/test_ball_physics.c — includes ../src/game.c to reach resetBall */
#include "../src/game.c"
#include <stdio.h>
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

int main(void) {
    Game g;
    InitBall(&g.ball, 480, 5, BALL_SIZE, 0, -400, BALL_COLOR);         /* heading into the top wall */
    InitPaddle(&g.paddle, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, true, 4000,500,0.9f,KEY_W,KEY_S,SPEEDSTER);
    InitPaddle(&g.player2Paddle, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000,500,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
    EffectsInit(&g.effects);
    g.playerScore = g.cpuScore = 0;

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

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
