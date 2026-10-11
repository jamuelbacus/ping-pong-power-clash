/*
 * viewport.c
 * See viewport.h.
 */
#include "viewport.h"
#include <math.h>

/* Current present transform, kept in sync by EndViewport so the mouse
 * helper can map window pixels back to virtual pixels. */
static float g_scale = 1.0f;
static Vector2 g_offset = { 0.0f, 0.0f };

static void SyncTransform(float scale, Vector2 offset) {
    g_scale = scale;
    g_offset = offset;
}

void InitViewport(Viewport* vp, int designWidth, int designHeight) {
    vp->designWidth = designWidth;
    vp->designHeight = designHeight;
    vp->target = LoadRenderTexture(designWidth, designHeight);
    SetTextureFilter(vp->target.texture, TEXTURE_FILTER_BILINEAR);
    vp->scale = 1.0f;
    vp->offset = (Vector2){ 0.0f, 0.0f };
    SyncTransform(1.0f, (Vector2){ 0.0f, 0.0f });
}

void UnloadViewport(Viewport* vp) {
    UnloadRenderTexture(vp->target);
}

void BeginViewport(Viewport* vp) {
    BeginTextureMode(vp->target);
}

void EndViewport(Viewport* vp) {
    EndTextureMode();

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float scale = fminf((float)screenW / (float)vp->designWidth,
                        (float)screenH / (float)vp->designHeight);
    float w = vp->designWidth * scale;
    float h = vp->designHeight * scale;
    Vector2 offset = { (screenW - w) / 2.0f, (screenH - h) / 2.0f };

    vp->scale = scale;
    vp->offset = offset;
    SyncTransform(scale, offset);

    BeginDrawing();
    ClearBackground(BLACK); /* letterbox bars */

    Rectangle src = { 0, 0, (float)vp->target.texture.width, -(float)vp->target.texture.height };
    Rectangle dst = { offset.x, offset.y, w, h };
    DrawTexturePro(vp->target.texture, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    EndDrawing();
}

Vector2 GetVirtualMouse(void) {
    Vector2 m = GetMousePosition();
    if (g_scale <= 0.0f) return m;
    return (Vector2){ (m.x - g_offset.x) / g_scale, (m.y - g_offset.y) / g_scale };
}
