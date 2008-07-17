#ifndef MOVEINFO_H
#define MOVEINFO_H

#include <vector>
#include <map>
#include <string>

class CMoveMath;

struct MoveData {
	CR_DECLARE_STRUCT(MoveData);

	MoveData(const MoveData* udefMD, int gs) {
		maxAcceleration = udefMD? udefMD->maxAcceleration:         0.0f;
		maxBreaking     = udefMD? udefMD->maxAcceleration * -3.0f: 0.0f;
		maxSpeed        = udefMD? udefMD->maxSpeed / gs:           0.0f;
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
		name            = udefMD? udefMD->name:                    "TANK";
	}

	enum MoveType {
		Ground_Move,
		Hover_Move,
		Ship_Move
	};
	enum MoveFamily {
		Tank,
		KBot,
		Hover,
		Ship
	};
	enum TerrainClass {
		Land,					// we are restricted to "land" (terrain with height >= 0)
		Water,					// we are restricted to "water" (terrain with height < 0)
		Mixed					// we can exist at heights both greater and smaller than 0
	};

	MoveType moveType;			// NOTE: rename? (because of (AMoveType*) CUnit::moveType)
	MoveFamily moveFamily;
	TerrainClass terrainClass;
	bool followGround;			// do we stick to the ground when in water?

	int size;					// of the footprint
	float depth;				// minWaterDepth for ships, maxWaterDepth otherwise
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

	bool subMarine;				// are we supposed to be a purely sub-surface ship?
};


class CMoveInfo
{
	CR_DECLARE(CMoveInfo);
public:
	CMoveInfo();
	~CMoveInfo();

	std::vector<MoveData*> moveData;
	std::map<std::string, int> name2moveData;
	MoveData* GetMoveDataFromName(const std::string& name, bool exactMatch = false);
	unsigned int moveInfoChecksum;

	float terrainType2MoveFamilySpeed[256][4];
};

extern CMoveInfo* moveinfo;

#endif /* MOVEINFO_H */
