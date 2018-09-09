/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WorldObject.h"
#include "Rendering/Models/3DModel.h"

CR_BIND_DERIVED(CWorldObject, CObject, )
CR_REG_METADATA(CWorldObject, (
	CR_MEMBER(id),
	CR_MEMBER(tempNum),

	CR_MEMBER(radius),
	CR_MEMBER(height),
	CR_MEMBER(sqRadius),
	CR_MEMBER(drawRadius),
	// the projectile system needs to know that 'pos' and 'speed' are accessible by script
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(pos),
		CR_MEMBER(speed),
		CR_MEMBER(useAirLos),
		CR_MEMBER(alwaysVisible),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_IGNORED(model) //FIXME
))


void CWorldObject::SetRadiusAndHeight(const S3DModel* mdl)
{
	// initial values; can be overridden by LSC::Set*RadiusAndHeight
	SetRadiusAndHeight(mdl->radius, mdl->height);

	// model->radius defaults to this, but can be badly overridden
	// we always want the (more reliable) auto-calculated DR value
	drawRadius = mdl->CalcDrawRadius();
}

