#ifndef SCOUTER_H
#define SCOUTER_H
#include <list>
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"

// The scouter agent determines a patrol path that will
// take the unit around the entire map, thus showing
// some enemy units at some point for chaser to chase,
// and providing scouting and exploration. It takes these
// patrol paths and gives them to any scout unit that is
// built. Because of this, any aircraft that cannot build will
// be added to this, though this will change surely in 
// the future.
class Scouter{
public:
	Scouter(){}
	virtual ~Scouter(){}
	void InitAI(Global* GLI);
	void UnitFinished(int unit);
	void UnitMoveFailed(int unit);
	void UnitIdle(int unit);
	void UnitDestroyed(int unit);
	float3 GetMexScout(int i);
	float3 GetStartPos(int i);
	list<float3> start_pos;
private:
	Global* G;
	map<int, list<float3> > cp;
	list<float3> mexes;
	list<float3> sectors;
};

#endif
