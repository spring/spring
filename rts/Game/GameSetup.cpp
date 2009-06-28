#include "StdAfx.h"

#include <algorithm>
#include <map>
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

CGameSetup::CGameSetup():
	startPosType(StartPos_Fixed),
	hostDemo(false),
	numDemoPlayers(0)
{}

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

	for(size_t a = 0; a < teamStartingData.size(); ++a) {
		float3 pos(1000.0f, 100.0f, 1000.0f);
		if (!mapParser.GetStartPos(teamStartingData[a].teamStartNum, pos) && (startPosType == StartPos_Fixed || startPosType == StartPos_Random)) // don't fail when playing with more players than startpositions and we didn't use them anyway
			throw content_error(mapParser.GetErrorLog());
		teamStartingData[a].startPos = float3(pos.x, pos.y, pos.z);
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
		rng.Seed(gameSetupText.length());
		rng.Seed((size_t)gameSetupText.c_str());
		std::vector<int> teamStartNum(teamStartingData.size());
		for (size_t i = 0; i < teamStartingData.size(); ++i)
			teamStartNum[i] = i;
		std::random_shuffle(teamStartNum.begin(), teamStartNum.end(), rng);
		for (size_t i = 0; i < teamStartingData.size(); ++i)
			teamStartingData[i].teamStartNum = teamStartNum[i];
	}
	else
	{
		for (size_t a = 0; a < teamStartingData.size(); ++a) {
		teamStartingData[a].teamStartNum = (int)a;
		}
	}

	if (!withoutMap)
		LoadStartPositionsFromMap();

	// Show that we havent selected start pos yet
	if (startPosType == StartPos_ChooseInGame) {
		for (size_t a = 0; a < teamStartingData.size(); ++a) {
			teamStartingData[a].startPos.y = -500;
		}
	}

	// Load start position from gameSetup script
	if (startPosType == StartPos_ChooseBeforeGame) {
		for (size_t a = 0; a < teamStartingData.size(); ++a) {
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
		for (std::map<std::string, std::string>::const_iterator it = setup.begin(); it != setup.end(); ++it)
			data.SetValue(it->first, it->second);

		// do checks for sanity
		if (data.name.empty())
			throw content_error(str( boost::format("GameSetup: No name given for Player %i") %a ));
		if (nameList.find(data.name) != nameList.end())
			throw content_error(str(boost::format("GameSetup: Player %i has name %s which is already taken")	%a %data.name.c_str() ));
		nameList.insert(data.name);

		if (data.isFromDemo)
			numDemoPlayers++;

		playerStartingData.push_back(data);
		playerRemap[a] = i;
		++i;
	}

	unsigned playerCount = 0;
	if (file.GetValue(playerCount, "GAME\\NumPlayers") && playerStartingData.size() != playerCount)
		logOutput.Print("Warning: %i players in GameSetup script (NumPlayers says %i)", playerStartingData.size(), playerCount);
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
		data.hostPlayerNum = atoi(file.SGetValueDef("-1", s + "Host").c_str());
		if (data.hostPlayerNum == -1) {
			throw content_error("missing AI.Host in GameSetup script");
		}

		data.shortName = file.SGetValueDef("", s + "ShortName");
		if (data.shortName == "") {
			throw content_error("missing AI.ShortName in GameSetup script");
		}

		data.version = file.SGetValueDef("", s + "Version");
		if (file.SectionExist(s + "Options")) {
			data.options = file.GetAllValues(s + "Options");
			std::map<std::string, std::string>::const_iterator kv;
			for (kv = data.options.begin(); kv != data.options.end(); ++kv) {
				data.optionKeys.push_back(kv->first);
			}
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
		sprintf(section, "GAME\\TEAM%i", a);
		string s(section);

		if (!file.SectionExist(s.substr(0, s.length()))) {
			continue;
		}

		TeamBase data;
		data.startMetal = startMetal;
		data.startEnergy = startEnergy;

		// Get default color from palette (based on "color" tag)
		for (size_t num = 0; num < 3; ++num)
		{
			data.color[num] = palette.teamColor[a][num];
		}
		data.color[3] = 255;

		std::map<std::string, std::string> setup = file.GetAllValues(s);
		for (std::map<std::string, std::string>::const_iterator it = setup.begin(); it != setup.end(); ++it)
			data.SetValue(it->first, it->second);

		teamStartingData.push_back(data);

		teamRemap[a] = i;
		++i;
	}

	unsigned teamCount = 0;
	if (file.GetValue(teamCount, "Game\\NumTeams") && teamStartingData.size() != teamCount)
		logOutput.Print("Warning: %i teams in GameSetup script (NumTeams: %i)", teamStartingData.size(), teamCount);
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
		sprintf(section,"GAME\\ALLYTEAM%i",a);
		string s(section);

		if (!file.SectionExist(s))
			continue;
		AllyTeam data;
		std::map<std::string, std::string> setup = file.GetAllValues(s);
		for (std::map<std::string, std::string>::const_iterator it = setup.begin(); it != setup.end(); ++it)
			data.SetValue(it->first, it->second);

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
		;
	else
		logOutput.Print("Warning: incorrect number of allyteams in GameSetup script");
}

/** @brief Update all player indices to refer to the right player. */
void CGameSetup::RemapPlayers()
{
	// relocate Team.TeamLeader field
	for (size_t a = 0; a < teamStartingData.size(); ++a) {
		if (playerRemap.find(teamStartingData[a].leader) == playerRemap.end()) {
			throw content_error("invalid Team.leader in GameSetup script");
		}
		teamStartingData[a].leader = playerRemap[teamStartingData[a].leader];
	}
	// relocate AI.hostPlayerNum field
	for (size_t a = 0; a < skirmishAIStartingData.size(); ++a) {
		if (playerRemap.find(skirmishAIStartingData[a].hostPlayerNum) == playerRemap.end()) {
			throw content_error("invalid AI.hostPlayerNum in GameSetup script");
		}
		skirmishAIStartingData[a].hostPlayerNum = playerRemap[skirmishAIStartingData[a].hostPlayerNum];
	}
}

/** @brief Update all team indices to refer to the right team. */
void CGameSetup::RemapTeams()
{
	// relocate Player.team field
	for (size_t a = 0; a < playerStartingData.size(); ++a) {
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
	for (size_t a = 0; a < skirmishAIStartingData.size(); ++a) {
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
	for (size_t a = 0; a < allyStartingData.size(); ++a) {
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
	file.GetTDef(mapHash, unsigned(0), "GAME\\MapHash");
	file.GetTDef(modHash, unsigned(0), "GAME\\ModHash");

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

