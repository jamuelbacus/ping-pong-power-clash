/*
 * theme.c
 * Implements the neon palette helpers declared in theme.h.
 */
#include "theme.h"
#include <math.h>

/* Alpha falloff for the i-th glow shell (i = 1 inner .. steps outer). */
static float GlowAlpha(int i, int steps) {
    return 0.14f * (1.0f - (float)i / (float)(steps + 1));
}

void DrawGridBackground(int screenWidth, int screenHeight) {
    const int step = 40;

    /* Slow horizontal drift so the grid feels alive. */
    float drift = fmodf((float)GetTime() * 18.0f, (float)step);

    for (float x = -drift; x <= (float)screenWidth; x += step)
        DrawLine((int)x, 0, (int)x, screenHeight, Fade(THEME_GRID, 0.55f));
    for (int y = 0; y <= screenHeight; y += step)
        DrawLine(0, y, screenWidth, y, Fade(THEME_GRID, 0.55f));

    /* Soft radial glow toward the arena center. */
    Vector2 center = { screenWidth / 2.0f, screenHeight / 2.0f };
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 16; i >= 1; i--)
        DrawCircleV(center, 70.0f + i * 24.0f, Fade(THEME_GRID_HI, 0.02f));
    EndBlendMode();
}

void DrawGlowRect(Rectangle rec, Color color, float glow) {
    int steps = (int)glow;
    if (steps > 0) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = steps; i >= 1; i--) {
            Rectangle shell = { rec.x - i, rec.y - i, rec.width + 2 * i, rec.height + 2 * i };
            DrawRectangleRec(shell, Fade(color, GlowAlpha(i, steps)));
        }
        EndBlendMode();
    }
    DrawRectangleRec(rec, color);
}

void DrawGlowRectLines(Rectangle rec, float thickness, Color color, float glow) {
    int steps = (int)glow;
    if (steps > 0) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = steps; i >= 1; i--) {
            Rectangle shell = { rec.x - i, rec.y - i, rec.width + 2 * i, rec.height + 2 * i };
            DrawRectangleLinesEx(shell, thickness + i, Fade(color, GlowAlpha(i, steps)));
        }
        EndBlendMode();
    }
    DrawRectangleLinesEx(rec, thickness, color);
}

void DrawGlowCircle(Vector2 center, float radius, Color color, float glow) {
    int steps = (int)glow;
    if (steps > 0) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = steps; i >= 1; i--)
            DrawCircleV(center, radius + i, Fade(color, GlowAlpha(i, steps)));
        EndBlendMode();
    }
    DrawCircleV(center, radius, color);
}

void DrawGlowLine(Vector2 a, Vector2 b, Color color, float glow) {
    int steps = (int)glow;
    if (steps > 0) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = steps; i >= 1; i--)
            DrawLineEx(a, b, (float)(i * 2), Fade(color, GlowAlpha(i, steps)));
        EndBlendMode();
    }
    DrawLineEx(a, b, 2.0f, color);
}

void DrawGlowText(const char* text, int x, int y, int fontSize, Color color) {
    Color halo = Fade(color, 0.30f);
    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
            if (dx != 0 || dy != 0)
                DrawText(text, x + dx, y + dy, fontSize, halo);
    DrawText(text, x, y, fontSize, color);
}
