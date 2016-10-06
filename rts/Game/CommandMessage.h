/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_MESSAGE_H
#define COMMAND_MESSAGE_H

#include <string>
#include <memory>

#include "Action.h"

namespace netcode {
	class RawPacket;
}

/// send console commands over network
class CommandMessage
{
public:
	CommandMessage(const std::string& cmd, int playerID);
	CommandMessage(const Action& action, int playerID);
	CommandMessage(std::shared_ptr<const netcode::RawPacket> pckt);

	const netcode::RawPacket* Pack() const;

	const Action& GetAction() const { return action; }
	int GetPlayerID() const { return playerID; }

private:
	Action action;
	int playerID;
};

#endif // COMMAND_MESSAGE_H

