#ifndef METALMAKER_H
#define METALMAKER_H


#include <map>
#include <set>

#include "ExternalAI/IGroupAI.h"

using namespace std;

class CMetalMaker {
	public:
		CR_DECLARE(CMetalMaker);
		CR_DECLARE_SUB(UnitInfo);
		CMetalMaker(IAICallback* aicb);
		virtual ~CMetalMaker();

		virtual bool Add(int unit);
		virtual bool Remove(int unit);
		virtual bool AllAreOn();
		virtual void Update(int);

		struct UnitInfo {
			CR_DECLARE_STRUCT(UnitInfo);
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
