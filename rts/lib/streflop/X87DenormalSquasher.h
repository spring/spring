/*
    streflop: STandalone REproducible FLOating-Point
    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License

    Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP, and IBM MP lib.
    Uses SoftFloat too.

    Please read the history and copyright information in the documentation provided with the source code
*/

#ifndef X87DenormalSquasher_H
#define X87DenormalSquasher_H

/// This file should be included from within a streflop namespace

template<typename ValueType> inline void X87DenormalSquashFunction(ValueType& value) {
    struct X {};
    X Unknown_numeric_type;
    // unknown types do not compile
    value = Unknown_numeric_type;
}

// Use cmov instruction to avoid branching?
// Pro: branching may be costly, though the branching predictor may compensate when there are few denormals
// Con: cmov forces an unconditional writeback to the mem just after read, which may be worse than the branch

template<> inline void X87DenormalSquashFunction<float>(float& value) {
    if ((reinterpret_cast<int*>(&value)[0] & 0x7F800000) == 0) value = 0.0f;
}

template<> inline void X87DenormalSquashFunction<double>(double& value) {
    if ((reinterpret_cast<int*>(&value)[1] & 0x7FF00000) == 0) value = 0.0;
}

template<> inline void X87DenormalSquashFunction<long double>(long double& value) {
    if ((reinterpret_cast<short*>(&value)[4] & 0x7FFF) == 0) value = 0.0L;
}

/// Wrapper class for the denormal squashing of X87
/// The goal is to be able to use this file for normal arithmetic operations
/// as if this was a native float or double, thanks to C++ operator overloading.
/// In particular, the GNU libm may be compiled this way, though no effort was made
/// to optimize out the denormal parts (and squashing denormals is non-IEEE compliant anyway)

template<typename ValueType> struct X87DenormalSquasher {

// Define value
    ValueType value;

    /// Uninitialized object
    inline X87DenormalSquasher() {}

    /// Copy and conversion from other precisions
    inline X87DenormalSquasher(const X87DenormalSquasher<float>& f) : value(f.value) {X87DenormalSquashFunction<ValueType>(value);}
    inline X87DenormalSquasher(const X87DenormalSquasher<double>& f) : value(f.value) {X87DenormalSquashFunction<ValueType>(value);}
    inline X87DenormalSquasher(const X87DenormalSquasher<long double>& f) : value(f.value) {X87DenormalSquashFunction<ValueType>(value);}
    inline X87DenormalSquasher& operator=(const X87DenormalSquasher<float>& f) {value = f.value; X87DenormalSquashFunction<ValueType>(value); return *this;} // self-ref OK
    inline X87DenormalSquasher& operator=(const X87DenormalSquasher<double>& f) {value = f.value; X87DenormalSquashFunction<ValueType>(value); return *this;} // self-ref OK
    inline X87DenormalSquasher& operator=(const X87DenormalSquasher<long double>& f) {value = f.value; X87DenormalSquashFunction<ValueType>(value); return *this;} // self-ref OK

    /// Destructor
    inline ~X87DenormalSquasher() {}

    /// Now the real fun, arithmetic operator overloading
    inline X87DenormalSquasher& operator+=(const X87DenormalSquasher& f) {value += f.value; X87DenormalSquashFunction<ValueType>(value); return *this;}
    inline X87DenormalSquasher& operator-=(const X87DenormalSquasher& f) {value -= f.value; X87DenormalSquashFunction<ValueType>(value); return *this;}
    inline X87DenormalSquasher& operator*=(const X87DenormalSquasher& f) {value *= f.value; X87DenormalSquashFunction<ValueType>(value); return *this;}
    inline X87DenormalSquasher& operator/=(const X87DenormalSquasher& f) {value /= f.value; X87DenormalSquashFunction<ValueType>(value); return *this;}
    inline bool operator==(const X87DenormalSquasher& f) const {return value == f.value;}
    inline bool operator!=(const X87DenormalSquasher& f) const {return value != f.value;}
    inline bool operator<(const X87DenormalSquasher& f) const {return value < f.value;}
    inline bool operator<=(const X87DenormalSquasher& f) const {return value <= f.value;}
    inline bool operator>(const X87DenormalSquasher& f) const {return value > f.value;}
    inline bool operator>=(const X87DenormalSquasher& f) const {return value >= f.value;}

#define STREFLOP_X87DENORMAL_NATIVE_FLOAT_OPS(native_type) \
    inline X87DenormalSquasher(const native_type f) {value = f; X87DenormalSquashFunction<ValueType>(value);} \
    inline X87DenormalSquasher& operator=(const native_type f) {value = f; X87DenormalSquashFunction<ValueType>(value); return *this;} \
    inline operator native_type() const {return static_cast<native_type>(value);} \
    inline X87DenormalSquasher& operator+=(const native_type f) {return operator+=(X87DenormalSquasher(f));} \
    inline X87DenormalSquasher& operator-=(const native_type f) {return operator-=(X87DenormalSquasher(f));} \
    inline X87DenormalSquasher& operator*=(const native_type f) {return operator*=(X87DenormalSquasher(f));} \
    inline X87DenormalSquasher& operator/=(const native_type f) {return operator/=(X87DenormalSquasher(f));} \
    inline bool operator==(const native_type f) const {return operator==(X87DenormalSquasher(f));} \
    inline bool operator!=(const native_type f) const {return operator!=(X87DenormalSquasher(f));} \
    inline bool operator<(const native_type f) const {return operator<(X87DenormalSquasher(f));} \
    inline bool operator<=(const native_type f) const {return operator<=(X87DenormalSquasher(f));} \
    inline bool operator>(const native_type f) const {return operator>(X87DenormalSquasher(f));} \
    inline bool operator>=(const native_type f) const {return operator>=(X87DenormalSquasher(f));}

#define STREFLOP_X87DENORMAL_NATIVE_INT_OPS(native_type) \
    inline X87DenormalSquasher(const native_type x) {value = x;} \
    inline X87DenormalSquasher& operator=(const native_type x) {value = x; return *this;} \
    inline operator native_type() const {return static_cast<native_type>(value);} \
    inline X87DenormalSquasher& operator+=(const native_type f) {value += f; X87DenormalSquashFunction<ValueType>(value); return *this;} \
    inline X87DenormalSquasher& operator-=(const native_type f) {value -= f; X87DenormalSquashFunction<ValueType>(value); return *this;} \
    inline X87DenormalSquasher& operator*=(const native_type f) {value *= f; X87DenormalSquashFunction<ValueType>(value); return *this;} \
    inline X87DenormalSquasher& operator/=(const native_type f) {value /= f; X87DenormalSquashFunction<ValueType>(value); return *this;} \
    inline bool operator==(const native_type f) const {return value == f;} \
    inline bool operator!=(const native_type f) const {return value != f;} \
    inline bool operator<(const native_type f) const {return value < f;} \
    inline bool operator<=(const native_type f) const {return value <= f;} \
    inline bool operator>(const native_type f) const {return value > f;} \
    inline bool operator>=(const native_type f) const {return value >= f;}

STREFLOP_X87DENORMAL_NATIVE_FLOAT_OPS(float)
STREFLOP_X87DENORMAL_NATIVE_FLOAT_OPS(double)
STREFLOP_X87DENORMAL_NATIVE_FLOAT_OPS(long double)

STREFLOP_X87DENORMAL_NATIVE_INT_OPS(char)
STREFLOP_X87DENORMAL_NATIVE_INT_OPS(unsigned char)
STREFLOP_X87DENORMAL_NATIVE_INT_OPS(short)
STREFLOP_X87DENORMAL_NATIVE_INT_OPS(unsigned short)
STREFLOP_X87DENORMAL_NATIVE_INT_OPS(int)
STREFLOP_X87DENORMAL_NATIVE_INT_OPS(unsigned int)
STREFLOP_X87DENORMAL_NATIVE_INT_OPS(long)
STREFLOP_X87DENORMAL_NATIVE_INT_OPS(unsigned long)
STREFLOP_X87DENORMAL_NATIVE_INT_OPS(long long)
STREFLOP_X87DENORMAL_NATIVE_INT_OPS(unsigned long long)

};

/// binary operators
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator+(const X87DenormalSquasher<ValueType>& f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1.value + f2.value);}
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator-(const X87DenormalSquasher<ValueType>& f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1.value - f2.value);}
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator*(const X87DenormalSquasher<ValueType>& f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1.value * f2.value);}
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator/(const X87DenormalSquasher<ValueType>& f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1.value / f2.value);}

#define STREFLOP_X87DENORMAL_FLOAT_BINARY(native_type) \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator+(const X87DenormalSquasher<ValueType>& f1, const native_type f2) {return f1 + X87DenormalSquasher<ValueType>(f2);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator-(const X87DenormalSquasher<ValueType>& f1, const native_type f2) {return f1 - X87DenormalSquasher<ValueType>(f2);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator*(const X87DenormalSquasher<ValueType>& f1, const native_type f2) {return f1 * X87DenormalSquasher<ValueType>(f2);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator/(const X87DenormalSquasher<ValueType>& f1, const native_type f2) {return f1 / X87DenormalSquasher<ValueType>(f2);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator+(const native_type f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1) + f2;} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator-(const native_type f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1) - f2;} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator*(const native_type f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1) * f2;} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator/(const native_type f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1) / f2;} \
template<typename ValueType> inline bool operator==(const native_type value, const X87DenormalSquasher<ValueType>& f) {return X87DenormalSquasher<ValueType>(value) == f;} \
template<typename ValueType> inline bool operator!=(const native_type value, const X87DenormalSquasher<ValueType>& f) {return X87DenormalSquasher<ValueType>(value) != f;} \
template<typename ValueType> inline bool operator<(const native_type value, const X87DenormalSquasher<ValueType>& f) {return X87DenormalSquasher<ValueType>(value) < f;} \
template<typename ValueType> inline bool operator<=(const native_type value, const X87DenormalSquasher<ValueType>& f) {return X87DenormalSquasher<ValueType>(value) <= f;} \
template<typename ValueType> inline bool operator>(const native_type value, const X87DenormalSquasher<ValueType>& f) {return X87DenormalSquasher<ValueType>(value) > f;} \
template<typename ValueType> inline bool operator>=(const native_type value, const X87DenormalSquasher<ValueType>& f) {return X87DenormalSquasher<ValueType>(value) >= f;}

#define STREFLOP_X87DENORMAL_INT_BINARY(native_type) \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator+(const X87DenormalSquasher<ValueType>& f1, const native_type f2) {return X87DenormalSquasher<ValueType>(f1.value + f2);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator-(const X87DenormalSquasher<ValueType>& f1, const native_type f2) {return X87DenormalSquasher<ValueType>(f1.value - f2);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator*(const X87DenormalSquasher<ValueType>& f1, const native_type f2) {return X87DenormalSquasher<ValueType>(f1.value * f2);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator/(const X87DenormalSquasher<ValueType>& f1, const native_type f2) {return X87DenormalSquasher<ValueType>(f1.value / f2);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator+(const native_type f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1 + f2.value);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator-(const native_type f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1 - f2.value);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator*(const native_type f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1 * f2.value);} \
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator/(const native_type f1, const X87DenormalSquasher<ValueType>& f2) {return X87DenormalSquasher<ValueType>(f1 / f2.value);} \
template<typename ValueType> inline bool operator==(const native_type value, const X87DenormalSquasher<ValueType>& f) {return value == f.value;} \
template<typename ValueType> inline bool operator!=(const native_type value, const X87DenormalSquasher<ValueType>& f) {return value != f.value;} \
template<typename ValueType> inline bool operator<(const native_type value, const X87DenormalSquasher<ValueType>& f) {return value < f.value;} \
template<typename ValueType> inline bool operator<=(const native_type value, const X87DenormalSquasher<ValueType>& f) {return value <= f.value;} \
template<typename ValueType> inline bool operator>(const native_type value, const X87DenormalSquasher<ValueType>& f) {return value > f.value;} \
template<typename ValueType> inline bool operator>=(const native_type value, const X87DenormalSquasher<ValueType>& f) {return value >= f.value;}

STREFLOP_X87DENORMAL_FLOAT_BINARY(float)
STREFLOP_X87DENORMAL_FLOAT_BINARY(double)
STREFLOP_X87DENORMAL_FLOAT_BINARY(long double)

STREFLOP_X87DENORMAL_INT_BINARY(char)
STREFLOP_X87DENORMAL_INT_BINARY(unsigned char)
STREFLOP_X87DENORMAL_INT_BINARY(short)
STREFLOP_X87DENORMAL_INT_BINARY(unsigned short)
STREFLOP_X87DENORMAL_INT_BINARY(int)
STREFLOP_X87DENORMAL_INT_BINARY(unsigned int)
STREFLOP_X87DENORMAL_INT_BINARY(long)
STREFLOP_X87DENORMAL_INT_BINARY(unsigned long)
STREFLOP_X87DENORMAL_INT_BINARY(long long)
STREFLOP_X87DENORMAL_INT_BINARY(unsigned long long)

/// Unary operators
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator-(const X87DenormalSquasher<ValueType>& f) {return X87DenormalSquasher<ValueType>(-f.value);}
template<typename ValueType> inline X87DenormalSquasher<ValueType> operator+(const X87DenormalSquasher<ValueType>& f) {return X87DenormalSquasher<ValueType>(+f.value);}

/*
/// Stream operators for output
/// Warning! The SYSTEM operators are used to the normal floats and therefore
/// this function may not be reliable (they may depend on system rounding mode, etc.)
/// Still useful for debug output and to report results with a few significant digits
template<typename ValueType> inline std::ostream& operator<<(std::ostream& out, const X87DenormalSquasher<ValueType>& f) {
    out << f.value;
    return out;
}
template<typename ValueType> inline std::istream& operator>>(std::istream& in, X87DenormalSquasher<ValueType>& f) {
    in >> f.value;
    return in;
}
*/

#endif
