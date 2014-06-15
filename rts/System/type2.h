/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TYPE2_H
#define TYPE2_H

#include "lib/streflop/streflop_cond.h"
#include "System/creg/creg_cond.h"
#include "System/FastMath.h"

template<typename t> struct type2 {
	CR_DECLARE_STRUCT(type2);

	type2<t>(): x(t(0)), y(t(0)) {}
	type2<t>(const t nx, const t ny) : x(nx), y(ny) {}

	bool operator == (const type2<t>& v) const { return (x == v.x) && (y == v.y); }
	bool operator != (const type2<t>& v) const { return (x != v.x) || (y != v.y); }

	type2<t> operator + (const type2<t>& v) const { return (type2<t>(x + v.x, y + v.y)); }
	type2<t> operator - (const type2<t>& v) const { return (type2<t>(x - v.x, y - v.y)); }

	type2<t>& operator += (const type2<t>& v) { x += v.x; y += v.y; return *this; }
	type2<t>& operator -= (const type2<t>& v) { x -= v.x; y -= v.y; return *this; }

	t distance(const type2<t>& f) const {
		const t dx = x - f.x;
		const t dy = y - f.y;
		return t(math::sqrt(dx*dx + dy*dy));
	}

	t x;
	t y;
};

typedef type2<  int>   int2;
typedef type2<float> float2;

#endif
