#include "game.h"

void InitGame(Game* game) {
    InitCharacters();
    game->logo = LoadTexture("pingpongpowerclashlogo.png");
    InitBall(&game->ball, BALL_POSITION.x, BALL_POSITION.y, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    InitPaddle(&game->paddle, PLAYER1_PADDLE_POSITION.x, PLAYER1_PADDLE_POSITION.y, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000.0f, 500.0f, 0.9f, KEY_W, KEY_S, SPEEDSTER);
    InitPaddle(&game->player2Paddle, PLAYER2_PADDLE_POSITION.x, PLAYER2_PADDLE_POSITION.y, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, false, 4000.0f, 500.0f, 0.9f, KEY_UP, KEY_DOWN, SPEEDSTER);
    game->playerScore = 0;
    game->cpuScore = 0;
    game->gameState = MENU;
    game->currentSelectingPlayer = 0;
    game->selectedChar[0] = SPEEDSTER;
    game->selectedChar[1] = SPEEDSTER;
}

static void resetBall(Game* game) {
    float velX = (game->ball.velocity.x > 0) ? BALL_BASE_VELOCITY_X : -BALL_BASE_VELOCITY_X;
}

static void updateGamePlay(Game* game, float deltaTime) {
    UpdateBall(&game->ball, deltaTime, WINDOW_WIDTH, WINDOW_HEIGHT, &game->paddle, &game->player2Paddle);
    
    if (game->ball.position.x < 0) {
        game->cpuScore++;
        resetBall(game);
    } else if (game->ball.position.x > WINDOW_WIDTH) {
        game->playerScore++;
        resetBall(game);
    }
    
    UpdatePaddle(&game->paddle, deltaTime, WINDOW_HEIGHT, &game->ball);
    UpdatePaddle(&game->player2Paddle, deltaTime, WINDOW_HEIGHT, &game->ball);
}

static void DrawMenu(const Game* game) {
    // Draw logo centered at top, scaled to fit
    if (game->logo.width > 0) {
        float scale = 250.0f / game->logo.width;
        DrawTextureEx(game->logo, (Vector2){(WINDOW_WIDTH - (game->logo.width * scale)) / 2, 30}, 0, scale, WHITE);
    }

    DrawRectangle(350, 250, 260, 50, GRAY);
    DrawText("Player vs CPU", 370, 260, 30, BLACK);
    DrawRectangle(350, 330, 260, 50, GRAY);
    DrawText("Player vs Player", 370, 340, 30, BLACK);
}

static void DrawCharacterSelect(const Game* game) {
    DrawText("Select Character", 350, 50, 30, BLACK);
    DrawText(game->currentSelectingPlayer == 0 ? "Player 1" : "Player 2", 400, 20, 25, BLACK);
    for (int i = 0; i < 8; i++) {
        int x = (i % 4) * 200 + 50;
        int y = (i / 4) * 100 + 100;
        DrawRectangle(x, y, 180, 80, GRAY);
        DrawText(characters[i].name, x + 10, y + 10, 20, BLACK);
        DrawText(characters[i].theme, x + 10, y + 30, 15, DARKGRAY);
    }
}

static void DrawGameplay(const Game* game) {
    DrawBall(&game->ball);
    DrawPaddle(game->paddle);
    DrawPaddle(game->player2Paddle);

    // Draw scores
    DrawText(TextFormat("%d", game->playerScore), WINDOW_WIDTH / 4, 20, 40, BLACK);
    DrawText(TextFormat("%d", game->cpuScore), 3 * WINDOW_WIDTH / 4, 20, 40, BLACK);
}

void UpdateGame(Game* game, float deltaTime) {
    if (game->gameState == PLAY_CPU || game->gameState == PLAY_PVP) {
        updateGamePlay(game, deltaTime);
    } else if (game->gameState == MENU) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            if (CheckCollisionPointRec(mousePos, (Rectangle){350,250,260,50})) {
                game->gameState = CHARACTER_SELECT;
                game->player2Paddle.isCpu = true;
            } else if (CheckCollisionPointRec(mousePos, (Rectangle){350,330,260,50})) {
                game->gameState = CHARACTER_SELECT;
                game->player2Paddle.isCpu = false;
            }
        }
    } else if (game->gameState == CHARACTER_SELECT) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            for (int i = 0; i < 8; i++) {
                int x = (i % 4) * 200 + 50;
                int y = (i / 4) * 100 + 100;
                if (CheckCollisionPointRec(mousePos, (Rectangle){x, y, 180, 80})) {
                    game->selectedChar[game->currentSelectingPlayer] = i;
                    if (game->player2Paddle.isCpu) { // CPU mode
                        game->selectedChar[1] = GetRandomValue(0, 7);
                        game->player2Paddle.charType = game->selectedChar[1];
                        game->gameState = PLAY_CPU;
                    } else { // PVP
                        if (game->currentSelectingPlayer == 0) {
                            game->currentSelectingPlayer = 1;
                        } else {
                            game->gameState = PLAY_PVP;
                        }
                    }
                    game->paddle.charType = game->selectedChar[0];
                    game->player2Paddle.charType = game->selectedChar[1];
                    break;
                }
            }
        }
    }
}

void DrawGame(const Game* game) {
    if (game->gameState == MENU) {
        DrawMenu(game);
    } else if (game->gameState == CHARACTER_SELECT) {
        DrawCharacterSelect(game);
    } else if (game->gameState == PLAY_CPU || game->gameState == PLAY_PVP) {
        if (game->playerScore >= WINNING_SCORE) {
            DrawText("You Win!", 400, 250, 40, BLUE);
        } else if (game->cpuScore >= WINNING_SCORE) {
            DrawText("CPU Wins!", 400, 250, 40, RED);
        } else {
            DrawGameplay(game);
            DrawText(characters[game->paddle.charType].name, 10, 50, 20, BLUE);
            DrawText(characters[game->player2Paddle.charType].name, 800, 50, 20, GREEN);
            DrawLine(480, 0, 480, 540, BLACK);  // Center line
            DrawText(TextFormat("Player: %d", game->playerScore), 10, 10, 20, BLUE);
            DrawText(TextFormat("%s: %d", game->player2Paddle.isCpu ? "CPU" : "Player 2", game->cpuScore), 800, 10, 20, RED);
            // Skill UI for Player 1
            DrawRectangle(10, 450, 200, 80, Fade(BLUE, 0.5f));
            DrawText("Player 1 Skills", 15, 455, 18, WHITE);
            DrawText(TextFormat("Q: %s (%.1fs)", characters[game->paddle.charType].skills[0], game->paddle.skill1Cooldown), 15, 475, 14, WHITE);
            DrawText(TextFormat("E: %s (%.1fs)", characters[game->paddle.charType].skills[1], game->paddle.skill2Cooldown), 15, 490, 14, WHITE);
            DrawText(TextFormat("R: %s (%.1fs)", characters[game->paddle.charType].skills[2], game->paddle.ultimateCooldown), 15, 505, 14, WHITE);
            // Skill UI for Player 2
            DrawRectangle(750, 450, 200, 80, Fade(GREEN, 0.5f));
            DrawText("Player 2 Skills", 755, 455, 18, WHITE);
            DrawText(TextFormat("1: %s (%.1fs)", characters[game->player2Paddle.charType].skills[0], game->player2Paddle.skill1Cooldown), 755, 475, 14, WHITE);
            DrawText(TextFormat("2: %s (%.1fs)", characters[game->player2Paddle.charType].skills[1], game->player2Paddle.skill2Cooldown), 755, 490, 14, WHITE);
            DrawText(TextFormat("3: %s (%.1fs)", characters[game->player2Paddle.charType].skills[2], game->player2Paddle.ultimateCooldown), 755, 505, 14, WHITE);
        }
    }
}

void ResetGame(Game* game) {
    game->playerScore = 0;
    game->cpuScore = 0;
    resetBall(game);
    game->gameState = MENU;
    game->currentSelectingPlayer = 0;
}