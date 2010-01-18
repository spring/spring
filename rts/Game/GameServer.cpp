#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "Net/UDPListener.h"
#include "Net/UDPConnection.h"

#include <stdarg.h>
#include <ctime>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/version.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>
#if defined DEDICATED || defined DEBUG
#include <iostream>
#endif
#include <stdlib.h> // why is this here?

#include "Net/Connection.h"
#include "mmgr.h"

#include "GameServer.h"

#ifndef NO_AVI
#include "Game.h"
#endif

#include "LogOutput.h"
#include "GameSetup.h"
#include "ClientSetup.h"
#include "Action.h"
#include "ChatMessage.h"
#include "CommandMessage.h"
#include "BaseNetProtocol.h"
#include "PlayerHandler.h"
#include "Net/LocalConnection.h"
#include "Net/UnpackPacket.h"
#include "DemoReader.h"
#ifdef DEDICATED
#include "DemoRecorder.h"
#endif
#include "AutohostInterface.h"
#include "Util.h"
#include "TdfParser.h"
#include "GlobalUnsynced.h" // for syncdebug
#include "Sim/Misc/GlobalConstants.h"
#include "ConfigHandler.h"
#include "FileSystem/CRC.h"
#include "Player.h"
#include "Server/GameParticipant.h"
#include "Server/GameSkirmishAI.h"
// This undef is needed, as somewhere there is a type interface specified,
// which we need not!
// (would cause problems in ExternalAI/Interface/SAIInterfaceLibrary.h)
#ifdef interface
	#undef interface
#endif
#include "Sim/Misc/GlobalSynced.h"
#include "Server/MsgStrings.h"


using netcode::RawPacket;


/// frames until a syncchech will time out and a warning is given out
const unsigned SYNCCHECK_TIMEOUT = 300;

/// used to prevent msg spam
const unsigned SYNCCHECK_MSG_TIMEOUT = 400;

///msecs to wait until the game starts after all players are ready
const spring_duration gameStartDelay = spring_secs(4);

/// The time intervall in msec for sending player statistics to each client
const spring_duration playerInfoTime = spring_secs(2);

/// msecs to wait until the timeout condition (na active clients) activates
const spring_duration serverTimeout = spring_secs(30);

/// every n'th frame will be a keyframe (and contain the server's framenumber)
const unsigned serverKeyframeIntervall = 16;

const std::string commands[numCommands] = { "kick", "kickbynum", "setminspeed", "setmaxspeed",
						"nopause", "nohelp", "cheat", "godmode", "globallos",
						"nocost", "forcestart", "nospectatorchat", "nospecdraw",
						"skip", "reloadcob", "devlua", "editdefs", "luagaia",
						"singlestep" };
using boost::format;

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

CGameServer::CGameServer(const ClientSetup* settings, bool onlyLocal, const GameData* const newGameData, const CGameSetup* const mysetup)
: setup(mysetup)
{
	assert(setup);
	serverStartTime = spring_gettime();
	lastUpdate = serverStartTime;
	lastPlayerInfo  = serverStartTime;
	syncErrorFrame=0;
	syncWarningFrame=0;
	serverframenum=0;
	timeLeft=0;
	modGameTime = 0.0f;
	quitServer=false;
	hasLocalClient = false;
	localClientNumber = 0;
	isPaused = false;
	userSpeedFactor = 1.0f;
	internalSpeed = 1.0f;
	gamePausable = true;
	noHelperAIs = false;
	allowSpecDraw = true;
	cheating = false;
	sentGameOverMsg = false;

	spring_notime(gameStartTime);
	spring_notime(gameEndTime);
	spring_notime(readyTime);

	medianCpu = 0.0f;
	medianPing = 0;
	// enforceSpeed - throttles speed based on:
	// 0 : players (max cpu)
	// 1 : players (median cpu)
	enforceSpeed = configHandler->Get("EnforceGameSpeed", 0);

	allowAdditionalPlayers = configHandler->Get("AllowAdditionalPlayers", false);

	if (!onlyLocal)
		UDPNet.reset(new netcode::UDPListener(settings->hostport));

	if (settings->autohostport > 0) {
		AddAutohostInterface(settings->autohostip, settings->autohostport);
	}
	rng.Seed(newGameData->GetSetup().length());
	Message(str( format(ServerStart) %settings->hostport), false);

	lastTick = spring_gettime();

	maxUserSpeed = setup->maxSpeed;
	minUserSpeed = setup->minSpeed;
	noHelperAIs = (bool)setup->noHelperAIs;

	{ // modify and save GameSetup text (remove passwords)
		TdfParser parser(newGameData->GetSetup().c_str(), newGameData->GetSetup().length());
		TdfParser::TdfSection* root = parser.GetRootSection();
		for (TdfParser::TdfSection::sectionsMap::iterator it = root->sections.begin(); it != root->sections.end(); ++it)
		{
			if (it->first.substr(0, 6) == "PLAYER")
				it->second->remove("Password");
		}
		std::ostringstream strbuf;
		parser.print(strbuf);
		std::string cleanedSetupText = strbuf.str();
		GameData* newData = new GameData(*newGameData);
		newData->SetSetup(cleanedSetupText);
		gameData.reset(newData);
	}

	if (setup->hostDemo)
	{
		Message(str( format(PlayingDemo) %setup->demoName ));
		demoReader.reset(new CDemoReader(setup->demoName, modGameTime+0.1f));
	}

	players.resize(setup->playerStartingData.size());
	std::copy(setup->playerStartingData.begin(), setup->playerStartingData.end(), players.begin());

	for (size_t a = 0; a < setup->GetSkirmishAIs().size(); ++a) {
		const size_t skirmishAIId = ReserveNextAvailableSkirmishAIId();
		ais[skirmishAIId] = setup->GetSkirmishAIs()[a];
	}

	teams.resize(setup->teamStartingData.size());
	std::copy(setup->teamStartingData.begin(), setup->teamStartingData.end(), teams.begin());

	commandBlacklist = std::set<std::string>(commands, commands+numCommands);

#ifdef DEDICATED
	demoRecorder.reset(new CDemoRecorder());
	demoRecorder->SetName(setup->mapName, setup->modName);
	demoRecorder->WriteSetupText(gameData->GetSetup());
	const netcode::RawPacket* ret = gameData->Pack();
	demoRecorder->SaveToDemo(ret->data, ret->length, modGameTime);
	delete ret;
#endif
	// AIs do not join in here, so just set their teams as active
	for (std::map<size_t, GameSkirmishAI>::const_iterator ai = ais.begin(); ai != ais.end(); ++ai) {
		const int t = ai->second.team;
		teams[t].active = true;
		if (teams[t].leader < 0) { // CAUTION, default value is 0, not -1
			teams[t].leader = ai->second.hostPlayer;
		}
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
#ifdef DEDICATED
	// TODO: move this to a method in CTeamHandler
	// Figure out who won the game.
	int numTeams = (int)setup->teamStartingData.size();
	if (setup->useLuaGaia) {
		--numTeams;
	}
	int winner = -1;
	/*for (int i = 0; i < numTeams; ++i) {
		if (teams[i] && !teamHandler->Team(i)->isDead) {
			winner = teamHandler->AllyTeam(i);
			break;
		}
	}*/ //TODO figure out who won
	// Finally pass it on to the CDemoRecorder.
	demoRecorder->SetTime(serverframenum / 30, spring_tomsecs(spring_gettime()-serverStartTime)/1000);
	demoRecorder->InitializeStats(players.size(), numTeams, winner);
	for (size_t i = 0; i < players.size(); ++i) {
		demoRecorder->SetPlayerStats(i, players[i].lastStats);
	}
	/*for (size_t i = 0; i < ais.size(); ++i) {
		demoRecorder->SetSkirmishAIStats(i, ais[i].lastStats);
	}*/
	/*for (int i = 0; i < numTeams; ++i) {
		record->SetTeamStats(i, teamHandler->Team(i)->statHistory);
	}*/ //TODO add
#endif // DEDICATED
}

void CGameServer::AddLocalClient(const std::string& myName, const std::string& myVersion)
{
	boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
	assert(!hasLocalClient);
	hasLocalClient = true;
	localClientNumber = BindConnection(myName, "", myVersion, true, boost::shared_ptr<netcode::CConnection>(new netcode::CLocalConnection()));
}

void CGameServer::AddAutohostInterface(const std::string& autohostip, const int remotePort)
{
	if (!hostif)
	{
		hostif.reset(new AutohostInterface(autohostip, remotePort));
		hostif->SendStart();
		Message(str(format(ConnectAutohost) %remotePort), false);
	}
}

void CGameServer::PostLoad(unsigned newlastTick, int newserverframenum)
{
	boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
#if SPRING_TIME
	lastTick = newlastTick;
#else
	//lastTick = boost::some_time; FIXME
#endif
	serverframenum = newserverframenum;

	std::vector<GameParticipant>::iterator it;
	for (it = players.begin(); it != players.end(); ++it) {
		it->lastFrameResponse = newserverframenum;
	}
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
		lastUpdate = spring_gettime();
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
			lastTick = spring_gettime();
			serverframenum++;
#ifdef SYNCCHECK
			if (!skipping)
				outstandingSyncFrames.push_back(serverframenum);
			CheckSync();
#endif
			Broadcast(boost::shared_ptr<const RawPacket>(buf));
		}
		else if (msgCode == NETMSG_GAMEOVER)
		{
			sentGameOverMsg = true;
			Broadcast(boost::shared_ptr<const RawPacket>(buf));
		}
		else if ( msgCode != NETMSG_GAMEDATA &&
						msgCode != NETMSG_SETPLAYERNUM &&
						msgCode != NETMSG_USER_SPEED &&
						msgCode != NETMSG_INTERNAL_SPEED &&
						msgCode != NETMSG_PAUSE) // dont send these from demo
		{
			Broadcast(boost::shared_ptr<const RawPacket>(buf));
		}
	}

	if (demoReader->ReachedEnd()) {
		demoReader.reset();
		Message(DemoEnd);
		gameEndTime = spring_gettime();
	}
}

void CGameServer::Broadcast(boost::shared_ptr<const netcode::RawPacket> packet)
{
	for (size_t p = 0; p < players.size(); ++p)
	{
		players[p].SendData(packet);
	}
	if (allowAdditionalPlayers || !spring_istime(gameStartTime))
	{
		packetCache.push_back(packet);
	}
#ifdef DEDICATED
	if (demoRecorder) {
		demoRecorder->SaveToDemo(packet->data, packet->length, modGameTime);
	}
#endif
}

void CGameServer::Message(const std::string& message, bool broadcast)
{
	if (broadcast) {
		Broadcast(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
	}
	else if (hasLocalClient)
	{
		// host should see
		players[localClientNumber].SendData(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
	}

	if (hostif) {
		hostif->Message(message);
	}
#if defined DEDICATED
	std::cout << message << std::endl;
#endif
}

void CGameServer::PrivateMessage(int playernum, const std::string& message) {
	players[playernum].SendData(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
}

void CGameServer::CheckSync()
{
#ifdef SYNCCHECK
	// Check sync
	std::deque<int>::iterator f = outstandingSyncFrames.begin();
	while (f != outstandingSyncFrames.end()) {
		unsigned correctChecksum = 0;
		bool bGotCorrectChecksum = false;
		if (hasLocalClient)
		{
			// dictatorship
			std::map<int, unsigned>::iterator it = players[localClientNumber].syncResponse.find(*f);
			if (it != players[localClientNumber].syncResponse.end())
			{
				correctChecksum = it->second;
				bGotCorrectChecksum = true;
			}
		}
		else
		{
			// democracy
			typedef std::vector< std::pair<unsigned, unsigned> > chkList;
			chkList checksums;
			unsigned checkMaxCount = 0;
			for (size_t a = 0; a < players.size(); ++a)
			{
				if (!players[a].link)
					continue;

				std::map<int, unsigned>::const_iterator it = players[a].syncResponse.find(*f);
				if (it != players[a].syncResponse.end())
				{
					bool found = false;
					for (chkList::iterator it2 = checksums.begin(); it2 != checksums.end(); ++it2)
					{
						if (it2->first == it->second)
						{
							found = true;
							it2->second++;
							if (checkMaxCount < it2->second)
							{
								checkMaxCount = it2->second;
								correctChecksum = it2->first;
							}
						}
					}
					if (!found)
					{
						checksums.push_back(std::pair<unsigned, unsigned>(it->second, 1));
						if (checkMaxCount == 0)
						{
							checkMaxCount = 1;
							correctChecksum = it->second;
						}
					}
				}
			}
			bGotCorrectChecksum = (checkMaxCount > 0);
		}

		std::vector<int> noSyncResponse;
		// maps incorrect checksum to players with that checksum
		std::map<unsigned, std::vector<int> > desyncGroups;
		std::map<int, unsigned> desyncSpecs;
		bool bComplete = true;
		for (size_t a = 0; a < players.size(); ++a) {
			if (!players[a].link) {
				continue;
			}
			std::map<int, unsigned>::iterator it = players[a].syncResponse.find(*f);
			if (it == players[a].syncResponse.end()) {
				if (*f >= serverframenum - static_cast<int>(SYNCCHECK_TIMEOUT))
					bComplete = false;
				else if (*f < players[a].lastFrameResponse)
					noSyncResponse.push_back(a);
			} else {
				if (bGotCorrectChecksum && it->second != correctChecksum)
				{
					players[a].desynced = true;
					if(demoReader || !players[a].spectator)
						desyncGroups[it->second].push_back(a);
					else
						desyncSpecs[a] = it->second;
				}
				else
					players[a].desynced = false;
			}
		}

		if (!noSyncResponse.empty()) {
			if (!syncWarningFrame || (*f - syncWarningFrame > static_cast<int>(SYNCCHECK_MSG_TIMEOUT))) {
				syncWarningFrame = *f;

				std::string playernames = GetPlayerNames(noSyncResponse);
				Message(str(format(NoSyncResponse) %playernames %(*f)));
			}
		}

		// If anything's in it, we have a desync.
		// TODO take care of !bComplete case?
		// Should we start resync then immediately or wait for the missing packets (while paused)?
		if ( /*bComplete && */ (!desyncGroups.empty() || !desyncSpecs.empty())) {
			if (!syncErrorFrame || (*f - syncErrorFrame > static_cast<int>(SYNCCHECK_MSG_TIMEOUT))) {
				syncErrorFrame = *f;

				// TODO enable this when we have resync
				//serverNet->SendPause(SERVER_PLAYER, true);
#ifdef SYNCDEBUG
				CSyncDebugger::GetInstance()->ServerTriggerSyncErrorHandling(serverframenum);
				Broadcast(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, true));
				isPaused = true;
				Broadcast(CBaseNetProtocol::Get().SendSdCheckrequest(serverframenum));
#endif
				// For each group, output a message with list of player names in it.
				// TODO this should be linked to the resync system so it can roundrobin
				// the resync checksum request packets to multiple clients in the same group.
				std::map<unsigned, std::vector<int> >::const_iterator g = desyncGroups.begin();
				for (; g != desyncGroups.end(); ++g) {
					std::string playernames = GetPlayerNames(g->second);
					Message(str(format(SyncError) %playernames %(*f) %(g->first ^ correctChecksum)));
				}

				// send spectator desyncs as private messages to reduce spam
				for(std::map<int, unsigned>::const_iterator s = desyncSpecs.begin(); s != desyncSpecs.end(); ++s) {
					int playernum = s->first;
					PrivateMessage(playernum, str(format(SyncError) %players[playernum].name %(*f) %(s->second ^ correctChecksum)));
				}
			}
		}

		// Remove complete sets (for which all player's checksums have been received).
		if (bComplete) {
			// Message(str ( boost::format("Succesfully purged outstanding sync frame %d from the deque") %(*f)));
			for (size_t a = 0; a < players.size(); ++a) {
				if (players[a].myState < GameParticipant::DISCONNECTED)
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
		Message(NoSyncCheck);
	}
#endif
}

void CGameServer::Update()
{
	if (!isPaused && spring_istime(gameStartTime))
	{
		modGameTime += float(spring_tomsecs(spring_gettime() - lastUpdate)) * 0.001f * internalSpeed;
	}
	lastUpdate = spring_gettime();

	if(lastPlayerInfo < (spring_gettime() - playerInfoTime)){
		lastPlayerInfo = spring_gettime();

		if (serverframenum > 0) {
			//send info about the players
			std::vector<float> cpu;
			std::vector<int> ping;
			float refCpu = 0.0f;
			for (size_t a = 0; a < players.size(); ++a) {
				if (players[a].myState == GameParticipant::INGAME) {
					Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, players[a].cpuUsage, (serverframenum - players[a].lastFrameResponse)));
					if(demoReader ? !players[a].isFromDemo : !players[a].spectator)
					{
						if (players[a].cpuUsage > refCpu) {
							refCpu = players[a].cpuUsage;
						}
						cpu.push_back(players[a].cpuUsage);
						ping.push_back(serverframenum - players[a].lastFrameResponse);
					}
				}
			}

			medianCpu = 0.0f;
			medianPing = 0;
			if(enforceSpeed > 0 && cpu.size() > 0) {
				std::sort(cpu.begin(), cpu.end());
				std::sort(ping.begin(), ping.end());

				int midpos = cpu.size() / 2;
				medianCpu = cpu[midpos];
				medianPing = ping[midpos];
				if(midpos * 2 == cpu.size()) {
					medianCpu = (medianCpu + cpu[midpos - 1]) / 2.0f;
					medianPing = (medianPing + ping[midpos - 1]) / 2;
				}
				refCpu = medianCpu;
			}

			if (refCpu > 0.0f) {
				// aim for 60% cpu usage if median is used as reference and 75% cpu usage if max is the reference
				float wantedCpu = (enforceSpeed > 0) ? 0.6f + (1 - internalSpeed / userSpeedFactor) * 0.5f : 0.75f + (1 - internalSpeed / userSpeedFactor) * 0.5f;
				float newSpeed = internalSpeed * wantedCpu / refCpu;
//				float speedMod=1+wantedCpu-refCpu;
//				logOutput.Print("Speed REF %f MED %f WANT %f SPEEDM %f NSPEED %f",refCpu,medianCpu,wantedCpu,speedMod,newSpeed);
				newSpeed = (newSpeed + internalSpeed) * 0.5f;
				newSpeed = std::max(newSpeed, (enforceSpeed > 0) ? userSpeedFactor * 0.8f : userSpeedFactor * 0.5f);
				if(newSpeed > userSpeedFactor)
					newSpeed = userSpeedFactor;
				if(newSpeed < 0.1f)
					newSpeed = 0.1f;
				if(newSpeed != internalSpeed)
					InternalSpeedChange(newSpeed);
			}
		}
		else {
			for (size_t a = 0; a < players.size(); ++a) {
				if (players[a].myState == GameParticipant::CONNECTED) { // send pathing status
					if(players[a].cpuUsage > 0)
						Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, players[a].cpuUsage, PATHING_FLAG));
				}
				else {
					Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, 0, 0)); // reset status
				}
			}
		}
	}

	if (!spring_istime(gameStartTime))
	{
		CheckForGameStart();
	}
	else if (serverframenum > 0 || demoReader)
	{
		CreateNewFrame(true, false);
		if (serverframenum > GAME_SPEED && !sentGameOverMsg && !demoReader)
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

	if (spring_gettime() > serverStartTime + serverTimeout || spring_istime(gameStartTime))
	{
		bool hasPlayers = false;
		for (size_t i = 0; i < players.size(); ++i)
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


/// has to be consistent with Game.cpp/CPlayerHandler
static std::vector<int> getPlayersInTeam(const std::vector<GameParticipant>& players, const int teamId) {

	std::vector<int> playersInTeam;

	for (size_t p = 0; p < players.size(); ++p) {
		// do not count spectators, or demos will desync
		if (!players[p].spectator && (players[p].team == teamId)) {
			playersInTeam.push_back(p);
		}
	}

	return playersInTeam;
}

/**
 * Duplicates functionality of CPlayerHandler::ActivePlayersInTeam(int teamId)
 * as playerHandler is not available on the server
 */
static int countNumPlayersInTeam(const std::vector<GameParticipant>& players, const int teamId) {
	return getPlayersInTeam(players, teamId).size();
}

/// has to be consistent with Game.cpp/CSkirmishAIHandler
static std::vector<size_t> getSkirmishAIIds(const std::map<size_t, GameSkirmishAI>& ais, const int teamId, const int hostPlayer = -2) {

	std::vector<size_t> skirmishAIIds;

	for (std::map<size_t, GameSkirmishAI>::const_iterator ai = ais.begin(); ai != ais.end(); ++ai) {
		if ((ai->second.team == teamId) && ((hostPlayer == -2) || (ai->second.hostPlayer == hostPlayer))) {
			skirmishAIIds.push_back(ai->first);
		}
	}

	return skirmishAIIds;
}

/**
 * Duplicates functionality of CSkirmishAIHandler::GetSkirmishAIsInTeam(const int teamId)
 * as skirmishAIHandler is not available on the server
 */
static int countNumSkirmishAIsInTeam(const std::map<size_t, GameSkirmishAI>& ais, const int teamId) {
	return getSkirmishAIIds(ais, teamId).size();
}

void CGameServer::ProcessPacket(const unsigned playernum, boost::shared_ptr<const netcode::RawPacket> packet)
{
	const boost::uint8_t* inbuf = packet->data;
	const unsigned a = playernum;

	switch (inbuf[0]){
		case NETMSG_KEYFRAME:
		{
			const int frameNum = *(int*)&inbuf[1];
			players[a].lastFrameResponse = frameNum;
			break;
		}

		case NETMSG_PAUSE:
			if(inbuf[1]!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!inbuf[2])  // reset sync checker
					syncErrorFrame = 0;
				if(gamePausable || players[a].isLocal) // allow host to pause even if nopause is set
				{
					if (!players[a].isLocal && players[a].spectator && !demoReader)
					{
						PrivateMessage(a, "Spectators cannot pause the game" );
					}
					else if (enforceSpeed > 0 && !isPaused &&
						(players[a].spectator || (enforceSpeed > 0 &&
						(players[a].cpuUsage - medianCpu > std::min(0.2f, std::max(0.0f, 0.8f - medianCpu) ) ||
						(serverframenum - players[a].lastFrameResponse) - medianPing > internalSpeed * GAME_SPEED / 2)))) {
						PrivateMessage(a, "Pausing rejected (cpu load or ping is too high)");
					}
					else
					{
						timeLeft=0;
						if (isPaused != !!inbuf[2]) {
							isPaused = !isPaused;
						}
						Broadcast(CBaseNetProtocol::Get().SendPause(a, inbuf[2]));
					}
				}
			}
			break;

		case NETMSG_USER_SPEED: {
			if (!players[a].isLocal && players[a].spectator && !demoReader)
			{
				PrivateMessage(a, "Spectators cannot change game speed");
			}
			else
			{
				float speed = *((float*) &inbuf[2]);
				UserSpeedChange(speed, a);
			}
		} break;

		case NETMSG_CPU_USAGE:
			players[a].cpuUsage = *((float*)&inbuf[1]);
			break;

		case NETMSG_QUIT: {
			Message(str(format(PlayerLeft) %players[a].GetType() %players[a].name %" normal quit"));
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 1));
			players[a].Kill("User exited");
			if (hostif)
			{
				hostif->SendPlayerLeft(a, 1);
			}
			break;
		}

		case NETMSG_PLAYERNAME: {
			unsigned playerNum = inbuf[2];
			if(playerNum!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %playerNum));
			} else {
				players[playerNum].name = (std::string)((char*)inbuf+3);
				players[playerNum].myState = GameParticipant::INGAME;
				Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, 0, 0)); // reset pathing display
				Message(str(format(PlayerJoined) %players[playerNum].GetType() %players[playerNum].name), false);
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
			if (static_cast<unsigned>(msg.fromPlayer) != a ) {
				Message(str(format(WrongPlayer) %(unsigned)NETMSG_CHAT %a %(unsigned)msg.fromPlayer));
			} else {
				GotChatMessage(msg);
			}
			break;
		}
		case NETMSG_SYSTEMMSG:
			if(inbuf[3]!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]));
			} else {
				Broadcast(CBaseNetProtocol::Get().SendSystemMessage(inbuf[3], (char*)(&inbuf[4])));
			}
			break;

		case NETMSG_STARTPOS: {
			const unsigned player = inbuf[1];
			if (player != a) {
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else if (setup->startPosType == CGameSetup::StartPos_ChooseInGame) {
				const unsigned team     = (unsigned)inbuf[2];
				if (team >= teams.size()) {
					Message(str( boost::format("Invalid teamID %d in NETMSG_STARTPOS from player %d") %team %player ));
				} else if (getSkirmishAIIds(ais, team, player).empty() && ((team != players[player].team) || (players[player].spectator))) {
					Message(str( boost::format("Player %d sent spoofed NETMSG_STARTPOS with teamID %d") %player %team ));
				} else {
					teams[team].startPos = float3(*((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12]));
					if (inbuf[3] == 1) {
						players[player].readyToStart = static_cast<bool>(inbuf[3]);
					}

					Broadcast(CBaseNetProtocol::Get().SendStartPos(inbuf[1],team, inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12]))); //forward data
					if (hostif) {
						hostif->SendPlayerReady(a, inbuf[3]);
					}
				}
			}
			else
			{
				Message(str(format(NoStartposChange) %a));
			}
			break;
		}

		case NETMSG_COMMAND:
			if(inbuf[3]!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]));
			} else {
				if (!demoReader)
					Broadcast(packet); //forward data
			}
			break;

		case NETMSG_SELECT:
			if(inbuf[3]!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]));
			} else {
				if (!demoReader)
					Broadcast(packet); //forward data
			}
			break;

		case NETMSG_AICOMMAND: {
			if (inbuf[3] != a) {
				Message(str(format(WrongPlayer) %(unsigned) inbuf[0]  %a  %(unsigned) inbuf[3]));
			}
			else if (noHelperAIs) {
				Message(str(format(NoHelperAI) %players[a].name %a));
			}
			else if (!demoReader) {
				Broadcast(packet); //forward data
			}
		} break;

		case NETMSG_AICOMMANDS: {
			if (inbuf[3] != a) {
				Message(str(format(WrongPlayer) %(unsigned) inbuf[0]  %a  %(unsigned) inbuf[3]));
			}
			else if (noHelperAIs) {
				Message(str(format(NoHelperAI) %players[a].name %a));
			}
			else if (!demoReader) {
				Broadcast(packet); //forward data
			}
		} break;

		case NETMSG_AISHARE: {
			if (inbuf[3] != a) {
				Message(str(format(WrongPlayer) %(unsigned) inbuf[0]  %a  %(unsigned) inbuf[3]));
			} else if (noHelperAIs) {
				Message(str(format(NoHelperAI) %players[a].name %a));
			} else if (!demoReader) {
				Broadcast(packet); //forward data
			}
		} break;

		case NETMSG_LUAMSG:
			if(inbuf[3]!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[3]));
			}
			else if (!demoReader) {
				Broadcast(packet); //forward data
				if (hostif)
					hostif->SendLuaMsg(packet->data, packet->length);
			}
			break;

		case NETMSG_SYNCRESPONSE: {
#ifdef SYNCCHECK
			int frameNum = *(int*)&inbuf[1];
			if (outstandingSyncFrames.empty() || frameNum >= outstandingSyncFrames.front())
				players[a].syncResponse[frameNum] = *(unsigned*)&inbuf[5];
			// update players' ping (if !defined(SYNCCHECK) this is done in NETMSG_KEYFRAME)
			players[a].lastFrameResponse = frameNum;
#endif
		}
			break;

		case NETMSG_SHARE:
			if(inbuf[1]!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!demoReader)
					Broadcast(CBaseNetProtocol::Get().SendShare(inbuf[1], inbuf[2], inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8])));
			}
			break;

		case NETMSG_SETSHARE:
			if(inbuf[1]!= a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!demoReader)
					Broadcast(CBaseNetProtocol::Get().SendSetShare(inbuf[1], inbuf[2], *((float*)&inbuf[3]), *((float*)&inbuf[7])));
			}
			break;

		case NETMSG_PLAYERSTAT:
			if(inbuf[1]!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				players[a].lastStats = *(PlayerStatistics*)&inbuf[2];
				Broadcast(packet); //forward data
			}
			break;

		case NETMSG_MAPDRAW:
			if(inbuf[2] != a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[2]));
			}
			else if (!players[playernum].spectator || allowSpecDraw)
			{
				Broadcast(packet); //forward data
			}
			break;

		case NETMSG_DIRECT_CONTROL:
			if(inbuf[1]!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!demoReader)
					Broadcast(CBaseNetProtocol::Get().SendDirectControl(inbuf[1]));
			}
			break;

		case NETMSG_DC_UPDATE:
			if(inbuf[1]!=a){
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)inbuf[1]));
			} else {
				if (!demoReader)
					Broadcast(CBaseNetProtocol::Get().SendDirectControlUpdate(inbuf[1], inbuf[2], *((short*)&inbuf[3]), *((short*)&inbuf[5])));
			}
			break;

		case NETMSG_STARTPLAYING:
		{
			if (players[a].isLocal && spring_istime(gameStartTime))
				CheckForGameStart(true);
			break;
		}
		case NETMSG_TEAM:
		{
			//TODO update players[] and teams[] and send all to hostif
			const unsigned player = (unsigned)inbuf[1];
			if (player != a)
			{
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)player));
			}
			else
			{
				const unsigned action = inbuf[2];
				const unsigned fromTeam = players[player].team;

				switch (action)
				{
					case TEAMMSG_GIVEAWAY: {
						const unsigned toTeam                    = inbuf[3];
						// may be the players team or a team controlled by one of his AIs
						const unsigned fromTeam_g                = inbuf[4];
						const int numPlayersInTeam_g             = countNumPlayersInTeam(players, fromTeam_g);
						const std::vector<size_t> totAIsInTeam_g = getSkirmishAIIds(ais, fromTeam_g);
						const std::vector<size_t> myAIsInTeam_g  = getSkirmishAIIds(ais, fromTeam_g, player);
						const size_t numControllersInTeam_g      = numPlayersInTeam_g + totAIsInTeam_g.size();
						const bool isLeader_g                    = (teams[fromTeam_g].leader == player);
						const bool isOwnTeam_g                   = (fromTeam_g == fromTeam);
						const bool isSpec                        = players[player].spectator;
						const bool hasAIs_g                      = (myAIsInTeam_g.size() > 0);
						const bool isAllied_g                    = (teams[fromTeam_g].teamAllyteam != teams[fromTeam].teamAllyteam);
						const char* playerType                   = players[player].GetType();
						const bool isSinglePlayer                = (players.size() <= 1);

						if (!isSinglePlayer &&
								(isSpec ||
								(!isOwnTeam_g && !isLeader_g) ||
								(hasAIs_g && (isAllied_g && !cheating))))
						{
							Message(str( boost::format("%s %s tried to hack the game (spoofed TEAMMSG_GIVEAWAY)") %playerType %players[player].name), true);
							break;
						}
						Broadcast(CBaseNetProtocol::Get().SendGiveAwayEverything(player, toTeam, fromTeam_g));

						bool giveAwayOk = false;
						if (isOwnTeam_g) {
							// player is giving stuff from his own team
							giveAwayOk = true;
							//players[player].team = 0;
							players[player].spectator = true;
							if (hostif) hostif->SendPlayerDefeated(player);
						} else {
							// player is giving stuff from one of his AI teams
							if (numPlayersInTeam_g == 0) {
								// kill the first AI
								ais.erase(myAIsInTeam_g[0]);
								giveAwayOk = true;
							} else {
								Message(str( boost::format("%s %s can not give away stuff of team %i (still has human players left)") %playerType %players[player].name %fromTeam_g), true);
							}
						}
						if (giveAwayOk && (numControllersInTeam_g == 1)) {
							// team has no controller left now
							teams[fromTeam_g].active = false;
							teams[fromTeam_g].leader = -1;
							std::ostringstream givenAwayMsg;
							givenAwayMsg << players[player].name << " gave everything to " << players[teams[toTeam].leader].name;
							Broadcast(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, givenAwayMsg.str()));
						}
						break;
					}
					case TEAMMSG_RESIGN: {
						const bool isSpec         = players[player].spectator;
						const bool isSinglePlayer = (players.size() <= 1);

						if (isSpec && !isSinglePlayer)
						{
							Message(str(boost::format("Spectator %s tried to hack the game (spoofed TEAMMSG_RESIGN)") %players[player].name), true);
							break;
						}
						Broadcast(CBaseNetProtocol::Get().SendResign(player));

						//players[player].team = 0;
						players[player].spectator = true;
						// actualize all teams of which the player is leader
						for (size_t t = 0; t < teams.size(); ++t) {
							if (teams[t].leader == player) {
								const std::vector<int> teamPlayers = getPlayersInTeam(players, t);
								const std::vector<size_t> teamAIs  = getSkirmishAIIds(ais, t);
								if ((teamPlayers.size() + teamAIs.size()) == 0) {
									// no controllers left in team
									teams[t].active = false;
									teams[t].leader = -1;
								} else if (teamPlayers.size() == 0) {
									// no human player left in team
									teams[t].leader = ais[teamAIs[0]].hostPlayer;
								} else {
									// still human controllers left in team
									teams[t].leader = teamPlayers[0];
								}
							}
						}
						if (hostif) hostif->SendPlayerDefeated(player);
						break;
					}
					case TEAMMSG_JOIN_TEAM: {
						const unsigned newTeam    = inbuf[3];
						const bool isNewTeamValid = (newTeam < teams.size());
						const bool isSinglePlayer = (players.size() <= 1);

						if (isNewTeamValid && (isSinglePlayer || cheating)) {
							// joining the team is ok
						} else {
							Message(str(format(NoTeamChange) %players[player].name %player));
							break;
						}
						Broadcast(CBaseNetProtocol::Get().SendJoinTeam(player, newTeam));

						players[player].team      = newTeam;
						players[player].spectator = false;
						if (teams[newTeam].leader == -1) {
							teams[newTeam].leader = player;
						}
						break;
					}
					case TEAMMSG_TEAM_DIED: { // don't send to clients, they don't need it
						const unsigned team = inbuf[3];
#ifndef DEDICATED
						if (players[player].isLocal) // currently only host is allowed
#else
						if (!players[player].desynced)
#endif
						{
							teams[team].active = false;
							teams[team].leader = -1;
							// convert all the teams players to spectators
							for (size_t p = 0; p < players.size(); ++p) {
								if ((players[p].team == team) && !(players[p].spectator)) {
									// are now spectating if this was their team
									//players[p].team = 0;
									players[p].spectator = true;
									if (hostif) hostif->SendPlayerDefeated(p);
								}
							}
							// The teams Skirmish AIs destruction process
							// is being initialized from the client they
							// run on. No need to do anything here.
						}
						break;
					}
					default: {
						Message(str(format(UnknownTeammsg) %action %player));
					}
				}
				break;
			}
		}
		case NETMSG_AI_CREATED: {
			const unsigned char playerId     = inbuf[2];
			//const unsigned skirmAIId_rec     = *((unsigned int*)&inbuf[3]); // 4 bytes; should be -1, as we have to create the real one
			const unsigned char aiTeamId     = inbuf[7];
			const char* aiName               = (const char*) (&inbuf[8]);
			const unsigned char playerTeamId = players[playerId].team;
			GameTeam* tpl                    = &teams[playerTeamId];
			GameTeam* tai                    = &teams[aiTeamId];
			//const int numPlayersInAITeam     = countNumPlayersInTeam(players, aiTeam);
			//const int numAIsInAITeam         = countNumSkirmishAIsInTeam(ais, aiTeam);
			const bool weAreLeader           = (tai->leader == playerId);
			const bool weAreAllied           = (tpl->teamAllyteam == tai->teamAllyteam);
			const bool singlePlayer          = (players.size() <= 1);
			const bool noLeader              = (tai->leader == -1);

			if (weAreLeader || singlePlayer || (weAreAllied && (cheating || noLeader))) {
				// creating the AI is ok
			} else {
				Message(str(format(NoAICreated) %players[playerId].name %(int)playerId %(int)aiTeamId));
				break;
			}
			const size_t skirmishAIId = ReserveNextAvailableSkirmishAIId();
			Broadcast(CBaseNetProtocol::Get().SendAICreated(playerId, skirmishAIId, aiTeamId, aiName));

/*
#ifdef SYNCDEBUG
			if (myId != skirmishAIId) {
				Message(str(format("Sync Error, Skirmish AI ID from player %s (%i) does not match the one on the server (%i).") %players[playerId].name %skirmishAIId %myId));
			}
#endif // SYNCDEBUG
*/
			ais[skirmishAIId].team = aiTeamId;
			ais[skirmishAIId].name = aiName;
			ais[skirmishAIId].hostPlayer = playerId;
			if (tai->leader == -1) {
				tai->leader = ais[skirmishAIId].hostPlayer;
				tai->active = true;
			}
			break;
		}
		case NETMSG_AI_STATE_CHANGED: {
			const unsigned char playerId     = inbuf[1];
			const unsigned int skirmishAIId  = *((unsigned int*)&inbuf[2]); // 4 bytes
			const ESkirmishAIStatus newState = (ESkirmishAIStatus) inbuf[6];

			const bool skirmishAIId_valid    = (ais.find(skirmishAIId) != ais.end());
			if (!skirmishAIId_valid) {
				Message(str(format(NoAIChangeState) %players[playerId].name %(int)playerId %skirmishAIId %(-1) %(int)newState));
				break;
			}

			const unsigned aiTeamId          = ais[skirmishAIId].team;
			const unsigned playerTeamId      = players[playerId].team;
			const size_t numPlayersInAITeam  = countNumPlayersInTeam(players, aiTeamId);
			const size_t numAIsInAITeam      = countNumSkirmishAIsInTeam(ais, aiTeamId);
			GameTeam* tpl                    = &teams[playerTeamId];
			GameTeam* tai                    = &teams[aiTeamId];
			const bool weAreAIHost           = (ais[skirmishAIId].hostPlayer == playerId);
			const bool weAreLeader           = (tai->leader == playerId);
			const bool weAreAllied           = (tpl->teamAllyteam == tai->teamAllyteam);
			const bool singlePlayer          = (players.size() <= 1);
			const ESkirmishAIStatus oldState = ais[skirmishAIId].status;

			if (!(weAreAIHost || weAreLeader || singlePlayer || (weAreAllied && cheating))) {
				Message(str(format(NoAIChangeState) %players[playerId].name %(int)playerId %skirmishAIId %(int)aiTeamId %(int)newState));
				break;
			}
			Broadcast(packet); // forward data

			ais[skirmishAIId].status = newState;
			if (newState == SKIRMAISTATE_DEAD) {
				if (oldState == SKIRMAISTATE_RELOADING) {
					// skip resetting this AIs management state,
					// as it will be reinitialized instantly
				} else {
					ais.erase(skirmishAIId);
					FreeSkirmishAIId(skirmishAIId);
					if ((numPlayersInAITeam + numAIsInAITeam) == 1) {
						// team has no controller left now
						tai->active = false;
						tai->leader = -1;
					}
				}
			}
			break;
		}
		case NETMSG_ALLIANCE: {
			const unsigned char player = inbuf[1];
			const int whichAllyTeam = inbuf[2];
			const unsigned char allied = inbuf[3];
			if (player != a)
			{
				Message(str(format(WrongPlayer) %(unsigned)inbuf[0] %a %(unsigned)player));
			}
			else if (whichAllyTeam == teams[players[a].team].teamAllyteam)
			{
				Message(str(format("Player %s tried to send spoofed alliance message") %players[a].name));
			}
			else
			{
				if (!setup->fixedAllies)
				{
					Broadcast(CBaseNetProtocol::Get().SendSetAllied(player, whichAllyTeam, allied));
				}
				else
				{ // not allowed
				}
			}
			break;
		}
		case NETMSG_CCOMMAND: {
			CommandMessage msg(packet);
			if (static_cast<unsigned>(msg.player) == a)
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
					Message(str(boost::format(CommandNotAllowed) %msg.player %msg.action.command.c_str()));
				}
			}
			break;
		}

		case NETMSG_TEAMSTAT: {
			if (hostif)
				hostif->Send(packet->data, packet->length);
			break;
		}

		case NETMSG_REGISTER_NETMSG: {
			const unsigned char player = inbuf[1];
			const unsigned char msg = inbuf[2];
			MsgToForwardMap::iterator itor = relayingMessagesMap.find( msg );

			if ( itor != relayingMessagesMap.end() ) { // one entry already exists in the map
				PlayersToForwardMsgvec &toForward; = itor->second;
				if ( toForward.find( player ) == toForward.end() ) {
					toForward.insert( player );
				}
			}
			else {
				PlayersToForwardMsgvec toForward;
				toForward.insert( player );
				relayingMessagesMap[msg] = toForward;
			}
			break;
		}

		case NETMSG_UNREGISTER_NETMSG: {
			const unsigned char player = inbuf[1];
			const unsigned char msg = inbuf[2];
			MsgToForwardMap::iterator itor = relayingMessagesMap.find( msg );
			if ( itor == relayingMessagesMap.end() ) { // no entry already exists in the map
				break;
			}
			PlayersToForwardMsgvec& toForward = itor->second;
			if ( toForward.find( player ) != toForward.end() ) {
				toForward.erase( player );
				if ( toForward.size() == 0 ) {
					relayingMessagesMap.erase( itor );
				}
			}
			break;
		}

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
		// CGameServer should never get these messages
		//case NETMSG_GAMEID:
		//case NETMSG_INTERNAL_SPEED:
		//case NETMSG_ATTEMPTCONNECT:
		//case NETMSG_GAMEDATA:
		//case NETMSG_RANDSEED:
		default:
		{
			Message(str(format(UnknownNetmsg) %(unsigned)inbuf[0] %a));
		}
		break;
	}

	// forward special messages to the players that request them
	size_t playersSize = players.size();
	MsgToForwardMap::iterator toRelay = relayingMessagesMap.find( inbuf[0] );
	if ( toRelay != relayingMessagesMap.end() ) {
		PlayersToForwardMsgvec& toRelaySet = toRelay->second;
		for ( PlayersToForwardMsgvec::iterator playerToRelay = toRelaySet.begin(); playerToRelay != toRelaySet.end(); playerToRelay++ ) {
			if ( *playerToRelay < playersSize ) {
				players[*playerToRelay].SendData(packet);
			}
		}
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
			std::string name, passwd, version;
			msg >> name;
			msg >> passwd;
			msg >> version;
			BindConnection(name, passwd, version, false, UDPNet->AcceptConnection());
		}
		else
		{
			if (packet && packet->length >= 3) {
				Message(str(format(ConnectionReject) %packet->data[0] %packet->data[2] %packet->length));
			}
			else {
				Message("Connection attempt rejected: Packet too short");
			}
			UDPNet->RejectConnection();
		}
	}

	for(size_t a=0; a < players.size(); a++)
	{
		if (!players[a].link)
			continue; // player not connected
		if (players[a].link->CheckTimeout())
		{
			Message(str(format(PlayerLeft) %players[a].GetType() %players[a].name %" timeout")); //this must happen BEFORE the reset!
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 0));
			players[a].Kill("User timeout");
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

	// Third dword is CRC of setupText.
	CRC crc;
	crc.Update(setup->gameSetupText.c_str(), setup->gameSetupText.length());
	gameID.intArray[2] = crc.GetDigest();

	CRC entropy;
	entropy.Update(spring_tomsecs(lastTick-serverStartTime));
	gameID.intArray[3] = entropy.GetDigest();

	Broadcast(CBaseNetProtocol::Get().SendGameID(gameID.charArray));
#ifdef DEDICATED
	demoRecorder->SetGameID(gameID.charArray);
#endif
}

void CGameServer::CheckForGameStart(bool forced)
{
	assert(!spring_istime(gameStartTime));
	bool allReady = true;

	for (size_t a = static_cast<size_t>(setup->numDemoPlayers); a < players.size(); a++)
	{
		if (players[a].myState == GameParticipant::UNCONNECTED && serverStartTime + spring_secs(30) < spring_gettime())
		{
			// autostart the game when 45 seconds have passed and everyone who managed to connect is ready
			continue;
		}
		else if (players[a].myState < GameParticipant::INGAME)
		{
			allReady = false;
			break;
		} else if (!players[a].spectator && teams[players[a].team].active && !players[a].readyToStart && !demoReader)
		{
			allReady = false;
			break;
		}
	}

	if (allReady || forced)
	{
		if (!spring_istime(readyTime)) {
			readyTime = spring_gettime();
			rng.Seed(spring_tomsecs(readyTime-serverStartTime));
			Broadcast(CBaseNetProtocol::Get().SendStartPlaying(spring_tomsecs(gameStartDelay)));
		}
	}
	if (spring_istime(readyTime) && (spring_gettime() - readyTime) > gameStartDelay)
	{
		StartGame();
	}
}

void CGameServer::StartGame()
{
	gameStartTime = spring_gettime();
	if (!allowAdditionalPlayers)
		packetCache.clear(); // free memory
	if (UDPNet && !allowAdditionalPlayers)
		UDPNet->Listen(false); // don't accept new connections

	// make sure initial game speed is within allowed range and sent a new speed if not
	UserSpeedChange(userSpeedFactor, SERVER_PLAYER);

	if (demoReader) {
		// the client told us to start a demo
		// no need to send startPos and startplaying since its in the demo
		Message(DemoStart);
		return;
	}

	GenerateAndSendGameID();

	std::vector<bool> teamStartPosSent(teams.size(), false);

	// send start position for player controlled teams
	for (size_t a = 0; a < players.size(); ++a)
	{
		if (!players[a].spectator)
		{
			const unsigned aTeam = players[a].team;
			Broadcast(CBaseNetProtocol::Get().SendStartPos(a, (int)aTeam, players[a].readyToStart, teams[aTeam].startPos.x, teams[aTeam].startPos.y, teams[aTeam].startPos.z));
			teamStartPosSent[aTeam] = true;
		}
	}

	// send start position for all other teams
	for (size_t a = 0; a < teams.size(); ++a) {
		if (!teamStartPosSent[a]) {
			// teams which aren't player controlled are always ready
			Broadcast(CBaseNetProtocol::Get().SendStartPos(teams[a].leader, a, true, teams[a].startPos.x, teams[a].startPos.y, teams[a].startPos.z));
		}
	}

	Broadcast(CBaseNetProtocol::Get().SendRandSeed(rng()));
	Broadcast(CBaseNetProtocol::Get().SendStartPlaying(0));
	if (hostif)
	{
		hostif->SendStartPlaying();
	}
	timeLeft=0;
	lastTick = spring_gettime() - spring_msecs(1);
	CreateNewFrame(true, false);
}

void CGameServer::SetGamePausable(const bool arg)
{
	gamePausable = arg;
}

void CGameServer::PushAction(const Action& action)
{
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
			for (size_t a=0; a < players.size();++a)
			{
				std::string playerLower = StringToLower(players[a].name);
				if (playerLower.find(name)==0)
				{	// can kick on substrings of name
					if (!players[a].isLocal) // do not kick host
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
	else if (action.command == "nospecdraw")
	{
		SetBoolArg(allowSpecDraw, action.extra);
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
		if (!spring_istime(gameStartTime))
			CheckForGameStart(true);
	}
	else if (action.command == "skip")
	{
		if (demoReader)
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
			CreateNewFrame(true, true);
	}
#ifdef DEDICATED // we already have a quit command in the client
	else if (action.command == "kill")
	{
		quitServer = true;
	}
	else if (action.command == "pause")
	{
		isPaused = !isPaused;
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
	if (spring_istime(gameEndTime)) {
		if (gameEndTime < spring_gettime() - spring_secs(2)) {
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
	std::vector<int> numActiveTeams(teams.size(), 0); // active teams per ally team

	for (size_t a = 0; a < teams.size(); ++a)
	{
		bool hasController = false;
		for (size_t b = 0; b < players.size() && !hasController; ++b) {
			if (!players[b].spectator && players[b].team == (int)a) {
				hasController = true;
			}
		}

		for (std::map<size_t, GameSkirmishAI>::const_iterator ai = ais.begin(); ai != ais.end() && !hasController; ++ai) {
			if (ai->second.team == a) {
				hasController = true;
			}
		}

		if (teams[a].active && hasController) {
			++numActiveTeams[teams[a].teamAllyteam];
		}
	}

	for (size_t a = 0; a < numActiveTeams.size(); ++a) {
		if (numActiveTeams[a] != 0) {
			++numActiveAllyTeams;
		}
	}

	if (numActiveAllyTeams <= 1)
	{
		gameEndTime = spring_gettime();
		Broadcast(CBaseNetProtocol::Get().SendSendPlayerStat());
	}
}

void CGameServer::CreateNewFrame(bool fromServerThread, bool fixedFrameTime)
{
	if (!demoReader) // use NEWFRAME_MSGes from demo otherwise
	{
#if BOOST_VERSION >= 103500
		if (!fromServerThread)
			boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex, boost::defer_lock);
		else
			boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
#else
		boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex, !fromServerThread);
#endif
		CheckSync();
		int newFrames = 1;

		if(!fixedFrameTime){
			spring_time currentTick = spring_gettime();
			spring_duration timeElapsed = currentTick - lastTick;

			if (timeElapsed > spring_msecs(200)) {
				timeElapsed = spring_msecs(200);
			}

			timeLeft += GAME_SPEED * internalSpeed * float(spring_tomsecs(timeElapsed)) * 0.001f;
			lastTick=currentTick;
			newFrames = (timeLeft > 0)? int(ceil(timeLeft)): 0;
			timeLeft -= newFrames;

			if (hasLocalClient)
			{
				// needs to set lastTick and stuff, otherwise we will get all the left out NEWFRAME's at once when client has catched up
				if (players[localClientNumber].lastFrameResponse + GAME_SPEED*2 <= serverframenum)
					return;
			}
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
		spring_sleep(spring_msecs(10));

		if (UDPNet)
			UDPNet->Update();

		boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
		ServerReadNet();
		Update();
	}
	if (hostif)
		hostif->SendQuit();
	Broadcast(CBaseNetProtocol::Get().SendQuit("Server shutdown"));
}

bool CGameServer::WaitsOnCon() const
{
	return (UDPNet && UDPNet->Listen());
}

bool CGameServer::GameHasStarted() const
{
	return (spring_istime(gameStartTime));
}

void CGameServer::KickPlayer(const int playerNum)
{
	if (players[playerNum].link) // only kick connected players
	{
		Message(str(format(PlayerLeft) %players[playerNum].GetType() %players[playerNum].name %"kicked"));
		Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(playerNum, 2));
		players[playerNum].Kill("Kicked from the battle");
		if (hostif)
		{
			hostif->SendPlayerLeft(playerNum, 2);
		}
	}
	else
		Message(str( format("Attempt to kick player %d who is not connected") %playerNum ));
}

unsigned CGameServer::BindConnection(std::string name, const std::string& passwd, const std::string& version, bool isLocal, boost::shared_ptr<netcode::CConnection> link)
{
	Message(str(format("Connection attempt from %s") %name));
	Message(str(format(" -> Version: %s") %version));
	Message(str(format(" -> Address: %s") %link->GetFullAddress()), false);
	size_t hisNewNumber = players.size();

	for (size_t i = 0; i < players.size(); ++i)
	{
		if (!players[i].isFromDemo && name == players[i].name)
		{
			if (players[i].myState == GameParticipant::UNCONNECTED || players[i].myState == GameParticipant::DISCONNECTED)
			{
				hisNewNumber = i;
				break;
			}
			else
			{
				Message(str(format(" -> %s is already ingame") %name));
				name += "_";
			}
		}
		else if (name == players[i].name)
		{
			Message(str(format(" -> %s (%i) duplicated in the demo") %name %i));
			name += "_";
		}
	}

	if (hisNewNumber >= players.size())
	{
		if (demoReader || allowAdditionalPlayers)
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
			Message(str(format(" -> %s not found in script, rejecting connection attempt") %name));
			link->SendData(CBaseNetProtocol::Get().SendQuit(str(format("Unknown playername: %s") %name)));
			return 0;
		}
	}

	GameParticipant::customOpts::const_iterator it = players[hisNewNumber].GetAllValues().find("Password");
	if (it != players[hisNewNumber].GetAllValues().end() && !isLocal)
	{
		if (passwd != it->second)
		{
			Message(str(format(" -> rejected because of wrong password")));
			link->SendData(CBaseNetProtocol::Get().SendQuit(str(format("Wrong passkey: %s") %name)));
			return 0;
		};
	}
	GameParticipant& newGuy = players[hisNewNumber];
	newGuy.Connected(link, isLocal);
	newGuy.SendData(boost::shared_ptr<const RawPacket>(gameData->Pack()));
	newGuy.SendData(CBaseNetProtocol::Get().SendSetPlayerNum((unsigned char)hisNewNumber));

	// after gamedata and playernum, the player can start loading
	for (std::list< boost::shared_ptr<const netcode::RawPacket> >::const_iterator it = packetCache.begin(); it != packetCache.end(); ++it)
	{
		newGuy.SendData(*it); // throw at him all stuff he missed until now
	}

	if (!demoReader || setup->demoName.empty()) // gamesetup from demo?
	{
		const unsigned hisTeam = setup->playerStartingData[hisNewNumber].team;
		if (!players[hisNewNumber].spectator && !teams[hisTeam].active) // create new team
		{
			players[hisNewNumber].readyToStart = (setup->startPosType != CGameSetup::StartPos_ChooseInGame);
			teams[hisTeam].active = true;
		}
			players[hisNewNumber].team = hisTeam;

		if (!setup->playerStartingData[hisNewNumber].spectator)
			Broadcast(CBaseNetProtocol::Get().SendJoinTeam(hisNewNumber, hisTeam));
	}

	Message(str(format(" -> connection established (given id %i)") %hisNewNumber));

	link->Flush(true);
	return hisNewNumber;
}

void CGameServer::GotChatMessage(const ChatMessage& msg)
{
	if (!msg.msg.empty()) // silently drop empty chat messages
	{
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
		if (hostif && msg.fromPlayer >= 0 && static_cast<unsigned int>(msg.fromPlayer) != SERVER_PLAYER) {
			// do not echo packets to the autohost
			hostif->SendPlayerChat(msg.fromPlayer, msg.destination,  msg.msg);
		}
	}
}

void CGameServer::InternalSpeedChange(float newSpeed)
{
	if (internalSpeed == newSpeed) {
		// TODO some error here
	}
	Broadcast(CBaseNetProtocol::Get().SendInternalSpeed(newSpeed));
	internalSpeed = newSpeed;
}

void CGameServer::UserSpeedChange(float newSpeed, int player)
{
	if (enforceSpeed > 0 &&
		player >= 0 && static_cast<unsigned int>(player) != SERVER_PLAYER &&
		!players[player].isLocal && !isPaused &&
		(players[player].spectator || (enforceSpeed > 0 &&
		(players[player].cpuUsage - medianCpu > std::min(0.2f, std::max(0.0f, 0.8f - medianCpu) ) ||
		(serverframenum - players[player].lastFrameResponse) - medianPing > internalSpeed * GAME_SPEED / 2)))) {
		PrivateMessage(player, "Speed change rejected (cpu load or ping is too high)");
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

size_t CGameServer::ReserveNextAvailableSkirmishAIId() {

	size_t skirmishAIId = 0;

	// find a free id
	std::list<size_t>::iterator it;
	for (it = usedSkirmishAIIds.begin(); it != usedSkirmishAIIds.end(); ++it, skirmishAIId++) {
		if (*it != skirmishAIId) {
			break;
		}
	}

	usedSkirmishAIIds.insert(it, skirmishAIId);

	return skirmishAIId;
}

void CGameServer::FreeSkirmishAIId(const size_t skirmishAIId) {
	usedSkirmishAIIds.remove(skirmishAIId);
}
