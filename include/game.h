#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "ball.h"
#include "paddle.h"
#include "character.h"
#include "assets.h"
#include "utils.h"
#include "effects.h"
#include "powerup.h"

static const int WINDOW_WIDTH = 1024;
static const int WINDOW_HEIGHT = 576;
static const char* WINDOW_TITLE = "Ping Pong: Power Clash";

static const int WINNING_SCORE = 7;

typedef enum GameState { MENU, CHARACTER_SELECT, PLAY_CPU, PLAY_PVP, GAME_OVER } GameState;

typedef struct Game {
    Ball ball;
    Paddle paddle;
    Paddle player2Paddle;
    int playerScore;
    int player2Score;
    GameState gameState;
    int currentSelectingPlayer;
    CharacterType selectedChar[2];
    Assets assets;
    Effects effects;
    PowerupPool powerups;
    int playerStreak[2];
    bool streakEffectsActive[2];
    int lastHitterProcessed;
    int lastStreakApplied[2];
} Game;


void InitGame(Game* game);
void UpdateGame(Game* game, float deltaTime);
void DrawGame(const Game* game);
void ResetGame(Game* game);

/* 0 = no winner yet, 1 = player 1, 2 = player 2. */
int GameWinner(const Game* game);

#endif // GAME_H
