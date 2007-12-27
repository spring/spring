#ifndef __GAME_SERVER_H__
#define __GAME_SERVER_H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <map>
#include <deque>

#include "System/GlobalStuff.h"

class CBaseNetProtocol;
class CDemoReader;
class AutohostInterface;

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

	bool hasRights;
#ifdef SYNCCHECK
	std::map<int, unsigned> syncResponse; // syncResponse[frameNum] = checksum
#endif
};

/**
@brief Server class for game handling
This class represents a gameserver. It is responsible for recieving, checking and forwarding gamedata to the clients. It keeps track of the sync, cpu and other stats and informs all clients about events.
*/
class CGameServer
{
	friend class CLoadSaveHandler;     //For initialize server state after load
public:
	CGameServer(int port, const std::string& mapName, const std::string& modName, const std::string& scriptName);
	~CGameServer();

	void AddLocalClient(unsigned wantedNumber);

	/**
	@brief Set frame after loading
	WARNING! No checks are done, so be carefull
	*/
	void PostLoad(unsigned lastTick, int serverframenum);
	
	/// Tell the server to play a demofile
	void StartDemoPlayback(const std::string& demoName);

	/**
	@brief skip frames
	@todo skipping is buggy and could need some improvements
	Currently only sets serverframenum
	*/
	void SkipTo(int targetframe);

	void CreateNewFrame(bool fromServerThread=false);

	bool WaitsOnCon() const;

	void PlayerDefeated(const int playerNum) const;

	void SetGamePausable(const bool arg);

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
	void GotChatMessage(const std::string& msg, unsigned player);

	void SendSystemMsg(const char* fmt,...);

	/**
	@brief kick the specified player from the battle
	*/
	void KickPlayer(const int playerNum);

	void BindConnection(unsigned wantedNumber, bool grantRights=false);

	void StartGame();
	void UpdateLoop();
	void Update();
	void CheckSync();
	void ServerReadNet();
	void CheckForGameEnd();

	/// Class for network communication
	CBaseNetProtocol* serverNet;

	CDemoReader* play;

	/// Inform 3. party programms about events
	AutohostInterface* hostif;

	void GenerateAndSendGameID();
	void SetBoolArg(bool& value, const std::string& str, const char* cmd);
	std::string GetPlayerNames(const std::vector<int>& indices) const;

	/////////////////// game status variables ///////////////////

	bool quitServer;
	int serverframenum;
	int nextserverframenum; //For loading game

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

	boost::scoped_ptr<GameParticipant> players[MAX_PLAYERS];

	/////////////////// game settings ///////////////////
	/// Wheter the game is pausable for others than the host
	bool gamePausable;

	/// The maximum speed users are allowed to set
	float maxUserSpeed;

	/// The minimum speed users are allowed to set (actual speed can be lower due to high cpu usage)
	float minUserSpeed;

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

	boost::thread* thread;
	mutable boost::mutex gameServerMutex;
};

extern CGameServer* gameServer;

#endif // __GAME_SERVER_H__
