#ifndef METALMAKER_H
#define METALMAKER_H


#include <map>
#include <set>

#include "ExternalAI/IGroupAI.h"

using namespace std;

class CMetalMaker {
	public:
		CMetalMaker(IAICallback* aicb);
		virtual ~CMetalMaker();

		virtual bool Add(int unit);
		virtual bool Remove(int unit);
		virtual bool AllAreOn();
		virtual void Update();

		struct UnitInfo {
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


#endif
