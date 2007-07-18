#include "StdAfx.h"
#include "GameServer.h"
#include "NetProtocol.h"
#include "Player.h"
#include "Team.h"
#include "LogOutput.h"
#include "Game.h"
#include "GameSetup.h"
#include "StartScripts/ScriptHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "Platform/errorhandler.h"
#include <stdarg.h>
#include <boost/bind.hpp>
#include <SDL_timer.h>

#define SYNCCHECK_TIMEOUT 300 //frames
#define SYNCCHECK_MSG_TIMEOUT 400  // used to prevent msg spam

CGameServer* gameServer=0;

extern bool globalQuit;

static Uint32 GameServerThreadProc(void* lpParameter)
{
	((CGameServer*)lpParameter)->UpdateLoop();
	return 0;
}

CGameServer::CGameServer(int port, const std::string& mapName, const std::string& modName, const std::string& scriptName, const std::string& demoName)
{
	delayedSyncResponseFrame = 0;
	syncErrorFrame=0;
	syncWarningFrame=0;
	lastPlayerInfo = 0;
	makeMemDump=true;
	serverframenum=0;
	timeLeft=0;
	gameLoading=true;
	gameEndDetected=false;
	gameEndTime=0;
	quitServer=false;
	gameClientUpdated=false;
	maxTimeLeft=2;

	serverNet=SAFE_NEW CNetProtocol();

	if (!demoName.empty())
	{
		serverNet->InitServer(port,demoName);
	}
	else // no demo, so set map and mod to send it to clients
	{
		serverNet->InitServer(port);
		serverNet->SendMapName(archiveScanner->GetMapChecksum(mapName), mapName);
		std::string modArchive = archiveScanner->ModNameToModArchive(modName);
		serverNet->SendModName(archiveScanner->GetModChecksum(modArchive), modName);
		serverNet->SendScript(scriptName);
	}

	int myPlayer=0;
	if(gameSetup){
		myPlayer=gameSetup->myPlayer;
	}
	// create local client
	net->InitLocalClient(myPlayer);
	serverNet->ServerInitLocalClient(myPlayer);

	lastTick = SDL_GetTicks();

#ifndef SYNCIFY		//syncify doesnt really support multithreading...
	thread = SAFE_NEW boost::thread(boost::bind(GameServerThreadProc,this));
#endif
#ifdef SYNCDEBUG
	fakeDesync = false;
#endif

#ifdef STREFLOP_H
	// Something in CGameServer::CGameServer borks the FPU control word
	// maybe the threading, or something in CNet::InitServer() ??
	// Set single precision floating point math.
	streflop_init<streflop::Simple>();
#endif
}

CGameServer::~CGameServer()
{
	quitServer=true;
	thread->join();
	delete serverNet;
	serverNet=0;
}

static std::string GetPlayerNames(const std::vector<int>& indices)
{
	std::string players;
	std::vector<int>::const_iterator p = indices.begin();
	for (; p != indices.end(); ++p) {
		if (!players.empty())
			players += ", ";
		players += gs->players[*p]->playerName;
	}
	return players;
}

void CGameServer::CheckSync()
{
#ifdef SYNCCHECK
	// Check sync
	std::deque<int>::iterator f = outstandingSyncFrames.begin();
	while (f != outstandingSyncFrames.end()) {
		std::vector<int> noSyncResponse;
		//maps incorrect checksum to players with that checksum
		std::map<unsigned, std::vector<int> > desyncGroups;
		bool bComplete = true;
		bool bGotCorrectChecksum = false;
		unsigned correctChecksum = 0;
		for (int a = 0; a < MAX_PLAYERS; ++a) {
			if (!gs->players[a]->active)
				continue;
			std::map<int, unsigned>::iterator it = syncResponse[a].find(*f);
			if (it == syncResponse[a].end()) {
				if (*f >= serverframenum - SYNCCHECK_TIMEOUT)
					bComplete = false;
				else
					noSyncResponse.push_back(a);
			} else {
				if (!bGotCorrectChecksum) {
					bGotCorrectChecksum = true;
					correctChecksum = it->second;
				} else if (it->second != correctChecksum) {
					desyncGroups[it->second].push_back(a);
				}
			}
		}

		if (!noSyncResponse.empty()) {
			if (!syncWarningFrame || (*f - syncWarningFrame > SYNCCHECK_MSG_TIMEOUT)) {
				syncWarningFrame = *f;

				std::string players = GetPlayerNames(noSyncResponse);
				logOutput.Print("No response from %s for frame %d", players.c_str(), *f);
			}
		}

		// If anything's in it, we have a desync.
		// TODO take care of !bComplete case?
		// Should we start resync then immediately or wait for the missing packets (while paused)?
		if ( /*bComplete && */ !desyncGroups.empty()) {
			if (!syncErrorFrame || (*f - syncErrorFrame > SYNCCHECK_MSG_TIMEOUT)) {
				syncErrorFrame = *f;

				// TODO enable this when we have resync
				//serverNet->SendPause(gu->myPlayerNum, true);

				//For each group, output a message with list of playernames in it.
				// TODO this should be linked to the resync system so it can roundrobin
				// the resync checksum request packets to multiple clients in the same group.
				std::map<unsigned, std::vector<int> >::const_iterator g = desyncGroups.begin();
				for (; g != desyncGroups.end(); ++g) {
					std::string players = GetPlayerNames(g->second);
					SendSystemMsg("Sync error for %s in frame %d (0x%X)", players.c_str(), *f, g->first ^ correctChecksum);
				}
			}
		}

		// Remove complete sets (for which all player's checksums have been received).
		if (bComplete) {
// 			if (*f >= serverframenum - SYNCCHECK_TIMEOUT)
// 				logOutput.Print("Succesfully purged outstanding sync frame %d from the deque", *f);
			for (int a = 0; a < MAX_PLAYERS; ++a) {
				if (gs->players[a]->active)
					syncResponse[a].erase(*f);
			}
			f = outstandingSyncFrames.erase(f);
		} else
			++f;
	}
#else
	// Make it clear this build isn't suitable for release.
	if (!syncErrorFrame || (serverframenum - syncErrorFrame > SYNCCHECK_MSG_TIMEOUT)) {
		syncErrorFrame = serverframenum;
		SendSystemMsg("Warning: Sync checking disabled!");
	}
#endif
}

bool CGameServer::Update()
{
	if(serverNet->IsDemoServer())
		gameServer->serverframenum=gs->frameNum;

	if(lastPlayerInfo<gu->gameTime-2){
		lastPlayerInfo=gu->gameTime;

		if (game && game->playing) {
			int firstReal=0;
			if(gameSetup)
				firstReal=gameSetup->numDemoPlayers;
			//send info about the players
			for(int a=firstReal;a<gs->activePlayers;++a){
				if(gs->players[a]->active){
					serverNet->SendPlayerInfo(a, gs->players[a]->cpuUsage, gs->players[a]->ping);
				}
			}

			//decide new internal speed
			float maxCpu=0;
			for(int a=0;a<gs->activePlayers;++a){
				if(gs->players[a]->cpuUsage>maxCpu && gs->players[a]->active){
					maxCpu=gs->players[a]->cpuUsage;
				}
			}

			if (maxCpu != 0) {
				float wantedCpu=0.35f+(1-gs->speedFactor/gs->userSpeedFactor)*0.5f;
				//float speedMod=1+wantedCpu-maxCpu;
				float newSpeed=gs->speedFactor*wantedCpu/maxCpu;
				//logOutput.Print("Speed %f %f %f %f",maxCpu,wantedCpu,speedMod,newSpeed);
				newSpeed=(newSpeed+gs->speedFactor)*0.5f;
				if(newSpeed>gs->userSpeedFactor)
					newSpeed=gs->userSpeedFactor;
				if(newSpeed<0.1f)
					newSpeed=0.1f;
				if(newSpeed!=gs->speedFactor)
					serverNet->SendInternalSpeed(newSpeed);
			}
		}
	}

	if(!ServerReadNet()){
		logOutput.Print("Server read net wanted quit");
		return false;
	}
	if (game && game->playing && !serverNet->IsDemoServer()){
		CheckSync();

		// Send out new frame messages.
		unsigned currentTick = SDL_GetTicks();
		float timeElapsed=((float)(currentTick - lastTick))/1000.f;
		if (timeElapsed>0.2) {
			timeElapsed=0.2;
		}
		if(gameEndDetected)
			gameEndTime+=timeElapsed;
//		logOutput.Print("float value is %f",timeElapsed);

		if(gameClientUpdated){
			gameClientUpdated=false;
			maxTimeLeft=2;
		}
		if(timeElapsed>maxTimeLeft)
			timeElapsed=maxTimeLeft;
		maxTimeLeft-=timeElapsed;

		timeLeft+=GAME_SPEED*gs->speedFactor*timeElapsed;
		lastTick=currentTick;

		while((timeLeft>0) && (!gs->paused || game->bOneStep)){
			if(!game->creatingVideo){
				game->bOneStep=false;
				CreateNewFrame(true);
			}
			timeLeft--;
		}
	}
	serverNet->Update();

	CheckForGameEnd();
	return true;
}

bool CGameServer::ServerReadNet()
{
	static int lastMsg[MAX_PLAYERS],thisMsg=0;

	for(int a=0;a<gs->activePlayers;a++){
		int inbufpos=0;
		int inbuflength=0;
		if (gs->players[a]->active &&
				  ((!net->localDemoPlayback && (!gameSetup || a>=gameSetup->numDemoPlayers)) ||
				  (net->localDemoPlayback && a == (gameSetup ? gameSetup->myPlayer : 0) ))) {
			if((inbuflength=serverNet->GetData(inbuf,netcode::NETWORK_BUFFER_SIZE,a))==-1){
				PUSH_CODE_MODE;		//this could lead to some nasty errors if the other thread switches code mode...
				ENTER_MIXED;
				gs->players[a]->active=false;
				POP_CODE_MODE;
				inbuflength=0;
				serverNet->SendPlayerLeft(a, 0);
			}
		}

		// dont bother handling anything if we are just sending a demo to
		// localhost, this would result in double chat messages and server warnings..
		if (net->localDemoPlayback)
			continue;

		//		logOutput << serverNet->numConnected << "\n";

		while(inbufpos<inbuflength){
			thisMsg=inbuf[inbufpos];
			int lastLength=0;
			// TODO rearrange this in order of importance (most used to the front)
			switch (inbuf[inbufpos]){

			case NETMSG_HELLO:
				lastLength=1;
				break;

			case NETMSG_ATTEMPTCONNECT: //handled directly in CNet
				lastLength=3;
				break;

			case NETMSG_RANDSEED:
				lastLength=5;
				serverNet->SendRandSeed(inbuf[inbufpos+1]); //forward data
				break;

			case NETMSG_NEWFRAME:
				gs->players[a]->ping=serverframenum-*(int*)&inbuf[inbufpos+1];
				lastLength=5;
				break;

			case NETMSG_PAUSE:
				if(inbuf[inbufpos+1]!=a){
					logOutput.Print("Server: Warning got pause msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if (!inbuf[inbufpos+2])  // reset sync checker
						syncErrorFrame = 0;
					assert(game);
					if(game->gamePausable || a==0){
						timeLeft=0;
						serverNet->SendPause(inbuf[inbufpos+1],inbuf[inbufpos+2]);
					}
				}
				lastLength=3;
				break;

			case NETMSG_INTERNAL_SPEED:
				logOutput.Print("Server shouldnt get internal speed msgs?");
				lastLength=5;
				break;

			case NETMSG_USER_SPEED: {
				float speed=*((float*)&inbuf[inbufpos+1]);
				assert(game);
				if(speed>game->maxUserSpeed)
					speed=game->maxUserSpeed;
				if(speed<game->minUserSpeed)
					speed=game->minUserSpeed;
				if(gs->userSpeedFactor!=speed){
					if(gs->speedFactor==gs->userSpeedFactor || gs->speedFactor>speed)
						serverNet->SendInternalSpeed(speed);
					serverNet->SendUserSpeed(speed); //forward data
				}
				lastLength=5;
				break;}

			case NETMSG_CPU_USAGE:
				ENTER_MIXED;
				gs->players[a]->cpuUsage=*((float*)&inbuf[inbufpos+1]);
				ENTER_UNSYNCED;
				lastLength=5;
				break;

			case NETMSG_QUIT:
				ENTER_MIXED;
				gs->players[a]->active=false;
				ENTER_UNSYNCED;
				serverNet->connections[a]->active=false;
				serverNet->SendPlayerLeft(a, 1);
				lastLength=1;
				break;

			case NETMSG_PLAYERNAME: {
				unsigned char playerNum = inbuf[inbufpos+2];
				if(playerNum!=a && a!=0){
					SendSystemMsg("Server: Warning got playername msg from %i claiming to be from %i",a,playerNum);
				} else {
					ENTER_MIXED;
					gs->players[playerNum]->playerName=(char*)(&inbuf[inbufpos+3]);
					gs->players[playerNum]->readyToStart=true;
					gs->players[playerNum]->active=true;
					ENTER_UNSYNCED;

					SendSystemMsg("Player %s joined as %i",&inbuf[inbufpos+3],playerNum);
					serverNet->SendPlayerName(playerNum,gs->players[playerNum]->playerName);
				}
				lastLength=inbuf[inbufpos+1];
				break;
			}

			case NETMSG_CHAT:
				if(inbuf[inbufpos+2]!=a){
					SendSystemMsg("Server: Warning got chat msg from %i claiming to be from %i",a,inbuf[inbufpos+2]);
				} else {
					serverNet->SendChat(inbuf[inbufpos+2], (char*)(&inbuf[inbufpos+3]));
				}
				lastLength=inbuf[inbufpos+1];
				break;

			case NETMSG_SYSTEMMSG:
				if(inbuf[inbufpos+2]!=a){
					logOutput.Print("Server: Warning got system msg from %i claiming to be from %i",a,inbuf[inbufpos+2]);
				} else {
					serverNet->SendSystemMessage(inbuf[inbufpos+2], (char*)(&inbuf[inbufpos+3]));
				}
				lastLength=inbuf[inbufpos+1];
				break;

			case NETMSG_STARTPOS:
				if(inbuf[inbufpos+1]!=gs->players[a]->team && a!=0){
					SendSystemMsg("Server: Warning got startpos msg from %i claiming to be from team %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->SendStartPos(inbuf[inbufpos+1],inbuf[inbufpos+2], *((float*)&inbuf[inbufpos+3]), *((float*)&inbuf[inbufpos+7]), *((float*)&inbuf[inbufpos+11])); //forward data
				}
				lastLength=15;
				break;

			case NETMSG_COMMAND:
				if(inbuf[inbufpos+3]!=a){
					SendSystemMsg("Server: Warning got command msg from %i claiming to be from %i",a,inbuf[inbufpos+3]);
				} else {
					if(!serverNet->IsDemoServer())
						serverNet->RawSend(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1])); //forward data
				}
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;

			case NETMSG_SELECT:
				if(inbuf[inbufpos+3]!=a){
					SendSystemMsg("Server: Warning got select msg from %i claiming to be from %i",a,inbuf[inbufpos+3]);
				} else {
					if(!serverNet->IsDemoServer())
						serverNet->RawSend(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1])); //forward data
				}
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;

			case NETMSG_AICOMMAND:
				if(inbuf[inbufpos+3]!=a){
					SendSystemMsg("Server: Warning got aicommand msg from %i claiming to be from %i",a,inbuf[inbufpos+3]);
				}
				else if (gs->noHelperAIs) {
					SendSystemMsg("Server: Player %i is using a helper AI illegally", a);
				}
				else if(!serverNet->IsDemoServer()) {
					serverNet->RawSend(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1])); //forward data
				}
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;

			case NETMSG_AICOMMANDS:
				if(inbuf[inbufpos+3]!=a){
					SendSystemMsg("Server: Warning got aicommands msg from %i claiming to be from %i",a,inbuf[inbufpos+3]);
				}
				else if (gs->noHelperAIs) {
					SendSystemMsg("Server: Player %i is using a helper AI illegally", a);
				}
				else if(!serverNet->IsDemoServer()) {
					serverNet->RawSend(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1])); //forward data
				}
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;

			case NETMSG_SYNCRESPONSE:
#ifdef SYNCCHECK
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got syncresponse msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->IsDemoServer()){
						int frameNum = *(int*)&inbuf[inbufpos+2];
						if (outstandingSyncFrames.empty() || frameNum >= outstandingSyncFrames.front())
							syncResponse[a][frameNum] = *(unsigned*)&inbuf[inbufpos+6];
						else if (serverframenum - delayedSyncResponseFrame > SYNCCHECK_MSG_TIMEOUT) {
							delayedSyncResponseFrame = serverframenum;
							logOutput.Print("Delayed respone from %s for frame %d (current %d)",
											gs->players[a]->playerName.c_str(), frameNum, serverframenum);
						}
					}
				}
#endif
				lastLength=10;
				break;

			case NETMSG_SHARE:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got share msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->IsDemoServer())
						serverNet->SendShare(inbuf[inbufpos+1], inbuf[inbufpos+2], inbuf[inbufpos+3], *((float*)&inbuf[inbufpos+4]), *((float*)&inbuf[inbufpos+8]));
				}
				lastLength=12;
				break;

			case NETMSG_SETSHARE:
				if(inbuf[inbufpos+1]!=gs->players[a]->team){
					SendSystemMsg("Server: Warning got setshare msg from player %i claiming to be from team %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->IsDemoServer())
						serverNet->SendSetShare(inbuf[inbufpos+1], *((float*)&inbuf[inbufpos+2]), *((float*)&inbuf[inbufpos+6]));
				}
				lastLength=10;
				break;

			case NETMSG_PLAYERSTAT:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got stat msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->RawSend(&inbuf[inbufpos],sizeof(CPlayer::Statistics)+2); //forward data
				}
				lastLength=sizeof(CPlayer::Statistics)+2;
				break;

			case NETMSG_MAPDRAW:
				serverNet->RawSend(&inbuf[inbufpos],inbuf[inbufpos+1]); //forward data
				lastLength=inbuf[inbufpos+1];
				break;

#ifdef DIRECT_CONTROL_ALLOWED
			case NETMSG_DIRECT_CONTROL:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got direct control msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->IsDemoServer())
						serverNet->SendDirectControl(inbuf[inbufpos+1]);
				}
				lastLength=2;
				break;

			case NETMSG_DC_UPDATE:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got dc update msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->IsDemoServer())
						serverNet->SendDirectControlUpdate(inbuf[inbufpos+1], inbuf[inbufpos+2], *((short*)&inbuf[inbufpos+3]), *((short*)&inbuf[inbufpos+5]));
				}
				lastLength=7;
				break;
#endif
			default:
#ifdef SYNCDEBUG
				// maybe something for the sync debugger?
				lastLength = CSyncDebugger::GetInstance()->ServerReceived(&inbuf[inbufpos]);
				if (!lastLength)
#endif
				{
					logOutput.Print("Unknown net msg in server %d from %d pos %d last %d", (int)inbuf[inbufpos], a, inbufpos, lastMsg[a]);
					lastLength=1;
				}
				break;
			}
			if(lastLength<=0){
				logOutput.Print("Server readnet got packet type %i length %i pos %i from %i??",thisMsg,lastLength,inbufpos,a);
				lastLength=1;
			}
			inbufpos+=lastLength;
			lastMsg[a]=thisMsg;
		}
		if(inbufpos!=inbuflength){
			char txt[200];
			sprintf(txt,"Wrong packet length got %d from %d instead of %d",inbufpos,a,inbuflength);
			logOutput.Print("%s", txt);
			handleerror(0,txt,"Server network error",0);
		}
	}
#ifdef SYNCDEBUG
	CSyncDebugger::GetInstance()->ServerHandlePendingBlockRequests();
#endif
	return true;
}

void CGameServer::StartGame()
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex);
	serverNet->StopListening();

	// This should probably work now that I fixed a bug (netbuf->inbuf)
	// in Game.cpp, in the code receiving NETMSG_RANDSEED. --tvo
	//serverNet->SendRandSeed(gs->randSeed);

	for(unsigned a=0;a<gs->activePlayers;a++){
		if(!gs->players[a]->active)
			continue;
		serverNet->SendPlayerName(a, gs->players[a]->playerName);
	}
	if(gameSetup){
		for(unsigned a=0;a<gs->activeTeams;a++){
			serverNet->SendStartPos(a, 1, gs->Team(a)->startPos.x, gs->Team(a)->startPos.y, gs->Team(a)->startPos.z);
		}
	}
	serverNet->SendStartPlaying();
	timeLeft=0;
}

void CGameServer::CheckForGameEnd()
{
	if(gameEndDetected){
		if(gameEndTime>2 && gameEndTime<500){
			serverNet->SendGameOver();
			gameEndTime=600;
		}
		return;
	}

	unsigned numActiveTeams[MAX_TEAMS]; // active teams per ally team
	unsigned numActiveAllyTeams = 0, a;
	memset(numActiveTeams, 0, sizeof(numActiveTeams));

	for (a = 0; a < gs->activeTeams; ++a)
		if (!gs->Team(a)->isDead && !gs->Team(a)->gaia)
			++numActiveTeams[gs->AllyTeam(a)];

	for (a = 0; a < gs->activeAllyTeams; ++a)
		if (numActiveTeams[a] != 0)
			++numActiveAllyTeams;

	if (numActiveAllyTeams <= 1) {
		gameEndDetected=true;
		gameEndTime=0;
		serverNet->SendSendPlayerStat();
	}
}

void CGameServer::CreateNewFrame(bool fromServerThread)
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex,!fromServerThread);
	serverframenum++;
	if(serverNet->SendNewFrame(serverframenum) == -1){
		logOutput.Print("Server net couldnt send new frame");
		globalQuit=true;
	}
#ifdef SYNCCHECK
	outstandingSyncFrames.push_back(serverframenum);
#endif
}

void CGameServer::UpdateLoop()
{
	while(gameLoading){		//avoid timing out while loading (esp if calculating path data)
		SDL_Delay(100);
		serverNet->Update();
	}
	SDL_Delay(100);		//we might crash if game hasnt finished initializing within this time
	/*
	 * Need a better solution than this for starvation.
	 * Decreasing thread priority (making it more important)
	 * requires root privileges on POSIX systems
	 */
	//SetThreadPriority(thisThread,THREAD_PRIORITY_ABOVE_NORMAL);		//we want the server to continue running smoothly even if the game client is struggling
	while(!quitServer)
	{
		{
			boost::mutex::scoped_lock scoped_lock(gameServerMutex);
			if(!Update()){
				logOutput.Print("Game server experienced an error in update");
				globalQuit=true;
			}
		}
		SDL_Delay(10);
	}
}

bool CGameServer::WaitsOnCon() const
{
	return serverNet->waitOnCon;
}

void CGameServer::KickPlayer(const int playerNum)
{
	if (playerNum != 0 && gs->players[playerNum]->active) {
		unsigned char a = NETMSG_QUIT;
		serverNet->SendPlayerLeft(playerNum, 2);
		serverNet->connections[playerNum]->SendData(&a, 1);
		serverNet->Kill(playerNum);
	}
}

void CGameServer::SendSystemMsg(const char* fmt,...)
{
	char text[500];
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	VSNPRINTF(text, sizeof(text), fmt, ap);				// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	std::string msg = text;
	serverNet->SendSystemMessage(gu->myPlayerNum, msg);
}
