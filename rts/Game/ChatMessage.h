#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <boost/shared_ptr.hpp>

#include "Action.h"

namespace netcode {
	class RawPacket;
}

class ChatMessage
{
public:
	ChatMessage(int fromP, int dest, const std::string& chat);
	ChatMessage(boost::shared_ptr<const netcode::RawPacket> packet);

	const netcode::RawPacket* Pack() const;

	static const int TO_ALLIES = 127;
	static const int TO_SPECTATORS = 126;
	static const int TO_EVERYONE = 125;

	int fromPlayer;
	
	/// the destination can be: TO_ALLIES, TO_SPECTATORS, TO_EVERYONE or a playernumber
	int destination;
	std::string msg;
};

#endif
