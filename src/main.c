/* 
    * main.c
    * entry point for Ping Pong: Power Clash.
    */

    #include "raylib.h"
    #include "ball.h"
    #include "paddle.h"
    #include "character.h"
    #include "game.h"
    #include "theme.h"

    int main() 
    {  
        InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
        SetTargetFPS(60);

        Game game;
        InitGame(&game);

        while (!WindowShouldClose())
        {
            float deltaTime = GetFrameTime();
            UpdateGame(&game, deltaTime);
            
            BeginDrawing();
            ClearBackground(THEME_BG);
            DrawGame(&game);
            EndDrawing();
        }
        
        UnloadAssets(&game.assets);
        CloseWindow();
    }
