/**
 * @file GlobalStuff.cpp
 * @brief Globally accessible stuff
 *
 * Contains implementation of synced and
 * unsynced global stuff
 */
#include "StdAfx.h"
#include <cstring>
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Sync/SyncTracer.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "Rendering/Textures/TAPalette.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "SDL_types.h"
#include "SDL_timer.h"
#include "mmgr.h"

/**
 * @brief global synced
 *
 * Global instance of CGlobalSyncedStuff
 */
CGlobalSyncedStuff* gs;

/**
 * @brief global unsynced
 *
 * Global instance of CGlobalUnsyncedStuff
 */
CGlobalUnsyncedStuff* gu;

/**
 * @brief randint max
 *
 * Defines the maximum random integer as 0x7fff
 */
const int RANDINT_MAX = 0x7fff;

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

CR_BIND(CGlobalUnsyncedStuff,);

CR_REG_METADATA(CGlobalUnsyncedStuff, (
				CR_MEMBER(usRandSeed),
				CR_MEMBER(modGameTime),
				CR_MEMBER(gameTime),
				CR_MEMBER(lastFrameTime),
				CR_MEMBER(myPlayerNum),
				CR_MEMBER(myTeam),
				CR_MEMBER(myAllyTeam),
				CR_MEMBER(spectating),
				CR_MEMBER(spectatingFullView),
				CR_MEMBER(spectatingFullSelect),
				CR_MEMBER(viewRange),
				CR_MEMBER(timeOffset),
				CR_MEMBER(drawFog),
				CR_MEMBER(autoQuit),
				CR_MEMBER(quitTime),
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
	for(int a = 0; a < gs->activePlayers; a++) {
		delete players[a];
	}
}

void CGlobalSyncedStuff::LoadFromSetup(const CGameSetup* setup)
{
	gameMode = setup->gameMode;
	noHelperAIs = !!setup->noHelperAIs;

	useLuaGaia  = CLuaGaia::SetConfigString(setup->luaGaiaStr);
	useLuaRules = CLuaRules::SetConfigString(setup->luaRulesStr);

	activeTeams = setup->numTeams;
	activeAllyTeams = setup->numAllyTeams;
	
	assert(activeTeams <= MAX_TEAMS);
	assert(activeAllyTeams <= MAX_TEAMS);
	
	for (unsigned i = 0; i < static_cast<unsigned>(setup->numPlayers); ++i)
	{
		*static_cast<PlayerBase*>(players[i]) = setup->playerStartingData[i];
	}
	
	for (unsigned i = 0; i < static_cast<unsigned>(activeTeams); ++i)
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
		if (setup->teamStartingData[i].aiDll.substr(0, 6) == "LuaAI:") {
			teams[i]->luaAI = setup->teamStartingData[i].aiDll.substr(6);
			teams[i]->isAI = true;
		}
		else {
			if (setup->hostDemo)
				teams[i]->dllAI = "";
			else
			{
				teams[i]->dllAI = setup->teamStartingData[i].aiDll;
				teams[i]->isAI = true;
			}
		}
		for (unsigned t = 0; t < static_cast<unsigned>(MAX_TEAMS); ++t)
		{
			allies[i][t] = setup->allyStartingData[i].allies[t];
		}
	}

	if (useLuaGaia) {
		//TODO duplicated in SpringApp::CreateGameSetup()
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

/**
 * Initializes variables in CGlobalUnsyncedStuff
 */
CGlobalUnsyncedStuff::CGlobalUnsyncedStuff()
{
	Uint64 randnum;
	randnum = SDL_GetTicks();
	usRandSeed = randnum&0xffffffff;
	modGameTime = 0;
	gameTime = 0;
	lastFrameTime = 0;
	drawFrame = 1;
	viewSizeX = 100;
	viewSizeY = 100;
	pixelX = 0.01f;
	pixelY = 0.01f;
	aspectRatio = 1.0f;
	myPlayerNum = 0;
	myTeam = 1;
	myAllyTeam = 1;
	spectating           = false;
	spectatingFullView   = false;
	spectatingFullSelect = false;
	drawdebug = false;
	active = true;
	viewRange = MAX_VIEW_RANGE;
	timeOffset = 0;
	drawFog = true;
	compressTextures = false;
	atiHacks = false;
	teamNanospray = false;
	autoQuit = false;
	quitTime = 0;
#ifdef DIRECT_CONTROL_ALLOWED
	directControl = 0;
#endif
}

/**
 * Destroys variables in CGlobalUnsyncedStuff
 */
CGlobalUnsyncedStuff::~CGlobalUnsyncedStuff()
{
}

/**
 * @return unsynced random integer
 *
 * Returns an unsynced random integer
 */
int CGlobalUnsyncedStuff::usRandInt()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return usRandSeed & RANDINT_MAX;
}

/**
 * @return unsynced random float
 *
 * returns an unsynced random float
 */
float CGlobalUnsyncedStuff::usRandFloat()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return float(usRandSeed & RANDINT_MAX)/RANDINT_MAX;
}

/**
 * @return unsynced random vector
 *
 * returns an unsynced random vector
 */
float3 CGlobalUnsyncedStuff::usRandVector()
{
	float3 ret;
	do {
		ret.x = usRandFloat() * 2 - 1;
		ret.y = usRandFloat() * 2 - 1;
		ret.z = usRandFloat() * 2 - 1;
	} while (ret.SqLength() > 1);

	return ret;
}

void CGlobalUnsyncedStuff::LoadFromSetup(const CGameSetup* setup)
{
	myPlayerNum = setup->myPlayerNum;
	myTeam = setup->playerStartingData[myPlayerNum].team;
	myAllyTeam = setup->teamStartingData[myPlayerNum].teamAllyteam;

	spectating = setup->playerStartingData[myPlayerNum].spectator;
	spectatingFullView   = setup->playerStartingData[myPlayerNum].spectator;
	spectatingFullSelect = setup->playerStartingData[myPlayerNum].spectator;
}

