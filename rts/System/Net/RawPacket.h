/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RAW_PACKET_H
#define RAW_PACKET_H

#include "System/Misc/NonCopyable.h"

namespace netcode
{

/**
 * @brief simple structure to hold some data
 */
class RawPacket : public spring::noncopyable
{
public:
	/**
	 * @brief create a new packet and store data inside
	 * @param data the data to store
	 * @param length the length of the data (is save even if its 0)
	 */
	RawPacket(const unsigned char* const data, const unsigned length);

	/**
	 * @brief create a new packet without data
	 * @param length the estimated length of the data (is save even if its 0)
	 */
	RawPacket(const unsigned length);

	/**
	 * @brief Free the memory
	 */
	~RawPacket();

	unsigned char* data;
	const unsigned length;
};

} // namespace netcode

#endif // RAW_PACKET_H
