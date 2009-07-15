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

#ifndef __OSCSTATSSENDER_H__
#define __OSCSTATSSENDER_H__


const char* const OSC_MSG_TOPIC_INIT_TITLES  = "/spring/info/initial/titles";
const char* const OSC_MSG_TOPIC_INIT_VALUES  = "/spring/info/initial/values";
const char* const OSC_MSG_TOPIC_PLAYER_TITLES  = "/spring/stats/player/titles";
const char* const OSC_MSG_TOPIC_PLAYER_VALUES  = "/spring/stats/player/values";
const char* const OSC_MSG_TOPIC_TEAM_TITLES  = "/spring/stats/team/titles";
const char* const OSC_MSG_TOPIC_TEAM_VALUES  = "/spring/stats/team/values";

#include <string>

namespace osc { class OutboundPacketStream; }

class COSCStatsSender
{
private:
	COSCStatsSender(const std::string& dstAddress, unsigned int dstPort);
	~COSCStatsSender();

public:
	static COSCStatsSender* GetInstance();

	void SetEnabled(bool enabled);
	bool IsEnabled() const;

	bool SendInit();
	bool Update(int frameNum);

	void SetDestinationAddress(const std::string& address);
	const std::string& GetDestinationAddress() const;

	void SetDestinationPort(unsigned int port);
	unsigned int GetDestinationPort() const;

	/**
	 * Generic method to send OSC messages to the configured receiver(s).
	 * @param  oscAdress  the messages title/address,
	 *                    eg: "/spring/stats/team/values"
	 * @param  fmt        describes the parrameter types of the message to send,
	 *                    eg: "fifs" means {float, int, float, const char*}
	 *                    allowed types (OSC base types):
	 *                    'i' 32bit integer [int]
	 *                    'f' 32bit floating point number [float]
	 *                    's' string [const char*]
	 *                    'b' blob; byte array [const unsigned char*]
	 * @param  params     pointers to the parameters described in fmt, eg:
	 *                    {&someFloat, &someInt, &otherFloat, &cStr}
	 * @return whether the sending was successfully done.
	 */
	bool SendPropertiesInfo(const char* oscAdress, const char* fmt,
			void* params[]);

private:
	void UpdateDestination();
	bool IsTimeToSend(int frameNum);

	bool SendOscBuffer();

	bool SendInitialInfo();
	bool SendPlayerStatsTitles();
	bool SendPlayerStats();
	bool SendTeamStatsTitles();
	bool SendTeamStats();

private:
	static const unsigned int OSC_OUTPUT_BUFFER_SIZE = 16318; // 16KB
	static COSCStatsSender* singleton;

private:
	bool sendingEnabled;
	std::string dstAddress;
	unsigned int dstPort;
	struct NetStruct;
	NetStruct* network;
	char* oscOutputBuffer;
	osc::OutboundPacketStream* oscPacker;
};

#define oscStatsSender COSCStatsSender::GetInstance()

#endif // __OSCSTATSSENDER_H__
