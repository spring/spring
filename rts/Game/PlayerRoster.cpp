// PlayerRoster.cpp: implementation of PlayerRoster namespace
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include <assert.h>

#include "PlayerRoster.h"
#include "Player.h"
#include "Team.h"
#include "Util.h"
#include "GlobalSynced.h"
#include "GlobalUnsynced.h"

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

const int* PlayerRoster::GetIndices(int* count)
{
	static int players[MAX_PLAYERS];
	static int knownPlayers = 0;

	if (gs->activePlayers > knownPlayers) {
		for (int i = 0; i < gs->activePlayers; i++) {
			players[i] = i;
		}
		knownPlayers = gs->activePlayers;
	}

	qsort(players, gs->activePlayers, sizeof(int), compareFunc);

	if (count != NULL) {
		// set the count
		int& c = *count;
		for (c = 0; c < gs->activePlayers; c++) {
			const CPlayer* p = gs->players[players[c]];
			if ((p == NULL) || !p->active) {
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

	return 0;
}


/******************************************************************************/

static int CompareAllies(const void* a, const void* b)
{
	const int aID = *((const int*)a);
	const int bID = *((const int*)b);
	const CPlayer* aP = gs->players[aID];
	const CPlayer* bP = gs->players[bID];

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
	const int aAlly = gs->AllyTeam(aTeam);
	const int bAlly = gs->AllyTeam(bTeam);
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
	const CPlayer* aP = gs->players[aID];
	const CPlayer* bP = gs->players[bID];

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
	const CPlayer* aP = gs->players[aID];
	const CPlayer* bP = gs->players[bID];

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
	const CPlayer* aP = gs->players[aID];
	const CPlayer* bP = gs->players[bID];

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
	const CPlayer* aP = gs->players[aID];
	const CPlayer* bP = gs->players[bID];

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
