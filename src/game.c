#include "game.h"
#include "theme.h"
#include "layout.h"
#include "abilities.h"
#include "viewport.h"
#include "visuals.h"
#include "trail.h"
#include "decoy.h"

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

/* Cooldown maxima for the three skill slots, indexed 0..2 (skill1, skill2,
 * ultimate). Skill cooldown maxima live in layout.h and are kept in sync with
 * paddle.c. */
static const float HUD_BAR_MAX[3] = { SKILL1_COOLDOWN, SKILL2_COOLDOWN, ULTIMATE_COOLDOWN };

/* Three cooldown bars under one side's score, centred on centerX. Each bar fills
 * via SkillRecoverFraction (empty = just used, full = ready). `glow` brightens
 * the whole set while that player has a live effect — the old standalone
 * "active dot", folded into the bars. */
static void DrawSkillBars(const Paddle* p, float centerX, float y, Color accent, bool glow,
                          const char* k0, const char* k1, const char* k2) {
    float cd[3] = { p->skill1Cooldown, p->skill2Cooldown, p->ultimateCooldown };
    const char* keys[3] = { k0, k1, k2 };
    float groupW = 3.0f * HUD_BAR_WIDTH + 2.0f * HUD_BAR_GAP;
    float x0 = centerX - groupW / 2.0f;
    for (int i = 0; i < 3; i++) {
        float f = SkillRecoverFraction(cd[i], HUD_BAR_MAX[i]);
        Rectangle track = { x0 + i * (HUD_BAR_WIDTH + HUD_BAR_GAP), y, HUD_BAR_WIDTH, HUD_BAR_HEIGHT };
        if (glow) DrawGlowRectLines(track, 1.0f, Fade(accent, 0.7f), 5.0f);
        DrawRectangleRec(track, Fade(THEME_TEXT_DIM, 0.30f));
        DrawRectangleRec((Rectangle){ track.x, track.y, track.width * f, track.height }, accent);
        DrawText(keys[i], (int)track.x + (int)(HUD_BAR_WIDTH / 2) - 3,
                 (int)(y + HUD_BAR_HEIGHT + 2), (int)HUD_KEY_FONT, THEME_TEXT_DIM);
    }
}

/* Translucent top-centre scoreline: P1 score | P2 score, each with its name and
 * three cooldown bars. Drawn BEFORE the ball so it can never hide it.
 * Vertical rhythm (all relative to HUD_SCORELINE_TOP, plate is 68px tall):
 *   +2 scores, +36 names, +48 bars, +54 key letters — stays above the bottom. */
static void DrawScoreline(const Game* game) {
    Color p1 = characters[game->paddle.charType].accent;
    Color p2 = characters[game->player2Paddle.charType].accent;
    Rectangle plate = { WINDOW_WIDTH / 2.0f - HUD_SCORELINE_WIDTH / 2.0f, HUD_SCORELINE_TOP,
                        HUD_SCORELINE_WIDTH, HUD_SCORELINE_BOTTOM - HUD_SCORELINE_TOP };
    DrawRectangleRec(plate, Fade(THEME_PANEL, 0.55f));

    float cx = WINDOW_WIDTH / 2.0f;
    int scoreY = (int)(HUD_SCORELINE_TOP + 2.0f);
    int nameY  = (int)(HUD_SCORELINE_TOP + 36.0f);
    float barY = HUD_SCORELINE_TOP + 48.0f;

    DrawTextCentered(TextFormat("%d", game->playerScore), cx - HUD_SCORE_OFFSET, scoreY, (int)HUD_SCORE_FONT, p1, true);
    DrawTextCentered(TextFormat("%d", game->player2Score), cx + HUD_SCORE_OFFSET, scoreY, (int)HUD_SCORE_FONT, p2, true);
    DrawTextCentered(":", cx, scoreY + 6, (int)HUD_SCORE_FONT, THEME_TEXT_DIM, false);

    DrawTextCentered(characters[game->paddle.charType].name, cx - HUD_SCORE_OFFSET, nameY, (int)HUD_NAME_FONT, THEME_TEXT, false);
    DrawTextCentered(characters[game->player2Paddle.charType].name, cx + HUD_SCORE_OFFSET, nameY, (int)HUD_NAME_FONT, THEME_TEXT, false);

    bool glow1 = PlayerHasActiveEffect(&game->effects, 0) || ShieldActive(&game->paddle) || AfterburnerArmed(&game->paddle);
    bool glow2 = PlayerHasActiveEffect(&game->effects, 1) || ShieldActive(&game->player2Paddle) || AfterburnerArmed(&game->player2Paddle);
    DrawSkillBars(&game->paddle, cx - HUD_SCORE_OFFSET, barY, p1, glow1, "Q", "E", "R");
    DrawSkillBars(&game->player2Paddle, cx + HUD_SCORE_OFFSET, barY, p2, glow2, "1", "2", "3");
}

/* ---------------------------------------------------------------- lifecycle */

void InitGame(Game* game) {
    InitCharacters();
    LoadAssets(&game->assets);
    InitBall(&game->ball, BALL_POSITION.x, BALL_POSITION.y, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    InitPaddle(&game->paddle, PLAYER1_PADDLE_POSITION.x, PLAYER1_PADDLE_POSITION.y, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000.0f, 320.0f, 0.9f, KEY_W, KEY_S, SPEEDSTER);
    InitPaddle(&game->player2Paddle, PLAYER2_PADDLE_POSITION.x, PLAYER2_PADDLE_POSITION.y, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, false, 4000.0f, 320.0f, 0.9f, KEY_UP, KEY_DOWN, SPEEDSTER);
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
    /* ...and any armed Speedster Afterburner charge, same rule. */
    game->paddle.afterburnerTimer = 0.0f;
    game->player2Paddle.afterburnerTimer = 0.0f;
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
    /* ...and no Shadow Clone decoy survives a point either. */
    DecoySwarmInit(&game->ball.decoys);
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

    /* Shadow Clone decoys ride the ball: purely cosmetic, advanced here so
     * they freeze with the rest of the sim at GAME_OVER. */
    DecoySwarmUpdate(&game->ball.decoys, deltaTime, WINDOW_HEIGHT);

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
 *  - ball trail: history echoes at recorded positions while STAT_TRAIL is live
 *  - ball tint: translucent accent glow over the ball while any ball effect runs
 *  - paddle aura + dash history echoes + defender shield barrier per paddle
 * Every pushed stat marker maps to a visible state here; nothing in the
 * registry is draw-only-dead (see self-review in the task report). */
static void DrawEffects(const Effects* fx, const Ball* ball, const Paddle* p1, const Paddle* p2) {
    if (EffectTrailActive(fx, TARGET_BALL, -1)) {
        TrailGhost ghosts[TRAIL_MAX_GHOSTS];
        int n = TrailActiveGhosts(&ball->trail, TRAIL_MAX_GHOSTS, ghosts);
        float r = ball->radius * ball->sizeMultiplier;
        for (int i = 0; i < n; i++) {
            float a = 0.5f - 0.4f * (ghosts[i].age / TRAIL_MAX_AGE);
            DrawGlowCircle(ghosts[i].pos, r, Fade(ball->color, a), 6.0f);
        }
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

        /* True afterimage: fading echoes at recorded positions — the paddle's own
         * footprint is always present (trail history starts at the pre-snap spot). */
        if (EffectTrailActive(fx, TARGET_PADDLE, i)) {
            TrailGhost ghosts[TRAIL_MAX_GHOSTS];
            int n = TrailActiveGhosts(&p->trail, TRAIL_MAX_GHOSTS, ghosts);
            Color ghost = characters[p->charType].accent;
            for (int j = 0; j < n; j++) {
                float a = 0.5f - 0.4f * (ghosts[j].age / TRAIL_MAX_AGE);
                Rectangle g = { ghosts[j].pos.x, ghosts[j].pos.y, p->size.x, p->size.y };
                DrawGlowRect(g, Fade(ghost, a), 4.0f);
            }
        }

        /* Defender shield barrier in front of the paddle's court face. */
        if (ShieldActive(p)) {
            float x = (p->position.x < 480.0f)
                ? p->position.x + p->size.x + 4.0f
                : p->position.x - 4.0f;
            Rectangle barrier = { x, p->position.y, 2.0f, p->size.y };
            DrawGlowRectLines(barrier, 2.0f, characters[DEFENDER].accent, 4.0f);
        }

        /* Speedster Afterburner: a charged outline while the power return is
         * armed, so the window is readable before the ball arrives. */
        if (AfterburnerArmed(p)) {
            Rectangle charge = { p->position.x - 2.0f, p->position.y - 2.0f,
                                 p->size.x + 4.0f, p->size.y + 4.0f };
            DrawGlowRectLines(charge, 2.0f, characters[SPEEDSTER].accent, 8.0f);
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

/* Shadow Clone: draw each live decoy exactly like the real ball so the clones
 * read as genuine balls, fading over the last DECOY_FADE_TIME. Purely visual —
 * no physics ever touched them. */
static void DrawDecoys(const Ball* ball) {
    for (int i = 0; i < DECOY_MAX; i++) {
        const Decoy* d = &ball->decoys.items[i];
        if (!d->live) continue;
        DrawGlowCircle(d->position, ball->radius, Fade(ball->color, DecoyAlpha(d->age)), 8.0f);
    }
}

static void DrawGameplay(const Game* game) {
    /* Center line */
    DrawGlowLine((Vector2){ WINDOW_WIDTH / 2.0f, 0 }, (Vector2){ WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT },
                 Fade(THEME_GRID_HI, 0.7f), 5.0f);

    DrawScoreline(game);          /* HUD first: everything below draws over it */

    DrawBall(&game->ball);
    DrawDecoys(&game->ball);
    DrawPaddle(game->paddle);
    DrawPaddle(game->player2Paddle);
    DrawEffects(&game->effects, &game->ball, &game->paddle, &game->player2Paddle);
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
