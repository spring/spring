// Game.h: interface for the CGame class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __GAME_H__
#define __GAME_H__

//#include <winsock2.h>
#ifdef _WIN32
#include <windows.h>		// Header File For Windows
#endif
#include <time.h>
#include <string>
#include "SDL_types.h"

#include "ProjectileHandler.h"
#include "Player.h"
#include "GameController.h"

#define FRAME_HISTORY 16

class CglList;
class CNet;
class CGuiKeyReader;
class CScript;
class CBaseWater;
class CAVIGenerator;

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
	CGame(bool server, std::string mapname);
	virtual ~CGame();

	unsigned int oldframenum;
	unsigned int fps;
	unsigned int thisFps;

	time_t   fpstimer,starttime;
	Uint64 lastUpdate;
	Uint64 lastMoveUpdate;

	unsigned char inbuf[40000];	//buffer space for incomming data	//should be NETWORK_BUFFER_SIZE
	int inbufpos;								//where in the input buffer we are
	int inbuflength;						//last byte in input buffer

	bool bOneStep;						//do a single step while game is paused

	float3 trackPos[16];
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

	CGuiKeyReader* guikeys;
	CScript* script;

	void MakeMemDump(void);

	bool creatingVideo;
	CAVIGenerator* aviGenerator;

	bool trackingUnit;
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
