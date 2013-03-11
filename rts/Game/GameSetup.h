/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_SETUP_H
#define _GAME_SETUP_H

#include <string>
#include <map>
#include <vector>
#include <set>

#include "PlayerBase.h"
#include "Sim/Misc/TeamBase.h"
#include "Sim/Misc/AllyTeam.h"
#include "ExternalAI/SkirmishAIData.h"
#include "System/creg/creg_cond.h"

class TdfParser;

class CGameSetup
{
	CR_DECLARE_STRUCT(CGameSetup);
	void PostLoad();

public:
	CGameSetup();
	~CGameSetup();

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

	int GetRestrictedUnitLimit(const std::string& name, int defLimit) const {
		const std::map<std::string, int>::const_iterator it = restrictedUnits.find(name);
		if (it == restrictedUnits.end())
			return defLimit;
		return (it->second);
	}

	enum StartPosType
	{
		StartPos_Fixed = 0,
		StartPos_Random = 1,
		StartPos_ChooseInGame = 2,
		StartPos_ChooseBeforeGame = 3,
		StartPos_Last = 3  // last entry in enum (for user input check)
	};

	bool fixedAllies;
	unsigned int mapHash;
	unsigned int modHash;
	unsigned int mapSeed;
	std::string MapFile() const;
	std::string mapName;
	std::string modName;
	std::string gameID;
	bool useLuaGaia;

	std::string gameSetupText;

	StartPosType startPosType;

	std::vector<PlayerBase> playerStartingData;

	const std::vector<SkirmishAIData>& GetSkirmishAIs() const;

public:

	std::vector<TeamBase> teamStartingData;
	std::vector<AllyTeam> allyStartingData;

	std::map<std::string, std::string> mapOptions;
	std::map<std::string, std::string> modOptions;

	int maxUnitsPerTeam;

	bool ghostedBuildings;
	bool disableMapDamage;

	float maxSpeed;
	float minSpeed;

	/** if true, this is a non-network game (one local client, eg. when watching a demo) */
	bool onlyLocal;

	bool hostDemo;
	std::string demoName;
	int numDemoPlayers;

	std::string saveName;

	/**
	 * The number of seconds till the game starts,
	 * counting from the moment when all players are connected and ready.
	 * Default: 4 (seconds)
	 */
	unsigned int gameStartDelay;

	bool noHelperAIs;

private:
	/**
	 * @brief Load startpositions from map
	 * @pre mapName, numTeams, teamStartNum initialized and the map loaded (LoadMap())
	 */
	void LoadStartPositionsFromMap();
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
	void LoadPlayers(const TdfParser& file, std::set<std::string>& nameList);
	/**
	 * @brief Load LUA and Skirmish AIs.
	 */
	void LoadSkirmishAIs(const TdfParser& file, std::set<std::string>& nameList);
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

	std::map<int, int> playerRemap;
	std::map<int, int> teamRemap;
	std::map<int, int> allyteamRemap;

	std::vector<SkirmishAIData> skirmishAIStartingData;
	std::map<int, const SkirmishAIData*> team_skirmishAI;

	std::map<std::string, int> restrictedUnits;
};

extern CGameSetup* gameSetup;

#endif // _GAME_SETUP_H
