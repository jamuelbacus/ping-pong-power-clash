/*
 * trail.h
 * fixed-capacity "true afterimage" position-history ring.
 * Records where an entity has been; ghosts decay by age, never move.
 */

#ifndef TRAIL_H
#define TRAIL_H

#include <stdbool.h>
#include "raylib.h"

#define TRAIL_MAX_GHOSTS 30
#define TRAIL_MAX_AGE 0.5f

typedef struct TrailGhost { Vector2 pos; float age; bool live; } TrailGhost;
typedef struct TrailHistory { TrailGhost ghosts[TRAIL_MAX_GHOSTS]; int head; } TrailHistory;

void TrailHistoryInit(TrailHistory* h);
void TrailHistoryRecord(TrailHistory* h, Vector2 pos);
void TrailHistoryUpdate(TrailHistory* h, float dt);
int TrailActiveGhosts(const TrailHistory* h, int maxCount, TrailGhost* out);

#endif // TRAIL_H
