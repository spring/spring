/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _COLOR_H
#define _COLOR_H

#include <boost/cstdint.hpp>

/**
 * A 32bit RGBA color.
 */
union SColor
{
	/// Initialize with values in the range [0, 255]
	SColor(const boost::uint8_t r, const boost::uint8_t g, const boost::uint8_t b, const boost::uint8_t a = 255)
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

	/// individual color channel values in the range [0, 255]
	struct { boost::uint8_t r, g, b, a; };
	/// The color as a single 32bit value
	boost::uint32_t i;

	operator const unsigned char* () const { return &r; }
	operator unsigned char* () { return &r; }
};

#endif // _COLOR_H
