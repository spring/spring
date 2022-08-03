/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEDEF_HANDLER_H
#define MOVEDEF_HANDLER_H

#include <array>
#include <string>

#include "System/float3.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"
#include "System/creg/creg_cond.h"

class MoveDefHandler;
class CSolidObject;
class LuaTable;


struct MoveDef {
	CR_DECLARE_STRUCT(MoveDef)

	MoveDef();
	MoveDef(const LuaTable& moveDefTable);
	MoveDef(const MoveDef& moveDef) = delete;
	MoveDef(MoveDef&& moveDef) = default;

	MoveDef& operator = (const MoveDef& moveDef) = delete;
	MoveDef& operator = (MoveDef&& moveDef) = default;

	bool TestMoveSquareRange(
		const CSolidObject* collider,
		const float3 rangeMins,
		const float3 rangeMaxs,
		const float3 testMoveDir,
		bool testTerrain = true,
		bool testObjects = true,
		bool centerOnly = false,
		float* minSpeedModPtr = nullptr,
		int* maxBlockBitPtr = nullptr
	) const;
	bool TestMoveSquare(
		const CSolidObject* collider,
		const float3 testMovePos,
		const float3 testMoveDir,
		bool testTerrain = true,
		bool testObjects = true,
		bool centerOnly = false,
		float* minSpeedModPtr = nullptr,
		int* maxBlockBitPtr = nullptr
	) const {
		return (TestMoveSquareRange(collider, testMovePos, testMovePos, testMoveDir, testTerrain, testObjects, centerOnly, minSpeedModPtr, maxBlockBitPtr));
	}

	// aircraft and buildings defer to UnitDef::floatOnWater
	bool FloatOnWater() const { return (speedModClass == MoveDef::Hover || speedModClass == MoveDef::Ship); }

	float2 GetFootPrint(float scale) const { return {xsize * scale, zsize * scale}; }

	float CalcFootPrintMinExteriorRadius(float scale = 1.0f) const; // radius minimally bounding the footprint
	float CalcFootPrintMaxInteriorRadius(float scale = 1.0f) const; // radius maximally bounded by the footprint
	float CalcFootPrintAxisStretchFactor() const; // 0 for square-shaped footprints, 1 for (impossible) line-shaped footprints

	float GetDepthMod(float height) const;

	unsigned int CalcCheckSum() const;

	static float GetDefaultMinWaterDepth() { return -1e6f; }
	static float GetDefaultMaxWaterDepth() { return +1e6f; }

	/// determines which of the {tank,kbot,hover,ship}Speed
	/// modifiers this MoveDef receives from a terrain-type
	enum SpeedModClass {
		Tank  = 0,
		KBot  = 1,
		Hover = 2,
		Ship  = 3
	};
	enum TerrainClass {
		Land  = 0, /// we are restricted to "land" (terrain with height >= 0)
		Water = 1, /// we are restricted to "water" (terrain with height < 0)
		Mixed = 2, /// we can exist at heights both greater and smaller than 0
	};
	enum DepthModParams {
		DEPTHMOD_MIN_HEIGHT = 0,
		DEPTHMOD_MAX_HEIGHT = 1,
		DEPTHMOD_MAX_SCALE  = 2,
		DEPTHMOD_QUA_COEFF  = 3,
		DEPTHMOD_LIN_COEFF  = 4,
		DEPTHMOD_CON_COEFF  = 5,
		DEPTHMOD_NUM_PARAMS = 6,
	};
	enum SpeedModMults {
		SPEEDMOD_MOBILE_IDLE_MULT = 0,
		SPEEDMOD_MOBILE_BUSY_MULT = 1,
		SPEEDMOD_MOBILE_MOVE_MULT = 2,
		SPEEDMOD_MOBILE_NUM_MULTS = 3,
	};

	std::string name;

#pragma pack(push, 1)
	SpeedModClass speedModClass = MoveDef::Tank;
	TerrainClass terrainClass = MoveDef::Mixed;

	unsigned int pathType = 0;

	/// of the footprint
	int xsize = 0, xsizeh = 0;
	int zsize = 0, zsizeh = 0;

	/// minWaterDepth for ships, maxWaterDepth otherwise
	/// controls movement and (un-)loading constraints
	float depth = 0.0f;
	float depthModParams[DEPTHMOD_NUM_PARAMS];
	float maxSlope = 1.0f;
	float slopeMod = 0.0f;
	float crushStrength = 0.0f;

	// PF speedmod-multipliers for squares blocked by mobile units
	// (which can respectively be "idle" == non-moving and have no
	// orders, "busy" == non-moving but have orders, or "moving")
	// NOTE:
	//     includes one extra padding element to make the moveMath
	//     member start on an 8-byte boundary for 64-bit platforms
	float speedModMults[SPEEDMOD_MOBILE_NUM_MULTS + 1];

	/// heatmap path-cost modifier
	float heatMod = 0.05f;
	float flowMod = 1.0f;

	/// heat produced by a path per tick
	int heatProduced = 30;

	/// do we stick to the ground when in water?
	bool followGround = true;
	/// are we supposed to be a purely sub-surface ship?
	bool isSubmarine = false;

	/// do we try to pathfind around squares blocked by mobile units?
	///
	/// this also serves as a padding byte for alignment so compiler
	/// does not insert it (GetCheckSum would need to skip such bytes
	/// otherwise, since they are never initialized)
	bool avoidMobilesOnPath = true;
	bool allowTerrainCollisions = true;
	bool allowRawMovement = false;

	/// do we leave heat and avoid any left by others?
	bool heatMapping = true;
	bool flowMapping = true;
#pragma pack(pop)
};



class LuaParser;
class MoveDefHandler
{
	CR_DECLARE_STRUCT(MoveDefHandler)
public:
	void Init(LuaParser* defsParser);
	void Kill() {
		nameMap.clear(); // never iterated

		mdCounter = 0;
		mdChecksum = 0;
	}

	MoveDef* GetMoveDefByPathType(unsigned int pathType) { return &moveDefs[pathType]; }
	MoveDef* GetMoveDefByName(const std::string& name);

	unsigned int GetNumMoveDefs() const { return mdCounter; }
	unsigned int GetCheckSum() const { return mdChecksum; }

private:
	std::array<MoveDef, 256> moveDefs;
	spring::unordered_map<unsigned int, int> nameMap;

	unsigned int mdCounter = 0;
	unsigned int mdChecksum = 0;
};

extern MoveDefHandler moveDefHandler;

#endif // MOVEDEF_HANDLER_H

