/* 
 * character.h
 * defines the Character struct and declares functions to initialize character data.
 */

#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"

typedef enum CharacterType {
    SPEEDSTER,
    POWER_STRIKER,
    GRAND_WIZARD,
    TRICKSTER,
    DEFENDER,
    ELEMENTALIST,
    SNIPER,
    CHAOS_BOT
} CharacterType;

typedef struct Character {
    char name[20];
    char theme[50];
    char playstyle[50];
    char skills[3][50]; // 0: skill1, 1: skill2, 2: ultimate
    char strength[50];
    char weakness[50];
} Character;

extern Character characters[8];

void InitCharacters();

#endif