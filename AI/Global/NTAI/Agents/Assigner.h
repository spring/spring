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
	void UnitDestroyed(int unit);
	struct UnitInfo{
		float energyUse;
		bool turnedOn;
		float efficiency;
		int unit;
		bool operator< (UnitInfo& u){
			return u.efficiency < efficiency;
		}
		bool operator>(UnitInfo& u){
			return u.efficiency > efficiency;
		}
	};
	struct CloakInfo{
		int uid;
		float cloakCost;
		float cloakCostMoving;
		float efficiency;
		bool cloaked;
		string name;
		bool operator< (CloakInfo& u){
			return u.efficiency < efficiency;
		}
		bool operator>(CloakInfo& u){
			return u.efficiency > efficiency;
		}
	};

	list<UnitInfo> myUnits;
	list<CloakInfo> CloakedUnits;
	float lastEnergy;
private:
	Global* G;
};

#endif

