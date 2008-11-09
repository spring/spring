#include "StdAfx.h"
#include "GlobalSynced.h"

#include <assert.h>
#include <cstring>

#include "Game/GameSetup.h"
#include "Team.h"
#include "Game/Player.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"

/**
 * @brief global synced
 *
 * Global instance of CGlobalSyncedStuff
 */
CGlobalSyncedStuff* gs;



CR_BIND(CGlobalSyncedStuff,);

CR_REG_METADATA(CGlobalSyncedStuff, (
	CR_MEMBER(randSeed),
	CR_MEMBER(initRandSeed),
	CR_MEMBER(frameNum),
	CR_MEMBER(speedFactor),
	CR_MEMBER(userSpeedFactor),
	CR_MEMBER(paused),
	CR_MEMBER(tempNum),
	CR_MEMBER(godMode),
	CR_MEMBER(globalLOS),
	CR_MEMBER(cheatEnabled),
	CR_MEMBER(noHelperAIs),
	CR_MEMBER(editDefsEnabled),
	CR_MEMBER(useLuaRules),
	CR_MEMBER(useLuaGaia),
	CR_MEMBER(gaiaTeamID),
	CR_MEMBER(gaiaAllyTeamID),
	CR_MEMBER(gameMode),
	CR_MEMBER(players),
	CR_MEMBER(activeTeams),
	CR_MEMBER(activeAllyTeams),
	CR_MEMBER(activePlayers),
	CR_MEMBER(allies),
	CR_MEMBER(team2allyteam),
	CR_MEMBER(teams),
	CR_RESERVED(64)
));


/**
 * Initializes variables in CGlobalSyncedStuff
 */
CGlobalSyncedStuff::CGlobalSyncedStuff()
{
	hmapx = 256;
	hmapy = 256;
	randSeed = 18655;
	initRandSeed = randSeed;
	frameNum = 0;
	speedFactor = 1;
	userSpeedFactor = 1;
	paused = false;
	godMode = false;
	globalLOS = false;
	cheatEnabled = false;
	noHelperAIs = false;
	editDefsEnabled = false;
	tempNum = 2;
	gameMode = 0;
	useLuaGaia = true;
	gaiaTeamID = -1;
	gaiaAllyTeamID = -1;
	useLuaRules = true;
	
	for(int a = 0; a < MAX_TEAMS; ++a) {
		teams[a] = SAFE_NEW CTeam();
		teams[a]->teamNum = a;
		team2allyteam[a] = a;
	}
	for(int a = 0; a < MAX_PLAYERS; ++a) {
		players[a] = SAFE_NEW CPlayer();
		players[a]->playerNum = a;
	}

	for (int a = 0; a < MAX_TEAMS; ++a) {
		for (int b = 0; b < MAX_TEAMS; ++b) {
			allies[a][b] = false;
		}
		allies[a][a] = true;
	}

	activeTeams = 2;
	activeAllyTeams = 2;
	activePlayers = MAX_PLAYERS;
}

/**
 * Destroys data inside CGlobalSyncedStuff
 */
CGlobalSyncedStuff::~CGlobalSyncedStuff()
{
	for(int a = 0; a < MAX_TEAMS; a++) {
		delete teams[a];
	}
	for(int a = 0; a < MAX_PLAYERS; a++) {
		delete players[a];
	}
}

void CGlobalSyncedStuff::LoadFromSetup(const CGameSetup* setup)
{
	gameMode = setup->gameMode;
	noHelperAIs = !!setup->noHelperAIs;

	useLuaGaia  = CLuaGaia::SetConfigString(setup->luaGaiaStr);
	useLuaRules = CLuaRules::SetConfigString(setup->luaRulesStr);

	activePlayers = setup->numPlayers;
	activeTeams = setup->numTeams;
	activeAllyTeams = setup->numAllyTeams;
	
	assert(activeTeams <= MAX_TEAMS);
	assert(activeAllyTeams <= MAX_TEAMS);

	for (int i = 0; i < activePlayers; ++i)
	{
		// TODO: refactor
		*static_cast<PlayerBase*>(players[i]) = setup->playerStartingData[i];
	}

	for (int i = 0; i < activeTeams; ++i)
	{
		teams[i]->metal = setup->startMetal;
		teams[i]->metalIncome = setup->startMetal; // for the endgame statistics

		teams[i]->energy = setup->startEnergy;
		teams[i]->energyIncome = setup->startEnergy;

		float3 start(setup->teamStartingData[i].startPos.x, setup->teamStartingData[i].startPos.y, setup->teamStartingData[i].startPos.z);
		teams[i]->StartposMessage(start, (setup->startPosType != CGameSetup::StartPos_ChooseInGame));
		std::memcpy(teams[i]->color, setup->teamStartingData[i].color, 4);
		teams[i]->handicap = setup->teamStartingData[i].handicap;
		teams[i]->leader = setup->teamStartingData[i].leader;
		teams[i]->side = setup->teamStartingData[i].side;
		SetAllyTeam(i, setup->teamStartingData[i].teamAllyteam);
		if (!setup->teamStartingData[i].aiDll.empty())
		{
			if (setup->teamStartingData[i].aiDll.substr(0, 6) == "LuaAI:") // its a LuaAI
			{
				teams[i]->luaAI = setup->teamStartingData[i].aiDll.substr(6);
				teams[i]->isAI = true;
			}
			else // no LuaAI
			{
				if (setup->hostDemo) // in demo replay, we don't need AI's to load again
					teams[i]->dllAI = "";
				else
				{
					teams[i]->dllAI = setup->teamStartingData[i].aiDll;
					teams[i]->isAI = true;
				}
			}
		}
	}

	for (int allyTeam1 = 0; allyTeam1 < activeAllyTeams; ++allyTeam1)
	{
		for (int allyTeam2 = 0; allyTeam2 < activeAllyTeams; ++allyTeam2)
			allies[allyTeam1][allyTeam2] = setup->allyStartingData[allyTeam1].allies[allyTeam2];
	}

	if (useLuaGaia) {
		// Gaia adjustments
		gaiaTeamID = activeTeams;
		gaiaAllyTeamID = activeAllyTeams;
		activeTeams++;
		activeAllyTeams++;

		// Setup the gaia team
		CTeam* team = gs->Team(gs->gaiaTeamID);
		team->color[0] = 255;
		team->color[1] = 255;
		team->color[2] = 255;
		team->color[3] = 255;
		team->gaia = true;
		team->StartposMessage(float3(0.0, 0.0, 0.0), true);
		//players[setup->numPlayers]->team = gaiaTeamID;
		SetAllyTeam(gaiaTeamID, gaiaAllyTeamID);
	}
}

/**
 * @return synced random integer
 *
 * returns a synced random integer
 */
int CGlobalSyncedStuff::randInt()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return randSeed & RANDINT_MAX;
}

/**
 * @return synced random float
 *
 * returns a synced random float
 */
float CGlobalSyncedStuff::randFloat()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return float(randSeed & RANDINT_MAX)/RANDINT_MAX;
}

/**
 * @return synced random vector
 *
 * returns a synced random vector
 */
float3 CGlobalSyncedStuff::randVector()
{
	float3 ret;
	do {
		ret.x = randFloat()*2-1;
		ret.y = randFloat()*2-1;
		ret.z = randFloat()*2-1;
	} while(ret.SqLength()>1);

	return ret;
}

int CGlobalSyncedStuff::Player(const std::string& name)
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (players[i] && players[i]->name == name) {
			return i;
		}
	}
	return -1;
}
