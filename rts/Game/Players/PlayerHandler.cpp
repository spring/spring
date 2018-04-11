/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <cassert>

#include "PlayerHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Game/GameSetup.h"
#include "Game/SelectedUnitsHandler.h"

CR_BIND(CPlayerHandler,)

CR_REG_METADATA(CPlayerHandler, (
	CR_MEMBER(players)
))


CPlayerHandler playerHandler;


void CPlayerHandler::ResetState()
{
	players.clear();
	players.reserve(MAX_PLAYERS);
}

void CPlayerHandler::LoadFromSetup(const CGameSetup* setup)
{
	const std::vector<PlayerBase>& playerData = setup->GetPlayerStartingDataCont();

	const int oldSize = players.size();
	const int newSize = std::max(players.size(), playerData.size());

	assert(newSize <= MAX_PLAYERS);
	assert(players.capacity() == MAX_PLAYERS);

	for (unsigned int i = oldSize; i < newSize; ++i) {
		players.emplace_back();
	}

	for (size_t i = 0; i < playerData.size(); ++i) {
		players[i] = playerData[i];

		players[i].playerNum = int(i);
		players[i].fpsController.SetControllerPlayer(&players[i]);
	}
}


int CPlayerHandler::Player(const std::string& name) const
{
	const auto pred = [&name](const CPlayer& player) { return (player.name == name); };
	const auto iter = std::find_if(players.begin(), players.end(), pred);

	if (iter != players.end())
		return (iter->playerNum);

	return -1;
}

void CPlayerHandler::PlayerLeft(int id, unsigned char reason)
{
	Player(id)->active = false;
	Player(id)->ping = 0;
}



unsigned int CPlayerHandler::NumActivePlayersInTeam(int teamId) const
{
	unsigned int n = 0;

	for (const CPlayer& player: players) {
		// do not count spectators, or demos will desync
		n += (player.active && !player.spectator && player.team == teamId);
	}

	return n;
}

std::vector<int> CPlayerHandler::ActivePlayersInTeam(int teamId) const
{
	std::vector<int> playersInTeam;

	for (const CPlayer& player: players) {
		// do not count spectators, or demos will desync
		if (!player.active)
			continue;
		if (player.spectator)
			continue;
		if (player.team != teamId)
			continue;

		playersInTeam.push_back(player.playerNum);
	}

	return playersInTeam;
}



void CPlayerHandler::GameFrame(int frameNum)
{
	for (CPlayer& player: players) {
		player.GameFrame(frameNum);
	}
}

void CPlayerHandler::AddPlayer(const CPlayer& player)
{
	const int oldSize = players.size();
	const int newSize = std::max(oldSize, player.playerNum + 1);

	assert(players.capacity() == MAX_PLAYERS);
	assert((players.size() + (newSize - oldSize)) <= MAX_PLAYERS);

	{
		for (unsigned int i = oldSize; i < newSize; ++i) {
			// fill gap with stubs
			players.emplace_back();

			CPlayer& stub = players.back();
			stub.name = "unknown";

			stub.isFromDemo = false;
			stub.spectator = true;

			stub.team = 0;
			stub.playerNum = (int)i;

			selectedUnitsHandler.netSelected.emplace_back();
		}

		CPlayer* newPlayer = &players[player.playerNum];
		*newPlayer = player;
		newPlayer->fpsController.SetControllerPlayer(newPlayer);
	}
}

