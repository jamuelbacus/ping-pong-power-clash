/* tests/test_speedster.c */
#include "abilities.h"
#include "effects.h"
#include "ball.h"
#include "paddle.h"
#include "character.h"
#include "trail.h"
#include "layout.h"
#include "overdrive.h"
#include <stdio.h>
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

/* Duration of the first active effect matching (target, paddleIndex, stat),
 * or -1 when no such effect exists. */
static float EffectDurationOf(const Effects* fx, EffectTarget t, int paddleIndex, EffectStat s) {
    for (int i = 0; i < EFFECT_CAPACITY; i++) {
        const Effect* e = &fx->items[i];
        if (e->active && e->target == t && e->paddleIndex == paddleIndex && e->stat == s)
            return e->duration;
    }
    return -1.0f;
}

int main(void) {
    InitCharacters();
    Effects fx; EffectsInit(&fx);
    Ball b; InitBall(&b, 480, 400, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,SPEEDSTER);

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 0);   /* Dash Return */
    CHECK(pad.position.y == 400.0f - PADDLE_HEIGHT/2, "dash snaps paddle to ball Y");
    CHECK(EffectsHas(&fx, TARGET_PADDLE, 0, STAT_TRAIL), "dash arms a paddle trail marker");
    CHECK(EffectDurationOf(&fx, TARGET_PADDLE, 0, STAT_TRAIL) == 0.3f, "dash trail lasts 0.3s");

    /* True afterimage: Dash Return parks a footprint at the pre-snap position. */
    {
        TrailGhost ghosts[TRAIL_MAX_GHOSTS];
        int n = TrailActiveGhosts(&pad.trail, TRAIL_MAX_GHOSTS, ghosts);
        bool foundOrigin = false;
        for (int i = 0; i < n; i++)
            if (ghosts[i].pos.x == 50.0f && ghosts[i].pos.y == 200.0f) foundOrigin = true;
        CHECK(foundOrigin, "dash leaves a ghost at the pre-snap position (200)");
        /* A frame of UpdatePaddle with the trail marker live records the new spot. */
        UpdatePaddle(&pad, &fx, 0, 0.1f, 540, &b);
        n = TrailActiveGhosts(&pad.trail, TRAIL_MAX_GHOSTS, ghosts);
        CHECK(n >= 2, "live dash trail records the destination on the next frame");
    }

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 1);   /* Speed Boost */
    CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_SPEED, 300.0f) == 600.0f, "speed boost x2");

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 2);   /* Afterimage Hit */
    CHECK(EffectsApply(&fx, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 405.0f, "afterimage speed x1.35");
    CHECK(EffectsHas(&fx, TARGET_BALL, -1, STAT_TRAIL), "afterimage arms a ball trail marker");
    CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_TRAIL) == 5.0f, "afterimage ball trail lasts 5s");

    /* Ball effects must land at the index the ball actually reads (-1),
     * independent of which paddle activated the ability. */
    Effects fx2; EffectsInit(&fx2);
    Ball b2; InitBall(&b2, 480, 400, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle pad2; InitPaddle(&pad2, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000,500,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);

    AbilitiesActivate(&fx2, &b2, &pad2, 1, SPEEDSTER, 2);   /* Afterimage Hit from paddle index 1 */
    CHECK(EffectsApply(&fx2, TARGET_BALL, -1, STAT_BALL_SPEED, 300.0f) == 405.0f,
          "afterimage speed lands at the ball's read index for paddleIndex=1");
    CHECK(EffectsHas(&fx2, TARGET_BALL, -1, STAT_TRAIL),
          "afterimage ball trail lands at the ball's read index for paddleIndex=1");

    /* --- Overdrive: heat model + Flow (Task 4) --- */
    {
        Effects fx; EffectsInit(&fx);
        Ball b; InitBall(&b, 480, 270, BALL_SIZE, 360.5551f, 0.0f, BALL_COLOR);  /* serve speed */
        Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,SPEEDSTER);

        /* Rally: serve-speed ball -> heat 0 -> maxSpeed 500. */
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 499.99f && pad.maxSpeed < 500.01f, "serve-speed ball: heat 0, maxSpeed 500");

        /* Rally: fast ball (2x base) -> heat 1 -> 500 * 1.5 = 750. */
        b.velocity.x = 721.1102f; b.velocity.y = 0.0f;
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 749.9f && pad.maxSpeed <= 750.1f, "2x-speed ball: heat caps at 1 -> 750");

        /* Rally clamps from above and from below. */
        b.velocity.x = 900.0f; b.velocity.y = 0.0f;
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 749.9f && pad.maxSpeed <= 750.1f, "extremely fast ball still caps at 750");
        b.velocity.x = 167.0f; b.velocity.y = 0.0f;
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 499.99f && pad.maxSpeed < 500.01f, "slow ball: rally floors at 0 -> 500");

        /* Flow: Q pushes +0.35 heat and trims E's cooldown. */
        pad.skill2Cooldown = SKILL2_COOLDOWN;                 /* 7s */
        AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 0);    /* Dash Return */
        CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT, 0.0f) > 0.3499f &&
              EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT, 0.0f) < 0.3501f,
              "Q arms 0.35 heat");
        CHECK(pad.skill2Cooldown == 6.0f, "Q trims E's cooldown by 1s");
        /* Heat at mid ball speed: 500 * (1 + 0.5*0.35) = 587.5. */
        b.velocity.x = 360.5551f; b.velocity.y = 0.0f;
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 587.4f && pad.maxSpeed < 587.6f, "heat 0.35 -> maxSpeed 587.5");

        /* E arms the same 0.35 heat (refresh), trims Q, and its x2 stacks. */
        pad.skill1Cooldown = SKILL1_COOLDOWN;
        AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 1);    /* Speed Boost */
        CHECK(pad.skill1Cooldown == 4.0f, "E trims Q's cooldown by 1s");
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);       /* heat 0.35 + x2 -> 1175 */
        CHECK(pad.maxSpeed > 1174.9f && pad.maxSpeed < 1175.1f, "Speed Boost x2 stacks on heat (1175)");

        /* Flow expires. */
        EffectsUpdate(&fx, 1.6f);
        CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT, 0.0f) == 0.0f,
              "flow heat expires");

        /* Review focus 2: rally heat + flow heat must clamp at full heat (750),
         * never sum past it (837.5). Wipe the still-live x2 Speed Boost first so
         * this reads the heat clamp on its own. */
        EffectsClear(&fx);
        b.velocity.x = 900.0f; b.velocity.y = 0.0f;
        EffectsPush(&fx, (Effect){ .active=true, .target=TARGET_PADDLE, .paddleIndex=0,
            .stat=STAT_PADDLE_HEAT, .op=OP_ADD, .value=OVERDRIVE_FLOW_HEAT,
            .timeLeft=OVERDRIVE_FLOW_DURATION, .duration=OVERDRIVE_FLOW_DURATION,
            .color=BLUE, .source=0 });
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 749.9f && pad.maxSpeed <= 750.1f, "rally+flow clamps at full heat (750, never 837.5)");

        /* Review focus 3: a sub-1s opposing cooldown trims to exactly 0, never
         * below. (E's own skill2 assignment only happens on ability 1, so this
         * Q trim on a 0.5s skill2 is clean.) */
        pad.skill2Cooldown = 0.5f;
        AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 0);   /* Q trims skill2Cooldown */
        CHECK(pad.skill2Cooldown == 0.0f, "flow trim clamps at 0, never negative");
    }

    /* --- Overdrive: heat is a Speedster-only passive --- */
    {
        Effects fx; EffectsInit(&fx);
        Ball b; InitBall(&b, 480, 270, BALL_SIZE, 721.1102f, 0.0f, BALL_COLOR);
        Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,GRAND_WIZARD);
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 499.99f && pad.maxSpeed < 500.01f,
              "non-Speedster ignores heat entirely");
    }

    /* --- Overdrive Clutch (Task 5) --- */
    {
        Effects fx; EffectsInit(&fx);
        Ball b; InitBall(&b, 55.0f, 215.0f, BALL_SIZE, -360.0f, 0.0f, BALL_COLOR); /* tip of pad */
        Paddle p1; InitPaddle(&p1, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,SPEEDSTER);
        Paddle p2; InitPaddle(&p2, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, RED, true, 4000,500,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
        UpdateBall(&b, &fx, 1.0f/60.0f, 960, 540, &p1, &p2);   /* hitPos 0.15 -> clutch */
        CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT, 0.0f) == 1.0f,
              "tip return spikes heat to 1");
        CHECK(EffectDurationOf(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT) == OVERDRIVE_CLUTCH_DURATION,
              "clutch heat spike lasts 0.5s");
        CHECK(EffectsHas(&fx, TARGET_BALL, -1, STAT_TRAIL), "clutch arms the ball afterimage");
        CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_TRAIL) == OVERDRIVE_CLUTCH_TRAIL,
              "clutch ball trail lasts 0.5s");

        /* Center hit: no clutch. */
        Effects fx2; EffectsInit(&fx2);
        Ball b2; InitBall(&b2, 55.0f, 250.0f, BALL_SIZE, -360.0f, 0.0f, BALL_COLOR); /* hitPos 0.5 */
        UpdateBall(&b2, &fx2, 1.0f/60.0f, 960, 540, &p1, &p2);
        CHECK(!EffectsHas(&fx2, TARGET_PADDLE, 0, STAT_PADDLE_HEAT) &&
              !EffectsHas(&fx2, TARGET_BALL, -1, STAT_TRAIL), "center return does not clutch");

        /* Non-Speedster: tip return does not clutch. */
        Effects fx3; EffectsInit(&fx3);
        Ball b3; InitBall(&b3, 55.0f, 215.0f, BALL_SIZE, -360.0f, 0.0f, BALL_COLOR);
        Paddle wz; InitPaddle(&wz, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,500,0.9f,KEY_W,KEY_S,GRAND_WIZARD);
        UpdateBall(&b3, &fx3, 1.0f/60.0f, 960, 540, &wz, &p2);
        CHECK(!EffectsHas(&fx3, TARGET_BALL, -1, STAT_TRAIL), "non-Speedster never clutches");

        /* Index-1 side mirrors index 0. */
        Effects fx4; EffectsInit(&fx4);
        Ball b4; InitBall(&b4, 885.0f, 280.0f, BALL_SIZE, 360.0f, 0.0f, BALL_COLOR); /* hitPos 0.8 */
        Paddle q2; InitPaddle(&q2, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, RED, true, 4000,500,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
        UpdateBall(&b4, &fx4, 1.0f/60.0f, 960, 540, &p1, &q2);
        CHECK(EffectsApply(&fx4, TARGET_PADDLE, 1, STAT_PADDLE_HEAT, 0.0f) == 1.0f,
              "right paddle clutch keys effects at index 1");

        /* C3: Clutch OP_SET replaces a live Flow ADD on the same heat slot. */
        {
            Effects fx5; EffectsInit(&fx5);
            Ball b5; InitBall(&b5, 55.0f, 215.0f, BALL_SIZE, -360.0f, 0.0f, BALL_COLOR);
            EffectsPush(&fx5, (Effect){ .active=true, .target=TARGET_PADDLE, .paddleIndex=0,
                .stat=STAT_PADDLE_HEAT, .op=OP_ADD, .value=OVERDRIVE_FLOW_HEAT,
                .timeLeft=OVERDRIVE_FLOW_DURATION, .duration=OVERDRIVE_FLOW_DURATION,
                .color=BLUE, .source=0 });   /* live Flow */
            UpdateBall(&b5, &fx5, 1.0f/60.0f, 960, 540, &p1, &p2);   /* tip clutch */
            CHECK(EffectsApply(&fx5, TARGET_PADDLE, 0, STAT_PADDLE_HEAT, 0.0f) == 1.0f,
                  "clutch SET overrides live flow ADD");
            CHECK(EffectDurationOf(&fx5, TARGET_PADDLE, 0, STAT_PADDLE_HEAT) == OVERDRIVE_CLUTCH_DURATION,
                  "clutch SET slot carries the 0.5s window, not flow's 1.5s");
        }
    }

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
