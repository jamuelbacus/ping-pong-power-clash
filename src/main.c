/* 
    * main.c
    * entry point for Ping Pong: Power Clash.
    */

    #include "raylib.h"
    #include "ball.h"
    #include "paddle.h"
    #include "character.h"
    #include "game.h"

    int main() 
    {  
        InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
        
        Game game;
        InitGame(&game);

        while (!WindowShouldClose())
        {
            float deltaTime = GetFrameTime();
            UpdateGame(&game, deltaTime);
            
            BeginDrawing();
            ClearBackground(WHITE);
            DrawGame(&game);
            EndDrawing();
        }
        
        UnloadTexture(game.logo);
        CloseWindow();
    }