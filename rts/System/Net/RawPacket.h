/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RAW_PACKET_H
#define RAW_PACKET_H

#include <cstdint>
#include <utility>

#include "System/Misc/NonCopyable.h"

namespace netcode
{

/**
 * @brief simple structure to hold some data
 */
class RawPacket : public spring::noncopyable
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

	RawPacket(RawPacket&& p) { *this = std::move(p); }
	~RawPacket() { Delete(); }

	RawPacket& operator = (RawPacket&& p) {
		data = p.data;
		p.data = nullptr;

		length = p.length;
		p.length = 0;
		return *this;
	}

	void Delete() {
		if (length == 0)
			return;

		delete[] data;
		data = nullptr;

		length = 0;
	}

	uint8_t* data = nullptr;
	uint32_t length = 0;
};

} // namespace netcode

#endif // RAW_PACKET_H
