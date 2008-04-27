#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H
// WorldObject.h: interface for the CWorldObject class.
//
//////////////////////////////////////////////////////////////////////

#include "Object.h"
#include "float3.h"

class CWorldObject : public CObject  
{
public:
	CR_DECLARE(CWorldObject);

	CWorldObject() :
		useAirLos(false), alwaysVisible(false) {}
	CWorldObject(const float3& pos) :
		pos(pos), useAirLos(false), alwaysVisible(false) {}

	void SetRadius(float r);
	virtual ~CWorldObject();

	float3 pos;
	float radius;     //used for collisions
	float sqRadius;

	float drawRadius; //used to see if in los
	bool useAirLos;
	bool alwaysVisible;
};

#endif /* WORLDOBJECT_H */
