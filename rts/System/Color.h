/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _COLOR_H
#define _COLOR_H

#include <cinttypes>
#include "System/creg/creg_cond.h"


/**
 * A 32bit RGBA color.
 */
struct SColor
{
	CR_DECLARE_STRUCT(SColor)

	SColor() : r(255), g(255), b(255), a(255) {}

	/// Initialize with values in the range [0, 255]
	SColor(const std::uint8_t r, const std::uint8_t g, const std::uint8_t b, const std::uint8_t a = 255)
		: r(r), g(g), b(b), a(a) {}
	/// Initialize with values in the range [0, 255]
	SColor(const int r, const int g, const int b, const int a = 255)
		: r(r), g(g), b(b), a(a) {}
	/// Initialize with values in the range [0.0, 1.0]
	SColor(const float r, const float g, const float b, const float a = 1.0f)
		: r((unsigned char)(r * 255.0f))
		, g((unsigned char)(g * 255.0f))
		, b((unsigned char)(b * 255.0f))
		, a((unsigned char)(a * 255.0f))
	{}
	SColor(const float* f)
		: r(f[0] * 255.0f)
		, g(f[1] * 255.0f)
		, b(f[2] * 255.0f)
		, a(f[3] * 255.0f)
	{}
	constexpr SColor(const unsigned char* u)
		: r(u[0])
		, g(u[1])
		, b(u[2])
		, a(u[3])
	{}

	SColor operator * (const float s) const {
		return SColor(int(float(r) * s), int(float(g) * s), int(float(b) * s), int(float(a) * s));
	}

	operator const unsigned char* () const { return &r; }
	operator unsigned char* () { return &r; }

public:
	union {
		/// individual color channel values in the range [0, 255]
		struct { std::uint8_t r, g, b, a; };
		/// The color as a single 32bit value
		std::uint32_t i;
	};
};

#endif // _COLOR_H
