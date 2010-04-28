#ifndef __GAME_SERVER_H__
#define __GAME_SERVER_H__

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <map>
#include <deque>
#include <set>
#include <vector>
#include <list>

#include "GameData.h"
#include "Sim/Misc/TeamBase.h"
#include "UnsyncedRNG.h"
#include "float3.h"
#include "System/myTime.h"

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
const unsigned numCommands = 19;
extern const std::string commands[numCommands];

class GameTeam : public TeamBase
{
public:
	GameTeam() : active(false) {};
	bool active;
	void operator=(const TeamBase& base) { TeamBase::operator=(base); };
};

/**
 * @brief Server class for game handling
 * This class represents a gameserver. It is responsible for recieving,
 * checking and forwarding gamedata to the clients. It keeps track of the sync,
 * cpu and other stats and informs all clients about events.
 */
class CGameServer
{
	friend class CLoadSaveHandler;     //For initialize server state after load
public:
	CGameServer(const ClientSetup* settings, bool onlyLocal, const GameData* const gameData, const CGameSetup* const setup);
	~CGameServer();

	void AddLocalClient(const std::string& myName, const std::string& myVersion);

	void AddAutohostInterface(const std::string& autohostip, const int remotePort);

	/**
	 * @brief Set frame after loading
	 * WARNING! No checks are done, so be carefull
	 */
	void PostLoad(unsigned lastTick, int serverframenum);

	void CreateNewFrame(bool fromServerThread, bool fixedFrameTime);

	bool WaitsOnCon() const;
	bool GameHasStarted() const;

	void SetGamePausable(const bool arg);

	bool HasDemo() const { return (demoReader != NULL); }
	/// Is the server still running?
	bool HasFinished() const;

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

	unsigned BindConnection(std::string name, const std::string& passwd, const std::string& version, bool isLocal, boost::shared_ptr<netcode::CConnection> link);

	void CheckForGameStart(bool forced=false);
	void StartGame();
	void UpdateLoop();
	void Update();
	void ProcessPacket(const unsigned playernum, boost::shared_ptr<const netcode::RawPacket> packet);
	void CheckSync();
	void ServerReadNet();
	void CheckForGameEnd();

	/** @brief Generate a unique game identifier and sent it to all clients. */
	void GenerateAndSendGameID();
	std::string GetPlayerNames(const std::vector<int>& indices) const;

	/// read data from demo and send it to clients
	void SendDemoData(const bool skipping=false);

	void Broadcast(boost::shared_ptr<const netcode::RawPacket> packet);

	/**
	 * @brief skip frames
	 *
	 * If you are watching a demo, this will push out all data until
	 * targetframe to all clients
	 */
	void SkipTo(int targetframe);

	void Message(const std::string& message, bool broadcast=true);
	void PrivateMessage(int playernum, const std::string& message);

	/////////////////// game status variables ///////////////////

	volatile bool quitServer;
	int serverframenum;

	spring_time serverStartTime;
	spring_time readyTime;
	spring_time gameStartTime;
	spring_time gameEndTime;	///< Tick when game end was detected
	bool sentGameOverMsg;
	spring_time lastTick;
	float timeLeft;
	spring_time lastPlayerInfo;
	spring_time lastUpdate;
	float modGameTime;

	bool isPaused;
	float userSpeedFactor;
	float internalSpeed;
	bool cheating;

	// Ugly hax for letting the script define initial team->isAI and team->leader for AI teams
	friend class CSkirmishAITestScript;
	std::vector<GameParticipant> players;
	size_t ReserveNextAvailableSkirmishAIId();
	
	std::map<size_t, GameSkirmishAI> ais;
	std::list<size_t> usedSkirmishAIIds;
	void FreeSkirmishAIId(const size_t skirmishAIId);
	
	std::vector<GameTeam> teams;

	float medianCpu;
	int medianPing;
	int enforceSpeed;
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
	bool allowSpecDraw;
	bool allowAdditionalPlayers;
	std::list< boost::shared_ptr<const netcode::RawPacket> > packetCache; //waaa, the overhead

	/////////////////// sync stuff ///////////////////
#ifdef SYNCCHECK
	std::deque<int> outstandingSyncFrames;
#endif
	int syncErrorFrame;
	int syncWarningFrame;

	///////////////// internal stuff //////////////////
	void InternalSpeedChange(float newSpeed);
	void UserSpeedChange(float newSpeed, int player);

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
	mutable boost::recursive_mutex gameServerMutex;
	typedef std::set<unsigned char> PlayersToForwardMsgvec;
	typedef std::map<unsigned char, PlayersToForwardMsgvec> MsgToForwardMap;
	MsgToForwardMap relayingMessagesMap;
};

extern CGameServer* gameServer;

#endif // __GAME_SERVER_H__
