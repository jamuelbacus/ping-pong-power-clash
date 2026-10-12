/* tests/test_speedster.c */
#include "abilities.h"
#include "effects.h"
#include "ball.h"
#include "paddle.h"
#include "character.h"
#include "trail.h"
#include "decoy.h"
#include "layout.h"
#include "overdrive.h"
#include <stdio.h>
#include <math.h>
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
    Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);

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

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 1);   /* Afterburner */
    CHECK(pad.afterburnerTimer == AFTERBURNER_WINDOW, "E arms the Afterburner window");
    CHECK(!EffectsHas(&fx, TARGET_PADDLE, 0, STAT_PADDLE_SPEED),
          "Afterburner no longer buffs paddle speed");

    AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 2);   /* Shadow Clone */
    CHECK(DecoySwarmActiveCount(&b.decoys) == DECOY_SPAWN_COUNT,
          "ultimate spawns the Shadow Clone burst");
    CHECK(b.decoys.items[0].position.x == b.position.x && b.decoys.items[0].position.y == b.position.y,
          "clones spawn at the ball's position");
    CHECK(!EffectsHas(&fx, TARGET_BALL, -1, STAT_BALL_SPEED),
          "Shadow Clone no longer buffs ball speed");
    CHECK(!EffectsHas(&fx, TARGET_BALL, -1, STAT_TRAIL),
          "Shadow Clone no longer arms a ball trail");

    /* The burst belongs to whichever ball was passed in, from either paddle. */
    Effects fx2; EffectsInit(&fx2);
    Ball b2; InitBall(&b2, 480, 400, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle pad2; InitPaddle(&pad2, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, GREEN, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);

    AbilitiesActivate(&fx2, &b2, &pad2, 1, SPEEDSTER, 2);   /* Shadow Clone from paddle index 1 */
    CHECK(DecoySwarmActiveCount(&b2.decoys) == DECOY_SPAWN_COUNT,
          "Shadow Clone spawns for paddle index 1 too");

    /* Non-Speedster ultimate does not clone. */
    Effects fx3; EffectsInit(&fx3);
    Ball b3; InitBall(&b3, 480, 400, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
    Paddle wz; InitPaddle(&wz, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,GRAND_WIZARD);
    AbilitiesActivate(&fx3, &b3, &wz, 0, GRAND_WIZARD, 2);
    CHECK(DecoySwarmActiveCount(&b3.decoys) == 0, "non-Speedster ultimate never clones");

    /* --- Overdrive: heat model + Flow (Task 4) --- */
    {
        Effects fx; EffectsInit(&fx);
        Ball b; InitBall(&b, 480, 270, BALL_SIZE, 360.5551f, 0.0f, BALL_COLOR);  /* serve speed */
        Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);

        /* Rally: serve-speed ball -> heat 0 -> maxSpeed 320. */
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 319.99f && pad.maxSpeed < 320.01f, "serve-speed ball: heat 0, maxSpeed 320");

        /* Rally: fast ball (2x base) -> heat 1 -> 320 * 1.5 = 480. */
        b.velocity.x = 721.1102f; b.velocity.y = 0.0f;
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 479.9f && pad.maxSpeed <= 480.1f, "2x-speed ball: heat caps at 1 -> 480");

        /* Rally clamps from above and from below. */
        b.velocity.x = 900.0f; b.velocity.y = 0.0f;
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 479.9f && pad.maxSpeed <= 480.1f, "extremely fast ball still caps at 480");
        b.velocity.x = 167.0f; b.velocity.y = 0.0f;
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 319.99f && pad.maxSpeed < 320.01f, "slow ball: rally floors at 0 -> 320");

        /* Flow: Q pushes +0.35 heat and trims E's cooldown. */
        pad.skill2Cooldown = SKILL2_COOLDOWN;                 /* 7s */
        AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 0);    /* Dash Return */
        CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT, 0.0f) > 0.3499f &&
              EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT, 0.0f) < 0.3501f,
              "Q arms 0.35 heat");
        CHECK(pad.skill2Cooldown == 6.0f, "Q trims E's cooldown by 1s");
        /* Heat at mid ball speed: 320 * (1 + 0.5*0.35) = 376. */
        b.velocity.x = 360.5551f; b.velocity.y = 0.0f;
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 375.9f && pad.maxSpeed < 376.1f, "heat 0.35 -> maxSpeed 376");

        /* E arms the same 0.35 heat (refresh) and trims Q. Afterburner no
         * longer touches paddle speed, so heat alone drives maxSpeed. */
        pad.skill1Cooldown = SKILL1_COOLDOWN;
        AbilitiesActivate(&fx, &b, &pad, 0, SPEEDSTER, 1);    /* Afterburner */
        CHECK(pad.skill1Cooldown == 4.0f, "E trims Q's cooldown by 1s");
        CHECK(pad.afterburnerTimer == AFTERBURNER_WINDOW, "E arms the Afterburner window");
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);       /* heat 0.35 -> 376 */
        CHECK(pad.maxSpeed > 375.9f && pad.maxSpeed < 376.1f,
              "Afterburner leaves paddle speed to heat (376)");

        /* Flow expires. */
        EffectsUpdate(&fx, 1.6f);
        CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT, 0.0f) == 0.0f,
              "flow heat expires");

        /* Review focus 2: rally heat + flow heat must clamp at full heat (480),
         * never sum past it (552). Wipe any live Flow so this reads the heat
         * clamp on its own. */
        EffectsClear(&fx);
        b.velocity.x = 900.0f; b.velocity.y = 0.0f;
        EffectsPush(&fx, (Effect){ .active=true, .target=TARGET_PADDLE, .paddleIndex=0,
            .stat=STAT_PADDLE_HEAT, .op=OP_ADD, .value=OVERDRIVE_FLOW_HEAT,
            .timeLeft=OVERDRIVE_FLOW_DURATION, .duration=OVERDRIVE_FLOW_DURATION,
            .color=BLUE, .source=0 });
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 479.9f && pad.maxSpeed <= 480.1f, "rally+flow clamps at full heat (480, never 552)");

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
        Paddle pad; InitPaddle(&pad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,GRAND_WIZARD);
        UpdatePaddle(&pad, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(pad.maxSpeed > 319.99f && pad.maxSpeed < 320.01f,
              "non-Speedster ignores heat entirely");
    }

    /* --- Overdrive Clutch (Task 5) --- */
    {
        Effects fx; EffectsInit(&fx);
        Ball b; InitBall(&b, 55.0f, 200.0f + 0.15f * PADDLE_HEIGHT, BALL_SIZE, -360.0f, 0.0f, BALL_COLOR); /* tip of pad */
        Paddle p1; InitPaddle(&p1, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);
        Paddle p2; InitPaddle(&p2, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, RED, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
        UpdateBall(&b, &fx, 1.0f/60.0f, 960, 540, &p1, &p2);   /* hitPos 0.15 -> clutch */
        CHECK(EffectsApply(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT, 0.0f) == 1.0f,
              "tip return spikes heat to 1");
        CHECK(EffectDurationOf(&fx, TARGET_PADDLE, 0, STAT_PADDLE_HEAT) == OVERDRIVE_CLUTCH_DURATION,
              "clutch heat spike lasts 0.5s");
        CHECK(EffectsHas(&fx, TARGET_BALL, -1, STAT_TRAIL), "clutch arms the ball afterimage");
        CHECK(EffectDurationOf(&fx, TARGET_BALL, -1, STAT_TRAIL) == OVERDRIVE_CLUTCH_TRAIL,
              "clutch ball trail lasts 0.5s");

        /* Clutch heat must reach the consumer (Review Focus 6): OP_SET 1.0 -> maxSpeed 480. */
        b.velocity.x = -360.0f; b.velocity.y = 0.0f;   /* rally floor: heat is pure Clutch */
        UpdatePaddle(&p1, &fx, 0, 1.0f/60.0f, 540, &b);
        CHECK(p1.maxSpeed > 479.9f && p1.maxSpeed <= 480.1f,
              "clutch heat reaches maxSpeed through UpdatePaddle (480)");

        /* Center hit: no clutch. */
        Effects fx2; EffectsInit(&fx2);
        Ball b2; InitBall(&b2, 55.0f, 200.0f + 0.5f * PADDLE_HEIGHT, BALL_SIZE, -360.0f, 0.0f, BALL_COLOR); /* hitPos 0.5 */
        UpdateBall(&b2, &fx2, 1.0f/60.0f, 960, 540, &p1, &p2);
        CHECK(!EffectsHas(&fx2, TARGET_PADDLE, 0, STAT_PADDLE_HEAT) &&
              !EffectsHas(&fx2, TARGET_BALL, -1, STAT_TRAIL), "center return does not clutch");

        /* Non-Speedster: tip return does not clutch. */
        Effects fx3; EffectsInit(&fx3);
        Ball b3; InitBall(&b3, 55.0f, 200.0f + 0.15f * PADDLE_HEIGHT, BALL_SIZE, -360.0f, 0.0f, BALL_COLOR);
        Paddle wz; InitPaddle(&wz, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,GRAND_WIZARD);
        UpdateBall(&b3, &fx3, 1.0f/60.0f, 960, 540, &wz, &p2);
        CHECK(!EffectsHas(&fx3, TARGET_BALL, -1, STAT_TRAIL), "non-Speedster never clutches");

        /* Index-1 side mirrors index 0. */
        Effects fx4; EffectsInit(&fx4);
        Ball b4; InitBall(&b4, 885.0f, 200.0f + 0.8f * PADDLE_HEIGHT, BALL_SIZE, 360.0f, 0.0f, BALL_COLOR); /* hitPos 0.8 */
        Paddle q2; InitPaddle(&q2, 890, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 250, RED, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
        UpdateBall(&b4, &fx4, 1.0f/60.0f, 960, 540, &p1, &q2);
        CHECK(EffectsApply(&fx4, TARGET_PADDLE, 1, STAT_PADDLE_HEAT, 0.0f) == 1.0f,
              "right paddle clutch keys effects at index 1");

        /* C3: Clutch OP_SET replaces a live Flow ADD on the same heat slot. */
        {
            Effects fx5; EffectsInit(&fx5);
            Ball b5; InitBall(&b5, 55.0f, 200.0f + 0.15f * PADDLE_HEIGHT, BALL_SIZE, -360.0f, 0.0f, BALL_COLOR);
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

    /* --- Afterburner: charged return (arm, consume, spike, cap, decay) --- */
    {
        Effects fx; EffectsInit(&fx);
        Ball b; InitBall(&b, 62, 250, BALL_SIZE, -BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
        Paddle p1; InitPaddle(&p1, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);
        Paddle p2; InitPaddle(&p2, 890, 250, PADDLE_WIDTH, PADDLE_HEIGHT, 300, RED, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);

        AbilitiesActivate(&fx, &b, &p1, 0, SPEEDSTER, 1);   /* arm */
        CHECK(p1.afterburnerTimer == AFTERBURNER_WINDOW, "E arms the Afterburner window");
        float pre = sqrtf(b.baseVelocity.x * b.baseVelocity.x + b.baseVelocity.y * b.baseVelocity.y);
        float preFactor = b.speedFactor;
        UpdateBall(&b, &fx, 1.0f/60.0f, 960, 540, &p1, &p2);   /* charged contact */
        CHECK(p1.afterburnerTimer == 0.0f, "Afterburner is consumed on hit");
        CHECK(b.speedFactor == preFactor, "charged return does not creep speedFactor");
        float post = sqrtf(b.baseVelocity.x * b.baseVelocity.x + b.baseVelocity.y * b.baseVelocity.y);
        CHECK(fabsf(post - pre * AFTERBURNER_SPEED_MULT) < 0.01f,
              "charged return launches at base speed x AFTERBURNER_SPEED_MULT");

        /* Without the window, the same contact keeps the normal creep step. */
        Effects ufx; EffectsInit(&ufx);
        Ball ub; InitBall(&ub, 62, 250, BALL_SIZE, -BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
        Paddle u1; InitPaddle(&u1, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);
        Paddle u2; InitPaddle(&u2, 890, 250, PADDLE_WIDTH, PADDLE_HEIGHT, 300, RED, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
        float uFactor = ub.speedFactor;
        UpdateBall(&ub, &ufx, 1.0f/60.0f, 960, 540, &u1, &u2);
        CHECK(ub.speedFactor == uFactor * 1.005f, "an uncharged return keeps the normal creep step");

        /* The charged return respects the game's ball speed cap. */
        Effects cfx; EffectsInit(&cfx);
        Ball cb; InitBall(&cb, 62, 250, BALL_SIZE, -721.1102f, 0.0f, BALL_COLOR);
        Paddle c1; InitPaddle(&c1, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);
        Paddle c2; InitPaddle(&c2, 890, 250, PADDLE_WIDTH, PADDLE_HEIGHT, 300, RED, true, 4000,320,0.9f,KEY_UP,KEY_DOWN,SPEEDSTER);
        AbilitiesActivate(&cfx, &cb, &c1, 0, SPEEDSTER, 1);
        UpdateBall(&cb, &cfx, 1.0f/60.0f, 960, 540, &c1, &c2);
        float capped = sqrtf(cb.baseVelocity.x * cb.baseVelocity.x + cb.baseVelocity.y * cb.baseVelocity.y);
        CHECK(capped <= BALL_MAX_VELOCITY + 0.01f, "charged return never exceeds BALL_MAX_VELOCITY");

        /* The armed window decays in UpdatePaddle and expires at exactly 0. */
        Effects dfx; EffectsInit(&dfx);
        Ball db; InitBall(&db, 480, 270, BALL_SIZE, BALL_BASE_VELOCITY_X, BALL_BASE_VELOCITY_Y, BALL_COLOR);
        Paddle dpad; InitPaddle(&dpad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,SPEEDSTER);
        AbilitiesActivate(&dfx, &db, &dpad, 0, SPEEDSTER, 1);
        UpdatePaddle(&dpad, &dfx, 0, 1.0f/60.0f, 540, &db);
        CHECK(dpad.afterburnerTimer < AFTERBURNER_WINDOW, "Afterburner window decays over time");
        for (int i = 0; i < 200; i++) UpdatePaddle(&dpad, &dfx, 0, 1.0f/60.0f, 540, &db);
        CHECK(dpad.afterburnerTimer == 0.0f, "Afterburner window expires at zero");

        /* Only the Speedster ever arms the window. */
        Effects nfx; EffectsInit(&nfx);
        Paddle npad; InitPaddle(&npad, 50, 200, PADDLE_WIDTH, PADDLE_HEIGHT, 300, BLUE, false, 4000,320,0.9f,KEY_W,KEY_S,GRAND_WIZARD);
        AbilitiesActivate(&nfx, &db, &npad, 0, GRAND_WIZARD, 1);   /* Time Slow */
        CHECK(npad.afterburnerTimer == 0.0f, "non-Speedster never arms the Afterburner window");
    }

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
