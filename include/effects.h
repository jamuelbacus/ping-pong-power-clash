/*
 * effects.h
 * defines a fixed-capacity registry of timed stat modifiers (MUL/ADD/SET)
 * that gameplay code queries via EffectsApply().
 */

#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdbool.h>
#include "raylib.h"

#define EFFECT_CAPACITY 64

typedef enum { TARGET_BALL, TARGET_PADDLE } EffectTarget;
typedef enum { STAT_BALL_SPEED, STAT_BALL_VEL_X, STAT_BALL_VEL_Y, STAT_BALL_SIZE, STAT_PADDLE_SPEED, STAT_PADDLE_SIZE, STAT_TRAIL, STAT_BALL_CURVE } EffectStat;
typedef enum { OP_MUL, OP_ADD, OP_SET } EffectOp;

typedef struct Effect { bool active; EffectTarget target; int paddleIndex; EffectStat stat; EffectOp op; float value; float timeLeft; float duration; Color color; int source; } Effect;

typedef struct Effects { Effect items[EFFECT_CAPACITY]; } Effects;

void EffectsInit(Effects* fx);
void EffectsPush(Effects* fx, Effect e);
void EffectsUpdate(Effects* fx, float dt);
void EffectsClear(Effects* fx);
bool EffectsHas(const Effects* fx, EffectTarget t, int paddleIndex, EffectStat s);
float EffectsApply(const Effects* fx, EffectTarget t, int paddleIndex, EffectStat s, float base);
const Effect* EffectsList(const Effects* fx, int* countOut);

#endif // EFFECTS_H
