/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* based on original los code in LosHandler.{cpp,h} and RadarHandler.{cpp,h} */

#ifndef LOS_MAP_H
#define LOS_MAP_H

#include <vector>
#include "System/Vec2.h"

/// map containing counts of how many units have Line Of Sight (LOS) to each square
class CLosMap
{
	CR_DECLARE_STRUCT(CLosMap);

public:
	CLosMap() : size(0, 0), sendReadmapEvents(false) {}

	void SetSize(int2 size, bool sendReadmapEvents);
	void SetSize(int w, int h, bool sendReadmapEvents) { SetSize(int2(w, h), sendReadmapEvents); }

	/// circular area, for airLosMap, circular radar maps, jammer maps, ...
	void AddMapArea(int2 pos, int allyteam, int radius, int amount);

	/// arbitrary area, for losMap, non-circular radar maps, ...
	void AddMapSquares(const std::vector<int>& squares, int allyteam, int amount);

	int operator[] (int square) const { return map[square]; }

	int At(int x, int y) const {
		x = std::max(0, std::min(size.x - 1, x));
		y = std::max(0, std::min(size.y - 1, y));
		return map[y * size.x + x];
	}

	// FIXME temp fix for CBaseGroundDrawer and AI interface, which need raw data
	unsigned short& front() { return map.front(); }

protected:
	int2 size;
	std::vector<unsigned short> map;
	bool sendReadmapEvents;
};


/// algorithm to calculate LOS squares using raycasting, taking terrain into account
class CLosAlgorithm
{
	CR_DECLARE_STRUCT(CLosAlgorithm);

public:
	CLosAlgorithm(int2 size, float minMaxAng, float extraHeight, const float* heightmap)
	: size(size), minMaxAng(minMaxAng), extraHeight(extraHeight), heightmap(heightmap) {}

	void LosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares);

private:
	void UnsafeLosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares);
	void SafeLosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares);

	int2 size;
	float minMaxAng;
	float extraHeight;
	const float* const heightmap;
};

#endif // LOS_MAP_H
