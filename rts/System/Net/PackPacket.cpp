/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PackPacket.h"

#include "System/Log/ILog.h"

#include <algorithm>
#include <cstdlib>

namespace netcode
{

PackPacket::PackPacket(const uint32_t length): RawPacket(length)
	, id(0) // NETMSG_NONE
	, pos(0)
{
}

PackPacket::PackPacket(const uint32_t length, uint8_t msgID): RawPacket(length)
	, id(msgID)
	, pos(0)
{
	*this << msgID;
}

PackPacket& PackPacket::operator<<(const std::string& text)
{
	size_t size = text.size() + 1;

	if (std::string::npos != text.find_first_of('\0')) {
		LOG_L(L_WARNING, "[PackPacket::%s][id=%u] text must not contain a '\\0' inside, truncating", __func__, id);
		size = text.find_first_of('\0') + 1;
	}
	if ((pos + size) > length) {
		LOG_L(L_WARNING, "[PackPacket::%s][id=%u] text truncated (pos=%u length=%u #text=%u)", __func__, id, pos, length, uint32_t(size));
		size = static_cast<size_t>(length - pos);
	}

	memcpy(reinterpret_cast<char*>(GetWritingPos()), text.c_str(), size);
	pos += size;
	return *this;
}

}

