#include "game.h"
#include "theme.h"
#include "layout.h"
#include "viewport.h"

/* ---------------------------------------------------------------- helpers */

static void DrawTextCentered(const char* text, float centerX, int y, int fontSize, Color color, bool glow) {
    int x = (int)(centerX - MeasureText(text, fontSize) / 2.0f);
    if (glow) DrawGlowText(text, x, y, fontSize, color);
    else DrawText(text, x, y, fontSize, color);
}

static void DrawMenuButton(Rectangle rec, const char* label, Color accent, bool hovered) {
    Color fill = hovered ? THEME_PANEL_HI : THEME_PANEL;
    DrawGlowRect(rec, fill, hovered ? 10.0f : 5.0f);
    DrawGlowRectLines(rec, 2.0f, accent, hovered ? 10.0f : 5.0f);
    DrawTextCentered(label, rec.x + rec.width / 2.0f, (int)(rec.y + (rec.height - 24) / 2.0f), 24, THEME_TEXT, true);
}

/* One skill row: "Q: Dash Return (2.3s)" + a cooldown progress bar. */
static void DrawSkillRow(int x, int y, int width, const char* key, const char* name,
                         float cooldown, float maxCooldown, Color accent) {
    const char* label = (cooldown > 0.05f)
        ? TextFormat("%s: %s (%.1fs)", key, name, cooldown)
        : TextFormat("%s: %s", key, name);
    DrawText(label, x, y, 13, THEME_TEXT);

    float t = (cooldown <= 0.0f) ? 1.0f : 1.0f - (cooldown / maxCooldown);
    DrawRectangle(x, y + 16, width, 3, Fade(THEME_TEXT_DIM, 0.30f));
    DrawRectangle(x, y + 16, (int)(width * t), 3, accent);
}

/* ---------------------------------------------------------------- lifecycle */

void InitGame(Game* game) {
    InitCharacters();
    LoadAssets(&game->assets);
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
    (void)velX; /* NOTE: ball reset is still a no-op; tracked as a known bug. */
}

/* ---------------------------------------------------------------- gameplay */

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

/* ---------------------------------------------------------------- draw */

static void DrawMenu(const Game* game) {
    if (game->assets.logo.loaded) {
        float scale = 250.0f / game->assets.logo.texture.width;
        DrawTextureEx(game->assets.logo.texture,
                      (Vector2){ (WINDOW_WIDTH - (game->assets.logo.texture.width * scale)) / 2, 40 },
                      0, scale, WHITE);
    }

    Vector2 mouse = GetVirtualMouse();
    DrawMenuButton(MENU_BTN_CPU, "Player vs CPU", characters[SPEEDSTER].accent,
                   CheckCollisionPointRec(mouse, MENU_BTN_CPU));
    DrawMenuButton(MENU_BTN_PVP, "Player vs Player", characters[TRICKSTER].accent,
                   CheckCollisionPointRec(mouse, MENU_BTN_PVP));
}

static void DrawCharacterSelect(const Game* game) {
    Vector2 mouse = GetVirtualMouse();

    DrawTextCentered("SELECT CHARACTER", WINDOW_WIDTH / 2.0f, 30, 36, THEME_TEXT, true);
    Color who = (game->currentSelectingPlayer == 0) ? characters[game->selectedChar[0]].accent : THEME_ACCENT;
    DrawTextCentered(game->currentSelectingPlayer == 0 ? "PLAYER 1" : "PLAYER 2",
                     WINDOW_WIDTH / 2.0f, 90, 22, who, true);

    for (int i = 0; i < CHARACTER_COUNT; i++) {
        Rectangle card = CharCellRect(i);
        bool hovered = CheckCollisionPointRec(mouse, card);
        Color accent = characters[i].accent;

        DrawGlowRect(card, hovered ? THEME_PANEL_HI : THEME_PANEL, hovered ? 10.0f : 4.0f);
        DrawGlowRectLines(card, 2.0f, accent, hovered ? 12.0f : 6.0f);

        float iconSize = 52.0f;
        Rectangle iconBox = { card.x + (card.width - iconSize) / 2.0f, card.y + 12.0f, iconSize, iconSize };
        if (game->assets.character[i].loaded) {
            DrawSpriteInBox(game->assets.character[i], iconBox, accent);
        } else {
            DrawGlowCircle((Vector2){ iconBox.x + iconSize / 2.0f, iconBox.y + iconSize / 2.0f },
                           iconSize * 0.35f, accent, 8.0f);
        }

        DrawTextCentered(characters[i].name, card.x + card.width / 2.0f, (int)(card.y + card.height - 44), 16, THEME_TEXT, false);
        DrawTextCentered(characters[i].theme, card.x + card.width / 2.0f, (int)(card.y + card.height - 24), 11, THEME_TEXT_DIM, false);

        /* P1 / P2 ownership tags */
        if ((int)game->selectedChar[0] == i)
            DrawText("P1", (int)card.x + 6, (int)card.y + 6, 12, characters[i].accent);
        if ((int)game->selectedChar[1] == i)
            DrawText("P2", (int)(card.x + card.width - 20), (int)card.y + 6, 12, characters[i].accent);
    }
}

static void DrawGameplay(const Game* game) {
    /* Center line */
    DrawGlowLine((Vector2){ WINDOW_WIDTH / 2.0f, 0 }, (Vector2){ WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT },
                 Fade(THEME_GRID_HI, 0.7f), 5.0f);

    DrawBall(&game->ball);
    DrawPaddle(game->paddle);
    DrawPaddle(game->player2Paddle);

    Color p1 = characters[game->paddle.charType].accent;
    Color p2 = characters[game->player2Paddle.charType].accent;

    /* Scores */
    DrawTextCentered(TextFormat("%d", game->playerScore), WINDOW_WIDTH / 4.0f, 20, 48, p1, true);
    DrawTextCentered(TextFormat("%d", game->cpuScore), 3 * WINDOW_WIDTH / 4.0f, 20, 48, p2, true);

    /* Character names */
    DrawText(characters[game->paddle.charType].name, 10, 50, 18, p1);
    DrawText(characters[game->player2Paddle.charType].name, WINDOW_WIDTH - 10 - MeasureText(characters[game->player2Paddle.charType].name, 18), 50, 18, p2);

    /* HUD panels */
    DrawGlowRect(HUD_P1, THEME_PANEL, 4.0f);
    DrawGlowRectLines(HUD_P1, 1.5f, p1, 5.0f);
    DrawText("SKILLS", (int)HUD_P1.x + 8, (int)HUD_P1.y + 6, 15, p1);

    const Paddle* left = &game->paddle;
    DrawSkillRow((int)HUD_P1.x + 8, (int)HUD_P1.y + 28, (int)HUD_P1.width - 16,
                 "Q", characters[left->charType].skills[0], left->skill1Cooldown, SKILL1_COOLDOWN, p1);
    DrawSkillRow((int)HUD_P1.x + 8, (int)HUD_P1.y + 52, (int)HUD_P1.width - 16,
                 "E", characters[left->charType].skills[1], left->skill2Cooldown, SKILL2_COOLDOWN, p1);
    DrawSkillRow((int)HUD_P1.x + 8, (int)HUD_P1.y + 76, (int)HUD_P1.width - 16,
                 "R", characters[left->charType].skills[2], left->ultimateCooldown, ULTIMATE_COOLDOWN, p1);

    DrawGlowRect(HUD_P2, THEME_PANEL, 4.0f);
    DrawGlowRectLines(HUD_P2, 1.5f, p2, 5.0f);
    DrawText("SKILLS", (int)HUD_P2.x + 8, (int)HUD_P2.y + 6, 15, p2);

    const Paddle* right = &game->player2Paddle;
    DrawSkillRow((int)HUD_P2.x + 8, (int)HUD_P2.y + 28, (int)HUD_P2.width - 16,
                 "1", characters[right->charType].skills[0], right->skill1Cooldown, SKILL1_COOLDOWN, p2);
    DrawSkillRow((int)HUD_P2.x + 8, (int)HUD_P2.y + 52, (int)HUD_P2.width - 16,
                 "2", characters[right->charType].skills[1], right->skill2Cooldown, SKILL2_COOLDOWN, p2);
    DrawSkillRow((int)HUD_P2.x + 8, (int)HUD_P2.y + 76, (int)HUD_P2.width - 16,
                 "3", characters[right->charType].skills[2], right->ultimateCooldown, ULTIMATE_COOLDOWN, p2);
}

void UpdateGame(Game* game, float deltaTime) {
    if (game->gameState == PLAY_CPU || game->gameState == PLAY_PVP) {
        updateGamePlay(game, deltaTime);
    } else if (game->gameState == MENU) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetVirtualMouse();
            if (CheckCollisionPointRec(mousePos, MENU_BTN_CPU)) {
                game->gameState = CHARACTER_SELECT;
                game->player2Paddle.isCpu = true;
            } else if (CheckCollisionPointRec(mousePos, MENU_BTN_PVP)) {
                game->gameState = CHARACTER_SELECT;
                game->player2Paddle.isCpu = false;
            }
        }
    } else if (game->gameState == CHARACTER_SELECT) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetVirtualMouse();
            for (int i = 0; i < CHARACTER_COUNT; i++) {
                if (CheckCollisionPointRec(mousePos, CharCellRect(i))) {
                    game->selectedChar[game->currentSelectingPlayer] = i;
                    if (game->player2Paddle.isCpu) { // CPU mode
                        game->selectedChar[1] = GetRandomValue(0, CHARACTER_COUNT - 1);
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
    DrawGridBackground(WINDOW_WIDTH, WINDOW_HEIGHT);

    if (game->gameState == MENU) {
        DrawMenu(game);
    } else if (game->gameState == CHARACTER_SELECT) {
        DrawCharacterSelect(game);
    } else if (game->gameState == PLAY_CPU || game->gameState == PLAY_PVP) {
        if (game->playerScore >= WINNING_SCORE) {
            DrawTextCentered("YOU WIN!", WINDOW_WIDTH / 2.0f, 250, 48, characters[game->paddle.charType].accent, true);
        } else if (game->cpuScore >= WINNING_SCORE) {
            const char* who = game->player2Paddle.isCpu ? "CPU WINS!" : "PLAYER 2 WINS!";
            DrawTextCentered(who, WINDOW_WIDTH / 2.0f, 250, 48, characters[game->player2Paddle.charType].accent, true);
        } else {
            DrawGameplay(game);
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
