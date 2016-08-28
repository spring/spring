/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_UNSYNCED_H
#define _GLOBAL_UNSYNCED_H

#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include "System/UnsyncedRNG.h"

class CPlayer;
class CGameSetup;

/**
 * @brief Globally accessible unsynced data
 *
 * Contains globally accessible data that does not remain synced
 */
class CGlobalUnsynced {
public:
	CR_DECLARE_STRUCT(CGlobalUnsynced)

	CGlobalUnsynced();
	~CGlobalUnsynced();

	static UnsyncedRNG rng;

	int    RandInt()    { return rng.RandInt(); }    //!< random int [0, (INT_MAX & 0x7FFF))
	float  RandFloat()  { return rng.RandFloat(); }  //!< random float [0, 1)
	float3 RandVector() { return rng.RandVector(); } //!< random vector with length = [0, 1)

	void ResetState();
	void LoadFromSetup(const CGameSetup* setup);

	void SetMyPlayer(const int myNumber);
	CPlayer* GetMyPlayer();

public:
	/**
	 * @brief minimum FramesPerSecond
	 *
	 * Defines how many Frames per second
	 * should be rendered. To reach it we
	 * will delay simframes.
	 */
	static const int minFPS = 2;

	/**
	 * @brief simulation drawing balance
	 *
	 * Defines how much percent of the time
	 * for simulation is minimum spend for
	 * drawing.
	 * This is important when reconnecting,
	 * i.e. it means that 15% of the total
	 * cpu time is spend for drawing and 85%
	 * for reconnecting/simulation.
	 */
	static const float reconnectSimDrawBalance;

	/**
	 * @brief simulation frames per second
	 *
	 * Should normally be:
	 * simFPS ~= GAME_SPEED * gs->wantedSpeedFactor;
	 * Only differs if the client lags or reconnects.
	 */
	float simFPS;

	/**
	 * @brief average frame time (in ms)
	 */
	float avgSimFrameTime;
	float avgDrawFrameTime;
	float avgFrameTime;

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
	 * @brief fpsMode
	 *
	 * if true, player is controlling a unit in FPS mode
	 */
	bool fpsMode;

	/**
	* @brief global quit
	*
	* Global boolean indicating whether the user
	* wants to quit
	*/
	volatile bool globalQuit;
	volatile bool globalReload;
	std::string reloadScript;
};

extern CGlobalUnsynced* gu;

#endif /* _GLOBAL_UNSYNCED_H */
