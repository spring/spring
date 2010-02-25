/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "ChatMessage.h"

#include "BaseNetProtocol.h"
#include "Net/PackPacket.h"
#include "Net/UnpackPacket.h"

using namespace netcode;

ChatMessage::ChatMessage(int from, int dest, const std::string& chat) : fromPlayer(from), destination(dest), msg(chat)
{
}

ChatMessage::ChatMessage(boost::shared_ptr<const netcode::RawPacket> data)
{
	assert(data->data[0] == NETMSG_CHAT);
	UnpackPacket packet(data, 2);
	unsigned char from;
	unsigned char dest;
	packet >> from;
	packet >> dest;
	packet >> msg;
	fromPlayer = from;
	destination = dest;
}

const netcode::RawPacket* ChatMessage::Pack() const
{
	unsigned char size = 4*sizeof(unsigned char) + msg.size()+1;
	PackPacket* buffer = new PackPacket(size, NETMSG_CHAT);
	*buffer << size;
	*buffer << (unsigned char)fromPlayer;
	*buffer << (unsigned char)destination;
	*buffer << msg;
	return buffer;
}
