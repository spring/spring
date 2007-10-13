#include "GameServer.h"

#include <stdarg.h>
#include <boost/bind.hpp>
#include <SDL_timer.h>

#include "FileSystem/ArchiveScanner.h"

#ifndef NO_AVI
#include "Game.h"
#endif

#include "GameSetup.h"
#include "System/StdAfx.h"
#include "System/NetProtocol.h"
#include "System/DemoReader.h"
#include "System/AutohostInterface.h"
#include "System/Sync/Syncify.h"
#include "LogOutput.h"
#include "Platform/ConfigHandler.h"
#include "FileSystem/CRC.h"
#include "Player.h"
#include "Team.h"
#include "StartScripts/ScriptHandler.h"

#define SYNCCHECK_TIMEOUT 300 //frames
#define SYNCCHECK_MSG_TIMEOUT 400  // used to prevent msg spam

CGameServer* gameServer=0;

extern bool globalQuit;

CGameServer::CGameServer(int port, const std::string& mapName, const std::string& modName, const std::string& scriptName, const std::string& demoName)
{
	delayedSyncResponseFrame = 0;
	syncErrorFrame=0;
	syncWarningFrame=0;
	lastPlayerInfo = 0;
	serverframenum=0;
	timeLeft=0;
	gameLoading=true;
	gameEndDetected=false;
	gameEndTime=0;
	quitServer=false;
#ifdef DEBUG
	gameClientUpdated=false;
#endif
	maxTimeLeft=2;
	play = 0;
	IsPaused = false;

	serverNet = new CNetProtocol();

	serverNet->InitServer(port);
	if (!demoName.empty())
	{
		logOutput << "Initializing demo server...\n";
		play = new CDemoReader(demoName);
		gameLoading = false;
	}
	else // no demo, so set map and mod to send it to clients
	{
		serverNet->SendMapName(archiveScanner->GetMapChecksum(mapName), mapName);
		std::string modArchive = archiveScanner->ModNameToModArchive(modName);
		serverNet->SendModName(archiveScanner->GetModChecksum(modArchive), modName);
		serverNet->SendScript(scriptName);
	}

	serverNet->ServerInitLocalClient( gameSetup ? gameSetup->myPlayer : 0 );

	lastTick = SDL_GetTicks();

	int autohostport = configHandler.GetInt("Autohost", 0);
	if (autohostport > 0)
	{
		logOutput.Print("Connecting to autohost on port %i", autohostport);
		hostif = new AutohostInterface(port+10, autohostport);
		hostif->SendStart();
	}
	else
	{
		hostif = 0;
	}

	if (gameSetup) {
		maxUserSpeed = gameSetup->maxSpeed;
		minUserSpeed = gameSetup->minSpeed;
		userSpeedFactor = gs->userSpeedFactor;
		internalSpeed = gs->speedFactor;
	}
	else
	{
		maxUserSpeed=3;
		minUserSpeed=0.3f;
		userSpeedFactor = 1.0f;
		internalSpeed = 1.0f;
	}

	thread = new boost::thread(boost::bind<void, CGameServer, CGameServer*>(&CGameServer::UpdateLoop, this));

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
	if (play)
		delete play;
	
	delete serverNet;
	serverNet=0;
	if (hostif)
	{
		hostif->SendQuit();
		delete hostif;
	}
}

std::string CGameServer::GetPlayerNames(const std::vector<int>& indices)
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

void CGameServer::Update()
{
	if(play)
		serverframenum=gs->frameNum;

	if(lastPlayerInfo<gu->gameTime-2){
		lastPlayerInfo=gu->gameTime;

		if (serverframenum > 0) {
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
				float wantedCpu=0.35f+(1-internalSpeed/userSpeedFactor)*0.5f;
				//float speedMod=1+wantedCpu-maxCpu;
				float newSpeed=internalSpeed*wantedCpu/maxCpu;
				//logOutput.Print("Speed %f %f %f %f",maxCpu,wantedCpu,speedMod,newSpeed);
				newSpeed=(newSpeed+internalSpeed)*0.5f;
				if(newSpeed>userSpeedFactor)
					newSpeed=userSpeedFactor;
				if(newSpeed<0.1f)
					newSpeed=0.1f;
				if(newSpeed!=internalSpeed)
					serverNet->SendInternalSpeed(newSpeed);
			}
		}
	}

	// when hosting a demo, read from file and broadcast data
	if (play != 0) {
		unsigned char demobuffer[netcode::NETWORK_BUFFER_SIZE];
		unsigned length = play->GetData(demobuffer, netcode::NETWORK_BUFFER_SIZE);
		
		while (length > 0) {
			serverNet->RawSend(demobuffer, length);
			length = play->GetData(demobuffer, netcode::NETWORK_BUFFER_SIZE);
		}
	}
	
	ServerReadNet();

	if (serverframenum > 0 && !play)
	{
		CreateNewFrame(true);
	}
	serverNet->Update();

	if (hostif)
	{
		std::string msg = hostif->GetChatMessage();

		if (!msg.empty())
			GotChatMessage(msg, gameSetup ? gameSetup->myPlayer : 0);
	}
	CheckForGameEnd();
}

void CGameServer::ServerReadNet()
{
	for(unsigned a=0; a<MAX_PLAYERS; a++)
	{
		if (serverNet->IsActiveConnection(a))
		{
			unsigned char inbuf[8000];
			int ret = serverNet->GetData(inbuf, a);
			while (ret > 0)
			{
				switch (inbuf[0]){
					case NETMSG_NEWFRAME:
						gs->players[a]->ping=serverframenum-*(int*)&inbuf[1];
						break;

					case NETMSG_PAUSE:
						if(inbuf[1]!=a){
							logOutput.Print("Server: Warning got pause msg from %i claiming to be from %i",a,inbuf[1]);
						} else {
							if (!inbuf[2])  // reset sync checker
								syncErrorFrame = 0;
							if(gamePausable || a==0) // allow host to pause even if nopause is set
							{
								timeLeft=0;
								IsPaused ? IsPaused = false : IsPaused = true;
								serverNet->SendPause(inbuf[1],inbuf[2]);
							}
						}
						break;

					case NETMSG_USER_SPEED: {
						unsigned char playerNum = inbuf[1];
						float speed = *((float*) &inbuf[2]);

						if (speed > maxUserSpeed)
							speed = maxUserSpeed;
						if (speed < minUserSpeed)
							speed = minUserSpeed;
						if (userSpeedFactor != speed)
						{
							if (internalSpeed == userSpeedFactor || internalSpeed>speed)
							{
								serverNet->SendInternalSpeed(speed);
								internalSpeed = speed;
							}
							// forward data
							serverNet->SendUserSpeed(playerNum, speed);
							userSpeedFactor = speed;
						}
					} break;


					case NETMSG_CPU_USAGE:
						ENTER_MIXED;
						gs->players[a]->cpuUsage=*((float*)&inbuf[1]);
						ENTER_UNSYNCED;
						break;

					case NETMSG_QUIT: {
						serverNet->SendPlayerLeft(a, 1);
						serverNet->Kill(a);
						if (hostif)
						{
							hostif->SendPlayerLeft(a, 1);
						}
						break;
					}

					case NETMSG_PLAYERNAME: {
						unsigned char playerNum = inbuf[2];
						if(playerNum!=a && a!=0){
							SendSystemMsg("Server: Warning got playername msg from %i claiming to be from %i",a,playerNum);
						} else {
							SendSystemMsg("Player %s joined as %i",&inbuf[3],playerNum);
							serverNet->SendPlayerName(playerNum,gs->players[playerNum]->playerName);
							if (hostif)
							{
								hostif->SendPlayerJoined(a, gs->players[playerNum]->playerName);
							}
						}
						break;
					}

					case NETMSG_CHAT:
						if(inbuf[2]!=a){
							SendSystemMsg("Server: Warning got chat msg from %i claiming to be from %i",a,inbuf[2]);
						} else {
							GotChatMessage(std::string((char*)(inbuf+3)), inbuf[2]);
						}
						break;

					case NETMSG_SYSTEMMSG:
						if(inbuf[2]!=a){
							logOutput.Print("Server: Warning got system msg from %i claiming to be from %i",a,inbuf[2]);
						} else {
							serverNet->SendSystemMessage(inbuf[2], (char*)(&inbuf[3]));
						}
						break;

					case NETMSG_STARTPOS:
						if(inbuf[1]!=gs->players[a]->team && a!=0){
							SendSystemMsg("Server: Warning got startpos msg from %i claiming to be from team %i",a,inbuf[1]);
						} else {
							serverNet->SendStartPos(inbuf[1],inbuf[2], *((float*)&inbuf[3]), *((float*)&inbuf[7]), *((float*)&inbuf[11])); //forward data
							if (hostif)
							{
								hostif->SendPlayerReady(a, inbuf[2]);
							}
						}
						break;

					case NETMSG_COMMAND:
						if(inbuf[3]!=a){
							SendSystemMsg("Server: Warning got command msg from %i claiming to be from %i",a,inbuf[3]);
						} else {
							if(!play)
								serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_SELECT:
						if(inbuf[3]!=a){
							SendSystemMsg("Server: Warning got select msg from %i claiming to be from %i",a,inbuf[3]);
						} else {
							if(!play)
								serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_AICOMMAND:
						if(inbuf[3]!=a){
							SendSystemMsg("Server: Warning got aicommand msg from %i claiming to be from %i",a,inbuf[3]);
						}
						else if (gs->noHelperAIs) {
							SendSystemMsg("Server: Player %i is using a helper AI illegally", a);
						}
						else if(!play) {
							serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_AICOMMANDS:
						if(inbuf[3]!=a){
							SendSystemMsg("Server: Warning got aicommands msg from %i claiming to be from %i",a,inbuf[3]);
						}
						else if (gs->noHelperAIs) {
							SendSystemMsg("Server: Player %i is using a helper AI illegally", a);
						}
						else if(!play) {
							serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_SYNCRESPONSE:
#ifdef SYNCCHECK
						if(inbuf[1]!=a){
							SendSystemMsg("Server: Warning got syncresponse msg from %i claiming to be from %i",a,inbuf[1]);
						} else {
							if(!play){
								int frameNum = *(int*)&inbuf[2];
								if (outstandingSyncFrames.empty() || frameNum >= outstandingSyncFrames.front())
									syncResponse[a][frameNum] = *(unsigned*)&inbuf[6];
								else if (serverframenum - delayedSyncResponseFrame > SYNCCHECK_MSG_TIMEOUT) {
									delayedSyncResponseFrame = serverframenum;
									logOutput.Print("Delayed respone from %s for frame %d (current %d)",
											gs->players[a]->playerName.c_str(), frameNum, serverframenum);
								}
							}
						}
#endif
						break;

					case NETMSG_SHARE:
						if(inbuf[1]!=a){
							SendSystemMsg("Server: Warning got share msg from %i claiming to be from %i",a,inbuf[1]);
						} else {
							if(!play)
								serverNet->SendShare(inbuf[1], inbuf[2], inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8]));
						}
						break;

					case NETMSG_SETSHARE:
						if(inbuf[1]!=gs->players[a]->team){
							SendSystemMsg("Server: Warning got setshare msg from player %i claiming to be from team %i",a,inbuf[1]);
						} else {
							if(!play)
								serverNet->SendSetShare(inbuf[1], *((float*)&inbuf[2]), *((float*)&inbuf[6]));
						}
						break;

					case NETMSG_PLAYERSTAT:
						if(inbuf[1]!=a){
							SendSystemMsg("Server: Warning got stat msg from %i claiming to be from %i",a,inbuf[1]);
						} else {
							serverNet->RawSend(inbuf,sizeof(CPlayer::Statistics)+2); //forward data
						}
						break;

					case NETMSG_MAPDRAW:
						serverNet->RawSend(inbuf,inbuf[1]); //forward data
						break;

#ifdef DIRECT_CONTROL_ALLOWED
					case NETMSG_DIRECT_CONTROL:
						if(inbuf[1]!=a){
							SendSystemMsg("Server: Warning got direct control msg from %i claiming to be from %i",a,inbuf[1]);
						} else {
							if(!play)
								serverNet->SendDirectControl(inbuf[1]);
						}
						break;

					case NETMSG_DC_UPDATE:
						if(inbuf[1]!=a){
							SendSystemMsg("Server: Warning got dc update msg from %i claiming to be from %i",a,inbuf[1]);
						} else {
							if(!play)
								serverNet->SendDirectControlUpdate(inbuf[1], inbuf[2], *((short*)&inbuf[3]), *((short*)&inbuf[5]));
						}
						break;
#endif

					// CGameServer should never get these messages
					case NETMSG_RANDSEED:
					case NETMSG_GAMEID:
					case NETMSG_INTERNAL_SPEED:
					case NETMSG_ATTEMPTCONNECT:
					case NETMSG_MAPNAME:
						break;
					default:
						{
							logOutput.Print("Unhandled net msg (%d) in server from %d", (int)inbuf[0], a);
						}
						break;
				}
				ret = serverNet->GetData(inbuf, a);
			}

			if (ret == -1)
			{
				serverNet->SendPlayerLeft(a, 0);
				if (hostif)
				{
					hostif->SendPlayerLeft(a, 0);
				}
			}
		}
	}

#ifdef SYNCDEBUG
	CSyncDebugger::GetInstance()->ServerHandlePendingBlockRequests();
#endif
}

// FIXME: move to separate file->make utility class of it and use it in GlobalStuff
class CRandomNumberGenerator
{
public:
	CRandomNumberGenerator() : randSeed(0) {}
	void Seed(unsigned seed) { randSeed = seed; }
	unsigned int operator()() {
		randSeed = (randSeed * 214013L + 2531011L);
		return randSeed & 0x7FFF;
	}
private:
	unsigned randSeed;
};

/** @brief Generate a unique game identifier and sent it to all clients. */
void CGameServer::GenerateAndSendGameID()
{
	CRandomNumberGenerator prand;
	prand.Seed(SDL_GetTicks());

	// This is where we'll store the ID temporarily.
	union {
		unsigned char charArray[16];
		unsigned int intArray[4];
	} gameID;

	// First and second dword are time based (current time and load time).
	gameID.intArray[0] = (unsigned) time(NULL);
	for (int i = 4; i < 12; ++i)
		gameID.charArray[i] = prand();

	CRC entropy;
	entropy.UpdateData((const unsigned char*)&lastTick, sizeof(lastTick));
	unsigned char buffer[128];	// uninitialised bytes (should be very random)
	entropy.UpdateData(buffer, 128);
	
	// Third dword is CRC of gameSetupText (if there is a gameSetup)
	// or pseudo random bytes (if there is no gameSetup)
	if (gameSetup != NULL) {
		CRC crc;
		crc.UpdateData((const unsigned char*)gameSetup->gameSetupText, gameSetup->gameSetupTextLength);
		gameID.intArray[2] = crc.GetCRC();
	}

	// Fourth dword is CRC of the network buffer, which should be pretty random.
	// (depends on start positions, chat messages, user input, etc.)
	gameID.intArray[3] = entropy.GetCRC();

	serverNet->SendGameID(gameID.charArray);
}

void CGameServer::StartGame()
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex);
	serverNet->Listening(false);

	serverNet->SendRandSeed(gs->randSeed);

	GenerateAndSendGameID();

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

	// make sure initial game speed is within allowed range and sent a new speed if not
	if(userSpeedFactor>maxUserSpeed)
	{
		serverNet->SendUserSpeed(0,maxUserSpeed);
		userSpeedFactor = maxUserSpeed;
	}
	else if(userSpeedFactor<minUserSpeed)
	{
		serverNet->SendUserSpeed(0,minUserSpeed);
		userSpeedFactor = minUserSpeed;
	}

	serverNet->SendStartPlaying();
	if (hostif)
	{
		hostif->SendStartPlaying();
	}
	timeLeft=0;
	CreateNewFrame(true);
}

void CGameServer::SetGamePausable(const bool arg)
{
	gamePausable = arg;
}

void CGameServer::CheckForGameEnd()
{
	if(gameEndDetected){
		if(gameEndTime>2 && gameEndTime<500){
			serverNet->SendGameOver();
			gameEndTime=600;
			if (hostif)
			{
				hostif->SendGameOver();
			}
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
	CheckSync();

	// Send out new frame messages.
	unsigned currentTick = SDL_GetTicks();
	float timeElapsed=((float)(currentTick - lastTick))/1000.f;
	if (timeElapsed>0.2) {
		timeElapsed=0.2;
	}
	if(gameEndDetected)
		gameEndTime+=timeElapsed;
	// logOutput.Print("float value is %f",timeElapsed);

#ifdef DEBUG
	if(gameClientUpdated){
		gameClientUpdated=false;
		maxTimeLeft=2;
	}
#else
	maxTimeLeft=2;
#endif
	if(timeElapsed>maxTimeLeft)
		timeElapsed=maxTimeLeft;
	maxTimeLeft-=timeElapsed;

	timeLeft+=GAME_SPEED*internalSpeed*timeElapsed;
	lastTick=currentTick;

	while((timeLeft>0) && !IsPaused)
	{
#ifndef NO_AVI
		if((!game || !game->creatingVideo) || !fromServerThread)
#endif
		{
			boost::mutex::scoped_lock scoped_lock(gameServerMutex,!fromServerThread);
			serverframenum++;
			serverNet->SendNewFrame(serverframenum);
#ifdef SYNCCHECK
			outstandingSyncFrames.push_back(serverframenum);
#endif
		}
		timeLeft--;
	}
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
			Update();
		}
		SDL_Delay(10);
	}
}

bool CGameServer::WaitsOnCon() const
{
	return serverNet->Listening();
}

void CGameServer::KickPlayer(const int playerNum)
{
	if (playerNum != 0 && gs->players[playerNum]->active) {
		serverNet->SendPlayerLeft(playerNum, 2);
		serverNet->SendQuit(playerNum);
		serverNet->Kill(playerNum);
		if (hostif)
		{
			hostif->SendPlayerLeft(playerNum, 2);
		}
	}
}

void CGameServer::PlayerDefeated(const int playerNum) const
{
	if (hostif)
	{
		hostif->SendPlayerDefeated((unsigned char)playerNum);
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

void CGameServer::GotChatMessage(const std::string& msg, unsigned player)
{
	//TODO: find better solution for the (player == 0) thingie
	//TODO 2: migrate more stuff from CGame::HandleChatMessage here
	if ((msg.find(".kickbynum") == 0) && (player == 0)) {
		if (msg.length() >= 11) {
			int playerNum = atoi(msg.substr(11, string::npos).c_str());
			KickPlayer(playerNum);
		}
	}
	else if ((msg.find(".kick") == 0) && (player == 0)) {
		if (msg.length() >= 6) {
			std::string name = msg.substr(6,string::npos);
			if (!name.empty()){
				StringToLowerInPlace(name);
				for (unsigned a=1;a<gs->activePlayers;++a){
					if (gs->players[a]->active){
						std::string playerLower = StringToLower(gs->players[a]->playerName);
						if (playerLower.find(name)==0){               //can kick on substrings of name
							KickPlayer(a);
						}
					}
				}
			}
		}
	}
	else if ((msg.find(".nopause") == 0) && (player == 0))
	{
		SetBoolArg(gamePausable, msg, ".nopause");
	}
	else if ((msg.find(".setmaxspeed") == 0) && (player == 0)  /*&& !net->localDemoPlayback*/) {
		maxUserSpeed = atof(msg.substr(12).c_str());
		if (userSpeedFactor > maxUserSpeed) {
			serverNet->SendUserSpeed(player, maxUserSpeed);
			userSpeedFactor = maxUserSpeed;
			if (internalSpeed > maxUserSpeed)
			{
				serverNet->SendInternalSpeed(userSpeedFactor);
				internalSpeed = userSpeedFactor;
			}
		}
	}
	else if ((msg.find(".setminspeed") == 0) && (player == 0)  /*&& !net->localDemoPlayback*/) {
		minUserSpeed = atof(msg.substr(12).c_str());
		if (userSpeedFactor < minUserSpeed) {
			serverNet->SendUserSpeed(player, minUserSpeed);
			userSpeedFactor = minUserSpeed;
			if (internalSpeed < minUserSpeed)
			{
				serverNet->SendInternalSpeed(userSpeedFactor);
				internalSpeed = userSpeedFactor;
			}
		}
	}
	else
	{
		serverNet->SendChat(player, msg);
		if (hostif)
		{
			hostif->SendPlayerChat(player, msg);
		}
	}
}

void CGameServer::SetBoolArg(bool& value, const std::string& str, const char* cmd)
{
	char* end;
	const char* start = str.c_str() + strlen(cmd);
	const int num = strtol(start, &end, 10);
	if (end != start) {
		value = (num != 0);
	} else {
		value = !value;
	}
}
