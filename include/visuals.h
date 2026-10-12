/*
 * visuals.h
 * Pure, headless-testable queries over the Effects registry that drive the
 * gameplay "juice" rendering (ball tint, paddle aura, dash trail, HUD active
 * dot, shield barrier). No raylib DRAW calls here — Color is a plain struct,
 * so these functions are unit-testable without a window (R22).
 *
 * The actual composition of glow shapes from these queries lives in
 * DrawEffects() (src/game.c), which owns the only raylib drawing.
 */

#ifndef VISUALS_H
#define VISUALS_H

#include "effects.h"
#include "ball.h"
#include "paddle.h"
#include "raylib.h"

/* Color of the FIRST active effect matching (target, paddleIndex) with any
 * stat, or the transparent black Color (0,0,0,0) when nothing matches. Call
 * with TARGET_BALL, -1 for the ball and TARGET_PADDLE, idx for a paddle. */
Color EffectTintColor(const Effects* fx, EffectTarget target, int paddleIndex);

/* True while a STAT_TRAIL marker is live for (target, paddleIndex). */
bool EffectTrailActive(const Effects* fx, EffectTarget target, int paddleIndex);

/* True if ANY active effect belongs to this player. Ball-targeted effects are
 * keyed at paddleIndex -1, so ownership runs through the effect's `.source`;
 * paddle-targeted effects are owned by their paddleIndex directly. */
bool PlayerHasActiveEffect(const Effects* fx, int paddleIndex);

/* True while the Defender shield reflect window is armed on this paddle. */
bool ShieldActive(const Paddle* paddle);

/* True while the Speedster Afterburner charged-return window is armed on this paddle. */
bool AfterburnerArmed(const Paddle* paddle);

#endif /* VISUALS_H */