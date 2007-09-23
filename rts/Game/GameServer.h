#ifndef __GAME_SERVER_H__
#define __GAME_SERVER_H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include "SDL_types.h"
#include <ctime>
#include <string>

#include "System/GlobalStuff.h"
#include "FileSystem/CRC.h"

class CNetProtocol;
class AutohostInterface;

/**
@brief Server class for game handling
This class represents a gameserver. It is responsible for recieving, checking and forwarding gamedata to the clients. It keeps track of the sync, cpu and other stats and informs all clients about events.
@TODO Make this class work without CGame to make dedicated hosting possible
*/
class CGameServer
{
public:
	CGameServer(int port, const std::string& mapName, const std::string& modName, const std::string& scriptName, const std::string& demoName="");
	~CGameServer();
	void CheckSync();
	bool Update();
	bool ServerReadNet();
	void CheckForGameEnd();
	void CreateNewFrame(bool fromServerThread=false);
	void UpdateLoop();

	bool WaitsOnCon() const;
	
	void PlayerDefeated(const int playerNum) const;
	
	void SetGamePausable(const bool arg);

// 	bool makeMemDump;

	unsigned lastTick;
	float timeLeft;

	int serverframenum;
	void StartGame();

	bool gameLoading;
	bool gameEndDetected;
	float gameEndTime;					//how long has gone by since the game end was detected

	float lastPlayerInfo;

	mutable boost::mutex gameServerMutex;
	boost::thread* thread;

	bool quitServer;
	bool gameClientUpdated;			//used to prevent the server part to update to fast when the client is mega slow (running some sort of debug mode)
	float maxTimeLeft;
	void SendSystemMsg(const char* fmt,...);
#ifdef SYNCDEBUG
	volatile bool fakeDesync; // set in client on .fakedesync, read and reset in server
#endif
#ifdef SYNCCHECK
	std::deque<int> outstandingSyncFrames;
	std::map<int, unsigned> syncResponse[MAX_PLAYERS]; // syncResponse[player][frameNum] = checksum
#endif
	
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
	
	int syncErrorFrame;
	int syncWarningFrame;
	int delayedSyncResponseFrame;

	/// Class for network communication
	CNetProtocol* serverNet;
	
	/// Inform 3. party programms about events
	AutohostInterface* hostif;
	CRC entropy;

	void GenerateAndSendGameID();
	void SetBoolArg(bool& value, const std::string& str, const char* cmd);
	
	// game settings
	/// Wheter the game is pausable for others than the host
	bool gamePausable;
	
	/// The maximum speed users are allowed to set
	float maxUserSpeed;
	/// The minimum speed users are allowed to set (actual speed can be lower due to high cpu usage)
	float minUserSpeed;
};

extern CGameServer* gameServer;

#endif // __GAME_SERVER_H__
