#ifndef FASTMATH_H
#define FASTMATH_H

#include "StdAfx.h"

/**
 * @file FastMath.cpp
 * @brief Fast math routines
 *
 * Contains faster alternatives for the more time
 * consuming standard math routines. These functions
 * are not as accurate, however, but are generally
 * acceptable for most applications.
 *
 */

namespace fastmath {
	/****************** Square root functions ******************/

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
	*/
	inline float isqrt(float x) {
		float xh = 0.5f * x;
		int i = *(int*) &x;
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
	inline float isqrt2(float x) {
		float xh = 0.5f * x;
		int i = *(int*) &x;
		// "magic number" which makes a very good first guess
		i = 0x5f375a86 - (i >> 1);
		x = *(float*) &i;
		// Newton's method. Two iterations for more accuracy but less speed.
		x = x * (1.5f - xh * (x * x));
		x = x * (1.5f - xh * (x * x));
		return x;
	
	}

	/**
	* @brief Calculates square root with less accuracy
	*
	* These square root functions use the inverse square
	* root routines to obtain the answer. This one uses
	* the less accurate, but faster isqrt().
	*
	*/
	inline float sqrt(float x) {
		return (isqrt(x) * x);
	}

	/**
	* @brief Calculates square root with more accuracy
	*
	* These square root functions use the inverse square
	* root routines to obtain the answer. This one uses
	* the more accurate, but slower isqrt2().
	*
	*/
	inline float sqrt2(float x) {
		return (isqrt2(x) * x);
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
	static const float PIU4 = 4.0 / PI;

	/**
	* @brief Negative four divided by pi squared
	*
	* Negative four over (pi squared)
	*/
	static const float PISUN4 = -4.0 / (PI * PI);

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
		x = (PIU4) * x + (PISUN4) * x * fabs(x);
		x = 0.225 * (x * fabs(x) - x) + x;
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
}

#endif
