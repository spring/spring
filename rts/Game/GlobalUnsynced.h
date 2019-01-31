/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_UNSYNCED_H
#define _GLOBAL_UNSYNCED_H

#include <atomic>

#include "System/creg/creg_cond.h"
#include "System/GlobalRNG.h"

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

	void Init() { ResetState(); }
	void Kill() { ResetState(); }

	void ResetState();
	void LoadFromSetup(const CGameSetup* setup);

	void SetMyPlayer(const int myNumber);
	CPlayer* GetMyPlayer();

public:
	/**
	 * @brief minimum Frames Per Second
	 *
	 * Defines how many frames per second should minimally be
	 * rendered. To reach this number we will delay simframes.
	 */
	static constexpr int minDrawFPS = 2;

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
	static constexpr float reconnectSimDrawBalance = 0.15f;

	/**
	 * @brief simulation frames per second
	 *
	 * Should normally be:
	 * simFPS ~= GAME_SPEED * gs->wantedSpeedFactor;
	 * Only differs if the client lags or reconnects.
	 */
	float simFPS = 0.0f;

	/**
	 * @brief average frame time (in ms)
	 */
	float avgSimFrameTime = 0.001f;
	float avgDrawFrameTime = 0.001f;
	float avgFrameTime = 0.001f;

	/**
	 * @brief mod game time
	 *
	 * How long the game has been going on
	 * (modified with game's speed factor)
	 */
	float modGameTime = 0.0f;

	/**
	 * @brief game time
	 *
	 * How long the game has been going on
	 * in real time
	 */
	float gameTime = 0.0f;

	/**
	 * @brief start time
	 *
	 * The value of gameTime when the game was started
	 */
	float startTime = 0.0f;

	/**
	 * @brief my player num
	 *
	 * Local player's number
	 */
	int myPlayerNum = 0;

	/**
	 * @brief my team
	 * @note changes when changing spectated team
	 *
	 * Local player's team
	 */
	int myTeam = 1;

	/**
	 * @brief my ally team
	 * @note changes when changing spectated team
	 *
	 * Local player's ally team
	 */
	int myAllyTeam = 1;

	/**
	 * @brief my team used when playing
	 * @note changes only from TEAMMSG_JOIN_TEAM and SetMyPlayer
	 * @note if we never joined any team, it's set to -1
	 *
	 * Local player's team
	 */
	int myPlayingTeam = -1;

	/**
	 * @brief my ally team
	 * @note changes only from TEAMMSG_JOIN_TEAM and SetMyPlayer
	 * @note if we never joined any team, it's set to -1
	 *
	 * Local player's ally team
	 */
	int myPlayingAllyTeam = -1;


	/**
	 * @brief spectating
	 *
	 * Whether this player is spectating
	 * (can't give orders, and can see everything if spectateFullView is true)
	 */
	bool spectating = false;

	/**
	 * @brief spectatingFullView
	 *
	 * Whether this player is a spectator, and can see everything
	 * (if set to false, visibility is determined by the current team)
	 */
	bool spectatingFullView = false;

	/**
	 * @brief spectatingFullSelect
	 *
	 * Can all units be selected when spectating?
	 */
	bool spectatingFullSelect = false;

	/**
	 * @brief fpsMode
	 *
	 * if true, player is controlling a unit in FPS mode
	 */
	bool fpsMode = false;

	/**
	* @brief global quit
	*
	* Global boolean indicating whether the user
	* wants to quit
	*/
	std::atomic<bool> globalQuit = {false};
	std::atomic<bool> globalReload = {false};
};


extern CGlobalUnsynced* gu;
extern CGlobalUnsyncedRNG guRNG;

#endif /* _GLOBAL_UNSYNCED_H */
