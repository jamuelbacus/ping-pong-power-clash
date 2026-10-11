/*
 * trail.c
 * pure position-history ring: record positions, age them, expire them.
 * No allocation; the ring wraps modulo TRAIL_MAX_GHOSTS.
 */

#include "trail.h"

void TrailHistoryInit(TrailHistory* h)
{
    h->head = 0;
    for (int i = 0; i < TRAIL_MAX_GHOSTS; i++)
        h->ghosts[i].live = false;
}

void TrailHistoryRecord(TrailHistory* h, Vector2 pos)
{
    h->ghosts[h->head].pos = pos;
    h->ghosts[h->head].age = 0.0f;
    h->ghosts[h->head].live = true;
    h->head = (h->head + 1) % TRAIL_MAX_GHOSTS;
}

void TrailHistoryUpdate(TrailHistory* h, float dt)
{
    for (int i = 0; i < TRAIL_MAX_GHOSTS; i++) {
        if (!h->ghosts[i].live)
            continue;
        h->ghosts[i].age += dt;
        if (h->ghosts[i].age >= TRAIL_MAX_AGE)
            h->ghosts[i].live = false;
    }
}

int TrailActiveGhosts(const TrailHistory* h, int maxCount, TrailGhost* out)
{
    int count = 0;
    for (int i = 1; i <= TRAIL_MAX_GHOSTS && count < maxCount; i++) {
        int idx = (h->head - i + TRAIL_MAX_GHOSTS) % TRAIL_MAX_GHOSTS;
        if (h->ghosts[idx].live) {
            out[count] = h->ghosts[idx];
            count++;
        }
    }
    return count;
}
