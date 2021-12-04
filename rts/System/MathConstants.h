/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MATH_CONSTANTS_H
#define MATH_CONSTANTS_H

#ifndef M_PI
    #define M_PI       3.14159265358979323846
#endif

namespace math {
	static constexpr float    PI  = 3.14159265358979323846f;
	static constexpr float INVPI  = 0.3183098861837907f;          // sic ( 1.0f /  PI       is not a constexpr)
	static constexpr float INVPI2 = 0.15915494309189535f;         // sic ( 1.0f / (PI *  2) is not a constexpr)
	static constexpr float TWOPI  = PI * 2.0f;
	static constexpr float SQRPI  = 9.869604401089358f;           // sic (         PI * PI  is not a constexpr)

	static constexpr float PIU4   =  1.2732395447351628f;         // sic ( 4.0f /       PI  is not a constexpr)
	static constexpr float PISUN4 = -0.4052847345693511f;         // sic (-4.0f / (PI * PI) is not a constexpr)

	static constexpr float    HALFPI = PI * 0.5f;
	static constexpr float QUARTERPI = PI * 0.25f;
	static constexpr float NEGHALFPI = -HALFPI;

	static constexpr float SQRT2 = 1.41421356237f;
	static constexpr float HALFSQRT2 = SQRT2 * 0.5f;

	static constexpr float RAD_TO_DEG = 57.29577951308232f;       // sic (360 / (2*PI) is not a constexpr)
	static constexpr float DEG_TO_RAD =  0.017453292519943295f;   // sic ((2*PI) / 360 is not a constexpr)
};

#endif

