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
    #include "viewport.h"

    int main() 
    {  
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(1280, 720, WINDOW_TITLE);
        SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);
        SetTargetFPS(60);

        Game game;
        InitGame(&game);

        Viewport viewport;
        InitViewport(&viewport, WINDOW_WIDTH, WINDOW_HEIGHT);

        while (!WindowShouldClose())
        {
            if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

            float deltaTime = GetFrameTime();
            UpdateGame(&game, deltaTime);
            
            BeginViewport(&viewport);
            ClearBackground(THEME_BG);
            DrawGame(&game);
            EndViewport(&viewport);
        }
        
        UnloadViewport(&viewport);
        UnloadAssets(&game.assets);
        CloseWindow();
    }
