/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <memory>
#include <string>

namespace netcode {
	class RawPacket;
}

class ChatMessage
{
public:
	ChatMessage(int from, int dest, const std::string& chat);
	ChatMessage(std::shared_ptr<const netcode::RawPacket> packet);

	const netcode::RawPacket* Pack() const;

	static constexpr size_t MAX_MSG_SIZE = UINT8_MAX / 2;

	static constexpr int TO_ALLIES     = 252;
	static constexpr int TO_SPECTATORS = 253;
	static constexpr int TO_EVERYONE   = 254;

	int fromPlayer = -1;
	/// can be TO_ALLIES, TO_SPECTATORS, TO_EVERYONE, or a player number
	int destination = -1;

	std::string msg;
};

#endif // CHAT_MESSAGE_H
