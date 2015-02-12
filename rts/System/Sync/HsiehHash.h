/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HSIEH_HASH_H
#define HSIEH_HASH_H

#include <boost/cstdint.hpp> /* Replace with <stdint.h> if appropriate */

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const boost::uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((boost::uint32_t)(((const boost::uint8_t *)(d))[1])) << 8)\
	+(boost::uint32_t)(((const boost::uint8_t *)(d))[0]) )
#endif

/** @brief a fast hash function
 *
 * This hash function is roughly 4x as fast as CRC32, but even that is too slow.
 * We use a very simplistic add/xor feedback scheme when not debugging.
 */
static inline boost::uint32_t HsiehHash (const void* data_, int len, boost::uint32_t hash)
{
	const char* data = static_cast<const char*>(data_);

	if (len <= 0 || data == NULL) return hash;

	boost::uint32_t tmp;
	int rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (boost::uint16_t);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
	case 3:
		hash += get16bits (data);
		hash ^= hash << 16;
		hash ^= data[sizeof (boost::uint16_t)] << 18;
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
