#ifndef __GAME_SETUP_DATA_H__
#define __GAME_SETUP_DATA_H__

#include <string>
#include <map>

#include "GlobalStuff.h"

class TdfParser;

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
	
	std::string setupFileName;
	int myPlayerNum;
	int numPlayers; //the expected amount of players
	int numTeams;
	int numAllyTeams;
	std::string mapName;
	std::string baseMod;
	std::string scriptName;
	std::string luaGaiaStr;
	std::string luaRulesStr;
	
	std::string hostip;
	int hostport;
	int sourceport; //the port clients will try to connect from
	
	int readyTime;
	bool forceReady;
	
	char* gameSetupText;
	int gameSetupTextLength;
	
	StartPosType startPosType;
	bool readyTeams[MAX_TEAMS];
	int teamStartNum[MAX_TEAMS];
	float startRectTop[MAX_TEAMS];
	float startRectBottom[MAX_TEAMS];
	float startRectLeft[MAX_TEAMS];
	float startRectRight[MAX_TEAMS];
	
	std::map<std::string, int> restrictedUnits;
	
	std::string aiDlls[MAX_TEAMS];
	
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

