// Game.h: interface for the CGame class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __GAME_H__
#define __GAME_H__

#include <time.h>
#include <string>
#include <map>
#include "SDL_types.h"

#include "GameController.h"

class CScript;
class CBaseWater;
class CAVIGenerator;
class CConsoleHistory;
class CWordCompletion;
class CKeySet;
class CInfoConsole;
class LuaParser;
class CLoadSaveHandler;
class Action;
class ChatMessage;


class CGame : public CGameController
{
public:
	CR_DECLARE(CGame);			//Don't use CGame pointer in CR_MEMBER()!!!
	void PostLoad();

	void SimFrameMT();
	static void SimFrameMTcb(void *c) {((CGame *)c)->SimFrameMT();}

	bool Draw();
	bool DrawMT();

	static void DrawMTcb(void *c) {((CGame *)c)->DrawMT();}
	void UnsyncedStuff();
	static void UnsyncedStuffcb(void *c) {((CGame *)c)->UnsyncedStuff();}
	bool Update();
	int KeyReleased(unsigned short k);
	int KeyPressed(unsigned short k,bool isRepeat);
	CGame(std::string mapname, std::string modName, CInfoConsole *infoConsole, CLoadSaveHandler *saveFile);
	void ResizeEvent();
	virtual ~CGame();

	bool ActionPressed(const Action&, const CKeySet& ks, bool isRepeat);
	bool ActionReleased(const Action&);
	
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

	bool playing;
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
	bool drawMapMarks;
	float crossSize; // locked mouse indicator size	

	bool drawSky;
	bool drawWater;
	bool drawGround;

	bool soundEnabled;
	float gameSoundVolume;
	float unitReplyVolume;

	unsigned char gameID[16];

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
	/// show GameEnd-window, calculate mouse movement etc.
	void GameEnd();
	/// Send a message to other players (allows prefixed messages with e.g. "a:...")
	void SendNetChat(std::string message, int destination = -1);
	/// Format and display a chat message recieved over network
	void HandleChatMsg(const ChatMessage& msg);
	
	/// synced actions (recieved from server) go in here
	void ActionReceived(const Action&, int playernum);

	void DrawInputText();
	void ParseInputTextGeometry(const std::string& geo);

	void SelectUnits(const std::string& line);
	void SelectCycle(const std::string& command);

	void ReColorTeams();

	void ReloadCOB(const std::string& msg, int player);
	void Skip(int toFrame);

	std::string hotBinding;
	float inputTextPosX;
	float inputTextPosY;
	float inputTextSizeX;
	float inputTextSizeY;
	float lastCpuUsageTime;
	bool skipping;
	
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
	void ClientReadNet();
	void UpdateUI();
	bool DrawWorld();
	
	void SimFrame();
	void StartPlaying();
	
	// to smooth out SimFrame calls
	int leastQue;       ///< Lowest value of que in the past second.
	float timeLeft;     ///< How many SimFrame() calls we still may do.
	float consumeSpeed; ///< How fast we should eat NETMSG_NEWFRAMEs.
	unsigned lastframe; ///< SDL_GetTicks() in previous ClientReadNet() call.

};


extern CGame* game;


#endif // __GAME_H__
