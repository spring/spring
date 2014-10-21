/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MOVE_DATA_H
#define _MOVE_DATA_H

#include <vector>
#include <map>
#include <string>
#include <functional>

#include "System/creg/creg_cond.h"


namespace springLegacyAI {

// FIXME: this is a million years behind the engine version now
struct MoveData {
	CR_DECLARE_STRUCT(MoveData)

	MoveData()
		: moveType(MoveData::Ground_Move)
		, moveFamily(MoveData::Tank)
		, terrainClass(MoveData::Mixed)
		, followGround(true)
		, xsize(0)
		, zsize(0)
		, depth(0.0f)
		, maxSlope(0.0f)
		, slopeMod(0.0f)
		, pathType(0)
		, crushStrength(0.0f)
		, name("tank")
		, maxSpeed(0.0f)
		, maxTurnRate(0)
		, maxAcceleration(0.0f)
		, maxBreaking(0.0f)
		, subMarine(false)
		, heatMapping(true)
		, heatMod(0.05f)
		, heatProduced(30)
	{
	}

	enum MoveType {
		Ground_Move = 0,
		Hover_Move  = 1,
		Ship_Move   = 2
	};
	enum MoveFamily {
		Tank  = 0,
		KBot  = 1,
		Hover = 2,
		Ship  = 3
	};
	enum TerrainClass {
		/// we are restricted to "land" (terrain with height >= 0)
		Land = 0,
		/// we are restricted to "water" (terrain with height < 0)
		Water = 1,
		/// we can exist at heights both greater and smaller than 0
		Mixed = 2
	};

	/// NOTE: rename? (because of (AMoveType*) CUnit::moveType)
	MoveType moveType;
	MoveFamily moveFamily;
	TerrainClass terrainClass;
	/// do we stick to the ground when in water?
	bool followGround;

	/// of the footprint
	int xsize;
	int zsize;
	/// minWaterDepth for ships, maxWaterDepth otherwise
	float depth;
	float maxSlope;
	float slopeMod;
	std::function<float (float height)> GetDepthMod;

	int pathType;
	float crushStrength;

	std::string name;


	// CMobility refugees
	float maxSpeed;
	short maxTurnRate;

	float maxAcceleration;
	float maxBreaking;

	/// are we supposed to be a purely sub-surface ship?
	bool subMarine;

	/// heat-map this unit
	bool heatMapping;
	/// heat-map path cost modifier
	float heatMod;
	/// heat produced by a path
	int heatProduced;
};

} // namespace springLegacyAI

#endif // _MOVE_DATA_H
