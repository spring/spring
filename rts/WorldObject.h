#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H
// WorldObject.h: interface for the CWorldObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WORLDOBJECT_H__DEAEAA2F_0C26_42AE_9298_712B63F8E888__INCLUDED_)
#define AFX_WORLDOBJECT_H__DEAEAA2F_0C26_42AE_9298_712B63F8E888__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#include "Object.h"
#include "float3.h"

class CWorldObject : public CObject  
{
public:
	void SetRadius(float r);
	inline CWorldObject(const float3& pos) : pos(pos),useAirLos(false),alwaysVisible(false) {};
	virtual ~CWorldObject();
	float3 pos;
	float radius;					//used for collisions
	float sqRadius;				
	float drawRadius;			//used to see if in los
	bool useAirLos;
	bool alwaysVisible;
};

#endif // !defined(AFX_WORLDOBJECT_H__DEAEAA2F_0C26_42AE_9298_712B63F8E888__INCLUDED_)


#endif /* WORLDOBJECT_H */
