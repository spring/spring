#ifndef __GAME_SETUP_H__
#define __GAME_SETUP_H__

#include <string>
#include <map>
#include "TdfParser.h"


class CGameSetup
{
public:
	CGameSetup();
	~CGameSetup();
	bool Init(std::string setupFile);
	void Draw();
	bool Update();

	TdfParser file;
	std::string setupFileName;
	int myPlayer;
	int numPlayers;				//the expected amount of players
	std::string mapname;
	std::string baseMod;
	std::string scriptName;

	std::string hostip;
	int hostport;
	int sourceport;			//the port clients will try to connect from

	int readyTime;
	bool forceReady;

	bool Init(char* buf, int size);

	char* gameSetupText;
	int gameSetupTextLength;

	int startPosType;			//0 fixed 1 random 2 select in map
	bool readyTeams[MAX_TEAMS];
	int teamStartNum[MAX_TEAMS];
	float startRectTop[MAX_TEAMS];
	float startRectBottom[MAX_TEAMS];
	float startRectLeft[MAX_TEAMS];
	float startRectRight[MAX_TEAMS];

	std::map<std::string, int> restrictedUnits;
	
	std::string aiDlls[MAX_TEAMS];

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
};

extern CGameSetup* gameSetup;

#endif // __GAME_SETUP_H__
