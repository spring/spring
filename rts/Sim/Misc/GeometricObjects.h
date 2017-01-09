/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GEOMETRIC_OBJECTS_H
#define GEOMETRIC_OBJECTS_H

#include <vector>
#include "System/Misc/NonCopyable.h"
#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include "System/UnorderedMap.hpp"


class CGeoSquareProjectile;

class CGeometricObjects : public spring::noncopyable
{
	CR_DECLARE_STRUCT(CGeometricObjects)
	CR_DECLARE_SUB(GeoGroup)

private:
	struct GeoGroup {
		CR_DECLARE_STRUCT(GeoGroup)
		std::vector<CGeoSquareProjectile*> squares;
	};

public:
	CGeometricObjects() { firstFreeGroup = 1; }
	~CGeometricObjects();

	int AddSpline(float3 b1, float3 b2, float3 b3, float3 b4, float width, int arrow, int lifeTime = -1, int group = 0);
	void DeleteGroup(int group);
	void SetColor(int group, float r, float g, float b, float a);
	float3 CalcSpline(float pos, const float3& p1, const float3& p2, const float3& p3, const float3& p4);
	int AddLine(float3 start, float3 end, float width, int arrow, int lifetime = -1, int group = 0);
	void Update();
	void MarkSquare(int mapSquare);

private:
	spring::unordered_map<int, GeoGroup> geoGroups;
	spring::unordered_map<int, std::vector<int> > timedGroups;

	int firstFreeGroup;
};

extern CGeometricObjects* geometricObjects;

#endif /* GEOMETRIC_OBJECTS_H */
