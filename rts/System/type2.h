/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TYPE2_H
#define TYPE2_H

#include "lib/streflop/streflop_cond.h"
#include "System/creg/creg_cond.h"
#include "System/FastMath.h"

template<typename t> struct type2 {
	CR_DECLARE_STRUCT(type2)

	constexpr type2<t>(): x(t(0)), y(t(0)) {}
	constexpr type2<t>(const t nx, const t ny) : x(nx), y(ny) {}
	template<typename T2> constexpr type2<t>(const T2 v) : x(v.x), y(v.y) {}

	bool operator == (const type2<t>& v) const { return (x == v.x) && (y == v.y); }
	bool operator != (const type2<t>& v) const { return (x != v.x) || (y != v.y); }
	bool operator  < (const type2<t>& f) const { return (y == f.y) ? (x < f.x) : (y < f.y); }

	type2<t> operator - () const { return (type2<t>(-x, -y)); }
	type2<t> operator + (const type2<t>& v) const { return (type2<t>(x + v.x, y + v.y)); }
	type2<t> operator - (const type2<t>& v) const { return (type2<t>(x - v.x, y - v.y)); }
	type2<t> operator / (const type2<t>& v) const { return (type2<t>(x / v.x, y / v.y)); }
	type2<t> operator / (const t& i) const        { return (type2<t>(x / i  , y / i  )); }
	type2<t> operator * (const type2<t>& v) const { return (type2<t>(x * v.x, y * v.y)); }
	type2<t> operator * (const t& i) const        { return (type2<t>(x * i  , y * i  )); }

	type2<t>& operator += (const t& i) { x += i; y += i; return *this; }
	type2<t>& operator += (const type2<t>& v) { x += v.x; y += v.y; return *this; }
	type2<t>& operator -= (const t& i) { x -= i; y -= i; return *this; }
	type2<t>& operator -= (const type2<t>& v) { x -= v.x; y -= v.y; return *this; }
	type2<t>& operator *= (const t& i) { x *= i; y *= i; return *this; }
	type2<t>& operator *= (const type2<t>& v) { x *= v.x; y *= v.y; return *this; }
	type2<t>& operator /= (const t& i) { x /= i; y /= i; return *this; }
	type2<t>& operator /= (const type2<t>& v) { x /= v.x; y /= v.y; return *this; }

	t distance(const type2<t>& f) const {
		const t dx = x - f.x;
		const t dy = y - f.y;
		return t(math::sqrt(dx*dx + dy*dy));
	}

	t x;
	t y;
};

template<typename t> struct itype2 : public type2<t> {
	CR_DECLARE_STRUCT(itype2)

	constexpr itype2<t>() {}
	constexpr itype2<t>(const t nx, const t ny) : type2<t>(nx, ny) {}
	constexpr itype2<t>(const type2<int>& v) : type2<t>(v.x, v.y) {}

	bool operator == (const type2<int>& v) const { return (type2<t>::x == v.x) && (type2<t>::y == v.y); }
	bool operator != (const type2<int>& v) const { return (type2<t>::x != v.x) || (type2<t>::y != v.y); }
	bool operator  < (const type2<int>& f) const { return (type2<t>::y == f.y) ? (type2<t>::x < f.x) : (type2<t>::y < f.y); }

	type2<int> operator + (const type2<int>& v) const { return (type2<int>(type2<t>::x + v.x, type2<t>::y + v.y)); }
	type2<int> operator - (const type2<int>& v) const { return (type2<int>(type2<t>::x - v.x, type2<t>::y - v.y)); }
	type2<int> operator / (const type2<int>& v) const { return (type2<int>(type2<t>::x / v.x, type2<t>::y / v.y)); }
	type2<int> operator / (const int& i) const        { return (type2<int>(type2<t>::x / i  , type2<t>::y / i  )); }
	type2<int> operator * (const type2<int>& v) const { return (type2<int>(type2<t>::x * v.x, type2<t>::y * v.y)); }
	type2<int> operator * (const int& i) const        { return (type2<int>(type2<t>::x * i  , type2<t>::y * i  )); }

	operator type2<int> () const { return (type2<int>(type2<t>::x, type2<t>::y)); }
};


typedef type2<  int>   int2;
typedef type2<float> float2;
typedef itype2<short> short2;
typedef itype2<unsigned short> ushort2;

#endif
