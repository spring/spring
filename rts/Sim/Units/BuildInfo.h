/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BUILD_INFO_H
#define BUILD_INFO_H

#include <string>

#include "System/float3.h"

struct Command;
struct UnitDef;

/**
 * Info about a build, which consists of additional properties
 * of a unit that only exist during construction.
 */
struct BuildInfo
{
	BuildInfo()
		: def(NULL)
		, pos(ZeroVector)
		, buildFacing(0) {}
	BuildInfo(const UnitDef* def, const float3& pos, int buildFacing)
		: def(def)
		, pos(pos)
		, buildFacing(buildFacing) {}
	BuildInfo(const Command& c) { Parse(c); }
	BuildInfo(const std::string& name, const float3& pos, int facing);

	int GetXSize() const;
	int GetZSize() const;
	bool Parse(const Command& c);
	void FillCmd(Command& c) const;

	const UnitDef* def;
	float3 pos;
	int buildFacing;
};

#endif /* BUILD_INFO_H */
