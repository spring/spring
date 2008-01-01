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
#include "System/BaseNetProtocol.h"
#include "System/DemoReader.h"
#include "System/AutohostInterface.h"
#include "Platform/ConfigHandler.h"
#include "FileSystem/CRC.h"
#include "Player.h"
#include "Team.h"

#define SYNCCHECK_TIMEOUT 300 //frames
#define SYNCCHECK_MSG_TIMEOUT 400  // used to prevent msg spam

GameParticipant::GameParticipant(bool willHaveRights)
: name("unnamed")
, readyToStart(false)
, cpuUsage (0.0f)
, ping (0)
, hasRights(willHaveRights)
{
}

CGameServer* gameServer=0;

CGameServer::CGameServer(int port, const std::string& newMapName, const std::string& newModName, const std::string& newScriptName, const std::string& demoName)
{
	delayedSyncResponseFrame = 0;
	syncErrorFrame=0;
	syncWarningFrame=0;
	lastPlayerInfo = 0;
	serverframenum=0;
	timeLeft=0;
	gameEndTime=0;
	lastUpdate = SDL_GetTicks();
	modGameTime = 0.0f;
	quitServer=false;
#ifdef DEBUG
	gameClientUpdated=false;
#endif
	play = 0;
	IsPaused = false;
	gamePausable = true;
	sentGameOverMsg = false;
	nextserverframenum = 0;
	serverNet = new CBaseNetProtocol();
	serverNet->InitServer(port);

	SendSystemMsg("Starting server on port %i", port);
	mapName = newMapName;
	mapChecksum = archiveScanner->GetMapChecksum(mapName);

	modName = newModName;
	std::string modArchive = archiveScanner->ModNameToModArchive(modName);
	modChecksum = archiveScanner->GetModChecksum(modArchive);

	scriptName = newScriptName;

	lastTick = SDL_GetTicks();

	int autohostport = configHandler.GetInt("Autohost", 0);
	if (autohostport > 0)
	{
		SendSystemMsg("Connecting to autohost on port %i", autohostport);
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
	
	if (!demoName.empty())
	{
		SendSystemMsg("Playing demo %s", demoName.c_str());
		play = new CDemoReader(demoName, modGameTime+0.1f);
	}

	thread = new boost::thread(boost::bind<void, CGameServer, CGameServer*>(&CGameServer::UpdateLoop, this));

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
	delete thread;
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

void CGameServer::AddLocalClient(unsigned wantedNumber)
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex);
	serverNet->ServerInitLocalClient();
	BindConnection(wantedNumber, true);
}

void CGameServer::PostLoad(unsigned newlastTick, int newserverframenum)
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex);
	lastTick = newlastTick;
//	serverframenum = newserverframenum;
	nextserverframenum = newserverframenum+1;
}

void CGameServer::SkipTo(int targetframe)
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex);
	serverframenum = targetframe;
}

std::string CGameServer::GetPlayerNames(const std::vector<int>& indices) const
{
	std::string playerstring;
	std::vector<int>::const_iterator p = indices.begin();
	for (; p != indices.end(); ++p) {
		if (!playerstring.empty())
			playerstring += ", ";
		playerstring += players[*p]->name;
	}
	return playerstring;
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
		for (unsigned a = 0; a < MAX_PLAYERS; ++a) {
			if (!players[a])
				continue;
			std::map<int, unsigned>::iterator it = players[a]->syncResponse.find(*f);
			if (it == players[a]->syncResponse.end()) {
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
				SendSystemMsg("No response from %s for frame %d", players.c_str(), *f);
			}
		}

		// If anything's in it, we have a desync.
		// TODO take care of !bComplete case?
		// Should we start resync then immediately or wait for the missing packets (while paused)?
		if ( /*bComplete && */ !desyncGroups.empty()) {
			if (!syncErrorFrame || (*f - syncErrorFrame > SYNCCHECK_MSG_TIMEOUT)) {
				syncErrorFrame = *f;

				// TODO enable this when we have resync
				//serverNet->SendPause(SERVER_PLAYER, true);

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
			for (unsigned a = 0; a < MAX_PLAYERS; ++a) {
				if (players[a])
					players[a]->syncResponse.erase(*f);
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
	if (!IsPaused && !WaitsOnCon())
	{
		modGameTime += float(SDL_GetTicks() - lastUpdate) * 0.001f * internalSpeed;
	}
	lastUpdate = SDL_GetTicks();

	if(lastPlayerInfo < (SDL_GetTicks() - 2000)){
		lastPlayerInfo = SDL_GetTicks();

		if (serverframenum > 0) {
			//send info about the players
			float maxCpu = 0.0f;
			for (int a = 0; a < MAX_PLAYERS; ++a) {
				if (players[a]) {
					serverNet->SendPlayerInfo(a, players[a]->cpuUsage, players[a]->ping);
					if (players[a]->cpuUsage > maxCpu) {
						maxCpu = players[a]->cpuUsage;
					}
				}
			}

			if (maxCpu != 0.0f) {
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
		unsigned length = 0;

		while ( (length = play->GetData(demobuffer, netcode::NETWORK_BUFFER_SIZE, modGameTime)) > 0 ) {
			if (demobuffer[0] == NETMSG_NEWFRAME)
			{
				// we can't use CreateNewFrame() here
				CheckSync();
				lastTick = SDL_GetTicks();
				serverframenum++;
				serverNet->SendNewFrame(serverframenum);
#ifdef SYNCCHECK
				outstandingSyncFrames.push_back(serverframenum);
#endif
			}
			else if (
			          demobuffer[0] != NETMSG_SETPLAYERNUM && 
			          demobuffer[0] != NETMSG_USER_SPEED && 
			          demobuffer[0] != NETMSG_INTERNAL_SPEED && 
			          demobuffer[0] != NETMSG_PAUSE) // dont send these from demo
			{
				serverNet->RawSend(demobuffer, length);
			}
		}

		if (play->ReachedEnd())
		{
			delete play;
			play=0;
			SendSystemMsg("End of demo reached");
		}
	}

	ServerReadNet();

	if (serverframenum > 0 && !play)
	{
		// Flushing the server after each new frame will result in smoother gameplay
		// don't need flushing at faster gameplay, we can save bandwith and lower the packet count
		// NOTE: smoother gameplay should be handled now by CGame, by smoothing
		// out network buffer reads if the packets arrive in short bursts.
		// (lots of packets have more chance to congest network stream)
		//if (CreateNewFrame(true) && internalSpeed < 1.2f)
		//	serverNet->FlushNet();
		CreateNewFrame(true);
	}
	serverNet->Update();

	if (hostif)
	{
		std::string msg = hostif->GetChatMessage();

		if (!msg.empty())
			GotChatMessage(msg, SERVER_PLAYER);
	}
	CheckForGameEnd();
}

void CGameServer::ServerReadNet()
{
	// handle new connections
	while (serverNet->HasIncomingConnection())
	{
		RawPacket* packet = serverNet->GetData();
		const unsigned char* inbuf = packet->data;

		if (packet->length >= 3 && inbuf[0] == NETMSG_ATTEMPTCONNECT && inbuf[2] == NETWORK_VERSION)
		{
			const unsigned wantedNumber = inbuf[1];
			BindConnection(wantedNumber);
		}
		else
		{
			SendSystemMsg("Client AttemptConnect rejected: NETMSG: %i VERSION: %i Length: %i", inbuf[0], inbuf[2], packet->length);
			serverNet->RejectIncomingConnection();
		}
		delete packet;
	}

	for(unsigned a=0; (int)a <= serverNet->MaxConnectionID(); a++)
	{
		if (serverNet->IsActiveConnection(a))
		{
			RawPacket* packet = 0;
			bool quit = false;
			while (!quit && (packet = serverNet->GetData(a)))
			{
				const unsigned char* inbuf = packet->data;
				switch (inbuf[0]){
					case NETMSG_NEWFRAME:
						players[a]->ping = serverframenum-*(int*)&inbuf[1];
						break;

					case NETMSG_PAUSE:
						if(inbuf[1]!=a){
							SendSystemMsg("Server: Warning got pause msg from %i claiming to be from %i",a,inbuf[1]);
						} else {
							if (!inbuf[2])  // reset sync checker
								syncErrorFrame = 0;
							if(gamePausable || players[a]->hasRights) // allow host to pause even if nopause is set
							{
								timeLeft=0;
								if (IsPaused != !!inbuf[2]) {
									IsPaused ? IsPaused = false : IsPaused = true;
								}
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
						players[a]->cpuUsage = *((float*)&inbuf[1]);
						break;

					case NETMSG_QUIT: {
						serverNet->SendPlayerLeft(a, 1);
						serverNet->Kill(a);
						quit = true;
						if (hostif)
						{
							hostif->SendPlayerLeft(a, 1);
						}
						break;
					}

					case NETMSG_PLAYERNAME: {
						unsigned char playerNum = inbuf[2];
						if(playerNum!=a){
							SendSystemMsg("Server: Warning got playername msg from %i claiming to be from %i",a,playerNum);
						} else {
							players[playerNum]->name = (std::string)((char*)inbuf+3);
							players[playerNum]->readyToStart = true;
							SendSystemMsg("Player %s joined as %i", players[playerNum]->name.c_str(), playerNum);
							serverNet->SendPlayerName(playerNum, players[playerNum]->name);
							if (hostif)
							{
								hostif->SendPlayerJoined(playerNum, players[playerNum]->name);
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
							SendSystemMsg("Server: Warning got system msg from %i claiming to be from %i",a,inbuf[2]);
						} else {
							serverNet->SendSystemMessage(inbuf[2], (char*)(&inbuf[3]));
						}
						break;

					case NETMSG_STARTPOS:
						if(inbuf[1] != a){
							SendSystemMsg("Server: Warning got startpos msg from %i claiming to be from %i",a,inbuf[1]);
						} else {
							serverNet->SendStartPos(inbuf[1],inbuf[2], inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12])); //forward data
							if (hostif)
							{
								hostif->SendPlayerReady(a, inbuf[3]);
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

					case NETMSG_LUAMSG:
						if(inbuf[3]!=a){
							SendSystemMsg("Server: Warning got LuaMsg from %i claiming to be from %i",a,inbuf[3]);
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
							int frameNum = *(int*)&inbuf[2];
							if (outstandingSyncFrames.empty() || frameNum >= outstandingSyncFrames.front())
								players[a]->syncResponse[frameNum] = *(unsigned*)&inbuf[6];
							else if (serverframenum - delayedSyncResponseFrame > SYNCCHECK_MSG_TIMEOUT) {
								delayedSyncResponseFrame = serverframenum;
								SendSystemMsg("Delayed response from %s for frame %d (current %d)",
										players[a]->name.c_str(), frameNum, serverframenum);
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
						if(inbuf[1]!= a){
							SendSystemMsg("Server: Warning got setshare msg from %i claiming to be from %i",a,inbuf[1]);
						} else {
							if(!play)
								serverNet->SendSetShare(inbuf[1], inbuf[2], *((float*)&inbuf[3]), *((float*)&inbuf[7]));
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

					case NETMSG_STARTPLAYING:
					{
						if (players[a]->hasRights)
							StartGame();
						break;
					}
					case NETMSG_RANDSEED:
					{
						if (players[a]->hasRights && !play)
							serverNet->SendRandSeed(*(unsigned int*)(inbuf+1));
						break;
					}

					// CGameServer should never get these messages
					case NETMSG_GAMEID:
					case NETMSG_INTERNAL_SPEED:
					case NETMSG_ATTEMPTCONNECT:
					case NETMSG_MAPNAME:
						break;
					default:
						{
							SendSystemMsg("Unhandled net msg (%d) in server from %d", (int)inbuf[0], a);
						}
						break;
				}
				delete packet;
			}
		}
		else if (players[a])
		{
			players[a].reset();
			serverNet->SendPlayerLeft(a, 0);
			SendSystemMsg("Lost connection to player %i (timeout)", a);
			if (hostif)
			{
				hostif->SendPlayerLeft(a, 0);
			}
		}
	}

	for (int a = (serverNet->MaxConnectionID() + 1); a < MAX_PLAYERS; ++a)
	{
		//HACK check if we lost connection to the last player(s)
		if (players[a])
		{
			players[a].reset();
			serverNet->SendPlayerLeft(a, 0);
			SendSystemMsg("Lost connection to player %i (timeout)", a);
			if (hostif)
			{
				hostif->SendPlayerLeft(a, 0);
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

	// Probably not that random since it's always the stuff that was on the
	// stack previously.  Also it's VERY frustrating with e.g. valgrind...
	//unsigned char buffer[128];	// uninitialised bytes (should be very random)
	//entropy.UpdateData(buffer, 128);

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
	serverNet->Listening(false);
	
	for(unsigned a=0; a < MAX_PLAYERS; ++a) {
			if(players[a])
				serverNet->SendPlayerName(a, players[a]->name);
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

	if (play) {
		// the client told us to start a demo
		// no need to send startpos and startplaying since its in the demo
		return;
	}
		
	GenerateAndSendGameID();

	if (gameSetup) {
		for (unsigned a = 0; a < gs->activeTeams; ++a)
		{
			serverNet->SendStartPos(SERVER_PLAYER, a, 1, gs->Team(a)->startPos.x, gs->Team(a)->startPos.y, gs->Team(a)->startPos.z);
		}
	}

	serverNet->SendStartPlaying();
	if (hostif)
	{
		hostif->SendStartPlaying();
	}
	timeLeft=0;
	lastTick = SDL_GetTicks()-1;
	CreateNewFrame(true);
}

void CGameServer::SetGamePausable(const bool arg)
{
	gamePausable = arg;
}

void CGameServer::CheckForGameEnd()
{
	if(gameEndTime > 0){
		if(gameEndTime < SDL_GetTicks()-2000 && !sentGameOverMsg){
			serverNet->SendGameOver();
			if (hostif)
			{
				hostif->SendGameOver();
			}
			sentGameOverMsg = true;
		}
		return;
	}

	unsigned numActiveTeams[MAX_TEAMS]; // active teams per ally team
	memset(numActiveTeams, 0, sizeof(numActiveTeams));
	unsigned numActiveAllyTeams = 0;

	for (unsigned a = 0; (int)a < gs->activeTeams; ++a)
		if (!gs->Team(a)->isDead && !gs->Team(a)->gaia)
			++numActiveTeams[gs->AllyTeam(a)];

	for (unsigned a = 0; (int)a < gs->activeAllyTeams; ++a)
		if (numActiveTeams[a] != 0)
			++numActiveAllyTeams;

	if (numActiveAllyTeams == 0)
	{
		// no ally left, end game
		gameEndTime=SDL_GetTicks();
		serverNet->SendSendPlayerStat();
	}
	else if (numActiveAllyTeams == 1 && !play)
	{
		// 1 ally left, end game only if we are not watching a demo
		gameEndTime=SDL_GetTicks();
		serverNet->SendSendPlayerStat();
	}
}

bool CGameServer::CreateNewFrame(bool fromServerThread)
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex,!fromServerThread);
	CheckSync();

	// Send out new frame messages.
	unsigned currentTick = SDL_GetTicks();
	unsigned timeElapsed = currentTick - lastTick;
	if (timeElapsed>200) {
		timeElapsed=200;
	}

#ifdef DEBUG
	if(gameClientUpdated){
		gameClientUpdated=false;
	}
#endif

	timeLeft+=GAME_SPEED*internalSpeed*float(timeElapsed)/1000.0f;
	lastTick=currentTick;
	bool newFrameCreated = false;

	while((timeLeft>0) && !IsPaused)
	{
#ifndef NO_AVI
		if((!game || !game->creatingVideo) || !fromServerThread)
#endif
		{
			if (nextserverframenum!=0) {
				serverframenum = nextserverframenum;
				nextserverframenum = 0;
			} else
				serverframenum++;
			serverNet->SendNewFrame(serverframenum);
			newFrameCreated = true;
#ifdef SYNCCHECK
			outstandingSyncFrames.push_back(serverframenum);
#endif
		}
		timeLeft--;
	}
	return newFrameCreated;
}

void CGameServer::UpdateLoop()
{
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
	if (playerNum != 0 && players[playerNum]) {
		serverNet->SendPlayerLeft(playerNum, 2);
		serverNet->SendQuit(playerNum);
		serverNet->Kill(playerNum);
		if (hostif)
		{
			hostif->SendPlayerLeft(playerNum, 2);
		}
	}
}

void CGameServer::BindConnection(unsigned wantedNumber, bool grantRights)
{
	if (play) {
		wantedNumber = std::max(wantedNumber, (unsigned)play->GetFileHeader().maxPlayerNum+1);
	}
	unsigned hisNewNumber = serverNet->AcceptIncomingConnection(wantedNumber);

	serverNet->SendSetPlayerNum((unsigned char)hisNewNumber, (unsigned char)hisNewNumber);

	// send game data for demo recording
	serverNet->SendScript(scriptName);
	serverNet->SendMapName(mapChecksum, mapName);
	serverNet->SendModName(modChecksum, modName);

	for (unsigned a = 0; a < MAX_PLAYERS; ++a) {
		if(players[a] && players[a]->readyToStart)
			serverNet->SendPlayerName(a, players[a]->name);
	}
	
	if (gameSetup) {
		for (unsigned a = 0; a < gs->activeTeams; ++a)
		{
			serverNet->SendStartPos(SERVER_PLAYER, a, 2, gs->Team(a)->startPos.x, gs->Team(a)->startPos.y, gs->Team(a)->startPos.z);
		}
	}
	
	players[hisNewNumber].reset(new GameParticipant(grantRights)); // give him rights to change speed, kick players etc
	SendSystemMsg("Client connected on slot %i", hisNewNumber);
	serverNet->FlushNet();
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
	serverNet->SendSystemMessage((unsigned char)SERVER_PLAYER, msg);
}

void CGameServer::GotChatMessage(const std::string& msg, unsigned player)
{
	//TODO: migrate more stuff from CGame::HandleChatMessage here
	if ((msg.find(".kickbynum") == 0) && (players[player]->hasRights)) {
		if (msg.length() >= 11) {
			int playerNum = atoi(msg.substr(11, string::npos).c_str());
			KickPlayer(playerNum);
		}
	}
	else if ((msg.find(".kick") == 0) && (players[player]->hasRights)) {
		if (msg.length() >= 6) {
			std::string name = msg.substr(6,string::npos);
			if (!name.empty()){
				StringToLowerInPlace(name);
				for (unsigned a=1; a < MAX_PLAYERS;++a){
					if (players[a]){
						std::string playerLower = StringToLower(players[a]->name);
						if (playerLower.find(name)==0) {               //can kick on substrings of name
							KickPlayer(a);
						}
					}
				}
			}
		}
	}
	else if ((msg.find(".nopause") == 0) && (players[player]->hasRights)) {
		SetBoolArg(gamePausable, msg, ".nopause");
	}
	else if ((msg.find(".setmaxspeed") == 0) && (players[player]->hasRights)) {
		maxUserSpeed = atof(msg.substr(12).c_str());
		if (userSpeedFactor > maxUserSpeed) {
			serverNet->SendUserSpeed(player, maxUserSpeed);
			userSpeedFactor = maxUserSpeed;
			if (internalSpeed > maxUserSpeed) {
				serverNet->SendInternalSpeed(userSpeedFactor);
				internalSpeed = userSpeedFactor;
			}
		}
	}
	else if ((msg.find(".setminspeed") == 0) && (players[player]->hasRights)) {
		minUserSpeed = atof(msg.substr(12).c_str());
		if (userSpeedFactor < minUserSpeed) {
			serverNet->SendUserSpeed(player, minUserSpeed);
			userSpeedFactor = minUserSpeed;
			if (internalSpeed < minUserSpeed) {
				serverNet->SendInternalSpeed(userSpeedFactor);
				internalSpeed = userSpeedFactor;
			}
		}
	}
	else {
		serverNet->SendChat(player, msg);
		if (hostif) {
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
