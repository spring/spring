// Game.h: interface for the CGame class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GAME_H__10D2D641_3483_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_GAME_H__10D2D641_3483_11D4_AD55_0080ADA84DE3__INCLUDED_

	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>
#include <windows.h>		// Header File For Windows
#include <time.h>
#include <string>

#include "ProjectileHandler.h"
#include "player.h"
#include "gamecontroller.h"

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
	int KeyReleased(unsigned char k);
	int KeyPressed(unsigned char k,bool isRepeat);
	CGame(bool server, std::string mapname);
	virtual ~CGame();

	unsigned int oldframenum;
	unsigned int fps;
	unsigned int thisFps;

	time_t   fpstimer,starttime;
	LARGE_INTEGER lastUpdate;
	LARGE_INTEGER lastMoveUpdate;

	unsigned char inbuf[40000];	//buffer space for incomming data	//should be NETWORK_BUFFER_SIZE
	int inbufpos;								//where in the input buffer we are
	int inbuflength;						//last byte in input buffer

	bool bOneStep;						//do a single step while game is paused

	float3 trackPos[16];
	int leastQue;

	int lastTick;
	int lastLength;
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

	LARGE_INTEGER lastModGameTimeMeasure;

	CglList* showList;
	LARGE_INTEGER lastframe;
	LARGE_INTEGER timeSpeed;
	double totalGameTime;			//time in seconds, stops at game end

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

#endif // !defined(AFX_GAME_H__10D2D641_3483_11D4_AD55_0080ADA84DE3__INCLUDED_)
