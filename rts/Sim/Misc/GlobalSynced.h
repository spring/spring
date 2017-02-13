/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_SYNCED_H
#define _GLOBAL_SYNCED_H

#include "System/creg/creg_cond.h"
#include "System/GlobalRNG.h"


class CGameSetup;
class CTeam;
class CPlayer;

/**
 * @brief Global synced data
 *
 * Class contains globally accessible
 * data that remains synced.
 */
class CGlobalSynced
{
public:
	CR_DECLARE_STRUCT(CGlobalSynced)

	CGlobalSynced();  //!< Constructor
	~CGlobalSynced(); //!< Destructor

	void ResetState();
	void LoadFromSetup(const CGameSetup*);

	// Lua should never see the pre-simframe value
	int GetLuaSimFrame() { return (frameNum > 0) ? frameNum : 0; }
	int GetTempNum() { return tempNum++; }

	// remains true until first SimFrame call
	bool PreSimFrame() const { return (frameNum == -1); }

public:
	/**
	* @brief frame number
	*
	* Stores the current frame number
	*/
	int frameNum;


	/**
	* @brief speed factor
	*
	* Contains the actual gamespeed factor
	* used by the game. The real gamespeed
	* can be up to this but is lowered if
	* clients can't keep up (lag protection)
	*/
	float speedFactor;

	/**
	* @brief wanted speed factor
	*
	* Contains the aimed speed factor.
	* The total simframes
	* per second calculate as follow:
	* wantedSimFPS = speedFactor * GAME_SPEED;
	*/
	float wantedSpeedFactor;


	/**
	* @brief paused
	*
	* Holds whether the game is paused
	*/
	bool paused;

	/**
	* @brief god mode
	*
	* Whether god-mode is enabled, which allows all players (even spectators)
	* to control all units.
	*/
	bool godMode;

	/**
	* @brief cheat enabled
	*
	* Whether cheating is enabled
	*/
	bool cheatEnabled;

	/**
	* @brief disable helper AIs
	*
	* Whether helper AIs are allowed, including LuaUI control widgets
	*/
	bool noHelperAIs;

	/**
	* @brief definition editing enabled
	*
	* Whether editing of unit-, feature- and weapon-defs through Lua is enabled.
	*/
	bool editDefsEnabled;

	/**
	* @brief LuaGaia control
	*
	* Whether or not LuaGaia is enabled
	*/
	bool useLuaGaia;

private:
	/**
	* @brief temp num
	*
	* Used for getting temporary but unique numbers
	* (increase after each use)
	*/
	int tempNum;
};


extern CGlobalSynced* gs;
extern CGlobalSyncedRNG gsRNG;

#endif // _GLOBAL_SYNCED_H

