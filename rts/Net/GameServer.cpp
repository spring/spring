/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Net/UDPListener.h"

#include "System/Net/UDPConnection.h"

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

#include "System/Net/Connection.h"

#include "GameServer.h"
#include "Net/Protocol/BaseNetProtocol.h"

#include "GameParticipant.h"
#include "GameSkirmishAI.h"
#include "AutohostInterface.h"
#include "Game/GameSetup.h"
#include "Game/Action.h"
#include "Game/ChatMessage.h"
#include "Game/CommandMessage.h"
#include "Game/GlobalUnsynced.h" // for syncdebug
#include "System/Util.h"
#include "System/TdfParser.h"
#include "Sim/Misc/GlobalConstants.h"

#ifdef DEDICATED
	#include "System/LoadSave/DemoRecorder.h"
#else
	#include "Sim/Misc/GlobalSynced.h"
#endif

#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/IVideoCapturing.h"
// This undef is needed, as somewhere there is a type interface specified,
// which we need not!
// (would cause problems in ExternalAI/Interface/SAIInterfaceLibrary.h)
#ifdef interface
	#undef interface
#endif
#include "System/Config/ConfigHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/CRC.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/MsgStrings.h"
#include "System/myMath.h"
#include "System/Net/LocalConnection.h"
#include "System/Net/UnpackPacket.h"
#include "System/LoadSave/DemoReader.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#ifndef DEDICATED
#include "lib/luasocket/src/restrictions.h"
#endif


#define ALLOW_DEMO_GODMODE
#define PKTCACHE_VECSIZE 1000

using netcode::RawPacket;

CONFIG(int, SpeedControl).defaultValue(1).minimumValue(1).maximumValue(2)
	.description("Sets how server adjusts speed according to player's load (CPU), 1: use average, 2: use highest");
CONFIG(bool, BypassScriptPasswordCheck).defaultValue(false);
CONFIG(bool, WhiteListAdditionalPlayers).defaultValue(true);
CONFIG(std::string, AutohostIP).defaultValue("127.0.0.1");
CONFIG(int, AutohostPort).defaultValue(0);

/// frames until a syncchech will time out and a warning is given out
const unsigned SYNCCHECK_TIMEOUT = 300;

/// used to prevent msg spam
const unsigned SYNCCHECK_MSG_TIMEOUT = 400;

/// The time intervall in msec for sending player statistics to each client
const spring_time playerInfoTime = spring_secs(2);

/// every n'th frame will be a keyframe (and contain the server's framenumber)
const unsigned serverKeyframeIntervall = 16;

/// players incoming bandwidth new allowance every X milliseconds
const unsigned playerBandwidthInterval = 100;

/// every 10 sec we'll broadcast current frame in a message that skips queue & cache
/// to let clients that are fast-forwarding to current point to know their loading %
const unsigned gameProgressFrameInterval = GAME_SPEED * 10;

const std::string commands[numCommands] = {
	"kick", "kickbynum", "mute", "mutebynum", "setminspeed", "setmaxspeed",
	"nopause", "nohelp", "cheat", "godmode", "globallos",
	"nocost", "forcestart", "nospectatorchat", "nospecdraw",
	"skip", "reloadcob", "reloadcegs", "devlua", "editdefs",
	"singlestep", "spec", "specbynum"
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
	canReconnect = false;
	allowSpecDraw = true;
	cheating = false;

	gameHasStarted = false;
	generatedGameID = false;

	gameEndTime = spring_notime;
	readyTime = spring_notime;

	medianCpu = 0.0f;
	medianPing = 0;
	curSpeedCtrl = configHandler->GetInt("SpeedControl");

	bypassScriptPasswordCheck = configHandler->GetBool("BypassScriptPasswordCheck");
	whiteListAdditionalPlayers = configHandler->GetBool("WhiteListAdditionalPlayers");

	if (!setup->onlyLocal) {
		UDPNet.reset(new netcode::UDPListener(hostPort, hostIP));
	}

	std::string autohostip = configHandler->GetString("AutohostIP");
	if (StringToLower(autohostip) == "localhost") {
		// FIXME temporary hack: we do not support (host-)names.
		// "localhost" was the only name supported in the past.
		// added 7. January 2011, to be removed in ~ 1 year
		autohostip = "127.0.0.1";
	}
	const int autohostport = configHandler->GetInt("AutohostPort");
	if (autohostport > 0) {
		AddAutohostInterface(autohostip, autohostport);
	}

	rng.Seed(newGameData->GetSetup().length());
	Message(str(format(ServerStart) %hostPort), false);

	lastTick = spring_gettime();

	maxUserSpeed = setup->maxSpeed;
	minUserSpeed = setup->minSpeed;
	noHelperAIs = setup->noHelperAIs;

	StripGameSetupText(newGameData);

	if (setup->hostDemo) {
		Message(str(format(PlayingDemo) %setup->demoName));
		demoReader.reset(new CDemoReader(setup->demoName, modGameTime + 0.1f));
	}

	players.reserve(MAX_PLAYERS); // no reallocation please
	for (int i = 0; i < setup->playerStartingData.size(); ++i)
		players.push_back(GameParticipant());
	std::copy(setup->playerStartingData.begin(), setup->playerStartingData.end(), players.begin());
	UpdatePlayerNumberMap();

	const std::vector<SkirmishAIData> &said = setup->GetSkirmishAIs();
	for (size_t a = 0; a < said.size(); ++a) {
		const unsigned char skirmishAIId = ReserveNextAvailableSkirmishAIId();
		if (skirmishAIId == MAX_AIS) {
			Message(str(format("Too many AIs (%d) in game setup") %said.size()));
			break;
		}
		players[said[a].hostPlayer].linkData[skirmishAIId] = GameParticipant::PlayerLinkData();
		ais[skirmishAIId] = said[a];
	}

	teams.resize(setup->teamStartingData.size());
	std::copy(setup->teamStartingData.begin(), setup->teamStartingData.end(), teams.begin());

	commandBlacklist = std::set<std::string>(commands, commands+numCommands);

#ifdef DEDICATED
	demoRecorder.reset(new CDemoRecorder(setup->mapName, setup->modName));
	demoRecorder->WriteSetupText(gameData->GetSetup());
	const netcode::RawPacket* ret = gameData->Pack();
	demoRecorder->SaveToDemo(ret->data, ret->length, GetDemoTime());
	delete ret;
#endif
	// AIs do not join in here, so just set their teams as active
	for (std::map<unsigned char, GameSkirmishAI>::const_iterator ai = ais.begin(); ai != ais.end(); ++ai) {
		const int t = ai->second.team;
		teams[t].active = true;
		if (teams[t].leader < 0) { // CAUTION, default value is 0, not -1
			teams[t].leader = ai->second.hostPlayer;
		}
	}

	canReconnect = false;
	linkMinPacketSize = globalConfig->linkIncomingMaxPacketRate > 0 ? (globalConfig->linkIncomingSustainedBandwidth / globalConfig->linkIncomingMaxPacketRate) : 1;
	lastBandwidthUpdate = spring_gettime();

	thread = new boost::thread(boost::bind<void, CGameServer, CGameServer*>(&CGameServer::UpdateLoop, this));

#ifdef STREFLOP_H
	// Something in CGameServer::CGameServer borks the FPU control word
	// maybe the threading, or something in CNet::InitServer() ??
	// Set single precision floating point math.
	streflop::streflop_init<streflop::Simple>();
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
	demoRecorder->SetTime(serverFrameNum / GAME_SPEED, spring_tomsecs(spring_gettime() - serverStartTime) / 1000);
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

void CGameServer::StripGameSetupText(const GameData* const newGameData)
{
	// modify and save GameSetup text (remove passwords)
	TdfParser parser(newGameData->GetSetup().c_str(), newGameData->GetSetup().length());
	TdfParser::TdfSection* rootSec = parser.GetRootSection();

	for (TdfParser::sectionsMap_t::iterator it = rootSec->sections.begin(); it != rootSec->sections.end(); ++it) {
		const std::string& sectionKey = StringToLower(it->first);

		if (sectionKey.find("player") != 0)
			continue;

		TdfParser::TdfSection* playerSec = it->second;
		playerSec->remove("password", false);
	}

	std::ostringstream strbuf;
	parser.print(strbuf);

	GameData* newData = new GameData(*newGameData);
	newData->SetSetup(strbuf.str());
	gameData.reset(newData);
}


void CGameServer::AddLocalClient(const std::string& myName, const std::string& myVersion)
{
	Threading::RecursiveScopedLock scoped_lock(gameServerMutex);
	assert(!hasLocalClient);
	hasLocalClient = true;
	localClientNumber = BindConnection(myName, "", myVersion, true, boost::shared_ptr<netcode::CConnection>(new netcode::CLocalConnection()));
}

void CGameServer::AddAutohostInterface(const std::string& autohostIP, const int autohostPort)
{
#ifndef DEDICATED
	//disallow luasockets access to autohost interface
	luaSocketRestrictions->addRule(CLuaSocketRestrictions::UDP_CONNECT, autohostIP.c_str(), autohostPort, false);
#endif
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

void CGameServer::PostLoad(int newServerFrameNum)
{
	Threading::RecursiveScopedLock scoped_lock(gameServerMutex);
	serverFrameNum = newServerFrameNum;

	gameHasStarted = (serverFrameNum > 0);

	std::vector<GameParticipant>::iterator it;
	for (it = players.begin(); it != players.end(); ++it) {
		it->lastFrameResponse = newServerFrameNum;
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
		playerNumberMap[i] = (i < MAX_PLAYERS) ? player : i; // ignore SERVER_PLAYER, ChatMessage::TO_XXX etc
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
		if (player >= players.size() && player < MAX_PLAYERS) { // ignore SERVER_PLAYER, ChatMessage::TO_XXX etc
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
				lastTick = spring_gettime();
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

			case NETMSG_AI_STATE_CHANGED: /* many of these messages are not likely to be sent by a spec, but there are cheats */
			case NETMSG_ALLIANCE:
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
					AddAdditionalUser(name, "", true,(bool)spectator,(int)team); // even though this is a demo, keep the players vector properly updated
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
				if (!AdjustPlayerNumber(buf ,3))
					continue;
				try {
					CommandMessage msg(rpkt);
					const Action& action = msg.GetAction();
					if (msg.GetPlayerID() == SERVER_PLAYER && action.command == "cheat")
						SetBoolArg(cheating, action.extra);
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
	for (size_t p = 0; p < players.size(); ++p)
		players[p].SendData(packet);
	if (canReconnect || bypassScriptPasswordCheck || !gameHasStarted)
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
					Message(str(format(SyncError) %playernames %(*f) %g->first %correctChecksum));
				}

				// send spectator desyncs as private messages to reduce spam
				for (std::map<int, unsigned>::const_iterator s = desyncSpecs.begin(); s != desyncSpecs.end(); ++s) {
					int playerNum = s->first;
#ifdef DEBUG
					LOG_L(L_ERROR,"%s", str(format(SyncError) %players[playerNum].name %(*f) %s->second %correctChecksum).c_str());
					Message(str(format(SyncError) %players[playerNum].name %(*f) %s->second %correctChecksum));
#else
					PrivateMessage(playerNum, str(format(SyncError) %players[playerNum].name %(*f) %s->second %correctChecksum));
#endif
				}
			}
			SetExitCode(-1);
		}

		// Remove complete sets (for which all player's checksums have been received).
		if (bComplete) {
			// Message(str (format("Succesfully purged outstanding sync frame %d from the deque") %(*f)));
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
		if (!demoReader || !hasLocalClient || (serverFrameNum - players[localClientNumber].lastFrameResponse) < GAME_SPEED)
			modGameTime += (tdif * internalSpeed);
	}

	if (lastPlayerInfo < (spring_gettime() - playerInfoTime)) {
		lastPlayerInfo = spring_gettime();

		if (serverFrameNum > 0) {
			LagProtection();
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

	const bool pregameTimeoutReached = (spring_gettime() > serverStartTime + spring_secs(globalConfig->initialNetworkTimeout));
	if (pregameTimeoutReached || gameHasStarted) {
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



void CGameServer::LagProtection()
{
	std::vector<float> cpu;
	std::vector<int> ping;
	cpu.reserve(players.size());
	ping.reserve(players.size());

	// detect reference cpu usage
	float refCpuUsage = 0.0f;
	for (size_t a = 0; a < players.size(); ++a) {
		GameParticipant& player = players[a];
		if (player.myState == GameParticipant::INGAME) {
			// send info about the players
			const int curPing = (serverFrameNum - player.lastFrameResponse);
			Broadcast(CBaseNetProtocol::Get().SendPlayerInfo(a, player.cpuUsage, curPing));

			const float playerCpuUsage = player.isLocal ? player.cpuUsage : player.cpuUsage;
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
		// aim for 60% cpu usage if median is used as reference and 75% cpu usage if max is the reference
		float wantedCpuUsage = (curSpeedCtrl == 1) ?  0.60f : 0.75f;
		wantedCpuUsage += (1.0f - internalSpeed / userSpeedFactor) * 0.5f; //???

		float newSpeed = internalSpeed * wantedCpuUsage / refCpuUsage;
		newSpeed = (newSpeed + internalSpeed) * 0.5f;
		newSpeed = Clamp(newSpeed, 0.1f, userSpeedFactor);
		if (userSpeedFactor <= 2.f)
			newSpeed = std::max(newSpeed, (curSpeedCtrl == 1) ? userSpeedFactor * 0.8f : userSpeedFactor * 0.5f);

#ifndef DEDICATED
		// adjust game speed to localclient's (:= host) maximum SimFrame rate
		const float maxSimFPS = (1000.0f / gu->avgSimFrameTime) * (1.0f - gu->reconnectSimDrawBalance);
		newSpeed = Clamp(newSpeed, 0.1f, ((maxSimFPS / GAME_SPEED) + internalSpeed) * 0.5f);
#endif

		//float speedMod = 1.f + wantedCpuUsage - refCpuUsage;
		//LOG_L(L_DEBUG, "Speed REF %f MED %f WANT %f SPEEDM %f NSPEED %f", refCpuUsage, medianCpu, wantedCpuUsage, speedMod, newSpeed);

		if (newSpeed != internalSpeed)
			InternalSpeedChange(newSpeed);
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
static std::vector<unsigned char> getSkirmishAIIds(const std::map<unsigned char, GameSkirmishAI>& ais, const int teamId, const int hostPlayer = -2) {

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
static int countNumSkirmishAIsInTeam(const std::map<unsigned char, GameSkirmishAI>& ais, const int teamId) {
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
				if (!players[a].isLocal && players[a].spectator && !demoReader) {
					PrivateMessage(a, "Spectators cannot pause the game");
				}
				else if (curSpeedCtrl == 1 && !isPaused && !players[a].isLocal &&
					(players[a].spectator || (curSpeedCtrl == 1 &&
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

		case NETMSG_QUIT: {
			Message(str(format(PlayerLeft) %players[a].GetType() %players[a].name %" normal quit"));
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 1));
			players[a].Kill("User exited");
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
			const unsigned player = inbuf[1];
			if (player != a) {
				Message(str(format(WrongPlayer) %msgCode %a %(unsigned)inbuf[1]));
				break;
			}
			if (setup->startPosType == CGameSetup::StartPos_ChooseInGame) {
				const unsigned team     = (unsigned)inbuf[2];
				if (team >= teams.size()) {
					Message(str(format("Invalid teamID %d in NETMSG_STARTPOS from player %d") %team %player));
				} else if (getSkirmishAIIds(ais, team, player).empty() && ((team != players[player].team) || (players[player].spectator))) {
					Message(str(format("Player %d sent spoofed NETMSG_STARTPOS with teamID %d") %player %team));
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

				#ifndef ALLOW_DEMO_GODMODE
				if (!demoReader)
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
				if (!demoReader)
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
				else if (!demoReader)
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
				else if (!demoReader)
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
				else if (!demoReader)
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
				if (!demoReader) {
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

			if (outstandingSyncFrames.find(frameNum) != outstandingSyncFrames.end())
				players[a].syncResponse[frameNum] = checkSum;

			// update player's ping (if !defined(SYNCCHECK) this is done in NETMSG_KEYFRAME)
			if (frameNum <= serverFrameNum && frameNum > players[a].lastFrameResponse)
				players[a].lastFrameResponse = frameNum;

#ifndef NDEBUG
			// send player <a>'s sync-response back to everybody
			// (the only purpose of this is to allow a client to
			// detect if it is desynced wrt. a demo-stream)
			if ((frameNum % GAME_SPEED) == 0) {
				Broadcast((CBaseNetProtocol::Get()).SendSyncResponse(playerNum, frameNum, checkSum));
			}
#endif
#endif
		} break;

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

					if (toTeam >= teams.size()) {
						Message(str(format("Invalid teamID %d in TEAMMSG_GIVEAWAY from player %d") %toTeam %player));
						break;
					}
					if (fromTeam_g >= teams.size()) {
						Message(str(format("Invalid teamID %d in TEAMMSG_GIVEAWAY from player %d") %fromTeam_g %player));
						break;
					}

					const int numPlayersInTeam_g             = countNumPlayersInTeam(players, fromTeam_g);
					const std::vector<unsigned char> &totAIsInTeam_g = getSkirmishAIIds(ais, fromTeam_g);
					const std::vector<unsigned char> &myAIsInTeam_g  = getSkirmishAIIds(ais, fromTeam_g, player);
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
							Message(str(format("%s %s sent invalid team giveaway") %playerType %players[player].name), true);
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
							Message(str(format("%s %s can not give away stuff of team %i (still has human players left)") %playerType %players[player].name %fromTeam_g), true);
						}
					}
					if (giveAwayOk && (numControllersInTeam_g == 1)) {
						// team has no controller left now
						teams[fromTeam_g].active = false;
						teams[fromTeam_g].leader = -1;
						std::ostringstream givenAwayMsg;
						const int toLeader = teams[toTeam].leader;
						const std::string& toLeaderName = (toLeader >= 0) ? players[toLeader].name : UncontrolledPlayerName;
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
					if (teams[newTeam].leader == -1) {
						teams[newTeam].leader = player;
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

						teams[teamID].active = false;
						teams[teamID].leader = -1;
						// convert all the teams players to spectators
						for (size_t p = 0; p < players.size(); ++p) {
							if ((players[p].team == teamID) && !(players[p].spectator)) {
								// are now spectating if this was their team
								//players[p].team = 0;
								players[p].spectator = true;
								if (hostif)
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
				if (tai->leader == -1) {
					tai->leader = ais[skirmishAIId].hostPlayer;
					tai->active = true;
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
					players[playerId].linkData.erase(skirmishAIId);
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

				if (static_cast<unsigned>(msg.GetPlayerID()) == a) {
					if ((commandBlacklist.find(msg.GetAction().command) != commandBlacklist.end()) && players[a].isLocal) {
						// command is restricted to server but player is allowed to execute it
						PushAction(msg.GetAction());
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
				unsigned char reconnect, netloss;
				unsigned short netversion;
				msg >> netversion;
				if (netversion != NETWORK_VERSION)
					throw netcode::UnpackPacketException(str(format("Wrong network version: %d, required version: %d") %(int)netversion %(int)NETWORK_VERSION));
				msg >> name;
				msg >> passwd;
				msg >> version;
				msg >> reconnect;
				msg >> netloss;
				BindConnection(name, passwd, version, false, UDPNet->AcceptConnection(), reconnect, netloss);
			} catch (const netcode::UnpackPacketException& ex) {
				Message(str(format(ConnectionReject) %ex.what() %packet->data[0] %packet->data[2] %packet->length));
				UDPNet->RejectConnection();
			}
		} else {
			if (packet) {
				if (packet->length >= 3) {
					Message(str(format(ConnectionReject) %"Invalid message ID" %packet->data[0] %packet->data[2] %packet->length));
				} else {
					std::string pkts;

					for (int i = 0; i < packet->length; ++i) {
						pkts += str(format(" 0x%x") %(int)packet->data[i]);
					}

					Message("Connection attempt rejected: Packet too short (data: " + pkts + ")");
				}
			}
			UDPNet->RejectConnection();
		}
	}

	const float updateBandwidth = spring_tomsecs(spring_gettime() - lastBandwidthUpdate) / (float)playerBandwidthInterval;
	if (updateBandwidth >= 1.0f)
		lastBandwidthUpdate = spring_gettime();

	for (size_t a = 0; a < players.size(); ++a) {
		GameParticipant &player = players[a];
		boost::shared_ptr<netcode::CConnection> &plink = player.link;
		if (!plink)
			continue; // player not connected
		if (plink->CheckTimeout(0, !gameHasStarted)) {
			Message(str(format(PlayerLeft) %player.GetType() %player.name %" timeout")); //this must happen BEFORE the reset!
			Broadcast(CBaseNetProtocol::Get().SendPlayerLeft(a, 0));
			player.Kill("User timeout");
			if (hostif)
				hostif->SendPlayerLeft(a, 0);
			continue;
		}

		std::map<unsigned char, GameParticipant::PlayerLinkData> &pld = player.linkData;
		boost::shared_ptr<const RawPacket> packet;
		while ((packet = plink->GetData())) {  // relay all the packets to separate connections for the player and AIs
			unsigned char aiID = MAX_AIS;
			int cID = -1;
			if (packet->length >= 5) {
				cID = packet->data[0];
				if (cID == NETMSG_AICOMMAND || cID == NETMSG_AICOMMAND_TRACKED || cID == NETMSG_AICOMMANDS || cID == NETMSG_AISHARE)
					aiID = packet->data[4];
			}
			std::map<unsigned char, GameParticipant::PlayerLinkData>::iterator liit = pld.find(aiID);
			if (liit != pld.end())
				liit->second.link->SendData(packet);
			else
				Message(str(format("Player %s sent invalid AI ID %d in AICOMMAND %d") %player.name %(int)aiID %cID));
		}

		for (std::map<unsigned char, GameParticipant::PlayerLinkData>::iterator lit = pld.begin(); lit != pld.end(); ++lit) {
			int &bandwidthUsage = lit->second.bandwidthUsage;
			boost::shared_ptr<netcode::CConnection> &link = lit->second.link;

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
					ProcessPacket(a, packet); // non droppable packets may be processed more than once, but this does no harm
					if (globalConfig->linkIncomingPeakBandwidth > 0 && droppablePacket) {
						bandwidthUsage += std::max((unsigned)linkMinPacketSize, packet->length);
						if (!bwLimitIsReached)
							bwLimitIsReached = (bandwidthUsage > globalConfig->linkIncomingPeakBandwidth);
					}
				}
			}
			if (numDropped > 0) {
				if (lit->first == MAX_AIS)
					PrivateMessage(a, str(format("Warning: Waiting packet limit was reached for %s [packets dropped]") %player.name));
				else
					PrivateMessage(a, str(format("Warning: Waiting packet limit was reached for %s AI #%d [packets dropped]") %player.name %(int)lit->first));
			}

			if (!bwLimitWasReached && bwLimitIsReached) {
				if (lit->first == MAX_AIS)
					PrivateMessage(a, str(format("Warning: Bandwidth limit was reached for %s [packets delayed]") %player.name));
				else
					PrivateMessage(a, str(format("Warning: Bandwidth limit was reached for %s AI #%d [packets delayed]") %player.name %(int)lit->first));
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
	crc.Update(setup->gameSetupText.c_str(), setup->gameSetupText.length());
	gameID.intArray[2] = crc.GetDigest();

	CRC entropy;
	entropy.Update(spring_tomsecs(lastTick - serverStartTime));
	gameID.intArray[3] = entropy.GetDigest();

	// fixed gameID?
	if (!setup->gameID.empty()) {
		unsigned char p[16];
	#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
		// workaround missing C99 support in a msvc lib with %02hhx
		generatedGameID = (sscanf(setup->gameID.c_str(),
		      "%02hc%02hc%02hc%02hc%02hc%02hc%02hc%02hc"
		      "%02hc%02hc%02hc%02hc%02hc%02hc%02hc%02hc",
		      &p[ 0], &p[ 1], &p[ 2], &p[ 3], &p[ 4], &p[ 5], &p[ 6], &p[ 7],
		      &p[ 8], &p[ 9], &p[10], &p[11], &p[12], &p[13], &p[14], &p[15]) == 16);
	#else
		auto s = reinterpret_cast<short unsigned int*>(&p[0]);
		generatedGameID = (sscanf(setup->gameID.c_str(),
		      "%02hx%02hx%02hx%02hx%02hx%02hx%02hx%02hx",
		      &s[ 0], &s[ 1], &s[ 2], &s[ 3], &s[ 4], &s[ 5], &s[ 6], &s[ 7]) == 8);
	#endif
		if (generatedGameID)
			for (int i = 0; i<16; ++i)
				gameID.charArray[i] =  p[i];
	}

	Broadcast(CBaseNetProtocol::Get().SendGameID(gameID.charArray));
#ifdef DEDICATED
	demoRecorder->SetGameID(gameID.charArray);
#endif

	generatedGameID = true;
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
	const spring_time gameStartDelay = spring_secs(setup->gameStartDelay);

	if (allReady || forced) {
		if (!spring_istime(readyTime)) {
			readyTime = spring_gettime();

			// we have to wait at least 1 msec, because 0 is a special case
			const unsigned countdown = std::max(1, spring_tomsecs(gameStartDelay));
			Broadcast(CBaseNetProtocol::Get().SendStartPlaying(countdown));

			// make seed more random
			if (setup->gameID.empty())
				rng.Seed(spring_tomsecs(readyTime - serverStartTime));
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
	if (!canReconnect && !bypassScriptPasswordCheck)
		packetCache.clear(); // free memory

	if (UDPNet && !canReconnect && !bypassScriptPasswordCheck)
		UDPNet->SetAcceptingConnections(false); // do not accept new connections

	// make sure initial game speed is within allowed range and sent a new speed if not
	UserSpeedChange(userSpeedFactor, SERVER_PLAYER);

	if (demoReader) {
		// the client told us to start a demo
		// no need to send startPos and startplaying since its in the demo
		Message(DemoStart);
		return;
	}

	{
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
	}

	GenerateAndSendGameID();
	rng.Seed(gameID.intArray[0] ^ gameID.intArray[1] ^ gameID.intArray[2] ^ gameID.intArray[3]);
	Broadcast(CBaseNetProtocol::Get().SendRandSeed(rng()));
	Broadcast(CBaseNetProtocol::Get().SendStartPlaying(0));
	if (hostif)
#ifdef DEDICATED
		hostif->SendStartPlaying(gameID.charArray, demoRecorder->GetName());
#else
		hostif->SendStartPlaying(gameID.charArray, "");
#endif
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
	else if (action.command == "mute") {

		if (action.extra.empty()) {
			LOG_L(L_WARNING,"failed to mute player, usage: /mute <playername> [chatmute] [drawmute]");
		}
		else {
			const std::vector<std::string> &tokens = CSimpleParser::Tokenize(action.extra);
			if ( tokens.empty() || tokens.size() > 3 ) {
				LOG_L(L_WARNING,"failed to mute player, usage: /mute <playername> [chatmute] [drawmute]");
			}
			else {
				std::string name = tokens[0];
				StringToLowerInPlace(name);
				bool muteChat;
				bool muteDraw;
				if ( !tokens.empty() ) SetBoolArg(muteChat, tokens[1]);
				if ( tokens.size() >= 2 ) SetBoolArg(muteDraw, tokens[2]);
				for (size_t a=0; a < players.size();++a) {
					std::string playerLower = StringToLower(players[a].name);
					if (playerLower.find(name)==0) {	// can kick on substrings of name
						MutePlayer(a,muteChat,muteDraw);
						break;
					}
				}
			}
		}
	}
	else if (action.command == "mutebynum") {

		if (action.extra.empty()) {
			LOG_L(L_WARNING,"failed to mute player, usage: /mutebynum <player-id> [chatmute] [drawmute]");
		}
		else {
			const std::vector<std::string> &tokens = CSimpleParser::Tokenize(action.extra);
			if ( tokens.empty() || tokens.size() > 3 ) {
				LOG_L(L_WARNING,"failed to mute player, usage: /mutebynum <player-id> [chatmute] [drawmute]");
			}
			else {
				int playerID = atoi(tokens[0].c_str());
				bool muteChat = true;
				bool muteDraw = true;
				if ( !tokens.empty() ) SetBoolArg(muteChat, tokens[1]);
				if ( tokens.size() >= 2 ) SetBoolArg(muteDraw, tokens[2]);
				MutePlayer(playerID,muteChat,muteDraw);
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
			for (size_t a=0; a < players.size();++a) {
				std::string playerLower = StringToLower(players[a].name);
				if (playerLower.find(name)==0) {	// can spec on substrings of name
					SpecPlayer(a);
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
				std::vector<GameParticipant>::iterator participantIter =
						std::find_if( players.begin(), players.end(), boost::bind( &GameParticipant::name, _1 ) == name );

				if (participantIter != players.end()) {
					const GameParticipant::customOpts &opts = participantIter->GetAllValues();
					GameParticipant::customOpts::const_iterator it;
					if ((it = opts.find("origpass")) == opts.end() || it->second == "") {
						it = opts.find("password");
						if(it != opts.end())
							participantIter->SetValue("origpass", it->second);
					}
					participantIter->SetValue("password", pwd);
					LOG("[%s] changed player/spectator password: \"%s\" \"%s\"", __FUNCTION__, name.c_str(), pwd.c_str());
				} else {
					AddAdditionalUser(name, pwd, false, spectator, team);

					LOG(
						"[%s] added client \"%s\" with password \"%s\" to team %d (as a %s)",
						__FUNCTION__, name.c_str(), pwd.c_str(), team, (spectator? "spectator": "player")
					);
				}
			} else {
				LOG_L(L_WARNING,
					"[%s] failed to add player/spectator password. usage: "
					"/adduser <player-name> <password> [spectator] [team]",
					__FUNCTION__
				);
			}
		}
	}
	else if (action.command == "kill") {
		LOG("[%s] server killed", __FUNCTION__);
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
	Threading::RecursiveScopedLock scoped_lock(gameServerMutex);
	return quitServer;
}

void CGameServer::CreateNewFrame(bool fromServerThread, bool fixedFrameTime)
{
	if (demoReader) {
		CheckSync();
		SendDemoData(-1);
		return;
	}

	Threading::RecursiveScopedLock(gameServerMutex, !fromServerThread);

	CheckSync();
	int newFrames = 1;

	if (!fixedFrameTime) {
		spring_time currentTick = spring_gettime();
		spring_time timeElapsed = currentTick - lastTick;

		if (timeElapsed > spring_msecs(200))
			timeElapsed = spring_msecs(200);

		timeLeft += GAME_SPEED * internalSpeed * timeElapsed.toSecsf();
		lastTick  = currentTick;
		newFrames = (timeLeft > 0)? int(math::ceil(timeLeft)): 0;
		timeLeft -= newFrames;

#ifndef DEDICATED
		if (hasLocalClient) {
			// Don't create newFrames when localClient (:= host) isn't able to process them fast enough.
			// Despite still allow to create a few in advance to not lag other connected clients.
			const float simFramesBehind = serverFrameNum - players[localClientNumber].lastFrameResponse;
			const float a          = std::min(simFramesBehind / GAME_SPEED, 1.0f);
			const int curSimFPS    = std::max((int)gu->simFPS, GAME_SPEED); // max advance 1sec in the future
			const int maxNewFrames = mix(curSimFPS, 0, a);
			newFrames = std::min(newFrames, maxNewFrames);
		}
#endif
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

			// every gameProgressFrameInterval, we broadcast current frame in a special message that doesn't get cached and skips normal queue, to let players know their loading %
			if ((serverFrameNum % gameProgressFrameInterval) == 0) {
				CBaseNetProtocol::PacketType progressPacket = CBaseNetProtocol::Get().SendCurrentFrameProgress(serverFrameNum);
				// we cannot use broadcast here, since we want to skip caching
				for (size_t p = 0; p < players.size(); ++p)
					players[p].SendData(progressPacket);
			}
#ifdef SYNCCHECK
			outstandingSyncFrames.insert(serverFrameNum);
#endif
		}
	}
}

void CGameServer::UpdateSpeedControl(int speedCtrl) {
	if (speedCtrl != curSpeedCtrl) {
		Message(str(format("Server speed control: %s")
			%(SpeedControlToString(speedCtrl).c_str())));
		curSpeedCtrl = speedCtrl;
	}
}

std::string CGameServer::SpeedControlToString(int speedCtrl) {

	std::string desc;
	if (speedCtrl == 0) {
		desc = "Maximum CPU";
	} else if (speedCtrl == 1) {
		desc = "Average CPU";
	} else {
		desc = "<invalid>";
	}
	return desc;
}

void CGameServer::UpdateLoop()
{
	try {
		Threading::SetThreadName("netcode");
		Threading::SetAffinity(~0);

		while (!quitServer) {
			spring_sleep(spring_msecs(10));

			if (UDPNet)
				UDPNet->Update();

			Threading::RecursiveScopedLock scoped_lock(gameServerMutex);
			ServerReadNet();
			Update();
		}

		if (hostif)
			hostif->SendQuit();
		Broadcast(CBaseNetProtocol::Get().SendQuit("Server shutdown"));

		// flush the quit messages to reduce ugly network error messages on the client side
		spring_sleep(spring_msecs(1000)); // this is to make sure the Flush has any effect at all (we don't want a forced flush)
		for (size_t i = 0; i < players.size(); ++i) {
			if (players[i].link)
				players[i].link->Flush();
		}
		spring_sleep(spring_msecs(3000)); // now let clients close their connections
	} CATCH_SPRING_ERRORS
}

bool CGameServer::WaitsOnCon() const
{
	return (UDPNet && UDPNet->IsAcceptingConnections());
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

void CGameServer::MutePlayer(const int playerNum, bool muteChat, bool muteDraw )
{
	if ( playerNum >= players.size() ) {
		LOG_L(L_WARNING,"invalid playerNum");
		return;
	}
	if ( playerNum < mutedPlayersChat.size() ) {
		mutedPlayersChat.resize(playerNum);
	}
	if ( playerNum < mutedPlayersDraw.size() ) {
		mutedPlayersDraw.resize(playerNum);
	}
	mutedPlayersChat[playerNum] = muteChat;
	mutedPlayersDraw[playerNum] = muteDraw;
}

void CGameServer::SpecPlayer(const int player) {
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
	// actualize all teams of which the player is leader
	for (size_t t = 0; t < teams.size(); ++t) {
		if (teams[t].leader == player) {
			const std::vector<int> &teamPlayers = getPlayersInTeam(players, t);
			const std::vector<unsigned char> &teamAIs  = getSkirmishAIIds(ais, t);
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
}


void CGameServer::AddAdditionalUser(const std::string& name, const std::string& passwd, bool fromDemo, bool spectator, int team)
{
	GameParticipant buf;
	buf.isFromDemo = fromDemo;
	buf.name = name;
	buf.spectator = spectator;
	buf.team = team;
	buf.isMidgameJoin = true;
	if (passwd.size() > 0)
		buf.SetValue("password",passwd);
	players.push_back(buf);
	UpdatePlayerNumberMap();
	if (!fromDemo)
		Broadcast(CBaseNetProtocol::Get().SendCreateNewPlayer(players.size() -1, buf.spectator, buf.team, buf.name)); // inform all the players of the newcomer
}


unsigned CGameServer::BindConnection(std::string name, const std::string& passwd, const std::string& version, bool isLocal, boost::shared_ptr<netcode::CConnection> link, bool reconnect, int netloss)
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
					if (reconnect)
						errmsg = "User is not ingame";
					else if (canReconnect || !gameHasStarted)
						newPlayerNumber = i;
					else
						errmsg = "Game has already started";
					break;
				}
				else {
					bool reconnectAllowed = canReconnect && players[i].link->CheckTimeout(-1);
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
		if (demoReader || bypassScriptPasswordCheck)
			AddAdditionalUser(name, passwd);
		else
			errmsg = "User name not authorized to connect";
	}

	// check for user's password
	if (errmsg == "" && !isLocal) {
		if (newPlayerNumber < players.size()) {
			const GameParticipant::customOpts &opts = players[newPlayerNumber].GetAllValues();
			GameParticipant::customOpts::const_iterator it;
			if ((it = opts.find("password")) != opts.end() && passwd != it->second) {
				if ((!reconnect || (it = opts.find("origpass")) == opts.end() || it->second == "" || passwd != it->second))
					errmsg = "Incorrect password";
			}
			else if (!reconnect) // forget about origpass whenever a real mid-game join succeeds
				players[newPlayerNumber].SetValue("origpass", "");
		}
	}

	if(!reconnect) // don't respond before we are sure we want to do it
		link->Unmute(); // never respond to reconnection attempts, it could interfere with the protocol and desync

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
		if (hostif)
			hostif->SendPlayerLeft(newPlayerNumber, 0);
	}

	if (newPlayer.isMidgameJoin) {
		link->SendData(CBaseNetProtocol::Get().SendCreateNewPlayer(newPlayerNumber, newPlayer.spectator, newPlayer.team, newPlayer.name)); // inform the player about himself if it's a midgame join
	}

	newPlayer.isReconn = gameHasStarted;

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

	link->SetLossFactor(netloss);
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
	if (curSpeedCtrl == 1 &&
		player >= 0 && static_cast<unsigned int>(player) != SERVER_PLAYER &&
		!players[player].isLocal && !isPaused &&
		(players[player].spectator || (curSpeedCtrl == 1 &&
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

unsigned char CGameServer::ReserveNextAvailableSkirmishAIId() {

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

void CGameServer::FreeSkirmishAIId(const unsigned char skirmishAIId) {
	usedSkirmishAIIds.remove(skirmishAIId);
}

void CGameServer::AddToPacketCache(boost::shared_ptr<const netcode::RawPacket> &pckt) {
	if (packetCache.empty() || packetCache.back().size() >= PKTCACHE_VECSIZE) {
		packetCache.push_back(std::vector<boost::shared_ptr<const netcode::RawPacket> >());
		packetCache.back().reserve(PKTCACHE_VECSIZE);
	}
	packetCache.back().push_back(pckt);
}
