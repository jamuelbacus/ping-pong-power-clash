/* tests/test_trail.c */
#include "trail.h"
#include <stdio.h>
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails = 1; } } while (0)

int main(void) {
    TrailHistory h; TrailHistoryInit(&h);
    TrailGhost out[TRAIL_MAX_GHOSTS];

    CHECK(TrailActiveGhosts(&h, TRAIL_MAX_GHOSTS, out) == 0, "init has no ghosts");

    TrailHistoryRecord(&h, (Vector2){100.0f, 200.0f});
    CHECK(TrailActiveGhosts(&h, TRAIL_MAX_GHOSTS, out) == 1, "one record -> one ghost");
    CHECK(out[0].pos.x == 100.0f && out[0].pos.y == 200.0f && out[0].age == 0.0f,
          "ghost keeps recorded position and starts at age 0");

    TrailHistoryRecord(&h, (Vector2){110.0f, 210.0f});
    CHECK(TrailActiveGhosts(&h, TRAIL_MAX_GHOSTS, out) == 2, "two records -> two ghosts");
    CHECK(out[0].pos.x == 110.0f && out[1].pos.x == 100.0f, "youngest ghost first");

    /* Expiry: a ghost aged to TRAIL_MAX_AGE is gone. */
    TrailHistoryUpdate(&h, TRAIL_MAX_AGE);
    CHECK(TrailActiveGhosts(&h, TRAIL_MAX_GHOSTS, out) == 0, "ghosts expire at TRAIL_MAX_AGE");

    /* Ring capacity: oldest records are dropped, never allocate. */
    TrailHistory big; TrailHistoryInit(&big);
    for (int i = 0; i < TRAIL_MAX_GHOSTS + 3; i++)
        TrailHistoryRecord(&big, (Vector2){ (float)i, 0.0f });
    int n = TrailActiveGhosts(&big, TRAIL_MAX_GHOSTS, out);
    CHECK(n == TRAIL_MAX_GHOSTS, "ring keeps at most TRAIL_MAX_GHOSTS");
    CHECK(out[n-1].pos.x == 3.0f, "ring evicts the 3 oldest, keeps surviving positions 3..32");

    /* Immutable under update: positions never move with any entity. */
    TrailHistory h2; TrailHistoryInit(&h2);
    TrailHistoryRecord(&h2, (Vector2){40.0f, 50.0f});
    TrailHistoryUpdate(&h2, 0.1f);
    CHECK(TrailActiveGhosts(&h2, TRAIL_MAX_GHOSTS, out) == 1 && out[0].pos.x == 40.0f,
          "update ages but never moves a ghost");

    if (fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n"); return 0;
}
