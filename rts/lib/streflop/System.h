/*
    streflop: STandalone REproducible FLOating-Point
    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License

    Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP, and IBM MP lib.
    Uses SoftFloat too.

    Please read the history and copyright information in the documentation provided with the source code
*/


#ifndef STREFLOP_SYSTEM_H
#define STREFLOP_SYSTEM_H

#if defined(STREFLOP_SSE) || defined(STREFLOP_X87)
// SSE or X87 machines are little-endian
#define __BYTE_ORDER 1234
#define __FLOAT_WORD_ORDER 1234

// Softfloat or other unknown FPU. TODO: Try some header autodetect?
#else

#define __BYTE_ORDER 1234
#define __FLOAT_WORD_ORDER 1234
/*
#include <endian.h>

#if !defined(__FLOAT_WORD_ORDER) || !defined(__BYTE_ORDER)
#undef __FLOAT_WORD_ORDER
#undef __BYTE_ORDER

#if defined(_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN) || defined(LITTLE_ENDIAN)
// avoid conflicting defs
#undef _LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#undef __LITTLE_ENDIAN
#undef __BYTE_ORDER
#define __BYTE_ORDER 1234
#define __FLOAT_WORD_ORDER 1234

#elif defined(_BIG_ENDIAN) || defined(__BIG_ENDIAN) || defined(BIG_ENDIAN)
// avoid conflicting defs
#undef _BIG_ENDIAN
#undef BIG_ENDIAN
#undef __BIG_ENDIAN
#undef __BYTE_ORDER
#define __BYTE_ORDER 4321
#define __FLOAT_WORD_ORDER 4321

#else
// Unfortunately, the endianity for the compilation target cannot be deduced at compile time
// => preprocessor #define is necessary, possibly with heuristics
#error Could not detect endianity.
#endif

#endif
*/
#endif


#include "IntegerTypes.h"

// Avoid conflict with system types, if any
namespace streflop {

// Use the assumption above for char
typedef char int8_t;
typedef unsigned char uint8_t;

// Now "run" the meta-program to define the types
// Entry point is (unsigned) short, then all types above are tried till the size match
typedef SizedInteger<16>::Type int16_t;
typedef SizedUnsignedInteger<16>::Type uint16_t;
typedef SizedInteger<32>::Type int32_t;
typedef SizedUnsignedInteger<32>::Type uint32_t;
typedef SizedInteger<64>::Type int64_t;
typedef SizedUnsignedInteger<64>::Type uint64_t;

}

#endif
