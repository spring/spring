#ifndef COMMANDMESSAGE_H
#define COMMANDMESSAGE_H

#include <string>

#include "Net/UnpackPacket.h"
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
	CommandMessage(netcode::UnpackPacket* packet);

	const netcode::RawPacket* Pack() const;

	Action action;
	int player;
private:
};

#endif
 
