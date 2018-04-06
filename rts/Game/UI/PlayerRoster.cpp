/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PlayerRoster.h"

#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/StringUtil.h"

#include <cassert>
#include <cstring>

static int CompareDummy      (const int aIdx, const int bIdx) { return ((aIdx > bIdx) * 2 - 1); }
static int CompareAllies     (const int aIdx, const int bIdx);
static int CompareTeamIDs    (const int aIdx, const int bIdx);
static int ComparePlayerNames(const int aIdx, const int bIdx);
static int ComparePlayerCPUs (const int aIdx, const int bIdx);
static int ComparePlayerPings(const int aIdx, const int bIdx);


PlayerRoster playerRoster;


PlayerRoster::PlayerRoster()
{
	compareType = Allies;
	compareFunc = CompareAllies;
}


void PlayerRoster::SetCompareFunc(SortType cmpType)
{
	switch (cmpType) {
		case Allies:     { compareFunc = CompareAllies;      } break;
		case TeamID:     { compareFunc = CompareTeamIDs;     } break;
		case PlayerName: { compareFunc = ComparePlayerNames; } break;
		case PlayerCPU:  { compareFunc = ComparePlayerCPUs;  } break;
		case PlayerPing: { compareFunc = ComparePlayerPings; } break;
		case Disabled:   { compareFunc = CompareDummy;       } break;
		default:         { compareFunc = CompareAllies;      } break;
	}
}


bool PlayerRoster::SetSortTypeByName(const std::string& name)
{
	const int num = atoi(name.c_str());

	if (num > 0)
		return SetSortTypeByCode((SortType)num);

	switch (name[0]) {
		case 'a': case 'A': { SetCompareFunc(compareType = Allies    ); return true; } break;
		case 't': case 'T': { SetCompareFunc(compareType = TeamID    ); return true; } break;
		case 'n': case 'N': { SetCompareFunc(compareType = PlayerName); return true; } break;
		case 'c': case 'C': { SetCompareFunc(compareType = PlayerCPU ); return true; } break;
		case 'p': case 'P': { SetCompareFunc(compareType = PlayerPing); return true; } break;
		default:            {                                                        } break;
	}

	SetCompareFunc(compareType = Disabled);
	return false;
}


bool PlayerRoster::SetSortTypeByCode(SortType type)
{
	switch (type) {
		case Allies:     {                                                   } // fall-through
		case TeamID:     {                                                   } // fall-through
		case PlayerName: {                                                   } // fall-through
		case PlayerCPU:  {                                                   } // fall-through
		case PlayerPing: { SetCompareFunc(compareType = type); return  true; } break;
		case Disabled:   { SetCompareFunc(compareType = type); return false; } break;
		default:         {                                                   } break;
	}

	return false;
}


const char* PlayerRoster::GetSortName() const
{
	switch (compareType) {
		case Allies:     return "Allies";
		case TeamID:     return "TeamID";
		case PlayerName: return "Name";
		case PlayerCPU:  return "CPU Usage";
		case PlayerPing: return "Ping";
		case Disabled:   return "Disabled";
		default: assert(false); break;
	}
	return nullptr;
}


const std::vector<int>& PlayerRoster::GetIndices(bool includePathingFlag, bool callerBlockResort)
{
	// if Disabled, compareFunc is a dummy so the indices are left alone
	// assert(compareType != Disabled);

	// number of players can change at runtime, must test this each call
	// or make PlayerRoster an EventClient and listen for Player* events
	// caller should ensure not to block resorting if a player joins and
	// another quits in between calls
	if (playerIndices.size() != playerHandler.ActivePlayers()) {
		playerIndices.clear();
		playerIndices.resize(playerHandler.ActivePlayers(), 0);
	}

	if (!callerBlockResort) {
		int playerIdx = 0;

		std::generate(playerIndices.begin(), playerIndices.end(), [&playerIdx]() { return (playerIdx++); });
		std::sort(playerIndices.begin(), playerIndices.end(), [&](int aIdx, int bIdx) { return (compareFunc(aIdx, bIdx) <= 0); });
	}

	// caller should do filtering for active players
	return playerIndices;
}




static inline int CompareBasics(const CPlayer* a, const CPlayer* b)
{
	// non-NULL first (and return 0 if both NULL)
	if ((a != nullptr) && (b == nullptr)) return -1;
	if ((a == nullptr) && (b != nullptr)) return +1;
	if ((a == nullptr) && (b == nullptr)) return  0;

	// active players first
	if ( a->active && !b->active) return -1;
	if (!a->active &&  b->active) return +1;

	// then pathing players
	if (gs->PreSimFrame()) {
		if (a->ping == PATHING_FLAG && b->ping != PATHING_FLAG) return -1;
		if (a->ping != PATHING_FLAG && b->ping == PATHING_FLAG) return +1;
	}

	return 0;
}


static int CompareAllies(const int aIdx, const int bIdx)
{
	const CPlayer* a = playerHandler.Player(aIdx);
	const CPlayer* b = playerHandler.Player(bIdx);

	const int basic = CompareBasics(a, b);
	if (basic != 0)
		return basic;

	// non-spectators first
	if (!a->spectator &&  b->spectator) return -1;
	if ( a->spectator && !b->spectator) return +1;

	// my player first
	const int myPNum = gu->myPlayerNum;
	if ((aIdx == myPNum) && (bIdx != myPNum)) return -1;
	if ((aIdx != myPNum) && (bIdx == myPNum)) return +1;

	// my teammates first
	const int aTeam = a->team;
	const int bTeam = b->team;
	const int myTeam = gu->myTeam;
	if ((aTeam == myTeam) && (bTeam != myTeam)) return -1;
	if ((aTeam != myTeam) && (bTeam == myTeam)) return +1;

	// my allies first
	const int aAlly = teamHandler.AllyTeam(aTeam);
	const int bAlly = teamHandler.AllyTeam(bTeam);
	const int myATeam = gu->myAllyTeam;
	if ((aAlly == myATeam) && (bAlly != myATeam)) return -1;
	if ((aAlly != myATeam) && (bAlly == myATeam)) return +1;

	// sort by ally team
	if (aAlly < bAlly) return -1;
	if (aAlly > bAlly) return +1;

	// sort by team
	if (aTeam < bTeam) return -1;
	if (aTeam > bTeam) return +1;

	// sort by player id
	if (aIdx < bIdx) return -1;
	if (aIdx > bIdx) return +1;

	return 0;
}

static int CompareTeamIDs(const int aIdx, const int bIdx)
{
	const CPlayer* a = playerHandler.Player(aIdx);
	const CPlayer* b = playerHandler.Player(bIdx);

	const int basic = CompareBasics(a, b);
	if (basic != 0)
		return basic;

	// sort by team id
	if (a->team < b->team) return -1;
	if (a->team > b->team) return +1;

	// sort by player id
	if (aIdx < bIdx) return -1;
	if (aIdx > bIdx) return +1;

	return 0;
}



static int ComparePlayerNames(const int aIdx, const int bIdx)
{
	const CPlayer* a = playerHandler.Player(aIdx);
	const CPlayer* b = playerHandler.Player(bIdx);

	const int basic = CompareBasics(a, b);
	if (basic != 0)
		return basic;

	// sort by player name
	const std::string aName = StringToLower(a->name);
	const std::string bName = StringToLower(b->name);
	return strcmp(aName.c_str(), bName.c_str());
}

static int ComparePlayerCPUs(const int aIdx, const int bIdx)
{
	const CPlayer* a = playerHandler.Player(aIdx);
	const CPlayer* b = playerHandler.Player(bIdx);

	const int basic = CompareBasics(a, b);
	if (basic != 0)
		return basic;

	// sort by player cpu usage
	if (a->cpuUsage < b->cpuUsage) return -1;
	if (a->cpuUsage > b->cpuUsage) return +1;

	return 0;
}

static int ComparePlayerPings(const int aIdx, const int bIdx)
{
	const CPlayer* a = playerHandler.Player(aIdx);
	const CPlayer* b = playerHandler.Player(bIdx);

	const int basic = CompareBasics(a, b);
	if (basic != 0)
		return basic;

	// sort by player pings
	if (a->ping < b->ping) return -1;
	if (a->ping > b->ping) return +1;

	return 0;
}

