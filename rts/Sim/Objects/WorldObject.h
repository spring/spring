/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WORLD_OBJECT_H
#define WORLD_OBJECT_H

#include "System/Object.h"
#include "System/float3.h"

struct S3DModel;


class CWorldObject: public CObject
{
public:
	CR_DECLARE(CWorldObject);

	CWorldObject()
		: id(0)
		, pos(ZeroVector)
		, radius(0.0f)
		, sqRadius(0.0f)
		, drawRadius(0.0f)
		, useAirLos(false)
		, alwaysVisible(false)
		, model(NULL)
	{}
	CWorldObject(const float3& pos)
		: id(0)
		, pos(pos)
		, radius(0.0f)
		, sqRadius(0.0f)
		, drawRadius(0.0f)
		, useAirLos(false)
		, alwaysVisible(false)
		, model(NULL)
	{}

	virtual ~CWorldObject();

	void SetRadius(float radius);

	int id;

	float3 pos;
	float radius;       ///< used for collisions
	float sqRadius;

	float drawRadius;   ///< used to see if in los
	/// if true, the object's visibility is checked against airLosMap[allyteam]
	bool useAirLos;
	bool alwaysVisible;

	S3DModel* model;
};

#endif /* WORLD_OBJECT_H */
