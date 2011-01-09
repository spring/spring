/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEINFO_H
#define MOVEINFO_H

#include <vector>
#include <map>
#include <string>
#include "System/creg/creg_cond.h"
#include "Sim/Misc/GlobalConstants.h"

class CMoveMath;
class CSolidObject;

struct MoveData {
	CR_DECLARE_STRUCT(MoveData);

	MoveData(const MoveData* unitDefMD) {
		name            = unitDefMD? unitDefMD->name:            "";

		xsize           = unitDefMD? unitDefMD->xsize:           0;
		zsize           = unitDefMD? unitDefMD->zsize:           0;
		depth           = unitDefMD? unitDefMD->depth:           0.0f;
		maxSlope        = unitDefMD? unitDefMD->maxSlope:        1.0f;
		slopeMod        = unitDefMD? unitDefMD->slopeMod:        0.0f;
		depthMod        = unitDefMD? unitDefMD->depthMod:        0.0f;
		pathType        = unitDefMD? unitDefMD->pathType:        0;
		crushStrength   = unitDefMD? unitDefMD->crushStrength:   0.0f;
		moveType        = unitDefMD? unitDefMD->moveType:        MoveData::Ground_Move;
		moveFamily      = unitDefMD? unitDefMD->moveFamily:      MoveData::Tank;
		terrainClass    = unitDefMD? unitDefMD->terrainClass:    MoveData::Mixed;
		followGround    = unitDefMD? unitDefMD->followGround:    true;
		subMarine       = unitDefMD? unitDefMD->subMarine:       false;
		heatMapping     = unitDefMD? unitDefMD->heatMapping:     true;
		heatMod         = unitDefMD? unitDefMD->heatMod:         0.05f;
		heatProduced    = unitDefMD? unitDefMD->heatProduced:    30;
		unitDefRefCount = unitDefMD? unitDefMD->unitDefRefCount: 0;
		moveMath        = unitDefMD? unitDefMD->moveMath:        NULL;

		tempOwner       = NULL;
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

	std::string name;

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
	float depthMod;

	int pathType;
	float crushStrength;


	/// are we supposed to be a purely sub-surface ship?
	bool subMarine;

	/// heatmap this unit
	bool heatMapping;
	/// heatmap path cost modifier
	float heatMod;
	/// heat produced by a path
	int heatProduced;

	/// number of UnitDef types that refer to this MoveData class
	unsigned int unitDefRefCount;

	CMoveMath* moveMath;
	CSolidObject* tempOwner;
};


class CMoveInfo
{
	CR_DECLARE(CMoveInfo);
public:
	CMoveInfo();
	~CMoveInfo();

	std::vector<MoveData*> moveData;
	std::map<std::string, int> name2moveData;

	MoveData* GetMoveDataFromName(const std::string& name);
	unsigned int moveInfoChecksum;

private:
	CMoveMath* groundMoveMath;
	CMoveMath* hoverMoveMath;
	CMoveMath* seaMoveMath;
};

extern CMoveInfo* moveinfo;

#endif // MOVEINFO_H
