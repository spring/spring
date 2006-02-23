#ifndef ASSIGNER_H
#define ASSIGNER_H
#include <list>
//#include "IAICallback.h"
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"

class Assigner{
public:
	Assigner(){}
	virtual ~Assigner(){
		myUnits.clear();
	}
	void InitAI(Global* GLI);
	void UnitFinished(int unit);
	void Update();
	struct UnitInfo{
		float energyUse;
		bool turnedOn;
	};
	void UnitDestroyed(int unit);
	map<int,UnitInfo> myUnits;
	int lastUpdate;
	float lastEnergy;
private:
	Global* G;
};

#endif

