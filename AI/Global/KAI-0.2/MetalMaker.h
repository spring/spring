#ifndef METALMAKER_H
#define METALMAKER_H

#include "ExternalAI/IGroupAI.h"
#include <map>
#include <set>

struct AIClasses;

using namespace std;

class CMetalMaker
{
public:
	CR_DECLARE(CMetalMaker);
	CR_DECLARE_SUB(UnitInfo);
	CMetalMaker(AIClasses* ai);
	virtual ~CMetalMaker();
//	virtual void CMetalMaker::Init(IAICallback* aicb);
	virtual bool Add(int unit);
	virtual bool Remove(int unit);
	virtual bool AllAreOn();
	virtual void Update();
	struct UnitInfo{
		CR_DECLARE_STRUCT(UnitInfo);
		int id;
		float energyUse;
		float metalPerEnergy;
		bool turnedOn;
	};
	vector<UnitInfo> myUnits;
	float lastEnergy;
	AIClasses* ai;
	int listIndex;
	int addedDelay;

};


#endif /* METALMAKER_H */
