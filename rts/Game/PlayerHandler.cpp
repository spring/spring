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
	CR_RESERVED(64)
));


CPlayerHandler* playerHandler;


CPlayerHandler::CPlayerHandler()
{
}


CPlayerHandler::~CPlayerHandler()
{
}


void CPlayerHandler::LoadFromSetup(const CGameSetup* setup)
{
	players.resize(setup->numPlayers);

	for (int i = 0; i < setup->numPlayers; ++i)
	{
		players[i] = setup->playerStartingData[i];
		players[i].playerNum = i;
		players[i].myControl.myController = &players[i];
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
	const char *type = Player(player)->spectator ? "Spectator" : "Player";
	switch (reason) {
		case 1: {
			logOutput.Print("%s %s left", type, Player(player)->name.c_str());
			break;
		}
		case 2:
			logOutput.Print("%s %s has been kicked", type, Player(player)->name.c_str());
			break;
		case 0:
			logOutput.Print("%s %s dropped (connection lost)", type, Player(player)->name.c_str());
			break;
		default:
			logOutput.Print("%s %s left the game (reason unknown: %i)", type, Player(player)->name.c_str(), reason);
	}
	Player(player)->active = false;
	Player(player)->ping = 0;
}

void CPlayerHandler::GameFrame(int frameNum)
{
	for(playerVec::iterator it = players.begin(); it != players.end(); ++it)
	{
		it->GameFrame(frameNum);
	}
}
