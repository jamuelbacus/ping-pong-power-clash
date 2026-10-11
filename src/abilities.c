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

    switch (type) {
        case SPEEDSTER:
            switch (ability) {
                case 0: /* Dash Return: snap the paddle onto the ball and leave a
                         * short afterimage trail where it flashed from. */
                    paddle->position.y = ball->position.y - paddle->size.y / 2.0f;
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_PADDLE, .paddleIndex = source,
                        .stat = STAT_TRAIL, .op = OP_SET, .value = 1.0f,
                        .timeLeft = 0.3f, .duration = 0.3f, .color = color, .source = source });
                    break;
                case 1: /* Speed Boost: doubled paddle speed for 4s. */
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_PADDLE, .paddleIndex = source,
                        .stat = STAT_PADDLE_SPEED, .op = OP_MUL, .value = 2.0f,
                        .timeLeft = 4.0f, .duration = 4.0f, .color = color, .source = source });
                    break;
                case 2: /* Afterimage Hit: faster ball for 5s plus a 5s ball trail. */
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_BALL, .paddleIndex = source - 1,
                        .stat = STAT_BALL_SPEED, .op = OP_MUL, .value = 1.35f,
                        .timeLeft = 5.0f, .duration = 5.0f, .color = color, .source = source });
                    EffectsPush(fx, (Effect){
                        .active = true, .target = TARGET_BALL, .paddleIndex = source - 1,
                        .stat = STAT_TRAIL, .op = OP_SET, .value = 1.0f,
                        .timeLeft = 5.0f, .duration = 5.0f, .color = color, .source = source });
                    break;
                default: break;
            }
            break;
        case GRAND_WIZARD: /* Task 7 */ break;
        case DEFENDER:     /* Task 8 */ break;
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
