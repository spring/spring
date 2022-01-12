/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "type2.h"
#include "SpringMath.h"
#include "System/creg/creg_cond.h"

struct SRectangle {
	CR_DECLARE_STRUCT(SRectangle)

	SRectangle()
		: x1(0)
		, z1(0)
		, x2(0)
		, z2(0)
	{}
	SRectangle(int x1_, int z1_, int x2_, int z2_)
		: x1(x1_)
		, z1(z1_)
		, x2(x2_)
		, z2(z2_)
	{}

	int GetWidth() const { return x2 - x1; }
	int GetHeight() const { return z2 - z1; }
	int GetArea() const { return (GetWidth() * GetHeight()); }

	bool Inside(const int2 pos) const {
		// note: *min inclusive, *max exclusive
		const bool xb = (pos.x >= x1 && pos.x < x2);
		const bool yb = (pos.y >= y1 && pos.y < y2);
		return (xb && yb);
	}

	void ClampPos(int2* pos) const {
		pos->x = Clamp(pos->x, x1, x2);
		pos->y = Clamp(pos->y, y1, y2);
	}

	void ClampIn(const SRectangle& rect) {
		x1 = Clamp(x1, rect.x1, rect.x2);
		x2 = Clamp(x2, rect.x1, rect.x2);
		y1 = Clamp(y1, rect.y1, rect.y2);
		y2 = Clamp(y2, rect.y1, rect.y2);
	}

	bool CheckOverlap(const SRectangle& rect) const {
		return
			x1 < rect.x2 && x2 > rect.x1 &&
			y1 < rect.y2 && y2 > rect.y1;
	}

	bool operator< (const SRectangle& other) const {
		if (x1 == other.x1)
			return (z1 < other.z1);

		return (x1 < other.x1);
	}

	template<typename T>
	SRectangle operator* (const T v) const {
		return SRectangle(
			x1 * v, z1 * v,
			x2 * v, z2 * v
		);
	}

	union {
		int x1;
		int left;
	};
	union {
		int z1;
		int y1;
		int top;
	};
	union {
		int x2;
		int right;
	};
	union {
		int z2;
		int y2;
		int bottom;
	};
};


template<typename T> struct TRectangle {
	TRectangle() = default;
	TRectangle(T _x1, T _y1, T _x2, T _y2): x1(_x1), y1(_y1), x2(_x2), y2(_y2) {}

	TRectangle operator + (const TRectangle& o) const {
		TRectangle r;
		r.x1 = x1 + o.x1;
		r.x2 = x1 + o.x2;
		r.y1 = y1 + o.y1;
		r.y2 = y1 + o.y2;
		return r;
	}

	T x1 = T(0);
	T y1 = T(0);
	T x2 = T(0);
	T y2 = T(0);
};

#endif // RECTANGLE_H

