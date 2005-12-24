#ifndef SCOUTER_H
#define SCOUTER_H
#include <list>
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"

// The scouter agent determines a patrol path that will
// take the unit around the entire map, thus showing
// some enemy units at some point for chaser to chase,
// and providing scouting and exploration. It takes these
// patrol paths and gives them to any scout unit that is
// built. Because of this, any aircraft that cannot build will
// be added to this, though this will change surely in 
// the future.
class Scouter : public base{
public:
	Scouter(){}
	virtual ~Scouter(){}
	void InitAI(Global* GLI);
	void UnitFinished(int unit);
	void Update();
	void UnitMoveFailed(int unit);
	void UnitIdle(int unit);
	void UnitDestroyed(int unit);
private:
	Global* G;
	map<int,vector<float3>::iterator> cp;
};

#endif
