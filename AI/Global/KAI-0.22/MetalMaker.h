#ifndef METALMAKER_H
#define METALMAKER_H

#include "ExternalAI/IGroupAI.h"
#include <map>
#include <set>

using namespace std;

class CMetalMaker
{
public:
	CMetalMaker(IAICallback* aicb);
	virtual ~CMetalMaker();
//	virtual void CMetalMaker::Init(IAICallback* aicb);
	virtual bool Add(int unit);
	virtual bool Remove(int unit);
	virtual bool AllAreOn();
	virtual void Update();
	struct UnitInfo{
		int id;
		float energyUse;
		float metalPerEnergy;
		bool turnedOn;
	};
	vector<UnitInfo> myUnits;
	float lastEnergy;
	IAICallback* aicb;
	int listIndex;
	int addedDelay;

};


#endif /* METALMAKER_H */
