/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "Vec2.h"
#include "myMath.h"
#include "System/creg/creg_cond.h"

struct SRectangle {
	CR_DECLARE_STRUCT(SRectangle);

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

	bool operator< (const SRectangle& other) {
		if (x1 == other.x1) {
			return (z1 < other.z1);
		} else {
			return (x1 < other.x1);
		}
	}

	template<typename T>
	SRectangle operator* (const T v) const {
		return SRectangle(
			x1 * v, z1 * v,
			x2 * v, z2 * v
		);
	}

	int x1;
	union {
		int z1;
		int y1;
	};
	int x2;
	union {
		int z2;
		int y2;
	};
};

#endif // RECTANGLE_H

