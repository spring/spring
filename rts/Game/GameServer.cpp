/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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

#include "LogOutput.h"
#include "GameSetup.h"
#include "Action.h"
#include "ChatMessage.h"
#include "CommandMessage.h"
#include "BaseNetProtocol.h"
#include "PlayerHandler.h"
#include "Net/LocalConnection.h"
#include "Net/UnpackPacket.h"
#include "LoadSave/DemoReader.h"
#ifdef DEDICATED
	#include "LoadSave/DemoRecorder.h"
#endif
#include "AutohostInterface.h"
#include "Util.h"
#include "TdfParser.h"
#include "GlobalUnsynced.h" // for syncdebug
#include "Sim/Misc/GlobalConstants.h"
#include "ConfigHandler.h"
#include "FileSystem/CRC.h"
#include "FileSystem/SimpleParser.h"
#include "Player.h"
#include "IVideoCapturing.h"
#include "Server/GameParticipant.h"
#include "Server/GameSkirmishAI.h"
// This undef is needed, as somewhere there is a type interface specified,
// which we need not!
// (would cause problems in ExternalAI/Interface/SAIInterfaceLibrary.h)
#ifdef interface
	#undef interface
#endif
#include "Server/MsgStrings.h"

#define PKTCACHE_VECSIZE 1000

using netcode::RawPacket;


/// frames until a syncchech will time out and a warning is given out
const unsigned SYNCCHECK_TIMEOUT = 300;

/// used to prevent msg spam
const unsigned SYNCCHECK_MSG_TIMEOUT = 400;

/// The time intervall in msec for sending player statistics to each client
const spring_duration playerInfoTime = spring_secs(2);

/// every n'th frame will be a keyframe (and contain the server's framenumber)
const unsigned serverKeyframeIntervall = 16;

/// players incoming bandwidth new allowance every X milliseconds
const unsigned playerBandwidthInterval = 100;

const std::string commands[numCommands] = {
	"kick", "kickbynum", "setminspeed", "setmaxspeed",
	"nopause", "nohelp", "cheat", "godmode", "globallos",
	"nocost", "forcestart", "nospectatorchat", "nospecdraw",
	"skip", "reloadcob", "reloadcegs", "devlua", "editdefs",
	"luagaia", "singlestep"
};
using boost::format;

namespace {
void SetBoolArg(bool& value, const std::string& str)
{
	if (str.empty()) { // toggle
		value = !value;
	}
	else { // set
		const int num = atoi(str.c_str());
		value = (num != 0);
	}
}
}


CGameServer* gameServer = 0;

CGameServer::CGameServer(const std::string& hostIP, int hostPort, const GameData* const newGameData, const CGameSetup* const mysetup)
: setup(mysetup)
{
	assert(setup);
	serverStartTime = spring_gettime();
	lastUpdate = serverStartTime;
	lastPlayerInfo = serverStartTime;
	syncErrorFrame = 0;
	syncWarningFrame = 0;
	serverFrameNum = 0;
	timeLeft = 0;
	modGameTime = 0.0f;
	gameTime = 0.0f;
	startTime = 0.0f;
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

	gameHasStarted = false;
	gameEndTime = spring_notime;
	readyTime = spring_notime;

	medianCpu = 0.0f;
	medianPing = 0;
	// speedControl - throttles speed based on:
	// 0 : players (max cpu)
	// 1 : players (median cpu)
	// 2 : (same as 0)
	// -x: same as x, but ignores votes from players that may change the speed control mode
	curSpeedCtrl = 0;
	speedControl = configHandler->Get("SpeedControl", 0);
	UpdateSpeedControl(--speedControl + 1);

	allowAdditionalPlayers = configHandler->Get("AllowAdditionalPlayers", false);
	whiteListAdditionalPlayers = configHandler->Get("WhiteListAdditionalPlayers", true);

	if (!setup->onlyLocal) {
		UDPNet.reset(new netcode::UDPListener(hostPort, hostIP));
	}

	std::string autohostip = configHandler->Get("AutohostIP", std::string("127.0.0.1"));
	if (StringToLower(autohostip) == "localhost") {
		// FIXME temporary hack: we do not support (host-)names.
		// "localhost" was the only name supported in the past.
		// added 7. January 2011, to be removed in ~ 1 year
		autohostip = "127.0.0.1";
	}
	const int autohostport = configHandler->Get("AutohostPort", 0);

	if (autohostport > 0) {
		AddAutohostInterface(autohostip, autohostport);
	}

	rng.Seed(newGameData->GetSetup().length());
	Message(str(format(ServerStart) %hostPort), false);

	lastTick = spring_gettime();

	maxUserSpeed = setup->maxSpeed;
	minUserSpeed = setup->minSpeed;
	noHelperAIs = (bool)setup->noHelperAIs;

	{ // modify and save GameSetup text (remove passwords)
		TdfParser parser(newGameData->GetSetup().c_str(), newGameData->GetSetup().length());
		TdfParser::TdfSection* root = parser.GetRootSection();
		for (TdfParser::sectionsMap_t::iterator it = root->sections.begin(); it != root->sections.end(); ++it) {
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

	if (setup->hostDemo) {
		Message(str(format(PlayingDemo) %setup->demoName));
		demoReader.reset(new CDemoReader(setup->demoName, modGameTime + 0.1f));
	}

	players.resize(setup->playerStartingData.size());
	std::copy(setup->playerStartingData.begin(), setup->playerStartingData.end(), players.begin());
	UpdatePlayerNumberMap();

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
	demoRecorder->SaveToDemo(ret->data, ret->length, GetDemoTime());
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

	canReconnect = false;
	linkMinPacketSize = gc->linkIncomingMaxPacketRate > 0 ? (gc->linkIncomingSustainedBandwidth / gc->linkIncomingMaxPacketRate) : 1;
	lastBandwidthUpdate = spring_gettime();

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
	int numTeams = (int)setup->teamStartingData.size();
	if (setup->useLuaGaia && (numTeams > 0)) {
		--numTeams;
	}
	demoRecorder->SetTime(serverFrameNum / 30, spring_tomsecs(spring_gettime()-serverStartTime)/1000);
	demoRecorder->InitializeStats(players.size(), numTeams);

	// Pass the winners to the CDemoRecorder.
	demoRecorder->SetWinningAllyTeams(winningAllyTeams);
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

void CGameServer::AddAutohostInterface(const std::string& autohostIP, const int autohostPort)
{
	if (!hostif) {
		hostif.reset(new AutohostInterface(autohostIP, autohostPort));
		if (hostif->IsInitialized()) {
			hostif->SendStart();
			Message(str(format(ConnectAutohost) %autohostPort), false);
		} else {
			// Quit if we are instructed to communicate with an auto-host,
			// but are unable to do so. As we do not want an auto-host running
			// a spring game that he has no control over. If we get here,
			// it suggests a configuration problem in the auto-host.
			hostif.reset();
			Message(str(format(ConnectAutohostFailed) %autohostIP %autohostPort), false);
			quitServer = true;
		}
	}
}

void CGameServer::PostLoad(unsigned newlastTick, int newServerFrameNum)
{
	boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex);
#if SPRING_TIME
	lastTick = newlastTick;
#else
	//lastTick = boost::some_time; FIXME
#endif
	serverFrameNum = newServerFrameNum;

	std::vector<GameParticipant>::iterator it;
	for (it = players.begin(); it != players.end(); ++it) {
		it->lastFrameResponse = newServerFrameNum;
	}
}

void CGameServer::SkipTo(int targetFrame)
{
	const bool wasPaused = isPaused;

	if ((serverFrameNum < targetFrame) && demoReader) {
		CommandMessage msg(str(boost::format("skip start %d") %targetFrame), SERVER_PLAYER);
		Broadcast(boost::shared_ptr<const netcode::RawPacket>(msg.Pack()));

		// fast-read and send demo data
		// if SendDemoData reaches EOS, demoReader becomes NULL
		while ((serverFrameNum < targetFrame) && demoReader) {
			float toSkipSecs = 0.1f; // ~ 3 frames
			if ((targetFrame - serverFrameNum) < 10) {
				// we only want to advance one frame at most, so
				// the next time-index must be "close enough" not
				// to move past more than 1 NETMSG_NEWFRAME
				toSkipSecs = (1.0f / (GAME_SPEED * 2)); // 1 frame
			}
			// skip time
			gameTime += toSkipSecs;
			modGameTime += toSkipSecs;

			SendDemoData(true);
			if (((serverFrameNum % 20) == 0) && UDPNet) {
				// send data every few frames, as otherwise packets would grow too big
				UDPNet->Update();
			}
		}

		CommandMessage msg2("skip end", SERVER_PLAYER);
		Broadcast(boost::shared_ptr<const netcode::RawPacket>(msg2.Pack()));

		if (UDPNet)
			UDPNet->Update();

		lastUpdate = spring_gettime();
	}

	// note: unnecessary, pause-commands from
	// demos are not broadcast anyway so /skip
	// cannot change the state
	isPaused = wasPaused;
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


void CGameServer::UpdatePlayerNumberMap() {
	unsigned char player = 0;
	for (int i = 0; i < 256; ++i, ++player) {
		if (i < players.size() && !players[i].isFromDemo)
			++player;
		playerNumberMap[i] = (i < 250) ? player : i; // ignore SERVER_PLAYER, ChatMessage::TO_XXX etc
	}
}


bool CGameServer::AdjustPlayerNumber(netcode::RawPacket* buf, int pos, int val) {
	if (buf->length <= pos) {
		Message(str(format("Warning: Discarding short packet in demo: ID %d, LEN %d") %buf->data[0] %buf->length));
		return false;
	}
	// spectators watching the demo will offset the demo spectators, compensate for this
	if (val < 0) {
		unsigned char player = playerNumberMap[buf->data[pos]];
		if (player >= players.size() && player < 250) { // ignore SERVER_PLAYER, ChatMessage::TO_XXX etc
			Message(str(format("Warning: Discarding packet with invalid player number in demo: ID %d, LEN %d") %buf->data[0] %buf->length));
			return false;
		}
		buf->data[pos] = player;
	}
	else {
		buf->data[pos] = val;
	}
	return true;
}


void CGameServer::SendDemoData(const bool skipping)
{
	netcode::RawPacket* buf = 0;
	while ((buf = demoReader->GetData(modGameTime))) {
		boost::shared_ptr<const RawPacket> rpkt(buf);
		if (buf->length <= 0) {
			Message(str(format("Warning: Discarding zero size packet in demo")));
			continue;
		}
		unsigned msgCode = buf->data[0];
		switch (msgCode) {
			case NETMSG_NEWFRAME:
			case NETMSG_KEYFRAME: {
				// we can't use CreateNewFrame() here
				lastTick = spring_gettime();
				serverFrameNum++;
#ifdef SYNCCHECK
				if (!skipping)
					outstandingSyncFrames.insert(serverFrameNum);
				CheckSync();
#endif
				Broadcast(rpkt);
				break;
			}
			case NETMSG_AI_STATE_CHANGED: /* many of these messages are not likely to be sent by a spec, but there are cheats */
			case NETMSG_ALLIANCE:
			case NETMSG_CUSTOM_DATA:
			case NETMSG_DC_UPDATE:
			case NETMSG_DIRECT_CONTROL:
			case NETMSG_PATH_CHECKSUM:
			case NETMSG_PAUSE: /* this is a synced message and must not be excluded */
			case NETMSG_PLAYERINFO:
			case NETMSG_PLAYERLEFT:
			case NETMSG_PLAYERSTAT:
			case NETMSG_SETSHARE:
			case NETMSG_SHARE:
			case NETMSG_STARTPOS:
			case NETMSG_TEAM: {
				// TODO: more messages may need adjusted player numbers, or maybe there is a better solution
				if (!AdjustPlayerNumber(buf, 1))
					continue;
				Broadcast(rpkt);
				break;
			}
			case NETMSG_AI_CREATED:
			case NETMSG_MAPDRAW:
			case NETMSG_PLAYERNAME: {
				if (!AdjustPlayerNumber(buf, 2))
					continue;
				Broadcast(rpkt);
				break;
			}
			case NETMSG_CHAT: {
				if (!AdjustPlayerNumber(buf, 2) || !AdjustPlayerNumber(buf, 3))
					continue;
				Broadcast(rpkt);
				break;
			}
			case NETMSG_AICOMMAND:
			case NETMSG_AISHARE:
			case NETMSG_COMMAND:
			case NETMSG_LUAMSG:
			case NETMSG_SELECT:
			case NETMSG_SYSTEMMSG: {
				if (!AdjustPlayerNumber(buf ,3))
					continue;
				Broadcast(rpkt);
				break;
			}
			case NETMSG_CREATE_NEWPLAYER: {
				if (!AdjustPlayerNumber(buf, 3, players.size()))
					continue;
				try {
					netcode::UnpackPacket pckt(rpkt, 3);
					unsigned char spectator, team, playerNum;
					std::string name;
					pckt >> playerNum;
					pckt >> spectator;
					pckt >> team;
					pckt >> name;
					AddAdditionalUser(name, "", true); // even though this is a demo, keep the players vector properly updated
				} catch (netcode::UnpackPacketException &e) {
					Message(str(format("Warning: Discarding invalid new player packet in demo: %s") %e.err));
					continue;
				}

				Broadcast(rpkt);
				break;
			}
			case NETMSG_GAMEDATA:
			case NETMSG_SETPLAYERNUM:
			case NETMSG_USER_SPEED:
			case NETMSG_INTERNAL_SPEED: {
				// dont send these from demo
				break;
			}
			default: {
				Broadcast(rpkt);
				break;
			}
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
		players[p].SendData(packet);
	if (canReconnect || allowAdditionalPlayers || !gameHasStarted)
		AddToPacketCache(packet);
#ifdef DEDICATED
	if (demoRecorder)
		demoRecorder->SaveToDemo(packet->data, packet->length, GetDemoTime());
#endif
}

void CGameServer::Message(const std::string& message, bool broadcast)
{
	if (broadcast) {
		Broadcast(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
	}
	else if (hasLocalClient) {
		// host should see
		players[localClientNumber].SendData(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
	}
	if (hostif)
		hostif->Message(message);
#if defined DEDICATED
	std::cout << message << std::endl;
#endif
}

void CGameServer::PrivateMessage(int playerNum, const std::string& message) {
	players[playerNum].SendData(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
}

void CGameServer::CheckSync()
{
#ifdef SYNCCHECK
	// Check sync
	std::set<int>::iterator f = outstandingSyncFrames.begin();
	while (f != outstandingSyncFrames.end()) {
		unsigned correctChecksum = 0;
		bool bGotCorrectChecksum = false;
		if (hasLocalClient) {
			// dictatorship
			std::map<int, unsigned>::iterator it = players[localClientNumber].syncResponse.find(*f);
			if (it != players[localClientNumber].syncResponse.end()) {
				correctChecksum = it->second;
				bGotCorrectChecksum = true;
			}
		}
		else {
			// democracy
			typedef std::vector< std::pair<unsigned, unsigned> > chkList;
			chkList checksums;
			unsigned checkMaxCount = 0;
			for (size_t a = 0; a < players.size(); ++a) {
				if (!players[a].link)
					continue;

				std::map<int, unsigned>::const_iterator it = players[a].syncResponse.find(*f);
				if (it != players[a].syncResponse.end()) {
					bool found = false;
					for (chkList::iterator it2 = checksums.begin(); it2 != checksums.end(); ++it2) {
						if (it2->first == it->second) {
							found = true;
							it2->second++;
							if (checkMaxCount < it2->second) {
								checkMaxCount = it2->second;
								correctChecksum = it2->first;
							}
						}
					}
					if (!found) {
						checksums.push_back(std::pair<unsigned, unsigned>(it->second, 1));
						if (checkMaxCount == 0) {
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
				if (*f >= serverFrameNum - static_cast<int>(SYNCCHECK_TIMEOUT))
					bComplete = false;
				else if (*f < players[a].lastFrameResponse)
					noSyncResponse.push_back(a);
			} else {
				if (bGotCorrectChecksum && it->second != correctChecksum) {
					players[a].desynced = true;
					if (demoReader || !players[a].spectator)
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
		if (/*bComplete && */ (!desyncGroups.empty() || !desyncSpecs.empty())) {
			if (!syncErrorFrame || (*f - syncErrorFrame > static_cast<int>(SYNCCHECK_MSG_TIMEOUT))) {
				syncErrorFrame = *f;

				// TODO enable this when we have resync
				//serverNet->SendPause(SERVER_PLAYER, true);
#ifdef SYNCDEBUG
				CSyncDebugger::GetInstance()->ServerTriggerSyncErrorHandling(serverFrameNum);
				if (demoReader) // pause is a synced message, thus demo spectators may not pause for real
					Message(str(format("%s paused the demo") %players[gu->myPlayerNum].name));
				else
					Broadcast(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, true));
				isPaused = true;
				Broadcast(CBaseNetProtocol::Get().SendSdCheckrequest(serverFrameNum));
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
					int playerNum = s->first;
					PrivateMessage(playerNum, str(format(SyncError) %players[playerNum].name %(*f) %(s->second ^ correctChecksum)));
				}
			}
		}

		// Remove complete sets (for which all player's checksums have been received).
		if (bComplete) {
			// Message(str (boost::format("Succesfully purged outstanding sync frame %d from the deque") %(*f)));
			for (size_t a = 0; a < players.size(); ++a) {
				if (players[a].myState < GameParticipant::DISCONNECTED)
					players[a].syncResponse.erase(*f);
			}
			f = set_erase(outstandingSyncFrames, f);
		} else
			++f;
	}
#else
	// Make it clear this build isn't suitable for release.
	if (!syncErrorFrame || (serverFrameNum - syncErrorFrame > SYNCCHECK_MSG_TIMEOUT)) {
		syncErrorFrame = serverFrameNum;
		Message(NoSyncCheck);
	}
#endif
}

float CGameServer::GetDemoTime() const {
	return !gameHasStarted ? gameTime : startTime + (float)serverFrameNum / (float)GAME_SPEED;
}

void CGameServer::Update()
{
	float tdif = float(spring_tomsecs(spring_gettime() - lastUpdate)) * 0.001f;
	gameTime += tdif;
	if (!isPaused && gameHasStarted) {
		if (!demoReader || !hasLocalClient || (serverFrameNum - players[localClientNumber].lastFrameResponse) < GAME_SPEED)
			modGameTime += tdif * internalSpeed;
	}
	lastUpdate = spring_gettime();

	if (lastPlayerInfo < (spring_gettime() - playerInfoTime)) {
		lastPlayerInfo = spring_gettime();

		if (serverFrameNum > 0) {
			//send info about the players
			std::vector<float> cpu;
			std::vector<int> ping;
			float refCpu = 0.0f;
			for (size_t a = 0; a < players.size(); ++a) {
				if (players[a].myState == GameParticipant::INGAME) {
					int curPing = (serverFrameNum - players[a].lastFrameResponse);
					if (players[a].isReconn && curPing < 2 * GAME_SPEED)
						players[a].isReconn = false;
					Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, players[a].cpuUsage, curPing));
					float correctedCpu = players[a].isLocal ? players[a].cpuUsage :
						std::max(0.0f, std::min(players[a].cpuUsage - 0.0025f * (float)players[a].luaDrawTime, 1.0f));
					if (demoReader ? !players[a].isFromDemo : !players[a].spectator) {
						if (!players[a].isReconn && correctedCpu > refCpu)
							refCpu = correctedCpu;
						cpu.push_back(correctedCpu);
						ping.push_back(curPing);
					}
				}
			}

			medianCpu = 0.0f;
			medianPing = 0;
			if (curSpeedCtrl > 0 && !cpu.empty()) {
				std::sort(cpu.begin(), cpu.end());
				std::sort(ping.begin(), ping.end());

				int midpos = cpu.size() / 2;
				medianCpu = cpu[midpos];
				medianPing = ping[midpos];
				if (midpos * 2 == cpu.size()) {
					medianCpu = (medianCpu + cpu[midpos - 1]) / 2.0f;
					medianPing = (medianPing + ping[midpos - 1]) / 2;
				}
				refCpu = medianCpu;
			}

			if (refCpu > 0.0f) {
				// aim for 60% cpu usage if median is used as reference and 75% cpu usage if max is the reference
				float wantedCpu = (curSpeedCtrl > 0) ? 0.6f + (1 - internalSpeed / userSpeedFactor) * 0.5f : 0.75f + (1 - internalSpeed / userSpeedFactor) * 0.5f;
				float newSpeed = internalSpeed * wantedCpu / refCpu;
//				float speedMod=1+wantedCpu-refCpu;
//				logOutput.Print("Speed REF %f MED %f WANT %f SPEEDM %f NSPEED %f",refCpu,medianCpu,wantedCpu,speedMod,newSpeed);
				newSpeed = (newSpeed + internalSpeed) * 0.5f;
				newSpeed = std::max(newSpeed, (curSpeedCtrl > 0) ? userSpeedFactor * 0.8f : userSpeedFactor * 0.5f);
				if (newSpeed > userSpeedFactor)
					newSpeed = userSpeedFactor;
				if (newSpeed < 0.1f)
					newSpeed = 0.1f;
				if (newSpeed != internalSpeed)
					InternalSpeedChange(newSpeed);
			}
		}
		else {
			for (size_t a = 0; a < players.size(); ++a) {
				if (!players[a].isFromDemo) {
					if (players[a].myState == GameParticipant::CONNECTED) { // send pathing status
						if (players[a].cpuUsage > 0)
							Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, players[a].cpuUsage, PATHING_FLAG));
					}
					else {
						Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, 0, 0)); // reset status
					}
				}
			}
		}
	}

	if (!gameHasStarted)
		CheckForGameStart();
	else if (serverFrameNum > 0 || demoReader)
		CreateNewFrame(true, false);

	if (hostif) {
		std::string msg = hostif->GetChatMessage();

		if (!msg.empty()) {
			if (msg.at(0) != '/') { // normal chat message
				GotChatMessage(ChatMessage(SERVER_PLAYER, ChatMessage::TO_EVERYONE, msg));
			}
			else if (msg.at(0) == '/' && msg.size() > 1 && msg.at(1) == '/') { // chatmessage with prefixed '/'
				GotChatMessage(ChatMessage(SERVER_PLAYER, ChatMessage::TO_EVERYONE, msg.substr(1)));
			}
			else if (msg.size() > 1) { // command
				Action buf(msg.substr(1));
				PushAction(buf);
			}
		}
	}

	if (spring_gettime() > serverStartTime + spring_secs(gc->initialNetworkTimeout) || gameHasStarted) {
		bool hasPlayers = false;
		for (size_t i = 0; i < players.size(); ++i) {
			if (players[i].link) {
				hasPlayers = true;
				break;
			}
		}

		if (!hasPlayers) {
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
		if (!players[p].spectator && (players[p].team == teamId))
			playersInTeam.push_back(p);
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
		if ((ai->second.team == teamId) && ((hostPlayer == -2) || (ai->second.hostPlayer == hostPlayer)))
			skirmishAIIds.push_back(ai->first);
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

void CGameServer::ProcessPacket(const unsigned playerNum, boost::shared_ptr<const netcode::RawPacket> packet)
{
	const boost::uint8_t* inbuf = packet->data;
	const unsigned a = playerNum;
	unsigned msgCode = (unsigned) inbuf[0];

	switch (msgCode) {
		case NETMSG_KEYFRAME: {
			const int frameNum = *(int*)&inbuf[1];
			if(frameNum <= serverFrameNum && frameNum > players[a].lastFrameResponse)
				players[a].lastFrameResponse = frameNum;
			break;
		}

		case NETMSG_PAUSE:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (!inbuf[2])  // reset sync checker
				syncErrorFrame = 0;
			if (gamePausable || players[a].isLocal) { // allow host to pause even if nopause is set
				if (!players[a].isLocal && players[a].spectator && !demoReader) {
					PrivateMessage(a, "Spectators cannot pause the game");
				}
				else if (curSpeedCtrl > 0 && !isPaused && !players[a].isLocal &&
					(players[a].spectator || (curSpeedCtrl > 0 &&
					(players[a].cpuUsage - medianCpu > std::min(0.2f, std::max(0.0f, 0.8f - medianCpu)) ||
					(serverFrameNum - players[a].lastFrameResponse) - medianPing > internalSpeed * GAME_SPEED / 2)))) {
						PrivateMessage(a, "Pausing rejected (cpu load or ping is too high)");
				}
				else {
					timeLeft=0;
					if ((isPaused != !!inbuf[2]) || demoReader)
						isPaused = !isPaused;
					if (demoReader) // pause is a synced message, thus demo spectators may not pause for real
						Message(str(format("%s %s the demo") %players[a].name %(isPaused ? "paused" : "unpaused")));
					else
						Broadcast(CBaseNetProtocol::Get().SendPause(a, inbuf[2]));
				}
			}
			break;

		case NETMSG_USER_SPEED: {
			if (!players[a].isLocal && players[a].spectator && !demoReader) {
				PrivateMessage(a, "Spectators cannot change game speed");
			}
			else {
				float speed = *((float*) &inbuf[2]);
				UserSpeedChange(speed, a);
			}
		} break;

		case NETMSG_CPU_USAGE:
			players[a].cpuUsage = *((float*)&inbuf[1]);
			break;

		case NETMSG_CUSTOM_DATA: {
			unsigned playerNum = inbuf[1];
			if (playerNum != a) {
				Message(str(format(WrongPlayer) %msgCode %a %playerNum));
				break;
			}
			switch(inbuf[2]) {
				case CUSTOM_DATA_SPEEDCONTROL:
					players[a].speedControl = *((int*)&inbuf[3]);
					UpdateSpeedControl(speedControl);
					break;
				case CUSTOM_DATA_LUADRAWTIME:
					players[a].luaDrawTime = *((int*)&inbuf[3]);
					break;
				default:
					Message(str(format("Player %s sent invalid CustomData type %d") %players[a].name %inbuf[2]));
			}
			break;
		}

		case NETMSG_QUIT: {
			Message(str(format(PlayerLeft) %players[a].GetType() %players[a].name %" normal quit"));
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 1));
			players[a].Kill("User exited");
			UpdateSpeedControl(speedControl);
			if (hostif)
				hostif->SendPlayerLeft(a, 1);
			break;
		}

		case NETMSG_PLAYERNAME: {
			try {
				netcode::UnpackPacket pckt(packet, 2);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode %a %playerNum));
					break;
				}
				pckt >> players[playerNum].name;
				players[playerNum].myState = GameParticipant::INGAME;
				Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, 0, 0)); // reset pathing display
				Message(str(format(PlayerJoined) %players[playerNum].GetType() %players[playerNum].name), false);
				Broadcast(CBaseNetProtocol::Get().SendPlayerName(playerNum, players[playerNum].name));
				if (hostif)
					hostif->SendPlayerJoined(playerNum, players[playerNum].name);
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %d sent invalid PlayerName: %s") %a %e.err));
			}
			break;
		}

		case NETMSG_PATH_CHECKSUM: {
			const unsigned char playerNum = inbuf[1];
			const boost::uint32_t playerCheckSum = *(boost::uint32_t*) &inbuf[2];
			if (playerNum != a) {
				Message(str(format(WrongPlayer) %msgCode %a %playerNum));
				break;
			}
			Broadcast(CBaseNetProtocol::Get().SendPathCheckSum(playerNum, playerCheckSum));
		} break;

		case NETMSG_CHAT: {
			try {
				ChatMessage msg(packet);
				if (static_cast<unsigned>(msg.fromPlayer) != a) {
					Message(str(format(WrongPlayer) %msgCode %a %(unsigned)msg.fromPlayer));
					break;
				}
				GotChatMessage(msg);
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %s sent invalid ChatMessage: %s") %players[a].name %e.err));
			}
			break;
		}
		case NETMSG_SYSTEMMSG:
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				std::string strmsg;
				pckt >> strmsg;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode %a %(unsigned)playerNum));
					break;
				}
				Broadcast(CBaseNetProtocol::Get().SendSystemMessage(playerNum, strmsg));
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %d sent invalid SystemMessage: %s") %a %e.err));
			}
			break;

		case NETMSG_STARTPOS: {
			const unsigned player = inbuf[1];
			if (player != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (setup->startPosType == CGameSetup::StartPos_ChooseInGame) {
				const unsigned team     = (unsigned)inbuf[2];
				if (team >= teams.size()) {
					Message(str(boost::format("Invalid teamID %d in NETMSG_STARTPOS from player %d") %team %player));
				} else if (getSkirmishAIIds(ais, team, player).empty() && ((team != players[player].team) || (players[player].spectator))) {
					Message(str(boost::format("Player %d sent spoofed NETMSG_STARTPOS with teamID %d") %player %team));
				} else {
					teams[team].startPos = float3(*((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12]));
					if (inbuf[3] == 1) {
						players[player].readyToStart = static_cast<bool>(inbuf[3]);
					}

					Broadcast(CBaseNetProtocol::Get().SendStartPos(inbuf[1],team, inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12]))); //forward data
					if (hostif)
						hostif->SendPlayerReady(a, inbuf[3]);
				}
			}
			else {
				Message(str(format(NoStartposChange) %a));
			}
			break;
		}

		case NETMSG_COMMAND:
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode %a %(unsigned)playerNum));
					break;
				}
				if (!demoReader)
					Broadcast(packet); //forward data
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %s sent invalid Command: %s") %players[a].name %e.err));
			}
			break;

		case NETMSG_SELECT:
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode %a %(unsigned)playerNum));
					break;
				}
				if (!demoReader)
					Broadcast(packet); //forward data
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %s sent invalid Select: %s") %players[a].name %e.err));
			}
			break;

		case NETMSG_AICOMMAND: {
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode  %a  %(unsigned) playerNum));
					break;
				}
				if (noHelperAIs)
					Message(str(format(NoHelperAI) %players[a].name %a));
				else if (!demoReader)
					Broadcast(packet); //forward data
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %s sent invalid AICommand: %s") %players[a].name %e.err));
			}
		}
		break;

		case NETMSG_AICOMMANDS: {
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode  %a  %(unsigned) playerNum));
					break;
				}
				if (noHelperAIs)
					Message(str(format(NoHelperAI) %players[a].name %a));
				else if (!demoReader)
					Broadcast(packet); //forward data
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %s sent invalid AICommands: %s") %players[a].name %e.err));
			}
		} break;

		case NETMSG_AISHARE: {
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode  %a  %(unsigned) playerNum));
					break;
				}
				if (noHelperAIs)
					Message(str(format(NoHelperAI) %players[a].name %a));
				else if (!demoReader)
					Broadcast(packet); //forward data
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %s sent invalid AIShare: %s") %players[a].name %e.err));
			}
		} break;

		case NETMSG_LUAMSG:
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode %a %(unsigned)playerNum));
					break;
				}
				if (!demoReader) {
					Broadcast(packet); //forward data
					if (hostif)
						hostif->SendLuaMsg(packet->data, packet->length);
				}
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %s sent invalid LuaMsg: %s") %players[a].name %e.err));
			}
			break;

		case NETMSG_SYNCRESPONSE: {
#ifdef SYNCCHECK
			const int frameNum = *(int*)&inbuf[1];
			if (outstandingSyncFrames.find(frameNum) != outstandingSyncFrames.end())
				players[a].syncResponse[frameNum] = *(unsigned*)&inbuf[5];
				// update players' ping (if !defined(SYNCCHECK) this is done in NETMSG_KEYFRAME)
			if (frameNum <= serverFrameNum && frameNum > players[a].lastFrameResponse)
				players[a].lastFrameResponse = frameNum;
#endif
		}
			break;

		case NETMSG_SHARE:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (!demoReader)
				Broadcast(CBaseNetProtocol::Get().SendShare(inbuf[1], inbuf[2], inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8])));
			break;

		case NETMSG_SETSHARE:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (!demoReader)
				Broadcast(CBaseNetProtocol::Get().SendSetShare(inbuf[1], inbuf[2], *((float*)&inbuf[3]), *((float*)&inbuf[7])));
			break;

		case NETMSG_PLAYERSTAT:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			players[a].lastStats = *(PlayerStatistics*)&inbuf[2];
			Broadcast(packet); //forward data
			break;

		case NETMSG_MAPDRAW:
			try {
				netcode::UnpackPacket pckt(packet, 2);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode %a %(unsigned)playerNum));
					break;
				}
				if (!players[playerNum].spectator || allowSpecDraw)
					Broadcast(packet); //forward data
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %s sent invalid MapDraw: %s") %players[a].name %e.err));
			}
			break;

		case NETMSG_DIRECT_CONTROL:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (!demoReader) {
				if (!players[inbuf[1]].spectator)
					Broadcast(CBaseNetProtocol::Get().SendDirectControl(inbuf[1]));
				else
					Message(str(format("Error: spectator %s tried direct-controlling a unit") %players[inbuf[1]].name));
			}
			break;

		case NETMSG_DC_UPDATE:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (!demoReader)
				Broadcast(CBaseNetProtocol::Get().SendDirectControlUpdate(inbuf[1], inbuf[2], *((short*)&inbuf[3]), *((short*)&inbuf[5])));
			break;

		case NETMSG_STARTPLAYING: {
			if (players[a].isLocal && gameHasStarted)
				CheckForGameStart(true);
			break;
		}
		case NETMSG_TEAM: {
			//TODO update players[] and teams[] and send all to hostif
			const unsigned player = (unsigned)inbuf[1];
			if (player != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)player));
				break;
			}
			const unsigned action = inbuf[2];
			const unsigned fromTeam = players[player].team;

			switch (action) {
				case TEAMMSG_GIVEAWAY: {
					const unsigned toTeam                    = inbuf[3];
					// may be the players team or a team controlled by one of his AIs
					const unsigned fromTeam_g                = inbuf[4];
					const int numPlayersInTeam_g             = countNumPlayersInTeam(players, fromTeam_g);
					const std::vector<size_t> &totAIsInTeam_g = getSkirmishAIIds(ais, fromTeam_g);
					const std::vector<size_t> &myAIsInTeam_g  = getSkirmishAIIds(ais, fromTeam_g, player);
					const size_t numControllersInTeam_g      = numPlayersInTeam_g + totAIsInTeam_g.size();
					const bool isLeader_g                    = (teams[fromTeam_g].leader == player);
					const bool isOwnTeam_g                   = (fromTeam_g == fromTeam);
					const bool isSpec                        = players[player].spectator;
					const bool hasAIs_g                      = (!myAIsInTeam_g.empty());
					const bool isAllied_g                    = (teams[fromTeam_g].teamAllyteam == teams[fromTeam].teamAllyteam);
					const char* playerType                   = players[player].GetType();
					const bool isSinglePlayer                = (players.size() <= 1);

					if (!isSinglePlayer &&
						(isSpec ||
						(!isOwnTeam_g && !isLeader_g) ||
						(hasAIs_g && !isAllied_g && !cheating))) {
							Message(str(boost::format("%s %s sent invalid team giveaway") %playerType %players[player].name), true);
							break;
					}
					Broadcast(CBaseNetProtocol::Get().SendGiveAwayEverything(player, toTeam, fromTeam_g));

					bool giveAwayOk = false;
					if (isOwnTeam_g) {
						// player is giving stuff from his own team
						giveAwayOk = true;
						//players[player].team = 0;
						players[player].spectator = true;
						if (hostif)
							hostif->SendPlayerDefeated(player);
					} else {
						// player is giving stuff from one of his AI teams
						if (numPlayersInTeam_g == 0) {
							// kill the first AI
							ais.erase(myAIsInTeam_g[0]);
							giveAwayOk = true;
						} else {
							Message(str(boost::format("%s %s can not give away stuff of team %i (still has human players left)") %playerType %players[player].name %fromTeam_g), true);
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

					if (isSpec && !isSinglePlayer) {
						Message(str(boost::format("Spectator %s sent invalid team resign") %players[player].name), true);
						break;
					}
					Broadcast(CBaseNetProtocol::Get().SendResign(player));

					//players[player].team = 0;
					players[player].spectator = true;
					// actualize all teams of which the player is leader
					for (size_t t = 0; t < teams.size(); ++t) {
						if (teams[t].leader == player) {
							const std::vector<int> &teamPlayers = getPlayersInTeam(players, t);
							const std::vector<size_t> &teamAIs  = getSkirmishAIIds(ais, t);
							if ((teamPlayers.size() + teamAIs.size()) == 0) {
								// no controllers left in team
								teams[t].active = false;
								teams[t].leader = -1;
							} else if (teamPlayers.empty()) {
								// no human player left in team
								teams[t].leader = ais[teamAIs[0]].hostPlayer;
							} else {
								// still human controllers left in team
								teams[t].leader = teamPlayers[0];
							}
						}
					}
					if (hostif)
						hostif->SendPlayerDefeated(player);
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
				case TEAMMSG_TEAM_DIED: {
					const unsigned team = inbuf[3];
#ifndef DEDICATED
					if (players[player].isLocal) { // currently only host is allowed
#else
					if (!players[player].desynced) {
#endif
						teams[team].active = false;
						teams[team].leader = -1;
						// convert all the teams players to spectators
						for (size_t p = 0; p < players.size(); ++p) {
							if ((players[p].team == team) && !(players[p].spectator)) {
								// are now spectating if this was their team
								//players[p].team = 0;
								players[p].spectator = true;
								if (hostif)
									hostif->SendPlayerDefeated(p);
								Broadcast(CBaseNetProtocol::Get().SendTeamDied(player, team));
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
		case NETMSG_AI_CREATED: {
			try {
				netcode::UnpackPacket pckt(packet, 2);
				unsigned char playerId;
				pckt >> playerId;
				unsigned int skirmishAIId_rec; // 4 bytes; should be -1, as we have to create the real one
				pckt >> skirmishAIId_rec;
				unsigned char aiTeamId;
				pckt >> aiTeamId;
				std::string aiName;
				pckt >> aiName;

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
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format("Player %s sent invalid AICreated: %s") %players[a].name %e.err));
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
			if (player != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)player));
				break;
			}
			if (whichAllyTeam == teams[players[a].team].teamAllyteam) {
				Message(str(format("Player %s tried to send spoofed alliance message") %players[a].name));
			}
			else {
				if (!setup->fixedAllies) {
					Broadcast(CBaseNetProtocol::Get().SendSetAllied(player, whichAllyTeam, allied));
				}
				else { // not allowed
				}
			}
			break;
		}
		case NETMSG_CCOMMAND: {
			try {
				CommandMessage msg(packet);

				if (static_cast<unsigned>(msg.player) == a) {
					if ((commandBlacklist.find(msg.action.command) != commandBlacklist.end()) && players[a].isLocal) {
						// command is restricted to server but player is allowed to execute it
						PushAction(msg.action);
					}
					else if (commandBlacklist.find(msg.action.command) == commandBlacklist.end()) {
						// command is save
						Broadcast(packet);
					}
					else {
						// hack!
						Message(str(boost::format(CommandNotAllowed) %msg.player %msg.action.command.c_str()));
					}
				}
			} catch (netcode::UnpackPacketException &e) {
				Message(str(boost::format("Player %s sent invalid CommandMessage: %s") %players[a].name %e.err));
			}
			break;
		}

		case NETMSG_TEAMSTAT: {
			if (hostif)
				hostif->Send(packet->data, packet->length);
			break;
		}

		case NETMSG_GAMEOVER: {
			try {
				// msgCode + msgSize + playerNum (all uchar's)
				const unsigned char fixedSize = 3 * sizeof(unsigned char);

				netcode::UnpackPacket pckt(packet, 1);

				unsigned char totalSize; pckt >> totalSize;
				unsigned char playerNum; pckt >> playerNum;

				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode %a %(unsigned)playerNum));
					break;
				}

				if (totalSize <= fixedSize) {
					throw netcode::UnpackPacketException("invalid size for NETMSG_GAMEOVER (no winning allyteams received)");
				}

				winningAllyTeams.resize(totalSize - fixedSize);
				pckt >> winningAllyTeams;

				if (hostif)
					hostif->SendGameOver(playerNum, winningAllyTeams);
				Broadcast(CBaseNetProtocol::Get().SendGameOver(playerNum, winningAllyTeams));

				gameEndTime = spring_gettime();
			} catch (netcode::UnpackPacketException& e) {
				Message(str(format("Player %s sent invalid GameOver: %s") %players[a].name %e.err));
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
		default: {
			Message(str(format(UnknownNetmsg) %msgCode %a));
		}
		break;
	}

}

void CGameServer::ServerReadNet()
{
	// handle new connections
	while (UDPNet && UDPNet->HasIncomingConnections()) {
		boost::shared_ptr<netcode::UDPConnection> prev = UDPNet->PreviewConnection().lock();
		boost::shared_ptr<const RawPacket> packet = prev->GetData();

		if (packet && packet->length >= 3 && packet->data[0] == NETMSG_ATTEMPTCONNECT) {
			try {
				netcode::UnpackPacket msg(packet, 3);
				std::string name, passwd, version;
				unsigned char reconnect;
				unsigned short netversion;
				msg >> netversion;
				if (netversion != NETWORK_VERSION)
					throw netcode::UnpackPacketException("Wrong network version");
				msg >> name;
				msg >> passwd;
				msg >> version;
				msg >> reconnect;
				BindConnection(name, passwd, version, false, UDPNet->AcceptConnection(), reconnect);
			} catch (netcode::UnpackPacketException &e) {
				Message(str(format(ConnectionReject) %e.err %packet->data[0] %packet->data[2] %packet->length));
				UDPNet->RejectConnection();
			}
		}
		else {
			if (packet && packet->length >= 3)
				Message(str(format(ConnectionReject) %"Invalid message ID" %packet->data[0] %packet->data[2] %packet->length));
			else
				Message("Connection attempt rejected: Packet too short");
			UDPNet->RejectConnection();
		}
	}

	float updateBandwidth = (float)(spring_gettime() - lastBandwidthUpdate) / (float)playerBandwidthInterval;
	if(updateBandwidth >= 1.0f)
		lastBandwidthUpdate = spring_gettime();

	for(size_t a=0; a < players.size(); a++) {
		if (!players[a].link)
			continue; // player not connected
		if (players[a].link->CheckTimeout(0, !gameHasStarted)) {
			Message(str(format(PlayerLeft) %players[a].GetType() %players[a].name %" timeout")); //this must happen BEFORE the reset!
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 0));
			players[a].Kill("User timeout");
			UpdateSpeedControl(speedControl);
			if (hostif)
				hostif->SendPlayerLeft(a, 0);
			continue;
		}

		bool bwLimitWasReached = (gc->linkIncomingPeakBandwidth > 0 && players[a].bandwidthUsage > gc->linkIncomingPeakBandwidth);

		if(updateBandwidth >= 1.0f && gc->linkIncomingSustainedBandwidth > 0)
			players[a].bandwidthUsage = std::max(0, players[a].bandwidthUsage - std::max(1, (int)((float)gc->linkIncomingSustainedBandwidth / (1000.0f / (playerBandwidthInterval * updateBandwidth)))));

		int numDropped = 0;
		boost::shared_ptr<const RawPacket> packet;

		bool dropPacket = gc->linkIncomingMaxWaitingPackets > 0 && (gc->linkIncomingPeakBandwidth <= 0 || bwLimitWasReached);
		int ahead = 0;
		bool bwLimitIsReached = gc->linkIncomingPeakBandwidth > 0 && players[a].bandwidthUsage > gc->linkIncomingPeakBandwidth;
		while(players[a].link) {
			if(dropPacket)
				dropPacket = (packet = players[a].link->Peek(gc->linkIncomingMaxWaitingPackets));
			packet = (!bwLimitIsReached || dropPacket) ? players[a].link->GetData() : players[a].link->Peek(ahead++);
			if(!packet)
				break;
			bool droppablePacket = (packet->length <= 0 || (packet->data[0] != NETMSG_SYNCRESPONSE && packet->data[0] != NETMSG_KEYFRAME));
			if(dropPacket && droppablePacket)
				++numDropped;
			else if(!bwLimitIsReached || !droppablePacket) {
				ProcessPacket(a, packet); // non droppable packets may be processed more than once, but this does no harm
				if(gc->linkIncomingPeakBandwidth > 0 && droppablePacket) {
					players[a].bandwidthUsage += std::max((unsigned)linkMinPacketSize, packet->length);
					if(!bwLimitIsReached)
						bwLimitIsReached = (players[a].bandwidthUsage > gc->linkIncomingPeakBandwidth);
				}
			}
		}
		if(numDropped > 0)
			PrivateMessage(a, str(format("Warning: Waiting packet limit was reached for %s [packets dropped]") %players[a].name));

		if(!bwLimitWasReached && bwLimitIsReached)
			PrivateMessage(a, str(format("Warning: Bandwidth limit was reached for %s [packets delayed]") %players[a].name));
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
	assert(!gameHasStarted);
	bool allReady = true;

	for (size_t a = static_cast<size_t>(setup->numDemoPlayers); a < players.size(); a++) {
		if (players[a].myState == GameParticipant::UNCONNECTED && serverStartTime + spring_secs(30) < spring_gettime()) {
			// autostart the game when 45 seconds have passed and everyone who managed to connect is ready
			continue;
		}
		else if (players[a].myState < GameParticipant::INGAME) {
			allReady = false;
			break;
		} else if (!players[a].spectator && teams[players[a].team].active && !players[a].readyToStart && !demoReader) {
			allReady = false;
			break;
		}
	}

	// msecs to wait until the game starts after all players are ready
	const spring_duration gameStartDelay = spring_secs(setup->gameStartDelay);

	if (allReady || forced) {
		if (!spring_istime(readyTime)) {
			readyTime = spring_gettime();
			rng.Seed(spring_tomsecs(readyTime-serverStartTime));
			// we have to wait at least 1 msec, because 0 is a special case
			const unsigned countdown = std::max(1, spring_tomsecs(gameStartDelay));
			Broadcast(CBaseNetProtocol::Get().SendStartPlaying(countdown));
		}
	}
	if (spring_istime(readyTime) && ((spring_gettime() - readyTime) > gameStartDelay)) {
		StartGame();
	}
}

void CGameServer::StartGame()
{
	assert(!gameHasStarted);
	gameHasStarted = true;
	startTime = gameTime;
	if (!canReconnect && !allowAdditionalPlayers)
		packetCache.clear(); // free memory

	if (UDPNet && !canReconnect && !allowAdditionalPlayers)
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
	for (size_t a = 0; a < players.size(); ++a) {
		if (!players[a].spectator) {
			const unsigned aTeam = players[a].team;
			Broadcast(CBaseNetProtocol::Get().SendStartPos(a, (int)aTeam, players[a].readyToStart, teams[aTeam].startPos.x, teams[aTeam].startPos.y, teams[aTeam].startPos.z));
			teamStartPosSent[aTeam] = true;
		}
	}

	// send start position for all other teams
	for (size_t a = 0; a < teams.size(); ++a) {
		if (!teamStartPosSent[a]) {
			// teams which aren't player controlled are always ready
			Broadcast(CBaseNetProtocol::Get().SendStartPos(SERVER_PLAYER, a, true, teams[a].startPos.x, teams[a].startPos.y, teams[a].startPos.z));
		}
	}

	Broadcast(CBaseNetProtocol::Get().SendRandSeed(rng()));
	Broadcast(CBaseNetProtocol::Get().SendStartPlaying(0));
	if (hostif)
		hostif->SendStartPlaying();
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
	if (action.command == "kickbynum") {
		if (!action.extra.empty()) {
			const int playerNum = atoi(action.extra.c_str());
			KickPlayer(playerNum);
		}
	}
	else if (action.command == "kick") {
		if (!action.extra.empty()) {
			std::string name = action.extra;
			StringToLowerInPlace(name);
			for (size_t a=0; a < players.size();++a) {
				std::string playerLower = StringToLower(players[a].name);
				if (playerLower.find(name)==0) {	// can kick on substrings of name
					if (!players[a].isLocal) // do not kick host
						KickPlayer(a);
				}
			}
		}
	}
	else if (action.command == "nopause") {
		SetBoolArg(gamePausable, action.extra);
	}
	else if (action.command == "nohelp") {
		SetBoolArg(noHelperAIs, action.extra);
		// sent it because clients have to do stuff when this changes
		CommandMessage msg(action, SERVER_PLAYER);
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
	}
	else if (action.command == "nospecdraw") {
		SetBoolArg(allowSpecDraw, action.extra);
		// sent it because clients have to do stuff when this changes
		CommandMessage msg(action, SERVER_PLAYER);
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
	}
	else if (action.command == "setmaxspeed" && !action.extra.empty()) {
		float newUserSpeed = std::max(static_cast<float>(atof(action.extra.c_str())), minUserSpeed);
		if (newUserSpeed > 0.2) {
			maxUserSpeed = newUserSpeed;
			UserSpeedChange(userSpeedFactor, SERVER_PLAYER);
		}
	}
	else if (action.command == "setminspeed" && !action.extra.empty()) {
		minUserSpeed = std::min(static_cast<float>(atof(action.extra.c_str())), maxUserSpeed);
		UserSpeedChange(userSpeedFactor, SERVER_PLAYER);
	}
	else if (action.command == "forcestart") {
		if (!gameHasStarted)
			CheckForGameStart(true);
	}
	else if (action.command == "skip") {
		if (demoReader) {
			std::string timeStr = action.extra;

			// parse the skip time

			// skip in seconds
			bool skipFrames = false;
			if (timeStr[0] == 'f') {
				// skip in frame
				skipFrames = true;
				timeStr.erase(0, 1); // remove first char
			}

			// skip to absolute game-second/-frame
			bool skipRelative = false;
			if (timeStr[0] == '+') {
				// skip to relative game-second/-frame
				skipRelative = true;
				timeStr.erase(0, 1); // remove first char
			}

			// amount of frames/seconds to skip (to)
			const int amount = atoi(timeStr.c_str());

			// the absolute frame to skip to
			int endFrame;

			if (skipFrames)
				endFrame = amount;
			else
				endFrame = GAME_SPEED * amount;

			if (skipRelative)
				endFrame += serverFrameNum;

			SkipTo(endFrame);
		}
	}
	else if (action.command == "cheat") {
		SetBoolArg(cheating, action.extra);
		CommandMessage msg(action, SERVER_PLAYER);
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
	}
	else if (action.command == "singlestep") {
		if (isPaused) {
			if (demoReader) {
				// we only want to advance one frame at most, so
				// the next time-index must be "close enough" not
				// to move past more than 1 NETMSG_NEWFRAME
				modGameTime = demoReader->GetModGameTime() + 0.001f;
			}

			CreateNewFrame(true, true);
		}
	}
	else if (action.command == "adduser") {
		if (!action.extra.empty() && whiteListAdditionalPlayers) {
			// split string by whitespaces
			const std::vector<std::string> &tokens = CSimpleParser::Tokenize(action.extra);

			if (tokens.size() > 1) {
				const std::string& name = tokens[0];
				const std::string& password = tokens[1];

				GameParticipant gp;
					gp.name = name;
				// note: this must only compare by name
				std::vector<GameParticipant>::iterator participantIter = std::find(players.begin(), players.end(), gp);

				if (participantIter != players.end()) {
					participantIter->SetValue("password", password);
					logOutput.Print("Changed player/spectator password: \"%s\" \"%s\"", name.c_str(), password.c_str());
				} else {
					AddAdditionalUser(name, password);
					logOutput.Print("Added player/spectator password: \"%s\" \"%s\"", name.c_str(), password.c_str());
				}
			} else {
				logOutput.Print("Failed to add player/spectator password. usage: /adduser <player-name> <password>");
			}
		}
	}
	else if (action.command == "kill") {
		quitServer = true;
	}
	else if (action.command == "pause") {
		bool newPausedState = isPaused;
		if (action.extra.empty()) {
			// if no param is given, toggle paused state
			newPausedState = !isPaused;
		} else {
			// if a param is given, interpret it as "bool setPaused"
			SetBoolArg(newPausedState, action.extra);
		}
		if (newPausedState != isPaused) {
			// the state changed
			isPaused = newPausedState;
		}
	}
	else {
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

void CGameServer::CreateNewFrame(bool fromServerThread, bool fixedFrameTime)
{
	if (!demoReader) {
		// use NEWFRAME_MSGes from demo otherwise
#if BOOST_VERSION >= 103500
		boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex, boost::defer_lock);
		if (!fromServerThread)
			scoped_lock.lock();
#else
		boost::recursive_mutex::scoped_lock scoped_lock(gameServerMutex, !fromServerThread);
#endif

		CheckSync();
		int newFrames = 1;

		if (!fixedFrameTime) {
			spring_time currentTick = spring_gettime();
			spring_duration timeElapsed = currentTick - lastTick;

			if (timeElapsed > spring_msecs(200))
				timeElapsed = spring_msecs(200);

			timeLeft += GAME_SPEED * internalSpeed * float(spring_tomsecs(timeElapsed)) * 0.001f;
			lastTick=currentTick;
			newFrames = (timeLeft > 0)? int(ceil(timeLeft)): 0;
			timeLeft -= newFrames;

			if (hasLocalClient) {
				// needs to set lastTick and stuff, otherwise we will get all the left out NEWFRAME's at once when client has catched up
				if (players[localClientNumber].lastFrameResponse + GAME_SPEED*2 <= serverFrameNum)
					return;
			}
		}

		const bool rec = videoCapturing->IsCapturing();
		const bool normalFrame = !isPaused && !rec;
		const bool videoFrame = !isPaused && fixedFrameTime;
		const bool singleStep = fixedFrameTime && !rec;

		if (normalFrame || videoFrame || singleStep) {
			for (int i=0; i < newFrames; ++i) {
				assert(!demoReader);
				++serverFrameNum;
				// Send out new frame messages.
				if ((serverFrameNum % serverKeyframeIntervall) == 0)
					Broadcast(CBaseNetProtocol::Get().SendKeyFrame(serverFrameNum));
				else
					Broadcast(CBaseNetProtocol::Get().SendNewFrame());
#ifdef SYNCCHECK
				outstandingSyncFrames.insert(serverFrameNum);
#endif
			}
		}
	} else {
		CheckSync();
		SendDemoData();
	}
}

void CGameServer::UpdateSpeedControl(int speedCtrl) {
	if (speedControl != speedCtrl) {
		speedControl = speedCtrl;
		curSpeedCtrl = (speedControl == 1 || speedControl == -1) ? 1 : 0;
	}
	if (speedControl >= 0) {
		int oldSpeedCtrl = curSpeedCtrl;
		int avgvotes = 0;
		int maxvotes = 0;
		for (size_t i = 0; i < players.size(); ++i) {
			if (players[i].link) {
				int sc = players[i].speedControl;
				if (sc == 1 || sc == -1)
					++avgvotes;
				else if (sc == 2 || sc == -2)
					++maxvotes;
			}
		}

		if (avgvotes > maxvotes)
			curSpeedCtrl = 1;
		else if (avgvotes < maxvotes)
			curSpeedCtrl = 0;
		else
			curSpeedCtrl = (speedControl == 1) ? 1 : 0;

		if (curSpeedCtrl != oldSpeedCtrl)
			Message(str(format("Server speed control: %s CPU [%d/%d]") %(curSpeedCtrl ? "Average" : "Maximum") %(curSpeedCtrl ? avgvotes : maxvotes) %(avgvotes + maxvotes)));
	}
}

void CGameServer::UpdateLoop()
{
	while (!quitServer) {
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

void CGameServer::KickPlayer(const int playerNum)
{
	if (players[playerNum].link) { // only kick connected players
		Message(str(format(PlayerLeft) %players[playerNum].GetType() %players[playerNum].name %"kicked"));
		Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(playerNum, 2));
		players[playerNum].Kill("Kicked from the battle");
		UpdateSpeedControl(speedControl);
		if (hostif)
			hostif->SendPlayerLeft(playerNum, 2);
	}
	else
		Message(str(format("Attempt to kick player %d who is not connected") %playerNum));

}


void CGameServer::AddAdditionalUser(const std::string& name, const std::string& passwd, bool fromDemo)
{
	GameParticipant buf;
	buf.isFromDemo = fromDemo;
	buf.name = name;
	buf.spectator = true;
	buf.team = 0;
	buf.isMidgameJoin = true;
	if (passwd.size() > 0)
		buf.SetValue("password",passwd);
	players.push_back(buf);
	UpdatePlayerNumberMap();
	if (!fromDemo)
		Broadcast(CBaseNetProtocol::Get().SendCreateNewPlayer(players.size() -1, buf.spectator, buf.team, buf.name)); // inform all the players of the newcomer
}


unsigned CGameServer::BindConnection(std::string name, const std::string& passwd, const std::string& version, bool isLocal, boost::shared_ptr<netcode::CConnection> link, bool reconnect)
{
	Message(str(format("%s attempt from %s") %(reconnect ? "Reconnection" : "Connection") %name));
	Message(str(format(" -> Version: %s") %version));
	Message(str(format(" -> Address: %s") %link->GetFullAddress()), false);
	size_t newPlayerNumber = players.size();

	if (link->CanReconnect())
		canReconnect = true;

	std::string errmsg = "";
	bool terminate = false;

	for (size_t i = 0; i < players.size(); ++i) {
		if (name == players[i].name) {
			if (!players[i].isFromDemo) {
				if (!players[i].link) {
					if (canReconnect || !gameHasStarted)
						newPlayerNumber = i;
					else
						errmsg = "Game has already started";
					break;
				}
				else {
					bool reconnectAllowed = canReconnect && gameHasStarted && players[i].link->CheckTimeout(-1);
					if (!reconnect && reconnectAllowed) {
						newPlayerNumber = i;
						terminate = true;
					}
					else if (reconnect && reconnectAllowed && players[i].link->GetFullAddress() != link->GetFullAddress())
						newPlayerNumber = i;
					else
						errmsg = "User is already ingame";
					break;
				}
			}
			else {
				errmsg = "User name duplicated in the demo";
			}
		}
	}

	if (newPlayerNumber >= players.size() && errmsg == "") {
		if (demoReader || allowAdditionalPlayers)
			AddAdditionalUser(name, passwd);
		else
			errmsg = "User name not authorized to connect";
	}

	// check for user's password
	if (errmsg == "" && !isLocal) {
		if (newPlayerNumber < players.size()) {
			GameParticipant::customOpts::const_iterator it = players[newPlayerNumber].GetAllValues().find("password");
			bool passwdFound = (it != players[newPlayerNumber].GetAllValues().end());
			if (passwdFound) {
				if (passwd != it->second)
					errmsg = "Incorrect password";
			}
		}
	}

	if (newPlayerNumber >= players.size() || errmsg != "") {
		Message(str(format(" -> %s") %errmsg));
		link->SendData(CBaseNetProtocol::Get().SendQuit(str(format("Connection rejected: %s") %errmsg)));
		return 0;
	}

	GameParticipant& newPlayer = players[newPlayerNumber];

	if (terminate) {
		Message(str(format(PlayerLeft) %newPlayer.GetType() %newPlayer.name %" terminating existing connection"));
		Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(newPlayerNumber, 0));
		newPlayer.link.reset(); // prevent sending a quit message since this might kill the new connection
		newPlayer.Kill("Terminating connection");
		UpdateSpeedControl(speedControl);
		if (hostif)
			hostif->SendPlayerLeft(newPlayerNumber, 0);
	}

	if (newPlayer.isMidgameJoin) {
		link->SendData(CBaseNetProtocol::Get().SendCreateNewPlayer(newPlayerNumber, newPlayer.spectator, newPlayer.team, newPlayer.name)); // inform the player about himself if it's a midgame join
	}

	newPlayer.isReconn = gameHasStarted;

	if (newPlayer.link) {
		newPlayer.link->ReconnectTo(*link);
		Message(str(format(" -> Connection reestablished (id %i)") %newPlayerNumber));
		link->Flush(!gameHasStarted);
		return newPlayerNumber;
	}

	newPlayer.Connected(link, isLocal);
	newPlayer.SendData(boost::shared_ptr<const RawPacket>(gameData->Pack()));
	newPlayer.SendData(CBaseNetProtocol::Get().SendSetPlayerNum((unsigned char)newPlayerNumber));

	// after gamedata and playerNum, the player can start loading
	for (std::list< std::vector<boost::shared_ptr<const netcode::RawPacket> > >::const_iterator lit = packetCache.begin(); lit != packetCache.end(); ++lit)
		for (std::vector<boost::shared_ptr<const netcode::RawPacket> >::const_iterator vit = lit->begin(); vit != lit->end(); ++vit)
			newPlayer.SendData(*vit); // throw at him all stuff he missed until now

	if (!demoReader || setup->demoName.empty()) { // gamesetup from demo?
		if (!newPlayer.spectator) {
			unsigned newPlayerTeam = newPlayer.team;
			if (!teams[newPlayerTeam].active) { // create new team
				newPlayer.readyToStart = (setup->startPosType != CGameSetup::StartPos_ChooseInGame);
				teams[newPlayerTeam].active = true;
			}
			Broadcast(CBaseNetProtocol::Get().SendJoinTeam(newPlayerNumber, newPlayerTeam));
		}
	}

	Message(str(format(" -> Connection established (given id %i)") %newPlayerNumber));

	link->Flush(!gameHasStarted);
	return newPlayerNumber;
}

void CGameServer::GotChatMessage(const ChatMessage& msg)
{
	if (!msg.msg.empty()) { // silently drop empty chat messages
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
	if (curSpeedCtrl > 0 &&
		player >= 0 && static_cast<unsigned int>(player) != SERVER_PLAYER &&
		!players[player].isLocal && !isPaused &&
		(players[player].spectator || (curSpeedCtrl > 0 &&
		(players[player].cpuUsage - medianCpu > std::min(0.2f, std::max(0.0f, 0.8f - medianCpu)) ||
		(serverFrameNum - players[player].lastFrameResponse) - medianPing > internalSpeed * GAME_SPEED / 2)))) {
		PrivateMessage(player, "Speed change rejected (cpu load or ping is too high)");
		return; // disallow speed change by players who cannot keep up gamespeed
	}

	newSpeed = std::min(maxUserSpeed, std::max(newSpeed, minUserSpeed));

	if (userSpeedFactor != newSpeed) {
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
		if (*it != skirmishAIId)
			break;
	}

	usedSkirmishAIIds.insert(it, skirmishAIId);

	return skirmishAIId;
}

void CGameServer::FreeSkirmishAIId(const size_t skirmishAIId) {
	usedSkirmishAIIds.remove(skirmishAIId);
}

void CGameServer::AddToPacketCache(boost::shared_ptr<const netcode::RawPacket> &pckt) {
	if (packetCache.empty() || packetCache.back().size() >= PKTCACHE_VECSIZE) {
		packetCache.push_back(std::vector<boost::shared_ptr<const netcode::RawPacket> >());
		packetCache.back().reserve(PKTCACHE_VECSIZE);
	}
	packetCache.back().push_back(pckt);
}

