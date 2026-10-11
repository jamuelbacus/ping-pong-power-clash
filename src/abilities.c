/*
 * abilities.c
 * Ability activation dispatch and the CPU decision policy. No dynamic
 * allocation: everything goes through the fixed-capacity Effects registry.
 */

#include "abilities.h"
#include "layout.h"

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

    /* Per-character effect bodies arrive in Tasks 6-8 (Speedster / Grand
     * Wizard / Defender). This dispatch is the scaffold they plug into. */
    switch (type) {
        case SPEEDSTER:    /* Task 6 */ break;
        case GRAND_WIZARD: /* Task 7 */ break;
        case DEFENDER:     /* Task 8 */ break;
        default: break;
    }

    (void)fx;
    (void)ball;
    (void)color;
    (void)source;
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
