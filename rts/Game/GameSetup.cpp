#include "StdAfx.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <SDL_timer.h>

#ifndef DEDICATED
	#include "Team.h"
#endif
#include "GameSetup.h"
#include "TdfParser.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "TdfParser.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Rendering/Textures/TAPalette.h"
#include "System/UnsyncedRNG.h"


using namespace std;


const CGameSetup* gameSetup = NULL;


CGameSetup::CGameSetup()
{
	gameSetupText=0;
	startPosType=StartPos_Fixed;
	numDemoPlayers=0;
	hostDemo=false;
	autohostport = 0;
}

CGameSetup::~CGameSetup()
{
	if (gameSetupText)
		delete[] gameSetupText;
}

bool CGameSetup::Init(std::string setupFile)
{
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
 */
void CGameSetup::LoadStartPositionsFromMap()
{
	MapParser mapParser(mapName);

	for(int a = 0; a < numTeams; ++a) {
		float3 pos(1000.0f, 100.0f, 1000.0f);
		mapParser.GetStartPos(a, pos);
		teamStartingData[a].startPos = SFloat3(pos.x, pos.y, pos.z);
	}
}

/**
@brief Load startpositions from map/script
@pre numTeams and startPosType initialized
@post readyTeams, teamStartNum and team start positions initialized

Unlike the other functions, this is not called on Init() , instead we wait for CPreGame to call this. The reason is that the map is not known before CPreGame recieves the gamedata from the server.
 */
void CGameSetup::LoadStartPositions()
{
	TdfParser file;
	file.LoadBuffer(gameSetupText, gameSetupTextLength-1);
	for (int a = 0; a < numTeams; ++a) {
		// Ready up automatically unless startPosType is choose in game
		//teamStartingData[a].readyTeams = (startPosType != StartPos_ChooseInGame);
		teamStartingData[a].teamStartNum = a;
	}

	if (startPosType == StartPos_Random) {
		// Server syncs these later, so we can use unsynced rng
		UnsyncedRNG rng;
		rng.Seed(gameSetupTextLength ^ SDL_GetTicks());
		int teamStartNum[MAX_TEAMS];
		for (int i = 0; i < MAX_TEAMS; ++i)
			teamStartNum[i] = i;
		std::random_shuffle(&teamStartNum[0], &teamStartNum[numTeams], rng);
		for (int i = 0; i < MAX_TEAMS; ++i)
			teamStartingData[i].teamStartNum = teamStartNum[i];
	}

	LoadStartPositionsFromMap();

	// Show that we havent selected start pos yet
	if (startPosType == StartPos_ChooseInGame) {
		for (int a = 0; a < numTeams; ++a) {
			teamStartingData[a].startPos.y = -500;
		}
	}

	// Load start position from gameSetup script
	if (startPosType == StartPos_ChooseBeforeGame) {
		for (int a = 0; a < numTeams; ++a) {
			char section[50];
			sprintf(section, "GAME\\TEAM%i\\", a);
			string s(section);
			std::string xpos = file.SGetValueDef("", s + "StartPosX");
			std::string zpos = file.SGetValueDef("", s + "StartPosZ");
			if (!xpos.empty())
			{
				teamStartingData[a].startPos.x = atoi(xpos.c_str());
			}
			if (!zpos.empty())
			{
				teamStartingData[a].startPos.z = atoi(zpos.c_str());
			}
		}
	}
}

/**
@brief Load players and remove gaps in the player numbering.
@pre numPlayers initialized
@post players loaded, numDemoPlayers initialized
 */
void CGameSetup::LoadPlayers(const TdfParser& file)
{
	numDemoPlayers = 0;
	// i = player index in game (no gaps), a = player index in script
	int i = 0;
	for (int a = 0; a < MAX_PLAYERS; ++a) {
		char section[50];
		sprintf(section, "GAME\\PLAYER%i", a);
		string s(section);

		if (!file.SectionExist(s)) {
			continue;
		}
		PlayerBase data;

		// expects lines of form team=x rather than team=TEAMx
		// team field is relocated in RemapTeams		
		std::map<std::string, std::string> setup = file.GetAllValues(s);
		std::map<std::string, std::string>::iterator it;
		if ((it = setup.find("team")) != setup.end())
			data.team = atoi(it->second.c_str());
		if ((it = setup.find("rank")) != setup.end())
			data.rank = atoi(it->second.c_str());
		if ((it = setup.find("name")) != setup.end())
			data.name = it->second;
		if ((it = setup.find("countryCode")) != setup.end())
			data.countryCode = it->second;
		if ((it = setup.find("spectator")) != setup.end())
			data.spectator = static_cast<bool>(atoi(it->second.c_str()));
		if ((it = setup.find("IsFromDemo")) != setup.end())
			data.isFromDemo = static_cast<bool>(atoi(it->second.c_str()));

		if (data.isFromDemo)
			numDemoPlayers++;

		playerStartingData.push_back(data);
		playerRemap[a] = i;
		++i;
	}

	int playerCount = -1;
	file.GetDef(playerCount,   "-1", "GAME\\NumPlayers");
	
	if (playerCount == -1 || playerStartingData.size() == playerCount)
		numPlayers = playerStartingData.size();
	else
		throw content_error("incorrect number of players in GameSetup script");
}

/**
@brief Load teams and remove gaps in the team numbering.
@pre numTeams, hostDemo initialized
@post teams loaded
 */
void CGameSetup::LoadTeams(const TdfParser& file)
{
	// i = team index in game (no gaps), a = team index in script
	int i = 0;
	for (int a = 0; a < MAX_TEAMS; ++a) {
		char section[50];
		sprintf(section, "GAME\\TEAM%i\\", a);
		string s(section);

		if (!file.SectionExist(s.substr(0, s.length() - 1))) {
			continue;
		}

		// Get default color from palette (based on "color" tag)
		int colorNum = atoi(file.SGetValueDef("0", s + "color").c_str());
		colorNum %= palette.NumTeamColors();
		float3 defaultCol(palette.teamColor[colorNum][0] / 255.0f, palette.teamColor[colorNum][1] / 255.0f, palette.teamColor[colorNum][2] / 255.0f);

		// Read RGBColor, this overrides color if both had been set.
		float3 color = file.GetFloat3(defaultCol, s + "rgbcolor");
		TeamData data;
		for (int b = 0; b < 3; ++b) {
			data.color[b] = int(color[b] * 255);
		}
		data.color[3] = 255;

		data.handicap = atof(file.SGetValueDef("0", s + "handicap").c_str()) / 100 + 1;
		data.leader = atoi(file.SGetValueDef("0", s + "teamleader").c_str());
		data.side = StringToLower(file.SGetValueDef("arm", s + "side").c_str());
		data.teamAllyteam = atoi(file.SGetValueDef("0", s + "allyteam").c_str());

		// Is this team (Lua) AI controlled?
		// If this is a demo replay, non-Lua AIs aren't loaded.
		const string aiDll = file.SGetValueDef("", s + "aidll");
		data.aiDll = aiDll;
		teamStartingData.push_back(data);

		teamRemap[a] = i;
		++i;
	}

	int teamCount = -1;
	file.GetDef(teamCount, "-1", "GAME\\NumTeams");
	
	if (teamCount == -1 || teamStartingData.size() == teamCount)
		numTeams = teamStartingData.size();
	else
		throw content_error("incorrect number of teams in GameSetup script");
}

/**
@brief Load allyteams and remove gaps in the allyteam numbering.
@pre numAllyTeams initialized
@post allyteams loaded
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
		AllyTeamData data;
		data.startRectTop    = atof(file.SGetValueDef("0", s + "StartRectTop").c_str());
		data.startRectBottom = atof(file.SGetValueDef("1", s + "StartRectBottom").c_str());
		data.startRectLeft   = atof(file.SGetValueDef("0", s + "StartRectLeft").c_str());
		data.startRectRight  = atof(file.SGetValueDef("1", s + "StartRectRight").c_str());

		int numAllies = atoi(file.SGetValueDef("0", s + "NumAllies").c_str());

		for (int i = 0; i < MAX_TEAMS; ++i) {
			data.allies[i] = (a == i);
		}
		for (int b = 0; b < numAllies; ++b) {
			char key[100];
			sprintf(key, "GAME\\ALLYTEAM%i\\Ally%i", a, b);
			int other = atoi(file.SGetValueDef("0",key).c_str());
			data.allies[other] = true;
		}
		data.allies[i] = true; // team i is ally from team i
		allyStartingData.push_back(data);

		allyteamRemap[a] = i;
		++i;
	}

	
	int allyCount = -1;
	file.GetDef(allyCount, "-1", "GAME\\NumAllyTeams");
	
	if (allyCount == -1 || allyStartingData.size() == allyCount)
		numAllyTeams = allyStartingData.size();
	else
		throw content_error("incorrect number of teams in GameSetup script");
}

/** @brief Update all player indices to refer to the right player. */
void CGameSetup::RemapPlayers()
{
	// relocate Team.TeamLeader field
	for (int a = 0; a < numTeams; ++a) {
		if (playerRemap.find(teamStartingData[a].leader) == playerRemap.end()) {
			throw content_error("invalid Team.TeamLeader in GameSetup script");
		};
		teamStartingData[a].leader = playerRemap[teamStartingData[a].leader];
	}

	// relocate myPlayerNum
	if (playerRemap.find(myPlayerNum) == playerRemap.end()) {
		throw content_error("invalid MyPlayerNum in GameSetup script");
	}
	myPlayerNum = playerRemap[myPlayerNum];
}

/** @brief Update all team indices to refer to the right team. */
void CGameSetup::RemapTeams()
{
	// relocate Player.Team field
	for (int a = 0; a < numPlayers; ++a) {
		if (teamRemap.find(playerStartingData[a].team) == teamRemap.end())
			throw content_error("invalid Player.Team in GameSetup script");
		playerStartingData[a].team = teamRemap[playerStartingData[a].team];
	}
}

/** @brief Update all allyteam indices to refer to the right allyteams. */
void CGameSetup::RemapAllyteams()
{
	// relocate Team.Allyteam field
	for (int a = 0; a < numTeams; ++a) {
		if (allyteamRemap.find(teamStartingData[a].teamAllyteam) == allyteamRemap.end()) {
			throw content_error("invalid Team.Allyteam in GameSetup script");
		}
		teamStartingData[a].teamAllyteam = allyteamRemap[teamStartingData[a].teamAllyteam];
	}

	// relocate gs->allies matrix
	for (int a = 0; a < MAX_TEAMS; ++a) {
		for (int b = 0; b < MAX_TEAMS; ++b) {
			if (allyteamRemap.find(a) != allyteamRemap.end() &&
				allyteamRemap.find(b) != allyteamRemap.end()) {
				allyStartingData[allyteamRemap[a]].allies[allyteamRemap[b]] = allyStartingData[a].allies[b];
			}
		}
	}
}

bool CGameSetup::Init(const char* buf, int size)
{
	// Copy buffer contents
	gameSetupText = SAFE_NEW char[size+1];
	memcpy(gameSetupText, buf, size);
	gameSetupText[size] = 0;
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
	file.GetDef(autohostport, "0", "GAME\\AutohostPort");

	// Game parameters
	scriptName  = file.SGetValueDef("Commanders", "GAME\\ScriptName");
	baseMod     = file.SGetValueDef("",  "GAME\\Gametype");
	mapName     = file.SGetValueDef("",  "GAME\\MapName");
	luaGaiaStr  = file.SGetValueDef("1", "GAME\\LuaGaia");
	if (luaGaiaStr == "0")
		useLuaGaia = false;
	else
		useLuaGaia = true;
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
	file.GetDef(fixedAllies, "1", "GAME\\FixedAllies");

	// Read the map & mod options
	if (file.SectionExist("GAME\\MapOptions")) {
		mapOptions = file.GetAllValues("GAME\\MapOptions");
	}
	if (file.SectionExist("GAME\\ModOptions")) {
		modOptions = file.GetAllValues("GAME\\ModOptions");
	}

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

	// Postprocessing
	baseMod = archiveScanner->ModArchiveToModName(baseMod);

	return true;
}

