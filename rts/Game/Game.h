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
#include "UI/KeyBindings.h"


#define FRAME_HISTORY 16

class CNet;
class CScript;
class CBaseWater;
class CAVIGenerator;
class CConsoleHistory;
class CWordCompletion;
class CKeySet;
class CInfoConsole;

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
	CGame(bool server, std::string mapname, std::string modName, CInfoConsole *infoConsole);
	void ResizeEvent();
	virtual ~CGame();
	
	bool ActionPressed(const CKeyBindings::Action&, const CKeySet& ks, bool isRepeat);
	bool ActionReleased(const CKeyBindings::Action&);

	unsigned int oldframenum;
	unsigned int fps;
	unsigned int thisFps;

	time_t fpstimer, starttime;
	Uint64 lastUpdate;
	Uint64 lastMoveUpdate;

	Uint64 lastModGameTimeMeasure;

	Uint64 lastframe;
	float totalGameTime;			//time in seconds, stops at game end

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

	int skipping;
	bool playing;
	bool allReady;
	bool chatting;
	bool camMove[8];
	bool camRot[4];
	bool hideInterface;
	bool gameOver;
	bool windowedEdgeMove;
	bool showClock;
	bool noSpectatorChat;			//prevents spectator msgs from being seen by players
	bool drawFpsHUD;

	float maxUserSpeed;
	float minUserSpeed;
	bool gamePausable;

	bool soundEnabled;
	float gameSoundVolume;

	CScript* script;

	CInfoConsole *infoConsole;

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
	
	void HandleChatMsg(std::string msg, int player, bool demoPlayer);
	
	void SetHotBinding(const std::string& action) { hotBinding = action; }

protected:
	void SendNetChat(const string& message);

	void DrawInputText();
	void ParseInputTextGeometry(const string& geo);

	void SelectUnits(const string& line);
	void SelectCycle(const string& command);

	void LogNetMsg(const string& msg, int player);
	void ReloadCOB(const string& msg, int player);
	void Skip(const string& msg, bool demoPlayer);

protected:
	std::string hotBinding;
	float inputTextPosX;
	float inputTextPosY;
	float inputTextSizeX;
	float inputTextSizeY;
	float lastCpuUsageTime;
};


extern CGame* game;


#endif // __GAME_H__
