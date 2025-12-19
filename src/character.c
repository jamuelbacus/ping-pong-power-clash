/* 
 * character.c
 * implements functions to initialize character data.
 */

#include "character.h"
#include <string.h>

Character characters[8];

void InitCharacters() {
    // Speedster
    strcpy(characters[SPEEDSTER].name, "Speedster");
    strcpy(characters[SPEEDSTER].theme, "Cyber runner / ninja");
    strcpy(characters[SPEEDSTER].playstyle, "Fast reactions, pressure play");
    strcpy(characters[SPEEDSTER].skills[0], "Dash Return");
    strcpy(characters[SPEEDSTER].skills[1], "Speed Boost");
    strcpy(characters[SPEEDSTER].skills[2], "Afterimage Hit");
    strcpy(characters[SPEEDSTER].strength, "Can save impossible shots");
    strcpy(characters[SPEEDSTER].weakness, "Low power, struggles against heavy shots");

    // Power Striker
    strcpy(characters[POWER_STRIKER].name, "Power Striker");
    strcpy(characters[POWER_STRIKER].theme, "Boxer, gladiator, or mech");
    strcpy(characters[POWER_STRIKER].playstyle, "Slow but devastating");
    strcpy(characters[POWER_STRIKER].skills[0], "Smash Shot");
    strcpy(characters[POWER_STRIKER].skills[1], "Heavy Paddle");
    strcpy(characters[POWER_STRIKER].skills[2], "Meteor Strike");
    strcpy(characters[POWER_STRIKER].strength, "Scores fast points");
    strcpy(characters[POWER_STRIKER].weakness, "Poor recovery, easy to outmaneuver");

    // Grand Wizard
    strcpy(characters[GRAND_WIZARD].name, "Grand Wizard");
    strcpy(characters[GRAND_WIZARD].theme, "Mage, sorcerer");
    strcpy(characters[GRAND_WIZARD].playstyle, "Ball manipulation");
    strcpy(characters[GRAND_WIZARD].skills[0], "Curve Spell");
    strcpy(characters[GRAND_WIZARD].skills[1], "Time Slow");
    strcpy(characters[GRAND_WIZARD].skills[2], "Arcane Chaos");
    strcpy(characters[GRAND_WIZARD].strength, "Confuses opponents");
    strcpy(characters[GRAND_WIZARD].weakness, "Requires timing & prediction");

    // Trickster
    strcpy(characters[TRICKSTER].name, "Trickster");
    strcpy(characters[TRICKSTER].theme, "Jester, thief, hacker");
    strcpy(characters[TRICKSTER].playstyle, "Deception");
    strcpy(characters[TRICKSTER].skills[0], "Fake Ball");
    strcpy(characters[TRICKSTER].skills[1], "Reverse Spin");
    strcpy(characters[TRICKSTER].skills[2], "Mirror Field");
    strcpy(characters[TRICKSTER].strength, "Psychological advantage");
    strcpy(characters[TRICKSTER].weakness, "Low raw damage");

    // Defender
    strcpy(characters[DEFENDER].name, "Defender");
    strcpy(characters[DEFENDER].theme, "Knight, guardian, robot shield");
    strcpy(characters[DEFENDER].playstyle, "Defense & endurance");
    strcpy(characters[DEFENDER].skills[0], "Shield Block");
    strcpy(characters[DEFENDER].skills[1], "Wide Paddle");
    strcpy(characters[DEFENDER].skills[2], "Iron Fortress");
    strcpy(characters[DEFENDER].strength, "Extremely hard to score against");
    strcpy(characters[DEFENDER].weakness, "Limited offensive options");

    // Elementalist
    strcpy(characters[ELEMENTALIST].name, "Elementalist");
    strcpy(characters[ELEMENTALIST].theme, "Elemental warrior");
    strcpy(characters[ELEMENTALIST].playstyle, "Status effects");
    strcpy(characters[ELEMENTALIST].skills[0], "Fire Ball");
    strcpy(characters[ELEMENTALIST].skills[1], "Ice Ball");
    strcpy(characters[ELEMENTALIST].skills[2], "Storm Surge");
    strcpy(characters[ELEMENTALIST].strength, "Adaptable");
    strcpy(characters[ELEMENTALIST].weakness, "Requires switching elements wisely");

    // Sniper
    strcpy(characters[SNIPER].name, "Sniper");
    strcpy(characters[SNIPER].theme, "Sharpshooter, archer");
    strcpy(characters[SNIPER].playstyle, "Accuracy-based");
    strcpy(characters[SNIPER].skills[0], "Perfect Angle");
    strcpy(characters[SNIPER].skills[1], "Piercing Shot");
    strcpy(characters[SNIPER].skills[2], "Deadeye");
    strcpy(characters[SNIPER].strength, "Rewards skill");
    strcpy(characters[SNIPER].weakness, "High skill ceiling");

    // Chaos Bot
    strcpy(characters[CHAOS_BOT].name, "Chaos Bot");
    strcpy(characters[CHAOS_BOT].theme, "Glitched AI, broken machine");
    strcpy(characters[CHAOS_BOT].playstyle, "Unpredictable");
    strcpy(characters[CHAOS_BOT].skills[0], "Glitch Bounce");
    strcpy(characters[CHAOS_BOT].skills[1], "Lag Spike");
    strcpy(characters[CHAOS_BOT].skills[2], "System Error");
    strcpy(characters[CHAOS_BOT].strength, "Unpredictable fun");
    strcpy(characters[CHAOS_BOT].weakness, "Unreliable control");
}