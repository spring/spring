#ifndef COMMANDMESSAGE_H
#define COMMANDMESSAGE_H

#include <string>
#include <boost/shared_ptr.hpp>

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
	CommandMessage(boost::shared_ptr<const netcode::RawPacket>);

	const netcode::RawPacket* Pack() const;

	Action action;
	int player;
private:
};

#endif
 
