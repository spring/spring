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
		*Player(i) = setup->playerStartingData[i];
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
