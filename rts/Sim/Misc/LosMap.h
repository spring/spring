/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* based on original los code in LosHandler.{cpp,h} and RadarHandler.{cpp,h} */

#ifndef LOSMAP_H
#define LOSMAP_H

#include <vector>
#include "Vec2.h"

/// map containing counts of how many units have Line Of Sight (LOS) to each square
class CLosMap
{
public:
	CLosMap() : size(0, 0) {}

	void SetSize(int2 size);
	void SetSize(int w, int h) { SetSize(int2(w, h)); }

	/// circular area, for airLosMap, circular radar maps, jammer maps, ...
	void AddMapArea(int2 pos, int radius, int amount);

	/// arbitrary area, for losMap, non-circular radar maps, ...
	void AddMapSquares(const std::vector<int>& squares, int amount);

	int operator[] (int square) const { return map[square]; }

	int At(int x, int y) const {
		x = std::max(0, std::min(size.x - 1, x));
		y = std::max(0, std::min(size.y - 1, y));
		return map[y * size.x + x];
	}

	// temp fix for CBaseGroundDrawer and AI interface, which need raw data
	unsigned short& front() { return map.front(); }

protected:

	int2 size;
	std::vector<unsigned short> map;
};


/// algorithm to calculate LOS squares using raycasting, taking terrain into account
class CLosAlgorithm
{
public:
	CLosAlgorithm(int2 size, float minMaxAng, float extraHeight, const float* heightmap)
	: size(size), minMaxAng(minMaxAng), extraHeight(extraHeight), heightmap(heightmap) {}

	void LosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares);

private:
	void UnsafeLosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares);
	void SafeLosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares);

	const int2 size;
	const float minMaxAng;
	const float extraHeight;
	const float* const heightmap;
};

#endif
