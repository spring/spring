/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEINFO_H
#define MOVEINFO_H

#include <vector>
#include <map>
#include <string>
#include "creg/creg_cond.h"
#include "Sim/Misc/GlobalConstants.h"

class CMoveMath;
class CSolidObject;

struct MoveData {
	CR_DECLARE_STRUCT(MoveData);

	MoveData(const MoveData* udefMD) {
		maxAcceleration = udefMD? udefMD->maxAcceleration:         0.0f;
		maxBreaking     = udefMD? udefMD->maxAcceleration * -3.0f: 0.0f;
		maxSpeed        = udefMD? udefMD->maxSpeed / GAME_SPEED:   0.0f;
		maxTurnRate     = udefMD? (short int) udefMD->maxTurnRate: 0;

		size            = udefMD? udefMD->size:                    0;
		depth           = udefMD? udefMD->depth:                   0.0f;
		maxSlope        = udefMD? udefMD->maxSlope:                0.0f;
		slopeMod        = udefMD? udefMD->slopeMod:                0.0f;
		depthMod        = udefMD? udefMD->depthMod:                0.0f;
		pathType        = udefMD? udefMD->pathType:                0;
		moveMath        = udefMD? udefMD->moveMath:                0x0;
		crushStrength   = udefMD? udefMD->crushStrength:           0.0f;
		moveType        = udefMD? udefMD->moveType:                MoveData::Ground_Move;
		moveFamily      = udefMD? udefMD->moveFamily:              MoveData::Tank;
		terrainClass    = udefMD? udefMD->terrainClass:            MoveData::Mixed;
		followGround    = udefMD? udefMD->followGround:            true;
		subMarine       = udefMD? udefMD->subMarine:               false;
		name            = udefMD? udefMD->name:                    "tank";
		heatMapping     = udefMD? udefMD->heatMapping:             true;
		heatMod	        = udefMD? udefMD->heatMod:                 0.05f;
		heatProduced    = udefMD? udefMD->heatProduced:            30;

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

	/// NOTE: rename? (because of (AMoveType*) CUnit::moveType)
	MoveType moveType;
	MoveFamily moveFamily;
	TerrainClass terrainClass;
	/// do we stick to the ground when in water?
	bool followGround;

	/// of the footprint
	int size;
	/// minWaterDepth for ships, maxWaterDepth otherwise
	float depth;
	float maxSlope;
	float slopeMod;
	float depthMod;

	int pathType;
	CMoveMath* moveMath;
	float crushStrength;

	std::string name;


	// CMobility refugees
	float maxSpeed;
	short maxTurnRate;

	float maxAcceleration;
	float maxBreaking;

	/// are we supposed to be a purely sub-surface ship?
	bool subMarine;

	/// heatmap this unit
	bool heatMapping;
	/// heatmap path cost modifier
	float heatMod;
	/// heat produced by a path
	int heatProduced;

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
