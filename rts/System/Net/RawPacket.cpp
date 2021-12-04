/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string.h>
#include <stdexcept>

#include "RawPacket.h"

#include "System/Log/ILog.h"

namespace netcode
{

RawPacket::RawPacket(const unsigned char* const tdata, const unsigned newLength)
	: length(newLength)
{
	if (length > 0) {
		data = new unsigned char[length];
		memcpy(data, tdata, length);
	} else {
		LOG_L(L_ERROR, "Tried to pack a zero lengh packet");
		// TODO handle error
	}
}

RawPacket::RawPacket(const unsigned newLength)
	: length(newLength)
{
	if (length > 0) {
		data = new unsigned char[length];
	}
}

RawPacket::~RawPacket()
{
	if (length > 0) {
		delete[] data;
	}
}

} // namespace netcode
