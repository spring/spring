#ifndef PLANNING_H
#define PLANNING_H
#include <list>
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"


class Planning{
public:
	Planning(Global* GLI);
	virtual ~Planning(){}
	void InitAI();
	bool feasable(string s, int builder); // Antistall algorithm
	void Update();
	float GetRealValue(float value); // used to obtaint eh true value e..g a solar giving 20 energy actually gives 18.75
	float3 GetDirVector(int enemy,float3 unit, const WeaponDef* def); // Predictive Targetting
	//int DrawLine(float3 LocA, float3 LocB, int LifeTime, float Width = 5.0f, bool Arrow = false);
	//int DrawLine(ARGB colours, float3 LocA, float3 LocB, int LifeTime, float Width = 5.0f, bool Arrow = false);
	float a;
	float b;
	float c;
	float d;
	vector<string> NoAntiStall;
	vector<string> AlwaysAntiStall;
	int fnum;
private:
	Global* G;
	struct envec{
		float3 last;
		float3 now;
		int last_frame;
	};
	map<int,envec> enemy_vectors;
};
#endif
