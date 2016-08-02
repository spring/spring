/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FASTMATH_H
#define FASTMATH_H

#ifndef DEDICATED_NOSSE
#include <xmmintrin.h>
#endif
#include <boost/cstdint.hpp>
#include "lib/streflop/streflop_cond.h"
#include "System/maindefines.h"

#ifdef _MSC_VER
#define __builtin_sqrtf sqrt_sse
#endif

#ifdef __GNUC__
	#define _const __attribute__((const))
	#define _pure __attribute__((pure))
	#define _warn_unused_result __attribute__((warn_unused_result))
#else
	#define _const
	#define _pure
	#define _warn_unused_result
#endif

/**
 * @brief Fast math routines
 *
 * Contains faster alternatives for the more time
 * consuming standard math routines. These functions
 * are not as accurate, however, but are generally
 * acceptable for most applications.
 *
 */

namespace fastmath {
	float isqrt_sse(float x) _const;
	float sqrt_sse(float x) _const;
	float isqrt_nosse(float x) _const;
	float isqrt2_nosse(float x) _const;
	float sqrt_builtin(float x) _const;
	float apxsqrt(float x) _const;
	float apxsqrt2(float x) _const;
	float sin(float x) _const;
	float cos(float x) _const;
	template<typename T> float floor(const T& f) _const;

	/****************** Square root functions ******************/

	/**
	* @brief DO NOT USE IN SYNCED CODE. Calculates 1/sqrt(x) using SSE instructions.
	*
	* This is much slower than isqrt_nosse (extremely slow on some AMDs) and
	* additionally gives different results on Intel and AMD processors.
	*/
	__FORCE_ALIGN_STACK__
	inline float isqrt_sse(float x)
	{
#ifndef DEDICATED_NOSSE
		__m128 vec = _mm_set_ss(x);
		vec = _mm_rsqrt_ss(vec);
		return _mm_cvtss_f32(vec);
#else
		return isqrt_nosse(x);
#endif
	}

	/**
	* @brief Sync-safe. Calculates square root with using SSE instructions.
	*
	* Slower than std::sqrtf, much faster than streflop
	*/
	__FORCE_ALIGN_STACK__
	inline float sqrt_sse(float x)
	{
#ifndef DEDICATED_NOSSE
		__m128 vec = _mm_set_ss(x);
		vec = _mm_sqrt_ss(vec);
		return _mm_cvtss_f32(vec);
#else
		return sqrt(x);
#endif
	}


	/**
	* @brief Calculates 1/sqrt(x) with less accuracy
	*
	* Gets a very good first guess and then uses one
	* iteration of newton's method for improved accuracy.
	* Average relative error: 0.093%
	* Highest relative error: 0.175%
	*
	* see "The Math Behind The Fast Inverse Square Root Function Code"
	* by Charles McEniry [2007] for a mathematical derivation of this
	* method (or Chris Lomont's 2003 "Fast Inverse Square Root" paper)
	*
	* It has been found to give slightly different results on different Intel CPUs.
	* Possible cause: 32bit vs 64bit. Use with care.
	*/
	inline float isqrt_nosse(float x) {
		float xh = 0.5f * x;
		boost::int32_t i = *(boost::int32_t*) &x;
		// "magic number" which makes a very good first guess
		i = 0x5f375a86 - (i >> 1);
		x = *(float*) &i;
		// Newton's method. One iteration for less accuracy but more speed.
		x = x * (1.5f - xh * (x * x));
		return x;
	}

	/**
	* @brief Calculates 1/sqrt(x) with more accuracy
	*
	* Gets a very good first guess and then uses two
	* iterations of newton's method for improved accuracy.
	* Average relative error: 0.00017%
	* Highest relative error: 0.00047%
	*
	*/
	inline float isqrt2_nosse(float x) {
		float xh = 0.5f * x;
		boost::int32_t i = *(boost::int32_t*) &x;
		// "magic number" which makes a very good first guess
		i = 0x5f375a86 - (i >> 1);
		x = *(float*) &i;
		// Newton's method. Two iterations for more accuracy but less speed.
		x = x * (1.5f - xh * (x * x));
		x = x * (1.5f - xh * (x * x));
		return x;

	}


	/****************** Square root ******************/


	/** Calculate sqrt using builtin sqrtf. (very fast) */
	inline float sqrt_builtin(float x)
	{
		return __builtin_sqrtf(x);
	}

	/**
	* @brief A (possibly very) inaccurate and numerically unstable, but fast, version of sqrt.
	*
	* Use with care.
	*/
	inline float apxsqrt(float x) {
		return x * isqrt_nosse(x);
	}

	/**
	* @brief A (possibly very) inaccurate and numerically unstable, but fast, version of sqrt.
	*
	* Use with care. This should be a little bit better, albeit slower, than fastmath::sqrt.
	*/
	inline float apxsqrt2(float x) {
		return x * isqrt2_nosse(x);
	}


	/****************** Trigonometric functions ******************/

	/**
	* @brief Pi
	*
	* Cherry flavored.
	*/
	static const float PI = 3.141592654f;

	/**
	* @brief Half of pi
	*
	* Pi divided by two
	*/
	static const float HALFPI = PI / 2.0f;

	/**
	* @brief Pi times two
	*
	* Pi times two
	*/
	static const float PI2 = PI * 2.0f;

	/**
	* @brief Four divided by pi
	*
	* Four over pi
	*/
	static const float PIU4 = 4.0f / PI;

	/**
	* @brief Negative four divided by pi squared
	*
	* Negative four over (pi squared)
	*/
	static const float PISUN4 = -4.0f / (PI * PI);

	/**
	* @brief reciprocal of pi
	*
	* One over (pi times two)
	*/
	static const float INVPI2 = 1.0f / PI2;

	/**
	* @brief negative half pi
	*
	* -pi / 2
	*/
	static const float NEGHALFPI = -HALFPI;

	/**
	* @brief square root of two
	*
	* sqrt(2)
	*/
	static const float SQRT2 = 1.41421356237f;

	/**
	* @brief Degree (360) to Radians (2pi)
	*
	* 360 / (2*PI)
	*/
	static const float DEG_TO_RAD = 180.f / PI;


	/**
	* @brief Radians (2pi) to Degrees (360)
	*
	* 360 / (2*PI)
	*/
	static const float RAD_TO_DEG = PI / 180.f;

	/**
	* @brief calculates the sine of x
	*
	* Range reduces x to -PI ... PI, and then uses the
	* sine approximation method as described at
	* http://www.devmaster.net/forums/showthread.php?t=5784
	*
	* Average percentage error: 0.15281632393574715%
	* Highest percentage error: 0.17455324009559584%
	*/
	inline float sin(float x) {
		/* range reduce to -PI ... PI, as the approximation
		method only works well for that range. */
		x = x - ((int)(x * INVPI2)) * PI2;
		if (x > HALFPI) {
			x = -x + PI;
		} else if (x < NEGHALFPI ) {
			x = -x - PI;
		}
		/* approximation */
		x = (PIU4) * x + (PISUN4) * x * math::fabs(x);
		x = 0.225f * (x * math::fabs(x) - x) + x;
		return x;
	}

	/**
	* @brief calculates the cosine of x
	*
	* Adds half of pi to x and then uses the sine method.
	*/
	inline float cos(float x) {
		return sin(x + HALFPI);
	}


	/**
	* @brief fast version of std::floor
	*
	* Like 2-3x faster than glibc ones.
	* Note: The results differ at the end of the 32bit precision range.
	*/
	template<typename T>
	inline float floor(const T& f)
	{
		return (f >= 0) ? int(f) : int(f+0.000001f)-1;
	}
}

using fastmath::PI;
namespace math {
	// override streflop with faster sqrt!
	float sqrt(float x) _const;
	inline float sqrt(float x) {
		return fastmath::sqrt_sse(x);
	}
	float sqrtf(float x) _const;
	inline float sqrtf(float x) {
		return fastmath::sqrt_sse(x);
	}

	float isqrt(float x) _const;
	inline float isqrt(float x) {
		return fastmath::isqrt2_nosse(x);
	}

	using fastmath::floor;
}

#endif
