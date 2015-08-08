/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_CONSTANTS_H
#define _GLOBAL_CONSTANTS_H

/**
 * @brief square size
 *
 * Defines the size of 1 heightmap square as 8 elmos.
 */
static constexpr int SQUARE_SIZE = 8;

/**
 * @brief footprint scale
 *
 * Multiplier for {Unit, Feature, Move}Def footprint sizes which are
 * assumed to be expressed in "TA units". The resolution of Spring's
 * blocking-map is twice that of TA's; a "TA-footprint" square covers
 * SQUARE_SIZE*2 x SQUARE_SIZE*2 elmos.
 */
static constexpr int SPRING_FOOTPRINT_SCALE = 2;

/**
 * conversion factor from elmos to meters
 */
static constexpr float ELMOS_TO_METERS = 1.0f / SQUARE_SIZE;

/**
 * @brief game speed
 *
 * Defines the game-/sim-frames per second.
 */
static constexpr int GAME_SPEED = 30;

/**
 * @brief unit SlowUpdate rate
 *
 * Defines the interval of SlowUpdate calls in sim-frames.
 */
static constexpr int UNIT_SLOWUPDATE_RATE = 15;

/**
 * @brief team SlowUpdate rate
 *
 * Defines the interval of CTeam::SlowUpdate calls in sim-frames.
 */
static constexpr int TEAM_SLOWUPDATE_RATE = 30;

/**
 * @brief los SlowUpdate rate
 *
 * Defines the interval of the LosHandler updates on terraform changes.
 */
static constexpr int LOS_TERRAFORM_SLOWUPDATE_RATE = 15;


/**
 * @brief max teams
 *
 * Defines the maximum number of teams as 255
 * (254 real teams, and an extra slot for the GAIA team).
 */
static constexpr int MAX_TEAMS = 255;

/**
 * @brief max players
 *
 * This is the hard limit.
 * It is currently 251. as it isrestricted by the size of the player-ID field
 * in the network, which is 1 byte (=> 255), plus 4 slots reserved for special
 * purposes, resulting in 251.
 */
static constexpr int MAX_PLAYERS = 251;

/**
 * @brief max AIs
 *
 * This is the hard limit.
 * It is currently 255. as it isrestricted by the size of the ai-ID field
 * in the network, which is 1 byte (=> 256), with the value 255 reserved for
 * special purpose, resulting in 255.
 */
static constexpr int MAX_AIS = 255;

/**
 * @brief max units
 *
 * Defines the absolute global maximum number of units allowed to exist in a
 * game at any time.
 * NOTE: This must be <= SHRT_MAX (32'766) because current network code
 * transmits unit-IDs as signed shorts. The effective global unit limit is
 * stored in UnitHandler::maxUnits, and is always clamped to this value.
 */
static constexpr int MAX_UNITS = 32000;

/**
 * @brief max weapons per unit
 *
 * Defines the maximum weapons per single unit type as 32.
 */
static constexpr int MAX_WEAPONS_PER_UNIT = 32;

/**
 * @brief randint max
 *
 * Defines the maximum random integer as 0x7fff.
 */
static constexpr int RANDINT_MAX = 0x7fff;

/**
 * maximum speed (elmos/frame) a unit is allowed to have outside the map
 */
static constexpr float MAX_UNIT_SPEED = 1e3f;

/**
 * maximum impulse strength an explosion is allowed to impart on a unit
 */
static constexpr float MAX_EXPLOSION_IMPULSE = 1e4f;

/**
 * if explosion distance is less than speed of explosion multiplied by
 * this factor, units are damaged directly rather than N>=1 frames later
 */
static constexpr float DIRECT_EXPLOSION_DAMAGE_SPEED_SCALE = 4.0f;

/**
 * maximum range of a weapon-projectile with a flight-time member
 */
static constexpr float MAX_PROJECTILE_RANGE = 1e6f;

/**
 * maximum absolute height a projectile is allowed to reach
 */
static constexpr float MAX_PROJECTILE_HEIGHT = 1e6f;

#endif // _GLOBAL_CONSTANTS_H
