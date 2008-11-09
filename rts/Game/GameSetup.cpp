#include "StdAfx.h"

#include <algorithm>
#include <map>
#include <SDL_timer.h>
#include <cctype>
#include <cstring>

#include "mmgr.h"

#include "GameSetup.h"
#include "TdfParser.h"
#include "FileSystem/ArchiveScanner.h"
#include "Map/MapParser.h"
#include "Rendering/Textures/TAPalette.h"
#include "UnsyncedRNG.h"
#include "Exceptions.h"
#include "Util.h"


using namespace std;


const CGameSetup* gameSetup = NULL;

LocalSetup::LocalSetup() :
		myPlayerNum(0),
		hostport(8452),
		sourceport(0),
		autohostport(0),
		isHost(true)
{
}

void LocalSetup::Init(const std::string& setup)
{
	TdfParser file(setup.c_str(), setup.length());
	
	if(!file.SectionExist("GAME"))
		throw content_error("GAME-section didn't exist in setupscript");

	// Technical parameters
	file.GetDef(hostip,     "0", "GAME\\HostIP");
	file.GetDef(hostport,   "0", "GAME\\HostPort");
	file.GetDef(sourceport, "0", "GAME\\SourcePort");
	file.GetDef(autohostport, "0", "GAME\\AutohostPort");
	
	file.GetDef(myPlayerName,  "", "GAME\\MyPlayerName");
	file.GetDef(myPlayerNum,  "0", "GAME\\MyPlayerNum");
	if (myPlayerName.empty())
	{
		char section[50];
		sprintf(section, "GAME\\PLAYER%i", myPlayerNum);
		string s(section);

		if (!file.SectionExist(s))
			throw content_error("myPlayer not found");

		std::map<std::string, std::string> setup = file.GetAllValues(s);
		std::map<std::string, std::string>::iterator it = setup.find("name");
		if (it != setup.end())
			myPlayerName = it->second;
		else
			throw content_error("Player doesn't have a name");
	}

	int tmp_isHost = 0;
	if (file.GetValue(tmp_isHost, "GAME\\IsHost"))
		isHost = static_cast<bool>(tmp_isHost);
	else
	{
		for (int a = 0; a < MAX_PLAYERS; ++a) {
			// search for the first player not from the demo, if it is ourself, we are the host
			char section[50];
			sprintf(section, "GAME\\PLAYER%i", a);
			string s(section);

			if (!file.SectionExist(s)) {
				continue;
			}
			bool fromdemo = false;
			std::string name;
			std::map<std::string, std::string> setup = file.GetAllValues(s);
			std::map<std::string, std::string>::iterator it;
			if ((it = setup.find("name")) != setup.end())
				name = it->second;
			if ((it = setup.find("isfromdemo")) != setup.end())
				fromdemo = static_cast<bool>(atoi(it->second.c_str()));
			
			if (!fromdemo)
			{
				isHost = (myPlayerName == name);
				break;
			}
		}
	}
}

CGameSetup::CGameSetup()
{
	startPosType=StartPos_Fixed;
	numDemoPlayers=0;
	hostDemo=false;
}

CGameSetup::~CGameSetup()
{
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
		if (!mapParser.GetStartPos(teamStartingData[a].teamStartNum, pos) && (startPosType == StartPos_Fixed || startPosType == StartPos_Random)) // don't fail when playing with more players than startpositions and we didn't use them anyway
			throw content_error(mapParser.GetErrorLog());
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
	TdfParser file(gameSetupText.c_str(), gameSetupText.length());

	if (startPosType == StartPos_Random) {
		// Server syncs these later, so we can use unsynced rng
		UnsyncedRNG rng;
		rng.Seed(gameSetupText.length()^ SDL_GetTicks());
		int teamStartNum[MAX_TEAMS];
		for (int i = 0; i < MAX_TEAMS; ++i)
			teamStartNum[i] = i;
		std::random_shuffle(&teamStartNum[0], &teamStartNum[numTeams], rng);
		for (unsigned i = 0; i < teamStartingData.size(); ++i)
			teamStartingData[i].teamStartNum = teamStartNum[i];
	}
	else
	{
		for (int a = 0; a < numTeams; ++a) {
		teamStartingData[a].teamStartNum = a;
		}
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
		if ((it = setup.find("isfromdemo")) != setup.end())
			data.isFromDemo = static_cast<bool>(atoi(it->second.c_str()));

		if (data.isFromDemo)
			numDemoPlayers++;

		playerStartingData.push_back(data);
		playerRemap[a] = i;
		++i;
	}

	unsigned playerCount = 0;
	if (!file.GetValue(playerCount, "GAME\\NumPlayers") || playerStartingData.size() == playerCount)
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
		int colorNum = atoi(file.SGetValueDef("-1", s + "color").c_str());
		if (colorNum == -1) colorNum = a;
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

	unsigned teamCount = 0;
	if (!file.GetValue(teamCount, "Game\\NumTeams") || teamStartingData.size() == teamCount)
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

		for (int otherAllyTeam = 0; otherAllyTeam < MAX_TEAMS; ++otherAllyTeam) {
			data.allies[otherAllyTeam] = (i == otherAllyTeam);
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


	unsigned allyCount = 0;
	if (!file.GetValue(allyCount, "GAME\\NumAllyTeams") || allyStartingData.size() == allyCount)
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

bool CGameSetup::Init(const std::string& buf)
{
	// Copy buffer contents
	gameSetupText = buf;

	// Parse
	TdfParser file(buf.c_str(),buf.size());

	if(!file.SectionExist("GAME"))
		return false;

	// Game parameters
	scriptName  = file.SGetValueDef("Commanders", "GAME\\ScriptName");

	// Used by dedicated server only
	int tempMapHash, tempModHash;
	file.GetDef(tempMapHash, "0", "GAME\\MapHash");
	file.GetDef(tempModHash, "0", "GAME\\ModHash");
	mapHash = (unsigned int) tempMapHash;
	modHash = (unsigned int) tempModHash;

	baseMod     = file.SGetValueDef("",  "GAME\\Gametype");
	mapName     = file.SGetValueDef("",  "GAME\\MapName");
	luaGaiaStr  = file.SGetValueDef("1", "GAME\\ModOptions\\LuaGaia");
	if (luaGaiaStr == "0")
		useLuaGaia = false;
	else
		useLuaGaia = true;
	luaRulesStr = file.SGetValueDef("1", "GAME\\ModOptions\\LuaRules");
	saveName    = file.SGetValueDef("",  "GAME\\Savefile");
	demoName    = file.SGetValueDef("",  "GAME\\Demofile");
	hostDemo    = !demoName.empty();

	file.GetDef(gameMode,         "0", "GAME\\ModOptions\\GameMode");
	file.GetDef(noHelperAIs,      "0", "GAME\\ModOptions\\NoHelperAIs");
	file.GetDef(maxUnits,       "500", "GAME\\ModOptions\\MaxUnits");
	file.GetDef(limitDgun,        "0", "GAME\\ModOptions\\LimitDgun");
	file.GetDef(diminishingMMs,   "0", "GAME\\ModOptions\\DiminishingMMs");
	file.GetDef(disableMapDamage, "0", "GAME\\ModOptions\\DisableMapDamage");
	file.GetDef(ghostedBuildings, "1", "GAME\\ModOptions\\GhostedBuildings");
	file.GetDef(startMetal,    "1000", "GAME\\ModOptions\\StartMetal");
	file.GetDef(startEnergy,   "1000", "GAME\\ModOptions\\StartEnergy");

	file.GetDef(maxSpeed, "3.0", "GAME\\ModOptions\\MaxSpeed");
	file.GetDef(minSpeed, "0.3", "GAME\\ModOptions\\MinSpeed");

	file.GetDef(fixedAllies, "1", "GAME\\ModOptions\\FixedAllies");

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

