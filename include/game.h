#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "ball.h"
#include "paddle.h"
#include "character.h"
#include "utils.h"

static const int WINDOW_WIDTH = 960;
static const int WINDOW_HEIGHT = 540;
static const char* WINDOW_TITLE = "Ping Pong: Power Clash";

static const int WINNING_SCORE = 5;

typedef enum GameState { MENU, CHARACTER_SELECT, PLAY_CPU, PLAY_PVP } GameState;

typedef struct Game {
    Ball ball;
    Paddle paddle;
    Paddle player2Paddle;
    int playerScore;
    int cpuScore;
    GameState gameState;
    int currentSelectingPlayer;
    CharacterType selectedChar[2];
    Texture2D logo;
} Game;


void InitGame(Game* game);
void UpdateGame(Game* game, float deltaTime);
void DrawGame(const Game* game);
void ResetGame(Game* game);

#endif // GAME_H
