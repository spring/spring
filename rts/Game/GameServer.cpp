#include "GameServer.h"

#include <stdarg.h>
#include <ctime>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/version.hpp>
#include <SDL_timer.h>
#include <cmath>

#ifndef NO_AVI
#include "Game.h"
#endif

#include "GameSetupData.h"
#include "Action.h"
#include "ChatMessage.h"
#include "CommandMessage.h"
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

#if _MSC_VER 
namespace std { using ::ceil; }
#endif

/// frames until a syncchech will time out and a warning is given out
const int SYNCCHECK_TIMEOUT = 300;

/// used to prevent msg spam
const int SYNCCHECK_MSG_TIMEOUT = 400;

///msecs to wait until the game starts after all players are ready
const unsigned GameStartDelay = 3800;

/// The time intervall in msec for sending player statistics to each client
const unsigned playerInfoTime= 2000;

/// msecs to wait until the timeout condition (na active clients) activates
const unsigned serverTimeout = 30000;

/// every n'th frame will be a keyframe (and contain the server's framenumber)
const unsigned serverKeyframeIntervall = 16;

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
namespace {
void SetBoolArg(bool& value, const std::string& str)
{
	if (str.empty()) // toggle
	{
		value = !value;
	}
	else // set
	{
		const int num = atoi(str.c_str());
		value = (num != 0);
	}
}
}


CGameServer* gameServer=0;

CGameServer::CGameServer(int port, const GameData* const newGameData, const CGameSetupData* const mysetup = 0, const std::string& demoName)
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

	gameData.reset(newGameData);
	if (!demoName.empty())
	{
		log.Message(format(PlayingDemo) %demoName);
		demoReader = new CDemoReader(demoName, modGameTime+0.1f);
	}

	RestrictedAction("kick");			RestrictedAction("kickbynum");
	RestrictedAction("setminspeed");	RestrictedAction("setmaxspeed");
	RestrictedAction("nopause");
	RestrictedAction("nohelp");
	RestrictedAction("cheat"); //TODO register cheats only after cheating is on
	RestrictedAction("godmode");
	RestrictedAction("nocost");
	RestrictedAction("forcestart");
	RestrictedAction("nospectatorchat");
	if (demoReader)
		RegisterAction("skip");
	commandBlacklist.insert("skip");
	RestrictedAction("reloadcob");
	RestrictedAction("devlua");
	RestrictedAction("editdefs");
	RestrictedAction("luarules");
	RestrictedAction("luagaia");
	RestrictedAction("singlestep");
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

void CGameServer::AddLocalClient()
{
	boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
	serverNet->ServerInitLocalClient();
}

void CGameServer::AddAutohostInterface(const int remotePort)
{
	if (hostif == 0)
	{
		boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
		hostif = new AutohostInterface(remotePort);
		hostif->SendStart();
		log.Subscribe(hostif);
		log.Message(format(ConnectAutohost) %remotePort);
	}
}

void CGameServer::PostLoad(unsigned newlastTick, int newserverframenum)
{
	boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
	lastTick = newlastTick;
	serverframenum = newserverframenum;
}

void CGameServer::SkipTo(int targetframe)
{
	if (targetframe > serverframenum && demoReader)
	{
		CommandMessage msg(str( boost::format("skip start %d") %targetframe ), SERVER_PLAYER);
		serverNet->SendData(msg.Pack());
		// fast-read and send demo data
		while (serverframenum < targetframe)
		{
			modGameTime = demoReader->GetNextReadTime()+0.1f; // skip time
			SendDemoData(true);
			if (serverframenum % 20 == 0)
				serverNet->Update(); // send some data
		}
		CommandMessage msg2("skip end", SERVER_PLAYER);
		serverNet->SendData(msg2.Pack());
		serverNet->Update();
	}
	else
	{
		// allready passed
	}
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

void CGameServer::SendDemoData(const bool skipping)
{
	RawPacket* buf = 0;
	while ( (buf = demoReader->GetData(modGameTime)) )
	{
		unsigned msgCode = static_cast<unsigned>(buf->data[0]);
		if (msgCode == NETMSG_NEWFRAME || msgCode == NETMSG_KEYFRAME)
		{
			// we can't use CreateNewFrame() here
			lastTick = SDL_GetTicks();
			serverframenum++;
#ifdef SYNCCHECK
			if (!skipping)
				outstandingSyncFrames.push_back(serverframenum);
#endif
			serverNet->SendData(buf);
		}
		else if ( msgCode != NETMSG_GAMEDATA &&
						msgCode != NETMSG_SETPLAYERNUM &&
						msgCode != NETMSG_USER_SPEED &&
						msgCode != NETMSG_INTERNAL_SPEED &&
						msgCode != NETMSG_PAUSE) // dont send these from demo
		{
			if (msgCode == NETMSG_GAMEOVER)
				sentGameOverMsg = true;
			serverNet->SendData(buf);
		}
	}

	if (demoReader->ReachedEnd()) {
		delete demoReader;
		demoReader = 0;
		log.Message(DemoEnd);
		gameEndTime = SDL_GetTicks();
	}
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
		for (int a = 0; a < MAX_PLAYERS; ++a) {
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
			for (int a = 0; a < MAX_PLAYERS; ++a) {
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

	if(lastPlayerInfo < (SDL_GetTicks() - playerInfoTime)){
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
	if (demoReader != 0)
	{
		CheckSync();
		SendDemoData();
	}

	ServerReadNet();

	if (!gameStartTime)
	{
		CheckForGameStart();
	}
	else if (serverframenum > 0 && !demoReader)
	{
		CreateNewFrame(true, false);
		if (!sentGameOverMsg)
			CheckForGameEnd();
	}
	serverNet->Update();

	if (hostif)
	{
		std::string msg = hostif->GetChatMessage();

		if (!msg.empty())
		{
			if (msg.at(0) != '/') // normal chat message
			{
				GotChatMessage(ChatMessage(SERVER_PLAYER, ChatMessage::TO_EVERYONE, msg));
			}
			else if (msg.at(0) == '/' && msg.size() > 1 && msg.at(1) == '/') // chatmessage with prefixed '/'
			{
				GotChatMessage(ChatMessage(SERVER_PLAYER, ChatMessage::TO_EVERYONE, msg.substr(1)));
			}
			else if (msg.size() > 1) // command
			{
				Action buf(msg.substr(1));
				PushAction(buf);
			}
		}
	}
	
	if ((SDL_GetTicks() - serverStartTime) > serverTimeout && serverNet->MaxConnectionID() == -1)
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
			if (packet->length >= 3) {
				log.Warning(format(ConnectionReject) %inbuf[0] %inbuf[2] %packet->length);
			}
			else {
				log.Warning("Connection attempt rejected: Packet too short");
			}
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
					case NETMSG_KEYFRAME:
						players[a]->ping = serverframenum-*(int*)&inbuf[1];
						break;

					case NETMSG_PAUSE:
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]);
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
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %playerNum);
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

					case NETMSG_CHAT: {
						ChatMessage msg(*packet);
						if (static_cast<unsigned>(msg.fromPlayer) != a ) {
							log.Warning(format(WrongPlayer) %(unsigned)NETMSG_CHAT %a %(unsigned)msg.fromPlayer);
						} else {
							GotChatMessage(msg);
						}
						break;
					}
					case NETMSG_SYSTEMMSG:
						if(inbuf[2]!=a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[2]);
						} else {
							serverNet->SendSystemMessage(inbuf[2], (char*)(&inbuf[3]));
						}
						break;

					case NETMSG_STARTPOS:
						if(inbuf[1] != a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]);
						}
						else if (setup && setup->startPosType == CGameSetupData::StartPos_ChooseInGame)
						{
							unsigned team = (unsigned)inbuf[2];
							if (teams[team])
							{
								teams[team]->startpos = SFloat3(*((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12]));
								teams[team]->readyToStart = static_cast<bool>(inbuf[3]);
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
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]);
						} else {
							if (!demoReader)
								serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_SELECT:
						if(inbuf[3]!=a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]);
						} else {
							if (!demoReader)
								// forward data
								serverNet->RawSend(inbuf,*((short int*)&inbuf[1]));
						}
						break;


					case NETMSG_AICOMMAND: {
						if (inbuf[3] != a) {
							log.Warning(format(WrongPlayer) %(unsigned) inbuf[0]  %a  %(unsigned) inbuf[3]);
						}
						else if (noHelperAIs) {
							log.Warning(format(NoHelperAI) %players[a]->name %a);
						}
						else if (!demoReader) {
							// forward data
							serverNet->RawSend(inbuf, *((short int*) &inbuf[1]));
						}
					} break;

					case NETMSG_AICOMMANDS: {
						if (inbuf[3] != a) {
							log.Warning(format(WrongPlayer) %(unsigned) inbuf[0]  %a  %(unsigned) inbuf[3]);
						}
						else if (noHelperAIs) {
							log.Warning(format(NoHelperAI) %players[a]->name %a);
						}
						else if (!demoReader) {
							// forward data
							serverNet->RawSend(inbuf, *((short int*) &inbuf[1]));
						}
					} break;

					case NETMSG_AISHARE: {
						if (inbuf[3] != a) {
							log.Warning(format(WrongPlayer) %(unsigned) inbuf[0]  %a  %(unsigned) inbuf[3]);
						} else if (noHelperAIs) {
							log.Warning(format(NoHelperAI) %players[a]->name %a);
						} else if (!demoReader) {
							// forward data
							serverNet->RawSend(inbuf, *((short int*) &inbuf[1]));
						}
					} break;


					case NETMSG_LUAMSG:
						if(inbuf[3]!=a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]);
						}
						else if (!demoReader) {
							serverNet->RawSend(inbuf,*((short int*)&inbuf[1])); //forward data
						}
						break;

					case NETMSG_SYNCRESPONSE:
#ifdef SYNCCHECK
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]);
						} else {
							int frameNum = *(int*)&inbuf[2];
							if (outstandingSyncFrames.empty() || frameNum >= outstandingSyncFrames.front())
								players[a]->syncResponse[frameNum] = *(unsigned*)&inbuf[6];
							else if (serverframenum - delayedSyncResponseFrame > SYNCCHECK_MSG_TIMEOUT) {
								delayedSyncResponseFrame = serverframenum;
								log.Warning(format(DelayedSyncResponse) %players[a]->name %frameNum %serverframenum);
							}
							// update players' ping (if !defined(SYNCCHECK) this is done in NETMSG_KEYFRAME)
							players[a]->ping = serverframenum - frameNum;
						}
#endif
						break;

					case NETMSG_SHARE:
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]);
						} else {
							if (!demoReader)
								serverNet->SendShare(inbuf[1], inbuf[2], inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8]));
						}
						break;

					case NETMSG_SETSHARE:
						if(inbuf[1]!= a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]);
						} else {
							if (!demoReader)
								serverNet->SendSetShare(inbuf[1], inbuf[2], *((float*)&inbuf[3]), *((float*)&inbuf[7]));
						}
						break;

					case NETMSG_PLAYERSTAT:
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]);
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
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]);
						} else {
							if (!demoReader)
								serverNet->SendDirectControl(inbuf[1]);
						}
						break;

					case NETMSG_DC_UPDATE:
						if(inbuf[1]!=a){
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]);
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
					case NETMSG_TEAM:
					{
						//TODO update players[] and teams[] and send to hostif
						const unsigned player = (unsigned)inbuf[1];
						if (player != a)
						{
							log.Warning(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)player);
						}
						else
						{
							const unsigned action = inbuf[2];
							const unsigned fromTeam = players[player]->team;
							unsigned numPlayersInTeam = 0;
							for (int a = 0; a < MAX_PLAYERS; ++a)
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
									if (cheating)
									{
										serverNet->SendJoinTeam(player, newTeam);
										players[player]->team = newTeam;
										if (!teams[newTeam])
											teams[newTeam].reset(new GameTeam);
									}
									else
									{
										log.Warning(format(NoTeamChange) %players[player]->name %player);
									}
									break;
								}
								case TEAMMSG_TEAM_DIED: {
									// don't send to clients, they don't need it
									unsigned char team = inbuf[3];
									if (teams[team] && players[player]->hasRights) // currently only host is allowed
									{
										teams[fromTeam].reset();
										for (int i = 0; i < MAX_PLAYERS; ++i)
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
					case NETMSG_ALLIANCE: {
						const unsigned char player = inbuf[1];
						const unsigned char whichAllyTeam = inbuf[2];
						const unsigned char allied = inbuf[3];
						if (setup && !setup->fixedAllies)
						{
							serverNet->SendSetAllied(player, whichAllyTeam, allied);
						}
						else
						{ // not allowed
						}
						break;
					}
					case NETMSG_CCOMMAND: {
						CommandMessage msg(*packet);
						if (static_cast<unsigned>(msg.player) == a)
						{
							if ((commandBlacklist.find(msg.action.command) != commandBlacklist.end()) && players[a]->hasRights)
							{
								// command is restricted to server but player is allowed to execute it
								PushAction(msg.action);
							}
							else if (commandBlacklist.find(msg.action.command) == commandBlacklist.end())
							{
								// command is save
								serverNet->SendData(msg.Pack());
							}
							else
							{
								// hack!
								log.Warning(boost::format(CommandNotAllowed) %msg.player %msg.action.command.c_str());
							}
						}
						break;
					}

					// CGameServer should never get these messages
					case NETMSG_GAMEID:
					case NETMSG_INTERNAL_SPEED:
					case NETMSG_ATTEMPTCONNECT:
					case NETMSG_GAMEDATA:
					case NETMSG_RANDSEED:
						break;
					default:
						{
							log.Warning(format(UnknownNetmsg) %(unsigned)inbuf[0] %a);
						}
						break;
				}
				delete packet;
			}
		}
		else if (players[a])
		{
			log.Message(format(PlayerLeft) %players[a]->name %" timeout"); //this must happen BEFORE the reset!
			players[a].reset();
			serverNet->SendPlayerLeft(a, 0);
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
			log.Message(format(PlayerLeft) %players[a]->name %" timeout"); //this must happen BEFORE the reset!
			players[a].reset();
			serverNet->SendPlayerLeft(a, 0);
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

	// Third dword is CRC of setupText (if there is a setup)
	// or pseudo random bytes (if there is no setup)
	if (setup != NULL) {
		CRC crc;
		crc.Update(setup->gameSetupText, setup->gameSetupTextLength);
		gameID.intArray[2] = crc.GetDigest();
	}

	CRC entropy;
	entropy.Update(lastTick);
	gameID.intArray[3] = entropy.GetDigest();

	serverNet->SendGameID(gameID.charArray);
}

void CGameServer::CheckForGameStart(bool forced)
{
	assert(!gameStartTime);
	bool allReady = true;
	
	if (setup)
	{
		unsigned numDemoPlayers = demoReader ? demoReader->GetFileHeader().maxPlayerNum+1 : 0;
		unsigned start = numDemoPlayers;
#ifdef DEDICATED
		// Lobby-protocol doesn't support creating games without players inside
		// so in dedicated mode there will always be the host-player in the script
		// which doesn't exist and will never join, so skip it in this case
		if (setup && (unsigned)setup->myPlayerNum == start)
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

	for(int a=0; a < MAX_PLAYERS; ++a) {
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
		for (int a = 0; a < setup->numTeams; ++a)
		{
			if (teams[a]) // its a player
				serverNet->SendStartPos(SERVER_PLAYER, a, 1, teams[a]->startpos.x, teams[a]->startpos.y, teams[a]->startpos.z);
			else // maybe an AI?
				serverNet->SendStartPos(SERVER_PLAYER, a, 1, setup->startPos[a].x, setup->startPos[a].y, setup->startPos[a].z);
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

void CGameServer::PushAction(const Action& action)
{
	boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
	if (action.command == "kickbynum")
	{
		if (!action.extra.empty())
		{
			const int playerNum = atoi(action.extra.c_str());
			KickPlayer(playerNum);
		}
	}
	else if (action.command == "kick")
	{
		if (!action.extra.empty())
		{
			std::string name = action.extra;
			StringToLowerInPlace(name);
			for (int a=1; a < MAX_PLAYERS;++a)
			{
				if (players[a])
				{
					std::string playerLower = StringToLower(players[a]->name);
					if (playerLower.find(name)==0)
					{	//can kick on substrings of name
						KickPlayer(a);
					}
				}
			}
		}
	}
	else if (action.command == "nopause")
	{
		SetBoolArg(gamePausable, action.extra);
	}
	else if (action.command == "nohelp")
	{
		SetBoolArg(noHelperAIs, action.extra);
		// sent it because clients have to do stuff when this changes
		CommandMessage msg(action, SERVER_PLAYER);
		serverNet->SendData(msg.Pack());
	}
	else if (action.command == "setmaxspeed")
	{
		maxUserSpeed = atof(action.extra.c_str());
		if (userSpeedFactor > maxUserSpeed) {
			serverNet->SendUserSpeed(SERVER_PLAYER, maxUserSpeed);
			userSpeedFactor = maxUserSpeed;
			if (internalSpeed > maxUserSpeed) {
				serverNet->SendInternalSpeed(userSpeedFactor);
				internalSpeed = userSpeedFactor;
			}
		}
	}
	else if (action.command == "setminspeed")
	{
		minUserSpeed = atof(action.extra.c_str());
		if (userSpeedFactor < minUserSpeed) {
			serverNet->SendUserSpeed(SERVER_PLAYER, minUserSpeed);
			userSpeedFactor = minUserSpeed;
			if (internalSpeed < minUserSpeed) {
				serverNet->SendInternalSpeed(userSpeedFactor);
				internalSpeed = userSpeedFactor;
			}
		}
	}
	else if (action.command == "forcestart")
	{
		if (!gameStartTime)
			CheckForGameStart(true);
	}
	else if (action.command == "skip")
	{
		if (demoReader && serverframenum > 1)
		{
			const std::string timeStr = action.extra;
			int endFrame;
			// parse the skip time
			if (timeStr[0] == 'f') {        // skip to frame
				endFrame = atoi(timeStr.c_str() + 1);
			} else if (timeStr[0] == '+') { // relative time
				endFrame = serverframenum + (GAME_SPEED * atoi(timeStr.c_str() + 1));
			} else {                        // absolute time
				endFrame = GAME_SPEED * atoi(timeStr.c_str());
			}
			SkipTo(endFrame);
		}
	}
	else if (action.command == "cheat")
	{
		SetBoolArg(cheating, action.extra);
		CommandMessage msg(action, SERVER_PLAYER);
		serverNet->SendData(msg.Pack());
	}
	else if (action.command == "singlestep")
	{
		if (IsPaused && !demoReader)
			gameServer->CreateNewFrame(true, true);
	}
	else
	{
		// only forward to players (send over network)
		CommandMessage msg(action, SERVER_PLAYER);
		serverNet->SendData(msg.Pack());
	}
}

bool CGameServer::HasFinished() const
{
	boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
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
	{
		bool hasPlayer = false;
		for (int b = 0; b < gs->activePlayers; ++b) {
			if (gs->players[b]->active && gs->players[b]->team==a && !gs->players[b]->spectator) {
				hasPlayer = true;
			}
		}
		if (!setup || !setup->aiDlls[a].empty())
			hasPlayer = true;

		if (!gs->Team(a)->isDead && !gs->Team(a)->gaia && hasPlayer)
			++numActiveTeams[gs->AllyTeam(a)];
	}

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
#if BOOST_VERSION >= 103500
	if (!fromServerThread)
		boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex, boost::defer_lock);
	else
		boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
#else
	boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex,!fromServerThread);
#endif
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

		timeLeft += GAME_SPEED * internalSpeed * float(timeElapsed) / 1000.0f;
		lastTick=currentTick;
		newFrames = (timeLeft > 0)? int(std::ceil(timeLeft)): 0;
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
			if (0 == (serverframenum % serverKeyframeIntervall))
				serverNet->SendKeyFrame(serverframenum);
			else
				serverNet->SendNewFrame();
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
			boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
			Update();
		}
		SDL_Delay(10);
	}
}

bool CGameServer::WaitsOnCon() const
{
	return serverNet->Listening();
}

bool CGameServer::GameHasStarted() const
{
	return (gameStartTime>0);
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

void CGameServer::BindConnection(unsigned wantedNumber)
{
	unsigned hisNewNumber = wantedNumber;
	if (demoReader) {
		hisNewNumber = std::max(wantedNumber, (unsigned)demoReader->GetFileHeader().maxPlayerNum+1);
	}
	hisNewNumber = serverNet->AcceptIncomingConnection(hisNewNumber);

	serverNet->SendSetPlayerNum((unsigned char)hisNewNumber, (unsigned char)hisNewNumber);
	static const int pregameRandomSeed = rng();
	serverNet->SendRandSeed(pregameRandomSeed, static_cast<int>(hisNewNumber));
	serverNet->SendData(gameData->Pack(), hisNewNumber);

	for (int a = 0; a < MAX_PLAYERS; ++a) {
		if(players[a] && players[a]->readyToStart)
			serverNet->SendPlayerName(a, players[a]->name);
	}

	// is this is the local player (== host) then he can kick, set options etc.
	bool grantRights = setup ? (hisNewNumber == (unsigned)setup->myPlayerNum) : (wantedNumber == 0);
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
		for (int a = 0; a < MAX_TEAMS; ++a)
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
		for (int a = 0; a < MAX_TEAMS; ++a)
		{
			if (teams[a] && a != (int)hisNewNumber)
				serverNet->SendJoinTeam(a, players[a]->team);
		}
	}
	log.Message(format(NewConnection) %hisNewNumber %wantedNumber);

	serverNet->FlushNet();
}

void CGameServer::GotChatMessage(const ChatMessage& msg)
{
	serverNet->SendData(msg.Pack());
	if (hostif && static_cast<unsigned>(msg.fromPlayer) != SERVER_PLAYER) {
		// don't echo packets to autohost
		hostif->SendPlayerChat(msg.fromPlayer, msg.msg);
	}
}

void CGameServer::RestrictedAction(const std::string& action)
{
	RegisterAction(action);
	commandBlacklist.insert(action);
}

