#ifndef __GAME_SETUP_H__
#define __GAME_SETUP_H__

#include <string>
#include <map>
#include <vector>
#include <set>

#include "SFloat3.h"
#include "PlayerBase.h"
#include "ExternalAI/SkirmishAIData.h"

class TdfParser;

class LocalSetup
{
public:
	LocalSetup();

	void Init(const std::string& setup);

	int myPlayerNum;
	std::string myPlayerName;

	std::string hostip;
	int hostport;
	int sourceport; //the port clients will try to connect from
	int autohostport;

	bool isHost;
};

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

	int numPlayers; //the expected amount of players
	int numTeams;
	int numAllyTeams;
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
	std::vector<SkirmishAIData> skirmishAIStartingData;
	std::map<int, const SkirmishAIData*> team_skirmishAI;
	const SkirmishAIData* GetSkirmishAIDataForTeam(int teamId) const;
	size_t GetSkirmishAIs() const;

	struct TeamData
	{
		unsigned leader;
		unsigned char color[4];
		float handicap;
		std::string side;
		SFloat3 startPos;
		int teamStartNum;
		int teamAllyteam;
	};
	std::vector<TeamData> teamStartingData;

	struct AllyTeamData
	{
		float startRectTop;
		float startRectBottom;
		float startRectLeft;
		float startRectRight;
		std::vector<bool> allies;
	};
	std::vector<AllyTeamData> allyStartingData;

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
};

extern const CGameSetup* gameSetup;

#endif // __GAME_SETUP_H__
