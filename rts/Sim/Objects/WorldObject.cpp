// WorldObject.cpp: implementation of the CWorldObject class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"
#include "WorldObject.h"

CR_BIND_DERIVED(CWorldObject, CObject, )
CR_REG_METADATA(CWorldObject, (
	CR_MEMBER(id),
	CR_MEMBER(radius),
	CR_MEMBER_BEGINFLAG(CM_Config), // the projectile system needs to know that 'pos' is accessible by script
		CR_MEMBER(pos),
		CR_MEMBER(useAirLos),
		CR_MEMBER(alwaysVisible),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(sqRadius),
	CR_MEMBER(drawRadius),
	CR_RESERVED(16))
);

CWorldObject::~CWorldObject()
{
}

void CWorldObject::DrawS3O()
{
	// implemented by CUnit, CFeature, CWeaponProjectile, CPieceProjectile
}

void CWorldObject::SetRadius(float r)
{
	radius = r;
	sqRadius = r * r;
	drawRadius = r;
}
