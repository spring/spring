/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PackPacket.h"

#include "System/Log/ILog.h"

#include <algorithm>
#include <cstdlib>

namespace netcode
{

PackPacket::PackPacket(const unsigned length)
	: RawPacket(length)
	, pos(0)
{
}

PackPacket::PackPacket(const unsigned length, unsigned char msgID)
	: RawPacket(length)
	, pos(0)
{
	*this << msgID;
}

PackPacket& PackPacket::operator<<(const std::string& text)
{
	size_t size = text.size() + 1;
	if (std::string::npos != text.find_first_of('\0')) {
		LOG_L(L_WARNING, "A text must not contain a '\\0' inside, truncating");
		size = text.find_first_of('\0') + 1;
	}
	if (size + pos > length) {
		LOG_L(L_WARNING, "netcode: string data truncated in packet");
		size = static_cast<size_t>(length - pos);
	}
	memcpy((char*)(data + pos), text.c_str(), size);
	pos += size;
	return *this;
}

}

