#include "sfx.h"
#include <stddef.h>

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
    sfx->bgm = LoadMusicStream("assets/audio/music/bgm_light.wav");
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
    UnloadMusicStream(sfx->bgm);
    sfx->initialized = false;
}

void PlayHitSound(Sfx *sfx, int quality)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    if (quality == 2)
    {
        PlaySound(sfx->hitPerfect);
    }
    else if (quality == 1)
    {
        PlaySound(sfx->hitGood);
    }
    else
    {
        PlaySound(sfx->hitWeak);
    }
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

void PlaySkillReady(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    PlaySound(sfx->skillReady);
}

void PlayBgm(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    if (!IsMusicStreamPlaying(sfx->bgm)) {
        SetMusicVolume(sfx->bgm, 0.4f);
        PlayMusicStream(sfx->bgm);
    }
}

void StopBgm(Sfx *sfx)
{
    if (sfx == NULL || !sfx->initialized)
        return;
    StopMusicStream(sfx->bgm);
}
