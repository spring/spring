/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* based on code from GlobalSynced.{cpp,h} */

#include "PlayerHandler.h"

#include "Player.h"
#include "lib/gml/gmlmut.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Game/GameSetup.h"
#include "SelectedUnitsHandler.h"

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
	for (playerVec::iterator pi = players.begin(); pi != players.end(); ++pi) {
		delete *pi;
	}
}


void CPlayerHandler::LoadFromSetup(const CGameSetup* setup)
{
	int oldSize = players.size();
	int newSize = std::max( players.size(), setup->playerStartingData.size() );
	
	for (unsigned int i = oldSize; i < newSize; ++i) {
		players.push_back(new CPlayer());
	}

	for (size_t i = 0; i < setup->playerStartingData.size(); ++i) {
		CPlayer* player = players[i];
		*player = setup->playerStartingData[i];

		player->playerNum = (int)i;
		player->fpsController.SetControllerPlayer(player);
	}
}


int CPlayerHandler::Player(const std::string& name) const
{
	playerVec::const_iterator pi;
	for (pi = players.begin(); pi != players.end(); ++pi) {
		if ((*pi)->name == name) {
			return (*pi)->playerNum;
		}
	}
	return -1;
}

void CPlayerHandler::PlayerLeft(int id, unsigned char reason)
{
	Player(id)->active = false;
	Player(id)->ping = 0;
}

std::vector<int> CPlayerHandler::ActivePlayersInTeam(int teamId) const
{
	std::vector<int> playersInTeam;

	size_t p = 0;
	playerVec::const_iterator pi;
	for (pi = players.begin(); pi != players.end(); ++pi, ++p) {
		// do not count spectators, or demos will desync
		if ((*pi)->active && !(*pi)->spectator && ((*pi)->team == teamId)) {
			playersInTeam.push_back(p);
		}
	}

	return playersInTeam;
}

void CPlayerHandler::GameFrame(int frameNum)
{
	for (playerVec::iterator pi = players.begin(); pi != players.end(); ++pi) {
		(*pi)->GameFrame(frameNum);
	}
}

void CPlayerHandler::AddPlayer(const CPlayer& player)
{
	GML_MSTMUTEX_DOUNLOCK(sim); // AddPlayer - temporarily unlock this mutex to prevent a deadlock

	const int oldSize = players.size();
	const int newSize = std::max((int)players.size(), player.playerNum + 1);

	{
		GML_STDMUTEX_LOCK(draw); // AddPlayer - rendering accesses Player(x) in too many places, lock the entire draw thread

		for (unsigned int i = oldSize; i < newSize; ++i) {
			// fill gap with stubs
			CPlayer* stub = new CPlayer();
			stub->name = "unknown";
			stub->isFromDemo = false;
			stub->spectator = true;
			stub->team = 0;
			stub->playerNum = (int)i;
			players.push_back(stub);
			selectedUnitsHandler.netSelected.push_back(std::vector<int>());
		}

		CPlayer* newPlayer = players[player.playerNum];
		*newPlayer = player;
		newPlayer->fpsController.SetControllerPlayer(newPlayer);
	}

	GML_MSTMUTEX_DOLOCK(sim); // AddPlayer - restore unlocked mutex
}
