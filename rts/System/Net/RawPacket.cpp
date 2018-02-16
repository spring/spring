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

} // namespace netcode

