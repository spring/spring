/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "System/StdAfx.h"
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
	buildFacing = abs(facing) % NUM_FACINGS;
}


int BuildInfo::CreateCommandID() const
{
	return -def->id;
}

void BuildInfo::AddCommandParams(Command& cmd) const
{
	cmd.params.reserve(cmd.params.size() + 4);
	cmd.AddParam(pos.x);
	cmd.AddParam(pos.y);
	cmd.AddParam(pos.z);
	cmd.AddParam((float) buildFacing);
}


bool BuildInfo::Parse(const Command& c)
{
	if (c.params.size() >= 3) {
		pos = float3(c.params[0], c.params[1], c.params[2]);

		if (c.GetID() < 0) {
			def = unitDefHandler->GetUnitDefByID(-c.GetID());
			buildFacing = 0;

			if (c.params.size() == 4) {
				buildFacing = int(abs(c.params[3])) % NUM_FACINGS;
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

