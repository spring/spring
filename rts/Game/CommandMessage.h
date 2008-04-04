#ifndef COMMANDMESSAGE_H
#define COMMANDMESSAGE_H

#include <string>

#include "Action.h"

namespace netcode {
	class RawPacket;
}

/// send console commands over network
class CommandMessage
{
public:
	CommandMessage(const std::string& cmd, int playernum);
	CommandMessage(const Action& action, int playernum);
	CommandMessage(const netcode::RawPacket& packet);

	const netcode::RawPacket* Pack() const;

	Action action;
	int player;
private:
};

#endif
 
