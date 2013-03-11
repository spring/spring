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
		: id(-1)
		, pos(ZeroVector)
		, radius(0.0f)
		, height(0.0f)
		, sqRadius(0.0f)
		, drawRadius(0.0f)
		, useAirLos(false)
		, alwaysVisible(false)
		, model(NULL)
	{}
	CWorldObject(const float3& pos) {
		*this = CWorldObject();
		this->pos = pos;
	}

	virtual ~CWorldObject() {}

	void SetPos(const float3& p) { pos = p; }
	void SetRadiusAndHeight(float r, float h)
	{
		radius = r;
		height = h;
		sqRadius = r * r;
		drawRadius = r;
	}

	void SetRadiusAndHeight(S3DModel* model);

public:
	int id;

	float3 pos;         ///< position of the very bottom of the object

	float radius;       ///< used for collisions
	float height;       ///< The height of this object
	float sqRadius;
	float drawRadius;   ///< unsynced, used to see if in view etc.

	bool useAirLos;     ///< if true, the object's visibility is checked against airLosMap[allyteam]
	bool alwaysVisible; ///< if true, object is drawn even if not in LOS

	S3DModel* model;
};

#endif /* WORLD_OBJECT_H */
