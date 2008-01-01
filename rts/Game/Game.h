// Game.h: interface for the CGame class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __GAME_H__
#define __GAME_H__

#include <time.h>
#include <string>
#include <deque>
#include <set>
#include <map>
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
class LuaParser;

class CGame : public CGameController
{
public:
	CR_DECLARE(CGame);			//Don't use CGame pointer in CR_MEMBER()!!!
	void UpdateUI();
	bool ClientReadNet();
	void PostLoad();

	void SimFrame();
	void StartPlaying();
	bool DrawWorld();
	bool Draw();
	bool Update();
	int KeyReleased(unsigned short k);
	int KeyPressed(unsigned short k,bool isRepeat);
	CGame(std::string mapname, std::string modName, CInfoConsole *infoConsole);
	void ResizeEvent();
	virtual ~CGame();

	bool ActionPressed(const CKeyBindings::Action&, const CKeySet& ks, bool isRepeat);
	bool ActionReleased(const CKeyBindings::Action&);
	
	bool HasLag() const;

	enum DrawMode {
		notDrawing     = 0,
		normalDraw     = 1,
		shadowDraw     = 2,
		reflectionDraw = 3,
		refractionDraw = 4
	};
	DrawMode drawMode;
	inline void     SetDrawMode(DrawMode mode) { drawMode = mode; }
	inline DrawMode GetDrawMode() const { return drawMode; }

	LuaParser* defsParser;

	unsigned int oldframenum;
	unsigned int fps;
	unsigned int thisFps;

	time_t fpstimer, starttime;
	unsigned lastUpdate;
	unsigned lastMoveUpdate;
	unsigned lastModGameTimeMeasure;

	unsigned lastUpdateRaw;
	float updateDeltaSeconds;

	float totalGameTime;			//time in seconds, stops at game end

	std::string userInputPrefix;

	int lastTick;
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
	bool fullscreenEdgeMove;
	bool showFPS;
	bool showClock;
	bool noSpectatorChat;			//prevents spectator msgs from being seen by players
	bool drawFpsHUD;
	float crossSize; // locked mouse indicator size	

	bool drawSky;
	bool drawWater;
	bool drawGround;

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

	void SetHotBinding(const std::string& action) { hotBinding = action; }

protected:
	void SendNetChat(const string& message);
	void HandleChatMsg(std::string msg, int player);

	void DrawInputText();
	void ParseInputTextGeometry(const string& geo);

	void SelectUnits(const string& line);
	void SelectCycle(const string& command);

	void ReColorTeams();

	void LogNetMsg(const string& msg, int player);
	void ReloadCOB(const string& msg, int player);
	void Skip(const string& msg);

	std::string hotBinding;
	float inputTextPosX;
	float inputTextPosY;
	float inputTextSizeX;
	float inputTextSizeY;
	float lastCpuUsageTime;
	
	unsigned lastFrameTime;

public:
	struct PlayerTrafficInfo {
		PlayerTrafficInfo() : total(0) {}
		int total;
		std::map<int, int> packets;
	};
	const std::map<int, PlayerTrafficInfo>& GetPlayerTraffic() const {
		return playerTraffic;
	}
protected:
	void AddTraffic(int playerID, int packetCode, int length);
	// <playerID, <packetCode, total bytes> >
	std::map<int, PlayerTrafficInfo> playerTraffic;

private:

	// to smooth out SimFrame calls
	int leastQue;       ///< Lowest value of que in the past second.
	float timeLeft;     ///< How many SimFrame() calls we still may do.
	float consumeSpeed; ///< How fast we should eat NETMSG_NEWFRAMEs.
	unsigned lastframe; ///< SDL_GetTicks() in previous ClientReadNet() call.

};


extern CGame* game;


#endif // __GAME_H__
