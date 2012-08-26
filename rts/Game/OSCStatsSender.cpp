/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/asio.hpp> // must be included before streflop!

#include "OSCStatsSender.h"

#include "Game.h"
#include "GameVersion.h"
#include "GlobalUnsynced.h"
#include "Player.h"
#include "PlayerHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Net/Socket.h"

#include "lib/streflop/streflop_cond.h"
#include "lib/oscpack/OscOutboundPacketStream.h"


CONFIG(bool, OscStatsSenderEnabled).defaultValue(false);
CONFIG(std::string, OscStatsSenderDestinationAddress).defaultValue("127.0.0.1");

CONFIG(int, OscStatsSenderDestinationPort)
	.defaultValue(6447)
	.minimumValue(0)
	.maximumValue(65535);

COSCStatsSender* COSCStatsSender::singleton = NULL;

struct COSCStatsSender::NetStruct
{
	NetStruct() : outSocket(NULL), destination(NULL) {};
	boost::asio::ip::udp::socket* outSocket;
	boost::asio::ip::udp::endpoint* destination;
};

COSCStatsSender::COSCStatsSender(const std::string& dstAddress,
		unsigned int dstPort)
		: sendingEnabled(false), dstAddress(dstAddress), dstPort(dstPort),
		network(NULL),
		oscOutputBuffer(NULL), oscPacker(NULL)
{
	SetEnabled(configHandler->GetBool("OscStatsSenderEnabled"));
}
COSCStatsSender::~COSCStatsSender() {
	SetEnabled(false);
}


void COSCStatsSender::SetEnabled(bool enabled) {

	bool statusChanged = (enabled != sendingEnabled);
	this->sendingEnabled = enabled;

	if (statusChanged) {
		if (enabled) {
			oscOutputBuffer = new char[OSC_OUTPUT_BUFFER_SIZE];
			oscPacker = new osc::OutboundPacketStream(oscOutputBuffer,
					OSC_OUTPUT_BUFFER_SIZE);
			network = new NetStruct;
			network->outSocket = new boost::asio::ip::udp::socket(netcode::netservice, boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6::any(), 0));
			boost::asio::socket_base::broadcast option(true);
			network->outSocket->set_option(option);
			UpdateDestination();
			LOG("Sending spring Statistics over OSC to: %s:%u",
					dstAddress.c_str(), dstPort);

			SendInit();
		} else {
			delete oscPacker;
			oscPacker = NULL;
			delete network->outSocket;
			network->outSocket = NULL;
			delete network->destination;
			network->destination = NULL;
			delete network;
			delete [] oscOutputBuffer;
			oscOutputBuffer = NULL;
		}
	}
}
bool COSCStatsSender::IsEnabled() const {
	return sendingEnabled;
}

COSCStatsSender* COSCStatsSender::GetInstance() {

	if (COSCStatsSender::singleton == NULL) {
		std::string dstAddress = configHandler->GetString("OscStatsSenderDestinationAddress");
		unsigned int dstPort   = configHandler->GetInt("OscStatsSenderDestinationPort");

		static COSCStatsSender instance(dstAddress, dstPort);
		COSCStatsSender::singleton = &instance;
	}

	return COSCStatsSender::singleton;
}


bool COSCStatsSender::SendInit() {

	if (IsEnabled()) {
		try {
			return SendInitialInfo()
					&& SendTeamStatsTitles()
					&& SendPlayerStatsTitles();
		} catch (const boost::system::system_error& ex) {
			LOG_L(L_ERROR, "Failed sending OSC Stats init: %s", ex.what());
			return false;
		}
	} else {
		return false;
	}
}

bool COSCStatsSender::Update(int frameNum) {

	if (IsEnabled() && IsTimeToSend(frameNum)) {
		try {
			// Try to send team stats first, as they are more important,
			// more interesting.
			return SendTeamStats() && SendPlayerStats();
		} catch (const boost::system::system_error& ex) {
			LOG_L(L_ERROR, "Failed sending OSC Stats init: %s", ex.what());
			return false;
		}
	} else {
		return false;
	}
}


void COSCStatsSender::SetDestinationAddress(const std::string& address) {
	this->dstAddress = address;
	UpdateDestination();
}
const std::string& COSCStatsSender::GetDestinationAddress() const {
	return dstAddress;
}

void COSCStatsSender::SetDestinationPort(unsigned int port) {
	this->dstPort = port;
	UpdateDestination();
}
unsigned int COSCStatsSender::GetDestinationPort() const {
	return dstPort;
}


void COSCStatsSender::UpdateDestination() {

	if (network->destination == NULL) {
		network->destination = new boost::asio::ip::udp::endpoint();
	}
	network->destination->address(netcode::WrapIP(dstAddress));
	network->destination->port(dstPort);
}

bool COSCStatsSender::IsTimeToSend(int frameNum) {

	if (IsEnabled() && !((frameNum+5) & 31)) {
		return true;
	}

	return false;
}

bool COSCStatsSender::SendOscBuffer() {

	bool success = false;

	if (oscPacker->IsReady()) {
		network->outSocket->send_to(boost::asio::buffer((const unsigned char*) oscPacker->Data(), oscPacker->Size()), *network->destination);
		oscPacker->Clear();

		success = true;
	}

	return success;
}

bool COSCStatsSender::SendInitialInfo() {

	if (IsEnabled()) {
		(*oscPacker)
				<< osc::BeginBundleImmediate
					<< osc::BeginMessage(OSC_MSG_TOPIC_INIT_TITLES)
						<< "Engine name"
						<< "Engine version"
						<< "Number of teams"
					<< osc::EndMessage
					<< osc::BeginMessage(OSC_MSG_TOPIC_INIT_VALUES)
						<< "spring"
						<< SpringVersion::GetFull().c_str()
						<< teamHandler->ActiveTeams()
					<< osc::EndMessage
				<< osc::EndBundle;

		return SendOscBuffer();
	} else {
		return false;
	}
}
bool COSCStatsSender::SendPlayerStatsTitles() {

	if (IsEnabled()) {
		(*oscPacker)
				<< osc::BeginBundleImmediate
					<< osc::BeginMessage(OSC_MSG_TOPIC_PLAYER_TITLES)
						<< "Player Name"
						<< "Mouse clicks per minute"
						<< "Mouse movement in pixels per minute"
						<< "Keyboard presses per minute"
						<< "Unit commands per minute"
						<< "Average command size (units affected per command)"
					<< osc::EndMessage
				<< osc::EndBundle;

		return SendOscBuffer();
	} else {
		return false;
	}
}

bool COSCStatsSender::SendPlayerStats() {

	static const CPlayer* localPlayer = playerHandler->Player(gu->myPlayerNum);
	static const char* localPlayerName = localPlayer->name.c_str();

	if (IsEnabled()) {
		// get the latest player stats
		const PlayerStatistics& playerStats = localPlayer->currentStats;

		(*oscPacker)
				<< osc::BeginBundleImmediate
					<< osc::BeginMessage(OSC_MSG_TOPIC_PLAYER_VALUES)
						<< (const char*) localPlayerName
						<< (float) (playerStats.mouseClicks*60/game->totalGameTime)
						<< (float) (playerStats.mousePixels*60/game->totalGameTime)
						<< (float) (playerStats.keyPresses*60/game->totalGameTime)
						<< (float) (playerStats.numCommands*60/game->totalGameTime)
						<< (float) ((playerStats.numCommands != 0) ?
							((float) playerStats.unitCommands / playerStats.numCommands) : 0.0)
					<< osc::EndMessage
				<< osc::EndBundle;

		return SendOscBuffer();
	} else {
		return false;
	}
}

bool COSCStatsSender::SendTeamStatsTitles() {

	if (IsEnabled()) {
		(*oscPacker)
				<< osc::BeginBundleImmediate
					<< osc::BeginMessage(OSC_MSG_TOPIC_TEAM_TITLES)
						<< "Team number"

						<< "Metal used"
						<< "Energy used"
						<< "Metal produced"
						<< "Energy produced"

						<< "Metal excess"
						<< "Energy excess"

						<< "Metal received"
						<< "Energy received"

						<< "Metal sent"
						<< "Energy sent"

						<< "Metal stored"
						<< "Energy stored"

						<< "Active Units"
						<< "Units killed"

						<< "Units produced"
						<< "Units died"

						<< "Units received"
						<< "Units sent"
						<< "Units captured"
						<< "Units stolen"

						<< "Damage Dealt"
						<< "Damage Received"
					<< osc::EndMessage
				<< osc::EndBundle;

		return SendOscBuffer();
	} else {
		return false;
	}
}

bool COSCStatsSender::SendTeamStats() {

	static const int teamId = gu->myTeam;
	static unsigned int prevHistSize = 0;

	if (IsEnabled()) {
		std::list<CTeam::Statistics> statHistory =
				teamHandler->Team(teamId)->statHistory;
		unsigned int currHistSize = statHistory.size();
		// only send if we did not yet send the latest history stats
		if (currHistSize <= prevHistSize) {
			return false;
		}

		// get the latest history stats
		std::list<CTeam::Statistics>::const_iterator latestStats_p =
				--(statHistory.end());
		const CTeam::Statistics& teamStats = *latestStats_p;

		(*oscPacker)
				<< osc::BeginBundleImmediate
					<< osc::BeginMessage(OSC_MSG_TOPIC_TEAM_VALUES)
						<< (int) teamId

						<< (float) teamStats.metalUsed
						<< (float) teamStats.energyUsed
						<< (float) teamStats.metalProduced
						<< (float) teamStats.energyProduced

						<< (float) teamStats.metalExcess
						<< (float) teamStats.energyExcess

						<< (float) teamStats.metalReceived
						<< (float) teamStats.energyReceived

						<< (float) teamStats.metalSent
						<< (float) teamStats.energySent

						<< (float) (teamStats.metalProduced+teamStats.metalReceived
							- (teamStats.metalUsed+teamStats.metalSent+teamStats.metalExcess))
						<< (float) (teamStats.energyProduced+teamStats.energyReceived
							- (teamStats.energyUsed+teamStats.energySent+teamStats.energyExcess))

						<< (int) (teamStats.unitsProduced+teamStats.unitsReceived+teamStats.unitsCaptured
							- (teamStats.unitsDied+teamStats.unitsSent+teamStats.unitsOutCaptured))
						<< (int) teamStats.unitsKilled

						<< (int) teamStats.unitsProduced
						<< (int) teamStats.unitsDied

						<< (int) teamStats.unitsReceived
						<< (int) teamStats.unitsSent
						<< (int) teamStats.unitsCaptured
						<< (int) teamStats.unitsOutCaptured

						<< (float) teamStats.damageDealt
						<< (float) teamStats.damageReceived
					<< osc::EndMessage
				<< osc::EndBundle;

		prevHistSize = currHistSize;

		return SendOscBuffer();
	} else {
		return false;
	}
}
