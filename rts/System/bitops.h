/**
 * @file bitops.h
 * @brief Bit twiddling operations
 *
 * Bit twiddling shortcuts for various
 * operations; see http://graphics.stanford.edu/%7Eseander/bithacks.html
 * for details.
 */
#ifndef _BITOPS_H
#define _BITOPS_H

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
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return ++x;
}

/**
 * @brief Count bits set
 * @param w Number in which to count bits
 * @return The number of bits set
 *
 * Counts the number of bits in an unsigned int
 * that are set to 1.  So, for example, in the
 * number 5, hich is 101 in binary, there are
 * two bits set to 1.
 */
static inline unsigned int count_bits_set(unsigned int w)
{
	/*
	 * This is faster, and runs in parallel
	 */
	const int S[] = {1, 2, 4, 8, 16};
	const int B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};
	int c = w;
	c = ((c >> S[0]) & B[0]) + (c & B[0]);
	c = ((c >> S[1]) & B[1]) + (c & B[1]);
	c = ((c >> S[2]) & B[2]) + (c & B[2]);
	c = ((c >> S[3]) & B[3]) + (c & B[3]);
	c = ((c >> S[4]) & B[4]) + (c & B[4]);
	return c;
}

/**
 * @brief Make even number macro
 * @param x Number to make even
 *
 * Quick macro to make a number even, by
 * forcing the rightmost bit to 0.
 */
#define make_even_number(x) 	((x) &= ~0x1)

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
