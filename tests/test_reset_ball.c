/*
 * test_reset_ball.c
 * Headless regression test for the scoring bug: a ball that leaves the
 * screen must score exactly ONE point, then return to the play area.
 *
 * It #includes ../src/game.c directly so it can exercise the file-static
 * updateGamePlay()/resetBall() without a window.
 *
 * Build & run (from the project root):
 *   gcc tests/test_reset_ball.c src/theme.c src/viewport.c src/assets.c \
 *       src/ball.c src/paddle.c src/character.c src/effects.c \
 *       -Iinclude $(pkg-config --cflags --libs raylib) -lm -o tests/test_reset_ball
 *   ./tests/test_reset_ball
 */
#include "../src/game.c"
#include "effects.h"

#include <stdio.h>

int main(void) {
    Game game;

    /* Ball already past the left edge, moving further left -> player 1 conceded. */
    InitBall(&game.ball, -60.0f, WINDOW_HEIGHT / 2.0f, BALL_SIZE,
             -BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);

    /* CPU-controlled paddles so the test never touches real input devices. */
    InitPaddle(&game.paddle, PLAYER1_PADDLE_POSITION.x, PLAYER1_PADDLE_POSITION.y,
               PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, true, 4000.0f, 500.0f, 0.9f,
               KEY_W, KEY_S, SPEEDSTER);
    InitPaddle(&game.player2Paddle, PLAYER2_PADDLE_POSITION.x, PLAYER2_PADDLE_POSITION.y,
               PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000.0f, 500.0f, 0.9f,
               KEY_UP, KEY_DOWN, SPEEDSTER);

    game.playerScore = 0;
    game.player2Score = 0;
    EffectsInit(&game.effects);

    /* Simulate one second (60 frames). A single miss must not award 60 points. */
    for (int i = 0; i < 60; i++)
        updateGamePlay(&game, 1.0f / 60.0f);

    int failed = 0;
    if (game.player2Score != 1) {
        printf("FAIL: player2Score = %d (expected exactly 1)\n", game.player2Score);
        failed = 1;
    }
    if (game.playerScore != 0) {
        printf("FAIL: playerScore = %d (expected 0)\n", game.playerScore);
        failed = 1;
    }
    if (!(game.ball.position.x > 0.0f && game.ball.position.x < WINDOW_WIDTH)) {
        printf("FAIL: ball not returned to the play area (x = %.1f)\n", game.ball.position.x);
        failed = 1;
    }
    if (game.ball.velocity.x >= 0.0f) {
        printf("FAIL: ball velocity not reset toward the conceding side (vx = %.1f)\n",
               game.ball.velocity.x);
        failed = 1;
    }

    if (failed) {
        printf("RESULT: FAIL\n");
        return 1;
    }
    printf("RESULT: PASS (one point for the miss; ball reset to the play area)\n");
    return 0;
}
