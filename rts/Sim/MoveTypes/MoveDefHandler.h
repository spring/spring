/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEDEF_HANDLER_H
#define MOVEDEF_HANDLER_H

#include <vector>
#include <map>
#include <string>
#include "System/creg/creg_cond.h"
#include "Sim/Misc/GlobalConstants.h"

class MoveDefHandler;
class CSolidObject;
class LuaTable;

#pragma pack(push, 1)
struct MoveDef {
	CR_DECLARE_STRUCT(MoveDef);

	MoveDef();
	MoveDef(const LuaTable& moveTable, int moveDefID);

	bool TestMoveSquare(const int hmx, const int hmz) const;
	float GetDepthMod(const float height) const;
	unsigned int GetCheckSum() const;

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

	MoveFamily moveFamily;
	TerrainClass terrainClass;

	/// of the footprint
	int xsize, xsizeh;
	int zsize, zsizeh;

	/// minWaterDepth for ships, maxWaterDepth otherwise
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
	unsigned int unitDefRefCount;

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

	/// do we leave heat and avoid any left by others?
	bool heatMapping;
	bool flowMapping;
};
#pragma pack(pop)


class MoveDefHandler
{
	CR_DECLARE_STRUCT(MoveDefHandler);
public:
	MoveDefHandler();
	~MoveDefHandler();

	MoveDef* GetMoveDefByPathType(unsigned int pathType) { return moveDefs[pathType]; }
	MoveDef* GetMoveDefByName(const std::string& name);

	unsigned int GetNumMoveDefs() const { return moveDefs.size(); }
	unsigned int GetCheckSum() const { return checksum; }

private:
	std::vector<MoveDef*> moveDefs;
	std::map<std::string, int> moveDefNames;

	unsigned int checksum;
};

extern MoveDefHandler* moveDefHandler;

#endif // MOVEDEF_HANDLER_H

