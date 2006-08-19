/*
    streflop: STandalone REproducible FLOating-Point
    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License

    Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP, and IBM MP lib.
    Uses SoftFloat too.

    Please read the history and copyright information in the documentation provided with the source code
*/

#ifndef STREFLOP_SOFT_WRAPPER_H
#define STREFLOP_SOFT_WRAPPER_H

/// This file is independent from SoftFloat itself
/// This way, the user programs are clean from SoftFloat details

/// Only the template declarations are done here
/// The template instanciations for N = 4, 8, 10 are done in the CPP file
/// this way, only these types will have defined symbols

/// This file should be included from within a streflop namespace
}
#include "IntegerTypes.h"
namespace streflop {

// Generic define
template<int Nbits> struct SoftFloatWrapper {

    char holder[(Nbits/STREFLOP_INTEGER_TYPES_CHAR_BITS)];
    template<typename T> inline T& value() {return *reinterpret_cast<T*>(&holder);}
    template<typename T> inline const T& value() const {return *reinterpret_cast<const T*>(&holder);}

    /// Use dummy bool argument for construction from an already initialized holder
    template<typename T> inline SoftFloatWrapper(T init_value, bool) {value<T>() = init_value;}

    /// Uninitialized object
    inline SoftFloatWrapper() {}

    /// Conversion between different types. Also unfortunately includes otherwise perfectly fine copy constructor
    SoftFloatWrapper(const SoftFloatWrapper<32>& f);
    SoftFloatWrapper& operator=(const SoftFloatWrapper<32>& f);
    SoftFloatWrapper(const SoftFloatWrapper<64>& f);
    SoftFloatWrapper& operator=(const SoftFloatWrapper<64>& f);
    SoftFloatWrapper(const SoftFloatWrapper<96>& f);
    SoftFloatWrapper& operator=(const SoftFloatWrapper<96>& f);

    /// Destructor
    inline ~SoftFloatWrapper() {}

    /// Now the real fun, arithmetic operator overloading
    SoftFloatWrapper& operator+=(const SoftFloatWrapper& f);
    SoftFloatWrapper& operator-=(const SoftFloatWrapper& f);
    SoftFloatWrapper& operator*=(const SoftFloatWrapper& f);
    SoftFloatWrapper& operator/=(const SoftFloatWrapper& f);
    bool operator==(const SoftFloatWrapper& f) const;
    bool operator!=(const SoftFloatWrapper& f) const;
    bool operator<(const SoftFloatWrapper& f) const;
    bool operator<=(const SoftFloatWrapper& f) const;
    bool operator>(const SoftFloatWrapper& f) const;
    bool operator>=(const SoftFloatWrapper& f) const;

#define STREFLOP_SOFT_WRAPPER_NATIVE_OPS(native_type) \
    SoftFloatWrapper(const native_type f); \
    SoftFloatWrapper& operator=(const native_type f); \
    operator native_type() const; \
    SoftFloatWrapper& operator+=(const native_type f); \
    SoftFloatWrapper& operator-=(const native_type f); \
    SoftFloatWrapper& operator*=(const native_type f); \
    SoftFloatWrapper& operator/=(const native_type f); \
    bool operator==(const native_type f) const; \
    bool operator!=(const native_type f) const; \
    bool operator<(const native_type f) const; \
    bool operator<=(const native_type f) const; \
    bool operator>(const native_type f) const; \
    bool operator>=(const native_type f) const;

STREFLOP_SOFT_WRAPPER_NATIVE_OPS(float)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(double)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(long double)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(char)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(unsigned char)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(short)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(unsigned short)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(int)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(unsigned int)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(long)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(unsigned long)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(long long)
STREFLOP_SOFT_WRAPPER_NATIVE_OPS(unsigned long long)

};

#define STREFLOP_SOFT_WRAPPER_MAKE_REAL_CLASS_OPS(N) \
template<> bool SoftFloatWrapper<N>::operator<(const SoftFloatWrapper& f) const; \
template<> SoftFloatWrapper<N>& SoftFloatWrapper<N>::operator+=(const SoftFloatWrapper<N>& f); \
template<> SoftFloatWrapper<N>& SoftFloatWrapper<N>::operator-=(const SoftFloatWrapper<N>& f); \
template<> SoftFloatWrapper<N>& SoftFloatWrapper<N>::operator*=(const SoftFloatWrapper<N>& f); \
template<> SoftFloatWrapper<N>& SoftFloatWrapper<N>::operator/=(const SoftFloatWrapper<N>& f); \
template<> bool SoftFloatWrapper<N>::operator==(const SoftFloatWrapper<N>& f) const; \
template<> bool SoftFloatWrapper<N>::operator!=(const SoftFloatWrapper<N>& f) const; \
template<> bool SoftFloatWrapper<N>::operator<(const SoftFloatWrapper<N>& f) const; \
template<> bool SoftFloatWrapper<N>::operator<=(const SoftFloatWrapper<N>& f) const; \
template<> bool SoftFloatWrapper<N>::operator>(const SoftFloatWrapper<N>& f) const; \
template<> bool SoftFloatWrapper<N>::operator>=(const SoftFloatWrapper<N>& f) const;
STREFLOP_SOFT_WRAPPER_MAKE_REAL_CLASS_OPS(32)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_CLASS_OPS(64)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_CLASS_OPS(96)


// Making real the conversion N->M
// Have to include default constructors for template overloading reasons
#define STREFLOP_SOFT_WRAPPER_MAKE_REAL_NM_CONVERSION(Nbits) \
template<> SoftFloatWrapper<Nbits>::SoftFloatWrapper(const SoftFloatWrapper<32>& f); \
template<> SoftFloatWrapper<Nbits>& SoftFloatWrapper<Nbits>::operator=(const SoftFloatWrapper<32>& f); \
template<> SoftFloatWrapper<Nbits>::SoftFloatWrapper(const SoftFloatWrapper<64>& f); \
template<> SoftFloatWrapper<Nbits>& SoftFloatWrapper<Nbits>::operator=(const SoftFloatWrapper<64>& f); \
template<> SoftFloatWrapper<Nbits>::SoftFloatWrapper(const SoftFloatWrapper<96>& f); \
template<> SoftFloatWrapper<Nbits>& SoftFloatWrapper<Nbits>::operator=(const SoftFloatWrapper<96>& f);
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NM_CONVERSION(32)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NM_CONVERSION(64)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NM_CONVERSION(96)


#define STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(N,native_type) \
template<> SoftFloatWrapper<N>::SoftFloatWrapper(const native_type f); \
template<> SoftFloatWrapper<N>& SoftFloatWrapper<N>::operator=(const native_type f); \
template<> SoftFloatWrapper<N>::operator native_type() const; \
template<> SoftFloatWrapper<N>& SoftFloatWrapper<N>::operator+=(const native_type f); \
template<> SoftFloatWrapper<N>& SoftFloatWrapper<N>::operator-=(const native_type f); \
template<> SoftFloatWrapper<N>& SoftFloatWrapper<N>::operator*=(const native_type f); \
template<> SoftFloatWrapper<N>& SoftFloatWrapper<N>::operator/=(const native_type f); \
template<> bool SoftFloatWrapper<N>::operator==(const native_type f) const; \
template<> bool SoftFloatWrapper<N>::operator!=(const native_type f) const; \
template<> bool SoftFloatWrapper<N>::operator<(const native_type f) const; \
template<> bool SoftFloatWrapper<N>::operator<=(const native_type f) const; \
template<> bool SoftFloatWrapper<N>::operator>(const native_type f) const; \
template<> bool SoftFloatWrapper<N>::operator>=(const native_type f) const;

STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,float)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,long double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,unsigned char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,unsigned short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,unsigned int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,unsigned long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(32,unsigned long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,float)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,long double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,unsigned char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,unsigned short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,unsigned int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,unsigned long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(64,unsigned long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,float)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,long double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,unsigned char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,unsigned short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,unsigned int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,unsigned long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_NATIVE_OPS(96,unsigned long long)



// Generic versions are fine here, specializations in the cpp

/// binary operators
template<int N> SoftFloatWrapper<N> operator+(const SoftFloatWrapper<N>& f1, const SoftFloatWrapper<N>& f2);
template<int N> SoftFloatWrapper<N> operator-(const SoftFloatWrapper<N>& f1, const SoftFloatWrapper<N>& f2);
template<int N> SoftFloatWrapper<N> operator*(const SoftFloatWrapper<N>& f1, const SoftFloatWrapper<N>& f2);
template<int N> SoftFloatWrapper<N> operator/(const SoftFloatWrapper<N>& f1, const SoftFloatWrapper<N>& f2);

#define STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_CLASS_OPS(N) \
template<> SoftFloatWrapper<N> operator+(const SoftFloatWrapper<N>& f1, const SoftFloatWrapper<N>& f2); \
template<> SoftFloatWrapper<N> operator-(const SoftFloatWrapper<N>& f1, const SoftFloatWrapper<N>& f2); \
template<> SoftFloatWrapper<N> operator*(const SoftFloatWrapper<N>& f1, const SoftFloatWrapper<N>& f2); \
template<> SoftFloatWrapper<N> operator/(const SoftFloatWrapper<N>& f1, const SoftFloatWrapper<N>& f2);
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_CLASS_OPS(32)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_CLASS_OPS(64)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_CLASS_OPS(96)

#define STREFLOP_SOFT_WRAPPER_BINARY_OPS(native_type) \
template<int N> SoftFloatWrapper<N> operator+(const SoftFloatWrapper<N>& f1, const native_type f2); \
template<int N> SoftFloatWrapper<N> operator-(const SoftFloatWrapper<N>& f1, const native_type f2); \
template<int N> SoftFloatWrapper<N> operator*(const SoftFloatWrapper<N>& f1, const native_type f2); \
template<int N> SoftFloatWrapper<N> operator/(const SoftFloatWrapper<N>& f1, const native_type f2); \
template<int N> SoftFloatWrapper<N> operator+(const native_type f1, const SoftFloatWrapper<N>& f2); \
template<int N> SoftFloatWrapper<N> operator-(const native_type f1, const SoftFloatWrapper<N>& f2); \
template<int N> SoftFloatWrapper<N> operator*(const native_type f1, const SoftFloatWrapper<N>& f2); \
template<int N> SoftFloatWrapper<N> operator/(const native_type f1, const SoftFloatWrapper<N>& f2); \
template<int N> bool operator==(const native_type value, const SoftFloatWrapper<N>& f); \
template<int N> bool operator!=(const native_type value, const SoftFloatWrapper<N>& f); \
template<int N> bool operator<(const native_type value, const SoftFloatWrapper<N>& f); \
template<int N> bool operator<=(const native_type value, const SoftFloatWrapper<N>& f); \
template<int N> bool operator>(const native_type value, const SoftFloatWrapper<N>& f); \
template<int N> bool operator>=(const native_type value, const SoftFloatWrapper<N>& f);

STREFLOP_SOFT_WRAPPER_BINARY_OPS(float)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(double)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(long double)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(char)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(unsigned char)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(short)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(unsigned short)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(int)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(unsigned int)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(long)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(unsigned long)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(long long)
STREFLOP_SOFT_WRAPPER_BINARY_OPS(unsigned long long)


#define STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(N,native_type) \
template<> SoftFloatWrapper<N> operator+(const SoftFloatWrapper<N>& f1, const native_type f2); \
template<> SoftFloatWrapper<N> operator-(const SoftFloatWrapper<N>& f1, const native_type f2); \
template<> SoftFloatWrapper<N> operator*(const SoftFloatWrapper<N>& f1, const native_type f2); \
template<> SoftFloatWrapper<N> operator/(const SoftFloatWrapper<N>& f1, const native_type f2); \
template<> SoftFloatWrapper<N> operator+(const native_type f1, const SoftFloatWrapper<N>& f2); \
template<> SoftFloatWrapper<N> operator-(const native_type f1, const SoftFloatWrapper<N>& f2); \
template<> SoftFloatWrapper<N> operator*(const native_type f1, const SoftFloatWrapper<N>& f2); \
template<> SoftFloatWrapper<N> operator/(const native_type f1, const SoftFloatWrapper<N>& f2); \
template<> bool operator==(const native_type value, const SoftFloatWrapper<N>& f); \
template<> bool operator!=(const native_type value, const SoftFloatWrapper<N>& f); \
template<> bool operator<(const native_type value, const SoftFloatWrapper<N>& f); \
template<> bool operator<=(const native_type value, const SoftFloatWrapper<N>& f); \
template<> bool operator>(const native_type value, const SoftFloatWrapper<N>& f); \
template<> bool operator>=(const native_type value, const SoftFloatWrapper<N>& f);
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,float)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,long double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,unsigned char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,unsigned short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,unsigned int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,unsigned long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(32,unsigned long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,float)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,long double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,unsigned char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,unsigned short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,unsigned int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,unsigned long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(64,unsigned long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,float)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,long double)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,unsigned char)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,unsigned short)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,unsigned int)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,unsigned long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,long long)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_BINARY_OPS(96,unsigned long long)

/// Unary operators
template<int N> SoftFloatWrapper<N> operator-(const SoftFloatWrapper<N>& f);
template<int N> SoftFloatWrapper<N> operator+(const SoftFloatWrapper<N>& f);

#define STREFLOP_SOFT_WRAPPER_MAKE_REAL_UNARY_CLASS_OPS(N) \
template<> SoftFloatWrapper<N> operator-(const SoftFloatWrapper<N>& f); \
template<> SoftFloatWrapper<N> operator+(const SoftFloatWrapper<N>& f);
STREFLOP_SOFT_WRAPPER_MAKE_REAL_UNARY_CLASS_OPS(32)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_UNARY_CLASS_OPS(64)
STREFLOP_SOFT_WRAPPER_MAKE_REAL_UNARY_CLASS_OPS(96)

#endif
