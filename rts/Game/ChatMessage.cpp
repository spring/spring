/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ChatMessage.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/Net/UnpackPacket.h"
#include <cinttypes>

using namespace netcode;

ChatMessage::ChatMessage(int from, int dest, const std::string& chat)
	: fromPlayer(from)
	, destination(dest)
	, msg(chat)
{
}

ChatMessage::ChatMessage(std::shared_ptr<const netcode::RawPacket> data)
{
	assert(data->data[0] == NETMSG_CHAT);
	UnpackPacket packet(data, 2);
	unsigned char from;
	unsigned char dest;
	packet >> from;
	packet >> dest;
	packet >> msg;
	fromPlayer = from;
	destination = dest;
}

const netcode::RawPacket* ChatMessage::Pack() const
{
	unsigned size = (4 * sizeof(unsigned char)) + (msg.size() + 1);
	std::uint8_t csize = (size > UINT8_MAX) ? UINT8_MAX : size;

	PackPacket* buffer = new PackPacket(size, NETMSG_CHAT);
	*buffer << csize;
	*buffer << (unsigned char)fromPlayer;
	*buffer << (unsigned char)destination;
	if (size > UINT16_MAX) {
		std::string msg_(msg);
		msg_.resize(size - csize);
		*buffer << msg_;
	} else {
		*buffer << msg;
	}
	return buffer;
}
