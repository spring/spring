/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ChatMessage.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/Net/UnpackPacket.h"

ChatMessage::ChatMessage(int from, int dest, const std::string& chat)
	: fromPlayer(from)
	, destination(dest)
{
	// restrict all (player and autohost) chat-messages before they get Pack()'ed
	msg.resize(std::min(chat.size(), MAX_MSG_SIZE));

	const auto beg = chat.begin();
	const auto end = chat.begin() + msg.size();

	std::fill(msg.begin(), msg.end(), 0);
	std::copy(beg, end, msg.begin());
}

ChatMessage::ChatMessage(std::shared_ptr<const netcode::RawPacket> data)
{
	assert(data->data[0] == NETMSG_CHAT);
	assert(data->length <= (4 * sizeof(uint8_t) + MAX_MSG_SIZE + 1));

	netcode::UnpackPacket packet(data, 2);

	uint8_t from;
	uint8_t dest;

	packet >> from;
	packet >> dest;
	packet >> msg;

	fromPlayer = from;
	destination = dest;
}

const netcode::RawPacket* ChatMessage::Pack() const
{
	// message id (uint8), size (uint8), from (uint8), dest (uint8), len(msg)+null
	// msg.size() itself is clamped to MAX_MSG_SIZE == UINT8_MAX/2 by construction
	constexpr uint8_t  headerSize = 4 * sizeof(uint8_t);
	const     uint8_t messageSize = static_cast<uint8_t>(msg.size() + 1);
	const     uint8_t  packetSize = headerSize + messageSize;

	assert(packetSize <= (headerSize + MAX_MSG_SIZE + 1));

	netcode::PackPacket* buffer = new netcode::PackPacket(packetSize, NETMSG_CHAT);

	*buffer << packetSize;
	*buffer << static_cast<uint8_t>(fromPlayer);
	*buffer << static_cast<uint8_t>(destination);
	*buffer << msg;

	return buffer;
}

