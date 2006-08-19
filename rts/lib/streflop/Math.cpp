// Includes Math.h in turn
#include "streflop.h"

namespace streflop {

	// MSVC chokes on these constants,
	// TODO: we need another way to specify them. (hardcode the bitpattern?)
#ifndef _MSC_VER

    // Constants

    const Simple SimplePositiveInfinity = Simple(1.0f) / Simple(0.0f);
    const Simple SimpleNegativeInfinity = Simple(-1.0f) / Simple(0.0f);
    // TODO: non-signaling version
    const Simple SimpleNaN = SimplePositiveInfinity + SimpleNegativeInfinity;

    const Double DoublePositiveInfinity = Double(1.0f) / Double(0.0f);
    const Double DoubleNegativeInfinity = Double(-1.0f) / Double(0.0f);
    // TODO: non-signaling version
    const Double DoubleNaN = DoublePositiveInfinity + DoubleNegativeInfinity;

// Extended are not always available
#ifdef Extended

    const Extended ExtendedPositiveInfinity = Extended(1.0f) / Extended(0.0f);
    const Extended ExtendedNegativeInfinity = Extended(-1.0f) / Extended(0.0f);
    // TODO: non-signaling version
    const Extended ExtendedNaN = ExtendedPositiveInfinity + ExtendedNegativeInfinity;

#endif

#endif // _MSC_VER


    // Default environment. Initalized to 0, and really set on first access
#if defined(STREFLOP_X87)
    fenv_t FE_DFL_ENV = 0;
#elif defined(STREFLOP_SSE)
    fenv_t FE_DFL_ENV = {0,0};
#elif defined(STREFLOP_SOFT)
    fenv_t FE_DFL_ENV = {42,0,0};
#else
#error STREFLOP: Invalid combination or unknown FPU type.
#endif

}
