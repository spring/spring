#ifndef GLOBAL_CONSTANTS_H
#define GLOBAL_CONSTANTS_H

/**
 * @brief maximum world size
 *
 * Defines the maximum world size as 1000000
 */
const int MAX_WORLD_SIZE = 1000000;

/**
 * @brief square size
 *
 * Defines the size of 1 square as 8
 */
const int  SQUARE_SIZE = 8;

/**
 * @brief game speed
 *
 * Defines the game speed as 30
 */
const int GAME_SPEED = 30;

/**
 * @brief max view range
 *
 * Defines the maximum view range as 8000
 */
const int MAX_VIEW_RANGE = 8000;

/**
 * @brief max teams
 *
 * Defines the maximum number of teams
 * as 255 (254 real teams, and an extra slot for the GAIA team)
 */
const int MAX_TEAMS = 255;

/**
 * @brief max players
 *
 * This is the hard limit, which is currently limited by the size of the playerID field in the network, which is 1 byte (=> 255)
 */
const int MAX_PLAYERS = 251;

/**
 * @brief max units
 *
 * Defines the maximum number of untis that may be set as maximum for a game.
 * The real maximum of the game is stored in uh->maxUnits,
 * and may not be higher then this value.
 */
const int MAX_UNITS = 32000;

/**
 * @brief near plane
 *
 * Defines the near plane as 2.8f
 */
const float NEAR_PLANE = 2.8f;

/**
 * @brief randint max
 *
 * Defines the maximum random integer as 0x7fff
 */
const int RANDINT_MAX = 0x7fff;


#endif
