/*
    streflop: STandalone REproducible FLOating-Point
    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License

    Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP, and IBM MP lib.
    Uses SoftFloat too.

    Please read the history and copyright information in the documentation provided with the source code
*/

// Include time(0) function to get a seed based on system time
#include <time.h>
#include <iostream>
#include "streflop.h"

// Include endian-specific code
#undef __BYTE_ORDER
#undef __FLOAT_WORD_ORDER
#include "System.h"

namespace streflop {

//////////////////////////////////////////////////////////////////////
// Code stolen and adapted from mt19937ar.c
//////////////////////////////////////////////////////////////////////

/*
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)  
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          
   Copyright (C) 2005, Mutsuo Saito,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#if STREFLOP_RANDOM_GEN_SIZE == 32

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

/* initializes mt[N] with a seed */
inline void init_genrand(SizedUnsignedInteger<32>::Type s, RandomState& state)
{
    state.seed = s;
    state.mt[0]= s; // & 0xffffffffUL; // NB060508: unnecessary with the use of sized types
    for (state.mti=1; state.mti<N; state.mti++) {
        state.mt[state.mti] = 
        (1812433253UL * (state.mt[state.mti-1] ^ (state.mt[state.mti-1] >> 30)) + state.mti);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        //mt[mti] &= 0xffffffffUL;  // NB060508: unnecessary with the use of sized types
        /* for >32 bit machines */
    }
}

/* generates a random number on [0,0xffffffff]-interval */
inline SizedUnsignedInteger<32>::Type genrand_int(RandomState& state)
{
    SizedUnsignedInteger<32>::Type y;
    static SizedUnsignedInteger<32>::Type mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (state.mti >= N) { /* generate N words at one time */
        int kk;

        //if (state.mti == N+1)   /* if init_genrand() has not been called, */
            //init_genrand(5489UL, state); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (state.mt[kk]&UPPER_MASK)|(state.mt[kk+1]&LOWER_MASK);
            state.mt[kk] = state.mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (state.mt[kk]&UPPER_MASK)|(state.mt[kk+1]&LOWER_MASK);
            state.mt[kk] = state.mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (state.mt[N-1]&UPPER_MASK)|(state.mt[0]&LOWER_MASK);
        state.mt[N-1] = state.mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        state.mti = 0;
    }
  
    y = state.mt[state.mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

#else

//////////////////////////////////////////////////////////////////////
// End of code adapted from mt19937ar.c
// Now adapt code from the 64-bit version in mt19937-64.c
//////////////////////////////////////////////////////////////////////

/* 
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Before using, initialize the state by using init_genrand64(seed)  
   or init_by_array64(init_key, key_length).

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
     ACM Transactions on Modeling and 
     Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
     ``Mersenne Twister: a 623-dimensionally equidistributed
       uniform pseudorandom number generator''
     ACM Transactions on Modeling and 
     Computer Simulation 8. (Jan. 1998) 3--30.

   Any feedback is very welcome.
   http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
*/

#define NN 312
#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */

/* initializes mt[NN] with a seed */
inline void init_genrand(SizedUnsignedInteger<64>::Type seed, RandomState& state)
{
    state.seed = seed;
    state.mt[0] = seed;
    for (state.mti=1; state.mti<NN; state.mti++)
        state.mt[state.mti] =  (SizedUnsignedInteger<64>::Type(6364136223846793005ULL) * (state.mt[state.mti-1] ^ (state.mt[state.mti-1] >> 62)) + state.mti);
}

/* generates a random number on [0, 2^64-1]-interval */
inline SizedUnsignedInteger<64>::Type genrand_int(RandomState& state)
{
    int i;
    SizedUnsignedInteger<64>::Type x;
    static SizedUnsignedInteger<64>::Type mag01[2]={0ULL, MATRIX_A};

    if (state.mti >= NN) { /* generate NN words at one time */

        /* if init_genrand64() has not been called, */
        /* a default initial seed is used     */
        //if (state.mti == NN+1)
            //init_genrand64(5489ULL, state);

        for (i=0;i<NN-MM;i++) {
            x = (state.mt[i]&UM)|(state.mt[i+1]&LM);
            state.mt[i] = state.mt[i+MM] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
        }
        for (;i<NN-1;i++) {
            x = (state.mt[i]&UM)|(state.mt[i+1]&LM);
            state.mt[i] = state.mt[i+(MM-NN)] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
        }
        x = (state.mt[NN-1]&UM)|(state.mt[0]&LM);
        state.mt[NN-1] = state.mt[MM-1] ^ (x>>1) ^ mag01[(int)(x&1ULL)];

        state.mti = 0;
    }
  
    x = state.mt[state.mti++];

    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);

    return x;
}
#endif

//////////////////////////////////////////////////////////////////////
// End of code adapted from mt19937-64.c
//////////////////////////////////////////////////////////////////////

// Bit getter utilities
template<int nbits> struct Accessor {
    typedef typename SizedUnsignedInteger<nbits>::Type Type;
    static inline Type getRandomInt(RandomState& state) {
        return static_cast<Type>(genrand_int(state));
    }
};

// Specialize for 32 bits generator case
#if STREFLOP_RANDOM_GEN_SIZE == 32
template<> struct Accessor<64> {
    typedef SizedUnsignedInteger<64>::Type Type;
    static inline Type getRandomInt(RandomState& state) {
        return static_cast<Type>(genrand_int(state)) | (static_cast<Type>(genrand_int(state)) << 32);
    }
};
#endif


// This code inspired from a trick found in Richard J. Wagner's Mersene class and the optimization
// by Magnus Jonsson, also found at http://aggregate.org.
// The trick consists in:
// - draw random numbers in as close as possible as the target
// - reject these which are out of range
// The goal is to avoid operator %

template<int bits_size> struct RandomIntRestrictor {
};

// for 8-bits
template<> struct RandomIntRestrictor<8> {
    typedef SizedUnsignedInteger<8>::Type Type;

    static inline Type getRestrictedRandomInt(Type n, RandomState& state)
    {
        // First propagate leading 1 to all other bits
        Type mask = n;
        mask |= mask >> 1;
        mask |= mask >> 2;
        mask |= mask >> 4;

        // Draw only that number of bits, until a number is in the desired range [0,n]
        // Worse case is number of loops proba decreasing in 1/2^nloops
        Type ret;
        do {
            ret = Accessor<8>::getRandomInt(state) & mask;
        } while( ret > n );

        return ret;
    }
};

// for 16-bits
template<> struct RandomIntRestrictor<16> {
    typedef SizedUnsignedInteger<16>::Type Type;

    static inline Type getRestrictedRandomInt(Type n, RandomState& state)
    {
        // First propagate leading 1 to all other bits
        Type mask = n;
        mask |= mask >> 1;
        mask |= mask >> 2;
        mask |= mask >> 4;
        mask |= mask >> 8;

        // Draw only that number of bits, until a number is in the desired range [0,n]
        // Worse case is number of loops proba decreasing in 1/2^nloops
        Type ret;
        do {
            ret = Accessor<16>::getRandomInt(state) & mask;
        } while( ret > n );

        return ret;
    }
};

// for 32-bits
template<> struct RandomIntRestrictor<32> {
    typedef SizedUnsignedInteger<32>::Type Type;

    static inline Type getRestrictedRandomInt(Type n, RandomState& state)
    {
        // First propagate leading 1 to all other bits
        Type mask = n;
        mask |= mask >> 1;
        mask |= mask >> 2;
        mask |= mask >> 4;
        mask |= mask >> 8;
        mask |= mask >> 16;

        // Draw only that number of bits, until a number is in the desired range [0,n]
        // Worse case is number of loops proba decreasing in 1/2^nloops
        Type ret;
        do {
            ret = Accessor<32>::getRandomInt(state) & mask;
        } while( ret > n );

        return ret;
    }
};

// for 64-bits
template<> struct RandomIntRestrictor<64> {
    typedef SizedUnsignedInteger<64>::Type Type;

    static inline Type getRestrictedRandomInt(Type n, RandomState& state)
    {
        // First propagate leading 1 to all other bits
        Type mask = n;
        mask |= mask >> 1;
        mask |= mask >> 2;
        mask |= mask >> 4;
        mask |= mask >> 8;
        mask |= mask >> 16;
        mask |= mask >> 32;

        // Draw only that number of bits, until a number is in the desired range [0,n]
        // Worse case is number of loops proba descreasing in 1/2^nloops
        Type ret;
        do {
            ret = Accessor<64>::getRandomInt(state) & mask;
        } while( ret > n );

        return ret;
    }
};


// Now implement the Random functions

/* range checker.
   But why should clean caller code be slowed down by checks?
   => caller code should provide good arguments, or be fixed

/// Common function that will be used for all random integer types
template<typename an_int_type, int num_bounds, int min_excluded> struct RandomSwitcher {
    // random selection
    static inline an_int_type getMinMaxRandom(an_int_type min, an_int_type max, RandomState& state) {
        // Works in both signed and unsigned arithmetic
        if (max<=min) {
            if (max==min) return min; // easy
            return RandomSwitcher<an_int_type,num_bounds,(num_bounds==1)?(!min_excluded):min_excluded>::getMinMaxRandom(max, min, state);
        }

        // now max > min
        an_int_type range = max - min + 1;

        // overflow, asking the whole range of integers
        if (range==0) return getRandomBits<a_type>(sizeof(an_int_type)*STREFLOP_INTEGER_TYPES_CHAR_BITS, state);

        // range >= 2, num_bounds <= 2, OK to subtract 2 - num_bounds
        range -= 2-num_bounds;
        // ask the impossible, like RandomEE(3,4)
        if (range == 0) return min;

        // This gives the number of possible integers to choose from (at least 1)
        // Now generate the number
        an_int_type ret = min + min_excluded + static_cast<an_int_type>(RandomIntRestrictor< sizeof(an_int_type)*STREFLOP_INTEGER_TYPES_CHAR_BITS >::getRestrictedRandomInt(range,state));
        return ret;
    }
};
*/

#define SPECIALIZE_RANDOM_FOR_TYPE(a_type,use_signed) \
template<> a_type Random<a_type>(RandomState& state) { \
    return Accessor<sizeof(a_type)*STREFLOP_INTEGER_TYPES_CHAR_BITS>::getRandomInt(state); \
} \
template<> a_type Random<true, true, a_type>(a_type min, a_type max, RandomState& state) { \
    return static_cast<a_type>(RandomIntRestrictor< sizeof(a_type)*STREFLOP_INTEGER_TYPES_CHAR_BITS >::getRestrictedRandomInt(max-min,state)) + min; \
} \
template<> a_type Random<true, false, a_type>(a_type min, a_type max, RandomState& state) { \
    return static_cast<a_type>(RandomIntRestrictor< sizeof(a_type)*STREFLOP_INTEGER_TYPES_CHAR_BITS >::getRestrictedRandomInt(max-min-1,state)) + min; \
} \
template<> a_type Random<false, true, a_type>(a_type min, a_type max, RandomState& state) { \
    return max - static_cast<a_type>(RandomIntRestrictor< sizeof(a_type)*STREFLOP_INTEGER_TYPES_CHAR_BITS >::getRestrictedRandomInt(max-min-1,state)); \
} \
template<> a_type Random<false, false, a_type>(a_type min, a_type max, RandomState& state) { \
    return static_cast<a_type>(RandomIntRestrictor< sizeof(a_type)*STREFLOP_INTEGER_TYPES_CHAR_BITS >::getRestrictedRandomInt(max-min-2,state)) + min + 1; \
}
SPECIALIZE_RANDOM_FOR_TYPE(char,true)
SPECIALIZE_RANDOM_FOR_TYPE(unsigned char,false)
SPECIALIZE_RANDOM_FOR_TYPE(short,true)
SPECIALIZE_RANDOM_FOR_TYPE(unsigned short,false)
SPECIALIZE_RANDOM_FOR_TYPE(int,true)
SPECIALIZE_RANDOM_FOR_TYPE(unsigned int,false)
SPECIALIZE_RANDOM_FOR_TYPE(long,true)
SPECIALIZE_RANDOM_FOR_TYPE(unsigned long,false)
SPECIALIZE_RANDOM_FOR_TYPE(long long,true)
SPECIALIZE_RANDOM_FOR_TYPE(unsigned long long,false)


// Random for float types is even more dependent on size


/*

EXPERIMENTAL:

    The ULP_Random functions use a uniform distribution in the ULP space.
    This means, each possibly representable float is given exactly the same weight for the
    random choice. This is an exponential scale where there are as many numbers between
    successive powers of 2 (i.e as many numbers between 1 and 2 as there are between 1024 and 2048).
    This effectively corresponds to the maximum machine precision, but it is unfortunately
    not what one means by "uniform" in the traditional sense of the term (it is uniform in terms
    of bit patterns, so for the computer!)

    Algorithm: treat floats as binary pattern, thanks to IEEE754 ordered property
    => this gives a "range" like max-min for the integers, where the unit is ulp
    => this gives the maximum precision for floats, because a random number is chosen between exactly how many
    numbers can be represented with that float format in that range


#define SPECIALIZE_RANDOM_FOR_SIMPLE_ULP(X,Y) \
Simple ULP_Random ## X ## Y(Simple min, Simple max) { \
 \
    // Convert to signed integer for quick test of bit sign \
    SizedInteger<32>::Type imin = *reinterpret_cast<SizedUnsignedInteger<32>::Type*>(&min); \
    SizedInteger<32>::Type imax = *reinterpret_cast<SizedUnsignedInteger<32>::Type*>(&max); \
 \
    // Infinity is a perfectly fine number, with a bit pattern contiguous to the min and max floats \
    // This makes sense if excluding bounds: RandomEE(-inf,+inf) returns a possible float at random \
 \
    // Rule out NaNs \
    if (imin&0x7fffffff > 0x7f800000) return SimpleNaN; \
    if (imax&0x7fffffff > 0x7f800000) return SimpleNaN; \
 \
    // Convert to 2-complement representation \
    if (imin<0) imin = 0x80000000 - imin; \
    if (imax<0) imax = 0x80000000 - imax; \
 \
    // Now magically get an integer at random in that range \
    // This gives EXACTLY one choice per representable float \
    // It is non-uniform in the float space, but uniform in the bit-pattern space \
    SizedInteger<32>::Type iret = Random ## X ## Y(imin,imax); \
 \
    // convert back to 2-complement to IEEE754 \
    if (iret<0) iret = 0x80000000 - iret; \
 \
    // cast to float \
    return *reinterpret_cast<Simple*>(&iret); \
}
SPECIALIZE_RANDOM_FOR_SIMPLE_ULP(E,I)
SPECIALIZE_RANDOM_FOR_SIMPLE_ULP(E,E)
SPECIALIZE_RANDOM_FOR_SIMPLE_ULP(I,E)
SPECIALIZE_RANDOM_FOR_SIMPLE_ULP(I,I)
*/


// Return a random float
template<> Simple Random<Simple>(RandomState& state) {
    // Generate bits
    SizedUnsignedInteger<32>::Type ret = Accessor<32>::getRandomInt(state);

    // Discard NaNs and Inf, ignore sign
    while ((ret & 0x7fffffff) >= 0x7f800000) ret = Accessor<32>::getRandomInt(state);

    // cast to float
    return *reinterpret_cast<Simple*>(&ret);
}

// Random in 1..2 - ideal IE case
template<> Simple Random12<true,false,Simple>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Generate bits
    SizedUnsignedInteger<32>::Type r12 = Accessor<32>::getRandomInt(state);

    // Simple precision keeps only 23 bits
    r12 &= 0x007FFFFF;

    // Insert exponent so it's in the [1.0-2.0) range
    r12 |= 0x3F800000;

    return *reinterpret_cast<Simple*>(&r12);
}

// Random in 1..2 - near ideal EI case
template<> Simple Random12<false,true,Simple>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Generate bits
    SizedUnsignedInteger<32>::Type r12 = Accessor<32>::getRandomInt(state);

    // Simple precision keeps only 23 bits
    r12 &= 0x007FFFFF;

    // Insert exponent so it's in the [1.0-2.0) range
    r12 |= 0x3F800000;

    // Bitwise add 1 so it's in the (1.0-2.0] range
    r12 += 1;

    return *reinterpret_cast<Simple*>(&r12);
}

// Random in 1..2 - need to include both bounds
template<> Simple Random12<true,true,Simple>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Generate bits
    SizedUnsignedInteger<32>::Type r12 = Accessor<32>::getRandomInt(state);

    // Keep 2^23 + 1 possibilities, discard others
    // Note: %= operator is nicely converted into reciprocal multiply and shift by compiler
    r12 %= 0x00800001;
    // Choose to avoid % operator by having about 1/2 chance of rejection. Not faster.
//    while ((r12 &= 0x00FFFFFF) > 0x00800000) r12 = Accessor<32>::getRandomInt(state);

/* could also use multiply by reciprocal and then find remainder. For div by 0x00800001:
; dividend: register other than EAX or memory location
MOV    EAX, 0FFFFFE01h
MUL    dividend
SHR    EDX, 23
; quotient now in EDX
*/

    // bitwise add exponent so it's in the [1.0-2.0] range
    r12 += 0x3F800000;

    return *reinterpret_cast<Simple*>(&r12);
}

// Random in 1..2 - need to exclude both bounds
template<> Simple Random12<false,false,Simple>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Generate bits
    SizedUnsignedInteger<32>::Type r12 = Accessor<32>::getRandomInt(state);

    // Keep 2^23 - 1 possibilities
    //r12 %= 0x007FFFFF;
    // Choose to avoid % operator by having very small chance of rejection
    // Could we find a branchless version for % by 2^N-1 ?
    while ((r12 &= 0x007FFFFF) == 0x007FFFFF) r12 = Accessor<32>::getRandomInt(state);

    // bitwise add exponent so it's in the (1.0-2.0) range
    r12 += 0x3F800001;

    return *reinterpret_cast<Simple*>(&r12);
}


///////// Double versions  ///////////

// Return a random float
template<> Double Random<Double>(RandomState& state) {
    // Generate bits
    SizedUnsignedInteger<64>::Type ret = Accessor<64>::getRandomInt(state);

    // Discard NaNs and Inf, ignore sign
    while ((ret & 0x7fffffffffffffffULL) >= 0x7ff0000000000000ULL) ret = Accessor<64>::getRandomInt(state);

    // cast to Double
    return *reinterpret_cast<Double*>(&ret);
}


// Random in a 1..2 - ideal IE case
template<> Double Random12<true,false,Double>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Generate bits
    SizedUnsignedInteger<64>::Type r12 = Accessor<64>::getRandomInt(state);

    // Double precision keeps only 52 bits
    r12 &= 0x000FFFFFFFFFFFFFULL;

    // Insert exponent so it's in the [1.0-2.0) range
    r12 |= 0x3FF0000000000000ULL;

    // scale from 1-2 interval to the desired interval
    return *reinterpret_cast<Double*>(&r12);
}

// Random in a 1..2 - near ideal EI case
template<> Double Random12<false,true,Double>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Generate bits
    SizedUnsignedInteger<64>::Type r12 = Accessor<64>::getRandomInt(state);

    // Double precision keeps only 52 bits
    r12 &= 0x000FFFFFFFFFFFFFULL;

    // Insert exponent so it's in the [1.0-2.0) range
    r12 |= 0x3FF0000000000000ULL;

    // Bitwise add 1 so it's in the (1.0-2.0] range
    r12 += 1;

    // scale from 1-2 interval to the desired interval
    return *reinterpret_cast<Double*>(&r12);
}

// Random in a 1..2 - need to include both bounds
template<> Double Random12<true,true,Double>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Generate bits
    SizedUnsignedInteger<64>::Type r12 = Accessor<64>::getRandomInt(state);

    // Keep 2^52 + 1 possibilities
    // See comment in Simple version
    // allow %= only for 64-bit register machines
#if STREFLOP_RANDOM_GEN_SIZE == 64
    r12 %= 0x0010000000000001ULL;
#else
    // Choose to avoid % operator by having about 1/2 chance of rejection. Is this faster?
    while ((r12 &= 0x001FFFFFFFFFFFFFULL) > 0x0010000000000000ULL) r12 = Accessor<64>::getRandomInt(state);
#endif

    // bitwise add exponent so it's in the [1.0-2.0] range
    r12 += 0x3FF0000000000000ULL;

    return *reinterpret_cast<Double*>(&r12);
}

// Random in a 1..2 - need to exclude both bounds
template<> Double Random12<false,false,Double>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Generate bits
    SizedUnsignedInteger<64>::Type r12 = Accessor<64>::getRandomInt(state);

    // Keep 2^52 - 1 possibilities
    // See comment in Simple version
    // r12 %= 0x000FFFFFFFFFFFFFULL;
    // Choose to avoid % operator by having very small chance of rejection
    while ((r12 &= 0x000FFFFFFFFFFFFFULL) == 0x000FFFFFFFFFFFFFULL) r12 = Accessor<64>::getRandomInt(state);

    // bitwise add exponent so it's in the (1.0-2.0) range
    r12 += 0x3FF0000000000001ULL;

    return *reinterpret_cast<Double*>(&r12);
}


#ifdef Extended

// little endian
#if __FLOAT_WORD_ORDER == 1234

/// Little endian is fine, always the same offsets whatever the memory model
template<int Nbytes> struct ExtendedConverter {
    // Sign and exponent
    static inline SizedUnsignedInteger<16>::Type* sexpPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<16>::Type*>(reinterpret_cast<char*>(e)+8);
    }
    // Mantissa
    static inline SizedUnsignedInteger<64>::Type* mPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<64>::Type*>(e);
    }
};

// Big endian
#elif __FLOAT_WORD_ORDER == 4321
template<int Nbytes> struct ExtendedConverter {
}

/// Extended is softfloat
template<> struct ExtendedConverter<10> {
    // Sign and exponent
    static inline SizedUnsignedInteger<16>::Type* sexpPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<16>::Type*>(e);
    }
    // Mantissa
    static inline SizedUnsignedInteger<64>::Type* mPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<64>::Type*>(reinterpret_cast<char*>(e)+2);
    }
};

/// Default gcc model for x87 - 32 bits (or with -m96bit-long-double)
template<> struct ExtendedConverter<12> {
    // Sign and exponent
    static inline SizedUnsignedInteger<16>::Type* sexpPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<16>::Type*>(reinterpret_cast<char*>(e)+2);
    }
    // Mantissa
    static inline SizedUnsignedInteger<64>::Type* mPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<64>::Type*>(reinterpret_cast<char*>(e)+4);
    }
};

/// Default gcc model for x87 - 64 bits (or with -m128bit-long-double)
template<> struct ExtendedConverter<16> {
    // Sign and exponent
    static inline SizedUnsignedInteger<16>::Type* sexpPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<16>::Type*>(reinterpret_cast<char*>(e)+6);
    }
    // Mantissa
    static inline SizedUnsignedInteger<64>::Type* mPtr(Extended* e) {
        return reinterpret_cast<SizedUnsignedInteger<64>::Type*>(reinterpret_cast<char*>(e)+8);
    }
};
#else
#error unknown byte order
#endif


// Return a random float
template<> Extended Random<Extended>(RandomState& state) {
    // Work directly on Extended bits
    Extended ret;

    // Generate 63 bits for the mantissa
    *ExtendedConverter<sizeof(Extended)>::mPtr(&ret) = Accessor<64>::getRandomInt(state);

    // Generate 16 bits for the exponent
    *ExtendedConverter<sizeof(Extended)>::sexpPtr(&ret) = Accessor<16>::getRandomInt(state);

    // Discard NaNs and Inf, ignore sign
    while ((*ExtendedConverter<sizeof(Extended)>::sexpPtr(&ret) & 0x7fff) == 0x7fff)
        *ExtendedConverter<sizeof(Extended)>::sexpPtr(&ret) = Accessor<16>::getRandomInt(state);

    // x87 extended format oddity: the first mantissa bit (always 1 for normal numbers) is visible, not hidden!
    if ((*ExtendedConverter<sizeof(Extended)>::sexpPtr(&ret) & 0x7fff) != 0)
        *ExtendedConverter<sizeof(Extended)>::mPtr(&ret) |= 0x8000000000000000ULL;

    return ret;
}


// Random in 1..2 - ideal IE case
template<> Extended Random12<true,false,Extended>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Work directly on Extended bits
    Extended r12;

    // Generate 63 bits for the mantissa
    *ExtendedConverter<sizeof(Extended)>::mPtr(&r12) = Accessor<64>::getRandomInt(state);

    // x87 extended format oddity: the first mantissa bit (always 1 for normal numbers) is visible, not hidden!
    // Since in this case this is a number between 1 and 2, this bit has to be set explicitly
    *ExtendedConverter<sizeof(Extended)>::mPtr(&r12) |= 0x8000000000000000ULL;

    // Set exponent so it's in the [1.0-2.0) range
    *ExtendedConverter<sizeof(Extended)>::sexpPtr(&r12) = 0x3FFF;

    // scale from 1-2 interval to the desired interval
    return r12;
}

// Random in 1..2 - near ideal EI case
template<> Extended Random12<false,true,Extended>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Work directly on Extended bits
    Extended r12;

    // Generate 63 bits for the mantissa
    *ExtendedConverter<sizeof(Extended)>::mPtr(&r12) = Accessor<64>::getRandomInt(state);

    // x87 extended format oddity: the first mantissa bit (always 1 for normal numbers) is visible, not hidden!
    // Since in this case this is a number between 1 and 2, this bit has to be set explicitly
    *ExtendedConverter<sizeof(Extended)>::mPtr(&r12) |= 0x8000000000000000LL;

    // Set exponent so it's in the (1.0-2.0] range
    // Replace 1.0 by 2.0
    if (*ExtendedConverter<sizeof(Extended)>::mPtr(&r12) == 0x8000000000000000LL)
        *ExtendedConverter<sizeof(Extended)>::sexpPtr(&r12) = 0x4000;
    else
        *ExtendedConverter<sizeof(Extended)>::sexpPtr(&r12) = 0x3FFF;

    return r12;
}

// Random in 1..2 - need to include both bounds
template<> Extended Random12<true,true,Extended>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Work directly on Extended bits
    Extended r12;

    // Generate 64 bits for the mantissa
    do {
        *ExtendedConverter<sizeof(Extended)>::mPtr(&r12) = Accessor<64>::getRandomInt(state);
    }
    // Include both bounds : repeat the random get till it's in the desired range
    // Keep the possibility for 2.0 in addition to all [1.0-2.0)
    while (*ExtendedConverter<sizeof(Extended)>::mPtr(&r12) > 0x8000000000000000LL);

    // Set exponent so it's in the [1.0-2.0] range
    // Check if 2.0 was selected
    if (*ExtendedConverter<sizeof(Extended)>::mPtr(&r12) == 0x8000000000000000LL)
        *ExtendedConverter<sizeof(Extended)>::sexpPtr(&r12) = 0x4000;
    // otherwise, set the correct mantissa bit and the correct exponent
    else {
        // x87 extended format oddity: the first mantissa bit (always 1 for normal numbers) is visible, not hidden!
        // Since in this case this is a number between 1 and 2, this bit has to be set explicitly
        *ExtendedConverter<sizeof(Extended)>::mPtr(&r12) |= 0x8000000000000000LL;
        *ExtendedConverter<sizeof(Extended)>::sexpPtr(&r12) = 0x3FFF;
    }

    return r12;
}

// Random in 1..2 - need to exclude both bounds
template<> Extended Random12<false,false,Extended>(RandomState& state) {

    // Get uniform number between 1 and 2 at max precision

    // Work directly on Extended bits
    Extended r12;

    // Generate 63 bits for the mantissa
    do {
        *ExtendedConverter<sizeof(Extended)>::mPtr(&r12) = Accessor<64>::getRandomInt(state);
        // x87 extended format oddity: the first mantissa bit (always 1 for normal numbers) is visible, not hidden!
        // Since in this case this is a number between 1 and 2, this bit has to be set explicitly
        *ExtendedConverter<sizeof(Extended)>::mPtr(&r12) |= 0x8000000000000000LL;
    }
    // Exclude both bounds : repeat the random get till it's in the desired range
    // Remove the possibility for 1.0 compared to all [1.0-2.0)
    while (*ExtendedConverter<sizeof(Extended)>::mPtr(&r12) == 0x8000000000000000LL);

    // Set exponent so it's in the (1.0-2.0) range
    *ExtendedConverter<sizeof(Extended)>::sexpPtr(&r12) = 0x3FFF;

    // scale from 1-2 interval to the desired interval
    return r12;
}

#endif

// This is a way to hide the implementation from the header
// And also to ensure there is only one template instanciation, instead of duplicating
// the code in all object files
template<typename FloatType> static inline FloatType NRandom_Generic(FloatType *secondary, RandomState& state) {

    FloatType x, y, d;
    // Generate a point strictly inside the unit circle
    do {
        // Use IE as this is the most convenient for the generation,
        // any will do since the bounds are rejected anyway by unit circle strict comparison
        x = RandomIE(FloatType(-1.0), FloatType(1.0), state);
        y = RandomIE(FloatType(-1.0), FloatType(1.0), state);
        d = x*x + y*y;
    } while (d>=FloatType(1.0));
    // Convert from x/y to uniform
    FloatType conv = sqrt( FloatType(-2.0) * log(d) / d );
    // Use one result as secondary independent variable, if user wishes
    if (secondary) *secondary = x * conv;
    // return the other
    return y * conv;
}

// Specialize for the Float types declared in the header
template<> Simple NRandom(Simple *secondary, RandomState& state) {
    return NRandom_Generic<Simple>(secondary,state);
}

/*
template<> Double NRandom(Double *secondary, RandomState& state) {
    return NRandom_Generic<Double>(secondary,state);
}
#if defined(Extended)
template<> Extended NRandom(Extended *secondary, RandomState& state) {
    return NRandom_Generic<Extended>(secondary,state);
}
#endif
*/

// May save one mul
template<typename FloatType> static inline FloatType NRandom_Generic(FloatType mean, FloatType std_dev, FloatType *secondary, RandomState& state) {

    FloatType x, y, d;
    // Generate a point strictly inside the unit circle
    do {
        // Use IE as this is the most convenient for the generation,
        // any will do since the bounds are rejected anyway by unit circle strict comparison
        x = RandomIE(FloatType(-1.0), FloatType(1.0), state);
        y = RandomIE(FloatType(-1.0), FloatType(1.0), state);
        d = x*x + y*y;
    } while (d>=FloatType(1.0));
    // Convert from x/y to uniform
    FloatType conv = sqrt( FloatType(-2.0) * log(d) / d ) * std_dev;
    // Use one result as secondary independent variable, if user wishes
    if (secondary) *secondary = x * conv + mean;
    // return the other
    return y * conv + mean;
}

// Specialize for the Float types declared in the header
template<> Simple NRandom(Simple mean, Simple std_dev, Simple *secondary, RandomState& state) {
    return NRandom_Generic<Simple>(mean, std_dev, secondary,state);
}

/*
template<> Double NRandom(Double mean, Double std_dev, Double *secondary, RandomState& state) {
    return NRandom_Generic<Double>(mean, std_dev, secondary,state);
}
#if defined(Extended)
template<> Extended NRandom(Extended mean, Extended std_dev, Extended *secondary, RandomState& state) {
    return NRandom_Generic<Extended>(mean, std_dev, secondary,state);
}
#endif
*/

SizedUnsignedInteger<32>::Type RandomInit(RandomState& state) {
    return RandomInit(SizedUnsignedInteger<32>::Type(time(0)));
}

SizedUnsignedInteger<32>::Type RandomInit(SizedUnsignedInteger<32>::Type seed, RandomState& state) {
    init_genrand(seed,state);
    return state.seed;
}

SizedUnsignedInteger<32>::Type RandomSeed(RandomState& state) {
    return state.seed;
}

// Default state holder, so single threaded applications don't bother setting up an object
RandomState DefaultRandomState;

} // end streflop namespace
