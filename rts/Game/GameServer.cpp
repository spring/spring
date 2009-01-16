#include "StdAfx.h"
#include "Rendering/GL/myGL.h"


#include <stdarg.h>
#include <ctime>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/version.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>
#include <SDL_timer.h>
#if defined DEDICATED || defined DEBUG
#include <iostream>
#endif
#include <stdlib.h> // why is this here?

#include "mmgr.h"

#include "GameServer.h"

#ifndef NO_AVI
#include "Game.h"
#endif

#include "LogOutput.h"
#include "GameSetup.h"
#include "Action.h"
#include "ChatMessage.h"
#include "CommandMessage.h"
#include "BaseNetProtocol.h"
#include "PlayerHandler.h"
#include "Net/UDPListener.h"
#include "Net/Connection.h"
#include "Net/UDPConnection.h"
#include "Net/LocalConnection.h"
#include "Net/UnpackPacket.h"
#include "DemoReader.h"
#include "AutohostInterface.h"
#include "Util.h"
#include "GlobalUnsynced.h" // for syncdebug
#include "ConfigHandler.h"
#include "FileSystem/CRC.h"
#include "Player.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Server/MsgStrings.h"


using netcode::RawPacket;


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

GameParticipant::GameParticipant()
: myState(UNCONNECTED)
, cpuUsage (0.0f)
, ping (0)
, isLocal(false)
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

CGameServer::CGameServer(const LocalSetup* settings, bool onlyLocal, const GameData* const newGameData, const CGameSetup* const mysetup)
: setup(mysetup)
{
	assert(setup);
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
	hasLocalClient = false;
	localClientNumber = 0;
	isPaused = false;
	userSpeedFactor = 1.0f;
	internalSpeed = 1.0f;
	gamePausable = true;
	noHelperAIs = false;
	cheating = false;
	sentGameOverMsg = false;

	medianCpu=0.0f;
	medianPing=0;
	enforceSpeed=configHandler.Get("EnforceGameSpeed", false);

	if (!onlyLocal)
		UDPNet.reset(new netcode::UDPListener(settings->hostport));

	if (settings->autohostport > 0) {
		AddAutohostInterface(settings->autohostport);
	}
	rng.Seed(SDL_GetTicks());
	Message(str( format(ServerStart) %settings->hostport) );

	lastTick = SDL_GetTicks();

	maxUserSpeed = setup->maxSpeed;
	minUserSpeed = setup->minSpeed;
	noHelperAIs = (bool)setup->noHelperAIs;

	gameData.reset(newGameData);
	if (setup->hostDemo)
	{
		Message(str( format(PlayingDemo) %setup->demoName ));
		demoReader.reset(new CDemoReader(setup->demoName, modGameTime+0.1f));
	}

	players.resize(setup->playerStartingData.size());
	std::copy(setup->playerStartingData.begin(), setup->playerStartingData.end(), players.begin());

	RestrictedAction("kick");			RestrictedAction("kickbynum");
	RestrictedAction("setminspeed");	RestrictedAction("setmaxspeed");
	RestrictedAction("nopause");
	RestrictedAction("nohelp");
	RestrictedAction("cheat"); //TODO register cheats only after cheating is on
	RestrictedAction("godmode");
	RestrictedAction("globallos");
	RestrictedAction("nocost");
	RestrictedAction("forcestart");
	RestrictedAction("nospectatorchat");
	if (demoReader)
		RegisterAction("skip");
	commandBlacklist.insert("skip");
	RestrictedAction("reloadcob");
	RestrictedAction("devlua");
	RestrictedAction("editdefs");
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
}

void CGameServer::AddLocalClient(const std::string& myName, const std::string& myVersion)
{
	boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
	assert(!hasLocalClient);
	hasLocalClient = true;
	localClientNumber = BindConnection(myName, myVersion, true, boost::shared_ptr<netcode::CConnection>(new netcode::CLocalConnection()));
}

void CGameServer::AddAutohostInterface(const int remotePort)
{
	if (!hostif)
	{
		hostif.reset(new AutohostInterface(remotePort));
		hostif->SendStart();
		Message(str(format(ConnectAutohost) %remotePort));
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
		Broadcast(boost::shared_ptr<const netcode::RawPacket>(msg.Pack()));
		// fast-read and send demo data
		while (serverframenum < targetframe && demoReader)
		{
			modGameTime = demoReader->GetNextReadTime()+0.1f; // skip time
			SendDemoData(true);
			if (serverframenum % 20 == 0 && UDPNet)
				UDPNet->Update(); // send some data (otherwise packets will grow too big)
		}
		CommandMessage msg2("skip end", SERVER_PLAYER);
		Broadcast(boost::shared_ptr<const netcode::RawPacket>(msg2.Pack()));

		if (UDPNet)
			UDPNet->Update();
		lastUpdate = SDL_GetTicks();
		isPaused = true;
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
		playerstring += players[*p].name;
	}
	return playerstring;
}

void CGameServer::SendDemoData(const bool skipping)
{
	netcode::RawPacket* buf = 0;
	while ( (buf = demoReader->GetData(modGameTime)) )
	{
		unsigned msgCode = buf->data[0];
		if (msgCode == NETMSG_NEWFRAME || msgCode == NETMSG_KEYFRAME)
		{
			// we can't use CreateNewFrame() here
			lastTick = SDL_GetTicks();
			serverframenum++;
#ifdef SYNCCHECK
			if (!skipping)
				outstandingSyncFrames.push_back(serverframenum);
			CheckSync();
#endif
			Broadcast(boost::shared_ptr<const RawPacket>(buf));
		}
		else if ( msgCode != NETMSG_GAMEDATA &&
						msgCode != NETMSG_SETPLAYERNUM &&
						msgCode != NETMSG_USER_SPEED &&
						msgCode != NETMSG_INTERNAL_SPEED &&
						msgCode != NETMSG_PAUSE) // dont send these from demo
		{
			if (msgCode == NETMSG_GAMEOVER)
				sentGameOverMsg = true;
			Broadcast(boost::shared_ptr<const RawPacket>(buf));
		}
	}

	if (demoReader->ReachedEnd()) {
		demoReader.reset();
		Message(DemoEnd);
		gameEndTime = SDL_GetTicks();
	}
}

void CGameServer::Broadcast(boost::shared_ptr<const netcode::RawPacket> packet)
{
	for (unsigned p = 0; p < players.size(); ++p)
	{
		if (players[p].link)
			players[p].link->SendData(packet);
	}
}

void CGameServer::Message(const std::string& message)
{
	Warning(message);
}

void CGameServer::Warning(const std::string& message)
{
	Broadcast(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
	if (hostif)
		hostif->Message(message);
#if defined DEDICATED || defined DEBUG
	std::cout << message << std::endl;
#endif
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
		for (unsigned a = 0; a < players.size(); ++a) {
			if (!players[a].link)
				continue;
			std::map<int, unsigned>::iterator it = players[a].syncResponse.find(*f);
			if (it == players[a].syncResponse.end()) {
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
				Warning(str(format(NoSyncResponse) %players %(*f)));
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
#ifdef SYNCDEBUG
				CSyncDebugger::GetInstance()->ServerTriggerSyncErrorHandling(serverframenum);
				Broadcast(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, true));
				isPaused = true;
				Broadcast(CBaseNetProtocol::Get().SendSdCheckrequest(serverframenum));
#endif
				//For each group, output a message with list of playernames in it.
				// TODO this should be linked to the resync system so it can roundrobin
				// the resync checksum request packets to multiple clients in the same group.
				std::map<unsigned, std::vector<int> >::const_iterator g = desyncGroups.begin();
				for (; g != desyncGroups.end(); ++g) {
					std::string players = GetPlayerNames(g->second);
					Warning(str(format(SyncError) %players %(*f) %(g->first ^ correctChecksum)));
				}
			}
		}

		// Remove complete sets (for which all player's checksums have been received).
		if (bComplete) {
// 			if (*f >= serverframenum - SYNCCHECK_TIMEOUT)
// 				logOutput.Print("Succesfully purged outstanding sync frame %d from the deque", *f);
			for (unsigned a = 0; a < players.size(); ++a) {
				if (players[a].myState >= GameParticipant::DISCONNECTED)
					players[a].syncResponse.erase(*f);
			}
			f = outstandingSyncFrames.erase(f);
		} else
			++f;
	}
#else
	// Make it clear this build isn't suitable for release.
	if (!syncErrorFrame || (serverframenum - syncErrorFrame > SYNCCHECK_MSG_TIMEOUT)) {
		syncErrorFrame = serverframenum;
		Warning(NoSyncCheck);
	}
#endif
}

void CGameServer::Update()
{
	if (!isPaused && gameStartTime > 0)
	{
		modGameTime += float(SDL_GetTicks() - lastUpdate) * 0.001f * internalSpeed;
	}
	lastUpdate = SDL_GetTicks();

	if(lastPlayerInfo < (SDL_GetTicks() - playerInfoTime)){
		lastPlayerInfo = SDL_GetTicks();

		if (serverframenum > 0) {
			//send info about the players
			int curpos=0;
			int ping[MAX_PLAYERS];
			float cpu[MAX_PLAYERS];
			float refCpu=0.0f;
			for (unsigned a = 0; a < players.size(); ++a) {
				if (players[a].myState >= GameParticipant::INGAME) {
					Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, players[a].cpuUsage, players[a].ping));
					if (players[a].cpuUsage > refCpu)
						refCpu = players[a].cpuUsage;
					cpu[curpos]=players[a].cpuUsage;
					ping[curpos]=players[a].ping;
					++curpos;
				}
			}

			medianCpu=0.0f;
			medianPing=0;
			if(enforceSpeed && curpos>0) {
				std::sort(cpu,cpu+curpos);
				std::sort(ping,ping+curpos);

				int midpos=curpos/2;
				medianCpu=cpu[midpos];
				medianPing=ping[midpos];
				if(midpos*2==curpos) {
					medianCpu=(medianCpu+cpu[midpos-1])/2.0f;
					medianPing=(medianPing+ping[midpos-1])/2;
				}
				refCpu=medianCpu;
			}

			if (refCpu > 0.0f) {
				// aim for 60% cpu usage if median is used as reference and 75% cpu usage if max is the reference
				float wantedCpu=enforceSpeed ? 0.6f+(1-internalSpeed/userSpeedFactor)*0.5f : 0.75f+(1-internalSpeed/userSpeedFactor)*0.5f;
				float newSpeed=internalSpeed*wantedCpu/refCpu;
//				float speedMod=1+wantedCpu-refCpu;
//				logOutput.Print("Speed REF %f MED %f WANT %f SPEEDM %f NSPEED %f",refCpu,medianCpu,wantedCpu,speedMod,newSpeed);
				newSpeed = (newSpeed+internalSpeed)*0.5f;
				newSpeed = std::max(newSpeed, enforceSpeed ? userSpeedFactor*0.8f : userSpeedFactor*0.5f);
				if(newSpeed>userSpeedFactor)
					newSpeed=userSpeedFactor;
				if(newSpeed<0.1f)
					newSpeed=0.1f;
				if(newSpeed!=internalSpeed)
					InternalSpeedChange(newSpeed);
			}
		}
	}

	if (!gameStartTime)
	{
		CheckForGameStart();
	}
	else if (serverframenum > 0 || demoReader)
	{
		CreateNewFrame(true, false);
		if (!sentGameOverMsg && !demoReader)
			CheckForGameEnd();
	}

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

	if (SDL_GetTicks() > serverTimeout + serverStartTime || gameStartTime)
	{
		bool hasPlayers = false;
		for (unsigned i = 0; i < players.size(); ++i)
		{
			if (players[i].link)
			{
				hasPlayers = true;
				break;
			}
		}

		if (!hasPlayers)
		{
			Message(NoClientsExit);
			quitServer = true;
		}
	}
}

void CGameServer::ProcessPacket(const unsigned playernum, boost::shared_ptr<const netcode::RawPacket> packet)
{
	const unsigned char* inbuf = packet->data;
	const unsigned a = playernum;

	switch (inbuf[0]){
		case NETMSG_KEYFRAME:
			players[a].ping = serverframenum-*(int*)&inbuf[1];
			break;

		case NETMSG_PAUSE:
			if(inbuf[1]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!inbuf[2])  // reset sync checker
					syncErrorFrame = 0;
				if(gamePausable || players[a].isLocal) // allow host to pause even if nopause is set
				{
					if(enforceSpeed && (players[a].cpuUsage - medianCpu > 0.25f || players[a].ping - medianPing > internalSpeed*GAME_SPEED/2)) {
						GotChatMessage(ChatMessage(a, a, "Pausing rejected (cpu load or ping is too high)"));
						break; // disallow pausing by players who cannot keep up gamespeed
					}
					timeLeft=0;
					if (isPaused != !!inbuf[2]) {
						isPaused = !isPaused;
					}
					Broadcast(CBaseNetProtocol::Get().SendPause(inbuf[1],inbuf[2]));
				}
			}
			break;

		case NETMSG_USER_SPEED: {
			unsigned char playerNum = inbuf[1];
			float speed = *((float*) &inbuf[2]);
			UserSpeedChange(speed, playerNum);
		} break;

		case NETMSG_CPU_USAGE:
			players[a].cpuUsage = *((float*)&inbuf[1]);
			break;

		case NETMSG_QUIT: {
			Message(str(format(PlayerLeft) %players[a].name %" normal quit"));
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 1));
			players[a].myState = GameParticipant::DISCONNECTED;
			players[a].link.reset();
			if (hostif)
			{
				hostif->SendPlayerLeft(a, 1);
			}
			break;
		}

		case NETMSG_PLAYERNAME: {
			unsigned playerNum = inbuf[2];
			if(playerNum!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %playerNum));
			} else {
				players[playerNum].name = (std::string)((char*)inbuf+3);
				players[playerNum].myState = GameParticipant::INGAME;
				Message(str(format(PlayerJoined) %players[playerNum].name));
				Broadcast(CBaseNetProtocol::Get().SendPlayerName(playerNum, players[playerNum].name));
				if (hostif)
				{
					hostif->SendPlayerJoined(playerNum, players[playerNum].name);
				}
			}
			break;
		}

		case NETMSG_CHAT: {
			ChatMessage msg(packet);
			if (msg.fromPlayer != a ) {
				Warning(str(format(WrongPlayer) %(unsigned)NETMSG_CHAT %a %(unsigned)msg.fromPlayer));
			} else {
				GotChatMessage(msg);
			}
			break;
		}
		case NETMSG_SYSTEMMSG:
			if(inbuf[2]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[2]));
			} else {
				Broadcast(CBaseNetProtocol::Get().SendSystemMessage(inbuf[2], (char*)(&inbuf[3])));
			}
			break;

		case NETMSG_STARTPOS:
			if(inbuf[1] != a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			}
			else if (setup->startPosType == CGameSetup::StartPos_ChooseInGame)
			{
				unsigned team = (unsigned)inbuf[2];
				if (teams[team])
				{
					teams[team]->startpos = SFloat3(*((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12]));
					if (inbuf[3] == 1)
						teams[team]->readyToStart = static_cast<bool>(inbuf[3]);
				}
				Broadcast(CBaseNetProtocol::Get().SendStartPos(inbuf[1],team, inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12]))); //forward data
				if (hostif)
				{
					hostif->SendPlayerReady(a, inbuf[3]);
				}
			}
			else
			{
				Warning(str(format(NoStartposChange) %a));
			}
			break;

		case NETMSG_COMMAND:
			if(inbuf[3]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]));
			} else {
				if (!demoReader)
					Broadcast(packet); //forward data
			}
			break;

		case NETMSG_SELECT:
			if(inbuf[3]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]));
			} else {
				if (!demoReader)
					Broadcast(packet); //forward data
			}
			break;

		case NETMSG_AICOMMAND: {
			if (inbuf[3] != a) {
				Warning(str(format(WrongPlayer) %(unsigned) inbuf[0]  %a  %(unsigned) inbuf[3]));
			}
			else if (noHelperAIs) {
				Warning(str(format(NoHelperAI) %players[a].name %a));
			}
			else if (!demoReader) {
				Broadcast(packet); //forward data
			}
		} break;

		case NETMSG_AICOMMANDS: {
			if (inbuf[3] != a) {
				Warning(str(format(WrongPlayer) %(unsigned) inbuf[0]  %a  %(unsigned) inbuf[3]));
			}
			else if (noHelperAIs) {
				Warning(str(format(NoHelperAI) %players[a].name %a));
			}
			else if (!demoReader) {
				Broadcast(packet); //forward data
			}
		} break;

		case NETMSG_AISHARE: {
			if (inbuf[3] != a) {
				Warning(str(format(WrongPlayer) %(unsigned) inbuf[0]  %a  %(unsigned) inbuf[3]));
			} else if (noHelperAIs) {
				Warning(str(format(NoHelperAI) %players[a].name %a));
			} else if (!demoReader) {
				Broadcast(packet); //forward data
			}
		} break;

		case NETMSG_LUAMSG:
			if(inbuf[3]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]));
			}
			else if (!demoReader) {
				Broadcast(packet); //forward data
			}
			break;

		case NETMSG_SYNCRESPONSE:
#ifdef SYNCCHECK
			if(inbuf[1]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				int frameNum = *(int*)&inbuf[2];
				if (outstandingSyncFrames.empty() || frameNum >= outstandingSyncFrames.front())
					players[a].syncResponse[frameNum] = *(unsigned*)&inbuf[6];
				else if (serverframenum - delayedSyncResponseFrame > SYNCCHECK_MSG_TIMEOUT) {
					delayedSyncResponseFrame = serverframenum;
					Warning(str(format(DelayedSyncResponse) %players[a].name %frameNum %serverframenum));
				}
				// update players' ping (if !defined(SYNCCHECK) this is done in NETMSG_KEYFRAME)
				players[a].ping = serverframenum - frameNum;
			}
#endif
			break;

		case NETMSG_SHARE:
			if(inbuf[1]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!demoReader)
					Broadcast(CBaseNetProtocol::Get().SendShare(inbuf[1], inbuf[2], inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8])));
			}
			break;

		case NETMSG_SETSHARE:
			if(inbuf[1]!= a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!demoReader)
					Broadcast(CBaseNetProtocol::Get().SendSetShare(inbuf[1], inbuf[2], *((float*)&inbuf[3]), *((float*)&inbuf[7])));
			}
			break;

		case NETMSG_PLAYERSTAT:
			if(inbuf[1]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				Broadcast(packet); //forward data
			}
			break;

		case NETMSG_MAPDRAW:
			Broadcast(packet); //forward data
			break;

#ifdef DIRECT_CONTROL_ALLOWED
		case NETMSG_DIRECT_CONTROL:
			if(inbuf[1]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!demoReader)
					Broadcast(CBaseNetProtocol::Get().SendDirectControl(inbuf[1]));
			}
			break;

		case NETMSG_DC_UPDATE:
			if(inbuf[1]!=a){
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!demoReader)
					Broadcast(CBaseNetProtocol::Get().SendDirectControlUpdate(inbuf[1], inbuf[2], *((short*)&inbuf[3]), *((short*)&inbuf[5])));
			}
			break;
#endif

		case NETMSG_STARTPLAYING:
		{
			if (players[a].isLocal && !gameStartTime)
				CheckForGameStart(true);
			break;
		}
		case NETMSG_TEAM:
		{
						//TODO update players[] and teams[] and send all to hostif
			const unsigned player = (unsigned)inbuf[1];
			if (player != a)
			{
				Warning(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)player));
			}
			else
			{
				const unsigned action = inbuf[2];
				const unsigned fromTeam = players[player].team;
				unsigned numPlayersInTeam = 0;
				for (unsigned a = 0; a < players.size(); ++a)
					if (players[a].team == fromTeam)
						++numPlayersInTeam;

				switch (action)
				{
					case TEAMMSG_GIVEAWAY: {
						const unsigned toTeam = inbuf[3];
						Broadcast(CBaseNetProtocol::Get().SendGiveAwayEverything(player, toTeam));
						if (numPlayersInTeam <= 1 && teams[fromTeam])
						{
							teams[fromTeam].reset();
						}
						players[player].team = 0;
						players[player].spectator = true;
						if (hostif) hostif->SendPlayerDefeated(player);
						break;
					}
					case TEAMMSG_RESIGN: {
						Broadcast(CBaseNetProtocol::Get().SendResign(player));
						players[player].team = 0;
						players[player].spectator = true;
						if (hostif) hostif->SendPlayerDefeated(player);
						break;
					}
					case TEAMMSG_JOIN_TEAM: {
						unsigned newTeam = inbuf[3];
						if (cheating)
						{
							Broadcast(CBaseNetProtocol::Get().SendJoinTeam(player, newTeam));
							players[player].team = newTeam;
							if (!teams[newTeam])
								teams[newTeam].reset(new GameTeam);
						}
						else
						{
							Warning(str(format(NoTeamChange) %players[player].name %player));
						}
						break;
					}
					case TEAMMSG_TEAM_DIED: { // don't send to clients, they don't need it
						unsigned char team = inbuf[3];
#ifndef DEDICATED
						if (teams[team] && players[player].isLocal) // currently only host is allowed
#else
						if (teams[team])
#endif
						{
							teams[team].reset();
							for (unsigned i = 0; i < players.size(); ++i)
							{
								if (players[i].team == team)
								{
									players[i].team = 0;
									players[i].spectator = true;
									if (hostif) hostif->SendPlayerDefeated(i);
								}
							}
						}
						break;
					}
					default: {
						Warning(str(format(UnknownTeammsg) %action %player));
					}
				}
				break;
			}
		}
		case NETMSG_ALLIANCE: {
			const unsigned char player = inbuf[1];
			const unsigned char whichAllyTeam = inbuf[2];
			const unsigned char allied = inbuf[3];
			if (!setup->fixedAllies)
			{
				Broadcast(CBaseNetProtocol::Get().SendSetAllied(player, whichAllyTeam, allied));
			}
			else
			{ // not allowed
			}
			break;
		}
		case NETMSG_CCOMMAND: {
			CommandMessage msg(packet);
			if (msg.player == a)
			{
				if ((commandBlacklist.find(msg.action.command) != commandBlacklist.end()) && players[a].isLocal)
				{
								// command is restricted to server but player is allowed to execute it
					PushAction(msg.action);
				}
				else if (commandBlacklist.find(msg.action.command) == commandBlacklist.end())
				{
								// command is save
					Broadcast(packet);
				}
				else
				{
								// hack!
					Warning(str(boost::format(CommandNotAllowed) %msg.player %msg.action.command.c_str()));
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
#ifdef DEBUG
			Warning(str(format(UnknownNetmsg) %(unsigned)inbuf[0] %a));
#endif
			break;
#ifdef SYNCDEBUG
		case NETMSG_SD_CHKRESPONSE:
		case NETMSG_SD_BLKRESPONSE:
			CSyncDebugger::GetInstance()->ServerReceived(inbuf);
			break;
		case NETMSG_SD_CHKREQUEST:
		case NETMSG_SD_BLKREQUEST:
		case NETMSG_SD_RESET:
			Broadcast(packet);
			break;
#endif
		default:
		{
			Warning(str(format(UnknownNetmsg) %(unsigned)inbuf[0] %a));
		}
		break;
	}
}

void CGameServer::ServerReadNet()
{
	// handle new connections
	while (UDPNet && UDPNet->HasIncomingConnections())
	{
		boost::shared_ptr<netcode::UDPConnection> prev = UDPNet->PreviewConnection().lock();
		boost::shared_ptr<const RawPacket> packet = prev->GetData();

		if (packet && packet->length >= 3 && packet->data[0] == NETMSG_ATTEMPTCONNECT)
		{
			netcode::UnpackPacket msg(packet, 3);
			std::string name, version;
			msg >> name;
			msg >> version;
			BindConnection(name, version, false, UDPNet->AcceptConnection());
		}
		else
		{
			if (packet && packet->length >= 3) {
				Warning(str(format(ConnectionReject) %packet->data[0] %packet->data[2] %packet->length));
			}
			else {
				Warning("Connection attempt rejected: Packet too short");
			}
			UDPNet->RejectConnection();
		}
	}

	for(unsigned a=0; (int)a < players.size(); a++)
	{
		if (!players[a].link)
			continue; // player not connected
		if (players[a].link->CheckTimeout())
		{
			Message(str(format(PlayerLeft) %players[a].name %" timeout")); //this must happen BEFORE the reset!
			players[a].myState = GameParticipant::DISCONNECTED;
			players[a].link.reset();
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 0));
			if (hostif)
			{
				hostif->SendPlayerLeft(a, 0);
			}
			continue;
		}

		boost::shared_ptr<const RawPacket> packet;
		while (players[a].link && (packet = players[a].link->GetData()))
		{
			ProcessPacket(a, packet);
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
	CRC crc;
	crc.Update(setup->gameSetupText.c_str(), setup->gameSetupText.length());
	gameID.intArray[2] = crc.GetDigest();

	CRC entropy;
	entropy.Update(lastTick);
	gameID.intArray[3] = entropy.GetDigest();

	Broadcast(CBaseNetProtocol::Get().SendGameID(gameID.charArray));
}

void CGameServer::CheckForGameStart(bool forced)
{
	assert(!gameStartTime);
	bool allReady = true;

#ifdef DEDICATED
	// Lobby-protocol doesn't support creating games without players inside
	// so in dedicated mode there will always be the host-player in the script
	// which doesn't exist and will never join, so skip it in this case
	for (int a = std::max(setup->numDemoPlayers,1); a < players.size(); a++)
#else
	for (int a = setup->numDemoPlayers; a < players.size(); a++)
#endif
	{
		if (players[a].myState < GameParticipant::INGAME) {
			allReady = false;
			break;
		} else if (teams[players[a].team] && !teams[players[a].team]->readyToStart && !demoReader)
		{
			allReady = false;
			break;
		}
	}

	if (allReady || forced)
	{
		if (readyTime == 0) {
			readyTime = SDL_GetTicks();
			rng.Seed(readyTime);
			Broadcast(CBaseNetProtocol::Get().SendStartPlaying(GameStartDelay));
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

	if (UDPNet)
		UDPNet->Listen(false); // don't accept new connections

	// make sure initial game speed is within allowed range and sent a new speed if not
	UserSpeedChange(userSpeedFactor, SERVER_PLAYER);

	if (demoReader) {
		// the client told us to start a demo
		// no need to send startpos and startplaying since its in the demo
		Message(DemoStart);
		return;
	}

	GenerateAndSendGameID();
	for (int a = 0; a < setup->numTeams; ++a)
	{
		if (teams[a]) // its a player
			Broadcast(CBaseNetProtocol::Get().SendStartPos(SERVER_PLAYER, a, 1, teams[a]->startpos.x, teams[a]->startpos.y, teams[a]->startpos.z));
		else // maybe an AI?
			Broadcast(CBaseNetProtocol::Get().SendStartPos(SERVER_PLAYER, a, 1, setup->teamStartingData[a].startPos.x, setup->teamStartingData[a].startPos.y, setup->teamStartingData[a].startPos.z));
	}

	Broadcast(CBaseNetProtocol::Get().SendRandSeed(rng()));
	Broadcast(CBaseNetProtocol::Get().SendStartPlaying(0));
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
			for (unsigned a=0; a < players.size();++a)
			{
				std::string playerLower = StringToLower(players[a].name);
				if (playerLower.find(name)==0)
				{	//can kick on substrings of name
					if (!players[a].isLocal) // don't kick host
						KickPlayer(a);
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
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
	}
	else if (action.command == "setmaxspeed" && !action.extra.empty())
	{
		float newUserSpeed = std::max(static_cast<float>(atof(action.extra.c_str())), minUserSpeed);
		if (newUserSpeed > 0.2)
		{
			maxUserSpeed = newUserSpeed;
			UserSpeedChange(userSpeedFactor, SERVER_PLAYER);
		}
	}
	else if (action.command == "setminspeed" && !action.extra.empty())
	{
		minUserSpeed = std::min(static_cast<float>(atof(action.extra.c_str())), maxUserSpeed);
		UserSpeedChange(userSpeedFactor, SERVER_PLAYER);
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
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
	}
	else if (action.command == "singlestep")
	{
		if (isPaused && !demoReader)
			gameServer->CreateNewFrame(true, true);
	}
#ifdef DEDICATED // we already have a quit command in the client
	else if (action.command == "kill")
	{
		quitServer = true;
	}
#endif
	else
	{
		// only forward to players (send over network)
		CommandMessage msg(action, SERVER_PLAYER);
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
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
			Message(GameEnd);
			Broadcast(CBaseNetProtocol::Get().SendGameOver());
			if (hostif) {
				hostif->SendGameOver();
			}
			sentGameOverMsg = true;
		}
		return;
	}

	if (setup->gameMode == GameMode::OpenEnd)
		return;

	int numActiveAllyTeams = 0;
	int numActiveTeams[MAX_TEAMS]; // active teams per ally team
	memset(numActiveTeams, 0, sizeof(numActiveTeams));

#ifndef DEDICATED
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a)
	{
		bool hasPlayer = false;
		for (int b = 0; b < playerHandler->ActivePlayers(); ++b) {
			if (playerHandler->Player(b)->active && playerHandler->Player(b)->team == static_cast<int>(a) && !playerHandler->Player(b)->spectator) {
				hasPlayer = true;
			}
		}
		if (teamHandler->Team(a)->isAI)
			hasPlayer = true;

		if (!teamHandler->Team(a)->isDead && !teamHandler->Team(a)->gaia && hasPlayer)
			++numActiveTeams[teamHandler->AllyTeam(a)];
	}

	for (int a = 0; a < teamHandler->ActiveAllyTeams(); ++a)
		if (numActiveTeams[a] != 0)
			++numActiveAllyTeams;
#else
	for (int a = 0; a < setup->numTeams; ++a)
	{
		bool hasPlayer = false;
		for (unsigned b = 0; b < players.size(); ++b) {
			if (!players[b].spectator && players[b].team == a) {
				hasPlayer = true;
			}
		}
		if (!setup->teamStartingData[a].aiDll.empty())
		{
			++numActiveTeams[setup->teamStartingData[a].teamAllyteam];
			continue;
		}

		if (teams[a] && hasPlayer)
			++numActiveTeams[teams[a]->allyTeam];
	}

	for (int a = 0; a < MAX_TEAMS; ++a)
		if (numActiveTeams[a] != 0)
			++numActiveAllyTeams;
#endif
	if (numActiveAllyTeams <= 1)
	{
		gameEndTime=SDL_GetTicks();
		Broadcast(CBaseNetProtocol::Get().SendSendPlayerStat());
	}
}

void CGameServer::CreateNewFrame(bool fromServerThread, bool fixedFrameTime)
{
	if (!demoReader) // use NEWFRAME_MSGes from demo otherwise
	{
#if (BOOST_VERSION >= 103500)
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
			newFrames = (timeLeft > 0)? int(ceil(timeLeft)): 0;
			timeLeft -= newFrames;
		}

		bool rec = false;
#ifndef NO_AVI
		rec = game && game->creatingVideo;
#endif
		bool normalFrame = !isPaused && !rec;
		bool videoFrame = !isPaused && fixedFrameTime;
		bool singleStep = fixedFrameTime && !rec;

		if(normalFrame || videoFrame || singleStep){
			for(int i=0; i < newFrames; ++i){
				assert(!demoReader);
				++serverframenum;
				//Send out new frame messages.
				if (0 == (serverframenum % serverKeyframeIntervall))
					Broadcast(CBaseNetProtocol::Get().SendKeyFrame(serverframenum));
				else
					Broadcast(CBaseNetProtocol::Get().SendNewFrame());
#ifdef SYNCCHECK
				outstandingSyncFrames.push_back(serverframenum);
#endif
			}
		}
	}
	else
	{
		CheckSync();
		SendDemoData();
	}
}

void CGameServer::UpdateLoop()
{
	while (!quitServer)
	{
		bool hasData = false;
		if (hasLocalClient || !UDPNet)
		{
			SDL_Delay(10); // don't take 100% cpu time
			hasData = true;
		}
		else
		{
			assert(UDPNet);
			hasData = UDPNet->HasIncomingData(10); // may block up to 10 ms if there is no data (don't need a lock)
		}

		if (UDPNet)
			UDPNet->Update();

		boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
		if (hasData)
			ServerReadNet(); // new data arrived, we may have new packets
		Update();
	}
	if (hostif)
		hostif->SendQuit();
	Broadcast(CBaseNetProtocol::Get().SendQuit());
}

bool CGameServer::WaitsOnCon() const
{
	return (UDPNet && UDPNet->Listen());
}

bool CGameServer::GameHasStarted() const
{
	return (gameStartTime>0);
}

void CGameServer::KickPlayer(const int playerNum)
{
	if (players[playerNum].link) // only kick connected players
	{
		Message(str(format(PlayerLeft) %playerNum %"kicked"));
		Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(playerNum, 2));
		players[playerNum].link->SendData(CBaseNetProtocol::Get().SendQuit());
		players[playerNum].link.reset();
		players[playerNum].myState = GameParticipant::DISCONNECTED;
		if (hostif)
		{
			hostif->SendPlayerLeft(playerNum, 2);
		}
	}
	else
		Message(str( format("Attempt to kick player $d who is not connected") %playerNum ));
}

unsigned CGameServer::BindConnection(const std::string& name, const std::string& version, bool isLocal, boost::shared_ptr<netcode::CConnection> link)
{
	unsigned hisNewNumber = 0;
	bool found = false;

	unsigned startnum;
	if (demoReader)
		 startnum = (unsigned)demoReader->GetFileHeader().maxPlayerNum+1;
	else
		startnum = 0;

	for (unsigned i = startnum; i < players.size(); ++i)
	{
		if (name == players[i].name)
		{
			if (players[i].isFromDemo)
			{
				Message(str(format("Player %s (%i) is from demo") %name %i));
			}
			if (players[i].myState == GameParticipant::UNCONNECTED || players[i].myState == GameParticipant::DISCONNECTED)
			{
				hisNewNumber = i;
				found = true;
				break;
			}
			else
			{
				Message(str(format("Player %s is already ingame") %name));
			}
		}
	}

	if (hisNewNumber >= players.size())
	{
		if (demoReader)
		{
			GameParticipant buf;
			buf.isFromDemo = false;
			buf.name = name;
			buf.spectator = true;
			buf.team = 0;
			players.push_back(buf);
		}
		else
		{
			// player not found
			Message(str(format("Player %s not found, rejecting connection attempt") %name));
			return 0;
		}
	}

	players[hisNewNumber].link = link;
	players[hisNewNumber].isLocal = isLocal;

	link->SendData(boost::shared_ptr<const RawPacket>(gameData->Pack()));
	link->SendData(CBaseNetProtocol::Get().SendSetPlayerNum((unsigned char)hisNewNumber));

	for (unsigned a = 0; a < players.size(); ++a) {
		if(players[a].myState >= GameParticipant::INGAME)
			Broadcast(CBaseNetProtocol::Get().SendPlayerName(a, players[a].name));
	}

	if (!demoReader || setup->demoName.empty()) // gamesetup from demo?
	{
		const unsigned hisTeam = setup->playerStartingData[hisNewNumber].team;
		if (!teams[hisTeam]) // create new team
		{
			teams[hisTeam].reset(new GameTeam());
			teams[hisTeam]->readyToStart = (setup->startPosType != CGameSetup::StartPos_ChooseInGame);
			teams[hisTeam]->allyTeam = setup->teamStartingData[hisTeam].teamAllyteam;
			if (setup->startPosType == CGameSetup::StartPos_ChooseInGame) {
				// if the player didn't choose a start position, choose one for him
				// it should be near the center of his startbox
				// we let the startscript handle it
				teams[hisTeam]->startpos.x = 0;
				teams[hisTeam]->startpos.y = -500;
				teams[hisTeam]->startpos.z = 0;
			} else {
				teams[hisTeam]->startpos = setup->teamStartingData[hisTeam].startPos;
			}
		}
		players[hisNewNumber].team = hisTeam;
		if (!setup->playerStartingData[hisNewNumber].spectator)
			Broadcast(CBaseNetProtocol::Get().SendJoinTeam(hisNewNumber, hisTeam));
		for (int a = 0; a < MAX_TEAMS; ++a)
		{
			if (teams[a])
				Broadcast(CBaseNetProtocol::Get().SendStartPos(SERVER_PLAYER, a, teams[a]->readyToStart, teams[a]->startpos.x, teams[a]->startpos.y, teams[a]->startpos.z));
		}
	}

	Message(str(format(NewConnection) %name %hisNewNumber %version));

	link->Flush(true);
	return hisNewNumber;
}

void CGameServer::GotChatMessage(const ChatMessage& msg)
{
	Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
	if (hostif && msg.fromPlayer != SERVER_PLAYER) {
		// don't echo packets to autohost
		hostif->SendPlayerChat(msg.fromPlayer, msg.destination,  msg.msg);
	}
}

void CGameServer::InternalSpeedChange(float newSpeed)
{
	if (internalSpeed == newSpeed)
		; //TODO some error here
	Broadcast(CBaseNetProtocol::Get().SendInternalSpeed(newSpeed));
	internalSpeed = newSpeed;
}

void CGameServer::UserSpeedChange(float newSpeed, int player)
{
	if(enforceSpeed && player!=SERVER_PLAYER && (players[player].cpuUsage - medianCpu > 0.25f || players[player].ping - medianPing > internalSpeed*GAME_SPEED/2)) {
		GotChatMessage(ChatMessage(player, player, "Speed change rejected (cpu load or ping is too high)"));
		return; // disallow speed change by players who cannot keep up gamespeed
	}

	newSpeed = std::min(maxUserSpeed, std::max(newSpeed, minUserSpeed));

	if (userSpeedFactor != newSpeed)
	{
		if (internalSpeed > newSpeed || internalSpeed == userSpeedFactor) // insta-raise speed when not slowed down
			InternalSpeedChange(newSpeed);

		Broadcast(CBaseNetProtocol::Get().SendUserSpeed(player, newSpeed));
		userSpeedFactor = newSpeed;
	}
}

void CGameServer::RestrictedAction(const std::string& action)
{
	RegisterAction(action);
	commandBlacklist.insert(action);
}


