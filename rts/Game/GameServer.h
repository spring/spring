#ifndef __GAME_SERVER_H__
#define __GAME_SERVER_H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <map>
#include <deque>

#include "System/GlobalStuff.h"
#include "System/UnsyncedRNG.h"
#include "SFloat3.h"
#include "Server/ServerLogHandler.h"
#include "Server/ServerLog.h"

class CBaseNetProtocol;
class CDemoReader;
class AutohostInterface;
class CGameSetupData;

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
};

/**
@brief Server class for game handling
This class represents a gameserver. It is responsible for recieving, checking and forwarding gamedata to the clients. It keeps track of the sync, cpu and other stats and informs all clients about events.
*/
class CGameServer : private ServerLog
{
	friend class CLoadSaveHandler;     //For initialize server state after load
public:
	CGameServer(int port, const std::string& mapName, const std::string& modName, const std::string& scriptName, const CGameSetupData* const setup, const std::string& demoName = "");
	~CGameServer();

	void AddLocalClient(unsigned wantedNumber);

	void AddAutohostInterface(const int usePort, const int remotePort);

	/**
	@brief Set frame after loading
	WARNING! No checks are done, so be carefull
	*/
	void PostLoad(unsigned lastTick, int serverframenum);

	/**
	@brief skip frames
	@todo skipping is buggy and could need some improvements
	Currently only sets serverframenum
	*/
	void SkipTo(int targetframe);

	void CreateNewFrame(bool fromServerThread, bool fixedFrameTime);

	bool WaitsOnCon() const;

	void SetGamePausable(const bool arg);
	
	/// Is the server still running?
	bool HasFinished() const;

#ifdef DEBUG
	bool gameClientUpdated;			//used to prevent the server part to update to fast when the client is mega slow (running some sort of debug mode)
#endif
	
	ServerLogHandler log; //TODO make private and add public interface

private:
	/**
	@brief catch commands from chat messages and handle them
	Insert chat messages here. If it contains a command (e.g. .nopause) usefull for the server it get filtered out, otherwise it will forwarded to all clients.
	@param msg The whole message
	@param player The playernumber which sent the message
	*/
	void GotChatMessage(const std::string& msg, unsigned player);

	/**
	@brief kick the specified player from the battle
	*/
	void KickPlayer(const int playerNum);

	void BindConnection(unsigned wantedNumber, bool grantRights=false);

	void CheckForGameStart(bool forced=false);
	void StartGame();
	void UpdateLoop();
	void Update();
	void CheckSync();
	void ServerReadNet();
	void CheckForGameEnd();

	void GenerateAndSendGameID();
	void SetBoolArg(bool& value, const std::string& str, const char* cmd);
	std::string GetPlayerNames(const std::vector<int>& indices) const;
	
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
	/// Wheter the game is pausable for others than the host
	bool gamePausable;

	/// The maximum speed users are allowed to set
	float maxUserSpeed;

	/// The minimum speed users are allowed to set (actual speed can be lower due to high cpu usage)
	float minUserSpeed;

	bool noHelperAIs;
	std::string scriptName;
	unsigned int mapChecksum;
	std::string mapName;
	unsigned int modChecksum;
	std::string modName;

	/////////////////// sync stuff ///////////////////
#ifdef SYNCCHECK
	std::deque<int> outstandingSyncFrames;
#endif
	int syncErrorFrame;
	int syncWarningFrame;
	int delayedSyncResponseFrame;

	///////////////// internal stuff //////////////////
	CBaseNetProtocol* serverNet;
	CDemoReader* demoReader;
	AutohostInterface* hostif;
	UnsyncedRNG rng;
	boost::thread* thread;
	mutable boost::mutex gameServerMutex;
};

extern CGameServer* gameServer;

#endif // __GAME_SERVER_H__
