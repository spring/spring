// Game.h: interface for the CGame class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __GAME_H__
#define __GAME_H__

//#include <winsock2.h>
#include <time.h>
#include <string>
#include <deque>
#include <set>
#include "SDL_types.h"

#include "Sim/Projectiles/ProjectileHandler.h"
#include "Player.h"
#include "GameController.h"


#define FRAME_HISTORY 16

class CglList;
class CNet;
class CScript;
class CBaseWater;
class CAVIGenerator;
class CConsoleHistory;
class CWordCompletion;

class CGame : public CGameController
{
public:
	void UpdateUI();
	bool ClientReadNet();

	bool debugging;
	int que;
	void SimFrame();
	void StartPlaying();
	bool Draw();
	bool Update();
	int KeyReleased(unsigned short k);
	int KeyPressed(unsigned short k,bool isRepeat);
	CGame(bool server, std::string mapname, std::string modName);
	virtual ~CGame();

	unsigned int oldframenum;
	unsigned int fps;
	unsigned int thisFps;

	time_t   fpstimer,starttime;
	Uint64 lastUpdate;
	Uint64 lastMoveUpdate;

	unsigned char inbuf[40000*2];	//buffer space for incomming data	//should be NETWORK_BUFFER_SIZE*2
	int inbufpos;								//where in the input buffer we are
	int inbuflength;						//last byte in input buffer

	bool bOneStep;						//do a single step while game is paused

	int leastQue;

	std::string userInputPrefix;

	int lastTick;
	float timeLeft;
	float consumeSpeed;
	int chatSound;

	bool playing;
	bool allReady;
	bool chatting;
	bool camMove[8];
	bool camRot[4];
	bool hideInterface;
	bool gameOver;
	bool windowedEdgeMove;
	bool showClock;
	bool showPlayerInfo;
	bool noSpectatorChat;			//prevents spectator msgs from being seen by players

	Uint64 lastModGameTimeMeasure;

	CglList* showList;
	Uint64 lastframe;
	double totalGameTime;			//time in seconds, stops at game end

	float maxUserSpeed;
	float minUserSpeed;
	bool gamePausable;

	bool soundEnabled;
	float gameSoundVolume;

	CScript* script;

	void MakeMemDump(void);

  CConsoleHistory* consoleHistory;
	CWordCompletion* wordCompletion;

	bool creatingVideo;
	CAVIGenerator* aviGenerator;

	void DrawDirectControlHud(void);
#ifdef DIRECT_CONTROL_ALLOWED
	short oldHeading,oldPitch;
	unsigned char oldStatus;
#endif
	
	void HandleChatMsg(std::string msg,int player);
	unsigned  int CreateExeChecksum(void);
};

extern CGame* game;

#endif // __GAME_H__
