#include "sfx.h"
#include <stddef.h>
#include <stdio.h>

static const char* charSkillNames[CHARACTER_COUNT] = {
    "speedster",
    "power_striker",
    "grand_wizard",
    "trickster",
    "defender",
    "elementalist",
    "sniper",
    "chaos_bot"
};

void InitSfx(Sfx *sfx)
{
    if (sfx == NULL)
        return;
    sfx->hitWeak = LoadSound("assets/audio/pong/hit_weak.wav");
    sfx->hitGood = LoadSound("assets/audio/pong/hit_good.wav");
    sfx->hitPerfect = LoadSound("assets/audio/pong/hit_perfect.wav");
    sfx->bounce = LoadSound("assets/audio/pong/bounce.wav");
    sfx->powerupCollect = LoadSound("assets/audio/effects/powerup_collect.wav");
    sfx->streak3 = LoadSound("assets/audio/effects/streak3.wav");
    sfx->streak5 = LoadSound("assets/audio/effects/streak5.wav");
    sfx->score = LoadSound("assets/audio/effects/score.wav");
    sfx->skillCast = LoadSound("assets/audio/skills/skill_cast.wav");
    sfx->skillReady = LoadSound("assets/audio/skills/skill_ready.wav");
    sfx->menuSelect = LoadSound("assets/audio/effects/menu_select.wav");
    sfx->menuConfirm = LoadSound("assets/audio/effects/menu_confirm.wav");
    for (int c = 0; c < CHARACTER_COUNT; c++) {
        for (int a = 0; a < 3; a++) {
            char path[256];
            snprintf(path, sizeof(path), "assets/audio/skills/%s_skill%d.wav", charSkillNames[c], a + 1);
            sfx->charSkillCast[c][a] = LoadSound(path);
        }
    }
    sfx->bgmGame = LoadMusicStream("assets/audio/music/bgm_light.wav");
    sfx->bgmMenu = LoadMusicStream("assets/audio/music/bgm_menu.wav");
    sfx->initialized = true;
}

void UnloadSfx(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    UnloadSound(sfx->hitWeak);
    UnloadSound(sfx->hitGood);
    UnloadSound(sfx->hitPerfect);
    UnloadSound(sfx->bounce);
    UnloadSound(sfx->powerupCollect);
    UnloadSound(sfx->streak3);
    UnloadSound(sfx->streak5);
    UnloadSound(sfx->score);
    UnloadSound(sfx->skillCast);
    UnloadSound(sfx->skillReady);
    UnloadSound(sfx->menuSelect);
    UnloadSound(sfx->menuConfirm);
    for (int c = 0; c < CHARACTER_COUNT; c++) {
        for (int a = 0; a < 3; a++) {
            UnloadSound(sfx->charSkillCast[c][a]);
        }
    }
    UnloadMusicStream(sfx->bgmGame);
    UnloadMusicStream(sfx->bgmMenu);
    sfx->initialized = false;
}
void PlayHitSound(Sfx *sfx, int quality)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    if (quality == 2)
        PlaySound(sfx->hitPerfect);
    else if (quality == 1)
        PlaySound(sfx->hitGood);
    else
        PlaySound(sfx->hitWeak);
}

void PlayBounce(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->bounce);
}

void PlayPowerupCollect(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->powerupCollect);
}

void PlayStreak3(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->streak3);
}

void PlayStreak5(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->streak5);
}

void PlayScore(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->score);
}

void PlaySkillCast(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->skillCast);
}

void PlaySkillCastFor(Sfx *sfx, CharacterType charType, int ability)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    if (charType < 0 || charType >= CHARACTER_COUNT)
        charType = 0;
    if (ability < 0 || ability > 2)
        ability = 0;
    PlaySound(sfx->charSkillCast[charType][ability]);
}

void PlaySkillReady(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->skillReady);
}

void PlayMenuSelect(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->menuSelect);
}

void PlayMenuConfirm(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->menuConfirm);
}

void PlayGameBgm(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    StopMenuBgm(sfx);
    if (!IsMusicStreamPlaying(sfx->bgmGame)) {
        SetMusicVolume(sfx->bgmGame, 0.4f);
        PlayMusicStream(sfx->bgmGame);
    }
}

void StopGameBgm(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    StopMusicStream(sfx->bgmGame);
}

void PlayMenuBgm(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    StopGameBgm(sfx);
    if (!IsMusicStreamPlaying(sfx->bgmMenu)) {
        SetMusicVolume(sfx->bgmMenu, 0.4f);
        PlayMusicStream(sfx->bgmMenu);
    }
}

void StopMenuBgm(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    StopMusicStream(sfx->bgmMenu);
}

void UpdateMusicStreams(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    if (IsMusicStreamPlaying(sfx->bgmMenu))
        UpdateMusicStream(sfx->bgmMenu);
    if (IsMusicStreamPlaying(sfx->bgmGame))
        UpdateMusicStream(sfx->bgmGame);
}

void PlayBgm(Sfx *sfx)
{
    PlayGameBgm(sfx);
}

void StopBgm(Sfx *sfx)
{
    StopGameBgm(sfx);
    StopMenuBgm(sfx);
}
