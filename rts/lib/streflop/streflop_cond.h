/* Copyright (C) 2008 Tobi Vollebregt */

/* Conditionally include streflop depending on STREFLOP_* #defines:
   If one of those is present, #include "streflop.h", otherwise #include <math.h>

   When faced with ambiguous call errors with e.g. fabs, use math::function.
   Add it to math namespace if it doesn't exist there yet. */

#ifndef STREFLOP_COND_H
#define STREFLOP_COND_H

#if defined(STREFLOP_X87) || defined(STREFLOP_SSE) || defined(STREFLOP_SOFT)
#include "streflop.h"
using namespace streflop;

namespace math {
	using namespace streflop;
}
#else
#include <cmath>
namespace math {
	using std::fabs;
	using std::sqrt;
	using std::sin;
	using std::cos;
}
#endif

#endif
