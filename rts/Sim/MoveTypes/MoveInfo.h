#ifndef MOVEINFO_H
#define MOVEINFO_H

#include <vector>
#include <map>
#include <string>

class CMoveMath;

struct MoveData{
	CR_DECLARE_STRUCT(MoveData);
	enum MoveType {
		Ground_Move,
		Hover_Move,
		Ship_Move
	};
	MoveType moveType;
	int size;

	float depth;
	float maxSlope;
	float slopeMod;
	float depthMod;

	int pathType;
	CMoveMath* moveMath;
	float crushStrength;
	int moveFamily;				//0=tank,1=kbot,2=hover,3=ship

	std::string name;
};

class CMoveInfo
{
	CR_DECLARE(CMoveInfo);
public:
	CMoveInfo();
	~CMoveInfo();

	std::vector<MoveData*> moveData;
	std::map<std::string,int> name2moveData;
	MoveData* GetMoveDataFromName(const std::string& name, bool exactMatch = false);
	unsigned int moveInfoChecksum;

	float terrainType2MoveFamilySpeed[256][4];
};

extern CMoveInfo* moveinfo;

#endif /* MOVEINFO_H */
