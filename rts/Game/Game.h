/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_H
#define _GAME_H

#include <time.h>
#include <string>
#include <map>
#include <set>

#include "GameController.h"
#include "System/creg/creg_cond.h"

class CBaseWater;
class CConsoleHistory;
class CKeySet;
class CInfoConsole;
class LuaParser;
class LuaInputReceiver;
class ILoadSaveHandler;
class Action;
class ISyncedActionExecutor;
class IUnsyncedActionExecutor;
class ChatMessage;
class SkirmishAIData;
class CWorldDrawer;


class CGame : public CGameController
{
private:
	CR_DECLARE(CGame);	// Do not use CGame pointer in CR_MEMBER()!!!

public:
	void LoadGame(const std::string& mapName);
	void SetupRenderingParams();

private:
	void LoadDefs();
	void LoadSimulation(const std::string& mapName);
	void LoadRendering();
	void LoadInterface();
	void LoadLua();
	void LoadFinalize();
	void PostLoad();

public:
	CGame(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile);
	virtual ~CGame();

	bool Draw();
	bool DrawMT();

	static void DrawMTcb(void* c) { ((CGame *)c)->DrawMT(); }
	bool Update();
	/// Called when a key is released by the user
	int KeyReleased(unsigned short k);
	/// Called when the key is pressed by the user (can be called several times due to key repeat)
	int KeyPressed(unsigned short k, bool isRepeat);
	void ResizeEvent();


	bool ProcessCommandText(unsigned int key, const std::string& command);
	bool ProcessKeyPressAction(unsigned int key, const Action& action);

	bool ActionPressed(unsigned int key, const Action& action, bool isRepeat);
	bool ActionReleased(const Action& action);

	bool HasLag() const;

	/// show GameEnd-window, calculate mouse movement etc.
	void GameEnd(const std::vector<unsigned char>& winningAllyTeams);


	enum GameDrawMode {
		gameNotDrawing     = 0,
		gameNormalDraw     = 1,
		gameShadowDraw     = 2,
		gameReflectionDraw = 3,
		gameRefractionDraw = 4
	};
	GameDrawMode gameDrawMode;

	inline void         SetDrawMode(GameDrawMode mode) { gameDrawMode = mode; }
	inline GameDrawMode GetDrawMode() const { return gameDrawMode; }

	LuaParser* defsParser;

	unsigned int fps;
	unsigned int thisFps;

	int lastSimFrame;

	time_t fpstimer, starttime;
	unsigned lastUpdate;
	unsigned lastMoveUpdate;
	unsigned lastModGameTimeMeasure;

	unsigned lastUpdateRaw;
	float updateDeltaSeconds;

	/// Time in seconds, stops at game end
	float totalGameTime;

	std::string userInputPrefix;

	int lastTick;
	int chatSound;

	bool camMove[8];
	bool camRot[4];
	bool hideInterface;
	bool gameOver;
	bool windowedEdgeMove;
	bool fullscreenEdgeMove;
	bool showFPS;
	bool showClock;
	bool showSpeed;
	int showMTInfo;
	int mtInfoCtrl;
	/// Prevents spectator msgs from being seen by players
	bool noSpectatorChat;
	volatile bool finishedLoading;

	unsigned char gameID[16];

	CInfoConsole* infoConsole;
	CConsoleHistory* consoleHistory;

	void SetHotBinding(const std::string& action) { hotBinding = action; }

public:
	/// Save the game state to file.
	void SaveGame(const std::string& filename, bool overwrite);
	void DumpState(int newMinFrameNum, int newMaxFrameNum, int newFramePeriod);

	/// Re-load the game.
	void ReloadGame();
	/// Send a message to other players (allows prefixed messages with e.g. "a:...")
	void SendNetChat(std::string message, int destination = -1);
	/// Format and display a chat message received over network
	void HandleChatMsg(const ChatMessage& msg);

	/// synced actions (received from server) go in here
	void ActionReceived(const Action& action, int playerID);

	void DrawInputText();
	void ParseInputTextGeometry(const std::string& geo);

	void SelectUnits(const std::string& line);
	void SelectCycle(const std::string& command);

	void ReColorTeams();

public:
	void ReloadCOB(const std::string& msg, int player);
	void ReloadCEGs(const std::string& tag);

	void StartSkip(int toFrame);
	void DrawSkip(bool blackscreen = true);
	void EndSkip();

public:
	std::string hotBinding;
	float inputTextPosX;
	float inputTextPosY;
	float inputTextSizeX;
	float inputTextSizeY;
	float lastCpuUsageTime;
	bool skipping;
	bool playing;
	bool chatting;

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

public:
	void AddTraffic(int playerID, int packetCode, int length);
	// <playerID, <packetCode, total bytes> >
	std::map<int, PlayerTrafficInfo> playerTraffic;

	void ClientReadNet();
	void UpdateUI(bool cam);

	void SimFrame();
	void StartPlaying();

	// to smooth out SimFrame calls
	int leastQue;       ///< Lowest value of que in the past second.
	float timeLeft;     ///< How many SimFrame() calls we still may do.
	float consumeSpeed; ///< How fast we should eat NETMSG_NEWFRAMEs.
	unsigned lastframe; ///< SDL_GetTicks() in previous ClientReadNet() call.

	int skipStartFrame;
	int skipEndFrame;
	int skipTotalFrames;
	float skipSeconds;
	bool skipSoundmute;
	float skipOldSpeed;
	float skipOldUserSpeed;
	unsigned skipLastDraw;

	/**
	 * @see CGameServer#speedControl
	 */
	int speedControl;
	int luaLockTime;
	int luaExportSize;


	/// for reloading the savefile
	ILoadSaveHandler* saveFile;

private:
	CWorldDrawer* worldDrawer;
};


extern CGame* game;


#endif // _GAME_H
