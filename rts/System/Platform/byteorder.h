/**
 * @file byteorder.h
 * @brief byte order handling
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 *
 * Mac   PPC: BIG    Endian
 * Mac   X86: little Endian
 *
 * Win   X86: little Endian
 *
 * BSD   X86: BIG    Endian
 * Linux X86: BIG    Endian
 *
 * Um... all x86 machines are little endian.
 * - cfh
 */
#ifndef BYTEORDER_H
#define BYTEORDER_H

#include <string.h>

/*
 * The swabbing stuff looks backwards, but the files
 * are _originally_ little endian (win32 x86).
 * So in this case little endian is the standard while
 * big endian is the exception.
 */

#if defined(__linux__)
#include <byteswap.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#define swabword(w)	(bswap_16(w))
#define swabdword(w)	(bswap_32(w))
/*
   My brother tells me that a C compiler must store floats in memory
   by a particular standard, except for the endianness; hence, this
   will work on all C compilers.
 */
static inline float swabfloat(float w) {
	char octets[4];
	char ret_octets[4];
	float ret;

	memcpy(octets, &w, 4);

	ret_octets[0] = octets[3];
	ret_octets[1] = octets[2];
	ret_octets[2] = octets[1];
	ret_octets[3] = octets[0];

	memcpy(&ret, ret_octets, 4);

	return ret;
}
#else
#define swabword(w)	(w)
#define swabdword(w)	(w)
#define swabfloat(w)	(w)
#endif

#elif defined(__FreeBSD__)

#include <sys/endian.h>

#define swabword(w)	(htole16(w))
#define swabdword(w)	(htole32(w))
static inline float swabfloat(float w) {
	/* compile time assertion to validate sizeof(int) == sizeof(float) */
	typedef int sizeof_long_equals_sizeof_float[sizeof(int) == sizeof(float) ? 1 : -1];
	int l = swabdword(*(int*)&w);
	return *(float*)&l;
}

#elif defined(__APPLE__) && defined(_BIG_ENDIAN)

#include <CoreFoundation/CFByteOrder.h>

#define swabword(w)	CFSwapInt32(w)
#define swabdword(w)	CFSwapInt64(w)
#define swabfloat(w)	(w)

#else

// empty versions for win32
#define swabword(w)	(w)
#define swabdword(w)	(w)
#define swabfloat(w)	(w)

#endif

#endif
