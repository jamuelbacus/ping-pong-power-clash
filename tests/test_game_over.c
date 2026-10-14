/* tests/test_game_over.c */
#include "../src/game.c"
#include <stdio.h>
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

int main(void) {
    Game g;
    InitBall(&g.ball, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    InitPaddle(&g.paddle, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);
    InitPaddle(&g.player2Paddle, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, false, 4000,320,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
    EffectsInit(&g.effects);
    g.playerScore = g.player2Score = 0;
    g.gameState = PLAY_CPU;

    g.playerScore = WINNING_SCORE;
    UpdateGame(&g, 1.0f/60.0f);                 /* detects win */
    CHECK(g.gameState == GAME_OVER, "reaching 7 enters GAME_OVER");
    CHECK(GameWinner(&g) == 1, "player 1 reported as winner");

    /* frozen: ball does not move while GAME_OVER */
    Vector2 before = g.ball.position;
    UpdateGame(&g, 1.0f/60.0f);
    CHECK(g.ball.position.x == before.x && g.ball.position.y == before.y, "simulation frozen at GAME_OVER");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
