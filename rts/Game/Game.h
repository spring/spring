/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_H
#define _GAME_H

#include <atomic>
#include <string>
#include <vector>

#include "GameController.h"
#include "GameDrawMode.h"
#include "GameJobDispatcher.h"
#include "Game/UI/KeySet.h"
#include "Rendering/WorldDrawer.h"
#include "System/UnorderedMap.hpp"
#include "System/creg/creg_cond.h"
#include "System/Misc/SpringTime.h"

class LuaParser;
class ILoadSaveHandler;
class Action;
class ChatMessage;


class CGame : public CGameController
{
private:
	CR_DECLARE_STRUCT(CGame)

public:
	CGame(const std::string& mapFileName, const std::string& modName, ILoadSaveHandler* saveFile);
	virtual ~CGame();
	void KillLua(bool dtor);

public:
	struct PlayerTrafficInfo {
		int total = 0;

		spring::unordered_map<int, int> packets;
	};

public:
	void LoadGame(const std::string& mapName);

	/// show GameEnd-window, calculate mouse movement etc.
	void GameEnd(const std::vector<unsigned char>& winningAllyTeams, bool timeout = false);

private:
	void AddTimedJobs();

	void LoadMap(const std::string& mapName);
	void LoadDefs(LuaParser* defsParser);
	void PreLoadSimulation(LuaParser* defsParser);
	void PostLoadSimulation(LuaParser* defsParser);
	void PreLoadRendering();
	void PostLoadRendering();
	void LoadInterface();
	void LoadLua(bool onlySynced, bool onlyUnsynced);
	void LoadSkirmishAIs();
	void LoadFinalize();
	void PostLoad();

	void KillMisc();
	void KillRendering();
	void KillInterface();
	void KillSimulation();

public:
	bool IsFinishedLoading() const { return finishedLoading; }
	bool IsGameOver() const { return gameOver; }
	bool IsLagging(float maxLatency = 500.0f) const;

	const spring::unordered_map<int, PlayerTrafficInfo>& GetPlayerTraffic() const {
		return playerTraffic;
	}
	void AddTraffic(int playerID, int packetCode, int length);

	/// Send a message to other players (allows prefixed messages with e.g. "a:...")
	void SendNetChat(std::string message, int destination = -1);

	bool ProcessCommandText(unsigned int key, const std::string& command);
	bool ProcessAction(const Action& action, unsigned int key = -1, bool isRepeat = false);

	void ReloadCOB(const std::string& msg, int player);
	void ReloadCEGs(const std::string& tag);

	void StartSkip(int toFrame);
	void EndSkip();

	void ParseInputTextGeometry(const std::string& geo);

	void ReloadGame();
	void SaveGame(std::string&& fileName, std::string&& saveArgs);

	void ResizeEvent() override;

	void SetDrawMode(Game::DrawMode mode) { gameDrawMode = mode; }
	Game::DrawMode GetDrawMode() const { return gameDrawMode; }

private:
	bool Draw() override;
	bool Update() override;
	bool UpdateUnsynced(const spring_time currentTime);

	void DrawSkip(bool blackscreen = true);
	void DrawInputReceivers();
	void DrawInputText();
	void DrawInterfaceWidgets();

	/// Format and display a chat message received over network
	void HandleChatMsg(const ChatMessage& msg);

	/// Called when a key is released by the user
	int KeyReleased(int k) override;
	/// Called when the key is pressed by the user (can be called several times due to key repeat)
	int KeyPressed(int k, bool isRepeat) override;
	///
	int TextInput(const std::string& utf8Text) override;
	int TextEditing(const std::string& utf8Text, unsigned int start, unsigned int length) override;

	bool ActionPressed(unsigned int key, const Action& action, bool isRepeat);
	bool ActionReleased(const Action& action);
	/// synced actions (received from server) go in here
	void ActionReceived(const Action& action, int playerID);

	uint32_t GetNumQueuedSimFrameMessages(uint32_t maxFrames) const;
	float GetNetMessageProcessingTimeLimit() const;

	void SendClientProcUsage();
	void ClientReadNet();
	void UpdateNumQueuedSimFrames();
	void UpdateNetMessageProcessingTimeLeft();
	void SimFrame();
	void StartPlaying();

public:
	Game::DrawMode gameDrawMode = Game::NotDrawing;

	unsigned char gameID[16];

	int lastSimFrame = -1;
	int lastNumQueuedSimFrames = -1;

	// number of Draw() calls per 1000ms
	unsigned int numDrawFrames = 0;

	spring_time frameStartTime;
	spring_time lastSimFrameTime;
	spring_time lastDrawFrameTime;
	spring_time lastFrameTime;
	spring_time lastReadNetTime; ///< time of previous ClientReadNet() call
	spring_time lastNetPacketProcessTime;
	spring_time lastReceivedNetPacketTime;
	spring_time lastSimFrameNetPacketTime;
	spring_time lastUnsyncedUpdateTime;
	spring_time skipLastDrawTime;

	float updateDeltaSeconds = 0.0f;
	/// Time in seconds, stops at game end
	float totalGameTime = 0.0f;

	int chatSound = -1;

	bool windowedEdgeMove = false;
	bool fullscreenEdgeMove = false;

	bool hideInterface = false;
	bool showFPS = true;
	bool showClock = true;
	bool showSpeed = true;

	bool skipping = false;
	bool playing = false;
	bool paused = false; // unsynced

	/// Prevents spectator msgs from being seen by players
	bool noSpectatorChat = false;

	// to smooth out SimFrame calls
	float msgProcTimeLeft = 0.0f;  ///< How many SimFrame() calls we still may do.
	float consumeSpeedMult = 1.0f; ///< How fast we should eat NETMSG_NEWFRAMEs.


	#if 0
	int skipStartFrame = 0;
	int skipEndFrame = 0;
	int skipTotalFrames = 0;
	float skipSeconds = 0.0f;
	bool skipSoundmute = false;
	float skipOldSpeed = 0.0f;
	float skipOldUserSpeed = 0.0f;
	#endif


	/**
	 * @see CGameServer#speedControl
	 */
	int speedControl = -1;

	// 0 := 1/f rate, 1 := 30/s rate
	int luaGCControl = 0;

private:
	JobDispatcher jobDispatcher;

	CTimedKeyChain curKeyChain;

	CWorldDrawer worldDrawer;

	/// <playerID, <packetCode, total bytes> >
	spring::unordered_map<int, PlayerTrafficInfo> playerTraffic;

	/// for reloading the savefile
	ILoadSaveHandler* saveFileHandler;

	std::atomic<bool> finishedLoading = {false};
	std::atomic<bool> gameOver = {false};
};


extern CGame* game;

#endif // _GAME_H

