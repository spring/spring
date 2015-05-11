/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameSetup.h"
#include "Map/MapParser.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/TdfParser.h"
#include "System/UnsyncedRNG.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/RapidHandler.h"
#include "System/Log/ILog.h"

#include <algorithm>
#include <map>
#include <cctype>
#include <cstring>
#include <fstream>
#include <boost/format.hpp>

CR_BIND(CGameSetup,)
CR_REG_METADATA(CGameSetup, (
	CR_IGNORED(fixedAllies),
	CR_IGNORED(useLuaGaia),
	CR_IGNORED(noHelperAIs),

	CR_IGNORED(ghostedBuildings),
	CR_IGNORED(disableMapDamage),

	CR_IGNORED(onlyLocal),
	CR_IGNORED(hostDemo),

	CR_IGNORED(mapHash),
	CR_IGNORED(modHash),
	CR_IGNORED(mapSeed),

	CR_IGNORED(gameStartDelay),

	CR_IGNORED(numDemoPlayers),
	CR_IGNORED(maxUnitsPerTeam),

	CR_IGNORED(minSpeed),
	CR_IGNORED(maxSpeed),

	CR_IGNORED(startPosType),

	CR_IGNORED(mapName),
	CR_IGNORED(modName),
	CR_IGNORED(gameID),

	// all other members can be reconstructed from this
	CR_MEMBER(setupText),

	CR_IGNORED(recordDemo),
	CR_IGNORED(demoName),
	CR_IGNORED(saveName),

	CR_IGNORED(playerRemap),
	CR_IGNORED(teamRemap),
	CR_IGNORED(allyteamRemap),

	CR_IGNORED(playerStartingData),
	CR_IGNORED(teamStartingData),
	CR_IGNORED(allyStartingData),
	CR_IGNORED(skirmishAIStartingData),
	CR_IGNORED(mutatorsList),

	CR_IGNORED(restrictedUnits),

	CR_IGNORED(mapOptions),
	CR_IGNORED(modOptions),

	CR_POSTLOAD(PostLoad)
))

CGameSetup* gameSetup = NULL;


bool CGameSetup::LoadReceivedScript(const std::string& script, bool isHost)
{
	CGameSetup* tempGameSetup = new CGameSetup();

	if (!tempGameSetup->Init(script)) {
		delete tempGameSetup;
		return false;
	}

	if (isHost) {
		std::fstream setupTextFile("_script.txt", std::ios::out);

		// copy the script to a local file for inspection
		setupTextFile.write(script.c_str(), script.size());
		setupTextFile.close();
	}

	// set the global instance
	gameSetup = tempGameSetup;
	return true;
}

bool CGameSetup::LoadSavedScript(const std::string& file, const std::string& script)
{
	if (script.empty())
		return false;
	if (gameSetup != NULL)
		return false;

	CGameSetup* tempGameSetup = new CGameSetup();

	if (!tempGameSetup->Init(script)) {
		delete tempGameSetup;
		return false;
	}

	tempGameSetup->saveName = file;
	// set the global instance
	gameSetup = tempGameSetup;
	return true;
}


const std::map<std::string, std::string>& CGameSetup::GetMapOptions()
{
	static std::map<std::string, std::string> dummyOptions;

	if (gameSetup != NULL) {
		return gameSetup->GetMapOptionsCont();
	}

	return dummyOptions;
}

const std::map<std::string, std::string>& CGameSetup::GetModOptions()
{
	static std::map<std::string, std::string> dummyOptions;

	if (gameSetup != NULL) {
		return gameSetup->GetModOptionsCont();
	}

	return dummyOptions;
}


const std::vector<PlayerBase>& CGameSetup::GetPlayerStartingData()
{
	static std::vector<PlayerBase> dummyData;

	if (gameSetup != NULL) {
		return gameSetup->GetPlayerStartingDataCont();
	}

	return dummyData;
}

const std::vector<TeamBase>& CGameSetup::GetTeamStartingData()
{
	static std::vector<TeamBase> dummyData;

	if (gameSetup != NULL) {
		return gameSetup->GetTeamStartingDataCont();
	}

	return dummyData;
}

const std::vector<AllyTeam>& CGameSetup::GetAllyStartingData()
{
	static std::vector<AllyTeam> dummyData;

	if (gameSetup != NULL) {
		return gameSetup->GetAllyStartingDataCont();
	}

	return dummyData;
}



void CGameSetup::ResetState()
{
	fixedAllies = true;
	useLuaGaia = true;
	noHelperAIs = false;

	ghostedBuildings = true;
	disableMapDamage = false;

	onlyLocal = false;
	hostDemo = false;
	recordDemo = true;

	mapHash = 0;
	modHash = 0;
	mapSeed = 0;

	gameStartDelay = 0;
	numDemoPlayers = 0;
	maxUnitsPerTeam = 1500;

	maxSpeed = 0.0f;
	minSpeed = 0.0f;

	startPosType = StartPos_Fixed;


	mapName.clear();
	modName.clear();
	gameID.clear();

	setupText.clear();
	demoName.clear();
	saveName.clear();


	playerRemap.clear();
	teamRemap.clear();
	allyteamRemap.clear();

	playerStartingData.clear();
	teamStartingData.clear();
	allyStartingData.clear();
	skirmishAIStartingData.clear();
	mutatorsList.clear();

	restrictedUnits.clear();

	mapOptions.clear();
	modOptions.clear();
}

void CGameSetup::PostLoad()
{
	Init(setupText);
}

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

void CGameSetup::LoadStartPositionsFromMap()
{
	MapParser mapParser(MapFile());
	if (!mapParser.IsValid()) {
		throw content_error("MapInfo: " + mapParser.GetErrorLog());
	}

	for (size_t a = 0; a < teamStartingData.size(); ++a) {
		float3 pos;

		// don't fail when playing with more players than
		// start positions and we didn't use them anyway
		if (!mapParser.GetStartPos(teamStartingData[a].teamStartNum, pos))
			throw content_error(mapParser.GetErrorLog());

		// map should ensure positions are valid
		// (but clients will always do clamping)
		teamStartingData[a].SetStartPos(pos);
	}
}

void CGameSetup::LoadStartPositions(bool withoutMap)
{
	if (withoutMap && (startPosType == StartPos_Random || startPosType == StartPos_Fixed))
		throw content_error("You need the map to use the map's start-positions");

	if (startPosType == StartPos_Random) {
		// Server syncs these later, so we can use unsynced rng
		UnsyncedRNG rng;
		rng.Seed(setupText.length());
		rng.Seed((size_t) setupText.c_str());

		std::vector<int> teamStartNum(teamStartingData.size());

		for (size_t i = 0; i < teamStartingData.size(); ++i)
			teamStartNum[i] = i;

		std::random_shuffle(teamStartNum.begin(), teamStartNum.end(), rng);

		for (size_t i = 0; i < teamStartingData.size(); ++i)
			teamStartingData[i].teamStartNum = teamStartNum[i];
	} else {
		for (size_t a = 0; a < teamStartingData.size(); ++a) {
			teamStartingData[a].teamStartNum = (int)a;
		}
	}

	if (startPosType == StartPos_Fixed || startPosType == StartPos_Random) {
		LoadStartPositionsFromMap();
	}
}

void CGameSetup::LoadMutators(const TdfParser& file, std::vector<std::string>& mutatorsList)
{
	for (int a = 0; a < 10; ++a) {
		std::string s = file.SGetValueDef("", IntToString(a, "GAME\\MUTATOR%i"));
		if (s.empty()) break;
		mutatorsList.push_back(s);
	}
}

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
	if (file.GetValue(playerCount, "GAME\\NumPlayers") && playerStartingData.size() != playerCount) {
		LOG_L(L_WARNING,
			_STPF_ " players in GameSetup script (NumPlayers says %i)",
			playerStartingData.size(), playerCount);
	}
}

void CGameSetup::LoadSkirmishAIs(const TdfParser& file, std::set<std::string>& nameList)
{
	// i = AI index in game (no gaps), a = AI index in script
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
		data.hostPlayer = atoi(file.SGetValueDef("-1", s + "Host").c_str());
		if (data.hostPlayer == -1) {
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
}

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

		// Get default color from palette (based on "color" tag)
		for (size_t num = 0; num < 3; ++num) {
			data.color[num] = TeamBase::teamDefaultColor[a][num];
		}
		data.color[3] = 255;

		const std::map<std::string, std::string>& setup = file.GetAllValues(s);

		for (std::map<std::string, std::string>::const_iterator it = setup.begin(); it != setup.end(); ++it)
			data.SetValue(it->first, it->second);

		teamStartingData.push_back(data);

		teamRemap[a] = i;
		++i;
	}

	unsigned teamCount = 0;
	if (file.GetValue(teamCount, "Game\\NumTeams") && teamStartingData.size() != teamCount) {
		LOG_L(L_WARNING,
				_STPF_ " teams in GameSetup script (NumTeams: %i)",
				teamStartingData.size(), teamCount);
	}
}

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
		for (size_t a = 0; a < numAllyTeams; ++a) {
			allyStartingData[a].allies.resize(numAllyTeams, false);
			allyStartingData[a].allies[a] = true; // each team is allied with itself

			std::ostringstream section;
			section << "GAME\\ALLYTEAM" << a << "\\";

			const size_t numAllies = atoi(file.SGetValueDef("0", section.str() + "NumAllies").c_str());

			for (size_t b = 0; b < numAllies; ++b) {
				std::ostringstream key;
				key << "GAME\\ALLYTEAM" << a << "\\Ally" << b;
				const int other = atoi(file.SGetValueDef("0",key.str()).c_str());
				allyStartingData[a].allies[allyteamRemap[other]] = true;
			}
		}
	}

	unsigned allyCount = 0;
	if (file.GetValue(allyCount, "GAME\\NumAllyTeams") && (allyStartingData.size() != allyCount)) {
		LOG_L(L_WARNING, "Incorrect number of ally teams in GameSetup script");
	}
}

void CGameSetup::RemapPlayers()
{
	// relocate Team.TeamLeader field
	for (size_t a = 0; a < teamStartingData.size(); ++a) {
		if (playerRemap.find(teamStartingData[a].GetLeader()) == playerRemap.end()) {
			std::ostringstream buf;
			buf << "GameSetup: Team " << a << " has invalid leader: " << teamStartingData[a].GetLeader();
			throw content_error(buf.str());
		}
		teamStartingData[a].SetLeader(playerRemap[teamStartingData[a].GetLeader()]);
	}
	// relocate AI.hostPlayer field
	for (size_t a = 0; a < skirmishAIStartingData.size(); ++a) {
		if (playerRemap.find(skirmishAIStartingData[a].hostPlayer) == playerRemap.end()) {
			throw content_error("invalid AI.Host in GameSetup script");
		}
		skirmishAIStartingData[a].hostPlayer = playerRemap[skirmishAIStartingData[a].hostPlayer];
	}
}

void CGameSetup::RemapTeams()
{
	// relocate Player.team field
	for (size_t a = 0; a < playerStartingData.size(); ++a) {
		if (playerStartingData[a].spectator) {
			// start spectating the first team (0)
			playerStartingData[a].team = 0;
		} else {
			if (teamRemap.find(playerStartingData[a].team) == teamRemap.end())
				throw content_error( str(boost::format("GameSetup: Player %i belong to wrong team: %i") %a %playerStartingData[a].team) );

			playerStartingData[a].team = teamRemap[playerStartingData[a].team];
		}
	}
	// relocate AI.team field
	for (size_t a = 0; a < skirmishAIStartingData.size(); ++a) {
		if (teamRemap.find(skirmishAIStartingData[a].team) == teamRemap.end())
			throw content_error("invalid AI.Team in GameSetup script");

		skirmishAIStartingData[a].team = teamRemap[skirmishAIStartingData[a].team];
		// unused (also seems redundant)
		// team_skirmishAI[skirmishAIStartingData[a].team] = &(skirmishAIStartingData[a]);
	}
}

void CGameSetup::RemapAllyteams()
{
	// relocate Team.Allyteam field
	for (size_t a = 0; a < teamStartingData.size(); ++a) {
		if (allyteamRemap.find(teamStartingData[a].teamAllyteam) == allyteamRemap.end()) {
			throw content_error("invalid Team.Allyteam in GameSetup script");
		}
		teamStartingData[a].teamAllyteam = allyteamRemap[teamStartingData[a].teamAllyteam];
	}
}

// TODO: RemapSkirmishAIs()
bool CGameSetup::Init(const std::string& buf)
{
	ResetState();

	// Copy buffer contents
	setupText = buf;

	// Parse game parameters
	TdfParser file(buf.c_str(),buf.size());

	if (!file.SectionExist("GAME"))
		return false;

	// Used by dedicated server only
	file.GetTDef(mapHash, unsigned(0), "GAME\\MapHash");
	file.GetTDef(modHash, unsigned(0), "GAME\\ModHash");
	file.GetTDef(mapSeed, unsigned(0), "GAME\\MapSeed");

	gameID      = file.SGetValueDef("",  "GAME\\GameID");
	modName     = file.SGetValueDef("",  "GAME\\Gametype");
	mapName     = file.SGetValueDef("",  "GAME\\MapName");
	saveName    = file.SGetValueDef("",  "GAME\\Savefile");
	demoName    = file.SGetValueDef("",  "GAME\\Demofile");
	hostDemo    = !demoName.empty();

	file.GetTDef(gameStartDelay, (unsigned int) 4, "GAME\\GameStartDelay");

	file.GetDef(recordDemo,          "1", "GAME\\RecordDemo");
	file.GetDef(onlyLocal,           "0", "GAME\\OnlyLocal");
	file.GetDef(useLuaGaia,          "1", "GAME\\ModOptions\\LuaGaia");
	file.GetDef(noHelperAIs,         "0", "GAME\\ModOptions\\NoHelperAIs");
	file.GetDef(maxUnitsPerTeam, "32000", "GAME\\ModOptions\\MaxUnits");
	file.GetDef(disableMapDamage,    "0", "GAME\\ModOptions\\DisableMapDamage");
	file.GetDef(ghostedBuildings,    "1", "GAME\\ModOptions\\GhostedBuildings");

	file.GetDef(maxSpeed, "3.0", "GAME\\ModOptions\\MaxSpeed");
	file.GetDef(minSpeed, "0.3", "GAME\\ModOptions\\MinSpeed");

	file.GetDef(fixedAllies, "1", "GAME\\ModOptions\\FixedAllies");

	// Read the map & mod options
	if (file.SectionExist("GAME\\MapOptions")) { mapOptions = file.GetAllValues("GAME\\MapOptions"); }
	if (file.SectionExist("GAME\\ModOptions")) { modOptions = file.GetAllValues("GAME\\ModOptions"); }

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

	LoadMutators(file, mutatorsList);
	LoadUnitRestrictions(file);

	// Postprocessing
	modName = GetRapidName(modName);
	modName = archiveScanner->NameFromArchive(modName);

	return true;
}

const std::string CGameSetup::MapFile() const
{
	return (archiveScanner->MapNameToMapFile(mapName));
}

