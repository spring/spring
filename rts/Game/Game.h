/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_H
#define _GAME_H

#include <string>
#include <map>
#include <set>

#include "GameController.h"
#include "System/creg/creg_cond.h"
#include "System/Misc/SpringTime.h"

class IWater;
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
	CR_DECLARE_STRUCT(CGame);

public:
	CGame(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile);
	virtual ~CGame();

public:
	enum GameDrawMode {
		gameNotDrawing     = 0,
		gameNormalDraw     = 1,
		gameShadowDraw     = 2,
		gameReflectionDraw = 3,
		gameRefractionDraw = 4
	};

	struct PlayerTrafficInfo {
		PlayerTrafficInfo() : total(0) {}
		int total;
		std::map<int, int> packets;
	};

public:
	void LoadGame(const std::string& mapName, bool threaded);

	/// show GameEnd-window, calculate mouse movement etc.
	void GameEnd(const std::vector<unsigned char>& winningAllyTeams, bool timeout = false);

private:
	void LoadMap(const std::string& mapName);
	void LoadDefs();
	void PreLoadSimulation();
	void PostLoadSimulation();
	void PreLoadRendering();
	void PostLoadRendering();
	void LoadInterface();
	void LoadLua();
	void LoadFinalize();
	void PostLoad();

public:
	volatile bool IsFinishedLoading() const { return finishedLoading; }
	bool IsGameOver() const { return gameOver; }
	bool IsLagging(float maxLatency = 500.0f) const;

	const std::map<int, PlayerTrafficInfo>& GetPlayerTraffic() const {
		return playerTraffic;
	}
	void AddTraffic(int playerID, int packetCode, int length);

	/// Send a message to other players (allows prefixed messages with e.g. "a:...")
	void SendNetChat(std::string message, int destination = -1);

	bool ProcessCommandText(unsigned int key, const std::string& command);
	bool ProcessKeyPressAction(unsigned int key, const Action& action);
	bool ProcessAction(const Action& action, unsigned int key = -1, bool isRepeat = false);

	void SetHotBinding(const std::string& action) { hotBinding = action; }

	void SelectUnits(const std::string& line);
	void SelectCycle(const std::string& command);

	void ReloadCOB(const std::string& msg, int player);
	void ReloadCEGs(const std::string& tag);

	void StartSkip(int toFrame);
	void EndSkip();

	void ParseInputTextGeometry(const std::string& geo);

	void ReloadGame();
	void SaveGame(const std::string& filename, bool overwrite);

	void ResizeEvent();
	void SetupRenderingParams();

	void SetDrawMode(GameDrawMode mode) { gameDrawMode = mode; }
	GameDrawMode GetDrawMode() const { return gameDrawMode; }

private:
	bool Draw();
	bool DrawMT();

	static void DrawMTcb(void* c) { static_cast<CGame*>(c)->DrawMT(); }
	bool UpdateUnsynced(const spring_time currentTime);

	void DrawSkip(bool blackscreen = true);
	void DrawInputText();
	void UpdateUI(bool cam);

	/// Format and display a chat message received over network
	void HandleChatMsg(const ChatMessage& msg);

	/// Called when a key is released by the user
	int KeyReleased(int k);
	/// Called when the key is pressed by the user (can be called several times due to key repeat)
	int KeyPressed(int k, bool isRepeat);

	bool ActionPressed(unsigned int key, const Action& action, bool isRepeat);
	bool ActionReleased(const Action& action);
	/// synced actions (received from server) go in here
	void ActionReceived(const Action& action, int playerID);

	void ReColorTeams();

	unsigned int GetNumQueuedSimFrameMessages(unsigned int maxFrames) const;
	float GetNetMessageProcessingTimeLimit() const;

	void SendClientProcUsage();
	void ClientReadNet();
	void UpdateNumQueuedSimFrames();
	void UpdateNetMessageProcessingTimeLeft();
	void SimFrame();
	void StartPlaying();
	bool Update();

public:
	GameDrawMode gameDrawMode;

	unsigned char gameID[16];

	int lastSimFrame;
	int lastNumQueuedSimFrames;

	// number of Draw() calls per 1000ms
	unsigned int numDrawFrames;

	spring_time frameStartTime;
	spring_time lastSimFrameTime;
	spring_time lastDrawFrameTime;
	spring_time lastFrameTime;
	spring_time lastReadNetTime; ///< time of previous ClientReadNet() call
	spring_time lastNetPacketProcessTime;
	spring_time lastReceivedNetPacketTime;
	spring_time lastSimFrameNetPacketTime;

	float updateDeltaSeconds;
	/// Time in seconds, stops at game end
	float totalGameTime;

	int chatSound;

	bool windowedEdgeMove;
	bool fullscreenEdgeMove;

	bool hideInterface;
	bool showFPS;
	bool showClock;
	bool showSpeed;
	int showMTInfo;

	float inputTextPosX;
	float inputTextPosY;
	float inputTextSizeX;
	float inputTextSizeY;

	bool skipping;
	bool playing;
	bool chatting;

	/// Prevents spectator msgs from being seen by players
	bool noSpectatorChat;

	std::string hotBinding;
	std::string userInputPrefix;

	/// <playerID, <packetCode, total bytes> >
	std::map<int, PlayerTrafficInfo> playerTraffic;

	// to smooth out SimFrame calls
	float msgProcTimeLeft;  ///< How many SimFrame() calls we still may do.
	float consumeSpeedMult; ///< How fast we should eat NETMSG_NEWFRAMEs.

	int skipStartFrame;
	int skipEndFrame;
	int skipTotalFrames;
	float skipSeconds;
	bool skipSoundmute;
	float skipOldSpeed;
	float skipOldUserSpeed;
	spring_time skipLastDrawTime;

	/**
	 * @see CGameServer#speedControl
	 */
	int speedControl;

	CInfoConsole* infoConsole;
	CConsoleHistory* consoleHistory;

private:
	CWorldDrawer* worldDrawer;

	LuaParser* defsParser;

	/// for reloading the savefile
	ILoadSaveHandler* saveFile;

	volatile bool finishedLoading;
	bool gameOver;
};


extern CGame* game;


#endif // _GAME_H
