/*
 * assets.h
 * Texture loading with graceful fallback. Missing files are logged and
 * reported via the `loaded` flag instead of crashing; draw code checks the
 * flag and renders a procedural placeholder when art isn't present yet.
 */
#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"
#include "character.h"

typedef struct Sprite {
    Texture2D texture;
    bool loaded;
} Sprite;

typedef struct Assets {
    Sprite logo;
    Sprite character[CHARACTER_COUNT]; /* index matches CharacterType */
} Assets;

void LoadAssets(Assets* assets);
void UnloadAssets(Assets* assets);

/* Draw `sprite` to fit inside `box` centered, tinted `tint` (flip-free). */
void DrawSpriteInBox(Sprite sprite, Rectangle box, Color tint);

#endif /* ASSETS_H */
