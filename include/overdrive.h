/*
 * overdrive.h
 * Speedster passive "Overdrive" tuning. Heat is one scalar in [0,1] that
 * widens a paddle's max speed: maxSpeed = originalMaxSpeed * (1 + 0.5*heat).
 * Rally heat comes from the ball's effective speed each frame; Flow (Q/E use)
 * adds a timed bump on top. Clutch (Task 5) sets it directly.
 */
#ifndef OVERDRIVE_H
#define OVERDRIVE_H

/* |(300,200)| serve speed — heat stays 0 while the ball is this slow. Keep this literal
 * bit-identical to the float32 rounding of sqrtf(300^2 + 200^2): a serve-speed ball must
 * yield heat exactly 0, which is what makes test_effective_stats' exact "maxSpeed ==
 * 1000.0f" / "== 500.0f" guards hold. */
static const float BALL_SPEED_BASE = 360.5551f;
static const float OVERDRIVE_SPEED_PER_HEAT = 0.5f;      /* +50% paddle speed at full heat */
static const float OVERDRIVE_FLOW_HEAT = 0.35f;
static const float OVERDRIVE_FLOW_DURATION = 1.5f;
static const float OVERDRIVE_FLOW_TRIM = 1.0f;          /* shaved off the other ability's cooldown */
static const float OVERDRIVE_CLUTCH_HEAT = 1.0f;
static const float OVERDRIVE_CLUTCH_DURATION = 0.5f;
static const float OVERDRIVE_CLUTCH_TRAIL = 0.5f;
static const float OVERDRIVE_CLUTCH_EDGE = 0.25f;

#endif
