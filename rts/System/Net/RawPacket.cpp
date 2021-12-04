/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string.h>
#include <stdexcept>

#include "RawPacket.h"

#include "System/Log/ILog.h"

namespace netcode
{

RawPacket::RawPacket(const uint8_t* const tdata, const uint32_t newLength): length(newLength)
{
	if (length > 0) {
		data = new uint8_t[length];
		memcpy(data, tdata, length);
	} else {
		LOG_L(L_ERROR, "[%s] tried to pack a zero-length packet", __func__);
		// TODO handle error
	}
}

RawPacket& RawPacket::operator<<(const std::string& text)
{
	size_t size = text.size() + 1;

	if (std::string::npos != text.find_first_of('\0')) {
		LOG_L(L_WARNING, "[RawPacket::%s][id=%u] text must not contain a '\\0' inside, truncating", __func__, id);
		size = text.find_first_of('\0') + 1;
	}
	if ((pos + size) > length) {
		LOG_L(L_WARNING, "[RawPacket::%s][id=%u] text truncated (pos=%u length=%u #text=%u)", __func__, id, pos, length, uint32_t(size));
		size = static_cast<size_t>(length - pos);
	}

	memcpy(reinterpret_cast<char*>(GetWritingPos()), text.c_str(), size);
	pos += size;
	return *this;
}

} // namespace netcode

