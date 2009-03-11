#include "StdAfx.h"

#include <algorithm>
#include <map>
#include <SDL_timer.h>
#include <cctype>
#include <cstring>
#include <boost/format.hpp>

#include "mmgr.h"

#include "GameSetup.h"
#include "TdfParser.h"
#include "FileSystem/ArchiveScanner.h"
#include "Map/MapParser.h"
#include "Rendering/Textures/TAPalette.h"
#include "Sim/Misc/GlobalConstants.h"
#include "UnsyncedRNG.h"
#include "Exceptions.h"
#include "Util.h"
#include "LogOutput.h"


using namespace std;


const CGameSetup* gameSetup = NULL;

LocalSetup::LocalSetup() :
		myPlayerNum(0),
		hostport(8452),
		sourceport(0),
		autohostport(0),
		isHost(false)
{
}

void LocalSetup::Init(const std::string& setup)
{
	TdfParser file(setup.c_str(), setup.length());

	if(!file.SectionExist("GAME"))
		throw content_error("GAME-section didn't exist in setupscript");

	// Technical parameters
	file.GetDef(hostip,     "localhost", "GAME\\HostIP");
	file.GetDef(hostport,   "8452", "GAME\\HostPort");
	file.GetDef(sourceport, "0", "GAME\\SourcePort");
	file.GetDef(autohostport, "0", "GAME\\AutohostPort");

	file.GetDef(myPlayerName,  "", "GAME\\MyPlayerName");

	if (!file.GetValue(isHost, "GAME\\IsHost"))
	{
		logOutput.Print("Warning: The script.txt is missing the IsHost-entry. Assuming this is a client.");
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
void CGameSetup::LoadStartPositions(bool withoutMap)
{
	TdfParser file(gameSetupText.c_str(), gameSetupText.length());

	if (withoutMap && (startPosType == StartPos_Random || startPosType == StartPos_Fixed))
		throw content_error("You need the map to use the map's startpositions");
	
	if (startPosType == StartPos_Random) {
		// Server syncs these later, so we can use unsynced rng
		UnsyncedRNG rng;
		size_t numTeams = teamStartingData.size();
		rng.Seed(gameSetupText.length()^ SDL_GetTicks());
		std::vector<int> teamStartNum(numTeams);
		for (size_t i = 0; i < numTeams; ++i)
			teamStartNum[i] = i;
		std::random_shuffle(teamStartNum.begin(), teamStartNum.end(), rng);
		for (size_t i = 0; i < numTeams; ++i)
			teamStartingData[i].teamStartNum = teamStartNum[i];
	}
	else
	{
		for (int a = 0; a < numTeams; ++a) {
		teamStartingData[a].teamStartNum = a;
		}
	}

	if (!withoutMap)
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
void CGameSetup::LoadPlayers(const TdfParser& file, std::set<std::string>& nameList)
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
		{
			if (nameList.find(it->second) != nameList.end())
				throw content_error(str(
						boost::format("GameSetup: Player %i has name %s which is already taken")
						%a %it->second.c_str() ));
			data.name = it->second;
			nameList.insert(data.name);
		}
		else
		{
			throw content_error(str( boost::format("GameSetup: No name given for Player %i") %a ));
		}
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
		logOutput.Print("Warning: incorrect number of players in GameSetup script");
}


static std::set<std::string> LoadLuaAINames()
{
	std::set<std::string> names;

	LuaParser luaParser("LuaAI.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
	if (!luaParser.Execute()) {
		// it is no error if the mod does not come with LUA AIs.
		return names;
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		throw content_error("root table invalid");
	}

	for (int i=1; root.KeyExists(i); i++) {
		// string format
		std::string name = root.GetString(i, "");
		if (!name.empty()) {
			names.insert(name);
			continue;
		}

		// table format  (name & desc)
		const LuaTable& optTbl = root.SubTable(i);
		if (!optTbl.IsValid()) {
			continue;
		}
		name = optTbl.GetString("name", "");
		if (name.empty()) {
			continue;
		}
		names.insert(name);
	}

	return names;
}

/**
 * @brief Load LUA and Skirmish AIs.
 */
void CGameSetup::LoadSkirmishAIs(const TdfParser& file, std::set<std::string>& nameList)
{
	// i = AI index in game (no gaps), a = AI index in script
//	int i = 0;
	for (int a = 0; a < MAX_PLAYERS; ++a) {
		char section[50];
		sprintf(section, "GAME\\AI%i\\", a);
		string s(section);

		if (!file.SectionExist(s.substr(0, s.length() - 1))) {
			continue;
		}

		SkirmishAIData data;

		data.team = atoi(file.SGetValueDef("-1", s + "Team").c_str());
		if (data.team == -1) {
			throw content_error("missing AI.Team in GameSetup script");
		}
		data.host = atoi(file.SGetValueDef("-1", s + "Host").c_str());
		if (data.host == -1) {
			throw content_error("missing AI.Host in GameSetup script");
		}

		data.shortName = file.SGetValueDef("", s + "ShortName");
		if (data.shortName == "") {
			throw content_error("missing AI.ShortName in GameSetup script");
		}

		// Is this team (Lua) AI controlled?
		//data.isLuaAI = (file.SGetValueDef("0", s + "IsLuaAI") == "1");
		// If this is a demo replay, non-Lua AIs aren't loaded.
		static std::set<std::string> luaAINames = LoadLuaAINames();
		data.isLuaAI = (luaAINames.find(data.shortName) != luaAINames.end());

		data.version = file.SGetValueDef("", s + "Version");
		if (file.SectionExist(s + "Options")) {
			data.options = file.GetAllValues(s + "Options");
		}

		// get the visible name (comparable to player-name)
		std::string name = file.SGetValueDef(data.shortName, s + "Name");
		int instanceIndex = 0;
		std::string name_unique = name;
		while (nameList.find(name_unique) != nameList.end()) {
			name_unique = name + "_" + IntToString(instanceIndex++);
			// so we possibly end up with something like myBot_0, or RAI_2
		}
		data.name = name_unique;
		nameList.insert(data.name);

		skirmishAIStartingData.push_back(data);
	}

	unsigned aiCount = 0;
	if (!file.GetValue(aiCount, "GAME\\NumSkirmishAIs")
			|| skirmishAIStartingData.size() == aiCount) {
		aiCount = skirmishAIStartingData.size();
	} else {
		throw content_error(
				"incorrect number of skirmish AIs in GameSetup script");
	}
}
const SkirmishAIData* CGameSetup::GetSkirmishAIDataForTeam(int teamId) const {

	std::map<int, const SkirmishAIData*>::const_iterator sad;
	sad = team_skirmishAI.find(teamId);
	if (sad == team_skirmishAI.end()) {
		return NULL;
	} else {
		return sad->second;
	}
}

size_t CGameSetup::GetSkirmishAIs() const {
	return team_skirmishAI.size();
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
		data.side = StringToLower(file.SGetValueDef("", s + "side").c_str());
		data.teamAllyteam = atoi(file.SGetValueDef("0", s + "allyteam").c_str());

		teamStartingData.push_back(data);

		teamRemap[a] = i;
		++i;
	}

	unsigned teamCount = 0;
	if (!file.GetValue(teamCount, "Game\\NumTeams") || teamStartingData.size() == teamCount)
		numTeams = teamStartingData.size();
	else
		logOutput.Print("Warning: incorrect number of teams in GameSetup script");
}

/**
@brief Load allyteams and remove gaps in the allyteam numbering.
@pre numAllyTeams initialized
@post allyteams loaded, alliances initialised (no remapping needed here)
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

		allyStartingData.push_back(data);

		allyteamRemap[a] = i;
		++i;
	}

	{
		const size_t numAllyTeams = allyStartingData.size();
		for (size_t a = 0; a < numAllyTeams; ++a)
		{
			allyStartingData[a].allies.resize(numAllyTeams, false);
			allyStartingData[a].allies[a] = true; // each team is allied with itself
	
			std::ostringstream section;
			section << "GAME\\ALLYTEAM" << a << "\\";
			size_t numAllies = atoi(file.SGetValueDef("0", section.str() + "NumAllies").c_str());
			for (size_t b = 0; b < numAllies; ++b) {
				std::ostringstream key;
				key << "GAME\\ALLYTEAM" << a << "\\Ally" << b;
				int other = atoi(file.SGetValueDef("0",key.str()).c_str());
				allyStartingData[a].allies[allyteamRemap[other]] = true;
			}
		}
	}

	unsigned allyCount = 0;
	if (!file.GetValue(allyCount, "GAME\\NumAllyTeams") || allyStartingData.size() == allyCount)
		numAllyTeams = allyStartingData.size();
	else
		logOutput.Print("Warning: incorrect number of allyteams in GameSetup script");
}

/** @brief Update all player indices to refer to the right player. */
void CGameSetup::RemapPlayers()
{
	// relocate Team.TeamLeader field
	for (int a = 0; a < numTeams; ++a) {
		if (playerRemap.find(teamStartingData[a].leader) == playerRemap.end()) {
			throw content_error("invalid Team.leader in GameSetup script");
		}
		teamStartingData[a].leader = playerRemap[teamStartingData[a].leader];
	}
	// relocate AI.host field
	for (unsigned int a = 0; a < skirmishAIStartingData.size(); ++a) {
		if (playerRemap.find(skirmishAIStartingData[a].host) == playerRemap.end())
			throw content_error("invalid AI.host in GameSetup script");
		skirmishAIStartingData[a].host = playerRemap[skirmishAIStartingData[a].host];
	}
}

/** @brief Update all team indices to refer to the right team. */
void CGameSetup::RemapTeams()
{
	// relocate Player.team field
	for (int a = 0; a < numPlayers; ++a) {
		if (playerStartingData[a].spectator)
			playerStartingData[a].team = 0; // start speccing on team 0
		else
		{
			if (teamRemap.find(playerStartingData[a].team) == teamRemap.end())
				throw content_error( str(boost::format("GameSetup: Player %i belong to wrong team: %i") %a %playerStartingData[a].team) );
			playerStartingData[a].team = teamRemap[playerStartingData[a].team];
		}
	}
	// relocate AI.team field
	for (unsigned int a = 0; a < skirmishAIStartingData.size(); ++a) {
		if (teamRemap.find(skirmishAIStartingData[a].team) == teamRemap.end())
			throw content_error("invalid AI.team in GameSetup script");
		skirmishAIStartingData[a].team = teamRemap[skirmishAIStartingData[a].team];
		team_skirmishAI[skirmishAIStartingData[a].team] = &(skirmishAIStartingData[a]);
	}
}

/** @brief Update all allyteam indices to refer to the right allyteams. (except allies) */
void CGameSetup::RemapAllyteams()
{
	// relocate Team.Allyteam field
	for (int a = 0; a < numTeams; ++a) {
		if (allyteamRemap.find(teamStartingData[a].teamAllyteam) == allyteamRemap.end()) {
			throw content_error("invalid Team.Allyteam in GameSetup script");
		}
		teamStartingData[a].teamAllyteam = allyteamRemap[teamStartingData[a].teamAllyteam];
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
	scriptName  = file.SGetValueDef("Commanders", "GAME\\ModOptions\\ScriptName");

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
	file.GetDef(maxUnits,       "1500", "GAME\\ModOptions\\MaxUnits");
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
	std::set<std::string> playersNameList;
	LoadPlayers(file, playersNameList);
	LoadSkirmishAIs(file, playersNameList);
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

