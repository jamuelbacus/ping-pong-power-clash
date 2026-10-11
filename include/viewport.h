/*
 * viewport.h
 * Fixed-resolution render target presented letterboxed to a resizable
 * window. Owns the window->virtual coordinate transform so mouse input
 * stays aligned with the game's 960x540 drawing space.
 */
#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "raylib.h"

typedef struct Viewport {
    RenderTexture2D target;
    int designWidth;
    int designHeight;
    float scale;
    Vector2 offset; /* top-left of the presented image, in window pixels */
} Viewport;

/* Create the render target and reset the transform. */
void InitViewport(Viewport* vp, int designWidth, int designHeight);
void UnloadViewport(Viewport* vp);

/* Draw the game into the virtual resolution between these two calls;
 * EndViewport presents it to the window (letterboxed, centered). */
void BeginViewport(Viewport* vp);
void EndViewport(Viewport* vp);

/* GetMousePosition() mapped back into virtual (design) coordinates. */
Vector2 GetVirtualMouse(void);

#endif /* VIEWPORT_H */
