/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief Bit twiddling operations
 *
 * Bit twiddling shortcuts for various
 * operations; see http://graphics.stanford.edu/%7Eseander/bithacks.html
 * for details.
 */
#ifndef _BITOPS_H
#define _BITOPS_H

#ifdef _MSC_VER
#include <intrin.h>
#endif

/**
 * @brief Next power of 2
 * @param x The number to be rounded
 * @return the rounded number
 *
 * Rounds an unsigned integer up to the
 * next power of 2; i.e. 2, 4, 8, 16, etc.
 */
static inline unsigned int next_power_of_2(unsigned int x)
{
#ifdef __GNUC__
	return 1 << (sizeof(unsigned int) * 8 - __builtin_clz(--x));
#else
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return ++x;
#endif
}

static inline unsigned int log_base_2(unsigned int value) {
	constexpr unsigned int mantissaShift = 23;
	constexpr unsigned int exponentBias = 126;
	union U {
		unsigned int x;
		float y;
	} u = {0};

	// value is constrained to [0,32]
	u.y = static_cast<float>(value - 1);

	// extract exponent bit
	return (((u.x & (0xFF << mantissaShift)) >> mantissaShift) - exponentBias);
}

/**
 * @brief Count bits set
 * @param w Number in which to count bits
 * @return The number of bits set
 *
 * Counts the number of bits in an unsigned int
 * that are set to 1.  So, for example, in the
 * number 5, which is 101 in binary, there are
 * two bits set to 1.
 */
static inline unsigned int count_bits_set(unsigned int w)
{
#ifdef __GNUC__
	return __builtin_popcount(w);
#else
	const int S[] = {1, 2, 4, 8, 16};
	const int B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};
	int c = w;
	c = ((c >> S[0]) & B[0]) + (c & B[0]);
	c = ((c >> S[1]) & B[1]) + (c & B[1]);
	c = ((c >> S[2]) & B[2]) + (c & B[2]);
	c = ((c >> S[3]) & B[3]) + (c & B[3]);
	c = ((c >> S[4]) & B[4]) + (c & B[4]);
	return c;
#endif
}

static inline unsigned count_leading_ones(unsigned int x)
{
	unsigned int i = ((~x) << 24) | 0x00FFFFFF;
#ifdef _MSC_VER
	unsigned long r;
	_BitScanReverse(&r, (unsigned long)i);
	return 31 - r;
#else
	return __builtin_clz(i);
#endif
}


/**
 * quote from GCC doc "Returns one plus the index of the least significant 1-bit of x, or if x is zero, returns zero."
 */
static inline unsigned bits_ffs(unsigned int x)
{
#ifdef __GNUC__
	return __builtin_ffs(x);
#else
	if (x == 0) return 0;
	unsigned i = 1;
	while (!(x & 0x1)) {
		x = x >> 1;
		++i;
	}
	return i;
#endif
}



/**
 * @brief Make even number macro
 * @param n Number to make even
 *
 * Quick macro to make a number even, by
 * forcing the rightmost bit to 0.
 */
#define make_even_number(n) 	((n) += ((n) & 0x1)?1:0) //! ~ceil()
//#define make_even_number(n) 	((x) &= ~0x1)            //! ~floor()

/**
 * @brief Conditionally set flag macro
 * @param number to set or unset flag in
 * @param mask bitmask to be set or unset
 * @param condition whether to set or unset the flag
 *
 * According to a condition, this macro will set or unset a
 * particular flag in a number.  It's a quick way of doing
 * this:
 * if (condition) number |= mask; else number &= ~mask;
 */
#define conditionally_set_flag(number, mask, condition) 	((number) ^= (-(condition) ^ (number)) & (mask))

#endif /* _BITOPS_H */
