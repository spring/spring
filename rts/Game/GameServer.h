#ifndef __GAME_SERVER_H__
#define __GAME_SERVER_H__

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <map>
#include <deque>
#include <set>

#include "Console.h"
#include "GameData.h"
#include "System/GlobalStuff.h"
#include "System/UnsyncedRNG.h"
#include "SFloat3.h"

class CBaseNetProtocol;
class CDemoReader;
class AutohostInterface;
class CGameSetupData;
class ChatMessage;

const unsigned SERVER_PLAYER = 255; //server generated message which needs a playernumber

//TODO: move to seperate file
class GameParticipant
{
public:
	GameParticipant(bool willHaveRights);

	std::string name;
	bool readyToStart;
	float cpuUsage;
	int ping;
	
	unsigned team;

	bool hasRights;
#ifdef SYNCCHECK
	std::map<int, unsigned> syncResponse; // syncResponse[frameNum] = checksum
#endif
};

class GameTeam
{
public:
	SFloat3 startpos;
	bool readyToStart;
	int allyTeam;
};

/**
@brief Server class for game handling
This class represents a gameserver. It is responsible for recieving, checking and forwarding gamedata to the clients. It keeps track of the sync, cpu and other stats and informs all clients about events.
*/
class CGameServer : public CommandReceiver
{
	friend class CLoadSaveHandler;     //For initialize server state after load
public:
	CGameServer(int port, const GameData* const gameData, const CGameSetupData* const setup, const std::string& demoName = "");
	virtual ~CGameServer();

	void AddLocalClient();

	void AddAutohostInterface(const int remotePort);

	/**
	@brief Set frame after loading
	WARNING! No checks are done, so be carefull
	*/
	void PostLoad(unsigned lastTick, int serverframenum);

	void CreateNewFrame(bool fromServerThread, bool fixedFrameTime);

	bool WaitsOnCon() const;
	bool GameHasStarted() const;

	void SetGamePausable(const bool arg);

	virtual void PushAction(const Action& action);

	/// Is the server still running?
	bool HasFinished() const;

#ifdef DEBUG
	bool gameClientUpdated;			//used to prevent the server part to update to fast when the client is mega slow (running some sort of debug mode)
#endif
	
private:
	/**
	@brief catch commands from chat messages and handle them
	Insert chat messages here. If it contains a command (e.g. .nopause) usefull for the server it get filtered out, otherwise it will forwarded to all clients.
	@param msg The whole message
	@param player The playernumber which sent the message
	*/
	void GotChatMessage(const ChatMessage& msg);

	/**
	@brief kick the specified player from the battle
	*/
	void KickPlayer(const int playerNum);

	void BindConnection(unsigned wantedNumber);

	void CheckForGameStart(bool forced=false);
	void StartGame();
	void UpdateLoop();
	void Update();
	void CheckSync();
	void ServerReadNet();
	void CheckForGameEnd();

	void GenerateAndSendGameID();
	std::string GetPlayerNames(const std::vector<int>& indices) const;
	
	/// read data from demo and send it to clients
	void SendDemoData(const bool skipping=false);
	
	/**
	@brief skip frames
	
	If you are watching a demo, this will push out all data until targetframe to all clients
	*/
	void SkipTo(int targetframe);
	
	void Message(const std::string& message);
	void Warning(const std::string& message);

	/////////////////// game status variables ///////////////////

	volatile bool quitServer;
	int serverframenum;

	unsigned serverStartTime;
	unsigned readyTime;
	unsigned gameStartTime;
	unsigned gameEndTime;	//Tick when game end was detected
	bool sentGameOverMsg;
	unsigned lastTick;
	float timeLeft;
	unsigned lastPlayerInfo;
	unsigned lastUpdate;
	float modGameTime;

	bool IsPaused;
	float userSpeedFactor;
	float internalSpeed;
	bool cheating;

	boost::scoped_ptr<GameParticipant> players[MAX_PLAYERS];
	boost::scoped_ptr<GameTeam> teams[MAX_TEAMS];

	/////////////////// game settings ///////////////////
	const CGameSetupData* const setup;
	boost::scoped_ptr<const GameData> gameData;
	/// Wheter the game is pausable for others than the host
	bool gamePausable;

	/// The maximum speed users are allowed to set
	float maxUserSpeed;

	/// The minimum speed users are allowed to set (actual speed can be lower due to high cpu usage)
	float minUserSpeed;

	bool noHelperAIs;

	/////////////////// sync stuff ///////////////////
#ifdef SYNCCHECK
	std::deque<int> outstandingSyncFrames;
#endif
	int syncErrorFrame;
	int syncWarningFrame;
	int delayedSyncResponseFrame;

	///////////////// internal stuff //////////////////
	void RestrictedAction(const std::string& action);
	
	/// If the server recieves a command, it will forward it to clients if it is not in this set
	std::set<std::string> commandBlacklist;
	CBaseNetProtocol* serverNet;
	CDemoReader* demoReader;
	AutohostInterface* hostif;
	UnsyncedRNG rng;
	boost::thread* thread;
	mutable boost::recursive_mutex gameServerMutex;
};

extern CGameServer* gameServer;

#endif // __GAME_SERVER_H__
