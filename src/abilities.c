/*
 * abilities.c
 * Ability activation dispatch and the CPU decision policy. No dynamic
 * allocation: everything goes through the fixed-capacity Effects registry.
 */

#include "abilities.h"
#include "layout.h"
#include "trail.h"
#include "decoy.h"
#include "overdrive.h"

#include <math.h>

void AbilitiesActivate(Effects* fx, Ball* ball, Paddle* paddle, int paddleIndex, CharacterType type, int ability) {
    Color color = characters[type].accent;
    int source = paddleIndex;

    /* The ability's cooldown is owned here, regardless of whether a
     * per-character body exists yet. */
    switch (ability) {
        case 0: paddle->skill1Cooldown = SKILL1_COOLDOWN; break;
        case 1: paddle->skill2Cooldown = SKILL2_COOLDOWN; break;
        case 2: paddle->ultimateCooldown = ULTIMATE_COOLDOWN; break;
        default: return;
    }

    switch (type) {
        case SPEEDSTER:
            switch (ability) {
                case 0: /* Dash Return: snap the paddle onto the ball and leave a
                         * short afterimage trail where it flashed from. */
                    /* Park the pre-snap footprint first so the dash origin is
                     * part of the history the trail marker is about to draw. */
                    TrailHistoryRecord(&paddle->trail, paddle->position);
                    paddle->position.y = ball->position.y - paddle->size.y / 2.0f;
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_PADDLE, .paddleIndex = source,
                        .stat = STAT_TRAIL, .op = OP_SET, .value = 1.0f,
                        .timeLeft = 0.3f, .duration = 0.3f, .color = color, .source = source });
                    /* Flow: the use itself banks Overdrive heat and trims the
                     * other ability's cooldown, chaining Q -> E. */
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_PADDLE, .paddleIndex = source,
                        .stat = STAT_PADDLE_HEAT, .op = OP_ADD, .value = OVERDRIVE_FLOW_HEAT,
                        .timeLeft = OVERDRIVE_FLOW_DURATION, .duration = OVERDRIVE_FLOW_DURATION,
                        .color = color, .source = source });
                    if (paddle->skill2Cooldown > 0.0f)
                        paddle->skill2Cooldown = fmaxf(0.0f, paddle->skill2Cooldown - OVERDRIVE_FLOW_TRIM);
                    break;
                case 1: /* Afterburner: arm a charged-return window. The next ball
                         * this paddle returns launches at a fixed speed spike
                         * (consumed by UpdateBall, which skips the usual
                         * speedFactor step) — a genuine one-shot power return. */
                    paddle->afterburnerTimer = AFTERBURNER_WINDOW;
                    /* Flow: same heat pulse, trimming the other side (Q). */
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_PADDLE, .paddleIndex = source,
                        .stat = STAT_PADDLE_HEAT, .op = OP_ADD, .value = OVERDRIVE_FLOW_HEAT,
                        .timeLeft = OVERDRIVE_FLOW_DURATION, .duration = OVERDRIVE_FLOW_DURATION,
                        .color = color, .source = source });
                    if (paddle->skill1Cooldown > 0.0f)
                        paddle->skill1Cooldown = fmaxf(0.0f, paddle->skill1Cooldown - OVERDRIVE_FLOW_TRIM);
                    break;
                case 2: /* Shadow Clone: split the ball into a pair of diverging
                         * visual decoys. The clones are pure illusion — no
                         * collision, no effects, no scoring — so the real ball
                         * keeps all physics untouched. Replaces Afterimage Hit. */
                    DecoySwarmSpawn(&ball->decoys, ball->position, ball->velocity);
                    break;
            }
            break;
        case GRAND_WIZARD:
            switch (ability) {
                case 0: /* Curve Spell: a 3s lateral bend on the ball. Modelled as
                         * a constant acceleration (STAT_BALL_CURVE) consumed by
                         * UpdateBall, so Pong wall bounces stay authoritative. */
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_BALL, .paddleIndex = -1,
                        .stat = STAT_BALL_CURVE, .op = OP_SET, .value = 220.0f,
                        .timeLeft = 3.0f, .duration = 3.0f, .color = color, .source = source });
                    break;
                case 1: /* Time Slow: halve the ball's speed for 2s. */
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_BALL, .paddleIndex = -1,
                        .stat = STAT_BALL_SPEED, .op = OP_MUL, .value = 0.5f,
                        .timeLeft = 2.0f, .duration = 2.0f, .color = color, .source = source });
                    break;
                case 2: /* Arcane Chaos: a stronger 5s lateral bend. Same registry
                         * key as Curve Spell, so re-activating refreshes it. */
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_BALL, .paddleIndex = -1,
                        .stat = STAT_BALL_CURVE, .op = OP_SET, .value = 400.0f,
                        .timeLeft = 5.0f, .duration = 5.0f, .color = color, .source = source });
                    break;
            }
            break;
        case DEFENDER:
            switch (ability) {
                case 0: /* Shield Block: arm a powered reflect for the next
                         * paddle hit. The window is consumed by UpdateBall on
                         * contact and decays in UpdatePaddle when unused. */
                    paddle->shieldTimer = 5.0f;
                    break;
                case 1: /* Wide Paddle: 1.8x height for 4s. */
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_PADDLE, .paddleIndex = source,
                        .stat = STAT_PADDLE_SIZE, .op = OP_MUL, .value = 1.8f,
                        .timeLeft = 4.0f, .duration = 4.0f, .color = color, .source = source });
                    break;
                case 2: /* Iron Fortress: lock the paddle to full height and
                         * mark it immovable for 5s. UpdatePaddle derives
                         * immovable from the marker each frame. */
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_PADDLE, .paddleIndex = source,
                        .stat = STAT_PADDLE_SIZE, .op = OP_SET, .value = FORTRESS_PADDLE_HEIGHT,
                        .timeLeft = 5.0f, .duration = 5.0f, .color = color, .source = source });
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_PADDLE, .paddleIndex = source,
                        .stat = STAT_PADDLE_IMMOVABLE, .op = OP_SET, .value = 1.0f,
                        .timeLeft = 5.0f, .duration = 5.0f, .color = color, .source = source });
                    break;
            }
            break;
        default: break;
    }
}

void CpuTickAbilities(Effects* fx, Ball* ball, Paddle* cpuPaddle, int paddleIndex, float dt) {
    cpuPaddle->cpuDecisionTimer -= dt;
    if (cpuPaddle->cpuDecisionTimer > 0.0f) return;

    /* A decision tick resets the timer no matter the outcome, so it can never
     * drift negative or accumulate. */
    cpuPaddle->cpuDecisionTimer = CPU_DECISION_INTERVAL;

    /* Context gate: only act when the ball is heading toward this paddle
     * (velocity.x shares the sign of paddle.x - ball.x). */
    float towardPaddle = cpuPaddle->position.x - ball->position.x;
    bool approaching = (ball->velocity.x > 0.0f && towardPaddle > 0.0f)
                    || (ball->velocity.x < 0.0f && towardPaddle < 0.0f);
    if (!approaching) return;

    /* Exactly one random roll per decision tick (never in the per-frame path). */
    if (GetRandomValue(0, 99) >= (int)(CPU_DIFFICULTY * 100.0f)) return;

    /* At most one ability fires, preferring the strongest ready one. */
    int ability = -1;
    if (cpuPaddle->ultimateCooldown <= 0.0f) ability = 2;
    else if (cpuPaddle->skill2Cooldown <= 0.0f) ability = 1;
    else if (cpuPaddle->skill1Cooldown <= 0.0f) ability = 0;
    if (ability < 0) return;

    AbilitiesActivate(fx, ball, cpuPaddle, paddleIndex, cpuPaddle->charType, ability);
}
