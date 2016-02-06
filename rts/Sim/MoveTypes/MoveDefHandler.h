/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEDEF_HANDLER_H
#define MOVEDEF_HANDLER_H

#include <vector>
#include <map>
#include <string>

#include "System/float3.h"
#include "System/creg/creg_cond.h"

class MoveDefHandler;
class CSolidObject;
class LuaTable;


struct MoveDef {
	CR_DECLARE_STRUCT(MoveDef)

	MoveDef();
	MoveDef(const LuaTable& moveDefTable, int moveDefID);

	bool TestMoveSquare(
		const CSolidObject* collider,
		const int xTestMoveSqr,
		const int zTestMoveSqr,
		const float3 testMoveDir,
		bool testTerrain = true,
		bool testObjects = true,
		bool centerOnly = false,
		float* minSpeedMod = NULL,
		int* maxBlockBit = NULL
	) const;
	bool TestMoveSquare(
		const CSolidObject* collider,
		const float3 testMovePos,
		const float3 testMoveDir,
		bool testTerrain = true,
		bool testObjects = true,
		bool centerOnly = false,
		float* minSpeedMod = NULL,
		int* maxBlockBit = NULL
	) const;
	float GetDepthMod(const float height) const;
	unsigned int GetCheckSum() const;

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

	SpeedModClass speedModClass;
	TerrainClass terrainClass;

	/// of the footprint
	int xsize, xsizeh;
	int zsize, zsizeh;

	/// minWaterDepth for ships, maxWaterDepth otherwise
	/// controls movement and (un-)loading constraints
	float depth;
	float depthModParams[DEPTHMOD_NUM_PARAMS];
	float maxSlope;
	float slopeMod;
	float crushStrength;

	// PF speedmod-multipliers for squares blocked by mobile units
	// (which can respectively be "idle" == non-moving and have no
	// orders, "busy" == non-moving but have orders, or "moving")
	// NOTE:
	//     includes one extra padding element to make the moveMath
	//     member start on an 8-byte boundary for 64-bit platforms
	float speedModMults[SPEEDMOD_MOBILE_NUM_MULTS + 1];

	unsigned int pathType;
	/// number of UnitDef types that refer to this MoveDef class
	unsigned int udRefCount;

	/// heatmap path-cost modifier
	float heatMod;
	float flowMod;

	/// heat produced by a path
	int heatProduced;

	/// do we stick to the ground when in water?
	bool followGround;
	/// are we supposed to be a purely sub-surface ship?
	bool subMarine;

	/// do we try to pathfind around squares blocked by mobile units?
	///
	/// this also serves as a padding byte for alignment so compiler
	/// does not insert it (GetCheckSum would need to skip such bytes
	/// otherwise, since they are never initialized)
	bool avoidMobilesOnPath;
	bool allowTerrainCollisions;

	/// do we leave heat and avoid any left by others?
	bool heatMapping;
	bool flowMapping;
};



class LuaParser;
class MoveDefHandler
{
	CR_DECLARE_STRUCT(MoveDefHandler)
public:
	MoveDefHandler(LuaParser* defsParser);

	MoveDef* GetMoveDefByPathType(unsigned int pathType) { return &moveDefs[pathType]; }
	MoveDef* GetMoveDefByName(const std::string& name);

	unsigned int GetNumMoveDefs() const { return moveDefs.size(); }
	unsigned int GetCheckSum() const { return checksum; }

	void TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2);
	float GetSpeedMod(const MoveDef& md, unsigned int x, unsigned int z) const;

private:
	std::vector<MoveDef> moveDefs;
	std::vector< std::vector<float> > defSpeedMods;
	std::map<std::string, int> moveDefNames;

	unsigned int checksum;
};

extern MoveDefHandler* moveDefHandler;

#endif // MOVEDEF_HANDLER_H

