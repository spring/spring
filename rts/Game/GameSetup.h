#ifndef __GAME_SETUP_H__
#define __GAME_SETUP_H__

#include <string>
#include <map>
#include <vector>
#include <set>

#include "float3.h"
#include "PlayerBase.h"
#include "Sim/Misc/TeamBase.h"
#include "Sim/Misc/AllyTeam.h"
#include "ExternalAI/SkirmishAIData.h"

class TdfParser;

namespace GameMode
{
	const int ComContinue = 0;
	const int ComEnd = 1;
	const int Lineage = 2;
	const int OpenEnd = 3;
};

class CGameSetup
{
public:
	CGameSetup();
	~CGameSetup();
	bool Init(const std::string& script);
	void LoadStartPositions(bool withoutMap = false);

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
	std::string mapName;
	std::string baseMod;
	std::string scriptName;
	bool useLuaGaia;
	std::string luaGaiaStr;
	std::string luaRulesStr;

	std::string gameSetupText;

	StartPosType startPosType;

	std::vector<PlayerBase> playerStartingData;
	const SkirmishAIData* GetSkirmishAIDataForTeam(int teamId) const;
	size_t GetSkirmishAIs() const;

	std::vector<TeamBase> teamStartingData;
	std::vector<AllyTeam> allyStartingData;

	std::map<std::string, int> restrictedUnits;

	std::map<std::string, std::string> mapOptions;
	std::map<std::string, std::string> modOptions;

	int maxUnits;

	bool ghostedBuildings;
	bool limitDgun;
	bool diminishingMMs;
	bool disableMapDamage;

	float maxSpeed;
	float minSpeed;

	bool hostDemo;
	std::string demoName;
	int numDemoPlayers;

	std::string saveName;

	int startMetal;
	int startEnergy;

	int gameMode;
	int noHelperAIs;

private:
	void LoadStartPositionsFromMap();
	void LoadUnitRestrictions(const TdfParser& file);
	void LoadPlayers(const TdfParser& file, std::set<std::string>& nameList);
	void LoadSkirmishAIs(const TdfParser& file, std::set<std::string>& nameList);
	void LoadTeams(const TdfParser& file);
	void LoadAllyTeams(const TdfParser& file);

	void RemapPlayers();
	void RemapTeams();
	void RemapAllyteams();

	std::map<int, int> playerRemap;
	std::map<int, int> teamRemap;
	std::map<int, int> allyteamRemap;

	std::vector<SkirmishAIData> skirmishAIStartingData;
	std::map<int, const SkirmishAIData*> team_skirmishAI;
};

extern const CGameSetup* gameSetup;

#endif // __GAME_SETUP_H__
