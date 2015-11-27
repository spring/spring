/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* based on original los code in LosHandler.{cpp,h} and RadarHandler.{cpp,h} */

#ifndef LOS_MAP_H
#define LOS_MAP_H

#include <vector>
#include "System/type2.h"
#include "System/myMath.h"


struct SLosInstance;


/// map containing counts of how many units have Line Of Sight (LOS) to each square
class CLosMap
{
public:
	CLosMap(int2 size_, bool sendReadmapEvents_, const float* heightmap_, const int2 mapDims)
	: size(size_)
	, LOS2HEIGHT(mapDims / size)
	, losmap(size.x * size.y, 0)
	, sendReadmapEvents(sendReadmapEvents_)
	, heightmap(heightmap_)
	{ }

public:
	/// circular area, for airLosMap, circular radar maps, jammer maps, ...
	void AddCircle(SLosInstance* instance, int amount);

	/// arbitrary area, for losMap, non-circular radar maps, ...
	void AddRaycast(SLosInstance* instance, int amount);

	/// arbitrary area, for losMap, non-circular radar maps, ...
	void PrepareRaycast(SLosInstance* instance) const;

public:
	int At(int2 p) const {
		p.x = Clamp(p.x, 0, size.x - 1);
		p.y = Clamp(p.y, 0, size.y - 1);
		return losmap[p.y * size.x + p.x];
	}

	// FIXME temp fix for CBaseGroundDrawer and AI interface, which need raw data
	unsigned short& front() { return losmap.front(); }

private:
	void LosAdd(SLosInstance* instance) const;
	void UnsafeLosAdd(SLosInstance* instance) const;
	void SafeLosAdd(SLosInstance* instance) const;

	inline void CastLos(float* maxAng, const int2 off, std::vector<bool>& squaresMap, std::vector<float>& anglesMap, const int radius) const;
	void AddSquaresToInstance(SLosInstance* li, const std::vector<bool>& squaresMap) const;

protected:
	const int2 size;
	const int2 LOS2HEIGHT;
	std::vector<unsigned short> losmap;
	bool sendReadmapEvents;
	const float* const heightmap;
};

#endif // LOS_MAP_H
