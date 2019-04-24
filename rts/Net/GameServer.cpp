/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Net/UDPListener.h"
#include "System/Net/UDPConnection.h"

#include <functional>

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
#ifndef DEDICATED
#include "Game/IVideoCapturing.h"
#endif
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"

#include "Net/Protocol/BaseNetProtocol.h"

// This undef is needed, as somewhere there is a type interface specified,
// which we need not!
// (would cause problems in ExternalAI/Interface/SAIInterfaceLibrary.h)
#ifdef interface
	#undef interface
#endif
#include "System/CRC.h"
#include "System/GlobalConfig.h"
#include "System/MsgStrings.h"
#include "System/SpringMath.h"
#include "System/SpringExitCode.h"
#include "System/SpringFormat.h"
#include "System/TdfParser.h"
#include "System/StringUtil.h"
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
#include "System/Threading/SpringThreading.h"

#ifndef DEDICATED
#include "lib/luasocket/src/restrictions.h"
#endif

#define ALLOW_DEMO_GODMODE

using netcode::RawPacket;


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
static constexpr unsigned SYNCCHECK_TIMEOUT = 300;

/// used to prevent msg spam
static constexpr unsigned SYNCCHECK_MSG_TIMEOUT = 400;

/// The time interval in msec for sending player statistics to each client
static const spring_time playerInfoTime = spring_secs(2);

/// every n'th frame will be a keyframe (and contain the server's framenumber)
static constexpr unsigned serverKeyframeInterval = 16;

/// players incoming bandwidth new allowance every X milliseconds
static constexpr unsigned playerBandwidthInterval = 100;

/// every 10 sec we'll broadcast current frame in a message that skips queue & cache
/// to let clients that are fast-forwarding to current point to know their loading %
static constexpr unsigned gameProgressFrameInterval = GAME_SPEED * 10;

static constexpr unsigned syncResponseEchoInterval = GAME_SPEED * 2;


//FIXME remodularize server commands, so they get registered in word completion etc.
static const std::array<std::string, 23> SERVER_COMMANDS = {
	"kick", "kickbynum",
	"mute", "mutebynum",
	"setminspeed", "setmaxspeed",
	"nopause", "nohelp", "cheat", "godmode", "globallos",
	"nocost", "forcestart", "nospectatorchat", "nospecdraw",
	"skip", "reloadcob", "reloadcegs", "devlua", "editdefs",
	"singlestep", "spec", "specbynum"
};

std::array<std::string, 23> CGameServer::commandBlacklist = SERVER_COMMANDS;



CGameServer* gameServer = nullptr;

CGameServer::CGameServer(
	const std::shared_ptr<const ClientSetup> newClientSetup,
	const std::shared_ptr<const    GameData> newGameData,
	const std::shared_ptr<const  CGameSetup> newGameSetup
)
: serverFrameNum(-1)

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
, quitServer(false)
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
	thread.join();
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
	if (!myGameSetup->onlyLocal)
		UDPNet.reset(new netcode::UDPListener(myClientSetup->hostPort, myClientSetup->hostIP));

	AddAutohostInterface(StringToLower(configHandler->GetString("AutohostIP")), configHandler->GetInt("AutohostPort"));
	Message(spring::format(ServerStart, myClientSetup->hostPort), false);

	// start script
	maxUserSpeed = myGameSetup->maxSpeed;
	minUserSpeed = myGameSetup->minSpeed;
	noHelperAIs  = myGameSetup->noHelperAIs;
	StripGameSetupText(myGameData.get());

	// load demo (if there is one)
	if (myGameSetup->hostDemo) {
		Message(spring::format(PlayingDemo, myGameSetup->demoName.c_str()));
		demoReader.reset(new CDemoReader(myGameSetup->demoName, modGameTime + 0.1f));
	}

	// initialize players, teams & ais
	{
		pingTimeFilter.fill(spring_notime);
		clientDrawFilter.fill({spring_notime, 0});
		clientMuteFilter.fill({false, false});

		const std::vector<PlayerBase>& playerStartData = myGameSetup->GetPlayerStartingDataCont();
		const std::vector<TeamBase>&     teamStartData = myGameSetup->GetTeamStartingDataCont();
		const std::vector<SkirmishAIData>& aiStartData = myGameSetup->GetAIStartingDataCont();

		players.reserve(MAX_PLAYERS); // no reallocation please
		teams.resize(teamStartData.size());

		players.resize(playerStartData.size());
		if (demoReader != nullptr) {
			const size_t demoPlayers = demoReader->GetFileHeader().numPlayers;
			players.resize(std::max(demoPlayers, playerStartData.size()));
			if (players.size() >= MAX_PLAYERS) {
				Message(spring::format("Too many Players (%d) in the demo", players.size()));
			}
		}

		std::copy(playerStartData.begin(), playerStartData.end(), players.begin());
		std::copy(teamStartData.begin(), teamStartData.end(), teams.begin());

		//FIXME move playerID to PlayerBase, so it gets copied from playerStartData
		for (size_t n = 0; n < players.size(); n++)
			players[n].id = n;

		skirmishAIs.clear();
		skirmishAIs.resize(MAX_AIS, {false, {}});
		freeSkirmishAIs.clear();
		freeSkirmishAIs.resize(MAX_AIS, 0);

		std::for_each(freeSkirmishAIs.begin(), freeSkirmishAIs.end(), [&](const uint8_t& id) { freeSkirmishAIs[&id - &freeSkirmishAIs[0]] = &id - &freeSkirmishAIs[0]; });
		std::reverse(freeSkirmishAIs.begin(), freeSkirmishAIs.end());

		for (const SkirmishAIData& skd: aiStartData) {
			const uint8_t skirmishAIId = ReserveSkirmishAIId();

			if (skirmishAIId == MAX_AIS) {
				Message(spring::format("Too many AIs (%d) specified in game-setup script", aiStartData.size()));
				break;
			}

			players[skd.hostPlayer].aiClientLinks[skirmishAIId] = {};
			skirmishAIs[skirmishAIId] = std::make_pair(true, skd);

			teams[skd.team].SetActive(true);

			if (!teams[skd.team].HasLeader())
				teams[skd.team].SetLeader(skd.hostPlayer);
		}
	}

	{
		// std::copy(SERVER_COMMANDS.begin(), SERVER_COMMANDS.end(), commandBlacklist.begin());
		std::sort(commandBlacklist.begin(), commandBlacklist.end());
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
	linkMinPacketSize = globalConfig.linkIncomingMaxPacketRate > 0 ? (globalConfig.linkIncomingSustainedBandwidth / globalConfig.linkIncomingMaxPacketRate) : 1;
	lastBandwidthUpdate = spring_gettime();

	thread = std::move(spring::thread(std::bind(&CGameServer::UpdateLoop, this)));

	// Something in CGameServer::CGameServer borks the FPU control word
	// maybe the threading, or something in CNet::InitServer() ??
	// Set single precision floating point math.
	streflop::streflop_init<streflop::Simple>();

	if (!demoReader) {
		GenerateAndSendGameID();
		rng.Seed(gameID.intArray[0] ^ gameID.intArray[1] ^ gameID.intArray[2] ^ gameID.intArray[3]);
		Broadcast(CBaseNetProtocol::Get().SendRandSeed(rng()));
	}
}

void CGameServer::PostLoad(int newServerFrameNum)
{
	std::lock_guard<spring::recursive_mutex> scoped_lock(gameServerMutex);
	serverFrameNum = newServerFrameNum;

	gameHasStarted = !PreSimFrame();

	// for all GameParticipant's
	for (GameParticipant& p: players) {
		p.lastFrameResponse = newServerFrameNum;
	}
}


void CGameServer::Reload(const std::shared_ptr<const CGameSetup> newGameSetup)
{
	const std::shared_ptr<const ClientSetup> clientSetup = gameServer->GetClientSetup();
	const std::shared_ptr<const    GameData>    gameData = gameServer->GetGameData();

	delete gameServer;

	// transfer ownership to new instance (assume only GameSetup changes)
	gameServer = new CGameServer(clientSetup, gameData, newGameSetup);
}


void CGameServer::WriteDemoData()
{
	if (demoRecorder == nullptr)
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
	for (size_t i = 0; i < skirmishAIs.size(); ++i) {
		if (!skirmishAIs[i].first)
			continue;

		demoRecorder->SetSkirmishAIStats(i, skirmishAIs[i].second.lastStats);
	}
	for (int i = 0; i < numTeams; ++i) {
		record->SetTeamStats(i, teamHandler.Team(i)->statHistory);
	}
	*/
}

void CGameServer::StripGameSetupText(const GameData* newGameData)
{
	// modify and save GameSetup text (remove passwords)
	TdfParser parser((newGameData->GetSetupText()).c_str(), (newGameData->GetSetupText()).length());
	TdfParser::TdfSection* rootSec = parser.GetRootSection()->sections["game"];

	for (const auto& item: rootSec->sections) {
		const std::string& sectionKey = StringToLower(item.first);

		if (!StringStartsWith(sectionKey, "player"))
			continue;

		TdfParser::TdfSection* playerSec = item.second;
		playerSec->remove("password", false);
	}

	std::ostringstream strbuf;
	parser.print(strbuf);

	GameData* modGameData = new GameData(*newGameData);

	modGameData->SetSetupText(strbuf.str());
	myGameData.reset(modGameData);
}


void CGameServer::AddLocalClient(const std::string& myName, const std::string& myVersion, const std::string& myPlatform)
{
	std::lock_guard<spring::recursive_mutex> scoped_lock(gameServerMutex);
	assert(!HasLocalClient());

	localClientNumber = BindConnection(std::shared_ptr<netcode::CConnection>(new netcode::CLocalConnection()), myName, "", myVersion, myPlatform, true);
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
	luaSocketRestrictions->addRule(CLuaSocketRestrictions::UDP_CONNECT, autohostIP, autohostPort, false);
#endif

	if (!hostif) {
		hostif.reset(new AutohostInterface(autohostIP, autohostPort));
		if (hostif->IsInitialized()) {
			hostif->SendStart();
			Message(spring::format(ConnectAutohost, autohostPort), false);
		} else {
			// Quit if we are instructed to communicate with an auto-host,
			// but are unable to do so: we do not want an auto-host running
			// a spring game that it has no control over. If we get here,
			// it suggests a configuration problem in the auto-host.
			hostif.reset();
			Message(spring::format(ConnectAutohostFailed, autohostIP.c_str(), autohostPort), false);
			quitServer = true;
		}
	}
}


void CGameServer::SkipTo(int targetFrameNum)
{
	const bool wasPaused = isPaused;

	if (!gameHasStarted) { return; }
	if (serverFrameNum >= targetFrameNum) { return; }
	if (demoReader == nullptr) { return; }

	CommandMessage startMsg(spring::format("skip start %d", targetFrameNum), SERVER_PLAYER);
	CommandMessage endMsg("skip end", SERVER_PLAYER);
	Broadcast(std::shared_ptr<const netcode::RawPacket>(startMsg.Pack()));

	// fast-read and send demo data
	//
	// note that we must maintain <modGameTime> ourselves
	// since we do we NOT go through ::Update when skipping
	while (SendDemoData(targetFrameNum)) {
		gameTime = GetDemoTime();
		modGameTime = demoReader->GetModGameTime() + 0.001f;

		if (UDPNet == nullptr) { continue; }
		if ((serverFrameNum % 20) != 0) { continue; }

		// send data every few frames, as otherwise packets would grow too big
		UDPNet->Update();
	}

	Broadcast(std::shared_ptr<const netcode::RawPacket>(endMsg.Pack()));

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
	netcode::RawPacket* buf = nullptr;

	// if we reached EOS before, demoReader has become NULL
	if (demoReader == nullptr)
		return ret;

	// get all packets from the stream up to <modGameTime>
	while ((buf = demoReader->GetData(modGameTime))) {
		std::shared_ptr<const RawPacket> rpkt(buf);

		if (buf->length <= 0) {
			Message("Warning: Discarding zero size packet in demo");
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
					Message(spring::format("Warning: Discarding invalid new player packet in demo: %s", ex.what()));
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
					Message(spring::format("Warning: Discarding invalid command message packet in demo: %s", ex.what()));
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

void CGameServer::Broadcast(std::shared_ptr<const netcode::RawPacket> packet)
{
	for (GameParticipant& p: players) {
		p.SendData(packet);
	}

	if (canReconnect || allowSpecJoin || !gameHasStarted)
		packetCache.push_back(packet);

	if (demoRecorder != nullptr)
		demoRecorder->SaveToDemo(packet->data, packet->length, GetDemoTime());
}

void CGameServer::Message(const std::string& message, bool broadcast, bool internal)
{
	if (!internal) {
		if (broadcast) {
			Broadcast(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
		}
		else if (HasLocalClient()) {
			// host should see
			players[localClientNumber].SendData(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, message));
		}
		if (hostif != nullptr)
			hostif->Message(message);
	}

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
	std::vector< std::pair<unsigned, unsigned> > checksums; // <response checkum, #clients matching checksum>
	std::vector<int> noSyncResponsePlayers;

	std::map<unsigned, std::vector<int> > desyncGroups; // <desync-checksum, [desynced players]>
	std::map<int, unsigned> desyncSpecs; // <playerNum, desync-checksum>

	auto outstandingSyncFrameIt = outstandingSyncFrames.begin();

	while (outstandingSyncFrameIt != outstandingSyncFrames.end()) {
		const unsigned outstandingSyncFrame = *outstandingSyncFrameIt;

		unsigned correctChecksum = 0;
		// maximum number of matched checksums
		unsigned maxChecksumCount = 0;

		bool haveCorrectChecksum = false;
		bool completeResponseSet =  true;


		if (HasLocalClient()) {
			// dictatorship; all player checksums must match the local client's for this frame
			const auto it = players[localClientNumber].syncResponse.find(outstandingSyncFrame);

			if (it != players[localClientNumber].syncResponse.end()) {
				correctChecksum = it->second;
				haveCorrectChecksum = true;
			}
		} else {
			// democracy; use the checksum that most players agree on as baseline
			checksums.clear();
			checksums.reserve(players.size());

			for (const GameParticipant& p: players) {
				if (p.clientLink == nullptr)
					continue;

				const auto pChecksumIt = p.syncResponse.find(outstandingSyncFrame);

				if (pChecksumIt == p.syncResponse.end())
					continue;

				const unsigned pChecksum = pChecksumIt->second;
				// const unsigned cChecksum = checksums[0].first;

				bool checksumFound = false;

				// compare player <p>'s sync-response checksum for the
				// outstanding frame to all others we have seen so far
				for (auto& checksumPair: checksums) {
					const unsigned  cChecksum      = checksumPair.first;
					      unsigned& cChecksumCount = checksumPair.second;

					if (cChecksum != pChecksum)
						continue;

					checksumFound = true;

					if (maxChecksumCount < (++cChecksumCount)) {
						maxChecksumCount = cChecksumCount;
						correctChecksum = cChecksum;
					}
				}

				if (checksumFound)
					continue;

				// first time we have seen this checksum
				checksums.emplace_back(pChecksum, 1);

				if (maxChecksumCount == 0) {
					maxChecksumCount = 1;
					correctChecksum = pChecksum;
				}
			}

			haveCorrectChecksum = (maxChecksumCount > 0);
		}


		noSyncResponsePlayers.clear();
		noSyncResponsePlayers.reserve(players.size());
		desyncGroups.clear();
		desyncSpecs.clear();

		for (GameParticipant& p: players) {
			if (p.clientLink == nullptr)
				continue;

			const auto pChecksumIt = p.syncResponse.find(outstandingSyncFrame);

			if (pChecksumIt == p.syncResponse.end()) {
				if (outstandingSyncFrame >= (serverFrameNum - static_cast<int>(SYNCCHECK_TIMEOUT)))
					completeResponseSet = false;
				else if (outstandingSyncFrame < p.lastFrameResponse)
					noSyncResponsePlayers.push_back(p.id);

				continue;
			}

			const unsigned pChecksum = pChecksumIt->second;

			if ((p.desynced = (haveCorrectChecksum && pChecksum != correctChecksum))) {
				if (demoReader || !p.spectator) {
					desyncGroups[pChecksum].push_back(p.id);
				} else {
					desyncSpecs[p.id] = pChecksum;
				}
			}
		}



		// warn about clients that failed to provide a sync-response checksum in time
		if (!noSyncResponsePlayers.empty()) {
			if (!syncWarningFrame || ((outstandingSyncFrame - syncWarningFrame) > static_cast<int>(SYNCCHECK_MSG_TIMEOUT))) {
				syncWarningFrame = outstandingSyncFrame;

				const std::string& playerNames = GetPlayerNames(noSyncResponsePlayers);
				Message(spring::format(NoSyncResponse, playerNames.c_str(), outstandingSyncFrame));
			}
		}



		// If anything's in it, we have a desync.
		// TODO take care of !completeResponseSet case?
		// Should we start resync then immediately or wait for the missing packets (while paused)?
		if (/*completeResponseSet && */ (!desyncGroups.empty() || !desyncSpecs.empty())) {
			if (syncErrorFrame == 0 || (outstandingSyncFrame - syncErrorFrame > static_cast<int>(SYNCCHECK_MSG_TIMEOUT))) {
				syncErrorFrame = outstandingSyncFrame;

			#ifdef SYNCDEBUG
				CSyncDebugger::GetInstance()->ServerTriggerSyncErrorHandling(serverFrameNum);

				if (demoReader) // pause is a synced message, thus demo spectators may not pause for real
					Message(spring::format("%s paused the demo", players[gu->myPlayerNum].name.c_str()));
				else
					Broadcast(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, true));

				isPaused = true;
				Broadcast(CBaseNetProtocol::Get().SendSdCheckrequest(serverFrameNum));
			#endif

				#ifndef DEDICATED
				// DS exit-codes are not used
				spring::exitCode = spring::EXIT_CODE_DESYNC;
				#endif

				// For each group, output a message with list of player names in it.
				// TODO this should be linked to the resync system so it can roundrobin
				// the resync checksum request packets to multiple clients in the same group.
				for (const auto& desyncGroup: desyncGroups) {
					const std::string& playerNames = GetPlayerNames(desyncGroup.second);
					Message(spring::format(SyncError, playerNames.c_str(), outstandingSyncFrame, desyncGroup.first, correctChecksum));
				}

				// send spectator desyncs as private messages to reduce spam
				for (const auto& p: desyncSpecs) {
					LOG_L(L_ERROR, "%s", spring::format(SyncError, players[p.first].name.c_str(), outstandingSyncFrame, p.second, correctChecksum).c_str());
					Message(spring::format(SyncError, players[p.first].name.c_str(), outstandingSyncFrame, p.second, correctChecksum));

					PrivateMessage(p.first, spring::format(SyncError, players[p.first].name.c_str(), outstandingSyncFrame, p.second, correctChecksum));
				}
			}
		}

		// Remove complete sets (for which all player's checksums have been received).
		if (completeResponseSet) {
			for (GameParticipant& p: players) {
				if (p.myState < GameParticipant::DISCONNECTED)
					p.syncResponse.erase(outstandingSyncFrame);
			}

			outstandingSyncFrameIt = outstandingSyncFrames.erase(outstandingSyncFrameIt);
			continue;
		}

		++outstandingSyncFrameIt;
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
		if (demoReader == nullptr || !HasLocalClient() || (serverFrameNum - players[localClientNumber].lastFrameResponse) < GAME_SPEED)
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
	else if (!PreSimFrame() || demoReader != nullptr)
		CreateNewFrame(true, false);

	if (hostif != nullptr) {
		const std::string msg = hostif->GetChatMessage();

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

	const bool pregameTimeoutReached = (spring_gettime() > (serverStartTime + spring_secs(globalConfig.initialNetworkTimeout)));
	const bool canCheckForPlayers = (pregameTimeoutReached || gameHasStarted);

	if (canCheckForPlayers) {
		bool hasPlayers = false;

		for (const GameParticipant& p: players) {
			if ((hasPlayers |= (p.clientLink != nullptr)))
				break;
		}

		if ((quitServer = (quitServer || !hasPlayers)))
			Message(NoClientsExit);
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
		float newSpeed = internalSpeed / refCpuUsage * wantedCpuUsage;

		newSpeed = Clamp(newSpeed, 0.1f, userSpeedFactor);
		//average to smooth the speed change over time to reduce the impact of cpu spikes in the players
		newSpeed = (newSpeed + internalSpeed) * 0.5f;

#ifndef DEDICATED
		// in non-dedicated hosting, we'll add an additional safeguard to make sure the host can keep up with the game's speed
		// adjust game speed to localclient's (:= host) maximum SimFrame rate
		const float invSimDrawFract = 1.0f - CGlobalUnsynced::reconnectSimDrawBalance;
		const float maxSimFrameRate = (1000.0f / gu->avgSimFrameTime) * invSimDrawFract;

		newSpeed = Clamp(newSpeed, 0.1f, ((maxSimFrameRate / GAME_SPEED) + internalSpeed) * 0.5f);
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

/// has to be consistent with Game.cpp/CSkirmishAIHandler (CSkirmishAIHandler::GetSkirmishAIsInTeam)
static std::vector<uint8_t> getSkirmishAIIds(
	const std::vector< std::pair<bool, GameSkirmishAI> >& skirmAIs,
	const std::vector<uint8_t>& freeAIs,
	const int teamId,
	const int hostPlayerId = -2
) {
	std::vector<uint8_t> ids;

	if (freeAIs.size() < MAX_AIS) {
		ids.reserve(MAX_AIS - freeAIs.size());

		for (const auto& p: skirmAIs) {
			const GameSkirmishAI& aiData = p.second;

			if (!p.first)
				continue;

			if (aiData.team != teamId)
				continue;
			if ((hostPlayerId >= 0) && (aiData.hostPlayer != hostPlayerId))
				continue;

			ids.push_back(&p - &skirmAIs[0]);
		}

		// not strictly necessary, only contents have to match client
		std::sort(ids.begin(), ids.end());
	}

	return ids;
}

/**
 * Duplicates functionality of CSkirmishAIHandler::GetSkirmishAIsInTeam(const int teamId)
 * as skirmishAIHandler is not available on the server
 */
static int countNumSkirmishAIsInTeam(
	const std::vector< std::pair<bool, GameSkirmishAI> >& skirmAIs,
	const std::vector<uint8_t>& freeAIs,
	const int teamId
) {
	return getSkirmishAIIds(skirmAIs, freeAIs, teamId).size();
}


void CGameServer::ProcessPacket(const unsigned playerNum, std::shared_ptr<const netcode::RawPacket> packet)
{
	const std::uint8_t* inbuf = packet->data;

	const unsigned a = playerNum;
	const unsigned msgCode = (unsigned) inbuf[0];

	switch (msgCode) {
		case NETMSG_KEYFRAME: {
			const int frameNum = *(int*) &inbuf[1];

			if (frameNum <= serverFrameNum && frameNum > players[a].lastFrameResponse)
				players[a].lastFrameResponse = frameNum;
			break;
		}

		case NETMSG_PING: {
			if (inbuf[1] != playerNum) {
				Message(spring::format(WrongPlayer, msgCode, playerNum, (unsigned)inbuf[1]));
				break;
			}

			// limit to 50 pings per second
			if (spring_diffmsecs(spring_now(), pingTimeFilter[playerNum]) >= 20) {
				players[playerNum].SendData(CBaseNetProtocol::Get().SendPing(playerNum, inbuf[2], *(reinterpret_cast<const float*>(&inbuf[3]))));
				pingTimeFilter[playerNum] = spring_now();
			}
		} break;

		case NETMSG_PAUSE:
			if (inbuf[1] != a) {
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)inbuf[1]));
				break;
			}
			if (!inbuf[2])  // reset sync checker
				syncErrorFrame = 0;
			if (gamePausable || players[a].isLocal) { // allow host to pause even if nopause is set
				if (!players[a].isLocal && players[a].spectator && demoReader == nullptr) {
					PrivateMessage(a, "Spectators cannot pause the game");
				} else {
					frameTimeLeft = 0.0f;

					if ((isPaused != !!inbuf[2]) || demoReader)
						isPaused = !isPaused;
					if (demoReader) // pause is a synced message, thus demo spectators may not pause for real
						Message(spring::format("%s %s the demo", players[a].name.c_str(), (isPaused ? "paused" : "unpaused")));
					else
						Broadcast(CBaseNetProtocol::Get().SendPause(a, inbuf[2]));
				}
			}
			break;

		case NETMSG_USER_SPEED: {
			if (!players[a].isLocal && players[a].spectator && demoReader == nullptr) {
				PrivateMessage(a, "Spectators cannot change game speed");
			} else {
				UserSpeedChange(*((float*) &inbuf[2]), a);
			}
		} break;

		case NETMSG_CPU_USAGE:
			players[a].cpuUsage = *((float*) &inbuf[1]);
			break;

		case NETMSG_QUIT: {
			Message(spring::format(PlayerLeft, players[a].GetType(), players[a].name.c_str(), " normal quit"));
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 1));
			players[a].Kill("[GameServer] user exited", true);
			if (hostif != nullptr)
				hostif->SendPlayerLeft(a, 1);
			break;
		}

		case NETMSG_PLAYERNAME: {
			try {
				netcode::UnpackPacket pckt(packet, 2);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(spring::format(WrongPlayer, msgCode, a, playerNum));
					break;
				}
				pckt >> players[playerNum].name;
				players[playerNum].myState = GameParticipant::INGAME;
				Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, 0, 0)); // reset pathing display
				Message(spring::format(PlayerJoined, players[playerNum].GetType(), players[playerNum].name.c_str()), false);
				Broadcast(CBaseNetProtocol::Get().SendPlayerName(playerNum, players[playerNum].name));
				if (hostif != nullptr)
					hostif->SendPlayerJoined(playerNum, players[playerNum].name);
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %d sent invalid PlayerName: %s", a, ex.what()));
			}
			break;
		}

		case NETMSG_PATH_CHECKSUM: {
			const unsigned char playerNum = inbuf[1];
			const std::uint32_t playerCheckSum = *(std::uint32_t*) &inbuf[2];
			if (playerNum != a) {
				Message(spring::format(WrongPlayer, msgCode, a, playerNum));
				break;
			}
			Broadcast(CBaseNetProtocol::Get().SendPathCheckSum(playerNum, playerCheckSum));
		} break;

		case NETMSG_CHAT: {
			try {
				ChatMessage msg(packet);
				if (static_cast<unsigned>(msg.fromPlayer) != a) {
					Message(spring::format(WrongPlayer, msgCode, a, (unsigned)msg.fromPlayer));
					break;
				}
				// if this player is chat-muted, drop his messages quietly
				if (clientMuteFilter[a].first)
					break;

				GotChatMessage(msg);
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %s sent invalid ChatMessage: %s", players[a].name.c_str(), ex.what()));
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
					Message(spring::format(WrongPlayer, msgCode, a, (unsigned)playerNum));
					break;
				}
				Broadcast(CBaseNetProtocol::Get().SendSystemMessage(playerNum, strmsg));
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %d sent invalid SystemMessage: %s", a, ex.what()));
			}
			break;

		case NETMSG_STARTPOS: {
			const unsigned char player = inbuf[1];
			const unsigned int team = (unsigned)inbuf[2];
			const unsigned char rdyState = inbuf[3];

			if (player != a) {
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)inbuf[1]));
				break;
			}
			if (myGameSetup->startPosType == CGameSetup::StartPos_ChooseInGame) {
				if (team >= teams.size()) {
					Message(spring::format("Invalid teamID %d in NETMSG_STARTPOS from player %d", team, player));
				} else if (getSkirmishAIIds(skirmishAIs, freeSkirmishAIs, team, player).empty() && ((team != players[player].team) || (players[player].spectator))) {
					Message(spring::format("Player %d sent spoofed NETMSG_STARTPOS with teamID %d", player, team));
				} else {
					teams[team].SetStartPos(float3(*((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12])));
					players[player].SetReadyToStart(rdyState != CPlayer::PLAYER_RDYSTATE_UPDATED);

					Broadcast(CBaseNetProtocol::Get().SendStartPos(player, team, rdyState, *((float*)&inbuf[4]), *((float*)&inbuf[8]), *((float*)&inbuf[12])));

					if (hostif != nullptr)
						hostif->SendPlayerReady(a, rdyState);
				}
			} else {
				Message(spring::format(NoStartposChange, a));
			}
			break;
		}

		case NETMSG_COMMAND:
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(spring::format(WrongPlayer, msgCode, a, (unsigned)playerNum));
					break;
				}

				#ifndef ALLOW_DEMO_GODMODE
				if (demoReader == nullptr)
				#endif
				{
					Broadcast(packet); //forward data
				}
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %s sent invalid Command: %s", players[a].name.c_str(), ex.what()));
			}
			break;

		case NETMSG_SELECT:
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(spring::format(WrongPlayer, msgCode, a, (unsigned)playerNum));
					break;
				}

				#ifndef ALLOW_DEMO_GODMODE
				if (demoReader == nullptr)
				#endif
				{
					Broadcast(packet); //forward data
				}
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %s sent invalid Select: %s", players[a].name.c_str(), ex.what()));
			}
			break;

		case NETMSG_AICOMMAND: {
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(spring::format(WrongPlayer, msgCode , a , (unsigned) playerNum));
					break;
				}

				if (noHelperAIs)
					Message(spring::format(NoHelperAI, players[a].name.c_str(), a));
				else if (demoReader == nullptr)
					Broadcast(packet); //forward data
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %s sent invalid AICommand: %s", players[a].name.c_str(), ex.what()));
			}
		}
		break;

		case NETMSG_AICOMMANDS: {
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;

				if (playerNum != a) {
					Message(spring::format(WrongPlayer, msgCode , a , (unsigned) playerNum));
					break;
				}

				if (noHelperAIs)
					Message(spring::format(NoHelperAI, players[a].name.c_str(), a));
				else if (demoReader == nullptr)
					Broadcast(packet); //forward data
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %s sent invalid AICommands: %s", players[a].name.c_str(), ex.what()));
			}
		} break;

		case NETMSG_AISHARE: {
			try {
				netcode::UnpackPacket pckt(packet, 3);
				unsigned char playerNum;
				pckt >> playerNum;
				if (playerNum != a) {
					Message(spring::format(WrongPlayer, msgCode , a , (unsigned) playerNum));
					break;
				}
				if (noHelperAIs)
					Message(spring::format(NoHelperAI, players[a].name.c_str(), a));
				else if (demoReader == nullptr)
					Broadcast(packet); //forward data
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %s sent invalid AIShare: %s", players[a].name.c_str(), ex.what()));
			}
		} break;


		case NETMSG_LOGMSG: {
			try {
				netcode::UnpackPacket pckt(packet, sizeof(uint8_t) + sizeof(uint16_t));
				uint8_t playerNum;

				pckt >> playerNum;

				if (playerNum != a) {
					Message(spring::format(WrongPlayer, msgCode, a, (unsigned)playerNum));
					break;
				}

				Broadcast(packet);
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("[GameServer::%s][NETMSG_LOGMSG] exception \"%s\" from player \"%s\"", ex.what(), players[a].name.c_str()));
			}
		} break;
		case NETMSG_LUAMSG: {
			try {
				netcode::UnpackPacket pckt(packet, sizeof(uint8_t) + sizeof(uint16_t));
				uint8_t playerNum;

				pckt >> playerNum;

				if (playerNum != a) {
					Message(spring::format(WrongPlayer, msgCode, a, (unsigned)playerNum));
					break;
				}

				if (demoReader != nullptr)
					break;

				Broadcast(packet);

				if (hostif != nullptr)
					hostif->SendLuaMsg(packet->data, packet->length);

			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("[GameServer::%s][NETMSG_LUAMSG] exception \"%s\" from player \"%s\"", ex.what(), players[a].name.c_str()));
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
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)inbuf[1]));
				break;
			}
			if (demoReader == nullptr)
				Broadcast(CBaseNetProtocol::Get().SendShare(inbuf[1], inbuf[2], inbuf[3], *((float*)&inbuf[4]), *((float*)&inbuf[8])));
			break;

		case NETMSG_SETSHARE:
			if (inbuf[1] != a) {
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)inbuf[1]));
				break;
			}
			if (demoReader == nullptr)
				Broadcast(CBaseNetProtocol::Get().SendSetShare(inbuf[1], inbuf[2], *((float*)&inbuf[3]), *((float*)&inbuf[7])));
			break;

		case NETMSG_PLAYERSTAT:
			if (inbuf[1] != a) {
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)inbuf[1]));
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
					Message(spring::format(WrongPlayer, msgCode, a, (unsigned)playerNum));
					break;
				}
				// if this player is draw-muted, drop his messages quietly
				if (clientMuteFilter[a].second)
					break;


				// also drop player commands if we received 25 or more and
				// each followed the previous by less than 50 milliseconds
				// this is impossible to reach manually, but (very) easily
				// through Lua and would allow clients to be DOS'ed
				clientDrawFilter[a].second += (spring_diffmsecs(spring_now(), clientDrawFilter[a].first) < 50);
				clientDrawFilter[a].second *= (spring_diffmsecs(spring_now(), clientDrawFilter[a].first) < 50);
				clientDrawFilter[a].first   = spring_now();

				if (clientDrawFilter[a].second > 25)
					break;


				if (!players[playerNum].spectator || allowSpecDraw)
					Broadcast(packet); //forward data
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %s sent invalid MapDraw: %s", players[a].name.c_str(), ex.what()));
			}
			break;

		case NETMSG_DIRECT_CONTROL:
			if (inbuf[1] != a) {
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)inbuf[1]));
				break;
			}
			if (demoReader == nullptr) {
				if (!players[inbuf[1]].spectator)
					Broadcast(CBaseNetProtocol::Get().SendDirectControl(inbuf[1]));
				else
					Message(spring::format("Error: spectator %s tried direct-controlling a unit", players[inbuf[1]].name.c_str()));
			}
			break;

		case NETMSG_DC_UPDATE:
			if (inbuf[1] != a) {
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)inbuf[1]));
				break;
			}
			if (demoReader == nullptr)
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
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)player));
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
						Message(spring::format("Invalid teamID %d in TEAMMSG_GIVEAWAY from player %d", toTeam, player));
						break;
					}
					if (giverTeam >= teams.size()) {
						Message(spring::format("Invalid teamID %d in TEAMMSG_GIVEAWAY from player %d", giverTeam, player));
						break;
					}

					const std::vector<uint8_t>& giverTeamAIs       = getSkirmishAIIds(skirmishAIs, freeSkirmishAIs, giverTeam);
					const std::vector<uint8_t>& giverTeamPlayerAIs = getSkirmishAIIds(skirmishAIs, freeSkirmishAIs, giverTeam, player);

					const size_t numPlayersInGiverTeam       = countNumPlayersInTeam(players, giverTeam);
					const size_t numControllersInGiverTeam   = numPlayersInGiverTeam + giverTeamAIs.size();

					const bool isGiverLeader                 = (teams[giverTeam].GetLeader() == player);
					const bool isGiverOwnTeam                = (giverTeam == fromTeam);
					const bool isSpec                        = players[player].spectator;
					const bool giverHasAIs                   = (!giverTeamPlayerAIs.empty());
					const bool giverIsAllied                 = (teams[giverTeam].teamAllyteam == teams[fromTeam].teamAllyteam);
					const bool isSinglePlayer                = (players.size() <= 1);
					const bool giveAwayOk                    = (isGiverOwnTeam || numPlayersInGiverTeam == 0);

					const char* playerName                   = players[player].name.c_str();
					const char* playerType                   = players[player].GetType();

					if (!isSinglePlayer &&
						(isSpec || (!isGiverOwnTeam && !isGiverLeader) ||
						(giverHasAIs && !giverIsAllied && !cheating))) {
							Message(spring::format("%s %s sent invalid team giveaway", playerType, playerName), true);
							break;
					}

					Broadcast(CBaseNetProtocol::Get().SendGiveAwayEverything(player, toTeam, giverTeam));

					if (isGiverOwnTeam) {
						// player is giving stuff from his own team
						//players[player].team = 0;
						players[player].spectator = true;

						if (hostif != nullptr)
							hostif->SendPlayerDefeated(player);
					} else {
						// player is giving stuff from one of his AI teams
						if (numPlayersInGiverTeam == 0) {
							// kill the first AI
							skirmishAIs[ giverTeamPlayerAIs[0] ] = std::make_pair(false, GameSkirmishAI{});
							freeSkirmishAIs.push_back(giverTeamPlayerAIs[0]);
						} else {
							Message(spring::format("%s %s can not give away stuff of team %i (still has human players left)", playerType, playerName, giverTeam), true);
						}
					}

					if (giveAwayOk && (numControllersInGiverTeam == 1)) {
						// team has no controller left now
						teams[giverTeam].SetActive(false);
						teams[giverTeam].SetLeader(-1);

						const int leadPlayer = teams[toTeam].GetLeader();

						const std::string  giverName = players[player].name;
						const std::string& recipName = (leadPlayer >= 0) ? players[leadPlayer].name : UncontrolledPlayerName;

						Broadcast(CBaseNetProtocol::Get().SendSystemMessage(SERVER_PLAYER, giverName + " gave everything to " + recipName));
					}
					break;
				}
				case TEAMMSG_RESIGN: {
					const bool isSpec         = players[player].spectator;
					const bool isSinglePlayer = (players.size() <= 1);

					if (isSpec && !isSinglePlayer) {
						Message(spring::format("Spectator %s sent invalid team resign", players[player].name.c_str()), true);
						break;
					}

					ResignPlayer(player);
					break;
				}
				case TEAMMSG_JOIN_TEAM: {
					const unsigned newTeamID = inbuf[3];

					const bool isNewTeamValid = (newTeamID < teams.size());
					const bool isSinglePlayer = (players.size() <= 1);

					if (!isNewTeamValid || (!isSinglePlayer && !cheating)) {
						Message(spring::format(NoTeamChange, players[player].name.c_str(), player, newTeamID));
						break;
					}

					// player can join this team
					Broadcast(CBaseNetProtocol::Get().SendJoinTeam(player, newTeamID));

					players[player].team      = newTeamID;
					players[player].spectator = false;

					if (!teams[newTeamID].HasLeader())
						teams[newTeamID].SetLeader(player);

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
							Message(spring::format("Invalid teamID %d in TEAMMSG_TEAM_DIED from player %d", teamID, player));
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

								if (hostif != nullptr)
									hostif->SendPlayerDefeated(p);

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
					Message(spring::format(UnknownTeammsg, action, player));
				}
			}
			break;
		}

		case NETMSG_AI_CREATED: {
			try {
				// client issued aicontrol command
				netcode::UnpackPacket pckt(packet, 2);
				std::string aiName;

				uint8_t playerId;
				uint8_t skirmishAIId;
				uint8_t aiTeamId;

				pckt >> playerId;

				if (playerId != a) {
					Message(spring::format(WrongPlayer, msgCode, a, (unsigned)playerId));
					break;
				}

				pckt >> skirmishAIId;
				pckt >> aiTeamId;
				pckt >> aiName;

				if (aiTeamId >= teams.size()) {
					Message(spring::format("[GameServer::%s][NETMSG_AI_CREATED] invalid teamID %d from player %d", __func__, unsigned(aiTeamId), unsigned(playerId)));
					break;
				}

				const uint8_t playerTeamId = players[playerId].team;

				GameTeam* tpl           = &teams[playerTeamId];
				GameTeam* tai           = &teams[aiTeamId];

				const bool weAreLeader  = (tai->GetLeader() == playerId);
				const bool weAreAllied  = (tpl->teamAllyteam == tai->teamAllyteam);
				const bool singlePlayer = (players.size() <= 1);

				if (!(weAreLeader || singlePlayer || (weAreAllied && (cheating || !tai->HasLeader())))) {
					Message(spring::format(NoAICreated, players[playerId].name.c_str(), (int)playerId, (int)aiTeamId));
					break;
				}

				// discard bogus ID from message, reserve actual slot here
				if ((skirmishAIId = ReserveSkirmishAIId()) == MAX_AIS) {
					Message(spring::format("[GameServer::%s][NETMSG_AI_CREATED] unable to create AI, limit reached (%d)", __func__, (int)MAX_AIS));
					break;
				}

				players[playerId].aiClientLinks[skirmishAIId] = {};

				skirmishAIs[skirmishAIId].first = true;
				skirmishAIs[skirmishAIId].second.team = aiTeamId;
				skirmishAIs[skirmishAIId].second.name = aiName;
				skirmishAIs[skirmishAIId].second.hostPlayer = playerId;

				// bounce back, sender will do local creation
				Broadcast(CBaseNetProtocol::Get().SendAICreated(playerId, skirmishAIId, aiTeamId, aiName));

				if (!tai->HasLeader()) {
					tai->SetLeader(playerId);
					tai->SetActive(true);
				}
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("[GameServer::%s][NETMSG_AI_CREATED] exception \"%s\" parsing message from player %s", ex.what(), players[a].name.c_str()));
			}
			break;
		}
		case NETMSG_AI_STATE_CHANGED: {
			const uint8_t playerId     = inbuf[1];
			const uint8_t skirmishAIId = inbuf[2];

			if (playerId != a) {
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)playerId));
				break;
			}

			const ESkirmishAIStatus newState = (ESkirmishAIStatus) inbuf[3];
			const ESkirmishAIStatus oldState = skirmishAIs[skirmishAIId].second.status;

			if (!skirmishAIs[skirmishAIId].first) {
				Message(spring::format(NoAIChangeState, players[playerId].name.c_str(), (int)playerId, skirmishAIId, (-1), (int)newState));
				break;
			}

			const uint8_t aiTeamId          = skirmishAIs[skirmishAIId].second.team;
			const uint8_t playerTeamId      = players[playerId].team;

			const size_t numPlayersInAITeam = countNumPlayersInTeam(players, aiTeamId);
			const size_t numAIsInAITeam     = countNumSkirmishAIsInTeam(skirmishAIs, freeSkirmishAIs, aiTeamId);

			GameTeam* tpl                   = &teams[playerTeamId];
			GameTeam* tai                   = &teams[aiTeamId];

			const bool weAreAIHost          = (skirmishAIs[skirmishAIId].second.hostPlayer == playerId);
			const bool weAreLeader          = (tai->GetLeader() == playerId);
			const bool weAreAllied          = (tpl->teamAllyteam == tai->teamAllyteam);
			const bool singlePlayer         = (players.size() <= 1);

			if (!(weAreAIHost || weAreLeader || singlePlayer || (weAreAllied && cheating))) {
				Message(spring::format(NoAIChangeState, players[playerId].name.c_str(), (int)playerId, skirmishAIId, (int)aiTeamId, (int)newState));
				break;
			}
			Broadcast(packet); // forward data

			// skip resetting management state for reloading AI's; will be reinitialized instantly
			if ((skirmishAIs[skirmishAIId].second.status = newState) == SKIRMAISTATE_DEAD && oldState != SKIRMAISTATE_RELOADING) {
				skirmishAIs[skirmishAIId] = std::make_pair(false, GameSkirmishAI{});

				freeSkirmishAIs.push_back(skirmishAIId);
				players[playerId].aiClientLinks.erase(skirmishAIId);

				if ((numPlayersInAITeam + numAIsInAITeam) == 1) {
					// team has no controller left now
					tai->SetActive(false);
					tai->SetLeader(-1);
				}
			}
			break;
		}

		case NETMSG_ALLIANCE: {
			const unsigned char player = inbuf[1];
			const int whichAllyTeam = inbuf[2];
			const unsigned char allied = inbuf[3];

			if (player != a) {
				Message(spring::format(WrongPlayer, msgCode, a, (unsigned)player));
				break;
			}

			if (whichAllyTeam == teams[players[a].team].teamAllyteam) {
				Message(spring::format("Player %s tried to send spoofed alliance message", players[a].name.c_str()));
			} else {
				if (!myGameSetup->fixedAllies)
					Broadcast(CBaseNetProtocol::Get().SendSetAllied(player, whichAllyTeam, allied));
			}
			break;
		}
		case NETMSG_CCOMMAND: {
			try {
				CommandMessage msg(packet);

				if (static_cast<unsigned>(msg.GetPlayerID()) == a) {
					const bool serverCommand = IsServerCommand(msg.GetAction().command);

					if (serverCommand && players[a].isLocal) {
						// command is restricted to server but player is allowed to execute it
						PushAction(msg.GetAction(), false);
					}
					else if (!serverCommand) {
						// command is safe
						Broadcast(packet);
					}
					else {
						// hack!
						Message(spring::format(CommandNotAllowed, msg.GetPlayerID(), msg.GetAction().command.c_str()));
					}
				}
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %s sent invalid CommandMessage: %s", players[a].name.c_str(), ex.what()));
			}
			break;
		}

		case NETMSG_TEAMSTAT: {
			if (hostif != nullptr)
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
					Message(spring::format(WrongPlayer, msgCode, a, (unsigned)playerNum));
					break;
				}

				if (totalSize > fixedSize) {
					// if empty, the game has no defined winner(s)
					winningAllyTeams.resize(totalSize - fixedSize);
					pckt >> winningAllyTeams;
				}

				if (hostif != nullptr)
					hostif->SendGameOver(playerNum, winningAllyTeams);
				Broadcast(CBaseNetProtocol::Get().SendGameOver(playerNum, winningAllyTeams));

				gameEndTime = spring_gettime();
			} catch (const netcode::UnpackPacketException& ex) {
				Message(spring::format("Player %s sent invalid GameOver: %s", players[a].name.c_str(), ex.what()));
			}
			break;
		}

		case NETMSG_CLIENTDATA: {
			Broadcast(packet);
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
			Message(spring::format(UnknownNetmsg, msgCode, a));
		}
		break;
	}
}


void CGameServer::HandleConnectionAttempts()
{
	while (UDPNet != nullptr && UDPNet->HasIncomingConnections()) {
		std::shared_ptr<netcode::UDPConnection> prev = UDPNet->PreviewConnection().lock();
		std::shared_ptr<const RawPacket> packet = prev->GetData();

		if (packet == nullptr) {
			UDPNet->RejectConnection();
			continue;
		}

		try {
			if (packet->length < 3) {
				std::string pkts;
				for (int i = 0; i < packet->length; ++i) {
					pkts += spring::format(" 0x%x", (int)packet->data[i]);
				}
				throw netcode::UnpackPacketException("Packet too short (data: " + pkts + ")");
			}

			if (packet->data[0] != NETMSG_ATTEMPTCONNECT)
				throw netcode::UnpackPacketException("Invalid message ID");

			netcode::UnpackPacket msg(packet, 3);
			std::string name;
			std::string passwd;
			std::string version;
			std::string platform;
			uint8_t reconnect;
			uint8_t netloss;
			uint16_t netversion;
			msg >> netversion;
			msg >> name;
			msg >> passwd;
			msg >> version;
			msg >> platform;
			msg >> reconnect;
			msg >> netloss;

			if (netversion != NETWORK_VERSION)
				throw netcode::UnpackPacketException(spring::format("Wrong network version: received %d, required %d", (int)netversion, (int)NETWORK_VERSION));

			BindConnection(UDPNet->AcceptConnection(), name, passwd, version, platform, false, reconnect, netloss);
		} catch (const netcode::UnpackPacketException& ex) {
			const asio::ip::udp::endpoint endp = prev->GetEndpoint();
			const asio::ip::address addr = endp.address();

			const std::string str = addr.to_string();
			const std::string msg = spring::format(ConnectionReject, str.c_str(), ex.what());

			auto  pair = std::make_pair(rejectedConnections.find(str), false);
			auto& iter = pair.first;

			if (iter == rejectedConnections.end()) {
				pair = rejectedConnections.insert(std::make_pair(str, 0));
				iter = pair.first;
			}

			if (iter->second < 5) {
				rejectedConnections.insert(std::make_pair(str, iter->second + 1));

				prev->Unmute();
				prev->SendData(CBaseNetProtocol::Get().SendRejectConnect(msg));
				prev->Flush(true);

				Message(msg);
			} else {
				// silently drop
				Message(msg, false, true);
			}

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
		std::shared_ptr<netcode::CConnection>& playerLink = player.clientLink;
		std::shared_ptr<const RawPacket> packet;
		spring::unordered_map<uint8_t, GameParticipant::ClientLinkData>& aiClientLinks = player.aiClientLinks;

		// if no link, player is not connected
		if (playerLink == nullptr)
			continue;

		if (playerLink->CheckTimeout(0, !gameHasStarted)) {
			// this must happen BEFORE the reset!
			Message(spring::format(PlayerLeft, player.GetType(), player.name.c_str(), " timeout"));
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(player.id, 0));

			player.Kill("User timeout");

			if (hostif != nullptr)
				hostif->SendPlayerLeft(player.id, 0);

			continue;
		}

		// relay all packets to separate connections for player and AIs
		while ((packet = playerLink->GetData()) != nullptr) {
			uint8_t aiID = MAX_AIS;
			int cmdID = -1;

			if (packet->length >= 5) {
				cmdID = packet->data[0];

				if (cmdID == NETMSG_AICOMMAND || cmdID == NETMSG_AICOMMAND_TRACKED || cmdID == NETMSG_AICOMMANDS || cmdID == NETMSG_AISHARE)
					aiID = packet->data[4];
			}

			const auto liit = aiClientLinks.find(aiID);

			if (liit != aiClientLinks.end()) {
				liit->second.link->SendData(packet);
			} else {
				// unreachable, aiClientLinks always contains a loopback entry for id=MAX_AIS
				Message(spring::format("Player %s sent invalid SkirmishAI ID %d in AICOMMAND %d", player.name.c_str(), (int)aiID, cmdID));
			}
		}

		for (auto& aiLinkData: aiClientLinks) {
			int  bandwidthUsage = aiLinkData.second.bandwidthUsage;
			int& numPacketsSent = aiLinkData.second.numPacketsSent;

			int numPktsDropped = 0;
			int peekAheadindex = 0;

			const bool bwLimitWasReached = (globalConfig.linkIncomingPeakBandwidth > 0 && bandwidthUsage > globalConfig.linkIncomingPeakBandwidth);

			if (updateBandwidth >= 1.0f && globalConfig.linkIncomingSustainedBandwidth > 0)
				bandwidthUsage = std::max(0, bandwidthUsage - std::max(1, (int)((float)globalConfig.linkIncomingSustainedBandwidth / (1000.0f / (playerBandwidthInterval * updateBandwidth)))));

			bool bwLimitIsReached  = (globalConfig.linkIncomingPeakBandwidth > 0 && bandwidthUsage > globalConfig.linkIncomingPeakBandwidth);
			bool dropPacket = globalConfig.linkIncomingMaxWaitingPackets > 0 && (globalConfig.linkIncomingPeakBandwidth <= 0 || bwLimitWasReached);

			std::shared_ptr<netcode::CConnection>& aiLink = aiLinkData.second.link;
			std::shared_ptr<const RawPacket> aiPacket;

			while (aiLink != nullptr) {
				if (dropPacket)
					dropPacket = ((aiPacket = aiLink->Peek(globalConfig.linkIncomingMaxWaitingPackets)) != nullptr);

				// if packet is to be dropped, just pull it from queue instead of peek
				if (!bwLimitIsReached || dropPacket)
					numPacketsSent += ((aiPacket = aiLink->GetData()) != nullptr);
				else
					aiPacket = aiLink->Peek(peekAheadindex++);

				if (aiPacket == nullptr)
					break;

				const bool droppablePacket = (aiPacket->length <= 0 || (aiPacket->data[0] != NETMSG_SYNCRESPONSE && aiPacket->data[0] != NETMSG_KEYFRAME));

				if (dropPacket && droppablePacket) {
					++numPktsDropped;
					continue;
				}

				if (bwLimitIsReached && droppablePacket)
					continue;

				// non-droppable packets may be processed more than once, but this does no harm
				ProcessPacket(player.id, aiPacket);

				if (globalConfig.linkIncomingPeakBandwidth > 0 && droppablePacket) {
					bandwidthUsage += std::max((unsigned)linkMinPacketSize, aiPacket->length);

					if (!bwLimitIsReached)
						bwLimitIsReached = (bandwidthUsage > globalConfig.linkIncomingPeakBandwidth);
				}
			}

			if (numPktsDropped > 0) {
				if (aiLinkData.first == MAX_AIS)
					PrivateMessage(player.id, spring::format("Warning: Waiting packet limit was reached for %s [%d packets dropped, %d sent]", player.name.c_str(), numPktsDropped, numPacketsSent));
				else
					PrivateMessage(player.id, spring::format("Warning: Waiting packet limit was reached for %s AI %d [%d packets dropped, %d sent]", player.name.c_str(), (int)aiLinkData.first, numPktsDropped, numPacketsSent));
			}

			if (!bwLimitWasReached && bwLimitIsReached) {
				if (aiLinkData.first == MAX_AIS)
					PrivateMessage(player.id, spring::format("Warning: Bandwidth limit was reached for %s [packets delayed, %d sent]", player.name.c_str(), numPacketsSent));
				else
					PrivateMessage(player.id, spring::format("Warning: Bandwidth limit was reached for %s AI %d [packets delayed, %d sent]", player.name.c_str(), (int)aiLinkData.first, numPacketsSent));
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
	gameID.intArray[0] = (unsigned) time(nullptr);
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

	if (demoRecorder != nullptr) {
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
		} else if (!players[a].spectator && teams[players[a].team].IsActive() && !players[a].IsReadyToStart() && demoReader == nullptr) {
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
			Broadcast(CBaseNetProtocol::Get().SendStartPlaying(std::max(std::int64_t(1), spring_tomsecs(gameStartDelay))));

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

			if (false && !teams[team].HasLeader()) // NOLINT{readability-simplify-boolean-expr}
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

	if (hostif != nullptr) {
		if (demoRecorder != nullptr) {
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
		Broadcast(std::shared_ptr<const RawPacket>(msg.Pack()));
	}
	else if (action.command == "nospecdraw") {
		InverseOrSetBool(allowSpecDraw, action.extra, true);
		// sent it because clients have to do stuff when this changes
		CommandMessage msg(action, SERVER_PLAYER);
		Broadcast(std::shared_ptr<const RawPacket>(msg.Pack()));
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
		Broadcast(std::shared_ptr<const RawPacket>(msg.Pack()));
	}
	else if (action.command == "singlestep") {
		if (isPaused) {
			if (demoReader != nullptr) {
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
				if (tokens.size() > 2) {
					spectator = (tokens[2] != "0");
				}
				if (tokens.size() > 3) {
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
		LOG("Server killed!");
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
		Broadcast(std::shared_ptr<const RawPacket>(msg.Pack()));
	}
}

bool CGameServer::HasFinished() const
{
	return quitServer;
}

void CGameServer::CreateNewFrame(bool fromServerThread, bool fixedFrameTime)
{
	if (demoReader != nullptr) {
		CheckSync();
		SendDemoData(-1);
		return;
	}

	std::unique_lock<spring::recursive_mutex> lck(gameServerMutex, std::defer_lock);
	if (!fromServerThread)
		lck.lock();

	CheckSync();
#ifndef DEDICATED
	const bool vidRecording = videoCapturing->AllowRecord();
#else
	const bool vidRecording = false;
#endif
	const bool normalFrame = !isPaused && !vidRecording;
	const bool videoFrame = !isPaused && fixedFrameTime;
	const bool singleStep = fixedFrameTime && !vidRecording;

	unsigned int numNewFrames = 1;

	if (logDebugMessages) {
		LOG_L(
			L_INFO, // L_DEBUG only works in DEBUG builds which are slow and affect timings
			"[%s][1][sf=%d] fromServerThread=%d fixedFrameTime=%d hasLocalClient=%d normalFrame=%d",
			__func__, serverFrameNum, fromServerThread, fixedFrameTime, HasLocalClient(), normalFrame
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
		assert(demoReader == nullptr);

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
		Message(spring::format("Server speed control: %s", (SpeedControlToString(speedCtrl).c_str())));
		curSpeedCtrl = speedCtrl;
	}
}


std::string CGameServer::SpeedControlToString(int speedCtrl)
{
	std::string desc = "<invalid>";
	if (speedCtrl == 0) {
		desc = "Maximum CPU";
	} else if (speedCtrl == 1) {
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
			spring_msecs(loopSleepTime).sleep(true);

			if (UDPNet != nullptr)
				UDPNet->Update();

			std::lock_guard<spring::recursive_mutex> scoped_lock(gameServerMutex);
			ServerReadNet();
			Update();
		}

		if (hostif != nullptr)
			hostif->SendQuit();

		Broadcast(CBaseNetProtocol::Get().SendQuit("Server shutdown"));

		// this is to make sure the Flush has any effect at all (we don't want a forced flush)
		// when reloading, we can assume there is only a local client and skip the sleep()'s
		if (!reloadingServer && !myGameSetup->onlyLocal)
			spring_sleep(spring_msecs(500));

		// flush the quit messages to reduce ugly network error messages on the client side
		for (GameParticipant& p: players) {
			if (p.clientLink != nullptr)
				p.clientLink->Flush();
		}

		// now let clients close their connections
		if (!reloadingServer && !myGameSetup->onlyLocal)
			spring_sleep(spring_msecs(1500));

	} CATCH_SPRING_ERRORS
}


void CGameServer::KickPlayer(const int playerNum)
{
	// only kick connected players
	if (players[playerNum].clientLink == nullptr) {
		Message(spring::format("Attempt to kick user %d who is not connected", playerNum));
		return;
	}

	Message(spring::format(PlayerLeft, players[playerNum].GetType(), players[playerNum].name.c_str(), "kicked"));
	Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(playerNum, 2));

	players[playerNum].Kill("Kicked from the battle", true);

	if (hostif != nullptr)
		hostif->SendPlayerLeft(playerNum, 2);
}


void CGameServer::MutePlayer(const int playerNum, bool muteChat, bool muteDraw)
{
	if (playerNum >= players.size()) {
		LOG_L(L_WARNING, "MutePlayer: invalid playerNum");
		return;
	}

	clientMuteFilter[playerNum].first  = muteChat;
	clientMuteFilter[playerNum].second = muteDraw;
}


void CGameServer::SpecPlayer(const int player)
{
	if (players[player].clientLink == nullptr) {
		Message(spring::format("Attempt to spec user %d who is not connected", player));
		return;
	}
	if (players[player].spectator) {
		Message(spring::format("Attempt to spec user %d who is spectating already", player));
		return;
	}

	Message(spring::format(PlayerResigned, players[player].name.c_str(), "forced spec"));
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

		const std::vector<int>& teamPlayers = getPlayersInTeam(players, t);
		const std::vector<uint8_t>& teamAIs = getSkirmishAIIds(skirmishAIs, freeSkirmishAIs, t);

		if ((teamPlayers.size() + teamAIs.size()) == 0) {
			// no controllers left in team
			teams[t].SetActive(false);
			teams[t].SetLeader(-1);
		} else if (teamPlayers.empty()) {
			// no human player left in team
			teams[t].SetLeader(skirmishAIs[teamAIs[0]].second.hostPlayer);
		} else {
			// still human controllers left in team
			teams[t].SetLeader(teamPlayers[0]);
		}
	}

	if (hostif != nullptr)
		hostif->SendPlayerDefeated(player);
}


bool CGameServer::CheckPlayerPassword(const int playerNum, const std::string& pw) const
{
	if (playerNum >= players.size()) // new player
		return true;

	const GameParticipant::customOpts& opts = players[playerNum].GetAllValues();
	const auto it = opts.find("password");

	return (it == opts.end() || it->second == pw);
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


unsigned CGameServer::BindConnection(
	std::shared_ptr<netcode::CConnection> clientLink,
	std::string clientName,
	const std::string& clientPassword,
	const std::string& clientVersion,
	const std::string& clientPlatform,
	bool isLocal,
	bool reconnect,
	int netloss
) {
	Message(spring::format("%s attempt from %s", (reconnect ? "Reconnection" : "Connection"), clientName.c_str()));
	Message(spring::format(" -> Version: %s [%s]", clientVersion.c_str(), clientPlatform.c_str()));
	Message(spring::format(" -> Address: %s", clientLink->GetFullAddress().c_str()), false);

	if (clientLink->CanReconnect())
		canReconnect = true;
	// first client to connect determines the reference version
	// the proliferation of maintenance builds since 104.0 means
	// comparing just NETWORK_VERSION is no longer strict enough
	if (refClientVersion.second.empty())
		refClientVersion = {clientName, clientVersion};

	std::string errMsg;

	size_t newPlayerNumber = players.size();

	bool killExistingLink = false;
	// bool reconnectAllowed = canReconnect;

	if (clientVersion != refClientVersion.second) {
		errMsg = "client version '" + clientVersion + "' mismatch, reference is '" + refClientVersion.second + "' set by '" + refClientVersion.first + "'";
	} else {
		struct ConnectionFlags {
			const char* error;
			bool allowConnect;
			bool forceNewLink;
		};

		// find the player in the current list
		const auto pred = [&clientName](const GameParticipant& gp) { return (clientName == gp.name); };
		const auto iter = std::find_if(players.begin(), players.end(), pred);

		const auto GetConnectionFlags = [&](const GameParticipant& gp) -> ConnectionFlags {
			if (gp.isFromDemo)
				return {"User name duplicated in the demo", false, false};

			if (gp.clientLink == nullptr) {
				// not an existing connection
				if (reconnect)
					return {"User is not ingame", false, false};
				if (canReconnect || !gameHasStarted)
					return {"", true, false};

				return {"Game has already started", false, false};
			}

			if (!canReconnect || !gp.clientLink->CheckTimeout(-1))
				return {"User can not reconnect", false, false};

			if (!reconnect)
				return {"", true, true};

			if (gp.clientLink->GetFullAddress() != clientLink->GetFullAddress())
				return {"", true, false};

			return {"User is already ingame", false, false};
		};

		if (iter != players.end()) {
			const GameParticipant& gameParticipant = *iter;
			const ConnectionFlags& gpConnectionFlags = GetConnectionFlags(gameParticipant);

			if (gpConnectionFlags.allowConnect) {
				// allowed, possibly with new link
				newPlayerNumber = gameParticipant.id;
				killExistingLink = gpConnectionFlags.forceNewLink;
			} else {
				// disallowed
				errMsg = gpConnectionFlags.error;
			}
		}

		// not found in the original start script, allow spectator join?
		if (errMsg.empty() && newPlayerNumber >= players.size()) {
			// add tilde prefix to "anonymous" spectators (#4949)
			if (!demoReader && allowSpecJoin)
				clientName = "~" + clientName;

			if (demoReader || allowSpecJoin)
				AddAdditionalUser(clientName, clientPassword);
			else
				errMsg = "User name not authorized to connect";
		}

		// check user's password; disabled for local host
		if (errMsg.empty() && !isLocal)
			if (!CheckPlayerPassword(newPlayerNumber, clientPassword))
				errMsg = "Incorrect password";

		// do not respond before we are sure we want to, and never respond to
		// reconnection attempts since it could interfere with the protocol and
		// desync
		if (!reconnect)
			clientLink->Unmute();
	}

	// >> Reject Connection <<
	if (!errMsg.empty() || newPlayerNumber >= players.size()) {
		Message(spring::format(" -> %s", errMsg.c_str()));
		clientLink->SendData(CBaseNetProtocol::Get().SendQuit(spring::format("Connection rejected: %s", errMsg.c_str())));
		return 0;
	}

	// >> Accept Connection <<
	GameParticipant& newPlayer = players[newPlayerNumber];
	newPlayer.isReconn = gameHasStarted;

	// there is a running link already -> terminate it
	if (killExistingLink) {
		Message(spring::format(PlayerLeft, newPlayer.GetType(), newPlayer.name.c_str(), " terminating existing connection"));
		Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(newPlayerNumber, 0));

		// prevent sending a quit message since that might kill the new connection
		newPlayer.clientLink.reset();
		newPlayer.Kill("Terminating connection");

		if (hostif != nullptr)
			hostif->SendPlayerLeft(newPlayerNumber, 0);
	}

	// inform the player about himself if it's a midgame join
	if (newPlayer.isMidgameJoin)
		clientLink->SendData(CBaseNetProtocol::Get().SendCreateNewPlayer(newPlayerNumber, newPlayer.spectator, newPlayer.team, newPlayer.name));

	// there is an open link -> reconnect
	if (newPlayer.clientLink != nullptr) {
		newPlayer.clientLink->ReconnectTo(*clientLink);

		if (UDPNet != nullptr)
			UDPNet->UpdateConnections();

		Message(spring::format(" -> Connection reestablished (id %i)", newPlayerNumber));
		newPlayer.clientLink->SetLossFactor(netloss);
		newPlayer.clientLink->Flush(!gameHasStarted);
		return newPlayerNumber;
	}

	newPlayer.Connected(clientLink, isLocal);
	newPlayer.SendData(std::shared_ptr<const RawPacket>(myGameData->Pack()));
	newPlayer.SendData(CBaseNetProtocol::Get().SendSetPlayerNum((unsigned char)newPlayerNumber));

	// after gamedata and playerNum, the player can start loading
	if (demoReader == nullptr || myGameSetup->demoName.empty()) {
		// player wants to play -> join team
		if (!newPlayer.spectator) {
			const unsigned newPlayerTeam = newPlayer.team;

			if (!teams[newPlayerTeam].IsActive()) {
				// create new team
				newPlayer.SetReadyToStart(myGameSetup->startPosType != CGameSetup::StartPos_ChooseInGame);
				teams[newPlayerTeam].SetActive(true);
			}

			Broadcast(CBaseNetProtocol::Get().SendJoinTeam(newPlayerNumber, newPlayerTeam));
		}
	}

	// finally send player all packets he missed until now
	for (const std::shared_ptr<const netcode::RawPacket>& p: packetCache)
		newPlayer.SendData(p);

	// new connection established
	Message(spring::format(" -> Connection established (given id %i)", newPlayerNumber));
	clientLink->SetLossFactor(netloss);
	clientLink->Flush(!gameHasStarted);
	return newPlayerNumber;
}


void CGameServer::GotChatMessage(const ChatMessage& msg)
{
	// silently drop empty chat messages
	if (msg.msg.empty())
		return;

	Broadcast(std::shared_ptr<const RawPacket>(msg.Pack()));

	if (hostif == nullptr)
		return;

	// do not echo packets to the autohost
	if (msg.fromPlayer < 0 || msg.fromPlayer == SERVER_PLAYER)
		return;

	hostif->SendPlayerChat(msg.fromPlayer, msg.destination, msg.msg);
}


void CGameServer::InternalSpeedChange(float newSpeed)
{
	if (internalSpeed == newSpeed)
		return;

	Broadcast(CBaseNetProtocol::Get().SendInternalSpeed(newSpeed));
	internalSpeed = newSpeed;
}


void CGameServer::UserSpeedChange(float newSpeed, int player)
{
	if (userSpeedFactor == (newSpeed = Clamp(newSpeed, minUserSpeed, maxUserSpeed)))
		return;

	if (internalSpeed > newSpeed || internalSpeed == userSpeedFactor) // insta-raise speed when not slowed down
		InternalSpeedChange(newSpeed);

	Broadcast(CBaseNetProtocol::Get().SendUserSpeed(player, newSpeed));
	userSpeedFactor = newSpeed;
}


uint8_t CGameServer::ReserveSkirmishAIId()
{
	if (freeSkirmishAIs.empty())
		return MAX_AIS;

	const uint8_t id = freeSkirmishAIs.back();
	freeSkirmishAIs.pop_back();
	return id;
}

