/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_UNSYNCED_H
#define _GLOBAL_UNSYNCED_H

#include "float3.h"
#include "creg/creg_cond.h"

class CUnit;

/**
 * @brief Globally accessible unsynced data
 *
 * Contains globally accessible data that does not remain synced
 */
class CGlobalUnsynced {
	CR_DECLARE(CGlobalUnsynced);

	CGlobalUnsynced();

public:
	void PostInit();

	int    usRandInt();    //!< Unsynced random int
	float  usRandFloat();  //!< Unsynced random float
	float3 usRandVector(); //!< Unsynced random vector

	void SetMyPlayer(const int mynumber);

	/**
	 * @brief mod game time
	 *
	 * How long the game has been going on
	 * (modified with game's speed factor)
	 */
	float modGameTime;

	/**
	 * @brief game time
	 *
	 * How long the game has been going on
	 * in real time
	 */
	float gameTime;

	/**
	 * @brief my player num
	 *
	 * Local player's number
	 */
	int myPlayerNum;

	/**
	 * @brief my team
	 *
	 * Local player's team
	 */
	int myTeam;

	/**
	 * @brief my ally team
	 *
	 * Local player's ally team
	 */
	int myAllyTeam;


	/**
	 * @brief spectating
	 *
	 * Whether this player is spectating
	 * (can't give orders, and can see everything if spectateFullView is true)
	 */
	bool spectating;

	/**
	 * @brief spectatingFullView
	 *
	 * Whether this player is a spectator, and can see everything
	 * (if set to false, visibility is determined by the current team)
	 */
	bool spectatingFullView;

	/**
	 * @brief spectatingFullSelect
	 *
	 * Can all units be selected when spectating?
	 */
	bool spectatingFullSelect;

	bool moveWarnings;
	bool buildWarnings;

	/**
	 * @brief direct control
	 *
	 * Pointer to unit being directly controlled by
	 * this player
	 */
	CUnit* directControl;

private:
	/**
	* @brief rand seed
	*
	* Stores the unsynced random seed
	*/
	int usRandSeed;
};

extern CGlobalUnsynced* gu;

#endif /* _GLOBAL_UNSYNCED_H */
