/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_SERVER_H
#define _GAME_SERVER_H

#include <boost/scoped_ptr.hpp>
#include <string>
#include <map>
#include <deque>
#include <set>
#include <vector>
#include <list>

#include "GameData.h"
#include "Sim/Misc/TeamBase.h"
#include "System/UnsyncedRNG.h"
#include "System/float3.h"
#include "System/Misc/SpringTime.h"
#include "System/Platform/Synchro.h"


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
class CGameSetup;
class ClientSetup;
class ChatMessage;
class GameParticipant;
class GameSkirmishAI;

/**
 * When the Server generates a message,
 * this value is used as the sending player-number.
 */
const unsigned SERVER_PLAYER = 255;
const unsigned numCommands = 22;
extern const std::string commands[numCommands];

class GameTeam : public TeamBase
{
public:
	GameTeam() : active(false) {}
	bool active;
	GameTeam& operator=(const TeamBase& base) { TeamBase::operator=(base); return *this; }
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
	CGameServer(const std::string& hostIP, int hostPort, const GameData* const gameData, const CGameSetup* const setup);
	~CGameServer();

	void AddLocalClient(const std::string& myName, const std::string& myVersion);

	void AddAutohostInterface(const std::string& autohostIP, const int autohostPort);

	/**
	 * @brief Set frame after loading
	 * WARNING! No checks are done, so be carefull
	 */
	void PostLoad(int serverFrameNum);

	void CreateNewFrame(bool fromServerThread, bool fixedFrameTime);

	bool WaitsOnCon() const;

	void SetGamePausable(const bool arg);

	bool HasStarted() const { return gameHasStarted; }
	bool HasGameID() const { return generatedGameID; }
	/// Is the server still running?
	bool HasFinished() const;

	void UpdateSpeedControl(int speedCtrl);
	static std::string SpeedControlToString(int speedCtrl);

	const boost::scoped_ptr<CDemoReader>& GetDemoReader() const { return demoReader; }
	#ifdef DEDICATED
	const boost::scoped_ptr<CDemoRecorder>& GetDemoRecorder() const { return demoRecorder; }
	#endif

private:
	/**
	 * @brief relay chat messages to players / autohost
	 */
	void GotChatMessage(const ChatMessage& msg);

	/// Execute textual messages received from clients
	void PushAction(const Action& action);

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
	void MutePlayer(const int playerNum, bool muteChat, bool muteDraw );

	void ResignPlayer(const int playerNum);

	unsigned BindConnection(std::string name, const std::string& passwd, const std::string& version, bool isLocal, boost::shared_ptr<netcode::CConnection> link, bool reconnect = false, int netloss = 0);

	void CheckForGameStart(bool forced=false);
	void StartGame();
	void UpdateLoop();
	void Update();
	void ProcessPacket(const unsigned playerNum, boost::shared_ptr<const netcode::RawPacket> packet);
	void CheckSync();
	void ServerReadNet();

	void LagProtection();

	/** @brief Generate a unique game identifier and send it to all clients. */
	void GenerateAndSendGameID();
	std::string GetPlayerNames(const std::vector<int>& indices) const;

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

	bool AdjustPlayerNumber(netcode::RawPacket* buf, int pos, int val = -1);
	void UpdatePlayerNumberMap();

	float GetDemoTime() const;

private:
	/////////////////// game status variables ///////////////////

	unsigned char playerNumberMap[256];
	volatile bool quitServer;
	int serverFrameNum;

	spring_time serverStartTime;
	spring_time readyTime;
	spring_time gameStartTime;
	spring_time gameEndTime;	///< Tick when game end was detected
	spring_time lastTick;
	float timeLeft;
	spring_time lastPlayerInfo;
	spring_time lastUpdate;
	float modGameTime;
	float gameTime;
	float startTime;

	bool isPaused;
	float userSpeedFactor;
	float internalSpeed;
	bool cheating;

	unsigned char ReserveNextAvailableSkirmishAIId();

	std::map<unsigned char, GameSkirmishAI> ais;
	std::list<unsigned char> usedSkirmishAIIds;
	void FreeSkirmishAIId(const unsigned char skirmishAIId);

	std::vector<GameParticipant> players;
	std::vector<GameTeam> teams;
	std::vector<unsigned char> winningAllyTeams;
	//! bit0: chat mute, bit1: drawing mute
	std::vector<int> mutedPlayers;

	float medianCpu;
	int medianPing;
	int curSpeedCtrl;

	/**
	 * throttles speed based on:
	 * 0 : players (max cpu)
	 * 1 : players (median cpu)
	 * 2 : (same as 0)
	 * -x: same as x, but ignores votes from players that may change
	 *     the speed-control mode
	 */
	int speedControl;
	/////////////////// game settings ///////////////////
	boost::scoped_ptr<const CGameSetup> setup;
	boost::scoped_ptr<const GameData> gameData;
	/// Wheter the game is pausable for others than the host
	bool gamePausable;

	/// The maximum speed users are allowed to set
	float maxUserSpeed;

	/// The minimum speed users are allowed to set (actual speed can be lower due to high cpu usage)
	float minUserSpeed;

	bool noHelperAIs;
	bool canReconnect;
	bool allowSpecDraw;
	bool bypassScriptPasswordCheck;
	bool whiteListAdditionalPlayers;
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

	void AddAdditionalUser( const std::string& name, const std::string& passwd, bool fromDemo = false, bool spectator = true, int team = 0);

	bool hasLocalClient;
	unsigned localClientNumber;

	/// If the server receives a command, it will forward it to clients if it is not in this set
	std::set<std::string> commandBlacklist;

	boost::scoped_ptr<netcode::UDPListener> UDPNet;
	boost::scoped_ptr<CDemoReader> demoReader;
#ifdef DEDICATED
	boost::scoped_ptr<CDemoRecorder> demoRecorder;
#endif
	boost::scoped_ptr<AutohostInterface> hostif;
	UnsyncedRNG rng;
	boost::thread* thread;

	mutable Threading::RecursiveMutex gameServerMutex;

	volatile bool gameHasStarted;
	volatile bool generatedGameID;

	spring_time lastBandwidthUpdate;
	int linkMinPacketSize;

	union {
		unsigned char charArray[16];
		unsigned int intArray[4];
	} gameID;
};

extern CGameServer* gameServer;

#endif // _GAME_SERVER_H
