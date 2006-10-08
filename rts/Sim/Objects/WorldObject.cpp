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

CR_REG_METADATA(CWorldObject, (
				CR_MEMBER(radius),
				CR_MEMBER_BEGINFLAG(CM_Config), // the projectile system needs to know that 'pos' is accessible by script
					CR_MEMBER(pos),
					CR_MEMBER(useAirLos),
					CR_MEMBER(alwaysVisible),
				CR_MEMBER_ENDFLAG(CM_Config),
				CR_MEMBER(sqRadius),
				CR_MEMBER(drawRadius))

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
