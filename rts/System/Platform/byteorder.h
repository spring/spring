/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief byte order handling
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

#ifndef BYTE_ORDER_H
#define BYTE_ORDER_H

/*
 * The swabbing stuff looks backwards, but the files
 * are _originally_ little endian (win32 x86).
 * So in this case little endian is the standard while
 * big endian is the exception.
 */

#if defined(__linux__)

	#include <string.h> // for memcpy
	#include <byteswap.h>

	#if __BYTE_ORDER == __BIG_ENDIAN
		#define swabWord(w)  (bswap_16(w))
		#define swabDWord(w) (bswap_32(w))
		#define swab64(w)    (bswap_64(w))
		/*
		 * My brother tells me that a C compiler must store floats in memory
		 * by a particular standard, except for the endianness; hence, this
		 * will work on all C compilers.
		 */
		static inline float swabFloat(float w) {
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
		// do not swab
	#endif

#elif defined(__FreeBSD__)

	#include <sys/endian.h>

	#define swabWord(w)  (htole16(w))
	#define swabDWord(w) (htole32(w))
	#define swab64(w)    (htole64(w))
	static inline float swabFloat(float w) {
		// compile time assertion to validate sizeof(int) == sizeof(float)
		typedef int sizeof_long_equals_sizeof_float[sizeof(int) == sizeof(float) ? 1 : -1];
		int l = swabDWord(*(int*)&w);
		return *(float*)&l;
	}

#elif defined(__APPLE__) && defined(_BIG_ENDIAN)

	#include <CoreFoundation/CFByteOrder.h>

	#define swabWord(w)  (CFSwapInt16(w))
	#define swabDWord(w) (CFSwapInt32(w))
	#define swab64(w)    (CFSwapInt64(w))
	// swabFloat(w) do not swab

#else
	// WIN32

	// do not swab

#endif



#if       defined(swabWord)
	#define swabWordInPlace(w)  (w = swabWord(w))
#else  // defined(swabWord)
	// do nothing
	#define swabWord(w)         (w)
	#define swabWordInPlace(w)
#endif // defined(swabWord)

#if       defined(swabDWord)
	#define swabDWordInPlace(w) (w = swabDWord(w))
#else  // defined(swabDWord)
	// do nothing
	#define swabDWord(w)        (w)
	#define swabDWordInPlace(w)
#endif // defined(swabDWord)

#if       defined(swab64)
	#define swab64InPlace(w) (w = swab64(w))
#else  // defined(swab64)
	// do nothing
	#define swab64(w)        (w)
	#define swab64InPlace(w)
#endif // defined(swab64)

#if       defined(swabFloat)
	#define swabFloatInPlace(w) (w = swabFloat(w))
#else  // defined(swabFloat)
	// do nothing
	#define swabFloat(w)        (w)
	#define swabFloatInPlace(w)
#endif // defined(swabFloat)

#endif // BYTE_ORDER_H
