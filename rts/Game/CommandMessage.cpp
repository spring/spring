#include "StdAfx.h"
#include <assert.h>

#include "mmgr.h"

#include "CommandMessage.h"

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
	assert(pckt->data[0] == NETMSG_CCOMMAND);
	UnpackPacket packet(pckt, 3);
	packet >> player;
	packet >> action.command;
	packet >> action.extra;
}

const netcode::RawPacket* CommandMessage::Pack() const
{
	unsigned short size = 3 + sizeof(player) + action.command.size() + action.extra.size() + 2;
	PackPacket* buffer = new PackPacket(size, NETMSG_CCOMMAND);
	*buffer << size;
	*buffer << player;
	*buffer << action.command;
	*buffer << action.extra;
	return buffer;
}

