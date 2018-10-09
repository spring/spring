/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_SERVER_H
#define _GAME_SERVER_H

// #include <asio/ip/udp.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <array>
#include <deque>
#include <map>
#include <set>
#include <vector>

#include "Game/GameData.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamBase.h"
#include "System/float3.h"
#include "System/GlobalRNG.h"
#include "System/Misc/SpringTime.h"
#include "System/Threading/SpringThreading.h"

/**
 * "player" number for GameServer-generated messages
 */
#define SERVER_PLAYER 255

namespace netcode
{
	class RawPacket;
	class CConnection;
	class UDPListener;
}
class CDemoReader;
class Action;
class CDemoRecorder;
class AutohostInterface;
class ClientSetup;
class CGameSetup;
class ChatMessage;
class GameParticipant;
class GameSkirmishAI;

class GameTeam : public TeamBase
{
public:
	GameTeam() : active(false) {}
	GameTeam& operator=(const TeamBase& base) { TeamBase::operator=(base); return *this; }

	void SetActive(bool b) { active = b; }
	bool IsActive() const { return active; }

private:
	bool active;
};

/**
 * @brief Server class for game handling
 * This class represents a gameserver. It is responsible for recieving,
 * checking and forwarding gamedata to the clients. It keeps track of the sync,
 * cpu and other stats and informs all clients about events.
 */
class CGameServer
{
	friend class CCregLoadSaveHandler; // For initializing server state after load
public:
	CGameServer(
		const std::shared_ptr<const ClientSetup> newClientSetup,
		const std::shared_ptr<const    GameData> newGameData,
		const std::shared_ptr<const  CGameSetup> newGameSetup
	);

	CGameServer(const CGameServer&) = delete; // no-copy
	~CGameServer();

	static void Reload(const std::shared_ptr<const CGameSetup> newGameSetup);

	void AddLocalClient(const std::string& myName, const std::string& myVersion, const std::string& myPlatform);
	void AddAutohostInterface(const std::string& autohostIP, const int autohostPort);

	void Initialize();
	/**
	 * @brief Set frame after loading
	 * WARNING! No checks are done, so be carefull
	 */
	void PostLoad(int serverFrameNum);

	void CreateNewFrame(bool fromServerThread, bool fixedFrameTime);

	void SetGamePausable(const bool arg);
	void SetReloading(const bool arg) { reloadingServer = arg; }

	bool PreSimFrame() const { return (serverFrameNum == -1); }
	bool HasStarted() const { return gameHasStarted; }
	bool HasGameID() const { return generatedGameID; }
	bool HasLocalClient() const { return (localClientNumber != -1u); }
	/// Is the server still running?
	bool HasFinished() const;

	void UpdateSpeedControl(int speedCtrl);
	static std::string SpeedControlToString(int speedCtrl);

	static bool IsServerCommand(const std::string& cmd) {
		const auto pred = [](const std::string& a, const std::string& b) { return (a < b); };
		const auto iter = std::lower_bound(commandBlacklist.begin(), commandBlacklist.end(), cmd, pred);
		return (iter != commandBlacklist.end() && *iter == cmd);
	}

	std::string GetPlayerNames(const std::vector<int>& indices) const;

	const std::shared_ptr<const ClientSetup> GetClientSetup() const { return myClientSetup; }
	const std::shared_ptr<const    GameData> GetGameData() const { return myGameData; }
	const std::shared_ptr<const  CGameSetup> GetGameSetup() const { return myGameSetup; }

	const std::unique_ptr<CDemoReader>& GetDemoReader() const { return demoReader; }
	const std::unique_ptr<CDemoRecorder>& GetDemoRecorder() const { return demoRecorder; }

private:
	/**
	 * @brief relay chat messages to players / autohost
	 */
	void GotChatMessage(const ChatMessage& msg);

	/// Execute textual messages received from clients
	void PushAction(const Action& action, bool fromAutoHost);

	void StripGameSetupText(const GameData* const newGameData);

	/**
	 * @brief kick the specified player from the battle
	 */
	void KickPlayer(const int playerNum);
	/**
	 * @brief force the specified player to spectate
	 */
	void SpecPlayer(const int playerNum);
	/**
	 * @brief drops chat or drawin messages for given playerNum
	 */
	void MutePlayer(const int playerNum, bool muteChat, bool muteDraw);
	void ResignPlayer(const int playerNum);

	bool CheckPlayerPassword(const int playerNum, const std::string& pw) const;

	unsigned BindConnection(
		std::shared_ptr<netcode::CConnection> clientLink,
		std::string clientName,
		const std::string& clientPassword,
		const std::string& clientVersion,
		const std::string& clientPlatform,
		bool isLocal,
		bool reconnect = false,
		int netloss = 0
	);

	void CheckForGameStart(bool forced = false);
	void StartGame(bool forced);
	void UpdateLoop();
	void Update();
	void ProcessPacket(const unsigned playerNum, std::shared_ptr<const netcode::RawPacket> packet);
	void CheckSync();
	void HandleConnectionAttempts();
	void ServerReadNet();

	void LagProtection();

	/** @brief Generate a unique game identifier and send it to all clients. */
	void GenerateAndSendGameID();

	void WriteDemoData();
	/// read data from demo and send it to clients
	bool SendDemoData(int targetFrameNum);

	void Broadcast(std::shared_ptr<const netcode::RawPacket> packet);

	/**
	 * @brief skip frames
	 *
	 * If you are watching a demo, this will push out all data until
	 * targetFrame to all clients
	 */
	void SkipTo(int targetFrameNum);

	void Message(const std::string& message, bool broadcast = true, bool internal = false);
	void PrivateMessage(int playerNum, const std::string& message);

	float GetDemoTime() const;

private:
	/////////////////// game settings ///////////////////
	std::shared_ptr<const ClientSetup> myClientSetup;
	std::shared_ptr<const    GameData> myGameData;
	std::shared_ptr<const  CGameSetup> myGameSetup;

	/////////////////// game status variables ///////////////////
	int serverFrameNum;

	spring_time serverStartTime;
	spring_time readyTime;
	spring_time gameStartTime;
	spring_time gameEndTime;	///< Tick when game end was detected
	spring_time lastNewFrameTick;
	spring_time lastPlayerInfo;
	spring_time lastUpdate;
	spring_time lastBandwidthUpdate;

	float modGameTime;
	float gameTime;
	float startTime;
	float frameTimeLeft;

	bool isPaused;
	/// whether the game is pausable for others than the host
	bool gamePausable;

	float userSpeedFactor;
	float internalSpeed;

	std::vector< std::pair<bool, GameSkirmishAI> > skirmishAIs;
	std::vector<uint8_t> freeSkirmishAIs;

	std::vector<GameParticipant> players;
	std::vector<GameTeam> teams;
	std::vector<unsigned char> winningAllyTeams;

	std::array<spring_time, MAX_PLAYERS> pingTimeFilter;

	std::array< std::pair<spring_time, uint32_t>, MAX_PLAYERS> clientDrawFilter;
	std::array< std::pair<       bool,     bool>, MAX_PLAYERS> clientMuteFilter;

	// std::map<asio::ip::udp::endpoint, int> rejectedConnections;
	std::map<std::string, int> rejectedConnections;

	std::pair<std::string, std::string> refClientVersion;

	float medianCpu;
	int medianPing;
	int curSpeedCtrl;
	int loopSleepTime;

	/// The maximum speed users are allowed to set
	float maxUserSpeed;
	/// The minimum speed users are allowed to set (actual speed can be lower due to high cpu usage)
	float minUserSpeed;

	bool cheating;
	bool noHelperAIs;
	bool canReconnect;
	bool allowSpecDraw;
	bool allowSpecJoin;
	bool whiteListAdditionalPlayers;

	bool logInfoMessages;
	bool logDebugMessages;

	std::deque< std::shared_ptr<const netcode::RawPacket> > packetCache;

	/////////////////// sync stuff ///////////////////
#ifdef SYNCCHECK
	std::set<int> outstandingSyncFrames;
#endif
	int syncErrorFrame;
	int syncWarningFrame;

	///////////////// internal stuff //////////////////
	void InternalSpeedChange(float newSpeed);
	void UserSpeedChange(float newSpeed, int player);

	void AddAdditionalUser( const std::string& name, const std::string& passwd, bool fromDemo = false, bool spectator = true, int team = 0, int playerNum = -1);

	uint8_t ReserveSkirmishAIId();

	unsigned localClientNumber;

	/// If the server receives a command, it will forward it to clients if it is not in this set
	static std::array<std::string, 23> commandBlacklist;

	std::unique_ptr<netcode::UDPListener> UDPNet;
	std::unique_ptr<CDemoReader> demoReader;
	std::unique_ptr<CDemoRecorder> demoRecorder;
	std::unique_ptr<AutohostInterface> hostif;

	CGlobalUnsyncedRNG rng;
	spring::thread thread;

	mutable spring::recursive_mutex gameServerMutex;

	std::atomic<bool> gameHasStarted;
	std::atomic<bool> generatedGameID;
	std::atomic<bool> reloadingServer;
	std::atomic<bool> quitServer;

	int linkMinPacketSize;

	union {
		unsigned char charArray[16];
		unsigned int intArray[4];
	} gameID;
};

extern CGameServer* gameServer;

#endif // _GAME_SERVER_H
