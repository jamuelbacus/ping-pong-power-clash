#include "game.h"
#include "theme.h"
#include "layout.h"
#include "abilities.h"
#include "viewport.h"
#include "visuals.h"
#include "trail.h"

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
    game->player2Score = 0;
    game->gameState = MENU;
    game->currentSelectingPlayer = 0;
    game->selectedChar[0] = SPEEDSTER;
    game->selectedChar[1] = SPEEDSTER;
    EffectsInit(&game->effects);
}

int GameWinner(const Game* game) {
    if (game->playerScore >= WINNING_SCORE) return 1;
    if (game->player2Score >= WINNING_SCORE) return 2;
    return 0;
}

static void resetBall(Game* game) {
    Ball* ball = &game->ball;

    /* Serve toward the side that just conceded (keep the last horizontal direction). */
    float dir = (ball->velocity.x > 0.0f) ? 1.0f : -1.0f;

    ball->position = BALL_POSITION;
    ball->baseVelocity = (Vector2){ BALL_BASE_VELOCITY_X * dir, BALL_BASE_VELOCITY_Y };
    ball->velocity = ball->baseVelocity;
    ball->speedFactor = 1.0f;

    /* Clear all active effects so nothing (ball- or paddle-targeted) lingers
     * across a point. */
    EffectsClear(&game->effects);
    /* Also disarm any armed Defender shield window: an unspent reflect must
     * not carry into the next point (R23). */
    game->paddle.shieldTimer = 0.0f;
    game->player2Paddle.shieldTimer = 0.0f;
    ball->slowTimer = 0.0f;
    ball->lagTimer = 0.0f;
    ball->lateralVelocity = 0.0f;
    ball->sizeMultiplier = 1.0f;
    ball->unpredictableBounce = false;
    ball->randomEffects = false;

    /* A point boundary is a hard stop for afterimages: wipe both paddle rings
     * and the ball ring so no ghost survives into the next point. */
    TrailHistoryInit(&game->ball.trail);
    TrailHistoryInit(&game->paddle.trail);
    TrailHistoryInit(&game->player2Paddle.trail);
}

/* ---------------------------------------------------------------- gameplay */

static void updateGamePlay(Game* game, float deltaTime) {
    /* Expire timed effects before physics so effects that end this frame are
     * gone before UpdateBall/UpdatePaddle query them. */
    EffectsUpdate(&game->effects, deltaTime);

    /* Match over: stop simulating until the player leaves GAME_OVER. */
    if (game->playerScore >= WINNING_SCORE || game->player2Score >= WINNING_SCORE) {
        game->gameState = GAME_OVER;
        return;
    }

    UpdateBall(&game->ball, &game->effects, deltaTime, WINDOW_WIDTH, WINDOW_HEIGHT, &game->paddle, &game->player2Paddle);

    if (game->ball.position.x < 0) {
        game->player2Score++;
        resetBall(game);
    } else if (game->ball.position.x > WINDOW_WIDTH) {
        game->playerScore++;
        resetBall(game);
    }

    UpdatePaddle(&game->paddle, &game->effects, 0, deltaTime, WINDOW_HEIGHT, &game->ball);
    UpdatePaddle(&game->player2Paddle, &game->effects, 1, deltaTime, WINDOW_HEIGHT, &game->ball);

    if (game->player2Paddle.isCpu) {
        CpuTickAbilities(&game->effects, &game->ball, &game->player2Paddle, 1, deltaTime);
    }
}

/* ---------------------------------------------------------------- draw */

/* Effect "juice", drawn AFTER the entities so auras tint over them (R22):
 *  - ball trail: two velocity-offset ghost circles while STAT_TRAIL is live
 *  - ball tint: translucent accent glow over the ball while any ball effect runs
 *  - paddle aura + dash ghost rects + defender shield barrier per paddle
 * Every pushed stat marker maps to a visible state here; nothing in the
 * registry is draw-only-dead (see self-review in the task report). */
static void DrawEffects(const Effects* fx, const Ball* ball, const Paddle* p1, const Paddle* p2) {
    if (EffectTrailActive(fx, TARGET_BALL, -1)) {
        Vector2 g1 = { ball->position.x - ball->velocity.x * 0.12f,
                       ball->position.y - ball->velocity.y * 0.12f };
        Vector2 g2 = { ball->position.x - ball->velocity.x * 0.24f,
                       ball->position.y - ball->velocity.y * 0.24f };
        float r = ball->radius * ball->sizeMultiplier;
        DrawGlowCircle(g1, r, Fade(ball->color, 0.35f), 6.0f);
        DrawGlowCircle(g2, r, Fade(ball->color, 0.20f), 6.0f);
    }

    Color ballTint = EffectTintColor(fx, TARGET_BALL, -1);
    if (ballTint.a > 0) {
        DrawGlowCircle(ball->position, ball->radius * ball->sizeMultiplier + 2.0f,
                       Fade(ballTint, 0.4f), 8.0f);
    }

    for (int i = 0; i < 2; i++) {
        const Paddle* p = (i == 0) ? p1 : p2;

        /* Accent aura border while any timed effect boosts this paddle. */
        Color tint = EffectTintColor(fx, TARGET_PADDLE, i);
        if (tint.a > 0) {
            Rectangle aura = { p->position.x - 4.0f, p->position.y - 4.0f,
                               p->size.x + 8.0f, p->size.y + 8.0f };
            DrawGlowRectLines(aura, 1.5f, tint, 6.0f);
        }

        /* Dash afterimage: two ghost rects lagging the movement (clamped so
         * a high velocityY can't fling the ghosts off the arena). */
        if (EffectTrailActive(fx, TARGET_PADDLE, i)) {
            float off1 = p->velocityY * 0.05f;
            float off2 = p->velocityY * 0.10f;
            if (off1 > 10.0f) off1 = 10.0f; else if (off1 < -10.0f) off1 = -10.0f;
            if (off2 > 16.0f) off2 = 16.0f; else if (off2 < -16.0f) off2 = -16.0f;
            Color ghost = characters[p->charType].accent;
            Rectangle g = { p->position.x, p->position.y - off1, p->size.x, p->size.y };
            DrawGlowRect(g, Fade(ghost, 0.35f), 4.0f);
            g.y = p->position.y - off2;
            DrawGlowRect(g, Fade(ghost, 0.20f), 4.0f);
        }

        /* Defender shield barrier in front of the paddle's court face. */
        if (ShieldActive(p)) {
            float x = (p->position.x < 480.0f)
                ? p->position.x + p->size.x + 4.0f
                : p->position.x - 4.0f;
            Rectangle barrier = { x, p->position.y, 2.0f, p->size.y };
            DrawGlowRectLines(barrier, 2.0f, characters[DEFENDER].accent, 4.0f);
        }
    }
}

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
    DrawEffects(&game->effects, &game->ball, &game->paddle, &game->player2Paddle);

    Color p1 = characters[game->paddle.charType].accent;
    Color p2 = characters[game->player2Paddle.charType].accent;

    /* Scores */
    DrawTextCentered(TextFormat("%d", game->playerScore), WINDOW_WIDTH / 4.0f, 20, 48, p1, true);
    DrawTextCentered(TextFormat("%d", game->player2Score), 3 * WINDOW_WIDTH / 4.0f, 20, 48, p2, true);

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

    /* Active-effect indicator: ONE glowing dot per player while any of their
     * timed effects (or the Defender shield window) is live. Deliberately not
     * a per-slot ability->effect map — the registry is the only source of
     * truth, so this can never drift from what DrawEffects actually renders. */
    if (PlayerHasActiveEffect(&game->effects, 0) || ShieldActive(&game->paddle))
        DrawGlowCircle((Vector2){ HUD_P1.x + HUD_P1.width - 10.0f, HUD_P1.y + 10.0f }, 4.0f, p1, 6.0f);
    if (PlayerHasActiveEffect(&game->effects, 1) || ShieldActive(&game->player2Paddle))
        DrawGlowCircle((Vector2){ HUD_P2.x + HUD_P2.width - 10.0f, HUD_P2.y + 10.0f }, 4.0f, p2, 6.0f);
}

static void DrawGameOver(const Game* game) {
    int winner = GameWinner(game);
    const char* message;
    Color accent;

    if (winner == 1) {
        message = game->player2Paddle.isCpu ? "You Win!" : "Player 1 Wins!";
        accent = characters[game->paddle.charType].accent;
    } else if (winner == 2) {
        message = game->player2Paddle.isCpu ? "CPU Wins!" : "Player 2 Wins!";
        accent = characters[game->player2Paddle.charType].accent;
    } else {
        message = "Game Over";
        accent = THEME_TEXT;
    }

    DrawTextCentered(message, WINDOW_WIDTH / 2.0f, 210, 48, accent, true);
    DrawTextCentered("SPACE: Menu    R: Rematch", WINDOW_WIDTH / 2.0f, 285, 20, THEME_TEXT, false);
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
    } else if (game->gameState == GAME_OVER) {
        if (IsKeyPressed(KEY_SPACE)) {
            game->gameState = MENU;
        } else if (IsKeyPressed(KEY_R)) {
            ResetGame(game);
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
        DrawGameplay(game);
    } else if (game->gameState == GAME_OVER) {
        DrawGameplay(game);
        DrawGameOver(game);
    }
}

void ResetGame(Game* game) {
    game->playerScore = 0;
    game->player2Score = 0;
    resetBall(game);
    EffectsClear(&game->effects);
    game->gameState = MENU;
    game->currentSelectingPlayer = 0;
}
