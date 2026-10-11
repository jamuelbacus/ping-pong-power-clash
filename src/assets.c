/*
 * assets.c
 * See assets.h. All paths are resolved relative to the working directory,
 * so the game must be launched from the project root (same rule the logo
 * already followed).
 */
#include "assets.h"

#include <math.h>
#include <stddef.h>

/* One icon per character, index matches the CharacterType enum. */
static const char* CHARACTER_ICON_PATHS[CHARACTER_COUNT] = {
    "assets/characters/speedster.png",
    "assets/characters/power_striker.png",
    "assets/characters/grand_wizard.png",
    "assets/characters/trickster.png",
    "assets/characters/defender.png",
    "assets/characters/elementalist.png",
    "assets/characters/sniper.png",
    "assets/characters/chaos_bot.png",
};

static Sprite LoadSprite(const char* path) {
    Sprite sprite = { 0 };
    if (path == NULL || !FileExists(path)) {
        TraceLog(LOG_WARNING, "ASSETS: missing '%s' (using placeholder)", path ? path : "(null)");
        return sprite;
    }
    sprite.texture = LoadTexture(path);
    sprite.loaded = (sprite.texture.id != 0);
    if (!sprite.loaded)
        TraceLog(LOG_WARNING, "ASSETS: failed to load '%s'", path);
    return sprite;
}

void LoadAssets(Assets* assets) {
    assets->logo = LoadSprite("pingpongpowerclashlogo.png");
    for (int i = 0; i < CHARACTER_COUNT; i++)
        assets->character[i] = LoadSprite(CHARACTER_ICON_PATHS[i]);
}

void UnloadAssets(Assets* assets) {
    if (assets->logo.loaded) UnloadTexture(assets->logo.texture);
    for (int i = 0; i < CHARACTER_COUNT; i++)
        if (assets->character[i].loaded) UnloadTexture(assets->character[i].texture);
}

void DrawSpriteInBox(Sprite sprite, Rectangle box, Color tint) {
    if (!sprite.loaded) return;

    float tw = (float)sprite.texture.width;
    float th = (float)sprite.texture.height;
    float scale = (tw > 0.0f && th > 0.0f) ? fminf(box.width / tw, box.height / th) : 1.0f;

    Rectangle src = { 0, 0, tw, th };
    Rectangle dst = {
        box.x + (box.width - tw * scale) / 2.0f,
        box.y + (box.height - th * scale) / 2.0f,
        tw * scale,
        th * scale,
    };
    DrawTexturePro(sprite.texture, src, dst, (Vector2){ 0, 0 }, 0.0f, tint);
}
