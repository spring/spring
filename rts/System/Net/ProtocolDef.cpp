/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ProtocolDef.h"

#include <string.h>
#include <boost/format.hpp>

#include "Exception.h"

namespace netcode {

ProtocolDef* ProtocolDef::instance_ptr = 0;

ProtocolDef* ProtocolDef::instance()
{
	if (!instance_ptr)
		instance_ptr = new ProtocolDef();
	return instance_ptr;
}

ProtocolDef::ProtocolDef()
{
	memset(msg, '\0', sizeof(MsgType)*256);
}

void ProtocolDef::AddType(const unsigned char id, const int MsgLength)
{
	msg[id].Length = MsgLength;
}

// <-1: invalid id
// -1: invalid length
// 0: unknown length, buffer too short
// >0: actual length
int ProtocolDef::PacketLength(const unsigned char* const buf, const unsigned bufLength) const
{
	if (bufLength == 0)
		return 0;
	
	unsigned char msgid = buf[0];
	int len = msg[msgid].Length;

	if (len > 0)
		return len;
	if (len == 0)
		return -2;
	if (len == -1) {
		if (bufLength < 2)
			return 0;
		return (buf[1] >= 2) ? buf[1] : -1;
	}
	if (len == -2) {
		if (bufLength < 3)
			return 0;
		unsigned short slen = *((unsigned short*)(buf + 1));
		return (slen >= 3) ? slen : -1;
	}

	throw network_error(str( boost::format("Invalid Message Length: %1%") %(unsigned int)msgid ));
}

bool ProtocolDef::IsValidLength(const int pktLength, const unsigned bufLength) const
{
	return (pktLength > 0) && (bufLength >= (unsigned)pktLength);
}

bool ProtocolDef::IsValidPacket(const unsigned char* const buf, const unsigned bufLength) const
{
	return IsValidLength(PacketLength(buf, bufLength), bufLength);
}

} // namespace netcode
