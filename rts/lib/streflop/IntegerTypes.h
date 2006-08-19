/*
    streflop: STandalone REproducible FLOating-Point
    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License

    Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP, and IBM MP lib.
    Uses SoftFloat too.

    Please read the history and copyright information in the documentation provided with the source code
*/

#ifndef STREFLOP_INTEGER_TYPES_H
#define STREFLOP_INTEGER_TYPES_H

// Given that:
// - C++ template metaprogramming is Turing complete
// - types are variables and integers are constants
// - sizeof(type) is known by the C++ compiler and a constant
// - sizeof(char) is 1 by definition, even if char != 8 bits
// Then: It is possible to derive the sized ints at compile time in C++, unlike C
// Note: This is NOT the same as the int32_t, etc from C99, in case char != 8 bits
//       For this reason, redefine the macro below if this is not the case for you
// Note2: Even if char != 8 bits, it's still possible to define ints in terms of number of char!
#define STREFLOP_INTEGER_TYPES_CHAR_BITS 8

// Avoid conflict with system types, if any
namespace streflop {

// Template meta-programming: this is the "program" which variables are types and constants are int template arguments
// Algorithm: provide an expected size, and recursively increase the integer types till the size match
template<typename int_type, int expected_size, bool final_recursion> struct SizedTypeMaker {
};

// start by long long to provide the recursion terminal condition

// false : the expected_size does not exist: do not define a type, there will be a compilation error
template<int expected_size> struct SizedTypeMaker<long long, expected_size, false> {
    // Error: Integer type with expected size does not exist
};

// true: end recursion by defining the correct type to be long long
template<int expected_size> struct SizedTypeMaker<long long, expected_size, true> {
    typedef long long final_type;
};

// false : recurse by increasing the integer type till it reaches the expected size
template<int expected_size> struct SizedTypeMaker<long, expected_size, false> {
    typedef typename SizedTypeMaker<long long, expected_size, (sizeof(long long)==expected_size)>::final_type final_type;
};

// true: end recursion by defining the correct type to be long
template<int expected_size> struct SizedTypeMaker<long, expected_size, true> {
    typedef long final_type;
};

// false : recurse by increasing the integer type till it reaches the expected size
template<int expected_size> struct SizedTypeMaker<int, expected_size, false> {
    typedef typename SizedTypeMaker<long, expected_size, (sizeof(long)==expected_size)>::final_type final_type;
};

// true: end recursion by defining the correct type to be int
template<int expected_size> struct SizedTypeMaker<int, expected_size, true> {
    typedef int final_type;
};

// false : recurse by increasing the integer type till it reaches the expected size
template<int expected_size> struct SizedTypeMaker<short, expected_size, false> {
    typedef typename SizedTypeMaker<int, expected_size, (sizeof(int)==expected_size)>::final_type final_type;
};

// true: end recursion by defining the correct type to be short
template<int expected_size> struct SizedTypeMaker<short, expected_size, true> {
    typedef short final_type;
};


// Do it again for unsigned types

// false : the expected_size does not exist: do not define a type, there will be a compilation error
template<int expected_size> struct SizedTypeMaker<unsigned long long, expected_size, false> {
    // Error: Integer type with expected size does not exist
};

// true: end recursion by defining the correct type to be long long
template<int expected_size> struct SizedTypeMaker<unsigned long long, expected_size, true> {
    typedef unsigned long long final_type;
};

// false : recurse by increasing the integer type till it reaches the expected size
template<int expected_size> struct SizedTypeMaker<unsigned long, expected_size, false> {
    typedef typename SizedTypeMaker<unsigned long long, expected_size, (sizeof(unsigned long long)==expected_size)>::final_type final_type;
};

// true: end recursion by defining the correct type to be long
template<int expected_size> struct SizedTypeMaker<unsigned long, expected_size, true> {
    typedef unsigned long final_type;
};

// false : recurse by increasing the integer type till it reaches the expected size
template<int expected_size> struct SizedTypeMaker<unsigned int, expected_size, false> {
    typedef typename SizedTypeMaker<unsigned long, expected_size, (sizeof(unsigned long)==expected_size)>::final_type final_type;
};

// true: end recursion by defining the correct type to be int
template<int expected_size> struct SizedTypeMaker<unsigned int, expected_size, true> {
    typedef unsigned int final_type;
};

// false : recurse by increasing the integer type till it reaches the expected size
template<int expected_size> struct SizedTypeMaker<unsigned short, expected_size, false> {
    typedef typename SizedTypeMaker<unsigned int, expected_size, (sizeof(unsigned int)==expected_size)>::final_type final_type;
};

// true: end recursion by defining the correct type to be short
template<int expected_size> struct SizedTypeMaker<unsigned short, expected_size, true> {
    typedef unsigned short final_type;
};

// Utility to get an int type with the selected size IN BITS
template<int N> struct SizedInteger {
    typedef typename SizedTypeMaker<short, (N/STREFLOP_INTEGER_TYPES_CHAR_BITS), (sizeof(short)==(N/STREFLOP_INTEGER_TYPES_CHAR_BITS))>::final_type Type;
};
template<int N> struct SizedUnsignedInteger {
    typedef typename SizedTypeMaker<unsigned short, (N/STREFLOP_INTEGER_TYPES_CHAR_BITS), (sizeof(unsigned short)==(N/STREFLOP_INTEGER_TYPES_CHAR_BITS))>::final_type Type;
};

// Specialize for size = STREFLOP_INTEGER_TYPES_CHAR_BITS

template<> struct SizedInteger<STREFLOP_INTEGER_TYPES_CHAR_BITS> {
    typedef char Type;
};

template<> struct SizedUnsignedInteger<STREFLOP_INTEGER_TYPES_CHAR_BITS> {
    typedef unsigned char Type;
};

}

#endif
