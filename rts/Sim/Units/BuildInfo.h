/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BUILD_INFO_H
#define BUILD_INFO_H

#include <string>

#include "System/float3.h"
#include "System/type2.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/CommandAI/Command.h"

struct UnitDef;

/**
 * Info about a build, which consists of additional properties
 * of a unit that only exist during construction.
 */
struct BuildInfo
{
	BuildInfo();
	BuildInfo(const UnitDef* def, const float3& pos, int buildFacing);
	BuildInfo(const std::string& name, const float3& pos, int facing);
	BuildInfo(const Command& c) { Parse(c); }

	int GetXSize() const;
	int GetZSize() const;

	bool Parse(const Command& c);
	bool FootPrintOverlap(const float3& fpMid, const float2& fpDims) const {
		const float xs = GetXSize() * SQUARE_SIZE * 0.5f;
		const float zs = GetZSize() * SQUARE_SIZE * 0.5f;

		const float2 bpMins = {pos.x - xs, pos.z - zs};
		const float2 bpMaxs = {pos.x + xs, pos.z + zs};

		const float2 fpMins = {fpMid.x - fpDims.x, fpMid.z - fpDims.y};
		const float2 fpMaxs = {fpMid.x + fpDims.x, fpMid.z + fpDims.y};

		// true for any of these implies no overlap
		return (!((fpMins.x > bpMaxs.x) || (fpMaxs.x < bpMins.x)  ||  (fpMins.y > bpMaxs.y) || (fpMaxs.y < bpMins.y)));
	}

	int CreateCommandID() const;
	void AddCommandParams(Command& cmd) const;
	inline Command CreateCommand(unsigned char options = 0) const
	{
		// this is inlined to prevent copying of cmd on return
		Command cmd(CreateCommandID(), options);
		AddCommandParams(cmd);
		return cmd;
	}

	const UnitDef* def;
	float3 pos;
	int buildFacing;
};

#endif /* BUILD_INFO_H */
