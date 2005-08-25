#ifndef __GAME_SETUP_H__
#define __GAME_SETUP_H__

#include <string>
#include <map>

class CSunParser;

class CGameSetup
{
public:
	CGameSetup(void);
	~CGameSetup(void);
	bool Init(std::string setupFile);
	bool Draw(void);

	CSunParser* file;
	int myPlayer;
	int numPlayers;				//the expected amount of players
	std::string mapname;
	std::string baseMod;

	std::string hostip;
	int hostport;
	int sourceport;			//the port clients will try to connect from

	float readyTime;
	bool forceReady;

	bool Init(char* buf, int size);

	char* gameSetupText;
	int gameSetupTextLength;

	int startPosType;			//0 fixed 1 random 2 select in map
	bool readyTeams[MAX_TEAMS];
	int teamStartNum[MAX_TEAMS];

	std::map<std::string, int> restrictedUnits;
	
	std::string aiDlls[MAX_TEAMS];

	int maxUnits;
};

extern CGameSetup* gameSetup;

#endif // __GAME_SETUP_H__
