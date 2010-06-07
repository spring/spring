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
	CPlayer* Player(int i) { assert(unsigned(i) < players.size()); return &players[i]; }

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
	 * Constant at runtime
	 */
	int ActivePlayers() const { return players.size(); };

	/**
	 * @brief Number of players in a team
	 * 
	 * Will change during runtime (Connection lost, died, ...).
	 * This excludes spectators and AIs.
	 */
	std::vector<int> ActivePlayersInTeam(int teamId) const;

	void GameFrame(int frameNum);

private:
	typedef std::vector<CPlayer> playerVec;
	/**
	 * @brief players
	 *
	 * for all the players in the game
	 */
	playerVec players;
};

extern CPlayerHandler* playerHandler;

#endif // !PLAYERHANDLER_H
