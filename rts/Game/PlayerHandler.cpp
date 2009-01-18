/* Author: Tobi Vollebregt */

/* based on code from GlobalSynced.{cpp,h} */

#include "StdAfx.h"
#include "PlayerHandler.h"

#include "LogOutput.h"
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
	activePlayers = 0;
}


CPlayerHandler::~CPlayerHandler()
{
}


void CPlayerHandler::LoadFromSetup(const CGameSetup* setup)
{
	activePlayers = setup->numPlayers;
	players.resize(setup->numPlayers);

	for (int i = 0; i < activePlayers; ++i)
	{
		players[i] = setup->playerStartingData[i];
		players[i].playerNum = i;
	}
}


int CPlayerHandler::Player(const std::string& name) const
{
	for (playerVec::const_iterator it = players.begin(); it != players.end(); ++it) {
		if (it->name == name)
			return it->playerNum;
	}
	return -1;
}

void CPlayerHandler::PlayerLeft(int player, unsigned char reason)
{
	switch (reason) {
		case 1: {
			if (Player(player)->spectator) {
				logOutput.Print("Spectator %s left", Player(player)->name.c_str());
			} else {
				logOutput.Print("Player %s left", Player(player)->name.c_str());
			}
			break;
		}
		case 2:
			logOutput.Print("Player %s has been kicked", Player(player)->name.c_str());
			break;
		case 0:
			logOutput.Print("Lost connection to %s", Player(player)->name.c_str());
			break;
		default:
			logOutput.Print("Player %s left the game (reason unknown: %i)", Player(player)->name.c_str(), reason);
	}
	Player(player)->active = false;
}