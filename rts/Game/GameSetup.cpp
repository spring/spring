/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GameSetup.h"
#include "System/TdfParser.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "Map/MapParser.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/UnsyncedRNG.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/Log/ILog.h"

#include <algorithm>
#include <map>
#include <cctype>
#include <cstring>
#include <boost/format.hpp>


CR_BIND(CGameSetup,);
CR_REG_METADATA(CGameSetup, (
	CR_MEMBER(gameSetupText),
	CR_POSTLOAD(PostLoad)
));

void CGameSetup::PostLoad()
{
	const std::string buf = gameSetupText;
	Init(buf);
}


CGameSetup* gameSetup = NULL;

CGameSetup::CGameSetup()
	: fixedAllies(true)
	, mapHash(0)
	, modHash(0)
	, mapSeed(0)
	, useLuaGaia(true)
	, startPosType(StartPos_Fixed)
	, maxUnitsPerTeam(1500)
	, ghostedBuildings(true)
	, disableMapDamage(false)
	, maxSpeed(0.0f)
	, minSpeed(0.0f)
	, onlyLocal(false)
	, hostDemo(false)
	, numDemoPlayers(0)
	, gameStartDelay(0)
	, noHelperAIs(false)
{}

CGameSetup::~CGameSetup()
{
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

	for(size_t a = 0; a < teamStartingData.size(); ++a) {
		float3 pos(1000.0f, 100.0f, 1000.0f);
		if (!mapParser.GetStartPos(teamStartingData[a].teamStartNum, pos)) // don't fail when playing with more players than startpositions and we didn't use them anyway
			throw content_error(mapParser.GetErrorLog());
		teamStartingData[a].startPos = float3(pos.x, pos.y, pos.z);
	}
}

void CGameSetup::LoadStartPositions(bool withoutMap)
{
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

	if (startPosType == StartPos_Fixed || startPosType == StartPos_Random)
		LoadStartPositionsFromMap();

	// Show that we havent selected start pos yet
	if (startPosType == StartPos_ChooseInGame) {
		for (size_t a = 0; a < teamStartingData.size(); ++a) {
			teamStartingData[a].startPos.y = -500;
		}
	}
}

std::string CGameSetup::MapFile() const
{
	return archiveScanner->MapNameToMapFile(mapName);
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
				_STPF_" players in GameSetup script (NumPlayers says %i)",
				playerStartingData.size(), playerCount);
	}
}

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

const std::vector<SkirmishAIData>& CGameSetup::GetSkirmishAIs() const {
	return skirmishAIStartingData;
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
		for (size_t num = 0; num < 3; ++num)
		{
			data.color[num] = TeamBase::teamDefaultColor[a][num];
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
	if (file.GetValue(teamCount, "Game\\NumTeams") && teamStartingData.size() != teamCount) {
		LOG_L(L_WARNING,
				_STPF_" teams in GameSetup script (NumTeams: %i)",
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
	if (file.GetValue(allyCount, "GAME\\NumAllyTeams")
			&& (allyStartingData.size() != allyCount))
	{
		LOG_L(L_WARNING, "Incorrect number of ally teams in GameSetup script");
	}
}

void CGameSetup::RemapPlayers()
{
	// relocate Team.TeamLeader field
	for (size_t a = 0; a < teamStartingData.size(); ++a) {
		if (playerRemap.find(teamStartingData[a].leader) == playerRemap.end()) {
			std::ostringstream buf;
			buf << "GameSetup: Team " << a << " has invalid leader: " << teamStartingData[a].leader;
			throw content_error(buf.str());
		}
		teamStartingData[a].leader = playerRemap[teamStartingData[a].leader];
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
			throw content_error("invalid AI.Team in GameSetup script");
		skirmishAIStartingData[a].team = teamRemap[skirmishAIStartingData[a].team];
		team_skirmishAI[skirmishAIStartingData[a].team] = &(skirmishAIStartingData[a]);
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
	// Copy buffer contents
	gameSetupText = buf;

	// Parse
	TdfParser file(buf.c_str(),buf.size());

	if (!file.SectionExist("GAME"))
		return false;

	// Game parameters

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

	file.GetDef(onlyLocal,           "0", "GAME\\OnlyLocal");
	file.GetDef(useLuaGaia,          "1", "GAME\\ModOptions\\LuaGaia");
	file.GetDef(noHelperAIs,         "0", "GAME\\ModOptions\\NoHelperAIs");
	file.GetDef(maxUnitsPerTeam,  "1500", "GAME\\ModOptions\\MaxUnits");
	file.GetDef(disableMapDamage,    "0", "GAME\\ModOptions\\DisableMapDamage");
	file.GetDef(ghostedBuildings,    "1", "GAME\\ModOptions\\GhostedBuildings");

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
	modName = archiveScanner->NameFromArchive(modName);

	return true;
}
