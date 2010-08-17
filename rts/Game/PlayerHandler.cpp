/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
	int newSize = std::max( players.size(), setup->playerStartingData.size() );
	players.resize(newSize);

	for (size_t i = 0; i < setup->playerStartingData.size(); ++i) {
		players[i] = setup->playerStartingData[i];
		players[i].playerNum = (int)i;
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
	Player(player)->active = false;
	Player(player)->ping = 0;
}

std::vector<int> CPlayerHandler::ActivePlayersInTeam(int teamId) const
{
	std::vector<int> playersInTeam;

	size_t p = 0;
	for(playerVec::const_iterator pi = players.begin(); pi != players.end(); ++pi, ++p) {
		// do not count spectators, or demos will desync
		if (pi->active && !pi->spectator && (pi->team == teamId)) {
			playersInTeam.push_back(p);
		}
	}

	return playersInTeam;
}

void CPlayerHandler::GameFrame(int frameNum)
{
	for(playerVec::iterator it = players.begin(); it != players.end(); ++it)
	{
		it->GameFrame(frameNum);
	}
}

void CPlayerHandler::AddPlayer( const CPlayer& player )
{
	int newSize = std::max( (int)players.size(), player.playerNum + 1 );
	players.resize(newSize);
	for ( unsigned int i = players.size(); i < player.playerNum; i++ ) // fill gap with stubs
	{
		CPlayer& stub = players[i];
		stub.name = "unknown";
		stub.isFromDemo = false;
		stub.spectator = true;
		stub.team = 0;
		stub.playerNum = (int)i;
	}
	players[player.playerNum] = player;
	players[player.playerNum].myControl.myController = &players[player.playerNum];
}
