#include "GameServer.h"

#include <stdarg.h>
#include <ctime>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <SDL_timer.h>

#include "FileSystem/ArchiveScanner.h"

#ifndef NO_AVI
#include "Game.h"
#endif

#include "GameSetupData.h"
#include "System/StdAfx.h"
#include "System/BaseNetProtocol.h"
#include "System/DemoReader.h"
#include "System/AutohostInterface.h"
#include "Platform/ConfigHandler.h"
#include "FileSystem/CRC.h"
#include "Player.h"
#include "Team.h"
#include "Server/MsgStrings.h"

#ifdef DEDICATED
#include <iostream>
#endif

#define SYNCCHECK_TIMEOUT 300 //frames
#define SYNCCHECK_MSG_TIMEOUT 400  // used to prevent msg spam
const unsigned GameStartDelay = 3800; //msecs to wait until the game starts after all players are ready
using boost::format;

GameParticipant::GameParticipant(bool willHaveRights)
: name("unnamed")
, readyToStart(false)
, cpuUsage (0.0f)
, ping (0)
, hasRights(willHaveRights)
, team(0)
{
}

CGameServer* gameServer=0;

CGameServer::CGameServer(int port, const std::string& newMapName, const std::string& newModName, const std::string& newScriptName, const CGameSetupData* const mysetup = 0, const std::string& demoName)
: setup(mysetup)
{
	serverStartTime = SDL_GetTicks();
	delayedSyncResponseFrame = 0;
	syncErrorFrame=0;
	syncWarningFrame=0;
	lastPlayerInfo = 0;
	serverframenum=0;
	timeLeft=0;
	readyTime = 0;
	gameStartTime = 0;
	gameEndTime=0;
	lastUpdate = SDL_GetTicks();
	modGameTime = 0.0f;
	quitServer=false;
#ifdef DEBUG
	gameClientUpdated=false;
#endif
	demoReader = 0;
	hostif = 0;
	IsPaused = false;
	userSpeedFactor = 1.0f;
	internalSpeed = 1.0f;
	gamePausable = true;
	noHelperAIs = false;
	cheating = false;
	sentGameOverMsg = false;
	serverNet = new CBaseNetProtocol();
	serverNet->InitServer(port);
	rng.Seed(SDL_GetTicks());
	log.Subscribe(this);
	log.Message(format(ServerStart) %port);
	mapName = newMapName;
	mapChecksum = archiveScanner->GetMapChecksum(mapName);

	modName = newModName;
	const std::string modArchive = archiveScanner->ModNameToModArchive(modName);
	modChecksum = archiveScanner->GetModChecksum(modArchive);

	scriptName = newScriptName;

	lastTick = SDL_GetTicks();

	if (setup) {
		maxUserSpeed = setup->maxSpeed;
		minUserSpeed = setup->minSpeed;
		noHelperAIs = (bool)setup->noHelperAIs;
	}
	else
	{
		maxUserSpeed=5;
		minUserSpeed=0.3f;
	}

	if (!demoName.empty())
	{
		log.Message(format(PlayingDemo) %demoName);
		demoReader = new CDemoReader(demoName, modGameTime+0.1f);
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
	if (demoReader)
		delete demoReader;

	delete serverNet;
	serverNet=0;
	if (hostif)
	{
		hostif->SendQuit();
		log.Unsubscribe(hostif);
		delete hostif;
	}
}

void CGameServer::AddLocalClient(unsigned wantedNumber)
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex);
	serverNet->ServerInitLocalClient();
	BindConnection(wantedNumber, true);
}

void CGameServer::AddAutohostInterface(const int usePort, const int remotePort)
{
	if (hostif == 0)
	{
		boost::mutex::scoped_lock scoped_lock(gameServerMutex);
		hostif = new AutohostInterface(usePort, remotePort);
		hostif->SendStart();
		log.Subscribe(hostif);
		log.Message(format(ConnectAutohost) %remotePort);
	}
}

void CGameServer::PostLoad(unsigned newlastTick, int newserverframenum)
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex);
	lastTick = newlastTick;
	serverframenum = newserverframenum;
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

void CGameServer::Message(const std::string& message)
{
	serverNet->SendSystemMessage(SERVER_PLAYER, message);
}

void CGameServer::Warning(const std::string& message)
{
	serverNet->SendSystemMessage(SERVER_PLAYER, message);
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
				log.Warning(format(NoSyncResponse) %players %(*f));
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
					log.Warning(format(SyncError) %players %(*f) %(g->first ^ correctChecksum));
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
		log.Warning(NoSyncCheck);
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
	if (demoReader != 0) {
		unsigned char demobuffer[netcode::NETWORK_BUFFER_SIZE];
		unsigned length = 0;

		while ( (length = demoReader->GetData(demobuffer, netcode::NETWORK_BUFFER_SIZE, modGameTime)) > 0 ) {
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

		if (demoReader->ReachedEnd()) {
			delete demoReader;
			demoReader = 0;
			log.Message(DemoEnd);
			gameEndTime = SDL_GetTicks();
		}
	}

	ServerReadNet();

	if (!gameStartTime)
	{
		CheckForGameStart();
	}
	else if (serverframenum > 0 && !demoReader)
	{
		CreateNewFrame(true, false);
	}
	serverNet->Update();

	if (hostif)
	{
		std::string msg = hostif->GetChatMessage();

		if (!msg.empty())
			GotChatMessage(msg, SERVER_PLAYER);
	}
	if (!demoReader && !sentGameOverMsg)
		CheckForGameEnd();
	
	if ((SDL_GetTicks() - serverStartTime) > 30000 && serverNet->MaxConnectionID() == -1)
	{
		log.Message(NoClientsExit);
		quitServer = true;
	}
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
			log.Warning(format(ConnectionReject) %inbuf[0] %inbuf[2] %packet->length);
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
				const unsigned length = packet->length;
				switch (inbuf[0]){
					case NETMSG_NEWFRAME:
						players[a]->ping = serverframenum-*(int*)&inbuf[1];
						break;

					case NETMSG_PAUSE:
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[1]);
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
						log.Message(format(PlayerLeft) %players[a]->name %" normal quit");
						serverNet->SendPlayerLeft(a, 1);
						serverNet->Kill(a);
						quit = true;
						players[a].reset();
						if (hostif)
						{
							hostif->SendPlayerLeft(a, 1);
						}
						break;
					}

					case NETMSG_PLAYERNAME: {
						unsigned playerNum = inbuf[2];
						if(playerNum!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %playerNum);
						} else {
							players[playerNum]->name = (std::string)((char*)inbuf+3);
							players[playerNum]->readyToStart = true;
							log.Message(format(PlayerJoined) %players[playerNum]->name %playerNum);
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
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[2]);
						} else {
							GotChatMessage(std::string((char*)(inbuf+3)), inbuf[2]);
						}
						break;

					case NETMSG_SYSTEMMSG:
						if(inbuf[2]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[2]);
						} else {
							serverNet->SendSystemMessage(inbuf[2], (char*)(&inbuf[3]));
						}
						break;

					case NETMSG_STARTPOS:
						if(inbuf[1] != a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[1]);
						}
						else if (setup && setup->startPosType == CGameSetupData::StartPos_ChooseInGame)
						{
							unsigned team = (unsigned)inbuf[2];
							if (teams[team])
							{
								teams[team]->startpos = SFloat3(*((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12]));
							}
							serverNet->SendStartPos(inbuf[1],team, inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12])); //forward data
							if (hostif)
							{
								hostif->SendPlayerReady(a, inbuf[3]);
							}
						}
						else
						{
							log.Warning(format(NoStartposChange) %a);
						}
						break;

					case NETMSG_COMMAND:
						if(inbuf[3]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[3]);
						} else {
							if (!demoReader)
								serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_SELECT:
						if(inbuf[3]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[3]);
						} else {
							if (!demoReader)
								serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_AICOMMAND:
						if(inbuf[3]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[3]);
						}
						else if (noHelperAIs) {
							log.Warning(format(NoHelperAI) %players[a]->name %a);
						}
						else if (!demoReader) {
							serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_AICOMMANDS:
						if(inbuf[3]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[3]);
						}
						else if (noHelperAIs) {
							log.Warning(format(NoHelperAI) %players[a]->name %a);
						}
						else if (!demoReader) {
							serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_LUAMSG:
						if(inbuf[3]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[3]);
						}
						else if (!demoReader) {
							serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_SYNCRESPONSE:
#ifdef SYNCCHECK
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[1]);
						} else {
							int frameNum = *(int*)&inbuf[2];
							if (outstandingSyncFrames.empty() || frameNum >= outstandingSyncFrames.front())
								players[a]->syncResponse[frameNum] = *(unsigned*)&inbuf[6];
							else if (serverframenum - delayedSyncResponseFrame > SYNCCHECK_MSG_TIMEOUT) {
								delayedSyncResponseFrame = serverframenum;
								log.Warning(format(DelayedSyncResponse) %players[a]->name %frameNum %serverframenum);
							}
							// update players' ping (if !defined(SYNCCHECK) this is done in NETMSG_NEWFRAME)
							players[a]->ping = serverframenum - frameNum;
						}
#endif
						break;

					case NETMSG_SHARE:
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[1]);
						} else {
							if (!demoReader)
								serverNet->SendShare(inbuf[1], inbuf[2], inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8]));
						}
						break;

					case NETMSG_SETSHARE:
						if(inbuf[1]!= a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[1]);
						} else {
							if (!demoReader)
								serverNet->SendSetShare(inbuf[1], inbuf[2], *((float*)&inbuf[3]), *((float*)&inbuf[7]));
						}
						break;

					case NETMSG_PLAYERSTAT:
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[1]);
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
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[1]);
						} else {
							if (!demoReader)
								serverNet->SendDirectControl(inbuf[1]);
						}
						break;

					case NETMSG_DC_UPDATE:
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %inbuf[0] %a %inbuf[1]);
						} else {
							if (!demoReader)
								serverNet->SendDirectControlUpdate(inbuf[1], inbuf[2], *((short*)&inbuf[3]), *((short*)&inbuf[5]));
						}
						break;
#endif

					case NETMSG_STARTPLAYING:
					{
						if (players[a]->hasRights && !gameStartTime)
							CheckForGameStart(true);
						break;
					}
					case NETMSG_RANDSEED:
					{
						if (players[a]->hasRights && !demoReader)
							serverNet->SendRandSeed(*(unsigned int*)(inbuf+1));
						break;
					}
					case NETMSG_TEAM:
					{
						//TODO update players[] and teams[] and send to hostif
						const unsigned player = (unsigned)inbuf[1];
						if (player != a)
						{
							log.Warning(format(WrongPlayer) %inbuf[0] %a %player);
						}
						else
						{
							const unsigned action = inbuf[2];
							const unsigned fromTeam = players[player]->team;
							unsigned numPlayersInTeam = 0;
							for (unsigned a = 0; a < MAX_PLAYERS; ++a)
								if (players[a] && players[a]->team == fromTeam)
									++numPlayersInTeam;
							
							switch (action)
							{
								case TEAMMSG_SELFD: {
									serverNet->SendSelfD(player);
									if (numPlayersInTeam <= 1 && teams[fromTeam])
									{
										teams[fromTeam].reset();
									}
									players[player]->team = 0;
									break;
								}
								case TEAMMSG_GIVEAWAY: {
									const unsigned toTeam = inbuf[3];
									serverNet->SendGiveAwayEverything(player, toTeam);
									if (numPlayersInTeam <= 1 && teams[fromTeam])
									{
										teams[fromTeam].reset();
									}
									players[player]->team = 0;
									break;
								}
								case TEAMMSG_RESIGN: {
									serverNet->SendResign(player);
									players[player]->team = 0;
									break;
								}
								case TEAMMSG_JOIN_TEAM: {
									unsigned newTeam = inbuf[3];
									if ((setup && !setup->fixedTeams) || demoReader || cheating)
									{
										serverNet->SendJoinTeam(player, newTeam);
										players[player]->team = newTeam;
										if (!teams[newTeam])
											teams[newTeam].reset(new GameTeam);
									}
									else
									{
										log.Warning(format(NoTeamChange) %players[player]->name % player);
									}
									break;
								}
								case TEAMMSG_TEAM_DIED: {
									// don't send to clients, they don't need it
									unsigned char team = inbuf[3];
									if (teams[team])
									{
										teams[fromTeam].reset();
										for (unsigned i = 0; i < MAX_PLAYERS; ++i)
										{
											if (players[i] && players[i]->team == team)
											{
												players[i]->team = 0;
											}
										}
									}
									break;
								}
								default: {
									log.Warning(format(UnknownTeammsg) %action %player);
								}
							}
							break;
						}
					}

					// CGameServer should never get these messages
					case NETMSG_GAMEID:
					case NETMSG_INTERNAL_SPEED:
					case NETMSG_ATTEMPTCONNECT:
					case NETMSG_MAPNAME:
						break;
					default:
						{
							log.Warning(format(UnknownNetmsg) %inbuf[0] %a);
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
			log.Message(format(PlayerLeft) %players[a]->name %" timeout");
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
			log.Message(format(PlayerLeft) %players[a]->name %" timeout");
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

/** @brief Generate a unique game identifier and sent it to all clients. */
void CGameServer::GenerateAndSendGameID()
{
	// This is where we'll store the ID temporarily.
	union {
		unsigned char charArray[16];
		unsigned int intArray[4];
	} gameID;

	// First and second dword are time based (current time and load time).
	gameID.intArray[0] = (unsigned) time(NULL);
	for (int i = 4; i < 12; ++i)
		gameID.charArray[i] = rng();

	CRC entropy;
	entropy.UpdateData((const unsigned char*)&lastTick, sizeof(lastTick));

	// Third dword is CRC of setupText (if there is a setup)
	// or pseudo random bytes (if there is no setup)
	if (setup != NULL) {
		CRC crc;
		crc.UpdateFile(setup->setupFileName);
		gameID.intArray[2] = crc.GetCRC();
	}

	gameID.intArray[3] = entropy.GetCRC();

	serverNet->SendGameID(gameID.charArray);
}

void CGameServer::CheckForGameStart(bool forced)
{
	assert(!gameStartTime);
	bool allReady = true;
	unsigned numDemoPlayers = demoReader ? demoReader->GetFileHeader().maxPlayerNum+1 : 0;
	
	if (setup)
	{
		unsigned start = numDemoPlayers;
#ifdef DEDICATED
		// Lobby-protocol doesn't support creating games without players inside
		// so in dedicated mode there will always be the host-player in the script
		// which doesn't exist and will never join, so skip it in this case
		if (setup && setup->myPlayerNum == start)
			start++;
#endif
		for (unsigned a = start; a < (unsigned)setup->numPlayers; a++) {
			if (!players[a] || !players[a]->readyToStart) {
				allReady = false;
				break;
			} else if (!teams[players[a]->team]->readyToStart && !demoReader)
			{
				allReady = false;
				break;
			}
		}
	}
	else
	{
		allReady = false;
		// no setup, wait for host to send NETMSG_STARTPLAYING
		// if this is the case, forced is true...
		if (forced)
		{
			// immediately start
			readyTime = SDL_GetTicks();
			rng.Seed(readyTime);
			StartGame();
		}
	}

	if (allReady || forced)
	{
		if (readyTime == 0) {
			readyTime = SDL_GetTicks();
			rng.Seed(readyTime);
			serverNet->SendStartPlaying(GameStartDelay);
		}
	}
	if (readyTime && (SDL_GetTicks() - readyTime) > GameStartDelay)
	{
		StartGame();
	}
}

void CGameServer::StartGame()
{
	gameStartTime = SDL_GetTicks();
	serverNet->Listening(false);

	for(unsigned a=0; a < MAX_PLAYERS; ++a) {
		if(players[a])
			serverNet->SendPlayerName(a, players[a]->name);
	}

	// make sure initial game speed is within allowed range and sent a new speed if not
	if(userSpeedFactor>maxUserSpeed)
	{
		serverNet->SendUserSpeed(SERVER_PLAYER, maxUserSpeed);
		userSpeedFactor = maxUserSpeed;
	}
	else if(userSpeedFactor<minUserSpeed)
	{
		serverNet->SendUserSpeed(SERVER_PLAYER, minUserSpeed);
		userSpeedFactor = minUserSpeed;
	}

	if (demoReader) {
		// the client told us to start a demo
		// no need to send startpos and startplaying since its in the demo
		log.Message(DemoStart);
		return;
	}

	GenerateAndSendGameID();
	if (setup) {
		for (unsigned a = 0; a < MAX_TEAMS; ++a)
		{
			if (teams[a])
				serverNet->SendStartPos(SERVER_PLAYER, a, 1, teams[a]->startpos.x, teams[a]->startpos.y, teams[a]->startpos.z);
		}
	}

	serverNet->SendRandSeed(rng());
	serverNet->SendStartPlaying(0);
	if (hostif)
	{
		hostif->SendStartPlaying();
	}
	timeLeft=0;
	lastTick = SDL_GetTicks()-1;
	CreateNewFrame(true, false);
}

void CGameServer::SetGamePausable(const bool arg)
{
	gamePausable = arg;
}

bool CGameServer::HasFinished() const
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex);
	return quitServer;
}

void CGameServer::CheckForGameEnd()
{
	if (gameEndTime > 0) {
		if (gameEndTime < SDL_GetTicks() - 2000) {
			log.Message(GameEnd);
			serverNet->SendGameOver();
			if (hostif) {
				hostif->SendGameOver();
			}
			sentGameOverMsg = true;
		}
		return;
	}

#ifndef DEDICATED
	unsigned numActiveTeams[MAX_TEAMS]; // active teams per ally team
	memset(numActiveTeams, 0, sizeof(numActiveTeams));
	unsigned numActiveAllyTeams = 0;

	for (unsigned a = 0; (int)a < gs->activeTeams; ++a)
		if (!gs->Team(a)->isDead && !gs->Team(a)->gaia)
			++numActiveTeams[gs->AllyTeam(a)];

	for (unsigned a = 0; (int)a < gs->activeAllyTeams; ++a)
		if (numActiveTeams[a] != 0)
			++numActiveAllyTeams;

	if (numActiveAllyTeams <= 1)
	{
		gameEndTime=SDL_GetTicks();
		serverNet->SendSendPlayerStat();
	}
#endif
}

void CGameServer::CreateNewFrame(bool fromServerThread, bool fixedFrameTime)
{
	boost::mutex::scoped_lock scoped_lock(gameServerMutex,!fromServerThread);
	CheckSync();
	int newFrames = 1;

	if(!fixedFrameTime){
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
		newFrames = (timeLeft > 0) ? ceil(timeLeft) : 0;
		timeLeft -= newFrames;
	}

	bool rec = false;
#ifndef NO_AVI
	rec = game && game->creatingVideo;
#endif
	bool normalFrame = !IsPaused && !rec;
	bool videoFrame = !IsPaused && fixedFrameTime;
	bool singleStep = fixedFrameTime && !rec;

	if(normalFrame || videoFrame || singleStep){
		for(int i=0; i < newFrames; ++i){
			++serverframenum;
			//Send out new frame messages.
			serverNet->SendNewFrame(serverframenum);
#ifdef SYNCCHECK
			outstandingSyncFrames.push_back(serverframenum);
#endif
		}
	}
}

void CGameServer::UpdateLoop()
{
	while (!quitServer)
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
	if (players[playerNum]) {
		log.Message(format(PlayerLeft) %playerNum %"kicked");
		serverNet->SendPlayerLeft(playerNum, 2);
		serverNet->SendQuit(playerNum);
		serverNet->Kill(playerNum);
		if (hostif)
		{
			hostif->SendPlayerLeft(playerNum, 2);
		}
		players[playerNum].reset();
	}
}

void CGameServer::BindConnection(unsigned wantedNumber, bool grantRights)
{
	unsigned hisNewNumber = wantedNumber;
	if (demoReader) {
		hisNewNumber = std::max(wantedNumber, (unsigned)demoReader->GetFileHeader().maxPlayerNum+1);
	}
	hisNewNumber = serverNet->AcceptIncomingConnection(hisNewNumber);

	serverNet->SendSetPlayerNum((unsigned char)hisNewNumber, (unsigned char)hisNewNumber);

	// send game data (in case he didn't know or for checksumming)
	serverNet->SendScript(scriptName);
	serverNet->SendMapName(mapChecksum, mapName);
	serverNet->SendModName(modChecksum, modName);

	for (unsigned a = 0; a < MAX_PLAYERS; ++a) {
		if(players[a] && players[a]->readyToStart)
			serverNet->SendPlayerName(a, players[a]->name);
	}

	players[hisNewNumber].reset(new GameParticipant(grantRights)); // give him rights to change speed, kick players etc
	if (setup && hisNewNumber < (unsigned)setup->numPlayers/* needed for non-hosted demo playback */)
	{
		unsigned hisTeam = setup->playerStartingTeam[hisNewNumber];
		if (!teams[hisTeam]) // create new team
		{
			teams[hisTeam].reset(new GameTeam());
			teams[hisTeam]->startpos = setup->startPos[hisTeam];
			teams[hisTeam]->readyToStart = (setup->startPosType != CGameSetupData::StartPos_ChooseInGame);
		}
		players[hisNewNumber]->team = hisTeam;
		serverNet->SendJoinTeam(hisNewNumber, hisTeam);
		for (unsigned a = 0; a < MAX_TEAMS; ++a)
		{
			if (teams[a])
				serverNet->SendStartPos(SERVER_PLAYER, a, 1, teams[a]->startpos.x, teams[a]->startpos.y, teams[a]->startpos.z);
		}
	}
	else
	{
		unsigned hisTeam = hisNewNumber;
		teams[hisTeam].reset(new GameTeam());
		players[hisNewNumber]->team = hisTeam;
		serverNet->SendJoinTeam(hisNewNumber, hisTeam);
		for (unsigned a = 0; a < MAX_TEAMS; ++a)
		{
			if (teams[a] && a != hisNewNumber)
				serverNet->SendJoinTeam(a, players[a]->team);
		}
	}
	log.Message(format(NewConnection) %hisNewNumber %wantedNumber);

	serverNet->FlushNet();
}

void CGameServer::GotChatMessage(const std::string& msg, unsigned player)
{
	bool canDoStuff = (player == SERVER_PLAYER || players[player]->hasRights);
	//TODO: migrate more stuff from CGame::HandleChatMessage here
	if ((msg.find(".kickbynum") == 0) && canDoStuff) {
		if (msg.length() >= 11) {
			int playerNum = atoi(msg.substr(11, string::npos).c_str());
			KickPlayer(playerNum);
		}
	}
	else if ((msg.find(".kick") == 0) && canDoStuff) {
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
	else if ((msg.find(".nopause") == 0) && canDoStuff) {
		SetBoolArg(gamePausable, msg, ".nopause");
	}
	else if ((msg.find(".nohelp") == 0) && canDoStuff) {
		SetBoolArg(noHelperAIs, msg, ".nohelp");
		// sent it because clients have to do stuff when this changes
		serverNet->SendChat(player, msg);
	}
	else if ((msg.find(".setmaxspeed") == 0) && canDoStuff) {
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
	else if ((msg.find(".setminspeed") == 0) && canDoStuff) {
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
	else if ((msg.find(".forcestart") == 0) && canDoStuff) {
		if (!gameStartTime)
			CheckForGameStart(true);
	}
	else if ((msg.find(".cheat") == 0) && canDoStuff) {
		cheating = !cheating;
		serverNet->SendChat(player, msg);
		if (hostif)
			hostif->SendPlayerChat(player, msg);
	}
	else {
		serverNet->SendChat(player, msg);
		if (hostif && player != SERVER_PLAYER) {
			// don't echo packets to autohost
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
