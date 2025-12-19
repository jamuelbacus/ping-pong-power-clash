/*
 * paddle.h
 * defines the Paddle struct and declares functions to initialize, update, and draw the paddle.
*/

#ifndef PADDLE_H
#define PADDLE_H
#include <stdbool.h>
#include "raylib.h"
#include "character.h"

static const Vector2 PLAYER1_PADDLE_POSITION = {50, 200};
static const Vector2 PLAYER2_PADDLE_POSITION = {890, 200};

static const float PADDLE_WIDTH = 20;
static const float PADDLE_HEIGHT = 100;

typedef struct Ball Ball;  // Forward declaration

typedef struct Paddle {
    Vector2 position;
    Vector2 size;
    float speed;
    Color color;
    bool isCpu;
    float velocityY;
    float accel;
    float maxSpeed;
    float friction;
    int upKey;
    int downKey;
    CharacterType charType;
    float skill1Cooldown;
    float skill2Cooldown;
    float ultimateCooldown;
    float skill1Timer;
    float skill2Timer;
    float ultimateTimer;
    bool skill1Active;
    bool skill2Active;
    bool ultimateActive;
    float speedMultiplier;
    float sizeMultiplier;
    bool immovable;
    // For Trickster mirror
    bool mirrorField;
    bool heavyPaddle;
    bool smashShotReady;
    bool shieldBlock;
    bool piercingShot;
    Vector2 originalSize;
    float originalMaxSpeed;
    float widthMultiplier;
    bool deadeye;
} Paddle;

void InitPaddle(Paddle* paddle, float x, float y, float width, float height, float speed, Color color, bool isCpu, float accel, float maxSpeed, float friction, int upKey, int downKey, CharacterType charType);
void UpdatePaddle(Paddle* paddle, float deltaTime, int screenHeight, Ball* ball);
void DrawPaddle(Paddle paddle);

#endif // PADDLE_H