#ifndef MOVEINFO_H
#define MOVEINFO_H

#include <vector>
#include <map>
#include <string>

class CSunParser;
class CMoveMath;

struct MoveData{
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
};

class CMoveInfo
{
public:
	CMoveInfo(void);
	~CMoveInfo(void);

	std::vector<MoveData*> moveData;
	std::map<std::string,int> name2moveData;
	MoveData* GetMoveDataFromName(std::string name);
	unsigned int moveInfoChecksum;

	float terrainType2MoveFamilySpeed[256][4];
protected:
	CSunParser* sunparser;
	bool ClassExists(int num);
};

extern CMoveInfo* moveinfo;

#endif /* MOVEINFO_H */
