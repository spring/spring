#include "StdAfx.h"
#include "GameServer.h"
#include "NetProtocol.h"
#include "Player.h"
#include "Team.h"
#include "LogOutput.h"
#include "Game.h"
#include "GameSetup.h"
#include "StartScripts/ScriptHandler.h"
#include "Platform/errorhandler.h"
#include <stdarg.h>
#include <boost/bind.hpp>
#include <SDL_timer.h>

#define SYNCCHECK_TIMEOUT 300 //frames
#define SYNCCHECK_MSG_TIMEOUT 500  // used to prevent msg spam

CGameServer* gameServer=0;

extern bool globalQuit;

static Uint32 GameServerThreadProc(void* lpParameter)
{
	((CGameServer*)lpParameter)->UpdateLoop();
	return 0;
}


CGameServer::CGameServer()
{
	syncErrorFrame=0;
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

	int port=8452;
	int myPlayer=0;
	if(gameSetup){
		port=gameSetup->hostport;
		myPlayer=gameSetup->myPlayer;
	}
	serverNet=SAFE_NEW CNetProtocol();
	serverNet->InitServer(port);
	net->InitClient("localhost",port,0,true);
	serverNet->connections[myPlayer].localConnection=&net->connections[0];
	net->connections[0].localConnection=&serverNet->connections[myPlayer];
	serverNet->InitNewConn(&net->connections[0].addr,true,myPlayer);
	net->onlyLocal=true;

	if(gameSetup && gameSetup->hostDemo)
		serverNet->CreateDemoServer(gameSetup->demoName);

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

void CGameServer::CheckSync()
{
#ifdef SYNCCHECK
	// Check sync
	std::deque<int>::iterator f = outstandingSyncFrames.begin();
	while (f != outstandingSyncFrames.end()) {
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
					logOutput.Print("No sync response from %s for frame %d", gs->players[a]->playerName.c_str(), *f);
			} else {
				if (!bGotCorrectChecksum) {
					bGotCorrectChecksum = true;
					correctChecksum = it->second;
				} else if (it->second != correctChecksum) {
					desyncGroups[it->second].push_back(a);
				}
			}
		}

		// If anything's in it, we have a desync.
		// TODO take care of !bComplete case?
		// Should we start resync then immediately or wait for the missing packets (while paused)?
		if (bComplete && !desyncGroups.empty()) {
			if (!syncErrorFrame || (*f - syncErrorFrame > SYNCCHECK_MSG_TIMEOUT)) {
				syncErrorFrame = *f;

				// TODO enable this when we have resync
				//serverNet->SendPause(gu->myPlayerNum, true);

				//For each group, output a message with list of playernames in it.
				// TODO this should be linked to the resync system so it can roundrobin
				// the resync checksum request packets to multiple clients in the same group.
				std::map<unsigned, std::vector<int> >::const_iterator g = desyncGroups.begin();
				for (; g != desyncGroups.end(); ++g) {
					std::string players;
					std::vector<int>::const_iterator p = g->second.begin();
					for (; p != g->second.end(); ++p) {
						if (!players.empty())
							players += ", ";
						players += gs->players[*p]->playerName;
					}
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
//FIXME -- annoying as hell when you're using stdout		SendSystemMsg("Warning: Sync checking disabled!");
	}
#endif
}

bool CGameServer::Update()
{
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
	if (game && game->playing && !serverNet->playbackDemo){
		CheckSync();

		// Send out new frame messages.
		unsigned currentTick = SDL_GetTicks();
		float timeElapsed=((float)(currentTick - lastTick))/1000.f;
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
		if(gs->players[a]->active && (!gameSetup || a>=gameSetup->numDemoPlayers)){
			if((inbuflength=serverNet->GetData(inbuf,NETWORK_BUFFER_SIZE,a))==-1){
				PUSH_CODE_MODE;		//this could lead to some nasty errors if the other thread switches code mode...
				ENTER_MIXED;
				gs->players[a]->active=false;
				POP_CODE_MODE;
				inbuflength=0;
				serverNet->SendPlayerLeft(a, 0);
			}
		}
		//		logOutput << serverNet->numConnected << "\n";

		while(inbufpos<inbuflength){
			thisMsg=inbuf[inbufpos];
			int lastLength=0;
			switch (inbuf[inbufpos]){

			case NETMSG_ATTEMPTCONNECT: //handled directly in CNet
				lastLength=3;
				break;

			case NETMSG_HELLO:
				lastLength=1;
				break;

			case NETMSG_RANDSEED:
				lastLength=5;
				serverNet->SendData(&inbuf[inbufpos],5); //forward data
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
						serverNet->SendData(&inbuf[inbufpos],3);
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
					serverNet->SendData(&inbuf[inbufpos],5); //forward data
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
				serverNet->connections[a].active=false;
				serverNet->SendPlayerLeft(a, 1);
				lastLength=1;
				break;

			case NETMSG_PLAYERNAME:
				if(inbuf[inbufpos+2]!=a && a!=0){
					SendSystemMsg("Server: Warning got playername msg from %i claiming to be from %i",a,inbuf[inbufpos+2]);
				} else {
					ENTER_MIXED;
					gs->players[inbuf[inbufpos+2]]->playerName=(char*)(&inbuf[inbufpos+3]);
					gs->players[inbuf[inbufpos+2]]->readyToStart=true;
					gs->players[inbuf[inbufpos+2]]->active=true;
					ENTER_UNSYNCED;

					SendSystemMsg("Player %s joined as %i",&inbuf[inbufpos+3],inbuf[inbufpos+2]);
					serverNet->SendData(&inbuf[inbufpos],inbuf[inbufpos+1]); //forward data
				}
				lastLength=inbuf[inbufpos+1];
				break;

			case NETMSG_CHAT:
				if(inbuf[inbufpos+2]!=a){
					SendSystemMsg("Server: Warning got chat msg from %i claiming to be from %i",a,inbuf[inbufpos+2]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],inbuf[inbufpos+1]); //forward data
				}
				lastLength=inbuf[inbufpos+1];
				break;

			case NETMSG_SYSTEMMSG:
				if(inbuf[inbufpos+2]!=a){
					logOutput.Print("Server: Warning got system msg from %i claiming to be from %i",a,inbuf[inbufpos+2]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],inbuf[inbufpos+1]); //forward data
				}
				lastLength=inbuf[inbufpos+1];
				break;

			case NETMSG_STARTPOS:
				if(inbuf[inbufpos+1]!=gs->players[a]->team && a!=0){
					SendSystemMsg("Server: Warning got startpos msg from %i claiming to be from team %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],15); //forward data
				}
				lastLength=15;
				break;

			case NETMSG_COMMAND:
				if(inbuf[inbufpos+3]!=a){
					SendSystemMsg("Server: Warning got command msg from %i claiming to be from %i",a,inbuf[inbufpos+3]);
				} else {
					if(!serverNet->playbackDemo)
						serverNet->SendData(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1])); //forward data
				}
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;

			case NETMSG_SELECT:
				if(inbuf[inbufpos+3]!=a){
					SendSystemMsg("Server: Warning got select msg from %i claiming to be from %i",a,inbuf[inbufpos+3]);
				} else {
					if(!serverNet->playbackDemo)
						serverNet->SendData(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1])); //forward data
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
				else if(!serverNet->playbackDemo) {
					serverNet->SendData(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1])); //forward data
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
				else if(!serverNet->playbackDemo) {
					serverNet->SendData(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1])); //forward data
				}
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;

			case NETMSG_SYNCRESPONSE:
#ifdef SYNCCHECK
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got syncresponse msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->playbackDemo){
						int frameNum = *(int*)&inbuf[inbufpos+2];
						if (outstandingSyncFrames.empty() || frameNum >= outstandingSyncFrames.front())
							syncResponse[a][frameNum] = *(unsigned*)&inbuf[inbufpos+6];
						else
							logOutput.Print("Delayed sync respone from %s for frame %d (current %d)",
											gs->players[a]->playerName.c_str(), frameNum, serverframenum);
					}
				}
#endif
				lastLength=10;
				break;

			case NETMSG_SHARE:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got share msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->playbackDemo)
						serverNet->SendData(&inbuf[inbufpos],12); //forward data
				}
				lastLength=12;
				break;

			case NETMSG_SETSHARE:
				if(inbuf[inbufpos+1]!=gs->players[a]->team){
					SendSystemMsg("Server: Warning got setshare msg from player %i claiming to be from team %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->playbackDemo)
						serverNet->SendData(&inbuf[inbufpos],10); //forward data
				}
				lastLength=10;
				break;

			case NETMSG_PLAYERSTAT:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got stat msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],sizeof(CPlayer::Statistics)+2); //forward data
				}
				lastLength=sizeof(CPlayer::Statistics)+2;
				break;

			case NETMSG_MAPDRAW:
				serverNet->SendData(&inbuf[inbufpos],inbuf[inbufpos+1]); //forward data
				lastLength=inbuf[inbufpos+1];
				break;

#ifdef DIRECT_CONTROL_ALLOWED
			case NETMSG_DIRECT_CONTROL:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got direct control msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->playbackDemo)
						serverNet->SendData(&inbuf[inbufpos],2); //forward data
				}
				lastLength=2;
				break;

			case NETMSG_DC_UPDATE:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got dc update msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if(!serverNet->playbackDemo)
						serverNet->SendData(&inbuf[inbufpos],7); //forward data
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
			logOutput.Print(txt);
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

	for(int a=0;a<gs->activePlayers;a++){
		if(!gs->players[a]->active)
			continue;
		serverNet->SendPlayerName(a, gs->players[a]->playerName);
	}
	if(gameSetup){
		for(int a=0;a<gs->activeTeams;a++){
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

	bool active[MAX_TEAMS];

	for(int a=0;a<MAX_TEAMS;++a)
		active[a]=false;

	for(int a=0;a<gs->activeTeams;++a)
		if(!gs->Team(a)->isDead && !gs->Team(a)->gaia)
			active[gs->AllyTeam(a)]=true;

	int numActive=0;

	for(int a=0;a<MAX_TEAMS;++a)
		if(active[a])
			numActive++;

	if(numActive<=1){
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
