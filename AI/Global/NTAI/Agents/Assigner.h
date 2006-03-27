#ifndef ASSIGNER_H
#define ASSIGNER_H
#include <list>
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"

class Assigner{
public:
	Assigner(){
	G = 0;}
	virtual ~Assigner(){
		myUnits.erase(myUnits.begin(),myUnits.end());
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

