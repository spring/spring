#ifndef MOVEINFO_H
#define MOVEINFO_H

#include <vector>
#include <map>
#include <string>

class CMoveMath;

struct MoveData {
	CR_DECLARE_STRUCT(MoveData);

	MoveData(float _maxAcc = 0.0f, float _maxBreaking = 0.0f,
			float _maxSpeed = 0.0f, short _maxTurnRate = 0,
			bool _canFly = false, bool _subMarine = false,
			const MoveData* udefMD = 0x0):
		maxSpeed(_maxSpeed), maxTurnRate(_maxTurnRate), maxAcceleration(_maxAcc),
		maxBreaking(_maxBreaking), canFly(_canFly), subMarine(_subMarine)
	{
		// udefMD is null when reading the MoveDefs
		moveType      = udefMD? udefMD->moveType:      MoveData::Ground_Move;
		size          = udefMD? udefMD->size:          0;
		depth         = udefMD? udefMD->depth:         0.0f;
		maxSlope      = udefMD? udefMD->maxSlope:      0.0f;
		slopeMod      = udefMD? udefMD->slopeMod:      0.0f;
		depthMod      = udefMD? udefMD->depthMod:      0.0f;
		pathType      = udefMD? udefMD->pathType:      0;
		moveMath      = udefMD? udefMD->moveMath:      0x0;
		crushStrength = udefMD? udefMD->crushStrength: 0.0f;
		moveFamily    = udefMD? udefMD->moveFamily:    MoveData::Tank;
		name          = udefMD? udefMD->name:          "";
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

	MoveType moveType;			// NOTE: rename? (because of (AMoveType*) CUnit::moveType)
	MoveFamily moveFamily;

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

	bool canFly;
	bool subMarine;				// always false
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
