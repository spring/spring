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
	CGameServer(void);
	~CGameServer(void);
	bool Update(void);
	bool ServerReadNet();
	void CheckForGameEnd(void);
	void CreateNewFrame(bool fromServerThread=false);
	void UpdateLoop(void);

	bool makeMemDump;
	unsigned char inbuf[40000];	//buffer space for incomming data	//should be NETWORK_BUFFER_SIZE but dont want to include net.h here
	unsigned char outbuf[40000];

	Uint64 lastframe;
	float timeLeft;

	unsigned int serverframenum;
	void StartGame(void);

	bool gameLoading;
	bool gameEndDetected;
	float gameEndTime;					//how long has gone by since the game end was detected

	double lastSyncRequest;
	int outstandingSyncFrame;
	CChecksum syncResponses[MAX_PLAYERS];
	unsigned int exeChecksum;

	mutable boost::mutex gameServerMutex;

	bool quitServer;
	bool gameClientUpdated;			//used to prevent the server part to update to fast when the client is mega slow (running some sort of debug mode)
	float maxTimeLeft;
	void SendSystemMsg(const char* fmt,...);
	bool terminate;
};

extern CGameServer* gameServer;

#endif // __GAME_SERVER_H__
