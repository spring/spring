/* Copyright (C) 2008 Tobi Vollebregt */

/* Conditionally include streflop depending on STREFLOP_* #defines:
   If one of those is present, #include "streflop.h", otherwise #include <math.h>

   When faced with ambiguous call errors with e.g. fabs, use math::function.
   Add it to math namespace if it doesn't exist there yet. */

#ifndef STREFLOP_COND_H
#define STREFLOP_COND_H

#if (defined(STREFLOP_X87) || defined(STREFLOP_SSE) || defined(STREFLOP_SOFT)) && (!defined(NOT_USING_STREFLOP))
#include "streflop.h"
namespace math {
	using namespace streflop;
}
#else
#include <cmath>

#ifdef __APPLE__
// macosx's cmath doesn't include c++11's std::hypot yet (tested 2013)
namespace std {
	template<typename T> T hypot(T x, T y);
}
#endif


namespace math {
	using std::fabs;
	// We are using fastmath::sqrt_sse instead!
	// using std::sqrt;
	using std::sin;
	using std::cos;

	using std::sinh;
	using std::cosh;
	using std::tan;
	using std::tanh;
	using std::asin;
	using std::acos;
	using std::atan;
	using std::atan2;

	using std::ceil;
	using std::floor;
	using std::fmod;
	using std::hypot;
	using std::pow;
	using std::log;
	using std::log10;
	using std::exp;
	using std::frexp;
	using std::ldexp;
// the following are C99 functions -> not supported by VS C
#if !defined(_MSC_VER) || _MSC_VER < 1500
	using std::isnan;
	using std::isinf;
	using std::isfinite;

#elif __cplusplus
}
#include <limits>
namespace math {
	template<typename T> inline bool isnan(T value) {
		return value != value;
	}
	// requires include <limits>
	template<typename T> inline bool isinf(T value) {
		return std::numeric_limits<T>::has_infinity && value == std::numeric_limits<T>::infinity();
	}
	// requires include <limits>
	template<typename T> inline bool isfinite(T value) {
		return !isinf<T>(value);
	}
#endif
}


#ifdef __APPLE__
// see above

#include <algorithm>

namespace std {
	template<typename T>
	T hypot(T x, T y) {
		x = std::abs(x);
		y = std::abs(y);
		auto t = std::min(x,y);
		     x = std::max(x,y);
		t = t / x;
		return x * sqrtf(1.f + t*t);
	}
}
#endif

#endif

#endif // STREFLOP_COND_H
