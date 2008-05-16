#include "CommandMessage.h"

#include <assert.h>

#include "BaseNetProtocol.h"
#include "Net/PackPacket.h"
#include "Net/UnpackPacket.h"

using namespace netcode;

CommandMessage::CommandMessage(const std::string& cmd, int playernum)
{
	action = Action(cmd);
	player = playernum;
}

CommandMessage::CommandMessage(const Action& myaction, int playernum)
{
	action = myaction;
	player = playernum;
}

CommandMessage::CommandMessage(boost::shared_ptr<const netcode::RawPacket> pckt)
{
	UnpackPacket packet(pckt);
	unsigned char ID;
	unsigned short length;
	packet >> ID;
	assert(ID == NETMSG_CCOMMAND);
	packet >> length;
	packet >> player;
	packet >> action.command;
	packet >> action.extra;
}

const netcode::RawPacket* CommandMessage::Pack() const
{
	unsigned short size = 3 + sizeof(player) + action.command.size() + action.extra.size() + 2;
	PackPacket* buffer = new PackPacket(size);
	*buffer << (unsigned char)NETMSG_CCOMMAND;
	*buffer << size;
	*buffer << player;
	*buffer << action.command;
	*buffer << action.extra;
	return buffer;
}

