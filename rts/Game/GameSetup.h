/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_SETUP_H
#define _GAME_SETUP_H

#include <functional>
#include <string>
#include <vector>

#include "Players/PlayerBase.h"
#include "Sim/Misc/TeamBase.h"
#include "Sim/Misc/AllyTeam.h"
#include "ExternalAI/SkirmishAIData.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"
#include "System/creg/creg_cond.h"

class MapParser;
class TdfParser;

class CGameSetup
{
	CR_DECLARE_STRUCT(CGameSetup)

public:
	CGameSetup() { ResetState(); }
	CGameSetup(const CGameSetup& gs) = delete;
	CGameSetup(CGameSetup&& gs) { *this = std::move(gs); }

	CGameSetup& operator = (const CGameSetup& gs) = delete;
	CGameSetup& operator = (CGameSetup&& gs) {
		fixedAllies = gs.fixedAllies;
		useLuaGaia = gs.useLuaGaia;
		luaDevMode = gs.luaDevMode;
		noHelperAIs = gs.noHelperAIs;

		ghostedBuildings = gs.ghostedBuildings;
		disableMapDamage = gs.disableMapDamage;

		onlyLocal = gs.onlyLocal;
		hostDemo = gs.hostDemo;
		recordDemo = gs.recordDemo;

		std::copy(gs.dsMapHash, gs.dsMapHash + sizeof(dsMapHash), dsMapHash);
		std::copy(gs.dsModHash, gs.dsModHash + sizeof(dsModHash), dsModHash);
		mapSeed = gs.mapSeed;

		gameStartDelay = gs.gameStartDelay;

		numDemoPlayers = gs.numDemoPlayers;
		maxUnitsPerTeam = gs.maxUnitsPerTeam;

		maxSpeed = gs.maxSpeed;
		minSpeed = gs.minSpeed;

		startPosType = gs.startPosType;

		mapName = std::move(gs.mapName);
		modName = std::move(gs.modName);
		gameID = std::move(gs.gameID);

		setupText = std::move(gs.setupText);
		reloadScript = std::move(gs.reloadScript);
		demoName = std::move(gs.demoName);

		playerRemap = std::move(gs.playerRemap);
		teamRemap = std::move(gs.teamRemap);
		allyteamRemap = std::move(gs.allyteamRemap);

		playerStartingData = std::move(gs.playerStartingData);
		teamStartingData = std::move(gs.teamStartingData);
		allyStartingData = std::move(gs.allyStartingData);
		skirmishAIStartingData = std::move(gs.skirmishAIStartingData);
		mutatorsList = std::move(gs.mutatorsList);

		restrictedUnits = std::move(gs.restrictedUnits);

		mapOptions = std::move(gs.mapOptions);
		modOptions = std::move(gs.modOptions);
		return *this;
	}

	static bool LoadReceivedScript(const std::string& script, bool isHost);
	static bool LoadSavedScript(const std::string& file, const std::string& script);
	static bool ScriptLoaded();

	// these act on the global GameSetup instance
	static const spring::unordered_map<std::string, std::string>& GetMapOptions();
	static const spring::unordered_map<std::string, std::string>& GetModOptions();
	static const std::vector<PlayerBase>& GetPlayerStartingData();
	static const std::vector<TeamBase>& GetTeamStartingData();
	static const std::vector<AllyTeam>& GetAllyStartingData();

	void ResetState();

	bool Init(const std::string& script);

	/**
	 * @brief Load startpositions from map/script
	 * @pre numTeams and startPosType initialized
	 * @post readyTeams, teamStartNum and team start positions initialized
	 *
	 * Unlike the other functions, this is not called on Init(),
	 * instead we wait for CPreGame to call this. The reason is that the map
	 * is not known before CPreGame recieves the gamedata from the server.
	 */
	void LoadStartPositions(bool withoutMap = false);
	/**
	 * @brief Load startpositions from map
	 * @pre mapName, numTeams, teamStartNum initialized and the map loaded (LoadMap())
	 */
	void LoadStartPositionsFromMap(int numTeams, const std::function<bool(MapParser& mapParser, int teamNum)>& startPosPred);

	int GetRestrictedUnitLimit(const std::string& name, int defLimit) const {
		const auto it = restrictedUnits.find(name);
		if (it == restrictedUnits.end())
			return defLimit;
		return (it->second);
	}

	const spring::unordered_map<std::string, std::string>& GetMapOptionsCont() const { return mapOptions; }
	const spring::unordered_map<std::string, std::string>& GetModOptionsCont() const { return modOptions; }
	const std::vector<PlayerBase>& GetPlayerStartingDataCont() const { return playerStartingData; }
	const std::vector<TeamBase>& GetTeamStartingDataCont() const { return teamStartingData; }
	const std::vector<AllyTeam>& GetAllyStartingDataCont() const { return allyStartingData; }
	const std::vector<SkirmishAIData>& GetAIStartingDataCont() const { return skirmishAIStartingData; }
	const std::vector<std::string>& GetMutatorsCont() const { return mutatorsList; }

	std::string MapFileName() const;

private:
	void LoadMutators(const TdfParser& file, std::vector<std::string>& mutatorsList);
	/**
	 * @brief Load unit restrictions
	 * @post restrictedUnits initialized
	 */
	void LoadUnitRestrictions(const TdfParser& file);
	/**
	 * @brief Load players and remove gaps in the player numbering.
	 * @pre numPlayers initialized
	 * @post players loaded, numDemoPlayers initialized
	 */
	void LoadPlayers(const TdfParser& file, spring::unordered_set<std::string>& nameList);
	/**
	 * @brief Load LUA and Skirmish AIs.
	 */
	void LoadSkirmishAIs(const TdfParser& file, spring::unordered_set<std::string>& nameList);
	/**
	 * @brief Load teams and remove gaps in the team numbering.
	 * @pre numTeams, hostDemo initialized
	 * @post teams loaded
	 */
	void LoadTeams(const TdfParser& file);
	/**
	 * @brief Load allyteams and remove gaps in the allyteam numbering.
	 * @pre numAllyTeams initialized
	 * @post allyteams loaded, alliances initialised (no remapping needed here)
	 */
	void LoadAllyTeams(const TdfParser& file);

	/** @brief Update all player indices to refer to the right player. */
	void RemapPlayers();
	/** @brief Update all team indices to refer to the right team. */
	void RemapTeams();
	/** @brief Update all allyteam indices to refer to the right allyteams. (except allies) */
	void RemapAllyteams();

public:
	enum StartPosType {
		StartPos_Fixed            = 0,
		StartPos_Random           = 1,
		StartPos_ChooseInGame     = 2,
		StartPos_ChooseBeforeGame = 3,
		StartPos_Last             = 3  // last entry in enum (for user input check)
	};


	bool fixedAllies;
	bool useLuaGaia;
	bool luaDevMode;
	bool noHelperAIs;

	bool ghostedBuildings;
	bool disableMapDamage;

	/** if true, this is a non-network game (one local client, eg. when watching a demo) */
	bool onlyLocal;
	bool hostDemo;
	bool recordDemo;

	uint8_t dsMapHash[64];
	uint8_t dsModHash[64];
	uint32_t mapSeed;

	/**
	 * Number of seconds until the game starts, counting
	 * from the moment when all players are connected and ready.
	 * Default: 4 (seconds)
	 */
	unsigned int gameStartDelay;

	int numDemoPlayers;
	int maxUnitsPerTeam;

	float maxSpeed;
	float minSpeed;

	StartPosType startPosType;

	std::string mapName;
	std::string modName;
	std::string gameID;

	std::string setupText;
	std::string reloadScript;
	std::string demoName;

private:
	spring::unordered_map<int, int> playerRemap;
	spring::unordered_map<int, int> teamRemap;
	spring::unordered_map<int, int> allyteamRemap;

	std::vector<PlayerBase> playerStartingData;
	std::vector<TeamBase> teamStartingData;
	std::vector<AllyTeam> allyStartingData;
	std::vector<SkirmishAIData> skirmishAIStartingData;
	std::vector<std::string> mutatorsList;

	spring::unordered_map<std::string, int> restrictedUnits;

	spring::unordered_map<std::string, std::string> mapOptions;
	spring::unordered_map<std::string, std::string> modOptions;
};

extern CGameSetup* gameSetup;

#endif // _GAME_SETUP_H
