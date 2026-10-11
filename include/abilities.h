/*
 * abilities.h
 * Single entry point for activating a character ability (AbilitiesActivate)
 * and the cooldown-gated CPU policy that decides when the CPU fires one
 * (CpuTickAbilities).
 *
 * AbilitiesActivate OWNS setting the ability's cooldown and pushing its
 * effect(s). Per-character effect bodies are filled in by Tasks 6-8; for now
 * the dispatch is scaffolded but the cooldown is still committed.
 */
#ifndef ABILITIES_H
#define ABILITIES_H

#include "raylib.h"
#include "character.h"
#include "ball.h"
#include "paddle.h"
#include "effects.h"

/* How often (seconds) the CPU is allowed to reconsider firing an ability. */
#define CPU_DECISION_INTERVAL 1.0f

/* Probability in [0,1] that a context-approved decision tick actually fires. */
#define CPU_DIFFICULTY 0.5f

/* ability: 0 = skill1, 1 = skill2, 2 = ultimate. Sets that ability's cooldown
 * and pushes the character's effect(s). `paddleIndex` is the source of the
 * effects; ball-targeted effects inside use paddleIndex -1. */
void AbilitiesActivate(Effects* fx, Ball* ball, Paddle* paddle, int paddleIndex, CharacterType type, int ability);

/* Decrements cpuPaddle->cpuDecisionTimer; when it reaches <= 0 the timer resets
 * to CPU_DECISION_INTERVAL and at most ONE ready ability may fire, gated on the
 * ball approaching the paddle and a single CPU_DIFFICULTY roll. */
void CpuTickAbilities(Effects* fx, Ball* ball, Paddle* cpuPaddle, int paddleIndex, float dt);

#endif // ABILITIES_H
