#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H
// WorldObject.h: interface for the CWorldObject class.
//
//////////////////////////////////////////////////////////////////////

#include "System/Object.h"
#include "System/float3.h"

class CWorldObject : public CObject  
{
public:
	void SetRadius(float r);
	inline CWorldObject(const float3& pos) : pos(pos),useAirLos(false),alwaysVisible(false) {};
	virtual ~CWorldObject();
	virtual void DrawS3O(){};

	float3 pos;
	float radius;					//used for collisions
	float sqRadius;				

	float drawRadius;			//used to see if in los
	bool useAirLos;
	bool alwaysVisible;
};

#endif /* WORLDOBJECT_H */
