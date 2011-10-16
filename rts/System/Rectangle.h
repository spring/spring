/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECTANGLE_H
#define RECTANGLE_H

struct SRectangle {
	SRectangle(int x1_, int z1_, int x2_, int z2_)
		: x1(x1_)
		, z1(z1_)
		, x2(x2_)
		, z2(z2_)
	{}

	int GetWidth() const  { return x2 - x1; }
	int GetHeight() const { return z2 - z1; }

	bool operator< (const SRectangle& other) {
		if (x1 == other.x1) {
			return (z1 < other.z1);
		} else {
			return (x1 < other.x1);
		}
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

