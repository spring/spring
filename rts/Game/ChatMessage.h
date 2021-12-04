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
	ChatMessage(int fromP, int dest, const std::string& chat);
	ChatMessage(std::shared_ptr<const netcode::RawPacket> packet);

	const netcode::RawPacket* Pack() const;

	static const int TO_ALLIES = 252;
	static const int TO_SPECTATORS = 253;
	static const int TO_EVERYONE = 254;

	int fromPlayer;

	/// the destination can be: TO_ALLIES, TO_SPECTATORS, TO_EVERYONE or a playernumber
	int destination;
	std::string msg;
};

#endif // CHAT_MESSAGE_H
