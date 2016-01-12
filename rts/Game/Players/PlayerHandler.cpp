/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* based on code from GlobalSynced.{cpp,h} */

#include "PlayerHandler.h"

#include "Player.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Game/GameSetup.h"
#include "Game/SelectedUnitsHandler.h"

CR_BIND(CPlayerHandler,)

CR_REG_METADATA(CPlayerHandler, (
	CR_MEMBER(players)
))


CPlayerHandler* playerHandler = NULL;

CPlayerHandler::~CPlayerHandler()
{
	ResetState();
}


void CPlayerHandler::ResetState()
{
	for (playerVec::iterator pi = players.begin(); pi != players.end(); ++pi) {
		delete *pi;
	}

	players.clear();
}

void CPlayerHandler::LoadFromSetup(const CGameSetup* setup)
{
	const std::vector<PlayerBase>& playerData = setup->GetPlayerStartingDataCont();

	const int oldSize = players.size();
	const int newSize = std::max(players.size(), playerData.size());

	for (unsigned int i = oldSize; i < newSize; ++i) {
		players.push_back(new CPlayer());
	}

	for (size_t i = 0; i < playerData.size(); ++i) {
		CPlayer* player = players[i];
		*player = playerData[i];

		player->playerNum = (int)i;
		player->fpsController.SetControllerPlayer(player);
	}
}


int CPlayerHandler::Player(const std::string& name) const
{
	for (auto pi = players.cbegin(); pi != players.cend(); ++pi) {
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

	for (auto pi = players.cbegin(); pi != players.cend(); ++pi, ++p) {
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
	const int oldSize = players.size();
	const int newSize = std::max(oldSize, player.playerNum + 1);

	{
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
}

