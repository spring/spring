/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H

#include "Object.h"
#include "float3.h"

class CWorldObject: public CObject
{
public:
	CR_DECLARE(CWorldObject);

	CWorldObject():
		id(0), useAirLos(false), alwaysVisible(false) {}
	CWorldObject(const float3& pos):
		id(0), pos(pos), useAirLos(false), alwaysVisible(false) {}

	void SetRadius(float r);
	virtual ~CWorldObject();
	virtual void DrawS3O();

	int id;

	float3 pos;
	float radius;     ///< used for collisions
	float sqRadius;

	float drawRadius; ///< used to see if in los
	bool useAirLos;
	bool alwaysVisible;
};

#endif /* WORLDOBJECT_H */
