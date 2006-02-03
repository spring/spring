/*
 * bitops.h
 * bit twiddling operations
 */
#ifndef _BITOPS_H
#define _BITOPS_H

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

static inline unsigned int count_bits_set(unsigned int w)
{
#if 0
	const unsigned int all1   = ~0;
	const unsigned int mask1h = all1 /  3  << 1;
	const unsigned int mask2l = all1 /  5;
	const unsigned int mask4l = all1 / 17;
	w -= (mask1h & w) >> 1;
	w = (w & mask2l) + ((w>>2) & mask2l);
	w = w + (w >> 4) & mask4l;
	w += w >> 8;
	w += w >> 16;
	return w & 0xff;
#else
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
#endif
}

#define make_even_number(x) 	((x) &= ~0x1)

#define conditionally_set_flag(number, mask, condition) 	((number) ^= (-(condition) ^ (number)) & (mask))

#endif /* _BITOPS_H */
