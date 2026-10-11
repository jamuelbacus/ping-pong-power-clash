/*
 * theme.h
 * Neon visual theme: palette + glow/backdrop drawing helpers.
 *
 * Colors are macros (rather than `static const`) so this header can be
 * included from any translation unit without triggering unused-variable
 * warnings for the ones a given file doesn't reference.
 */
#ifndef THEME_H
#define THEME_H

#include "raylib.h"

/* --- Palette (dark neon) --- */
#define THEME_BG       ((Color){ 8,   6,  20, 255 })  /* deep indigo-black   */
#define THEME_GRID     ((Color){ 38,  26,  78, 255 })  /* dim violet grid     */
#define THEME_GRID_HI  ((Color){ 96,  60, 190, 255 })  /* grid glow           */
#define THEME_PANEL    ((Color){ 16,  12,  38, 235 })  /* translucent panel   */
#define THEME_PANEL_HI ((Color){ 30,  22,  64, 245 })  /* hovered panel       */
#define THEME_TEXT     ((Color){ 236, 238, 255, 255 }) /* near-white text     */
#define THEME_TEXT_DIM ((Color){ 140, 144, 195, 255 }) /* secondary text      */
#define THEME_ACCENT   ((Color){  90, 220, 255, 255 }) /* default accent      */

/* --- Backdrop --- */
void DrawGridBackground(int screenWidth, int screenHeight);

/* --- Glow primitives ---
 * `glow` is the halo spread in pixels (0 = no glow). Shapes draw a soft
 * additive halo and then the solid shape on top. */
void DrawGlowRect(Rectangle rec, Color color, float glow);
void DrawGlowRectLines(Rectangle rec, float thickness, Color color, float glow);
void DrawGlowCircle(Vector2 center, float radius, Color color, float glow);
void DrawGlowLine(Vector2 a, Vector2 b, Color color, float glow);

/* Text with a soft drop-glow (drawn under normal alpha blending). */
void DrawGlowText(const char* text, int x, int y, int fontSize, Color color);

#endif /* THEME_H */
