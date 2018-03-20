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
 * blocking-map is twice that of TA's; each square of a TA footprint
 * covers SQUARE_SIZE*2 x SQUARE_SIZE*2 elmos.
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
 * @brief max teams
 *
 * Defines the maximum number of teams as 255
 * (254 real teams, and an extra slot for the GAIA team).
 */
static constexpr int MAX_TEAMS = 255;

/**
 * @brief max players
 *
 * Hard limit, currently restricted by the size of the player-ID field
 * (1 byte) in network messages with the values 252 to 255 reserved for
 * special purposes. (FIXME: max should be 252 then?)
 */
static constexpr int MAX_PLAYERS = 251;

/**
 * @brief max AIs
 *
 * Hard limit, currently restricted by the size of the ai-ID field
 * (1 byte) in network messages with the value 255 reserved for
 * special purposes.
 */
static constexpr int MAX_AIS = 255;

/**
 * @brief max units / features / projectiles
 *
 * Defines the absolute global maximum number of simulation objects
 * (units, features, projectiles) that are allowed to exist in a game
 * at any time.
 *
 * NOTE:
 * MAX_UNITS must be LEQ SHORT_MAX (32767) because current network code
 * transmits unit IDs as signed shorts. The effective global unit limit
 * is stored in UnitHandler::maxUnits, and always clamped to this value.
 *
 * All types of IDs are also passed to Lua callins, while feature IDs are
 * additionally transmitted as (32-bit) floating-point command parameters
 * which places a further cap at 1 << 24 (far beyond the realm of feasible
 * runtime performance) should these maxima ever be removed.
 */
static constexpr int MAX_UNITS       =  32000;
static constexpr int MAX_FEATURES    =  32000;
static constexpr int MAX_PROJECTILES = 128000;

/**
 * @brief max weapons per unit
 *
 * Defines the maximum weapons per single unit type as 32.
 */
static constexpr int MAX_WEAPONS_PER_UNIT = 32;


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
static constexpr int DIRECT_EXPLOSION_DAMAGE_SPEED_SCALE = 4;

/**
 * maximum range of a weapon-projectile with a flight-time member
 */
static constexpr float MAX_PROJECTILE_RANGE = 1e6f;

/**
 * maximum absolute height a projectile is allowed to reach
 */
static constexpr float MAX_PROJECTILE_HEIGHT = 1e6f;

/**
 * maximum allowed sensor radius (LOS, airLOS, ...) of any unit, in elmos
 * the value chosen is sufficient to cover a 40x40 map with room to spare
 * from any point
 */
static constexpr int MAX_UNIT_SENSOR_RADIUS = 32768;

#endif // _GLOBAL_CONSTANTS_H

