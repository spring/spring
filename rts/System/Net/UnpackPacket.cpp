/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnpackPacket.h"

namespace netcode
{

UnpackPacket::UnpackPacket(boost::shared_ptr<const RawPacket> packet, size_t skipBytes) : pckt(packet), pos(skipBytes)
{
	if(pos > pckt->length)
		throw UnpackPacketException("Unpack failure (byte skip)");
}

}
