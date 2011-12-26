/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _COLOR_H
#define _COLOR_H

#include <boost/cstdint.hpp>

union SColor
{
	SColor(const boost::uint8_t& r_, const boost::uint8_t& g_, const boost::uint8_t& b_, const boost::uint8_t& a_ = 255) : r(r_), g(g_), b(b_), a(a_) {};
	SColor(const int& r_, const int& g_, const int& b_, const int& a_ = 255) : r(r_), g(g_), b(b_), a(a_) {};
	SColor(const float& r_, const float& g_, const float& b_, const float& a_ = 1.0f)
	{
		r = (unsigned char)(r_ * 255.f);
		g = (unsigned char)(g_ * 255.f);
		b = (unsigned char)(b_ * 255.f);
		a = (unsigned char)(a_ * 255.f);
	};

	struct { boost::uint8_t r,g,b,a; };
	boost::uint32_t i;
};

#endif // _COLOR_H
