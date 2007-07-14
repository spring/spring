#ifndef __GAME_SERVER_H__
#define __GAME_SERVER_H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include "SDL_types.h"

#include <ctime>
#include <string>

#include <Sim/Units/UnitHandler.h> // for CChecksum (should be moved somewhere else tho)
#include <System/NetProtocol.h>

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
	/**
	@brief kick the specified player from the battle
	@todo in order to build a dedicated server, there should be some kind of rights system which allow some people to kick players or force a start, but currently only the host is able to do this
	*/
	void KickPlayer(const int playerNum);

	bool makeMemDump;
	unsigned char inbuf[netcode::NETWORK_BUFFER_SIZE];

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
	int syncErrorFrame;
	int syncWarningFrame;
	int delayedSyncResponseFrame;

private:
	CNetProtocol* serverNet;
};

extern CGameServer* gameServer;

#endif // __GAME_SERVER_H__
