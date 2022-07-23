/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _COLOR_H
#define _COLOR_H

#include <cinttypes>
#include <array>
#include "System/creg/creg_cond.h"
#include "System/float4.h"


/**
 * A 32bit RGBA color.
 */
struct SColor
{
	CR_DECLARE_STRUCT(SColor)

	constexpr SColor() : r(255), g(255), b(255), a(255) {}

	/// Initialize with values in the range [0, 255]
	constexpr SColor(const std::uint8_t r, const std::uint8_t g, const std::uint8_t b, const std::uint8_t a = 255)
		: r(r), g(g), b(b), a(a) {}
	/// Initialize with values in the range [0, 255]
	constexpr SColor(const int r, const int g, const int b, const int a = 255)
		: r(r), g(g), b(b), a(a) {}
	/// Initialize with values in the range [0.0, 1.0]
	constexpr SColor(float r, float g, float b, float a = 1.0f)
		: r(static_cast<uint8_t>(r * 255.0f))
		, g(static_cast<uint8_t>(g * 255.0f))
		, b(static_cast<uint8_t>(b * 255.0f))
		, a(static_cast<uint8_t>(a * 255.0f))
	{}
	constexpr SColor(const float* f)
		: r(static_cast<uint8_t>(f[0] * 255.0f))
		, g(static_cast<uint8_t>(f[1] * 255.0f))
		, b(static_cast<uint8_t>(f[2] * 255.0f))
		, a(static_cast<uint8_t>(f[3] * 255.0f))
	{}
	constexpr SColor(const uint8_t* u)
		: r(u[0])
		, g(u[1])
		, b(u[2])
		, a(u[3])
	{}

	constexpr SColor operator* (float s) const {
		return SColor(int(float(r) * s), int(float(g) * s), int(float(b) * s), int(float(a) * s));
	}
	constexpr SColor& operator*= (float s) {
		r = uint8_t(float(r) * s);
		g = uint8_t(float(g) * s);
		b = uint8_t(float(b) * s);
		a = uint8_t(float(a) * s);

		return *this;
	}

	constexpr operator float4 () const {
		constexpr float div = 255.0f;
		return float4(
			static_cast<float>(r) / div,
			static_cast<float>(g) / div,
			static_cast<float>(b) / div,
			static_cast<float>(a) / div
		);
	}

	constexpr operator const uint8_t* () const { return &r; }
	constexpr operator uint8_t* () { return &r; }

	static const SColor Zero;
	static const SColor One ;
public:
	union {
		/// individual color channel values in the range [0, 255]
		struct { std::uint8_t r, g, b, a; };
		/// The color as a single 32bit value
		std::uint32_t i;
	};
};

#endif // _COLOR_H
