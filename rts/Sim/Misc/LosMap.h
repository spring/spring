/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* based on original los code in LosHandler.{cpp,h} and RadarHandler.{cpp,h} */

#ifndef LOS_MAP_H
#define LOS_MAP_H

#include <vector>
#include "System/type2.h"
#include "System/SpringMath.h"


struct SLosInstance;


/// map containing counts of how many units have Line Of Sight (LOS) to each square
class CLosMap
{
public:
	void Init(const int2 size_, const int2 mapDims, const float* ctrHeightMap_, const float* mipHeightMap_, bool sendReadmapEvents_)
	{
		size = size_;
		LOS2HEIGHT = mapDims / size;

		losmap.clear();
		losmap.resize(size.x * size.y, 0);

		ctrHeightMap = ctrHeightMap_;
		mipHeightMap = mipHeightMap_;

		sendReadmapEvents = sendReadmapEvents_;
	}

	void Kill() {}

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
	const unsigned short& front() const { return (losmap.front()); }

private:
	void LosAdd(SLosInstance* instance) const;
	void UnsafeLosAdd(SLosInstance* instance) const;
	void SafeLosAdd(SLosInstance* instance) const;

	void AddSquaresToInstance(SLosInstance* li, const std::vector<char>& losRaySquares) const;

protected:
	int2 size;
	int2 LOS2HEIGHT;

	std::vector<unsigned short> losmap;

	const float* ctrHeightMap = nullptr;
	const float* mipHeightMap = nullptr;

	bool sendReadmapEvents = false;
};

#endif // LOS_MAP_H
