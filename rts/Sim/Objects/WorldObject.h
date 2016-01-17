/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WORLD_OBJECT_H
#define WORLD_OBJECT_H

#include "System/Object.h"
#include "System/float4.h"

struct S3DModel;

class CWorldObject: public CObject
{
public:
	CR_DECLARE(CWorldObject)

	CWorldObject()
		: id(-1)
		, pos(ZeroVector)
		, speed(ZeroVector)
		, radius(0.0f)
		, height(0.0f)
		, sqRadius(0.0f)
		, drawRadius(0.0f)
		, useAirLos(false)
		, alwaysVisible(false)
		, model(NULL)
	{}
	CWorldObject(const float3& pos, const float3& spd): CWorldObject()
	{
		SetPosition(pos);
		SetVelocity(spd);
	}

	virtual ~CWorldObject() {}

	// NOTE: used only by projectiles, SolidObject's override this!
	virtual float GetDrawRadius() const { return drawRadius; }

	virtual void SetPosition(const float3& p) {   pos = p; }
	virtual void SetVelocity(const float3& v) { speed = v; }

	virtual void SetVelocityAndSpeed(const float3& v) {
		// set velocity first; do not assume f4::op=(f3) will not touch .w
		SetVelocity(v);
		SetSpeed(v);
	}

	// by default, SetVelocity does not set magnitude (for efficiency)
	// so SetSpeed must be explicitly called to update the w-component
	float SetSpeed(const float3& v) { return (speed.w = v.Length()); }

	void SetRadiusAndHeight(float r, float h) {
		radius = r;
		height = h;
		sqRadius = r * r;
		drawRadius = r;
	}

	void SetRadiusAndHeight(const S3DModel* model);

	// extrapolated base-positions; used in unsynced code
	float3 GetDrawPos(                float t) const { return (pos + speed * t); }
	float3 GetDrawPos(const float3 v, float t) const { return (pos +     v * t); }

public:
	int id;

	float3 pos;         ///< position of the very bottom of the object
	float4 speed;       ///< current velocity vector (elmos/frame), .w = |velocity|

	float radius;       ///< used for collisions
	float height;       ///< The height of this object
	float sqRadius;
	float drawRadius;   ///< unsynced, used for projectile visibility culling

	bool useAirLos;     ///< if true, the object's visibility is checked against airLosMap[allyteam]
	bool alwaysVisible; ///< if true, object is drawn even if not in LOS

	S3DModel* model;
};

#endif /* WORLD_OBJECT_H */
