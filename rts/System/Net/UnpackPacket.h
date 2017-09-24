/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNPACK_PACKET_H
#define UNPACK_PACKET_H

#include "RawPacket.h"

#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <memory>

namespace netcode
{

class UnpackPacketException : public std::runtime_error {
public:
	UnpackPacketException(const std::string& what) : std::runtime_error(what) {}
};

class UnpackPacket
{
public:
	UnpackPacket(std::shared_ptr<const RawPacket>, size_t skipBytes = 0);

	template <typename T>
	void operator>>(T& t)
	{
		if ((pos + sizeof(T)) > pckt->length) {
			throw UnpackPacketException("Unpack failure (type)");
		}
		t = *reinterpret_cast<T*>(pckt->data + pos);
		pos += sizeof(T);
	}

	template <typename element>
	void operator>>(std::vector<element>& vec)
	{
		if ((pckt->length - pos) < (vec.size() * sizeof(element))) {
			throw UnpackPacketException("Unpack failure (vector)");
		}
		const size_t toCopy = vec.size() * sizeof(element);
		std::memcpy((void*)(&vec[0]), pckt->data + pos, toCopy);
		pos += toCopy;
	}

	void operator>>(std::string& text)
	{
		int i = pos;
		for (; i < pckt->length; ++i) {
			if (pckt->data[i] == '\0') {
				break;
			}
		}
		if (i >= pckt->length) {
			throw UnpackPacketException("Unpack failure (string)");
		}
		text = std::string((char*)(pckt->data + pos));
		pos += text.size() + 1;
	}

private:
	std::shared_ptr<const RawPacket> pckt;
	size_t pos;
};

} // namespace netcode

#endif // UNPACK_PACKET_H
