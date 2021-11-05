/*
    streflop: STandalone REproducible FLOating-Point
    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License

    Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP, and IBM MP lib.
    Uses SoftFloat too.

    Please read the history and copyright information in the documentation provided with the source code
*/

// Include generic version
#include "streflop.h"

// Macro to select the correct version of a softfloat function according to user flags
#if N_SPECIALIZED == 96

#define SF_PREPEND(func) floatx80 ## func
#define SF_APPEND(func) func ## floatx80
#define SF_TYPE floatx80

#elif N_SPECIALIZED == 64
#define SF_PREPEND(func) float64 ## func
#define SF_APPEND(func) func ## float64
#define SF_TYPE float64

#elif N_SPECIALIZED == 32
#define SF_PREPEND(func) float32 ## func
#define SF_APPEND(func) func ## float32
#define SF_TYPE float32

#else
#error Unknown specialization size (N_SPECIALIZED)
#endif

// This file may include System.h and SoftFloat
#include "System.h"

#include "softfloat/softfloat.h"

namespace streflop {

using namespace streflop::SoftFloat;


// The template instanciations for N = 4, 8, 10 are done here

template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator+=(const SoftFloatWrapper<N_SPECIALIZED>& f) {
    value<SF_TYPE>() = SF_PREPEND(_add)(value<SF_TYPE>(), f.value<SF_TYPE>());
     return *this;
}
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator-=(const SoftFloatWrapper<N_SPECIALIZED>& f) {
    value<SF_TYPE>() = SF_PREPEND(_sub)(value<SF_TYPE>(), f.value<SF_TYPE>());
     return *this;
}
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator*=(const SoftFloatWrapper<N_SPECIALIZED>& f) {
    value<SF_TYPE>() = SF_PREPEND(_mul)(value<SF_TYPE>(), f.value<SF_TYPE>());
     return *this;
}
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator/=(const SoftFloatWrapper<N_SPECIALIZED>& f) {
    value<SF_TYPE>() = SF_PREPEND(_div)(value<SF_TYPE>(), f.value<SF_TYPE>());
     return *this;
}
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator==(const SoftFloatWrapper<N_SPECIALIZED>& f) const {
    return SF_PREPEND(_eq)(value<SF_TYPE>(), f.value<SF_TYPE>());
}
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator!=(const SoftFloatWrapper<N_SPECIALIZED>& f) const {
    // Boolean negation is OK for equality comparison
    return !SF_PREPEND(_eq)(value<SF_TYPE>(), f.value<SF_TYPE>());
}
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator<(const SoftFloatWrapper<N_SPECIALIZED>& f) const {
    return SF_PREPEND(_lt)(value<SF_TYPE>(), f.value<SF_TYPE>());
}
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator<=(const SoftFloatWrapper<N_SPECIALIZED>& f) const {
    return SF_PREPEND(_le)(value<SF_TYPE>(), f.value<SF_TYPE>());
}
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator>(const SoftFloatWrapper<N_SPECIALIZED>& f) const {
    // Take care of NaN, reverse arguments and do NOT take the boolean negation of <=
    return SF_PREPEND(_lt)(f.value<SF_TYPE>(), value<SF_TYPE>());
}
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator>=(const SoftFloatWrapper<N_SPECIALIZED>& f) const {
    // Take care of NaN, reverse arguments and do NOT take the boolean negation of <
    return SF_PREPEND(_le)(f.value<SF_TYPE>(), value<SF_TYPE>());
}


// Use compile-time specialization with template meta-programming
// instead of macros to decide on the correct softfloat function
// => sizeof is useable
// Note: To avoid duplicate symbols, insert a third template argument corresponding to N_SPECIALIZED
//       This is consistent with the use of SF_XXPEND macros
template<int N, typename T, bool is_large> struct IntConverter {
};

// Specialization for large ints > 32 bits
template<typename T> struct IntConverter<N_SPECIALIZED, T, true> {
    static inline SF_TYPE
    convert_from_int(T an_int) {
        return SF_APPEND(int64_to_)((int64_t)an_int);
    }
    static inline T convert_to_int(SF_TYPE value) {
        return (T)SF_PREPEND(_to_int64_round_to_zero)(value);
    }
};

// Specialization for ints <= 32 bits
template<typename T> struct IntConverter<N_SPECIALIZED, T, false> {
    static inline SF_TYPE
    convert_from_int(T an_int) {
        return SF_APPEND(int32_to_)((int32_t)an_int);
    }
    static inline T convert_to_int(SF_TYPE value) {
        return (T)SF_PREPEND(_to_int32_round_to_zero)(value);
    }
};

#define STREFLOP_X87DENORMAL_NATIVE_OPS_INT(native_type) \
template<> SoftFloatWrapper<N_SPECIALIZED>::SoftFloatWrapper(const native_type f) { \
    value<SF_TYPE>() = IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator=(const native_type f) { \
    value<SF_TYPE>() = IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f); \
    return *this; \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>::operator native_type() const { \
    return IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_to_int(value<SF_TYPE>()); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator+=(const native_type f) { \
    value<SF_TYPE>() = SF_PREPEND(_add)(value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f)); \
    return *this; \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator-=(const native_type f) { \
    value<SF_TYPE>() = SF_PREPEND(_sub)(value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f)); \
    return *this; \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator*=(const native_type f) { \
    value<SF_TYPE>() = SF_PREPEND(_mul)(value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f)); \
    return *this; \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator/=(const native_type f) { \
    value<SF_TYPE>() = SF_PREPEND(_div)(value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f)); \
    return *this; \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator==(const native_type f) const { \
    return SF_PREPEND(_eq)(value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f)); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator!=(const native_type f) const { \
    return !SF_PREPEND(_eq)(value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f)); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator<(const native_type f) const { \
    return SF_PREPEND(_lt)(value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f)); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator<=(const native_type f) const { \
    return SF_PREPEND(_le)(value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f)); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator>(const native_type f) const { \
    return SF_PREPEND(_lt)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f), value<SF_TYPE>()); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator>=(const native_type f) const { \
    return SF_PREPEND(_le)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f), value<SF_TYPE>()); \
}

// Now handle the same operations with native float types
// Use the softfloat property of memory pattern equivalence.
// => consider the float as a memory zone, then pass that to the softfloat conversion routines
// => this way, conversion is done by softfloat, not by the FPU
// Use a sizeof trick:
// - Specialize for BOTH C type and the expected type size of C type for correct memory pattern
// - Call the template with sizeof(C type) to rule out mismatching combinations
// - this way, it would be possible to extend the scheme to other architectures
//   Ex: could specialize for <long double, 10>, <long double, 12> and <long double, 16>
// Note: read above note for specialization on N_SPECIALIZED
template<int N, typename ctype, int ctype_size> struct FloatConverter {
};

// dummy wrapers to cover all cases
inline float32 float32_to_float32(float32 a_float) {return a_float;}
inline float64 float64_to_float64(float64 a_float) {return a_float;}
inline floatx80 floatx80_to_floatx80(floatx80 a_float) {return a_float;}

// Specialization for float32 when C float type size is 4
template<> struct FloatConverter<N_SPECIALIZED, float, 4> {
    static inline SF_TYPE
    convert_from_float(const float a_float) {
        return SF_APPEND(float32_to_)(*reinterpret_cast<const float32*>(&a_float));
    }
    static inline float convert_to_float(SF_TYPE value) {
        float32 res = SF_PREPEND(_to_float32)(value);
        return *reinterpret_cast<float*>(&res);
    }
};

// Specialization for double64 when C double type size is 8
template<> struct FloatConverter<N_SPECIALIZED, double, 8> {
    static inline SF_TYPE
    convert_from_float(const double a_float) {
        return SF_APPEND(float64_to_)(*reinterpret_cast<const float64*>(&a_float));
    }
    static inline double convert_to_float(SF_TYPE value) {
        float64 res = SF_PREPEND(_to_float64)(value);
        return *reinterpret_cast<double*>(&res);
    }
};

// Specialization for floatx80 when C long double type size is 12 (there is 16 bit padding, endian dependent)
template<> struct FloatConverter<N_SPECIALIZED, long double, 12> {
// Little endian OK: both address are the same
#if __FLOAT_WORD_ORDER == 1234
    static inline SF_TYPE
    convert_from_float(const long double a_float) {
        return SF_APPEND(floatx80_to_)(*reinterpret_cast<const floatx80*>(&a_float));
    }
    static inline long double convert_to_float(SF_TYPE value) {
        // avoid invalid memory access: must return a 12-bytes value from a 10-byte type
        // do it this way, by declaring the 12-byte on the stack
        long double holder;
        // And use that space for the result using the softfloat memory bit pattern equivalence property
        *reinterpret_cast<floatx80*>(&holder) = SF_PREPEND(_to_floatx80)(value);
        return holder;
    }
// big endian needs address modification, but for what architecture?
#elif __FLOAT_WORD_ORDER == 4321
#warning You are using a completely UNTESTED new architecture. Please check that the 12-byte long double containing a 10-byte float is properly aligned in memory so that softfloat may correctly read the bit pattern. If this works for you, remove this warning and please consider sending a patch!
    static inline SF_TYPE
    convert_from_float(const long double a_float) {
        return SF_APPEND(floatx80_to_)(*reinterpret_cast<const floatx80*>(reinterpret_cast<const char*>(&a_float)+2));
    }
    static inline long double convert_to_float(SF_TYPE value) {
        // avoid invalid memory access: must return a 12-bytes value from a 10-byte type
        // do it this way, by declaring the 12-byte on the stack
        long double holder;
        // And use that space for the result using the softfloat memory bit pattern equivalence property
        *reinterpret_cast<floatx80*>(reinterpret_cast<const char*>(&holder)+2) = SF_PREPEND(_to_floatx80)(value);
        return holder;
    }
#else
#error Unknown byte order
#endif
};

// Specialization for floatx80 when C long double type size is 16. This is the case for g++ using -m128bit-long-double, which is itself the default on x86_64
template<> struct FloatConverter<N_SPECIALIZED, long double, 16> {
// Little endian OK: both address are the same
#if __FLOAT_WORD_ORDER == 1234
    static inline SF_TYPE
    convert_from_float(const long double a_float) {
        return SF_APPEND(floatx80_to_)(*reinterpret_cast<const floatx80*>(&a_float));
    }
    static inline long double convert_to_float(SF_TYPE value) {
        // avoid invalid memory access: must return a 16-bytes value from a 10-byte type
        // do it this way, by declaring the 16-byte on the stack
        long double holder;
        // And use that space for the result using the softfloat memory bit pattern equivalence property
        *reinterpret_cast<floatx80*>(&holder) = SF_PREPEND(_to_floatx80)(value);
        return holder;
    }
// big endian needs address modification, but for what architecture?
#elif __FLOAT_WORD_ORDER == 4321
#warning You are using a completely UNTESTED new architecture. Please check that the 16-byte long double containing a 10-byte float is properly aligned in memory so that softfloat may correctly read the bit pattern. If this works for you, remove this warning and please consider sending a patch!
    static inline SF_TYPE
    convert_from_float(const long double a_float) {
        return SF_APPEND(floatx80_to_)(*reinterpret_cast<const floatx80*>(reinterpret_cast<const char*>(&a_float)+6));
    }
    static inline long double convert_to_float(SF_TYPE value) {
        // avoid invalid memory access: must return a 12-bytes value from a 10-byte type
        // do it this way, by declaring the 12-byte on the stack
        long double holder;
        // And use that space for the result using the softfloat memory bit pattern equivalence property
        *reinterpret_cast<floatx80*>(reinterpret_cast<const char*>(&holder)+6) = SF_PREPEND(_to_floatx80)(value);
        return holder;
    }
#else
#error Unknown byte order
#endif
};


#define STREFLOP_X87DENORMAL_NATIVE_OPS_FLOAT(native_type) \
template<> SoftFloatWrapper<N_SPECIALIZED>::SoftFloatWrapper(const native_type f) { \
    value<SF_TYPE>() = FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator=(const native_type f) { \
    value<SF_TYPE>() = FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f); \
    return *this; \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>::operator native_type() const { \
    return FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_to_float(value<SF_TYPE>()); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator+=(const native_type f) { \
    value<SF_TYPE>() = SF_PREPEND(_add)(value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f)); \
    return *this; \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator-=(const native_type f) { \
    value<SF_TYPE>() = SF_PREPEND(_sub)(value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f)); \
    return *this; \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator*=(const native_type f) { \
    value<SF_TYPE>() = SF_PREPEND(_mul)(value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f)); \
    return *this; \
} \
template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator/=(const native_type f) { \
    value<SF_TYPE>() = SF_PREPEND(_div)(value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f)); \
    return *this; \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator==(const native_type f) const { \
    return SF_PREPEND(_eq)(value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f)); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator!=(const native_type f) const { \
    return !SF_PREPEND(_eq)(value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f)); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator<(const native_type f) const { \
    return SF_PREPEND(_lt)(value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f)); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator<=(const native_type f) const { \
    return SF_PREPEND(_le)(value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f)); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator>(const native_type f) const { \
    return SF_PREPEND(_lt)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f), value<SF_TYPE>()); \
} \
template<> bool SoftFloatWrapper<N_SPECIALIZED>::operator>=(const native_type f) const { \
    return SF_PREPEND(_le)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f), value<SF_TYPE>()); \
}

STREFLOP_X87DENORMAL_NATIVE_OPS_INT(char)
STREFLOP_X87DENORMAL_NATIVE_OPS_INT(unsigned char)
STREFLOP_X87DENORMAL_NATIVE_OPS_INT(short)
STREFLOP_X87DENORMAL_NATIVE_OPS_INT(unsigned short)
STREFLOP_X87DENORMAL_NATIVE_OPS_INT(int)
STREFLOP_X87DENORMAL_NATIVE_OPS_INT(unsigned int)
STREFLOP_X87DENORMAL_NATIVE_OPS_INT(long)
STREFLOP_X87DENORMAL_NATIVE_OPS_INT(unsigned long)
STREFLOP_X87DENORMAL_NATIVE_OPS_INT(long long)
STREFLOP_X87DENORMAL_NATIVE_OPS_INT(unsigned long long)

STREFLOP_X87DENORMAL_NATIVE_OPS_FLOAT(float)
STREFLOP_X87DENORMAL_NATIVE_OPS_FLOAT(double)
STREFLOP_X87DENORMAL_NATIVE_OPS_FLOAT(long double)

/// binary operators
/// use dummy argument factories to distinguish from integer conversion and avoid creating temporary object
template<> SoftFloatWrapper<N_SPECIALIZED> operator+(const SoftFloatWrapper<N_SPECIALIZED>& f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) {
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_add)(f1.value<SF_TYPE>(), f2.value<SF_TYPE>()), true);
}
template<> SoftFloatWrapper<N_SPECIALIZED> operator-(const SoftFloatWrapper<N_SPECIALIZED>& f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) {
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_sub)(f1.value<SF_TYPE>(), f2.value<SF_TYPE>()), true);
}
template<> SoftFloatWrapper<N_SPECIALIZED> operator*(const SoftFloatWrapper<N_SPECIALIZED>& f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) {
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_mul)(f1.value<SF_TYPE>(), f2.value<SF_TYPE>()), true);
}
template<> SoftFloatWrapper<N_SPECIALIZED> operator/(const SoftFloatWrapper<N_SPECIALIZED>& f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) {
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_div)(f1.value<SF_TYPE>(), f2.value<SF_TYPE>()), true);
}

#define STREFLOP_X87DENORMAL_BINARY_OPS_INT(native_type) \
template<> SoftFloatWrapper<N_SPECIALIZED> operator+(const SoftFloatWrapper<N_SPECIALIZED>& f1, const native_type f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_add)(f1.value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f2)), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator-(const SoftFloatWrapper<N_SPECIALIZED>& f1, const native_type f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_sub)(f1.value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f2)), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator*(const SoftFloatWrapper<N_SPECIALIZED>& f1, const native_type f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_mul)(f1.value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f2)), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator/(const SoftFloatWrapper<N_SPECIALIZED>& f1, const native_type f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_div)(f1.value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f2)), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator+(const native_type f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_add)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f1), f2.value<SF_TYPE>()), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator-(const native_type f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_sub)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f1), f2.value<SF_TYPE>()), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator*(const native_type f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_mul)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f1), f2.value<SF_TYPE>()), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator/(const native_type f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_div)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(f1), f2.value<SF_TYPE>()), true); \
} \
template<> bool operator==(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_eq)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(value), f.value<SF_TYPE>()); \
} \
template<> bool operator!=(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return !SF_PREPEND(_eq)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(value), f.value<SF_TYPE>()); \
} \
template<> bool operator<(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_lt)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(value), f.value<SF_TYPE>()); \
} \
template<> bool operator<=(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_le)(IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(value), f.value<SF_TYPE>()); \
} \
template<> bool operator>(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_lt)(f.value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(value)); \
} \
template<> bool operator>=(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_le)(f.value<SF_TYPE>(), IntConverter< N_SPECIALIZED, native_type, (sizeof(native_type)>4) >::convert_from_int(value)); \
}


#define STREFLOP_X87DENORMAL_BINARY_OPS_FLOAT(native_type) \
template<> SoftFloatWrapper<N_SPECIALIZED> operator+(const SoftFloatWrapper<N_SPECIALIZED>& f1, const native_type f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_add)(f1.value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f2)), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator-(const SoftFloatWrapper<N_SPECIALIZED>& f1, const native_type f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_sub)(f1.value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f2)), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator*(const SoftFloatWrapper<N_SPECIALIZED>& f1, const native_type f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_mul)(f1.value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f2)), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator/(const SoftFloatWrapper<N_SPECIALIZED>& f1, const native_type f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_div)(f1.value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f2)), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator+(const native_type f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_add)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f1), f2.value<SF_TYPE>()), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator-(const native_type f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_sub)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f1), f2.value<SF_TYPE>()), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator*(const native_type f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_mul)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f1), f2.value<SF_TYPE>()), true); \
} \
template<> SoftFloatWrapper<N_SPECIALIZED> operator/(const native_type f1, const SoftFloatWrapper<N_SPECIALIZED>& f2) { \
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_div)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(f1), f2.value<SF_TYPE>()), true); \
} \
template<> bool operator==(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_eq)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(value), f.value<SF_TYPE>()); \
} \
template<> bool operator!=(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return !SF_PREPEND(_eq)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(value), f.value<SF_TYPE>()); \
} \
template<> bool operator<(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_lt)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(value), f.value<SF_TYPE>()); \
} \
template<> bool operator<=(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_le)(FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(value), f.value<SF_TYPE>()); \
} \
template<> bool operator>(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_lt)(f.value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(value)); \
} \
template<> bool operator>=(const native_type value, const SoftFloatWrapper<N_SPECIALIZED>& f) { \
    return SF_PREPEND(_le)(f.value<SF_TYPE>(), FloatConverter< N_SPECIALIZED, native_type, sizeof(native_type)>::convert_from_float(value)); \
}

STREFLOP_X87DENORMAL_BINARY_OPS_INT(char)
STREFLOP_X87DENORMAL_BINARY_OPS_INT(unsigned char)
STREFLOP_X87DENORMAL_BINARY_OPS_INT(short)
STREFLOP_X87DENORMAL_BINARY_OPS_INT(unsigned short)
STREFLOP_X87DENORMAL_BINARY_OPS_INT(int)
STREFLOP_X87DENORMAL_BINARY_OPS_INT(unsigned int)
STREFLOP_X87DENORMAL_BINARY_OPS_INT(long)
STREFLOP_X87DENORMAL_BINARY_OPS_INT(unsigned long)
STREFLOP_X87DENORMAL_BINARY_OPS_INT(long long)
STREFLOP_X87DENORMAL_BINARY_OPS_INT(unsigned long long)

STREFLOP_X87DENORMAL_BINARY_OPS_FLOAT(float)
STREFLOP_X87DENORMAL_BINARY_OPS_FLOAT(double)
STREFLOP_X87DENORMAL_BINARY_OPS_FLOAT(long double)

/// Unary operators
template<> SoftFloatWrapper<N_SPECIALIZED> operator-(const SoftFloatWrapper<N_SPECIALIZED>& f) {
    // We could do it right here by flipping the bit sign
    // However, there is the exceptions handling and such, so...
    return SoftFloatWrapper<N_SPECIALIZED>(SF_PREPEND(_sub)(SF_APPEND(int32_to_)(0), f.value<SF_TYPE>()), true);
}
template<> SoftFloatWrapper<N_SPECIALIZED> operator+(const SoftFloatWrapper<N_SPECIALIZED>& f) {
    return f; // makes a copy
}


template<> SoftFloatWrapper<N_SPECIALIZED>::SoftFloatWrapper(const SoftFloatWrapper<32>& f) {
    value<SF_TYPE>() = SF_APPEND(float32_to_)(f.value<float32>());
}

template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator=(const SoftFloatWrapper<32>& f) {
    value<SF_TYPE>() = SF_APPEND(float32_to_)(f.value<float32>());
    return *this;
}

template<> SoftFloatWrapper<N_SPECIALIZED>::SoftFloatWrapper(const SoftFloatWrapper<64>& f) {
    value<SF_TYPE>() = SF_APPEND(float64_to_)(f.value<float64>());
}

template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator=(const SoftFloatWrapper<64>& f) {
    value<SF_TYPE>() = SF_APPEND(float64_to_)(f.value<float64>());
    return *this;
}

template<> SoftFloatWrapper<N_SPECIALIZED>::SoftFloatWrapper(const SoftFloatWrapper<96>& f) {
    value<SF_TYPE>() = SF_APPEND(floatx80_to_)(f.value<floatx80>());
}

template<> SoftFloatWrapper<N_SPECIALIZED>& SoftFloatWrapper<N_SPECIALIZED>::operator=(const SoftFloatWrapper<96>& f) {
    value<SF_TYPE>() = SF_APPEND(floatx80_to_)(f.value<floatx80>());
    return *this;
}

} // end of namespace
