/* Author: Tobi Vollebregt */

/* based on code from GlobalSynced.{cpp,h} */

#ifndef PLAYERHANDLER_H
#define PLAYERHANDLER_H

#include "creg/creg.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Player.h"

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
	CPlayer* Player(int i) { return &players[i]; }

	/**
	 * @brief Player
	 * @param name name of the player
	 * @return his playernumber of -1 if not found
	 *
	 * Search a player by name.
	 */
	int Player(const std::string& name);

	int ActivePlayers() const { return activePlayers; }

private:

	/**
	 * @brief active players
	 *
	 * The number of active players
	 */
	int activePlayers;

	/**
	 * @brief players
	 *
	 * Array of CPlayer instances, for all the
	 * players in the game (size MAX_PLAYERS)
	 */
	CPlayer players[MAX_PLAYERS];
};

extern CPlayerHandler* playerHandler;

#endif // !PLAYERHANDLER_H
