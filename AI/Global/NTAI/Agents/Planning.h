#ifndef PLANNING_H
#define PLANNING_H
#include <list>
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"


class Planning{
public:
	Planning(){}
	virtual ~Planning(){}
	void InitAI(Global* GLI);
	bool feasable(string s, int builder);
	//int DrawLine(float3 LocA, float3 LocB, int LifeTime, float Width = 5.0f, bool Arrow = false);
	//int DrawLine(ARGB colours, float3 LocA, float3 LocB, int LifeTime, float Width = 5.0f, bool Arrow = false);
private:
	Global* G;
};
#endif
