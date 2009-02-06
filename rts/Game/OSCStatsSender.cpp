/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "OSCStatsSender.h"

#include "lib/oscpack/OscOutboundPacketStream.h"
#include "Net/UDPSocket.h"
#include "Sim/Misc/TeamHandler.h"
#include "Game/Game.h"
#include "Game/GameVersion.h"
#include "Game/PlayerHandler.h"
#include "GlobalUnsynced.h"
#include "ConfigHandler.h"

#include <arpa/inet.h>


COSCStatsSender* COSCStatsSender::singleton = NULL;


COSCStatsSender::COSCStatsSender(const std::string& dstAddress,
		unsigned int dstPort)
		: sendingEnabled(configHandler.Get("OscStatsSenderEnabled", false)),
		dstAddress(dstAddress), dstPort(dstPort),
		oscOutputBuffer(NULL), oscPacker(NULL), outSocket(NULL)
{
	if (IsEnabled()) {
		oscOutputBuffer = new char[OSC_OUTPUT_BUFFER_SIZE];
		oscPacker = new osc::OutboundPacketStream(oscOutputBuffer,
				OSC_OUTPUT_BUFFER_SIZE);
		outSocket = new netcode::UDPSocket(OSC_IN_PORT);
		UpdateDestination();

		SendInit();
	}
}
COSCStatsSender::~COSCStatsSender() {

	delete [] oscOutputBuffer;
	delete outSocket;
	delete oscPacker;
}


bool COSCStatsSender::IsEnabled() const {
	return sendingEnabled;
}

COSCStatsSender* COSCStatsSender::GetInstance() {

	if (COSCStatsSender::singleton == NULL) {
		std::string dstAddress =configHandler.GetString(
				"OscStatsSenderDestinationAddress", "127.0.0.1");
		unsigned int dstPort = configHandler.Get(
				"OscStatsSenderDestinationPort", (unsigned int) 6447);
		COSCStatsSender::singleton = new COSCStatsSender(dstAddress, dstPort);
	}

	return COSCStatsSender::singleton;
}


bool COSCStatsSender::SendInit() {

	if (IsEnabled()) {
		return SendInitialInfo()
				&& SendTeamStatsTitles()
				&& SendPlayerStatsTitles();
	} else {
		return false;
	}
}

bool COSCStatsSender::Update(int frameNum) {

	if (IsEnabled() && IsTimeToSend(frameNum)) {
		// Try to send team stats first, as they are more important,
		// more interesting.
		return SendTeamStats() && SendPlayerStats();
	} else {
		return false;
	}
}


const std::string& COSCStatsSender::GetDestinationAddress() const {
	return dstAddress;
}

unsigned int COSCStatsSender::GetDestinationPort() const {
	return dstPort;
}


void COSCStatsSender::UpdateDestination() {

	destination.sin_family = AF_INET;
	destination.sin_addr.s_addr = inet_addr(dstAddress.c_str());
	destination.sin_port = htons(dstPort);
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
		outSocket->SendTo((const unsigned char*) oscPacker->Data(),
				oscPacker->Size(), &destination);
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
		const CPlayer::Statistics& playerStats = localPlayer->currentStats;

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
