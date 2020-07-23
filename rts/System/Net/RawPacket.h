/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RAW_PACKET_H
#define RAW_PACKET_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

#include <string>
#include <vector>

#include "System/Misc/NonCopyable.h"
#include "System/SafeVector.h"

namespace netcode
{

/**
 * @brief simple structure to hold some data
 */
class RawPacket
{
public:
	RawPacket() = default;

	/**
	 * @brief create a new packet and store data inside
	 * @param data the data to store
	 * @param length the length of the data (is safe even if 0)
	 */
	RawPacket(const uint8_t* const data, const uint32_t length);

	/**
	 * @brief create a new packet without data
	 * @param length the estimated length of the data
	 */
	RawPacket(const uint32_t newLength): length(newLength) {
		if (length == 0)
			return;

		data = new uint8_t[length];
	}

	RawPacket(const uint32_t length, uint8_t msgID): RawPacket(length) {
		*this << (id = msgID);
	}

	RawPacket(const RawPacket&  p) = delete;
	RawPacket(      RawPacket&& p) { *this = std::move(p); }

	~RawPacket() { Delete(); }


	RawPacket& operator = (const RawPacket&  p) = delete;
	RawPacket& operator = (      RawPacket&& p) {
		// assume no self-assignment
		data = p.data;
		p.data = nullptr;

		id = p.id;
		p.id = 0;

		pos = p.pos;
		p.pos = 0;

		length = p.length;
		p.length = 0;

		return *this;
	}



	// packing operations
	template <typename T>
	RawPacket& operator << (const T& t) {
		constexpr uint32_t size = sizeof(T);
		assert((size + pos) <= length);
		memcpy(reinterpret_cast<T*>(GetWritingPos()), reinterpret_cast<const void*>(&t), sizeof(T));
		pos += size;
		return *this;
	}

	RawPacket& operator << (const std::string& text);

	template <typename element>
	RawPacket& operator << (const std::vector<element>& vec) {
		const size_t size = vec.size() * sizeof(element);
		assert((size + pos) <= length);
		if (size > 0) {
			std::memcpy(GetWritingPos(), reinterpret_cast<const void*>(vec.data()), size);
			pos += size;
		}
		return *this;
	}

	#ifdef USE_SAFE_VECTOR
	template <typename element>
	RawPacket& operator << (const safe_vector<element>& vec) {
		const size_t size = vec.size() * sizeof(element);
		assert((size + pos) <= length);
		if (size > 0) {
			std::memcpy(GetWritingPos(), reinterpret_cast<const void*>(vec.data()), size);
			pos += size;
		}
		return *this;
	}
	#endif


	uint8_t* GetWritingPos() { return (data + pos); }


	void Delete() {
		if (length == 0)
			return;

		delete[] data;
		data = nullptr;

		length = 0;
	}

public:
	uint8_t id = 0;
	uint8_t* data = nullptr;

	uint32_t pos = 0;
	uint32_t length = 0;
};

} // namespace netcode

#endif // RAW_PACKET_H
