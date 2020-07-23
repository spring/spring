/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PACK_PACKET_H
#define PACK_PACKET_H

#include "RawPacket.h"

#include <string>
#include <stdexcept>


namespace netcode
{

class PackPacketException : public std::runtime_error {
public:
	PackPacketException(const std::string& what) : std::runtime_error(what) {}
};

using PackPacket = RawPacket;

} // namespace netcode

#endif // PACK_PACKET_H
