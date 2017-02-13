/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameSetup.h"
#include "Map/MapParser.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/GlobalRNG.h"
#include "System/TdfParser.h"
#include "System/Exceptions.h"
#include "System/SpringFormat.h"
#include "System/Util.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/RapidHandler.h"
#include "System/Sync/HsiehHash.h"
#include "System/Log/ILog.h"

#include <algorithm>
#include <numeric>
#include <map>
#include <cctype>
#include <cstring>
#include <fstream>

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
	CR_IGNORED(menuName),

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

CGameSetup* gameSetup = nullptr;


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
	if (gameSetup != nullptr)
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


const spring::unordered_map<std::string, std::string>& CGameSetup::GetMapOptions()
{
	static spring::unordered_map<std::string, std::string> dummyOptions;

	if (gameSetup != nullptr)
		return gameSetup->GetMapOptionsCont();

	return dummyOptions;
}

const spring::unordered_map<std::string, std::string>& CGameSetup::GetModOptions()
{
	static spring::unordered_map<std::string, std::string> dummyOptions;

	if (gameSetup != nullptr)
		return gameSetup->GetModOptionsCont();

	return dummyOptions;
}


const std::vector<PlayerBase>& CGameSetup::GetPlayerStartingData()
{
	static std::vector<PlayerBase> dummyData;

	if (gameSetup != nullptr)
		return gameSetup->GetPlayerStartingDataCont();

	return dummyData;
}

const std::vector<TeamBase>& CGameSetup::GetTeamStartingData()
{
	static std::vector<TeamBase> dummyData;

	if (gameSetup != nullptr)
		return gameSetup->GetTeamStartingDataCont();

	return dummyData;
}

const std::vector<AllyTeam>& CGameSetup::GetAllyStartingData()
{
	static std::vector<AllyTeam> dummyData;

	if (gameSetup != nullptr)
		return gameSetup->GetAllyStartingDataCont();

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
		const std::string resUnitName = "GAME\\RESTRICT\\Unit" + IntToString(i, "%d");
		const std::string resUnitLimit = "GAME\\RESTRICT\\Limit" + IntToString(i, "%d");

		int unitLimit;
		file.GetDef(unitLimit, "0", resUnitLimit);

		restrictedUnits[ file.SGetValueDef("", resUnitName) ] = unitLimit;
	}
}

void CGameSetup::LoadStartPositionsFromMap()
{
	MapParser mapParser(MapFile());

	if (!mapParser.IsValid())
		throw content_error("MapInfo: " + mapParser.GetErrorLog());

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

	std::vector<int> teamStartNum(teamStartingData.size());
	std::iota(teamStartNum.begin(), teamStartNum.end(), 0);

	if (startPosType == StartPos_Random) {
		// Server syncs these later, so we can use unsynced rng
		CGlobalUnsyncedRNG rng;
		rng.Seed(HsiehHash(setupText.c_str(), setupText.length(), 1234567));
		std::random_shuffle(teamStartNum.begin(), teamStartNum.end(), rng);
	}

	for (size_t i = 0; i < teamStartingData.size(); ++i)
		teamStartingData[i].teamStartNum = teamStartNum[i];

	if (startPosType == StartPos_Fixed || startPosType == StartPos_Random) {
		LoadStartPositionsFromMap();
	}
}

void CGameSetup::LoadMutators(const TdfParser& file, std::vector<std::string>& mutatorsList)
{
	for (int a = 0; a < 10; ++a) {
		const std::string s = file.SGetValueDef("", IntToString(a, "GAME\\MUTATOR%i"));
		if (s.empty())
			break;
		mutatorsList.push_back(s);
	}
}

void CGameSetup::LoadPlayers(const TdfParser& file, spring::unordered_set<std::string>& nameList)
{
	assert(numDemoPlayers == 0);

	// i = player index in game (no gaps), a = player index in script
	int i = 0;
	for (int a = 0; a < MAX_PLAYERS; ++a) {
		const std::string section = "GAME\\PLAYER" + IntToString(a, "%i");

		if (!file.SectionExist(section))
			continue;

		PlayerBase playerBase;

		// expects lines of form team=x rather than team=TEAMx
		// team field is relocated in RemapTeams
		for (auto it: file.GetAllValues(section))
			playerBase.SetValue(it.first, it.second);

		// do checks for sanity
		if (playerBase.name.empty())
			throw content_error(spring::format("GameSetup: No name given for Player %i", a));
		if (nameList.find(playerBase.name) != nameList.end())
			throw content_error(spring::format("GameSetup: Player %i has name %s which is already taken", a, playerBase.name.c_str()));

		numDemoPlayers += playerBase.isFromDemo;

		nameList.insert(playerBase.name);

		playerStartingData.push_back(playerBase);
		playerRemap[a] = i++;
	}

	unsigned int playerCount = 0;

	if (file.GetValue(playerCount, "GAME\\NumPlayers") == 0)
		return;
	if (playerStartingData.size() == playerCount)
		return;

	LOG_L(L_WARNING, _STPF_ " players in GameSetup script (NumPlayers says %i)", playerStartingData.size(), playerCount);
}

void CGameSetup::LoadSkirmishAIs(const TdfParser& file, spring::unordered_set<std::string>& nameList)
{
	// i = AI index in game (no gaps), a = AI index in script
	for (int a = 0; a < MAX_PLAYERS; ++a) {
		const std::string section = "GAME\\AI" + IntToString(a, "%i") + "\\";

		if (!file.SectionExist(section.substr(0, section.length() - 1)))
			continue;

		SkirmishAIData data;

		data.team = atoi(file.SGetValueDef("-1", section + "Team").c_str());
		data.hostPlayer = atoi(file.SGetValueDef("-1", section + "Host").c_str());
		data.shortName = file.SGetValueDef("", section + "ShortName");
		data.version = file.SGetValueDef("", section + "Version");

		if (data.team == -1)
			throw content_error("missing AI.Team in GameSetup script");

		if (data.hostPlayer == -1)
			throw content_error("missing AI.Host in GameSetup script");

		if (data.shortName == "")
			throw content_error("missing AI.ShortName in GameSetup script");

		if (file.SectionExist(section + "Options")) {
			data.options = file.GetAllValues(section + "Options");

			for (const auto& kv: data.options) {
				data.optionKeys.push_back(kv.first);
			}
		}

		// get the visible name (comparable to player-name)
		std::string visibleName = file.SGetValueDef(data.shortName, section + "Name");
		std::string uniqueName = visibleName;

		int instanceIndex = 0;

		while (nameList.find(uniqueName) != nameList.end()) {
			uniqueName = visibleName + "_" + IntToString(instanceIndex++);
			// so we possibly end up with something like myBot_0, or RAI_2
		}

		data.name = uniqueName;
		nameList.insert(data.name);

		skirmishAIStartingData.push_back(data);
	}
}

void CGameSetup::LoadTeams(const TdfParser& file)
{
	// i = team index in game (no gaps), a = team index in script
	int i = 0;
	for (int a = 0; a < MAX_TEAMS; ++a) {
		const std::string section = "GAME\\TEAM" + IntToString(a, "%i");

		if (!file.SectionExist(section))
			continue;

		TeamBase teamBase;

		// Get default color from palette (based on "color" tag)
		for (size_t num = 0; num < 3; ++num)
			teamBase.color[num] = TeamBase::teamDefaultColor[a][num];

		teamBase.color[3] = 255;

		for (auto it: file.GetAllValues(section))
			teamBase.SetValue(it.first, it.second);

		teamStartingData.push_back(teamBase);

		teamRemap[a] = i++;
	}

	unsigned int teamCount = 0;

	if (file.GetValue(teamCount, "Game\\NumTeams") == 0)
		return;
	if (teamStartingData.size() == teamCount)
		return;

	LOG_L(L_WARNING, _STPF_ " teams in GameSetup script (NumTeams: %i)", teamStartingData.size(), teamCount);
}

void CGameSetup::LoadAllyTeams(const TdfParser& file)
{
	// i = allyteam index in game (no gaps), a = allyteam index in script
	int i = 0;
	for (int a = 0; a < MAX_TEAMS; ++a) {
		const std::string section = "GAME\\ALLYTEAM" + IntToString(a, "%i");

		if (!file.SectionExist(section))
			continue;

		AllyTeam allyTeam;

		for (auto it: file.GetAllValues(section))
			allyTeam.SetValue(it.first, it.second);

		allyStartingData.push_back(allyTeam);

		allyteamRemap[a] = i++;
	}

	{
		const size_t numAllyTeams = allyStartingData.size();
		for (size_t a = 0; a < numAllyTeams; ++a) {
			allyStartingData[a].allies.resize(numAllyTeams, false);
			allyStartingData[a].allies[a] = true; // each team is allied with itself

			const std::string section = "GAME\\ALLYTEAM" + IntToString(a) + "\\";

			const size_t numAllies = atoi(file.SGetValueDef("0", section + "NumAllies").c_str());

			for (size_t b = 0; b < numAllies; ++b) {
				const std::string key = "GAME\\ALLYTEAM" + IntToString(a) + "\\Ally" + IntToString(b);
				const std::string val = file.SGetValueDef("0", key);

				const int other = atoi(val.c_str());

				allyStartingData[a].allies[ allyteamRemap[other] ] = true;
			}
		}
	}

	unsigned int allyCount = 0;

	if (file.GetValue(allyCount, "GAME\\NumAllyTeams") == 0)
		return;
	if (allyStartingData.size() == allyCount)
		return;

	LOG_L(L_WARNING, "Incorrect number of ally teams in GameSetup script");
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
		if (playerRemap.find(skirmishAIStartingData[a].hostPlayer) == playerRemap.end())
			throw content_error("invalid AI.Host in GameSetup script");

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
				throw content_error(spring::format("GameSetup: Player %i belong to wrong team: %i", a, playerStartingData[a].team));

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
		if (allyteamRemap.find(teamStartingData[a].teamAllyteam) == allyteamRemap.end())
			throw content_error("invalid Team.Allyteam in GameSetup script");

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
	menuName    = file.SGetValueDef("",  "GAME\\MenuName");
	demoName    = file.SGetValueDef("",  "GAME\\Demofile");
	hostDemo    = !demoName.empty();

	file.GetTDef(gameStartDelay, 4u, "GAME\\GameStartDelay");

	file.GetDef(recordDemo,          "1", "GAME\\RecordDemo");
	file.GetDef(useLuaGaia,          "1", "GAME\\ModOptions\\LuaGaia");
	file.GetDef(noHelperAIs,         "0", "GAME\\ModOptions\\NoHelperAIs");
	file.GetDef(maxUnitsPerTeam, "32000", "GAME\\ModOptions\\MaxUnits");
	file.GetDef(disableMapDamage,    "0", "GAME\\ModOptions\\DisableMapDamage");
	file.GetDef(ghostedBuildings,    "1", "GAME\\ModOptions\\GhostedBuildings");

	file.GetDef(maxSpeed, "20.", "GAME\\ModOptions\\MaxSpeed");
	file.GetDef(minSpeed, "0.3", "GAME\\ModOptions\\MinSpeed");

	file.GetDef(fixedAllies, "1", "GAME\\ModOptions\\FixedAllies");

	// Read the map & mod options
	if (file.SectionExist("GAME\\MapOptions")) { mapOptions = file.GetAllValues("GAME\\MapOptions"); }
	if (file.SectionExist("GAME\\ModOptions")) { modOptions = file.GetAllValues("GAME\\ModOptions"); }

	// Read startPosType (with clamping)
	file.GetDef((std::underlying_type<StartPosType>::type&) startPosType, IntToString(StartPos_Fixed), "GAME\\StartPosType");

	startPosType = std::max(startPosType, StartPos_Fixed);
	startPosType = std::min(startPosType, StartPos_Last);

	// Read subsections
	spring::unordered_set<std::string> playersNameList;

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
	modName = GetRapidPackageFromTag(modName);
	modName = archiveScanner->NameFromArchive(modName);
	file.GetDef(onlyLocal, (archiveScanner->GetArchiveData(modName).GetOnlyLocal() ? "1" : "0"), "GAME\\OnlyLocal");

	return true;
}

const std::string CGameSetup::MapFile() const
{
	return (archiveScanner->MapNameToMapFile(mapName));
}

