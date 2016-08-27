/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_H
#define _GAME_H

#include <string>
#include <map>
#include <vector>

#include "GameController.h"
#include "Game/UI/KeySet.h"
#include "System/creg/creg_cond.h"
#include "System/Misc/SpringTime.h"

class JobDispatcher;
class CConsoleHistory;
class CInfoConsole;
class LuaParser;
class ILoadSaveHandler;
class Action;
class ChatMessage;
class CWorldDrawer;


class CGame : public CGameController
{
private:
	CR_DECLARE_STRUCT(CGame)

public:
	CGame(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile);
	virtual ~CGame();
	void KillLua();

public:
	enum GameDrawMode {
		gameNotDrawing     = 0,
		gameNormalDraw     = 1,
		gameShadowDraw     = 2,
		gameReflectionDraw = 3,
		gameRefractionDraw = 4,
		gameDeferredDraw   = 5,
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
	void AddTimedJobs();

	void LoadMap(const std::string& mapName);
	void LoadDefs();
	void PreLoadSimulation();
	void PostLoadSimulation();
	void PreLoadRendering();
	void PostLoadRendering();
	void LoadInterface();
	void LoadLua();
	void LoadSkirmishAIs();
	void LoadFinalize();
	void PostLoad();

	void KillMisc();
	void KillRendering();
	void KillInterface();
	void KillSimulation();

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

	void ReloadCOB(const std::string& msg, int player);
	void ReloadCEGs(const std::string& tag);

	void StartSkip(int toFrame);
	void EndSkip();

	void ParseInputTextGeometry(const std::string& geo);

	void ReloadGame();
	void SaveGame(const std::string& filename, bool overwrite, bool usecreg);

	void ResizeEvent();

	void SetDrawMode(GameDrawMode mode) { gameDrawMode = mode; }
	GameDrawMode GetDrawMode() const { return gameDrawMode; }

private:
	bool Draw();
	bool UpdateUnsynced(const spring_time currentTime);

	void DrawSkip(bool blackscreen = true);
	void DrawInputReceivers();
	void DrawInputText();
	void DrawInterfaceWidgets();

	/// Format and display a chat message received over network
	void HandleChatMsg(const ChatMessage& msg);

	/// Called when a key is released by the user
	int KeyReleased(int k);
	/// Called when the key is pressed by the user (can be called several times due to key repeat)
	int KeyPressed(int k, bool isRepeat);
	///
	int TextInput(const std::string& utf8Text);

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

	float inputTextPosX;
	float inputTextPosY;
	float inputTextSizeX;
	float inputTextSizeY;

	bool skipping;
	bool playing;
	bool chatting;

	/// Prevents spectator msgs from being seen by players
	bool noSpectatorChat;

	CTimedKeyChain curKeyChain;
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
	JobDispatcher* jobDispatcher;

	CWorldDrawer* worldDrawer;

	LuaParser* defsParser;

	/// for reloading the savefile
	ILoadSaveHandler* saveFile;

	volatile bool finishedLoading;
	bool gameOver;
};


extern CGame* game;


#endif // _GAME_H
