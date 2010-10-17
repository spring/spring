/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BuildInfo.h"

#include "UnitDef.h"
#include "UnitDefHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "System/myMath.h"
#include "System/float3.h"


BuildInfo::BuildInfo(const std::string& name, const float3& pos, int facing)
	: pos(pos)
{
	def = unitDefHandler->GetUnitDefByName(name);
	buildFacing = abs(facing) % 4;
}


void BuildInfo::FillCmd(Command& c) const
{
	c.id = -def->id;
	c.params.resize(4);
	c.params[0] = pos.x;
	c.params[1] = pos.y;
	c.params[2] = pos.z;
	c.params[3] = (float) buildFacing;
}


bool BuildInfo::Parse(const Command& c)
{
	if (c.params.size() >= 3) {
		pos = float3(c.params[0], c.params[1], c.params[2]);

		if (c.id < 0) {
			def = unitDefHandler->GetUnitDefByID(-c.id);
			buildFacing = 0;

			if (c.params.size() == 4) {
				buildFacing = int(abs(c.params[3])) % 4;
			}

			return true;
		}
	}
	return false;
}


int BuildInfo::GetXSize() const
{
	return ((buildFacing & 1) == 0) ? def->xsize : def->zsize;
}


int BuildInfo::GetZSize() const
{
	return ((buildFacing & 1) == 1) ? def->xsize : def->zsize;
}

