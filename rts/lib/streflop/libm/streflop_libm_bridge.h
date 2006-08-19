/*
    This file acts as a bridge between the libm files and streflop.
    It replaces the missing type definitions and macros that were defined
    by the external files that were included by the libm, so it is now
    standalone.

    In addition, this allows to wrap the basic types for compiling
    the libm, ex, with SoftFloat.

    This file is NOT included by the streflop main files, and thus NOT
    included by the user programs.
    It is included by the libm converted files to produce the streflop binary,
    that will be linked by the user program instead of the system libm.

    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License
*/

#ifndef STREFLOP_LIBM_BRIDGE
#define STREFLOP_LIBM_BRIDGE

// prevent inclusion of the main Math.h file
// => the function symbol are not defined when compiling libm
// => this adds a level of protection, a function inadvertantly using a wrong precision function is detected
#define STREFLOP_MATH_H

// First define our custom types
#include "../streflop.h"

// Export type definitions, etc.
using namespace streflop;

// for int32_t, etc. Assume WORDSIZE is defined by the Makefile.common
#include "../System.h"
#undef  __USE_ISOC99
#undef  __USE_POSIX
#undef  __USE_POSIX2
#undef  __USE_POSIX199309
#undef  __USE_POSIX199506
#undef  __USE_XOPEN
#undef  __USE_XOPEN_EXTENDED
#undef  __USE_UNIX98
#undef  __USE_XOPEN2K
#undef  __USE_LARGEFILE
#undef  __USE_LARGEFILE64
#undef  __USE_FILE_OFFSET64
#undef  __USE_BSD
#undef  __USE_SVID
#undef  __USE_MISC
#undef  __USE_GNU
#undef  __USE_REENTRANT
#undef  __USE_FORTIFY_LEVEL
#undef  __FAVOR_BSD
#undef  __KERNEL_STRICT_NAMES

#undef weak_alias
#undef strong_alias
#undef hidden_def
#undef libm_hidden_def
#undef INTDEF
#define weak_alias(x,y)
#define strong_alias(x,y)
#define hidden_def(x)
#define libm_hidden_def(x)
#define INTDEF(x)
#define __set_errno(x) do {} while (0)

#define _IEEE_LIBM 1

// u_int32_t is not C99 compliant
typedef streflop::uint32_t u_int32_t;

/////////////////////////////////////////////////////////////////////////////
// Common definitions
/////////////////////////////////////////////////////////////////////////////

// <float.h> is not included, define only these constants that are needed to compile libm
#define FLT_MIN_EXP (-128)
#define FLT_MAX_EXP 127
#define FLT_MANT_DIG 24
#define DBL_MIN_EXP (-1024)
#define DBL_MAX_EXP 1023
#define DBL_MANT_DIG 53
#define LDBL_MANT_DIG 64

#define HUGE_VALF SimplePositiveInfinity
#define HUGE_VAL DoublePositiveInfinity
#define HUGE_VALL ExtendedPositiveInfinity

#define INT_MAX ((-1)>>1)

// Stolen from bits/mathdef.h
#define FP_ILOGB0      (-2147483647)
#define FP_ILOGBNAN    2147483647

namespace streflop_libm {

// No template, only allow this for int types (could use traits, but it's simpler this way)
inline char abs(char x) {return x < 0 ? -x : x;}
inline short abs(short x) {return x < 0 ? -x : x;}
inline int abs(int x) {return x < 0 ? -x : x;}
inline long abs(long x) {return x < 0 ? -x : x;}
inline long long abs(long long x) {return x < 0 ? -x : x;}

inline char MIN(char x, char y) {return x<y ? x : y;}
inline short MIN(short x, short y) {return x<y ? x : y;}
inline int MIN(int x, int y) {return x<y ? x : y;}
inline long MIN(long x, long y) {return x<y ? x : y;}
inline long long MIN(long long x, long long y) {return x<y ? x : y;}


// Compensate lack of Math.h
enum
{
    FP_NAN = 0,
    FP_INFINITE = 1,
    FP_ZERO = 2,
    FP_SUBNORMAL = 3,
    FP_NORMAL = 4
};

#ifdef LIBM_COMPILING_FLT32
    extern int __fpclassifyf(Simple x);
    extern int __isnanf(Simple x);
    extern int __isinff(Simple x);
    inline int fpclassify(Simple x) {return streflop_libm::__fpclassifyf(x);}
    inline int isnan(Simple x) {return streflop_libm::__isnanf(x);}
    inline int isinf(Simple x) {return streflop_libm::__isinff(x);}
    inline int isfinite(Simple x) {return !(isnan(x) || isinf(x));}
    inline int isnormal(Simple x) {return fpclassify(x) == FP_NORMAL;}
    inline bool isunordered(Simple x, Simple y) {
        return (fpclassify(x) == FP_NAN) || (fpclassify (y) == FP_NAN);
    }
    inline bool isgreater(Simple x, Simple y) {
        return (!isunordered(x,y)) && (x > y);
    }
    inline bool isgreaterequal(Simple x, Simple y) {
        return (!isunordered(x,y)) && (x >= y);
    }
    inline bool isless(Simple x, Simple y) {
        return (!isunordered(x,y)) && (x < y);
    }
    inline bool islessequal(Simple x, Simple y) {
        return (!isunordered(x,y)) && (x <= y);
    }
    inline bool islessgreater(Simple x, Simple y) {
        return (!isunordered(x,y)) && ((x < y) || (x > y));
    }
    extern const Simple SimplePositiveInfinity;
    extern const Simple SimpleNegativeInfinity;
    extern const Simple SimpleNaN;
#endif

#ifdef LIBM_COMPILING_DBL64
    extern int __fpclassify(Double x);
    extern int __isnanl(Double x);
    extern int __isinf(Double x);
    inline int fpclassify(Double x) {return streflop_libm::__fpclassify(x);}
    inline int isnan(Double x) {return streflop_libm::__isnanl(x);}
    inline int isinf(Double x) {return streflop_libm::__isinf(x);}
    inline int isfinite(Double x) {return !(isnan(x) || isinf(x));}
    inline int isnormal(Double x) {return fpclassify(x) == FP_NORMAL;}
    inline bool isunordered(Double x, Double y) {
        return (fpclassify(x) == FP_NAN) || (fpclassify (y) == FP_NAN);
    }
    inline bool isgreater(Double x, Double y) {
        return (!isunordered(x,y)) && (x > y);
    }
    inline bool isgreaterequal(Double x, Double y) {
        return (!isunordered(x,y)) && (x >= y);
    }
    inline bool isless(Double x, Double y) {
        return (!isunordered(x,y)) && (x < y);
    }
    inline bool islessequal(Double x, Double y) {
        return (!isunordered(x,y)) && (x <= y);
    }
    inline bool islessgreater(Double x, Double y) {
        return (!isunordered(x,y)) && ((x < y) || (x > y));
    }
    extern const Double DoublePositiveInfinity;
    extern const Double DoubleNegativeInfinity;
    extern const Double DoubleNaN;
#endif

#ifdef LIBM_COMPILING_LDBL96
#ifdef Extended
    extern int __fpclassifyl(Extended x);
    extern int __isnanl(Extended x);
    extern int __isinfl(Extended x);
    inline int fpclassify(Extended x) {return streflop_libm::__fpclassifyl(x);}
    inline int isnan(Extended x) {return streflop_libm::__isnanl(x);}
    inline int isinf(Extended x) {return streflop_libm::__isinfl(x);}
    inline int isfinite(Extended x) {return !(isnan(x) || isinf(x));}
    inline int isnormal(Extended x) {return fpclassify(x) == FP_NORMAL;}
    inline bool isunordered(Extended x, Extended y) {
        return (fpclassify(x) == FP_NAN) || (fpclassify (y) == FP_NAN);
    }
    inline bool isgreater(Extended x, Extended y) {
        return (!isunordered(x,y)) && (x > y);
    }
    inline bool isgreaterequal(Extended x, Extended y) {
        return (!isunordered(x,y)) && (x >= y);
    }
    inline bool isless(Extended x, Extended y) {
        return (!isunordered(x,y)) && (x < y);
    }
    inline bool islessequal(Extended x, Extended y) {
        return (!isunordered(x,y)) && (x <= y);
    }
    inline bool islessgreater(Extended x, Extended y) {
        return (!isunordered(x,y)) && ((x < y) || (x > y));
    }
    extern const Extended ExtendedPositiveInfinity;
    extern const Extended ExtendedNegativeInfinity;
    extern const Extended ExtendedNaN;
#endif
#endif


}


// eval method is 0 for x87, sse, soft, that is float_t has size float and double_t has size double
#define FLT_EVAL_METHOD 0



// Endianity checking
#if __FLOAT_WORD_ORDER == 1234
#define LOW_WORD_IDX 0
#define HIGH_WORD_IDX 1
#elif __FLOAT_WORD_ORDER == 4321
#define LOW_WORD_IDX 1
#define HIGH_WORD_IDX 0
#else
#error unknown byte order
#endif


#ifdef Extended

// little endian
#if __FLOAT_WORD_ORDER == 1234

/// Little endian is fine, always the same offsets whatever the memory model
template<int N> struct ExtendedConverter {
    // Sign and exponent
    static inline SizedUnsignedInteger<16>::Type* sexpPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<16>::Type*>(reinterpret_cast<char*>(e)+8);
    }
    // Mantissa
    static inline SizedUnsignedInteger<32>::Type* mPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<32>::Type*>(e);
    }
};

// Big endian
#elif __FLOAT_WORD_ORDER == 4321
/// Extended is softfloat
template<> struct ExtendedConverter<10> {
    // Sign and exponent
    static inline SizedUnsignedInteger<16>::Type* sexpPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<16>::Type*>(e);
    }
    // Mantissa
    static inline SizedUnsignedInteger<32>::Type* mPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<32>::Type*>(reinterpret_cast<char*>(e)+2);
    }
};

/// Default gcc model for x87 - 32 bits (or with -m96bit-long-double)
template<> struct ExtendedConverter<12> {
    // Sign and exponent
    static inline SizedUnsignedInteger<16>::Type* sexpPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<16>::Type*>(reinterpret_cast<char*>(e)+2);
    }
    // Mantissa
    static inline SizedUnsignedInteger<32>::Type* mPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<32>::Type*>(reinterpret_cast<char*>(e)+4);
    }
};

/// Default gcc model for x87 - 64 bits (or with -m128bit-long-double)
template<> struct ExtendedConverter<16> {
    // Sign and exponent
    static inline SizedUnsignedInteger<16>::Type* sexpPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<16>::Type*>(reinterpret_cast<char*>(e)+6);
    }
    // Mantissa
    static inline SizedUnsignedInteger<32>::Type* mPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<32>::Type*>(reinterpret_cast<char*>(e)+8);
    }
};
#else
#error unknown byte order
#endif

#endif


// Simple
#ifdef LIBM_COMPILING_FLT32

// SSE is best case, always plain types
// X87 uses a wrapper for no denormals case
// First member of struct guaranted aligned at mem location => OK
// Idem for SoftFloat wrapper, though endianity has to be checked explicitly
#define SIMPLE_FROM_INT_PTR(x) *reinterpret_cast<Simple*>(x)
#define CONST_SIMPLE_FROM_INT_PTR(x) *reinterpret_cast<const Simple*>(x)


#define GET_FLOAT_WORD(i,d)                                     \
do {                                                            \
  Simple f = (d);                                               \
  (i) = *reinterpret_cast<streflop::uint32_t*>(&f);                            \
} while (0)


/* Set a float from a 32 bit int.  */
#define SET_FLOAT_WORD(d,i)                                     \
do {                                                            \
    int ii = (i);                                               \
    (d) = *reinterpret_cast<Simple*>(&ii);                      \
} while (0)

#endif

// Double

#ifdef LIBM_COMPILING_DBL64

#define DOUBLE_FROM_INT_PTR(x) *reinterpret_cast<Double*>(x)
#define CONST_DOUBLE_FROM_INT_PTR(x) *reinterpret_cast<const Double*>(x)

#define EXTRACT_WORDS(ix0,ix1,d)            \
do {                                        \
  Double f = (d);                           \
  (ix0) = reinterpret_cast<streflop::uint32_t*>(&f)[HIGH_WORD_IDX];  \
  (ix1) = reinterpret_cast<streflop::uint32_t*>(&f)[LOW_WORD_IDX];      \
} while (0)

#define GET_HIGH_WORD(i,d)          \
do {                                \
  Double f = (d);                           \
  (i) = reinterpret_cast<streflop::uint32_t*>(&f)[HIGH_WORD_IDX];  \
} while (0)

#define GET_LOW_WORD(i,d)           \
do {                                \
  Double f = (d);                           \
  (i) = reinterpret_cast<streflop::uint32_t*>(&f)[LOW_WORD_IDX];      \
} while (0)

#define INSERT_WORDS(d,ix0,ix1)     \
do {                                \
  Double f;                         \
  reinterpret_cast<streflop::uint32_t*>(&f)[HIGH_WORD_IDX] = (ix0);  \
  reinterpret_cast<streflop::uint32_t*>(&f)[LOW_WORD_IDX] = (ix1);      \
  (d) = f; \
} while (0)

#define SET_HIGH_WORD(d,v)          \
do {                                \
  Double f = (d);                   \
  reinterpret_cast<streflop::uint32_t*>(&f)[HIGH_WORD_IDX] = (v);  \
  (d) = f; \
} while (0)

#define SET_LOW_WORD(d,v)           \
do {                                \
  Double f = (d);                   \
  reinterpret_cast<streflop::uint32_t*>(&f)[LOW_WORD_IDX] = (v);  \
  (d) = f; \
} while (0)

#endif

// Extended
#ifdef LIBM_COMPILING_LDBL96

#define EXTENDED_FROM_INT_PTR(x) *reinterpret_cast<Extended*>(x)
#define CONST_EXTENDED_FROM_INT_PTR(x) *reinterpret_cast<const Extended*>(x)

#define GET_LDOUBLE_WORDS(exp,ix0,ix1,d)            \
do {                                \
  Extended f = (d);                           \
    (exp) = *ExtendedConverter<sizeof(Extended)>::sexpPtr(&f); \
    (ix0) = ExtendedConverter<sizeof(Extended)>::mPtr(&f)[HIGH_WORD_IDX]; \
    (ix1) = ExtendedConverter<sizeof(Extended)>::mPtr(&f)[LOW_WORD_IDX]; \
} while (0)

#define SET_LDOUBLE_WORDS(d,exp,ix0,ix1)            \
do {                                \
  Extended f;                         \
    *ExtendedConverter<sizeof(Extended)>::sexpPtr(&f) = (exp); \
    ExtendedConverter<sizeof(Extended)>::mPtr(&f)[HIGH_WORD_IDX] = (ix0); \
    ExtendedConverter<sizeof(Extended)>::mPtr(&f)[LOW_WORD_IDX] = (ix1); \
  (d) = f; \
} while (0)

#define GET_LDOUBLE_MSW(v,d)                    \
do {                                \
  Extended f = (d);                           \
  (v) = ExtendedConverter<sizeof(Extended)>::mPtr(&f)[HIGH_WORD_IDX]; \
} while (0)

#define SET_LDOUBLE_MSW(d,v)                    \
do {                                \
  Extended f = (d);                           \
  ExtendedConverter<sizeof(Extended)>::mPtr(&f)[HIGH_WORD_IDX] = (v); \
  (d) = f;                     \
} while (0)

#define GET_LDOUBLE_EXP(exp,d)                  \
do {                                \
  Extended f = (d);                           \
  (exp) = *ExtendedConverter<sizeof(Extended)>::sexpPtr(&f); \
} while (0)

#define SET_LDOUBLE_EXP(d,exp)                  \
do {                                \
  Extended f = (d);                           \
  *ExtendedConverter<sizeof(Extended)>::sexpPtr(&f) = (exp); \
  (d) = f;                     \
} while (0)

#endif


#ifdef _MSC_VER
// Even if MSVC != STDC, we still need to define it,
// otherwise MSVC chokes on the K&R style function headers.
# define __STDC__
#endif


#endif
