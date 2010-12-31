/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_UNSYNCED_H
#define _GLOBAL_UNSYNCED_H

#include "System/creg/creg_cond.h"
#include "System/float3.h"

class CGameSetup;
class CUnit;

/**
 * @brief Globally accessible unsynced data
 *
 * Contains globally accessible data that does not remain synced
 */
class CGlobalUnsynced {
	CR_DECLARE(CGlobalUnsynced);

	CGlobalUnsynced();
	~CGlobalUnsynced();

public:
	int    usRandInt();    //!< Unsynced random int
	float  usRandFloat();  //!< Unsynced random float
	float3 usRandVector(); //!< Unsynced random vector

	void LoadFromSetup(const CGameSetup* setup);
	void SetMyPlayer(const int myNumber);

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
	 * @brief start time
	 *
	 * The value of gameTime when the game was started
	 */
	float startTime;

	/**
	 * @brief my player num
	 *
	 * Local player's number
	 */
	int myPlayerNum;

	/**
	 * @brief my team
	 * @note changes when changing spectated team
	 *
	 * Local player's team
	 */
	int myTeam;

	/**
	 * @brief my ally team
	 * @note changes when changing spectated team
	 *
	 * Local player's ally team
	 */
	int myAllyTeam;

	/**
	 * @brief my team used when playing
	 * @note changes only from TEAMMSG_JOIN_TEAM and SetMyPlayer
	 * @note if we never joined any team, it's set to -1
	 *
	 * Local player's team
	 */
	int myPlayingTeam;

	/**
	 * @brief my ally team
	 * @note changes only from TEAMMSG_JOIN_TEAM and SetMyPlayer
	 * @note if we never joined any team, it's set to -1
	 *
	 * Local player's ally team
	 */
	int myPlayingAllyTeam;


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

	/**
	 * @brief direct control
	 *
	 * Pointer to unit being directly controlled by
	 * this player
	 */
	CUnit* directControl;

	/**
	* @brief global quit
	*
	* Global boolean indicating whether the user
	* wants to quit
	*/
	volatile bool globalQuit;

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
