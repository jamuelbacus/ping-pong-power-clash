#ifndef SFX_H
#define SFX_H

#include "raylib.h"
#include "character.h"

typedef struct {
    Sound hitWeak;
    Sound hitGood;
    Sound hitPerfect;
    Sound bounce;
    Sound powerupCollect;
    Sound streak3;
    Sound streak5;
    Sound score;
    Sound skillCast;
    Sound skillReady;
    Sound menuSelect;
    Sound menuConfirm;
    Sound charSkillCast[CHARACTER_COUNT][3];
    Music bgmGame;
    Music bgmMenu;
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
void PlaySkillCast(Sfx* sfx);
void PlaySkillCastFor(Sfx* sfx, CharacterType charType, int ability);
void PlaySkillReady(Sfx* sfx);
void PlayMenuSelect(Sfx* sfx);
void PlayMenuConfirm(Sfx* sfx);
void PlayGameBgm(Sfx* sfx);
void StopGameBgm(Sfx* sfx);
void PlayMenuBgm(Sfx* sfx);
void StopMenuBgm(Sfx* sfx);
void UpdateMusicStreams(Sfx* sfx);
void PlayBgm(Sfx* sfx);
void StopBgm(Sfx* sfx);

#endif /* SFX_H */
