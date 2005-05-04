#ifndef __GAME_SETUP_H__
#define __GAME_SETUP_H__

#include "archdef.h"

#include <string>

class CSunParser;

class CGameSetup
{
public:
	CGameSetup(void);
	~CGameSetup(void);
	bool Init(std::string setupFile);

	CSunParser* file;
	int myPlayer;
	int numPlayers;				//the expected amount of players
	std::string mapname;

	std::string hostip;
	int hostport;
	bool Draw(void);

	float readyTime;
	bool Init(char* buf, int size);

	char* gameSetupText;
	int gameSetupTextLength;

	int startPosType;			//0 fixed 1 random 2 select in map
	bool readyTeams[MAX_TEAMS];
	int teamStartNum[MAX_TEAMS];
};

extern CGameSetup* gameSetup;

#endif // __GAME_SETUP_H__
