/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OSCSTATSSENDER_H_
#define _OSCSTATSSENDER_H_

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

#endif // _OSCSTATSSENDER_H_
