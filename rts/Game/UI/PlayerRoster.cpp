/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PlayerRoster.h"

#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Util.h"

#include <cassert>
#include <cstring>

static int CompareAllies     (const void* a, const void* b);
static int CompareTeamIDs    (const void* a, const void* b);
static int ComparePlayerNames(const void* a, const void* b);
static int ComparePlayerCPUs (const void* a, const void* b);
static int ComparePlayerPings(const void* a, const void* b);


PlayerRoster playerRoster;


/******************************************************************************/

PlayerRoster::PlayerRoster()
{
	compareType = Allies;
	compareFunc = CompareAllies;
}


void PlayerRoster::SetCompareFunc()
{
	switch (compareType) {
		case Allies:     { compareFunc = CompareAllies;      break; }
		case TeamID:     { compareFunc = CompareTeamIDs;     break; }
		case PlayerName: { compareFunc = ComparePlayerNames; break; }
		case PlayerCPU:  { compareFunc = ComparePlayerCPUs;  break; }
		case PlayerPing: { compareFunc = ComparePlayerPings; break; }
		case Disabled:   { compareFunc = CompareAllies;      break; }
		default:         { compareFunc = CompareAllies;      break; }
	}
}


bool PlayerRoster::SetSortTypeByName(const std::string& name)
{
	const int num = atoi(name.c_str());
	if (num > 0) {
		return SetSortTypeByCode((SortType)num);
	}

	const std::string lower = StringToLower(name);
	     if (lower == "ally") { compareType = Allies;     }
	else if (lower == "team") { compareType = TeamID;     }
	else if (lower == "name") { compareType = PlayerName; }
	else if (lower == "cpu")  { compareType = PlayerCPU;  }
	else if (lower == "ping") { compareType = PlayerPing; }
	else {
		compareType = Disabled;
		SetCompareFunc();
		return false;
	}

	SetCompareFunc();
	return true;
}


bool PlayerRoster::SetSortTypeByCode(SortType type)
{
	if ((type == Allies)     ||
	    (type == TeamID)     ||
	    (type == PlayerName) ||
	    (type == PlayerCPU)  ||
	    (type == PlayerPing)) {
		compareType = type;
		SetCompareFunc();
		return true;
	}
	compareType = Disabled;
	SetCompareFunc();
	return false;
}


const char* PlayerRoster::GetSortName()
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
	return NULL;
}


/******************************************************************************/

const std::vector<int>& PlayerRoster::GetIndices(int* count, bool includePathingFlag) const
{
	static std::vector<int> players;
	static int knownPlayers = 0;
	players.resize(playerHandler->ActivePlayers());

	if (playerHandler->ActivePlayers() > knownPlayers) {
		for (int i = 0; i < playerHandler->ActivePlayers(); i++) {
			players[i] = i;
		}
		knownPlayers = playerHandler->ActivePlayers();
	}

	// TODO: use std::sort
	qsort(&players[0], players.size(), sizeof(int), compareFunc);

	if (count != NULL) {
		// set the count
		int& c = *count;
		for (c = 0; c < playerHandler->ActivePlayers(); c++) {
			const CPlayer* p = playerHandler->Player(players[c]);

			if (p == NULL)
				break;
			if (p->active)
				continue;

			if (!includePathingFlag || p->ping != PATHING_FLAG || !gs->PreSimFrame()) {
				break;
			}
		}
	}

	return players;
}


/******************************************************************************/

static inline int CompareBasics(const CPlayer* aP, const CPlayer* bP)
{
	// non-NULL first  (and return 0 if both NULL)
	if ((aP != NULL) && (bP == NULL)) { return -1; }
	if ((aP == NULL) && (bP != NULL)) { return +1; }
	if ((aP == NULL) && (bP == NULL)) { return  0; }

	// active players first
	if ( aP->active && !bP->active) { return -1; }
	if (!aP->active &&  bP->active) { return +1; }

	// then pathing players
	if (gs->PreSimFrame()) {
		if (aP->ping == PATHING_FLAG && bP->ping != PATHING_FLAG) { return -1; }
		if (aP->ping != PATHING_FLAG && bP->ping == PATHING_FLAG) { return +1; }
	}

	return 0;
}


/******************************************************************************/

static int CompareAllies(const void* a, const void* b)
{
	const int aID = *((const int*)a);
	const int bID = *((const int*)b);
	const CPlayer* aP = playerHandler->Player(aID);
	const CPlayer* bP = playerHandler->Player(bID);

	const int basic = CompareBasics(aP, bP);
	if (basic != 0) {
		return basic;
	}

	// non-spectators first
	if (!aP->spectator &&  bP->spectator) { return -1; }
	if ( aP->spectator && !bP->spectator) { return +1; }

	// my player first
	const int myPNum = gu->myPlayerNum;
	if ((aID == myPNum) && (bID != myPNum)) { return -1; }
	if ((aID != myPNum) && (bID == myPNum)) { return +1; }

	// my teammates first
	const int aTeam = aP->team;
	const int bTeam = bP->team;
	const int myTeam = gu->myTeam;
	if ((aTeam == myTeam) && (bTeam != myTeam)) { return -1; }
	if ((aTeam != myTeam) && (bTeam == myTeam)) { return +1; }

	// my allies first
	const int aAlly = teamHandler->AllyTeam(aTeam);
	const int bAlly = teamHandler->AllyTeam(bTeam);
	const int myATeam = gu->myAllyTeam;
	if ((aAlly == myATeam) && (bAlly != myATeam)) { return -1; }
	if ((aAlly != myATeam) && (bAlly == myATeam)) { return +1; }

	// sort by ally team
	if (aAlly < bAlly) { return -1; }
	if (aAlly > bAlly) { return +1; }

	// sort by team
	if (aTeam < bTeam) { return -1; }
	if (aTeam > bTeam) { return +1; }

	// sort by player id
	if (aID < bID) { return -1; }
	if (aID > bID) { return +1; }

	return 0;
}


/******************************************************************************/

static int CompareTeamIDs(const void* a, const void* b)
{
	const int aID = *((const int*)a);
	const int bID = *((const int*)b);
	const CPlayer* aP = playerHandler->Player(aID);
	const CPlayer* bP = playerHandler->Player(bID);

	const int basic = CompareBasics(aP, bP);
	if (basic != 0) {
		return basic;
	}

	// sort by team id
	if (aP->team < bP->team) { return -1; }
	if (aP->team > bP->team) { return +1; }

	// sort by player id
	if (aID < bID) { return -1; }
	if (aID > bID) { return +1; }

	return 0;
}


/******************************************************************************/

static int ComparePlayerNames(const void* a, const void* b)
{
	const int aID = *((const int*)a);
	const int bID = *((const int*)b);
	const CPlayer* aP = playerHandler->Player(aID);
	const CPlayer* bP = playerHandler->Player(bID);

	const int basic = CompareBasics(aP, bP);
	if (basic != 0) {
		return basic;
	}

	// sort by player name
	const std::string aName = StringToLower(aP->name);
	const std::string bName = StringToLower(bP->name);
	return strcmp(aName.c_str(), bName.c_str());
}


/******************************************************************************/

static int ComparePlayerCPUs(const void* a, const void* b)
{
	const int aID = *((const int*)a);
	const int bID = *((const int*)b);
	const CPlayer* aP = playerHandler->Player(aID);
	const CPlayer* bP = playerHandler->Player(bID);

	const int basic = CompareBasics(aP, bP);
	if (basic != 0) {
		return basic;
	}

	// sort by player cpu usage
	if (aP->cpuUsage < bP->cpuUsage) { return -1; }
	if (aP->cpuUsage > bP->cpuUsage) { return +1; }

	return 0;
}


/******************************************************************************/

static int ComparePlayerPings(const void* a, const void* b)
{
	const int aID = *((const int*)a);
	const int bID = *((const int*)b);
	const CPlayer* aP = playerHandler->Player(aID);
	const CPlayer* bP = playerHandler->Player(bID);

	const int basic = CompareBasics(aP, bP);
	if (basic != 0) {
		return basic;
	}

	// sort by player pings
	if (aP->ping < bP->ping) { return -1; }
	if (aP->ping > bP->ping) { return +1; }

	return 0;
}


/******************************************************************************/
