/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Net/UDPListener.h"
#include "System/Net/UDPConnection.h"

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/version.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <deque>
#if defined DEDICATED || defined DEBUG
	#include <iostream>
#endif

#include "GameServer.h"

#include "GameParticipant.h"
#include "GameSkirmishAI.h"
#include "AutohostInterface.h"

#include "Game/ClientSetup.h"
#include "Game/GameSetup.h"

#include "Game/Action.h"
#include "Game/ChatMessage.h"
#include "Game/CommandMessage.h"
#include "Game/GlobalUnsynced.h" // for syncdebug
#include "Game/IVideoCapturing.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "Sim/Misc/GlobalConstants.h"

// This undef is needed, as somewhere there is a type interface specified,
// which we need not!
// (would cause problems in ExternalAI/Interface/SAIInterfaceLibrary.h)
#ifdef interface
	#undef interface
#endif
#include "System/CRC.h"
#include "System/GlobalConfig.h"
#include "System/MsgStrings.h"
#include "System/myMath.h"
#include "System/Util.h"
#include "System/TdfParser.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Net/Connection.h"
#include "System/Net/LocalConnection.h"
#include "System/Net/UnpackPacket.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/LoadSave/DemoReader.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"

#ifndef DEDICATED
#include "lib/luasocket/src/restrictions.h"
#endif

#define ALLOW_DEMO_GODMODE
#define PKTCACHE_VECSIZE 1000

using netcode::RawPacket;
using boost::format;


CONFIG(int, AutohostPort).defaultValue(0);
CONFIG(int, ServerSleepTime).defaultValue(5).description("number of milliseconds to sleep per tick");
CONFIG(int, SpeedControl).defaultValue(1).minimumValue(1).maximumValue(2)
	.description("Sets how server adjusts speed according to player's load (CPU), 1: use average, 2: use highest");
CONFIG(bool, AllowSpectatorJoin).defaultValue(true).description("allow any unauthenticated clients to join as spectator with any name, name will be prefixed with ~");
CONFIG(bool, WhiteListAdditionalPlayers).defaultValue(true);
CONFIG(bool, ServerRecordDemos).defaultValue(false).dedicatedValue(true);
CONFIG(bool, ServerLogInfoMessages).defaultValue(false);
CONFIG(bool, ServerLogDebugMessages).defaultValue(false);
CONFIG(std::string, AutohostIP).defaultValue("127.0.0.1");


// use the specific section for all LOG*() calls in this source file
#define LOG_SECTION_GAMESERVER "GameServer"
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_GAMESERVER

LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_GAMESERVER)



/// frames until a synccheck will time out and a warning is given out
static const unsigned SYNCCHECK_TIMEOUT = 300;

/// used to prevent msg spam
static const unsigned SYNCCHECK_MSG_TIMEOUT = 400;

/// The time interval in msec for sending player statistics to each client
static const spring_time playerInfoTime = spring_secs(2);

/// every n'th frame will be a keyframe (and contain the server's framenumber)
static const unsigned serverKeyframeInterval = 16;

/// players incoming bandwidth new allowance every X milliseconds
static const unsigned playerBandwidthInterval = 100;

/// every 10 sec we'll broadcast current frame in a message that skips queue & cache
/// to let clients that are fast-forwarding to current point to know their loading %
static const unsigned gameProgressFrameInterval = GAME_SPEED * 10;

static const unsigned syncResponseEchoInterval = GAME_SPEED * 2;


//FIXME remodularize server commands, so they get registered in word completion etc.
static const std::string SERVER_COMMANDS[] = {
	"kick", "kickbynum",
	"mute", "mutebynum",
	"setminspeed", "setmaxspeed",
	"nopause", "nohelp", "cheat", "godmode", "globallos",
	"nocost", "forcestart", "nospectatorchat", "nospecdraw",
	"skip", "reloadcob", "reloadcegs", "devlua", "editdefs",
	"singlestep", "spec", "specbynum"
};


std::set<std::string> CGameServer::commandBlacklist;



CGameServer* gameServer = NULL;

CGameServer::CGameServer(
	const boost::shared_ptr<const ClientSetup> newClientSetup,
	const boost::shared_ptr<const    GameData> newGameData,
	const boost::shared_ptr<const  CGameSetup> newGameSetup
)
: quitServer(false)
, serverFrameNum(-1)

, serverStartTime(spring_gettime())
, readyTime(spring_notime)
, gameEndTime(spring_notime)
, lastPlayerInfo(serverStartTime)
, lastUpdate(serverStartTime)

, modGameTime(0.0f)
, gameTime(0.0f)
, startTime(0.0f)
, frameTimeLeft(0.0f)

, isPaused(false)
, gamePausable(true)

, userSpeedFactor(1.0f)
, internalSpeed(1.0f)

, medianCpu(0.0f)
, medianPing(0)

, cheating(false)
, noHelperAIs(false)
, canReconnect(false)
, allowSpecDraw(true)

, syncErrorFrame(0)
, syncWarningFrame(0)

, localClientNumber(-1u)

, gameHasStarted(false)
, generatedGameID(false)
, reloadingServer(false)
{
	myClientSetup = newClientSetup;
	myGameData = newGameData;
	myGameSetup = newGameSetup;

	Initialize();
}

CGameServer::~CGameServer()
{
	quitServer = true;

	LOG_L(L_INFO, "[%s][1]", __FUNCTION__);
	thread->join();
	delete thread;
	LOG_L(L_INFO, "[%s][2]", __FUNCTION__);

	// after this, demoRecorder goes out of scope and its dtor is called
	WriteDemoData();
}


void CGameServer::Initialize()
{
	// configs
	curSpeedCtrl = configHandler->GetInt("SpeedControl");
	allowSpecJoin = configHandler->GetBool("AllowSpectatorJoin");
	whiteListAdditionalPlayers = configHandler->GetBool("WhiteListAdditionalPlayers");
	logInfoMessages = configHandler->GetBool("ServerLogInfoMessages");
	logDebugMessages = configHandler->GetBool("ServerLogDebugMessages");

	rng.Seed((myGameData->GetSetupText()).length());

	// start network
	if (!myGameSetup->onlyLocal) {
		UDPNet.reset(new netcode::UDPListener(myClientSetup->hostPort, myClientSetup->hostIP));
	}
	AddAutohostInterface(StringToLower(configHandler->GetString("AutohostIP")), configHandler->GetInt("AutohostPort"));
	Message(str(format(ServerStart) %myClientSetup->hostPort), false);

	// start script
	maxUserSpeed = myGameSetup->maxSpeed;
	minUserSpeed = myGameSetup->minSpeed;
	noHelperAIs  = myGameSetup->noHelperAIs;
	StripGameSetupText(myGameData.get());

	// load demo (if there is one)
	if (myGameSetup->hostDemo) {
		Message(str(format(PlayingDemo) %myGameSetup->demoName));
		demoReader.reset(new CDemoReader(myGameSetup->demoName, modGameTime + 0.1f));
	}

	// initialize players, teams & ais
	{
		const std::vector<PlayerBase>& playerStartData = myGameSetup->GetPlayerStartingDataCont();
		const std::vector<TeamBase>&     teamStartData = myGameSetup->GetTeamStartingDataCont();
		const std::vector<SkirmishAIData>& aiStartData = myGameSetup->GetAIStartingDataCont();

		players.reserve(MAX_PLAYERS); // no reallocation please
		teams.resize(teamStartData.size());

		players.resize(playerStartData.size());
		if (demoReader != NULL) {
			const size_t demoPlayers = demoReader->GetFileHeader().numPlayers;
			players.resize(std::max(demoPlayers, playerStartData.size()));
			if (players.size() >= MAX_PLAYERS) {
				Message(str(format("Too many Players (%d) in the demo") %players.size()));
			}
		}

		std::copy(playerStartData.begin(), playerStartData.end(), players.begin());
		std::copy(teamStartData.begin(), teamStartData.end(), teams.begin());

		//FIXME move playerID to PlayerBase, so it gets copied from playerStartData
		int playerID = 0;
		for (GameParticipant& p: players) {
			p.id = playerID++;
		}

		for (const SkirmishAIData& skd: aiStartData) {
			const unsigned char skirmishAIId = ReserveNextAvailableSkirmishAIId();
			if (skirmishAIId == MAX_AIS) {
				Message(str(format("Too many AIs (%d) in game setup") %aiStartData.size()));
				break;
			}
			players[skd.hostPlayer].linkData[skirmishAIId] = GameParticipant::PlayerLinkData();
			ais[skirmishAIId] = skd;

			teams[skd.team].SetActive(true);
			if (!teams[skd.team].HasLeader()) {
				teams[skd.team].SetLeader(skd.hostPlayer);
			}
		}
	}

	for (unsigned int n = 0; n < (sizeof(SERVER_COMMANDS) / sizeof(std::string)); n++) {
		commandBlacklist.insert(SERVER_COMMANDS[n]);
	}

	if (configHandler->GetBool("ServerRecordDemos")) {
		demoRecorder.reset(new CDemoRecorder(myGameSetup->mapName, myGameSetup->modName, true));
		demoRecorder->WriteSetupText(myGameData->GetSetupText());
		const netcode::RawPacket* ret = myGameData->Pack();
		demoRecorder->SaveToDemo(ret->data, ret->length, GetDemoTime());
		delete ret;
	}

	loopSleepTime = configHandler->GetInt("ServerSleepTime");
	lastNewFrameTick = spring_gettime();
	linkMinPacketSize = globalConfig->linkIncomingMaxPacketRate > 0 ? (globalConfig->linkIncomingSustainedBandwidth / globalConfig->linkIncomingMaxPacketRate) : 1;
	lastBandwidthUpdate = spring_gettime();

	thread = new boost::thread(boost::bind<void, CGameServer, CGameServer*>(&CGameServer::UpdateLoop, this));

#ifdef STREFLOP_H
	// Something in CGameServer::CGameServer borks the FPU control word
	// maybe the threading, or something in CNet::InitServer() ??
	// Set single precision floating point math.
	streflop::streflop_init<streflop::Simple>();
#endif

	if (!demoReader) {
		GenerateAndSendGameID();
		rng.Seed(gameID.intArray[0] ^ gameID.intArray[1] ^ gameID.intArray[2] ^ gameID.intArray[3]);
		Broadcast(CBaseNetProtocol::Get().SendRandSeed(rng()));
	}
}

void CGameServer::PostLoad(int newServerFrameNum)
{
	Threading::RecursiveScopedLock scoped_lock(gameServerMutex);
	serverFrameNum = newServerFrameNum;

	gameHasStarted = !PreSimFrame();

	// for all GameParticipant's
	for (GameParticipant& p: players) {
		p.lastFrameResponse = newServerFrameNum;
	}
}


void CGameServer::Reload(const boost::shared_ptr<const CGameSetup> newGameSetup)
{
	const boost::shared_ptr<const ClientSetup> clientSetup = gameServer->GetClientSetup();
	const boost::shared_ptr<const    GameData>    gameData = gameServer->GetGameData();

	delete gameServer;

	// transfer ownership to new instance (assume only GameSetup changes)
	gameServer = new CGameServer(clientSetup, gameData, newGameSetup);
}


void CGameServer::WriteDemoData()
{
	if (demoRecorder == NULL)
		return;

	// there is always at least one non-Gaia team (numTeams > 0)
	// the Gaia team itself does not count toward the statistics
	demoRecorder->SetTime(serverFrameNum / GAME_SPEED, spring_tomsecs(spring_gettime() - serverStartTime) / 1000);
	demoRecorder->InitializeStats(players.size(), int((myGameSetup->GetTeamStartingDataCont()).size()) - myGameSetup->useLuaGaia);

	// Pass the winners to the CDemoRecorder.
	demoRecorder->SetWinningAllyTeams(winningAllyTeams);

	for (GameParticipant& p: players) {
		demoRecorder->SetPlayerStats(p.id, p.lastStats);
	}

	/*
	// TODO?
	for (size_t i = 0; i < ais.size(); ++i) {
		demoRecorder->SetSkirmishAIStats(i, ais[i].lastStats);
	}
	for (int i = 0; i < numTeams; ++i) {
		record->SetTeamStats(i, teamHandler->Team(i)->statHistory);
	}
	*/
}

void CGameServer::StripGameSetupText(const GameData* newGameData)
{
	// modify and save GameSetup text (remove passwords)
	TdfParser parser((newGameData->GetSetupText()).c_str(), (newGameData->GetSetupText()).length());
	TdfParser::TdfSection* rootSec = parser.GetRootSection()->sections["game"];

	for (TdfParser::sectionsMap_t::iterator it = rootSec->sections.begin(); it != rootSec->sections.end(); ++it) {
		const std::string& sectionKey = StringToLower(it->first);

		if (!StringStartsWith(sectionKey, "player"))
			continue;

		TdfParser::TdfSection* playerSec = it->second;
		playerSec->remove("password", false);
	}

	std::ostringstream strbuf;
	parser.print(strbuf);

	GameData* modGameData = new GameData(*newGameData);

	modGameData->SetSetupText(strbuf.str());
	myGameData.reset(modGameData);
}


void CGameServer::AddLocalClient(const std::string& myName, const std::string& myVersion)
{
	Threading::RecursiveScopedLock scoped_lock(gameServerMutex);
	assert(!HasLocalClient());

	localClientNumber = BindConnection(myName, "", myVersion, true, boost::shared_ptr<netcode::CConnection>(new netcode::CLocalConnection()));
}

void CGameServer::AddAutohostInterface(const std::string& autohostIP, const int autohostPort)
{
	if (autohostPort <= 0)
		return;

	if (autohostIP == "localhost") {
		LOG_L(L_ERROR, "Autohost IP address not in x.y.z.w format!");
		return;
	}

#ifndef DEDICATED
	// disallow luasockets access to autohost interface
	luaSocketRestrictions->addRule(CLuaSocketRestrictions::UDP_CONNECT, autohostIP.c_str(), autohostPort, false);
#endif

	if (!hostif) {
		hostif.reset(new AutohostInterface(autohostIP, autohostPort));
		if (hostif->IsInitialized()) {
			hostif->SendStart();
			Message(str(format(ConnectAutohost) %autohostPort), false);
		} else {
			// Quit if we are instructed to communicate with an auto-host,
			// but are unable to do so: we do not want an auto-host running
			// a spring game that it has no control over. If we get here,
			// it suggests a configuration problem in the auto-host.
			hostif.reset();
			Message(str(format(ConnectAutohostFailed) %autohostIP %autohostPort), false);
			quitServer = true;
		}
	}
}


void CGameServer::SkipTo(int targetFrameNum)
{
	const bool wasPaused = isPaused;

	if (!gameHasStarted) { return; }
	if (serverFrameNum >= targetFrameNum) { return; }
	if (demoReader == NULL) { return; }

	CommandMessage startMsg(str(format("skip start %d") %targetFrameNum), SERVER_PLAYER);
	CommandMessage endMsg("skip end", SERVER_PLAYER);
	Broadcast(boost::shared_ptr<const netcode::RawPacket>(startMsg.Pack()));

	// fast-read and send demo data
	//
	// note that we must maintain <modGameTime> ourselves
	// since we do we NOT go through ::Update when skipping
	while (SendDemoData(targetFrameNum)) {
		gameTime = GetDemoTime();
		modGameTime = demoReader->GetModGameTime() + 0.001f;

		if (UDPNet == NULL) { continue; }
		if ((serverFrameNum % 20) != 0) { continue; }

		// send data every few frames, as otherwise packets would grow too big
		UDPNet->Update();
	}

	Broadcast(boost::shared_ptr<const netcode::RawPacket>(endMsg.Pack()));

	if (UDPNet) {
		UDPNet->Update();
	}

	lastUpdate = spring_gettime();
	isPaused = wasPaused;
}

std::string CGameServer::GetPlayerNames(const std::vector<int>& indices) const
{
	std::string playerstring;
	for (int id: indices) {
		if (!playerstring.empty())
			playerstring += ", ";
		playerstring += players[id].name;
	}
	return playerstring;
}


bool CGameServer::SendDemoData(int targetFrameNum)
{
	bool ret = false;
	netcode::RawPacket* buf = NULL;

	// if we reached EOS before, demoReader has become NULL
	if (demoReader == NULL)
		return ret;

	// get all packets from the stream up to <modGameTime>
	while ((buf = demoReader->GetData(modGameTime))) {
		boost::shared_ptr<const RawPacket> rpkt(buf);

		if (buf->length <= 0) {
			Message(str(format("Warning: Discarding zero size packet in demo")));
			continue;
		}

		const unsigned msgCode = buf->data[0];

		switch (msgCode) {
			case NETMSG_NEWFRAME:
			case NETMSG_KEYFRAME: {
				// we can't use CreateNewFrame() here
				lastNewFrameTick = spring_gettime();
				serverFrameNum++;
#ifdef SYNCCHECK
				if (targetFrameNum == -1) {
					// not skipping
					outstandingSyncFrames.insert(serverFrameNum);
				}
				CheckSync();
#endif
				Broadcast(rpkt);
				break;
			}

			case NETMSG_CREATE_NEWPLAYER: {
				try {
					netcode::UnpackPacket pckt(rpkt, 3);
					unsigned char spectator, team, playerNum;
					std::string name;
					pckt >> playerNum;
					pckt >> spectator;
					pckt >> team;
					pckt >> name;
					AddAdditionalUser(name, "", true, (bool)spectator, (int)team, playerNum); // even though this is a demo, keep the players vector properly updated
				} catch (const netcode::UnpackPacketException& ex) {
					Message(str(format("Warning: Discarding invalid new player packet in demo: %s") %ex.what()));
					continue;
				}

				Broadcast(rpkt);
				break;
			}

			case NETMSG_GAMEDATA:
			case NETMSG_SETPLAYERNUM:
			case NETMSG_USER_SPEED:
			case NETMSG_INTERNAL_SPEED: {
				// never send these from demos
				break;
			}
			case NETMSG_CCOMMAND: {
				try {
					CommandMessage msg(rpkt);
					const Action& action = msg.GetAction();
					if (msg.GetPlayerID() == SERVER_PLAYER && action.command == "cheat")
						InverseOrSetBool(cheating, action.extra);
				} catch (const netcode::UnpackPacketException& ex) {
					Message(str(format("Warning: Discarding invalid command message packet in demo: %s") %ex.what()));
					continue;
				}
				Broadcast(rpkt);
				break;
			}
			default: {
				Broadcast(rpkt);
				break;
			}
		}
	}

	if (targetFrameNum > 0) {
		// skipping
		ret = (serverFrameNum < targetFrameNum);
	}

	if (demoReader->ReachedEnd()) {
		demoReader.reset();
		Message(DemoEnd);
		gameEndTime = spring_gettime();
		ret = false;
	}

	return ret;
}

void CGameServer::Broadcast(boost::shared_ptr<const netcode::RawPacket> packet)
{
	for (GameParticipant& p: players) {
		p.SendData(packet);
	}

	if (canReconnect || allowSpecJoin || !gameHasStarted)
		AddToPacketCache(packet);

	if (demoRecorder != NULL)
		demoRecorder->SaveToDemo(packet->data, packet->length, GetDemoTime());
}

void CGameServer::Message(const std::string& message, bool broadcast)
{
	if (broadcast) {
		Broadcast(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
	}
	else if (HasLocalClient()) {
		// host should see
		players[localClientNumber].SendData(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
	}
	if (hostif)
		hostif->Message(message);

#ifdef DEDICATED
	LOG("%s", message.c_str());
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
		if (HasLocalClient()) {
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
			for (GameParticipant& p: players) {
				if (!p.link)
					continue;

				std::map<int, unsigned>::const_iterator it = p.syncResponse.find(*f);
				if (it != p.syncResponse.end()) {
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
		for (GameParticipant& p: players) {
			if (!p.link) {
				continue;
			}
			std::map<int, unsigned>::iterator it = p.syncResponse.find(*f);
			if (it == p.syncResponse.end()) {
				if (*f >= serverFrameNum - static_cast<int>(SYNCCHECK_TIMEOUT))
					bComplete = false;
				else if (*f < p.lastFrameResponse)
					noSyncResponse.push_back(p.id);
			} else {
				if (bGotCorrectChecksum && it->second != correctChecksum) {
					p.desynced = true;
					if (demoReader || !p.spectator)
						desyncGroups[it->second].push_back(p.id);
					else
						desyncSpecs[p.id] = it->second;
				}
				else
					p.desynced = false;
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
					Message(str(format(SyncError) %playernames %(*f) %g->first %correctChecksum));
				}

				// send spectator desyncs as private messages to reduce spam
				for (std::map<int, unsigned>::const_iterator s = desyncSpecs.begin(); s != desyncSpecs.end(); ++s) {
					int playerNum = s->first;

					LOG_L(L_ERROR, "%s", str(format(SyncError) %players[playerNum].name %(*f) %s->second %correctChecksum).c_str());
					Message(str(format(SyncError) %players[playerNum].name %(*f) %s->second %correctChecksum));

					PrivateMessage(playerNum, str(format(SyncError) %players[playerNum].name %(*f) %s->second %correctChecksum));
				}
			}
			SetExitCode(-1);
		}

		// Remove complete sets (for which all player's checksums have been received).
		if (bComplete) {
			// Message(str (format("Succesfully purged outstanding sync frame %d from the deque") %(*f)));
			for (GameParticipant& p: players) {
				if (p.myState < GameParticipant::DISCONNECTED)
					p.syncResponse.erase(*f);
			}
			f = outstandingSyncFrames.erase(f);
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
	if (!gameHasStarted) return gameTime;
	return (startTime + serverFrameNum / float(GAME_SPEED));
}


void CGameServer::Update()
{
	const float tdif = spring_tomsecs(spring_gettime() - lastUpdate) * 0.001f;

	gameTime += tdif;
	lastUpdate = spring_gettime();

	if (!isPaused && gameHasStarted) {
		// if we are not playing a demo, or have no local client, or the
		// local client is less than <GAME_SPEED> frames behind, advance
		// <modGameTime>
		if (demoReader == NULL || !HasLocalClient() || (serverFrameNum - players[localClientNumber].lastFrameResponse) < GAME_SPEED)
			modGameTime += (tdif * internalSpeed);
	}

	if (lastPlayerInfo < (spring_gettime() - playerInfoTime)) {
		lastPlayerInfo = spring_gettime();

		if (!PreSimFrame()) {
			LagProtection();
		} else {
			for (GameParticipant& p: players) {
				if (p.isFromDemo)
					continue;

				switch (p.myState) {
					case GameParticipant::CONNECTED: {
						// send pathing status
						if (p.cpuUsage > 0)
							Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(p.id, p.cpuUsage, PATHING_FLAG));
					} break;
					case GameParticipant::INGAME:
					case GameParticipant::DISCONNECTED: {
						Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(p.id, 0, 0)); // reset status
					} break;
					case GameParticipant::UNCONNECTED:
					default: break;
				}
			}
		}
	}

	if (!gameHasStarted)
		CheckForGameStart();
	else if (!PreSimFrame() || demoReader != NULL)
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
				PushAction(Action(msg.substr(1)), true);
			}
		}
	}

	const bool pregameTimeoutReached = (spring_gettime() > serverStartTime + spring_secs(globalConfig->initialNetworkTimeout));
	if (pregameTimeoutReached || gameHasStarted) {
		bool hasPlayers = false;
		for (GameParticipant& p: players) {
			if (p.link) {
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



void CGameServer::LagProtection()
{
	std::vector<float> cpu;
	std::vector<int> ping;
	cpu.reserve(players.size());
	ping.reserve(players.size());

	// detect reference cpu usage ( highest )
	float refCpuUsage = 0.0f;
	for (GameParticipant& player: players) {
		if (player.myState == GameParticipant::INGAME) {
			// send info about the players
			const int curPing = ((serverFrameNum - player.lastFrameResponse) * 1000) / (GAME_SPEED * internalSpeed);
			Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(player.id, player.cpuUsage, curPing));

			const float playerCpuUsage = player.cpuUsage;
			const float correctedCpu   = Clamp(playerCpuUsage, 0.0f, 1.0f);

			if (player.isReconn && curPing < 2 * GAME_SPEED)
				player.isReconn = false;

			if ((player.isLocal) || (demoReader ? !player.isFromDemo : !player.spectator)) {
				if (!player.isReconn && correctedCpu > refCpuUsage)
					refCpuUsage = correctedCpu;
				cpu.push_back(correctedCpu);
				ping.push_back(curPing);
			}
		}
	}

	// calculate median values
	medianCpu = 0.0f;
	medianPing = 0;
	if (curSpeedCtrl == 1 && !cpu.empty()) {
		std::sort(cpu.begin(), cpu.end());
		std::sort(ping.begin(), ping.end());

		int midpos = cpu.size() / 2;
		medianCpu = cpu[midpos];
		medianPing = ping[midpos];
		if (midpos * 2 == cpu.size()) {
			medianCpu = (medianCpu + cpu[midpos - 1]) / 2.0f;
			medianPing = (medianPing + ping[midpos - 1]) / 2;
		}
		refCpuUsage = medianCpu;
	}

	// adjust game speed
	if (refCpuUsage > 0.0f && !isPaused) {
		//userSpeedFactor holds the wanted speed adjusted manually by user ( normally 1)
		//internalSpeed holds the current speed the sim is running
		//refCpuUsage holds the highest cpu if curSpeedCtrl == 0 or median if curSpeedCtrl == 1

		// aim for 60% cpu usage if median is used as reference and 75% cpu usage if max is the reference
		float wantedCpuUsage = (curSpeedCtrl == 1) ?  0.60f : 0.75f;

		//the following line can actually make it go faster than wanted normal speed ( userSpeedFactor )
		//if the current cpu of the target is smaller than the aimed cpu target but the clamp will cap it
		// the clamp will throttle it to the wanted one, otherwise it's a simple linear proportion aiming
		// to keep cpu load constant
		float newSpeed = internalSpeed/refCpuUsage*wantedCpuUsage;
		newSpeed = Clamp(newSpeed, 0.1f, userSpeedFactor);
		//average to smooth the speed change over time to reduce the impact of cpu spikes in the players
		newSpeed = (newSpeed + internalSpeed) * 0.5f;
#ifndef DEDICATED
		// in non-dedicated hosting, we'll add an additional safeguard to make sure the host can keep up with the game's speed
		// adjust game speed to localclient's (:= host) maximum SimFrame rate
		const float maxSimFPS = (1000.0f / gu->avgSimFrameTime) * (1.0f - gu->reconnectSimDrawBalance);
		newSpeed = Clamp(newSpeed, 0.1f, ((maxSimFPS / GAME_SPEED) + internalSpeed) * 0.5f);
#endif

		if (newSpeed != internalSpeed)
			InternalSpeedChange(newSpeed);
	}
}


/// has to be consistent with Game.cpp/CPlayerHandler
static std::vector<int> getPlayersInTeam(const std::vector<GameParticipant>& players, const int teamId)
{
	std::vector<int> playersInTeam;
	for (const GameParticipant& p: players) {
		// do not count spectators, or demos will desync
		if (!p.spectator && (p.team == teamId))
			playersInTeam.push_back(p.id);
	}
	return playersInTeam;
}

/**
 * Duplicates functionality of CPlayerHandler::ActivePlayersInTeam(int teamId)
 * as playerHandler is not available on the server
 */
static int countNumPlayersInTeam(const std::vector<GameParticipant>& players, const int teamId)
{
	return getPlayersInTeam(players, teamId).size();
}

/// has to be consistent with Game.cpp/CSkirmishAIHandler
static std::vector<unsigned char> getSkirmishAIIds(const std::map<unsigned char, GameSkirmishAI>& ais, const int teamId, const int hostPlayer = -2)
{
	std::vector<unsigned char> skirmishAIIds;
	for (std::map<unsigned char, GameSkirmishAI>::const_iterator ai = ais.begin(); ai != ais.end(); ++ai) {
		if ((ai->second.team == teamId) && ((hostPlayer == -2) || (ai->second.hostPlayer == hostPlayer)))
			skirmishAIIds.push_back(ai->first);
	}
	return skirmishAIIds;
}

/**
 * Duplicates functionality of CSkirmishAIHandler::GetSkirmishAIsInTeam(const int teamId)
 * as skirmishAIHandler is not available on the server
 */
static int countNumSkirmishAIsInTeam(const std::map<unsigned char, GameSkirmishAI>& ais, const int teamId)
{
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
			if (frameNum <= serverFrameNum && frameNum > players[a].lastFrameResponse)
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
				if (!players[a].isLocal && players[a].spectator && demoReader == NULL) {
					PrivateMessage(a, "Spectators cannot pause the game");
				}
				else {
					frameTimeLeft = 0.0f;

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
			if (!players[a].isLocal && players[a].spectator && demoReader == NULL) {
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

		case NETMSG_QUIT: {
			Message(str(format(PlayerLeft) %players[a].GetType() %players[a].name %" normal quit"));
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 1));
			players[a].Kill("[GameServer] user exited", true);
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
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %d sent invalid PlayerName: %s") %a %ex.what()));
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
				if (a < mutedPlayersChat.size() && mutedPlayersChat[a] ) {
					//this player is muted, drop his messages quietly
					break;
				}
				GotChatMessage(msg);
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid ChatMessage: %s") %players[a].name %ex.what()));
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
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %d sent invalid SystemMessage: %s") %a %ex.what()));
			}
			break;

		case NETMSG_STARTPOS: {
			const unsigned char player = inbuf[1];
			const unsigned int team = (unsigned)inbuf[2];
			const unsigned char rdyState = inbuf[3];

			if (player != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (myGameSetup->startPosType == CGameSetup::StartPos_ChooseInGame) {
				if (team >= teams.size()) {
					Message(str(format("Invalid teamID %d in NETMSG_STARTPOS from player %d") %team %player));
				} else if (getSkirmishAIIds(ais, team, player).empty() && ((team != players[player].team) || (players[player].spectator))) {
					Message(str(format("Player %d sent spoofed NETMSG_STARTPOS with teamID %d") %player %team));
				} else {
					teams[team].SetStartPos(float3(*((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12])));
					players[player].SetReadyToStart(rdyState != CPlayer::PLAYER_RDYSTATE_UPDATED);

					Broadcast(CBaseNetProtocol::Get().SendStartPos(player, team, rdyState, *((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12])));

					if (hostif) {
						hostif->SendPlayerReady(a, rdyState);
					}
				}
			} else {
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

				#ifndef ALLOW_DEMO_GODMODE
				if (demoReader == NULL)
				#endif
				{
					Broadcast(packet); //forward data
				}
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid Command: %s") %players[a].name %ex.what()));
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

				#ifndef ALLOW_DEMO_GODMODE
				if (demoReader == NULL)
				#endif
				{
					Broadcast(packet); //forward data
				}
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid Select: %s") %players[a].name %ex.what()));
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
				else if (demoReader == NULL)
					Broadcast(packet); //forward data
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid AICommand: %s") %players[a].name %ex.what()));
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
				else if (demoReader == NULL)
					Broadcast(packet); //forward data
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid AICommands: %s") %players[a].name %ex.what()));
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
				else if (demoReader == NULL)
					Broadcast(packet); //forward data
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid AIShare: %s") %players[a].name %ex.what()));
			}
		} break;


		case NETMSG_LUAMSG: {
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(str(format(WrongPlayer) %msgCode %a %(unsigned)playerNum));
					break;
				}
				if (demoReader == NULL) {
					Broadcast(packet); //forward data
					if (hostif)
						hostif->SendLuaMsg(packet->data, packet->length);
				}
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid LuaMsg: %s") %players[a].name %ex.what()));
			}
		} break;


		case NETMSG_SYNCRESPONSE: {
#ifdef SYNCCHECK
			netcode::UnpackPacket pckt(packet, 1);

			unsigned char playerNum; pckt >> playerNum;
			          int  frameNum; pckt >> frameNum;
			unsigned  int  checkSum; pckt >> checkSum;

			assert(a == playerNum);
			GameParticipant& p = players[a];

			if (outstandingSyncFrames.find(frameNum) != outstandingSyncFrames.end())
				p.syncResponse[frameNum] = checkSum;

			// update player's ping (if !defined(SYNCCHECK) this is done in NETMSG_KEYFRAME)
			if (frameNum <= serverFrameNum && frameNum > p.lastFrameResponse)
				p.lastFrameResponse = frameNum;

			// send player <a>'s sync-response back to everybody
			// (the only purpose of this is to allow a client to
			// detect if it is desynced wrt. a demo-stream)
			if ((frameNum % syncResponseEchoInterval) == 0) {
				Broadcast((CBaseNetProtocol::Get()).SendSyncResponse(playerNum, frameNum, checkSum));
			}
#endif
		} break;

		case NETMSG_SHARE:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (demoReader == NULL)
				Broadcast(CBaseNetProtocol::Get().SendShare(inbuf[1], inbuf[2], inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8])));
			break;

		case NETMSG_SETSHARE:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (demoReader == NULL)
				Broadcast(CBaseNetProtocol::Get().SendSetShare(inbuf[1], inbuf[2], *((float*)&inbuf[3]), *((float*)&inbuf[7])));
			break;

		case NETMSG_PLAYERSTAT:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			players[a].lastStats = *reinterpret_cast<const PlayerStatistics*>(&inbuf[2]);
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
				if (a < mutedPlayersDraw.size() && mutedPlayersDraw[a] ) {
					//this player is muted, drop his messages quietly
					break;
				}
				if (!players[playerNum].spectator || allowSpecDraw)
					Broadcast(packet); //forward data
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid MapDraw: %s") %players[a].name %ex.what()));
			}
			break;

		case NETMSG_DIRECT_CONTROL:
			if (inbuf[1] != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (demoReader == NULL) {
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
			if (demoReader == NULL)
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
					const unsigned toTeam = inbuf[3];
					// may be the players team or a team controlled by one of his AIs
					const unsigned giverTeam = inbuf[4];

					if (toTeam >= teams.size()) {
						Message(str(format("Invalid teamID %d in TEAMMSG_GIVEAWAY from player %d") %toTeam %player));
						break;
					}
					if (giverTeam >= teams.size()) {
						Message(str(format("Invalid teamID %d in TEAMMSG_GIVEAWAY from player %d") %giverTeam %player));
						break;
					}

					const std::vector<unsigned char>& totAIsInGiverTeam = getSkirmishAIIds(ais, giverTeam);
					const std::vector<unsigned char>& myAIsInGiverTeam  = getSkirmishAIIds(ais, giverTeam, player);

					const int numPlayersInGiverTeam          = countNumPlayersInTeam(players, giverTeam);
					const size_t numControllersInGiverTeam   = numPlayersInGiverTeam + totAIsInGiverTeam.size();
					const bool isGiverLeader                 = (teams[giverTeam].GetLeader() == player);
					const bool isGiverOwnTeam                = (giverTeam == fromTeam);
					const bool isSpec                        = players[player].spectator;
					const bool giverHasAIs                   = (!myAIsInGiverTeam.empty());
					const bool giverIsAllied                 = (teams[giverTeam].teamAllyteam == teams[fromTeam].teamAllyteam);
					const bool isSinglePlayer                = (players.size() <= 1);
					const char* playerType                   = players[player].GetType();

					if (!isSinglePlayer &&
						(isSpec || (!isGiverOwnTeam && !isGiverLeader) ||
						(giverHasAIs && !giverIsAllied && !cheating))) {
							Message(str(format("%s %s sent invalid team giveaway") %playerType %players[player].name), true);
							break;
					}

					Broadcast(CBaseNetProtocol::Get().SendGiveAwayEverything(player, toTeam, giverTeam));

					bool giveAwayOk = false;

					if (isGiverOwnTeam) {
						// player is giving stuff from his own team
						giveAwayOk = true;
						//players[player].team = 0;
						players[player].spectator = true;

						if (hostif)
							hostif->SendPlayerDefeated(player);
					} else {
						// player is giving stuff from one of his AI teams
						if (numPlayersInGiverTeam == 0) {
							// kill the first AI
							ais.erase(myAIsInGiverTeam[0]);
							giveAwayOk = true;
						} else {
							Message(str(format("%s %s can not give away stuff of team %i (still has human players left)") %playerType %players[player].name %giverTeam), true);
						}
					}

					if (giveAwayOk && (numControllersInGiverTeam == 1)) {
						// team has no controller left now
						teams[giverTeam].SetActive(false);
						teams[giverTeam].SetLeader(-1);

						const int toLeader = teams[toTeam].GetLeader();
						const std::string& toLeaderName = (toLeader >= 0) ? players[toLeader].name : UncontrolledPlayerName;

						std::ostringstream givenAwayMsg;
						givenAwayMsg << players[player].name << " gave everything to " << toLeaderName;
						Broadcast(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, givenAwayMsg.str()));
					}
					break;
				}
				case TEAMMSG_RESIGN: {
					const bool isSpec         = players[player].spectator;
					const bool isSinglePlayer = (players.size() <= 1);

					if (isSpec && !isSinglePlayer) {
						Message(str(format("Spectator %s sent invalid team resign") %players[player].name), true);
						break;
					}
					ResignPlayer(player);

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

					if (!teams[newTeam].HasLeader()) {
						teams[newTeam].SetLeader(player);
					}
					break;
				}
				case TEAMMSG_TEAM_DIED: {
					const unsigned teamID = inbuf[3];

#ifndef DEDICATED
					if (players[player].isLocal) { // currently only host is allowed
#else
					if (!players[player].desynced) {
#endif
						if (teamID >= teams.size()) {
							Message(str(format("Invalid teamID %d in TEAMMSG_TEAM_DIED from player %d") %teamID %player));
							break;
						}

						teams[teamID].SetActive(false);
						teams[teamID].SetLeader(-1);

						// convert all the teams players to spectators
						for (size_t p = 0; p < players.size(); ++p) {
							if ((players[p].team == teamID) && !(players[p].spectator)) {
								// are now spectating if this was their team
								//players[p].team = 0;
								players[p].spectator = true;

								if (hostif) {
									hostif->SendPlayerDefeated(p);
								}

								Broadcast(CBaseNetProtocol::Get().SendTeamDied(player, teamID));
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
				if (playerId != a) {
					Message(str(format(WrongPlayer) %msgCode %a %(unsigned)playerId));
					break;
				}
				unsigned char skirmishAIId_rec; // ignored, we have to create the real one
				pckt >> skirmishAIId_rec;
				unsigned char aiTeamId;
				pckt >> aiTeamId;
				std::string aiName;
				pckt >> aiName;

				if (aiTeamId >= teams.size()) {
					Message(str(format("Invalid teamID %d in NETMSG_AI_CREATED from player %d") %unsigned(aiTeamId) %unsigned(playerId)));
					break;
				}

				const unsigned char playerTeamId = players[playerId].team;

				GameTeam* tpl                    = &teams[playerTeamId];
				GameTeam* tai                    = &teams[aiTeamId];

				const bool weAreLeader           = (tai->GetLeader() == playerId);
				const bool weAreAllied           = (tpl->teamAllyteam == tai->teamAllyteam);
				const bool singlePlayer          = (players.size() <= 1);

				if (weAreLeader || singlePlayer || (weAreAllied && (cheating || !tai->HasLeader()))) {
					// creating the AI is ok
				} else {
					Message(str(format(NoAICreated) %players[playerId].name %(int)playerId %(int)aiTeamId));
					break;
				}
				const unsigned char skirmishAIId = ReserveNextAvailableSkirmishAIId();
				if (skirmishAIId == MAX_AIS) {
					Message(str(format("Unable to create AI, limit reached (%d)") %(int)MAX_AIS));
					break;
				}
				players[playerId].linkData[skirmishAIId] = GameParticipant::PlayerLinkData();
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

				if (!tai->HasLeader()) {
					tai->SetLeader(ais[skirmishAIId].hostPlayer);
					tai->SetActive(true);
				}
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid AICreated: %s") %players[a].name %ex.what()));
			}
			break;
		}
		case NETMSG_AI_STATE_CHANGED: {
			const unsigned char playerId     = inbuf[1];
			if (playerId != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)playerId));
				break;
			}
			const unsigned char skirmishAIId = inbuf[2];
			const ESkirmishAIStatus newState = (ESkirmishAIStatus) inbuf[3];

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
			const bool weAreLeader           = (tai->GetLeader() == playerId);
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
					players[playerId].linkData.erase(skirmishAIId);
					FreeSkirmishAIId(skirmishAIId);
					if ((numPlayersInAITeam + numAIsInAITeam) == 1) {
						// team has no controller left now
						tai->SetActive(false);
						tai->SetLeader(-1);
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
				if (!myGameSetup->fixedAllies) {
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

				if (static_cast<unsigned>(msg.GetPlayerID()) == a) {
					if ((commandBlacklist.find(msg.GetAction().command) != commandBlacklist.end()) && players[a].isLocal) {
						// command is restricted to server but player is allowed to execute it
						PushAction(msg.GetAction(), false);
					}
					else if (commandBlacklist.find(msg.GetAction().command) == commandBlacklist.end()) {
						// command is save
						Broadcast(packet);
					}
					else {
						// hack!
						Message(str(format(CommandNotAllowed) %msg.GetPlayerID() %msg.GetAction().command.c_str()));
					}
				}
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid CommandMessage: %s") %players[a].name %ex.what()));
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

				if (totalSize > fixedSize) {
					// if empty, the game has no defined winner(s)
					winningAllyTeams.resize(totalSize - fixedSize);
					pckt >> winningAllyTeams;
				}

				if (hostif)
					hostif->SendGameOver(playerNum, winningAllyTeams);
				Broadcast(CBaseNetProtocol::Get().SendGameOver(playerNum, winningAllyTeams));

				gameEndTime = spring_gettime();
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format("Player %s sent invalid GameOver: %s") %players[a].name %ex.what()));
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
		//case NETMSG_REJECT_CONNECT:
		//case NETMSG_GAMEDATA:
		//case NETMSG_RANDSEED:
		default: {
			Message(str(format(UnknownNetmsg) %msgCode %a));
		}
		break;
	}
}


void CGameServer::HandleConnectionAttempts()
{
	while (UDPNet && UDPNet->HasIncomingConnections()) {
		boost::shared_ptr<netcode::UDPConnection> prev = UDPNet->PreviewConnection().lock();
		boost::shared_ptr<const RawPacket> packet = prev->GetData();

		if (!packet) {
			UDPNet->RejectConnection();
			continue;
		}

		try {
			if (packet->length < 3) {
				std::string pkts;
				for (int i = 0; i < packet->length; ++i) {
					pkts += str(format(" 0x%x") %(int)packet->data[i]);
				}
				throw netcode::UnpackPacketException("Packet too short (data: " + pkts + ")");
			}

			if (packet->data[0] != NETMSG_ATTEMPTCONNECT)
				throw netcode::UnpackPacketException("Invalid message ID");

			netcode::UnpackPacket msg(packet, 3);
			std::string name, passwd, version;
			unsigned char reconnect, netloss;
			unsigned short netversion;
			msg >> netversion;
			msg >> name;
			msg >> passwd;
			msg >> version;
			msg >> reconnect;
			msg >> netloss;

			if (netversion != NETWORK_VERSION)
				throw netcode::UnpackPacketException(str(format("Wrong network version: %d, required version: %d") %(int)netversion %(int)NETWORK_VERSION));

			BindConnection(name, passwd, version, false, UDPNet->AcceptConnection(), reconnect, netloss);
		} catch (const netcode::UnpackPacketException& ex) {
			const std::string msg = str(format(ConnectionReject) %ex.what());
			prev->Unmute();
			prev->SendData(CBaseNetProtocol::Get().SendRejectConnect(msg));
			prev->SendData(CBaseNetProtocol::Get().SendQuit(msg)); //FIXME backward compatibility (remove ~2017)
			prev->Flush(true);
			Message(msg);
			UDPNet->RejectConnection();
		}
	}
}


void CGameServer::ServerReadNet()
{
	// handle new connections
	HandleConnectionAttempts();

	const float updateBandwidth = spring_tomsecs(spring_gettime() - lastBandwidthUpdate) / (float)playerBandwidthInterval;
	if (updateBandwidth >= 1.0f)
		lastBandwidthUpdate = spring_gettime();

	for (GameParticipant& player: players) {
		boost::shared_ptr<netcode::CConnection>& plink = player.link;
		if (!plink)
			continue; // player not connected
		if (plink->CheckTimeout(0, !gameHasStarted)) {
			Message(str(format(PlayerLeft) %player.GetType() %player.name %" timeout")); //this must happen BEFORE the reset!
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(player.id, 0));
			player.Kill("User timeout");
			if (hostif)
				hostif->SendPlayerLeft(player.id, 0);
			continue;
		}

		std::map<unsigned char, GameParticipant::PlayerLinkData>& pld = player.linkData;
		boost::shared_ptr<const RawPacket> packet;
		while ((packet = plink->GetData())) {  // relay all the packets to separate connections for the player and AIs
			unsigned char aiID = MAX_AIS;
			int cID = -1;
			if (packet->length >= 5) {
				cID = packet->data[0];
				if (cID == NETMSG_AICOMMAND || cID == NETMSG_AICOMMAND_TRACKED || cID == NETMSG_AICOMMANDS || cID == NETMSG_AISHARE)
					aiID = packet->data[4];
			}
			auto liit = pld.find(aiID);
			if (liit != pld.end())
				liit->second.link->SendData(packet);
			else
				Message(str(format("Player %s sent invalid AI ID %d in AICOMMAND %d") %player.name %(int)aiID %cID));
		}

		for (std::map<unsigned char, GameParticipant::PlayerLinkData>::iterator lit = pld.begin(); lit != pld.end(); ++lit) {
			int bandwidthUsage = lit->second.bandwidthUsage;
			boost::shared_ptr<netcode::CConnection>& link = lit->second.link;

			bool bwLimitWasReached = (globalConfig->linkIncomingPeakBandwidth > 0 && bandwidthUsage > globalConfig->linkIncomingPeakBandwidth);
			if (updateBandwidth >= 1.0f && globalConfig->linkIncomingSustainedBandwidth > 0)
				bandwidthUsage = std::max(0, bandwidthUsage - std::max(1, (int)((float)globalConfig->linkIncomingSustainedBandwidth / (1000.0f / (playerBandwidthInterval * updateBandwidth)))));

			int numDropped = 0;
			boost::shared_ptr<const RawPacket> packet;

			bool dropPacket = globalConfig->linkIncomingMaxWaitingPackets > 0 && (globalConfig->linkIncomingPeakBandwidth <= 0 || bwLimitWasReached);
			int ahead = 0;
			bool bwLimitIsReached = globalConfig->linkIncomingPeakBandwidth > 0 && bandwidthUsage > globalConfig->linkIncomingPeakBandwidth;
			while (link) {
				if (dropPacket)
					dropPacket = (NULL != (packet = link->Peek(globalConfig->linkIncomingMaxWaitingPackets)));
				packet = (!bwLimitIsReached || dropPacket) ? link->GetData() : link->Peek(ahead++);
				if (!packet)
					break;

				bool droppablePacket = (packet->length <= 0 || (packet->data[0] != NETMSG_SYNCRESPONSE && packet->data[0] != NETMSG_KEYFRAME));
				if (dropPacket && droppablePacket)
					++numDropped;
				else if (!bwLimitIsReached || !droppablePacket) {
					ProcessPacket(player.id, packet); // non droppable packets may be processed more than once, but this does no harm
					if (globalConfig->linkIncomingPeakBandwidth > 0 && droppablePacket) {
						bandwidthUsage += std::max((unsigned)linkMinPacketSize, packet->length);
						if (!bwLimitIsReached)
							bwLimitIsReached = (bandwidthUsage > globalConfig->linkIncomingPeakBandwidth);
					}
				}
			}
			if (numDropped > 0) {
				if (lit->first == MAX_AIS)
					PrivateMessage(player.id, str(format("Warning: Waiting packet limit was reached for %s [packets dropped]") %player.name));
				else
					PrivateMessage(player.id, str(format("Warning: Waiting packet limit was reached for %s AI #%d [packets dropped]") %player.name %(int)lit->first));
			}

			if (!bwLimitWasReached && bwLimitIsReached) {
				if (lit->first == MAX_AIS)
					PrivateMessage(player.id, str(format("Warning: Bandwidth limit was reached for %s [packets delayed]") %player.name));
				else
					PrivateMessage(player.id, str(format("Warning: Bandwidth limit was reached for %s AI #%d [packets delayed]") %player.name %(int)lit->first));
			}
		}
	}

#ifdef SYNCDEBUG
	CSyncDebugger::GetInstance()->ServerHandlePendingBlockRequests();
#endif
}


void CGameServer::GenerateAndSendGameID()
{
	// First and second dword are time based (current time and load time).
	gameID.intArray[0] = (unsigned) time(NULL);
	for (int i = 4; i < 12; ++i)
		gameID.charArray[i] = rng();

	// Third dword is CRC of setupText.
	CRC crc;
	crc.Update(myGameSetup->setupText.c_str(), myGameSetup->setupText.length());
	gameID.intArray[2] = crc.GetDigest();

	CRC entropy;
	entropy.Update((lastNewFrameTick - serverStartTime).toNanoSecsi());
	gameID.intArray[3] = entropy.GetDigest();

	// fixed gameID?
	if (!myGameSetup->gameID.empty()) {
		unsigned char p[16];
	#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
		// workaround missing C99 support in a msvc lib with %2hhx
		generatedGameID = (sscanf(myGameSetup->gameID.c_str(),
		      "%02hc%02hc%02hc%02hc%02hc%02hc%02hc%02hc"
		      "%02hc%02hc%02hc%02hc%02hc%02hc%02hc%02hc",
		      &p[ 0], &p[ 1], &p[ 2], &p[ 3], &p[ 4], &p[ 5], &p[ 6], &p[ 7],
		      &p[ 8], &p[ 9], &p[10], &p[11], &p[12], &p[13], &p[14], &p[15]) == 16);
	#else
		generatedGameID = (sscanf(myGameSetup->gameID.c_str(),
		       "%hhx%hhx%hhx%hhx%hhx%hhx%hhx%hhx"
		       "%hhx%hhx%hhx%hhx%hhx%hhx%hhx%hhx",
		       &p[ 0], &p[ 1], &p[ 2], &p[ 3], &p[ 4], &p[ 5], &p[ 6], &p[ 7],
		       &p[ 8], &p[ 9], &p[10], &p[11], &p[12], &p[13], &p[14], &p[15]) == 16);
	#endif
		if (generatedGameID)
			for (int i = 0; i<16; ++i)
				gameID.charArray[i] = p[i];
	}

	Broadcast(CBaseNetProtocol::Get().SendGameID(gameID.charArray));

	if (demoRecorder != NULL) {
		demoRecorder->SetGameID(gameID.charArray);
	}

	generatedGameID = true;
}

void CGameServer::CheckForGameStart(bool forced)
{
	assert(!gameHasStarted);
	bool allReady = true;

	for (size_t a = static_cast<size_t>(myGameSetup->numDemoPlayers); a < players.size(); a++) {
		if (players[a].myState == GameParticipant::UNCONNECTED && serverStartTime + spring_secs(30) < spring_gettime()) {
			// autostart the game when 45 seconds have passed and everyone who managed to connect is ready
			continue;
		}
		else if (players[a].myState < GameParticipant::INGAME) {
			allReady = false;
			break;
		} else if (!players[a].spectator && teams[players[a].team].IsActive() && !players[a].IsReadyToStart() && demoReader == NULL) {
			allReady = false;
			break;
		}
	}

	// msecs to wait until the game starts after all players are ready
	const spring_time gameStartDelay = spring_secs(myGameSetup->gameStartDelay);

	if (allReady || forced) {
		if (!spring_istime(readyTime)) {
			readyTime = spring_gettime();

			// we have to wait at least 1 msec during countdown, because 0 is a special case
			Broadcast(CBaseNetProtocol::Get().SendStartPlaying(std::max(boost::int64_t(1), spring_tomsecs(gameStartDelay))));

			// make seed more random
			if (myGameSetup->gameID.empty())
				rng.Seed(spring_tomsecs(readyTime - serverStartTime));
		}
	}
	if (spring_istime(readyTime) && ((spring_gettime() - readyTime) > gameStartDelay)) {
		StartGame(forced);
	}
}

void CGameServer::StartGame(bool forced)
{
	assert(!gameHasStarted);
	gameHasStarted = true;
	startTime = gameTime;
	if (!canReconnect && !allowSpecJoin)
		packetCache.clear(); // free memory

	if (UDPNet && !canReconnect && !allowSpecJoin)
		UDPNet->SetAcceptingConnections(false); // do not accept new connections

	// make sure initial game speed is within allowed range and send a new speed if not
	UserSpeedChange(userSpeedFactor, SERVER_PLAYER);

	if (demoReader) {
		// the client told us to start a demo
		// no need to send startPos and startplaying since its in the demo
		Message(DemoStart);
		return;
	}

	{
		std::vector<bool> teamStartPosSent(teams.size(), false);

		// send start position for player-controlled teams
		for (GameParticipant& p: players) {
			if (p.spectator)
				continue;

			const unsigned int team = p.team;
			const float3& teamStartPos = teams[team].GetStartPos();

			if (false && !teams[team].HasLeader())
				continue;

			teamStartPosSent[team] = true;

			if (p.IsReadyToStart()) {
				// ready players do not care whether start is forced
				Broadcast(CBaseNetProtocol::Get().SendStartPos(p.id, (int) team, CPlayer::PLAYER_RDYSTATE_READIED, teamStartPos.x, teamStartPos.y, teamStartPos.z));
				continue;
			}
			if (forced) {
				// everyone else does
				Broadcast(CBaseNetProtocol::Get().SendStartPos(p.id, (int) team, CPlayer::PLAYER_RDYSTATE_FORCED, teamStartPos.x, teamStartPos.y, teamStartPos.z));
				continue;
			}

			// normal start but player failed to ready (is this possible?)
			// if player never even sent a position to us, this will equal
			// ZeroVector which is rarely picked during normal game-starts
			Broadcast(CBaseNetProtocol::Get().SendStartPos(p.id, (int) team, CPlayer::PLAYER_RDYSTATE_FAILED, teamStartPos.x, teamStartPos.y, teamStartPos.z));
		}

		// send start position for all other teams
		// non-controlled teams are always ready
		for (size_t a = 0; a < teams.size(); ++a) {
			if (teamStartPosSent[a])
				continue;

			const float3& teamStartPos = teams[a].GetStartPos();

			// FIXME?
			//   if a team has no players controlling it, then it
			//   should also not have have a lead player yet this
			//   fails
			//
			// assert(!teams[a].HasLeader());
			Broadcast(CBaseNetProtocol::Get().SendStartPos(SERVER_PLAYER, a, CPlayer::PLAYER_RDYSTATE_READIED, teamStartPos.x, teamStartPos.y, teamStartPos.z));
		}
	}

	Broadcast(CBaseNetProtocol::Get().SendStartPlaying(0));

	if (hostif != NULL) {
		if (demoRecorder != NULL) {
			hostif->SendStartPlaying(gameID.charArray, demoRecorder->GetName());
		} else {
			hostif->SendStartPlaying(gameID.charArray, "");
		}
	}

	frameTimeLeft = 0.0f;
	lastNewFrameTick = spring_gettime() - spring_msecs(1);

	CreateNewFrame(true, false);
}

void CGameServer::SetGamePausable(const bool arg)
{
	gamePausable = arg;
}

void CGameServer::PushAction(const Action& action, bool fromAutoHost)
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
			for (GameParticipant& p: players) {
				std::string playerLower = StringToLower(p.name);
				if (playerLower.find(name)==0) {	// can kick on substrings of name
					if (!p.isLocal) // do not kick host
						KickPlayer(p.id);
				}
			}
		}
	}
	else if (action.command == "mute") {
		if (action.extra.empty()) {
			LOG_L(L_WARNING, "Failed to mute player, usage: /mute <playername> [chatmute] [drawmute]");
		} else {
			const std::vector<std::string>& tokens = CSimpleParser::Tokenize(action.extra);

			if (tokens.empty() || tokens.size() > 3) {
				LOG_L(L_WARNING, "Failed to mute player, usage: /mute <playername> [chatmute] [drawmute]");
			} else {
				const std::string name = StringToLower(tokens[0]);

				bool muteChat = true;
				bool muteDraw = true;

				if (tokens.size() >= 2) InverseOrSetBool(muteChat, tokens[1]);
				if (tokens.size() >= 3) InverseOrSetBool(muteDraw, tokens[2]);

				for (GameParticipant& p: players) {
					const std::string playerLower = StringToLower(p.name);

					if (playerLower.find(name) == 0) {	// can kick on substrings of name
						MutePlayer(p.id, muteChat, muteDraw);
						break;
					}
				}
			}
		}
	}
	else if (action.command == "mutebynum") {

		if (action.extra.empty()) {
			LOG_L(L_WARNING, "Failed to mute player, usage: /mutebynum <player-id> [chatmute] [drawmute]");
		} else {
			const std::vector<std::string>& tokens = CSimpleParser::Tokenize(action.extra);

			if (tokens.empty() || tokens.size() > 3) {
				LOG_L(L_WARNING, "Failed to mute player, usage: /mutebynum <player-id> [chatmute] [drawmute]");
			} else {
				const int playerID = atoi(tokens[0].c_str());
				bool muteChat = true;
				bool muteDraw = true;

				if (tokens.size() >= 2) InverseOrSetBool(muteChat, tokens[1]);
				if (tokens.size() >= 3) InverseOrSetBool(muteDraw, tokens[2]);

				MutePlayer(playerID, muteChat, muteDraw);
			}
		}
	}
	if (action.command == "specbynum") {
		if (!action.extra.empty()) {
			const int playerNum = atoi(action.extra.c_str());
			SpecPlayer(playerNum);
		}
	}
	else if (action.command == "spec") {
		if (!action.extra.empty()) {
			std::string name = action.extra;
			StringToLowerInPlace(name);
			for (GameParticipant& p: players) {
				std::string playerLower = StringToLower(p.name);
				if (playerLower.find(name)==0) {	// can spec on substrings of name
					SpecPlayer(p.id);
				}
			}
		}
	}
	else if (action.command == "nopause") {
		InverseOrSetBool(gamePausable, action.extra);
	}
	else if (action.command == "nohelp") {
		InverseOrSetBool(noHelperAIs, action.extra);
		// sent it because clients have to do stuff when this changes
		CommandMessage msg(action, SERVER_PLAYER);
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
	}
	else if (action.command == "nospecdraw") {
		InverseOrSetBool(allowSpecDraw, action.extra, true);
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
		InverseOrSetBool(cheating, action.extra);
		CommandMessage msg(action, SERVER_PLAYER);
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
	}
	else if (action.command == "singlestep") {
		if (isPaused) {
			if (demoReader != NULL) {
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
				const std::string& pwd = tokens[1];
				int team = 0;
				bool spectator = true;
				if ( tokens.size() > 2 ) {
					spectator = (tokens[2] == "0") ? false : true;
				}
				if ( tokens.size() > 3 ) {
					team = atoi(tokens[3].c_str());
				}
				// note: this must only compare by name
				auto participantIter = std::find_if(players.begin(), players.end(), [&name](GameParticipant& p) {
					return p.name == name;
				});

				if (participantIter != players.end()) {
					participantIter->SetValue("password", pwd);

					LOG("Changed player/spectator password: \"%s\" \"%s\"", name.c_str(), pwd.c_str());
				} else {
					AddAdditionalUser(name, pwd, false, spectator, team);

					LOG("Added client \"%s\" with password \"%s\" to team %d (as a %s)",
						name.c_str(), pwd.c_str(), team, (spectator? "spectator": "player")
					);
				}
			} else {
				LOG_L(L_WARNING,
					"Failed to add player/spectator password. usage: "
					"/adduser <player-name> <password> [spectator] [team]"
				);
			}
		}
	}
	else if (action.command == "kill") {
		LOG("Server killed!!!");
		quitServer = true;
	}
	else if (action.command == "pause") {
		if (gameHasStarted) {
			// action can originate from autohost prior to start
			// (normal clients are blocked from sending any pause
			// commands during this period)
			bool newPausedState = isPaused;

			if (action.extra.empty()) {
				// if no param is given, toggle paused state
				newPausedState = !isPaused;
			} else {
				// if a param is given, interpret it as "bool setPaused"
				InverseOrSetBool(newPausedState, action.extra);
			}

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
	return quitServer;
}

void CGameServer::CreateNewFrame(bool fromServerThread, bool fixedFrameTime)
{
	if (demoReader != NULL) {
		CheckSync();
		SendDemoData(-1);
		return;
	}

	Threading::RecursiveScopedLock(gameServerMutex, !fromServerThread);
	CheckSync();

	const bool vidRecording = videoCapturing->IsCapturing();
	const bool normalFrame = !isPaused && !vidRecording;
	const bool videoFrame = !isPaused && fixedFrameTime;
	const bool singleStep = fixedFrameTime && !vidRecording;

	unsigned int numNewFrames = 1;

	if (logDebugMessages) {
		LOG_L(
			L_INFO, // L_DEBUG only works in DEBUG builds which are slow and affect timings
			"[%s][1][sf=%d] fromServerThread=%d fixedFrameTime=%d hasLocalClient=%d normalFrame=%d",
			__FUNCTION__, serverFrameNum, fromServerThread, fixedFrameTime, HasLocalClient(), normalFrame
		);
	}

	if (!fixedFrameTime) {
		spring_time currentTick = spring_gettime();
		spring_time timeElapsed = currentTick - lastNewFrameTick;

		if (timeElapsed > spring_msecs(200))
			timeElapsed = spring_msecs(200);

		frameTimeLeft += ((GAME_SPEED * 0.001f) * internalSpeed * timeElapsed.toMilliSecsf());
		lastNewFrameTick = currentTick;
		numNewFrames = (frameTimeLeft > 0.0f)? int(math::ceil(frameTimeLeft)): 0;
		frameTimeLeft -= numNewFrames;

		if (logDebugMessages) {
			LOG_L(
				L_INFO,
				"\t[2] timeElapsed=%fms frameTimeLeft=%f internalSpeed=%f numNewFrames=%u",
				timeElapsed.toMilliSecsf(), frameTimeLeft, internalSpeed, numNewFrames
			);
		}

		#ifndef DEDICATED
		if (HasLocalClient()) {
			// Don't create new frames when localClient (:= host) isn't able to process them fast enough.
			// Despite this still allow to create a few in advance to not lag other connected clients.
			//
			// DS never has a local client and isn't linked against GU
			//
			// simFramesBehind =  0 --> ratio = 0.00 --> maxNewFrames = 30
			// simFramesBehind =  1 --> ratio = 0.03 --> maxNewFrames = 29
			// simFramesBehind = 15 --> ratio = 0.5  --> maxNewFrames = 15
			// simFramesBehind = 29 --> ratio = 0.97 --> maxNewFrames =  1
			// simFramesBehind = 30 --> ratio = 1.00 --> maxNewFrames =  1
			const float simFramesBehind = serverFrameNum - players[localClientNumber].lastFrameResponse;
			const float simFrameMixRatio = std::min(simFramesBehind / GAME_SPEED, 1.0f);

			const unsigned int curSimRate   = std::max(gu->simFPS, GAME_SPEED * 1.0f); // advance at most 1s into the future
			const unsigned int maxNewFrames = mix(curSimRate * 1.0f, 0.0f, simFrameMixRatio); // (fps + (0 - fps) * a)

			numNewFrames = std::min(numNewFrames, maxNewFrames);

			if (logDebugMessages) {
				LOG_L(
					L_INFO,
					"\t[3] simFramesBehind=%f simFrameMixRatio=%f curSimRate=%u maxNewFrames=%u numNewFrames=%u",
					simFramesBehind, simFrameMixRatio, curSimRate, maxNewFrames, numNewFrames
				);
			}
		}
		#endif
	}

	if (normalFrame || videoFrame || singleStep) {
		assert(demoReader == NULL);

		for (unsigned int i = 0; i < numNewFrames; ++i) {
			++serverFrameNum;

			// Send out new frame messages.
			if ((serverFrameNum % serverKeyframeInterval) == 0) {
				Broadcast(CBaseNetProtocol::Get().SendKeyFrame(serverFrameNum));
			} else {
				Broadcast(CBaseNetProtocol::Get().SendNewFrame());
			}

			// every gameProgressFrameInterval, we broadcast current frame in a
			// special message (that doesn't get cached and skips normal queue)
			// to let players know their loading %
			if ((serverFrameNum % gameProgressFrameInterval) == 0) {
				CBaseNetProtocol::PacketType progressPacket = CBaseNetProtocol::Get().SendCurrentFrameProgress(serverFrameNum);
				// we cannot use broadcast here, since we want to skip caching
				for (GameParticipant& p: players) {
					p.SendData(progressPacket);
				}
			}
		#ifdef SYNCCHECK
			outstandingSyncFrames.insert(serverFrameNum);
		#endif
		}
	}
}


void CGameServer::UpdateSpeedControl(int speedCtrl)
{
	if (speedCtrl != curSpeedCtrl) {
		Message(str(format("Server speed control: %s") %(SpeedControlToString(speedCtrl).c_str())));
		curSpeedCtrl = speedCtrl;
	}
}


std::string CGameServer::SpeedControlToString(int speedCtrl)
{
	std::string desc = "<invalid>";
	if (speedCtrl == 0) {
		desc = "Maximum CPU";
	} else
	if (speedCtrl == 1) {
		desc = "Average CPU";
	}
	return desc;
}


__FORCE_ALIGN_STACK__
void CGameServer::UpdateLoop()
{
	try {
		Threading::SetThreadName("netcode");
		Threading::SetAffinity(~0);

		while (!quitServer) {
			spring_sleep(spring_msecs(loopSleepTime));

			if (UDPNet)
				UDPNet->Update();

			Threading::RecursiveScopedLock scoped_lock(gameServerMutex);
			ServerReadNet();
			Update();
		}

		if (hostif)
			hostif->SendQuit();

		Broadcast(CBaseNetProtocol::Get().SendQuit("Server shutdown"));

		if (!reloadingServer) {
			// this is to make sure the Flush has any effect at all (we don't want a forced flush)
			// when reloading, we can assume there is only a local client and skip the sleep()'s
			spring_sleep(spring_msecs(500));
		}

		// flush the quit messages to reduce ugly network error messages on the client side
		for (GameParticipant& p: players) {
			if (p.link)
				p.link->Flush();
		}

		if (!reloadingServer) {
			// now let clients close their connections
			spring_sleep(spring_msecs(1500));
		}

	} CATCH_SPRING_ERRORS
}


void CGameServer::KickPlayer(const int playerNum)
{
	if (!players[playerNum].link) { // only kick connected players
		Message(str(format("Attempt to kick user %d who is not connected") %playerNum));
		return;
	}
	Message(str(format(PlayerLeft) %players[playerNum].GetType() %players[playerNum].name %"kicked"));
	Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(playerNum, 2));
	players[playerNum].Kill("Kicked from the battle", true);
	if (hostif)
		hostif->SendPlayerLeft(playerNum, 2);
}


void CGameServer::MutePlayer(const int playerNum, bool muteChat, bool muteDraw)
{
	if (playerNum >= players.size()) {
		LOG_L(L_WARNING, "MutePlayer: invalid playerNum");
		return;
	}
	if ( playerNum >= mutedPlayersChat.size() ) {
		mutedPlayersChat.resize(playerNum+1);
	}
	if ( playerNum >= mutedPlayersDraw.size() ) {
		mutedPlayersDraw.resize(playerNum+1);
	}
	mutedPlayersChat[playerNum] = muteChat;
	mutedPlayersDraw[playerNum] = muteDraw;
}


void CGameServer::SpecPlayer(const int player)
{
	if (!players[player].link) {
		Message(str(format("Attempt to spec user %d who is not connected") %player));
		return;
	}
	if (players[player].spectator) {
		Message(str(format("Attempt to spec user %d who is spectating already") %player));
		return;
	}
	Message(str(format(PlayerResigned) %players[player].name %"forced spec"));
	ResignPlayer(player);
}


void CGameServer::ResignPlayer(const int player)
{
	Broadcast(CBaseNetProtocol::Get().SendResign(player));

	//players[player].team = 0;
	players[player].spectator = true;

	// update all teams of which the player is leader
	for (size_t t = 0; t < teams.size(); ++t) {
		if (teams[t].GetLeader() != player)
			continue;

		const std::vector<int> &teamPlayers = getPlayersInTeam(players, t);
		const std::vector<unsigned char>& teamAIs  = getSkirmishAIIds(ais, t);

		if ((teamPlayers.size() + teamAIs.size()) == 0) {
			// no controllers left in team
			teams[t].SetActive(false);
			teams[t].SetLeader(-1);
		} else if (teamPlayers.empty()) {
			// no human player left in team
			teams[t].SetLeader(ais[teamAIs[0]].hostPlayer);
		} else {
			// still human controllers left in team
			teams[t].SetLeader(teamPlayers[0]);
		}
	}

	if (hostif)
		hostif->SendPlayerDefeated(player);
}


bool CGameServer::CheckPlayersPassword(const int playerNum, const std::string& pw) const
{
	if (playerNum >= players.size()) // new player
		return true;

	const GameParticipant::customOpts& opts = players[playerNum].GetAllValues();
	auto it = opts.find("password");

	if (it == opts.end() || it->second == pw)
		return true;

	return false;
}


void CGameServer::AddAdditionalUser(const std::string& name, const std::string& passwd, bool fromDemo, bool spectator, int team, int playerNum)
{
	if (playerNum < 0)
		playerNum = players.size();
	if (playerNum >= players.size())
		players.resize(playerNum + 1);

	GameParticipant& p = players[playerNum];
	assert(p.myState == GameParticipant::UNCONNECTED); // we only add _new_ players here, we don't handle reconnects here!
	p.id = playerNum;
	p.name = name;
	p.spectator = spectator;
	p.team = team;
	p.isMidgameJoin = true;
	p.isFromDemo = fromDemo;
	if (!passwd.empty())
		p.SetValue("password", passwd);

	// inform all the players of the newcomer
	if (!fromDemo)
		Broadcast(CBaseNetProtocol::Get().SendCreateNewPlayer(p.id, p.spectator, p.team, p.name));
}


unsigned CGameServer::BindConnection(std::string name, const std::string& passwd, const std::string& version, bool isLocal, boost::shared_ptr<netcode::CConnection> link, bool reconnect, int netloss)
{
	Message(str(format("%s attempt from %s") %(reconnect ? "Reconnection" : "Connection") %name));
	Message(str(format(" -> Version: %s") %version));
	Message(str(format(" -> Address: %s") %link->GetFullAddress()), false);

	if (link->CanReconnect())
		canReconnect = true;

	std::string errmsg = "";
	bool terminate = false;

	size_t newPlayerNumber = players.size();

	// find the player in the current list
	for (GameParticipant& p: players) {
		if (name != p.name)
			continue;

		if (p.isFromDemo) {
			errmsg = "User name duplicated in the demo";
		} else
		if (!p.link) {
			if (reconnect)
				errmsg = "User is not ingame";
			else if (canReconnect || !gameHasStarted)
				newPlayerNumber = p.id;
			else
				errmsg = "Game has already started";
		}
		else {
			bool reconnectAllowed = canReconnect && p.link->CheckTimeout(-1);
			if (!reconnect && reconnectAllowed) {
				newPlayerNumber = p.id;
				terminate = true;
			}
			else if (reconnect && reconnectAllowed && p.link->GetFullAddress() != link->GetFullAddress())
				newPlayerNumber = p.id;
			else
				errmsg = "User is already ingame";
		}
		break;
	}

	// not found in the original start script, allow spector join?
	if (errmsg.empty() && newPlayerNumber >= players.size()) {
		if (!demoReader && allowSpecJoin) { //add prefix to "anonymous" spectators (#4949)
			name = std::string("~") + name;
		}
		if (demoReader || allowSpecJoin)
			AddAdditionalUser(name, passwd);
		else
			errmsg = "User name not authorized to connect";
	}

	// check user's password
	if (errmsg.empty() && !isLocal) // disable pw check for local host
		if (!CheckPlayersPassword(newPlayerNumber, passwd))
			errmsg = "Incorrect password";

	if (!reconnect) // don't respond before we are sure we want to do it
		link->Unmute(); // never respond to reconnection attempts, it could interfere with the protocol and desync

	// >> Reject Connection <<
	if (!errmsg.empty() || newPlayerNumber >= players.size()) {
		Message(str(format(" -> %s") %errmsg));
		link->SendData(CBaseNetProtocol::Get().SendQuit(str(format("Connection rejected: %s") %errmsg)));
		return 0;
	}

	// >> Accept Connection <<
	GameParticipant& newPlayer = players[newPlayerNumber];
	newPlayer.isReconn = gameHasStarted;

	// there is a running link already -> terminate it
	if (terminate) {
		Message(str(format(PlayerLeft) %newPlayer.GetType() %newPlayer.name %" terminating existing connection"));
		Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(newPlayerNumber, 0));
		newPlayer.link.reset(); // prevent sending a quit message since this might kill the new connection
		newPlayer.Kill("Terminating connection");
		if (hostif)
			hostif->SendPlayerLeft(newPlayerNumber, 0);
	}

	// inform the player about himself if it's a midgame join
	if (newPlayer.isMidgameJoin)
		link->SendData(CBaseNetProtocol::Get().SendCreateNewPlayer(newPlayerNumber, newPlayer.spectator, newPlayer.team, newPlayer.name));

	// there is an open link -> reconnect
	if (newPlayer.link) {
		newPlayer.link->ReconnectTo(*link);
		if (UDPNet)
			UDPNet->UpdateConnections();
		Message(str(format(" -> Connection reestablished (id %i)") %newPlayerNumber));
		newPlayer.link->SetLossFactor(netloss);
		newPlayer.link->Flush(!gameHasStarted);
		return newPlayerNumber;
	}

	newPlayer.Connected(link, isLocal);
	newPlayer.SendData(boost::shared_ptr<const RawPacket>(myGameData->Pack()));
	newPlayer.SendData(CBaseNetProtocol::Get().SendSetPlayerNum((unsigned char)newPlayerNumber));

	// after gamedata and playerNum, the player can start loading
	// throw at him all stuff he missed until now
	for (auto& pv: packetCache)
		for (boost::shared_ptr<const netcode::RawPacket>& p: pv)
			newPlayer.SendData(p);

	if (demoReader == NULL || myGameSetup->demoName.empty()) {
		// player wants to play -> join team
		if (!newPlayer.spectator) {
			unsigned newPlayerTeam = newPlayer.team;
			if (!teams[newPlayerTeam].IsActive()) { // create new team
				newPlayer.SetReadyToStart(myGameSetup->startPosType != CGameSetup::StartPos_ChooseInGame);
				teams[newPlayerTeam].SetActive(true);
			}
			Broadcast(CBaseNetProtocol::Get().SendJoinTeam(newPlayerNumber, newPlayerTeam));
		}
	}

	// new connection established
	Message(str(format(" -> Connection established (given id %i)") %newPlayerNumber));
	link->SetLossFactor(netloss);
	link->Flush(!gameHasStarted);
	return newPlayerNumber;
}


void CGameServer::GotChatMessage(const ChatMessage& msg)
{
	if (!msg.msg.empty()) { // silently drop empty chat messages
		Broadcast(boost::shared_ptr<const RawPacket>(msg.Pack()));
		if (hostif && msg.fromPlayer >= 0 && msg.fromPlayer != SERVER_PLAYER) {
			// do not echo packets to the autohost
			hostif->SendPlayerChat(msg.fromPlayer, msg.destination, msg.msg);
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
	newSpeed = Clamp(newSpeed, minUserSpeed, maxUserSpeed);

	if (userSpeedFactor != newSpeed) {
		if (internalSpeed > newSpeed || internalSpeed == userSpeedFactor) // insta-raise speed when not slowed down
			InternalSpeedChange(newSpeed);

		Broadcast(CBaseNetProtocol::Get().SendUserSpeed(player, newSpeed));
		userSpeedFactor = newSpeed;
	}
}


unsigned char CGameServer::ReserveNextAvailableSkirmishAIId()
{
	if (usedSkirmishAIIds.size() >= MAX_AIS)
		return MAX_AIS; // no available IDs

	unsigned char skirmishAIId = 0;
	// find a free id
	std::list<unsigned char>::iterator it;
	for (it = usedSkirmishAIIds.begin(); it != usedSkirmishAIIds.end(); ++it, skirmishAIId++) {
		if (*it != skirmishAIId)
			break;
	}

	usedSkirmishAIIds.insert(it, skirmishAIId);

	return skirmishAIId;
}


void CGameServer::FreeSkirmishAIId(const unsigned char skirmishAIId)
{
	usedSkirmishAIIds.remove(skirmishAIId);
}


void CGameServer::AddToPacketCache(boost::shared_ptr<const netcode::RawPacket> &pckt)
{
	if (packetCache.empty() || packetCache.back().size() >= PKTCACHE_VECSIZE) {
		packetCache.push_back(std::vector<boost::shared_ptr<const netcode::RawPacket> >());
		packetCache.back().reserve(PKTCACHE_VECSIZE);
	}
	packetCache.back().push_back(pckt);
}
