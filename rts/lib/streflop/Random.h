/*
    streflop: STandalone REproducible FLOating-Point
    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License

    Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP, and IBM MP lib.
    Uses SoftFloat too.

    Please read the history and copyright information in the documentation provided with the source code
*/

#ifndef RANDOM_H
#define RANDOM_H

// Need sized integer types, which are now system-independent thanks to template metaprogramming
#include "IntegerTypes.h"

namespace streflop {

/** Random state holder object
    Declare one of this per thread, and use it as context to the random functions
    Object is properly set by the RandomInit functions
    This allows it to remain POD type
*/
struct RandomState {
#if !defined(STREFLOP_RANDOM_GEN_SIZE)
#define STREFLOP_RANDOM_GEN_SIZE 32
#endif
    // state vector
    SizedUnsignedInteger<STREFLOP_RANDOM_GEN_SIZE>::Type mt[19968/STREFLOP_RANDOM_GEN_SIZE];
    int mti;
    // random seed that was used for initialization
    SizedUnsignedInteger<32>::Type seed;
}
#ifdef __GNUC__
__attribute__ ((aligned (64))) // align state vector on cache line size
#endif
;

/// Default random state holder
extern RandomState DefaultRandomState;

/** Initialize the random number generator with the given seed.

    By default, the seed is taken from system time and printed out
    so the experiment is reproducible by inputing the same seed again.

    You can set here a previous seed to reproduce it.

    This interface allows independance from the actual RNG used,
    and/or system functions.

    The RNG used is the Mersenne twister implementation by the
    original authors Takuji Nishimura and Makoto Matsumoto.

    See also Random.cpp for more information.
*/
SizedUnsignedInteger<32>::Type RandomInit(RandomState& state = DefaultRandomState);
SizedUnsignedInteger<32>::Type RandomInit(SizedUnsignedInteger<32>::Type seed, RandomState& state = DefaultRandomState);

/// Returns the random seed that was used for the initialization
/// Defaults to 0 if the RNG is not yet initialized
SizedUnsignedInteger<32>::Type RandomSeed(RandomState& state = DefaultRandomState);

/** Returns a random number from a uniform distribution.

    All integer types are supported, as well as Simple, Double, and Extended

    The Random(min, max) template takes as argument:
    - bool: whether or not including the min bound
    - bool: whether or not including the max bound
    - type: to generate a number from that type, and decide on min/max bounds
    Example: Double x = Random<true, false, Double>(7.0, 18.0)
             This will return a Double number between 7.0 (included) and 18.0 (excluded)
    This works for both float and integer types.

    Aliases are named like RandomXY with X,Y = E,I for bounds Excluded,Included.
    Example: RandomEI(min,max) will return a number between min (excluded) and max (included).

    The Random() template returns a number of the given type chosen uniformly between
    all representable numbers of that type.
    - For integer types, this means what you expect: any int, char, whatever on the whole range
    - For float types, just recall that there are as many representable floats in each range
      2^x - 2^(x+1), this is what floating-point means

    Notes:
    - If min > max then the result is undefined.
    - If you ask for an empty interval (like RandomEE with min==max) then the result is undefined.
    - If a NaN value is passed to the float functions, NaN is returned.
    - By order of performance, for float types, IE is fastest, then EI, then EE, then II. For integer
    types, it happens that the order is II, IE and EI ex-aequo, and EE, but the difference is much
    less pronounced than for the float types. Use IE preferably for floats, II for ints.

    The floating-point functions always compute a random number with the maximum digits of precision,
    taking care of bounds. That is, it uses the 1-2 interval as this is a power-of-two bounded interval,
    so each representable number in that interval (matching a distinct bit pattern) is given exactly
    the same weight. This is really a truly uniform distribution, unlike the 0-1 range (see note below).
    That 1-2 interval is then converted to the min-max range, hopefully resulting in the loss of as
    few random bits as possible. See also the additional functions below for better performance.

    Note: Getting numbers in the 0-1 interval may be tricky. Half the 2^X exponents are in that range as
    well as denormal numbers. This could result in an horrible loss of bits when scaling from one exponent
    to another. Fortunately, the numbers in 1-2 are generated with maximum precision, and then subtracting 1.0
    to get a number in 0-1 keeps that precision because all the numbers obtained this way are still perfectly
    representable. Moreover, the minimum exponent reached this way is still far above the denormals range.

    Note3: The random number generator MUST be initialized for this function to work correctly.

    Note4: These functions are thread-safe if you use one RandomState object per thread (or if you
    synchronize the access to a shared state object, of course).
*/

template<bool include_min, bool include_max, typename a_type> a_type Random(a_type min, a_type max, RandomState& state = DefaultRandomState);
// function that returns a random number on the whole possible range
template<typename a_type> a_type Random(RandomState& state = DefaultRandomState);
// Alias that can be useful too
template<typename a_type> inline a_type RandomIE(a_type min, a_type max, RandomState& state = DefaultRandomState) {return Random<true, false, a_type>(min, max, state);}
template<typename a_type> inline a_type RandomEI(a_type min, a_type max, RandomState& state = DefaultRandomState) {return Random<false, true, a_type>(min, max, state);}
template<typename a_type> inline a_type RandomEE(a_type min, a_type max, RandomState& state = DefaultRandomState) {return Random<false, false, a_type>(min, max, state);}
template<typename a_type> inline a_type RandomII(a_type min, a_type max, RandomState& state = DefaultRandomState) {return Random<true, true, a_type>(min, max, state);}
#define STREFLOP_RANDOM_MAKE_REAL(a_type) \
template<> a_type Random<a_type>(RandomState& state); \
template<> a_type Random<true, true, a_type>(a_type min, a_type max, RandomState& state); \
template<> a_type Random<true, false, a_type>(a_type min, a_type max, RandomState& state); \
template<> a_type Random<false, true, a_type>(a_type min, a_type max, RandomState& state); \
template<> a_type Random<false, false, a_type>(a_type min, a_type max, RandomState& state);

STREFLOP_RANDOM_MAKE_REAL(char)
STREFLOP_RANDOM_MAKE_REAL(unsigned char)
STREFLOP_RANDOM_MAKE_REAL(short)
STREFLOP_RANDOM_MAKE_REAL(unsigned short)
STREFLOP_RANDOM_MAKE_REAL(int)
STREFLOP_RANDOM_MAKE_REAL(unsigned int)
STREFLOP_RANDOM_MAKE_REAL(long)
STREFLOP_RANDOM_MAKE_REAL(unsigned long)
STREFLOP_RANDOM_MAKE_REAL(long long)
STREFLOP_RANDOM_MAKE_REAL(unsigned long long)


/** Additional and faster functions for real numbers
    These return a number in the 1..2 range, the base for all the other random functions
*/
template<bool include_min, bool include_max, typename a_type> a_type Random12(RandomState& state = DefaultRandomState);
// Alias that can be useful too
template<typename a_type> inline a_type Random12IE(RandomState& state = DefaultRandomState) {return Random12<true, false, a_type>(state);}
template<typename a_type> inline a_type Random12EI(RandomState& state = DefaultRandomState) {return Random12<false, true, a_type>(state);}
template<typename a_type> inline a_type Random12EE(RandomState& state = DefaultRandomState) {return Random12<false, false, a_type>(state);}
template<typename a_type> inline a_type Random12II(RandomState& state = DefaultRandomState) {return Random12<true, true, a_type>(state);}

/** Additional and faster functions for real numbers

    These return a number in the 0..1 range by subtracting one to the 1..2 function
    This avoids a multiplication for the min...max scaling

    Provided for convenience only, use the 1..2 range as the base random function
*/
template<bool include_min, bool include_max, typename a_type> inline a_type Random01(RandomState& state = DefaultRandomState) {
    return Random12<include_min, include_max, a_type>(state) - a_type(1.0);
}
// Alias that can be useful too
template<typename a_type> inline a_type Random01IE(RandomState& state = DefaultRandomState) {return Random01<true, false, a_type>(state);}
template<typename a_type> inline a_type Random01EI(RandomState& state = DefaultRandomState) {return Random01<false, true, a_type>(state);}
template<typename a_type> inline a_type Random01EE(RandomState& state = DefaultRandomState) {return Random01<false, false, a_type>(state);}
template<typename a_type> inline a_type Random01II(RandomState& state = DefaultRandomState) {return Random01<true, true, a_type>(state);}

/// Define all 12 and 01 functions only for real types
/// use the 12 function to generate the other

#define STREFLOP_RANDOM_MAKE_REAL_FLOAT_TYPES(a_type) \
template<> a_type Random12<true, true, a_type>(RandomState& state); \
template<> a_type Random12<true, false, a_type>(RandomState& state); \
template<> a_type Random12<false, true, a_type>(RandomState& state); \
template<> a_type Random12<false, false, a_type>(RandomState& state); \
template<> a_type Random<a_type>(RandomState& state); \
template<> inline a_type Random<true, true, a_type>(a_type min, a_type max, RandomState& state) { \
    a_type range = max - min;\
    return Random12<true,true,a_type>(DefaultRandomState) * range - range + min;\
} \
template<> inline a_type Random<true, false, a_type>(a_type min, a_type max, RandomState& state) { \
    a_type range = max - min;\
    return Random12<true,false,a_type>(DefaultRandomState) * range - range + min;\
} \
template<> inline a_type Random<false, true, a_type>(a_type min, a_type max, RandomState& state) { \
    a_type range = max - min;\
    return Random12<false,true,a_type>(DefaultRandomState) * range - range + min;\
} \
template<> inline a_type Random<false, false, a_type>(a_type min, a_type max, RandomState& state) { \
    a_type range = max - min;\
    return Random12<false,false,a_type>(DefaultRandomState) * range - range + min;\
}


STREFLOP_RANDOM_MAKE_REAL_FLOAT_TYPES(Simple)
STREFLOP_RANDOM_MAKE_REAL_FLOAT_TYPES(Double)

#if defined(Extended)
STREFLOP_RANDOM_MAKE_REAL_FLOAT_TYPES(Extended)
#endif


/**
    Utility to get a number from a Normal distribution
    The no argument version returns a distribution with mean 0, variance 1.
    The 2 argument version takes the desired mean standard deviation.
    Both are only defined over the floating-point types

    This uses the common polar-coordinate method to transform between uniform and normal
    Step 1: generate a pair of numbers in the -1,1 x -1,1 square
    Step 2: loop over to step 1 until it is also in the unit circle
    Step 3: Convert using the property property (U1, U2) = (X,Y) * sqrt( -2 * log(D) / D)
            with D = X*X+Y*Y the squared distance and log(D) the natural logarithm function
    Step 4: Choose U1 or U2, they are independent (keep the other for the next call)
    Step 5 (optional): Scale and translate U to a given mean, std_dev

    There may be better numerical methods, but I'm too lazy to implement them.
    Any suggestion/contribution is welcome!

    In particular, it seems quite horrendous to do computations close to the 0, in the 0-1 range,
    where there may be denormals and the lot, to scale and translate later on. It would be better
    to compute everything in another float interval (ex: 2 to 4 centered on 3 for the same square
    has a uniform representable range of floats), and only then convert to the given mean/std_dev
    at the last moment.

    Note: An optional argument "secondary" may be specified, and in that case, a second number
    indepedent from the first will be returned at negligible cost (in other words, by default, half the values
    are thrown away)
*/
template<typename a_type> a_type NRandom(a_type mean, a_type std_dev, a_type *secondary = 0, RandomState& state = DefaultRandomState);
template<> Simple NRandom(Simple mean, Simple std_dev, Simple *secondary, RandomState& state);
template<> Double NRandom(Double mean, Double std_dev, Double *secondary, RandomState& state);
#if defined(Extended)
template<> Extended NRandom(Extended mean, Extended std_dev, Extended *secondary, RandomState& state);
#endif
/// Simplified versions
template<typename a_type> a_type NRandom(a_type *secondary = 0, RandomState& state = DefaultRandomState);
template<> Simple NRandom(Simple *secondary, RandomState& state);
template<> Double NRandom(Double *secondary, RandomState& state);
#if defined(Extended)
template<> Extended NRandom(Extended *secondary, RandomState& state);
#endif

}

#endif

