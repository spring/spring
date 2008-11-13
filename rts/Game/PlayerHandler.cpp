/* Author: Tobi Vollebregt */

/* based on code from GlobalSynced.{cpp,h} */

#include "StdAfx.h"
#include "PlayerHandler.h"
#include "Game/GameSetup.h"
#include "mmgr.h"

CR_BIND(CPlayerHandler,);

CR_REG_METADATA(CPlayerHandler, (
	CR_MEMBER(players),
	CR_MEMBER(activePlayers),
	CR_RESERVED(64)
));


CPlayerHandler* playerHandler;


CPlayerHandler::CPlayerHandler()
{
	for(int i = 0; i < MAX_PLAYERS; ++i) {
		players[i].playerNum = i;
	}

	activePlayers = MAX_PLAYERS;
}


CPlayerHandler::~CPlayerHandler()
{
}


void CPlayerHandler::LoadFromSetup(const CGameSetup* setup)
{
	activePlayers = setup->numPlayers;

	for (int i = 0; i < activePlayers; ++i)
	{
		// TODO: refactor
		// NOTE: this seems slightly better already then the static_cast
		Player(i)->PlayerBase::operator=(setup->playerStartingData[i]);
	}
}


int CPlayerHandler::Player(const std::string& name)
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (Player(i)->name == name) {
			return i;
		}
	}
	return -1;
}
