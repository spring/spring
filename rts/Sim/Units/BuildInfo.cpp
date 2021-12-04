/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "BuildInfo.h"

#include "UnitDef.h"
#include "UnitDefHandler.h"
#include "System/SpringMath.h"
#include "System/float3.h"


BuildInfo::BuildInfo()
	: def(nullptr)
	, pos(ZeroVector)
	, buildFacing(FACING_SOUTH)
{}

BuildInfo::BuildInfo(const UnitDef* def, const float3& pos, int facing)
	: def(def)
	, pos(pos)
	, buildFacing(std::abs(facing) % NUM_FACINGS)
{}

BuildInfo::BuildInfo(const std::string& name, const float3& pos, int facing)
	: def(unitDefHandler->GetUnitDefByName(name))
	, pos(pos)
	, buildFacing(std::abs(facing) % NUM_FACINGS)
{}


int BuildInfo::CreateCommandID() const
{
	return -def->id;
}

void BuildInfo::AddCommandParams(Command& cmd) const
{
	cmd.PushPos(pos);
	cmd.PushParam((float) buildFacing);
}


bool BuildInfo::Parse(const Command& c)
{
	if (c.GetNumParams() < 3)
		return false;

	// FIXME:
	//   this MUST come before the ID-check because
	//   CAI::GetOverlapQueued uses a BuildInfo (!!)
	//   structure to find out if some newly issued
	//   command overlaps existing ones --> any non-
	//   build orders would never have <pos> parsed
	pos = c.GetPos(0);

	if (c.GetID() >= 0)
		return false;

	// pos = c.GetPos(0);
	def = unitDefHandler->GetUnitDefByID(-c.GetID());
	buildFacing = FACING_SOUTH;

	if (c.GetNumParams() == 4)
		buildFacing = std::abs(int(c.GetParam(3))) % NUM_FACINGS;

	return true;
}


int BuildInfo::GetXSize() const
{
	return ((buildFacing & 1) == 0) ? def->xsize : def->zsize;
}

int BuildInfo::GetZSize() const
{
	return ((buildFacing & 1) == 1) ? def->xsize : def->zsize;
}
