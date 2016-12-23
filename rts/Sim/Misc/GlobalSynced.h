/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_SYNCED_H
#define _GLOBAL_SYNCED_H

#include <algorithm>
#include <string>

#include "System/float3.h"
#include "System/creg/creg_cond.h"
#include "GlobalConstants.h"


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
	int GetLuaSimFrame() { return std::max(frameNum, 0); }
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


// crappy but fast LCG
class CGlobalSyncedRNG {
public:
	CGlobalSyncedRNG() { SetSeed(0, true); }

	// needed for random_shuffle
	int operator()(unsigned int N) { return (NextInt() % N); }

	int NextInt() { return (((randSeed = (randSeed * 214013L + 2531011L)) >> 16) & RANDINT_MAX); }
	float NextFloat() { return ((NextInt() * 1.0f) / RANDINT_MAX); }

	float3 NextVector() {
		float3 ret;

		do {
			ret.x = NextFloat() * 2.0f - 1.0f;
			ret.y = NextFloat() * 2.0f - 1.0f;
			ret.z = NextFloat() * 2.0f - 1.0f;
		} while (ret.SqLength() > 1.0f);

		return ret;
	}

	unsigned int GetSeed() const { return randSeed; }
	unsigned int GetInitSeed() const { return initRandSeed; }

	void SetSeed(unsigned int seed, bool init) {
		randSeed = seed;
		initRandSeed = (seed * init) + initRandSeed * (1 - init);
	}

private:
	/**
	* @brief random seed
	*
	* Holds the synced random seed
	*/
	int randSeed;

	/**
	* @brief initial random seed
	*
	* Holds the synced initial random seed
	*/
	int initRandSeed;
};


extern CGlobalSynced* gs;
extern CGlobalSyncedRNG gsRNG;

#endif // _GLOBAL_SYNCED_H

