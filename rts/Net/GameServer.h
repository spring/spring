/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_SERVER_H
#define _GAME_SERVER_H

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>
#include <map>
#include <deque>
#include <set>
#include <vector>
#include <list>

#include "Game/GameData.h"
#include "Sim/Misc/TeamBase.h"
#include "System/UnsyncedRNG.h"
#include "System/float3.h"
#include "System/Misc/SpringTime.h"
#include "System/Platform/RecursiveScopedLock.h"

/**
 * "player" number for GameServer-generated messages
 */
#define SERVER_PLAYER 255



namespace boost {
	class thread;
}

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
		const boost::shared_ptr<const ClientSetup> newClientSetup,
		const boost::shared_ptr<const    GameData> newGameData,
		const boost::shared_ptr<const  CGameSetup> newGameSetup
	);

	CGameServer(const CGameServer&) = delete; // no-copy
	~CGameServer();

	static void Reload(const boost::shared_ptr<const CGameSetup> newGameSetup);

	void AddLocalClient(const std::string& myName, const std::string& myVersion);
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
	static const std::set<std::string>& GetCommandBlackList() { return commandBlacklist; }

	std::string GetPlayerNames(const std::vector<int>& indices) const;

	const boost::shared_ptr<const ClientSetup> GetClientSetup() const { return myClientSetup; }
	const boost::shared_ptr<const    GameData> GetGameData() const { return myGameData; }
	const boost::shared_ptr<const  CGameSetup> GetGameSetup() const { return myGameSetup; }

	const boost::scoped_ptr<CDemoReader>& GetDemoReader() const { return demoReader; }
	const boost::scoped_ptr<CDemoRecorder>& GetDemoRecorder() const { return demoRecorder; }

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

	bool CheckPlayersPassword(const int playerNum, const std::string& pw) const;

	unsigned BindConnection(std::string name, const std::string& passwd, const std::string& version, bool isLocal, boost::shared_ptr<netcode::CConnection> link, bool reconnect = false, int netloss = 0);

	void CheckForGameStart(bool forced = false);
	void StartGame(bool forced);
	void UpdateLoop();
	void Update();
	void ProcessPacket(const unsigned playerNum, boost::shared_ptr<const netcode::RawPacket> packet);
	void CheckSync();
	void HandleConnectionAttempts();
	void ServerReadNet();

	void LagProtection();

	/** @brief Generate a unique game identifier and send it to all clients. */
	void GenerateAndSendGameID();

	void WriteDemoData();
	/// read data from demo and send it to clients
	bool SendDemoData(int targetFrameNum);

	void Broadcast(boost::shared_ptr<const netcode::RawPacket> packet);

	/**
	 * @brief skip frames
	 *
	 * If you are watching a demo, this will push out all data until
	 * targetFrame to all clients
	 */
	void SkipTo(int targetFrameNum);

	void Message(const std::string& message, bool broadcast = true);
	void PrivateMessage(int playerNum, const std::string& message);

	void AddToPacketCache(boost::shared_ptr<const netcode::RawPacket>& pckt);

	float GetDemoTime() const;

private:
	/////////////////// game settings ///////////////////
	boost::shared_ptr<const ClientSetup> myClientSetup;
	boost::shared_ptr<const    GameData> myGameData;
	boost::shared_ptr<const  CGameSetup> myGameSetup;

	/////////////////// game status variables ///////////////////
	volatile bool quitServer;
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

	std::map<unsigned char, GameSkirmishAI> ais;
	std::list<unsigned char> usedSkirmishAIIds;

	std::vector<GameParticipant> players;
	std::vector<GameTeam> teams;
	std::vector<unsigned char> winningAllyTeams;

	std::vector<bool> mutedPlayersChat;
	std::vector<bool> mutedPlayersDraw;

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

	std::list< std::vector<boost::shared_ptr<const netcode::RawPacket> > > packetCache;

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
	unsigned char ReserveNextAvailableSkirmishAIId();
	void FreeSkirmishAIId(const unsigned char skirmishAIId);

	unsigned localClientNumber;

	/// If the server receives a command, it will forward it to clients if it is not in this set
	static std::set<std::string> commandBlacklist;

	boost::scoped_ptr<netcode::UDPListener> UDPNet;
	boost::scoped_ptr<CDemoReader> demoReader;
	boost::scoped_ptr<CDemoRecorder> demoRecorder;
	boost::scoped_ptr<AutohostInterface> hostif;

	UnsyncedRNG rng;
	boost::thread* thread;

	mutable Threading::RecursiveMutex gameServerMutex;

	volatile bool gameHasStarted;
	volatile bool generatedGameID;
	volatile bool reloadingServer;

	int linkMinPacketSize;

	union {
		unsigned char charArray[16];
		unsigned int intArray[4];
	} gameID;
};

extern CGameServer* gameServer;

#endif // _GAME_SERVER_H
