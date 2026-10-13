#ifndef SFX_H
#define SFX_H

#include "raylib.h"

typedef struct {
    Sound hitWeak;
    Sound hitGood;
    Sound hitPerfect;
    Sound bounce;
    Sound powerupCollect;
    Sound streak3;
    Sound streak5;
    Sound score;
    bool initialized;
} Sfx;

void InitSfx(Sfx* sfx);
void UnloadSfx(Sfx* sfx);
void PlayHitSound(Sfx* sfx, int quality); /* 0=weak,1=good,2=perfect */
void PlayBounce(Sfx* sfx);
void PlayPowerupCollect(Sfx* sfx);
void PlayStreak3(Sfx* sfx);
void PlayStreak5(Sfx* sfx);
void PlayScore(Sfx* sfx);

#endif /* SFX_H */
