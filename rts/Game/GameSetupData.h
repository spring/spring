#ifndef __GAME_SETUP_DATA_H__
#define __GAME_SETUP_DATA_H__

#include <string>
#include <map>

#include "SFloat3.h"
#include "GlobalStuff.h"


class CGameSetupData
{
public:	
	enum StartPosType
	{
		StartPos_Fixed = 0,
		StartPos_Random = 1,
		StartPos_ChooseInGame = 2,
		StartPos_ChooseBeforeGame = 3,
		StartPos_Last = 3  // last entry in enum (for user input check)
	};
	
	CGameSetupData();
	~CGameSetupData();
	
	int myPlayerNum;
	int numPlayers; //the expected amount of players
	int numTeams;
	int numAllyTeams;
	bool fixedAllies;
	std::string mapName;
	std::string baseMod;
	std::string scriptName;
	bool useLuaGaia;
	std::string luaGaiaStr;
	std::string luaRulesStr;
	
	std::string hostip;
	int hostport;
	int sourceport; //the port clients will try to connect from
	int autohostport;
	
	char* gameSetupText;
	int gameSetupTextLength;
	
	StartPosType startPosType;
	
	
	/// Team the player will start in (read-only)
	struct PlayerData
	{
		unsigned team;
		int rank;
		std::string name;
		std::string countryCode;
		bool spectator;
		bool isFromDemo;
	};
	PlayerData playerStartingData[MAX_PLAYERS];
	
	struct TeamData
	{
		unsigned leader;
		unsigned char color[4];
		float handicap;
		std::string side;
		SFloat3 startPos;
		int teamStartNum;
		int teamAllyteam;
		std::string aiDll;
	};
	TeamData teamStartingData[MAX_TEAMS];
	
	struct AllyTeamData
	{
		float startRectTop;
		float startRectBottom;
		float startRectLeft;
		float startRectRight;
		bool allies[MAX_TEAMS];
	};
	AllyTeamData allyStartingData[MAX_TEAMS];
	
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

protected:

	std::map<int, int> playerRemap;
	std::map<int, int> teamRemap;
	std::map<int, int> allyteamRemap;
};

#endif // __GAME_SETUP_DATA_H__

