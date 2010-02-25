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
	{
		instance_ptr = new ProtocolDef();
	}
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

bool ProtocolDef::HasFixedLength(const unsigned char id) const
{
	if (msg[id].Length > 0)
		return true;
	else if (msg[id].Length < 0)
		return false;
	else
	{
		throw network_error(str( boost::format("Unbound Message Type: %1%") %(unsigned int)id ));
	}
}

bool ProtocolDef::IsAllowed(const unsigned char id) const
{
	if (msg[id].Length != 0)
		return true;
	else
		return false;
}

int ProtocolDef::GetLength(const unsigned char id) const
{
	return msg[id].Length;
}

unsigned ProtocolDef::IsComplete(const unsigned char* const buf, const unsigned bufLength) const
{
	if (bufLength == 0)
	{
		return 0;
	}
	else
	{
		if (HasFixedLength(buf[0]))
		{
			if (bufLength >= (unsigned int)GetLength(buf[0]))
				return GetLength(buf[0]);
			else
				return 0;
		}
		else
		{
			int var = GetLength(buf[0]);
			if (var == -1)
			{
				if (bufLength < 2)
					return 0;
				
				var = buf[1];
			}
			else if (var == -2)
			{
				if (bufLength <= 2)
					return 0;
				
				var = *((unsigned short*)(buf + 1));
			}
			
			if (bufLength >= (unsigned int)var)
				return var;
			else
				return 0;
		}
	}
}

} // namespace netcode
