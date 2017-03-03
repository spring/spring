/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HSIEH_HASH_H
#define HSIEH_HASH_H

#include <cassert>
#include <cinttypes>

#undef get16bits
#define get16bits(d) (*reinterpret_cast<const std::uint16_t*>(d))


/** @brief a fast hash function
 *
 * This hash function is roughly 4x as fast as CRC32, but even that is too slow.
 * We use a very simplistic add/xor feedback scheme when not debugging.
 */
static inline std::uint32_t HsiehHash (const void* data_, int len, std::uint32_t hash)
{
	const std::uint8_t* data = reinterpret_cast<const std::uint8_t*>(data_);
	assert(data != nullptr || len == 0);

	int rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits (data);
		std::uint32_t tmp = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		hash  += hash >> 11;
		data  += 2*sizeof (std::uint16_t);
	}

	/* Handle end cases */
	switch (rem) {
	case 3:
		hash += get16bits (data);
		hash ^= hash << 16;
		hash ^= data[sizeof (std::uint16_t)] << 18;
		hash += hash >> 11;
		break;
	case 2:
		hash += get16bits (data);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1:
		hash += *data;
		hash ^= hash << 10;
		hash += hash >> 1;
		break;
	default:
		break;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

#endif // !defined(HSIEH_HASH_H)
