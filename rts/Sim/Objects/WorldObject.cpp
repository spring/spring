// WorldObject.cpp: implementation of the CWorldObject class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "WorldObject.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CR_BIND_DERIVED(CWorldObject, CObject)

CR_BIND_MEMBERS(CWorldObject, (
				CR_MEMBER(radius),
				CR_MEMBER(pos),
				CR_MEMBER(sqRadius),
				CR_MEMBER(drawRadius),
				CR_MEMBER(useAirLos),
				CR_MEMBER(alwaysVisible))
			);

CWorldObject::~CWorldObject()
{

}

void CWorldObject::SetRadius(float r)
{
	radius=r;
	sqRadius=r*r;
	drawRadius=r;
}
