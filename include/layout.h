/*
 * layout.h
 * Named UI layout constants. Used by BOTH the draw code and the mouse
 * hit-tests in game.c so the two can never drift apart.
 */
#ifndef LAYOUT_H
#define LAYOUT_H

#include "raylib.h"

/* --- Main menu --- */
static const Rectangle MENU_BTN_CPU = { 300, 300, 360, 64 };
static const Rectangle MENU_BTN_PVP = { 300, 384, 360, 64 };

/* --- Character select grid --- */
#define CHAR_GRID_COLS 4
#define CHAR_GRID_ROWS 2
static const float CHAR_CELL_W = 220.0f;
static const float CHAR_CELL_H = 150.0f;
static const float CHAR_CARD_PAD = 12.0f;
static const Vector2 CHAR_GRID_ORIGIN = { 40.0f, 150.0f };

/* Cell rectangle (includes padding) for grid index 0..7. */
static inline Rectangle CharCellRect(int index) {
    float x = CHAR_GRID_ORIGIN.x + (index % CHAR_GRID_COLS) * CHAR_CELL_W;
    float y = CHAR_GRID_ORIGIN.y + (index / CHAR_GRID_COLS) * CHAR_CELL_H;
    return (Rectangle){ x, y, CHAR_CELL_W - CHAR_CARD_PAD, CHAR_CELL_H - CHAR_CARD_PAD };
}

/* --- Gameplay HUD --- */
static const Rectangle HUD_P1 = {  10, 430, 230, 100 };
static const Rectangle HUD_P2 = { 720, 430, 230, 100 };

/* Skill cooldown maxima (seconds) — keep in sync with paddle.c */
#define SKILL1_COOLDOWN 5.0f
#define SKILL2_COOLDOWN 7.0f
#define ULTIMATE_COOLDOWN 15.0f

#endif /* LAYOUT_H */
