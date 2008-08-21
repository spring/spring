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
	UnpackPacket packet(data);
	unsigned char ID;
	unsigned char length;
	unsigned char from;
	unsigned char dest;
	packet >> ID;
	assert(ID == NETMSG_CHAT);
	packet >> length;
	packet >> from;
	packet >> dest;
	packet >> msg;
	fromPlayer = from;
	destination = dest;
}

const netcode::RawPacket* ChatMessage::Pack() const
{
	unsigned char size = 4*sizeof(unsigned char) + msg.size()+1;
	PackPacket* buffer = new PackPacket(size);
	*buffer << (unsigned char)NETMSG_CHAT;
	*buffer << size;
	*buffer << (unsigned char)fromPlayer;
	*buffer << (unsigned char)destination;
	*buffer << msg;
	return buffer;
}
