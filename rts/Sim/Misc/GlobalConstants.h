/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
const int SQUARE_SIZE = 8;

/**
 * @brief game speed
 *
 * Defines the game speed as 30
 */
const int GAME_SPEED = 30;

/**
 * @brief unit SlowUpdate rate
 *
 * Defines the interval of CUnit::SlowUpdate calls
 */
const int UNIT_SLOWUPDATE_RATE = 16;

/**
 * @brief team SlowUpdate rate
 *
 * Defines the interval of CTeam::SlowUpdate calls
 */
const int TEAM_SLOWUPDATE_RATE = 32;

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
 * Defines the absolute global maximum number of units allowed to exist in a game.
 * MUST be <= SHRT_MAX ((1 << ((sizeof(short) * 8) - 1)) - 1) because current net
 * code transmits unit ID's as signed shorts. The effective global unit limit is
 * stored in UnitHandler::maxUnits and is always clamped to this value.
 */
const int MAX_UNITS = 32000;

/**
 * @brief max weapons per unit
 */
const int MAX_WEAPONS_PER_UNIT = 32;

/**
 * @brief randint max
 *
 * Defines the maximum random integer as 0x7fff
 */
const int RANDINT_MAX = 0x7fff;


#endif
