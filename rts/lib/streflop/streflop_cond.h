/* Copyright (C) 2008 Tobi Vollebregt */

/* Conditionally include streflop depending on STREFLOP_* #defines:
   If one of those is present, #include "streflop.h", otherwise #include <math.h>

   When faced with ambiguous call errors with e.g. fabs, use math::function.
   Add it to math namespace if it doesn't exist there yet. */

#ifndef STREFLOP_COND_H
#define STREFLOP_COND_H

#if (!defined(NOT_USING_STREFLOP) && (defined(STREFLOP_SSE) || defined(STREFLOP_X87) || defined(STREFLOP_SOFT)))
#define STREFLOP_ENABLED 1
#endif

// these need to be known in FastMath.h and SpringMath.h which both include us
#ifdef __GNUC__
	#define _const __attribute__((const))
	#define _pure __attribute__((pure))
	#define _warn_unused_result __attribute__((warn_unused_result))
#else
	#define _const
	#define _pure
	#define _warn_unused_result
#endif



#if STREFLOP_ENABLED
#include "streflop.h"

namespace math {
	using namespace streflop;
}

#else
#include <cmath>

namespace streflop {
	typedef float Simple;
	typedef double Double;
	template<typename T> void streflop_init() {}
};



#ifdef __APPLE__
// macosx's cmath doesn't include c++11's std::hypot yet (tested 2013)
namespace std {
	template<typename T> T hypot(T x, T y);
};
#endif



namespace math {
	using std::fabs;

	// see FastMath NOTE below
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
	using std::log2;
	using std::log10;
	using std::exp;
	using std::frexp;
	using std::ldexp;
	using std::round;
	using std::erf;

	// these are in streflop:: but not in std::, FastMath adds sqrtf
	// static inline float sqrtf(float x) { return std::sqrt(x); }
	static inline float cosf(float x) { return std::cos(x); }
	static inline float sinf(float x) { return std::sin(x); }
	static inline float tanf(float x) { return std::tan(x); }
	static inline float acosf(float x) { return std::acos(x); }
	static inline float fabsf(float x) { return std::fabs(x); }


// the following are C99 functions -> not supported by VS C
#if !defined(_MSC_VER) || _MSC_VER < 1500
	using std::isnan;
	using std::isinf;
	using std::isfinite;
#else
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



// NOTE:
//   for non-streflop builds we replace std::sqrt by fastmath::sqrt_sse in math::
//   any code that only includes streflop_cond.h (assimp, etc) and not FastMath.h
//   would not know about this without also including the latter, do so here
#include "System/FastMath.h"



#ifdef __APPLE__
#include <algorithm>

namespace std {
	template<typename T>
	T hypot(T x, T y) {
		x = std::abs(x);
		y = std::abs(y);
		auto t = std::min(x, y);
		     x = std::max(x, y);
		t = t / x;
		return x * math::sqrtf(1.0f + t*t);
	}
}
#endif



#endif // STREFLOP_ENABLED

#endif // STREFLOP_COND_H
