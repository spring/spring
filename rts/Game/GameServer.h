#ifndef __GAME_SERVER_H__
#define __GAME_SERVER_H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include "SDL_types.h"

#include <ctime>
#include <string>

#include <Sim/Units/UnitHandler.h> // for CChecksum (should be moved somewhere else tho)

class CGameServer
{
public:
	CGameServer();
	~CGameServer();
	bool Update();
	bool ServerReadNet();
	void CheckForGameEnd();
	void CreateNewFrame(bool fromServerThread=false);
	void UpdateLoop();

	bool makeMemDump;
	unsigned char inbuf[40000];	//buffer space for incomming data	//should be NETWORK_BUFFER_SIZE but dont want to include net.h here
	unsigned char outbuf[40000];

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
	int syncErrorFrame;
#endif
};

extern CGameServer* gameServer;

#endif // __GAME_SERVER_H__
