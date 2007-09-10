#ifndef METALMAKER_H
#define METALMAKER_H


#include "Include.h"

class CMetalMaker {
	public:
		CR_DECLARE(CMetalMaker);
		CR_DECLARE_SUB(UnitInfo);
		CMetalMaker(IAICallback* aicb);
		~CMetalMaker();

		bool Add(int unit);
		bool Remove(int unit);
		bool AllAreOn();
		void Update(int);

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
