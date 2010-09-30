/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* based on code from GlobalSynced.{cpp,h} */

#ifndef PLAYERHANDLER_H
#define PLAYERHANDLER_H

#include <assert.h>
#include <vector>

#include "creg/creg_cond.h"
#include "Player.h"

#define PATHING_FLAG 0xFFFFFFFF

class CGameSetup;


class CPlayerHandler
{
public:
	CR_DECLARE(CPlayerHandler);

	CPlayerHandler();
	~CPlayerHandler();

	void LoadFromSetup(const CGameSetup* setup);

	/**
	 * @brief Player
	 * @param i index to fetch
	 * @return CPlayer pointer
	 *
	 * Accesses a CPlayer instance at a given index
	 */
	CPlayer* Player(int id) { assert(unsigned(id) < players.size()); return players[id]; }

	/**
	 * @brief Player
	 * @param name name of the player
	 * @return his playernumber of -1 if not found
	 *
	 * Search a player by name.
	 */
	int Player(const std::string& name) const;

	void PlayerLeft(int playernum, unsigned char reason);

	/**
	 * @brief Number of players the game was created for
	 * 
	 * Will change at runtime, for example if a new spectator joins
	 */
	int ActivePlayers() const { return players.size(); }

	/**
	 * @brief Number of players in a team
	 * 
	 * Will change during runtime (Connection lost, died, ...).
	 * This excludes spectators and AIs.
	 */
	std::vector<int> ActivePlayersInTeam(int teamId) const;

	/**
	 * @brief is the supplied id a valid playerId?
	 * 
	 * Will change during at runtime when a new spectator joins
	 */
	bool IsValidPlayer(int id) const {
		return ((id >= 0) && (id < ActivePlayers()));
	}

	void GameFrame(int frameNum);

	/**
	 * @brief Adds a new player for dynamic join
	 *
	 * This resizes the playerlist adding stubs if there's gaps to his playerNum
	 */
	void AddPlayer(const CPlayer& player);

private:
	typedef std::vector<CPlayer *> playerVec;
	/**
	 * @brief players
	 *
	 * for all the players in the game
	 */
	playerVec players;
};

extern CPlayerHandler* playerHandler;

#endif // !PLAYERHANDLER_H
