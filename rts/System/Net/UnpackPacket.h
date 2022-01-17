/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNPACK_PACKET_H
#define UNPACK_PACKET_H

#include "RawPacket.h"

#include <algorithm>
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
		if ((pos + sizeof(T)) > pckt->length)
			throw UnpackPacketException("Unpack failure (type)");

		std::memcpy(reinterpret_cast<void*>(&t), pckt->data + pos, sizeof(T));
		pos += sizeof(T);
	}

	template <typename T>
	void operator>>(std::vector<T>& vec)
	{
		if (vec.empty())
			return;

		const size_t size = vec.size() * sizeof(T);

		if ((pckt->length - pos) < size)
			throw UnpackPacketException("Unpack failure (vector)");

		std::memcpy(reinterpret_cast<void*>(&vec[0]), pckt->data + pos, size);
		pos += size;
	}

	void operator>>(std::string& text)
	{
		const auto beg = pckt->data + pos;
		const auto end = pckt->data + pckt->length;
		const auto  it = std::find(beg, end, 0);

		// for strings, require a null-term to have been added during packing
		if (it == end)
			throw UnpackPacketException("Unpack failure (string)");

		text.clear();
		text.assign(reinterpret_cast<const char*>(beg), it - beg);

		pos += (text.size() + 1);
	}

private:
	std::shared_ptr<const RawPacket> pckt;
	size_t pos;
};

} // namespace netcode

#endif // UNPACK_PACKET_H
