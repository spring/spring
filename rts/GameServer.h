#ifndef __GAME_SERVER_H__
#define __GAME_SERVER_H__

#include "archdef.h"

#include <winsock2.h>
#include <windows.h>

#include <ctime>
#include <string>

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

	LARGE_INTEGER lastframe;
	LARGE_INTEGER timeSpeed;
	float timeLeft;

	unsigned int serverframenum;
	void StartGame(void);

	bool gameEndDetected;
	float gameEndTime;					//how long has gone by since the game end was detected

	double lastSyncRequest;
	int outstandingSyncFrame;
	int syncResponses[MAX_PLAYERS];
	unsigned int exeChecksum;

	HANDLE gameServerMutex;
	HANDLE thisThread;

	bool quitServer;
	bool gameClientUpdated;			//used to prevent the server part to update to fast when the client is mega slow (running some sort of debug mode)
	float maxTimeLeft;
	void SendSystemMsg(const char* fmt,...);
};

extern CGameServer* gameServer;

#endif // __GAME_SERVER_H__
