#include "utils.h"

void UpdateTimer(float* timer, float deltaTime) {
    if (*timer > 0) {
        *timer -= deltaTime;
        if (*timer < 0) *timer = 0;
    }
}