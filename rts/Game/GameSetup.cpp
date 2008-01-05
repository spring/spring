#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <SDL.h>
#include "CameraHandler.h"
#include "GameSetup.h"
#include "GameVersion.h"
#include "LogOutput.h"
#include "GameServer.h"
#include "Player.h"
#include "TdfParser.h"
#include "Team.h"
#include "System/NetProtocol.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "TdfParser.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaParser.h"
#include "Map/ReadMap.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/Textures/TAPalette.h"

CGameSetup* gameSetup=0;

using namespace std;
extern Uint8 *keys;

extern string stupidGlobalMapname;

CGameSetup::CGameSetup()
{
}

CGameSetup::~CGameSetup()
{
}

bool CGameSetup::Init(std::string setupFile)
{
	setupFileName = setupFile;
	if(setupFile.empty())
		return false;
	CFileHandler fh(setupFile);
	if (!fh.FileExists())
		return false;
	char* c=SAFE_NEW char[fh.FileSize()];
	fh.Read(c,fh.FileSize());

	bool ret=Init(c,fh.FileSize());

	delete[] c;

	return ret;
}

/** @brief random number generator function object for use with std::random_shuffle */
struct UnsyncedRandomNumberGenerator {
	/** @brief returns a random number in the range [0, n) */
	int operator()(int n) {
		// a simple gu->usRandInt() % n isn't random enough
		return gu->usRandInt() * n / (RANDINT_MAX + 1);
	}
};

/**
@brief Determine if the map is inside an archive, and possibly map needed archives
@pre mapName initialized
@post map archives (if any) have been mapped into the VFS
 */
void CGameSetup::LoadMap()
{
	CFileHandler* f = SAFE_NEW CFileHandler("maps/" + mapName);
	if (!f->FileExists()) {
		vector<string> ars = archiveScanner->GetArchivesForMap(mapName);
		if (ars.empty())
			throw content_error("Couldn't find any map archives for map '" + mapName + "'.");
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
			if (!hpiHandler->AddArchive(*i, false))
				throw content_error("Couldn't load archive '" + *i + "' for map '" + mapName + "'.");
		}
	}
	delete f;
}

/**
@brief Load unit restrictions
@post restrictedUnits initialized
 */
void CGameSetup::LoadUnitRestrictions(const TdfParser& file)
{
	int numRestrictions;
	file.GetDef(numRestrictions, "0", "GAME\\NumRestrictions");

	for (int i = 0; i < numRestrictions; ++i) {
		char key[100];
		sprintf(key, "GAME\\RESTRICT\\Unit%d", i);
		string resName = file.SGetValueDef("", key);
		sprintf(key, "GAME\\RESTRICT\\Limit%d", i);
		int resLimit;
		file.GetDef(resLimit, "0", key);

		restrictedUnits[resName] = resLimit;
	}
}

/**
@brief Load startpositions from map
@pre mapName, numTeams, teamStartNum initialized and the map loaded (LoadMap())
@todo don't store in global variables directly
 */
void CGameSetup::LoadStartPositionsFromMap()
{
	TdfParser p2;
	CReadMap::OpenTDF (mapName, p2);

	for(int a=0;a<numTeams;++a){
		float x,z;
		char teamName[50];
		sprintf(teamName, "TEAM%i", teamStartNum[a]);
		p2.GetDef(x, "1000", string("MAP\\") + teamName + "\\StartPosX");
		p2.GetDef(z, "1000", string("MAP\\") + teamName + "\\StartPosZ");
		gs->Team(a)->startPos = float3(x, 100, z);
	}
}

/**
@brief Load startpositions from map/script
@pre numTeams and startPosType initialized
@post readyTeams, teamStartNum and team start positions initialized
@todo don't store in global variables directly
 */
void CGameSetup::LoadStartPositions(const TdfParser& file)
{
	for (int a = 0; a < numTeams; ++a) {
		// Ready up automatically unless startPosType is choose in game
		readyTeams[a] = (startPosType != StartPos_ChooseInGame);
		teamStartNum[a] = a;
	}

	if (startPosType == StartPos_Random) {
		// Server syncs these later, so we can use unsynced rng
		UnsyncedRandomNumberGenerator rng;
		std::random_shuffle(&teamStartNum[0], &teamStartNum[numTeams], rng);
	}

	LoadStartPositionsFromMap();

	// Show that we havent selected start pos yet
	if (startPosType == StartPos_ChooseInGame) {
		for (int a = 0; a < numTeams; ++a)
			gs->Team(a)->startPos.y = -500;
	}

	// Load start position from gameSetup script
	if (startPosType == StartPos_ChooseBeforeGame) {
		for (int a = 0; a < numTeams; ++a) {
			char section[50];
			sprintf(section, "GAME\\TEAM%i\\", a);
			string s(section);
			std::string xpos = file.SGetValueDef("", s + "StartPosX");
			std::string zpos = file.SGetValueDef("", s + "StartPosZ");
			if (!xpos.empty()) gs->Team(a)->startPos.x = atoi(xpos.c_str());
			if (!zpos.empty()) gs->Team(a)->startPos.z = atoi(zpos.c_str());
		}
	}
}

/**
@brief Load players and remove gaps in the player numbering.
@pre numPlayers initialized
@post players loaded, numDemoPlayers initialized
@todo don't store in global variables directly
 */
void CGameSetup::LoadPlayers(const TdfParser& file)
{
	numDemoPlayers = 0;
	for (int a = 0; a < MAX_PLAYERS; a++) {
		gs->players[a]->team = 0; //needed in case one tries to spec a game with only one team
	}
	// i = player index in game (no gaps), a = player index in script
	int i = 0;
	for (int a = 0; a < MAX_PLAYERS; ++a) {
		char section[50];
		sprintf(section, "GAME\\PLAYER%i\\", a);
		string s(section);

		if (!file.SectionExist(s.substr(0, s.length() - 1)))
			continue;

		// expects lines of form team=x rather than team=TEAMx
		// team field is relocated in RemapTeams
		gs->players[i]->team = atoi(file.SGetValueDef("0",   s + "team").c_str());
		gs->players[i]->rank = atoi(file.SGetValueDef("-1",  s + "rank").c_str());
		gs->players[i]->playerName  = file.SGetValueDef("no name", s + "name");
		gs->players[i]->countryCode = file.SGetValueDef("",  s + "countryCode");
		gs->players[i]->spectator = !!atoi(file.SGetValueDef("0", s + "spectator").c_str());

		int fromDemo;
		file.GetDef(fromDemo, "0", s + "IsFromDemo");
		if (fromDemo)
			numDemoPlayers++;

		playerRemap[a] = i;
		++i;
	}

	if (playerRemap.size() != numPlayers)
		throw content_error("incorrect number of players in GameSetup script");
}

/**
@brief Load teams and remove gaps in the team numbering.
@pre numTeams, hostDemo initialized
@post teams loaded
@todo don't store in global variables directly
 */
void CGameSetup::LoadTeams(const TdfParser& file)
{
	// i = team index in game (no gaps), a = team index in script
	int i = 0;
	for (int a = 0; a < MAX_TEAMS; ++a) {
		char section[50];
		sprintf(section, "GAME\\TEAM%i\\", a);
		string s(section);

		if (!file.SectionExist(s.substr(0, s.length() - 1)))
			continue;

		// Get default color from palette (based on "color" tag)
		int colorNum = atoi(file.SGetValueDef("0", s + "color").c_str());
		colorNum %= palette.NumTeamColors();
		float3 defaultCol(palette.teamColor[colorNum][0] / 255.0f, palette.teamColor[colorNum][1] / 255.0f, palette.teamColor[colorNum][2] / 255.0f);

		// Read RGBColor, this overrides color if both had been set.
		float3 color = file.GetFloat3(defaultCol, s + "rgbcolor");
		for (int b = 0; b < 3; ++b) {
			gs->Team(i)->color[b] = int(color[b] * 255);
		}
		gs->Team(i)->color[3] = 255;

		gs->Team(i)->handicap = atof(file.SGetValueDef("0", s + "handicap").c_str()) / 100 + 1;
		// leader field is relocated in RemapPlayers
		gs->Team(i)->leader = atoi(file.SGetValueDef("0", s + "teamleader").c_str());
		gs->Team(i)->side = StringToLower(file.SGetValueDef("arm", s + "side").c_str());
		// allyteam field is relocated in RemapAllyteams
		gs->SetAllyTeam(i, atoi(file.SGetValueDef("0", s + "allyteam").c_str()));

		// Is this team (Lua) AI controlled?
		// If this is a demo replay, non-Lua AIs aren't loaded.
		const string aiDll = file.SGetValueDef("", s + "aidll");
		if (aiDll.substr(0, 6) == "LuaAI:") {
			gs->Team(i)->luaAI = aiDll.substr(6);
		} else {
			if (hostDemo) {
				aiDlls[i] = "";
			} else {
				aiDlls[i] = aiDll;
			}
		}

		teamRemap[a] = i;
		++i;
	}

	if (teamRemap.size() != numTeams)
		throw content_error("incorrect number of teams in GameSetup script");
}

/**
@brief Load allyteams and remove gaps in the allyteam numbering.
@pre numAllyTeams initialized
@post allyteams loaded
@todo don't store in global variables directly
*/
void CGameSetup::LoadAllyTeams(const TdfParser& file)
{
	// i = allyteam index in game (no gaps), a = allyteam index in script
	int i = 0;
	for (int a = 0; a < MAX_TEAMS; ++a) {
		char section[50];
		sprintf(section,"GAME\\ALLYTEAM%i\\",a);
		string s(section);

		if (!file.SectionExist(s.substr(0, s.length() - 1)))
			continue;

		startRectTop[i]    = atof(file.SGetValueDef("0", s + "StartRectTop").c_str());
		startRectBottom[i] = atof(file.SGetValueDef("1", s + "StartRectBottom").c_str());
		startRectLeft[i]   = atof(file.SGetValueDef("0", s + "StartRectLeft").c_str());
		startRectRight[i]  = atof(file.SGetValueDef("1", s + "StartRectRight").c_str());

		int numAllies = atoi(file.SGetValueDef("0", s + "NumAllies").c_str());

		for (int b = 0; b < numAllies; ++b) {
			char key[100];
			sprintf(key, "GAME\\ALLYTEAM%i\\Ally%i", a, b);
			int other = atoi(file.SGetValueDef("0",key).c_str());
			gs->SetAlly(a, other, true); // relocated in RemapAllyteams
		}

		allyteamRemap[a] = i;
		++i;
	}

	if (allyteamRemap.size() != numAllyTeams)
		throw content_error("incorrect number of allyteams in GameSetup script");
}

/** @brief Update all player indices to refer to the right player. */
void CGameSetup::RemapPlayers()
{
	// relocate Team.TeamLeader field
	for (int a = 0; a < numTeams; ++a) {
		if (playerRemap.find(gs->Team(a)->leader) == playerRemap.end())
			throw content_error("invalid Team.TeamLeader in GameSetup script");
		gs->Team(a)->leader = playerRemap[gs->Team(a)->leader];
	}

	// relocate myPlayerNum
	if (playerRemap.find(myPlayerNum) == playerRemap.end())
		throw content_error("invalid MyPlayerNum in GameSetup script");
	myPlayerNum = playerRemap[myPlayerNum];
}

/** @brief Update all team indices to refer to the right team. */
void CGameSetup::RemapTeams()
{
	// relocate Player.Team field
	for (int a = 0; a < numPlayers; ++a) {
		if (teamRemap.find(gs->players[a]->team) == teamRemap.end())
			throw content_error("invalid Player.Team in GameSetup script");
		gs->players[a]->team = teamRemap[gs->players[a]->team];
	}
}

/** @brief Update all allyteam indices to refer to the right allyteams. */
void CGameSetup::RemapAllyteams()
{
	// relocate Team.Allyteam field
	for (int a = 0; a < numTeams; ++a) {
		if (allyteamRemap.find(gs->AllyTeam(a)) == allyteamRemap.end())
			throw content_error("invalid Team.Allyteam in GameSetup script");
		gs->SetAllyTeam(a, allyteamRemap[gs->AllyTeam(a)]);
	}

	// relocate gs->allies matrix
	for (int a = 0; a < MAX_TEAMS; ++a) {
		for (int b = 0; b < MAX_TEAMS; ++b) {
			if (allyteamRemap.find(a) != allyteamRemap.end() &&
				allyteamRemap.find(b) != allyteamRemap.end()) {
				gs->SetAlly(allyteamRemap[a], allyteamRemap[b], gs->Ally(a, b));
			}
		}
	}
}

bool CGameSetup::Init(const char* buf, int size)
{
	// Copy buffer contents
	gameSetupText = SAFE_NEW char[size];
	memcpy(gameSetupText, buf, size);
	gameSetupTextLength = size;

	// Parse
	TdfParser file;
	file.LoadBuffer(buf, size);

	if(!file.SectionExist("GAME"))
		return false;

	// Technical parameters
	file.GetDef(hostip,     "0", "GAME\\HostIP");
	file.GetDef(hostport,   "0", "GAME\\HostPort");
	file.GetDef(sourceport, "0", "GAME\\SourcePort");

	// Game parameters
	scriptName  = file.SGetValueDef("Commanders", "GAME\\ScriptName");
	baseMod     = file.SGetValueDef("",  "GAME\\Gametype");
	mapName     = file.SGetValueDef("",  "GAME\\MapName");
	luaGaiaStr  = file.SGetValueDef("1", "GAME\\LuaGaia");
	luaRulesStr = file.SGetValueDef("1", "GAME\\LuaRules");
	saveName    = file.SGetValueDef("",  "GAME\\Savefile");
	demoName    = file.SGetValueDef("",  "GAME\\Demofile");
	hostDemo    = !demoName.empty();

	file.GetDef(gameMode,         "0", "GAME\\GameMode");
	file.GetDef(noHelperAIs,      "0", "GAME\\NoHelperAIs");
	file.GetDef(maxUnits,       "500", "GAME\\MaxUnits");
	file.GetDef(limitDgun,        "0", "GAME\\LimitDgun");
	file.GetDef(diminishingMMs,   "0", "GAME\\DiminishingMMs");
	file.GetDef(disableMapDamage, "0", "GAME\\DisableMapDamage");
	file.GetDef(ghostedBuildings, "1", "GAME\\GhostedBuildings");
	file.GetDef(startMetal,    "1000", "GAME\\StartMetal");
	file.GetDef(startEnergy,   "1000", "GAME\\StartEnergy");

	file.GetDef(maxSpeed, "3.0", "GAME\\MaxSpeed");
	file.GetDef(minSpeed, "0.3", "GAME\\MinSpeed");

	file.GetDef(myPlayerNum,  "0", "GAME\\MyPlayerNum");
	file.GetDef(numPlayers,   "2", "GAME\\NumPlayers");
	file.GetDef(numTeams,     "2", "GAME\\NumTeams");
	file.GetDef(numAllyTeams, "2", "GAME\\NumAllyTeams");

	// Read the map & mod options
	mapOptions = file.GetAllValues("GAME\\MapOptions");
	modOptions = file.GetAllValues("GAME\\ModOptions");

	// Read startPosType (with clamping)
	int startPosTypeInt;
	file.GetDef(startPosTypeInt, "0", "GAME\\StartPosType");
	if (startPosTypeInt < 0 || startPosTypeInt > StartPos_Last)
		startPosTypeInt = 0;
	startPosType = (StartPosType)startPosTypeInt;

	// Read subsections
	LoadPlayers(file);
	LoadTeams(file);
	LoadAllyTeams(file);

	// Relocate indices (for gap removing)
	RemapPlayers();
	RemapTeams();
	RemapAllyteams();

	LoadUnitRestrictions(file);

	LoadMap();
	LoadStartPositions(file);

	// Postprocessing
	baseMod = archiveScanner->ModArchiveToModName(baseMod);

	gu->myPlayerNum = myPlayerNum;
	gu->myTeam = gs->players[myPlayerNum]->team;
	gu->myAllyTeam = gs->AllyTeam(gu->myTeam);

	gu->spectating = gs->players[myPlayerNum]->spectator;
	gu->spectatingFullView   = gu->spectating;
	gu->spectatingFullSelect = gu->spectating;

	gs->gameMode = gameMode;
	gs->noHelperAIs = !!noHelperAIs;

	gs->useLuaGaia  = CLuaGaia::SetConfigString(luaGaiaStr);
	gs->useLuaRules = CLuaRules::SetConfigString(luaRulesStr);

	gs->activeTeams = numTeams;
	gs->activeAllyTeams = numAllyTeams;

	if (gs->useLuaGaia) {
		// Gaia adjustments
		gs->gaiaTeamID = gs->activeTeams;
		gs->gaiaAllyTeamID = gs->activeAllyTeams;
		gs->activeTeams++;
		gs->activeAllyTeams++;
		teamStartNum[gs->gaiaTeamID] = gs->gaiaTeamID;

		// Setup the gaia team
		CTeam* team = gs->Team(gs->gaiaTeamID);
		team->color[0] = 255;
		team->color[1] = 255;
		team->color[2] = 255;
		team->color[3] = 255;
		team->gaia = true;
		readyTeams[gs->gaiaTeamID] = true;
		gs->players[numPlayers]->team = gs->gaiaTeamID;
		gs->SetAllyTeam(gs->gaiaTeamID, gs->gaiaAllyTeamID);
	}

	for(int a = 0; a < gs->activeTeams; ++a) {
		gs->Team(a)->metal = startMetal;
		gs->Team(a)->metalIncome = startMetal; // for the endgame statistics

		gs->Team(a)->energy = startEnergy;
		gs->Team(a)->energyIncome = startEnergy;
	}

	assert(gs->activeTeams <= MAX_TEAMS);
	assert(gs->activeAllyTeams <= MAX_TEAMS);

	return true;
}

bool CGameSetup::Update()
{
	bool allReady = true;
	if (!gameServer) // let the server/host start the game
		return false;
	for (int a = numDemoPlayers; a < numPlayers; a++) {
		if (!gs->players[a]->readyToStart) {
			allReady = false;
			break;
		} else if (!readyTeams[gs->players[a]->team] && !hostDemo)
		{
			allReady = false;
			break;
		}
	}

	if (forceReady)
		allReady = true;
	if (allReady) {
		if (readyTime == 0) {
			int mode = configHandler.GetInt("CamMode", 1);
			camHandler->SetCameraMode(mode);
			readyTime = SDL_GetTicks();
		}
	}
	return (readyTime && (SDL_GetTicks() - readyTime) > 3000);
}
