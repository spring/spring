/*
    streflop: STandalone REproducible FLOating-Point
    Nicolas Brodu, 2006
    Code released according to the GNU Lesser General Public License

    Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP, and IBM MP lib.
    Uses SoftFloat too.

    Please read the history and copyright information in the documentation provided with the source code
*/

#ifndef STREFLOP_H
#define STREFLOP_H

// protect against bad defines
#if   defined(STREFLOP_SSE) && defined(STREFLOP_X87)
#error You have to define exactly one of STREFLOP_SSE STREFLOP_X87 STREFLOP_SOFT, but you defined both STREFLOP_SSE and STREFLOP_X87
#elif defined(STREFLOP_SSE) && defined(STREFLOP_SOFT)
#error You have to define exactly one of STREFLOP_SSE STREFLOP_X87 STREFLOP_SOFT, but you defined both STREFLOP_SSE and STREFLOP_SOFT
#elif defined(STREFLOP_X87) && defined(STREFLOP_SOFT)
#error You have to define exactly one of STREFLOP_SSE STREFLOP_X87 STREFLOP_SOFT, but you defined both STREFLOP_X87 and STREFLOP_SOFT
#elif !defined(STREFLOP_SSE) && !defined(STREFLOP_X87) && !defined(STREFLOP_SOFT)
#error You have to define exactly one of STREFLOP_SSE STREFLOP_X87 STREFLOP_SOFT, but you defined none
#endif

// First, define the numerical types
namespace streflop {

// Handle the 6 cells of the configuration array. See README.txt
#if defined(STREFLOP_SSE)

    // SSE always uses native types, denormals are handled by FPU flags
    typedef float Simple;
    typedef double Double;
    #undef Extended

#elif defined(STREFLOP_X87)

    // X87 uses a wrapper for no denormals case
#if defined(STREFLOP_NO_DENORMALS)
#include "X87DenormalSquasher.h"
    typedef X87DenormalSquasher<float> Simple;
    typedef X87DenormalSquasher<double> Double;
    typedef X87DenormalSquasher<long double> Extended;
    #define Extended Extended

#else
    // Use FPU flags for x87 with denormals
    typedef float Simple;
    typedef double Double;
    typedef long double Extended;
    #define Extended Extended
#endif

#elif defined(STREFLOP_SOFT) && !defined(STREFLOP_NO_DENORMALS)
    // Use SoftFloat wrapper
#include "SoftFloatWrapper.h"
    typedef SoftFloatWrapper<32> Simple;
    typedef SoftFloatWrapper<64> Double;
    typedef SoftFloatWrapper<96> Extended;
    #define Extended Extended

#else

#error STREFLOP: Invalid combination or unknown FPU type.

#endif

}

#if defined(STREFLOP_SSE) && defined(_MSC_VER) 
    // MSVC will not compile without the long double variants defined, so we either have to declare Extended:
    //    typedef long double Extended;
    //    #define Extended Extended
    // or write some wrapper macros:
    #define atan2l(a, b) atan2((double)a, (double)b)
    #define cosl(a) cos((double)a)
    #define expl(a) exp((double)a)
    #define ldexpl(a, b) ldexp((double)a, (double)b)
    #define logl(a) log((double)a)
    #define powl(a,b) pow((double)a, (double)b)
    #define sinl(a) sin((double)a)
    #define sqrtl(a) sqrt((double)a)
    #define tanl(a) tan((double)a)
    #define frexpl(a, b) frexp((double)a, b)
    // The wrappers are possibly the better choice for sync reasons.
#endif

// Include the FPU settings file, so the user can initialize the library
#include "FPUSettings.h"

// Now that types are defined, include the Math.h file for the prototypes
#include "SMath.h"

// And now that math functions are defined, include the random numbers
#include "Random.h"

#endif

