/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECTANGLE_H
#define RECTANGLE_H


struct Rectangle {
	Rectangle(int x1_, int z1_, int x2_, int z2_)
		: x1(x1_)
		, z1(z1_)
		, x2(x2_)
		, z2(z2_)
	{}

	int GetWidth() const  { return x2 - x1; }
	int GetHeight() const { return z2 - z1; }

	bool operator< (const Rectangle& other) {
		if (x1 == other.x1) {
			return (z1 < other.z1);
		} else {
			return (x1 < other.x1);
		}
	}


	int x1, z1;
	int x2, z2;
};

#endif // RECTANGLE_H

