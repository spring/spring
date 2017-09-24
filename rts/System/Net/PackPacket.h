/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PACK_PACKET_H
#define PACK_PACKET_H

#include "RawPacket.h"
#include "System/SafeVector.h"

#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include <stdexcept>


namespace netcode
{

class PackPacketException : public std::runtime_error {
public:
	PackPacketException(const std::string& what) : std::runtime_error(what) {}
};

class PackPacket : public RawPacket
{
public:
	PackPacket(const uint32_t length);
	PackPacket(const uint32_t length, uint8_t msgID);

	template <typename T>
	PackPacket& operator<<(const T& t) {
		const uint32_t size = sizeof(T);
		assert((size + pos) <= length);
		*reinterpret_cast<T*>(GetWritingPos()) = t;
		pos += size;
		return *this;
	}

	PackPacket& operator<<(const std::string& text);

	template <typename element>
	PackPacket& operator<<(const std::vector<element>& vec) {
		const size_t size = vec.size() * sizeof(element);
		assert((size + pos) <= length);
		if (size > 0) {
			std::memcpy(GetWritingPos(), (void*)(vec.data()), size);
			pos += size;
		}
		return *this;
	}

#ifdef USE_SAFE_VECTOR
	template <typename element>
	PackPacket& operator<<(const safe_vector<element>& vec) {
		const size_t size = vec.size() * sizeof(element);
		assert((size + pos) <= length);
		if (size > 0) {
			std::memcpy(GetWritingPos(), (void*)(vec.data()), size);
			pos += size;
		}
		return *this;
	}
#endif

	uint8_t* GetWritingPos() { return (data + pos); }

private:
	uint8_t id;
	uint32_t pos;
};

} // namespace netcode

#endif // PACK_PACKET_H
