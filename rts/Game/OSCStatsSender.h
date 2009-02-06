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

#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace netcode { class UDPSocket; }
namespace osc { class OutboundPacketStream; }

class COSCStatsSender
{
private:
	COSCStatsSender(const std::string& dstAddress, unsigned int dstPort);
	~COSCStatsSender();

public:
	static COSCStatsSender* GetInstance();

	bool IsEnabled() const;

	bool SendInit();
	bool Update(int frameNum);

	const std::string& GetDestinationAddress() const;
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
	// We do not want to receive data on our socket,
	// only send, but we still have to define an in-port,
	// as a socket can not be constructed without one. (?)
	static const unsigned int OSC_IN_PORT = 10101;
	static COSCStatsSender* singleton;

private:
	bool sendingEnabled;
	std::string dstAddress;
	unsigned int dstPort;
	struct sockaddr_in destination;
	char* oscOutputBuffer;
	osc::OutboundPacketStream* oscPacker;
	netcode::UDPSocket* outSocket;
};

#define oscStatsSender COSCStatsSender::GetInstance()

#endif // __OSCSTATSSENDER_H__
